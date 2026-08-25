# Final Monorepo Consolidation Report

Date: 2026-06-28

## 2026-08-25 Production Correction

The path consolidation remains valid, but its external-infrastructure assumptions are superseded. Production is now verified on Pages project `aifred-site` from `apps/website`. `AIFRED_DOWNLOADS` (`aifred-downloads`) stores both versioned release packages and `assets/`; the proposed `AIFRED_WEBSITE_ASSETS` binding/bucket does not exist and is not required. PayPal and `MAILER` are removed from the active distribution contract. See `website-cloudflare-production-2026-08-25.md` for evidence.

## Active Authorities

| System | Active path |
| --- | --- |
| VST plugin | `plugin-aifred/` |
| Local engine | `tools/AifredEngine/` |
| Website and Cloudflare backend | `apps/website/` |
| Android admin app | `apps/admin-android/` |
| Cloudflare support config | `infra/cloudflare/` |

## Backend Routing

- The VST talks to the local AIFRED engine at `http://127.0.0.1:8787`.
- The local engine talks to Ollama at `http://127.0.0.1:11434` with `aifred:latest`.
- The local engine supports OpenAI through `https://api.openai.com/v1/responses` when an API key is configured.
- The website/admin backend lives under `/api/v1` and `/ws/chat` on Cloudflare Pages.
- The website/backend defaults to `kaeganscott26/AIFRED` for GitHub-backed admin file operations.

## Website Assets

- Catalog stream URLs now route through `/api/v1/assets/audio/catalog/<file>`.
- The Worker first reads those objects from the `AIFRED_DOWNLOADS` R2 binding.
- Local static files remain as a development fallback when the R2 binding or object is not available.
- Release downloads continue to use `AIFRED_DOWNLOADS`.
- Reference-pool storage continues to use `AIFRED_REFERENCE_BUCKET`.

## Local Cleanup

- Removed the old duplicate `website/` tree from this repo.
- Removed the old duplicate `android_admin/` tree from this repo.
- Removed the raw `North3rnlight3r_Beatz/` stash from this repo.
- Kept the canonical app, plugin, engine, and website sources in the single AIFRED monorepo.

## GitHub Repository Cleanup

No GitHub repositories were deleted by this consolidation pass.

Potential stale external repositories to review before deletion or archive:

- `kaeganscott26/aifred-site`
- `kaeganscott26/aifred-admin`
- `kaeganscott26/aifred-plugin`
- `kaeganscott26/aifred-downloads`
- `kaeganscott26/AIFRED_Official-`

Deletion is intentionally not automated here because it is destructive and requires an explicit final repository-name list. Review repository settings, secrets, release artifacts, and branch protections before deletion.

## Remaining External Requirements

- Catalog audio objects are verified in `aifred-downloads` under `assets/audio/catalog/<file>`.
- Existing Cloudflare secrets must be reviewed and rotated outside Git if any old sibling repo docs exposed values.
- Stale GitHub repos should be archived or deleted only after explicit owner confirmation.
