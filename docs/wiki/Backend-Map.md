# Backend Map

The canonical website/backend source lives under `apps/website/`.

## Deployment Authority

- GitHub repository: `kaeganscott26/AIFRED`
- Production branch: `main`
- Website source: `apps/website/`
- Cloudflare Pages project: `aifred-site`
- Production domains:
  - `https://www.north3rnlight3r.com`
  - `https://north3rnlight3r.com`

Cloudflare configuration roles:

| File | Role |
| --- | --- |
| `apps/website/wrangler.toml` | Primary website Pages config and resource bindings |
| `infra/cloudflare/wrangler.toml` | Operations/support mirror |
| `wrangler.jsonc` | Root convenience config pointed at `apps/website` |

The main GitHub Actions workflow validates pushes and can deploy `apps/website/` through a manual dispatch. Local and CI deployment use the same npm script. Cloudflare native Git deployment is disabled to avoid a second competing deployment path.

## Worker Entrypoints

`apps/website/_worker.js` routes:

| Route | Handler |
| --- | --- |
| `/api/v1/*` | `apps/website/functions/api/v1/[[path]].js` |
| `/api/*` | `apps/website/functions/api/[[path]].js` |
| `/ws/chat` | `apps/website/functions/ws/chat.js` |
| static assets | `env.ASSETS.fetch(request)` |

The `/api/*` handler is a small legacy compatibility shim. It is intentionally preserved until production usage is explicitly verified.

## Public API

| Method | Route | Purpose |
| --- | --- | --- |
| `GET` | `/api/v1/health` | Backend health |
| `GET` | `/api/v1/catalog/list` | Public beat catalog |
| `GET` | `/api/v1/soundpacks/list` | Soundpack catalog route |
| `GET` | `/api/v1/content/get` | Website content payload |
| `POST` | `/api/v1/analysis/submit` | Browser analyzer metadata gate |
| `POST` | `/api/v1/analyzer/submit` | Alias for analyzer gate |
| `POST` | `/api/v1/activity/record` | Public activity logging |
| `GET` | `/api/v1/chat/settings` | Chat transport/settings |
| `GET` | `/api/v1/models/list` | Configured model catalog |
| `POST` | `/api/v1/chat/ask` | HTTP chat request |
| `POST` | `/api/v1/inquiries/submit` | Contact form capture |
| `GET`, `HEAD` | `/api/v1/downloads/plugin?asset=setup` | Free Windows installer download |
| `GET`, `HEAD` | `/api/v1/downloads/plugin?asset=zip` | Free Windows ZIP download |
| `GET`, `HEAD` | `/api/v1/downloads/plugin?asset=macos` | Free macOS VST3 ZIP download |
| `GET` | `/api/v1/assets/audio/catalog/<file>` | Catalog audio stream |
| `GET`, `HEAD` | `/api/v1/assets/audio/catalog/<file>?download=1` | Free catalog MP3 download |
| `GET` | `/ws/chat` | WebSocket chat upgrade |

## Admin API

| Method | Route | Purpose |
| --- | --- | --- |
| `POST` | `/api/v1/admin/login` | Admin session creation |
| `POST` | `/api/v1/command/run` | Registered command execution |
| `GET` | `/api/v1/registry/actions` | Command registry |
| `GET` | `/api/v1/admin/dashboard/state` | Dashboard summary |
| `GET` | `/api/v1/admin/catalog/list` | Catalog administration |
| `POST` | `/api/v1/admin/catalog/upload` | Upload catalog audio and update metadata |
| `POST` | `/api/v1/admin/catalog/remove` | Reserved catalog removal route |
| `POST` | `/api/v1/admin/reference/upload` | Upload licensed reference audio |
| `POST` | `/api/v1/admin/files/read` | Read approved repo/deployed file |
| `POST` | `/api/v1/admin/files/write` | Commit approved text-file update |
| `GET` | `/api/v1/admin/files/list` | List approved repository path |
| `POST` | `/api/v1/admin/files/delete` | Delete approved website path |
| `POST` | `/api/v1/admin/files/upload` | Upload website asset |
| `GET` | `/api/v1/admin/inquiries/list` | Inquiry list |
| `GET` | `/api/v1/admin/logs/list` | Activity/log list |
| `GET` | `/api/v1/admin/sales/list` | Sales list |
| `GET`, `POST` | `/api/v1/admin/api/config` | Read or update secret-safe runtime model routing in KV |
| `GET`, `POST` | `/api/v1/admin/api/test` | Test Website, Ollama, or OpenAI connectivity without exposing secrets |
| `GET` | `/api/v1/admin/ops/status` | Operations/binding/distribution status |
| `POST` | `/api/v1/admin/chat/settings/save` | Save chat settings payload |

## Analyzer Gate

The browser computes upload metrics locally and submits metadata to `/api/v1/analysis/submit`.

The current `proGate` logic in `apps/website/functions/api/v1/[[path]].js` uses weighted loudness, peak, tone, crest, width, low-end-control, and harshness-control checks.

Current hard/essential behavior includes:

- Invalid loudness/peak values can cause rejection.
- Peak above `0.05 dBFS` is treated as clipping.
- The broad `essentialPass` loudness lane is `-24.0` through `-3.0 LUFS`.
- `essentialPass` also requires peak at or below `0.0 dBFS` and no severe tone/low-end/harshness failure.
- Material above `-7.0 LUFS` is classified through the technically-hot branch rather than being automatically rejected.
- Final classifications include Strong Reference, Usable Reference, Style-Specific Reference, Technically Hot Reference, Poor Reference, and Reject.

Accepted metadata is persisted only when the optional `AIFRED_REFERENCE_POOL` KV binding exists. Otherwise the API can report `accepted-no-binding` so the site remains usable while persistence is being configured.

## Model Routing

Website/admin backend model settings:

- Local model: `aifred:latest`
- OpenAI default: `gpt-5.6-luna`
- OpenAI API route: `https://api.openai.com/v1/responses`

The authenticated `/ops` console and Android admin app can update the non-secret provider, endpoint, and model selection stored under `AIFRED_SALES_LOG` KV. Provider keys, Ollama gateway tokens, and Cloudflare Access service-token credentials remain Pages secrets. Runtime chat uses the KV selection without requiring a source commit or redeployment.

The website backend and the local VST engine are separate systems. The VST normally talks to the local engine at `127.0.0.1:8787`; the Cloudflare backend serves the public site and admin app.

## Storage And Bindings

Current Cloudflare config includes:

| Binding | Purpose |
| --- | --- |
| `AIFRED_DOWNLOADS` | Versioned plugin releases plus catalog/website assets |
| `AIFRED_REFERENCE_BUCKET` | Accepted reference material |
| `AIFRED_REFERENCE_POOL` | Reference metadata persistence |
| `AIFRED_SALES_LOG` | Current activity/inquiry logging plus historical sales compatibility |

Catalog audio uses R2 first and local files as a development fallback.

## Runtime Configuration Names

Current repository examples and backend code reference configuration names including:

- `OPENAI_API_KEY`
- `OPENAI_MODEL`
- `OLLAMA_BASE_URL`
- `OLLAMA_MODEL`
- `OLLAMA_API_TOKEN` when the HTTPS Ollama gateway uses bearer authentication
- `OLLAMA_ACCESS_CLIENT_ID` and `OLLAMA_ACCESS_CLIENT_SECRET` for a Cloudflare Access service-token policy
- `AIFRED_ADMIN_USERNAME`
- `AIFRED_ADMIN_PASSWORD_SHA256`
- `AIFRED_ADMIN_SESSION_SECRET`
- `AIFRED_GITHUB_REPO`
- `AIFRED_GITHUB_BRANCH`
- `AIFRED_PLUGIN_REPO`
- `AIFRED_PLUGIN_RELEASE_TAG`
- `AIFRED_RELEASE_VERSION`
- GitHub/Cloudflare deployment configuration used by server-side code or CI

Private values belong in deployment configuration, not committed source files.

Local development may use `http://127.0.0.1:11434`. Production Pages must use a publicly reachable HTTPS URL, normally a named Cloudflare Tunnel protected by Access. Never point Pages at loopback/private addresses or expose Ollama through an unauthenticated temporary tunnel.
