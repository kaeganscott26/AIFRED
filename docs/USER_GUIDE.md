# AIFRED Beta user guide

## Install the development dependencies

The supported build and install path is Windows x64. Install Visual Studio 2022 with Desktop development with C++, MSVC x64 tools, and a Windows 10 or 11 SDK. Also install CMake, Ninja, PowerShell 7, Python 3, the .NET 10 SDK/runtime, and Git.

Verify the command-line tools in PowerShell 7:

```powershell
git --version
cmake --version
ninja --version
pwsh --version
python --version
dotnet --list-sdks
```

The build script locates Visual Studio and imports its x64 developer environment.

## Build, install, update, and uninstall

Build and test without changing an installed plugin:

```powershell
pwsh -NoProfile -File scripts/windows/build.ps1 -Action test
```

Create and promote a verified release without installing it:

```powershell
pwsh -NoProfile -File scripts/windows/build.ps1 -Action release
```

For either a first install or an update, close the DAW, start elevated PowerShell 7, and run:

```powershell
pwsh -NoProfile -File scripts/windows/lifecycle.ps1 -Action update
```

That lifecycle builds, tests, releases, verifies, installs the VST3 and host, registers startup, starts the host, and checks copied file hashes. Reload or rescan the plugin in the DAW.

Uninstall only the Beta channel with:

```powershell
pwsh -NoProfile -File scripts/windows/lifecycle.ps1 -Action uninstall
```

Settings, references, provider data, and other AIFRED channels are retained. Read [Installation](INSTALLATION.md) before touching an older global-slot install.

## Use the plugin

DSP operates without chat. AifredIntelligenceHost on port 8787 requires .NET 10 and a configured available Ollama/OpenAI-compatible provider. No model is downloaded implicitly. macOS/Linux runtime remains unvalidated.

Select MIX_BALANCED for general mixing, SPECTRUM_SURGICAL for detailed FFT, MASTERING_PRECISION for programme metering, or STEREO_PHASE_DIAGNOSTIC for fast phase response. Profile switching starts a clean observation epoch. Live correlation/width follow current audio; other engineering values summarize observed measurements. Short observations remain insufficient for sustained conclusions. The default FFT viewport is `-96..0 dBFS`; the Options panel can select the other safe display floors without changing measurements.

Analyze observes Mix A; Reference uses compatible locally measured reference files; Compare has independent Mix A/B input pipelines. The first enabled reference slot supplies the selected distribution; slot controls do not average LUFS or fabricate FFT data. Chat sends explicit filtered observations only when asked. Four recent observation/question/response records support follow-up questions. Stated actions are not verified DAW edits.

The plugin does not require Cloudflare to measure audio. Selecting Reference performs one explicit read of the Official metadata pool; configuring the Official API as an OpenAI-compatible provider routes user-triggered model discovery and Chat through `https://north3rnlight3r.com/api/v1`. The production server, website, downloads, telemetry, and administrator software are not part of this repository. Plugin observation history is bounded in memory, not a long-term session archive.
