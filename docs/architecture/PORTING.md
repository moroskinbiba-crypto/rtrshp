# PipeNSX-to-RetroTorrent port

## Goal

Keep the battle-tested Switch BitTorrent engine and application foundations from PipeNSX,
but replace its Nintendo Switch package catalogue and title installer with a retro-content
catalogue and a safe ROM deployment pipeline.

This is an adaptation, not a clean-room reimplementation. The project remains under GPL-3.0
and keeps upstream attribution and third-party notices.

## Reuse

| PipeNSX area | RetroTorrent use |
|---|---|
| `src/core/` | bencode, metainfo, torrent sessions, peer protocol, piece verification, trackers, DHT and hashing |
| `vendor/libutp/` | uTP transport |
| `vendor/dht/` | DHT implementation |
| `src/platform/storage.c/.h` | multi-file torrent storage, safe paths, file I/O |
| download queue / manager | persistent queue, pause, resume, retry, recheck and shutdown |
| HTTP / TLS helpers | HTTPS catalogue, artwork and metadata transport |
| Borealis | Switch UI, input and rendering |
| PC test harness | regression tests for shared code |

The precise source-file selection is established during the import. PipeNSX has evolved quickly;
use a pinned upstream commit and do not silently mix files from different revisions.

## Replace or remove from the application

- Nintendo title catalogue and Switch game metadata.
- NSP/NSZ content handling and streaming installation.
- Nintendo title, patch, add-on, ticket, rights and firmware flows.
- eShop-oriented UI, search facets and package detail screens.
- Switch-title update and installed-title management that is unrelated to ROM deployment.
- Any bundled catalogue snapshot or metadata dataset that is not explicitly licensed for redistribution.

Do not delete upstream code before checking whether the torrent manager or tests depend on it.
Prefer isolating reusable download code behind a small interface first.

## Runtime pipeline

1. Load the signed/allowlisted retro catalogue and its local cache.
2. Filter catalogue entries by platform, region, language, genre and content type.
3. Show an entry only when its rights metadata satisfies the catalogue policy.
4. Resolve the selected entry to a validated magnet or torrent metainfo.
5. Add it to the persistent PipeNSX-derived download queue.
6. Download to a staging directory and verify every piece / final SHA-256.
7. Inspect archives and reject absolute paths, parent traversal, links and unsafe entries.
8. Extract or copy supported ROM files into a user-selected platform directory.
9. Atomically write an install record and refresh the frontend library if supported.
10. Keep the original archive until the user chooses to remove it.

## Important BitTorrent distinction

DHT and PEX discover peers for a known info-hash. They do not provide a reliable keyword search for
game titles. Human-readable discovery belongs to the catalogue/search layer; peer discovery and
transfer belong to the BitTorrent engine.

## Content policy and trust

The built-in catalogue may list only:
- homebrew releases;
- freeware and public-domain releases;
- releases whose owners explicitly authorize redistribution.

Personal dumps are imported from user-selected local files or user-supplied links and are not
published in the built-in catalogue by RetroTorrent.

Each catalogue record must include a rights class and a machine-readable provenance reference.
A record without valid rights metadata is rejected. Catalogue transport must be HTTPS and
manifest integrity must be verified before parsing or caching.

## Platform and path mapping

The destination is configurable per platform; never assume tico's directory layout is identical
across releases. The first implementation should let the user select a root directory and then
map platform IDs to subdirectories. Do not overwrite an existing file silently.

## Build gates

- PC unit tests for metainfo, queue persistence, path validation, archive extraction and mapping.
- Switch NRO build with devkitPro / devkitA64.
- On-device checks for suspend/resume, SD capacity, FAT32 limits and low-memory behavior.
- GPL and third-party notice audit before distribution.
