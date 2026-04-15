# AIFRED Handoff Log

## 2026-04-15 v0.3.2 Pass

Scope started after v0.3.1 was confirmed working in FL Studio.

Completed in this pass:

- Raised root CMake version to `0.3.2`.
- Added version text to the plugin header through `AIFRED_VERSION_STRING`.
- Replaced the previous RMS-based loudness display path with a K-weighted loudness path:
  - 38 Hz second-order high-pass.
  - 4 dB high-shelf around 1681.974 Hz.
  - BS.1770-style `-0.691 + 10log10(weighted mean square)` display.
- Kept true peak display as direct dBFS peak.
- Kept the 150 Hz high-passed correlation path from v0.3.1.
- Added host project state persistence for:
  - mode
  - theme
  - layout
  - gate slider
  - API endpoint
  - API key
- Updated README and wiki docs for the v0.3.2 metering/state changes.

Known remaining work:

- Panel layout still uses presets; free drag-and-drop panel rearranging is not implemented yet.
- API keys are saved in host project state for convenience. If project-sharing becomes a concern, replace this with OS credential storage or a local proxy token.
- Loudness is now K-weighted but not yet full integrated LUFS with gating windows. Next DSP pass should add momentary, short-term, integrated, and gated history.
- Cloudflare R2 paid-download flow is still documented but not fully implemented.
- Admin app deploy flow still needs a stronger one-button website publish path that cannot flicker back to an old static deploy.
