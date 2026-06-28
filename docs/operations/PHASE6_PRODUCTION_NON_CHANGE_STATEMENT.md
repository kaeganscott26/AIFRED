# Phase 6 Production Non-Change Statement

Phase 6 is a gate and review phase only.

Phase 6 does not:

- deploy.
- change production.
- switch Cloudflare paths.
- alter `.github/workflows/build.yml` production deployment behavior.
- alter tag release publishing.
- touch secrets.
- move `plugin-aifred/`.
- move `tools/AifredEngine/`.
- delete `website/`.
- delete `android_admin/`.

Any future production change requires a separate explicit phase with its own scope, validation, and approval.
