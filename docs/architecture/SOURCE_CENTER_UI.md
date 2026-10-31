# Source Center UI integration

The runtime source center is implemented in `src/library/source_center.c` and declared in
`src/library/source_center.h`. It loads `catalog/sources-v1.json` from the path supplied by the
application and gives the UI a compact, validated view of the configured sources.

## Native UI status

A first Borealis application shell is now present:

- `src/main_switch.cpp` loads the registry and persistent personal library.
- `src/ui/main_view.hpp` displays the configured primary source and the manual import actions.
- The source registry is bundled as `romfs:/sources-v1.json`.
- Personal records are stored at `sdmc:/switch/retrotorrent/library.json`.

The four UI sections are Overview, Sources, Personal library and Settings. See
[the Borealis UI port notes](BOREALIS_UI_PORT.md).

## Runtime contract

1. Resolve the config path, normally under the RetroTorrent application data directory.
2. Call `source_center_load(path, &registry)`.
3. On success, use `source_center_primary(&registry)` for the featured source and
   `source_center_at(&registry, index)` for the ordered list.
4. Sources are ordered by descending `priority`; the selected primary also has
   `is_primary == 1`.
5. If loading fails, keep the local library available and show a source-configuration warning.
   Do not silently invent a different primary source.

The returned source structure includes title, homepage, topic URL prefix, enabled state, import
capabilities, priority and notes. The app's platform/browser layer is responsible for opening the
homepage; the portable source-center module itself does not launch a browser or perform network IO.

## Import actions currently wired to the UI

For the configured primary source, the Borealis screen exposes:

- Save a RuTracker topic reference.
- Import a user-provided `.torrent` file by absolute SD/USB path.
- Add a magnet URI.

The UI also supports adding a local ROM path directly to the personal library. All four actions
persist through `import_manager`; they do not start transfers.

The source-center C functions are:

- `source_center_save_topic_reference(...)`
- `source_center_import_torrent_file(...)`
- `source_center_import_magnet_uri(...)`

The local-file action uses `library_import_source_set(...)` followed by
`import_manager_add(...)`.

The torrent-file path is parsed immediately, recording the info-hash and expected size. Topic
references remain bookmarks and are not treated as transfers. Magnet imports are stored as personal
entries; actual magnet metadata exchange and transfer queue submission remain a later integration
task.

## Error handling

- A source that lacks a requested import method does not show that action as enabled.
- Invalid or out-of-prefix topic URLs are rejected before being written to the library.
- Duplicate item IDs are reported by the import manager.
- File parsing and persistence failures are surfaced to the UI; never mark an import as queued or
  ready when the transfer subsystem has not accepted or completed it.

## Current boundary

The Borealis shell is an initial native UI slice, not the finished application. The live PipeNSX
transfer queue, magnet metadata exchange, piece transfer, archive extraction, browser-launch
adapter, tico path selection/deployment, and on-device verification remain unimplemented. The
NRO packaging target exists but has not yet been verified on a devkitPro runner.
