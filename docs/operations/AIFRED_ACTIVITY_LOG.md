# AIFRED activity ledger

## Namespace identity

The production Cloudflare KV binding remains physically named `AIFRED_SALES_LOG` and retains namespace ID `2c66da7795b54135a4d67e514b97491f`. It is treated by the application as the AIFRED activity ledger, conceptually `AIFRED_ACTIVITY_LOG`. Historical `activity:<uuid>`, `inquiry:<uuid>`, configuration, and any sales-compatible records are not renamed or migrated.

The same binding continues to hold the non-secret `admin:config:api-runtime` record for compatibility. Provider credentials, authorization headers, API keys, session tokens, passwords, and Cloudflare Access secrets must never be written to activity events.

## Version 1 event envelope

New activity records use a compact envelope. Empty or irrelevant optional fields are omitted.

```json
{
  "event_id": "uuid",
  "event_type": "download.completed",
  "timestamp": "2026-08-27T12:00:00.000Z",
  "session_id": "anonymous-session-id",
  "request_id": "request-correlation-id",
  "actor": {
    "type": "anonymous",
    "id": "anonymous-session-id"
  },
  "source": {
    "surface": "website.downloads",
    "route": "/api/v1/downloads/plugin",
    "referrer": "https://www.north3rnlight3r.com/",
    "user_agent": "...",
    "country": "US",
    "colo": "ORD"
  },
  "subject": {
    "type": "download",
    "id": "setup",
    "name": "AIFRED-VST3-Setup.exe"
  },
  "operation": {
    "action": "download",
    "status": "success",
    "result": "response_resolved"
  },
  "metadata": {
    "artifact": "setup",
    "object_key": "releases/v0.3.6-installer-ai-alias/AIFRED-VST3-Setup.exe",
    "http_status": 200
  }
}
```

`apps/website/lib/activity-log.js` creates IDs and timestamps, supplies request context, removes secret-named metadata fields, caps untrusted field sizes, serializes the envelope, and catches KV write failures so logging does not break successful product behavior.

## Key and KV metadata strategy

Future keys use:

```text
activity:v1:<ISO-8601 UTC timestamp>:<event_type>:<event_id>
```

This makes version 1 events chronologically inspectable and collision resistant without rewriting historical keys. Each KV write also supplies compact Cloudflare key metadata containing `schema`, `type`, `ts`, and `request_id` when available.

## Event types emitted

Public website and Worker events:

- `website.page.view`
- `website.resource.clicked`
- `catalog.loaded`
- `catalog.playback.started`
- `catalog.playback.failed`
- `analysis.submitted`
- `analysis.failed`
- `download.clicked`
- `download.requested`
- `download.completed`
- `download.failed`
- `inquiry.submitted`
- `inquiry.fallback.opened`

Resolved admin events:

- `admin.login.succeeded`
- `admin.api_configuration.updated`
- `admin.file.updated`
- `admin.file.deleted`
- `admin.file.uploaded`
- `admin.reference.updated`
- `admin.catalog.updated`
- `admin.operation.completed`
- `admin.operation.failed`
- `admin.catalog.reviewed`
- `admin.models.reviewed`
- `admin.reference.reviewed`
- `admin.deploy.reviewed`
- `admin.sales.reviewed`
- `admin.inquiry.reviewed`

Admin allowlist actions are resolved before logging. Raw command bodies are not copied into the ledger. Failure records contain a safe operation result and error type, not credentials, request bodies, or authorization material.

## Download correlation semantics

The browser generates a random `request_id` for each download activation and reuses its stable anonymous session ID. Anchor downloads carry these as `rid` and `sid` query parameters because browser navigation cannot attach custom headers. The Worker uses that correlation for the lifecycle:

1. `download.clicked`: the browser activated the artifact link. This does not prove a download.
2. `download.requested`: the Worker received a GET for the artifact.
3. `download.completed`: the Worker successfully resolved the R2 or fallback object and returned a successful response, including range responses where applicable.
4. `download.failed`: artifact resolution threw or returned a non-success status.

`download.completed` proves successful server-side response resolution. It cannot prove that a browser consumed every byte after the streaming response left Cloudflare. FORGE must not reinterpret historical `plugin.download.requested` records as completed downloads.

The lifecycle covers Windows installer, Windows portable ZIP, macOS VST3 ZIP, and catalog MP3 attachment downloads. R2 object identity and response source are recorded when safe.

## Inquiry behavior

The full inquiry remains under its existing `inquiry:<id>` record for the admin workflow. The activity ledger receives a separate `inquiry.submitted` event with only the inquiry ID, anonymous correlation, storage result, and notification result. The activity event does not duplicate the sender's email address or message.

## FORGE-consumable historical exports

- `smart-env/Aifred_Site_Activity.jsonl` is the canonical key-preserving snapshot. Each `kv_entry` line includes the original key, Cloudflare expiration/metadata when exposed, HTTP status, byte length, exact base64 bytes, and exact UTF-8 text when round-trip safe.
- `smart-env/Aifred_Site_Activity.md` retains the earlier human-oriented values and now contains every value from the same snapshot.

The final 2026-08-27 snapshot contains 265 KV entries, including two intentional post-deploy correlation smoke-test events. Cloudflare did not expose created/modified timestamps for the historical keys; timestamps embedded in stored values remain verbatim. Historical flat events prove only what their original event names and fields state.

## Export again

Authenticate without placing a token in the repository:

```powershell
npx --yes wrangler@4.125.0 login
```

Then run the read-only cursor exporter from the repository root:

```powershell
pwsh -File .\tools\cloudflare\export-aifred-activity.ps1
```

It writes a timestamped JSONL file by default, follows every Cloudflare cursor, fetches every value, preserves exact bytes as base64, records non-200 reads, and prints only counts and the output path. To replace a chosen snapshot intentionally, pass `-OutputPath <path> -Force`.
