# AIFRED System Map

| System | Current authority | Runtime |
| --- | --- | --- |
| VST3 | `plugin-aifred/` | DAW process; local analysis |
| Local model gateway | `tools/AifredEngine/` | Loopback `127.0.0.1:8787` |
| Website/API/`/ops` | `apps/website/` | Cloudflare Pages project `aifred-site` |
| Android Admin | `apps/admin-android/` | Private owner device |
| Windows Desktop | `apps/admin-android/tools/windows-admin/` | WinForms + archive CLI |
| macOS Desktop | `apps/admin-desktop/macos/` | AppKit/WebKit + archive CLI |
| FORGE bridge | `integrations/forge/` | Bounded mirror and archive discovery |
| Cold storage | `tools/lib/aifred-archive.mjs` | Ignored `runtime/aifred-archive/` |

```text
VST -> AifredEngine -> Ollama/OpenAI-compatible provider
Admin clients -> Cloudflare API -> KV/R2
FORGE current mirror -> verified archive -> small manifest pointer
```

The local engine and Cloudflare API have different routes, security and availability. Do not merge them.
