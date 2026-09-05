# Installation and user data

CURRENT Windows artifacts come from out/windows-x64/current after release validation. Build/test/stage/release never install. Close the DAW before replacing a loaded bundle. Verify the manifest before use.

The existing .NET installer is out/windows-x64/current/installer/AIFRED-VST3-Setup.exe. It requests elevation, installs the shared VST3, engine, startup registration and model setup. Run it only after deliberately choosing Beta for the shared slot; the new install wrapper requires -ReplaceSharedSlot. The uninstaller remains out/windows-x64/current/uninstaller/AIFRED-Uninstall.exe.

Both products currently install Aifred.vst3 into %CommonProgramFiles%/VST3. Beta installs engine/config/models/logs under %ProgramFiles%/Aifred and user_settings.json under %APPDATA%/Aifred; logs can fall back to application data. This pass preserves current IDs and runtime settings formats. Consult [channel collisions](COEXISTENCE.md); shared-slot replacement is not side-by-side support.

Uninstall/update/rollback wrappers document their boundary and refuse automatic mutation until channel ownership is established. Beta retains its existing uninstaller, which predates channel isolation. Inspect its targets before using it on a machine with Official installed. User references, settings, models, reports and other-channel files must survive by default. Build cleanup never touches installed files or user data.

Beta macOS packaging uses /Library/Audio/Plug-Ins/VST3/Aifred.vst3, /Library/Application Support/Aifred and /Library/LaunchAgents/com.aifred.engine.plist. User overrides live below ~/Library/Application Support/Aifred. The existing postinstall/AI repair/control scripts remain owned by Beta. pkg signing, notarization, login restart and host behavior need Mac validation.

The gateway is 127.0.0.1:8787; Ollama is a separate service normally at 127.0.0.1:11434. A healthy port alone does not identify the correct channel. Do not auto-start arbitrary provider processes during a build. Installation and model downloads require a deliberate runtime operation.

Future lifecycle: a channel manifest records installed files and hashes; update validates product/channel and waits for host shutdown; uninstall removes only owned components; rollback restores a verified prior same-channel package. These are planned boundaries, not a new updater implementation.
