# Developer Guide

## Requirements

Windows:

- Visual Studio with Desktop development with C++
- CMake
- Ninja or Visual Studio generator
- .NET SDK 10
- Java 17
- Android SDK command-line tools and platform tools for local phone install testing
- Node.js 22+
- Wrangler 4+ through `npx wrangler`

## Local VST Build

```powershell
cmake -S . -B build/aifred -DCMAKE_BUILD_TYPE=Release
cmake --build build/aifred --config Release --parallel
```

The plugin version is taken from the root CMake `project(AIFRED VERSION ...)` value and passed into the UI as `AIFRED_VERSION_STRING`.

The analyzer uses:

- K-weighted loudness path for LUFS-style display and loudness-domain scoring.
- Separate 150 Hz high-passed correlation path so low-frequency phase does not dominate the stereo meter.
- Raw peak still measured directly in dBFS.

If MSVC cannot find standard headers like `string` or `algorithm`, run from the Visual Studio Developer Shell:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64
cmake --build 'C:\Users\Scott\Projects\AIFRED\build\aifred' --config Release --parallel
```

## Windows Installer

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\package-aifred.ps1 -BuildRoot build\aifred -OutputDir dist -Platform windows
dotnet publish tools\AifredWindowsInstaller\AifredWindowsInstaller.csproj -c Release -o dist\installer\windows
```

Install:

```powershell
.\dist\installer\windows\AIFRED-VST3-Setup.exe
```

The installer requests administrator elevation and installs:

- `C:\Program Files\Common Files\VST3\Aifred.vst3`
- `C:\Program Files\Aifred\bin\AifredEngine.exe`
- `C:\Program Files\Aifred\config\config.json`
- `C:\Program Files\Aifred\models\`
- `C:\Program Files\Aifred\logs\`

The installer registers the engine under `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`, starts it silently, and validates `GET http://127.0.0.1:8787/health`.

## AIFRED Engine

Local development:

```powershell
dotnet publish tools\AifredEngine\AifredEngine.csproj -c Release -o dist\engine\windows
Start-Process dist\engine\windows\AifredEngine.exe -WindowStyle Hidden
Invoke-WebRequest -UseBasicParsing http://127.0.0.1:8787/health
```

The engine must never be called from the audio thread. The plugin pings health from UI/background-safe code and keeps deterministic analysis active when the engine is unavailable.

## Android Admin Build

```powershell
$env:JAVA_HOME='C:\Program Files\Microsoft\jdk-17.0.18.8-hotspot'
$env:ANDROID_HOME=Join-Path $env:LOCALAPPDATA 'Android\Sdk'
$env:ANDROID_SDK_ROOT=$env:ANDROID_HOME
$env:Path="$env:JAVA_HOME\bin;$env:ANDROID_HOME\platform-tools;$env:ANDROID_HOME\cmdline-tools\latest\bin;$env:Path"
cd android_admin
.\gradlew.bat assembleDebug
```

The APK is private and should stay local.

## Website Deploy

```powershell
npx wrangler pages deploy website --project-name=north3rnlight3r --branch=main
```

Production must resolve through the custom domains:

- `www.north3rnlight3r.com`
- `north3rnlight3r.com`

The `.pages.dev` URL is only a Cloudflare preview/deployment endpoint.

## GitHub Actions

The workflow validates:

- Windows VST3 build, AIFRED engine publish, and `.exe` package
- Android admin compile
- Website JavaScript syntax
- Hardcoded path guard

Release tags publish only VST packages. The Android admin APK is not released.
