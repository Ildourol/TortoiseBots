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

#include <set>
#include <utility>
#include <vector>

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

struct ThrowawayScope
{
    uint32_t guidLow;
    bool destroyAll;
    std::set<uint32_t> ownedBeforeScope;
};

// Gear paths run on the world thread and scopes never outlive the call.
std::vector<ThrowawayScope> g_throwawayScopes;

ThrowawayScope* FindThrowawayScope(uint32_t guidLow)
{
    for (auto it = g_throwawayScopes.rbegin(); it != g_throwawayScopes.rend(); ++it)
        if (it->guidLow == guidLow)
            return &*it;
    return nullptr;
}

void CollectOwnedItemGuids(Player* bot, std::set<uint32_t>& out)
{
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
        if (Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            out.insert(item->GetGUIDLow());
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
        if (Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            out.insert(item->GetGUIDLow());
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        Bag* container = static_cast<Bag*>(bot->GetItemByPos(INVENTORY_SLOT_BAG_0, bag));
        if (!container)
            continue;
        for (uint32 slot = 0; slot < container->GetBagSize(); ++slot)
            if (Item* item = container->GetItemByPos(static_cast<uint8>(slot)))
                out.insert(item->GetGUIDLow());
    }
}
} // namespace

ThrowawayGearScope::ThrowawayGearScope(Player* bot, bool destroyAll)
    : m_guidLow(bot ? bot->GetGUIDLow() : 0)
{
    if (!bot)
        return;
    ThrowawayScope scope;
    scope.guidLow = m_guidLow;
    scope.destroyAll = destroyAll;
    if (!destroyAll)
        CollectOwnedItemGuids(bot, scope.ownedBeforeScope);
    g_throwawayScopes.push_back(std::move(scope));
}

ThrowawayGearScope::~ThrowawayGearScope()
{
    for (size_t i = g_throwawayScopes.size(); i-- > 0;)
        if (g_throwawayScopes[i].guidLow == m_guidLow)
        {
            g_throwawayScopes.erase(g_throwawayScopes.begin() + i);
            return;
        }
}

ReplacedGearOutcome ReleaseReplacedItem(Player* bot, Item* item)
{
    if (!bot || !item)
        return ReplacedGearOutcome::Destroyed;

    ObjectGuid const botGuid = bot->getObjectGuid();
    ThrowawayScope* scope = FindThrowawayScope(bot->GetGUIDLow());
    if (scope && DiscardsReplacedGear(true, scope->destroyAll,
            scope->ownedBeforeScope.count(item->GetGUIDLow()) != 0))
    {
        bot->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
        return ReplacedGearOutcome::Destroyed;
    }

    bool const isBot = BotManager::Instance().IsBot(botGuid);
    uint32 const accountId = bot->GetSession() ? bot->GetSession()->GetAccountId() : 0;
    // An unset account or an unvalidated registry says nothing about
    // ownership: count it as pool, the same fail-closed rule the per-tick
    // classification uses.
    bool const isPoolAccount = accountId == 0 ||
        AccountIsPool(RandomBotAccountRegistry::Instance().IsValidated(),
            RandomBotAccountRegistry::Instance().IsRegistered(accountId));
    // Matches PlayerBotClassification: record.random is not an ownership
    // signal, the account is.
    bool const isPoolBot = isBot && isPoolAccount;

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
    // No explicit expire_delay: the core's default is 30 days (Mail.cpp).
    draft.SendMailTo(MailReceiver(bot), MailSender(bot, MAIL_STATIONERY_GM),
        MAIL_CHECK_MASK_COPIED);
    return ReplacedGearOutcome::Mailed;
}

} // namespace TortoiseBots
