// Standalone regression test for the one spec read behind AiFactory::
// GetKitSpecTab (ai/playerbot/policy/gear/KitSpecPolicy.h). The spent talents
// pick the tree; a forced role takes over where it names a single tree.
//
// A `.bot role <bot> holy` on a retribution paladin left the talents alone, so
// gear scoring kept the retribution weights and +Int greens were never seen as
// upgrades.
//
// Build and run (from the repo root):
//   g++ -std=c++17 -Wall -Wextra tools/test_kit_spec_policy.cpp -o /tmp/test_kit_spec
//   /tmp/test_kit_spec

#include "../ai/playerbot/policy/gear/KitSpecPolicy.h"

#include <cstdio>
#include <cstdlib>

using namespace ai;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

// No role: the talents decide, every class and tab unchanged.
static void TestNoRole()
{
    for (int tab = 0; tab < 3; ++tab)
    {
        CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_WARRIOR, tab, 0) == tab);
        CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_PALADIN, tab, 0) == tab);
        CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_ROGUE, tab, 0) == tab);
        CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_PRIEST, tab, 0) == tab);
        CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_SHAMAN, tab, 0) == tab);
        CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_DRUID, tab, 0) == tab);
    }
}

// A forced tank plays its class's protection/feral tree.
static void TestTank()
{
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_WARRIOR, 0, KIT_SPEC_ROLE_TANK) == 2);
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_WARRIOR, 1, KIT_SPEC_ROLE_TANK) == 2);
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_PALADIN, 0, KIT_SPEC_ROLE_TANK) == 1);
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_PALADIN, 2, KIT_SPEC_ROLE_TANK) == 1);
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_DRUID, 0, KIT_SPEC_ROLE_TANK) == 1);
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_DRUID, 2, KIT_SPEC_ROLE_TANK) == 1);
    // A tank bit combined with another role is not the plain tank order.
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_PALADIN, 2, KIT_SPEC_ROLE_TANK | KIT_SPEC_ROLE_DPS) == 2);
    // No tanking tree: the talents stand.
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_PRIEST, 1, KIT_SPEC_ROLE_TANK) == 1);
}

// A forced healer is geared and fought as the class's healing tree, even when
// the talent points still sit in a damage tree.
static void TestHealer()
{
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_PALADIN, 2, KIT_SPEC_ROLE_HEALER) == 0); // ret -> holy
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_PALADIN, 1, KIT_SPEC_ROLE_HEALER) == 0); // prot -> holy
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_DRUID, 0, KIT_SPEC_ROLE_HEALER) == 2);   // balance -> resto
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_DRUID, 1, KIT_SPEC_ROLE_HEALER) == 2);   // feral -> resto
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_SHAMAN, 1, KIT_SPEC_ROLE_HEALER) == 2);  // enhance -> resto
    // Priest heals from two trees: keep the spent one, move shadow to holy.
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_PRIEST, 0, KIT_SPEC_ROLE_HEALER) == 0);  // discipline
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_PRIEST, 1, KIT_SPEC_ROLE_HEALER) == 1);  // holy
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_PRIEST, 2, KIT_SPEC_ROLE_HEALER) == 1);  // shadow -> holy
    // A class with no healing tree keeps its talents.
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_ROGUE, 1, KIT_SPEC_ROLE_HEALER) == 1);
}

// A damage role keeps the talent choice where several trees deal damage, and
// names the class's single damage tree otherwise.
static void TestDps()
{
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_WARRIOR, 2, KIT_SPEC_ROLE_DPS) == 0); // prot -> arms
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_WARRIOR, 1, KIT_SPEC_ROLE_DPS) == 1); // fury stays
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_PALADIN, 1, KIT_SPEC_ROLE_DPS) == 2); // prot -> ret
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_PALADIN, 0, KIT_SPEC_ROLE_DPS) == 2); // holy -> ret
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_PALADIN, 2, KIT_SPEC_ROLE_DPS) == 2); // ret stays
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_PRIEST, 0, KIT_SPEC_ROLE_DPS) == 2);  // disc -> shadow
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_PRIEST, 1, KIT_SPEC_ROLE_DPS) == 2);  // holy -> shadow
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_PRIEST, 2, KIT_SPEC_ROLE_DPS) == 2);  // shadow stays
    // Several damage trees: the spent one stands.
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_SHAMAN, 0, KIT_SPEC_ROLE_DPS) == 0);  // elemental
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_SHAMAN, 1, KIT_SPEC_ROLE_DPS) == 1);  // enhancement
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_DRUID, 0, KIT_SPEC_ROLE_DPS) == 0);   // balance
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_DRUID, 1, KIT_SPEC_ROLE_DPS) == 1);   // feral
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_ROGUE, 2, KIT_SPEC_ROLE_DPS) == 2);   // subtlety
}

// A combined role bitmask never falls through a role that already answered:
// a priest carrying healer and damage bits stays on his healing tree.
static void TestCombinedRoles()
{
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_PRIEST, 0, KIT_SPEC_ROLE_HEALER | KIT_SPEC_ROLE_DPS) == 0); // discipline
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_PRIEST, 1, KIT_SPEC_ROLE_HEALER | KIT_SPEC_ROLE_DPS) == 1); // holy
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_PRIEST, 2, KIT_SPEC_ROLE_HEALER | KIT_SPEC_ROLE_DPS) == 1); // shadow -> holy
    // A class with no healing tree still takes the damage role.
    CHECK(ResolveKitSpecTab(KIT_SPEC_CLASS_WARRIOR, 2, KIT_SPEC_ROLE_HEALER | KIT_SPEC_ROLE_DPS) == 0); // prot -> arms
}

int main()
{
    TestNoRole();
    TestTank();
    TestHealer();
    TestDps();
    TestCombinedRoles();
    std::printf("kit spec policy OK (%d checks)\n", checks);
    return 0;
}
