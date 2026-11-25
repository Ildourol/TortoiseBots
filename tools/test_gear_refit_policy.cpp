// Standalone test for the remembered gear mode (TB #1690): a bot geared with
// `.bot gear tier N ilvl=X` that changes talents refits the same tier kit
// under the same cap, a pool bot re-rolls the pool, and a bot nobody geared
// explicitly is left alone on an autonomous level-up talent convergence.
//
// Build and run:
//   g++ -std=c++17 -Wall -Wextra tools/test_gear_refit_policy.cpp -o /tmp/test_gear_refit_policy
//   /tmp/test_gear_refit_policy

#include "../runtime/policy/gear/GearRefitPolicy.h"

#include <cstdio>
#include <cstdlib>

using namespace TortoiseBots;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

int main()
{
    // The bug: a pinned protection warrior ran the pool regear after a
    // respec. A remembered tier kit refits the same kit instead.
    GearMode kit;
    kit.tier = 3;
    kit.ilvlCap = 66;
    CHECK(DecideGearRefit(true, true, true, kit) == GearRefit::Kit);
    CHECK(DecideGearRefit(true, false, true, kit) == GearRefit::Kit);
    // Every tier is a kit, also without an item-level cap.
    kit.ilvlCap = 0;
    CHECK(DecideGearRefit(true, true, true, kit) == GearRefit::Kit);

    // A pool bot nobody pinned keeps the old pool refit.
    GearMode none;
    CHECK(DecideGearRefit(true, true, false, none) == GearRefit::Pool);
    // An owned bot with no remembered mode keeps what it wears (old rule).
    CHECK(DecideGearRefit(true, false, false, none) == GearRefit::None);

    // An explicit pool mode (`.bot gear tier 0 ilvl=X`) refits the pool under
    // the same cap, for any bot (the GM pinned it on purpose).
    GearMode capped;
    capped.ilvlCap = 63;
    CHECK(DecideGearRefit(true, true, true, capped) == GearRefit::PoolWithCap);
    CHECK(DecideGearRefit(true, false, true, capped) == GearRefit::PoolWithCap);

    // Autonomous level-up talent convergence never re-gears by itself: the
    // existing paths own that (seed, top-up, maintenance).
    CHECK(DecideGearRefit(false, true, true, kit) == GearRefit::None);
    CHECK(DecideGearRefit(false, true, false, none) == GearRefit::None);

    // A tier 0 / cap 0 row is still the pool, never a kit.
    CHECK(DecideGearRefit(true, true, true, none) == GearRefit::Pool);

    std::printf("test_gear_refit_policy: %d checks passed\n", checks);
    return 0;
}
