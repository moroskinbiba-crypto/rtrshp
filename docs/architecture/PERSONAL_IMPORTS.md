# Personal library and import workflow

The personal library is stored separately from the shared catalogue. A library item is not a
catalogue publication and does not need a rights/proof field.

## Supported import sources

- **Local file** — scan or select a ROM/archive already on SD or USB.
- **Torrent file** — select a user-provided `.torrent` file.
- **Magnet URI** — paste or open a magnet link supplied by the user.
- **Shared catalogue entry** — save a catalogue entry into the personal library by reference.

The persisted format is defined in `library/library-v1.schema.json`. Do not store personal paths or
user-supplied magnet URIs in the shared catalogue cache.

## Import flow

1. The user chooses a file, torrent file, or magnet URI.
2. Create a local library record immediately, using a stable ID and `status: imported` or
   `status: queued`.
3. For a local file, inspect its extension and metadata; do not copy it unless the user chooses
   an import/copy operation.
4. For a torrent file or magnet, parse and validate metainfo before adding a job to the download
   queue. A magnet may need peer metadata exchange before file names and sizes are known.
5. Let the user select the target platform and destination. Offer discovered tico module paths,
   but preserve custom SD and USB paths reported by the platform layer.
6. Download or copy into a staging location. Keep the original source untouched.
7. Verify torrent pieces and, when available, the final SHA-256. Check free space before extraction.
8. Safely inspect archives. Reject path traversal, absolute paths, links, device nodes, duplicate
   normalized paths and resource-exhaustion cases.
9. Move selected files into the chosen destination only after validation succeeds.
10. Mark the record `ready`, store the resolved destination and refresh the frontend library.

## State transitions

```text
imported -> queued -> downloading -> verifying -> ready
                 \-> paused -> queued
                 \-> error
ready -> missing
missing -> imported
```

A failed or interrupted job must remain visible and resumable. Do not discard queue state on
application shutdown. The queue's transfer state and the library's user metadata should be
persisted separately so a damaged cache cannot erase the user's titles, platform assignments or
destination choices.

## Platform and destination selection

Use exact `systems[].slug` values discovered from installed tico module manifests. The platform
may be assigned after import. Resolve destinations from the module's configured paths at runtime;
never hard-code `sdmc:/tico/roms/<slug>/` as the only destination. USB and custom paths must remain
valid choices.

## Deduplication

- Local files: compare normalized URI/path, file size and modification time; offer a content hash
  when practical.
- Torrent jobs: use the parsed info-hash as the transfer identity.
- Catalogue references: use the pair `catalog_id + entry_id`.
- Do not merge records solely because their display titles match.

## Privacy and export

Personal imports are local by default. Do not upload local paths, magnet URIs, notes, play history
or the personal library to a shared catalogue. Export must be an explicit user action and should
offer a preview of the exported metadata.

## Implementation boundaries

The first personal-import implementation now includes:

- `src/library/import_source.c`: validates absolute SD/USB/PC paths, rejects parent-directory
  segments, validates BitTorrent v1 magnet `xt=urn:btih` values, and loads `.torrent` metainfo.
- `src/library/library_store.c`: reads and writes the versioned JSON library, rejects malformed
  records and duplicate IDs, and writes to a temporary file before replacing the previous save.
- `src/library/import_manager.c`: adds/removes library items, remembers platform and destination,
  tracks progress and state transitions, and recovers interrupted `downloading`/`verifying` jobs
  as `paused` on startup. Imported `.torrent` files are parsed immediately and their info-hash
  and total size are recorded.

The manager is a persistent state layer, not the live PipeNSX transfer queue yet. It does not
perform magnet metadata exchange, start peers, download pieces, or extract archives. Those require
the later application-level queue and UI integration.

The dependency-light checks run in `.github/workflows/schema-checks.yml`; tests cover source
validation, torrent metainfo loading, library save/load, process-restart recovery, and legal state
transitions.
