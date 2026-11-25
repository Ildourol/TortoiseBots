#pragma once

#include <cstdint>

// One rule for every gear path that takes an item off a bot (fresh-seed
// cleanup, level top-up, respec refit, hire provisioning, `.bot gear`,
// `.bot autogear`, `.autogear`, gear progression): a piece a player is behind
// goes to the bags, then to the bot's own mailbox; it is never silently
// destroyed. Only a masterless pool bot keeps the old destroy behaviour, so a
// bulk re-seed does not stuff bags with replaced starter gear.
//
// Gear that only exists because of the operation running right now is exempt
// and is destroyed instead: the outfit a brand-new hire arrives in, and any
// piece an earlier step of the same operation equipped and a later step
// replaces. See ThrowawayGearScope.
//
// The decisions are pure functions, unit-tested in
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

// Inside a ThrowawayGearScope: a release destroys the piece when the scope
// discards everything (initial provisioning) or when the piece did not exist
// when the scope opened (gear an earlier step of the same operation equipped).
inline bool DiscardsReplacedGear(bool inThrowawayScope, bool scopeDiscardsAll, bool ownedBeforeScope)
{
    return inThrowawayScope && (scopeDiscardsAll || !ownedBeforeScope);
}

enum class ReplacedGearOutcome : uint8_t
{
    Destroyed, // masterless pool bot, throwaway gear of the running operation, or nothing to move
    Stored,    // kept in the bags
    Mailed,    // bags full: the bot's own mailbox, the core's default 30-day expiry
};

// Opened around an operation that generates its own gear (hire provisioning,
// the two passes of `.bot gear`): what it equips is not something a player
// owned, so a later step that replaces it must not file it in the bags.
// destroyAll covers provisioning a fresh character, whose starting outfit is
// throwaway too. Gear the bot wore before the scope opened stays under the
// keep rule. One bot, one scope; the world thread owns them.
class ThrowawayGearScope
{
public:
    explicit ThrowawayGearScope(Player* bot, bool destroyAll = false);
    ~ThrowawayGearScope();
    ThrowawayGearScope(ThrowawayGearScope const&) = delete;
    ThrowawayGearScope& operator=(ThrowawayGearScope const&) = delete;

private:
    uint32_t m_guidLow;
};

// Takes `item` out of wherever it sits (an equipment slot or a bag) and keeps
// it under the rule above. The caller must not touch the item afterwards.
ReplacedGearOutcome ReleaseReplacedItem(Player* bot, Item* item);

} // namespace TortoiseBots
