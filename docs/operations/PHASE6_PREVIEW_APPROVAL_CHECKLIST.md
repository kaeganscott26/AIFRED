# Phase 6 Preview Approval Checklist

> Historical migration record captured before the 2026-08-25 production promotion. Preserve as evidence; current authority is `website-cloudflare-production-2026-08-25.md`.

This checklist is for human approval before any future non-production Cloudflare preview run for `apps/website`.

Do not treat this document as deployment approval. Approval owner and date fields are intentionally blank for human completion.

Approval owner:

Approval date:

## Repository State

- [ ] Phase 1 branch has been pushed.
- [ ] Phase 2 branch has been pushed.
- [ ] Phase 3 branch has been pushed.
- [ ] Phase 4 branch has been pushed.
- [ ] Phase 5 branch has been pushed.
- [ ] Phase 6 branch has been validated.
- [ ] `main` remains untouched.
- [ ] No production workflow has changed.
- [ ] No runtime folders have been deleted.
- [ ] `plugin-aifred/` is still present.
- [ ] `tools/AifredEngine/` is still present.
- [ ] `website/` is still present.
- [ ] `android_admin/` is still present.

## Website Readiness

- [ ] `apps/website` dry-run report has been reviewed.
- [ ] `apps/website` Worker route shape has been reviewed.
- [ ] `/api` route has been reviewed.
- [ ] `/api/v1` route has been reviewed.
- [ ] `/ws/chat` route has been reviewed.
- [ ] Frontend entry has been reviewed.
- [ ] Catalog, download, PayPal, inquiry, admin, and activity references have been reviewed.

## Cloudflare Manual Checks

- [ ] Pages project has been confirmed.
- [ ] GitHub repo binding has been confirmed.
- [ ] Production branch has been confirmed.
- [ ] Production build root has been confirmed.
- [ ] Preview deployment behavior has been confirmed.
- [ ] R2 bindings have been confirmed without exposing values.
- [ ] PayPal env vars have been confirmed without exposing values.
- [ ] Admin auth env vars have been confirmed without exposing values.
- [ ] Domain binding has been confirmed.
- [ ] Rollback path has been confirmed.

## Preview Run Authorization

- [ ] Preview is explicitly non-production.
- [ ] Preview is manually triggered.
- [ ] Preview does not use a production deploy command.
- [ ] Preview does not promote to production.
- [ ] Preview URL will be manually inspected.
- [ ] No secrets will be written to Git.
- [ ] Approval owner field is completed by a human.
- [ ] Approval date field is completed by a human.
