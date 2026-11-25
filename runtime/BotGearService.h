#pragma once

// `.bot gear` job queue: gears bots one tick at a time so a 40-bot raid does
// not hitch the world. World thread only. The pure grammar, cap filter, bag
// purge decision and summary line live in policy/hire/BotGearPolicy.h.

// pi-lens-ignore: clang:pp_file_not_found
#include "ObjectGuid.h"

#include "policy/gear/GearRefitPolicy.h"
#include "policy/hire/BotGearPolicy.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class Player;
class PlayerbotAI;

namespace TortoiseBots
{

// How the bot was last geared, persisted in tortoise_bots_gear_mode so a
// talent change refits the same mode and the item-level cap survives a
// restart. See policy/gear/GearRefitPolicy.h.
using BotGearPin = GearMode;

class BotGearService
{
public:
    static BotGearService& Instance();

    struct Job
    {
        uint64_t requesterGuid = 0;
        std::vector<ObjectGuid> bots;
        BotGear::Args args;
    };

    struct BotOutcome
    {
        std::string name;
        std::string role;
        bool geared = false;
        std::string skip;
        float avgIlvl = 0;
        uint32_t cappedSlots = 0;
        uint32_t purged = 0;
        uint32_t kept = 0;
    };

    struct JobResult
    {
        uint8_t tier = 0;
        uint32_t ilvlCap = 0;
        std::vector<BotOutcome> bots;
    };

    bool Queue(uint64_t requesterGuid, std::vector<ObjectGuid> const& bots, BotGear::Args const& args);
    void Update(uint32_t diff);
    // Startup load of the remembered gear modes (tortoise_bots_gear_mode).
    void Load();

    bool Busy() const { return m_running; }
    bool IsPinned(uint32_t guidLow) const;
    // The mode a pinned bot was geared in; false when no pass recorded one.
    // ChangeTalentsAction refits this mode after a spec change.
    bool GearMode(uint32_t guidLow, BotGearPin& out) const;
    // Records the mode and persists it. Used by the `.bot gear` pass and by
    // hire kit provisioning (a tier kit is the same kind of mode).
    void RememberGearMode(uint32_t guidLow, BotGearPin mode);
    // The cap a pinned bot gears under (0 = no cap recorded). Consulted by
    // the bot's own auto-equip path so a pinned bot never puts an above-cap
    // item back on (raid-w1-r2 item 2).
    uint32_t PinnedCap(uint32_t guidLow) const;

    // Moves worn weapon/armor above `cap` back to the bags (never destroys;
    // a full bag keeps the piece worn). The `.bot gear` pass and a respec
    // refit both end with nothing worn above the cap.
    static uint32_t UnequipAboveCapWorn(Player* bot, uint32_t cap);

    // Clears the pin. The pin survives logout/relog; it ends only when a new
    // `.bot gear` pass overwrites it, the hire is dismissed, or the
    // character is deleted. `synchronous` is for character deletion: the row
    // is read back right away there, and the async queue could otherwise
    // delete a pin written for a character that reused the recycled guid.
    void UnpinBot(uint32_t guidLow, bool synchronous = false);

private:
    BotGearService() = default;

    struct ActiveJob
    {
        uint64_t requesterGuid = 0;
        std::vector<ObjectGuid> bots;
        BotGear::Args args;
        size_t next = 0;
        JobResult result;
    };

    void FinishJob();
    static void SendLines(uint64_t requesterGuid, std::string const& plain, std::string const& structured);
    static uint32_t WornMaxIlvl(Player* bot);
    static float AverageWornIlvl(Player* bot);

    bool m_running = false;
    ActiveJob m_job;
    std::unordered_map<uint32_t, BotGearPin> m_pinned;
};

} // namespace TortoiseBots
