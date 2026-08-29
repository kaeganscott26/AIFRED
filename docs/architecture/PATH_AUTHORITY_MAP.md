# Path Authority Map

Final canonical paths:

| Path | Authority status |
| --- | --- |
| `apps/website` | Canonical website and Cloudflare backend source. |
| `apps/admin-android` | Canonical Android admin app source. |
| `plugin-aifred` | Canonical JUCE VST3 plugin source. |
| `tools/AifredEngine` | Canonical local engine source. |
| `infra/cloudflare` | Cloudflare support docs and config. |
| `apps/admin-desktop` | Desktop Admin shared docs and macOS source; generated apps are local-only. |
| `integrations/forge` | FORGE discovery, schemas and export bridge. |
| `config/admin-commands.json` | Structured administrator command metadata authority. |
| `runtime/aifred-archive` | Ignored local archive root; never tracked. |
| `packages/plugin-juce` | Placeholder reference wrapper only. |
| `packages/local-engine` | Placeholder reference wrapper only. |

## Warning

The old duplicate `website/` and `android_admin/` trees are not active authorities and should not be restored. GitHub repository deletion remains a separate reviewed action.
