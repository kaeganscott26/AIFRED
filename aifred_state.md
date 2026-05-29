# AIFRED Development State

Logged: 2026-05-27 23:08:41 CDT

## Current Goal

Make the macOS AIFRED beta path match the Windows one-click local backend experience as closely as possible, while preserving Windows behavior and avoiding fake success states.

The current focus is the macOS VST3 plus local AIFRED Engine path:

- Plugin talks to the local AIFRED gateway on `127.0.0.1:8787`.
- The canonical local AI provider is Ollama on `http://127.0.0.1:11434`.
- Default local model is `aifred:latest`.
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

## Plugin Stabilization Log

Logged: 2026-05-28 CDT

Scope rules for this pass:

- Do not touch installer/package scripts.
- Do not touch `aifred-site`.
- Complete one plugin stabilization task at a time.
- Run `git diff --check` and `cmake --build build-mac --config Release` before moving to the next task.
- If visible UI is not backed by real data, either bind it to real data or make the unavailable state explicit.

Completed tasks:

1. Chatbox layout collision
   - Changed `plugin-aifred/Source/PluginEditor.cpp`.
   - Root cause: `resized()` positioned chat components from a different right-column rectangle than `paint()` used to draw the chat panel.
   - Fix: component bounds now reserve the left column before taking the right-column chat panel area, matching the painted panel geometry.
   - Validation: `git diff --check` passed; `cmake --build build-mac --config Release` passed.

2. Tone/Width meter binding truthfulness
   - Changed `plugin-aifred/Source/AnalysisEngine.cpp`.
   - Root cause: bars used populated `state.metrics.tone01` and `state.metrics.width01`, while labels read unpopulated `state.metrics.spectralTilt` and `state.metrics.stereoWidth`.
   - Fix: `buildHaloState()` now propagates `smoothed_.spectralTilt` and `smoothed_.stereoWidth` into both metric aliases used by the UI.
   - Validation: `git diff --check` passed; `cmake --build build-mac --config Release` passed.

3. Punch meter truthfulness
   - Changed `plugin-aifred/Source/PluginEditor.cpp`.
   - Root cause: the Punch bar was driven by `state.metrics.punch01` / `smoothed_.transientDensity`, while the main Punch label displayed `state.metrics.crestDb`.
   - Fix: the main Punch label now displays `state.metrics.punch01`, matching the bar source; crest remains secondary context in the footer.
   - Validation: `git diff --check` passed; `cmake --build build-mac --config Release` passed.

4. Loudness/LUFS meter truthfulness
   - Changed `plugin-aifred/Source/AnalysisEngine.cpp` and `plugin-aifred/Source/PluginEditor.cpp`.
   - Root cause: the Loudness bar used `state.metrics.loudness01` derived from smoothed RMS, while the main label displayed integrated LUFS and the footer described short-term LUFS.
   - Fix: the visible Loudness card primary value is now short-term LUFS: `state.metrics.loudness01` is derived from `smoothed_.shortTermLufs`, and the main label displays `state.metrics.shortTermLufs`.
   - Validation: `git diff --check` passed; `cmake --build build-mac --config Release` passed.

5. Correlation meter truthfulness
   - Changed `plugin-aifred/Source/PluginEditor.cpp`.
   - Root cause: correlation defaults to `1.0f`, so the standalone correlation meter could draw a valid-looking locked meter while no signal was present.
   - Fix: `drawCorrelationMeter()` now gates the value marker/fill on `state.hasSignal && state.valuesValid`; idle or invalid windows display `corr unavailable`.
   - Validation: `git diff --check` passed; `cmake --build build-mac --config Release` passed.

6. Spectrometer truthfulness
   - Changed `plugin-aifred/Source/PluginEditor.cpp`.
   - Root cause: the standalone spectrometer always drew a minimum 2px fill for every band, which could make silence or invalid analysis look like measured energy.
   - Fix: `drawSpectrumMeter()` now draws fills only when `state.hasSignal && state.valuesValid`; unavailable windows keep empty slots and display `spectrum unavailable`.
   - Validation: `git diff --check` passed; `cmake --build build-mac --config Release` passed.

7. Candlestick/dynamics meter truthfulness
   - Changed `plugin-aifred/Source/PluginEditor.cpp`.
   - Root cause: the current candlestick display could render default zero-valued candle geometry while idle or invalid, which made an unavailable window look like real loudness history.
   - Fix: `drawCandles()` now shows `current candle unavailable` and placeholder O/C/delta text when there is no valid signal; `drawCandleStrip()` now labels empty minute/session history instead of drawing no-context empty plots.
   - Validation: `git diff --check` passed; `cmake --build build-mac --config Release` passed.

8. Halo data truthfulness
   - Changed `plugin-aifred/Source/PluginEditor.cpp`.
   - Root cause: `drawHalo()` could render metric rings, scale labels, embedded spectrum, and correlation labels from default/stale values when no valid signal was available.
   - Fix: `drawHalo()` now gates the full metric visualization on `state.hasSignal && state.valuesValid`; unavailable windows show `waiting for signal` or `analysis unavailable`.
   - Validation: `git diff --check` passed; `cmake --build build-mac --config Release` passed.

9. Mix Signature truthfulness
   - Changed `plugin-aifred/Source/PluginEditor.cpp`.
   - Root cause: the live Mix Signature polygon used `metricValue()`, which changes meaning when a reference is loaded, and it could draw a collapsed/default signature when analysis was unavailable.
   - Fix: `drawMixSignature()` now gates drawing on `state.hasSignal && state.valuesValid` and uses raw live metrics (`tone01`, `width01`, `punch01`, `loudness01`) for the live polygon.
   - Validation: `git diff --check` passed; `cmake --build build-mac --config Release` passed.

10. Local engine status display correctness
   - Changed `plugin-aifred/Source/PluginEditor.cpp`.
   - Root cause: the chat panel footer forced `Local AI ready.` whenever the cached health flag was true, hiding more specific client status text such as failed chat/settings requests.
   - Fix: `drawChatPanel()` now always displays `AifredEngineClient::statusText()`; color still reflects the current health availability flag.
   - Validation: `git diff --check` passed; `cmake --build build-mac --config Release` passed.

11. Backend/chat availability state correctness
   - Changed `plugin-aifred/Source/AifredEngineClient.cpp`, `plugin-aifred/Source/AifredEngineClient.h`, and `plugin-aifred/Source/PluginEditor.cpp`.
   - Root cause: `askAsync()` silently ignored duplicate chat requests while the editor still displayed a pending readout, and failed chat/settings requests did not clear the cached availability flag.
   - Fix: `askAsync()` now returns whether a request actually started; the editor reports `Chat request already in progress` when appropriate; failed chat/settings routes set availability false.
   - Validation: `git diff --check` passed; `cmake --build build-mac --config Release` passed.

12. Responsive layout and DPI scaling
   - Changed `plugin-aifred/Source/PluginEditor.cpp`.
   - Root cause: `updateUiScale()` multiplied window-size scaling by display DPI scale even though JUCE component bounds are already logical coordinates, which increased clipping/overlap risk on high-DPI displays.
   - Fix: layout/font/padding scale now derives from editor logical size only, preserving existing min/max clamps.
   - Validation: `git diff --check` passed; `cmake --build build-mac --config Release` passed.

13. Text clipping and label readability
   - Changed `plugin-aifred/Source/PluginEditor.cpp`.
   - Root cause: several long status/legend/footer strings used single-line `drawText()`, which can clip at minimum editor size or with platform font differences.
   - Fix: converted header reference status, domain footer text, compare status, and correlation legends to `drawFittedText()`.
   - Validation: `git diff --check` passed; `cmake --build build-mac --config Release` passed.

14. Visible placeholder audit
   - Changed `plugin-aifred/Source/PluginEditor.cpp`.
   - Root cause: visible copy still included unfinished wording: chat file selection said file analysis was "not wired yet," and the reference mixer said target analysis was pending despite reference-file analysis being implemented.
   - Fix: chat file status now states selected files are not analyzed and chat uses the live mix snapshot; reference mixer copy now states analyzed targets feed reference mode.
   - Validation: `git diff --check` passed; `cmake --build build-mac --config Release` passed.

Current plugin stabilization status:

- The 12-task ordered plugin stabilization pass requested on 2026-05-28 has been completed.
- Each task was tested with `git diff --check` and `cmake --build build-mac --config Release` before moving to the next task.
- No installer/package scripts were touched.
- `aifred-site` was not touched.
- DSP analysis math was only changed where required to align visible meter bindings with existing real analysis values.
- Remaining release risk: Windows runtime regression has not been executed in this macOS environment.
