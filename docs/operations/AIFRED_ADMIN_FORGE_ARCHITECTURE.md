# AIFRED Admin, Export, and FORGE Architecture

## Routing authority

- Production web/API base: `https://www.north3rnlight3r.com`
- Pages project: `aifred-site`; aliases include apex, `www`, and `aifred-site.pages.dev`
- Canonical AI contract: `/health`, `/v1/models`, `/v1/chat/completions`
- Administrative contract: `/api/v1/admin/*`, protected by the existing signed bearer session
- VST → AifredEngine: `http://127.0.0.1:8787`
- AifredEngine → local Ollama by default: `http://127.0.0.1:11434`

The cloud and local bases intentionally differ. Route semantics are normalized; standalone VST operation remains local-first.

## Export contract

`GET /api/v1/admin/export/site` produces schema `aifred.site-data` version `1.0.0`, including bounded activity, page views/referrers, correlated download events, session identifiers, inquiries, failures, API/admin activity, and deployment metadata.

`GET /api/v1/admin/export/tracks` produces schema `aifred.track-analysis` version `1.0.0`, including the catalog, accepted reference records, analysis/reference events, failures, and engine route metadata. Sources are the catalog, `AIFRED_REFERENCE_POOL` KV, and sanitized `AIFRED_SALES_LOG` activity records.

Both exports use UTC ISO-8601 timestamps, deterministic timestamped filenames, JSON, `Cache-Control: no-store`, and the existing admin authorization boundary. Secret-shaped fields are recursively removed.

## FORGE bridge

`integrations/forge/manifest.json` is the machine-readable discovery document. `integrations/forge/bridge/export.mjs` mirrors authenticated exports into ignored local paths and keeps a latest copy. Credentials are accepted only through process environment and are not stored.

FORGE's AIFRED mirror is bounded by `AIFRED_FORGE_ACTIVE_LOG_LIMIT_MB` (default `25`). At or above the threshold, the bridge selects completed history runs while retaining the newest run and every `latest` snapshot. It writes gzip JSONL under ignored `runtime/aifred-archive/forge-context/YYYY/MM/DD`, verifies gzip readability, record counts, per-record SHA-256 checksums and the bundle SHA-256, atomically updates the small archive manifest, and only then removes the archived local mirror runs. Production KV, R2, Pages data, current work, and the newest snapshot are never pruned.

The manifest declares bounded `status`, `list`, `verify`, `search`, and `restore` commands. Search/restore require record and byte limits (CLI ceilings: 1,000 records and 10 MiB). Restore materializes a requested slice under ignored `integrations/forge/archive-workspace`; it does not import permanent history back into active context. Permanent deletion is manual-only: `prune` requires both an exact archive ID and `--confirm`; both desktop clients add an interactive confirmation.

## Shared administration

- Android, browser `/ops`, and both desktops use `https://www.north3rnlight3r.com` by default and the same protected `/api/v1/admin/*` routes.
- `/ops` provides Overview, Analytics, Downloads, Track Analysis, API, Logs, Inquiries, Exports, FORGE, and Archive sections. UTC wire timestamps are converted once at presentation.
- Windows preserves its WinForms architecture. Live buttons use the protected API; archive buttons call the named archive CLI. Offline access applies only to local archives.
- macOS uses a native AppKit shell with production `/ops` in a WebKit view and the same native archive commands. Build output is local-only under ignored `apps/admin-desktop/build/`.

`node tools/aifred-archive.mjs status|rotate|archive|list|verify|search|restore` is the sole local archive command contract. `archive` forces eligible completed-run archival; `rotate` acts only at the configured threshold. Neither command deletes permanent archives.

The repository contains no FORGE ToolRouter implementation. The bridge does not execute FORGE tools or bypass authority; an external FORGE workspace may consume the manifest/API through its own router or use the safe link scripts in `tools/`.
