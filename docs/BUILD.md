# Beta build

Install Visual Studio 2022 Desktop development with C++ and a Windows SDK, CMake, Ninja, PowerShell 7, Python 3, the .NET 10 SDK/runtime, Git, and Node.js 22 or newer. Verify them with the commands in the [user guide](USER_GUIDE.md).

Canonical entry: `pwsh -NoProfile -File scripts/windows/build.ps1 -Action release`. Actions configure/build/test/stage/package/release share incremental `out/windows-x64/build`. Stage/package assemble and verify without promotion; release promotes after validation.

Exact compiler VST3: `out/windows-x64/build/plugin-aifred/Aifred_artefacts/Release/VST3/Aifred.vst3`. Exact current: `out/windows-x64/current/AIFRED-VST3-windows/Aifred.vst3`. Never select a recursive first-match artifact.

.NET outputs use Directory.Build.props under the canonical platform build root. Host publish includes executable, DLL, runtime configuration and channel.json. Building does not install or launch a plugin.

Use `scripts/windows/lifecycle.ps1 -Action update` for a first install or update, and `-Action uninstall` to remove only the Beta channel.

macOS arm64/Linux x64 presets and configure/build scripts are SCAFFOLDED / NOT VALIDATED. Their distribution/install/promotion workflows are unvalidated.
