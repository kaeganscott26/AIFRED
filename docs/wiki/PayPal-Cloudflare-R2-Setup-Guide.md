# AIFRED PayPal, Cloudflare R2, and GitHub Secret Setup Guide

This guide explains how to fix the Cloudflare deploy secret, move paid installer downloads out of the private GitHub repo, store the installers in Cloudflare R2, and connect PayPal so buyers receive a download only after payment is captured.

The goal is simple:

- GitHub stays private so the website source, backend code, and admin app source are not public.
- GitHub Actions can still deploy the website to Cloudflare Pages.
- Paid installers live in Cloudflare R2, not in a public GitHub release.
- PayPal handles payment approval and capture.
- Cloudflare only gives a buyer a temporary download after PayPal confirms payment.

Source references:

- Cloudflare Pages API token requirements: https://developers.cloudflare.com/pages/configuration/api/
- Cloudflare Workers GitHub Actions authentication: https://developers.cloudflare.com/workers/ci-cd/external-cicd/github-actions/
- Cloudflare R2 presigned URLs: https://developers.cloudflare.com/r2/api/s3/presigned-urls/
- PayPal Orders API capture endpoint: https://developer.paypal.com/docs/api/orders/sdk/v2/
- PayPal Checkout webhook events: https://developer.paypal.com/docs/checkout/apm/reference/subscribe-to-webhooks/

## Part 1 - Replace the broken Cloudflare GitHub secret

Your GitHub Actions workflow already uses these secret names:

- `CLOUDFLARE_ACCOUNT_ID`
- `CLOUDFLARE_API_TOKEN`

The account ID secret exists. The token secret exists, but Cloudflare rejected it in CI. That means you need a fresh API token with Cloudflare Pages deploy permission.

### Step 1 - Open Cloudflare API token page

1. Go to https://dash.cloudflare.com/profile/api-tokens
2. Sign in to the Cloudflare account that owns `north3rnlight3r.com`.
3. Click `Create Token`.
4. Do not use a random old token from the list.
5. Do not use a Tunnel token.
6. Do not use a Workers AI token.
7. Create a new token specifically for GitHub Actions website deployment.

### Step 2 - Create the correct Cloudflare token

Use a custom token.

1. Click `Create Custom Token` or `Get started` under custom token.
2. Token name:

```text
AIFRED GitHub Pages Deploy
```

3. Add this permission:

```text
Account -> Cloudflare Pages -> Edit
```

4. Set account resources:

```text
Include -> Your Cloudflare account
```

5. Zone resources are not required for a Pages direct upload deploy.
6. Client IP filtering can stay blank unless you know you need it.
7. TTL can be blank, or set an expiration date if you want forced rotation.
8. Click `Continue to summary`.
9. Confirm the token summary includes `Cloudflare Pages: Edit`.
10. Click `Create Token`.
11. Copy the token immediately.

Cloudflare only shows the token once. If you close the page before copying it, delete the token and create another one.

### Step 3 - Test the token locally before putting it in GitHub

Open PowerShell in the repo:

```powershell
cd C:\Users\Scott\Projects\AIFRED
$env:CLOUDFLARE_API_TOKEN="PASTE_NEW_TOKEN_HERE"
npx wrangler pages project list
```

Expected result:

- Wrangler should list Cloudflare Pages projects.
- You should see `north3rnlight3r`.
- If you get authentication error `9106`, the token is wrong.
- If you get permission denied, the token is missing `Account -> Cloudflare Pages -> Edit`.

Then test the actual deploy command:

```powershell
npx wrangler pages deploy website --project-name=north3rnlight3r --branch=main --commit-dirty=true
```

Expected result:

- Wrangler creates a Pages deployment.
- The output may mention a `.pages.dev` URL.
- That is normal. The production domain still stays `www.north3rnlight3r.com`.

### Step 4 - Replace the GitHub secret

Use GitHub CLI from PowerShell:

```powershell
cd C:\Users\Scott\Projects\AIFRED
gh secret set CLOUDFLARE_API_TOKEN --repo kaeganscott26/AIFRED
```

Paste the new Cloudflare token when the command waits for input.

Then verify the secret name exists:

```powershell
gh secret list --repo kaeganscott26/AIFRED
```

Expected result:

```text
CLOUDFLARE_ACCOUNT_ID
CLOUDFLARE_API_TOKEN
```

GitHub will not show the value. That is correct. It only shows the secret name.

### Step 5 - Trigger a new GitHub Actions run

After replacing the secret, push any small commit or manually rerun the workflow:

```powershell
gh workflow run build.yml --repo kaeganscott26/AIFRED
```

Watch the run:

```powershell
gh run watch --repo kaeganscott26/AIFRED
```

The Cloudflare deploy job should stop warning about rejected credentials. It should deploy the website from CI.

## Part 2 - Create the Cloudflare R2 bucket for paid installers

R2 is where the paid installer files should live. This keeps the source repo private and avoids forcing customers through GitHub release permissions.

### Step 1 - Create a private R2 bucket

In Cloudflare:

1. Open the Cloudflare dashboard.
2. Go to `R2 Object Storage`.
3. Click `Create bucket`.
4. Bucket name:

```text
aifred-downloads
```

5. Keep the bucket private.
6. Do not enable public access for the whole bucket.

The bucket should hold release files like:

```text
releases/v0.3.4-chat-scroll-actions/AIFRED-VST3-Setup.exe
releases/v0.3.4-chat-scroll-actions/AIFRED-Uninstall.exe
releases/v0.3.4-chat-scroll-actions/AIFRED-VST3-windows.zip
releases/v0.3.4-chat-scroll-actions/AIFRED-VST3-macos.zip
releases/v0.3.4-chat-scroll-actions/AIFRED-VST3-arch.zip
```

### Step 2 - Upload files to R2

Use Wrangler:

```powershell
cd C:\Users\North\Documents\Projects\AIFRED
npx wrangler r2 object put aifred-downloads/releases/v0.3.4-chat-scroll-actions/AIFRED-VST3-Setup.exe --file dist\installer\windows\AIFRED-VST3-Setup.exe --remote
npx wrangler r2 object put aifred-downloads/releases/v0.3.4-chat-scroll-actions/AIFRED-Uninstall.exe --file dist\uninstaller\windows\AIFRED-Uninstall.exe --remote
```

Upload the other installers after they are available locally:

```powershell
npx wrangler r2 object put aifred-downloads/releases/v0.3.4-chat-scroll-actions/AIFRED-VST3-windows.zip --file dist\AIFRED-VST3-windows.zip --remote
npx wrangler r2 object put aifred-downloads/releases/v0.3.4-chat-scroll-actions/AIFRED-VST3-macos.zip --file dist\AIFRED-VST3-macos.zip --remote
npx wrangler r2 object put aifred-downloads/releases/v0.3.4-chat-scroll-actions/AIFRED-VST3-arch.zip --file dist\AIFRED-VST3-arch.zip --remote
```

Confirm the bucket contents:

```powershell
npx wrangler r2 object get aifred-downloads/releases/v0.3.4-chat-scroll-actions/AIFRED-VST3-Setup.exe --file $env:TEMP\aifred-setup-check.exe --remote
```

### Step 3 - Bind R2 to the website backend

The website backend should not expose R2 credentials in browser JavaScript. It should access R2 from Cloudflare server-side code.

Add an R2 binding to the Cloudflare Pages project:

```powershell
npx wrangler pages project list
```

Then configure the Pages project in the Cloudflare dashboard:

1. Go to `Workers & Pages`.
2. Open `north3rnlight3r`.
3. Go to `Settings`.
4. Go to `Functions`.
5. Add an R2 bucket binding.
6. Variable name:

```text
AIFRED_DOWNLOADS
```

7. Bucket:

```text
aifred-downloads
```

The backend can then call `env.AIFRED_DOWNLOADS.get(objectKey)` to fetch private installer files.

## Part 3 - Create PayPal app credentials

PayPal needs a REST app so the backend can create and capture checkout orders.

### Step 1 - Open PayPal Developer Dashboard

1. Go to https://developer.paypal.com/dashboard/
2. Sign in with the PayPal account that will receive AIFRED payments.
3. Go to `Apps & Credentials`.
4. Choose `Live` when you are ready to take real money.
5. Use `Sandbox` first if you want to test without charging real money.

### Step 2 - Create the PayPal app

1. Click `Create App`.
2. App name:

```text
AIFRED Website Checkout
```

3. Merchant account:

```text
Your PayPal business account
```

4. Click `Create App`.

PayPal will show:

- Client ID
- Secret

The Client ID can be used by the browser. The Secret must only live in Cloudflare environment variables.

### Step 3 - Add PayPal secrets to Cloudflare Pages

In Cloudflare dashboard:

1. Go to `Workers & Pages`.
2. Open `north3rnlight3r`.
3. Go to `Settings`.
4. Go to `Environment variables`.
5. Add these production variables:

```text
PAYPAL_CLIENT_ID=your live PayPal client ID
PAYPAL_CLIENT_SECRET=your live PayPal secret
PAYPAL_ENV=live
AIFRED_PRODUCT_PRICE=49.00
AIFRED_PRODUCT_CURRENCY=USD
AIFRED_RELEASE_VERSION=v0.3.4-chat-scroll-actions
```

For sandbox testing, use:

```text
PAYPAL_ENV=sandbox
```

and use sandbox credentials instead of live credentials.

## Part 4 - Payment and download flow

The secure flow should work like this:

1. Buyer clicks `Buy AIFRED`.
2. Website calls your backend endpoint:

```text
POST /api/v1/paypal/create-order
```

3. Backend calls PayPal and creates an order for the product price.
4. Browser opens PayPal approval.
5. Buyer approves payment.
6. Website calls your backend endpoint:

```text
POST /api/v1/paypal/capture-order
```

7. Backend captures the PayPal order.
8. Backend verifies the capture status is completed.
9. Backend creates a short-lived download token.
10. Website redirects buyer to:

```text
/api/v1/download?token=SHORT_LIVED_TOKEN&platform=windows
```

11. Backend validates the token.
12. Backend reads the installer from private R2.
13. Backend streams the installer to the buyer.

The buyer never receives:

- GitHub source access
- R2 account credentials
- R2 write credentials
- Admin app APK
- Admin app source
- Permanent unrestricted file links

## Part 5 - Recommended token storage

Use Cloudflare KV or D1 for paid download tokens.

KV is simpler:

```text
download-token -> order ID, buyer email, expiry, allowed files
```

D1 is stronger for order records:

```sql
CREATE TABLE orders (
  id TEXT PRIMARY KEY,
  paypal_order_id TEXT NOT NULL,
  paypal_capture_id TEXT,
  buyer_email TEXT,
  status TEXT NOT NULL,
  created_at TEXT NOT NULL,
  captured_at TEXT,
  expires_at TEXT
);

CREATE TABLE download_tokens (
  token TEXT PRIMARY KEY,
  order_id TEXT NOT NULL,
  allowed_platforms TEXT NOT NULL,
  expires_at TEXT NOT NULL,
  used_count INTEGER NOT NULL DEFAULT 0
);
```

For AIFRED, use D1 if you want clean admin records. Use KV if you only need quick one-time download links.

## Part 6 - Worker endpoint design

Use these endpoints:

```text
POST /api/v1/paypal/create-order
POST /api/v1/paypal/capture-order
GET  /api/v1/download
POST /api/v1/paypal/webhook
```

`create-order` should:

- Read product price from `AIFRED_PRODUCT_PRICE`.
- Read currency from `AIFRED_PRODUCT_CURRENCY`.
- Create the PayPal order server-side.
- Return the PayPal order ID to the browser.

`capture-order` should:

- Accept the PayPal order ID.
- Call PayPal capture.
- Confirm the capture status is complete.
- Store the order.
- Create a download token.
- Return the download URL.

`download` should:

- Require a token.
- Reject expired tokens.
- Reject unknown tokens.
- Reject platform values not allowed for that order.
- Read the installer from R2.
- Stream the installer with `Content-Disposition: attachment`.

`webhook` should:

- Receive PayPal events.
- Store completed captures.
- Help recover purchases if the buyer closes the browser after paying.

## Part 7 - Website button behavior

The public website button should not link directly to GitHub.

Replace direct GitHub download links with purchase actions:

```text
Buy AIFRED for Windows
Buy AIFRED for macOS
Buy AIFRED for Arch Linux
```

Each button should:

- Start PayPal checkout.
- Capture the order through the backend.
- Download the matching installer from R2.

For free demos, use separate public files:

```text
demo/AIFRED-demo-readme.pdf
demo/AIFRED-demo-pack.zip
```

Do not mix free files and paid installer files under the same public path.

## Part 8 - PayPal webhooks

Webhooks are not optional for a clean paid product. Browser redirects can fail. Webhooks let the backend know the payment happened even if the buyer closes the tab.

Create a PayPal webhook:

1. Go to PayPal Developer Dashboard.
2. Open the AIFRED app.
3. Go to `Webhooks`.
4. Add webhook URL:

```text
https://www.north3rnlight3r.com/api/v1/paypal/webhook
```

5. Subscribe to checkout/order events.
6. At minimum, handle the approved and completed/captured payment path.

Store the PayPal webhook ID as a Cloudflare environment variable:

```text
PAYPAL_WEBHOOK_ID=your webhook ID
```

Production code should verify PayPal webhook signatures before trusting webhook events.

## Part 9 - Final test checklist

Test in this exact order:

1. Confirm the site is live:

```powershell
Invoke-WebRequest -UseBasicParsing https://www.north3rnlight3r.com/api/v1/health
```

2. Confirm GitHub Actions deploys without Cloudflare credential warnings.
3. Confirm R2 bucket contains all installer files.
4. Confirm PayPal sandbox app can create an order.
5. Confirm PayPal sandbox capture creates a download token.
6. Confirm the download endpoint streams the Windows installer.
7. Confirm expired tokens stop working.
8. Confirm direct R2 URLs do not expose paid installers publicly.
9. Switch PayPal from sandbox to live.
10. Make one real low-price test purchase.
11. Restore the real product price.

## Part 10 - What not to do

Do not put PayPal secrets in website JavaScript.

Do not put R2 API keys in website JavaScript.

Do not make the whole R2 bucket public.

Do not use GitHub release assets as public paid downloads while the repo is private.

Do not distribute the Android admin APK through GitHub releases.

Do not store buyer download access only in browser local storage.

Do not trust a browser message saying payment succeeded. Always confirm through PayPal capture or verified webhook.

## Part 11 - Clean production architecture

Use this final architecture:

```text
Buyer browser
  -> www.north3rnlight3r.com
  -> PayPal approval popup
  -> Cloudflare Pages Function captures order
  -> Cloudflare D1 or KV stores purchase
  -> Cloudflare R2 stores private installers
  -> Cloudflare download endpoint streams installer
```

The admin app remains private:

```text
Owner phone
  -> AIFRED Admin app
  -> authenticated admin API
  -> website/backend maintenance
  -> local shell/file tools on the phone
```

The public repo exposure stays controlled:

```text
GitHub private repo
  -> source code private
  -> CI builds private
  -> public-facing website deploys through Cloudflare
  -> paid downloads served from R2 after PayPal capture
```

