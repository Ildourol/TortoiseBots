#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "../ai/playerbot/policy/gear/RepairRefusalPolicy.h"

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using namespace ai;

int main()
{
    // Only an item below its maximum durability can be repaired.
    CHECK(!ItemNeedsRepair(100, 100));
    CHECK(ItemNeedsRepair(100, 99));
    CHECK(ItemNeedsRepair(100, 0));
    CHECK(!ItemNeedsRepair(0, 0));

    // Nothing damaged wins over the npc checks.
    CHECK(RepairRefusalReason(false, false, 12) == "nothing to repair");
    CHECK(RepairRefusalReason(false, true, -1) == "nothing to repair");

    // A repair npc at hand that refused the bot is not "walk closer".
    CHECK(RepairRefusalReason(true, true, 2) == "a repair npc is in reach but cannot be used");

    // Out of reach names the distance, so "move closer" is actionable.
    CHECK(RepairRefusalReason(true, false, 12) == "no repair npc in reach (the nearest is 12 yards away)");
    CHECK(RepairRefusalReason(true, false, 0) == "no repair npc in reach (the nearest is 0 yards away)");

    // No repair npc known at all.
    CHECK(RepairRefusalReason(true, false, -1) == "no repair npc nearby");

    // No refusals recorded still gives a complete sentence.
    CHECK(FormatRepairRefusal({}) == "No bot repaired anything.");

    CHECK(FormatRepairRefusal({ "nothing to repair" }) ==
        "No bot repaired anything: nothing to repair.");

    // Repeated reasons are listed once, in first-seen order.
    CHECK(FormatRepairRefusal({ "nothing to repair", "no repair npc nearby", "nothing to repair" }) ==
        "No bot repaired anything: nothing to repair; no repair npc nearby.");

    std::cout << "repair refusal policy: OK\n";
    return 0;
}
