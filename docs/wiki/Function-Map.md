# Function Map

## VST3 Plugin

| File | Responsibility |
| --- | --- |
| `plugin-aifred/Source/PluginProcessor.*` | Audio bus setup, process block routing, mode state, Mix A/Mix B analysis snapshots |
| `plugin-aifred/Source/AnalysisEngine.*` | DSP metric extraction, smoothing, Halo state generation |
| `plugin-aifred/Source/HaloState.*` | Shared analysis structures, severity/confidence helpers |
| `plugin-aifred/Source/PluginEditor.*` | Neon GUI, Analyze/Reference/Compare views, overlays, metric rendering |
| `plugin-aifred/Source/AifredLookAndFeel.*` | Branded JUCE control styling |

## Website

| File | Responsibility |
| --- | --- |
| `website/index.html` | Public page structure |
| `website/styles.css` | Brand styling, analyzer, catalog, release download layout |
| `website/app.js` | Catalog player, PayPal links, analyzer DSP, chat form, metadata gate submission |
| `website/config.js` | Public runtime config and release URLs |
| `website/_worker.js` | Cloudflare Pages Worker router |
| `website/functions/api/v1/[[path]].js` | Main backend and admin API |
| `website/functions/ws/chat.js` | WebSocket chat bridge |

## Android Admin App

| Function/Class | Responsibility |
| --- | --- |
| `MainActivity` | Android entry point |
| `AIFREDAdminApp` | App state, login, tab routing, client setup |
| `ChatScreen` | Chat UI, model selector, memory controls |
| `UploadScreen` | Catalog/reference/website asset upload UI |
| `CommandScreen` | Backend action buttons and local shell terminal |
| `ApiClient` | HTTP, WebSocket, admin file, upload, command, catalog, and chat API calls |
| `localAdminLogin` | Offline owner login validation |
| `runLocalShellCommand` | Android sandbox command execution |

## Backend Functions

| Function | Responsibility |
| --- | --- |
| `readJson` | Safe JSON body parse |
| `createAdminSession` | Signed session token creation |
| `verifyAdmin` | Admin bearer token validation |
| `loadCatalog` | Catalog metadata load |
| `proGate` | Analyzer professional range decision |
| `handleAnalysisSubmit` | Analyzer metadata persistence/disposal |
| `askOpenAI` | OpenAI backend call |
| `askOllama` | Ollama backend call |
| `handleChat` | Chat provider routing |
| `chatSettingsPayload` | Settings shape for app/backend |
| `commandCatalog` | Registered admin command definitions |
| `githubRequest` | GitHub Contents/Actions API wrapper |
| `handleAdminFileRead` | Read website/repo files |
| `handleAdminFileWrite` | Commit website/repo text updates |
| `handleAdminFileList` | List GitHub-backed repo paths |
| `handleAdminFileDelete` | Delete website paths |
| `handleAdminFileUpload` | Upload binary website assets |
| `handleAdminReferenceUpload` | Upload reference pool files |
| `handleAdminCatalogUpload` | Upload catalog audio and append catalog metadata |
| `dispatchDeploy` | Dispatch GitHub Actions deployment |
| `handleCommand` | Execute allowlisted admin commands |
| `handleAdminLogin` | Online admin login |

