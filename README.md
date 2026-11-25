<img width="2752" height="1536" alt="TortoiseBots" src="https://github.com/user-attachments/assets/a9a7f76d-b8ac-46f0-a058-30836559fe54" />

[![Latest release](https://img.shields.io/github/v/release/Sagiroth/TortoiseBots?label=release&logo=github)](https://github.com/Sagiroth/TortoiseBots/releases/latest)
[![Linux + Windows build](https://img.shields.io/github/actions/workflow/status/Sagiroth/TortoiseBots/build.yml?branch=main&event=push&label=Linux%20%2B%20Windows%20build&logo=github)](https://github.com/Sagiroth/TortoiseBots/actions/workflows/build.yml?query=branch%3Amain)
[![Support on Ko-fi](https://img.shields.io/badge/Ko--fi-support-FF5E5B?logo=ko-fi&logoColor=white)](https://ko-fi.com/sagiroth)
[![Discord](https://img.shields.io/badge/Discord-join-5865F2?logo=discord&logoColor=white)](https://discord.gg/FT2rZcXm3)

# TortoiseBots

An independent, native PlayerBots module for **Tortoise WoW 1.18.1** ([`tortoise-wow`](https://github.com/tortoise-wow/tortoise-wow), `main` branch). Bot AI, combat strategies, and lifecycle management live entirely inside this module as decoupled C++ headless sessions with no game client; the core compiles and runs cleanly without it.

> [!TIP]
> **New here? Start with [Getting Started](docs/play/getting-started.md)** — client setup, [TortoiseBotsManager](https://github.com/TortoiseBots/TortoiseBotsManager) (`/tbm`), bringing alts into your party, and the [feature table](docs/play/features.md).

## Highlights

- **Living world & earned progression:** Autonomous pool bots level, quest, grind, travel by foot/flight/boat, and trade on the Auction House with their own loot and gold.
- **Player control first:** Adventure with account alts or hired mercenaries; explicit `.bot` orders always override automation.
- **Combat, dungeons & PvP:** Nine-class rotations with Turtle 1.18.1 custom abilities, LoS pullback pulls, CC discipline, wipe recovery, LFT backfill, and WSG/AB/AV matchmaking.
- **Observability:** Live 2D browser map, Prometheus metrics, structured CSV logs, and in-game `/tbm` panel.

## Quick Build

Inside your `tortoise-wow` (`main` branch) checkout:

```bash
git clone https://github.com/Sagiroth/TortoiseBots.git modules/TortoiseBots
cmake -B build -DMODULES=static -DMODULE_TORTOISEBOTS=static
cmake --build build -j"$(nproc)"
```

Install copies `aiplayerbot.conf.dist` next to `mangosd.conf` and creates `aiplayerbot.conf` and `modules/tortoise_bots.conf` on first install only. Module SQL in `data/sql/world` and `data/sql/character` is applied automatically on startup.

## Documentation

| Area | Topics |
|---|---|
| [Knowledge index](knowledge/index.md) | [Vision & Architecture](knowledge/architecture/overview.md), [Features & Gameplay](knowledge/play/features.md), [Build & Configuration](knowledge/operations/build-and-config.md) |
| [Full guides (`docs/`)](docs/README.md) | [Getting Started](docs/play/getting-started.md), [Player Commands](docs/play/commands/README.md), [Class AI](docs/classes/overview.md), [Dungeons & Raids](docs/play/dungeons-and-raids.md), [Living World](docs/world/living-world.md), [Configuration](docs/server/configuration.md), [Dashboard](docs/server/dashboard.md), [Architecture](docs/developers/architecture.md) |

## Credit & Support

TortoiseBots is made by **Sagiroth**. Forks, repacks, Docker bundles, videos, and public servers are welcome — please credit it:

```text
Playerbots: TortoiseBots by Sagiroth — https://github.com/Sagiroth/TortoiseBots
```

Keep the in-game attribution line (`by Sagiroth`, source URL, licence) visible in modified builds (see [`NOTICE.md`](NOTICE.md#attribution-author-notice)). Optional support: [**Ko-fi**](https://ko-fi.com/sagiroth).

## Licence

Dual-licensed under **AGPL-3.0-or-later** ([`LICENSE`](LICENSE)) or **GPL-2.0-or-later** ([`LICENSE-GPL-2.0`](LICENSE-GPL-2.0)); a few donor-derived files stay AGPL-only. See [`NOTICE.md`](NOTICE.md) and [`docs/developers/licensing-audit.md`](docs/developers/licensing-audit.md).

Non-commercial community project made for educational purposes. Donations only support development and buy no access or service. Not affiliated with Turtle WoW or Blizzard Entertainment.
