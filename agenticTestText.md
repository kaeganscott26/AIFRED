# AIFRED Workspace and Installation Guide

## What this repository contains

AIFRED is a private product monorepo containing an audio-analysis plugin, its local AI gateway, a public website and backend, release tooling, and a private Android administration app.

### Main workspace paths

- `plugin-aifred/` — Canonical JUCE/C++ source for the AIFRED VST3 audio plugin.
- `tools/AifredEngine/` — Cross-platform local engine used as the gateway between the plugin and local Ollama or an OpenAI-compatible provider.
- `apps/website/` — Cloudflare Pages website, browser analyzer, beat catalog, storefront, and backend API routes.
- `apps/admin-android/` — Private owner-only Android administration app.
- `infra/cloudflare/` — Cloudflare configuration support and operational documentation.
- `tools/AifredWindowsInstaller/` — Windows installer source.
- `tools/AifredWindowsUninstaller/` — Windows uninstaller source.
- `tools/macos/` — macOS packaging and engine-startup tooling.
- `tools/windows/` — Windows setup and repair tooling for local AI.
- `tools/release/` — Repository validation, inventory, parity, dry-run, and release checks.
- `.github/workflows/` — Continuous-integration workflows for Windows and macOS packages, website validation, and releases.
- `docs/` — Architecture, release, operations, troubleshooting, and user/developer documentation.
- `models/aifred/` — AIFRED local-model resources.
- `packages/` — Preserved package-related directories; some are placeholders pending explicit verification.
- `CMakeLists.txt` — Root CMake build definition.
- `CMakePresets.json` — Includes the `ninja-release` CMake configure/build preset.
- `README.md` — Canonical repository overview, build commands, product map, and documentation links.
- `docs/RELEASE_NOTES.md` — Current v0.3.6 release behavior and packaging notes.

The local AI engine and Cloudflare website backend are separate systems and should not be merged. The local engine normally listens at `http://127.0.0.1:8787`; the public website/backend is deployed through Cloudflare.

## Product features

The AIFRED VST3 analyzes tone balance, stereo width and correlation, punch and transient density, loudness and headroom, dynamics and crest factor, and alignment to reference targets. Its current interface provides Analyze, Reference, and Compare surfaces; Halo visualizations; multiband and waveform views; session and minute-history candlesticks; separate Mix A and Mix B analysis; and scrollable AI-assisted feedback based on current DSP and reference context.

## Installing a released build

Installing a published package is the simplest approach.

### Windows

1. Obtain `AIFRED-VST3-Setup.exe` from the authorized AIFRED release/download location.
2. Run the installer and follow its prompts.
3. The installer places the VST3 and local engine, writes the default local Ollama configuration, registers engine startup, starts the engine, and checks the local health endpoint.
4. Restart or rescan plugins in your DAW, then load AIFRED as a VST3 effect.
5. If using the default local AI route, ensure Ollama is installed and the `aifred:latest` model is available.

A portable Windows artifact, `AIFRED-VST3-windows.zip`, may also be provided, but the installer is the preferred path because it configures the bundled engine.

### macOS

1. Obtain `AIFRED-VST3-macOS.pkg` from the authorized AIFRED release/download location.
2. Open the package and follow the installer prompts.
3. Restart or rescan plugins in your DAW, then load AIFRED as a VST3 effect.
4. If needed, use the installed `AIFRED Engine Control.command` utility to start, restart, stop, or check the engine.
5. If using the default local AI route, ensure Ollama and the `aifred:latest` model are available.

The current macOS package is not signed or notarized, so macOS may require explicit approval in Privacy & Security before it can be opened. Only bypass warnings when the package came from a trusted AIFRED source.

## Building and installing from source

### Windows build

Prerequisites include Git, CMake, a supported C++ toolchain, and the .NET SDK required by the installer project.

```powershell
cmake -S . -B build/aifred -DCMAKE_BUILD_TYPE=Release
cmake --build build/aifred --config Release --parallel
powershell -NoProfile -ExecutionPolicy Bypass -File tools\package-aifred.ps1 -BuildRoot build\aifred -OutputDir dist -Platform windows
dotnet publish tools\AifredWindowsInstaller\AifredWindowsInstaller.csproj -c Release -o dist\installer\windows
```

After a successful build, use the generated installer under `dist/installer/windows/`.

### macOS build

Prerequisites include Git, CMake, Xcode command-line tools, and the dependencies required by the packaging script.

```sh
cmake -S . -B build-mac -DCMAKE_BUILD_TYPE=Release
cmake --build build-mac --config Release --parallel
tools/macos/package-aifred-macos.sh
```

Install the generated `.pkg` after the package step completes.

### Optional Ninja preset

If Ninja is installed, the repository also defines a release preset:

```sh
cmake --preset ninja-release
cmake --build --preset ninja-release
```

## Validation

Run the monorepo validation script after source changes or before packaging:

```sh
bash tools/release/aifred_monorepo_validate.sh
```

## Notes

- Windows and macOS are the current supported release-package targets.
- Linux and Arch packages are not current GitHub Actions release targets.
- The Android admin app is private and is not distributed as a public release artifact.
- Do not put API keys or other credentials into this repository. Configure provider credentials through the intended local or deployment configuration mechanisms.
