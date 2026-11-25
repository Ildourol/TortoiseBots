#pragma once

#include <algorithm>
#include <string>
#include <vector>

// Plain wording for a repair order that repaired nothing (`.bot action
// repair`, the TBM Repair button, the rpg repair verb). The caller gathers the
// facts, so the rule and the grouping stay testable without the server.

namespace ai
{
    // An item the repair verb can fix: it has durability and is below it.
    inline bool ItemNeedsRepair(unsigned maxDurability, unsigned durability)
    {
        return maxDurability && durability < maxDurability;
    }

    // anythingToRepair: the bot carries an item below maximum durability.
    // repairNpcInReach: a repair npc is at hand (whether or not it accepted
    // the bot).
    // nearestRepairNpcYards: distance to the closest repair npc within sight,
    // negative when none is known.
    inline std::string RepairRefusalReason(bool anythingToRepair, bool repairNpcInReach, int nearestRepairNpcYards)
    {
        if (!anythingToRepair)
            return "nothing to repair";
        if (repairNpcInReach)
            return "a repair npc is in reach but cannot be used";
        if (nearestRepairNpcYards >= 0)
            return "no repair npc in reach (the nearest is " + std::to_string(nearestRepairNpcYards) + " yards away)";
        return "no repair npc nearby";
    }

    // "No bot repaired anything: nothing to repair; no repair npc in reach
    // (the nearest is 12 yards away)." Each distinct reason appears once, in
    // first-seen order.
    inline std::string FormatRepairRefusal(std::vector<std::string> const& reasons)
    {
        std::vector<std::string> distinct;
        for (std::string const& reason : reasons)
            if (std::find(distinct.begin(), distinct.end(), reason) == distinct.end())
                distinct.push_back(reason);

        std::string text = "No bot repaired anything";
        for (std::size_t i = 0; i < distinct.size(); ++i)
            text += (i == 0 ? ": " : "; ") + distinct[i];
        return text + ".";
    }
}
