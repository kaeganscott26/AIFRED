# Beta backend map

This map records the Beta repository's current fallback role and its eventual public-plugin boundary. Do not remove the existing website or backend while it still carries production traffic.

## Current source ownership

| Surface | Beta source | Current role |
| --- | --- | --- |
| Beta VST3 | `plugin-aifred/` | public Beta plugin |
| shared measurements and filtering | `shared-dsp/`, engine, BufferHunter, filter | public Beta runtime, pinned by `shared-core.lock.json` |
| local provider transport | `tools/AifredIntelligenceHost/` | Beta channel host on port 8787 |
| website | `apps/website/` | live fallback until Official Pages ownership is proven |
| Pages Functions API | `apps/website/functions/` and supporting packages | live fallback until the dedicated Official Worker is proven |
| Android and desktop administration | `apps/admin-android/`, `apps/admin-desktop/` | temporary production fallback; future authority belongs in Official |
| web operations console | `apps/website/ops/` | temporary production fallback; future authority belongs in Official |
| Cloudflare configuration and deployment scripts | `infra/cloudflare/`, `scripts/` | preserve until production migration is complete |

## Target boundary after migration

Beta will retain only the plugin, required shared DSP/measurement/filtering code, Beta Intelligence Host and API client configuration, Beta build/install/package/release scripts, plugin tests, and Beta-specific documentation.

Official will own the website, Android Admin, desktop Admin, `/ops`, the dedicated `aifred-api` Worker, production Cloudflare configuration, and production deployment operations.

Target traffic after an authorized cutover:

```text
north3rnlight3r.com/*      -> Cloudflare Pages website from Official
north3rnlight3r.com/api/*  -> dedicated aifred-api Worker from Official
```

That target is not proof of current deployment. Beta retirement begins only after staging and production smoke tests, rollback capture, API route cutover, Pages source migration, and joint website/API validation.

## Plugin traffic policy

The VST3 performs measurement locally and must not make periodic Cloudflare calls merely because it is open. Reference, Chat, and Analysis are explicit user actions. The local health behavior on port 8787 remains local-only.

See [Repository Construction](docs/REPOSITORY_CONSTRUCTION.md), [Repository Map](docs/REPOSITORY_MAP.md), and [Installation](docs/INSTALLATION.md).
