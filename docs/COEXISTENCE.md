# Beta and Official coexistence

| Ownership | Beta | Official |
|---|---|---|
| VST3 install | VST3/AIFRED Beta/Aifred.vst3 | VST3/AIFRED Official/Aifred.vst3 |
| Host port | 8787 | 8788 |
| Runtime channel | beta | official |
| Binary root | LocalAppData/Aifred/beta | LocalAppData/Aifred/official |
| Host settings | AppData/Aifred/beta/IntelligenceHost | AppData/Aifred/official/IntelligenceHost |
| Startup entry | AIFRED Beta Intelligence Host | AIFRED Official Intelligence Host |
| Manufacturer / code | N3Lr / Aifr | Aifr / Af40 |
| Bundle ID | com.aifred.plugin | com.aifred.audio.aifred |

Compatibility IDs and inner filenames are preserved. Some DAWs still report both historical product names as Aifred; distinct class IDs/channel directories separate them. Frontends identify their channel. Requests carry channel/version/instance/session/profile/revision/schema. Hosts reject cross-channel requests; histories live per processor, never merged in the host.

Existing global VST3/Aifred.vst3 and old global host/startup files are not automatically deleted because ownership can be ambiguous. Before using new installs: close DAW, identify the old product, preserve state, remove its old bundle from scanning and disable its startup entry through its known owner. An old host on 8787 prevents Beta binding; the new client rejects that host. Do not run a broad old uninstaller across both products. This migration and simultaneous DAW validation remain manual blockers; plugin IDs are not changed to hide them.

Updates/uninstalls stop only the executable at the channel's exact installed path. Provider process/model weights may be shared, but context/instance/session state is separate.
