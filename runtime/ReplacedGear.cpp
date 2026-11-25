#include "ReplacedGear.h"

#include "BotManager.h"
#include "ClaimLifecycle.h"
#include "HireLifecycle.h"
#include "PlayerBotClassification.h"
#include "RandomBotAccountRegistry.h"

#include "../ai/playerbot/GroupMembers.h"

#include "ObjectAccessor.h"
#include "Player.h"
#include "WorldSession.h"
#include "Database/DatabaseEnv.h"
#include "Mail/Mail.h"

namespace TortoiseBots
{

namespace
{
bool HasLiveRealPlayerMaster(ObjectGuid botGuid)
{
    BotRecord* record = BotManager::Instance().FindBot(botGuid);
    if (!record || record->masterGuid.IsEmpty())
        return false;
    Player* master = sObjectAccessor.FindPlayer(record->masterGuid);
    return master && master->GetSession() && master->GetSession()->HasNetworkTransport();
}

bool GroupHasRealPlayer(Player* bot)
{
    Group* group = bot->GetGroup();
    if (!group)
        return false;
    for (Player* member : LiveGroupMembers(group))
        if (member && member != bot && member->GetSession() &&
            member->GetSession()->HasNetworkTransport())
            return true;
    return false;
}
} // namespace

ReplacedGearOutcome ReleaseReplacedItem(Player* bot, Item* item)
{
    if (!bot || !item)
        return ReplacedGearOutcome::Destroyed;

    ObjectGuid const botGuid = bot->getObjectGuid();
    bool const isBot = BotManager::Instance().IsBot(botGuid);
    uint32 const accountId = bot->GetSession() ? bot->GetSession()->GetAccountId() : 0;
    // An unset account or an unvalidated registry says nothing about
    // ownership: count it as pool, the same fail-closed rule the per-tick
    // classification uses.
    bool const isPoolAccount = accountId == 0 ||
        AccountIsPool(RandomBotAccountRegistry::Instance().IsValidated(),
            RandomBotAccountRegistry::Instance().IsRegistered(accountId));
    bool const isPoolBot = isBot && isPoolAccount &&
        BotManager::Instance().IsRandomBot(botGuid);

    if (!KeepsReplacedGear(isBot, isPoolBot, HasLiveRealPlayerMaster(botGuid),
            HireLifecycle::Instance().IsHired(botGuid),
            ClaimLifecycle::Instance().IsClaimed(bot->GetGUIDLow()),
            GroupHasRealPlayer(bot)))
    {
        bot->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
        return ReplacedGearOutcome::Destroyed;
    }

    uint8 const bag = item->GetBagSlot();
    uint8 const slot = item->GetSlot();
    ItemPosCountVec dest;
    if (bot->CanStoreItem(NULL_BAG, NULL_SLOT, dest, item, false) == EQUIP_ERR_OK)
    {
        // StoreItem (not MoveItemToInventory): the piece already has a
        // character_inventory row, so the save must run an UPDATE
        // (ITEM_CHANGED), not an INSERT (ITEM_NEW) - same pattern as the
        // core's AutoUnequipItemFromSlot.
        bot->RemoveItem(bag, slot, true);
        bot->StoreItem(dest, item, true);
        return ReplacedGearOutcome::Stored;
    }

    bot->MoveItemFromInventory(bag, slot, true);
    CharacterDatabase.BeginTransaction(bot->GetGUIDLow());
    item->DeleteFromInventoryDB();
    item->SaveToDB();
    CharacterDatabase.CommitTransaction();
    MailDraft draft("Replaced equipment", "An item no longer worn is attached.");
    draft.AddItem(item);
    // expire_delay UINT32_MAX: the core's mail row keeps the items ~136 years,
    // i.e. no expiry (0 would give the default 30 days).
    draft.SendMailTo(MailReceiver(bot), MailSender(bot, MAIL_STATIONERY_GM),
        MAIL_CHECK_MASK_COPIED, 0, 0xFFFFFFFF);
    return ReplacedGearOutcome::Mailed;
}

} // namespace TortoiseBots
