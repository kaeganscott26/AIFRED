# AIFRED v0.3.6 Canonical Snapshot Release

This release keeps the distributable JUCE VST3 line and corrects the analysis data flow so the UI and Ask AI chat consume the same interpreted AIFRED analysis snapshot.

## Plugin

- JUCE VST3 build target versioned as `0.3.6`.
- Windows setup now creates the local `aifred:latest` Ollama model alias from `llama3.2:3b` on clean machines.
- Tone, Width, Punch, Loudness bars, Halo quadrants, Compare view, Mix Signature, and chat context now read the canonical interpreted `HaloState` analysis snapshot.
- Score formulas now use distance-from-reference/target scoring instead of "more is better" magnitude scoring.
- No-signal, invalid, stale, or no-reference states display `Waiting`, `--`, or `No Ref` instead of perfect scores.
- Current values, loudness windows, and 5-second history values are labeled separately in the chat context.
- Chat context includes displayed percentages, raw current metrics, reference targets, validity flags, stale/fallback flags, timestamps, and a human-readable metric summary.
- Chat text cleaning decodes escaped unicode apostrophes/quotes and blocks JSON/code remnants from visible output.
- Added analysis regression checks for false 100% Width/Punch/Loudness-style states and chat sanitizer leaks.
- Chat output is now a scrollable read-only text window.
- AI responses are cleaned before display so JSON/code-fence remnants are not shown as the chat answer.
- Theme/layout choices were removed from the options surface; the plugin uses the Chat Focus layout.
- Reference mixer exposes five independent file buttons, one per reference lane.
- Windows release assets include `AIFRED-Uninstall.exe`.
- Resizable Halo interface with quadrant scale ticks and readable labels.
- Switchable Halo center display for multiband, waveform, and combined spectrometer views.
- Session candlestick meter remembers the latest 10 plugin-open/plugin-close sessions.
- Minute history meter shows one candle per minute for the latest 10 minutes of the current session.
- Reference mode shows a default reference-pool ring plus five per-reference rings and five volume lanes.
- Compare mode keeps separate Mix A and Mix B analysis routes for FL Studio sidechain comparison.
- AIFRED chat only responds when the user asks. The model receives current DSP metrics and reference-pool metadata; non-mix-aware canned phrases were removed.

## Website And Delivery

- Website config uses the $5 PayPal purchase path.
- Cloudflare backend records PayPal IPN sales and issues tokenized download links for the configured release package.
- Download defaults target `kaeganscott26/AIFRED` tag `v0.3.6-installer-ai-alias`.
- Notification email support remains environment-variable driven.

## Packaging

- Windows installer packages the VST3 and local AIFRED engine.
- The installer configures the local Ollama endpoint and can save an OpenAI-compatible endpoint, API key, and model into `%AppData%\Aifred\user_settings.json` when the user supplies those private credentials.
- The engine defaults to relative model paths so packages are not tied to a developer machine.
- GitHub Actions build Windows, macOS, Linux, and Arch zip packages and publish release assets from tags.

