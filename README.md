# AIFRED

**AIFRED** is the product repository for the North3rnLight3r JUCE VST3, local AIFRED engine, website, beat catalog, Cloudflare backend, and owner-only Android admin app.

The public-facing product is the AIFRED VST3 and North3rnLight3r beat catalog. The source code, admin app, credentials, prompts, backend maps, and deployment controls are private operational assets.

## Monorepo Consolidation Status

Phase 1 consolidation imports the authoritative website/backend into `apps/website` and the authoritative Android admin app into `apps/admin-android`.

The current plugin and local engine runtime paths remain unchanged for this phase:

- `plugin-aifred/` remains the active JUCE plugin source.
- `tools/AifredEngine/` remains the active local engine source.

The Cloudflare website/backend and the local AIFRED engine are separate systems. The website/admin backend lives at `https://www.north3rnlight3r.com/api/v1`; the local engine serves plugin AI/chat at `http://127.0.0.1:8787` and talks to Ollama at `http://127.0.0.1:11434` with model `aifred:latest`.

Older folders and sibling repo copies are preserved until smoke tests and later migration phases pass.

Phase 2 adds validation tooling and path-authority documentation. No runtime plugin or engine move has happened yet, no deployment path has changed, and old/new website and admin paths intentionally coexist until later smoke tests pass.

Phase 3 adds workflow audit tooling and a manual-only monorepo validation workflow. Live deployment behavior is still unchanged, existing website/admin/plugin/engine runtime paths remain preserved, and asset strategy must be decided before merging to `main`.

Phase 4 adds deployment dry-run checks and path migration readiness docs. `apps/website` is not the live deployment root yet, `apps/admin-android` is not the release build root yet, the plugin and engine remain unmoved, old `website/` and `android_admin/` remain preserved, and Cloudflare manual verification plus asset strategy are required before merging to `main`.

Phase 5 adds preview migration planning and a manual-only website preview dry-run workflow. The preview workflow does not deploy, production still uses existing behavior, `apps/website` is still not production root, asset strategy remains a merge blocker, and the plugin and engine remain unmoved.

Phase 6 adds preview approval gates, merge-blocker review, a production non-change statement, a preview runbook draft, and an asset acceptance checklist. No production behavior changed, no deployment path changed, and `main` should still not be merged until asset strategy and Cloudflare binding are approved.

Phase 7 adds preview authorization docs, an evidence template, abort criteria, a production promotion blocker, and an approval record template. No deployment occurred, no production behavior changed, and no merge should happen until human preview approval, asset strategy, and Cloudflare binding are complete.

Phase 8 adds local preview preflight harnesses, website/admin parity manifests, and a preview gate report. No deployment occurred, no production behavior changed, `apps/website` remains preview candidate only, `apps/admin-android` remains task-discovery candidate only, and asset strategy plus Cloudflare verification remain blockers.

## Production

- Website: https://www.north3rnlight3r.com
- Apex domain: https://north3rnlight3r.com
- Latest release: https://github.com/kaeganscott26/AIFRED/releases/latest

## Products

| Product | Purpose | Distribution |
| --- | --- | --- |
| AIFRED VST3 | Mix analysis, reference alignment, compare metering, and chat-guided fix output | Windows installer plus CI-built Windows, macOS, Linux, and Arch packages |
| AIFRED Engine | Local gateway at `127.0.0.1:8787` between the plugin and model providers | Bundled with packages; local AI provider remains Ollama at `http://127.0.0.1:11434` using `aifred:latest` |
| North3rnLight3r Website | Brand storefront, beat catalog playback, VST sales path, free mix analyzer | Cloudflare Pages custom domain |
| Android Admin App | Owner-only control panel for chat, catalog uploads, website file control, shell access, and admin operations | Private install only, never public release |

## What AIFRED Measures

AIFRED converts live audio behavior into a compact release-readiness view:

- Tone balance
- Stereo width and correlation
- Punch and transient density
- Loudness and headroom
- Dynamics and crest factor
- Reference target alignment

The VST separates **Analyze**, **Reference**, and **Compare** into distinct surfaces. Analyze focuses on the current mix signature and candlestick metering. Reference uses one Halo with a target overlay. Compare uses two independent Halo routes for Mix A and Mix B.

Current v0.3.6 JUCE metering surface:

- One-stick session candlestick meter plus 10-minute history meter
- Switchable Halo center display for multiband lanes, waveform, or combined spectrometer view
- Correlation meter filtered above 150 Hz so bass energy does not distort the phase read
- Halo quadrant labels, scale ticks, and readable frequency/loudness/correlation labels
- Center Halo spectrometer matching the website visualizer direction
- Compare-mode analog-style match VU between the two Halos
- Dedicated scrollable chat module without predetermined fix suggestions
- Chat Focus layout with genre target, reference gate sensitivity, and BYO OpenAI/Ollama endpoint setup
- Reference mode with pool ring, five reference rings, five independent reference file pickers, and five reference volume lanes
- K-weighted loudness readout with momentary, short-term, integrated, LRA, and estimated 4x true peak fields
- Local AIFRED engine health detection with request-driven Ollama chat
- Version text in the plugin header so FL Studio cache/install state is visible
- Genre, gate, and BYO API fields save into the host project state

## Repository Map

| Path | Role |
| --- | --- |
| `plugin-aifred/` | JUCE/C++ VST3 source |
| `website/` | Cloudflare Pages site, static catalog, browser analyzer, backend Worker routes |
| `android_admin/` | Private Android admin app |
| `tools/AifredEngine/` | Windows local engine source |
| `tools/AifredWindowsInstaller/` | Windows installer source |
| `tools/AifredWindowsUninstaller/` | Windows uninstaller source |
| `tools/` | Packaging, installer, and verification utilities |
| `.github/workflows/build.yml` | Windows, macOS, Linux, Arch package builds, website checks, release publishing, and Android validation |
| `docs/wiki/` | Operational wiki, guides, maps, and troubleshooting |

## Release Targets

The release workflow builds and packages:

- `AIFRED-VST3-Setup.exe` for Windows
- `AIFRED-Uninstall.exe` for Windows
- `AIFRED-VST3-windows.zip`
- `AIFRED-VST3-macOS.zip`
- `AIFRED-VST3-linux.zip`
- `AIFRED-VST3-arch.zip`

The Android admin app is validated by CI but is **not uploaded as a public artifact** and is **not attached to GitHub Releases**.

## Build Overview

Windows local build:

```powershell
cmake -S . -B build/aifred -DCMAKE_BUILD_TYPE=Release
cmake --build build/aifred --config Release --parallel
powershell -NoProfile -ExecutionPolicy Bypass -File tools\package-aifred.ps1 -BuildRoot build\aifred -OutputDir dist -Platform windows
dotnet publish tools\AifredWindowsInstaller\AifredWindowsInstaller.csproj -c Release -o dist\installer\windows
```

Run `dist\installer\windows\AIFRED-VST3-Setup.exe` to install. The installer requests administrator elevation, installs `Aifred.vst3` to `C:\Program Files\Common Files\VST3`, installs `AifredEngine.exe` to `C:\Program Files\Aifred\bin`, registers the engine gateway at user login, starts it silently, verifies Ollama at `http://127.0.0.1:11434` with `aifred:latest`, then checks the gateway health at `http://127.0.0.1:8787/health`. OpenAI-compatible endpoint/API key settings remain optional in `%AppData%\Aifred\user_settings.json`.

Android local build:

```powershell
$env:JAVA_HOME='C:\Program Files\Microsoft\jdk-17.0.18.8-hotspot'
$env:ANDROID_HOME=Join-Path $env:LOCALAPPDATA 'Android\Sdk'
$env:ANDROID_SDK_ROOT=$env:ANDROID_HOME
$env:Path="$env:JAVA_HOME\bin;$env:ANDROID_HOME\platform-tools;$env:ANDROID_HOME\cmdline-tools\latest\bin;$env:Path"
cd android_admin
.\gradlew.bat assembleDebug
```

## Cloudflare Deployment

Cloudflare Pages serves `website/` with custom domains:

- `www.north3rnlight3r.com`
- `north3rnlight3r.com`

The Pages project can be deployed locally with Wrangler:

```powershell
npx wrangler pages deploy website --project-name=north3rnlight3r --branch=main
```

GitHub Actions can deploy only when repository secrets contain valid Cloudflare deploy credentials. If credentials are rejected, the build still passes and emits a warning because package builds must not be blocked by Cloudflare auth rotation.

## Private Operations

Do not make this repository public while it contains:

- Android admin app source
- Admin login logic
- Backend route maps
- Internal prompts or conversation exports
- Deployment controls
- Website source and catalog management code

The production website is public. This repository is not.

## Documentation

Start here:

- [Wiki Home](docs/wiki/Home.md)
- [User Guide](docs/wiki/User-Guide.md)
- [Admin App Guide](docs/wiki/Admin-App-Guide.md)
- [Developer Guide](docs/wiki/Developer-Guide.md)
- [Backend Map](docs/wiki/Backend-Map.md)
- [Function Map](docs/wiki/Function-Map.md)
- [Troubleshooting](docs/wiki/Troubleshooting.md)
- [Security And Distribution](docs/wiki/Security-And-Distribution.md)
