import { readdirSync } from "node:fs";
import { join, relative, resolve } from "node:path";
import { spawnSync } from "node:child_process";
import { fileURLToPath, pathToFileURL } from "node:url";

const root = resolve(fileURLToPath(new URL("..", import.meta.url)));
const website = join(root, "apps", "website");
const files = [];

function visit(directory) {
  for (const entry of readdirSync(directory, { withFileTypes: true })) {
    const path = join(directory, entry.name);
    if (entry.isDirectory()) visit(path);
    else if (entry.isFile() && entry.name.endsWith(".js")) files.push(path);
  }
}

visit(website);
for (const file of files) {
  const checked = spawnSync(process.execPath, ["--check", file], { cwd: root, encoding: "utf8" });
  if (checked.status !== 0) {
    process.stderr.write(checked.stderr || checked.stdout);
    process.exit(checked.status || 1);
  }
}

for (const file of files.filter((path) => path.includes(`${join("website", "lib", "backend")}`) || path.includes(`${join("website", "functions")}`))) {
  await import(pathToFileURL(file));
}

console.log(`website syntax/import graph ok (${files.length} JavaScript modules)`);
