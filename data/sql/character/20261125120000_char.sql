-- Remembered gear mode per bot: how the bot was last geared.
--
-- One row per bot pinned by `.bot gear` or provisioned with a curated hire
-- kit. `tier` 1-7 is the curated hire tier; `tier` 0 with an `ilvl_cap` is the
-- random pool under that item-level cap (a `.bot gear tier 0 ilvl=N` pin). No
-- row means the plain pool. A talent change refits the bot in the recorded
-- mode instead of always re-rolling pool gear, and `ilvl_cap` keeps the
-- equipment audit from putting an above-cap piece back on after a restart.
--
-- Loaded once at startup and kept in memory; writes only on a gear pass or a
-- character deletion. Never edit this file once applied.

CREATE TABLE IF NOT EXISTS `tortoise_bots_gear_mode` (
  `guid` int unsigned NOT NULL,
  `tier` tinyint unsigned NOT NULL DEFAULT 0,
  `ilvl_cap` int unsigned NOT NULL DEFAULT 0,
  `updated` int unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
