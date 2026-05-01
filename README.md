# AIFRED

**AIFRED** is the product repository for the North3rnLight3r JUCE VST3, local AIFRED engine, website, beat catalog, Cloudflare backend, and owner-only Android admin app.

The public-facing product is the AIFRED VST3 and North3rnLight3r beat catalog. The source code, admin app, credentials, prompts, backend maps, and deployment controls are private operational assets.

## Production

- Website: https://www.north3rnlight3r.com
- Apex domain: https://north3rnlight3r.com
- Latest release: https://github.com/kaeganscott26/AIFRED/releases/latest

## Products

| Product | Purpose | Distribution |
| --- | --- | --- |
| AIFRED VST3 | Mix analysis, reference alignment, compare metering, and chat-guided fix output | Windows installer plus CI-built Windows, macOS, Linux, and Arch packages |
| AIFRED Engine | Local AI chat companion at `127.0.0.1:8787` | Bundled with Windows packages; local Ollama-backed chat uses the installed aifred:latest model |
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

Current v0.3.3 JUCE metering surface:

- Distinct cyan, gold, and redline theme options
- One-stick session candlestick meter plus 10-minute history meter
- Switchable Halo center display for multiband lanes, waveform, or combined spectrometer view
- Correlation meter filtered above 150 Hz so bass energy does not distort the phase read
- Halo quadrant labels, scale ticks, and readable frequency/loudness/correlation labels
- Center Halo spectrometer matching the website visualizer direction
- Compare-mode analog-style match VU between the two Halos
- Dedicated chat module without predetermined fix suggestions
- Options for theme, layout focus, genre target, reference gate sensitivity, and BYO OpenAI/Ollama endpoint setup
- Reference mode with pool ring, five reference rings, reference file picker, and five reference volume lanes
- K-weighted loudness readout with momentary, short-term, integrated, LRA, and estimated 4x true peak fields
- Local AIFRED engine health detection with request-driven Ollama chat
- Version text in the plugin header so FL Studio cache/install state is visible
- Theme, layout, genre, gate, and BYO API fields save into the host project state

## Repository Map

| Path | Role |
| --- | --- |
| `plugin-aifred/` | JUCE/C++ VST3 source |
| `website/` | Cloudflare Pages site, static catalog, browser analyzer, backend Worker routes |
| `android_admin/` | Private Android admin app |
| `tools/AifredEngine/` | Windows local engine source |
| `tools/AifredWindowsInstaller/` | Windows installer source |
| `tools/` | Packaging, installer, and verification utilities |
| `.github/workflows/build.yml` | Windows, macOS, Linux, Arch package builds, website checks, release publishing, and Android validation |
| `docs/wiki/` | Operational wiki, guides, maps, and troubleshooting |

## Release Targets

The release workflow builds and packages:

- `AIFRED-VST3-Setup.exe` for Windows
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

Run `dist\installer\windows\AIFRED-VST3-Setup.exe` to install. The installer requests administrator elevation, installs `Aifred.vst3` to `C:\Program Files\Common Files\VST3`, installs `AifredEngine.exe` to `C:\Program Files\Aifred\bin`, registers the engine at user login, starts it silently, checks `http://127.0.0.1:8787/health`, and can save an OpenAI-compatible endpoint/API key into `%AppData%\Aifred\user_settings.json`.

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




