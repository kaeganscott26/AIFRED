# Changelog

## Unreleased - AIFRED Windows Release Architecture

- Added `AifredEngine.exe`, a Windows companion process that exposes `http://127.0.0.1:8787`.
- Added engine endpoints for `/health`, `/analyze`, `/chat`, `/v1/settings`, and `/v1/restart`.
- Added deterministic engine fallback coaching so AIFRED remains useful when the bundled model is missing.
- Added plugin-side local engine health checks through WinHTTP, off the audio thread.
- Added `DiagnosticInterpreter` as the DSP-to-UI/context bridge with a rolling 5-second mix-health buffer.
- Added momentary LUFS, short-term LUFS, integrated LUFS, loudness range, and estimated 4x true peak fields.
- Updated Windows packaging so the installer payload contains the VST3, engine, config, logs/model folders, and no manual install script.
- Updated the Windows installer to request administrator elevation, install to the standard VST3 path, register engine startup, start the engine silently, and validate `/health`.
- Reworked the website reference gate to classify material as Strong Reference, Usable Reference, Style-Specific Reference, Technically Hot Reference, Poor Reference, or Reject.
- Removed duplicate `apps/website` shadow source and removed archived conversation/export files that are not required for the ecosystem runtime.

Known release blocker:

- The licensed bundled GGUF model file is not present in the repo. `AifredEngine.exe` correctly reports `model_loaded:false` and falls back to deterministic coaching until `C:\Program Files\Aifred\models\aifred-assistant-q4.gguf` is included in the installer payload.
