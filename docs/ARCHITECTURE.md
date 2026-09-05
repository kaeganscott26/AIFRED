# Current Beta architecture

CURRENT: operational AIFRED VST3 0.3.6, JUCE 8.0.14, with .NET AifredEngine 1.0.0, website/API and private admin tools. Official is a separate flagship repository. Beta is reference behavior for the future shared design; do not copy its DSP into that engine.

DAW -> plugin-aifred/Source/PluginProcessor -> AnalysisEngine -> HaloState -> PluginEditor. Existing DiagnosticInterpreter and local /analyze threshold behavior remain unchanged. A question passes supplied analysis context to the loopback AifredEngine, which routes to Ollama or a configured compatible provider. The plugin does not require Cloudflare for its local path.

| Folder/component | Purpose/status | Owner/runtime | Build/output |
|---|---|---|---|
| plugin-aifred/Source | CURRENT processor, DSP, Halo, interpreter, GUI, local-engine client | JUCE C++ | CMake Aifred_VST3 -> out/<platform>/build/plugin-aifred/Aifred_artefacts/Release/VST3/Aifred.vst3 |
| tools/AifredEngine | CURRENT loopback health/analyze/chat/settings/restart | .NET 10 | Windows/macOS companion in staged product |
| tools/AifredWindowsInstaller, Uninstaller | CURRENT Windows packaging/install programs | .NET | out/windows-x64/current installer/uninstaller components |
| tools/windows, tools/macos, tools/linux | CURRENT platform AI setup/control; Linux full product unsupported | Platform scripts | Installed config/services on explicit invocation |
| apps/website | CURRENT static frontend, Pages _worker/Functions/API, /ops | Browser/Cloudflare | Source deployment root; separate website validation |
| apps/admin-android | CURRENT owner-only Compose app and Windows desktop scripts | Android/PowerShell | Gradle APK; scripts; no public admin release |
| apps/admin-desktop | CURRENT macOS AppKit/WebKit shell, native archive commands | Swift | out/macos-arm64/build/admin/AIFRED Admin.app |
| infra/cloudflare | CURRENT operational mirror, not deployment authority | Wrangler reference | No independent deployment |
| integrations/forge | CURRENT credential-free discovery, sanitized export bridge/schemas | Node and external FORGE | Ignored bounded exports/restore workspace |
| tools/lib, tools/aifred-archive.mjs | CURRENT local archive implementation/CLI | Node, desktop-invoked | runtime/aifred-archive user records; not build junk |
| config | CURRENT admin command/settings metadata | Admin/API/developer tooling | Generated command registry/docs |
| models | CURRENT model definitions/assets | Installed local provider setup | User/provider model storage, not generated build output |
| packages/plugin-juce, packages/local-engine | CURRENT source-path pointers | Documentation only | No second implementation |
| api, include, lib, assets, ops | Tracked support/compatibility/assets/operational data | Inspect consumers before removal | Not disposable build directories |
| tests | CURRENT API and archive regression tests | Node | Test results |
| tools/release, scripts | CURRENT repository validation, platform build/package tools | Developer tools | out/<platform>/build, stage, current |
| out | GENERATED canonical output | Build tooling | Platform-specific products |
| .obsidian, .vscode, workspace_memory | Developer/editor material; not runtime architecture authority | Local tools | Preserve functional personal settings |
| shared-dsp | PLANNED frontend adapter contract to Official-owned future library | No runtime | No linked target |

Cloud authority: apps/website/wrangler.toml and apps/website/functions/api/v1/[[path]].js. Root wrangler.jsonc is convenience; infra is a mirror. Public downloads use controlled R2 routes; historical PayPal routes are not current payment behavior. Local and cloud APIs have distinct owners; see API_REFERENCE.md and CLOUDFLARE_PRODUCTION.md.

FORGE receives bounded sanitized operational exports. The repository does not contain FORGE's ToolRouter. Local cold-storage archives are functional user data, distinct from forbidden source-code archive directories. See FORGE_INTEGRATION.md and ARCHIVE_GUIDE.md.

Reference behavior: Beta can analyze selected local references. A future versioned profile-pack synchronization scheme remains PLANNED; private audio and user-identifying metadata must not become public profile payloads. The new shared analyzer/BufferHunter/aifred_filter is UNIMPLEMENTED here. Official owns its construction guide; the future adapter must use an explicit versioned dependency rather than an absolute sibling path.

Current symbols and source files outrank historical function maps or old phase claims. [Build](BUILD.md), [distribution](DISTRIBUTION.md) and [coexistence](COEXISTENCE.md) own their respective operational contracts.
