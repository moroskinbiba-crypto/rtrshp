# RetroTorrent

RetroTorrent is a Nintendo Switch homebrew application for discovering, downloading, verifying, and organizing retro-game files from a shared catalogue or a user's personal imports.

It is being built by adapting the architecture and reusable BitTorrent components of [PipeNSX](https://gitlab.com/pipensx-group/pipensx), under the GNU GPL v3.0. This repository is a derivative project; upstream attribution and third-party license notices are preserved as the port progresses.

## Project status

**Early port / architecture stage. No installable NRO is available yet.**

The first development target is to retain PipeNSX's proven download foundation while replacing its Nintendo Switch package catalogue and installer with a retro-game catalogue and ROM deployment workflow.

## Planned features

- Native Nintendo Switch UI built on Borealis.
- Separate, signed catalogue manifest with platform metadata and rights information.
- Platform filters for all 34 system slugs declared by the official tico 0.8.0 emulator modules, plus automatic discovery of additional/custom systems from installed `sdmc:/tico/modules/*/module.json` manifests. The canonical registry is `catalog/tico-systems-v0.8.0.json`.
- BitTorrent downloads with DHT, PEX, trackers, persistent queue, resume, and piece verification.
- Safe archive inspection and extraction.
- Automatic organization into user-configured emulator and frontend folders, including tico where its current folder layout is configured.
- Artwork and metadata cache.
- Manual import of user-provided `.torrent` and magnet links.
- RuTracker as the primary configurable external tracker, with per-tico-platform manual search filters for homebrew/freeware/rights-cleared releases and personal `.torrent`/magnet imports.
- Source allowlisting and catalogue signature/checksum verification.

## Content policy

The built-in catalogue is intended only for homebrew, freeware, public-domain releases, rights-cleared material, and other files the distributor is authorized to share. RetroTorrent will not ship commercial ROMs, BIOS files, copyrighted game images, or an index of unauthorized copies. Personal imports (local files, user-supplied `.torrent` files and magnet URIs) do not require a rights-proof field. They remain local and are not published to the shared catalogue automatically.

## Build

The initial port retains PipeNSX's devkitPro / devkitA64 and Borealis build approach. Build instructions will be updated once the adapted dependency set and source tree have been imported and verified.

## Personal library

RuTracker is the primary external source in `catalog/sources-v1.json`. The app stores manually selected RuTracker topic references and accepts user-imported `.torrent`/magnet sources; it does not automatically scrape topic pages.

Personal imports are stored separately from the shared catalogue. Local files, user-provided
`.torrent` files, magnet URIs and saved RuTracker topic references do not require a rights-proof
field and are not published automatically. The runtime source center reads the source registry,
orders sources by priority and exposes RuTracker plus its three manual import actions to the
future Borealis UI. See [the personal import workflow](docs/architecture/PERSONAL_IMPORTS.md),
[RuTracker source and filters](docs/architecture/RUTRACKER_SOURCE.md),
[the source-center UI contract](docs/architecture/SOURCE_CENTER_UI.md) and
[the library schema](library/library-v1.schema.json).

The example library fixture is in `library/examples/`. The C personal-import layer now includes source validation (`src/library/import_source.c`),
JSON persistence (`src/library/library_store.c`) and state management
(`src/library/import_manager.c`). It restores the library after restart, records platform and
destination choices, persists transfer progress, turns interrupted active states into `paused`,
and parses imported `.torrent` files to record their info-hash and total size. The live PipeNSX queue, magnet metadata exchange, piece transfer, archive extraction, browser-launch adapter and actual Borealis screens are not wired up yet. GitHub Actions runs schema checks and C source-center/import/library tests. The schema check can
also be executed locally with:

```sh
node scripts/check_library_schema.mjs
```

This check validates the JSON files and core schema invariants; it is not a substitute for the
application's future import-service tests.

## Upstream and licensing

- Upstream project: [PipeNSX](https://gitlab.com/pipensx-group/pipensx)
- Upstream license: GNU General Public License v3.0
- Upstream source and attribution must remain clearly documented in derivative distributions.
- See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for bundled dependency notices.

## Roadmap

1. Import the reusable upstream download engine, platform layer, tests, and pinned dependencies.
2. Remove Nintendo-specific package catalogue, title-management, and NSP/NSZ installation paths from the application build.
3. Add a retro catalogue schema, rights metadata, console filtering, and search.
4. Add ROM archive validation, safe extraction, deduplication, and destination mapping.
5. Integrate tico and configurable standalone emulator paths.
6. Run PC tests, Switch build, and on-device verification.
