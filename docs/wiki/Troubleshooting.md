# Troubleshooting

## Website Opens On `.pages.dev`

Cloudflare Wrangler prints a `.pages.dev` preview URL after deployment. That is normal. Production must be tested at:

```text
https://www.north3rnlight3r.com
https://north3rnlight3r.com
```

Verify:

```powershell
Invoke-WebRequest -UseBasicParsing https://www.north3rnlight3r.com/api/v1/health
```

Expected JSON:

```json
{"ok":true,"service":"AIFRED website backend"}
```

## API Returns HTML

If `/api/v1/health` returns HTML, the Pages Worker did not deploy. Confirm `website/_worker.js` exists and redeploy:

```powershell
npx wrangler pages deploy website --project-name=north3rnlight3r --branch=main
```

## GitHub Actions Cloudflare Deploy Warning

Cloudflare deploy can warn if repository credentials are stale. Build/package jobs still pass. Refresh:

- `CLOUDFLARE_ACCOUNT_ID`
- `CLOUDFLARE_API_TOKEN`

The token must be accepted by Wrangler for Pages deployment.

## FL Studio Does Not See AIFRED

1. Close FL Studio.
2. Install system-wide:

```powershell
.\dist\installer\windows\AIFRED-VST3-Setup.exe --system
```

3. Open FL Studio Plugin Manager.
4. Add/rescan:

```text
C:\Program Files\Common Files\VST3
```

If Windows refuses to replace the VST, FL Studio probably has the old DLL loaded. Close FL Studio, then retry.

## Android App Will Not Install

Confirm ADB:

```powershell
$adb=Join-Path $env:LOCALAPPDATA 'Android\Sdk\platform-tools\adb.exe'
& $adb devices
```

Install:

```powershell
& $adb install -r android_admin\app\build\outputs\apk\debug\app-debug.apk
```

If the device is unauthorized, unlock the phone and accept the USB debugging prompt.

## Admin File Upload Fails

The backend needs a GitHub token in Cloudflare Pages:

- `GITHUB_TOKEN`
- `AIFRED_GITHUB_REPO=kaeganscott26/AIFRED`
- `AIFRED_GITHUB_BRANCH=main`

The token needs repo contents read/write and workflow dispatch permissions.

## Ollama From Phone Fails

Do not use `localhost` from the phone unless Ollama runs on the phone. Use the computer LAN IP:

```text
http://192.168.x.x:11434
```

Ollama must listen on a network interface reachable from the phone, and Windows Firewall must allow the connection.
