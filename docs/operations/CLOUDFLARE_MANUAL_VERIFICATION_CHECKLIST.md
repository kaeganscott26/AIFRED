# Cloudflare Manual Verification Checklist

Do not record secret values in this document or in Git.

Before any deployment migration, manually confirm:

- [ ] Cloudflare account is the expected account.
- [ ] Pages project name is confirmed.
- [ ] GitHub repo binding is confirmed.
- [ ] Production branch is confirmed.
- [ ] Build root is confirmed.
- [ ] Output directory is confirmed.
- [ ] Worker route handling is confirmed for `/api`, `/api/v1`, and `/ws/chat`.
- [ ] Environment variables and bindings are present without exposing values.
- [ ] R2 bucket bindings are present.
- [ ] PayPal-related environment variables exist without exposing values.
- [ ] Admin auth environment variables exist without exposing values.
- [ ] Domain binding for `www.north3rnlight3r.com` is present.
- [ ] Apex redirect behavior is confirmed if used.
- [ ] Preview deployments are available before production migration.

Phase 4 does not run Wrangler or Cloudflare deploy commands. This checklist is manual verification guidance only.
