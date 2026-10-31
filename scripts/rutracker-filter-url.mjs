import { readFile } from "node:fs/promises";
import { pathToFileURL } from "node:url";
import path from "node:path";

export function buildRuTrackerSearchUrl(filters, platformSlug, releaseTerm, aliasIndex = 0) {
  const system = filters.systems.find((item) => item.slug === platformSlug);
  if (!system) throw new Error(`Unknown tico platform: ${platformSlug}`);
  if (!filters.policy.allowed_release_terms.includes(releaseTerm)) {
    throw new Error("Release term is not in the RuTracker allowlist");
  }
  if (!Number.isInteger(aliasIndex) || aliasIndex < 0 || aliasIndex >= system.search_terms.length) {
    throw new Error("Platform search-term index is out of range");
  }

  const url = new URL(filters.search.endpoint);
  for (const forumId of filters.search.forum_ids) {
    url.searchParams.append(filters.search.forum_parameter, String(forumId));
  }
  url.searchParams.set(filters.search.query_parameter,
    `"${system.search_terms[aliasIndex]}" "${releaseTerm}"`);
  url.searchParams.set("o", "10");
  url.searchParams.set("s", "2");
  return url.toString();
}

async function main() {
  const [, , platformSlug, releaseTerm = "homebrew", aliasIndexArg = "0"] = process.argv;
  if (!platformSlug) {
    throw new Error("Usage: node scripts/rutracker-filter-url.mjs <tico-slug> [allowed-release-term] [alias-index]");
  }
  const filters = JSON.parse(await readFile(new URL("../catalog/rutracker-filters-v1.json", import.meta.url), "utf8"));
  console.log(buildRuTrackerSearchUrl(filters, platformSlug, releaseTerm, Number(aliasIndexArg)));
}

if (process.argv[1] && import.meta.url === pathToFileURL(path.resolve(process.argv[1])).href) {
  main().catch((error) => {
    console.error(error.message);
    process.exitCode = 1;
  });
}
