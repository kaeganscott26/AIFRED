# Developer Guide

This guide covers the current consolidated AIFRED monorepo.

## Canonical Paths

- Plugin: `plugin-aifred/`
- Local engine: `tools/AifredEngine/`
- Website/backend: `apps/website/`
- Android admin app: `apps/admin-android/`
- Cloudflare support config/docs: `infra/cloudflare/`

Do not restore the old top-level `website/` or `android_admin/` trees.

## Requirements

### Windows

- Visual Studio with **Desktop development with C++**
- CMake 3.24+
- Ninja or a supported Visual Studio generator
- .NET SDK 10
- Java 17 for the Android admin app
- Android SDK command-line tools and platform tools for phone install testing
- Node.js 22+
- Wrangler through `npx wrangler`

### macOS

- Xcode command-line tools
- CMake 3.24+
- .NET SDK 10
- Ollama for the default local-AI route
- `pkgbuild`

## Plugin Build

The plugin is AIFRED `0.3.6`, uses C++20, and currently pins JUCE `8.0.14`.

### Windows

```powershell
cmake -S . -B build/aifred -DCMAKE_BUILD_TYPE=Release
cmake --build build/aifred --config Release --parallel
```

### macOS

```sh
cmake -S . -B build-mac -DCMAKE_BUILD_TYPE=Release
cmake --build build-mac --config Release --parallel
```

The plugin version comes from the root `project(AIFRED VERSION ...)` value and is passed into the UI as `AIFRED_VERSION_STRING`.

Current analysis architecture includes:

- K-weighted loudness path for LUFS-style display and loudness-domain scoring.
- Separate 150 Hz high-passed correlation path so low-frequency phase does not dominate the stereo meter.
- Direct dBFS peak measurement.
- Canonical interpreted `HaloState` snapshot consumed by UI and chat context.
- Background-safe engine networking outside the audio thread.

## Windows Packaging

Build the plugin first, then package:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\package-aifred.ps1 -BuildRoot build\aifred -OutputDir dist -Platform windows
dotnet publish tools\AifredWindowsInstaller\AifredWindowsInstaller.csproj -c Release -o dist\installer\windows
dotnet publish tools\AifredWindowsUninstaller\AifredWindowsUninstaller.csproj -c Release -o dist\uninstaller\windows
```

Install:

```powershell
.\dist\installer\windows\AIFRED-VST3-Setup.exe
```

The installer places the runtime under standard system locations, including:

- `C:\Program Files\Common Files\VST3\Aifred.vst3`
- `C:\Program Files\Aifred\bin\AifredEngine.exe`
- `C:\Program Files\Aifred\config\config.json`
- `C:\Program Files\Aifred\models\`
- `C:\Program Files\Aifred\logs\`

The default local AI route is:

```text
AIFRED VST3
  -> http://127.0.0.1:8787
  -> http://127.0.0.1:11434
  -> aifred:latest
```

The optional OpenAI route uses:

```text
https://api.openai.com/v1/responses
model: gpt-5.6-luna
```

when an API key is configured.

## macOS Packaging

```sh
cmake -S . -B build-mac -DCMAKE_BUILD_TYPE=Release
cmake --build build-mac --config Release --parallel
tools/macos/package-aifred-macos.sh
sudo installer -pkg dist/macos/AIFRED-VST3-macOS.pkg -target /
```

The package installs:

- `/Library/Audio/Plug-Ins/VST3/Aifred.vst3`
- `/Library/Application Support/Aifred/bin/AifredEngine`
- `/Library/Application Support/Aifred/config/config.json`
- `/Library/Application Support/Aifred/setup-aifred-local-ai.sh`
- `/Library/Application Support/Aifred/AIFRED Engine Control.command`
- `/Library/LaunchAgents/com.aifred.engine.plist`

The LaunchAgent starts the engine at login. The control command can start, restart, stop, repair, or inspect the local runtime.

## AIFRED Engine

### Windows publish

```powershell
dotnet publish tools\AifredEngine\AifredEngine.csproj -c Release -o dist\engine\windows
Start-Process dist\engine\windows\AifredEngine.exe -WindowStyle Hidden
Invoke-WebRequest -UseBasicParsing http://127.0.0.1:8787/health
```

### macOS publish

```sh
dotnet publish tools/AifredEngine/AifredEngine.Mac.csproj -c Release -o dist/engine/macos/osx-arm64
```

Engine routes:

- `GET /health`
- `POST /analyze`
- `POST /chat`
- `GET /v1/settings`
- `POST /v1/settings`
- `POST /v1/restart`

The plugin currently uses `/health`, `/chat`, and `/v1/settings`. `/analyze` and `/v1/restart` remain in place pending explicit verification before any removal.

## Android Admin Build

Current app configuration:

- App version: `2.3.0` (`versionCode = 243`)
- `compileSdk = 35`
- `targetSdk = 35`
- JVM target: 17

Build:

```sh
cd apps/admin-android
./gradlew :app:assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

Windows PowerShell:

```powershell
$env:JAVA_HOME='C:\Program Files\Microsoft\jdk-17.0.18.8-hotspot'
$env:ANDROID_HOME=Join-Path $env:LOCALAPPDATA 'Android\Sdk'
$env:ANDROID_SDK_ROOT=$env:ANDROID_HOME
$env:Path="$env:JAVA_HOME\bin;$env:ANDROID_HOME\platform-tools;$env:ANDROID_HOME\cmdline-tools\latest\bin;$env:Path"
cd apps/admin-android
.\gradlew.bat assembleDebug
```

Install locally:

```powershell
$adb=Join-Path $env:LOCALAPPDATA 'Android\Sdk\platform-tools\adb.exe'
& $adb install -r app\build\outputs\apk\debug\app-debug.apk
```

The Android admin app is owner-only and is not a public release artifact.

Major Gradle/AGP/Kotlin version changes should be handled as a separate tested migration, not mixed into routine cleanup.

## Website Development

Static source and backend routes live under `apps/website/`.

Syntax checks:

```sh
node --check apps/website/app.js
node --check apps/website/_worker.js
node --check 'apps/website/functions/api/v1/[[path]].js'
node --check 'apps/website/functions/api/[[path]].js'
node --check apps/website/functions/ws/chat.js
```

The legacy `apps/website/functions/api/[[path]].js` compatibility shim is intentionally preserved until production usage is verified.

## Cloudflare Config Roles

- `apps/website/wrangler.toml` — primary app-level Pages config and bindings.
- `infra/cloudflare/wrangler.toml` — operations/support mirror.
- `wrangler.jsonc` — root convenience config pointed at `apps/website`.

Current configured storage resources include:

- `AIFRED_DOWNLOADS`
- `AIFRED_REFERENCE_BUCKET`
- `AIFRED_REFERENCE_POOL`
- `AIFRED_SALES_LOG`

## Website Deploy

```powershell
npm ci --prefix apps
npm --prefix apps run website:check
npm --prefix apps run website:deploy
```

Production domains:

- `www.north3rnlight3r.com`
- `north3rnlight3r.com`

Pushes validate and package the repository. Production deploys are explicit through the repository npm command or a manual workflow dispatch, preventing Cloudflare native Git and GitHub Actions from racing to deploy the same commit.

## GitHub Actions

The main workflow currently validates and builds:

- Windows VST3 zip and installer.
- Windows uninstaller.
- macOS VST3 pkg.
- Website JavaScript syntax.
- Hardcoded-path and product-name guard.
- Analysis snapshot regression checks.
- Final monorepo validation.

Release tags publish only the active Windows/macOS VST release artifacts. The Android admin APK is not published as a public release artifact.

Linux and Arch are not current GitHub Actions release targets. Generic UNIX CPack configuration remains pending separate verification.

## Monorepo Validation

```sh
bash tools/release/aifred_monorepo_validate.sh
```

Optional Gradle task discovery:

```sh
bash tools/release/aifred_monorepo_validate.sh --gradle
```

The validator checks canonical paths, stale-path absence, routing references, current model references, Cloudflare config paths, workflow paths, and excluded build/cache folders.
