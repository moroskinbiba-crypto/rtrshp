import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";

const readJson = async (path) => JSON.parse(await readFile(new URL(path, import.meta.url), "utf8"));

const librarySchema = await readJson("../library/library-v1.schema.json");
const fixture = await readJson("../library/examples/personal-library.example.json");
const catalogueSchema = await readJson("../catalog/schema-v1.json");

assert.equal(librarySchema.properties.schema_version.const, 1);
assert.ok(librarySchema.properties.items.items.$ref === "#/$defs/item");

const itemSchema = librarySchema.$defs.item;
assert.ok(itemSchema.required.includes("source"));
assert.ok(itemSchema.required.includes("status"));
assert.ok(!itemSchema.required.includes("rights"), "Personal imports must not require rights metadata");
assert.ok(!Object.hasOwn(itemSchema.properties, "rights"), "Personal library records must not carry catalogue-only rights fields");

const sourceVariants = itemSchema.properties.source.oneOf;
const sourceTypes = sourceVariants.map((variant) => variant.properties.type.const).sort();
assert.deepEqual(sourceTypes, ["catalog_entry", "local_file", "magnet", "torrent_file", "tracker_topic"]);
const trackerSource = sourceVariants.find((variant) => variant.properties.type.const === "tracker_topic");
assert.ok(trackerSource, "RuTracker topic references must be supported by the personal library schema");
assert.ok(trackerSource.properties.uri.pattern.includes("rutracker\\\\.org"), "RuTracker topic URLs must be limited to the official host");

const sourceRegistry = await readJson("../catalog/sources-v1.json");
assert.equal(sourceRegistry.primary_source, "rutracker");
const primarySource = sourceRegistry.sources.find((source) => source.id === sourceRegistry.primary_source);
assert.ok(primarySource?.enabled, "Primary source must be enabled");
assert.equal(primarySource.kind, "external_tracker");
assert.equal(primarySource.automated_indexing, false, "RuTracker is configured for user-selected manual imports");

assert.equal(fixture.schema_version, 1);
assert.ok(Array.isArray(fixture.items));
const ids = new Set();
for (const item of fixture.items) {
  assert.ok(item.id && item.title && item.source?.type && item.status && item.created_at);
  assert.ok(!ids.has(item.id), `Duplicate library item id: ${item.id}`);
  ids.add(item.id);

  const matchingSource = sourceVariants.find((variant) => variant.properties.type.const === item.source.type);
  assert.ok(matchingSource, `Unknown source type: ${item.source.type}`);
  for (const required of matchingSource.required) {
    assert.ok(item.source[required] !== undefined, `Missing source field ${required} for ${item.id}`);
  }
  if (item.source.type === "magnet") {
    assert.ok(item.source.uri.startsWith("magnet:?"), "Magnet fixture must be a magnet URI");
  }
}

const catalogueItem = catalogueSchema.properties.entries.items;
assert.ok(catalogueItem.required.includes("origin"));
assert.ok(
  catalogueItem.allOf.some((rule) =>
    rule.if?.properties?.origin?.const === "catalog" && rule.then?.required?.includes("rights")
  ),
  "Shared catalogue entries must retain provenance requirements"
);

console.log(`Library schema smoke checks passed (${fixture.items.length} fixture items; ${sourceTypes.length} source types).`);
