# Beta installation

Install Visual Studio 2022 Desktop development with C++ and a Windows SDK, CMake, Ninja, PowerShell 7, Python 3, the .NET 10 SDK/runtime, Git, and Node.js 22 or newer. The [user guide](USER_GUIDE.md) provides verification commands. Install and uninstall need elevation for CommonProgramFiles and a closed DAW. Verified current files are copied and hash-checked before replacement. Retained `.candidate` or `.previous` directories block updates for inspection. Successful replacement recycles old binaries.

Owned VST3: CommonProgramFiles/VST3/AIFRED Beta/Aifred.vst3. Host: %LOCALAPPDATA%/Aifred/beta/IntelligenceHost. HKCU Run: AIFRED Beta Intelligence Host. Settings: %APPDATA%/Aifred/beta/IntelligenceHost/settings.json. Explicit startup script writes channel logs under LocalAppData/Aifred/beta/logs, overwritten each launch.

For a first install or update:

```powershell
pwsh -NoProfile -File scripts/windows/lifecycle.ps1 -Action update
```

For uninstall:

```powershell
pwsh -NoProfile -File scripts/windows/lifecycle.ps1 -Action uninstall
```

Uninstall removes only channel binaries/startup; settings, references, other channels and provider data remain. Public settings hide credentials. Compatible prior provider settings are read without deleting originals. Configure provider through existing settings UI; no model is implicitly downloaded.

Read [coexistence migration](COEXISTENCE.md) for global-slot installs. Installed/DAW validation remains manual. lifecycle.ps1 -Action update releases then installs. Installed rollback is not automatic; inspect recovery paths before recovery.
