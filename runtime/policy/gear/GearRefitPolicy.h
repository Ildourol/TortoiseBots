#pragma once

// How a bot was last geared, and what an explicit talent change does about it.
//
// A bot pinned by `.bot gear tier N ilvl=X` wears a curated hire kit; a bot
// provisioned with a hire tier wears the same kit; everyone else is geared
// from the random item pool. A talent change used to re-roll the pool for any
// bot without a hire ledger tier, so a pinned protection warrior that went
// protection after `.bot gear` came out of the pool pass in the wrong slots
// (two one-handers, an empty ring). The remembered mode now decides the refit:
// the same tier kit under the same cap, or a pool pass under the recorded cap.
//
// Pure policy: no core/engine headers, unit-tested standalone
// (tools/test_gear_refit_policy.cpp).

#include <cstdint>

namespace TortoiseBots
{

// The last gear pass recorded for a bot.
//   tier     0 = random pool, 1-7 = curated hire kit at that tier
//   ilvlCap  0 = no cap of its own (each mode's own ceiling applies)
struct GearMode
{
    std::uint8_t tier = 0;
    std::uint32_t ilvlCap = 0;
};

enum class GearRefit : std::uint8_t
{
    None,        // no explicit talent change, or nothing this module may gear
    Pool,        // pool pass, same rules as a pool seed/regear
    PoolWithCap, // pool pass under the recorded item-level cap
    Kit,         // curated tier kit at the recorded tier (and cap)
};

// Damage-type-free decision for ChangeTalentsAction.
//   explicitTalentChange  the player asked for a build (`.bot talents ...`),
//                         not the autonomous level-up convergence
//   randomPoolBot         the bot belongs to the random pool (organic gear)
//   hasMode               a gear-mode row exists for this bot
// A bot without a row keeps the old behaviour: only pool bots are re-geared
// on an explicit respec, everyone else keeps what they wear.
inline GearRefit DecideGearRefit(bool explicitTalentChange, bool randomPoolBot,
    bool hasMode, GearMode const& mode)
{
    if (!explicitTalentChange)
        return GearRefit::None;
    if (hasMode)
    {
        if (mode.tier > 0)
            return GearRefit::Kit;
        if (mode.ilvlCap > 0)
            return GearRefit::PoolWithCap;
        return GearRefit::Pool;
    }
    return randomPoolBot ? GearRefit::Pool : GearRefit::None;
}

} // namespace TortoiseBots
