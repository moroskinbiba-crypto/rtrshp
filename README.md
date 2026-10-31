# RetroTorrent

RetroTorrent is a Nintendo Switch homebrew application for discovering, downloading, verifying, and organizing legally distributable retro-game files.

It is being built by adapting the architecture and reusable BitTorrent components of [PipeNSX](https://gitlab.com/pipensx-group/pipensx), under the GNU GPL v3.0. This repository is a derivative project; upstream attribution and third-party license notices are preserved as the port progresses.

## Project status

**Early port / architecture stage. No installable NRO is available yet.**

The first development target is to retain PipeNSX's proven download foundation while replacing its Nintendo Switch package catalogue and installer with a retro-game catalogue and ROM deployment workflow.

## Planned features

- Native Nintendo Switch UI built on Borealis.
- Separate, signed catalogue manifest with platform metadata and rights information.
- Platform filters for NES, SNES, Game Boy, Game Boy Color, Game Boy Advance, Nintendo 64, Nintendo DS, PlayStation, PSP, Sega systems, and other supported systems.
- BitTorrent downloads with DHT, PEX, trackers, persistent queue, resume, and piece verification.
- Safe archive inspection and extraction.
- Automatic organization into user-configured emulator and frontend folders, including tico where its current folder layout is configured.
- Artwork and metadata cache.
- Manual import of user-provided `.torrent` and magnet links.
- Source allowlisting and catalogue signature/checksum verification.

## Content policy

The built-in catalogue is intended only for homebrew, freeware, public-domain releases, rights-cleared material, and other files the distributor is authorized to share. RetroTorrent will not ship commercial ROMs, BIOS files, copyrighted game images, or an index of unauthorized copies. Users may import their own legally obtained dumps.

## Build

The initial port retains PipeNSX's devkitPro / devkitA64 and Borealis build approach. Build instructions will be updated once the adapted dependency set and source tree have been imported and verified.

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
