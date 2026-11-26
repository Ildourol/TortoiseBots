// Standalone regression test for the Armory tab rules (runtime/policy/armory/
// ArmoryPolicy.h): reported slots, permanent-enchant expectations, miss flags,
// the one spec read and the level-63 hit/defense goals.
//
// Build and run (from the repo root):
//   g++ -std=c++17 -Wall -Wextra tools/test_armory_policy.cpp -o /tmp/test_armory_policy
//   /tmp/test_armory_policy

#include "../runtime/policy/armory/ArmoryPolicy.h"

#include <cstdio>
#include <cstdlib>
#include <string>

using namespace TortoiseBots::Armory;

static int checks = 0;
#define CHECK(cond) do { \
    ++checks; \
    if (!(cond)) { \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        std::exit(1); \
    } \
} while (0)

// ai_playerbot_enchant_candidates as shipped: head, neck, shoulder, chest,
// waist, legs, feet, wrists, hands, both rings, back, both weapons, ranged.
static constexpr uint32_t kShippedCandidateSlots =
    (1u << 0) | (1u << 1) | (1u << 2) | (1u << 4) | (1u << 5) | (1u << 6) |
    (1u << 7) | (1u << 8) | (1u << 9) | (1u << 10) | (1u << 11) |
    (1u << 14) | (1u << 15) | (1u << 16) | (1u << 17);

static void TestReportedSlots()
{
    CHECK(SlotReported(kSlotHead));
    CHECK(SlotReported(kSlotOffHand));
    CHECK(SlotReported(kSlotRanged));
    CHECK(!SlotReported(kSlotShirt));
    CHECK(!SlotReported(kSlotTabard));
    CHECK(!SlotReported(-1));
    CHECK(!SlotReported(19));
}

static void TestEnchantExpectations()
{
    // Head, shoulder, waist (belt buckles) and legs (librams/arcanums) are
    // real permanent-enchant slots on Tortoise.
    CHECK(SlotCanTakePermanentEnchant(kSlotHead, kShippedCandidateSlots));
    CHECK(SlotCanTakePermanentEnchant(kSlotShoulder, kShippedCandidateSlots));
    CHECK(SlotCanTakePermanentEnchant(kSlotWaist, kShippedCandidateSlots));
    CHECK(SlotCanTakePermanentEnchant(kSlotLegs, kShippedCandidateSlots));
    CHECK(SlotCanTakePermanentEnchant(kSlotBack, kShippedCandidateSlots));
    CHECK(SlotCanTakePermanentEnchant(kSlotMainHand, kShippedCandidateSlots));
    CHECK(SlotCanTakePermanentEnchant(kSlotOffHand, kShippedCandidateSlots));
    // Gemstone rows never make neck or rings look missing.
    CHECK(!SlotCanTakePermanentEnchant(kSlotNeck, kShippedCandidateSlots));
    CHECK(!SlotCanTakePermanentEnchant(kSlotFinger1, kShippedCandidateSlots));
    CHECK(!SlotCanTakePermanentEnchant(kSlotFinger2, kShippedCandidateSlots));
    CHECK(!SlotCanTakePermanentEnchant(kSlotTrinket1, kShippedCandidateSlots));
    CHECK(!SlotCanTakePermanentEnchant(kSlotTrinket2, kShippedCandidateSlots));
    // The shirt and tabard never carry one.
    CHECK(!SlotCanTakePermanentEnchant(kSlotShirt, kShippedCandidateSlots));
    CHECK(!SlotCanTakePermanentEnchant(kSlotTabard, kShippedCandidateSlots));
    // A DB without the candidate rows expects no enchant at all.
    CHECK(!SlotCanTakePermanentEnchant(kSlotHead, 0));
    CHECK(!SlotCanTakePermanentEnchant(kSlotRanged, 0));

    // Scopes are the hunter's ranged slot only.
    CHECK(SlotWantsPermanentEnchant(kSlotRanged, kClassHunter, kShippedCandidateSlots));
    CHECK(!SlotWantsPermanentEnchant(kSlotRanged, kClassWarrior, kShippedCandidateSlots));
    CHECK(!SlotWantsPermanentEnchant(kSlotRanged, kClassMage, kShippedCandidateSlots));
    CHECK(SlotWantsPermanentEnchant(kSlotHead, kClassMage, kShippedCandidateSlots));
}

static void TestItemFit()
{
    // Shield and one-hand weapon take an off-hand enchant; a 2H cannot sit
    // there and a held-in-off-hand frill has no permanent enchant.
    CHECK(ItemTakesSlotEnchant(kSlotOffHand, kItemClassArmor, kInvTypeShield));
    CHECK(ItemTakesSlotEnchant(kSlotOffHand, kItemClassWeapon, 13)); // one hand
    CHECK(ItemTakesSlotEnchant(kSlotOffHand, kItemClassWeapon, 22)); // off hand
    CHECK(!ItemTakesSlotEnchant(kSlotOffHand, kItemClassWeapon, kInvTypeTwoHand));
    CHECK(!ItemTakesSlotEnchant(kSlotOffHand, kItemClassArmor, kInvTypeHoldable));
    CHECK(ItemTakesSlotEnchant(kSlotMainHand, kItemClassWeapon, 13));
    CHECK(!ItemTakesSlotEnchant(kSlotMainHand, kItemClassArmor, kInvTypeShield));
    CHECK(ItemTakesSlotEnchant(kSlotHead, kItemClassArmor, 1));
    CHECK(ItemTakesSlotEnchant(kSlotRanged, kItemClassWeapon, 15));

    // A two-hander in the main hand hides the off hand on purpose.
    CHECK(OffHandIsIntentional(kInvTypeTwoHand));
    CHECK(!OffHandIsIntentional(13));
    CHECK(!OffHandIsIntentional(kInvTypeShield));
}

static void TestMissFlags()
{
    CHECK(SlotMiss(false, true, false) == kMissEmpty);   // empty slot
    CHECK(SlotMiss(false, false, false) == kMissEmpty);  // neck/ring empty
    CHECK(SlotMiss(true, true, false) == kMissEnchant);  // item without enchant
    CHECK(SlotMiss(true, false, false) == kMissNone);    // present, not expected
    CHECK(SlotMiss(true, true, true) == kMissNone);      // enchanted
    CHECK(SlotMiss(true, false, true) == kMissNone);
}

static void TestMeleeCap()
{
    // 5% - (300 - 315) * 0.2 = 8% against a level-63 boss.
    CHECK(MeleeHitCap(300.0) > 7.99 && MeleeHitCap(300.0) < 8.01);
    CHECK(MeleeHitCap(305.0) > 6.99 && MeleeHitCap(305.0) < 7.01);
    CHECK(MeleeHitCap(315.0) > 4.99 && MeleeHitCap(315.0) < 5.01);
    CHECK(MeleeHitCap(200.0) > 27.99 && MeleeHitCap(200.0) < 28.01);
    // Very low skill cannot exceed the core's 60% miss ceiling.
    CHECK(MeleeHitCap(0.0) > 59.99 && MeleeHitCap(0.0) < 60.01);
    // Weapon skill above the boss's defense removes the base miss.
    CHECK(MeleeHitCap(340.0) == 0.0);
}

static void TestSpellCap()
{
    // 94 - (63-60-2)*11 = 83% hit, 17% miss at level 60.
    CHECK(SpellHitCap(60) > 16.99 && SpellHitCap(60) < 17.01);
    // Level 55: 94 - (8-2)*11 = 28% hit, below Turtle's 33% floor, so 67%.
    CHECK(SpellHitCap(55) > 66.99 && SpellHitCap(55) < 67.01);
    // Level 63 is outside play, but the formula stays monotonic and clamped.
    CHECK(SpellHitCap(63) > 3.99 && SpellHitCap(63) < 4.01);
    CHECK(SpellHitCap(1) <= 67.0); // Turtle floor: never below 33% hit
}

static void TestDefenseCap()
{
    // 63*5 defense + 5% base crit / 0.04% = 440.
    CHECK(DefenseCritCap() == 440);
    CHECK(DefenseCritCap(60) == 425);
}

static void TestSpecRead()
{
    SpecRole warrior = ResolveSpecRole(kClassWarrior, 2, false);
    CHECK(std::string(warrior.spec) == "Protection");
    CHECK(std::string(warrior.role) == "tank");
    CHECK(warrior.hitKind == HitKind::Melee);

    SpecRole paladin = ResolveSpecRole(kClassPaladin, 0, false);
    CHECK(std::string(paladin.spec) == "Holy");
    CHECK(paladin.hitKind == HitKind::None);

    SpecRole hunter = ResolveSpecRole(kClassHunter, 1, false);
    CHECK(std::string(hunter.spec) == "Marksmanship");
    CHECK(std::string(hunter.role) == "ranged");
    CHECK(hunter.hitKind == HitKind::Ranged);

    SpecRole rogue = ResolveSpecRole(kClassRogue, 1, false);
    CHECK(std::string(rogue.spec) == "Combat");
    CHECK(rogue.hitKind == HitKind::Melee);

    SpecRole priest = ResolveSpecRole(kClassPriest, 2, false);
    CHECK(std::string(priest.spec) == "Shadow");
    CHECK(priest.hitKind == HitKind::Spell);

    SpecRole shamanDps = ResolveSpecRole(kClassShaman, 1, false);
    CHECK(std::string(shamanDps.spec) == "Enhancement");
    CHECK(std::string(shamanDps.role) == "melee");
    SpecRole shamanTank = ResolveSpecRole(kClassShaman, 1, true);
    CHECK(std::string(shamanTank.role) == "tank");

    SpecRole mage = ResolveSpecRole(kClassMage, 2, false);
    CHECK(std::string(mage.spec) == "Frost");
    CHECK(mage.hitKind == HitKind::Spell);

    SpecRole warlock = ResolveSpecRole(kClassWarlock, 0, false);
    CHECK(std::string(warlock.spec) == "Affliction");

    SpecRole feralTank = ResolveSpecRole(kClassDruid, 1, true);
    CHECK(std::string(feralTank.spec) == "Feral");
    CHECK(std::string(feralTank.role) == "tank");
    SpecRole feralCat = ResolveSpecRole(kClassDruid, 1, false);
    CHECK(std::string(feralCat.role) == "melee");
    SpecRole druidResto = ResolveSpecRole(kClassDruid, 2, false);
    CHECK(std::string(druidResto.spec) == "Restoration");
    CHECK(druidResto.hitKind == HitKind::None);

    SpecRole unknown = ResolveSpecRole(42, 1, false);
    CHECK(std::string(unknown.spec) == "-");
    CHECK(std::string(unknown.role) == "-");
    CHECK(unknown.hitKind == HitKind::None);
}

static void TestHitCapDispatch()
{
    CHECK(HitCap(HitKind::Melee, 300.0, 60) > 7.99 && HitCap(HitKind::Melee, 300.0, 60) < 8.01);
    CHECK(HitCap(HitKind::Ranged, 300.0, 60) > 7.99 && HitCap(HitKind::Ranged, 300.0, 60) < 8.01);
    CHECK(HitCap(HitKind::Spell, 0.0, 60) > 16.99 && HitCap(HitKind::Spell, 0.0, 60) < 17.01);
    CHECK(HitCap(HitKind::None, 300.0, 60) == 0.0);
}

int main()
{
    TestReportedSlots();
    TestEnchantExpectations();
    TestItemFit();
    TestMissFlags();
    TestMeleeCap();
    TestSpellCap();
    TestDefenseCap();
    TestSpecRead();
    TestHitCapDispatch();

    std::printf("test_armory_policy: %d checks passed\n", checks);
    return 0;
}
