# Phase 9 Human Preview Review Packet

> Historical migration record captured before the 2026-08-25 production promotion. Preserve as evidence; current authority is `website-cloudflare-production-2026-08-25.md`.

Phase 9 does not authorize deployment.
Phase 9 does not authorize production.
Phase 9 summarizes readiness for human review only.

Human approval is still required before any Cloudflare preview. Production remains unchanged. `website/` remains the fallback production root, `apps/website/` remains preview candidate only, and `apps/admin-android/` remains task-discovery candidate only. `plugin-aifred/` and `tools/AifredEngine/` remain unmoved.

## Current Branch And Source

- Current review branch: `aifred-consolidation-phase9-preview-review-closure`
- Expected source branch: `origin/aifred-consolidation-phase8-local-preview-harness`
- Expected source commit: Phase 8 local preview harness commit `158d1df`

## Review Objective

Prepare a human review package for a future non-production Cloudflare preview request without approving deployment, production promotion, merge to `main`, asset deletion, Git LFS conversion, release publishing, plugin movement, engine movement, or old-folder removal.

## Inputs To Review

- [ ] `docs/operations/PHASE6_PREVIEW_APPROVAL_CHECKLIST.md`
- [ ] `docs/operations/PHASE7_PREVIEW_AUTHORIZATION_PACKAGE.md`
- [ ] `docs/operations/PHASE8_WEBSITE_PARITY_MANIFEST.md`
- [ ] `docs/operations/PHASE8_ADMIN_PARITY_MANIFEST.md`
- [ ] `docs/operations/PHASE8_PREVIEW_GATE_REPORT.md`
- [ ] `docs/operations/CLOUDFLARE_MANUAL_VERIFICATION_CHECKLIST.md`
- [ ] `docs/operations/PHASE6_ASSET_ACCEPTANCE_CHECKLIST.md`

## Human Reviewer Fields

- Reviewer:
- Date:
- Decision:
- Notes:

## Allowed Decision Values

- approve future non-production preview planning
- request changes
- block preview

## Explicitly Not Approved

- Production deployment.
- Merge to `main`.
- Deleting `website/`.
- Deleting `android_admin/`.
- Moving `plugin-aifred/`.
- Moving `tools/AifredEngine/`.
- Publishing releases.
- Touching secrets.
