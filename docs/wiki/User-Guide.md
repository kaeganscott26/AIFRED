# User Guide

## Website

The public site lives at:

- https://www.north3rnlight3r.com
- https://north3rnlight3r.com

The website sells the AIFRED VST3 and North3rnLight3r beat catalog. Visitors can preview catalog audio, license beats through PayPal links, download VST release packages, and run the free mix analyzer.

The public site keeps the approved AIFRED mascot as the brand logo and uses `Website asset album art (2).jpg` plus `Brand Mascot Hero.jpg` as beat catalog artwork.

## Beat Catalog

The catalog player reads metadata from:

`website/assets/data/beat_catalog.json`

Audio files are served from:

`website/assets/audio/catalog/`

Each catalog item should include a title, genre, BPM, price, and stream URL or asset filename. The public player uses only the approved catalog artwork assets.

The catalog grid is collapsed by default. The audio player remains visible, but browser download controls are disabled with `controlsList="nodownload"` so the catalog can be previewed without exposing a direct download button in the player UI.

While catalog audio plays, the analyzer canvas runs as a live visualizer. Uploaded-track analysis still draws a still-frame diagnostic after decoding the uploaded file.

## Free Mix Analyzer

The analyzer section accepts browser-supported audio files. It calculates a client-side diagnostic profile and sends metadata to:

`POST /api/v1/analysis/submit`

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

If the uploaded mix lands inside the professional target range, metadata can enter the reference pool. If it misses the target, metadata is disposed. Audio is not required to be persisted for the public gate.

The current gate is intentionally broad. The backend accepts a wide review lane for released records and rejects only hard failures such as clipping or obviously broken tone-control scores.

## AIFRED VST3

The VST has three primary modes.

Analyze:

- One active mix route.
- Candlestick-style metering.
- Mix signature panel.
- Real metrics for tone, width, punch, loudness, RMS, peak, crest, correlation, and transient density.

Reference:

- One Halo.
- Genre target overlay.
- Personal reference file picker.
- Reference mixer panel with fader, balance, mute/solo, and DAW-routing lanes.

Compare:

- Two independent Halo views.
- Mix A and Mix B have separate DSP routes.
- Comparison bars show tone, width, punch, loudness, and dynamics differences.
- A center analog-style match VU shows how close Mix A is to Mix B.
- In FL Studio, put AIFRED on the master or a bus, enable the Mix B sidechain input in the wrapper, then route the reference track to that sidechain from the mixer send.

Metering notes:

- The spectrometer is labeled by frequency lane.
- Loudness and peak read in K-weighted LUFS and dBFS.
- Crest reads in dB.
- Correlation reads as a -1 to +1 value after a 150 Hz high-pass so bass energy does not dominate the phase read.
- Chat has its own dedicated module and no hardcoded fix suggestions.
- The plugin header displays the current AIFRED version so stale FL Studio scans are easier to spot.
- Theme, layout, gate, API endpoint, and API key fields save into the FL Studio project state.

## Install Paths

Windows system-wide VST3:

`C:\Program Files\Common Files\VST3\Aifred.vst3`

Windows current-user VST3:

`%LOCALAPPDATA%\Programs\Common\VST3\Aifred.vst3`

macOS user VST3:

`~/Library/Audio/Plug-Ins/VST3/Aifred.vst3`

Arch Linux user VST3:

`~/.vst3/Aifred.vst3`
