# AIFRED PayPal + R2 Delivery Pipeline

The detailed current setup guide is:

- [`docs/wiki/PayPal-Cloudflare-R2-Setup-Guide.md`](../../../docs/wiki/PayPal-Cloudflare-R2-Setup-Guide.md)

This file is the concise operations summary.

## Current Customer Flow

1. Customer starts the $5 one-time AIFRED beta purchase on `www.north3rnlight3r.com`.
2. Website calls `POST /api/v1/paypal/create-order`.
3. Customer approves the PayPal order.
4. Website calls `POST /api/v1/paypal/capture-order`.
5. Backend verifies the capture result.
6. Sale/activity metadata is recorded when configured.
7. Backend issues tokenized download access.
8. Download uses `GET /api/v1/sales/download`.
9. Backend reads the requested release object from `AIFRED_DOWNLOADS` R2 or the configured fallback path.

## Current Release Metadata

Repository examples currently use:

```text
AIFRED_GITHUB_REPO=kaeganscott26/AIFRED
AIFRED_PLUGIN_REPO=kaeganscott26/AIFRED
AIFRED_PLUGIN_RELEASE_TAG=v0.3.6-installer-ai-alias
AIFRED_RELEASE_VERSION=v0.3.6-installer-ai-alias
```

## Current Release Objects

```text
releases/v0.3.6-installer-ai-alias/AIFRED-VST3-Setup.exe
releases/v0.3.6-installer-ai-alias/AIFRED-Uninstall.exe
releases/v0.3.6-installer-ai-alias/AIFRED-VST3-windows.zip
releases/v0.3.6-installer-ai-alias/AIFRED-VST3-macOS.pkg
```

Linux and Arch packages are not current GitHub Actions release outputs.

## Cloudflare Storage

| Binding | Resource / purpose |
| --- | --- |
| `AIFRED_DOWNLOADS` | `aifred-downloads` bucket for release delivery |
| `AIFRED_WEBSITE_ASSETS` | `aifred-website-assets` bucket for catalog/site assets |
| `AIFRED_REFERENCE_BUCKET` | `aifred-reference-pool` bucket for reference material |
| `AIFRED_REFERENCE_POOL` | Reference metadata KV |
| `AIFRED_SALES_LOG` | Sales/activity KV |

## Catalog Audio

Production catalog streams use:

```text
/api/v1/assets/audio/catalog/<file>
```

The backend reads from `AIFRED_WEBSITE_ASSETS` first. Local static files remain a development fallback until R2 parity is verified.

## Admin Visibility

Current admin routes include:

```text
GET /api/v1/admin/sales/list
GET /api/v1/admin/logs/list
GET /api/v1/admin/inquiries/list
GET /api/v1/admin/dashboard/state
```

## Important Boundaries

- Do not point GitHub-backed operations at the old `kaeganscott26/aifred-site` repository.
- Do not advertise old macOS zip, Linux, or Arch artifacts as current release outputs.
- Do not remove local catalog-audio fallbacks until R2 parity is verified.
- Do not place live private values in committed files.
