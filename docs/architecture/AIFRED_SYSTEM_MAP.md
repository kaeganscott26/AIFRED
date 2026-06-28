# AIFRED System Map

Logged for Phase 1 monorepo consolidation.

## Current Authorities

| System | Phase 1 canonical location | Source imported or referenced from |
| --- | --- | --- |
| Website/backend | `apps/website` | imported from `../AIFRED/website` |
| Cloudflare/web operations | `infra/cloudflare` | imported from `../AIFRED` config, docs, cloudflare, and ops files |
| Android admin app | `apps/admin-android` | imported from `../AIFRED` |
| Plugin runtime | `plugin-aifred` | remains in original path during Phase 1 |
| Local AI engine runtime | `tools/AifredEngine` | remains in original path during Phase 1 |
| Legacy plugin reference | `../aifred-plugin` | documented under `docs/archive/legacy-vstgui` |
| Future flagship reference | `../AIFRED_Official-` | documented under `docs/archive/flagship-contracts` |
| Release placeholder reference | `../aifred-downloads` | documented under `docs/archive/release-placeholder` |

## Backend Separation

AIFRED has two different backend systems. They must stay separate.

### Cloud/Web Backend

Canonical values:

```text
AIFRED_WEB_ORIGIN=https://www.north3rnlight3r.com
AIFRED_WEB_API_BASE=https://www.north3rnlight3r.com/api/v1
```

Role:

- Served by Cloudflare Pages/Worker.
- Used by `apps/website`.
- Used by `apps/admin-android`.
- Owns public website routes, contact/inquiry routes, catalog routes, admin routes, activity logs, PayPal/download flow, and web chat routes.
- May be used by the plugin only for metadata, release, or fallback routes if explicitly wired.

Important web routes:

```text
https://www.north3rnlight3r.com/api/v1
https://www.north3rnlight3r.com/ws/chat
```

### Local AI Engine

Canonical values:

```text
AIFRED_LOCAL_ENGINE_URL=http://127.0.0.1:8787
AIFRED_OLLAMA_URL=http://127.0.0.1:11434
AIFRED_LOCAL_MODEL=aifred:latest
```

Role:

- Served by `AifredEngine`.
- Used by the plugin for local AI/chat.
- Talks to Ollama at `http://127.0.0.1:11434`.
- Expects model `aifred:latest`.
- Does not serve the production website.
- Does not replace the Cloudflare backend.

Important local routes:

```text
GET  http://127.0.0.1:8787/health
POST http://127.0.0.1:8787/chat
GET  http://127.0.0.1:8787/v1/settings
POST http://127.0.0.1:8787/v1/settings
```

## Rule

Do not merge the Cloudflare web backend and the local AI engine. They are separate systems with separate deployment, availability, security, and runtime expectations.

Phase 1 imports the authoritative website and admin app into `apps/`, but it does not move the plugin or engine runtime paths.
