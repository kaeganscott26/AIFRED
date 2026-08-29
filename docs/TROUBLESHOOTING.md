# AIFRED Troubleshooting

## Local engine unavailable

```sh
curl -i http://127.0.0.1:8787/health
```

Confirm AifredEngine is running and no other process owns port 8787. Installed logs are under the engine installation's `logs/engine.log`. The plugin remains usable for analysis without chat.

## Ollama/model unavailable

```sh
curl http://127.0.0.1:11434/api/tags
ollama list
```

Health distinguishes unreachable Ollama from missing `aifred:latest`. Use the supplied platform setup tool if the alias is missing. Do not point a phone's `127.0.0.1` at a workstation unless ADB reverse or an appropriate private address is configured.

## Production API or deployment mismatch

```sh
curl -i https://www.north3rnlight3r.com/health
curl -i https://north3rnlight3r.com/ops
```

Both domains should reach Pages project `aifred-site`. Confirm the latest production branch is `main` and that Pages Functions compiled. A model list may validly be empty when no provider is configured.

## Admin authentication

A 401 means no valid signed session. A 503 login response means required admin environment names are incomplete. Repeated failures may activate the KV throttle. Desktop offline archive access does not grant production access.

## Android connectivity

Confirm `AIFRED_BASE_URL` has no duplicated `/api` or `/v1`. Production defaults to the custom domain. Public cleartext is rejected; HTTP is allowed only for private/loopback development. If Gradle cannot find the SDK, set untracked `local.properties` `sdk.dir`.

## Export failure

Exports require an online admin session and return `no-store` JSON. Check `/api/v1/admin/ops/status`, the relevant KV/R2 binding, available local storage on Android, and network timeout. No secret should appear in output.

## Track analysis unavailable

Check `/api/v1/admin/reference/list` and `/api/v1/admin/catalog/list`. Accepted reference metadata needs `AIFRED_REFERENCE_POOL`; object mirroring needs `AIFRED_REFERENCE_BUCKET`. Empty data is not a fabricated error.

## Archive/rotation failure

```sh
node tools/aifred-archive.mjs status
node tools/aifred-archive.mjs verify
```

Ensure the repository and archive root are writable. On failure, the source completed runs remain. A checksum error means the permanent bundle changed or is corrupt; preserve it and recover from the source/export rather than forcing manifest edits. Use `rebuild-index` only after bundles verify.

If threshold rotation does not run, confirm at least two completed history directories exist: the newest is always protected. Check `AIFRED_FORGE_ACTIVE_LOG_LIMIT_MB` and active byte count.

## FORGE bridge unavailable

The export process requires `AIFRED_ADMIN_SESSION_TOKEN` in its environment. The token is not stored. Verify `integrations/forge/manifest.json`; recreate a symlink with the provided script rather than hard-coding a user path. AIFRED contains no ToolRouter implementation.

## Timestamp mismatch

API/export/archive timestamps are UTC ISO-8601. Android, `/ops`, and desktop convert once at presentation. If a value is shifted, inspect the raw `Z` timestamp and do not add a second manual timezone offset.
