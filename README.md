# AIFRED Beta

AIFRED Beta is the free public Windows VST3 channel for realtime mix measurement, Reference and Compare workflows, and user-triggered AI chat. Its audio measurements run locally. This repository owns the Beta plugin, AifredIntelligenceHost, installers, lifecycle scripts, tests, and GitHub releases.

The production website, API, downloads, telemetry, Ops, and administrator applications are owned by the private `kaeganscott26/AIFRED_Official-` repository. Beta is a client of the single public API at `https://north3rnlight3r.com/api/v1`; this repository contains no production server or website implementation.

## Requirements

The supported build host is Windows x64 with:

- Visual Studio 2022 Desktop development with C++, MSVC x64 tools, and a Windows 10 or 11 SDK
- CMake 3.24 or newer and Ninja
- PowerShell 7
- Python 3
- .NET 10 SDK/runtime
- Git

Node.js, Wrangler, Android tooling, and Cloudflare credentials are not required to build or release the Beta plugin.

## Build and test

From the repository root:

```powershell
pwsh -NoProfile -File scripts/windows/build.ps1 -Action configure
pwsh -NoProfile -File scripts/windows/build.ps1 -Action test
pwsh -NoProfile -File scripts/windows/build.ps1 -Action release
```

`release` builds and validates the VST3, IntelligenceHost, installer, uninstaller, and ZIP, then promotes the verified set to `out/windows-x64/current/`.

## Install, update, and uninstall

Close every DAW before changing installed files. Installation needs permission to write the system VST3 directory.

```powershell
# Install the already validated current build
pwsh -NoProfile -File scripts/windows/install.ps1

# Rebuild, validate, promote, and install an update
pwsh -NoProfile -File scripts/windows/lifecycle.ps1 -Action update

# Remove Beta binaries while retaining user settings
pwsh -NoProfile -File scripts/windows/lifecycle.ps1 -Action uninstall
```

Beta installs separately from AIFRED Official and uses IntelligenceHost port `8787`. See [installation](docs/INSTALLATION.md) and [coexistence](docs/COEXISTENCE.md) before resolving an older global Aifred installation.

## Local AI and Chat

Audio measurement never requires a model or network connection. For the default local AI path, install Ollama and create the AIFRED model:

```powershell
ollama create aifred:latest -f models/aifred/Modelfile
```

The installer configures the Beta IntelligenceHost to start automatically. To start the installed host manually:

```powershell
pwsh -NoProfile -File scripts/windows/start-host.ps1
```

Chat is user-triggered: AIFRED sends the question plus filtered measurement context only after **ASK AI** is pressed. The host defaults to local Ollama. A user may explicitly configure the OpenAI-compatible Official API; AIFRED normalizes its production base to `https://north3rnlight3r.com/api/v1` and requires the existing API token. Secrets stay in per-user host settings and must never be committed.

## Reference, Compare, and remote Analysis

- **Reference** analyzes user-selected audio locally. Opening Reference also reads metadata availability from the single Official reference pool at `GET /api/v1/reference/pool`. Browser-derived pool records are not reinterpreted as native plugin DSP.
- **Compare** measures the live Mix A and sidechain Mix B signals locally.
- **Chat/Analyze** is explicit and user-triggered. When the host is configured for the Official API, provider discovery and chat use `/api/v1/models` and `/api/v1/chat/completions` with the Beta channel contract. There are no periodic Cloudflare API calls.

## Releases and help

- Releases and validated installers: <https://github.com/kaeganscott26/AIFRED/releases>
- Architecture: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- Build details: [docs/BUILD.md](docs/BUILD.md)
- Installation: [docs/INSTALLATION.md](docs/INSTALLATION.md)
- Testing and validation limits: [docs/TESTING.md](docs/TESTING.md)
- Troubleshooting: [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md)
- Release notes: [docs/RELEASE_NOTES.md](docs/RELEASE_NOTES.md)

Automatic behavior is limited to local realtime measurement, bounded observation, UI refresh, and the installed local host startup entry. Reference-pool refresh happens when Reference is selected. Chat/provider calls, file selection, builds, installs, updates, uninstalls, and release publication require an explicit user or developer action.

## Repository layout

```text
plugin-aifred/                 Beta JUCE VST3 and UI
shared-dsp/                    aifred_engine, BufferHunter, filter, client contract
tools/AifredIntelligenceHost/  channel-local provider transport
tools/AifredWindowsInstaller/  Windows installer source
tools/AifredWindowsUninstaller/ Windows uninstaller source
scripts/                       build, validation, packaging, install lifecycle
config/distribution/           packaged user configuration template
models/aifred/                 optional local Ollama model definition
tests/                         plugin/client contract tests
docs/                          current Beta documentation
```

Generated artifacts belong under `out/` and remain untracked. The source-of-truth DSP rule is unchanged: measurements come from `aifred_engine`, observation from BufferHunter, and interpretation begins in `aifred_filter` or later.


THIS REPO IS PUBLIC AND IS OPEN-SOURCE and FREE to use for private purposes only.
ALL CODE IS OWNED BY NORTH3RNLIGHTR and IN NO WAY GRANT PERMISSION TO USE or COPY CODE for profitable gain, commercial, or otherwise in any way that 
the codebase within this repository be used against the intent of the OWNER and Sole Proprieter of North3rnLight3rLLC and all Intellectual Property held within the NORTH3RNLIGHT3R estate
