# Phase 8 Local Preview Preflight

## Purpose

Phase 8 provides local and pre-CI checks before any future non-production preview is requested. It compares preserved fallback paths with imported monorepo candidates, checks preview workflow safety, and records whether the repo is shaped for human review.

Phase 8 does not deploy.
Phase 8 does not switch production.
Phase 8 does not remove old folders.

## Commands

```sh
./tools/release/aifred_monorepo_validate.sh
python3 tools/release/aifred_website_parity_manifest.py
python3 tools/release/aifred_website_parity_manifest.py --check
python3 tools/release/aifred_admin_parity_manifest.py
python3 tools/release/aifred_admin_parity_manifest.py --check
python3 tools/release/aifred_preview_gate_report.py
python3 tools/release/aifred_preview_gate_report.py --check
python3 tools/release/aifred_workflow_audit.py --check
python3 tools/release/aifred_repo_inventory.py --check
```

## What Passing Means

- Local structure is ready for human review.
- Preview workflow remains manual and non-deploying.
- `apps/website` appears preview-candidate shaped.
- `apps/admin-android` appears task-discovery shaped.

## What Passing Does Not Mean

- Not production ready.
- Not deployed.
- Not merged.
- Not approved.
- Not asset strategy accepted.
- Not Cloudflare verified.

## Safety Boundaries

- Do not deploy.
- Do not run Cloudflare commands.
- Do not publish releases.
- Do not run Gradle by default.
- Do not delete `website/`.
- Do not delete `android_admin/`.
- Do not move `plugin-aifred/`.
- Do not move `tools/AifredEngine/`.
