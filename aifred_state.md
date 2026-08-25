# AIFRED Development State

Updated: 2026-07-11

This document describes the committed repository state. It does not attempt to report uncommitted files from any one developer workstation.

## Current Goal

Keep AIFRED's consolidated monorepo truthful, buildable, and easy to reason about while completing the current Windows/macOS plugin path, local AI routing, OpenAI routing, and Cloudflare/R2 production setup.

Current AI routes:

- Plugin gateway: `http://127.0.0.1:8787`
- Local Ollama endpoint: `http://127.0.0.1:11434`
- Local model: `aifred:latest`
- OpenAI endpoint: `https://api.openai.com/v1/responses`
- Default OpenAI model: `gpt-5.6-luna`

## Canonical Repository Authorities

- VST3 plugin: `plugin-aifred/`
- Local engine: `tools/AifredEngine/`
- Website and Cloudflare backend: `apps/website/`
- Android admin app: `apps/admin-android/`
- Cloudflare support config/docs: `infra/cloudflare/`

Old duplicate `website/` and `android_admin/` source trees are not active and should not be restored.

## Current Release Targets

The active GitHub Actions release matrix currently produces:

- Windows VST3 zip
- Windows one-click installer
- Windows uninstaller
- macOS VST3 pkg

Linux and Arch are not current CI release targets. Generic UNIX packaging configuration is preserved pending explicit verification rather than being removed blindly.

Current release metadata in `apps/website/.dev.vars.example` points to:

- Plugin release tag: `v0.3.6-installer-ai-alias`
- Release version: `v0.3.6-installer-ai-alias`

## Plugin State

The JUCE VST3 is versioned as `0.3.6` and currently includes:

- Analyze, Reference, and Compare modes
- Canonical interpreted `HaloState` analysis snapshot
- Tone, Width, Punch, Loudness, Dynamics, correlation, crest, and transient measurements
- K-weighted loudness windows
- 150 Hz high-passed correlation path
- Session and minute-history candlesticks
- Reference pool plus five personal reference lanes
- Mix A / Mix B comparison routing
- Scrollable request-driven chat
- Local engine health checks off the audio thread
- Optional OpenAI-compatible routing

JUCE is pinned in `plugin-aifred/CMakeLists.txt`.

## Local Engine State

`tools/AifredEngine/Program.cs` serves:

- `GET /health`
- `POST /analyze`
- `POST /chat`
- `GET /v1/settings`
- `POST /v1/settings`
- `POST /v1/restart`

The plugin's active runtime path uses `/health`, `/chat`, and `/v1/settings`.

The `/analyze` and `/v1/restart` routes are preserved for verification before any removal decision.

Windows publishes from:

- `tools/AifredEngine/AifredEngine.csproj`

macOS publishes from:

- `tools/AifredEngine/AifredEngine.Mac.csproj`

Both use the shared `Program.cs` runtime.

## Website And Cloudflare State

Production domains:

- `https://www.north3rnlight3r.com`
- `https://north3rnlight3r.com`

Canonical website/backend source:

- `apps/website/`

Main routes:

- `/api/v1/*`
- `/api/*` legacy compatibility shim
- `/ws/chat`

The legacy `/api/*` shim is preserved until production usage is explicitly verified.

Cloudflare configuration roles:

- `apps/website/wrangler.toml` — app-level Pages configuration and bindings
- `infra/cloudflare/wrangler.toml` — operations/support mirror
- `wrangler.jsonc` — root convenience configuration pointed at `apps/website`

Storage bindings in the current repo include:

- `AIFRED_DOWNLOADS`
- `AIFRED_REFERENCE_BUCKET`
- `AIFRED_REFERENCE_POOL`
- `AIFRED_SALES_LOG`

Catalog audio uses R2 first and local static files as a development/resilience fallback. All 63 meaningful website assets matched R2 by key and size on 2026-08-25.

## Android Admin State

The private Android app is under `apps/admin-android/` and currently uses:

- App version `2.3.0` (`versionCode = 243`)
- `compileSdk = 35`
- `targetSdk = 35`
- Java/Kotlin JVM target 17
- Jetpack Compose
- OkHttp
- Coroutines

The admin app is owner-only. It is built locally or validated by CI and is not a public release artifact.

Version 2.3.0 reflects free website distribution, keeps sales data read-only/historical, and includes local-only non-root Linux/Termux/Android command registry actions.

The Chat tab also provides app-private Website/Ollama/OpenAI API profiles with endpoint/model/key testing. Catalog playback resolves relative website stream paths against the production base URL, so Android and browser playback use the same R2-backed API route.

## Current Validation

Primary validation command:

```sh
bash tools/release/aifred_monorepo_validate.sh
```

Optional Gradle task discovery:

```sh
bash tools/release/aifred_monorepo_validate.sh --gradle
```

The main build workflow validates Windows/macOS packaging, website JavaScript syntax, product/path guards, analysis regressions, and monorepo shape.

## Verification-First Candidates

Do not remove these without a separate verification pass:

- `apps/website/functions/api/[[path]].js`
- Local engine `/analyze`
- Local engine `/v1/restart`
- Generic UNIX CPack configuration
- `packages/plugin-juce/`
- `packages/local-engine/`
- Local MP3 website fallback files
- Sibling AIFRED repositories

## Known Remaining Work

- Verify Cloudflare bindings and production environment configuration.
- Verify R2 catalog parity before removing local audio fallbacks.
- Verify the current Windows installer end to end on Windows.
- Verify macOS LaunchAgent behavior after a real reboot.
- Add macOS signing/notarization when the install path is otherwise stable.
- Review Android build-tool modernization as a separate tested migration rather than mixing major toolchain changes into a cleanup pass.
