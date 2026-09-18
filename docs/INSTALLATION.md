# Beta installation

## Windows end users

Download the single-file `AIFRED-VST3-Setup.exe` from the validated Windows release, close the DAW, and run it. The installer requests administrator permission, installs the VST3 and self-contained IntelligenceHost, downloads Ollama when it is missing, starts Ollama, pulls `aifred:latest`, registers the host to run at user sign-in, and verifies the host on port `8787`. No .NET runtime, PowerShell, Python, or manual `ollama serve` command is required on the target machine. Internet access is required for the first Ollama download and model pull.

The installer does not embed repository scripts. Scripts remain developer and maintenance tools in `scripts/windows`.

## Installed ownership

Owned VST3: CommonProgramFiles/VST3/AIFRED Beta/Aifred.vst3. Host: %LOCALAPPDATA%/Aifred/beta/IntelligenceHost. HKCU Run: AIFRED Beta Intelligence Host. Settings: %APPDATA%/Aifred/beta/IntelligenceHost/settings.json. Explicit startup script writes channel logs under LocalAppData/Aifred/beta/logs, overwritten each launch.

The VST3 is installed at `%CommonProgramFiles%\VST3\AIFRED Beta\Aifred.vst3`, the host at `%LOCALAPPDATA%\Aifred\beta\IntelligenceHost`, Ollama uses its normal per-user installation, and the startup entry is `HKCU\Software\Microsoft\Windows\CurrentVersion\Run\AIFRED Beta Intelligence Host`. Host settings remain at `%APPDATA%\Aifred\beta\IntelligenceHost\settings.json`.

## Developer install or update

The developer path requires Visual Studio 2022 C++, Windows SDK, CMake, Ninja, PowerShell 7, Python 3, the .NET 10 SDK, and Git. For a first install or update:

```powershell
pwsh -NoProfile -File scripts/windows/lifecycle.ps1 -Action update
```

For uninstall:

```powershell
pwsh -NoProfile -File scripts/windows/lifecycle.ps1 -Action uninstall
```

The script also runs the same Ollama setup and model pull before starting the host. Uninstall removes only channel binaries/startup; settings, references, other channels and provider data remain. Public settings hide credentials. Compatible prior provider settings are read without deleting originals.

Read [coexistence migration](COEXISTENCE.md) for global-slot installs. Verify `http://127.0.0.1:8787/health` and rescan VST3 plugins in the DAW after setup. Installed rollback is not automatic; inspect recovery paths before recovery.
