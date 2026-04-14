# Admin App Guide

The Android admin app is private owner-only software. It is not published in the GitHub release and is not distributed publicly.

## Install

Build the APK locally:

```powershell
cd android_admin
.\gradlew.bat assembleDebug
```

Install over ADB:

```powershell
$adb=Join-Path $env:LOCALAPPDATA 'Android\Sdk\platform-tools\adb.exe'
& $adb install -r android_admin\app\build\outputs\apk\debug\app-debug.apk
```

## Login

The app supports offline admin login. If the website is unavailable, valid local owner credentials still unlock the admin runtime.

Do not store or publish the password in docs, screenshots, release notes, or issue comments.

## Main Tabs

Chat:

- Connects to the website chat API through WebSocket or HTTP.
- Uses `/ws/chat` and `/api/v1/chat/ask`.
- Can select backend models returned by `/api/v1/models/list`.
- OpenAI and Ollama routing are controlled by the website backend environment.

Upload:

- Upload catalog audio to the website repo.
- Upload licensed reference tracks.
- Upload website assets.
- File uploads require `GITHUB_TOKEN` in Cloudflare Pages so the backend can commit to the private repo.

Command:

- Runs registered backend commands through `/api/v1/command/run`.
- Runs local Android sandbox shell commands when logged in with local admin credentials.
- Shows output in the app terminal panel.

## Registered Backend Commands

| Command | Purpose |
| --- | --- |
| `health` | Check live backend health |
| `catalog:list` | Count catalog tracks |
| `models:list` | Show OpenAI/Ollama model routes |
| `reference:stats` | Show reference pool persistence status |
| `deploy:status` | Confirm production domain status |
| `deploy:site` | Dispatch GitHub Actions deployment when configured |

## Local Android Shell Examples

These commands run inside the Android app sandbox unless the device grants broader access:

```sh
pwd
ls
id
getprop ro.build.version.release
df -h
```

The app cannot bypass Android sandboxing or grant root. For full-device shell operations, use ADB from the workstation.

## Website File Access

The app can call:

- `POST /api/v1/admin/files/read`
- `POST /api/v1/admin/files/write`
- `GET /api/v1/admin/files/list`
- `POST /api/v1/admin/files/delete`
- `POST /api/v1/admin/files/upload`

Delete is restricted to `website/` paths. The backend rejects unsafe paths containing traversal or `.git/`.

## Ollama And OpenAI

The app uses the website backend model router by default:

- OpenAI requires `OPENAI_API_KEY`.
- Ollama requires `OLLAMA_BASE_URL`.
- The app can select models returned by `/api/v1/models/list`.

The app can also be built for direct provider access:

OpenAI direct:

```powershell
cd android_admin
.\gradlew.bat assembleDebug -PAIFRED_BASE_URL=https://api.openai.com/v1 -PAIFRED_API_TOKEN=sk-your-key
```

Ollama direct:

```powershell
cd android_admin
.\gradlew.bat assembleDebug -PAIFRED_BASE_URL=http://192.168.x.x:11434
```

For local Ollama from a phone, the phone must be able to reach the host running Ollama. Use the workstation LAN IP, not `localhost`, because `localhost` on Android means the phone itself.

Direct provider mode uses HTTP chat in the app. WebSocket streaming is reserved for the AIFRED website backend.
