# Operation Log - 2026-05-01

## Completed

- Added offline-friendly Android admin behavior for log, sales, dashboard, inquiry, and reference-pool views.
- Added `adminReferenceLog()` Android client route for `/api/v1/admin/reference/list`.
- Added a Windows desktop admin companion using the same live admin routes.
- Added a Windows installer script that creates a Desktop shortcut.
- Added admin desktop/mobile manual and updated README release version references.
- Installed current AIFRED VST3 to the per-user VST3 path:
  `C:\Users\North\AppData\Local\Programs\Common\VST3\Aifred.vst3`
- Added ADB platform-tools to the user PATH:
  `%LOCALAPPDATA%\Android\Sdk\platform-tools`
- Removed stale local source repo:
  `C:\Users\North\Documents\Projects\aifred-plugin`
- Removed temporary AIFRED installer/check artifacts from `%TEMP%`.

## Not Completed / Blocked

- Android APK compile was not completed because Java/JDK was not available in PATH and the JDK install was interrupted.
- Android APK was not installed to the phone because `adb devices -l` did not show a connected device at the time of execution.
- System-wide stale VST/backend removal under `C:\Program Files` was attempted but Windows denied access. Remaining paths:
  `C:\Program Files\Common Files\VST3\Aifred.vst3`
  `C:\Program Files\Aifred`
- Large R2 upload of `AIFRED-VST3-Setup.exe` did not verify successfully; the Windows zip did upload and verify, and GitHub release fallback remains available.

## Current Source Of Truth

- Current plugin/release repo: `C:\Users\North\Documents\Projects\AIFRED`
- Live website repo: `C:\Users\North\Documents\Projects\aifred-site`
- Admin app repo: `C:\Users\North\Documents\Projects\aifred-admin`
- Current release tag: `v0.3.3-ollama-chat`
