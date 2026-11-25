// Standalone regression test for issue #490 PR 1: static best-in-slot sets
// for level-60 hires. Guards
//  - the spec-key table (28 menu options; druid bear/cat get distinct keys
//    even though they share the feral premade path),
//  - role-only fallback resolution (`.bot hire` without a spec word),
//  - the effective-tier and eligibility rules (level 60, tier >= 1, enabled),
//  - the race-override lookup (race row wins, else race 0, else no row).
//
// Build and run (from the repo root):
//   g++ -std=c++17 -Wall -Wextra tools/test_hire_gear_policy.cpp -o /tmp/test_hire_gear
//   /tmp/test_hire_gear

#include "../runtime/policy/hire/HireGearPolicy.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace TortoiseBots;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

static uint8_t const kClasses[] = {
    kHireClassWarrior, kHireClassPaladin, kHireClassHunter, kHireClassRogue, kHireClassPriest,
    kHireClassShaman, kHireClassMage, kHireClassWarlock, kHireClassDruid,
};

// All 29 menu options resolve to a non-empty key; druid gets 4 distinct keys
// with bear != cat.
static void TestSpecKeys()
{
    size_t total = 0;
    for (uint8_t cls : kClasses)
    {
        uint32_t count = 0;
        HireSpecOptions(cls, count);
        CHECK(count == ((cls == kHireClassDruid || cls == kHireClassShaman) ? 4u : 3u));
        for (uint32_t i = 0; i < count; ++i)
        {
            char const* key = HireGearSpecKey(cls, static_cast<int>(i));
            CHECK(key != nullptr && key[0] != '\0');
            CHECK(std::strlen(key) <= 15);
            ++total;
        }
    }
    CHECK(total == 29);
    CHECK(std::string(HireGearSpecKey(kHireClassDruid, 0)) == "bear");
    CHECK(std::string(HireGearSpecKey(kHireClassDruid, 3)) == "cat");
    CHECK(std::string(HireGearSpecKey(kHireClassShaman, 3)) == "tank");
    CHECK(std::string(HireGearSpecKey(kHireClassWarrior, 1)) == "arms");
    CHECK(std::string(HireGearSpecKey(kHireClassWarrior, 2)) == "fury");
    CHECK(HireGearSpecKey(kHireClassWarrior, -1) == nullptr);
    CHECK(HireGearSpecKey(kHireClassWarrior, 3) == nullptr);
    CHECK(HireGearSpecKey(6, 0) == nullptr);
}

// Role-only requests pin to the menu's first option for the role.
static void TestRoleFallback()
{
    CHECK(std::string(ResolveHireGearSpecKey(kHireClassWarrior, 1, kHireRoleDps)) == "arms");
    CHECK(std::string(ResolveHireGearSpecKey(kHireClassWarrior, -1, kHireRoleTank)) == "protection");
    CHECK(std::string(ResolveHireGearSpecKey(kHireClassWarrior, -1, kHireRoleDps)) == "arms");
    CHECK(std::string(ResolveHireGearSpecKey(kHireClassPriest, -1, kHireRoleHealer)) == "holy");
    CHECK(std::string(ResolveHireGearSpecKey(kHireClassShaman, -1, kHireRoleTank)) == "tank");
    CHECK(std::string(ResolveHireGearSpecKey(kHireClassShaman, -1, kHireRoleHealer)) == "restoration");
    CHECK(std::string(ResolveHireGearSpecKey(kHireClassDruid, -1, kHireRoleDps)) == "balance");
    CHECK(ResolveHireGearSpecKey(6, -1, kHireRoleDps) == nullptr);
}

// Kit spec key -> weightscale name the slot rules check rows against.
static void TestGearScaleName()
{
    CHECK(std::string(HireGearScaleName("protection")) == "prot");
    CHECK(std::string(HireGearScaleName("retribution")) == "retrib");
    CHECK(std::string(HireGearScaleName("tank")) == "shamantank");
    CHECK(std::string(HireGearScaleName("enhancement")) == "enhance");
    CHECK(std::string(HireGearScaleName("bear")) == "feraltank");
    CHECK(std::string(HireGearScaleName("cat")) == "feraldps");
    CHECK(std::string(HireGearScaleName("restoration")) == "resto");
    CHECK(std::string(HireGearScaleName("beastmastery")) == "beast");
    CHECK(std::string(HireGearScaleName("destruction")) == "destro");
    CHECK(HireGearScaleName(nullptr) == nullptr);
    CHECK(HireGearScaleName("") == nullptr);
    CHECK(HireGearScaleName("nonsense") == nullptr);
    // Every recruiter menu key must resolve, or a kit row is never checked.
    for (uint8_t cls : { kHireClassWarrior, kHireClassPaladin, kHireClassHunter, kHireClassRogue,
             kHireClassPriest, kHireClassShaman, kHireClassMage, kHireClassWarlock, kHireClassDruid })
    {
        uint32_t count = 0;
        (void)HireSpecOptions(cls, count);
        for (uint32_t i = 0; i < count; ++i)
            CHECK(HireGearScaleName(HireGearSpecKey(cls, static_cast<int>(i))) != nullptr);
    }
}

// Master switch gates everything; StartTier clamps to 0..7.
static void TestEffectiveTier()
{
    CHECK(EffectiveHireGearTier(false, 7) == 0);
    CHECK(EffectiveHireGearTier(true, 0) == 0);
    CHECK(EffectiveHireGearTier(true, 3) == 3);
    CHECK(EffectiveHireGearTier(true, 7) == 7);
    CHECK(EffectiveHireGearTier(true, 70) == 7);
}

// Only enabled level-60 hires at tier >= 1 take the static pass.
static void TestEligibility()
{
    CHECK(HireGearTierEligible(60, true, 1));
    CHECK(HireGearTierEligible(60, true, 7));
    CHECK(!HireGearTierEligible(60, true, 0));
    CHECK(!HireGearTierEligible(59, true, 7));
    CHECK(!HireGearTierEligible(60, false, 7));
    CHECK(!HireGearTierEligible(60, true, 8));
}

// A tier already recorded for the bot refits even with tiers switched off;
// only the level and tier range still apply.
static void TestRecordedTier()
{
    CHECK(RecordedHireGearTierUsable(60, 1));
    CHECK(RecordedHireGearTierUsable(60, 7));
    CHECK(!RecordedHireGearTierUsable(60, 0));
    CHECK(!RecordedHireGearTierUsable(59, 7));
    CHECK(!RecordedHireGearTierUsable(60, 8));
}

static HireGearRow MakeRow(uint8_t cls, char const* spec, uint8_t race, uint8_t tier, uint8_t slot,
    uint32_t item, uint32_t enchant, uint32_t itemLevel = 0)
{
    HireGearRow row;
    row.classId = cls;
    std::strncpy(row.spec, spec, sizeof(row.spec) - 1);
    row.race = race;
    row.tier = tier;
    row.slot = slot;
    row.itemId = item;
    row.enchantSpell = enchant;
    row.itemLevel = itemLevel;
    return row;
}

// Race-specific row wins; else race 0; else no row. Same input, same row.
static void TestLookup()
{
    std::vector<HireGearRow> rows;
    rows.push_back(MakeRow(1, "fury", 0, 1, 15, 1001, 2001));
    rows.push_back(MakeRow(1, "fury", 1, 1, 15, 1002, 2002));
    rows.push_back(MakeRow(1, "fury", 0, 1, 0, 1003, 0));
    rows.push_back(MakeRow(1, "arms", 0, 1, 15, 1004, 0));

    HireGearRow const* out = nullptr;
    CHECK(LookupHireGearRow(rows.data(), rows.size(), 1, "fury", 1, 1, 15, out));
    CHECK(out && out->itemId == 1002 && out->enchantSpell == 2002);
    CHECK(LookupHireGearRow(rows.data(), rows.size(), 1, "fury", 2, 1, 15, out));
    CHECK(out && out->itemId == 1001 && out->enchantSpell == 2001);
    CHECK(LookupHireGearRow(rows.data(), rows.size(), 1, "fury", 2, 1, 0, out));
    CHECK(out && out->itemId == 1003);
    CHECK(!LookupHireGearRow(rows.data(), rows.size(), 1, "fury", 2, 1, 1, out));
    CHECK(out == nullptr);
    CHECK(!LookupHireGearRow(rows.data(), rows.size(), 1, "fury", 2, 2, 15, out));
    CHECK(!LookupHireGearRow(rows.data(), rows.size(), 1, "protection", 1, 1, 15, out));
    // Deterministic: repeated lookups find the same row.
    HireGearRow const* first = nullptr;
    HireGearRow const* again = nullptr;
    CHECK(LookupHireGearRow(rows.data(), rows.size(), 1, "fury", 1, 1, 15, first));
    CHECK(LookupHireGearRow(rows.data(), rows.size(), 1, "fury", 1, 1, 15, again));
    CHECK(first && again && first->itemId == again->itemId && first->itemId == 1002);
}

// A bot's own premade path picks its kit; the role decides only when the path
// cannot be read. Menu option 0 is the tank spec, so a DPS warrior must not
// resolve to it.
static void TestOwnSpecKey()
{
    CHECK(std::string(ResolveHireGearKeyForPath(kHireClassWarrior, "fury", kHireRoleDps)) == "fury");
    CHECK(std::string(ResolveHireGearKeyForPath(kHireClassWarrior, "arms", kHireRoleDps)) == "arms");
    CHECK(std::string(ResolveHireGearKeyForPath(kHireClassWarrior, "protection", kHireRoleTank)) == "protection");
    CHECK(std::string(ResolveHireGearKeyForPath(kHireClassPaladin, "holy", kHireRoleHealer)) == "holy");
    CHECK(std::string(ResolveHireGearKeyForPath(kHireClassDruid, "restoration", kHireRoleHealer)) == "restoration");
    CHECK(std::string(ResolveHireGearKeyForPath(kHireClassDruid, "feral", kHireRoleTank)) == "bear");
    CHECK(std::string(ResolveHireGearKeyForPath(kHireClassDruid, "feral", kHireRoleDps)) == "cat");
    CHECK(std::string(ResolveHireGearKeyForPath(kHireClassWarrior, nullptr, kHireRoleDps)) == "arms");
    CHECK(std::string(ResolveHireGearKeyForPath(kHireClassWarrior, "", kHireRoleDps)) == "arms");
    CHECK(std::string(ResolveHireGearKeyForPath(kHireClassPaladin, "nonsense", kHireRoleHealer)) == "holy");
    CHECK(ResolveHireGearKeyForPath(6, "fury", kHireRoleDps) == nullptr);
}

// Talent tab + tank read pick the same keys every gear pass uses: the tab is
// the one spec source (AiFactory::GetKitSpecTab), so a paladin's holy tab can
// never key a retribution set and a feral druid's tab alone cannot pick bear.
static std::string TabKey(uint8_t classId, int tab, bool tank)
{
    char const* key = ResolveHireGearKeyForTab(classId, tab, tank);
    return key ? std::string(key) : std::string("<none>");
}

static void TestTabKey()
{
    CHECK(TabKey(kHireClassWarrior, 0, false) == "arms");
    CHECK(TabKey(kHireClassWarrior, 1, false) == "fury");
    CHECK(TabKey(kHireClassWarrior, 2, true) == "protection");
    CHECK(TabKey(kHireClassPaladin, 0, false) == "holy");
    CHECK(TabKey(kHireClassPaladin, 1, true) == "protection");
    CHECK(TabKey(kHireClassPaladin, 2, false) == "retribution");
    CHECK(TabKey(kHireClassPriest, 0, false) == "discipline");
    CHECK(TabKey(kHireClassPriest, 1, false) == "holy");
    CHECK(TabKey(kHireClassPriest, 2, false) == "shadow");
    CHECK(TabKey(kHireClassShaman, 0, false) == "elemental");
    CHECK(TabKey(kHireClassShaman, 1, false) == "enhancement");
    CHECK(TabKey(kHireClassShaman, 1, true) == "tank");
    CHECK(TabKey(kHireClassShaman, 2, false) == "restoration");
    CHECK(TabKey(kHireClassMage, 0, false) == "arcane");
    CHECK(TabKey(kHireClassMage, 1, false) == "fire");
    CHECK(TabKey(kHireClassMage, 2, false) == "frost");
    CHECK(TabKey(kHireClassHunter, 0, false) == "beastmastery");
    CHECK(TabKey(kHireClassHunter, 1, false) == "marksmanship");
    CHECK(TabKey(kHireClassHunter, 2, false) == "survival");
    CHECK(TabKey(kHireClassRogue, 0, false) == "assassination");
    CHECK(TabKey(kHireClassRogue, 1, false) == "combat");
    CHECK(TabKey(kHireClassRogue, 2, false) == "subtlety");
    CHECK(TabKey(kHireClassWarlock, 0, false) == "affliction");
    CHECK(TabKey(kHireClassWarlock, 1, false) == "demonology");
    CHECK(TabKey(kHireClassWarlock, 2, false) == "destruction");
    CHECK(TabKey(kHireClassDruid, 0, false) == "balance");
    CHECK(TabKey(kHireClassDruid, 1, true) == "bear");
    CHECK(TabKey(kHireClassDruid, 1, false) == "cat");
    CHECK(TabKey(kHireClassDruid, 2, false) == "restoration");
    CHECK(TabKey(kHireClassWarrior, 3, false) == "<none>");
    CHECK(TabKey(kHireClassWarrior, -1, false) == "<none>");
    CHECK(TabKey(6, 0, false) == "<none>");
}

// The pool pass before a tier kit may touch only the slots the kit leaves
// alone: no row, an intentionally empty row, or a row above the cap. A
// covered slot is left for the kit - otherwise the kit swaps out the pool
// piece, the piece the bot already wore stays in the bags, and the kit makes
// a fresh copy (bag growth; a duplicate the equip check then refuses for a
// max_count 1 row).
static void TestPoolFillMask()
{
    std::vector<HireGearRow> rows;
    rows.push_back(MakeRow(1, "protection", 0, 2, 0, 2001, 0, 60)); // head, under the cap
    rows.push_back(MakeRow(1, "protection", 0, 2, 4, 2002, 0, 66)); // chest, above 62
    rows.push_back(MakeRow(1, "protection", 0, 2, 5, 0, 0));        // deliberately cleared
    rows.push_back(MakeRow(1, "protection", 1, 2, 6, 2003, 0, 55)); // race-specific legs
    rows.push_back(MakeRow(1, "protection", 0, 2, 10, 2004, 0, 58)); // ring 1
    rows.push_back(MakeRow(1, "protection", 0, 2, 11, 2005, 0, 59)); // ring 2
    rows.push_back(MakeRow(1, "arms", 0, 2, 15, 2006, 0, 63));      // other spec

    uint32_t const underCap = HireGearPoolFillSlotMask(rows.data(), rows.size(), 1, "protection", 1, 2, 62);
    CHECK((underCap & (1u << 0)) == 0);   // kit covers the head
    CHECK((underCap & (1u << 4)) != 0);   // kit skips the above-cap chest
    CHECK((underCap & (1u << 5)) == 0);   // kit clears the slot itself
    CHECK((underCap & (1u << 6)) == 0);   // race row covers the legs
    CHECK((underCap & (1u << 10)) == 0);
    CHECK((underCap & (1u << 11)) == 0);
    CHECK((underCap & (1u << 1)) != 0);   // no row: nothing to cover it
    CHECK((underCap & (1u << 15)) != 0);  // another spec's row covers nothing here

    // No cap: only the slots without a row (or with an empty one) may be filled.
    uint32_t const noCap = HireGearPoolFillSlotMask(rows.data(), rows.size(), 1, "protection", 1, 2, 0);
    CHECK(noCap != 0);
    CHECK((noCap & (1u << 0)) == 0);
    CHECK((noCap & (1u << 4)) == 0);
    CHECK((noCap & (1u << 5)) == 0);
    CHECK((noCap & (1u << 1)) != 0);

    // A kit that covers every slot leaves the pool pass nothing to do.
    std::vector<HireGearRow> full;
    for (uint8_t slot = 0; slot < kHireGearEquipSlotCount; ++slot)
        full.push_back(MakeRow(1, "protection", 0, 2, slot, 3000u + slot, 0, 60));
    CHECK(HireGearPoolFillSlotMask(full.data(), full.size(), 1, "protection", 1, 2, 66) == 0);

    // Only the equipment slots are considered (no bit above the last one).
    CHECK((underCap >> kHireGearEquipSlotCount) == 0);

    // Unknown spec / unknown tier: nothing is covered.
    CHECK(HireGearPoolFillSlotMask(rows.data(), rows.size(), 1, nullptr, 1, 2, 62) == 0);
    uint32_t const otherTier = HireGearPoolFillSlotMask(rows.data(), rows.size(), 1, "protection", 1, 3, 62);
    CHECK(otherTier != 0);
    CHECK((otherTier & (1u << 0)) != 0);
}

// A kit row whose item is unique by max_count and already worn in its paired
// slot (ring/trinket) must move that copy, not be skipped: the unseen-item
// check counts the worn copy and refuses a second one, which left the old
// spec's ring on the body and the kit's ring in the bags after a respec.
static void TestWornCopySlot()
{
    uint32_t worn[kHireGearEquipSlotCount] = {};
    worn[11] = 2246;  // Myrmidon's Signet in the second finger
    worn[10] = 13098; // the arms kit's ring in the first finger
    worn[12] = 17774; // trinket pair
    worn[13] = 18370;

    CHECK(HireGearWornCopySlot(worn, 10, 2246) == 11);
    CHECK(HireGearWornCopySlot(worn, 11, 13098) == 10);
    // The slot asked for is never its own answer.
    CHECK(HireGearWornCopySlot(worn, 11, 2246) == kHireGearNoSlot);
    // Not worn anywhere.
    CHECK(HireGearWornCopySlot(worn, 10, 11669) == kHireGearNoSlot);
    // The pair's other slot is found from either side.
    CHECK(HireGearWornCopySlot(worn, 10, 17774) == 12);
    CHECK(HireGearWornCopySlot(worn, 13, 17774) == 12);
    CHECK(HireGearWornCopySlot(nullptr, 10, 2246) == kHireGearNoSlot);
    CHECK(HireGearWornCopySlot(worn, 10, 0) == kHireGearNoSlot);

    // The arms -> protection refit: the protection kit's finger rows are
    // 2246 + 11669 and the bot wears 13098 + 2246. Row by row, the first
    // finger moves the worn 2246 across, the second then replaces the 13098
    // that landed there, so the pair ends as exactly 2246 + 11669.
    uint32_t rows[kHireGearEquipSlotCount] = {};
    rows[10] = 2246;
    rows[11] = 11669;
    for (uint8_t slot = 10; slot < 12; ++slot)
    {
        if (worn[slot] == rows[slot])
            continue;
        uint8_t source = HireGearWornCopySlot(worn, slot, rows[slot]);
        if (source != kHireGearNoSlot)
            std::swap(worn[slot], worn[source]);
        else
            worn[slot] = rows[slot]; // the row's own copy or a fresh one
    }
    CHECK(worn[10] == 2246);
    CHECK(worn[11] == 11669);
}

int main()
{
    TestSpecKeys();
    TestRoleFallback();
    TestGearScaleName();
    TestOwnSpecKey();
    TestTabKey();
    TestEffectiveTier();
    TestEligibility();
    TestRecordedTier();
    TestLookup();
    TestPoolFillMask();
    TestWornCopySlot();
    std::printf("hire gear policy OK (%d checks)\n", checks);
    return 0;
}
