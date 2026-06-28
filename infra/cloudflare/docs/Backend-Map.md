# Backend Map

The backend is a Cloudflare Pages Worker mounted from `website/_worker.js`.

Deployment path:

- Cloudflare Pages project `aifred-site`
- GitHub source repo `kaeganscott26/aifred-site`
- Production branch `main`
- Cloudflare Pages build root `website/`
- Automatic production deploys are handled by Cloudflare Pages, not GitHub Actions

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

## Analyzer Gate

The browser computes upload metrics locally, then submits metadata to `/api/v1/analysis/submit`.

The backend gate accepts a broad review lane so commercial references are not rejected over tiny audible differences. Peak is still protected: material above 0.05 dBFS is treated as clipping and is rejected.

The score combines loudness, peak, tone balance, crest factor, stereo width, low-end control, and harshness control. A mix can pass through either a broad weighted score or the essential pro lane:

- Integrated loudness between -24.0 and -3.0 LUFS.
- Peak at or below 0.0 dBFS.
- No severe tone, low-end, or harshness failure.

Accepted metadata is persisted only when the optional `AIFRED_REFERENCE_POOL` KV binding exists. Otherwise the API reports `accepted-no-binding` so the site remains usable while Cloudflare bindings are being configured.

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

Deploy automation:

- No GitHub Actions deploy token is required because production pushes are deployed directly by the Cloudflare Pages GitHub integration.
