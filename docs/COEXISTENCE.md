# Channel coexistence

CURRENT: both channels retain their existing plugin identities. The products do not yet have a verified side-by-side installation contract. Installing one into the shared slot can replace the other. No IDs change in this construction pass.

| Identity / owner | Beta | Official |
|---|---|---|
| Product name / bundle filename | Aifred / Aifred.vst3 | Aifred / Aifred.vst3 |
| Manufacturer name | North3rnLight3r | AIFRED |
| Manufacturer code | N3Lr | Aifr |
| Plugin code | Aifr | Af40 |
| Bundle ID | com.aifred.plugin | com.aifred.audio.aifred |
| Version / channel | 0.3.6 / beta | 4.0.0-alpha.2 / alpha |
| Native target | Aifred_VST3 | Aifred_VST3 |
| Windows plugin slot | %CommonProgramFiles%/VST3/Aifred.vst3 | Same slot: collision |
| Windows engine install | %ProgramFiles%/Aifred/bin | %LOCALAPPDATA%/Aifred/bin |
| Loopback gateway | 127.0.0.1:8787 | Same port: collision |
| Default provider | Ollama 127.0.0.1:11434, aifred:latest | Same provider/model alias |
| Windows settings | %APPDATA%/Aifred/user_settings.json | %APPDATA%/Aifred/Engine/settings.json |
| Logs | install root/logs/engine.log, application-data fallback | %APPDATA%/Aifred/Engine/engine.log |
| macOS plugin | /Library/Audio/Plug-Ins/VST3/Aifred.vst3 | No supported installer yet; reserve separate channel slot |
| macOS engine/data | /Library/Application Support/Aifred; user Library/Application Support/Aifred | Windows-targeted companion; unavailable on macOS |
| Native state | APVTS AIFRED_PARAMETERS; session_initialized parameter; AIFRED_STATE XML version 2 (mode/theme/layout/genre/gate/provider/endpoint/model/session flag) | getStateInformation clears output; restore currently does nothing |
| Reference ownership | Plugin local reference analysis; current Halo/reference state | Read-only reference catalog and editor selection; process-wide ReferenceClient |
| Compare ownership | Beta processor/comparison state | Editor-owned A/B snapshot captures |
| Client scope | Current Beta client implementation | Process-wide AifredEngineClient singleton; reply/in-flight state is shared |
| Python data | No native Python runtime | Offline experiments use AIFRED_HOME or ~/.aifred, Reports subfolder |
| Build cache | Own repository out/<platform>/build | Own repository out/<platform>/build |

PLANNED: visibly separate names AIFRED Beta and AIFRED 4, channel-owned bundle/install slots, settings/log/cache roots, and instance/session IDs. Preserve current manufacturer/plugin codes until a reviewed host migration proves old sessions still load. Do not fix coexistence by renaming an inner VST3 binary alone.

The future handshake must identify product, channel, version, instance, session and protocol. A successful /health from another product is not sufficient. A shared daemon, if selected later, must namespace requests, reference state, history, responses and cancellation by instance. Sharing the Ollama service is optional; sharing mutable analysis state is forbidden.

Before enabling automatic installation/update/uninstall, test both plugins in one DAW session, saved-session reload, instance isolation, engine endpoint selection, plugin scanning and removal of only the requested channel. Installation into today's shared slot requires an explicit channel-replacement decision. User settings, references, model files and reports survive build cleanup and uninstall by default.

The proposed identity/ownership fields live in scripts/channel-contract.json. This file documents future interfaces; current runtime code does not consume it.
