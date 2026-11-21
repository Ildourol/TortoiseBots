#include "../ai/playerbot/policy/loot/LootRollPolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using ai::IsWonLootDisposableUsage;
using ai::IsWonLootReliefEligible;
using ai::kWonLootPendingTimeoutSec;
using ai::LootMethodTakesRolls;
using ai::LootRollVote;
using ai::LootVoteIsEcho;
using ai::LootWireVote;
using ai::PoliteRollVote;
using ai::RealPlayerCompetes;
using ai::RecipeRollVote;
using ai::RollWaitElapsed;
using ai::RollWaitVoteMarginMs;
using ai::SelectWonLootReliefIndex;
using ai::TokenUsableByClass;
using ai::UniqueCopyOwned;
using ai::WonLootNeedsPendingPickup;
using ai::WonLootPendingExpired;
using ai::WonLootReliefCandidate;

int main()
{
    std::cout << "Starting TortoiseBots loot-roll gate tests...\n";

    // (1) Loot method: free-for-all (0) and master loot (2) take no auto
    // vote (donor passes); group (3) and need-before-greed (4) do.
    {
        CHECK(!LootMethodTakesRolls(0));
        CHECK(!LootMethodTakesRolls(2));
        CHECK(LootMethodTakesRolls(3));
        CHECK(LootMethodTakesRolls(4));
        std::cout << "  [PASS] FFA/master pass, group/NBG vote\n";
    }

    // (2) Recipe: learnable NEEDs, soulbound-unusable PASSes (nobody else
    // can use it), tradeable-unusable GREEDs for the AH.
    {
        CHECK(RecipeRollVote(true, true) == LootRollVote::Need);
        CHECK(RecipeRollVote(true, false) == LootRollVote::Need);
        CHECK(RecipeRollVote(false, true) == LootRollVote::Pass);
        CHECK(RecipeRollVote(false, false) == LootRollVote::Greed);
        std::cout << "  [PASS] recipe NEED/PASS/GREED split\n";
    }

    // (3) Duplicate unique: UNIQUE_EQUIPPED already worn, or a stack at
    // MaxCount, is owned (donor RollUniqueCheck demotes NEED to GREED).
    // Zero MaxCount means no cap; an unworn unique is not owned.
    {
        CHECK(UniqueCopyOwned(true, true, 1, 1));
        CHECK(UniqueCopyOwned(false, false, 5, 5));
        CHECK(UniqueCopyOwned(false, false, 3, 5) == false);
        CHECK(UniqueCopyOwned(false, false, 3, 0) == false);
        CHECK(UniqueCopyOwned(false, true, 0, 0) == false);
        std::cout << "  [PASS] duplicate-unique ownership\n";
    }

    // (4) Class token: NEED only when the bot's class is in the
    // AllowableClass mask; an empty mask is unrestricted (all classes).
    // Warrior mask bit: class 1 -> bit 0.
    {
        const std::uint32_t warrior = 1u << (1 - 1);
        CHECK(TokenUsableByClass(warrior, warrior));
        CHECK(!TokenUsableByClass(warrior << 2, warrior));
        CHECK(TokenUsableByClass(0, warrior));
        std::cout << "  [PASS] token class-mask gate\n";
    }

    // (5) Real player in the roll: a seen NEED competes, a seen GREED or
    // PASS steps aside even for an item they could use. An unseen vote
    // never competes on bare usability alone (#967: in an LFT/pool group
    // every usable drop is usable by someone, which demoted every upgrade
    // NEED to GREED); only a hired companion grouped with its master yields
    // those. Alts wait like pool bots instead.
    {
        CHECK(RealPlayerCompetes(true, LootRollVote::Need, false));
        CHECK(!RealPlayerCompetes(true, LootRollVote::Greed, true));
        CHECK(!RealPlayerCompetes(true, LootRollVote::Pass, true));
        CHECK(!RealPlayerCompetes(false, LootRollVote::Pass, true));
        CHECK(!RealPlayerCompetes(false, LootRollVote::Pass, false));
        CHECK(RealPlayerCompetes(false, LootRollVote::Pass, true, true));
        CHECK(!RealPlayerCompetes(false, LootRollVote::Pass, false, true));
        std::cout << "  [PASS] real-player competition\n";
    }

    // (6) Polite vote: NEED steps down to GREED against a competing player
    // unless the player forced NEED; GREED/PASS untouched; no competitor,
    // no change.
    {
        CHECK(PoliteRollVote(LootRollVote::Need, true, false) == LootRollVote::Greed);
        CHECK(PoliteRollVote(LootRollVote::Need, true, true) == LootRollVote::Need);
        CHECK(PoliteRollVote(LootRollVote::Need, false, false) == LootRollVote::Need);
        CHECK(PoliteRollVote(LootRollVote::Greed, true, false) == LootRollVote::Greed);
        CHECK(PoliteRollVote(LootRollVote::Pass, true, false) == LootRollVote::Pass);
        std::cout << "  [PASS] never NEED against a player\n";
    }

    // (7) Roll wait (#967): the wait ends when every real roller voted, or
    // when the window ends within the 15 s margin (clears the 5 s "very
    // often" re-check gap plus tick jitter under load); unknown remaining
    // time never ends it on its own (the call site votes at once instead).
    {
        CHECK(RollWaitVoteMarginMs() == 15000);
        CHECK(RollWaitElapsed(60000, false) == false);
        CHECK(RollWaitElapsed(15001, false) == false);
        CHECK(RollWaitElapsed(15000, false));
        CHECK(RollWaitElapsed(5000, false));
        CHECK(RollWaitElapsed(1000, false));
        CHECK(RollWaitElapsed(60000, true));
        CHECK(RollWaitElapsed(0, false) == false);
        std::cout << "  [PASS] roll wait elapses on votes or margin\n";
    }

    // (8) Wire mapping (#967 M2): live votes are (number 0, type 0) NEED,
    // (128, 2) GREED, (128, 128) PASS per Group::CountRollVote; anything
    // with a real roll number is a post-resolution echo (CountTheRoll).
    {
        CHECK(LootWireVote(0, 0) == LootRollVote::Need);
        CHECK(LootWireVote(128, 2) == LootRollVote::Greed);
        CHECK(LootWireVote(128, 128) == LootRollVote::Pass);
        CHECK(LootWireVote(42, 1) == LootRollVote::Pass);
        CHECK(LootWireVote(87, 2) == LootRollVote::Pass);
        CHECK(!LootVoteIsEcho(0, 0));
        CHECK(!LootVoteIsEcho(128, 2));
        CHECK(!LootVoteIsEcho(128, 128));
        CHECK(LootVoteIsEcho(42, 1));
        CHECK(LootVoteIsEcho(87, 2));
        CHECK(LootVoteIsEcho(100, 0));
        std::cout << "  [PASS] wire vote mapping, echoes ignored\n";
    }

    // (9) Won-roll pending pickup detection and timeout:
    // - Only the winner schedules a pending pickup.
    // - When the loot source is inspected, an item that took the last free
    //   slot (already looted from the source) is NOT pending even though
    //   CanStoreNewItem now reports full bags afterwards.
    // - An item still sitting on the corpse/chest reserved for the winner IS
    //   pending.
    // - Entries expire after kWonLootPendingTimeoutSec (180 s).
    {
        CHECK(!WonLootNeedsPendingPickup(false, true, true, false));
        CHECK(!WonLootNeedsPendingPickup(true, true, false, false));
        CHECK(WonLootNeedsPendingPickup(true, true, true, false));
        CHECK(WonLootNeedsPendingPickup(true, false, false, false));
        CHECK(!WonLootNeedsPendingPickup(true, false, false, true));

        CHECK(kWonLootPendingTimeoutSec == 180);
        CHECK(!WonLootPendingExpired(1000, 1000));
        CHECK(!WonLootPendingExpired(1000, 1179));
        CHECK(WonLootPendingExpired(1000, 1180));
        CHECK(WonLootPendingExpired(0, 1000));
        std::cout << "  [PASS] won-roll pending pickup and timeout\n";
    }

    // (10) Won-roll bag relief selection:
    // - Grey trash goes first, cheapest stack first.
    // - When no grey item is available, lowest-vendor-value non-critical
    //   item (NONE/VENDOR/BROKEN_AH/AH usage) is chosen.
    // - Protected categories (autonomous alt, non-fitting specialty bag,
    //   force usage, guild order stock, quest item, bag, ammo, used
    //   reagent/consumable, profession tool, hearthstone, key, equip/skill/
    //   keep usages) are never destroyed.
    {
        CHECK(IsWonLootDisposableUsage(0));   // NONE
        CHECK(IsWonLootDisposableUsage(9));   // AH
        CHECK(IsWonLootDisposableUsage(10));  // BROKEN_AH
        CHECK(IsWonLootDisposableUsage(12));  // VENDOR
        CHECK(!IsWonLootDisposableUsage(1));  // EQUIP
        CHECK(!IsWonLootDisposableUsage(2));  // BAD_EQUIP
        CHECK(!IsWonLootDisposableUsage(3));  // BROKEN_EQUIP
        CHECK(!IsWonLootDisposableUsage(4));  // QUEST
        CHECK(!IsWonLootDisposableUsage(5));  // SKILL
        CHECK(!IsWonLootDisposableUsage(6));  // USE
        CHECK(!IsWonLootDisposableUsage(7));  // GUILD_TASK
        CHECK(!IsWonLootDisposableUsage(8));  // DISENCHANT
        CHECK(!IsWonLootDisposableUsage(11)); // KEEP
        CHECK(!IsWonLootDisposableUsage(13)); // AMMO
        CHECK(!IsWonLootDisposableUsage(14)); // FORCE_NEED
        CHECK(!IsWonLootDisposableUsage(15)); // FORCE_GREED

        WonLootReliefCandidate base{};
        base.bag = 255;
        base.slot = 23;
        base.itemId = 1001;
        base.quality = 0;
        base.stackSellValue = 50;
        base.usage = 12; // VENDOR
        base.bagFitsWonItem = true;
        CHECK(IsWonLootReliefEligible(base, false));
        CHECK(!IsWonLootReliefEligible(base, true)); // autonomous alt

        auto checkProtectedFlag = [&](auto mutate)
        {
            WonLootReliefCandidate c = base;
            mutate(c);
            CHECK(!IsWonLootReliefEligible(c, false));
        };
        checkProtectedFlag([](WonLootReliefCandidate& c) { c.bagFitsWonItem = false; });
        checkProtectedFlag([](WonLootReliefCandidate& c) { c.hasForceUsage = true; });
        checkProtectedFlag([](WonLootReliefCandidate& c) { c.isGuildOrderStock = true; });
        checkProtectedFlag([](WonLootReliefCandidate& c) { c.isQuestItem = true; });
        checkProtectedFlag([](WonLootReliefCandidate& c) { c.isBag = true; });
        checkProtectedFlag([](WonLootReliefCandidate& c) { c.isAmmo = true; });
        checkProtectedFlag([](WonLootReliefCandidate& c) { c.isReagentOrConsumableUsed = true; });
        checkProtectedFlag([](WonLootReliefCandidate& c) { c.isProfessionTool = true; });
        checkProtectedFlag([](WonLootReliefCandidate& c) { c.isHearthstone = true; });
        checkProtectedFlag([](WonLootReliefCandidate& c) { c.isKey = true; });
        checkProtectedFlag([](WonLootReliefCandidate& c) { c.usage = 1; }); // EQUIP

        // Grey beats even a cheaper white item; cheapest grey wins among greys.
        WonLootReliefCandidate cheapWhite = base;
        cheapWhite.itemId = 2001;
        cheapWhite.quality = 1;
        cheapWhite.stackSellValue = 5;

        WonLootReliefCandidate priceyGrey = base;
        priceyGrey.itemId = 2002;
        priceyGrey.quality = 0;
        priceyGrey.stackSellValue = 80;

        WonLootReliefCandidate cheapGrey = base;
        cheapGrey.itemId = 2003;
        cheapGrey.quality = 0;
        cheapGrey.stackSellValue = 25;

        WonLootReliefCandidate const withGreys[] = { cheapWhite, priceyGrey, cheapGrey };
        CHECK(SelectWonLootReliefIndex(withGreys, 3, false) == 2);
        CHECK(SelectWonLootReliefIndex(withGreys, 3, true) == -1);

        // Without grey items, lowest-vendor-value non-critical item wins.
        WonLootReliefCandidate midWhite = base;
        midWhite.itemId = 3001;
        midWhite.quality = 1;
        midWhite.stackSellValue = 40;

        WonLootReliefCandidate cheapVendorGreen = base;
        cheapVendorGreen.itemId = 3002;
        cheapVendorGreen.quality = 2;
        cheapVendorGreen.stackSellValue = 15;
        cheapVendorGreen.usage = 9; // AH

        WonLootReliefCandidate questItem = base;
        questItem.itemId = 3003;
        questItem.quality = 1;
        questItem.stackSellValue = 0;
        questItem.isQuestItem = true;

        WonLootReliefCandidate const noGreys[] = { midWhite, cheapVendorGreen, questItem };
        CHECK(SelectWonLootReliefIndex(noGreys, 3, false) == 1);

        // Only critical items -> -1.
        WonLootReliefCandidate const onlyCritical[] = { questItem };
        CHECK(SelectWonLootReliefIndex(onlyCritical, 1, false) == -1);
        std::cout << "  [PASS] won-roll bag relief selection\n";
    }

    std::cout << "All loot-roll gate checks PASSED!\n";
    return 0;
}
