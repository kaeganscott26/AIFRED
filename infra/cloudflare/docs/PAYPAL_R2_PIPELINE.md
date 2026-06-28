# AIFRED PayPal + R2 Delivery Pipeline

## Customer Flow

1. Customer clicks the $5 PayPal button on `north3rnlight3r.com`.
2. PayPal sends IPN to `/api/v1/paypal/ipn`.
3. The backend verifies the IPN with PayPal.
4. The backend fulfills only `payment_status=Completed`, receiver email match, USD, and amount `5.00`.
5. A sale record is committed to `ops/payments/sales.json`.
6. Customer and owner emails are sent with tokenized download links.
7. Download links call `/api/v1/sales/download`.
8. The backend validates the token and serves the release asset from R2.

If an R2 object is missing, the backend falls back to the matching GitHub release asset when `GITHUB_TOKEN` is configured.

Pending PayPal payments are not fulfilled. PayPal can later send another IPN when the payment becomes `Completed`.

## Required Cloudflare Bindings And Secrets

R2 bucket binding:

- Binding name: `AIFRED_DOWNLOADS`
- Bucket: `aifred-downloads`

Reference intake storage:

- KV binding name: `AIFRED_REFERENCE_POOL`
- R2 binding name: `AIFRED_REFERENCE_BUCKET`
- R2 bucket: `aifred-reference-pool`

Sales log storage:

- KV binding name: `AIFRED_SALES_LOG`

Release/version variable:

- `AIFRED_RELEASE_VERSION=v0.3.3-ollama-chat-actions`

PayPal:

- `AIFRED_PAYPAL_BUSINESS=kaeganscott@outlook.com`

GitHub sale log storage:

- `AIFRED_GITHUB_REPO=kaeganscott26/aifred-site`
- `AIFRED_GITHUB_BRANCH=main`
- `GITHUB_TOKEN=<repo contents read/write token>`

Email delivery:

- `AIFRED_CONTACT_EMAIL=north3rnlight3rofficial@outlook.com`
- `AIFRED_EMAIL_FROM=sales@north3rnlight3r.com`
- Configure either the `MAILER` service binding or Cloudflare Email Workers binding used by the backend.

Fallback release downloads if R2 is not bound:

- `AIFRED_PLUGIN_REPO=kaeganscott26/AIFRED`
- `AIFRED_PLUGIN_RELEASE_TAG=v0.3.3-ollama-chat`
- `GITHUB_TOKEN` must be able to read the release asset.

## R2 Object Keys

Upload these objects:

```text
releases/v0.3.3-ollama-chat-actions/AIFRED-VST3-Setup.exe
releases/v0.3.3-ollama-chat-actions/AIFRED-VST3-windows.zip
```

Optional additional packages:

```text
releases/v0.3.3-ollama-chat-actions/AIFRED-VST3-macos.zip
releases/v0.3.3-ollama-chat-actions/AIFRED-VST3-linux.zip
releases/v0.3.3-ollama-chat-actions/AIFRED-VST3-arch.zip
```

## PayPal Account Settings

Use a verified PayPal business account. In PayPal, make sure IPN is enabled and points to:

```text
https://north3rnlight3r.com/api/v1/paypal/ipn
```

PayPal may hold or mark some payments pending because of account verification, eCheck/bank funding, risk review, or seller holds. The site should not release a download until PayPal sends a completed payment event.

## Admin Visibility

The admin clients read:

```text
GET /api/v1/admin/sales/list
GET /api/v1/admin/reference/list
GET /api/v1/admin/logs/list
```

Sales are loaded from `AIFRED_SALES_LOG` KV with repository fallback. Accepted analyzer references are loaded from `AIFRED_REFERENCE_POOL` KV and mirrored into R2 metadata objects when the R2 binding is present.
