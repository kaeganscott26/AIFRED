import { mkdir, rename, rm, writeFile } from "node:fs/promises";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { rotate } from "../../../tools/lib/aifred-archive.mjs";

const bridgeRoot = dirname(dirname(fileURLToPath(import.meta.url)));
const exportRoot = join(bridgeRoot, "exports");
const base = String(process.env.AIFRED_API_BASE_URL || "https://www.north3rnlight3r.com").replace(/\/+$/, "");
const token = String(process.env.AIFRED_ADMIN_SESSION_TOKEN || "").trim();
const selected = process.argv[2] || "all";
const definitions = {
  site: { endpoint: "/api/v1/admin/export/site", folder: "site", file: "site-export.json" },
  tracks: { endpoint: "/api/v1/admin/export/tracks", folder: "track-analysis", file: "track-analysis-export.json" }
};

if (!token) throw new Error("AIFRED_ADMIN_SESSION_TOKEN is required and is never persisted");
const names = selected === "all" ? Object.keys(definitions) : [selected];
if (names.some((name) => !definitions[name])) throw new Error("usage: export.mjs [all|site|tracks]");

const stamp = new Date().toISOString().replace(/[:.]/g, "-");
const history = join(exportRoot, "history", stamp);
for (const name of names) {
  const definition = definitions[name];
  const response = await fetch(`${base}${definition.endpoint}`, { headers: { authorization: `Bearer ${token}` } });
  const text = await response.text();
  if (!response.ok) throw new Error(`${name} export failed (${response.status}): ${text.slice(0, 300)}`);
  JSON.parse(text);
  const historyFile = join(history, definition.folder, definition.file);
  await mkdir(dirname(historyFile), { recursive: true });
  await writeFile(historyFile, `${text.trim()}\n`, { mode: 0o600 });
  const latestDir = join(exportRoot, "latest", definition.folder);
  const staged = `${latestDir}.next`;
  await rm(staged, { recursive: true, force: true });
  await mkdir(staged, { recursive: true });
  await writeFile(join(staged, definition.file), `${text.trim()}\n`, { mode: 0o600 });
  await rm(latestDir, { recursive: true, force: true });
  await rename(staged, latestDir);
}

const thresholdMb = Number(process.env.AIFRED_FORGE_ACTIVE_LOG_LIMIT_MB || 25);
const archiveRoot = join(bridgeRoot, "..", "..", "runtime", "aifred-archive");
const rotation = await rotate({ activeRoot: join(exportRoot, "history"), archiveRoot, thresholdMb });
console.log(`AIFRED export mirror updated: ${join(exportRoot, "latest")}`);
if (rotation.rotated) console.log(`Verified archive ${rotation.metadata.archiveId}; pruned ${rotation.prunedRuns.length} completed FORGE mirror run(s).`);
