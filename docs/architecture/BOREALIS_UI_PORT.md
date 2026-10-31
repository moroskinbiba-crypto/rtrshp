# Borealis UI port — first native app slice

## Pinned dependency

The original PipeNSX source snapshot used Borealis commit
`b1fa20d809ee3eeb05af8b7a3661ce3cb098635e` from the i3sey fork. That exact
commit is available in the public `XITRIX/borealis` repository and is pinned as
the `vendor/borealis` submodule. GLM is pinned to the same revision used by the
PipeNSX snapshot. The Borealis library is Apache-2.0; GLM keeps its upstream
license.

The app build targets Switch + Deko3D, with SDL2 and GLFW disabled. A recursive
checkout initializes the Borealis source and the GLM dependency:

```sh
git submodule update --init --recursive
```

## Current UI

`src/main_switch.cpp` initializes Borealis, loads the source registry from
ROMFS, opens the persistent personal library on SD, and pushes the root view.
`src/ui/main_view.hpp` contains the first four-screen navigation:

- Overview: personal-library count and actions.
- Sources: the configured primary source and its supported manual imports.
- Personal library: recent imported records and their status.
- Settings: the current registry and library storage locations.

The UI calls the portable C controller directly. The source registry is loaded
from `romfs:/sources-v1.json`; personal records are stored at
`sdmc:/switch/retrotorrent/library.json`. The source registry is shipped read-only
and does not get copied over user settings at launch.

## Import behavior and limitations

The native keyboard collects a title and source URI/path. Topic references,
magnet URIs and `.torrent` files are validated by Source Center before being
persisted. Local ROM paths go through the personal-import validator. Imported
records are not automatically queued: the live PipeNSX transfer manager,
magnet metadata exchange, archive extraction, emulator-path discovery and the
tico deployment adapter are not yet connected.

The source card displays the configured homepage; launching an external browser
is deliberately deferred until a Switch URL-launch adapter is selected and
tested.

## Build

The root CMake project now defines a Switch executable and packages
`build/switch/retrotorrent.nro`. On a machine with devkitPro, devkitA64,
devkitA64's CMake integration, Borealis' Deko3D requirements, `nacptool` and
`elf2nro` installed:

```sh
export DEVKITPRO=/opt/devkitpro
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target switch
```

This target has not yet been built on a Switch SDK runner or tested on hardware.
The existing CI continues to validate the portable C import and persistence
backend; it does not claim the NRO is ready.
