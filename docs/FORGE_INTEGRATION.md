# AIFRED ↔ FORGE Integration

FORGE is not the long-term raw AIFRED log archive. It retains bounded current operational context and lightweight pointers; Desktop Admin/local AIFRED storage owns history.

## Integration boundary

- Root: `integrations/forge/`
- Discovery: `manifest.json` (schema 1.1.0)
- Export bridge: `bridge/export.mjs`
- Schemas: `schemas/`
- Ignored current/history mirror: `exports/`
- Ignored bounded restore area: `archive-workspace/`

The manifest contains no credentials. `AIFRED_ADMIN_SESSION_TOKEN` is supplied only to the export process. `AIFRED_API_BASE_URL` may override the production origin for development.

```sh
node integrations/forge/bridge/export.mjs all
tools/link-forge-integration.sh /path/to/FORGE/integrations/aifred
```

The PowerShell link script provides the equivalent Windows setup. Both refuse unsafe replacement of unrelated destinations.

## Bounded retention

`AIFRED_FORGE_ACTIVE_LOG_LIMIT_MB` controls the completed-history mirror threshold; default is 25 MB. At or above the threshold, the bridge retains `latest` and the newest completed run, archives older completed runs, validates them, updates the archive manifest, and only then removes those local mirror copies. It never deletes Cloudflare KV/R2 or current work.

FORGE reads `runtime/aifred-archive/manifest.json` to discover history. Search and restore require query/category/date plus record and byte bounds. Restored slices are temporary analysis material, not permanent active memory.

No FORGE ToolRouter implementation is stored in AIFRED. The manifest declares capabilities for registration by FORGE's existing authority/router; the bridge does not bypass it.
