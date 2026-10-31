# RuTracker as the primary external source

The source registry is `catalog/sources-v1.json`. It sets `rutracker` as the primary external
tracker and records the canonical homepage and topic URL prefix.

## User workflow

1. Open RuTracker from the source entry.
2. Choose a tico platform filter and a permitted release-type term.
3. Review the results on RuTracker and select a topic manually.
4. Optionally save its topic URL in the personal library as a reference.
5. To add a transfer, import a user-provided `.torrent` file or magnet URI.
6. Assign a tico platform and destination; the personal library remembers both.

A saved `tracker_topic` is a bookmark/reference, not a torrent job. It does not contain an info-hash
or transfer metadata. A user must explicitly import a torrent file or magnet URI to create a
transfer source.

## Filter configuration

The RuTracker filter definitions are stored separately from the tico platform registry in
`catalog/rutracker-filters-v1.json`. They map all 34 checked-in tico 0.8.0 system slugs to
search aliases and restrict the preset query terms to homebrew, freeware, public-domain,
open-source, Creative Commons and explicitly authorized releases.

The current verified forum scope is the parent section **Игры для консолей**, forum ID `548`.
The filter set does not guess individual subforum IDs where they could not be verified. Instead,
it combines a platform alias with one permitted release term through RuTracker's `tracker.php`
search endpoint. To print a manual search URL:

```sh
node scripts/rutracker-filter-url.mjs psx homebrew
node scripts/rutracker-filter-url.mjs nes freeware
```

The first search-term alias is used by default. Pass an optional zero-based alias index to use
another spelling, for example `node scripts/rutracker-filter-url.mjs psx homebrew 2`.

These title terms are discovery hints, not proof of rights. A matching result must still be
reviewed manually; the filter system never marks a release authorized by itself. The topic
reference workflow remains user-selected and the configuration keeps
`automated_indexing: false`.

## Current implementation boundary

- The source registry selects RuTracker as the primary external source.
- `src/library/source_center.c` validates and exposes source metadata and manual import actions.
- `src/library/import_source.c` accepts only HTTPS topic references on
  `rutracker.org/forum/viewtopic.php?t=<numeric-id>`.
- Personal imports are persisted separately from a shared catalogue.
- RuTracker pages are not crawled and no topic list is bulk-indexed.
- The current repository has no in-app RuTracker browser/search screen or automatic transfer
  submission from a search result. The new filter configuration and URL helper define the
  focused search contract for the next UI integration step.

No RuTracker credentials are stored by RetroTorrent.
