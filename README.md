# AIFRED Beta

> **Public proving ground for the AIFRED VST3 measurement, observation, filtering, and release pipeline.**

AIFRED Beta is the public Windows x64 VST3 channel. It measures the audio actually playing in the DAW, keeps high-frequency measurement separate from sustained observation, and exposes deterministic mix context without modifying the audio stream.

Current source identity: **0.3.6 Beta** · shared core **1.2.0** · profile schema **2** · Windows channel host **8787**.

```text
DAW audio
  -> aifred_engine
  -> EngineSnapshot
  -> BufferHunter
  -> ObservationSnapshot
  -> aifred_filter
  -> FilteredMixContext
  -> AifredIntelligenceHost transport
```

The solid pipeline above exists. Future flagship intelligence work is developed privately and is not a promise of capability in this public Beta.

---

# START HERE

If you only open a few files, use these doorways:

- **[Architecture](docs/ARCHITECTURE.md)** — runtime ownership and realtime boundaries.
- **[Shared DSP contract](shared-dsp/README.md)** — measurement algorithms, profiles, snapshots, observation, and filtering.
- **[DSP configuration](docs/DSP_CONFIGURATION.md)** — profile-selected measurement/observation/presentation policy.
- **[Build](docs/BUILD.md)** — exact Windows build output and prerequisites.
- **[Testing](docs/TESTING.md)** — what automation proves and what still requires manual validation.
- **[Installation](docs/INSTALLATION.md)** — owned Beta install/runtime locations.
- **[Repository map](docs/REPOSITORY_MAP.md)** — how the public tree fits together.

---

# Main Doorways

## I want to understand the audio machine

1. [Architecture](docs/ARCHITECTURE.md)
2. [Shared DSP](shared-dsp/README.md)
3. [BufferHunter](docs/BUFFER_HUNTER.md)
4. [AIFRED Filter](docs/AIFRED_FILTER.md)
5. [DSP Configuration](docs/DSP_CONFIGURATION.md)

## I want to build or test it

1. [Build](docs/BUILD.md)
2. [Testing](docs/TESTING.md)
3. [Windows scripts](scripts/windows/)
4. [Tests](tests/)
5. [Distribution](docs/DISTRIBUTION.md)

## I want to inspect the plugin frontend

1. [Beta frontend](plugin-aifred/README.md)
2. [Plugin source](plugin-aifred/Source/)
3. [Plugin assets](plugin-aifred/Assets/)
4. [Frontend contract tests](tests/frontend_contract_tests.cpp)
5. [State contract tests](tests/state_contract_tests.cpp)

## I want to install or update the Beta

1. [Installation](docs/INSTALLATION.md)
2. [Coexistence](docs/COEXISTENCE.md)
3. [Distribution](docs/DISTRIBUTION.md)
4. [Changelog](CHANGELOG.md)

## I want to contribute without breaking the architecture

1. [Development](docs/DEVELOPMENT.md)
2. [Architecture](docs/ARCHITECTURE.md)
3. [Repository Construction](docs/REPOSITORY_CONSTRUCTION.md)
4. [Testing](docs/TESTING.md)
5. [Future / non-implemented work](docs/FUTURE.md)

---

# What the Beta measures

The shared engine currently covers sample peak, RMS, true peak, momentary/short-term/integrated loudness, LRA, crest factor, correlation, L/R/M/S energy, balance, side-to-mid behavior, stereo width, and full-resolution FFT spectrum data. BufferHunter turns valid engine publications into bounded sustained observations with latest/median/P10/P90/min/max, coverage, count, and trend where applicable.

The frontend exposes four analysis profiles:

- `MIX_BALANCED`
- `SPECTRUM_SURGICAL`
- `MASTERING_PRECISION`
- `STEREO_PHASE_DIAGNOSTIC`

Full-resolution FFT power remains authoritative. The 30-band telemetry view is derived downstream. Presentation floors (`-120`, `-96`, `-72`, `-48 dBFS`) change the viewport only; they do not clip or redefine measured FFT power.

---

# Build the current Beta

Prerequisites: Visual Studio 2022 C++ x64 + Windows SDK, CMake, Ninja, PowerShell 7, Python 3, and .NET 10 SDK/runtime.

```powershell
pwsh -NoProfile -File scripts/windows/build.ps1 -Action configure
pwsh -NoProfile -File scripts/windows/build.ps1 -Action test
pwsh -NoProfile -File scripts/windows/build.ps1 -Action release
```

Canonical output:

```text
out/windows-x64/
  build/      compiler scratch
  stage/      verified candidate
  current/    promoted artifact
```

Current Beta bundle:

```text
out/windows-x64/current/AIFRED-VST3-windows/
  Aifred.vst3
  AifredIntelligenceHost/
```

Release/package inventory also includes the Windows ZIP, installer, uninstaller, and manifest when the release action succeeds. Generated output is not source and should not be committed.

---

# Install / update

Close the DAW and use elevated PowerShell 7 for installation ownership:

```powershell
pwsh -NoProfile -File scripts/windows/install.ps1
pwsh -NoProfile -File scripts/windows/start-host.ps1
```

Normal source update:

```powershell
git switch main
git pull --ff-only origin main
pwsh -NoProfile -File scripts/windows/lifecycle.ps1 -Action update
```

Owned Beta locations:

```text
VST3     %CommonProgramFiles%\VST3\AIFRED Beta\Aifred.vst3
Host     %LOCALAPPDATA%\Aifred\beta\IntelligenceHost
Settings %APPDATA%\Aifred\beta\IntelligenceHost\settings.json
Port     8787
```

See [Coexistence](docs/COEXISTENCE.md) before touching older global-slot installs.

---

# Validation boundary

Automated tests cover native DSP/contracts, profiles, BufferHunter/filter/pipeline behavior, frontend/state behavior, release safety, host transport, shared-core parity, and an independent FFmpeg EBU R128 fixture.

That does **not** mean every professional validation gate is complete. Manual FL Studio validation, proprietary-meter comparisons, the broader EBU/ITU test material, realtime CPU profiling, macOS validation, and Linux validation remain separate work. See [Testing](docs/TESTING.md).

A build that compiles is not automatically a DAW-validated release.

---

# Public / private boundary

This repository is the **public Beta and public Beta source**. Owner-only administration, production control-plane tooling, private operational credentials, and flagship intelligence development belong outside the public Beta surface.

No secret value belongs in source, examples, workflow YAML, APK source, or documentation. GitHub/Cloudflare workflows reference secret **names** only; values remain in their respective secret stores/runtime configuration.

---

# Repository Map

```text
AIFRED/
├── plugin-aifred/       JUCE Beta VST3 frontend + processor adapter
├── shared-dsp/          authoritative shared measurement/observation/filter core
├── tests/               native and repository contract tests
├── scripts/             build/release/install/shared-core tooling
├── tools/               validation/release utilities
├── docs/                canonical public documentation
├── apps/website/        public Beta website/runtime surface
├── infra/cloudflare/    public website deployment/storage configuration
├── integrations/        integration contracts/adapters that remain public
├── packages/            supporting public packages
├── models/              public model/configuration material where applicable
├── CMakeLists.txt        root native build entry
├── CMakePresets.json    canonical CMake presets
└── shared-core.lock.json pinned shared-core inventory
```

For the detailed ownership and navigation map, use **[docs/REPOSITORY_MAP.md](docs/REPOSITORY_MAP.md)**.

---

# Repo Loop

```text
README
  ↓
Architecture
  ↓
Shared DSP
  ↓
Build / Tests
  ↓
Installation / Distribution
  ↓
Development rules
  ↓
README
```

If a document contradicts current source, investigate the mismatch rather than inventing a third behavior. Current source + tests + canonical contracts are the authority.

---

# Documentation Index

**[Documentation Hub](docs/README.md)** · **[Architecture](docs/ARCHITECTURE.md)** · **[Repository Map](docs/REPOSITORY_MAP.md)** · **[Shared DSP](shared-dsp/README.md)** · **[DSP Configuration](docs/DSP_CONFIGURATION.md)** · **[BufferHunter](docs/BUFFER_HUNTER.md)** · **[Filter](docs/AIFRED_FILTER.md)** · **[Build](docs/BUILD.md)** · **[Testing](docs/TESTING.md)** · **[Installation](docs/INSTALLATION.md)** · **[Distribution](docs/DISTRIBUTION.md)** · **[Coexistence](docs/COEXISTENCE.md)** · **[Development](docs/DEVELOPMENT.md)** · **[Future](docs/FUTURE.md)**
