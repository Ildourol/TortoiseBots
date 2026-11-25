---
type: Architecture
title: Vision and Architecture
description: Core design principles, decoupled module boundary, and headless bot sessions.
tags: [architecture, vision, playerbots, vanilla]
status: stable
generated: { by: "antigravity/gemini", at: "2026-11-25T14:53:00Z" }
sources:
  - id: arch-doc
    resource: https://github.com/TortoiseBots/TortoiseBots/blob/main/docs/developers/architecture.md
    title: TortoiseBots architecture invariants
  - id: notice
    resource: https://github.com/TortoiseBots/TortoiseBots/blob/main/NOTICE.md
    title: Attribution and licensing notice
---

# Design Vision

TortoiseBots makes a solo **Tortoise WoW 1.18.1** server feel like a living, populated vanilla realm:[^arch-doc]

- **Organic living world:** Pool bots quest, grind, and travel by foot, flight path, and boat (levels 5-60 by default, or all starting at level 1). Long-range teleports are a stuck-recovery fallback, not routine travel.
- **Earned progression:** Pool bots receive a starter outfit and coin once on creation; equipment, gold, and Auction House listings come from playing the game.
- **Player control first:** Explicit player orders always override automation; extra automation for owned bots is opt-in.
- **Modern AI standard:** Adapts modern `mod-playerbots` combat and class AI to Vanilla 1.12 and Turtle WoW 1.18.1 content.

# Decoupled Module Boundary

- All bot AI, combat strategies, and lifecycle management live inside `modules/TortoiseBots`.
- Bots log in as headless sessions with no game client while character state remains owned by the core.
- The `tortoise-wow` core compiles and runs cleanly with TortoiseBots disabled.
- In-game attribution (`by Sagiroth`, source URL, licence) stays visible in login messages and `.bot version` / `.bot about`.[^notice]

Related: [Features and Gameplay](../play/features.md), [Build and Configuration](../operations/build-and-config.md).

[^arch-doc]: TortoiseBots architecture invariants
[^notice]: Attribution and licensing notice
