#pragma once

// The live Player reads behind the Armory sheet: hit bonuses, defense,
// resistances and the sheet's main stats. Shared by the observability
// tortoise_bots_armory_stats row and `.bot armory`, so the dashboard and the
// addon always show the same numbers for the same bot.

#include <algorithm>
#include <cstdint>

// pi-lens-ignore: clang:pp_file_not_found
#include "Object.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "Player.h"

namespace TortoiseBots
{
namespace Armory
{

struct Stats
{
    // Hit bonus the core adds to its own miss roll (gear, talents, auras).
    float meleeHit = 0.0f;
    float rangedHit = 0.0f;
    float spellHit = 0.0f;
    uint32_t defense = 0;
    int32_t fireResist = 0;
    int32_t natureResist = 0;
    int32_t frostResist = 0;
    int32_t shadowResist = 0;
    int32_t arcaneResist = 0;
    int32_t attackPower = 0;
    float crit = 0.0f;       // melee crit, percent
    int32_t armor = 0;
    int32_t stamina = 0;
    int32_t intellect = 0;
    int32_t healingPower = 0;
    float spellCrit = 0.0f;  // percent
    int32_t manaRegen = 0;   // MP5
    // Indexed like SpellSchools: 0 = normal, 1 = holy ... 6 = arcane.
    int32_t spellDamage[7] = {};

    // Shared base spell damage: the lowest magic school; talents and gear
    // raise single schools on top.
    int32_t SpellPower() const
    {
        int32_t base = spellDamage[1];
        for (int school = 2; school < 7; ++school)
            base = std::min(base, spellDamage[school]);
        return base;
    }
};

inline Stats ReadStats(Player* bot)
{
    Stats stats;
    if (!bot)
        return stats;

    for (int school = SPELL_SCHOOL_HOLY; school < MAX_SPELL_SCHOOL; ++school)
        stats.spellDamage[school] = bot->SpellBaseDamageBonusDone(SpellSchoolMask(1 << school));
    // Talents raise single schools; the sheet shows the best one.
    float spellCrit = 0.0f;
    for (int school = SPELL_SCHOOL_HOLY; school < MAX_SPELL_SCHOOL; ++school)
        spellCrit = std::max(spellCrit, bot->GetSpellCritPercent(SpellSchools(school)));

    stats.meleeHit = bot->m_modMeleeHitChance;
    stats.rangedHit = bot->m_modRangedHitChance;
    stats.spellHit = bot->m_modSpellHitChance;
    stats.defense = bot->GetDefenseSkillValue();
    stats.fireResist = bot->GetResistance(SPELL_SCHOOL_FIRE);
    stats.natureResist = bot->GetResistance(SPELL_SCHOOL_NATURE);
    stats.frostResist = bot->GetResistance(SPELL_SCHOOL_FROST);
    stats.shadowResist = bot->GetResistance(SPELL_SCHOOL_SHADOW);
    stats.arcaneResist = bot->GetResistance(SPELL_SCHOOL_ARCANE);
    stats.attackPower = int32_t(bot->GetTotalAttackPowerValue(BASE_ATTACK));
    stats.crit = bot->GetFloatValue(PLAYER_CRIT_PERCENTAGE);
    stats.armor = bot->GetArmor();
    stats.stamina = bot->GetStat(STAT_STAMINA);
    stats.intellect = bot->GetStat(STAT_INTELLECT);
    stats.healingPower = bot->SpellBaseHealingBonusDone(SPELL_SCHOOL_MASK_HOLY);
    stats.spellCrit = spellCrit;
    stats.manaRegen = bot->GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_POWER_REGEN, POWER_MANA);
    return stats;
}

} // namespace Armory
} // namespace TortoiseBots
