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

Default mode is safe and read-only. It checks paths, syntax, and references.
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

require_reference() {
  local needle="$1"
  shift
  if ! grep -R -F -q --exclude-dir=.git "$needle" "$@"; then
    echo "Missing required reference '$needle' under: $*" >&2
    exit 1
  fi
  pass "reference found: $needle"
}

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
cd "$repo_root"

section "AIFRED Monorepo Phase 2/3/4/5/6/7 Validation"
pass "repo root: $repo_root"
pass "branch: $(git branch --show-current 2>/dev/null || echo unknown)"
pass "commit: $(git rev-parse --short HEAD 2>/dev/null || echo unknown)"

section "Expected Directory Authorities"
for path in \
  apps/website \
  apps/admin-android \
  plugin-aifred \
  tools/AifredEngine \
  website \
  android_admin \
  infra/cloudflare \
  packages/plugin-juce \
  packages/local-engine
do
  require_dir "$path"
done

section "Coexistence Warnings"
if [ -d website ] && [ -d apps/website ]; then
  warn "both website/ and apps/website exist"
fi
if [ -d android_admin ] && [ -d apps/admin-android ]; then
  warn "both android_admin/ and apps/admin-android exist"
fi
pass "coexistence is expected during Phase 2/3/4/5/6/7"

section "Website Worker Files"
for path in \
  apps/website/_worker.js \
  'apps/website/functions/api/v1/[[path]].js' \
  'apps/website/functions/api/[[path]].js' \
  apps/website/functions/ws/chat.js
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
    apps/website/functions/ws/chat.js
  do
    require_file "$path"
    node --check "$path"
    pass "node --check: $path"
  done
else
  warn "node is not available; skipped JavaScript syntax checks"
fi

section "Android Admin Gradle Files"
for path in \
  apps/admin-android/settings.gradle.kts \
  apps/admin-android/build.gradle.kts \
  apps/admin-android/app/build.gradle.kts \
  apps/admin-android/gradlew
do
  require_file "$path"
done

if [ "$run_gradle" -eq 1 ]; then
  section "Android Admin Gradle Task Discovery"
  (cd apps/admin-android && ./gradlew tasks)
  pass "Gradle task discovery completed"
else
  pass "Gradle task discovery skipped; pass --gradle to run ./gradlew tasks"
fi

section "Plugin Runtime Files"
for path in \
  CMakeLists.txt \
  plugin-aifred/CMakeLists.txt \
  CMakePresets.json
do
  require_file "$path"
done

section "Local Engine Files"
require_file tools/AifredEngine/Program.cs

section "Local Engine Reference Checks"
require_reference "127.0.0.1:8787" tools/AifredEngine tools/windows tools/macos models/aifred docs packages README.md
require_reference "127.0.0.1:11434" tools/AifredEngine tools/windows tools/macos models/aifred docs packages README.md
require_reference "aifred:latest" tools/AifredEngine tools/windows tools/macos models/aifred docs packages README.md

section "Web Backend Reference Checks"
require_reference "north3rnlight3r.com" apps/website apps/admin-android infra/cloudflare docs README.md
require_reference "/api/v1" apps/website apps/admin-android infra/cloudflare docs README.md

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

section "Manual Validation Workflow Safety"
if [ -d .github/workflows ]; then
  pass ".github/workflows directory exists"
else
  warn ".github/workflows directory is not present"
fi

validation_workflow=.github/workflows/aifred-monorepo-validate.yml
require_file "$validation_workflow"

if ! grep -F -q "workflow_dispatch" "$validation_workflow"; then
  echo "Validation workflow must be manual-only and include workflow_dispatch" >&2
  exit 1
fi
pass "validation workflow contains workflow_dispatch"

if grep -E -q "^[[:space:]]+(push|pull_request):" "$validation_workflow"; then
  echo "Validation workflow must not include push or pull_request triggers" >&2
  exit 1
fi
pass "validation workflow does not include push or pull_request triggers"

for forbidden in \
  "wrangler deploy" \
  "gh release create" \
  "git push" \
  "assembleRelease" \
  "CLOUDFLARE_API_TOKEN" \
  "PAYPAL_CLIENT_SECRET" \
  "OPENAI_API_KEY"
do
  if grep -F -q "$forbidden" "$validation_workflow"; then
    echo "Validation workflow contains forbidden deployment/secret token: $forbidden" >&2
    exit 1
  fi
done
pass "validation workflow does not contain forbidden deployment or secret tokens"

section "Phase 4 Dry-Run Readiness Files"
for path in \
  tools/release/aifred_website_dryrun_check.py \
  tools/release/aifred_admin_dryrun_check.py \
  docs/operations/PHASE4_WEBSITE_DRYRUN_REPORT.md \
  docs/operations/PHASE4_ADMIN_DRYRUN_REPORT.md \
  docs/operations/PHASE4_PATH_MIGRATION_PLAN.md \
  docs/operations/CLOUDFLARE_MANUAL_VERIFICATION_CHECKLIST.md \
  docs/operations/RELEASE_WORKFLOW_SAFETY_CHECKLIST.md
do
  require_file "$path"
done

section "Phase 5 Preview Dry-Run Safety"
preview_workflow=.github/workflows/aifred-website-preview-dryrun.yml
require_file "$preview_workflow"

if ! grep -F -q "workflow_dispatch" "$preview_workflow"; then
  echo "Preview dry-run workflow must be manual-only and include workflow_dispatch" >&2
  exit 1
fi
pass "preview dry-run workflow contains workflow_dispatch"

if grep -E -q "^[[:space:]]+(push|pull_request):" "$preview_workflow"; then
  echo "Preview dry-run workflow must not include push or pull_request triggers" >&2
  exit 1
fi
pass "preview dry-run workflow does not include push or pull_request triggers"

for forbidden in \
  "wrangler deploy" \
  "pages deploy" \
  "CLOUDFLARE_API_TOKEN" \
  "CF_API_TOKEN" \
  "gh release create" \
  "git push" \
  "upload-artifact" \
  "actions/upload-artifact" \
  "assembleRelease" \
  "PAYPAL_CLIENT_SECRET" \
  "OPENAI_API_KEY"
do
  if grep -F -q "$forbidden" "$preview_workflow"; then
    echo "Preview dry-run workflow contains forbidden deployment/secret token: $forbidden" >&2
    exit 1
  fi
done
pass "preview dry-run workflow does not contain forbidden deployment, artifact, release, APK, or secret tokens"

section "Phase 5 Planning Files"
for path in \
  docs/operations/PHASE5_PREVIEW_MIGRATION_PLAN.md \
  docs/operations/PHASE5_ASSET_DECISION_RECORD.md \
  docs/operations/PHASE5_ROLLBACK_PLAN.md \
  docs/operations/PHASE5_ADMIN_GRADLE_DISCOVERY_PLAN.md
do
  require_file "$path"
done

section "Phase 6 Preview Gate Files"
for path in \
  docs/operations/PHASE6_PREVIEW_APPROVAL_CHECKLIST.md \
  docs/operations/PHASE6_MERGE_BLOCKER_REPORT.md \
  docs/operations/PHASE6_PRODUCTION_NON_CHANGE_STATEMENT.md \
  docs/operations/PHASE6_PREVIEW_RUNBOOK_DRAFT.md \
  docs/operations/PHASE6_ASSET_ACCEPTANCE_CHECKLIST.md
do
  require_file "$path"
done

section "Phase 7 Preview Authorization Files"
for path in \
  docs/operations/PHASE7_PREVIEW_AUTHORIZATION_PACKAGE.md \
  docs/operations/PHASE7_PREVIEW_EVIDENCE_TEMPLATE.md \
  docs/operations/PHASE7_PREVIEW_ABORT_CRITERIA.md \
  docs/operations/PHASE7_PRODUCTION_PROMOTION_BLOCKER.md \
  docs/operations/PHASE7_APPROVAL_RECORD_TEMPLATE.md
do
  require_file "$path"
done

section "Final Result"
pass "AIFRED monorepo Phase 7 validation PASS"
