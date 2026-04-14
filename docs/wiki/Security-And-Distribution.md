# Security And Distribution

## Repository Visibility

The repository must remain private. It contains:

- Website source code
- Admin app source code
- Backend route maps
- Internal operational docs
- Admin authentication flow
- Deployment controls

The website itself is public. The repository is not.

## Android Admin App

The Android admin app is owner-only. It must not be attached to public GitHub releases and must not be distributed through public app stores.

Allowed:

- Local debug install to the owner's phone
- Private source in the private repository
- CI compile validation without public artifact upload

Not allowed:

- Public APK release asset
- Public artifact download
- Public screenshots containing credentials
- Public docs containing the admin password

## Secrets

Never commit:

- Cloudflare keys or tokens
- GitHub tokens
- OpenAI API keys
- Admin passwords
- Ollama LAN credentials or private tunnels

Use Cloudflare Pages environment variables and GitHub repository secrets.

## Admin API Controls

Admin endpoints require a signed admin session. File operations reject unsafe paths. Delete is restricted to `website/`.

High-risk routes:

- `/api/v1/admin/files/write`
- `/api/v1/admin/files/delete`
- `/api/v1/admin/files/upload`
- `/api/v1/admin/catalog/upload`
- `/api/v1/admin/reference/upload`
- `/api/v1/command/run`

Only the owner should have access to the Android app and admin credentials.

