# Catalogue policy

RetroTorrent's catalogue is a metadata index, not a source of game files by itself.
A catalogue provider supplies records that point to a torrent or magnet, and the PipeNSX-derived
BitTorrent engine handles peer discovery and transfer after the info-hash is known.

## Allowed catalogue records

A record must be one of:

- homebrew, with a project or author page;
- freeware, with the author's redistribution terms;
- public domain, with a reliable provenance reference;
- explicitly authorized redistribution, with a written permission or license reference.

Commercial game images without redistribution permission must not be included in the built-in
catalogue. A record cannot be made acceptable merely by adding a "personal backup" label.

## Search and filtering

The catalogue layer provides title search and filters by platform, region, language, genre and
content type. Platform matching happens on metadata, before any network transfer begins.

The torrent layer is not a title search engine: it receives a validated magnet or torrent record
and discovers peers for its info-hash.

## Sources and integrity

- Only configured HTTPS catalogue sources are loaded.
- The catalogue manifest is checked before the catalogue body is fetched.
- The body is cached only after checksum and schema validation.
- A provider is allowlisted by explicit configuration; redirects must remain within the configured
  host policy.
- Each content record must include `rights.class` and a verifiable `rights.proof_url`.
- Each downloadable file must include a SHA-256 hash and expected size.
- A mismatch, missing rights proof, unknown platform or invalid path rejects the record.

## User-provided material

Users may add their own `.torrent` files, magnet links or local archives. Such imports are stored
separately from the public catalogue and are clearly marked as user-provided. RetroTorrent does
not upload or publish user-provided metadata to shared catalogues.

## Archive safety

Before extraction, reject absolute paths, `..` path components, symlinks, hardlinks, device nodes,
duplicate normalized paths, excessive file counts, decompression bombs and files exceeding the
configured capacity. Extract to a staging directory and publish only after validation succeeds.
