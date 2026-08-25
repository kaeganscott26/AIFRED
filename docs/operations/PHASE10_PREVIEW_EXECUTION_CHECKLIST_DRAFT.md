# Phase 10 Preview Execution Checklist Draft

> Historical migration record captured before the 2026-08-25 production promotion. Preserve as evidence; current authority is `website-cloudflare-production-2026-08-25.md`.

This is a draft checklist only. Phase 10 does not execute the preview. Phase 10 does not authorize production. Phase 10 does not authorize merge to `main`.

A future preview requires explicit human approval outside this phase.

## Pre-Authorization

- [ ] Human review packet completed.
- [ ] Blocker closure checklist completed.
- [ ] Asset decision accepted or explicitly deferred.
- [ ] Cloudflare manual verification completed without exposing secrets.
- [ ] Rollback owner identified.
- [ ] Evidence recorder identified.
- [ ] Preview window identified.

## Local Preflight

- [ ] Monorepo validator passes.
- [ ] Website parity manifest check passes.
- [ ] Admin parity manifest check passes.
- [ ] Preview gate report check passes.
- [ ] Workflow audit check passes.
- [ ] Repo inventory check passes.

## GitHub Actions Dry-Run

- [ ] `AIFRED Monorepo Validation` workflow manually run.
- [ ] `AIFRED Website Preview Dry-Run` workflow manually run.
- [ ] Both workflows remain manual-only.
- [ ] No deploy workflow run.
- [ ] No production workflow run.
- [ ] No release workflow run.

## Cloudflare Preview

- [ ] Not implemented in Phase 10.
- [ ] Future preview command/process must be approved in a later phase.
- [ ] No Cloudflare command is included in this document.

## Post-Preview Evidence

- [ ] Preview URL recorded only after real preview exists.
- [ ] Homepage checked.
- [ ] Catalog checked.
- [ ] Download flow checked.
- [ ] API routes checked if preview backend exists.
- [ ] Console/network errors recorded.
- [ ] Production domain checked.
- [ ] Rollback need checked.
