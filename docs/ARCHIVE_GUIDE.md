# AIFRED Archive Guide

Desktop Admin is AIFRED's local cold-storage manager. The canonical ignored archive root is `runtime/aifred-archive/`; it is never committed.

## Trigger and safety

- Setting: `AIFRED_FORGE_ACTIVE_LOG_LIMIT_MB`
- Default: 25 MB
- Active source: `integrations/forge/exports/history`
- Trigger: active bytes are greater than or equal to the configured threshold
- Eligible: older completed export-run directories
- Protected: `exports/latest`, newest completed run, incomplete/staged writes, production KV/R2, permanent archives

The engine writes staged gzip JSONL, reopens and decompresses it, parses every record, verifies source sizes and per-record SHA-256 values, verifies the bundle SHA-256 and count, writes metadata, atomically updates `manifest.json`, then prunes only archived FORGE-local mirror runs. Failure before manifest completion leaves source data intact.

## Format

Bundles live at `forge-context/YYYY/MM/DD/aifred-forge-context-<UTC>.jsonl.gz`. Each line is a versioned record containing its completed run, relative path, size, checksum and base64 content. The companion `.meta.json` contains archive ID, schema 1.0.0, UTC range/creation time, counts, sizes, categories, checksum and relative path. The manifest contains metadata only.

## Commands

```sh
node tools/aifred-archive.mjs status
node tools/aifred-archive.mjs rotate
node tools/aifred-archive.mjs archive --force
node tools/aifred-archive.mjs list
node tools/aifred-archive.mjs verify
node tools/aifred-archive.mjs search --query error --limit 100 --byte-limit 1048576
node tools/aifred-archive.mjs restore --category site --start 2026-08-01T00:00:00Z --end 2026-08-31T23:59:59Z --limit 100 --byte-limit 1048576
node tools/aifred-archive.mjs rebuild-index
node tools/aifred-archive.mjs prune --id <exact-archive-id> --confirm
```

Search/restore hard ceilings are 1,000 records and 10 MiB. Restore replaces the ignored `integrations/forge/archive-workspace/` with the requested bounded slice. Permanent pruning is never automatic; Desktop Admin asks for confirmation and the CLI requires both exact ID and `--confirm`.

If verification fails, do not delete or edit the source run. Correct disk/permission/corruption problems and retry. `rebuild-index` reads metadata and verifies each referenced archive before replacing the index.
