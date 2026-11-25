---
type: Playbook
title: Build and Configuration
description: Static module compilation, config templates, and SQL migration notes.
tags: [build, cmake, config, sql, migrations]
status: stable
generated: { by: "antigravity/gemini", at: "2026-11-25T14:53:00Z" }
sources:
  - id: config-doc
    resource: https://github.com/TortoiseBots/TortoiseBots/blob/main/docs/server/configuration.md
    title: Server configuration and tuning guide
---

# Build

Inside a `tortoise-wow` (`main` branch) checkout:

```bash
git clone https://github.com/Sagiroth/TortoiseBots.git modules/TortoiseBots
cmake -B build -DMODULES=static -DMODULE_TORTOISEBOTS=static
cmake --build build -j"$(nproc)"
```

# Config Files and SQL Migrations

- **First-install configs:** Install places `aiplayerbot.conf.dist` next to `mangosd.conf` and creates `aiplayerbot.conf` and `modules/tortoise_bots.conf` on first install only (reinstalls preserve edited configs).[^config-doc]
- **Auto-migrations:** Module SQL in `data/sql/world` and `data/sql/character` is applied automatically by the core updater at startup.
- **Harmless old-migration notice:** A startup warning `Migration ... for module TortoiseBots exists in DB but not as file, old migration?` is harmless; the module cleans up known old rows automatically. If lines persist after manual world DB edits, reload `tw_world` from `sql/base` and restart.

Related: [Vision and Architecture](../architecture/overview.md), [Features and Gameplay](../play/features.md).

[^config-doc]: Server configuration and tuning guide
