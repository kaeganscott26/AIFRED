# Backend Map

The backend is a Cloudflare Pages Worker mounted from `website/_worker.js`.

## Worker Entrypoints

| Route | Handler |
| --- | --- |
| `/api/v1/*` | `website/functions/api/v1/[[path]].js` |
| `/api/*` | `website/functions/api/[[path]].js` |
| `/ws/chat` | `website/functions/ws/chat.js` |
| static assets | `env.ASSETS.fetch(request)` |

## Public API

| Method | Route | Purpose |
| --- | --- | --- |
| `GET` | `/api/v1/health` | Backend health |
| `GET` | `/api/v1/catalog/list` | Public beat catalog |
| `GET` | `/api/v1/soundpacks/list` | Soundpack placeholder route |
| `GET` | `/api/v1/content/get` | Website content payload |
| `POST` | `/api/v1/analysis/submit` | Free analyzer metadata gate |
| `POST` | `/api/v1/analyzer/submit` | Alias for analyzer gate |
| `GET` | `/api/v1/chat/settings` | Chat transport/settings |
| `GET` | `/api/v1/models/list` | Configured model catalog |
| `POST` | `/api/v1/chat/ask` | HTTP chat request |
| `POST` | `/api/v1/inquiries/submit` | Contact form capture |
| `GET` | `/ws/chat` | WebSocket chat upgrade |

## Admin API

| Method | Route | Purpose |
| --- | --- | --- |
| `POST` | `/api/v1/admin/login` | Admin session creation |
| `POST` | `/api/v1/command/run` | Registered command execution |
| `GET` | `/api/v1/registry/actions` | Command registry |
| `GET` | `/api/v1/admin/dashboard/state` | Dashboard summary |
| `GET` | `/api/v1/admin/catalog/list` | Catalog administration |
| `POST` | `/api/v1/admin/catalog/upload` | Upload catalog audio and append metadata |
| `POST` | `/api/v1/admin/catalog/remove` | Reserved catalog removal route |
| `POST` | `/api/v1/admin/reference/upload` | Upload licensed reference audio |
| `POST` | `/api/v1/admin/files/read` | Read repo/deployed file |
| `POST` | `/api/v1/admin/files/write` | Commit text file update |
| `GET` | `/api/v1/admin/files/list` | List GitHub-backed repo path |
| `POST` | `/api/v1/admin/files/delete` | Delete website path |
| `POST` | `/api/v1/admin/files/upload` | Upload binary website asset |
| `GET` | `/api/v1/admin/inquiries/list` | Inquiry list |
| `GET` | `/api/v1/admin/logs/list` | Log list |
| `GET` | `/api/v1/admin/sales/list` | Sales list |
| `POST` | `/api/v1/admin/sales/record` | Record PayPal sale metadata |
| `POST` | `/api/v1/admin/chat/settings/save` | Save chat settings payload |

## Required Secrets And Bindings

Cloudflare Pages:

- `OPENAI_API_KEY`
- `OPENAI_MODEL`
- `OLLAMA_BASE_URL`
- `OLLAMA_MODEL`
- `AIFRED_ADMIN_USERNAME`
- `AIFRED_ADMIN_PASSWORD_SHA256`
- `AIFRED_ADMIN_SESSION_SECRET`
- `GITHUB_TOKEN`
- `AIFRED_GITHUB_REPO`
- `AIFRED_GITHUB_BRANCH`
- `AIFRED_REFERENCE_POOL` KV binding, optional

GitHub Actions:

- `CLOUDFLARE_ACCOUNT_ID`
- `CLOUDFLARE_API_TOKEN`

If Cloudflare credentials reject in CI, the workflow emits a warning and build validation continues.

