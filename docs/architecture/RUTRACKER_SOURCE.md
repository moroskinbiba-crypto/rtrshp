# RuTracker as the primary external source

The source registry is `catalog/sources-v1.json`. It sets `rutracker` as the primary external
tracker and records the canonical homepage and topic URL prefix.

## User workflow

1. Open RuTracker from the source entry in the future RetroTorrent source screen.
2. Browse and select a topic manually.
3. Optionally save its topic URL in the personal library as a reference.
4. To add a transfer, download the `.torrent` file to SD/USB and import it, or copy a magnet URI
   and paste it into RetroTorrent's personal import flow.
5. Assign a tico platform and destination; the personal library remembers both.

A saved `tracker_topic` is a bookmark/reference, not a torrent job. It does not contain an info-hash
or transfer metadata. The user must explicitly import a torrent file or magnet URI to create a
transfer source.

## Current implementation

- Registry: RuTracker is the primary source with priority 100.
- C validation: `library_import_source_set(..., LIBRARY_IMPORT_TRACKER_TOPIC, ...)` accepts only
  HTTPS topic URLs on `rutracker.org/forum/viewtopic.php?t=<numeric-id>` (optional simple query
  parameters are accepted).
- Persistence: topic references are stored in the personal library as `tracker_topic`.
- Transfers: `.torrent` files are parsed by the existing PipeNSX metainfo parser; magnet URI
  validation exists, but resolving magnet metadata and starting a live transfer are not yet wired
  into the PipeNSX queue.

The current implementation does not crawl RuTracker, scrape topic pages, bulk-index commercial
game releases, or publish torrent links into a shared catalogue. This keeps source selection
separate from the user's explicit personal imports. No RuTracker credentials are stored by
RetroTorrent.
