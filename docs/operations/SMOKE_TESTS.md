# Smoke Tests

These checks validate the final AIFRED monorepo shape. They do not deploy, push, publish releases, upload APKs, or delete GitHub repositories.

## Final Monorepo Validation

```sh
chmod +x tools/release/aifred_monorepo_validate.sh
./tools/release/aifred_monorepo_validate.sh
```

Optional Android task discovery:

```sh
./tools/release/aifred_monorepo_validate.sh --gradle
```

## Website And Cloudflare Backend

```sh
test -d apps/website
test -f apps/website/_worker.js
test -f 'apps/website/functions/api/v1/[[path]].js'
test -f 'apps/website/functions/api/[[path]].js'
test -f apps/website/functions/ws/chat.js
test -f apps/website/assets/data/beat_catalog.json
node --check apps/website/app.js
node --check apps/website/_worker.js
node --check 'apps/website/functions/api/v1/[[path]].js'
node --check 'apps/website/functions/api/[[path]].js'
node --check apps/website/functions/ws/chat.js
```

Required routing checks:

```sh
rg -n "AIFRED_WEBSITE_ASSETS|/api/v1/assets/audio/catalog|AIFRED_DOWNLOADS|AIFRED_REFERENCE_BUCKET" apps/website infra/cloudflare
```

Do not run Wrangler deploy commands from this smoke-test checklist.

## Plugin And Local Engine

```sh
test -d plugin-aifred
test -d tools/AifredEngine
test -f CMakeLists.txt
test -f plugin-aifred/CMakeLists.txt
dotnet build tools/AifredEngine/AifredEngine.csproj
cmake -S . -B build/aifred-final -DCMAKE_BUILD_TYPE=Release
cmake --build build/aifred-final --target Aifred --config Release --parallel
```

Expected local routes:

```sh
rg -n "127\\.0\\.0\\.1:8787|127\\.0\\.0\\.1:11434|aifred:latest|https://api.openai.com/v1" tools/AifredEngine plugin-aifred
```

## Android Admin

```sh
test -d apps/admin-android
test -f apps/admin-android/settings.gradle.kts
test -f apps/admin-android/build.gradle.kts
test -f apps/admin-android/app/build.gradle.kts
test -f apps/admin-android/gradlew
cd apps/admin-android
./gradlew tasks
```

Only build a debug APK when intentionally testing the Android app locally:

```sh
cd apps/admin-android
./gradlew :app:assembleDebug
```

Do not run `assembleRelease`, sign APKs, upload APKs, or attach Android artifacts to public releases from this checklist.

## Git And Cleanup Checks

```sh
test ! -e website
test ! -e android_admin
test ! -e North3rnlight3r_Beatz
git status --short
git diff --check
```

## Prohibited From Smoke Tests

- No deployment.
- No push.
- No merge.
- No Cloudflare command.
- No release publishing.
- No release artifact upload.
- No APK signing.
- No deletion of GitHub repositories without an explicit reviewed repository-name list.
- No fake preview evidence.
- No secrets in Git.
