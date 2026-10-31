# Catalogue and personal-import behaviour

RetroTorrent has two deliberately separate data paths.

## 1. Shared catalogue

The shared catalogue is a published index that RetroTorrent distributes to its users. Entries in
this index use `origin: "catalog"` and carry structured provenance metadata. The catalogue
publisher is responsible for ensuring that the material it publishes may be redistributed.

The shared catalogue may include homebrew, freeware, public-domain releases, and releases whose
owners have authorized redistribution. The schema requires provenance metadata for these shared
entries. This is a publication rule for the built-in/shared catalogue, not a scan of a user's SD
card and not a rights check on personal imports.

## 2. Personal imports

A user may manually add a local ROM/archive, a `.torrent` file, or a magnet URI. These entries use
`origin: "user_import"` and do not require a `rights` object or proof URL.

RetroTorrent does not attempt to determine whether a user owns a physical cartridge, disc, or
digital entitlement. It does not inspect purchase receipts, serial numbers, console accounts, or
personal ownership records. The application treats these as user-provided items and keeps them
separate from the shared catalogue.

Personal imports are not automatically uploaded, published, or sent to other users. They remain
in the user's local library unless the user explicitly exports metadata.

## Search and filtering

The catalogue layer provides title search and filters by platform, region, language, genre and
content type. Platform matching happens on metadata, before any network transfer begins.

The torrent layer receives a magnet or torrent metainfo and discovers peers for its info-hash.
DHT and PEX are peer-discovery protocols, not keyword search engines.

## Source integrity and file safety

Regardless of origin:
- validate catalogue JSON and torrent metainfo before use;
- verify downloaded pieces and expected file sizes;
- reject archive paths that escape the selected destination;
- reject symlinks, hardlinks, device nodes, duplicate normalized paths and unsafe archive entries;
- check available storage before extraction;
- extract to a staging directory and publish files only after validation succeeds;
- never overwrite existing files silently.

These checks protect data integrity and the SD card. They are not checks of who owns a game.

## RuTracker adapter boundary

The public catalogue adapter is designed as a metadata-source interface. A source may provide
searchable metadata and links, but adding a source does not make every item suitable for publication
in the shared catalogue. Personal links supplied manually by the user stay on the personal-import
path and are not promoted into the shared catalogue automatically.
