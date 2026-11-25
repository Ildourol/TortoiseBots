#include "playerbot/playerbot.h"
#include "RepairAllAction.h"
#include "ServiceNpcsInReach.h"

#include "playerbot/ServerFacade.h"
#include "playerbot/policy/gear/RepairRefusalPolicy.h"

using namespace ai;

namespace
{
    // Durability points missing from everything DurabilityRepairAll fixes, so a
    // repair that only touched a spare weapon in a bag still counts as done.
    uint64 MissingDurability(Player* bot)
    {
        uint64 missing = 0;

        auto countItem = [&missing](Item* item)
        {
            if (!item)
                return;

            uint32 const maxDurability = item->GetUInt32Value(ITEM_FIELD_MAXDURABILITY);
            uint32 const durability = item->GetUInt32Value(ITEM_FIELD_DURABILITY);
            if (ItemNeedsRepair(maxDurability, durability))
                missing += maxDurability - durability;
        };

        for (int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i)
            countItem(bot->GetItemByPos(uint16((INVENTORY_SLOT_BAG_0 << 8) | i)));

        for (int bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
            for (int slot = 0; slot < MAX_BAG_SIZE; ++slot)
                countItem(bot->GetItemByPos(uint16((bag << 8) | slot)));

        return missing;
    }
}

std::string RepairAllAction::RefusalReason(PlayerbotAI* ai)
{
    Player* bot = ai->GetBot();
    bool const anythingToRepair = MissingDurability(bot) > 0;

    bool repairNpcInReach = false;
    if (anythingToRepair)
    {
        for (Creature* candidate : ServiceCreaturesInReach(bot))
        {
            if (bot->GetNPCIfCanInteractWith(candidate->GetObjectGuid(), UNIT_NPC_FLAG_REPAIR))
            {
                repairNpcInReach = true;
                break;
            }
        }
    }

    float yards = 0.0f;
    Creature* nearest = anythingToRepair && !repairNpcInReach
        ? NearestNpcWithFlagInSight(bot, UNIT_NPC_FLAG_REPAIR, yards) : nullptr;
    // A repair npc close enough that only the usability check refused it (in
    // combat, invisible, not selectable) is a different answer than "walk
    // closer": say the npc is here but cannot be used.
    if (nearest && yards <= INTERACTION_DISTANCE)
        repairNpcInReach = true;

    return RepairRefusalReason(anythingToRepair, repairNpcInReach,
        nearest ? int(yards + 0.5f) : -1);
}

bool RepairAllAction::Execute(Event& event)
{
    Player* requester = event.GetOwner() ? event.GetOwner() : GetMaster();

    // Interaction distance is the whole reach rule. The creatures at hand come
    // from the map rather than the cached, line-of-sight-filtered "nearest
    // npcs" value, so a summoned field repair bot (a temporary summon or a
    // player's companion) is found like any other repair npc.
    for (Creature* candidate : ServiceCreaturesInReach(bot))
    {
        Creature *unit = bot->GetNPCIfCanInteractWith(candidate->GetObjectGuid(), UNIT_NPC_FLAG_REPAIR);
        if (!unit)
            continue;

#ifdef MANGOS
        if(bot->HasUnitState(UNIT_STAT_DIED))
#endif
#ifdef CMANGOS
        if (bot->HasUnitState(UNIT_STAT_FEIGN_DEATH))
#endif
            bot->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

        sServerFacade.SetFacingTo(bot, unit);
        float discountMod = bot->GetReputationPriceDiscount(unit);

        float durability = AI_VALUE(uint8, "durability inventory");
        uint64 const missingBefore = MissingDurability(bot);

        uint32 botMoney = bot->GetMoney();
        if (ai->HasCheat(BotCheatMask::gold))
        {
            bot->SetMoney(10000000);
        }

        //Repair weapons first.
        uint32 totalCost = bot->DurabilityRepair((INVENTORY_SLOT_BAG_0 << 8) | EQUIPMENT_SLOT_MAINHAND, true, discountMod
        );

        totalCost += bot->DurabilityRepair((INVENTORY_SLOT_BAG_0 << 8) | EQUIPMENT_SLOT_RANGED, true, discountMod
        );

        totalCost += bot->DurabilityRepair((INVENTORY_SLOT_BAG_0 << 8) | EQUIPMENT_SLOT_OFFHAND, true, discountMod
        );

        totalCost += bot->DurabilityRepairAll(true, discountMod
        );

        if (ai->HasCheat(BotCheatMask::gold))
        {
            bot->SetMoney(botMoney);
        }

        //Totalcost is bugged in core. For now we use this work-around.
        totalCost = botMoney - bot->GetMoney();

        // A cheated purse is put back before this point, so the money spent
        // cannot tell a free repair from no repair: the durability drop does.
        bool const repaired = totalCost > 0 || MissingDurability(bot) < missingBefore;

        if (totalCost > 0)
        {
            std::ostringstream out;
            out << "Repair: " << chat->formatMoney(totalCost) << " (" << unit->GetName() << ")";
            ai->TellPlayerNoFacing(requester, out.str(),PlayerbotSecurityLevel::PLAYERBOT_SECURITY_ALLOW_ALL, false);
            if (sPlayerbotAIConfig.globalSoundEffects)
                bot->PlayDistanceSound(7994);

            sPlayerbotAIConfig.logEvent(ai, "RepairAllAction", std::to_string(durability), std::to_string(totalCost));
        }

        if (repaired)
            ai->DoSpecificAction("equip upgrades", event, true);

        SET_AI_VALUE(uint32, "death count", 0);
        RESET_AI_VALUE(uint8, "durability inventory");

        return repaired;
    }

    ai->TellPlayerNoFacing(requester, "Cannot repair: " + RefusalReason(ai));
    return false;
}
