# Phase 11 Future Approval Instructions

> Historical migration record captured before the 2026-08-25 production promotion. Preserve as evidence; current authority is `website-cloudflare-production-2026-08-25.md`.

Future human approval must be explicit before any non-production preview. If any required approval text is missing, preview remains blocked.

Do not infer approval from casual language like "looks good" or "send it." Do not treat approval for planning as approval for deployment.

## Required Approval Text

Future approval must explicitly say:

- Approve non-production Cloudflare preview.
- Branch name.
- Commit SHA.
- Preview is non-production only.
- No merge to `main`.
- No production deployment.
- No deletion of `website/`.
- No deletion of `android_admin/`.
- No plugin/engine move.
- Rollback owner.
- Evidence recorder.
- Accepted or deferred asset decision.
- Cloudflare verification completed or explicitly deferred with reason.

## Still Not Approved By Future Preview Approval

Unless separately stated in a later phase, preview approval does not approve production deployment, release publishing, merge to `main`, secret changes, media deletion, Git LFS conversion, plugin movement, engine movement, or old-folder removal.
