# AIFRED

**AIFRED** is the private product monorepo for the North3rnLight3r JUCE VST3, local AIFRED engine, public website and beat catalog, Cloudflare backend, release tooling, and owner-only Android admin app.

## Current Repository State

AIFRED is consolidated into one private monorepo. The canonical authorities are:

- `plugin-aifred/` — JUCE/C++ VST3 plugin source.
- `tools/AifredEngine/` — cross-platform local engine source.
- `apps/website/` — Cloudflare Pages website, browser analyzer, catalog, and backend routes.
- `apps/admin-android/` — private Android admin app source.
- `infra/cloudflare/` — Cloudflare support configuration and operational docs.

The old duplicate `website/`, `android_admin/`, and raw `North3rnlight3r_Beatz/` trees were removed during monorepo consolidation. Historical phase reports under `docs/operations/` remain migration evidence and are not current runtime authority.

## Backend Separation

AIFRED intentionally has two separate backend systems.

### Local AI engine

- Plugin gateway: `http://127.0.0.1:8787`
- Default local provider: Ollama at `http://127.0.0.1:11434`
- Default local model: `aifred:latest`
- OpenAI route: `https://api.openai.com/v1/responses`
- Default OpenAI model when configured: `gpt-5.6-luna`

### Cloudflare website/backend

- Production website: `https://www.north3rnlight3r.com`
- API base: `https://www.north3rnlight3r.com/api/v1`
- WebSocket chat: `https://www.north3rnlight3r.com/ws/chat`
- Website source: `apps/website/`
- Primary app config: `apps/website/wrangler.toml`
- Operations mirror: `infra/cloudflare/wrangler.toml`
- Root convenience config: `wrangler.jsonc`, pointed at `apps/website`

Do not merge the local engine and Cloudflare backend. They have separate runtime, deployment, availability, and security responsibilities.

## Website Assets And Delivery

- Catalog audio URLs route through `/api/v1/assets/audio/catalog/<file>`.
- The Worker reads release and catalog objects from the existing `AIFRED_DOWNLOADS` R2 binding first.
- Local static files under `apps/website/assets/` remain a development/resilience fallback; production R2 parity was verified on 2026-08-25.
- Public plugin downloads use `/api/v1/downloads/plugin?asset=setup|zip|macos`; beat downloads use `/api/v1/assets/audio/catalog/<file>?download=1`.
- Reference material can use `AIFRED_REFERENCE_POOL` KV and `AIFRED_REFERENCE_BUCKET` R2.

The public website currently offers the AIFRED Windows and macOS beta packages plus catalog MP3s as **free downloads**. The PayPal pipeline is disabled.

## Products

| Product | Purpose | Current distribution |
| --- | --- | --- |
| AIFRED VST3 | Mix analysis, reference alignment, comparison metering, and chat-guided feedback | Windows installer/zip and macOS pkg |
| AIFRED Engine | Local gateway between the plugin and model providers | Bundled with Windows and macOS packages |
| North3rnLight3r Website | Storefront, beat catalog, browser analyzer, contact flow, and backend API | Cloudflare Pages custom domain |
| Android Admin App | Owner-only control panel for chat, uploads, files, commands, catalog, and activity | Private local build/install only |

## What AIFRED Measures

- Tone balance
- Stereo width and correlation
- Punch and transient density
- Loudness and headroom
- Dynamics and crest factor
- Reference target alignment

The VST separates **Analyze**, **Reference**, and **Compare** into distinct surfaces. Analyze focuses on the current mix signature and candlestick metering. Reference uses one Halo with target/reference overlays. Compare uses separate Mix A and Mix B analysis routes.

Current v0.3.6 plugin surface includes session and minute-history candlesticks, switchable Halo center views, 150 Hz high-passed correlation, reference rings and lanes, Compare mode, scrollable chat, K-weighted loudness fields, local-engine health detection, version display, and persisted AI/reference settings.

## Repository Map

| Path | Role |
| --- | --- |
| `plugin-aifred/` | JUCE/C++ VST3 source |
| `apps/website/` | Cloudflare Pages site, browser analyzer, catalog, and backend routes |
| `apps/admin-android/` | Private Android admin app |
| `tools/AifredEngine/` | Cross-platform local engine source |
| `tools/AifredWindowsInstaller/` | Windows installer source |
| `tools/AifredWindowsUninstaller/` | Windows uninstaller source |
| `tools/macos/` | macOS packaging and startup tooling |
| `tools/windows/` | Windows local-AI repair/setup tooling |
| `tools/release/` | Inventory, validation, dry-run, parity, and release checks |
| `infra/cloudflare/` | Cloudflare support config and operational docs |
| `.github/workflows/build.yml` | Windows/macOS builds, website validation, tagged releases, and optional Cloudflare deploy |
| `docs/wiki/` | Current operational guides and maps |
| `docs/operations/` | Historical consolidation and preview evidence unless explicitly marked current |

## Current Release Targets

The active release workflow builds and packages:

- `AIFRED-VST3-Setup.exe` for Windows.
- `AIFRED-Uninstall.exe` for Windows.
- `AIFRED-VST3-windows.zip`.
- `AIFRED-VST3-macOS.pkg`.

The currently published `v0.3.6-installer-ai-alias` release predates that pkg output and provides `AIFRED-VST3-macos.zip` as the verified macOS download. The website labels it as a manual-install ZIP; no signed/notarized macOS installer is currently published.

Linux and Arch packaging references may still exist in historical records or generic CPack configuration, but they are **not current GitHub Actions release targets** and were intentionally left untouched pending explicit verification.

The Android admin app is private and is not uploaded as a public artifact or attached to public GitHub releases.

## Build Overview

### Windows

```powershell
cmake -S . -B build/aifred -DCMAKE_BUILD_TYPE=Release
cmake --build build/aifred --config Release --parallel
powershell -NoProfile -ExecutionPolicy Bypass -File tools\package-aifred.ps1 -BuildRoot build\aifred -OutputDir dist -Platform windows
dotnet publish tools\AifredWindowsInstaller\AifredWindowsInstaller.csproj -c Release -o dist\installer\windows
```

The installer installs the VST3 and local engine, writes the default local Ollama configuration, registers engine startup, starts the engine silently, verifies Ollama, and checks `http://127.0.0.1:8787/health`.

### macOS

```sh
cmake -S . -B build-mac -DCMAKE_BUILD_TYPE=Release
cmake --build build-mac --config Release --parallel
tools/macos/package-aifred-macos.sh
```

The pkg installs the VST3 and engine, writes default local Ollama configuration, registers the LaunchAgent, and includes `AIFRED Engine Control.command` for manual start/restart/stop/health operations.

### Android admin app

```powershell
cd apps/admin-android
.\gradlew.bat assembleDebug
```

The admin APK is private and should stay local to the owner.

## Cloudflare Deployment

Cloudflare Pages serves `apps/website/` on:

- `www.north3rnlight3r.com`
- `north3rnlight3r.com`

Install the pinned website tooling, validate, and deploy to the active Pages project:

```powershell
npm ci --prefix apps
npm --prefix apps run website:check
npm --prefix apps run website:deploy
```

Pushes to `main` run validation and packaging. Production deployment is explicit: run the repository npm command after the pushed SHA is confirmed, or manually dispatch the workflow when Cloudflare repository secrets are configured. Cloudflare's duplicate native Git deployment path is disabled.

## Validation

```sh
bash tools/release/aifred_monorepo_validate.sh
```

Optional Android Gradle task discovery:

```sh
bash tools/release/aifred_monorepo_validate.sh --gradle
```

## Verification-First Cleanup Rule

These items were intentionally preserved because removal requires explicit verification first:

- Legacy `/api/*` compatibility shim.
- Local engine `/analyze` route.
- Local engine `/v1/restart` route.
- Generic UNIX CPack configuration.
- `packages/plugin-juce/` and `packages/local-engine/` placeholder directories.
- Local MP3 website fallback files, retained for development/resilience after production R2 parity verification.
- Separate sibling GitHub repositories pending individual archive/delete review.

## Documentation

- [Wiki Home](docs/wiki/Home.md)
- [User Guide](docs/wiki/User-Guide.md)
- [Admin App Guide](docs/wiki/Admin-App-Guide.md)
- [Developer Guide](docs/wiki/Developer-Guide.md)
- [Backend Map](docs/wiki/Backend-Map.md)
- [Function Map](docs/wiki/Function-Map.md)
- [Cloudflare / R2 Setup Guide](docs/wiki/Cloudflare-R2-Setup-Guide.md)
- [Troubleshooting](docs/wiki/Troubleshooting.md)
- [Security And Distribution](docs/wiki/Security-And-Distribution.md)
