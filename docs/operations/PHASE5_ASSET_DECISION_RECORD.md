# Phase 5 Asset Decision Record

> Historical migration record captured before the 2026-08-25 production promotion. Preserve as evidence; current authority is `website-cloudflare-production-2026-08-25.md`.

## Current Issue

Large media exists in both `website/` and `apps/website`.

This duplication was intentional during safe migration. Phase 1 preserved website fidelity, Phase 4 confirmed `apps/website` has the required deployment shape, and the old `website/` folder remains the production fallback.

This cannot remain unresolved before merge to `main`.

## Inputs

- `docs/operations/PHASE2_REPO_INVENTORY.md`
- `docs/operations/PHASE3_ASSET_STRATEGY_RECOMMENDATION.md`
- `docs/operations/PHASE4_WEBSITE_DRYRUN_REPORT.md`

Phase 2 inventory shows large audio assets and duplicate website media. Phase 4 website dry-run shows `website/` and `apps/website` are close in deployment shape but still have file-count and reference-count differences.

## Decision Options

### A. Keep All Media In Git Temporarily

This keeps migration simple and preserves exact fidelity, but the repository remains large.

### B. Convert Large Media To Git LFS Later

This can reduce future Git object growth, but it changes contributor and CI requirements and should not happen during Phase 5.

### C. Move Public Catalog Audio To Cloudflare R2 Or Release Storage

This is the preferred long-term shape. Git should keep metadata, manifests, code, small images, and small docs while public catalog audio lives in R2 or release storage.

### D. Keep Duplication Until `apps/website` Preview Succeeds

This is the safest short-term option. It preserves rollback while preview migration is still unproven.

## Recommendation

- Short term: keep duplication until `apps/website` preview succeeds.
- Medium term: remove old `website/` only after `apps/website` is proven.
- Long term: move public catalog audio to Cloudflare R2 or release storage.
- Git should keep metadata, manifests, code, small images, and small docs.
- Do not rewrite history in Phase 5.
- Do not convert to Git LFS in Phase 5.
- Do not delete media in Phase 5.

## Merge Blocker

Do not merge consolidation to `main` until asset strategy is explicitly accepted.

Do not merge if duplicate website media would make `main` too heavy without approval.
