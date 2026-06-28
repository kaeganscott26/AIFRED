# Phase 10 Go/No-Go Criteria

These criteria apply only to a future human-approved non-production preview. They do not approve production deployment, merge to `main`, release publishing, plugin movement, engine movement, old-folder removal, media deletion, Git LFS conversion, or secret changes.

## GO Criteria For Future Non-Production Preview

- [ ] All Phase 9 review fields completed by a human.
- [ ] Asset blocker accepted or explicitly deferred.
- [ ] Cloudflare binding verified.
- [ ] Preview behavior verified.
- [ ] Rollback path verified.
- [ ] Manual workflows pass.
- [ ] No secrets written to Git.
- [ ] No production path switch required.
- [ ] `apps/website/` remains preview-only.

## NO-GO Criteria

- [ ] Unclear Cloudflare project binding.
- [ ] Asset strategy rejected or unresolved without deferral.
- [ ] Preview would require production domain change.
- [ ] Preview would require secrets in Git.
- [ ] Preview workflow includes `push` or `pull_request` triggers.
- [ ] Preview workflow includes deploy, release, or artifact commands.
- [ ] Old `website/` fallback unavailable.
- [ ] `build.yml` production behavior unclear.
- [ ] Release workflow behavior unclear.
- [ ] Any validation report stale or failing.

## Final Statement

A GO decision allows only future non-production preview planning/execution in a later explicit phase.

A GO decision does not approve production deployment or merge to `main`.
