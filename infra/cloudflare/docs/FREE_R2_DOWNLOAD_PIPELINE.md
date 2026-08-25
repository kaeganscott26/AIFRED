# AIFRED Free R2 Download Pipeline

The current runtime pipeline is:

```text
browser
  -> Cloudflare Pages project aifred-site
  -> apps/website/_worker.js
  -> allowlisted plugin route or safe catalog asset route
  -> private AIFRED_DOWNLOADS binding
  -> aifred-downloads R2 object
```

Public activity and inquiry events are written to `AIFRED_SALES_LOG` KV. They do not mutate GitHub.

The canonical operational guide is [`docs/wiki/Cloudflare-R2-Setup-Guide.md`](../../../docs/wiki/Cloudflare-R2-Setup-Guide.md). This file is only an infrastructure entry point.

The active pipeline has no checkout, order creation, capture, IPN, entitlement token, `MAILER` service binding, or separate website-assets bucket.

Plugin routes expose Windows setup/ZIP and the published macOS ZIP through allowlisted `asset=setup|zip|macos` values. Catalog streaming supports explicit byte ranges for browser seeking; `?download=1` adds attachment behavior.
