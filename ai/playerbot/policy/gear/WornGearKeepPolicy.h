#pragma once

// Worn-piece retention for the incremental gear passes (InitEquipment).
//
// A pool pass that keeps running over a slot the bot already fills re-rolls it:
// the worn piece is released to the bags and a near-identical one takes its
// place, so every `.bot gear` (and every respec refit) grows the bags without
// the bot getting better. Two rules stop that, and each is pure so
// tools/test_worn_gear_keep_policy.cpp can pin it.

#include <cstdint>

namespace ai
{
    // Cosmetic slots (shirt, tabard) are never re-rolled once worn: a random
    // swap changes nothing about the bot's stats and only files the old copy in
    // the bags. An empty cosmetic slot is still filled.
    inline bool CosmeticSlotKeepsWorn(bool hasWorn)
    {
        return hasWorn;
    }

    // Whether an incremental pass keeps the piece a slot already wears instead
    // of taking the candidate. A candidate wins only when it scores strictly
    // higher; a worn piece the item-level cap allows is kept on a tie or a
    // worse score too, because the previous pass already picked the best
    // candidate it had. Above the cap the pre-existing rule applies: a worn
    // piece scoring 1 or less is replaced by anything with real stats, so a
    // bot stuck with a stat-less piece does not keep it forever.
    inline bool IncrementalKeepsWornPiece(std::uint32_t oldScore, std::uint32_t newScore,
        bool wornLegalUnderCap)
    {
        if (newScore > oldScore)
            return false;
        return wornLegalUnderCap || oldScore > 1;
    }
}
