---
type: Reference
title: Features and Gameplay Systems
description: Party bots, class rotations, dungeons/raids, living world, economy, PvP, and observability.
tags: [features, gameplay, classes, dungeons, economy]
status: stable
generated: { by: "antigravity/gemini", at: "2026-11-25T14:53:00Z" }
sources:
  - id: features-doc
    resource: https://github.com/TortoiseBots/TortoiseBots/blob/main/docs/play/features.md
    title: TortoiseBots feature catalog
  - id: commands-doc
    resource: https://github.com/TortoiseBots/TortoiseBots/blob/main/docs/play/commands/README.md
    title: Player .bot command reference
---

# Gameplay Systems

- **Party & mercenaries:** Summon your own account alts or hire mercenaries at inn recruiters; command formations, follow behaviour, and tactical orders via `.bot` commands or the `/tbm` addon.[^features-doc][^commands-doc]
- **Combat & classes:** Nine-class combat rotations, spec-based roles, interrupt coordination, and Turtle WoW 1.18.1 custom abilities.
- **Dungeons & raids:** Corner pullback line-of-sight pulls, CC protection, boss encounter tactics, hazard avoidance, and wipe recovery.
- **Living world:** Autonomous pool bots level, quest, grind, train spells, gather materials, and travel by foot, boat, and flight path.
- **Social & economy:** Roaming bots invite solo questers, join guilds, execute guild farm/buy/craft orders, and buy/sell real loot on the Auction House (enabled by default).
- **Dungeon Finder & PvP:** Looking for Turtles (LFT) role backfill and WSG, AB, and AV battleground matchmaking.
- **Monitoring:** Real-time 2D browser map, Prometheus metrics, and structured CSV logs.

Related: [Vision and Architecture](../architecture/overview.md), [Build and Configuration](../operations/build-and-config.md).

[^features-doc]: TortoiseBots feature catalog
[^commands-doc]: Player .bot command reference
