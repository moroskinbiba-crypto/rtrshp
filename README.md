# RetroTorrent

RetroTorrent is a Nintendo Switch homebrew application for discovering, downloading, verifying, and organizing retro-game files from a shared catalogue or a user's personal imports.

It is being built by adapting the architecture and reusable BitTorrent components of [PipeNSX](https://gitlab.com/pipensx-group/pipensx), under the GNU GPL v3.0. This repository is a derivative project; upstream attribution and third-party license notices are preserved as the port progresses.

## Project status

**Early application port. A native Borealis shell and Switch NRO packaging target are now in the repository, but no NRO has yet been verified on a Switch SDK runner or on hardware.**

The first native screen set contains Overview, Sources, Personal library and Settings. It loads the source registry from ROMFS, restores personal imports from SD, and exposes manual topic-reference, `.torrent`, magnet and local-ROM imports through the existing C controller.

## Planned features

- Native Nintendo Switch UI built on Borealis.
- Separate, signed catalogue manifest with platform metadata and rights information.
- Platform filters for all 34 system slugs declared by the official tico 0.8.0 emulator modules, plus automatic discovery of additional/custom systems from installed `sdmc:/tico/modules/*/module.json` manifests. The canonical registry is `catalog/tico-systems-v0.8.0.json`.
- BitTorrent downloads with DHT, PEX, trackers, persistent queue, resume, and piece verification.
- Safe archive inspection and extraction.
- Automatic organization into user-configured emulator and frontend folders, including tico where its current folder layout is configured.
- Artwork and metadata cache.
- Manual import of user-provided `.torrent` and magnet links.
- RuTracker as the primary configurable external tracker, with manual topic references and personal `.torrent`/magnet imports.
- Source allowlisting and catalogue signature/checksum verification.

## Content policy

The built-in catalogue is intended only for homebrew, freeware, public-domain releases, rights-cleared material, and other files the distributor is authorized to share. RetroTorrent will not ship commercial ROMs, BIOS files, copyrighted game images, or an index of unauthorized copies. Personal imports (local files, user-supplied `.torrent` files and magnet URIs) do not require a rights-proof field. They remain local and are not published to the shared catalogue automatically.

## Build

The first Switch app target uses the pinned Borealis + GLM submodules:

```sh
export DEVKITPRO=/opt/devkitpro
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target switch
```

The expected artifact is `build/switch/retrotorrent.nro`. The build target is present but not yet verified on a devkitPro runner; see [the Borealis port notes](docs/architecture/BOREALIS_UI_PORT.md).

## Personal library

RuTracker is the primary external source in `catalog/sources-v1.json`. The app stores manually selected RuTracker topic references and accepts user-imported `.torrent`/magnet sources; it does not automatically scrape topic pages.

Personal imports are stored separately from the shared catalogue. Local files, user-provided
`.torrent` files, magnet URIs and saved RuTracker topic references do not require a rights-proof
field and are not published automatically. The runtime source center reads the source registry,
orders sources by priority and exposes RuTracker plus its three manual import actions to the
Borealis UI. See [the personal import workflow](docs/architecture/PERSONAL_IMPORTS.md),
[RuTracker source setup](docs/architecture/RUTRACKER_SOURCE.md),
[the source-center UI contract](docs/architecture/SOURCE_CENTER_UI.md) and
[the library schema](library/library-v1.schema.json).

The example library fixture is in `library/examples/`. The C personal-import layer includes source validation (`src/library/import_source.c`),
JSON persistence (`src/library/library_store.c`) and state management
(`src/library/import_manager.c`). It restores the library after restart, records platform and
destination choices, persists transfer progress, turns interrupted active states into `paused`, and
parses imported `.torrent` files to record their info-hash and total size. The live BitTorrent
transfer queue, magnet metadata exchange, piece transfer, archive extraction, browser-launch
adapter and tico deployment adapter are not wired up yet. GitHub Actions runs schema checks and C
source-center/import/library tests.

## Upstream and licensing

- Upstream project: [PipeNSX](https://gitlab.com/pipensx-group/pipensx)
- Upstream license: GNU General Public License v3.0
- Upstream source and attribution must remain clearly documented in derivative distributions.
- See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for bundled dependency notices.

## Roadmap

1. Finish the native UI shell and connect it to the controller.
2. Adapt the reusable transfer engine into a persistent RetroTorrent queue.
3. Add a retro catalogue schema, rights metadata, console filtering, and search.
4. Add ROM archive validation, safe extraction, deduplication, and destination mapping.
5. Integrate tico and configurable standalone emulator paths.
6. Run PC tests, Switch build, and on-device verification.
