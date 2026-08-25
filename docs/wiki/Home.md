# AIFRED Wiki

This wiki documents the current consolidated AIFRED monorepo: VST3 plugin, local engine, website/backend, private Android admin app, Cloudflare storage/deployment, packaging, distribution, and troubleshooting.

## Production Entry Points

- Website: https://www.north3rnlight3r.com
- Apex domain: https://north3rnlight3r.com
- GitHub release page: https://github.com/kaeganscott26/AIFRED/releases/latest

## Current Authority Paths

- `plugin-aifred/` — VST3 plugin
- `tools/AifredEngine/` — local engine
- `apps/website/` — website and Cloudflare backend
- `apps/admin-android/` — owner-only Android admin app
- `infra/cloudflare/` — Cloudflare support configuration and operations docs

## Wiki Index

| Page | Purpose |
| --- | --- |
| [User Guide](User-Guide.md) | Website, analyzer, VST, AI routing, installation, and free downloads |
| [Admin App Guide](Admin-App-Guide.md) | Owner-only Android app operation and command/file/upload behavior |
| [Developer Guide](Developer-Guide.md) | Local setup, builds, packages, CI, validation, and deployment |
| [Backend Map](Backend-Map.md) | Cloudflare Worker routes, storage bindings, and data flow |
| [Function Map](Function-Map.md) | Active module and responsibility map across the monorepo |
| [Cloudflare / R2 Setup Guide](Cloudflare-R2-Setup-Guide.md) | Current storage, release-object, asset-upload, and deployment configuration |
| [Troubleshooting](Troubleshooting.md) | Common failures and recovery steps |
| [Security And Distribution](Security-And-Distribution.md) | Privacy, release boundaries, admin app rules, and public delivery |

## Current Product Truth

- Plugin version: `0.3.6`
- Default local model: `aifred:latest`
- Default OpenAI model when configured: `gpt-5.6-luna`
- Current public beta distribution: free Windows plugin and catalog MP3 downloads
- Current CI release targets: Windows installer/uninstaller/zip and macOS pkg
- Android admin app: private owner-only, not a public release artifact

## Product Principles

- Keep the public product honest about what currently exists.
- Keep admin tooling private.
- Keep the website on the North3rnLight3r production domains, not a preview domain.
- Keep the Cloudflare website/backend separate from the local VST engine.
- Keep canonical source paths explicit.
- Keep removal decisions verification-first when runtime usage is uncertain.
- Keep release packages easy to install.
- Keep analysis modes visually and technically distinct.
- Keep backend controls auditable and route-based.
