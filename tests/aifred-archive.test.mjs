import test from "node:test";
import assert from "node:assert/strict";
import { mkdtemp, mkdir, readFile, rm, writeFile } from "node:fs/promises";
import { join } from "node:path";
import { tmpdir } from "node:os";
import { archiveStatus, pruneArchive, readManifest, rebuildArchiveIndex, restoreArchiveSlice, rotate, searchArchives, verifyArchive } from "../tools/lib/aifred-archive.mjs";

async function fixture(sizes = [16, 16, 16]) {
  const root = await mkdtemp(join(tmpdir(), "aifred-archive-test-"));
  const activeRoot = join(root, "active");
  const archiveRoot = join(root, "archive");
  const workspace = join(root, "workspace");
  for (let index = 0; index < sizes.length; index += 1) {
    const run = join(activeRoot, `2026-08-${String(index + 1).padStart(2, "0")}T00-00-00Z`, "site");
    await mkdir(run, { recursive: true });
    await writeFile(join(run, "events.json"), JSON.stringify({ timestamp: `2026-08-${String(index + 1).padStart(2, "0")}T00:00:00.000Z`, marker: "x".repeat(sizes[index]) }));
  }
  return { root, activeRoot, archiveRoot, workspace, cleanup: () => rm(root, { recursive: true, force: true }) };
}

test("below threshold does not rotate", async () => { const f = await fixture(); try { const r = await rotate({ ...f, thresholdMb: 10 }); assert.equal(r.rotated, false); } finally { await f.cleanup(); } });
test("exact threshold triggers rotation", async () => { const f = await fixture([100, 100, 100]); try { const s = await archiveStatus({ ...f, thresholdMb: 1 }); const r = await rotate({ ...f, thresholdMb: s.activeBytes / 1024 / 1024 }); assert.equal(r.rotated, true); } finally { await f.cleanup(); } });
test("above threshold archives old completed runs and retains newest", async () => { const f = await fixture(); try { const r = await rotate({ ...f, thresholdMb: 0.000001 }); assert.equal(r.rotated, true); assert.equal(r.prunedRuns.length, 2); assert.ok(r.after.activeBytes > 0); } finally { await f.cleanup(); } });
test("archive is gzip compressed and checksum verifies", async () => { const f = await fixture(); try { const r = await rotate({ ...f, thresholdMb: 0.000001 }); assert.ok(r.metadata.compressedSize > 0); assert.deepEqual(await verifyArchive(f.archiveRoot, r.metadata.archiveId), [{ archiveId: r.metadata.archiveId, ok: true, recordCount: 2 }]); } finally { await f.cleanup(); } });
test("manifest is updated with archive metadata", async () => { const f = await fixture(); try { await rotate({ ...f, thresholdMb: 0.000001 }); const m = await readManifest(f.archiveRoot); assert.equal(m.archives.length, 1); assert.equal(m.archives[0].schemaVersion, "1.0.0"); } finally { await f.cleanup(); } });
test("source is pruned only after successful archive", async () => { const f = await fixture(); try { const r = await rotate({ ...f, thresholdMb: 0.000001 }); assert.equal(r.after.activeBytes < r.before.activeBytes, true); } finally { await f.cleanup(); } });
test("failed archive leaves source intact", async () => { const f = await fixture(); try { await assert.rejects(() => rotate({ ...f, thresholdMb: 0.000001, failureHook: (stage) => { if (stage === "archive-verified") throw new Error("simulated failure"); } })); const s = await archiveStatus({ ...f, thresholdMb: 1 }); assert.equal(s.activeBytes > 0, true); assert.equal((await readManifest(f.archiveRoot)).archives.length, 0); } finally { await f.cleanup(); } });
test("archive listing reports bundles", async () => { const f = await fixture(); try { await rotate({ ...f, thresholdMb: 0.000001 }); assert.equal((await readManifest(f.archiveRoot)).archives.length, 1); } finally { await f.cleanup(); } });
test("bounded search honors record and byte limits", async () => { const f = await fixture(); try { await rotate({ ...f, thresholdMb: 0.000001 }); const r = await searchArchives({ archiveRoot: f.archiveRoot, query: "marker", limit: 1, byteLimit: 1024 }); assert.equal(r.items.length, 1); assert.equal(r.truncated, true); } finally { await f.cleanup(); } });
test("bounded restore materializes only selected slice", async () => { const f = await fixture(); try { await rotate({ ...f, thresholdMb: 0.000001 }); const r = await restoreArchiveSlice({ archiveRoot: f.archiveRoot, workspace: f.workspace, query: "2026-08-01", limit: 1, byteLimit: 4096 }); assert.equal(r.items.length, 1); assert.ok(JSON.parse(await readFile(r.path, "utf8")).generatedAt.endsWith("Z")); } finally { await f.cleanup(); } });
test("empty archive source is safe", async () => { const f = await fixture([]); try { const r = await rotate({ ...f, thresholdMb: 0.000001, force: true }); assert.equal(r.rotated, false); } finally { await f.cleanup(); } });
test("malformed source remains archivable as opaque bounded content", async () => { const f = await fixture(); try { await writeFile(join(f.activeRoot, "2026-08-01T00-00-00Z", "site", "events.json"), "{not json"); const r = await rotate({ ...f, thresholdMb: 0.000001 }); assert.equal(r.rotated, true); const s = await searchArchives({ archiveRoot: f.archiveRoot, query: "not json" }); assert.equal(s.items.length, 1); } finally { await f.cleanup(); } });
test("large archive records remain valid", async () => { const f = await fixture([2 * 1024 * 1024, 2 * 1024 * 1024, 16]); try { const r = await rotate({ ...f, thresholdMb: 1 }); assert.equal(r.rotated, true); assert.equal((await verifyArchive(f.archiveRoot))[0].ok, true); } finally { await f.cleanup(); } });
test("newest snapshot is never archived", async () => { const f = await fixture([10]); try { const r = await rotate({ ...f, thresholdMb: 0.000001 }); assert.equal(r.rotated, false); assert.match(r.reason, /newest/); } finally { await f.cleanup(); } });
test("archive metadata preserves UTC timestamps", async () => { const f = await fixture(); try { const r = await rotate({ ...f, thresholdMb: 0.000001 }); assert.ok(r.metadata.createdAt.endsWith("Z")); } finally { await f.cleanup(); } });
test("archive index can be rebuilt from verified metadata", async () => { const f = await fixture(); try { await rotate({ ...f, thresholdMb: 0.000001 }); const m = await rebuildArchiveIndex(f.archiveRoot); assert.equal(m.archives.length, 1); } finally { await f.cleanup(); } });
test("permanent prune requires confirmation and an exact archive ID", async () => { const f = await fixture(); try { const r = await rotate({ ...f, thresholdMb: 0.000001 }); await assert.rejects(() => pruneArchive({ archiveRoot: f.archiveRoot, archiveId: r.metadata.archiveId })); const p = await pruneArchive({ archiveRoot: f.archiveRoot, archiveId: r.metadata.archiveId, confirmed: true }); assert.equal(p.pruned, true); assert.equal((await readManifest(f.archiveRoot)).archives.length, 0); } finally { await f.cleanup(); } });
