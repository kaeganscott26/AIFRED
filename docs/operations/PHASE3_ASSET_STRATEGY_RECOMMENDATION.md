# Phase 3 Asset Strategy Recommendation

Phase 2 inventory shows large media in both `apps/website` and `website`.

This duplication is expected after Phase 1 because the imported `apps/website` copy preserved production website fidelity from `aifred-site`, while the old in-repo `website` folder remains untouched as evidence and fallback.

## Current Finding

From `docs/operations/PHASE2_REPO_INVENTORY.md`:

- `apps/website`: 263.6 MB.
- `website`: 259.2 MB.
- Media summary: 114 `.mp3` files at 576.4 MB and 18 `.wav` files at 928.7 MB.
- The largest repository files are mostly `.wav` files under `North3rnlight3r_Beatz`.

## Options

### A. Keep All Media In Git

Pros:

- Simple checkout and deploy model.
- No extra storage tool required.
- Website fidelity is preserved exactly.

Cons:

- Repository stays large.
- Clone and CI checkout times stay high.
- Binary media changes make history grow quickly.

### B. Move Large Media To Git LFS

Pros:

- Keeps Git object history lighter for future large asset changes.
- Familiar Git-adjacent workflow.

Cons:

- Requires LFS setup for every contributor and CI environment.
- Does not solve public delivery strategy by itself.
- Should not be introduced mid-consolidation without release and deployment testing.

### C. Keep Large Media In Cloudflare R2 Or Release Storage

Pros:

- Better long-term fit for public catalog audio.
- Keeps Git focused on source, metadata, manifests, and small public assets.
- Can scale without bloating repository history.

Cons:

- Requires manifest discipline.
- Requires Cloudflare/R2 binding and access verification.
- Needs a migration plan to avoid broken public download and playback URLs.

### D. Keep Temporary Duplication Until Website Path Migration Is Proven

Pros:

- Lowest risk during Phase 3.
- Preserves old and new website paths until deployment behavior is proven.
- Avoids deleting or rewriting media while workflow authority is still being audited.

Cons:

- Repository remains temporarily duplicated.
- Main merge should wait until asset strategy is accepted.

## Recommendation

Do not rewrite history now. Do not delete assets now. Do not convert files to Git LFS during Phase 3.

Short term: keep the duplication until workflow and deploy path migration is proven. The old `website/` duplicate should be removed only in a later approved phase after `apps/website` deployment is proven.

Before merging to `main`, keep the Phase 3 branch unmerged until asset strategy is chosen.

Long term: public catalog audio should live in Cloudflare R2 or release storage. Git should keep metadata, manifests, route code, small images, and small public assets. Large audio should be referenced by durable URLs or generated manifests rather than duplicated across runtime folders.
