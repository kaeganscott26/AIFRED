#!/usr/bin/env node
import { access, readFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";
const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const files = ["README.md", ...["README", "BUILD", "ARCHITECTURE", "DEVELOPMENT", "DISTRIBUTION", "INSTALLATION", "TESTING", "COEXISTENCE", "USER_GUIDE", "ADMIN_GUIDE", "ADMIN_COMMAND_REFERENCE", "API_REFERENCE", "FORGE_INTEGRATION", "ARCHIVE_GUIDE", "CLOUDFLARE_PRODUCTION", "TROUBLESHOOTING", "OPS_GUIDE"].map(name => `docs/${name}.md`)];
const failures = [];
for (const name of files) {
  const path = resolve(root, name);
  const text = await readFile(path, "utf8").catch(() => { failures.push(`missing document: ${name}`); return ""; });
  for (const match of text.matchAll(/\[[^\]]+\]\(([^)]+)\)/g)) {
    const target = match[1].split("#")[0];
    if (!target || /^[a-z]+:/i.test(target)) continue;
    await access(resolve(dirname(path), decodeURIComponent(target))).catch(() => failures.push(`broken link: ${name} -> ${target}`));
  }
}
const registry = JSON.parse(await readFile(resolve(root, "config/admin-commands.json"), "utf8"));
const commands = [...registry.backend, ...registry.androidLocal].map((x) => x.command);
if (new Set(commands).size !== commands.length) failures.push("duplicate canonical command in config/admin-commands.json");
if (registry.backend.length !== 10 || registry.androidLocal.length !== 15) failures.push("unexpected command registry counts");
if (failures.length) { failures.forEach((failure) => console.error(failure)); process.exit(1); }
console.log(`Documentation links valid; ${registry.backend.length} backend and ${registry.androidLocal.length} Android-local commands accounted for.`);
