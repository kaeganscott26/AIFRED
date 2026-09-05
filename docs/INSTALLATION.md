# Beta installation

Use [README commands](../README.md). Install/uninstall needs elevation for CommonProgramFiles, closed DAW, PowerShell 7 and Python. Host needs .NET 10 runtime. Verified current files are copied/hash-checked before replacement. Retained .candidate/.previous directories block updates for inspection. Successful replacement recycles old binaries.

Owned VST3: CommonProgramFiles/VST3/AIFRED Beta/Aifred.vst3. Host: %LOCALAPPDATA%/Aifred/beta/IntelligenceHost. HKCU Run: AIFRED Beta Intelligence Host. Settings: %APPDATA%/Aifred/beta/IntelligenceHost/settings.json. Explicit startup script writes channel logs under LocalAppData/Aifred/beta/logs, overwritten each launch.

Uninstall removes only channel binaries/startup; settings, references, other channels and provider data remain. Public settings hide credentials. Compatible prior provider settings are read without deleting originals. Configure provider through existing settings UI; no model is implicitly downloaded.

Read [coexistence migration](COEXISTENCE.md) for global-slot installs. Installed/DAW validation remains manual. lifecycle.ps1 -Action update releases then installs. Installed rollback is not automatic; inspect recovery paths before recovery.
