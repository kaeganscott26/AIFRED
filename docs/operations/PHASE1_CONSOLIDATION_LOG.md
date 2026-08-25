# Phase 1 Consolidation Log

> Historical migration record captured before the 2026-08-25 production promotion. Preserve as evidence; current authority is `website-cloudflare-production-2026-08-25.md`.

Date/time: 2026-06-27 03:13:41 CDT

Branch:

```text
aifred-consolidation-phase1
```

## What Was Copied

Website/backend authority:

- Copied `../aifred-site/website/` to `apps/website/`.
- Copied `../aifred-site/wrangler.toml` to `infra/cloudflare/wrangler.toml`.
- Copied `../aifred-site/cloudflare/` to `infra/cloudflare/cloudflare/`.
- Copied `../aifred-site/docs/Backend-Map.md` to `infra/cloudflare/docs/Backend-Map.md`.
- Copied `../aifred-site/docs/PAYPAL_R2_PIPELINE.md` to `infra/cloudflare/docs/PAYPAL_R2_PIPELINE.md`.
- Copied `../aifred-site/ops/` to `infra/cloudflare/ops/`.

Android admin authority:

- Copied selected project files from `../aifred-admin/` to `apps/admin-android/`.
- Included `app/`, `gradle/`, Gradle wrapper files, Kotlin Gradle build files, `gradle.properties`, `README.md`, `docs/`, and `tools/` where present.

Monorepo structure:

- Created `packages/plugin-juce/` as a Phase 1 reference wrapper.
- Created `packages/local-engine/` as a Phase 1 reference wrapper.
- Created `tools/installers/windows/`, `tools/installers/macos/`, `tools/installers/linux/`, and `tools/release/` for later consolidation.
- Created architecture, operations, and archive documentation folders.

## Intentionally Left Untouched

- `plugin-aifred/` remains the current runtime plugin source.
- `tools/AifredEngine/` remains the current runtime local engine source.
- `website/` remains in place as a previous in-repo website copy.
- `android_admin/` remains in place as a previous in-repo admin copy.
- `.github/workflows/` was not modified.
- No deployment command was run.
- No GitHub push was performed.
- No secrets or live credentials were edited.
- Sibling repositories were not deleted or moved.

## Commands Used

```sh
git status --short
git branch --show-current
git rev-parse --show-toplevel
find . -name .DS_Store -print
find . -name .DS_Store -delete
git switch aifred-consolidation-phase1 || git switch -c aifred-consolidation-phase1
mkdir -p apps/website apps/admin-android packages/plugin-juce packages/local-engine infra/cloudflare docs/architecture docs/operations docs/archive/flagship-contracts docs/archive/legacy-vstgui docs/archive/release-placeholder tools/installers/windows tools/installers/macos tools/installers/linux tools/release
rsync -a --exclude='.git/' --exclude='node_modules/' --exclude='.wrangler/' --exclude='dist/' --exclude='build/' --exclude='cache/' ../aifred-site/website/ apps/website/
rsync/cp selected Cloudflare files from ../aifred-site into infra/cloudflare/
rsync selected Android admin files from ../aifred-admin into apps/admin-android/
```

## Known Risks

- `AIFRED/website` and `apps/website` now coexist and must not both be treated as production deployment roots.
- `AIFRED/android_admin` and `apps/admin-android` now coexist and must not both be treated as the active admin app.
- Existing workflows still reference old paths.
- Release/download tags and R2 object versions still need live verification before any deploy.
- Local AI plugin/engine paths remain intentionally unmoved until build and package scripts are updated in a later phase.

## Next Steps

1. Run the smoke checks in `docs/operations/SMOKE_TESTS.md`.
2. Decide whether Phase 2 updates workflows and docs to point at `apps/website` and `apps/admin-android`.
3. Verify website worker syntax from the imported path without deploying.
4. Verify admin Gradle project from the imported path.
5. Plan plugin/engine path migration only after CMake, installer, package, and release workflow references are mapped.
