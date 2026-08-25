# Cloudflare Manual Verification Checklist

Do not record secret values in this document or in Git. The 2026-08-25 production pass completed this checklist; repeat it for later promotions.

Before any deployment migration, manually confirm:

- [x] Cloudflare account is the expected account.
- [x] Pages project is `aifred-site`.
- [x] GitHub repository is `kaeganscott26/AIFRED`; automatic Git deployment is disabled.
- [x] Production branch is `main`.
- [x] Canonical source/output is `apps/website`.
- [x] Worker route handling is confirmed for `/api`, `/api/v1`, `/v1`, and `/ws/chat`.
- [x] Environment-variable names and bindings were inspected without recording values.
- [x] R2 bindings are `AIFRED_DOWNLOADS` and `AIFRED_REFERENCE_BUCKET`.
- [x] Payment secrets/routes are absent from the active contract.
- [x] Admin auth remains server-enforced.
- [x] `aifred-site.pages.dev`, `north3rnlight3r.com`, and `www.north3rnlight3r.com` serve the production deployment.
- [x] The generated deployment hostname is protected by Cloudflare Access; public aliases were verified directly.
- [x] Production promotion uses `npm --prefix apps run website:deploy` after the commit is pushed.

See `website-cloudflare-production-2026-08-25.md` for command output, deployment identifiers, differences, and blockers.
