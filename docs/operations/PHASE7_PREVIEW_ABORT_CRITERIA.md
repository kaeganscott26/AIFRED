# Phase 7 Preview Abort Criteria

> Historical migration record captured before the 2026-08-25 production promotion. Preserve as evidence; current authority is `website-cloudflare-production-2026-08-25.md`.

A future preview must be aborted or not promoted if any of these conditions occur:

- Production domain changes unexpectedly.
- Preview requires secrets to be written to Git.
- Cloudflare project binding is unclear.
- R2 binding is missing or unclear.
- PayPal environment variables are missing or unclear.
- Admin auth environment variables are missing or unclear.
- Preview deployment cannot be confirmed as non-production.
- `apps/website` routes fail.
- Download or catalog flow breaks.
- Existing `website/` fallback path is not available.
- Asset strategy is still rejected or unresolved.
- Workflow attempts to deploy from `push` or `pull_request`.
- Workflow attempts release publishing.
- Workflow attempts artifact upload unexpectedly.

Any abort condition blocks production promotion and merge-to-main decisions until it is resolved in a later explicit phase.
