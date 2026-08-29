import { createHash } from "node:crypto";
import { gzipSync, gunzipSync } from "node:zlib";
import { mkdir, readFile, readdir, rename, rm, stat, writeFile } from "node:fs/promises";
import { basename, dirname, join, relative, resolve, sep } from "node:path";

export const SCHEMA_VERSION = "1.0.0";
const MANIFEST_SCHEMA = "aifred.archive-manifest";
const RECORD_SCHEMA = "aifred.archive-record";

const iso = () => new Date().toISOString();
const safeStamp = (value) => value.replace(/[:.]/g, "-");
const sha256 = (buffer) => createHash("sha256").update(buffer).digest("hex");
const json = (value) => `${JSON.stringify(value, null, 2)}\n`;

async function atomicJson(path, value) {
  await mkdir(dirname(path), { recursive: true });
  const staged = `${path}.next`;
  await writeFile(staged, json(value), { mode: 0o600 });
  await rename(staged, path);
}

async function filesUnder(root, current = root) {
  let entries;
  try { entries = await readdir(current, { withFileTypes: true }); } catch (error) {
    if (error.code === "ENOENT") return [];
    throw error;
  }
  const output = [];
  for (const entry of entries.sort((a, b) => a.name.localeCompare(b.name))) {
    const path = join(current, entry.name);
    if (entry.isDirectory()) output.push(...await filesUnder(root, path));
    else if (entry.isFile() && !entry.name.endsWith(".next") && !entry.name.endsWith(".tmp")) output.push(path);
  }
  return output;
}

async function pathSize(path) {
  const files = await filesUnder(path);
  let bytes = 0;
  for (const file of files) bytes += (await stat(file)).size;
  return { bytes, files };
}

async function completedRuns(activeRoot) {
  let entries;
  try { entries = await readdir(activeRoot, { withFileTypes: true }); } catch (error) {
    if (error.code === "ENOENT") return [];
    throw error;
  }
  const runs = entries.filter((entry) => entry.isDirectory() && !entry.name.startsWith(".") && !entry.name.endsWith(".next"))
    .map((entry) => join(activeRoot, entry.name)).sort();
  return runs;
}

export async function readManifest(archiveRoot) {
  try {
    const parsed = JSON.parse(await readFile(join(archiveRoot, "manifest.json"), "utf8"));
    if (parsed.schema !== MANIFEST_SCHEMA || !Array.isArray(parsed.archives)) throw new Error("invalid archive manifest");
    return parsed;
  } catch (error) {
    if (error.code !== "ENOENT") throw error;
    return { schema: MANIFEST_SCHEMA, schemaVersion: SCHEMA_VERSION, updatedAt: null, archives: [] };
  }
}

export async function archiveStatus({ activeRoot, archiveRoot, thresholdMb }) {
  const active = await pathSize(activeRoot);
  const manifest = await readManifest(archiveRoot);
  let archiveBytes = 0;
  for (const item of manifest.archives) archiveBytes += Number(item.compressedSize || 0);
  const thresholdBytes = Math.max(0, Number(thresholdMb)) * 1024 * 1024;
  return {
    schemaVersion: SCHEMA_VERSION,
    activeBytes: active.bytes,
    activeMegabytes: Number((active.bytes / 1024 / 1024).toFixed(3)),
    thresholdMb: Number(thresholdMb),
    thresholdBytes,
    usagePercent: thresholdBytes ? Number((active.bytes / thresholdBytes * 100).toFixed(1)) : 0,
    shouldRotate: thresholdBytes > 0 && active.bytes >= thresholdBytes,
    archiveCount: manifest.archives.length,
    archiveBytes,
    latestArchive: manifest.archives.at(-1) || null
  };
}

function assertInside(root, path) {
  const rel = relative(resolve(root), resolve(path));
  if (rel.startsWith(`..${sep}`) || rel === ".." || rel.startsWith(sep)) throw new Error("path escapes configured root");
}

export async function createArchive({ activeRoot, archiveRoot, runPaths, source = "forge-context", failureHook }) {
  if (!runPaths.length) return { archived: false, reason: "no eligible completed runs" };
  runPaths.forEach((path) => assertInside(activeRoot, path));
  const createdAt = iso();
  const archiveId = `aifred-${source}-${safeStamp(createdAt)}`;
  const records = [];
  let uncompressedSourceSize = 0;
  for (const runPath of runPaths) {
    for (const file of await filesUnder(runPath)) {
      const content = await readFile(file);
      uncompressedSourceSize += content.length;
      records.push({
        schema: RECORD_SCHEMA,
        schemaVersion: SCHEMA_VERSION,
        run: basename(runPath),
        path: relative(runPath, file).split(sep).join("/"),
        size: content.length,
        checksum: `sha256:${sha256(content)}`,
        contentEncoding: "base64",
        content: content.toString("base64")
      });
    }
  }
  if (!records.length) return { archived: false, reason: "eligible runs contain no completed files" };
  const jsonl = Buffer.from(`${records.map((record) => JSON.stringify(record)).join("\n")}\n`);
  const compressed = gzipSync(jsonl, { level: 9 });
  const day = createdAt.slice(0, 10).split("-");
  const categoryRoot = join(archiveRoot, source, ...day);
  const bundlePath = join(categoryRoot, `${archiveId}.jsonl.gz`);
  const metadataPath = join(categoryRoot, `${archiveId}.meta.json`);
  const stagedBundle = `${bundlePath}.next`;
  await mkdir(categoryRoot, { recursive: true });
  await writeFile(stagedBundle, compressed, { mode: 0o600 });
  if (failureHook) await failureHook("bundle-written");
  const verifiedBuffer = await readFile(stagedBundle);
  const decodedLines = gunzipSync(verifiedBuffer).toString("utf8").trim().split("\n").filter(Boolean);
  const decoded = decodedLines.map((line) => JSON.parse(line));
  if (decoded.length !== records.length || sha256(verifiedBuffer) !== sha256(compressed)) throw new Error("archive verification failed");
  for (const record of decoded) {
    const body = Buffer.from(record.content, "base64");
    if (body.length !== record.size || `sha256:${sha256(body)}` !== record.checksum) throw new Error("archive record verification failed");
  }
  const metadata = {
    archiveId, schema: "aifred.archive-metadata", schemaVersion: SCHEMA_VERSION, source,
    createdAt, startTimestamp: basename(runPaths[0]), endTimestamp: basename(runPaths.at(-1)),
    recordCount: records.length, sourceRunCount: runPaths.length, uncompressedSourceSize,
    jsonlSize: jsonl.length, compressedSize: compressed.length,
    checksum: `sha256:${sha256(compressed)}`, categories: [...new Set(records.map((record) => record.path.split("/")[0]))].sort(),
    sourceVersion: SCHEMA_VERSION, path: relative(archiveRoot, bundlePath).split(sep).join("/")
  };
  await atomicJson(`${metadataPath}.next`, metadata);
  await rename(stagedBundle, bundlePath);
  await rename(`${metadataPath}.next`, metadataPath);
  if (failureHook) await failureHook("archive-verified");
  const manifest = await readManifest(archiveRoot);
  manifest.archives.push(metadata);
  manifest.archives.sort((a, b) => a.createdAt.localeCompare(b.createdAt));
  manifest.updatedAt = createdAt;
  await atomicJson(join(archiveRoot, "manifest.json"), manifest);
  if (failureHook) await failureHook("manifest-updated");
  for (const runPath of runPaths) await rm(runPath, { recursive: true, force: true });
  return { archived: true, metadata, prunedRuns: runPaths.map((path) => basename(path)) };
}

export async function rotate({ activeRoot, archiveRoot, thresholdMb, force = false, failureHook }) {
  const before = await archiveStatus({ activeRoot, archiveRoot, thresholdMb });
  if (!force && !before.shouldRotate) return { rotated: false, reason: "below threshold", before };
  const runs = await completedRuns(activeRoot);
  if (runs.length < 2) return { rotated: false, reason: "newest completed run is retained", before };
  const eligible = runs.slice(0, -1);
  const result = await createArchive({ activeRoot, archiveRoot, runPaths: eligible, failureHook });
  return { rotated: result.archived, before, ...result, after: await archiveStatus({ activeRoot, archiveRoot, thresholdMb }) };
}

async function archiveRecords(archiveRoot, metadata) {
  const bytes = await readFile(join(archiveRoot, metadata.path));
  if (`sha256:${sha256(bytes)}` !== metadata.checksum) throw new Error(`checksum mismatch for ${metadata.archiveId}`);
  return gunzipSync(bytes).toString("utf8").trim().split("\n").filter(Boolean).map((line) => JSON.parse(line));
}

export async function verifyArchive(archiveRoot, archiveId) {
  const manifest = await readManifest(archiveRoot);
  const matches = archiveId ? manifest.archives.filter((item) => item.archiveId === archiveId) : manifest.archives;
  const results = [];
  for (const metadata of matches) {
    const records = await archiveRecords(archiveRoot, metadata);
    results.push({ archiveId: metadata.archiveId, ok: records.length === metadata.recordCount, recordCount: records.length });
  }
  return results;
}

export async function searchArchives({ archiveRoot, query = "", category, start, end, limit = 100, byteLimit = 1024 * 1024 }) {
  const safeLimit = Math.min(Math.max(Number(limit) || 1, 1), 1000);
  const safeBytes = Math.min(Math.max(Number(byteLimit) || 1024, 1024), 10 * 1024 * 1024);
  const manifest = await readManifest(archiveRoot);
  const output = [];
  let returnedBytes = 0;
  for (const metadata of manifest.archives) {
    if (category && metadata.source !== category && !metadata.categories.includes(category)) continue;
    if (start && metadata.endTimestamp < start) continue;
    if (end && metadata.startTimestamp > end) continue;
    for (const record of await archiveRecords(archiveRoot, metadata)) {
      const text = Buffer.from(record.content, "base64").toString("utf8");
      if (query && !`${record.path}\n${text}`.toLowerCase().includes(query.toLowerCase())) continue;
      const item = { archiveId: metadata.archiveId, run: record.run, path: record.path, size: record.size, text };
      const size = Buffer.byteLength(JSON.stringify(item));
      if (output.length >= safeLimit || returnedBytes + size > safeBytes) return { items: output, truncated: true, returnedBytes };
      output.push(item); returnedBytes += size;
    }
  }
  return { items: output, truncated: false, returnedBytes };
}

export async function restoreArchiveSlice(options) {
  const result = await searchArchives(options);
  await rm(options.workspace, { recursive: true, force: true });
  await mkdir(options.workspace, { recursive: true });
  const path = join(options.workspace, `aifred-archive-slice-${safeStamp(iso())}.json`);
  await writeFile(path, json({ schema: "aifred.archive-slice", schemaVersion: SCHEMA_VERSION, generatedAt: iso(), ...result }), { mode: 0o600 });
  return { ...result, path };
}

export async function rebuildArchiveIndex(archiveRoot) {
  const files = (await filesUnder(archiveRoot)).filter((path) => path.endsWith(".meta.json"));
  const archives = [];
  for (const path of files) {
    const metadata = JSON.parse(await readFile(path, "utf8"));
    await archiveRecords(archiveRoot, metadata);
    archives.push(metadata);
  }
  archives.sort((a, b) => a.createdAt.localeCompare(b.createdAt));
  const manifest = { schema: MANIFEST_SCHEMA, schemaVersion: SCHEMA_VERSION, updatedAt: iso(), archives };
  await atomicJson(join(archiveRoot, "manifest.json"), manifest);
  return manifest;
}

export async function pruneArchive({ archiveRoot, archiveId, confirmed = false }) {
  if (!confirmed || !archiveId) throw new Error("permanent archive deletion requires --id and --confirm");
  const manifest = await readManifest(archiveRoot);
  const metadata = manifest.archives.find((item) => item.archiveId === archiveId);
  if (!metadata) throw new Error("archive ID not found");
  await archiveRecords(archiveRoot, metadata);
  const bundle = join(archiveRoot, metadata.path);
  assertInside(archiveRoot, bundle);
  await rm(bundle, { force: true });
  await rm(bundle.replace(/\.jsonl\.gz$/, ".meta.json"), { force: true });
  manifest.archives = manifest.archives.filter((item) => item.archiveId !== archiveId);
  manifest.updatedAt = iso();
  await atomicJson(join(archiveRoot, "manifest.json"), manifest);
  return { pruned: true, archiveId };
}
