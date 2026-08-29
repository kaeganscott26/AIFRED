# AIFRED Development State

Updated: 2026-08-29. Current source/configuration and [docs/README.md](docs/README.md) are authoritative.

## Versions and authorities

- VST3 0.3.6: `plugin-aifred/`
- AifredEngine 1.0.0: `tools/AifredEngine/`
- Website/API/`/ops`: `apps/website/`, Pages project `aifred-site`
- Android Admin 2.3.0, code 243: `apps/admin-android/`
- Windows Desktop: `apps/admin-android/tools/windows-admin/`
- macOS Desktop 1.0.0 source: `apps/admin-desktop/macos/`
- FORGE manifest 1.1.0: `integrations/forge/`
- Export/archive schemas 1.0.0: `integrations/forge/schemas/`, `tools/lib/aifred-archive.mjs`

## Runtime boundaries

Plugin traffic stays local through AifredEngine at `127.0.0.1:8787`, normally to Ollama at `127.0.0.1:11434` and `aifred:latest`. Website/admin traffic uses `https://www.north3rnlight3r.com`; the normalized public AI contract is `/health`, `/v1/models`, `/v1/chat/completions`.

Cloudflare bindings are `AIFRED_DOWNLOADS`, `AIFRED_REFERENCE_BUCKET`, `AIFRED_REFERENCE_POOL`, and historical-name `AIFRED_SALES_LOG` for the activity ledger. Public distribution is free; PayPal is disabled.

Only Android has the user-entered command interface: 10 generated backend commands and 15 generated local actions. `/ops` and desktop clients use controls. Desktop clients additionally own local archives.

FORGE keeps latest/current sanitized exports and rotates older completed local mirror runs at `AIFRED_FORGE_ACTIVE_LOG_LIMIT_MB` (25 MB default) only after gzip/checksum/manifest verification. Production data is never pruned.

## Validation

```sh
npm --prefix apps run website:check
node --test tests/aifred-archive.test.mjs
bash tools/release/aifred_monorepo_validate.sh
```

Windows runtime validation requires Windows; Android requires SDK 35/JDK 17; plugin/engine builds require CMake and .NET. Generated binaries, local exports, archives and `.forge` runtime data are ignored.
