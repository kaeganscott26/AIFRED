# Cloudflare Production Guide

## Current topology

- Pages project: `aifred-site`
- Production branch: `main`
- Domains: `north3rnlight3r.com`, `www.north3rnlight3r.com`
- Pages hostname: `aifred-site.pages.dev` (preview/Pages access policy may apply)
- Authoritative config: `apps/website/wrangler.toml`
- Operational mirror: `infra/cloudflare/wrangler.toml` (do not deploy independently)
- Root convenience config: `wrangler.jsonc`
- Functions: `apps/website/functions/` plus advanced-mode `_worker.js`

The checked-in architecture assigns the API to Pages Functions. Confirm live Worker routes, Pages bindings and access policies before a deployment; historical dashboard observations are not a current account audit.

## Storage and bindings

| Binding | Resource | Use |
| --- | --- | --- |
| `AIFRED_SALES_LOG` | KV namespace whose historical physical name is AIFRED_SALES_LOG | Activity, inquiries, historical sales fallback, API runtime config, login throttling |
| `AIFRED_REFERENCE_POOL` | KV | Accepted reference/analysis metadata |
| `AIFRED_DOWNLOADS` | R2 bucket `aifred-downloads` | Versioned release packages and catalog media |
| `AIFRED_REFERENCE_BUCKET` | R2 bucket `aifred-reference-pool` | Licensed reference objects |

## Analytics, caching and security

Cloudflare Web Analytics is enabled at the zone and supplies privacy-preserving visits, paths, referrers, region/device/browser trends. Application events are sanitized and stored in activity KV with 90-day TTL for new events. Query strings are redacted from Worker observability; traces are disabled.

Versioned media uses long browser/edge caching and Range support. HTML uses revalidation. `/ops`, APIs and exports use `no-store`; chat/admin responses are never cached. CSP, HSTS, MIME protection, referrer policy, permissions policy and frame restrictions are active. Admin login uses hashed-password verification, signed sessions and KV-backed throttling. CORS is restricted by the Function wrapper for API responses.

## Configuration names

Required for admin login: `AIFRED_ADMIN_USERNAME`, `AIFRED_ADMIN_PASSWORD_SHA256`, `AIFRED_ADMIN_SESSION_SECRET`.

Provider options: `AIFRED_CHAT_PROVIDER`, `OPENAI_API_KEY`, `OPENAI_MODEL`, `OLLAMA_BASE_URL`, `OLLAMA_MODEL`, `OLLAMA_API_TOKEN`, `OLLAMA_ACCESS_CLIENT_ID`, `OLLAMA_ACCESS_CLIENT_SECRET`.

Optional metadata/integration: `AIFRED_API_BASE_URL`, `AIFRED_ANALYTICS_API_TOKEN`, `AIFRED_CHAT_SESSIONS`, `AIFRED_CONTACT_EMAIL`, `AIFRED_EMAIL_FROM`, `AIFRED_GITHUB_REPO`, `AIFRED_GITHUB_BRANCH`, `AIFRED_PLUGIN_REPO`, `AIFRED_PLUGIN_RELEASE_TAG`, `AIFRED_RELEASE_VERSION`, `GITHUB_TOKEN`.

Never commit values. Local names are illustrated in `apps/website/.dev.vars.example`.

## Validate and deploy

```sh
npm ci --prefix apps
npm --prefix apps run website:check
npx --prefix apps wrangler pages functions build apps/website --outdir out/website/build/functions
npm --prefix apps run website:deploy
```

GitHub Actions validates pushes/tags. Production deployment is explicit through the npm command or the manually dispatched workflow when Cloudflare repository secrets exist; native duplicate Git deployment is not the authority.


## Promotion checks

Before deployment, verify account/project, main branch, apps/website source, /api and /api/v1 compatibility, /v1 and /ws/chat handling, binding names, admin authorization and public/preview hostname policy. Run repository/API checks, capture the previous deployment identifier, deploy only within the authorized environment, verify responses and retain a rollback target. Validation workflows must not publish packages or deploy. Never record secret values. Historical migration checklists do not grant current deployment approval.
