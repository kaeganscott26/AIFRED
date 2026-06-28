# Phase 5 Rollback Plan

## Git Rollback

- Branches remain unmerged.
- `main` remains untouched.
- If Phase 5 is bad, abandon this branch.

## Website Rollback

- Existing `website/` remains preserved.
- Existing `build.yml` behavior remains unchanged.
- Existing `aifred-site` production authority remains valid until monorepo deployment is proven.

## Admin Rollback

- `android_admin` remains preserved.
- `apps/admin-android` is not release root yet.
- No APK release behavior changed.

## Plugin And Engine Rollback

- `plugin-aifred` remains canonical runtime plugin source.
- `tools/AifredEngine` remains canonical local engine source.
- No installer or release behavior changed.

## Asset Rollback

- No assets deleted.
- No Git LFS conversion.
- No history rewrite.

## Production Rollback

There are no production changes in Phase 5, so production rollback should not be needed.

If a later preview fails, do not promote.
