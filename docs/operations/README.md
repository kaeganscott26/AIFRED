# Operations Documentation Authority

Current production authority as of 2026-08-25:

- `website-cloudflare-production-2026-08-25.md` — deployed Cloudflare/R2/Git/HTTP evidence and blockers.
- `CLOUDFLARE_MANUAL_VERIFICATION_CHECKLIST.md` — repeatable promotion checks.
- `SMOKE_TESTS.md` — local validation commands.
- `FINAL_MONOREPO_CONSOLIDATION_REPORT.md` — canonical source paths with the 2026-08-25 infrastructure correction.
- `MEDIA_ASSET_POLICY.md` and `RELEASE_WORKFLOW_SAFETY_CHECKLIST.md` — current safety boundaries.

Files beginning with `PHASE` and `AIFRED_CLOUD_ARCHITECTURE_IMPLEMENTATION_LOG.md` are dated migration evidence. Their observations about the old `website/` root, PayPal, `AIFRED_WEBSITE_ASSETS`, preview approval, or deployment blockers describe the state when captured and are not current instructions.

The current runtime contract is:

```text
Pages project: aifred-site
source: apps/website
distribution.mode: free
payment_pipeline: disabled
release/assets bucket: aifred-downloads
website activity/inquiries: AIFRED_SALES_LOG KV
production promotion: explicit repository npm command after push
```
