#!/usr/bin/env node
import { readFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { archiveStatus, pruneArchive, readManifest, rebuildArchiveIndex, restoreArchiveSlice, rotate, searchArchives, verifyArchive } from "./lib/aifred-archive.mjs";

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const config = JSON.parse(await readFile(resolve(repoRoot, "config/aifred-admin.json"), "utf8"));
const args = process.argv.slice(2);
const command = args.shift() || "status";
const value = (name, fallback) => { const index = args.indexOf(`--${name}`); return index >= 0 ? args[index + 1] : fallback; };
const thresholdMb = Number(process.env[config.archive.thresholdEnvironmentVariable] || value("threshold-mb", config.archive.defaultThresholdMb));
const activeRoot = resolve(repoRoot, value("active-root", config.archive.activeRoot));
const archiveRoot = resolve(repoRoot, value("archive-root", config.archive.root));
const workspace = resolve(repoRoot, value("workspace", config.archive.workspace));
let result;
if (command === "status") result = await archiveStatus({ activeRoot, archiveRoot, thresholdMb });
else if (command === "rotate" || command === "archive") result = await rotate({ activeRoot, archiveRoot, thresholdMb, force: command === "archive" || args.includes("--force") });
else if (command === "list") result = await readManifest(archiveRoot);
else if (command === "verify") result = await verifyArchive(archiveRoot, value("id"));
else if (command === "search") result = await searchArchives({ archiveRoot, query: value("query", ""), category: value("category"), start: value("start"), end: value("end"), limit: value("limit", 100), byteLimit: value("byte-limit", 1048576) });
else if (command === "restore") result = await restoreArchiveSlice({ archiveRoot, workspace, query: value("query", ""), category: value("category"), start: value("start"), end: value("end"), limit: value("limit", 100), byteLimit: value("byte-limit", 1048576) });
else if (command === "rebuild-index") result = await rebuildArchiveIndex(archiveRoot);
else if (command === "prune") result = await pruneArchive({ archiveRoot, archiveId: value("id"), confirmed: args.includes("--confirm") });
else throw new Error("usage: aifred-archive.mjs status|rotate|archive|list|verify|search|restore|rebuild-index|prune [bounded options]");
process.stdout.write(`${JSON.stringify(result, null, 2)}\n`);
