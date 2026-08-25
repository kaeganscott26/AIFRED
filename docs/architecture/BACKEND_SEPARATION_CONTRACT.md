# Backend Separation Contract

AIFRED has two different backend systems. They must stay separate.

## Cloud/Web Backend

Canonical values:

```text
Origin: https://www.north3rnlight3r.com
API base: https://www.north3rnlight3r.com/api/v1
Worker/WebSocket route: /ws/chat
```

Used by:

- `apps/website`
- `apps/admin-android`

Responsibilities:

- Public website.
- Beat catalog.
- Inquiry/contact backend.
- Admin APIs.
- Free plugin/catalog download flow through the private R2 binding.
- Activity logs.
- Possible web chat.

The Cloud/web backend is deployed through Cloudflare Pages/Worker infrastructure. It must not become the local plugin AI engine.

## Local AI Engine

Canonical values:

```text
Gateway: http://127.0.0.1:8787
Ollama: http://127.0.0.1:11434
Model: aifred:latest
```

Used by:

- `plugin-aifred`

Responsibilities:

- Local plugin AI/chat gateway.
- Local settings route.
- Local health route.
- Ollama-backed model calls.

The local AI engine should not depend on public website availability for local analysis or chat. It should not be merged into the Cloudflare worker.

## Explicit Rule

If a future plugin feature submits metadata to the web backend, it must be documented as a web API feature and must not replace the local engine chat path.
