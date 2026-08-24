# AIFRED Admin | Private Android Management Console

The AIFRED Android admin app is owner-only operational software for the North3rnLight3r website/backend and catalog.

It lives at:

```text
apps/admin-android/
```

## Current Responsibilities

- Authenticated admin access.
- Website/backend chat and model selection.
- Catalog audio and metadata uploads.
- Licensed reference uploads.
- Website/repository file operations through approved backend routes.
- Registered backend commands.
- Sales, inquiries, activity, downloads, and upload visibility.
- Local Android sandbox shell tools.

## Current Technical State

- Backend: `https://www.north3rnlight3r.com`
- App version: `2.4.2`
- Version code: `241`
- `compileSdk = 35`
- `targetSdk = 35`
- Minimum SDK: 29
- JVM target: 17
- UI: Jetpack Compose
- Networking: OkHttp
- Async work: Kotlin coroutines

The AI client uses the OpenAI-compatible API base contract. Configure the host/root with `AIFRED_BASE_URL`; the client appends `/v1` routes such as `/v1/models` and `/v1/chat/completions`.

Current model routes can include:

- `aifred:latest`
- `gpt-5.6-luna`

## Local Build

```powershell
cd apps/admin-android
.\gradlew.bat assembleDebug
```

Install locally with ADB:

```powershell
$adb=Join-Path $env:LOCALAPPDATA 'Android\Sdk\platform-tools\adb.exe'
& $adb install -r app\build\outputs\apk\debug\app-debug.apk
```

## Distribution

The app is private and owner-only.

Current policy:

- Keep source in the private AIFRED repository.
- Build/install locally for the owner's device.
- Do not attach the APK to public GitHub releases.
- Do not publish the app through a public app-store listing.

The current `release` build type uses the debug signing configuration and has minification disabled, so it should not be treated as a hardened public production build.

## Security

Do not commit or publish private local configuration, owner credentials, or provider/deployment values.

The app can use offline-aware owner login and authenticated backend routes. Access should remain restricted to the owner.
