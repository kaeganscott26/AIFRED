# Cloudflare Backend Map

The canonical current backend map is:

- [`docs/wiki/Backend-Map.md`](../../../docs/wiki/Backend-Map.md)

This file is retained as an operations entry point so old links under `infra/cloudflare/docs/` do not break, but it must not become a second independent source of backend truth.

Current authority summary:

- Repository: `kaeganscott26/AIFRED`
- Website/backend source: `apps/website/`
- Worker router: `apps/website/_worker.js`
- Main API: `apps/website/functions/api/v1/[[path]].js`
- Legacy compatibility shim: `apps/website/functions/api/[[path]].js`
- WebSocket chat: `apps/website/functions/ws/chat.js`
- Production project name used by deploy tooling: `aifred-site`
- Production branch: `main`

Cloudflare config roles:

- `apps/website/wrangler.toml` — primary app config and bindings.
- `infra/cloudflare/wrangler.toml` — operations/support mirror.
- `wrangler.jsonc` — root convenience config pointed at `apps/website`.

Do not restore the deleted top-level `website/` source tree or point backend file operations at the retired standalone website repository.
