# Developer Guide

## Requirements

Windows:

- Visual Studio with Desktop development with C++
- CMake
- Ninja or Visual Studio generator
- .NET SDK 10
- Java 17
- Android SDK command-line tools and platform tools
- Node.js 22+
- Wrangler 4+

macOS CI:

- GitHub-hosted macOS runner
- CMake
- Apple toolchain

Arch Linux CI:

- `archlinux:latest` container
- `base-devel`, `cmake`, `ninja`, `alsa-lib`, `curl`, `freetype2`, X11 libraries, Mesa, WebKitGTK

## Local VST Build

```powershell
cmake -S . -B build/aifred -DCMAKE_BUILD_TYPE=Release
cmake --build build/aifred --config Release --parallel
```

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

Install current user:

```powershell
.\dist\installer\windows\AIFRED-VST3-Setup.exe
```

Install system-wide:

```powershell
.\dist\installer\windows\AIFRED-VST3-Setup.exe --system
```

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
npx wrangler pages deploy website --project-name=aifred-website --branch=main
```

Production must resolve through the custom domains:

- `www.north3rnlight3r.com`
- `north3rnlight3r.com`

The `.pages.dev` URL is only a Cloudflare preview/deployment endpoint.

## GitHub Actions

The workflow validates:

- Windows VST3 build and `.exe` package
- macOS VST3 build and zip
- Arch Linux VST3 build and tarball
- Android admin compile
- Website JavaScript syntax
- Hardcoded path guard

Release tags publish only VST packages. The Android admin APK is not released.

