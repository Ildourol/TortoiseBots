#pragma once

#include <cstdint>

// One rule for every gear path that takes an item off a bot (fresh-seed
// cleanup, level top-up, respec refit, hire provisioning, `.bot gear`,
// `.bot autogear`, `.autogear`, gear progression): a piece a player is behind
// goes to the bags, then to the bot's own mailbox; it is never silently
// destroyed. Only a masterless pool bot keeps the old destroy behaviour, so a
// bulk re-seed does not stuff bags with replaced starter gear.
//
// The decision is a pure function, unit-tested in
// tools/test_replaced_gear_policy.cpp; runtime/ReplacedGear.cpp owns the
// bag/mail move and gathers the facts from PlayerBotClassification's sources.

class Player;
class Item;

namespace TortoiseBots
{

// isBot: the character runs on a module headless session (a real player always
//   keeps what they wore).
// isPoolBot: a module random-pool record on a registered pool account.
// hasLiveRealPlayerMaster: the bot's bound master is online with a network
//   session (hired companions, adopted party bots).
// isHired / isClaimed: the hire ledger / claim registry own the character.
// groupedWithRealPlayer: a network player shares the bot's group.
inline bool KeepsReplacedGear(bool isBot, bool isPoolBot, bool hasLiveRealPlayerMaster,
    bool isHired, bool isClaimed, bool groupedWithRealPlayer)
{
    if (!isBot)
        return true;
    return !isPoolBot || hasLiveRealPlayerMaster || isHired || isClaimed ||
        groupedWithRealPlayer;
}

enum class ReplacedGearOutcome : uint8_t
{
    Destroyed, // masterless pool bot, or nothing to move
    Stored,    // kept in the bags
    Mailed,    // bags full: the bot's own mailbox, no expiry
};

// Takes `item` out of wherever it sits (an equipment slot or a bag) and keeps
// it under the rule above. The caller must not touch the item afterwards.
ReplacedGearOutcome ReleaseReplacedItem(Player* bot, Item* item);

} // namespace TortoiseBots
