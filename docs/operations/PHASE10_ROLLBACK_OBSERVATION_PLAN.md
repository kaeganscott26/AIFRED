# Phase 10 Rollback Observation Plan

> Historical migration record captured before the 2026-08-25 production promotion. Preserve as evidence; current authority is `website-cloudflare-production-2026-08-25.md`.

Phase 10 does not trigger rollback because no production change occurs.

Future preview rollback means do not promote, do not merge, and keep the old `website/` path.

## Future Preview Observations

- [ ] Observe production domain before preview.
- [ ] Observe production domain after preview.
- [ ] Confirm `build.yml` remains unchanged.
- [ ] Confirm `website/` remains present.
- [ ] Confirm `apps/website/` preview candidate can be abandoned.
- [ ] Confirm no release workflow ran.
- [ ] Confirm no Cloudflare production route changed.
- [ ] Confirm no DNS/domain binding changed.
- [ ] Confirm no assets deleted.
- [ ] Confirm no secrets changed.

## Rollback Boundary

If a future preview fails, the immediate rollback action is to stop preview promotion planning. Production should remain on the existing path unless a separately approved production migration phase says otherwise.
