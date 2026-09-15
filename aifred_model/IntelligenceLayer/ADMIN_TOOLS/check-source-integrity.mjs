import { readFileSync } from "node:fs";
import { extname, relative, resolve } from "node:path";
import { spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";

const root = resolve(fileURLToPath(new URL("..", import.meta.url)));
const listed = spawnSync("git", ["ls-files", "-co", "--exclude-standard", "-z"], { cwd: root, encoding: "utf8" });
if (listed.status !== 0) {
  throw new Error(listed.stderr || "git ls-files failed");
}

const files = [...new Set(listed.stdout.split("\0").filter(Boolean))];
const failures = [];
const deployableExtensions = new Set([".css", ".html", ".js", ".json", ".toml"]);
const conflictMarker = /^(?:<<<<<<<(?: .*)?|=======|>>>>>>>(?: .*)?)\s*$/m;
const markdownFence = /^```(?:html|css|js|javascript|toml|json)?\s*$/i;

for (const file of files) {
  const absolute = resolve(root, file);
  let content;
  try {
    content = readFileSync(absolute, "utf8");
  } catch {
    continue;
  }
  if (content.includes("\0")) continue;
  if (conflictMarker.test(content)) failures.push(`${file}: unresolved merge marker`);

  const normalized = relative(root, absolute).replaceAll("\\", "/");
  if (normalized.startsWith("apps/website/") && deployableExtensions.has(extname(file).toLowerCase())) {
    const nonblank = content.split(/\r?\n/).map((line) => line.trim()).filter(Boolean);
    if (markdownFence.test(nonblank[0] || "") || markdownFence.test(nonblank.at(-1) || "")) {
      failures.push(`${file}: deployable source is wrapped in a Markdown fence`);
    }
  }

  if (normalized === "apps/website/index.html") {
    const scriptTags = [...content.matchAll(/<script\b[^>]*\bsrc=["']([^"']+)["'][^>]*>/gi)];
    for (const source of ["./config.js", "./app.js"]) {
      const matches = scriptTags.filter((match) => match[1] === source || match[1] === source.slice(2));
      if (matches.length !== 1) failures.push(`${file}: expected exactly one ${source} script include, found ${matches.length}`);
      else if (!/\bdefer\b/i.test(matches[0][0])) failures.push(`${file}: ${source} must use defer so DOM bindings exist before runtime startup`);
    }

    for (const id of ["aifred-downloads", "catalog-list", "catalog-toggle", "catalog-refresh", "audio-player", "year"]) {
      const count = [...content.matchAll(new RegExp(`\\bid=["']${id}["']`, "gi"))].length;
      if (count !== 1) failures.push(`${file}: expected exactly one #${id}, found ${count}`);
    }
  }
}

if (failures.length) {
  console.error(failures.join("\n"));
  process.exit(1);
}

console.log(`source integrity ok (${files.length} tracked and untracked source files)`);