// Standalone regression test for the incremental gear passes re-rolling what
// they already wear (`.bot gear` grew the bags by one shirt per call, and a
// respec refit re-rolled every slot the kit leaves alone). Guards
//  - cosmetic slots (shirt, tabard) keep a worn piece, an empty one is filled,
//  - a worn piece the item-level cap allows is kept on a tie or a worse score,
//  - a strictly better candidate always wins (real upgrades still land),
//  - above the cap the old rule holds (a better candidate wins, stat-less
//    worn pieces are replaced).
//
// Build and run (from the repo root):
//   g++ -std=c++17 -Wall -Wextra tools/test_worn_gear_keep_policy.cpp -o /tmp/test_worn_gear
//   /tmp/test_worn_gear

#include "../ai/playerbot/policy/gear/WornGearKeepPolicy.h"

#include <cstdio>
#include <cstdlib>
#include <initializer_list>

using namespace ai;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

static void TestCosmeticSlots()
{
    CHECK(CosmeticSlotKeepsWorn(true));
    CHECK(!CosmeticSlotKeepsWorn(false));
}

static void TestIncrementalKeep()
{
    // A worn piece under the cap is kept against an equal or worse candidate.
    CHECK(IncrementalKeepsWornPiece(40, 40, true));
    CHECK(IncrementalKeepsWornPiece(40, 39, true));
    CHECK(IncrementalKeepsWornPiece(40, 0, true));
    // ... and gets replaced by a strictly better one.
    CHECK(!IncrementalKeepsWornPiece(40, 41, true));
    CHECK(!IncrementalKeepsWornPiece(0, 1, true));

    // Above the cap: the old cap-churn rule, a real score still wins.
    CHECK(IncrementalKeepsWornPiece(40, 40, false));
    CHECK(IncrementalKeepsWornPiece(40, 12, false));
    CHECK(!IncrementalKeepsWornPiece(40, 41, false));

    // Worn junk scoring nothing is replaced by any candidate with stats -
    // cap or no cap - so a stat-less piece is not kept forever.
    CHECK(!IncrementalKeepsWornPiece(0, 1, false));
    CHECK(!IncrementalKeepsWornPiece(1, 2, false));
    CHECK(IncrementalKeepsWornPiece(1, 1, true));   // a scored tie under the cap

    // Repeated passes over the same slot converge: the piece the first pass
    // picked is kept by the second one.
    uint32_t worn = 0;
    for (uint32_t candidate : { 30u, 42u })
        if (!IncrementalKeepsWornPiece(worn, candidate, true))
            worn = candidate;
    CHECK(worn == 42);
    CHECK(IncrementalKeepsWornPiece(worn, 42, true));
    CHECK(IncrementalKeepsWornPiece(worn, 41, true));
}

int main()
{
    TestCosmeticSlots();
    TestIncrementalKeep();
    std::printf("PASS test_worn_gear_keep_policy (%d checks)\n", checks);
    return 0;
}
