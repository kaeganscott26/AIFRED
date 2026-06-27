# Path Authority Map

Phase 2 canonical paths:

| Path | Phase 2 authority status |
| --- | --- |
| `apps/website` | Canonical imported website/backend source from `aifred-site`. |
| `apps/admin-android` | Canonical imported Android admin source from `aifred-admin`. |
| `plugin-aifred` | Current canonical runtime plugin source until a later migration. |
| `tools/AifredEngine` | Current canonical runtime local engine source until a later migration. |
| `infra/cloudflare` | Imported Cloudflare docs/config/ops support from `aifred-site`. |
| `website` | Preserved old in-repo website copy. Not Phase 2 authority. |
| `android_admin` | Preserved old in-repo admin copy. Not Phase 2 authority. |
| `packages/plugin-juce` | Placeholder reference wrapper only. |
| `packages/local-engine` | Placeholder reference wrapper only. |

## Warning

Do not update build/deploy automation to use `apps/website` or `apps/admin-android` until Phase 3 explicitly updates workflows and smoke tests.

Phase 2 is validation-only hardening. It documents authority, confirms imports, and makes coexistence measurable without changing deployment behavior.
