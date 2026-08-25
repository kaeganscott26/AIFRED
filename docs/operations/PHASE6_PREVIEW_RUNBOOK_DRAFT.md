# Phase 6 Preview Runbook Draft

> Historical migration record captured before the 2026-08-25 production promotion. Preserve as evidence; current authority is `website-cloudflare-production-2026-08-25.md`.

This is a draft only. It does not contain real secrets and does not contain live commands that deploy.

## Purpose

Support future non-production preview validation of `apps/website` before any production path switch.

## Prerequisites

- Phase 6 preview approval checklist is completed by a human.
- Phase 6 branch validation has passed.
- `main` remains untouched.
- `website/` remains the production path.
- No secrets have been written to Git.
- Cloudflare manual checks have been completed outside this repository without exposing values.

## Preflight Local Commands

```sh
./tools/release/aifred_monorepo_validate.sh
python3 tools/release/aifred_website_dryrun_check.py --check
python3 tools/release/aifred_workflow_audit.py --check
python3 tools/release/aifred_repo_inventory.py --check
```

## Manual GitHub Actions Command Concept

Run `AIFRED Website Preview Dry-Run` manually from GitHub Actions.

This confirms the checked-in dry-run workflow shape. It is not production approval.

## Manual Cloudflare Preview Concept

not implemented in Phase 6.

## Verification

- Inspect the preview URL manually.
- Check the homepage.
- Check the catalog.
- Check API route behavior if a preview backend exists.
- Confirm no production domain changed.

## Rollback

- Do nothing if preview fails.
- Keep the old `website/` production path.
- Do not merge a production path change.
