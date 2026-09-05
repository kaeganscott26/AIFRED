# AIFRED Beta

CURRENT: 0.3.6 / JUCE 8.0.14. Operational Beta: VST3 analysis, local .NET companion, Cloudflare website/API and private admin clients.

The repositories remain separate. AIFRED helps producers interpret measured audio; it does not automatically mix a session. Windows x64 is the construction validation target. macOS has an existing Beta package route, not validated in this pass. Linux (Arch/Debian/Ubuntu) is SCAFFOLDED / NOT VALIDATED. No complete Linux release is claimed.

Build/test/release from this repository:

```powershell
pwsh -NoProfile -File scripts/windows/build.ps1 -Action release
```

Prerequisites and configure/build/test actions: [BUILD](docs/BUILD.md). Output: out/windows-x64/current. Installation is separate: [INSTALLATION](docs/INSTALLATION.md). Read [DISTRIBUTION](docs/DISTRIBUTION.md) before packaging or replacing artifacts and [COEXISTENCE](docs/COEXISTENCE.md) before installing either channel. Both currently use the shared Aifred.vst3 slot and gateway port 8787.

[ARCHITECTURE](docs/ARCHITECTURE.md) maps current folders/runtime ownership. [DEVELOPMENT](docs/DEVELOPMENT.md) explains configuration and contribution boundaries. [TESTING](docs/TESTING.md) lists actual tests and release gates. [Documentation index](docs/README.md) links specialized component contracts.

PLANNED / UNIMPLEMENTED: shared aifred_engine -> BufferHunter -> aifred_filter, selectable DSP profiles and matching controls. Existing DSP/model/GUI behavior remains unchanged. Official owns the new shared design; Beta will integrate a versioned adapter, never an absolute sibling source tree. Local default model routing remains aifred:latest; compatible provider configuration includes gpt-5.6-luna where configured. Future LLM/context tools begin only after analyzer validation.
