# Security And Distribution

## Repository Visibility

The AIFRED monorepo should remain private because it contains source code, owner-only admin tooling, backend route maps, operational documentation, and deployment controls.

The public website is public. The repository is not.

## Android Admin App

The Android admin app is owner-only.

Current allowed workflow:

- Private source under `apps/admin-android/`.
- Local debug build and sideload to the owner's device.
- Private validation of the app source and Gradle project.

Do not:

- Attach the APK to public GitHub releases.
- Publish a public artifact download.
- Publish screenshots containing private configuration.
- Publish owner credentials.

The current app `release` build type still uses debug signing and has minification disabled, so it should not be treated as a hardened public production build.

## Private Configuration

Do not commit private runtime values such as provider keys, deployment credentials, payment secrets, owner passwords, or private tokens.

Use deployment environment configuration and repository secret storage for private values.

The repository should contain examples and variable names only, never live secret values.

## Paid Beta Delivery

The current public website presents AIFRED as a **$5 one-time beta purchase** with no subscription or recurring charge.

Current payment/download flow:

```text
Buyer
  -> POST /api/v1/paypal/create-order
  -> PayPal approval
  -> POST /api/v1/paypal/capture-order
  -> short-lived download token
  -> GET /api/v1/sales/download
  -> AIFRED_DOWNLOADS R2 object or configured fallback
```

Current release metadata examples point to:

```text
AIFRED_PLUGIN_REPO=kaeganscott26/AIFRED
AIFRED_PLUGIN_RELEASE_TAG=v0.3.6-installer-ai-alias
AIFRED_RELEASE_VERSION=v0.3.6-installer-ai-alias
```

Current Windows/macOS release artifacts are:

- `AIFRED-VST3-Setup.exe`
- `AIFRED-Uninstall.exe`
- `AIFRED-VST3-windows.zip`
- `AIFRED-VST3-macOS.pkg`

Do not advertise Linux or Arch packages as current release outputs unless those targets are restored to CI and verified.

## Cloudflare Storage

Current storage bindings include:

- `AIFRED_DOWNLOADS` — paid installer/download storage.
- `AIFRED_WEBSITE_ASSETS` — catalog audio and website assets.
- `AIFRED_REFERENCE_BUCKET` — reference audio storage.
- `AIFRED_REFERENCE_POOL` — reference metadata persistence.
- `AIFRED_SALES_LOG` — sale/activity persistence.

Keep private buckets private unless a specific asset is intentionally designed for public access.

## Admin API Controls

Admin endpoints require authenticated admin access. File operations reject unsafe paths, and delete operations are restricted to approved `apps/website/` paths.

High-risk routes include:

- `/api/v1/admin/files/write`
- `/api/v1/admin/files/delete`
- `/api/v1/admin/files/upload`
- `/api/v1/admin/catalog/upload`
- `/api/v1/admin/reference/upload`
- `/api/v1/command/run`

Only the owner should have access to the Android app and admin credentials.

## Local AI And OpenAI

The default local path is:

```text
AIFRED VST3
  -> http://127.0.0.1:8787
  -> http://127.0.0.1:11434
  -> aifred:latest
```

The OpenAI-compatible path uses:

```text
https://api.openai.com/v1/responses
model: gpt-5.6-luna
```

when an API key is configured.

Never embed a private provider key into public website JavaScript, committed plugin binaries, screenshots, docs, or repository text.

## Release Boundaries

Current public release targets:

- Windows VST3 zip/installer/uninstaller.
- macOS VST3 pkg.

Not current public release targets:

- Android admin APK.
- Linux package.
- Arch package.

Generic UNIX packaging code and old references may remain in historical files until separately verified. Historical changelog entries should remain truthful records of past states rather than being rewritten as if those releases never existed.
