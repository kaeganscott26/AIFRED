# AIFRED PayPal, Cloudflare R2, And GitHub Setup Guide

This guide reflects the current consolidated `kaeganscott26/AIFRED` monorepo and the website/backend under `apps/website/`.

It documents the configuration the repository expects. It does not claim that every production Cloudflare binding or private value is already configured; those must be verified in the Cloudflare dashboard and live environment.

## Current Architecture

```text
Buyer browser
  -> https://www.north3rnlight3r.com
  -> Cloudflare Pages Worker from apps/website/_worker.js
  -> /api/v1/paypal/create-order
  -> PayPal approval
  -> /api/v1/paypal/capture-order
  -> short-lived download token
  -> /api/v1/sales/download
  -> AIFRED_DOWNLOADS R2 object or configured fallback
```

The private source repository remains:

```text
kaeganscott26/AIFRED
```

The public website source is:

```text
apps/website/
```

## Current Product And Release Metadata

The public website currently presents:

```text
$5 one-time beta purchase
Free updates for life
No subscription
No recurring charge
```

Current repository examples use:

```text
AIFRED_PLUGIN_REPO=kaeganscott26/AIFRED
AIFRED_PLUGIN_RELEASE_TAG=v0.3.6-installer-ai-alias
AIFRED_RELEASE_VERSION=v0.3.6-installer-ai-alias
```

Current release artifacts:

```text
AIFRED-VST3-Setup.exe
AIFRED-Uninstall.exe
AIFRED-VST3-windows.zip
AIFRED-VST3-macOS.pkg
```

Linux and Arch packages are not current GitHub Actions release targets.

## Cloudflare Config Roles

| File | Role |
| --- | --- |
| `apps/website/wrangler.toml` | Primary website Pages config and bindings |
| `infra/cloudflare/wrangler.toml` | Operations/support mirror |
| `wrangler.jsonc` | Root convenience config pointed at `apps/website` |

Current configured bindings include:

```text
MAILER
AIFRED_REFERENCE_POOL
AIFRED_SALES_LOG
AIFRED_DOWNLOADS
AIFRED_WEBSITE_ASSETS
AIFRED_REFERENCE_BUCKET
```

## Part 1 — Cloudflare Pages Deployment

The production Pages project name used by the current workflow is:

```text
north3rnlight3r
```

Production domains:

```text
https://www.north3rnlight3r.com
https://north3rnlight3r.com
```

Local deployment command:

```powershell
npx wrangler pages deploy apps/website --project-name=north3rnlight3r --branch=main
```

The main GitHub Actions workflow can deploy from `main` when the repository's Cloudflare deployment configuration is present and accepted.

After deployment, verify:

```powershell
Invoke-WebRequest -UseBasicParsing https://www.north3rnlight3r.com/api/v1/health
```

If the API returns HTML instead of JSON, verify that `apps/website/_worker.js` was included in the deployment.

## Part 2 — R2 Buckets

### Installer downloads

Binding:

```text
AIFRED_DOWNLOADS
```

Bucket:

```text
aifred-downloads
```

Recommended current object layout:

```text
releases/v0.3.6-installer-ai-alias/AIFRED-VST3-Setup.exe
releases/v0.3.6-installer-ai-alias/AIFRED-Uninstall.exe
releases/v0.3.6-installer-ai-alias/AIFRED-VST3-windows.zip
releases/v0.3.6-installer-ai-alias/AIFRED-VST3-macOS.pkg
```

Do not upload Linux or Arch objects as if they are current release outputs unless those packages are restored to CI and verified.

### Website/catalog assets

Binding:

```text
AIFRED_WEBSITE_ASSETS
```

Bucket:

```text
aifred-website-assets
```

Catalog objects use keys under:

```text
assets/audio/catalog/<file>
```

The website backend serves them through:

```text
/api/v1/assets/audio/catalog/<file>
```

Local static audio remains a development fallback until R2 parity is explicitly verified.

### Reference storage

Metadata binding:

```text
AIFRED_REFERENCE_POOL
```

R2 binding:

```text
AIFRED_REFERENCE_BUCKET
```

Bucket:

```text
aifred-reference-pool
```

### Sales/activity storage

Binding:

```text
AIFRED_SALES_LOG
```

## Part 3 — Current Website Environment Example

The repository example file is:

```text
apps/website/.dev.vars.example
```

It currently documents configuration names for:

```text
AIFRED_CHAT_PROVIDER
OPENAI_API_KEY
OPENAI_MODEL
OLLAMA_BASE_URL
OLLAMA_MODEL
AIFRED_CONTACT_EMAIL
AIFRED_PAYPAL_BUSINESS
AIFRED_PAYPAL_AMOUNT
AIFRED_PAYPAL_CURRENCY
PAYPAL_ENVIRONMENT
PAYPAL_CLIENT_ID
PAYPAL_CLIENT_SECRET
AIFRED_EMAIL_FROM
AIFRED_GITHUB_REPO
AIFRED_PLUGIN_REPO
AIFRED_PLUGIN_RELEASE_TAG
AIFRED_RELEASE_VERSION
AIFRED_WEBSITE_ASSETS_BUCKET
MAILER_SHARED_TOKEN
```

Do not place live private values into committed files.

Current non-secret defaults include:

```text
OPENAI_MODEL=gpt-5.6-luna
OLLAMA_BASE_URL=http://127.0.0.1:11434
OLLAMA_MODEL=aifred:latest
AIFRED_GITHUB_REPO=kaeganscott26/AIFRED
AIFRED_PLUGIN_REPO=kaeganscott26/AIFRED
AIFRED_PLUGIN_RELEASE_TAG=v0.3.6-installer-ai-alias
AIFRED_RELEASE_VERSION=v0.3.6-installer-ai-alias
```

## Part 4 — PayPal Flow

Current frontend/backend flow:

1. Buyer selects AIFRED on the website.
2. Website calls:

```text
POST /api/v1/paypal/create-order
```

3. Backend creates a PayPal order using server-side configuration.
4. Buyer approves payment.
5. Website calls:

```text
POST /api/v1/paypal/capture-order
```

6. Backend verifies the capture result.
7. Backend records activity/sale metadata when configured.
8. Backend issues tokenized download access.
9. Buyer downloads through:

```text
GET /api/v1/sales/download
```

The browser should never receive infrastructure credentials or direct write access to private storage.

## Part 5 — Upload Current Release Objects

Example Windows uploads:

```powershell
npx wrangler r2 object put aifred-downloads/releases/v0.3.6-installer-ai-alias/AIFRED-VST3-Setup.exe --file dist\installer\windows\AIFRED-VST3-Setup.exe --remote
npx wrangler r2 object put aifred-downloads/releases/v0.3.6-installer-ai-alias/AIFRED-Uninstall.exe --file dist\uninstaller\windows\AIFRED-Uninstall.exe --remote
npx wrangler r2 object put aifred-downloads/releases/v0.3.6-installer-ai-alias/AIFRED-VST3-windows.zip --file dist\AIFRED-VST3-windows.zip --remote
```

Example macOS upload:

```sh
npx wrangler r2 object put aifred-downloads/releases/v0.3.6-installer-ai-alias/AIFRED-VST3-macOS.pkg --file dist/macos/AIFRED-VST3-macOS.pkg --remote
```

Confirm exact local output paths after a successful build before uploading.

## Part 6 — Admin Visibility

Current admin-facing routes include:

```text
GET /api/v1/admin/sales/list
GET /api/v1/admin/logs/list
GET /api/v1/admin/inquiries/list
GET /api/v1/admin/dashboard/state
```

The Android admin app can also manage catalog, reference, and website asset uploads through authenticated admin routes.

## Part 7 — Model Routing

Website/admin backend defaults:

```text
OpenAI endpoint: https://api.openai.com/v1/responses
OpenAI model: gpt-5.6-luna
Local Ollama endpoint: http://127.0.0.1:11434
Local Ollama model: aifred:latest
```

The Cloudflare website/backend and the local VST engine are separate systems. Do not merge their runtime responsibilities.

## Part 8 — Final Verification Checklist

Verify in this order:

1. `apps/website/wrangler.toml` reflects the intended bindings.
2. Cloudflare Pages project `north3rnlight3r` has the expected bindings/environment configuration.
3. `https://www.north3rnlight3r.com/api/v1/health` returns JSON.
4. `AIFRED_WEBSITE_ASSETS` can serve catalog objects.
5. The website catalog works with R2-backed audio.
6. Local static fallback is still available during the R2 verification phase.
7. `AIFRED_DOWNLOADS` contains the intended current release objects.
8. PayPal sandbox can create an order.
9. PayPal sandbox can capture an approved order.
10. Successful capture produces tokenized download access.
11. Invalid or expired download tokens are rejected.
12. Windows installer download works.
13. macOS pkg download works when that object is present.
14. Admin sale/activity visibility works when configured.
15. Only after R2 parity is proven should local catalog audio be considered for removal from Git.

## What Not To Do

- Do not restore the deleted top-level `website/` tree.
- Do not configure the backend to write to the old `kaeganscott26/aifred-site` repository.
- Do not publish the Android admin APK as a public release artifact.
- Do not advertise Linux or Arch release packages as current outputs.
- Do not remove the local MP3 fallback until R2 parity is verified.
- Do not delete the legacy `/api/*` compatibility shim until production usage is verified.
- Do not store live private values in committed source files.
