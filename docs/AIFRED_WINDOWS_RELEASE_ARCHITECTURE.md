# AIFRED Release Architecture

AIFRED is a JUCE mix-analysis plugin with a local companion engine. The plugin remains the product: it performs DSP, metering, Halo rendering, and measured diagnosis inside the DAW. The companion engine explains the measured state and handles chat without running model work inside the host process.

## Project Structure

```text
plugin-aifred/
  Source/
    PluginProcessor.*
    PluginEditor.*
    AnalysisEngine.*
    DiagnosticInterpreter.*
tools/
  AifredEngine/
    AifredEngine.csproj
    Program.cs
  AifredWindowsInstaller/
    AifredWindowsInstaller.csproj
    Program.cs
  package-aifred.ps1
website/
android_admin/
```

Distributable repository layout:

```text
AIFRED/
  CMakeLists.txt
  CMakePresets.json
  plugin-aifred/
  tools/AifredEngine/
  tools/AifredWindowsInstaller/
  tools/package-aifred.ps1
  docs/
  .github/workflows/build.yml
```

## Component Responsibilities

| Component | Responsibility |
| --- | --- |
| `Aifred.vst3` | Real-time audio pass-through, DSP feature extraction, Halo UI, DSP diagnosis, request-driven local AI chat. |
| `AifredEngine.exe` | Localhost AI orchestration, session memory, settings, Ollama routing, future GGUF/llama.cpp bridge. |
| Bundled model | A compact GGUF model named `aifred-assistant-q4.gguf`, replaced by Ollama chat routing through the local engine. |
| Installer | Installs the VST3, engine, config, logs/model folders, startup registration, optional API endpoint settings, and health validation. |

## Local API Contract

Base URL:

```text
Gateway URL: http://127.0.0.1:8787
Ollama URL: http://127.0.0.1:11434
```

Endpoints:

- `GET /health`
- `POST /analyze`
- `POST /chat`
- `GET /v1/settings`
- `POST /v1/settings`
- `POST /v1/restart`

`/health` returns engine status, engine version, model availability, provider mode, and model path.

`/analyze` accepts structured DSP metrics and diagnosis data. It returns summary text, issue list, action list, confidence, and dominant diagnosis. The engine does not invent metrics.

`/chat` accepts a user message and structured context. It answers from the provided context and should not claim measurements that were not supplied.

## Windows Startup Strategy

AIFRED uses a per-user registry Run entry:

```text
HKCU\Software\Microsoft\Windows\CurrentVersion\Run
  AIFRED Engine = "C:\Program Files\Aifred\bin\AifredEngine.exe"
```

This avoids a service installer for the first Windows release, keeps startup repair simple, and does not require a terminal. The installer also starts the engine silently after copying files.

## Config Schema

Installed config:

```json
{
  "mode": "local",
  "port": 8787,
  "gateway_url": "http://127.0.0.1:8787",
  "provider": "ollama",
  "ollama_url": "http://127.0.0.1:11434",
  "model_path": "C:\\Program Files\\Aifred\\models\\aifred-assistant-q4.gguf",
  "openai_api_key": "",
  "custom_endpoint": "",
  "timeout_ms": 8000
}
```

User override:

```json
{
  "provider_override_enabled": false,
  "provider_mode": "ollama",
  "api_key": "",
  "custom_endpoint": "",
  "model_name": ""
}
```

## Plugin Fallback State Machine

| State | Trigger | User-facing behavior |
| --- | --- | --- |
| Core metering | Always available | Meters, Halo, LUFS, true peak, crest, width, spectrum, and measured diagnosis remain active. |
| Engine ready | `/health.ok = true` | AI chat can use local Ollama responses. |
| Engine unavailable | Connection refused, timeout, missing process | UI says `Local AI route unavailable`; no broken-state language. |
| Model missing | Engine reachable but `model_loaded = false` | Engine keeps metering active and reports model routing status; chat requires Ollama or a configured API route. |
| Provider override error | Bad key or endpoint | Engine keeps metering active and reports provider status in settings/logs. |

## Installer Design

The Windows installer is a single self-contained `AIFRED-VST3-Setup.exe`.

It performs these actions:

1. Copies `Aifred.vst3` to `C:\Program Files\Common Files\VST3\Aifred.vst3` when run elevated.
2. Copies engine files to `C:\Program Files\Aifred\bin`.
3. Creates `C:\Program Files\Aifred\config`, `models`, and `logs`.
4. Writes default `config.json`.
5. Registers `AifredEngine.exe` at user login.
6. Starts the engine without opening a console window.
7. Verifies Ollama at `http://127.0.0.1:11434` and model `aifred:latest`, then calls gateway health at `GET http://127.0.0.1:8787/health`.
8. Shows a normal installer success dialog with installed paths and engine status.

Normal users do not run scripts, install Python, install Node, install Docker, install Ollama, or edit PATH.

## Model Runtime Integration

Preferred runtime:

- llama.cpp compiled as a Windows-native dependency or embedded library used by `AifredEngine.exe`.
- Bundled quantized GGUF file at `C:\Program Files\Aifred\models\aifred-assistant-q4.gguf`.
- Engine prompt must always include structured metrics and diagnosis. The model explains the supplied facts; it is not a measurement source.

Current scaffold:

- `AifredEngine.exe` exposes the API and Ollama chat routing behavior.
- `model_loaded` is `true` only when the GGUF file exists.
- Public installer production must add the licensed GGUF model before release.

## First-Pass Folder Layout

```text
C:\Program Files\Common Files\VST3\Aifred.vst3
C:\Program Files\Aifred\bin\AifredEngine.exe
C:\Program Files\Aifred\config\config.json
C:\Program Files\Aifred\models\aifred-assistant-q4.gguf
C:\Program Files\Aifred\logs\engine.log
%AppData%\Aifred\user_settings.json
```

## Plugin-To-Engine Connection

The plugin should only call the engine from a background/UI-safe path:

```cpp
// Audio thread:
analysisEngine.pushAudioBlock(buffer);

// UI/timer/background path:
auto contextJson = DiagnosticInterpreter::instance().update(state, ...).aiContextJson;
// POST contextJson to the AIFRED gateway at http://127.0.0.1:8787/analyze with timeout.
// If request fails, keep DiagnosticInterpreter metrics-only output.
```

Never call HTTP, start processes, or wait on model output from `processBlock`.




