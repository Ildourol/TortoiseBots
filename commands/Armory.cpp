// pi-lens-ignore-file: all
#include "BotCommandHandlers.h"
#include "CommandShared.h"
#include "BotCommandContext.h"
#include "../runtime/PlayerbotAIStorage.h"
#include "../runtime/ArmoryStats.h"
#include "../runtime/policy/armory/ArmoryPolicy.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "../ai/playerbot/AiFactory.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "../ai/playerbot/PlayerbotAI.h"
#include "../ai/playerbot/RandomItemMgr.h"
// pi-lens-ignore: clang:pp_file_not_found
#include "Chat.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Item.h"
#include "Group.h"
#include <cctype>
#include <cstdio>
#include <string>
#include <unordered_set>
#include <vector>

namespace TortoiseBots {
namespace BotCommands {
namespace {

std::string LowerName(std::string value)
{
    for (char& c : value)
        c = char(std::tolower(static_cast<unsigned char>(c)));
    return value;
}

// The Armory set: the claimed snapshot's rows (claimed guild bots plus the
// requester's autonomous alts in the guild) and the controllable party bots,
// deduplicated by guid.
std::vector<uint32> ArmoryGuids(Player* requester)
{
    std::vector<uint32> guids;
    if (!requester)
        return guids;

    std::unordered_set<uint32> seen;
    auto add = [&](uint32 guidLow)
    {
        if (guidLow && seen.insert(guidLow).second)
            guids.push_back(guidLow);
    };

    std::vector<OwnedCharacter> const rows = RosterOwnedCharacters(requester);
    ClaimedRosterSnapshot const claimed = BuildClaimedRoster(requester, rows);
    for (uint32 guidLow : claimed.guids)
        add(guidLow);
    for (Player* bot : BuildContext(requester).partyBots)
        if (bot)
            add(bot->GetGUIDLow());
    return guids;
}

struct ArmoryRow
{
    std::string name = "-";
    uint32 classId = 0;
    uint32 level = 0;
    bool online = false;
    bool inGroup = false;
};

void SendArmoryRow(ChatHandler* handler, ArmoryRow const& row, Player* bot, uint32 enchantSlotMask)
{
    Armory::SpecRole spec = { "-", "-", Armory::HitKind::None };
    Armory::Stats stats;
    double hitCap = 0.0;
    uint32 defenseCap = 0;
    uint32 averageIlvl = 0;
    std::string statsLine = "-";
    std::string equipment;

    if (row.online && bot)
    {
        PlayerbotAI* ai = PlayerbotAIStorage::Instance().GetAI(bot);
        int const tab = AiFactory::GetKitSpecTab(bot, ai);
        spec = Armory::ResolveSpecRole(bot->GetClass(), tab, PlayerbotAI::IsTank(bot));
        stats = Armory::ReadStats(bot);

        double weaponSkill = 0.0;
        if (spec.hitKind == Armory::HitKind::Melee)
            weaponSkill = bot->GetWeaponSkillValue(BASE_ATTACK);
        else if (spec.hitKind == Armory::HitKind::Ranged)
            weaponSkill = bot->GetWeaponSkillValue(RANGED_ATTACK);
        hitCap = Armory::HitCap(spec.hitKind, weaponSkill, bot->GetLevel());
        if (std::string(spec.role) == "tank")
            defenseCap = Armory::DefenseCritCap();

        uint32 ilvlSum = 0;
        uint32 ilvlCount = 0;
        Item* mainHand = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, Armory::kSlotMainHand);
        bool const mainHandTwoHanded = mainHand &&
            Armory::OffHandIsIntentional(mainHand->GetProto()->InventoryType);
        for (int slot = 0; slot <= Armory::kMaxEquipmentSlot; ++slot)
        {
            if (!Armory::SlotReported(slot))
                continue;
            Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
            ItemPrototype const* proto = item ? item->GetProto() : nullptr;
            uint32 itemId = proto ? proto->ItemId : 0;
            uint32 enchantId = item ? item->GetEnchantmentId(PERM_ENCHANTMENT_SLOT) : 0;
            uint8 miss = Armory::kMissNone;

            if (slot == Armory::kSlotOffHand && mainHandTwoHanded)
            {
                itemId = 0;
                enchantId = 0;
            }
            else
            {
                bool const expects = proto &&
                    Armory::SlotWantsPermanentEnchant(slot, row.classId, enchantSlotMask) &&
                    Armory::ItemTakesSlotEnchant(slot, proto->Class, proto->InventoryType);
                miss = Armory::SlotMiss(proto != nullptr, expects, enchantId != 0);
                if (proto)
                {
                    ilvlSum += proto->ItemLevel;
                    ++ilvlCount;
                }
            }

            if (!equipment.empty())
                equipment += ",";
            equipment += std::to_string(slot) + ":" + std::to_string(itemId) + ":" +
                std::to_string(enchantId) + ":" + std::to_string(uint32(miss));
        }
        averageIlvl = ilvlCount ? (ilvlSum + ilvlCount / 2) / ilvlCount : 0;

        char sheet[320];
        std::snprintf(sheet, sizeof(sheet),
            "ap=%d,crit=%.1f,armor=%d,sta=%d,sp=%d,heal=%d,scrit=%.1f,mp5=%d,int=%d",
            int(stats.attackPower), double(stats.crit), int(stats.armor), int(stats.stamina),
            int(stats.SpellPower()), int(stats.healingPower), double(stats.spellCrit),
            int(stats.manaRegen), int(stats.intellect));
        statsLine = sheet;
    }

    handler->PSendSysMessage(
        "TBM:ARMORY|%s|%u|%u|%u|%u|%s|%s|%u|%.1f|%.1f|%.1f|%u|%d+%d+%d+%d+%d|%.1f|%u|%s",
        ProtocolSafe(row.name).c_str(), row.classId, row.level, row.online ? 1u : 0u,
        row.inGroup ? 1u : 0u, spec.role, spec.spec, averageIlvl,
        double(stats.meleeHit), double(stats.rangedHit), double(stats.spellHit),
        row.online ? stats.defense : 0u,
        row.online ? stats.fireResist : 0, row.online ? stats.natureResist : 0,
        row.online ? stats.frostResist : 0, row.online ? stats.shadowResist : 0,
        row.online ? stats.arcaneResist : 0,
        row.online ? hitCap : 0.0, row.online ? defenseCap : 0u, statsLine.c_str());

    if (row.online && bot)
        handler->PSendSysMessage("TBM:ARMORY_EQ|%s|%s", ProtocolSafe(row.name).c_str(),
            equipment.c_str());
}

} // namespace

bool HandleArmory(ChatHandler* handler, char const* args)
{
    Player* requester = Requester(handler);
    if (!requester || !requester->GetSession())
    {
        handler->PSendSysMessage("You must be in-game to read the armory.");
        return true;
    }

    std::string wanted = Trim(args ? args : "");
    std::vector<uint32> guids = ArmoryGuids(requester);

    if (!wanted.empty())
    {
        std::string const lowerWanted = LowerName(wanted);
        uint32 match = 0;
        for (uint32 guidLow : guids)
        {
            Player* bot = sObjectAccessor.FindPlayer(ObjectGuid(HIGHGUID_PLAYER, guidLow));
            std::string name;
            if (bot)
                name = bot->GetName();
            else if (PlayerCacheData const* data = sObjectMgr.GetPlayerDataByGUID(guidLow))
                name = data->sName;
            if (!name.empty() && LowerName(name) == lowerWanted)
            {
                match = guidLow;
                break;
            }
        }
        if (!match)
        {
            SendActionError(handler, "armory", "no-bot",
                ("'" + wanted + "' is not in your armory (claimed guild bots and party bots only).").c_str());
            return true;
        }
        guids.assign(1, match);
    }

    uint32 const enchantSlotMask = sRandomItemMgr.GetPermanentEnchantSlotMask();
    handler->PSendSysMessage("TBM:ARMORY_BEGIN|%u", static_cast<uint32>(guids.size()));
    for (uint32 guidLow : guids)
    {
        ArmoryRow row;
        ObjectGuid const guid(HIGHGUID_PLAYER, guidLow);
        BotRecord* record = BotManager::Instance().FindBot(guid);
        Player* bot = sObjectAccessor.FindPlayer(guid);
        row.online = bot && record && IsLiveHeadlessBot(bot, record);
        row.inGroup = requester->GetGroup() && requester->GetGroup()->IsMember(guid);

        if (row.online)
        {
            row.name = bot->GetName();
            row.classId = bot->GetClass();
            row.level = bot->GetLevel();
        }
        else if (PlayerCacheData const* data = sObjectMgr.GetPlayerDataByGUID(guidLow))
        {
            row.name = data->sName.empty() ? "-" : data->sName;
            row.classId = data->uiClass;
            row.level = data->uiLevel;
        }

        SendArmoryRow(handler, row, row.online ? bot : nullptr, enchantSlotMask);
    }
    handler->PSendSysMessage("TBM:ARMORY_END|%u", static_cast<uint32>(guids.size()));
    return true;
}

} // namespace BotCommands
} // namespace TortoiseBots
