# AIFRED Development State

Logged: 2026-05-27 23:08:41 CDT

## Current Goal

Make the macOS AIFRED beta path match the Windows one-click local backend experience as closely as possible, while preserving Windows behavior and avoiding fake success states.

The current focus is the macOS VST3 plus local AIFRED Engine path:

- Plugin talks to local engine on `127.0.0.1:8787`.
- Engine talks to Ollama on `127.0.0.1:11434`.
- Default local model is `aifred:latest`.
- macOS package generation exists and has been tested, but install/sign/notarize validation is not complete.

## Current Repo State

Modified files currently shown by `git status --short`:

- `plugin-aifred/Source/AifredEngineClient.cpp`
- `plugin-aifred/Source/AifredEngineClient.h`
- `plugin-aifred/Source/AnalysisEngine.cpp`
- `plugin-aifred/Source/AnalysisEngine.h`
- `plugin-aifred/Source/HaloState.h`
- `plugin-aifred/Source/PluginEditor.cpp`
- `plugin-aifred/Source/PluginEditor.h`
- `plugin-aifred/Source/PluginProcessor.cpp`
- `plugin-aifred/Source/PluginProcessor.h`
- `tools/AifredEngine/Program.cs`

Untracked files currently shown by `git status --short`:

- `docs/BACKEND-ROUTING-MACOS.md`
- `tools/AifredEngine/AifredEngine.Mac.csproj`
- `tools/macos/`

Generated local artifacts:

- `dist/macos/AIFRED-VST3-macOS.pkg`
- Package size at last check: 28 MB
- This package was generated and inspected, but not installed.

## Verified Working

macOS VST3 build:

- `cmake --build build-mac --config Release` passes.

Plugin backend routing:

- macOS plugin client now supports local engine calls.
- Windows behavior remains guarded separately and was not intentionally removed.
- Plugin routes:
  - `GET http://127.0.0.1:8787/health`
  - `POST http://127.0.0.1:8787/chat`
  - `POST http://127.0.0.1:8787/v1/settings`

Local machine prerequisites:

- Homebrew present at `/opt/homebrew/bin/brew`.
- .NET SDK installed via Homebrew formula at `/opt/homebrew/bin/dotnet`.
- Ollama installed via Homebrew at `/opt/homebrew/bin/ollama`.
- `pkgbuild` present at `/usr/bin/pkgbuild`.

Ollama:

- Ollama service starts with `brew services start ollama`.
- Ollama responds on `http://127.0.0.1:11434`.
- `llama3.2:3b` is pulled locally.
- `aifred:latest` has been created locally as an Ollama model alias.
- `ollama run aifred:latest` returns a natural assistant response.

AIFRED Engine:

- macOS engine publishes from `tools/AifredEngine/AifredEngine.Mac.csproj`.
- Manual engine run works when a staged config exists.
- `/health` verified.
- `/chat` verified through Ollama.
- `/v1/settings` verified for provider/model/endpoint update.

macOS package:

- `tools/macos/package-aifred-macos.sh` completes.
- `dist/macos/AIFRED-VST3-macOS.pkg` is generated.
- Package payload inspection confirms it contains:
  - `/Library/Audio/Plug-Ins/VST3/Aifred.vst3`
  - `/Library/Application Support/Aifred/bin/AifredEngine`
  - `/Library/Application Support/Aifred/config/config.json`
  - `/Library/Application Support/Aifred/models/README.txt`
  - `/Library/Application Support/Aifred/logs`
  - `/Library/LaunchAgents/com.aifred.engine.plist`
  - postinstall script

Validation checks:

- `git diff --check` passes.
- `sh -n tools/macos/package-aifred-macos.sh` passes.
- `sh -n tools/macos/postinstall` passes.
- `plutil -lint tools/macos/com.aifred.engine.plist` passes.

## Backend Routing

Plugin to engine:

```text
AIFRED VST3
  -> GET  http://127.0.0.1:8787/health
  -> POST http://127.0.0.1:8787/chat
  -> POST http://127.0.0.1:8787/v1/settings
```

Engine to Ollama:

```text
AifredEngine
  -> POST http://127.0.0.1:11434/api/generate
  -> model: aifred:latest
```

Engine to OpenAI-compatible fallback:

```text
AifredEngine
  -> POST {custom_endpoint}/chat/completions
  -> optional Authorization: Bearer {api_key}
```

Settings resolution:

```text
~/Library/Application Support/Aifred/user_settings.json
  used when provider_override_enabled = true

/Library/Application Support/Aifred/config/config.json
  used when provider_override_enabled = false
```

More detail is documented in `docs/BACKEND-ROUTING-MACOS.md`.

## Current Blockers

The generated macOS package has not been installed yet.

The LaunchAgent has not been validated after a real package install and login/session bootstrap.

The package is not signed or notarized.

The package postinstall configures Ollama only if `ollama` already exists on the target Mac. It does not install Ollama. A product decision is still needed:

- require Ollama as a prerequisite,
- install Ollama through a trusted path,
- or bundle an approved local runtime.

Direct manual engine execution from `dist/engine/macos/osx-arm64` fails without a staged parent `config/config.json`, because the engine falls back to `/Library/Application Support/Aifred` and lacks permission. The packaged install path should avoid this because config is installed under `/Library/Application Support/Aifred/config/config.json`.

Windows build/package regression has not been rerun on Windows after the cross-platform changes.

Plugin metering/UI audit work identified previous fake/placeholder concerns, but the full beta-quality pass should still be reviewed before release.

## Known Risks

macOS package install may expose permission or LaunchAgent ownership issues that are not visible from payload inspection.

The current LaunchAgent is installed at `/Library/LaunchAgents/com.aifred.engine.plist`; real install testing must confirm it bootstraps correctly for the active GUI user.

Ollama model setup can be slow on a first install because `llama3.2:3b` is about 2 GB.

The local `aifred:latest` model was created from a temporary Modelfile because no project Modelfile currently exists.

The VST3 package is unsigned. Some hosts or macOS security settings may reject or quarantine it until signing/notarization is handled.

## Next Best Steps For Tomorrow

1. Review and commit the current backend/macOS package preparation changes only after confirming the diff is intentional.

2. Install `dist/macos/AIFRED-VST3-macOS.pkg` on a disposable macOS user profile or VM.

3. After install, verify package-installed paths:

```sh
test -d "/Library/Audio/Plug-Ins/VST3/Aifred.vst3"
test -x "/Library/Application Support/Aifred/bin/AifredEngine"
test -f "/Library/Application Support/Aifred/config/config.json"
test -f "/Library/LaunchAgents/com.aifred.engine.plist"
```

4. Verify LaunchAgent behavior:

```sh
launchctl print gui/$(id -u)/com.aifred.engine
curl -sS http://127.0.0.1:8787/health
```

5. Verify installed engine chat path:

```sh
curl -sS -X POST http://127.0.0.1:8787/chat \
  -H 'Content-Type: application/json' \
  -d '{"message":"Say AIFRED installed engine ready in one short sentence.","context":{"analysis_snapshot":{"has_signal":true,"values_valid":true}}}'
```

6. Open the installed VST3 in a DAW and verify:

- plugin loads,
- backend status shows available,
- chat reaches the local engine,
- settings save reaches `/v1/settings`,
- no UI overlap blocks the chat or meters.

7. Decide the Ollama distribution policy before calling the macOS installer beta-ready.

8. Run a Windows regression pass before merging/package release:

- Windows plugin build,
- Windows installer build,
- Windows engine `/health`,
- Windows engine `/chat`,
- Windows settings route,
- existing Windows one-click behavior.

9. Only after install behavior is verified, start signing/notarization work for macOS.

## Do Not Claim Yet

Do not claim the macOS installer is beta-ready yet.

Do not claim one-click macOS setup is complete until a fresh package install proves:

- plugin installed,
- engine installed,
- LaunchAgent starts engine,
- Ollama is present and responding,
- `aifred:latest` exists,
- `/health` works,
- `/chat` works,
- plugin reaches engine from inside a DAW.
