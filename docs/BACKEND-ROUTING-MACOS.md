# AIFRED macOS Backend Routing

## Current local prerequisites

| Tool | Required for | Verified location |
| --- | --- | --- |
| Homebrew | Developer prerequisite install path | `/opt/homebrew/bin/brew` |
| .NET SDK | Build/publish `AifredEngine` | `/opt/homebrew/bin/dotnet` |
| Ollama | Local chat model runtime | `/opt/homebrew/bin/ollama` |
| pkgbuild | macOS installer package generation | `/usr/bin/pkgbuild` |
| AIFRED VST3 | Plugin payload | `build-mac/plugin-aifred/Aifred_artefacts/Release/VST3/Aifred.vst3` |

The Homebrew cask `dotnet-sdk` requires an interactive sudo installer on this Mac, so the local test environment uses the Homebrew `dotnet` formula instead.

## Install locations

Packaged macOS payload:

- Plugin: `/Library/Audio/Plug-Ins/VST3/Aifred.vst3`
- Engine binary: `/Library/Application Support/Aifred/bin/AifredEngine`
- Engine config: `/Library/Application Support/Aifred/config/config.json`
- Engine logs: `/Library/Application Support/Aifred/logs`
- Optional local model slot: `/Library/Application Support/Aifred/models/aifred-assistant-q4.gguf`
- LaunchAgent: `/Library/LaunchAgents/com.aifred.engine.plist`
- Manual control command: `/Library/Application Support/Aifred/AIFRED Engine Control.command`
- Repair/setup script: `/Library/Application Support/Aifred/setup-aifred-local-ai.sh`

Per-user engine settings are written to:

- `~/Library/Application Support/Aifred/user_settings.json`

## Runtime ports and endpoints

Plugin to engine:

- Gateway health: `GET http://127.0.0.1:8787/health`
- Gateway chat: `POST http://127.0.0.1:8787/chat`
- Gateway settings: `POST http://127.0.0.1:8787/v1/settings`
- Gateway settings readback: `GET http://127.0.0.1:8787/v1/settings`

Engine to Ollama:

- Endpoint base: `http://127.0.0.1:11434`
- Generate route: `POST http://127.0.0.1:11434/api/generate`
- Default model: `aifred:latest`
- Local base model used for this Mac test: `llama3.2:3b`

Engine to OpenAI-compatible fallback:

- Effective endpoint comes from `custom_endpoint` when provider override is enabled, otherwise from config.
- Chat route: `POST {custom_endpoint}/chat/completions`
- Auth: `Authorization: Bearer {api_key}` only when an API key is configured.

## Routing rules

The plugin talks to the local AIFRED gateway on `127.0.0.1:8787`. That is not the Ollama provider URL. The canonical local AI provider is Ollama at `http://127.0.0.1:11434` with model `aifred:latest`.

The engine resolves provider/model/endpoint from:

1. `~/Library/Application Support/Aifred/user_settings.json` when `provider_override_enabled` is `true`.
2. `/Library/Application Support/Aifred/config/config.json` when override is disabled.

If the effective provider contains `ollama`, or the endpoint contains `11434`, `/chat` calls Ollama `/api/generate`. Otherwise `/chat` calls the OpenAI-compatible `/chat/completions` route.

## Install and configuration

1. Install prerequisites on the Mac:

```sh
brew install dotnet ollama
brew services start ollama
```

2. Build and package from the repo:

```sh
cmake -S . -B build-mac -DCMAKE_BUILD_TYPE=Release
cmake --build build-mac --config Release --parallel
tools/macos/package-aifred-macos.sh
```

3. Install the package:

```sh
sudo installer -pkg dist/macos/AIFRED-VST3-macOS.pkg -target /
```

4. Confirm or repair local AI:

```sh
"/Library/Application Support/Aifred/setup-aifred-local-ai.sh"
curl -sS http://127.0.0.1:8787/health
```

The package writes `/Library/Application Support/Aifred/config/config.json` with Ollama at `http://127.0.0.1:11434`, model `aifred:latest`, and gateway `http://127.0.0.1:8787`. It also writes the active user's `~/Library/Application Support/Aifred/user_settings.json` with provider override disabled so the default local route is used until the plugin UI saves an OpenAI-compatible route.

## Startup, cancel, and manual restart

The installed LaunchAgent starts `AifredEngine` at login and keeps it alive for the logged-in user. Opening the plugin also attempts to start `ollama serve` and the installed engine if health checks fail.

Use the double-click command at `/Library/Application Support/Aifred/AIFRED Engine Control.command` to:

- start or repair local AI
- restart the engine
- stop the engine for the current login session
- show gateway health

Stopping from the control command unloads the LaunchAgent for the current login session and kills the engine process. It can be started again from the same command or by logging out and back in. To disable the login behavior persistently, remove or move `/Library/LaunchAgents/com.aifred.engine.plist` with administrator privileges.

## Manual verification commands

```sh
which brew
which dotnet
dotnet --info
which ollama
ollama --version
which pkgbuild
test -d build-mac/plugin-aifred/Aifred_artefacts/Release/VST3/Aifred.vst3
brew services start ollama
curl -sS http://127.0.0.1:11434/api/tags
ollama pull llama3.2:3b
ollama create aifred:latest -f /path/to/temporary/Modelfile
ollama list
ollama run aifred:latest "Say AIFRED local model ready in one short sentence."
dotnet publish tools/AifredEngine/AifredEngine.Mac.csproj -c Release -r osx-arm64 -o dist/engine/macos/osx-arm64
./dist/engine/macos/osx-arm64/AifredEngine
curl -sS http://127.0.0.1:8787/health
curl -sS -X POST http://127.0.0.1:8787/chat -H 'Content-Type: application/json' -d '{"message":"Say AIFRED engine chat ready in one short sentence.","context":{"analysis_snapshot":{"has_signal":true,"values_valid":true}}}'
curl -sS -X POST http://127.0.0.1:8787/v1/settings -H 'Content-Type: application/json' -d '{"provider_override_enabled":true,"provider_mode":"ollama","api_key":"","custom_endpoint":"http://127.0.0.1:11434","model_name":"aifred:latest"}'
curl -sS http://127.0.0.1:8787/v1/settings
tools/macos/package-aifred-macos.sh
pkgutil --payload-files dist/macos/AIFRED-VST3-macOS.pkg
```

## Known blockers and risks

- The package is not signed or notarized.
- The postinstall script configures Ollama only if `ollama` already exists on the target Mac; it does not install Ollama.
- Windows compatibility is preserved by platform-specific plugin networking, but Windows packaging still needs a separate regression pass on Windows.

## Next steps before beta

1. Confirm LaunchAgent bootstrap starts the engine after a real reboot.
2. Confirm the plugin reaches `/health`, `/chat`, and `/v1/settings` after a package install.
3. Decide whether the macOS installer should install Ollama, require it as a prerequisite, or bundle an approved local runtime.
4. Add signing and notarization once install behavior is verified.
