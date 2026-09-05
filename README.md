# AIFRED Beta 0.3.6

Independently buildable Windows x64 VST3, using **aifred_engine → EngineSnapshot → BufferHunter → ObservationSnapshot → aifred_filter → FilteredMixContext → AifredIntelligenceHost → LLM**. Shared core 1.1.0 is vendored and checksum-verified. DSP works without a provider.

The existing frontend exposes MIX_BALANCED, SPECTRUM_SURGICAL, MASTERING_PRECISION and STEREO_PHASE_DIAGNOSTIC, persisted in plugin state. Full-resolution FFT remains authoritative; telemetry includes 850 Hz. Live correlation/width use continuous engine values. Other engineering meters use unrounded observation values; text/model rounding is separate. Spectrum display range is -24..0 dB only at rendering.

## Build and test

Prerequisites: VS 2022 C++ x64/Windows SDK, CMake, Ninja, PowerShell 7, Python 3, .NET 10 SDK. Beta website checks also require Node/npm. Configure downloads pinned JUCE dependencies.

```powershell
pwsh -NoProfile -File scripts/windows/build.ps1 -Action configure
pwsh -NoProfile -File scripts/windows/build.ps1 -Action test
pwsh -NoProfile -File scripts/windows/build.ps1 -Action release
```

Compiler output is incremental `out/windows-x64/build`. Release builds/tests, assembles `stage`, verifies exact hashes and promotes **out/windows-x64/current**. Failed candidates retain previous current. Successful promotion recycles superseded current; versioned junk folders are not normal output.

Exact current VST3: `out/windows-x64/current/AIFRED-VST3-windows/Aifred.vst3`; binary inside: `Contents/x86_64-win/Aifred.vst3`. Host: `out/windows-x64/current/AIFRED-VST3-windows/AifredIntelligenceHost`. manifest.json records source identity, version, DSP/profile schemas and inventory.

## Install, uninstall and update

Close the DAW and use elevated PowerShell 7 for installation ownership:

```powershell
pwsh -NoProfile -File scripts/windows/install.ps1
pwsh -NoProfile -File scripts/windows/start-host.ps1
pwsh -NoProfile -File scripts/windows/uninstall.ps1
pwsh -NoProfile -File scripts/windows/lifecycle.ps1 -Action update
```

Update rebuilds/tests/promotes then installs current. Install verifies copied hashes and registers the channel host at login. Uninstall removes only channel binaries/startup, retaining settings. Host requires .NET 10 runtime and a configured available Ollama/OpenAI-compatible provider; model weights are not bundled. Port: **8787**. Settings: `%APPDATA%/Aifred/beta/IntelligenceHost`. Binaries/logs: `%LOCALAPPDATA%/Aifred/beta`. VST3: `CommonProgramFiles/VST3/AIFRED Beta/Aifred.vst3`.

Safe Git update, starting clean:

```powershell
git switch main
git pull --ff-only origin main
pwsh -NoProfile -File scripts/windows/build.ps1 -Action release
```

Install new current separately when ready. Never overwrite dirty work or force-update history.

## Validation and limitations

Native/module-load, DSP/context, runtime and release checks are automated. Manual FL Studio/Waves/SPAN/FabFilter/Ozone comparisons remain required. Compilation is not DAW validation; full ITU/EBU conformance material has not been validated. macOS/Linux are **SCAFFOLDED / NOT VALIDATED**.

Existing global-slot installations need explicit migration: [coexistence](docs/COEXISTENCE.md). Compatibility IDs are preserved; old global files/settings are not silently deleted. Official catalog records without matching DSP definitions remain metadata, with comparison unavailable. Beta local references are measured by the same core. Future profiles, personality files and DAW/MCP/long-term-memory tooling are unimplemented.

[Architecture](docs/ARCHITECTURE.md) · [DSP contracts](shared-dsp/README.md) · [Build](docs/BUILD.md) · [Testing](docs/TESTING.md) · [Install](docs/INSTALLATION.md) · [Distribution](docs/DISTRIBUTION.md) · [Development](docs/DEVELOPMENT.md)
