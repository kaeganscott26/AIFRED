# AIFRED Cloud Architecture and Implementation Log

> Historical implementation snapshot. Its PayPal, mailer, and separate website-assets assumptions were removed from the active runtime on 2026-08-25. See `website-cloudflare-production-2026-08-25.md`.

Status: controlled migration in progress. No Cloudflare deployment or resource deletion was performed by this change.

## Scope

This log covers AIFRED website, Pages/Workers, API, R2/KV, mail, PayPal, admin, operations tooling, and CI configuration. `plugin-aifred/`, `packages/local-engine/`, `tools/AifredEngine/`, and all FORGE/FORGE-OS material are out of scope and were not edited.

## Existing architecture (OBSERVED)

| Area | Current implementation | Classification |
| --- | --- | --- |
| Website | `apps/website` deployed as a Pages output directory and also wrapped by `apps/website/_worker.js` | ACTIVE / UNKNOWN deployment mode |
| API | `apps/website/functions/api/v1/[[path]].js`, one large handler for public, admin, storage, PayPal, and chat routes | ACTIVE / DUPLICATE responsibilities |
| Legacy API | `/api/*` adapter and `/api/v1/*` routes | ACTIVE / DEPRECATED |
| Chat | `chat/ask` plus `/ws/chat`; Ollama `/api/chat`, OpenAI `/v1/responses` | ACTIVE / NON-CANONICAL |
| KV | `AIFRED_REFERENCE_POOL`, `AIFRED_SALES_LOG` | REQUIRED by source; live state UNVERIFIED |
| R2 | `AIFRED_DOWNLOADS`, `AIFRED_WEBSITE_ASSETS`, `AIFRED_REFERENCE_BUCKET` | REQUIRED by source; live state UNVERIFIED |
| Mail | service binding to `aifred-mailer`; source at `infra/cloudflare/cloudflare/aifred-mailer.js` | UNKNOWN deployment |
| PayPal | create/capture order and IPN under `/api/v1/paypal/*` | ACTIVE in source; live callbacks UNVERIFIED |
| Admin | Android/desktop client using the website host and custom bearer sessions | ACTIVE / auth boundary needs hardening |
| CI | GitHub Actions builds and optionally Pages-deploys `apps/website` to project `north3rnlight3r` | ACTIVE / deployment authority conflict |

## Cloudflare resource classification

- `AIFRED_REFERENCE_POOL` KV: REQUIRED, UNKNOWN live contents and binding ownership.
- `AIFRED_SALES_LOG` KV: REQUIRED, UNKNOWN live contents and binding ownership.
- `aifred-downloads` R2: REQUIRED for release delivery, UNKNOWN live parity.
- `aifred-website-assets` R2: REQUIRED for catalog/static asset acceleration, UNKNOWN live parity.
- `aifred-reference-pool` R2: REQUIRED by reference persistence, UNKNOWN live parity.
- `aifred-mailer` Worker/service: REQUIRED by the source integration, UNKNOWN deployed revision and email binding.
- `AIFRED` Pages/Worker and `aifred-site` configuration names: DUPLICATE/UNKNOWN; do not remove either production resource without dashboard verification.
- Account ID, routes, custom domains, Access policies, D1, Queues, and Durable Objects: UNVERIFIED; no source declaration was found for D1, Queues, or Durable Objects.

## Routes before migration (OBSERVED)

| Route | Role | Status |
| --- | --- | --- |
| `/api/v1/chat/ask` | AIFRED-specific chat payload | DEPRECATED adapter |
| `/api/v1/models/list` | AIFRED-specific model listing | DEPRECATED adapter |
| `/api/v1/*` | public/admin/catalog/PayPal/storage routes | ACTIVE; non-AI routes retained |
| `/api/*` | legacy fallback | DEPRECATED |
| `/ws/chat` | WebSocket chat | ACTIVE but separate contract |
| `/health` | intended operational health endpoint | CHANGED to explicit top-level route |

## Routes after this migration phase (CHANGED)

| Route | Role |
| --- | --- |
| `/health` | public liveness check, outside `/v1` |
| `/v1/models` | OpenAI-compatible model list |
| `/v1/chat/completions` | canonical OpenAI-compatible chat endpoint, including SSE when `stream=true` |
| `/v1/embeddings` | explicit 501 response until an embedding provider is configured |
| `/v1/responses` | reserved; returns structured not-implemented response |
| `/api/v1/chat/ask` | deprecated adapter to canonical chat implementation |
| `/api/v1/models/list` | deprecated adapter to canonical model implementation |
| `/api/v1/*` | retained for non-AI compatibility |

## Target architecture (PROPOSED)

Pages serves the website. A dedicated API Worker owns `/v1`, health, admin API, PayPal callbacks, and storage routing. R2 remains authoritative for release artifacts and large public assets; KV remains for reference and sales/activity state until a data-model review justifies D1. Mail remains a dedicated service Worker. Admin and `/ops` call server-authorized admin endpoints; no browser-supplied `isAdmin` claim is trusted.

The AIFRED UI and admin clients use one base URL plus one OpenAI-compatible client operation. Local clients can point to `http://127.0.0.1:8787/v1`; cloud clients can point to `https://<AIFRED_API_HOST>/v1`.

## Bindings before/after

Before: the same five bindings are duplicated in two TOML files with different names, compatibility dates, Pages output paths, and a production mail service environment (`production` versus empty string).

After target: one authoritative deployment manifest should own the API/Pages deployment. Existing IDs and bucket names are retained pending live verification. The current duplicate `infra/cloudflare/wrangler.toml` is documentation/operations input only and must not be used for an independent production deploy.

## Authentication boundaries

- Public user: catalog, public content, health, and checkout configuration only.
- Admin user/site operator: bearer session issued by `/api/v1/admin/login`, checked server-side for every privileged route.
- Service-to-service: mailer token and Cloudflare bindings; never exposed to browser responses.
- Secrets: environment secret storage only. Production must explicitly configure admin username, password hash, and session secret; insecure source fallbacks are not valid production configuration.

## Payment and mail flow

Browser -> AIFRED API -> PayPal create/capture -> server-side capture verification -> entitlement/download token -> R2/GitHub release fallback -> dedicated mail Worker. PayPal credentials remain server-side. IPN is retained as a legacy callback and requires live endpoint verification.

## Tests and verification

The repository test harness added in this phase exercises canonical health/models/chat validation, malformed requests, streaming, legacy adapter routing, and admin authorization. Live Cloudflare bindings, custom domains, Access, production callbacks, R2 object parity, and deployed mail Worker revision remain UNVERIFIED until a non-destructive dashboard/API verification is authorized.

Executed locally: `node --check` for the changed website Worker/config files; `node --test tests/aifred-api.test.mjs` — 7 passed, 0 failed; `git diff --check` — passed. Android compile — NOT RUN because Java/JAVA_HOME is unavailable. Cloudflare Wrangler deploy — NOT RUN by design.

## Files changed (CHANGED)

- `apps/website/functions/api/v1/[[path]].js` — canonical API adapters, SSE, structured errors, ops status, and production auth fallback removal.
- `apps/website/_worker.js`, `apps/website/config.js`, `apps/website/ops.html` — top-level routing, centralized base URL, and authenticated maintenance console shell.
- `apps/website/wrangler.toml`, `infra/cloudflare/wrangler.toml` — normalized project identity/date and marked the infrastructure copy as a non-deploying historical mirror.
- `apps/website/.dev.vars.example` — non-secret configuration matrix entries.
- `apps/admin-android/app/src/main/java/com/aifred/admin/MainActivity.kt` and its README — canonical `/v1` client calls.
- `.github/workflows/build.yml` — API tests and explicit deployment failure behavior.
- `docs/architecture/AIFRED_API_CONTRACT.md`, this log, and `tests/aifred-api.test.mjs`.

The pre-existing `.forge/metadata.sqlite` worktree/index changes and temporary file were preserved and not part of this task.

## Manual Cloudflare actions still required

1. Confirm the canonical Pages project and API Worker names/custom domains.
2. Verify all KV IDs, R2 buckets, service binding target/environment, and production variables without exposing values.
3. Configure production secrets: `OPENAI_API_KEY` or another provider, `AIFRED_ADMIN_USERNAME`, `AIFRED_ADMIN_PASSWORD_SHA256`, `AIFRED_ADMIN_SESSION_SECRET`, PayPal credentials, GitHub token if repository persistence is required, and mailer token.
4. Verify DNS/routes, Cloudflare Access or equivalent operator policy, PayPal callback URLs, mail sender/domain, and R2 CORS/cache policy.
5. Deploy in order: mailer -> API/Pages -> smoke tests -> client configuration promotion. Keep the prior deployment available for rollback.

## Completion status

- Fully working in repository: canonical route shape, structured errors, local/cloud base URL contract, compatibility routing, configuration documentation, and automated unit-style Worker tests.
- Partially working: provider normalization (OpenAI Responses and Ollama are adapted behind the canonical route; tools and embeddings require provider-specific verification), admin runtime and `/ops` surface.
- Requires Cloudflare configuration: resource bindings, production secrets, domains/routes, Access, PayPal callbacks, mail deployment, and R2 parity.
- Unverified: live production behavior and rollback evidence.
