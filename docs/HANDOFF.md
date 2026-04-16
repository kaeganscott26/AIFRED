# AIFRED Handoff Log

## 2026-04-16 v0.3.3 Website and Plugin Pass

Completed in this pass:

- Removed extra website showcase imagery and left only the approved catalog artwork plus the existing website background.
- Collapsed the beat catalog by default.
- Replaced fake services with:
  - Mixing and Mastering
  - Beat liscenscing
  - VST Sales
- Widened the website analyzer/reference-pool gate so released tracks are not rejected over small metric differences.
- Added plugin genre targets: Hip-Hop / Trap, Boom Bap, Drill / Trap, Electronic / Dubstep, R&B, and Same Genre.
- Made reference mode visually distinct with genre-colored Halo background and a reference mixer panel.
- Added plugin file-pick buttons for chat, reference, and compare files.
- Changed the plugin chat output to a dedicated module without predetermined fix suggestions.
- Removed the center Halo text box and added Punch, Tone, Loudness, and Width quadrant labels.
- Added a horizontal spectrometer in the middle of the Halo and changed the left spectrometer to horizontal lanes.
- Increased DSP smoothing for less jumpy visual readings.
- Kept the startup tutorial to one popup per plugin editor session.
- Documented FL Studio compare routing in the user guide.

Known remaining work:

- The reference mixer panel is a UI and routing target surface. Full third-party VST hosting inside AIFRED requires a JUCE plugin-host graph and a larger architecture pass.
- Native macOS and Linux release artifacts must be produced by their CI runners, not from the local Windows build.
- Bundling and auto-starting Ollama inside installers still needs installer-level work and licensing/size decisions.

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
