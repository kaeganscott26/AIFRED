# Phase 7 Preview Authorization Package

Phase 7 does not authorize deployment by itself.

This package defines what a human must approve before a future non-production Cloudflare preview. Production remains unchanged, `website/` remains the fallback production root, `apps/website` remains a preview candidate only, and `main` remains unmerged.

## Required Approval Owner

Approval owner:

## Approval Date

Approval date:

## Preview Purpose

Authorize a future non-production preview validation of `apps/website` so humans can inspect route behavior, catalog/download surfaces, and rollback readiness before any production migration is considered.

## Preview Scope

- Preview candidate: `apps/website`.
- Fallback production root: `website/`.
- Existing production workflow behavior remains unchanged.
- Existing release publishing behavior remains unchanged.
- `plugin-aifred/` and `tools/AifredEngine/` remain unmoved.
- `website/` and `android_admin/` remain preserved.

## Explicit Non-Production Boundary

- Preview approval is not production deployment approval.
- Preview approval is not approval to merge to `main`.
- Preview approval is not approval to switch Cloudflare production paths.
- Preview approval is not approval to publish releases.
- Preview approval is not approval to write secrets to Git.

## Required Checklist Inputs From Phase 6

- Phase 6 preview approval checklist has been completed by a human.
- Phase 6 merge blocker report has been reviewed.
- Phase 6 production non-change statement has been reviewed.
- Phase 6 preview runbook draft has been reviewed.
- Phase 6 asset acceptance checklist has been reviewed.

## Required Cloudflare Manual Verification

- Pages project identity has been confirmed.
- GitHub repository binding has been confirmed.
- Production branch and production root have been confirmed.
- Preview behavior has been confirmed as non-production.
- R2 bindings have been confirmed without exposing values.
- PayPal environment variables have been confirmed without exposing values.
- Admin auth environment variables have been confirmed without exposing values.
- Domain binding and rollback path have been confirmed.

## Required Asset Strategy Acknowledgement

The approving human acknowledges that duplicated website media remains a merge blocker until the asset strategy is explicitly accepted.

Asset strategy acknowledged by:

## Required Rollback Acknowledgement

The approving human acknowledges that `website/` remains the fallback production path and that no production path change should be merged unless rollback has been proven in a later explicit phase.

Rollback acknowledged by:

## Required Abort Criteria Acknowledgement

The approving human acknowledges the Phase 7 abort criteria and agrees that any matching condition blocks promotion or requires stopping the preview process.

Abort criteria acknowledged by:

## Final Human Approval

Final approval owner:

Final approval date:

Approved branch:

Approved commit:

Approved action:

Conditions:

Signature or initials:
