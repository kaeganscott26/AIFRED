#!/usr/bin/env bash
set -euo pipefail

run_gradle=0
for arg in "$@"; do
  case "$arg" in
    --gradle)
      run_gradle=1
      ;;
    -h|--help)
      cat <<'USAGE'
Usage: tools/release/aifred_monorepo_validate.sh [--gradle]

Default mode is safe and read-only. It checks the final consolidated monorepo
shape, active website/backend routes, local engine routing, and workflow paths.
--gradle runs Gradle task discovery in apps/admin-android only.
USAGE
      exit 0
      ;;
    *)
      echo "Unknown argument: $arg" >&2
      exit 2
      ;;
  esac
done

section() {
  printf '\n== %s ==\n' "$1"
}

pass() {
  printf '[ok] %s\n' "$1"
}

warn() {
  printf '[warn] %s\n' "$1"
}

require_dir() {
  local path="$1"
  if [ ! -d "$path" ]; then
    echo "Missing required directory: $path" >&2
    exit 1
  fi
  pass "directory exists: $path"
}

require_file() {
  local path="$1"
  if [ ! -f "$path" ]; then
    echo "Missing required file: $path" >&2
    exit 1
  fi
  pass "file exists: $path"
}

forbid_path() {
  local path="$1"
  if [ -e "$path" ]; then
    echo "Stale path must not exist in the final monorepo: $path" >&2
    exit 1
  fi
  pass "stale path absent: $path"
}

require_reference() {
  local needle="$1"
  shift
  if ! grep -R -F -q --exclude-dir=.git "$needle" "$@"; then
    echo "Missing required reference '$needle' under: $*" >&2
    exit 1
  fi
  pass "reference found: $needle"
}

forbid_reference() {
  local needle="$1"
  shift
  if grep -R -F -q --exclude-dir=.git "$needle" "$@"; then
    echo "Forbidden reference '$needle' found under: $*" >&2
    grep -R -F -n --exclude-dir=.git "$needle" "$@" >&2 || true
    exit 1
  fi
  pass "forbidden reference absent: $needle"
}

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
cd "$repo_root"

section "AIFRED Final Monorepo Validation"
pass "repo root: $repo_root"
pass "branch: $(git branch --show-current 2>/dev/null || echo unknown)"
pass "commit: $(git rev-parse --short HEAD 2>/dev/null || echo unknown)"

section "Final Directory Authorities"
for path in \
  apps/website \
  apps/admin-android \
  plugin-aifred \
  tools/AifredEngine \
  infra/cloudflare \
  packages/plugin-juce \
  packages/local-engine
do
  require_dir "$path"
done

for path in website android_admin; do
  forbid_path "$path"
done

section "Website Backend Files"
for path in \
  apps/website/_worker.js \
  'apps/website/functions/api/v1/[[path]].js' \
  'apps/website/functions/api/[[path]].js' \
  apps/website/functions/ws/chat.js \
  apps/website/index.html \
  apps/website/app.js \
  apps/website/config.js \
  apps/website/styles.css \
  apps/website/assets/data/beat_catalog.json \
  apps/website/wrangler.toml
do
  require_file "$path"
done

section "Website JavaScript Syntax"
if command -v node >/dev/null 2>&1; then
  for path in \
    apps/website/app.js \
    apps/website/_worker.js \
    'apps/website/functions/api/v1/[[path]].js' \
    'apps/website/functions/api/[[path]].js' \
    apps/website/functions/ws/chat.js \
    tools/serve-website.mjs
  do
    node --check "$path"
    pass "node --check: $path"
  done
else
  warn "node is not available; skipped JavaScript syntax checks"
fi

section "Website R2 And Backend Routing"
require_reference "AIFRED_WEBSITE_ASSETS" apps/website infra/cloudflare docs README.md
require_reference "/api/v1/assets/audio/catalog" apps/website apps/admin-android docs README.md
require_reference "AIFRED_DOWNLOADS" apps/website infra/cloudflare docs README.md
require_reference "AIFRED_REFERENCE_BUCKET" apps/website infra/cloudflare docs README.md
require_reference "kaeganscott26/AIFRED" apps/website apps/admin-android docs README.md
forbid_reference "kaeganscott26/aifred-site" apps/website apps/admin-android .github README.md

section "Android Admin Shape"
for path in \
  apps/admin-android/settings.gradle.kts \
  apps/admin-android/build.gradle.kts \
  apps/admin-android/app/build.gradle.kts \
  apps/admin-android/gradlew \
  apps/admin-android/app/src/main/AndroidManifest.xml \
  apps/admin-android/app/src/main/java/com/aifred/admin/MainActivity.kt
do
  require_file "$path"
done

forbid_reference "AIFRED_API_TOKEN=sk-" apps/admin-android
require_reference "apps/website" apps/admin-android/app/src/main/java/com/aifred/admin/MainActivity.kt
require_reference "aifred:latest" apps/admin-android tools/AifredEngine plugin-aifred README.md
require_reference "gpt-5.6-luna" apps/admin-android apps/website tools/AifredEngine README.md

if [ "$run_gradle" -eq 1 ]; then
  section "Android Admin Gradle Task Discovery"
  (cd apps/admin-android && ./gradlew tasks)
  pass "Gradle task discovery completed"
else
  pass "Gradle task discovery skipped; pass --gradle to run ./gradlew tasks"
fi

section "Plugin And Local Engine"
for path in \
  CMakeLists.txt \
  plugin-aifred/CMakeLists.txt \
  CMakePresets.json \
  tools/AifredEngine/Program.cs
do
  require_file "$path"
done
require_reference "127.0.0.1:8787" tools/AifredEngine plugin-aifred tools/windows tools/macos models/aifred docs README.md
require_reference "127.0.0.1:11434" tools/AifredEngine plugin-aifred tools/windows tools/macos models/aifred docs README.md
require_reference "https://api.openai.com/v1" tools/AifredEngine apps/admin-android apps/website docs README.md

section "Nested Repo And Excluded Folder Checks"
if find apps infra -type d -name .git -print -quit | grep -q .; then
  echo "Unexpected .git directory found under apps/ or infra/" >&2
  find apps infra -type d -name .git -print >&2
  exit 1
fi
pass "no nested .git directories under apps/ or infra/"

excluded_found="$(find apps infra \( \
  -name node_modules -o \
  -name .wrangler -o \
  -name .gradle -o \
  -name build -o \
  -name dist -o \
  -name cache -o \
  -name local.properties -o \
  -name .env -o \
  -name '.env.*' \
\) -print)"
if [ -n "$excluded_found" ]; then
  echo "Unexpected excluded paths found under apps/ or infra/:" >&2
  printf '%s\n' "$excluded_found" >&2
  exit 1
fi
pass "no excluded folders/files found under apps/ or infra/"

section "Workflow Path Checks"
require_file .github/workflows/build.yml
require_file .github/workflows/aifred-monorepo-validate.yml
require_file .github/workflows/aifred-website-preview-dryrun.yml
require_reference "apps/website" .github/workflows
forbid_reference "pages deploy website" .github/workflows
forbid_reference "node --check website/" .github/workflows

section "Stale Sibling Repo Deletion Guard"
pass "GitHub repository deletion is intentionally not automated by this validator."
pass "Delete/archive stale GitHub repositories only after an explicit reviewed repository-name list."

section "Validation Complete"
pass "final monorepo shape is valid"
