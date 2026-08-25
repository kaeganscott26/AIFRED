# AIFRED Admin Desktop And Mobile Manual

## Installed Targets

- Android admin app package: `com.aifred.admin`
- Windows desktop companion: `%LOCALAPPDATA%\Programs\AIFRED Admin\AIFRED-Admin-Desktop.ps1`
- Windows shortcut: Desktop `AIFRED Admin Desktop`

## Offline Access

Both admin clients support offline login.

Offline mode unlocks the app interface and local command/status tools without internet. Live website operations still require an online admin session because they call `https://www.north3rnlight3r.com/api/v1/...`.

## Online Admin Routes

- `POST /api/v1/admin/login`
- `GET /api/v1/admin/dashboard/state`
- `GET /api/v1/admin/sales/list` (read-only historical compatibility)
- `GET /api/v1/admin/reference/list`
- `GET /api/v1/admin/logs/list`
- `GET /api/v1/admin/inquiries/list`
- `GET /api/v1/admin/catalog/list`

## USB / ADB

ADB is installed at:

```text
%LOCALAPPDATA%\Android\Sdk\platform-tools\adb.exe
```

The platform-tools folder was added to the user PATH. New terminals should be able to run:

```powershell
adb devices -l
```

If the phone does not show up:

- Enable Developer Options on Android.
- Enable USB debugging.
- Reconnect USB.
- Accept the RSA authorization prompt on the phone.
- Run `adb kill-server`, then `adb devices -l`.

## Git Sync Expectations

Ordinary website activity and inquiries write to Cloudflare KV and never commit to GitHub. Explicit authenticated admin file/catalog operations may commit approved repository paths; production deployment is still a separate validated/manual action.

## Version 2.3.0 Local Command Registry

The mobile app merges backend allowlisted actions with local-only read-only/non-root commands. Local actions include `pwd`, `ls -la`, `df -h`, `du -sh .`, `id`, `uname -a`, `ps -A`, address/route inspection, sorted environment output, Termux package/info queries, Android version/package/log inspection, and the production health check.

Local actions execute inside the app shell on the phone and do not become backend commands. Arbitrary root operations are not added to the registry.

Current source repos:

- Plugin/release monorepo: `AIFRED_REPO_ROOT`
- Live website: `AIFRED_REPO_ROOT`
- Admin app: `AIFRED_REPO_ROOT`

The canonical plugin source is `plugin-aifred/` inside this monorepo. Historical references to a separate repository are not current authority.
