# PipeNSX source snapshot used for the initial port

- User-provided canonical project URL: https://gitlab.com/pipensx-group/pipensx
- Public source mirror used for the initial file import: https://github.com/frandiazc/pipensx
- Mirror relationship: fork of https://github.com/i3sey/pipensx
- Imported source revision: `5accddf576f61c6dc11299c21a911ca712e7856b`
- Snapshot commit date: 2026-10-18
- Snapshot commit subject: `fix(torrent): prevent thread stack overflow and unbounded piece buffer crash`
- Upstream license: GNU GPL v3.0

## Imported in the first source pass

- The complete `src/core/` BitTorrent engine (bencode, metainfo, peer, piece,
  trackers, DHT interface, SHA-1/SHA-256, torrent lifecycle and helpers).
- `src/platform/storage.c/.h` and the upstream PC Switch shim.
- Vendored `vendor/dht/` and `vendor/libutp/` source and notices.
- Core networking / piece / tracker test files and fuzz entry points.

## Important source-version limitation

The GitLab URL above is the canonical source supplied for this project. The GitLab host was
not readable through the repository connector during this initial import. To avoid claiming a
direct GitLab checkout, this first source pass was taken from the public GitHub fork listed above,
which identifies `i3sey/pipensx` as its parent. Its snapshot is dated 2026-10-18 and may be
behind the GitLab project's current default branch.

Before importing the application layer or preparing a release, compare the imported files with
the current GitLab default branch and update the pinned snapshot deliberately. Do not combine
source files from different commits without recording the change.

## License obligations

This is a derivative work of GPL-3.0 software. Preserve copyright and license notices, mark
modified files, provide corresponding source when distributing binaries, and comply with GPL v3.0.
Third-party dependency notices remain separate; see [THIRD_PARTY_NOTICES.md](../../THIRD_PARTY_NOTICES.md).
