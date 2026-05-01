# Changelog

## v0.3.4-chat-scroll

- Replaced the chat answer label with a scrollable read-only chat output window.
- Cleaned model text before display so JSON wrappers and code fences do not appear as the visible answer.
- Changed the engine prompt to keep chat open-ended while still using DAW audio context when relevant.
- Removed visible theme/layout options and forced the plugin to the Chat Focus layout.
- Added five independent reference file buttons for the five reference mixer lanes.
- Added a Windows uninstaller release artifact and updated the installer to remove prior backend files/settings before installing the current backend.

## v0.3.3-ollama-chat - AIFRED Windows Release Architecture

- Added `AifredEngine.exe`, a Windows companion process that exposes `http://127.0.0.1:8787`.
- Added engine endpoints for `/health`, `/analyze`, `/chat`, `/v1/settings`, and `/v1/restart`.
- Replaced canned coaching text with request-only Ollama chat routed through `aifred:latest`.
- Added plugin-side local engine health checks through WinHTTP, off the audio thread.
- Updated `DiagnosticInterpreter` so it prepares DSP and reference-pool context for the model without emitting canned advice.
- Added momentary LUFS, short-term LUFS, integrated LUFS, loudness range, and estimated 4x true peak fields.
- Updated Windows packaging so the installer payload contains the VST3, engine, config, logs/model folders, and no manual install script.
- Updated the Windows installer to request administrator elevation, install to the standard VST3 path, register engine startup, start the engine silently, and validate `/health`.
- Reworked the website reference gate to classify material as Strong Reference, Usable Reference, Style-Specific Reference, Technically Hot Reference, Poor Reference, or Reject.
- Removed duplicate `apps/website` shadow source and removed archived conversation/export files that are not required for the ecosystem runtime.

Release verification:

- GitHub Actions builds and publishes Windows, macOS, Linux, and Arch release packages from `v0.3.3-ollama-chat`.
- The Windows installer configures the local Ollama route. OpenAI-compatible API settings remain user-supplied because a distributable installer cannot know a customer's private API key.
