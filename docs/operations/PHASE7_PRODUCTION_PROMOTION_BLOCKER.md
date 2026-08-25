# Phase 7 Production Promotion Blocker

> Historical migration record captured before the 2026-08-25 production promotion. Preserve as evidence; current authority is `website-cloudflare-production-2026-08-25.md`.

Phase 7 does not permit production promotion.

Production promotion requires a later explicit phase with separate approval, validation, and rollback evidence.

Before production promotion:

- Non-production preview must pass.
- Asset strategy must be accepted.
- Cloudflare binding must be verified.
- Rollback path must be proven.
- `.github/workflows/build.yml` path migration must be reviewed.
- Release workflow must be isolated from website path migration.
- Old `website/` removal must be delayed until after production proof.
