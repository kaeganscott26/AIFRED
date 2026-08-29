# AIFRED API Reference

Production origin: `https://www.north3rnlight3r.com`. Function responses use JSON and `Cache-Control: no-store` unless a download/media handler explicitly supplies cache/range headers. `/api/*` is a compatibility shim to the canonical `/api/v1/*` implementation.

## Canonical public contract

| Method | Path | Auth | Behavior |
| --- | --- | --- | --- |
| GET | `/health` | No | API health and contract version |
| GET | `/v1/models` | No | OpenAI-compatible model list; may be empty when no provider is configured |
| POST | `/v1/chat/completions` | Provider-dependent | OpenAI-compatible chat; supports JSON or SSE with `stream: true` |
| POST | `/api/v1/analysis/submit` | No | Browser analysis/reference gate submission |
| POST | `/api/v1/analyzer/submit` | No | Alias of analysis submission |
| GET | `/api/v1/catalog/list` | No | Catalog with controlled media URLs |
| GET/HEAD | `/api/v1/downloads/plugin?asset=setup\|zip\|macos` | No | Allowlisted R2 release download |
| GET/HEAD | `/api/v1/assets/audio/catalog/<file>` | No | Catalog stream; Range supported; `download=1` requests attachment |
| POST | `/api/v1/activity/record` | No | Allowlisted public activity event; cannot forge admin/server events |
| POST | `/api/v1/inquiries/submit` | No | Contact inquiry persistence in activity KV |
| GET | `/api/v1/content/get` | No | Website content/config payload |
| GET | `/api/v1/chat/settings` | No | Non-secret client chat settings |

`/v1/embeddings` and `/v1/responses` are reserved and return 501; they are not implemented capabilities.

## Admin session and operations

`POST /api/v1/admin/login` accepts JSON `username` and `password`. Success returns a signed bearer session; missing configuration returns 503, invalid/throttled authentication returns an error response. All routes below require `Authorization: Bearer <session>` and return 401 otherwise.

| Methods | Path | Purpose |
| --- | --- | --- |
| GET | `/api/v1/admin/dashboard/state` | Bounded combined operational snapshot |
| GET | `/api/v1/admin/ops/status` | Binding/API/deployment status without secret values |
| GET | `/api/v1/admin/logs/list` | Bounded activity and admin logs |
| GET | `/api/v1/admin/inquiries/list` | Inquiry records |
| GET | `/api/v1/admin/sales/list` | Historical sales records |
| GET | `/api/v1/admin/catalog/list` | Administrative catalog view |
| GET | `/api/v1/admin/reference/list` | Accepted reference/analysis records |
| GET | `/api/v1/admin/export/site` | `aifred.site-data` 1.0.0 JSON attachment |
| GET | `/api/v1/admin/export/tracks` | `aifred.track-analysis` 1.0.0 JSON attachment |
| GET/POST | `/api/v1/admin/api/config` | Read/save non-secret runtime provider routing in KV |
| GET/POST | `/api/v1/admin/api/test` | Test selected provider route |
| POST | `/api/v1/admin/chat/settings/save` | Return/save supported chat settings contract |
| POST | `/api/v1/admin/catalog/upload` | Upload catalog metadata/media through controlled storage |
| POST | `/api/v1/admin/reference/upload` | Upload licensed reference data |
| POST | `/api/v1/admin/files/read\|write\|list\|delete\|upload` | Approved repository/file administration |

## Command API

- `GET /api/v1/registry/actions` returns the structured backend allowlist.
- `POST /api/v1/command/run` accepts `{ "command_line": "<exact command>" }` and returns `{ok, exit_code, stdout, stderr}`.
- Both require an admin session. Exact commands are generated in [Administrator Command Reference](ADMIN_COMMAND_REFERENCE.md).

## Local AifredEngine API

Origin: `http://127.0.0.1:8787`; loopback only, no Cloudflare involvement.

| Method | Path | Purpose |
| --- | --- | --- |
| GET | `/health` | Engine 1.0.0, provider/model readiness |
| POST | `/analyze` | Threshold-based interpretation of submitted metrics |
| POST | `/chat` | Request-driven chat with supplied analysis context |
| GET/POST | `/v1/settings` | Read/save local user settings |
| POST | `/v1/restart` | Acknowledge restart and exit for supervisor restart |

Unknown routes return 404 JSON. Engine responses are `no-store`; logs go to the installed engine `logs/engine.log` location.
