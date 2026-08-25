# Function Map

This page maps the active source files and major responsibilities in the consolidated AIFRED monorepo.

## VST3 Plugin

| File | Responsibility |
| --- | --- |
| `plugin-aifred/Source/PluginProcessor.*` | Audio bus setup, process-block routing, host state, Mix A/Mix B analysis snapshots |
| `plugin-aifred/Source/AnalysisEngine.*` | DSP metric extraction, smoothing, loudness/correlation paths, Halo state generation |
| `plugin-aifred/Source/HaloState.*` | Shared raw/interpreted analysis structures, displayed scores, validity/stale/fallback state |
| `plugin-aifred/Source/DiagnosticInterpreter.*` | Converts current DSP/reference state into model-ready diagnostic context without canned fix text |
| `plugin-aifred/Source/AifredEngineClient.*` | Background-safe local engine health, chat, settings, provider/model normalization, and runtime relaunch attempts |
| `plugin-aifred/Source/PluginEditor.*` | Analyze/Reference/Compare UI, Halo rendering, meters, reference controls, chat, provider settings |
| `plugin-aifred/Source/AifredLookAndFeel.*` | Branded JUCE control styling |

The active plugin source list is defined in `plugin-aifred/CMakeLists.txt`.

## Local AIFRED Engine

| File | Responsibility |
| --- | --- |
| `tools/AifredEngine/Program.cs` | Local HTTP gateway, health, chat routing, settings persistence, Ollama/OpenAI provider selection |
| `tools/AifredEngine/AifredEngine.csproj` | Windows self-contained engine publish target |
| `tools/AifredEngine/AifredEngine.Mac.csproj` | macOS arm64 self-contained engine publish target |

Current engine endpoints:

| Method | Route | Purpose |
| --- | --- | --- |
| `GET` | `/health` | Engine/provider/model readiness |
| `POST` | `/analyze` | Engine-side analysis response surface; preserved pending verification of current callers |
| `POST` | `/chat` | Request-driven chat using current provider settings |
| `GET` | `/v1/settings` | Read engine config and user overrides |
| `POST` | `/v1/settings` | Save provider/model/endpoint settings |
| `POST` | `/v1/restart` | Exit for external restart supervision; preserved pending verification of current callers |

The plugin's active runtime path uses `/health`, `/chat`, and `/v1/settings`.

## Website

| File | Responsibility |
| --- | --- |
| `apps/website/index.html` | Public page structure and product/catalog/analyzer surfaces |
| `apps/website/styles.css` | Brand styling, analyzer, catalog, free-download, and release layout |
| `apps/website/app.js` | Catalog player, analyzer DSP, free downloads, contact flow, activity events |
| `apps/website/config.js` | Public runtime config and product display settings |
| `apps/website/_worker.js` | Cloudflare Pages Worker router |
| `apps/website/functions/api/v1/[[path]].js` | Main website/backend/admin API |
| `apps/website/functions/api/[[path]].js` | Legacy `/api/*` compatibility shim; preserved pending production-usage verification |
| `apps/website/functions/ws/chat.js` | WebSocket chat bridge |
| `apps/website/ops.html` | Authenticated status, runtime API routing, provider test, and admin chat console |
| `apps/website/assets/data/beat_catalog.json` | Canonical beat catalog metadata |

## Cloudflare Backend

Main backend responsibilities in `apps/website/functions/api/v1/[[path]].js` include:

| Function/area | Responsibility |
| --- | --- |
| JSON/request helpers | Safe request parsing and JSON responses |
| Admin session helpers | Signed admin session creation and verification |
| Catalog loading | Read and serve catalog metadata |
| Analyzer gate | Score submitted browser-analysis metadata and optionally persist accepted reference metadata |
| `askOpenAI` | OpenAI Responses API call using configured model |
| `askOllama` | Ollama generation call |
| Chat routing | Select configured provider and return chat output |
| Model/settings payloads | Report configured Ollama/OpenAI model routes |
| Runtime API configuration | Store secret-safe provider/endpoint/model selection in KV and test protected provider connectivity |
| GitHub file operations | Read/write/list/delete approved repository paths |
| Upload handlers | Website assets, catalog audio, and reference uploads |
| Activity/inquiry handlers | Persist public runtime events to KV and expose historical read-only records |
| Free download handlers | Stream allowlisted plugin releases and safe catalog paths from R2 with range support |
| Command registry | Execute allowlisted owner/admin operations |

## Android Admin App

Primary implementation:

- `apps/admin-android/app/src/main/java/com/aifred/admin/MainActivity.kt`

Major responsibilities:

| Function/Class | Responsibility |
| --- | --- |
| `MainActivity` | Android entry point |
| `AIFREDAdminApp` | App state, login, navigation, and API client setup |
| `ChatScreen` | Chat UI, local API profiles, model selection, and authenticated Cloudflare route management |
| `UploadScreen` | Catalog/reference/website asset upload UI |
| `CommandScreen` | Backend action buttons and local Android sandbox terminal |
| `ApiClient` | HTTP, WebSocket, admin file, upload, command, catalog, activity, and chat API calls |
| Local login helpers | Offline-aware owner authentication |
| Local shell helpers | Android sandbox command execution |

The app is owner-only and is not a public release artifact.

## Packaging And Release

| Path | Responsibility |
| --- | --- |
| `tools/package-aifred.ps1` | Windows VST3 package staging |
| `tools/AifredWindowsInstaller/` | Windows one-click installer source |
| `tools/AifredWindowsUninstaller/` | Windows uninstaller source |
| `tools/windows/setup-aifred-local-ai.ps1` | Windows local-AI repair/setup helper |
| `tools/macos/package-aifred-macos.sh` | macOS VST3 + engine pkg construction |
| `tools/macos/postinstall` | macOS postinstall configuration/startup setup |
| `tools/macos/setup-aifred-local-ai.sh` | macOS local-AI repair/setup helper |
| `plugin-aifred/AIFRED Engine Control.command` | macOS manual engine/local-AI control surface |
| `.github/workflows/build.yml` | Windows/macOS builds, website checks, tagged releases, optional Cloudflare deploy |

## Validation And Repository Integrity

| File | Responsibility |
| --- | --- |
| `tools/release/aifred_monorepo_validate.sh` | Canonical path, routing, config, stale-path, and workflow validation |
| `tools/release/aifred_repo_inventory.py` | Repository inventory and size/reference report |
| `tools/release/aifred_workflow_audit.py` | Workflow and environment-reference audit |
| `tools/release/aifred_website_dryrun_check.py` | Website dry-run readiness checks |
| `tools/release/aifred_admin_dryrun_check.py` | Admin app dry-run checks |
| `tools/check-no-hardcoded-paths.ps1` | Hardcoded developer-path and old-product-name guard |
| `tools/check-aifred-analysis-regressions.ps1` | Analysis snapshot and sanitizer regression checks |

## Cloudflare Config Roles

| File | Role |
| --- | --- |
| `apps/website/wrangler.toml` | Primary website Pages config and bindings |
| `infra/cloudflare/wrangler.toml` | Operations/support mirror for the same production resources |
| `wrangler.jsonc` | Root convenience config pointed at `apps/website` |
