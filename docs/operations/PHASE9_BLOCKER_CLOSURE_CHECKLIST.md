# Phase 9 Blocker Closure Checklist

> Historical migration record captured before the 2026-08-25 production promotion. Preserve as evidence; current authority is `website-cloudflare-production-2026-08-25.md`.

This checklist is for human closure review before any future non-production preview. It does not authorize deployment, production promotion, merge to `main`, old-folder removal, plugin movement, engine movement, release publishing, secret edits, or media deletion.

## Asset Blocker

- [ ] Duplicated website media reviewed.
- [ ] Repo size impact accepted or rejected.
- [ ] R2/release-storage direction accepted.
- [ ] No Git LFS conversion before explicit approval.
- [ ] No media deletion before preview proof.

## Cloudflare Blocker

- [ ] Pages project verified.
- [ ] GitHub binding verified.
- [ ] Production branch verified.
- [ ] Current build root verified.
- [ ] Preview behavior verified.
- [ ] R2 bindings verified without exposing values.
- [ ] PayPal env vars verified without exposing values.
- [ ] Admin auth env vars verified without exposing values.
- [ ] Domain binding verified.
- [ ] Rollback path verified.

## Workflow Blocker

- [ ] `build.yml` still reviewed.
- [ ] Old `website/` deployment root understood.
- [ ] Tag release publishing behavior understood.
- [ ] Validation workflows remain manual-only.
- [ ] Preview workflow remains manual-only.
- [ ] No `push` or `pull_request` triggers added.

## Old Folder Blocker

- [ ] `website/` remains fallback.
- [ ] `android_admin/` remains fallback/reference.
- [ ] Removal delayed until later explicit phase.

## Plugin/Engine Blocker

- [ ] `plugin-aifred/` remains current runtime plugin.
- [ ] `tools/AifredEngine/` remains current local engine.
- [ ] No installer path migration attempted.
- [ ] No release path migration attempted.

## Final

- [ ] Human reviewer signs off before future preview.
