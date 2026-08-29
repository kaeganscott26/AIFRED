# AIFRED Website and API

This directory is the Cloudflare Pages output and Pages Functions source for project `aifred-site`. It serves the public product/catalog/analyzer site, controlled R2 downloads, normalized API and authenticated `/ops` console.

Authoritative configuration: `wrangler.toml`. Advanced-mode `_worker.js` normalizes `/health` and `/v1/*`, wraps API CORS/security behavior and delegates static assets. Functions and admin routes live under `functions/`.

```sh
npm ci --prefix apps
npm --prefix apps run website:check
npm --prefix apps run website:dev
npm --prefix apps run website:deploy
```

Never commit `.dev.vars`; use `.dev.vars.example` for names only. See [API Reference](../../docs/API_REFERENCE.md), [`/ops` Guide](../../docs/OPS_GUIDE.md), and [Cloudflare Production Guide](../../docs/CLOUDFLARE_PRODUCTION.md).
