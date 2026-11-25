"""The glue of `.bot gear` (raid task A7): the GM-gated handler parses with
BotGear::Parse, the kit pass honours the item-level cap and the raid gear
service purges bagged upgrades and pins what it set.
"""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
BOTCOMMANDS = (ROOT / "commands" / "BotCommands.cpp").read_text()
PROVISION_CPP = (ROOT / "runtime" / "HireProvisionService.cpp").read_text()
PROVISION_H = (ROOT / "runtime" / "HireProvisionService.h").read_text()
SERVICE = (ROOT / "runtime" / "BotGearService.cpp").read_text()
CONFIG = (ROOT / "ai/playerbot/PlayerbotAIConfig.cpp").read_text()
TEMPLATE = (ROOT / "ai/playerbot/aiplayerbot.conf.dist.in").read_text()
POLICY = (ROOT / "runtime" / "policy" / "hire" / "BotGearPolicy.h").read_text()


def strip_comments(text):
    return "\n".join(line.split("//", 1)[0].rstrip() for line in text.splitlines())


def body(text, start, end):
    i = text.index(start)
    return text[i:text.index(end, i + len(start))]


class BotGearWiringTest(unittest.TestCase):
    def test_handler_is_gm_gated_before_parse(self):
        handle = strip_comments(body(BOTCOMMANDS, "bool HandleBotGear(", "\n}\n"))
        self.assertIn("IsBotAdministrator(requester)", handle)
        self.assertIn("IsGmCliHandler(handler)", handle)
        self.assertIn("BotGear::Parse(", handle)
        self.assertLess(handle.index("IsBotAdministrator(requester)"), handle.index("BotGear::Parse("))
        self.assertLess(handle.index("IsGmCliHandler(handler)"), handle.index("BotGear::Parse("))

    def test_command_row(self):
        self.assertIn('{"gear", HandleBotGear,', strip_comments(BOTCOMMANDS))

    def test_kit_honours_the_cap(self):
        self.assertIn("BotGear::RowAllowed(", PROVISION_CPP)
        self.assertIn("cappedSlots->push_back(", PROVISION_CPP)

    def test_pool_pass_fills_only_kit_uncovered_slots(self):
        # A pool pass over a slot the kit covers displaces the piece the bot
        # already wore, and the kit then creates a fresh copy: the bags grow and
        # every repeat mails the old piece. Only the uncovered slots may be
        # filled, and a kit row the bot already owns is re-equipped, never
        # duplicated (a max_count 1 row is otherwise refused and skipped).
        self.assertIn("PoolFillSlotMask(", strip_comments(SERVICE))
        self.assertIn("AutogearOwned(ITEM_QUALITY_EPIC, m_job.args.ilvlCap, hasKit ? poolSlots : 0)", strip_comments(SERVICE))
        talents = strip_comments((ROOT / "ai/playerbot/strategy/actions/ChangeTalentsAction.cpp").read_text())
        self.assertIn("PoolFillSlotMask(", talents)
        self.assertIn("AutogearOwned(ITEM_QUALITY_EPIC, mode.ilvlCap, poolSlots)", talents)
        kit = strip_comments(PROVISION_CPP)
        self.assertIn("FindBagCopy(bot, row->itemId)", kit)
        self.assertIn("bot->SwapItem(owned->GetPos()", kit)

    def test_kit_moves_a_copy_worn_in_the_paired_slot(self):
        # A max_count 1 kit row whose copy is worn in the paired ring/trinket
        # slot cannot be equipped a second time; the pass must move that copy
        # into the row's slot instead of skipping the row.
        kit = strip_comments(PROVISION_CPP)
        self.assertIn("FindWornCopyInOtherSlot(bot, row->itemId, slot, wornSlot)", kit)
        self.assertIn("bot->SwapItem(uint16((INVENTORY_SLOT_BAG_0 << 8) | wornSlot), uint16((INVENTORY_SLOT_BAG_0 << 8) | slot))", kit)

    def test_incremental_pass_keeps_what_it_already_wears(self):
        # Repeating `.bot gear` must change nothing: the shirt/tabard slots are
        # never re-rolled once worn, and a worn piece under the cap stays unless
        # the pool offers a strictly better one.
        factory = strip_comments((ROOT / "ai/playerbot/PlayerbotFactory.cpp").read_text())
        self.assertIn("ai::CosmeticSlotKeepsWorn(", factory)
        self.assertIn("IncrementalKeepsWornPiece(oldStatValue, newStatValue,", factory)

    def test_service_gears_then_purges(self):
        auto = SERVICE.index("AutogearOwned(ITEM_QUALITY_EPIC")
        kit = SERVICE.index("EquipHireGearSet(")
        purge = SERVICE.index("BotGear::PurgeBagItem(")
        save = SERVICE.index("SaveToDB()")
        self.assertLess(auto, kit)
        self.assertLess(kit, purge)
        self.assertLess(purge, save)
        self.assertIn("kGearBotsPerTick", SERVICE)
        self.assertIn("IsPinned(", SERVICE)

    def test_purge_destroys_only_what_the_call_created(self):
        # r2: pre-existing bag items survive; only a created-then-unused
        # item may be destroyed.
        self.assertIn("CollectCreatedItemGuids(", SERVICE)
        self.assertIn("BotGear::CreatedItemUnused(", SERVICE)
        self.assertIn("CreatedItemMayBeDestroyed", POLICY)
        purge = strip_comments(body(SERVICE, "auto purgeSlot = [&](uint8 bag, uint8 slot, Item* item)", "};"))
        self.assertIn("BotGear::PurgeBagItem(", purge)
        self.assertIn("BotGear::CreatedItemUnused(", purge)
        self.assertIn("bot->DestroyItem(bag, slot, true);", purge)
        self.assertIn("++kept;", purge)

    def test_before_snapshot_includes_worn_equipment(self):
        # r3 (Codex): an item the gear pass swaps off the body lands in a
        # bag; it must already be in `before`, or the purge would treat it
        # as created and could destroy it.
        owned = strip_comments(body(SERVICE, "void CollectOwnedItemGuids(Player* bot, std::set<uint32_t>& out)", "\n}"))
        self.assertIn("CollectBagItemGuids(bot, out);", owned)
        self.assertIn("EQUIPMENT_SLOT_START", owned)
        self.assertIn("EQUIPMENT_SLOT_END", owned)
        code = strip_comments(SERVICE)
        self.assertIn("CollectOwnedItemGuids(bot, before);", code)
        self.assertNotIn("CollectBagItemGuids(bot, before);", code)

    def test_worn_above_cap_is_unequipped_not_destroyed(self):
        self.assertIn("UnequipAboveCapWorn(", SERVICE)
        self.assertIn("BotGear::WornAboveCap(", SERVICE)
        wear = strip_comments(body(SERVICE, "uint32_t BotGearService::UnequipAboveCapWorn(Player* bot, uint32_t cap)", "\n}\n"))
        self.assertIn("bot->RemoveItem(", wear)
        self.assertIn("bot->StoreItem(", wear)
        self.assertNotIn("DestroyItem", wear)

    def test_gear_mode_persists_and_refits(self):
        # The mode a bot was last geared in survives a restart, and a respec
        # refits that mode (kit under the same cap, or the pool under the
        # recorded cap) instead of re-rolling the pool.
        adapter = (ROOT / "host" / "BotGearAdapter.cpp").read_text()
        self.assertIn("BotGearService::Instance().Load()", adapter)
        self.assertIn("RememberGearMode(", SERVICE)
        self.assertIn("RememberGearMode(", PROVISION_CPP)
        talents = strip_comments((ROOT / "ai/playerbot/strategy/actions/ChangeTalentsAction.cpp").read_text())
        self.assertIn("BotGearService::Instance().GearMode(", talents)
        self.assertIn("DecideGearRefit(", talents)
        # An already-recorded tier refits with the master switch off.
        self.assertIn("RecordedHireGearTierUsable(", talents)
        # A capped refit backfills skipped kit slots and ends under the cap.
        self.assertIn("AutogearOwned(ITEM_QUALITY_EPIC", talents)
        self.assertIn("UnequipAboveCapWorn(", talents)

    def test_pin_holds_the_cap_in_the_equip_path(self):
        equip = (ROOT / "ai/playerbot/strategy/actions/EquipAction.cpp").read_text()
        self.assertIn("BotGearService::Instance().PinnedCap(", equip)
        self.assertIn("ITEM_CLASS_WEAPON", equip)
        self.assertIn("ITEM_CLASS_ARMOR", equip)

    def test_skip_reasons_and_null_ai(self):
        svc = strip_comments(SERVICE)
        self.assertIn('outcome.skip = "offline";', svc)
        self.assertIn('outcome.skip = "not-bot";', svc)
        self.assertIn('outcome.skip = "not-in-world";', svc)
        self.assertIn('outcome.skip = "no-ai";', svc)

    def test_pin_survives_logout_and_ends_on_dismiss_or_delete(self):
        mgr = (ROOT / "runtime/BotManager.cpp").read_text()
        self.assertNotIn("BotGearService::Instance().OnBotLogout(", mgr)
        hire = (ROOT / "runtime/HireLifecycle.cpp").read_text()
        self.assertIn("BotGearService::Instance().UnpinBot(", hire)
        cleanup = (ROOT / "runtime/CharacterCleanup.cpp").read_text()
        self.assertIn("BotGearService::Instance().UnpinBot(", cleanup)
        # Synchronous there: startup recovery reads the table back before the
        # async queue drains, and a recycled guid must not lose a new pin.
        self.assertIn("UnpinBot(guidLow, true)", cleanup)

    def test_config_default_matches_header(self):
        self.assertIn('AiPlayerbot.BotGearPresets', CONFIG)
        self.assertIn("BotGear::kPresetsDefault", CONFIG)
        match = re.search(r'kPresetsDefault\s*=\s*((?:"[^"]*"\s*)+);', POLICY)
        self.assertIsNotNone(match)
        literal = "".join(re.findall(r'"([^"]*)"', match.group(1)))
        for line in TEMPLATE.splitlines():
            if line.startswith("AiPlayerbot.BotGearPresets ="):
                self.assertEqual(line.split("=", 1)[1].strip(), literal)
                break
        else:
            self.fail("template has no active AiPlayerbot.BotGearPresets line")


if __name__ == "__main__":
    unittest.main()
