#pragma once

#include "playerbot/playerbot.h"
#include "playerbot/policy/loot/LootViewRulePolicy.h"

namespace ai
{
    // One rule for "this corpse still has body loot for this bot": the raw lootable flag
    // or the viewer's own loot right. The core masks the flag per viewer, so a corpse
    // holding only this bot's quest item can lack it while IsAllowedToLoot still says yes.
    inline bool CorpseHasBodyLootFor(Player* bot, Creature* creature)
    {
        return creature->HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE) || bot->IsAllowedToLoot(creature);
    }

    // True when a group-loot roll item won by this viewer could not fit in
    // their bags and is still sitting on the corpse or chest reserved for them.
    inline bool HasWonLootFor(Loot const& loot, Player const* viewer)
    {
        if (!viewer)
            return false;
        ObjectGuid const me = viewer->GetObjectGuid();
        for (LootItem const& item : loot.items)
            if (!item.is_looted && !item.is_blocked && item.lootOwner == me)
                return true;
        return false;
    }

    // Loot::hasItemFor, restricted to the copies the core will actually show this viewer.
    inline bool HasVisiblePersonalItemFor(Loot const& loot, Player* viewer)
    {
        uint32 const me = viewer->GetGUIDLow();
        bool const turn = loot.roundRobinPlayer != 0;
        bool const mine = loot.roundRobinPlayer == viewer->GetGUID();
        uint32 const holder = ObjectGuid(loot.roundRobinPlayer).GetCounter();
        QuestItemMap const& q = loot.GetPlayerQuestItems();
        QuestItemMap const& f = loot.GetPlayerFFAItems();
        QuestItemMap const& c = loot.GetPlayerNonQuestNonFFAConditionalItems();
        auto has = [](QuestItemMap const& m, uint32 k) { auto i = m.find(k); return i != m.end() && i->second; };
        if (has(q, me))
            for (QuestItem const& qi : *q.find(me)->second)
            {
                LootItem const& it = loot.m_questItems[qi.index];
                if (!qi.is_looted && !it.is_looted && PersonalCopyVisible(it.freeforall, turn, mine, has(q, holder)))
                    return true;
            }
        if (has(f, me))
            for (QuestItem const& fi : *f.find(me)->second)
                if (!fi.is_looted && !loot.items[fi.index].is_looted)
                    return true;                               // FFA: always shown (LootMgr.cpp:1029-1043)
        if (has(c, me))
            for (QuestItem const& ci : *c.find(me)->second)
            {
                LootItem const& it = loot.items[ci.index];
                if (!ci.is_looted && !it.is_looted && PersonalCopyVisible(it.freeforall, turn, mine, has(c, holder)))
                    return true;
            }
        return false;
    }
}
