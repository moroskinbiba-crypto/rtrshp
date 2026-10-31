# tico platform filters

## Source of truth

tico 0.8.0 no longer has one hard-coded console list. Emulator modules describe their supported consoles in each module's `module.json`, under `systems[]`. A system's canonical identifier is `systems[].slug`; its display name is `systems[].title`, and accepted file extensions are `systems[].extensions`.

RetroTorrent therefore uses two inputs:

1. `catalog/tico-systems-v0.8.0.json` is the checked-in fallback registry for all official tico 0.8.0 emulator modules.
2. At runtime, scan `sdmc:/tico/modules/*/module.json`, parse `systems[]`, and merge those records into the registry. Installed/custom modules can add platforms without a RetroTorrent update.

Do not use emulator names or guessed labels as platform IDs. Catalogue records use the exact tico system slug.

## Current official registry

The 34 official tico 0.8.0 system slugs represented in the registry are:

- Nintendo: `nes`, `snes`, `gb`, `gbc`, `gba`, `n64`, `nds`, `3ds`, `gc`, `wii`, `vb`
- Sony: `psx`, `ps2`, `psp`
- Sega: `genesis`, `master-system`, `game-gear`, `sg-1000`, `sega-cd`, `sega-32x`, `saturn`, `dc`, `sc-3000`
- Bandai: `ws`, `wsc`
- Atari: `atari-2600`
- Computers and other systems: `msx`, `msx2`, `colecovision`, `spectravideo`
- Arcade: `triforce`, `naomi`, `atomiswave`, `fbneo`

The registry stores the module ID, module name/repository, display title, accepted extensions, DAT naming family where known, disc-based status and default ROM directory.

## Filter behaviour

- The filter list is built from the merged system registry, not from the currently visible catalogue entries. A platform can therefore be selected even when its catalogue currently has zero games.
- Platforms are grouped by hardware family in the UI.
- The app distinguishes **emulator installed** from **emulator not installed**. Do not hide a platform just because its tico module is not installed; let the user browse it and show a clear install requirement before launch.
- A source entry is indexed by its exact `platform` slug. No aliases such as `dreamcast` or `ps1` are silently substituted for tico's `dc` or `psx`.
- New/custom tico modules are discovered dynamically and can contribute extra systems. An unknown module should not break catalogue loading: skip malformed systems and log the reason.

## ROM destination handling

The tico documentation describes `sdmc:/tico/roms/<slug>/` as the default location for a console's ROMs. Store this as the initial suggestion only.

tico modules can define per-system path overrides, and users can configure multiple ROM folders, USB drives and NTFS volumes. RetroTorrent must therefore:
1. look for the system's configured ROM path when it can be obtained safely;
2. otherwise offer the tico default path;
3. let the user choose another folder;
4. persist the selected destination per platform slug and storage volume.

Never assume the default SD path is the only path or overwrite an existing file silently.

## Catalogue contract

Each catalogue entry stores:

- `platform`: canonical tico slug (required);
- `platform_title`: optional display-name snapshot;
- `emulator_module`: optional tico module ID;
- `files[].path`, expected size and SHA-256;
- rights class and proof URL.

The JSON Schema intentionally accepts any valid slug rather than a closed enum. This is necessary because tico's module set is extensible and modules can be added or removed independently of tico itself.

## Sources

- tico 0.8.0 release and official emulator list: https://github.com/ticohq/tico/releases/tag/0.8.0
- Creating tico Modules: https://forum.ticoverse.com/topic/149/creating-tico-modules/1
- Creating a Store Repository: https://forum.ticoverse.com/topic/150/creating-a-store-repository/1
