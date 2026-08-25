# User Guide

## Website

The public site lives at:

- https://www.north3rnlight3r.com
- https://north3rnlight3r.com

The site contains free AIFRED VST3 beta downloads, free North3rnLight3r catalog MP3 downloads, a browser mix analyzer, contact form, release information, and download delivery.

The current website presents AIFRED as a **free beta download** with no checkout or account requirement.

## Beat Catalog

Catalog metadata lives at:

```text
apps/website/assets/data/beat_catalog.json
```

Audio streams use:

```text
/api/v1/assets/audio/catalog/<file>
```

The backend reads catalog audio from the `AIFRED_DOWNLOADS` Cloudflare R2 binding first and falls back to local static files during development when needed.

Each catalog entry can include title, genre, BPM, and stream URL or asset filename. Every card includes an explicit free MP3 download action.

While catalog audio plays, the analyzer canvas runs as a live visualizer. Uploaded-track analysis draws a diagnostic view after the browser decodes the selected audio file.

## Free Mix Analyzer

The browser analyzer accepts browser-supported audio files, computes a local diagnostic profile, and submits metadata to:

```text
POST /api/v1/analysis/submit
```

Measured metadata includes:

- Tone balance
- Integrated loudness estimate
- Peak level
- Crest factor
- Stereo width
- Low-end control
- Harshness control
- Transient density
- Spectral centroid

The backend applies the current analyzer gate. Accepted metadata can be stored when the optional reference-pool binding is configured. Audio does not need to be persisted for the public metadata gate.

## AIFRED VST3

AIFRED has three primary modes.

### Analyze

- One active mix route.
- Candlestick-style session and history metering.
- Mix signature panel.
- Tone, width, punch, loudness, RMS, peak, crest, correlation, transient, and related diagnostic measurements.
- Request-driven chat using current DSP/reference context.

### Reference

- One primary Halo.
- Genre target overlay.
- Default reference-pool ring.
- Up to five personal reference rings.
- Five independent reference file pickers.
- Five reference volume lanes.

### Compare

- Two independent Halo views.
- Separate Mix A and Mix B DSP routes.
- Tone, width, punch, loudness, and dynamics comparison.
- Center analog-style match VU.

In FL Studio, place AIFRED on the master or a bus, enable the Mix B sidechain input in the wrapper, then route the comparison track to that sidechain from the mixer.

## Metering Notes

- The Halo center can switch between multiband frequency lanes, waveform, and combined spectrometer view.
- Halo quadrants show readable scales and labels for tone, width/correlation, loudness, and punch.
- The current-session history meter shows one candle per minute for the latest 10 minutes.
- Loudness uses K-weighted LUFS-style measurements.
- Peak values use dBFS.
- Crest is shown in dB.
- Correlation is measured on a -1 to +1 scale after a 150 Hz high-pass so bass energy does not dominate the phase read.
- Chat has a dedicated scrollable output area and no hardcoded fix suggestions.
- The plugin header displays the AIFRED version so stale DAW scans are easier to detect.
- Reference, gate, and AI settings are persisted where appropriate.

## AI Routing

### Default local route

```text
AIFRED VST3
  -> AIFRED Engine at http://127.0.0.1:8787
  -> Ollama at http://127.0.0.1:11434
  -> aifred:latest
```

The local engine keeps network work off the audio thread. If the engine or Ollama is unavailable, the plugin continues metering and reports the missing dependency.

### OpenAI route

When OpenAI is selected and an API key is configured, the engine uses:

```text
https://api.openai.com/v1/responses
model: gpt-5.6-luna
```

The plugin normalizes an empty or local-only model choice to the OpenAI default when an OpenAI-compatible provider is selected.

## Windows Installation

Current Windows release artifacts:

- `AIFRED-VST3-Setup.exe`
- `AIFRED-Uninstall.exe`
- `AIFRED-VST3-windows.zip`

Primary install paths:

```text
C:\Program Files\Common Files\VST3\Aifred.vst3
C:\Program Files\Aifred\bin\AifredEngine.exe
C:\Program Files\Aifred\config\config.json
%AppData%\Aifred\user_settings.json
```

The installer configures the default Ollama route, registers engine startup, starts the engine silently, and checks local readiness.

## macOS Installation

Current macOS release artifact:

```text
AIFRED-VST3-macOS.pkg
```

Primary install paths:

```text
/Library/Audio/Plug-Ins/VST3/Aifred.vst3
/Library/Application Support/Aifred/bin/AifredEngine
/Library/Application Support/Aifred/config/config.json
/Library/Application Support/Aifred/setup-aifred-local-ai.sh
/Library/Application Support/Aifred/AIFRED Engine Control.command
/Library/LaunchAgents/com.aifred.engine.plist
~/Library/Application Support/Aifred/user_settings.json
```

The LaunchAgent starts the engine at login. Double-click `AIFRED Engine Control.command` to start or repair local AI, restart the engine, stop it for the current login session, or show gateway health.

The macOS package is not yet signed or notarized; that remains a release-hardening task.

## Free Downloads

The current website flow serves versioned release objects through the `AIFRED_DOWNLOADS` R2 binding. The bucket stays private; the Pages Worker provides the public download response and attachment headers.

Current backend routes include:

```text
GET /api/v1/downloads/plugin?asset=setup
GET /api/v1/downloads/plugin?asset=zip
GET /api/v1/assets/audio/catalog/<file>?download=1
```

The PayPal create, capture, IPN, and tokenized sale-download routes are not exposed while free distribution is active.

## Android Admin App

The Android admin app is owner-only operational software. It can manage chat, catalog uploads, website/repository files, activity, sales, inquiries, references, and registered backend commands.

It is not a public release artifact.
