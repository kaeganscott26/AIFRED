# Media Asset Policy

`apps/website` currently contains public website media assets copied from `aifred-site`.

This was intentional in Phase 1 for fidelity. The import preserved the website as it was serving the product, including public catalog audio, brand images, data files, and documentation assets.

## Phase 2 Rules

- Do not delete media assets during Phase 2.
- Do not rewrite history during Phase 2.
- Do not convert assets to another storage system during Phase 2.
- Do not change Cloudflare or release behavior during Phase 2.

## Decision Needed Before Merge To Main

Before merging the consolidation branch to `main`, decide whether large audio/catalog assets should:

- remain in Git,
- move to Git LFS,
- stay in Cloudflare/R2,
- or be referenced externally from the website.

## Safe Inspection Commands

```sh
git rev-list --objects --all
find apps/website -type f -size +10M -print
find . -type f -size +50M -print
```

Phase 3 should decide the asset strategy before merge to `main`.
