# Phase 3 Workflow Audit

Timestamp: 2026-06-28T00:09:33-05:00

Git branch: `aifred-consolidation-phase11-preview-authorization-decision`
Git commit: `2c669c1`

## Workflows Found

| File | Name | Triggers |
| --- | --- | --- |
| `.github/workflows/aifred-monorepo-validate.yml` | AIFRED Monorepo Validation | workflow_dispatch |
| `.github/workflows/aifred-website-preview-dryrun.yml` | AIFRED Website Preview Dry-Run | workflow_dispatch |
| `.github/workflows/build.yml` | Build AIFRED | push, pull_request, workflow_dispatch |

## Existing Workflow Path Usage

- References old `website/`: yes
- References new `apps/website`: yes
- References old `android_admin`: no
- References new `apps/admin-android`: no
- References `plugin-aifred`: no
- References `tools/AifredEngine`: no

## Deployment And Release Behavior Detection

- Old website deployment root detected: yes
- New `apps/website` deployment root detected: no
- Release publishing behavior detected: yes
- Tag-triggered release behavior detected: yes
- Manual-only monorepo validation workflow detected: yes

## Preview Dry-Run Workflow Detection

- Manual-only preview dry-run workflow detected: yes
- Preview workflow uses `apps/website`: yes
- Preview workflow uses secrets: no
- Preview workflow contains deploy/release/artifact commands: no

## Phase 6 Gate Summary

- Production workflow still unchanged: yes
- Preview dry-run workflow is manual-only: yes
- `apps/website` preview shape exists: yes
- Merge remains blocked by asset strategy and Cloudflare manual verification.

## Phase 7 Authorization Summary

- Preview authorization docs exist: yes
- Production promotion remains blocked.
- Preview workflow remains manual-only: yes
- Production workflow remains unchanged: yes
- Merge remains blocked pending human approval and asset strategy.

## Phase 8 Local Preview Harness Summary

- Website parity manifest exists: yes
- Admin parity manifest exists: yes
- Preview gate report exists: yes
- Preview workflow remains manual-only: yes
- Production workflow remains unchanged: yes
- Production promotion remains blocked: yes

## Phase 9 Review Closure Summary

- Human review packet exists: yes
- Blocker closure checklist exists: yes
- Preview evidence readiness checklist exists: yes
- Final non-approval statement exists: yes
- Preview readiness closure report exists: yes
- Production promotion remains blocked: yes
- Preview remains not executed.

## Phase 10 Evidence Planning Summary

- Preview execution checklist draft exists: yes
- Go/no-go criteria exists: yes
- Evidence capture table exists: yes
- Rollback observation plan exists: yes
- Preview evidence readiness report exists: yes
- Preview remains not executed.
- Production remains unchanged: yes
- Production promotion remains blocked: yes

## Phase 11 Approval Intake Summary

- Approval intake form exists: yes
- Decision record exists: yes
- Approval blocker summary exists: yes
- Future approval instructions exist: yes
- Decision closure report exists: yes
- Preview remains not executed.
- Production remains unchanged: yes
- Preview is not authorized in this phase.
- Production promotion remains blocked: yes

## Deployment-Related References By File

| File | References |
| --- | --- |
| `.github/workflows/build.yml` | `wrangler`, `pages deploy`, `cloudflare`, `CLOUDFLARE_API_TOKEN`, `project-name`, `north3rnlight3r`, `github release`, `gh release`, `upload-artifact`, `download-artifact`, `package-aifred`, `AifredWindowsInstaller`, `AifredWindowsUninstaller`, `VST3`, `cmake`, `dotnet publish` |
| `README.md` | `wrangler`, `pages deploy`, `cloudflare`, `project-name`, `north3rnlight3r`, `github release`, `package-aifred`, `AifredWindowsInstaller`, `AifredWindowsUninstaller`, `VST3`, `.vst3`, `cmake`, `dotnet publish`, `gradlew`, `openai` |
| `android_admin/AIFRED_ADMIN_APP_USE_GUIDE.md` | `cloudflare`, `north3rnlight3r`, `gradlew`, `openai`, `OPENAI_API_KEY`, `AIFRED_REFERENCE_POOL` |
| `android_admin/app/build.gradle.kts` | `north3rnlight3r` |
| `android_admin/app/src/main/java/com/aifred/admin/MainActivity.kt` | `north3rnlight3r`, `VST3`, `openai`, `PAYPAL` |
| `android_admin/gradle.properties` | `north3rnlight3r` |
| `android_admin/gradlew` | `gradlew` |
| `android_admin/gradlew.bat` | `gradlew` |
| `apps/admin-android/README.md` | `north3rnlight3r`, `gradlew`, `assembleRelease` |
| `apps/admin-android/app/build.gradle.kts` | `north3rnlight3r` |
| `apps/admin-android/app/src/main/java/com/aifred/admin/MainActivity.kt` | `north3rnlight3r`, `VST3`, `openai`, `PAYPAL` |
| `apps/admin-android/docs/ADMIN_DESKTOP_AND_MOBILE_MANUAL.md` | `north3rnlight3r`, `aifred-site` |
| `apps/admin-android/docs/OPERATION_LOG_2026-05-01.md` | `aifred-site`, `github release`, `VST3`, `.vst3`, `R2` |
| `apps/admin-android/gradle.properties` | `north3rnlight3r` |
| `apps/admin-android/gradlew` | `gradlew` |
| `apps/admin-android/gradlew.bat` | `gradlew` |
| `apps/admin-android/tools/windows-admin/AIFRED-Admin-Desktop.ps1` | `north3rnlight3r`, `aifred-site` |
| `apps/website/.dev.vars.example` | `north3rnlight3r`, `AIFRED_RELEASE_VERSION`, `AIFRED_PLUGIN_RELEASE_TAG`, `AIFRED_GITHUB_REPO`, `openai`, `OPENAI_API_KEY`, `PAYPAL` |
| `apps/website/app.js` | `north3rnlight3r`, `aifred-site`, `PAYPAL` |
| `apps/website/assets/data/beat_catalog.json` | `north3rnlight3r` |
| `apps/website/assets/docs/aifred-beta-release-notes.txt` | `north3rnlight3r`, `VST3` |
| `apps/website/assets/docs/aifred-installation.txt` | `VST3`, `.vst3`, `openai`, `OPENAI_API_KEY`, `PAYPAL` |
| `apps/website/assets/docs/aifred-release-notes.txt` | `VST3`, `openai`, `PAYPAL` |
| `apps/website/assets/docs/aifred-system-requirements.txt` | `cloudflare`, `VST3`, `openai` |
| `apps/website/config.js` | `north3rnlight3r`, `PAYPAL` |
| `apps/website/functions/api/[[path]].js` | `cloudflare`, `github release`, `VST3` |
| `apps/website/functions/api/v1/[[path]].js` | `pages deploy`, `cloudflare`, `north3rnlight3r`, `aifred-site`, `AIFRED_RELEASE_VERSION`, `AIFRED_PLUGIN_RELEASE_TAG`, `AIFRED_GITHUB_REPO`, `VST3`, `openai`, `OPENAI_API_KEY`, `PAYPAL`, `R2`, `AIFRED_REFERENCE_POOL` |
| `apps/website/functions/ws/chat.js` | `cloudflare`, `north3rnlight3r`, `openai`, `OPENAI_API_KEY` |
| `apps/website/index.html` | `north3rnlight3r`, `VST3`, `PAYPAL` |
| `apps/website/styles.css` | `north3rnlight3r`, `PAYPAL` |
| `apps/website/wrangler.toml` | `aifred-site`, `R2`, `AIFRED_REFERENCE_POOL` |
| `docs/AIFRED_WINDOWS_RELEASE_ARCHITECTURE.md` | `package-aifred`, `AifredWindowsInstaller`, `VST3`, `.vst3`, `cmake`, `openai`, `OPENAI_API_KEY` |
| `docs/BACKEND-ROUTING-MACOS.md` | `package-aifred`, `package-aifred-macos`, `VST3`, `.vst3`, `dotnet publish`, `openai` |
| `docs/PLUGIN-STABILIZATION.md` | `aifred-site`, `VST3`, `cmake` |
| `docs/REFERENCE_PROFILE_SYNC_FOUNDATION.md` | `cloudflare`, `R2`, `AIFRED_REFERENCE_POOL` |
| `docs/RELEASE_NOTES.md` | `VST3`, `openai`, `PAYPAL`, `R2` |
| `docs/architecture/AIFRED_SYSTEM_MAP.md` | `cloudflare`, `north3rnlight3r`, `aifred-site`, `PAYPAL` |
| `docs/architecture/BACKEND_SEPARATION_CONTRACT.md` | `cloudflare`, `north3rnlight3r`, `PAYPAL` |
| `docs/architecture/PATH_AUTHORITY_MAP.md` | `cloudflare`, `aifred-site` |
| `docs/archive/release-placeholder/README.md` | `cloudflare`, `R2` |
| `docs/operations/CLOUDFLARE_MANUAL_VERIFICATION_CHECKLIST.md` | `wrangler`, `cloudflare`, `north3rnlight3r`, `PAYPAL`, `R2` |
| `docs/operations/MEDIA_ASSET_POLICY.md` | `cloudflare`, `aifred-site`, `R2` |
| `docs/operations/PHASE10_GO_NO_GO_CRITERIA.md` | `cloudflare` |
| `docs/operations/PHASE10_PREVIEW_EVIDENCE_CAPTURE_TABLE.md` | `cloudflare`, `PAYPAL` |
| `docs/operations/PHASE10_PREVIEW_EVIDENCE_READINESS_REPORT.md` | `cloudflare` |
| `docs/operations/PHASE10_PREVIEW_EXECUTION_CHECKLIST_DRAFT.md` | `cloudflare` |
| `docs/operations/PHASE10_ROLLBACK_OBSERVATION_PLAN.md` | `cloudflare` |
| `docs/operations/PHASE11_APPROVAL_BLOCKER_SUMMARY.md` | `cloudflare` |
| `docs/operations/PHASE11_DECISION_CLOSURE_REPORT.md` | `cloudflare` |
| `docs/operations/PHASE11_FUTURE_APPROVAL_INSTRUCTIONS.md` | `cloudflare` |
| `docs/operations/PHASE11_HUMAN_APPROVAL_INTAKE_FORM.md` | `cloudflare` |
| `docs/operations/PHASE11_PREVIEW_AUTHORIZATION_DECISION_RECORD.md` | `cloudflare` |
| `docs/operations/PHASE1_CONSOLIDATION_LOG.md` | `wrangler`, `cloudflare`, `aifred-site`, `cmake`, `PAYPAL`, `R2` |
| `docs/operations/PHASE3_ASSET_STRATEGY_RECOMMENDATION.md` | `cloudflare`, `north3rnlight3r`, `aifred-site`, `R2` |
| `docs/operations/PHASE3_WORKFLOW_LOG.md` | `pages deploy`, `cloudflare`, `github release` |
| `docs/operations/PHASE3_WORKFLOW_SAFETY.md` | `cloudflare`, `north3rnlight3r`, `aifred-site` |
| `docs/operations/PHASE4_PATH_MIGRATION_PLAN.md` | `cloudflare` |
| `docs/operations/PHASE5_ADMIN_GRADLE_DISCOVERY_PLAN.md` | `gradlew`, `assembleRelease` |
| `docs/operations/PHASE5_ASSET_DECISION_RECORD.md` | `cloudflare`, `R2` |
| `docs/operations/PHASE5_PREVIEW_MIGRATION_PLAN.md` | `cloudflare`, `PAYPAL`, `R2` |
| `docs/operations/PHASE5_ROLLBACK_PLAN.md` | `aifred-site` |
| `docs/operations/PHASE6_ASSET_ACCEPTANCE_CHECKLIST.md` | `R2` |
| `docs/operations/PHASE6_MERGE_BLOCKER_REPORT.md` | `cloudflare`, `R2` |
| `docs/operations/PHASE6_PREVIEW_APPROVAL_CHECKLIST.md` | `cloudflare`, `PAYPAL`, `R2` |
| `docs/operations/PHASE6_PREVIEW_RUNBOOK_DRAFT.md` | `cloudflare` |
| `docs/operations/PHASE6_PRODUCTION_NON_CHANGE_STATEMENT.md` | `cloudflare` |
| `docs/operations/PHASE7_PREVIEW_ABORT_CRITERIA.md` | `cloudflare`, `PAYPAL`, `R2` |
| `docs/operations/PHASE7_PREVIEW_AUTHORIZATION_PACKAGE.md` | `cloudflare`, `PAYPAL`, `R2` |
| `docs/operations/PHASE7_PREVIEW_EVIDENCE_TEMPLATE.md` | `cloudflare` |
| `docs/operations/PHASE7_PRODUCTION_PROMOTION_BLOCKER.md` | `cloudflare` |
| `docs/operations/PHASE8_LOCAL_PREVIEW_PREFLIGHT.md` | `cloudflare` |
| `docs/operations/PHASE9_BLOCKER_CLOSURE_CHECKLIST.md` | `cloudflare`, `PAYPAL`, `R2` |
| `docs/operations/PHASE9_FINAL_NON_APPROVAL_STATEMENT.md` | `cloudflare` |
| `docs/operations/PHASE9_HUMAN_PREVIEW_REVIEW_PACKET.md` | `cloudflare` |
| `docs/operations/PHASE9_PREVIEW_EVIDENCE_READINESS_CHECKLIST.md` | `cloudflare` |
| `docs/operations/PHASE9_PREVIEW_READINESS_CLOSURE_REPORT.md` | `cloudflare` |
| `docs/operations/RELEASE_WORKFLOW_SAFETY_CHECKLIST.md` | `github release` |
| `docs/operations/SMOKE_TESTS.md` | `cloudflare`, `VST3`, `cmake`, `gradlew` |
| `docs/wiki/Admin-App-Guide.md` | `cloudflare`, `github release`, `gradlew`, `openai`, `OPENAI_API_KEY` |
| `docs/wiki/Backend-Map.md` | `cloudflare`, `CLOUDFLARE_API_TOKEN`, `AIFRED_GITHUB_REPO`, `openai`, `OPENAI_API_KEY`, `PAYPAL`, `AIFRED_REFERENCE_POOL` |
| `docs/wiki/Developer-Guide.md` | `wrangler`, `pages deploy`, `cloudflare`, `project-name`, `north3rnlight3r`, `AifredWindowsInstaller`, `VST3`, `.vst3`, `cmake`, `dotnet publish`, `gradlew` |
| `docs/wiki/Function-Map.md` | `cloudflare`, `VST3`, `openai` |
| `docs/wiki/Home.md` | `cloudflare`, `north3rnlight3r`, `github release`, `VST3` |
| `docs/wiki/PayPal-Cloudflare-R2-Setup-Guide.md` | `wrangler`, `pages deploy`, `cloudflare`, `CLOUDFLARE_API_TOKEN`, `project-name`, `north3rnlight3r`, `github release`, `AIFRED_RELEASE_VERSION`, `VST3`, `PAYPAL`, `R2` |
| `docs/wiki/Security-And-Distribution.md` | `cloudflare`, `github release`, `openai`, `PAYPAL`, `R2` |
| `docs/wiki/Troubleshooting.md` | `wrangler`, `pages deploy`, `cloudflare`, `CLOUDFLARE_API_TOKEN`, `project-name`, `north3rnlight3r`, `AIFRED_GITHUB_REPO`, `VST3` |
| `docs/wiki/User-Guide.md` | `north3rnlight3r`, `VST3`, `.vst3`, `openai` |
| `infra/cloudflare/cloudflare/aifred-mailer.js` | `cloudflare`, `north3rnlight3r` |
| `infra/cloudflare/docs/Backend-Map.md` | `cloudflare`, `aifred-site`, `AIFRED_GITHUB_REPO`, `openai`, `OPENAI_API_KEY`, `PAYPAL`, `AIFRED_REFERENCE_POOL` |
| `infra/cloudflare/docs/PAYPAL_R2_PIPELINE.md` | `cloudflare`, `north3rnlight3r`, `aifred-site`, `github release`, `AIFRED_RELEASE_VERSION`, `AIFRED_PLUGIN_RELEASE_TAG`, `AIFRED_GITHUB_REPO`, `VST3`, `PAYPAL`, `R2`, `AIFRED_REFERENCE_POOL` |
| `infra/cloudflare/wrangler.toml` | `wrangler`, `aifred-site`, `R2`, `AIFRED_REFERENCE_POOL` |
| `tools/AifredEngine/Program.cs` | `openai`, `OPENAI_API_KEY` |
| `tools/AifredWindowsInstaller/AifredWindowsInstaller.csproj` | `AifredWindowsInstaller`, `VST3` |
| `tools/AifredWindowsInstaller/Program.cs` | `VST3`, `.vst3`, `openai`, `OPENAI_API_KEY` |
| `tools/AifredWindowsUninstaller/AifredWindowsUninstaller.csproj` | `AifredWindowsUninstaller` |
| `tools/AifredWindowsUninstaller/Program.cs` | `VST3`, `.vst3` |
| `tools/check-no-hardcoded-paths.ps1` | `cmake` |
| `tools/check_integrity.py` | `VST3`, `.vst3` |
| `tools/macos/package-aifred-macos.sh` | `setup-aifred-local-ai`, `VST3`, `.vst3`, `dotnet publish`, `openai`, `OPENAI_API_KEY` |
| `tools/package-aifred.ps1` | `setup-aifred-local-ai`, `VST3`, `.vst3`, `dotnet publish`, `openai`, `OPENAI_API_KEY` |
| `tools/release/aifred_admin_dryrun_check.py` | `wrangler`, `north3rnlight3r`, `gradlew` |
| `tools/release/aifred_admin_parity_manifest.py` | `wrangler`, `north3rnlight3r`, `gradlew`, `assembleRelease` |
| `tools/release/aifred_monorepo_validate.sh` | `wrangler`, `pages deploy`, `cloudflare`, `CLOUDFLARE_API_TOKEN`, `north3rnlight3r`, `gh release`, `upload-artifact`, `cmake`, `gradlew`, `assembleRelease`, `openai`, `OPENAI_API_KEY`, `PAYPAL` |
| `tools/release/aifred_preview_gate_report.py` | `wrangler`, `pages deploy`, `cloudflare`, `CLOUDFLARE_API_TOKEN`, `gh release`, `upload-artifact`, `download-artifact`, `assembleRelease`, `openai`, `OPENAI_API_KEY`, `PAYPAL` |
| `tools/release/aifred_repo_inventory.py` | `cloudflare`, `north3rnlight3r` |
| `tools/release/aifred_website_dryrun_check.py` | `wrangler`, `cloudflare`, `north3rnlight3r`, `github release`, `PAYPAL`, `R2` |
| `tools/release/aifred_website_parity_manifest.py` | `wrangler`, `north3rnlight3r`, `PAYPAL`, `R2` |
| `tools/release/aifred_workflow_audit.py` | `wrangler`, `pages deploy`, `cloudflare`, `CLOUDFLARE_API_TOKEN`, `project-name`, `north3rnlight3r`, `aifred-site`, `github release`, `gh release`, `upload-artifact`, `download-artifact`, `AIFRED_RELEASE_VERSION`, `AIFRED_PLUGIN_RELEASE_TAG`, `AIFRED_GITHUB_REPO`, `package-aifred`, `package-aifred-macos`, `setup-aifred-local-ai`, `AifredWindowsInstaller`, `AifredWindowsUninstaller`, `VST3`, `.vst3`, `msbuild`, `cmake`, `dotnet publish`, `gradlew`, `assembleRelease`, `openai`, `OPENAI_API_KEY`, `PAYPAL`, `R2`, `AIFRED_REFERENCE_POOL` |
| `tools/serve-website.mjs` | `cloudflare`, `north3rnlight3r`, `openai`, `OPENAI_API_KEY` |
| `tools/windows/setup-aifred-local-ai.ps1` | `openai`, `OPENAI_API_KEY` |
| `website/.dev.vars.example` | `north3rnlight3r`, `AIFRED_RELEASE_VERSION`, `AIFRED_PLUGIN_RELEASE_TAG`, `AIFRED_GITHUB_REPO`, `openai`, `OPENAI_API_KEY`, `PAYPAL` |
| `website/app.js` | `north3rnlight3r`, `aifred-site`, `github release`, `PAYPAL` |
| `website/assets/data/beat_catalog.json` | `north3rnlight3r` |
| `website/assets/docs/aifred-beta-release-notes.txt` | `north3rnlight3r`, `VST3`, `openai` |
| `website/assets/docs/aifred-installation.txt` | `VST3`, `.vst3`, `openai`, `OPENAI_API_KEY`, `PAYPAL` |
| `website/assets/docs/aifred-release-notes.txt` | `VST3`, `openai`, `PAYPAL` |
| `website/assets/docs/aifred-system-requirements.txt` | `cloudflare`, `VST3`, `openai` |
| `website/config.js` | `north3rnlight3r`, `PAYPAL` |
| `website/functions/api/[[path]].js` | `cloudflare`, `github release`, `VST3` |
| `website/functions/api/v1/[[path]].js` | `pages deploy`, `cloudflare`, `north3rnlight3r`, `aifred-site`, `AIFRED_RELEASE_VERSION`, `AIFRED_PLUGIN_RELEASE_TAG`, `AIFRED_GITHUB_REPO`, `VST3`, `openai`, `OPENAI_API_KEY`, `PAYPAL`, `R2`, `AIFRED_REFERENCE_POOL` |
| `website/functions/ws/chat.js` | `cloudflare`, `north3rnlight3r`, `openai`, `OPENAI_API_KEY` |
| `website/index.html` | `north3rnlight3r`, `VST3`, `openai` |
| `website/styles.css` | `north3rnlight3r`, `PAYPAL` |
| `website/wrangler.toml` | `aifred-site`, `R2`, `AIFRED_REFERENCE_POOL` |

## Path References By File

| File | References |
| --- | --- |
| `.github/workflows/aifred-website-preview-dryrun.yml` | `website/`, `apps/website` |
| `.github/workflows/build.yml` | `website/` |
| `README.md` | `website/`, `apps/website`, `android_admin`, `apps/admin-android`, `plugin-aifred`, `tools/AifredEngine` |
| `android_admin/app/src/main/java/com/aifred/admin/MainActivity.kt` | `website/` |
| `apps/admin-android/app/src/main/java/com/aifred/admin/MainActivity.kt` | `website/` |
| `apps/admin-android/docs/ADMIN_DESKTOP_AND_MOBILE_MANUAL.md` | `website/` |
| `apps/website/assets/docs/aifred-installation.txt` | `website/` |
| `apps/website/functions/api/v1/[[path]].js` | `website/` |
| `docs/AIFRED_WINDOWS_RELEASE_ARCHITECTURE.md` | `website/`, `android_admin`, `plugin-aifred`, `tools/AifredEngine` |
| `docs/BACKEND-ROUTING-MACOS.md` | `plugin-aifred`, `tools/AifredEngine` |
| `docs/architecture/AIFRED_SYSTEM_MAP.md` | `website/`, `apps/website`, `apps/admin-android`, `plugin-aifred`, `tools/AifredEngine` |
| `docs/architecture/BACKEND_SEPARATION_CONTRACT.md` | `apps/website`, `apps/admin-android`, `plugin-aifred` |
| `docs/architecture/PATH_AUTHORITY_MAP.md` | `website/`, `apps/website`, `android_admin`, `apps/admin-android`, `plugin-aifred`, `tools/AifredEngine` |
| `docs/operations/MEDIA_ASSET_POLICY.md` | `apps/website` |
| `docs/operations/PHASE10_GO_NO_GO_CRITERIA.md` | `website/`, `apps/website` |
| `docs/operations/PHASE10_ROLLBACK_OBSERVATION_PLAN.md` | `website/`, `apps/website` |
| `docs/operations/PHASE11_FUTURE_APPROVAL_INSTRUCTIONS.md` | `website/`, `android_admin` |
| `docs/operations/PHASE11_HUMAN_APPROVAL_INTAKE_FORM.md` | `website/`, `android_admin`, `plugin-aifred`, `tools/AifredEngine` |
| `docs/operations/PHASE1_CONSOLIDATION_LOG.md` | `website/`, `apps/website`, `android_admin`, `apps/admin-android`, `plugin-aifred`, `tools/AifredEngine` |
| `docs/operations/PHASE3_ASSET_STRATEGY_RECOMMENDATION.md` | `website/`, `apps/website` |
| `docs/operations/PHASE3_WORKFLOW_LOG.md` | `website/`, `apps/website`, `android_admin`, `plugin-aifred`, `tools/AifredEngine` |
| `docs/operations/PHASE3_WORKFLOW_SAFETY.md` | `apps/website`, `plugin-aifred`, `tools/AifredEngine` |
| `docs/operations/PHASE4_PATH_MIGRATION_PLAN.md` | `website/`, `apps/website`, `android_admin`, `apps/admin-android`, `plugin-aifred`, `tools/AifredEngine` |
| `docs/operations/PHASE5_ADMIN_GRADLE_DISCOVERY_PLAN.md` | `apps/admin-android` |
| `docs/operations/PHASE5_ASSET_DECISION_RECORD.md` | `website/`, `apps/website` |
| `docs/operations/PHASE5_PREVIEW_MIGRATION_PLAN.md` | `website/`, `apps/website` |
| `docs/operations/PHASE5_ROLLBACK_PLAN.md` | `website/`, `android_admin`, `apps/admin-android`, `plugin-aifred`, `tools/AifredEngine` |
| `docs/operations/PHASE6_MERGE_BLOCKER_REPORT.md` | `website/`, `apps/website`, `android_admin`, `apps/admin-android`, `plugin-aifred`, `tools/AifredEngine` |
| `docs/operations/PHASE6_PREVIEW_APPROVAL_CHECKLIST.md` | `website/`, `apps/website`, `android_admin`, `plugin-aifred`, `tools/AifredEngine` |
| `docs/operations/PHASE6_PREVIEW_RUNBOOK_DRAFT.md` | `website/`, `apps/website` |
| `docs/operations/PHASE6_PRODUCTION_NON_CHANGE_STATEMENT.md` | `website/`, `android_admin`, `plugin-aifred`, `tools/AifredEngine` |
| `docs/operations/PHASE7_APPROVAL_RECORD_TEMPLATE.md` | `website/`, `android_admin`, `plugin-aifred`, `tools/AifredEngine` |
| `docs/operations/PHASE7_PREVIEW_ABORT_CRITERIA.md` | `website/`, `apps/website` |
| `docs/operations/PHASE7_PREVIEW_AUTHORIZATION_PACKAGE.md` | `website/`, `apps/website`, `android_admin`, `plugin-aifred`, `tools/AifredEngine` |
| `docs/operations/PHASE7_PRODUCTION_PROMOTION_BLOCKER.md` | `website/` |
| `docs/operations/PHASE8_LOCAL_PREVIEW_PREFLIGHT.md` | `website/`, `apps/website`, `android_admin`, `apps/admin-android`, `plugin-aifred`, `tools/AifredEngine` |
| `docs/operations/PHASE9_BLOCKER_CLOSURE_CHECKLIST.md` | `website/`, `android_admin`, `plugin-aifred`, `tools/AifredEngine` |
| `docs/operations/PHASE9_FINAL_NON_APPROVAL_STATEMENT.md` | `website/`, `android_admin`, `plugin-aifred`, `tools/AifredEngine` |
| `docs/operations/PHASE9_HUMAN_PREVIEW_REVIEW_PACKET.md` | `website/`, `apps/website`, `android_admin`, `apps/admin-android`, `plugin-aifred`, `tools/AifredEngine` |
| `docs/operations/PHASE9_PREVIEW_READINESS_CLOSURE_REPORT.md` | `website/`, `apps/website`, `apps/admin-android` |
| `docs/operations/RELEASE_WORKFLOW_SAFETY_CHECKLIST.md` | `plugin-aifred`, `tools/AifredEngine` |
| `docs/operations/SMOKE_TESTS.md` | `website/`, `apps/website`, `android_admin`, `apps/admin-android`, `plugin-aifred`, `tools/AifredEngine` |
| `docs/wiki/Admin-App-Guide.md` | `website/`, `android_admin` |
| `docs/wiki/Backend-Map.md` | `website/` |
| `docs/wiki/Developer-Guide.md` | `android_admin` |
| `docs/wiki/Function-Map.md` | `website/`, `plugin-aifred` |
| `docs/wiki/PayPal-Cloudflare-R2-Setup-Guide.md` | `website/` |
| `docs/wiki/Security-And-Distribution.md` | `website/` |
| `docs/wiki/Troubleshooting.md` | `website/`, `android_admin` |
| `docs/wiki/User-Guide.md` | `website/` |
| `infra/cloudflare/docs/Backend-Map.md` | `website/` |
| `tools/check-aifred-analysis-regressions.ps1` | `plugin-aifred` |
| `tools/check-no-hardcoded-paths.ps1` | `apps/website`, `android_admin`, `plugin-aifred` |
| `tools/macos/package-aifred-macos.sh` | `website/`, `tools/AifredEngine` |
| `tools/release/aifred_admin_dryrun_check.py` | `android_admin`, `apps/admin-android` |
| `tools/release/aifred_admin_parity_manifest.py` | `android_admin`, `apps/admin-android` |
| `tools/release/aifred_monorepo_validate.sh` | `website/`, `apps/website`, `android_admin`, `apps/admin-android`, `plugin-aifred`, `tools/AifredEngine` |
| `tools/release/aifred_preview_gate_report.py` | `website/`, `apps/website`, `android_admin`, `apps/admin-android`, `plugin-aifred`, `tools/AifredEngine` |
| `tools/release/aifred_repo_inventory.py` | `website/`, `apps/website`, `android_admin`, `apps/admin-android`, `plugin-aifred`, `tools/AifredEngine` |
| `tools/release/aifred_website_dryrun_check.py` | `website/`, `apps/website` |
| `tools/release/aifred_website_parity_manifest.py` | `website/`, `apps/website` |
| `tools/release/aifred_workflow_audit.py` | `website/`, `apps/website`, `android_admin`, `apps/admin-android`, `plugin-aifred`, `tools/AifredEngine` |
| `website/assets/docs/aifred-installation.txt` | `website/` |
| `website/functions/api/v1/[[path]].js` | `website/` |

## Potential Duplicate Deployment Authorities

- Workflows reference both `website/` and `apps/website`.
- No workflow Android admin path reference detected.

## Deployment-Looking Command Warnings

| File | References |
| --- | --- |
| `.github/workflows/build.yml` | `wrangler deploy`, `pages deploy`, `gh release`, `upload-artifact`, `download-artifact`, `dotnet publish`, `cmake` |
| `README.md` | `pages deploy`, `dotnet publish`, `cmake` |
| `apps/admin-android/README.md` | `assembleRelease` |
| `apps/website/functions/api/v1/[[path]].js` | `pages deploy` |
| `docs/AIFRED_WINDOWS_RELEASE_ARCHITECTURE.md` | `cmake` |
| `docs/BACKEND-ROUTING-MACOS.md` | `dotnet publish` |
| `docs/PLUGIN-STABILIZATION.md` | `cmake` |
| `docs/operations/PHASE1_CONSOLIDATION_LOG.md` | `cmake` |
| `docs/operations/PHASE3_WORKFLOW_LOG.md` | `pages deploy` |
| `docs/operations/PHASE5_ADMIN_GRADLE_DISCOVERY_PLAN.md` | `assembleRelease` |
| `docs/operations/SMOKE_TESTS.md` | `cmake` |
| `docs/wiki/Developer-Guide.md` | `pages deploy`, `dotnet publish`, `cmake` |
| `docs/wiki/PayPal-Cloudflare-R2-Setup-Guide.md` | `pages deploy` |
| `docs/wiki/Troubleshooting.md` | `pages deploy` |
| `tools/check-no-hardcoded-paths.ps1` | `cmake` |
| `tools/macos/package-aifred-macos.sh` | `dotnet publish` |
| `tools/package-aifred.ps1` | `dotnet publish` |
| `tools/release/aifred_admin_parity_manifest.py` | `assembleRelease` |
| `tools/release/aifred_monorepo_validate.sh` | `wrangler deploy`, `pages deploy`, `gh release`, `upload-artifact`, `assembleRelease`, `cmake` |
| `tools/release/aifred_preview_gate_report.py` | `wrangler deploy`, `pages deploy`, `gh release`, `upload-artifact`, `download-artifact`, `assembleRelease` |
| `tools/release/aifred_workflow_audit.py` | `wrangler deploy`, `pages deploy`, `gh release`, `upload-artifact`, `download-artifact`, `assembleRelease`, `dotnet publish`, `cmake`, `msbuild` |
| `website/functions/api/v1/[[path]].js` | `pages deploy` |

## Secret-Looking Variable Name Warnings

| File | References |
| --- | --- |
| `.github/workflows/build.yml` | `CLOUDFLARE_API_TOKEN` |
| `android_admin/AIFRED_ADMIN_APP_USE_GUIDE.md` | `OPENAI_API_KEY` |
| `android_admin/app/src/main/java/com/aifred/admin/MainActivity.kt` | `PAYPAL` |
| `apps/admin-android/app/src/main/java/com/aifred/admin/MainActivity.kt` | `PAYPAL` |
| `apps/website/.dev.vars.example` | `OPENAI_API_KEY`, `PAYPAL`, `PAYPAL_CLIENT_SECRET` |
| `apps/website/app.js` | `PAYPAL` |
| `apps/website/assets/docs/aifred-installation.txt` | `OPENAI_API_KEY`, `PAYPAL` |
| `apps/website/assets/docs/aifred-release-notes.txt` | `PAYPAL` |
| `apps/website/config.js` | `PAYPAL` |
| `apps/website/functions/api/v1/[[path]].js` | `OPENAI_API_KEY`, `PAYPAL`, `PAYPAL_CLIENT_SECRET` |
| `apps/website/functions/ws/chat.js` | `OPENAI_API_KEY` |
| `apps/website/index.html` | `PAYPAL` |
| `apps/website/styles.css` | `PAYPAL` |
| `docs/AIFRED_WINDOWS_RELEASE_ARCHITECTURE.md` | `OPENAI_API_KEY` |
| `docs/RELEASE_NOTES.md` | `PAYPAL` |
| `docs/architecture/AIFRED_SYSTEM_MAP.md` | `PAYPAL` |
| `docs/architecture/BACKEND_SEPARATION_CONTRACT.md` | `PAYPAL` |
| `docs/operations/CLOUDFLARE_MANUAL_VERIFICATION_CHECKLIST.md` | `PAYPAL` |
| `docs/operations/PHASE10_PREVIEW_EVIDENCE_CAPTURE_TABLE.md` | `PAYPAL` |
| `docs/operations/PHASE1_CONSOLIDATION_LOG.md` | `PAYPAL` |
| `docs/operations/PHASE5_PREVIEW_MIGRATION_PLAN.md` | `PAYPAL` |
| `docs/operations/PHASE6_PREVIEW_APPROVAL_CHECKLIST.md` | `PAYPAL` |
| `docs/operations/PHASE7_PREVIEW_ABORT_CRITERIA.md` | `PAYPAL` |
| `docs/operations/PHASE7_PREVIEW_AUTHORIZATION_PACKAGE.md` | `PAYPAL` |
| `docs/operations/PHASE9_BLOCKER_CLOSURE_CHECKLIST.md` | `PAYPAL` |
| `docs/wiki/Admin-App-Guide.md` | `OPENAI_API_KEY` |
| `docs/wiki/Backend-Map.md` | `CLOUDFLARE_API_TOKEN`, `OPENAI_API_KEY`, `PAYPAL` |
| `docs/wiki/PayPal-Cloudflare-R2-Setup-Guide.md` | `CLOUDFLARE_API_TOKEN`, `PAYPAL`, `PAYPAL_CLIENT_SECRET` |
| `docs/wiki/Security-And-Distribution.md` | `PAYPAL` |
| `docs/wiki/Troubleshooting.md` | `CLOUDFLARE_API_TOKEN` |
| `infra/cloudflare/docs/Backend-Map.md` | `OPENAI_API_KEY`, `PAYPAL` |
| `infra/cloudflare/docs/PAYPAL_R2_PIPELINE.md` | `PAYPAL` |
| `tools/AifredEngine/Program.cs` | `OPENAI_API_KEY` |
| `tools/AifredWindowsInstaller/Program.cs` | `OPENAI_API_KEY` |
| `tools/macos/package-aifred-macos.sh` | `OPENAI_API_KEY` |
| `tools/package-aifred.ps1` | `OPENAI_API_KEY` |
| `tools/release/aifred_monorepo_validate.sh` | `CLOUDFLARE_API_TOKEN`, `OPENAI_API_KEY`, `PAYPAL`, `PAYPAL_CLIENT_SECRET` |
| `tools/release/aifred_preview_gate_report.py` | `CLOUDFLARE_API_TOKEN`, `OPENAI_API_KEY`, `PAYPAL`, `PAYPAL_CLIENT_SECRET` |
| `tools/release/aifred_website_dryrun_check.py` | `PAYPAL` |
| `tools/release/aifred_website_parity_manifest.py` | `PAYPAL` |
| `tools/release/aifred_workflow_audit.py` | `CLOUDFLARE_API_TOKEN`, `OPENAI_API_KEY`, `PAYPAL`, `PAYPAL_CLIENT_SECRET` |
| `tools/serve-website.mjs` | `OPENAI_API_KEY` |
| `tools/windows/setup-aifred-local-ai.ps1` | `OPENAI_API_KEY` |
| `website/.dev.vars.example` | `OPENAI_API_KEY`, `PAYPAL`, `PAYPAL_CLIENT_SECRET` |
| `website/app.js` | `PAYPAL` |
| `website/assets/docs/aifred-installation.txt` | `OPENAI_API_KEY`, `PAYPAL` |
| `website/assets/docs/aifred-release-notes.txt` | `PAYPAL` |
| `website/config.js` | `PAYPAL` |
| `website/functions/api/v1/[[path]].js` | `OPENAI_API_KEY`, `PAYPAL`, `PAYPAL_CLIENT_SECRET` |
| `website/functions/ws/chat.js` | `OPENAI_API_KEY` |
| `website/styles.css` | `PAYPAL` |

No secret values are printed in this report. Only variable names and path references are reported.

## Phase 4 Recommendations

- Keep existing live deployment workflow behavior unchanged until explicit path migration approval.
- Prove `apps/website` with non-deploying syntax and route checks before changing Cloudflare Pages commands.
- Verify Cloudflare Pages project bindings manually before any monorepo deployment migration.
- Keep GitHub release publishing on existing package paths until plugin and engine migrations are separately proven.
- Decide media asset strategy before merging the consolidation branch to `main`.

## Notes

- Excluded directories: `.git`, `node_modules`, `.wrangler`, `.gradle`, `build`, `dist`, `cache`.
- File contents and secret values are not printed.
- This report is generated by `tools/release/aifred_workflow_audit.py`.
