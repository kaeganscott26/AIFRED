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
- `GET /api/v1/admin/sales/list`
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

The admin clients do not bypass GitHub. Website/admin edits should go through the live backend or local repo scripts, then commit and push to GitHub.

Current source repos:

- Plugin/release monorepo: `C:\Users\North\Documents\Projects\AIFRED`
- Live website: `C:\Users\North\Documents\Projects\aifred-site`
- Admin app: `C:\Users\North\Documents\Projects\aifred-admin`

Stale source repo:

- `C:\Users\North\Documents\Projects\aifred-plugin` is the old VSTGUI line and should not be used for current release work.
