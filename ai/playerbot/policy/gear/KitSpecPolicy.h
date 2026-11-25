#pragma once

// Pure rule behind AiFactory::GetKitSpecTab: which talent tree the bot's kit
// and gear are read from. The spent talents decide, except where a forced
// role names a role with a single tree of its own. `.bot role <bot> holy`
// (and the addon spec buttons) set the role without paying for a respec, so
// a retribution paladin kept buying strength gear after the player moved
// him to healing.

#include <cstdint>

namespace ai
{
    // ai::BotRoles bits, repeated so this header stays core-free.
    constexpr uint8_t KIT_SPEC_ROLE_TANK = 0x01;
    constexpr uint8_t KIT_SPEC_ROLE_HEALER = 0x02;
    constexpr uint8_t KIT_SPEC_ROLE_DPS = 0x04;

    // Classes (SharedDefines.h).
    constexpr uint8_t KIT_SPEC_CLASS_WARRIOR = 1;
    constexpr uint8_t KIT_SPEC_CLASS_PALADIN = 2;
    constexpr uint8_t KIT_SPEC_CLASS_ROGUE = 4;
    constexpr uint8_t KIT_SPEC_CLASS_PRIEST = 5;
    constexpr uint8_t KIT_SPEC_CLASS_SHAMAN = 7;
    constexpr uint8_t KIT_SPEC_CLASS_DRUID = 11;

    // talentTab is the tree with the most spent points (0..2).
    inline int ResolveKitSpecTab(uint8_t classId, int talentTab, uint8_t role)
    {
        if (!role)
            return talentTab;

        // One tanking tree per class: a forced tank plays it even with talent
        // points still in a damage tree, so the group that reads him as the
        // tank gets a kit that can hold the mobs.
        if (role == KIT_SPEC_ROLE_TANK)
        {
            if (classId == KIT_SPEC_CLASS_WARRIOR)
                return 2; // protection
            if (classId == KIT_SPEC_CLASS_PALADIN)
                return 1; // protection
            if (classId == KIT_SPEC_CLASS_DRUID)
                return 1; // feral
            return talentTab;
        }
        if (role & KIT_SPEC_ROLE_TANK)
            return talentTab;

        // One healing tree per class too (priest splits its healing points
        // over two): a forced healer is geared and fights as a healer.
        if (role & KIT_SPEC_ROLE_HEALER)
        {
            if (classId == KIT_SPEC_CLASS_PALADIN)
                return 0; // holy
            if (classId == KIT_SPEC_CLASS_DRUID)
                return 2; // restoration
            if (classId == KIT_SPEC_CLASS_SHAMAN)
                return 2; // restoration
            if (classId == KIT_SPEC_CLASS_PRIEST && talentTab == 2)
                return 1; // shadow points, healing role: holy
        }

        // A damage role leaves several trees to choose from, so the talents
        // keep the choice - except out of the one protection tree, which is
        // not a damage kit: `.bot role <bot> dps` on a protection warrior or
        // paladin kept "protection" strategies and taunted off the tank.
        if (classId == KIT_SPEC_CLASS_WARRIOR && talentTab == 2)
            return 0; // arms
        if (classId == KIT_SPEC_CLASS_PALADIN && talentTab == 1)
            return 2; // retribution
        return talentTab;
    }
}
