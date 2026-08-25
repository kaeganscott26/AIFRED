# AIFRED Cloudflare Pages And R2 Guide

This is the current deployment and distribution contract for the public AIFRED website.

## Authority

- GitHub repository: `kaeganscott26/AIFRED`
- Production branch: `main`
- Cloudflare Pages project: `aifred-site`
- Website source/output: `apps/website`
- Authoritative Pages config: `apps/website/wrangler.toml`
- Root convenience config: `wrangler.jsonc`
- Operations mirror: `infra/cloudflare/wrangler.toml`

The root and operations configs must describe the same bindings as the authoritative app config. Do not restore the deleted top-level `website/` path or use `north3rnlight3r` as a Pages project name.

## Deployment

```sh
npm ci --prefix apps
npm --prefix apps run website:check
npm --prefix apps run website:deploy
```

The npm script uses the repository-pinned Wrangler version and runs from `apps/website`, ensuring Wrangler reads the authoritative config. GitHub Actions uses the same script when manually dispatched. Pushes validate without deploying, and Cloudflare native Git deployment is disabled so there is one promotion path.

## Bindings

| Binding | Resource | Purpose |
| --- | --- | --- |
| `AIFRED_DOWNLOADS` | R2 `aifred-downloads` | Versioned plugin releases and `assets/` catalog/site objects |
| `AIFRED_REFERENCE_BUCKET` | R2 `aifred-reference-pool` | Accepted reference material |
| `AIFRED_REFERENCE_POOL` | KV | Reference metadata |
| `AIFRED_SALES_LOG` | KV | Current activity/inquiry records and historical sales compatibility |

There is no active `MAILER` service binding and no `AIFRED_WEBSITE_ASSETS` binding. The previously referenced `aifred-website-assets` bucket does not exist in the production account.

## Object Layout

Release objects:

```text
releases/v0.3.6-installer-ai-alias/AIFRED-VST3-Setup.exe
releases/v0.3.6-installer-ai-alias/AIFRED-VST3-windows.zip
```

Website/catalog objects mirror the local `apps/website/assets/` tree:

```text
assets/brand/...
assets/data/...
assets/docs/...
assets/audio/catalog/<file>.mp3
```

Keep `aifred-downloads` private. The Pages Worker reads it through the R2 binding and controls public response headers.

## Public Downloads

```text
GET|HEAD /api/v1/downloads/plugin?asset=setup
GET|HEAD /api/v1/downloads/plugin?asset=zip
GET|HEAD /api/v1/assets/audio/catalog/<file>?download=1
```

Audio streaming uses the same asset route without `download=1`. GET byte ranges return `206 Partial Content` so browser seeking works. Plugin asset names are allowlisted and catalog paths reject traversal.

## Distribution And Logging

```text
distribution.mode = free
payment_pipeline = disabled
```

Public activity and inquiry writes use `AIFRED_SALES_LOG` KV. Repository activity, inquiry, and sales files are read-only historical compatibility; public requests never write them. Authorized admin file-management routes may still write approved repository paths.

## Verification

After deployment, verify both the generated Pages URL and custom domains:

```sh
curl -fsS https://north3rnlight3r.com/api/v1/health
curl -I 'https://north3rnlight3r.com/api/v1/downloads/plugin?asset=zip'
curl -H 'Range: bytes=0-1023' -D - -o /dev/null 'https://north3rnlight3r.com/api/v1/assets/audio/catalog/3amDrill.mp3'
```

The removed payment routes must return `404`.
