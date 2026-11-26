#pragma once

// Pure rules behind the Armory tab and the `.bot armory` stream: which
// equipment slots a row reports, when a slot counts as missing an item or a
// permanent enchant, and the level-63 hit and defense goals the core's own
// miss formulas imply. Header-only and free of core headers so the rules stay
// unit tested on their own (tools/test_armory_policy.cpp).

#include <cstdint>

namespace TortoiseBots
{
namespace Armory
{

// Core class ids (SharedDefines.h), repeated so this header stays core-free.
constexpr uint8_t kClassWarrior = 1;
constexpr uint8_t kClassPaladin = 2;
constexpr uint8_t kClassHunter = 3;
constexpr uint8_t kClassRogue = 4;
constexpr uint8_t kClassPriest = 5;
constexpr uint8_t kClassShaman = 7;
constexpr uint8_t kClassMage = 8;
constexpr uint8_t kClassWarlock = 9;
constexpr uint8_t kClassDruid = 11;

// EquipmentSlots values (SharedDefines.h).
constexpr int kSlotHead = 0;
constexpr int kSlotNeck = 1;
constexpr int kSlotShoulder = 2;
constexpr int kSlotShirt = 3;
constexpr int kSlotChest = 4;
constexpr int kSlotWaist = 5;
constexpr int kSlotLegs = 6;
constexpr int kSlotFeet = 7;
constexpr int kSlotWrists = 8;
constexpr int kSlotHands = 9;
constexpr int kSlotFinger1 = 10;
constexpr int kSlotFinger2 = 11;
constexpr int kSlotTrinket1 = 12;
constexpr int kSlotTrinket2 = 13;
constexpr int kSlotBack = 14;
constexpr int kSlotMainHand = 15;
constexpr int kSlotOffHand = 16;
constexpr int kSlotRanged = 17;
constexpr int kSlotTabard = 18;
constexpr int kMaxEquipmentSlot = 18;

// Item class and inventory type mirrors (ItemPrototype.h).
constexpr uint32_t kItemClassWeapon = 2;
constexpr uint32_t kItemClassArmor = 4;
constexpr uint32_t kInvTypeShield = 14;
constexpr uint32_t kInvTypeTwoHand = 17;
constexpr uint32_t kInvTypeHoldable = 23;

// What an ARMORY_EQ miss flag means.
enum SlotMiss : uint8_t
{
    kMissNone = 0,
    kMissEnchant = 1,
    kMissEmpty = 2,
};

// Every slot the strip shows: all equipment slots except the shirt and
// tabard, which are cosmetic and never part of a readiness read.
inline bool SlotReported(int slot)
{
    return slot >= 0 && slot <= kMaxEquipmentSlot && slot != kSlotShirt && slot != kSlotTabard;
}

// The slots the shipped ai_playerbot_enchant_candidates table has rows for.
// candidatesSlotMask is one bit per equipment slot id, built from those rows.
// Neck and ring rows are gemstones: optional picks, so they never make a slot
// look missing. Trinkets have no permanent enchant at all.
inline bool SlotCanTakePermanentEnchant(int slot, uint32_t candidatesSlotMask)
{
    if (!SlotReported(slot))
        return false;
    switch (slot)
    {
        case kSlotNeck:
        case kSlotFinger1:
        case kSlotFinger2:
        case kSlotTrinket1:
        case kSlotTrinket2:
            return false;
        default:
            return (candidatesSlotMask & (uint32_t(1) << slot)) != 0;
    }
}

// Whether a bot expects a permanent enchant in a slot: the data says the slot
// takes one, and the ranged slot only counts for hunters (scopes). Every
// other class can carry something in ranged, but its empty state is a
// different problem from an unenchanted scope.
inline bool SlotWantsPermanentEnchant(int slot, uint8_t classId, uint32_t candidatesSlotMask)
{
    if (!SlotCanTakePermanentEnchant(slot, candidatesSlotMask))
        return false;
    if (slot == kSlotRanged)
        return classId == kClassHunter;
    return true;
}

// Whether the equipped item itself accepts the slot's permanent enchant. An
// off hand only takes one as a shield or a one-hand weapon; a held-in-off-hand
// frill does not.
inline bool ItemTakesSlotEnchant(int slot, uint32_t itemClass, uint32_t inventoryType)
{
    switch (slot)
    {
        case kSlotMainHand:
            return itemClass == kItemClassWeapon;
        case kSlotOffHand:
            if (inventoryType == kInvTypeShield)
                return true;
            return itemClass == kItemClassWeapon && inventoryType != kInvTypeTwoHand;
        case kSlotRanged:
            return true;
        default:
            return true;
    }
}

// A two-hander in the main hand leaves the off hand intentionally empty:
// report no item and no miss instead of a missing shield.
inline bool OffHandIsIntentional(uint32_t mainHandInventoryType)
{
    return mainHandInventoryType == kInvTypeTwoHand;
}

inline uint8_t SlotMiss(bool itemPresent, bool expectsPermanentEnchant, bool hasPermanentEnchant)
{
    if (!itemPresent)
        return kMissEmpty;
    if (expectsPermanentEnchant && !hasPermanentEnchant)
        return kMissEnchant;
    return kMissNone;
}

constexpr uint32_t kBossLevel = 63;

// Core formulas (cite in the command docs):
// - Unit::MeleeMissChanceCalc (src/game/Objects/Unit.cpp): 5% base, 0.2% per
//   weapon-skill point below the victim's defense skill for creatures.
// - WorldObject::GetDefenseSkillValue / GetUnitMeleeSkill: a level-63
//   creature has 63*5 = 315 defense.
// - WorldObject::MagicSpellHitChance (src/game/Objects/Object.cpp): against a
//   victim 3 levels higher, 94 - (3-2)*11 = 83% hit, so 17% miss; Turtle
//   floors the level-based hit chance at 33%.
// - Unit::GetUnitCriticalChance: a creature attacker starts at 5% crit and a
//   player victim loses 0.04% per defense point above the attacker's skill.
constexpr double kMeleeBaseMiss = 5.0;
constexpr double kMeleeMissPerSkillPoint = 0.2;
constexpr double kSpellHitBase = 94.0;
constexpr double kSpellHitPerLevelPastTwo = 11.0;
constexpr double kTurtleMinSpellHitChance = 33.0;
constexpr double kCreatureBaseCrit = 5.0;
constexpr double kCritReductionPerDefense = 0.04;
constexpr double kMeleeMissCap = 60.0;

inline double BossDefenseSkill(uint32_t bossLevel = kBossLevel)
{
    return double(bossLevel) * 5.0;
}

// Hit bonus a melee or ranged attacker needs for no miss at all, from that
// bot's actual weapon skill (the hit stat itself does not include it).
inline double MeleeHitCap(double weaponSkill, uint32_t bossLevel = kBossLevel)
{
    double miss = kMeleeBaseMiss - (weaponSkill - BossDefenseSkill(bossLevel)) * kMeleeMissPerSkillPoint;
    if (miss < 0.0)
        return 0.0;
    if (miss > kMeleeMissCap)
        return kMeleeMissCap;
    return miss;
}

// Spell hit bonus needed for no level-based miss.
inline double SpellHitCap(uint32_t botLevel, uint32_t bossLevel = kBossLevel)
{
    int32_t levelDiff = int32_t(bossLevel) - int32_t(botLevel);
    double hit = levelDiff < 3
        ? 96.0 - levelDiff
        : kSpellHitBase - (levelDiff - 2) * kSpellHitPerLevelPastTwo;
    if (hit < kTurtleMinSpellHitChance)
        hit = kTurtleMinSpellHitChance;
    // The core caps spell hit at 99%, so the last 1% of miss can never be removed.
    double miss = 100.0 - hit - 1.0;
    return miss < 0.0 ? 0.0 : miss;
}

// Defense skill that makes a level-63 boss unable to crit a tank.
inline uint32_t DefenseCritCap(uint32_t bossLevel = kBossLevel)
{
    double const needed = BossDefenseSkill(bossLevel) + kCreatureBaseCrit / kCritReductionPerDefense;
    return uint32_t(needed + 0.5);
}

// Which hit stat a role is measured by.
enum class HitKind : uint8_t
{
    None = 0,
    Melee,
    Ranged,
    Spell,
};

struct SpecRole
{
    char const* spec;
    char const* role;
    HitKind hitKind;
};

// The one spec read the gear code uses: AiFactory::GetKitSpecTab already
// folds the talents and a forced `.bot role` into a talent tab. tank
// separates the builds that share a tab (feral bear/cat, shaman tank).
inline SpecRole ResolveSpecRole(uint8_t classId, int tab, bool tank)
{
    switch (classId)
    {
        case kClassWarrior:
            if (tab == 0) return { "Arms", "melee", HitKind::Melee };
            if (tab == 1) return { "Fury", "melee", HitKind::Melee };
            if (tab == 2) return { "Protection", "tank", HitKind::Melee };
            break;
        case kClassPaladin:
            if (tab == 0) return { "Holy", "healer", HitKind::None };
            if (tab == 1) return { "Protection", "tank", HitKind::Melee };
            if (tab == 2) return { "Retribution", "melee", HitKind::Melee };
            break;
        case kClassHunter:
            if (tab == 0) return { "Beast Mastery", "ranged", HitKind::Ranged };
            if (tab == 1) return { "Marksmanship", "ranged", HitKind::Ranged };
            if (tab == 2) return { "Survival", "ranged", HitKind::Ranged };
            break;
        case kClassRogue:
            if (tab == 0) return { "Assassination", "melee", HitKind::Melee };
            if (tab == 1) return { "Combat", "melee", HitKind::Melee };
            if (tab == 2) return { "Subtlety", "melee", HitKind::Melee };
            break;
        case kClassPriest:
            if (tab == 0) return { "Discipline", "healer", HitKind::None };
            if (tab == 1) return { "Holy", "healer", HitKind::None };
            if (tab == 2) return { "Shadow", "caster", HitKind::Spell };
            break;
        case kClassShaman:
            if (tab == 0) return { "Elemental", "caster", HitKind::Spell };
            if (tab == 1)
                return tank ? SpecRole{ "Enhancement", "tank", HitKind::Melee }
                            : SpecRole{ "Enhancement", "melee", HitKind::Melee };
            if (tab == 2) return { "Restoration", "healer", HitKind::None };
            break;
        case kClassMage:
            if (tab == 0) return { "Arcane", "caster", HitKind::Spell };
            if (tab == 1) return { "Fire", "caster", HitKind::Spell };
            if (tab == 2) return { "Frost", "caster", HitKind::Spell };
            break;
        case kClassWarlock:
            if (tab == 0) return { "Affliction", "caster", HitKind::Spell };
            if (tab == 1) return { "Demonology", "caster", HitKind::Spell };
            if (tab == 2) return { "Destruction", "caster", HitKind::Spell };
            break;
        case kClassDruid:
            if (tab == 0) return { "Balance", "caster", HitKind::Spell };
            if (tab == 1)
                return tank ? SpecRole{ "Feral", "tank", HitKind::Melee }
                            : SpecRole{ "Feral", "melee", HitKind::Melee };
            if (tab == 2) return { "Restoration", "healer", HitKind::None };
            break;
        default:
            break;
    }
    return { "-", "-", HitKind::None };
}

// The hit goal for one spec read; healers have no goal.
inline double HitCap(HitKind kind, double weaponSkill, uint32_t botLevel, uint32_t bossLevel = kBossLevel)
{
    switch (kind)
    {
        case HitKind::Melee:
        case HitKind::Ranged:
            return MeleeHitCap(weaponSkill, bossLevel);
        case HitKind::Spell:
            return SpellHitCap(botLevel, bossLevel);
        case HitKind::None:
            break;
    }
    return 0.0;
}

} // namespace Armory
} // namespace TortoiseBots
