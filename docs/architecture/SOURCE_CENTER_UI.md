# Source Center UI contract

The runtime source center is implemented in `src/library/source_center.c` and declared in
`src/library/source_center.h`. It loads `catalog/sources-v1.json` from the path supplied by the
application and gives the UI a compact, validated view of the configured sources.

## Startup

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

## RuTracker source card

For the configured primary source, the future Borealis screen should show:
- RuTracker title and primary marker;
- the homepage URL;
- an "Open website" action routed through the platform URL-launch adapter;
- actions for "Save topic reference", "Import .torrent" and "Import magnet".

Each action is exposed as a normal C function:
- `source_center_save_topic_reference(...)`
- `source_center_import_torrent_file(...)`
- `source_center_import_magnet_uri(...)`

All three create a record in the personal library through `import_manager_add`. The torrent-file
path is parsed immediately, recording the info-hash and expected size. Topic references remain
bookmarks and are not treated as transfers. Magnet imports are stored as personal entries; actual
magnet metadata exchange and transfer queue submission remain a later integration task.

## Error handling

- A source that lacks a requested import method must not show that action as enabled.
- Invalid or out-of-prefix topic URLs are rejected before being written to the library.
- Duplicate item IDs are reported by the import manager.
- File parsing and persistence failures are surfaced to the UI; never mark an import as queued or
  ready when the transfer subsystem has not accepted or completed it.

## Current boundary

This module is a portable backend/controller contract. The repository still has no Borealis app
entry point or source-screen implementation, and no NRO build target. A future UI should bind to
this API rather than duplicate registry parsing or source validation.
