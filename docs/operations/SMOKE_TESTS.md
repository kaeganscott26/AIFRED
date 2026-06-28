# Smoke Tests

These checks are safe for Phase 1. Do not deploy, push, delete, or run destructive cleanup from this checklist.

## Website

Inspect imported files:

```sh
test -d apps/website && echo "apps/website imported"
test -f apps/website/_worker.js && echo "worker present"
test -f apps/website/functions/api/v1/[[path]].js && echo "api v1 worker present"
test -f apps/website/functions/api/[[path]].js && echo "legacy api worker present"
test -f apps/website/functions/ws/chat.js && echo "websocket worker present"
test -f apps/website/index.html && echo "website frontend present"
```

Optional syntax checks:

```sh
node --check apps/website/app.js
node --check apps/website/_worker.js
node --check 'apps/website/functions/api/v1/[[path]].js'
node --check 'apps/website/functions/api/[[path]].js'
node --check apps/website/functions/ws/chat.js
```

Do not run Cloudflare deploy commands in Phase 1.

## Admin

Inspect imported files:

```sh
test -d apps/admin-android && echo "apps/admin-android imported"
test -f apps/admin-android/settings.gradle.kts && echo "settings present"
test -f apps/admin-android/build.gradle.kts && echo "root build present"
test -f apps/admin-android/app/build.gradle.kts && echo "app build present"
test -f apps/admin-android/gradlew && echo "gradle wrapper present"
```

Optional Gradle discovery if the local Android/Java environment is ready:

```sh
cd apps/admin-android
./gradlew tasks
```

Do not publish or upload an APK in Phase 1.

## Plugin

Confirm the runtime plugin source remains in the original path:

```sh
test -d plugin-aifred && echo "plugin-aifred present"
test -f CMakeLists.txt && echo "root CMake present"
test -f plugin-aifred/CMakeLists.txt && echo "plugin CMake present"
```

Do not move the plugin in Phase 1.

## Engine

Confirm the runtime local engine remains in the original path and still documents the canonical local routing:

```sh
test -d tools/AifredEngine && echo "AifredEngine present"
rg -n "127\\.0\\.0\\.1:8787|127\\.0\\.0\\.1:11434|aifred:latest" tools/AifredEngine tools/windows tools/macos models/aifred
```

Do not merge the local engine with the Cloudflare backend.

## Git

Confirm no runtime files were deleted:

```sh
git status --short
test -d plugin-aifred && echo "plugin-aifred present"
test -d tools/AifredEngine && echo "AifredEngine present"
test -d website && echo "old website copy preserved"
test -d android_admin && echo "old admin copy preserved"
```

## Phase 2 Validation Tooling

Default validation is safe and read-only:

```sh
./tools/release/aifred_monorepo_validate.sh
```

Optional Gradle task discovery:

```sh
./tools/release/aifred_monorepo_validate.sh --gradle
```

Default mode checks path authority, imported website worker files, JavaScript syntax when `node` is available, Android Gradle file presence, plugin/engine file presence, backend reference separation, and excluded-folder hygiene.

`--gradle` only runs `./gradlew tasks` inside `apps/admin-android`. It does not run a release build, publish an APK, deploy Cloudflare, push Git commits, or delete files.

Do not run deploys, pushes, or destructive cleanup from Phase 2 smoke tests.

## Phase 3 Workflow Safety Checks

Run the monorepo validator:

```sh
./tools/release/aifred_monorepo_validate.sh
```

Confirm checked-in generated reports are current:

```sh
python3 tools/release/aifred_repo_inventory.py --check
python3 tools/release/aifred_workflow_audit.py --check
```

The GitHub Actions validation workflow is manual-only through `workflow_dispatch`.

It should not:

- deploy,
- push,
- publish releases,
- upload APKs,
- upload VST3 artifacts,
- or use repository secrets.

Do not run Cloudflare deploy commands, release publishing commands, pushes, or destructive cleanup from Phase 3 smoke tests.

## Phase 4 Deployment Dry-Run Checks

Run the monorepo validator:

```sh
./tools/release/aifred_monorepo_validate.sh
```

Generate and check website dry-run readiness:

```sh
python3 tools/release/aifred_website_dryrun_check.py
python3 tools/release/aifred_website_dryrun_check.py --check
```

Generate and check admin dry-run readiness:

```sh
python3 tools/release/aifred_admin_dryrun_check.py
python3 tools/release/aifred_admin_dryrun_check.py --check
```

Confirm workflow audit is current:

```sh
python3 tools/release/aifred_workflow_audit.py --check
```

Phase 4 smoke tests must not:

- deploy,
- push,
- merge,
- run Gradle builds by default,
- run Cloudflare commands,
- publish releases,
- delete old runtime folders,
- move `plugin-aifred`,
- or move `tools/AifredEngine`.

## Phase 5 Preview Planning Checks

Run the Phase 5 validator and report checks:

```sh
./tools/release/aifred_monorepo_validate.sh
python3 tools/release/aifred_website_dryrun_check.py --check
python3 tools/release/aifred_admin_dryrun_check.py --check
python3 tools/release/aifred_workflow_audit.py --check
python3 tools/release/aifred_repo_inventory.py --check
```

Manual workflow notes:

- `AIFRED Monorepo Validation` is manual-only.
- `AIFRED Website Preview Dry-Run` is manual-only.
- Neither workflow deploys.
- Neither workflow uses secrets.
- Neither workflow publishes releases.
- Neither workflow uploads artifacts.

Phase 5 smoke tests must not deploy, push, merge, run Gradle, run Cloudflare commands, publish releases, delete old folders, move `plugin-aifred`, or move `tools/AifredEngine`.

## Phase 6 Preview Gate Checks

Run the Phase 6 validator and report checks:

```sh
./tools/release/aifred_monorepo_validate.sh
python3 tools/release/aifred_website_dryrun_check.py --check
python3 tools/release/aifred_admin_dryrun_check.py --check
python3 tools/release/aifred_workflow_audit.py --check
python3 tools/release/aifred_repo_inventory.py --check
```

Phase 6 smoke tests must not:

- deploy,
- push,
- merge,
- run any Cloudflare command,
- run a Gradle build,
- publish releases,
- move `plugin-aifred`,
- move `tools/AifredEngine`,
- delete `website/`,
- or delete `android_admin/`.

## Phase 7 Preview Authorization Checks

Run the Phase 7 validator and report checks:

```sh
./tools/release/aifred_monorepo_validate.sh
python3 tools/release/aifred_website_dryrun_check.py --check
python3 tools/release/aifred_admin_dryrun_check.py --check
python3 tools/release/aifred_workflow_audit.py --check
python3 tools/release/aifred_repo_inventory.py --check
```

Phase 7 smoke tests must not:

- deploy,
- push,
- merge,
- run any Cloudflare command,
- run a Gradle build,
- publish releases,
- move `plugin-aifred`,
- move `tools/AifredEngine`,
- delete `website/`,
- or delete `android_admin/`.

## Phase 8 Local Preview Preflight Checks

Run the Phase 8 validator and report checks:

```sh
./tools/release/aifred_monorepo_validate.sh
python3 tools/release/aifred_website_parity_manifest.py --check
python3 tools/release/aifred_admin_parity_manifest.py --check
python3 tools/release/aifred_preview_gate_report.py --check
python3 tools/release/aifred_website_dryrun_check.py --check
python3 tools/release/aifred_admin_dryrun_check.py --check
python3 tools/release/aifred_workflow_audit.py --check
python3 tools/release/aifred_repo_inventory.py --check
```

Phase 8 smoke tests must not:

- deploy,
- push,
- merge,
- run any Cloudflare command,
- run a Gradle build,
- publish releases,
- move `plugin-aifred`,
- move `tools/AifredEngine`,
- delete `website/`,
- or delete `android_admin/`.

## Phase 9 Human Preview Review Closure Checks

Run the Phase 9 validator and report checks:

```sh
./tools/release/aifred_monorepo_validate.sh
python3 tools/release/aifred_website_parity_manifest.py --check
python3 tools/release/aifred_admin_parity_manifest.py --check
python3 tools/release/aifred_preview_gate_report.py --check
python3 tools/release/aifred_website_dryrun_check.py --check
python3 tools/release/aifred_admin_dryrun_check.py --check
python3 tools/release/aifred_workflow_audit.py --check
python3 tools/release/aifred_repo_inventory.py --check
```

Phase 9 smoke tests must not:

- deploy,
- push,
- merge,
- run any Cloudflare command,
- run a Gradle build,
- publish releases,
- move `plugin-aifred`,
- move `tools/AifredEngine`,
- delete `website/`,
- delete `android_admin/`,
- or create fake preview evidence.
