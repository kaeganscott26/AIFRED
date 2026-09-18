# AIFRED Beta

AIFRED Beta is a VST3 audio plugin for realtime mix measurement, Reference and Compare workflows, and user-triggered AI chat. Audio measurement runs locally in the plugin. AI chat is provided by the local `AifredIntelligenceHost`, which routes requests to Ollama by default or to a configured OpenAI-compatible provider.

Beta uses its own runtime channel and IntelligenceHost port so it can coexist with AIFRED Official:

| Component | Beta |
| --- | --- |
| Product channel | `beta` |
| IntelligenceHost | `AifredIntelligenceHost` |
| Host address | `http://127.0.0.1:8787` |
| Default provider | Ollama |
| Default model | `aifred:latest` |
| Plugin format | VST3 |

## Install The macOS DMG

The macOS installer supports Apple Silicon (`arm64`). The DMG contains the plugin, a self-contained IntelligenceHost, and the setup package. The installer configures the local services automatically.

### Requirements

- macOS on Apple Silicon
- A logged-in macOS user during installation
- Internet access for the Ollama download and model pull
- A DAW that supports VST3 plugins
- Administrator permission for installation

### Installation

1. Close your DAW.
2. Open `AIFRED-Beta-0.3.6-macos-arm64.dmg`.
3. Double-click `Install AIFRED Beta.command`.
4. Approve the administrator prompt.
5. Leave the installer terminal open until it reports that setup is complete.
6. Restart or rescan VST3 plugins in your DAW.

The installer:

1. Installs `Aifred.vst3` into `/Library/Audio/Plug-Ins/VST3`.
2. Installs the self-contained Beta IntelligenceHost under `/Library/Application Support/Aifred/beta/IntelligenceHost`.
3. Downloads `/Applications/Ollama.app` if Ollama is not installed.
4. Starts Ollama and waits for its API on port `11434`.
5. Pulls the `aifred:latest` model.
6. Registers and starts the Beta host in the logged-in user's LaunchAgent session.

The host LaunchAgent is installed at:

```text
~/Library/LaunchAgents/com.north3rnlight3r.aifred-intelligence-host.plist
```

### Verify macOS setup

After installation, check the host directly:

```bash
curl http://127.0.0.1:8787/health
```

The response should identify `AifredIntelligenceHost` and the `beta` product channel. Check Ollama separately:

```bash
curl http://127.0.0.1:11434/api/tags
```

If the host is not running, reload the user LaunchAgent:

```bash
launchctl kickstart -k "gui/$(id -u)/com.north3rnlight3r.aifred-intelligence-host"
```

Host logs are written to:

```text
/tmp/aifred-intelligence-host.log
/tmp/aifred-intelligence-host.error.log
```

## Install On Windows

The Windows build and lifecycle scripts target x64 Windows.

### Requirements

- Windows 10 or 11 x64
- Visual Studio 2022 with Desktop development with C++
- MSVC x64 build tools and a Windows SDK
- CMake 3.24 or newer and Ninja
- PowerShell 7
- Python 3
- .NET 10 SDK/runtime
- Git

Build or obtain a validated current release, then run:

```powershell
pwsh -NoProfile -File scripts/windows/lifecycle.ps1 -Action update
```

The Beta plugin is installed at:

```text
%CommonProgramFiles%/VST3/AIFRED Beta/Aifred.vst3
```

The Beta IntelligenceHost is installed at:

```text
%LOCALAPPDATA%/Aifred/beta/IntelligenceHost
```

To remove Beta binaries and startup entries while retaining user settings:

```powershell
pwsh -NoProfile -File scripts/windows/lifecycle.ps1 -Action uninstall
```

## Configure Ollama And Chat

Audio measurement, meters, Reference, and Compare do not require Ollama, a model, or a network connection. Only AI chat requires the IntelligenceHost and a provider.

On the packaged macOS path, Ollama and `aifred:latest` are configured automatically. On a development machine, the model definition is in `models/aifred/Modelfile`:

```bash
ollama create aifred:latest -f models/aifred/Modelfile
```

The host defaults are:

```text
Provider: ollama
Endpoint: http://127.0.0.1:11434
Model: aifred:latest
API key: empty
Beta host: http://127.0.0.1:8787
```

Chat is explicit. AIFRED sends the question and filtered measurement context only when **ASK AI** is pressed. The host validates the Beta channel and context contract before forwarding the request. Provider failures do not disable DSP measurement.

For the packaged macOS host, settings are stored at:

```text
~/Library/Application Support/Aifred/beta/IntelligenceHost/settings.json
```

For Windows, settings are stored at:

```text
%APPDATA%/Aifred/beta/IntelligenceHost/settings.json
```

Do not commit API keys or other credentials.

## Use The Plugin

1. Insert `Aifred.vst3` on the master bus or an analysis bus.
2. Route the main mix to Mix A.
3. For Compare, enable the wrapper sidechain input and route the comparison signal to Mix B.
4. Choose an analysis mode: Analyze, Reference, or Compare.
5. Select a DSP profile when needed.
6. Use **ASK AI** to request an interpretation of the current filtered context.

### Modes

- **Analyze** measures the live Mix A signal.
- **Reference** compares the live signal with a compatible local reference and can read availability from the Official reference pool.
- **Compare** measures Mix A and Mix B side by side. The delta is always A minus B.
- **Chat** sends a user question and the current filtered context to the configured provider through the local host.

## Build And Test

### macOS arm64

The complete DMG build is:

```bash
scripts/macos/package-dmg.sh
```

The script configures CMake, builds the VST3, publishes a self-contained `osx-arm64` IntelligenceHost, creates the installer package, and writes:

```text
out/macos-arm64/AIFRED-Beta-0.3.6-macos-arm64.dmg
```

Optional signing identities:

```bash
export AIFRED_CODESIGN_IDENTITY="Developer ID Application: ..."
export AIFRED_PKG_SIGNING_IDENTITY="Developer ID Installer: ..."
scripts/macos/package-dmg.sh
```

The script removes the previous DMG before starting a replacement build. Notarization and installed DAW validation remain release checks.

For a staged macOS build without creating the DMG:

```bash
scripts/macos/build.sh
```

### Windows

From the repository root:

```powershell
pwsh -NoProfile -File scripts/windows/build.ps1 -Action configure
pwsh -NoProfile -File scripts/windows/build.ps1 -Action test
pwsh -NoProfile -File scripts/windows/build.ps1 -Action release
```

The verified Windows release is promoted to `out/windows-x64/current/`.

### Tests and checks

The repository contains C++ DSP/plugin tests, IntelligenceHost contract tests, packaging tests, and Python release checks. The main Windows release action runs the supported validation set. Individual .NET host contract tests can be run with:

```bash
dotnet run --project tools/AifredIntelligenceHost.Tests/AifredIntelligenceHost.ContractTests.csproj -c Release
```

## Repository Folders

```text
config/                 Distribution settings examples and configuration notes
docs/                   Architecture, build, install, testing, and troubleshooting docs
models/                 Local Ollama model definitions and intelligence design notes
packages/               Package-level pointers and local runtime documentation
plugin-aifred/          JUCE VST3 plugin, editor, processor, and UI source
scripts/                Platform builds, installers, packaging, validation, and lifecycle tools
shared-dsp/             Shared DSP engine, filters, meters, pipeline, and IntelligenceClient
src/                    Standalone application entry point and shared executable source
tests/                  Plugin and contract tests
tools/                  IntelligenceHost, Windows installer, uninstaller, and helper tools
out/                    Generated builds, staging directories, packages, and release artifacts
```

### Important source areas

- `plugin-aifred/Source/PluginProcessor.*` owns the plugin processor and connects the editor to shared services.
- `plugin-aifred/Source/PluginEditor.*` draws the Beta UI and handles user interaction.
- `shared-dsp/src/Engine.cpp` and related files own realtime measurement.
- `shared-dsp/src/IntelligenceClient.cpp` selects the channel-local host. Beta resolves to port `8787`; Official resolves to port `8788`.
- `tools/AifredIntelligenceHost/` owns HTTP transport, provider routing, settings, health checks, and channel validation.
- `models/aifred/Modelfile` defines the local Ollama model used by the default provider.
- `scripts/channel-contract.json` and `scripts/release-layout.json` define release channel, port, context, and packaging contracts.

## Generated macOS Package Folders

During `scripts/macos/package-dmg.sh`, the following temporary folders are created under `out/macos-arm64/package/`:

```text
package/
|-- dmg/          Files shown when the DMG is opened
|-- host/         Temporary self-contained IntelligenceHost publish output
|-- payload/      Files installed into the target filesystem by pkgbuild
|-- scripts/      Installer scripts, including postinstall
`-- expanded-pkg/ Expanded package used to remove unwanted metadata before flattening
```

The final `dmg/` folder contains:

```text
AIFRED Beta Installer.pkg       Installs the plugin, host, LaunchAgent template, and postinstall
Install AIFRED Beta.command     Runs the package installer with administrator permission
README.txt                      Short instructions displayed with the DMG contents
```

The package payload contains:

```text
Library/Audio/Plug-Ins/VST3/Aifred.vst3
Library/Application Support/Aifred/beta/IntelligenceHost/bin/AifredIntelligenceHost
Library/Application Support/Aifred/beta/IntelligenceHost/bin/channel.json
Library/Application Support/Aifred/beta/IntelligenceHost/com.north3rnlight3r.aifred-intelligence-host.plist
```

The generated `out/` tree is build output and should not be committed. The package staging folders can be deleted and recreated safely.

## Troubleshooting

### The plugin says the IntelligenceHost is unavailable

Check the Beta health endpoint:

```bash
curl http://127.0.0.1:8787/health
```

If it fails, restart the LaunchAgent or inspect `/tmp/aifred-intelligence-host.error.log`. Confirm that another process is not occupying port `8787`.

### Ollama opens but chat is unavailable

Check the Ollama API and installed model:

```bash
curl http://127.0.0.1:11434/api/tags
ollama list
```

The model must be named `aifred:latest`, or the host settings must be changed to the installed model name.

### The plugin does not appear in the DAW

Close and reopen the DAW, rescan its VST3 locations, and confirm that the plugin exists in the platform-specific install path. On macOS, verify that the DAW is running natively or that it supports the installed Apple Silicon architecture.

### Beta and Official are installed together

Keep Beta on port `8787` and Official on port `8788`. Do not replace one channel's host or settings directory with the other. See [docs/COEXISTENCE.md](docs/COEXISTENCE.md).

## Documentation

- [Architecture](docs/ARCHITECTURE.md)
- [Build](docs/BUILD.md)
- [Installation](docs/INSTALLATION.md)
- [Distribution](docs/DISTRIBUTION.md)
- [Testing](docs/TESTING.md)
- [Troubleshooting](docs/TROUBLESHOOTING.md)
- [User guide](docs/USER_GUIDE.md)
- [Coexistence](docs/COEXISTENCE.md)
- [Release notes](docs/RELEASE_NOTES.md)

This repository is public and free to use for private, non-commercial purposes only. All code and associated intellectual property are owned by North3rnLight3r. Public access does not grant permission to copy, redistribute, resell, sublicense, incorporate into commercial products or services, or otherwise use the code for commercial purposes without prior written permission from the owner.
