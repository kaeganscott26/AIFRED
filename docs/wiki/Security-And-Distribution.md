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

## Paid Delivery

The public website buy button is a $5 PayPal path. Completed PayPal IPN events are recorded by the Cloudflare backend, then the backend issues a short-lived tokenized download link for the configured AIFRED release asset.

Required production settings:

- PayPal receiver email must match the account used for AIFRED sales.
- `DOWNLOAD_REPO`, `DOWNLOAD_TAG`, and `DOWNLOAD_ASSET` must point to the current distributable package.
- `GITHUB_TOKEN` must be available to the Worker when private release assets are used.
- `NOTIFY_TO_EMAIL`, `NOTIFY_FROM_EMAIL`, and the email provider token must be set if payment/download notifications are expected.

Do not bypass the token route with a public direct installer URL unless the release is intentionally free.

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

