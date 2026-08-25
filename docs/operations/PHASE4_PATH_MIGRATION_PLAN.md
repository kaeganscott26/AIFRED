# Phase 4 Path Migration Plan

> Historical migration record captured before the 2026-08-25 production promotion. Preserve as evidence; current authority is `website-cloudflare-production-2026-08-25.md`.

Phase 4 does not switch live paths. It proves readiness for a later, explicitly approved migration.

## Current Live Behavior

- Existing `.github/workflows/build.yml` still references old `website/`.
- Existing `.github/workflows/build.yml` may deploy Cloudflare Pages from old `website/` on `main` when conditions and secrets exist.
- Existing release and tag publishing behavior remains unchanged.
- Existing package, plugin, engine, and installer paths remain unchanged.

## Phase 4 Target

- Do not switch paths yet.
- Prove `apps/website` and `apps/admin-android` are ready with dry-run reports.
- Keep `plugin-aifred` unchanged.
- Keep `tools/AifredEngine` unchanged.
- Keep `website/` and `android_admin/` preserved until later smoke tests and migration proof pass.

## Future Migration Plan

1. Confirm Cloudflare Pages project binding manually.
2. Confirm production source repo, production branch, and root directory in the Cloudflare dashboard.
3. Run `python3 tools/release/aifred_website_dryrun_check.py --check`.
4. Run the manual `AIFRED Monorepo Validation` workflow.
5. Create a later branch that updates workflow references from `website/` to `apps/website`.
6. Use preview or non-production validation before production deploy.
7. Only after `apps/website` deployment is proven, plan removal of old `website/`.
8. Only after `apps/admin-android` build readiness is proven, plan removal of old `android_admin/`.

## Explicit Warning

Do not merge path changes to `main` until asset strategy is decided and Cloudflare binding is manually verified.

Do not change the live Cloudflare deployment root in Phase 4.
