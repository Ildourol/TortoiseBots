#include "playerbot/playerbot.h"
#include "ServiceNpcsInReach.h"

#include "playerbot/PlayerbotAIConfig.h"
#include "Maps/GridNotifiers.h"
#include "Maps/GridNotifiersImpl.h"
#include "Maps/CellImpl.h"

using namespace ai;

std::list<Creature*> ai::ServiceCreaturesInReach(Player* bot)
{
    std::list<Creature*> result;
    if (!bot || !bot->IsInWorld())
        return result;

    std::list<Unit*> units;
    MaNGOS::AnyUnitInObjectRangeCheck check(bot, INTERACTION_DISTANCE);
    MaNGOS::UnitListSearcher<MaNGOS::AnyUnitInObjectRangeCheck> searcher(units, check);
    Cell::VisitAllObjects(bot, searcher, INTERACTION_DISTANCE);

    for (Unit* unit : units)
        if (unit->IsCreature())
            result.push_back(unit->ToCreature());
    return result;
}

Creature* ai::NearestNpcWithFlagInSight(Player* bot, unsigned npcFlags, float& yards)
{
    yards = 0.0f;
    if (!bot || !bot->IsInWorld())
        return nullptr;

    float const range = sPlayerbotAIConfig.sightDistance;
    std::list<Unit*> units;
    MaNGOS::AnyUnitInObjectRangeCheck check(bot, range);
    MaNGOS::UnitListSearcher<MaNGOS::AnyUnitInObjectRangeCheck> searcher(units, check);
    Cell::VisitAllObjects(bot, searcher, range);

    Creature* nearest = nullptr;
    for (Unit* unit : units)
    {
        if (!unit->IsCreature() || bot->IsHostileTo(unit))
            continue;

        Creature* creature = unit->ToCreature();
        if (!creature->HasFlag(UNIT_NPC_FLAGS, npcFlags))
            continue;

        float const distance = bot->GetDistance(creature);
        if (!nearest || distance < yards)
        {
            nearest = creature;
            yards = distance;
        }
    }
    return nearest;
}
