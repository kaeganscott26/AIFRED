# Admin App Guide

The Android admin app is private, owner-only operational software under `apps/admin-android/`. It is not part of the public AIFRED release and is not attached to public GitHub releases.

## Current App State

- Application ID: `com.aifred.admin`
- App version: `2.3.0`
- Version code: `243`
- `compileSdk = 35`
- `targetSdk = 35`
- Minimum SDK: 29
- Java/Kotlin JVM target: 17
- UI: Jetpack Compose
- Networking: OkHttp
- Async work: Kotlin coroutines

## Build And Install

Build the debug APK locally:

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

Install over ADB:

```powershell
$adb=Join-Path $env:LOCALAPPDATA 'Android\Sdk\platform-tools\adb.exe'
& $adb install -r app\build\outputs\apk\debug\app-debug.apk
```

The app is intended for private local installation on the owner's device. The current public release workflow does not publish an admin APK.

## Login

The app supports offline-aware owner login. Valid local owner credentials can unlock the app even when the website backend is unavailable.

Do not publish credentials in documentation, screenshots, release notes, or issue comments.

## Main Areas

### Chat

- Connects to the website chat API through WebSocket or HTTP.
- Uses `/ws/chat` and `/api/v1/chat/ask`.
- Reads model choices from `/api/v1/models/list`.
- Current model catalog can include `aifred:latest` and `gpt-5.6-luna` depending on backend/provider configuration.

### Upload

- Upload catalog audio and metadata.
- Upload licensed reference tracks.
- Upload website assets.
- Keep catalog metadata and uploaded audio aligned.
- Surface current download/upload/inquiry activity plus read-only historical sale/payment events.

Catalog audio is served from the `AIFRED_DOWNLOADS` R2 binding first when configured. The repository catalog JSON remains under `apps/website/assets/data/beat_catalog.json`.

### Command

- Runs registered backend commands through `/api/v1/command/run`.
- Merges those with local-only read-only/non-root Linux/Termux/Android actions.
- Shows output in the app terminal panel.

## Registered Backend Commands

| Command | Purpose |
| --- | --- |
| `health` | Check live backend health |
| `catalog:list` | Count catalog tracks |
| `models:list` | Show configured model routes |
| `reference:stats` | Show reference-pool persistence status |
| `deploy:status` | Confirm production domain status |
| `sales:list` | Show historical sale records |
| `inquiries:list` | Show contact inquiries |

## Local Android Shell

Version 2.3.0 registers these command families inside the Android app sandbox:

```sh
pwd
ls -la
df -h
du -sh .
id
uname -a
ps -A
ip addr
ip route
printenv | sort
pkg list-installed
termux-info
getprop ro.build.version.release
pm list packages -3
logcat -d -t 100
curl -fsS https://www.north3rnlight3r.com/api/v1/health
```

Unavailable Termux commands return a clear message. The registry does not request root or bypass Android sandboxing. Use ADB from the workstation for broader device shell access.

Catalog playback uses the same `/api/v1/assets/audio/catalog/...` streaming URLs returned to the website. Relative API paths are resolved against the configured production base URL before Android opens them; they are never treated as local filesystem paths.

## API Configuration

The Chat tab provides three switchable profiles:

- `website`: the production AIFRED API and its configured local/OpenAI routing.
- `ollama`: direct Ollama access, defaulting to `http://127.0.0.1:11434` and `aifred:latest`.
- `openai`: direct OpenAI access, defaulting to `https://api.openai.com/v1` and `gpt-5.6-luna`.

Edit the endpoint/model, provide a key when the selected provider requires one, use **Test API**, then use **Apply + Save**. The profile is stored only in the app's private preferences and Android backup is disabled. No API key belongs in Gradle files, tracked examples, documentation, screenshots, or Git history.

On Android, `127.0.0.1` means the phone. To use Ollama on a workstation or Termux host, enter an HTTP endpoint reachable from the phone on the same trusted network and configure that service's bind/firewall controls deliberately.

## Website File Access

The app can call:

- `POST /api/v1/admin/files/read`
- `POST /api/v1/admin/files/write`
- `GET /api/v1/admin/files/list`
- `POST /api/v1/admin/files/delete`
- `POST /api/v1/admin/files/upload`

Delete operations are restricted to approved `apps/website/` paths. Unsafe traversal and `.git/` paths are rejected by the backend.

## Model Routing

The normal app path uses the website backend model router:

- Local model: `aifred:latest`
- OpenAI default: `gpt-5.6-luna`
- OpenAI endpoint: `https://api.openai.com/v1/responses`
- Ollama endpoint: `http://127.0.0.1:11434` when run locally on the host machine

When accessing Ollama from a phone, use the workstation's reachable LAN address rather than `localhost`, because `localhost` on Android means the phone itself.

## Build Configuration

Local app settings can be supplied through Gradle project properties or `local.properties`. The repository `.gitignore` excludes the canonical Android build/cache paths and `apps/admin-android/local.properties`.

Do not commit private local configuration.

## Release Hardening Note

The current `release` build type uses the debug signing configuration and has minification disabled. That is acceptable for the current private owner-only workflow, but a future production-distribution decision should use a dedicated signing configuration and explicit release hardening.
