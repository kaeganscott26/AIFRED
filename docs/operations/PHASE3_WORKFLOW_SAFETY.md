# Phase 3 Workflow Safety

Phase 3 makes workflow and deployment risk visible before any live path migration.

## Safety Rules

- Existing live deployment behavior is intentionally unchanged.
- The new `AIFRED Monorepo Validation` workflow is manual-only through `workflow_dispatch`.
- Phase 3 does not deploy anything.
- Phase 3 does not publish releases.
- Phase 3 does not move `plugin-aifred` or `tools/AifredEngine`.
- Phase 3 does not switch Cloudflare from old paths to new paths.
- Phase 3 does not disable existing workflows.
- Any workflow path changes must wait for explicit approval.

## Cloudflare Deployment Boundary

Existing `aifred-site` deployment behavior should remain the production authority until monorepo deployment is proven.

Before any deployment migration:

- Verify the Cloudflare Pages project binding manually.
- Confirm which directory Cloudflare currently deploys.
- Confirm Worker route handling for `/api`, `/api/v1`, and `/ws/chat`.
- Confirm the domain binding for `www.north3rnlight3r.com`.
- Run non-deploying syntax and route checks against `apps/website`.

Phase 3 adds audit and validation infrastructure only. It does not change the live Cloudflare command, GitHub Actions deployment behavior, or release publishing behavior.
