# Phase 5 Preview Migration Plan

> Historical migration record captured before the 2026-08-25 production promotion. Preserve as evidence; current authority is `website-cloudflare-production-2026-08-25.md`.

## Purpose

Phase 5 prepares a future preview or non-production migration for `apps/website`.

Phase 5 does not deploy. Phase 5 does not change production. Phase 5 does not remove old `website/`.

## Preview Migration Concept

- Existing production deployment remains old `website/`.
- `apps/website` should be tested as a preview deployment root only after Cloudflare manual verification.
- Any real preview deployment must be explicitly manual, explicitly non-production, and explicitly approved in a later phase.
- The current `build.yml` production deployment behavior remains unchanged.

## Required Manual Prerequisites Before Preview

- Cloudflare Pages project binding verified.
- Production branch and root verified.
- Preview deployment behavior verified.
- R2 bindings verified.
- PayPal environment variables verified without exposing values.
- Admin auth environment variables verified without exposing values.
- Domain binding verified.
- Existing production deployment rollback path verified.

## Preview Readiness Sequence

1. Run the monorepo validator.
2. Run the website dry-run checker.
3. Run the workflow audit checker.
4. Run the manual-only validation workflow.
5. Confirm the Cloudflare checklist manually.
6. Create a future preview-only workflow branch.
7. Run the preview workflow manually.
8. Inspect the preview URL manually.
9. Do not promote to production until approved.

## Rollback

- Keep `website/` as fallback production root.
- Keep `build.yml` unchanged until preview succeeds.
- If `apps/website` preview fails, do not merge path migration.
- Re-run the old `website/` deployment path if needed from the existing production workflow.

## Explicit Warning

Do not make `apps/website` the live production root in Phase 5.

Do not remove `website/` in Phase 5.
