#pragma once

#include <list>

class Player;
class Creature;

namespace ai
{
    // Alive creatures whose world-interaction window the bot could open right
    // now (within INTERACTION_DISTANCE), read straight from the map. A field
    // repair bot is a summoned creature or a player's companion, and the
    // cached, line-of-sight-filtered "nearest npcs" value can miss it; verbs
    // that only need what is at arm's length look here instead.
    std::list<Creature*> ServiceCreaturesInReach(Player* bot);

    // The closest non-hostile creature carrying npcFlags within sight, with
    // its distance in yards; nullptr when none. Only for telling the player
    // where the service npc is, so it does not check usability.
    Creature* NearestNpcWithFlagInSight(Player* bot, unsigned npcFlags, float& yards);
}
