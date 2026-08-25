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
- Registered backend commands plus a separate local-only registry of non-root Linux/Termux/Android diagnostics.
- Historical sales, inquiries, activity, downloads, and upload visibility.
- Free catalog-distribution metadata; commercial licensing remains inquiry-based.

## Current Technical State

- Backend: `https://www.north3rnlight3r.com`
- App version: `2.3.0`
- Version code: `243` (kept above 241 so Android accepts it as an upgrade)
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

Linux/Termux-style host shell:

```sh
cd apps/admin-android
./gradlew :app:assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

Windows PowerShell:

```powershell
cd apps/admin-android
.\gradlew.bat assembleDebug
```

Install locally with ADB:

```powershell
$adb=Join-Path $env:LOCALAPPDATA 'Android\Sdk\platform-tools\adb.exe'
& $adb install -r app\build\outputs\apk\debug\app-debug.apk
```

The in-app local registry includes read-only/non-root commands for working directory, file listing, disk usage, identity, kernel details, visible processes, network state, environment, Termux package/info queries, Android version/packages/logs, and production API health. Local actions run on the phone; backend actions remain server allowlisted.

Catalog playback resolves the website API's relative `/api/v1/assets/audio/catalog/...` paths against `AIFRED_BASE_URL` before passing them to Android `MediaPlayer`. This keeps mobile playback on the same controlled R2-backed streaming routes as the website.

The Chat tab includes an API Configuration module with Website, Local Ollama, and OpenAI profiles. Operators can edit the endpoint and model, enter an API key when required, test discovery, and apply the profile without rebuilding the APK. The selected profile is stored in app-private storage with Android backup disabled; API keys are never written to the repository. A phone cannot reach a workstation's Ollama service through `127.0.0.1`, so use the workstation's reachable LAN address when Ollama is not running on the phone itself.

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
