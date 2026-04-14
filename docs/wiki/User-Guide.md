# User Guide

## Website

The public site lives at:

- https://www.north3rnlight3r.com
- https://north3rnlight3r.com

The website sells the AIFRED VST3 and North3rnLight3r beat catalog. Visitors can preview catalog audio, license beats through PayPal links, download VST release packages, and run the free mix analyzer.

## Beat Catalog

The catalog player reads metadata from:

`website/assets/data/beat_catalog.json`

Audio files are served from:

`website/assets/audio/catalog/`

Each catalog item should include a title, genre, BPM, price, stream URL or asset filename, and artwork URL. The public player falls back to the AIFRED mascot if artwork is missing.

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

## AIFRED VST3

The VST has three primary modes.

Analyze:

- One active mix route.
- Candlestick-style metering.
- Mix signature panel.
- Real metrics for tone, width, punch, loudness, RMS, peak, crest, correlation, and transient density.

Reference:

- One Halo.
- Reference target overlay.
- Target label from the reference pool.
- Designed for alignment against pro reference behavior.

Compare:

- Two independent Halo views.
- Mix A and Mix B have separate DSP routes.
- Comparison bars show tone, width, punch, loudness, and dynamics differences.

## Install Paths

Windows system-wide VST3:

`C:\Program Files\Common Files\VST3\Aifred.vst3`

Windows current-user VST3:

`%LOCALAPPDATA%\Programs\Common\VST3\Aifred.vst3`

macOS user VST3:

`~/Library/Audio/Plug-Ins/VST3/Aifred.vst3`

Arch Linux user VST3:

`~/.vst3/Aifred.vst3`

