# AIFRED v0.3.6 Release Notes

AIFRED v0.3.6 is the current consolidated JUCE VST3 line. It aligns the plugin UI and chat context around the same interpreted analysis snapshot, improves local engine startup behavior, adds current Windows/macOS packaging, and supports OpenAI routing through `gpt-5.6-luna` when configured.

## Plugin

- JUCE VST3 build target versioned as `0.3.6`.
- JUCE dependency updated to `8.0.14`.
- Tone, Width, Punch, Loudness bars, Halo quadrants, Compare view, Mix Signature, and chat context read the canonical interpreted `HaloState` analysis snapshot.
- Score formulas use distance-from-reference/target scoring instead of simple magnitude scoring.
- No-signal, invalid, stale, or no-reference states display waiting/unavailable state instead of false perfect scores.
- Current values, loudness windows, and 5-second history values are labeled separately in chat context.
- Chat context includes displayed percentages, raw metrics, reference targets, validity flags, stale/fallback flags, timestamps, and human-readable summaries.
- Chat text cleaning decodes escaped punctuation and removes JSON/code-fence remnants from visible answers.
- Analysis regression checks protect against false 100% states and sanitizer leaks.
- Chat output uses a scrollable read-only text window.
- The plugin uses the Chat Focus layout.
- Reference mode exposes five independent file buttons and five reference volume lanes.
- Halo UI includes readable scale ticks and switchable multiband/waveform/combined center displays.
- Session and minute-history candlesticks expose level behavior over time.
- Compare mode keeps separate Mix A and Mix B analysis routes.
- AIFRED chat responds on request and receives current DSP/reference context rather than canned fix text.

## AI Routing

Default local route:

```text
AIFRED VST3
  -> http://127.0.0.1:8787
  -> http://127.0.0.1:11434
  -> aifred:latest
```

OpenAI route when configured:

```text
https://api.openai.com/v1/responses
model: gpt-5.6-luna
```

The plugin and installer normalize an empty/local-only model choice to `gpt-5.6-luna` when an OpenAI-compatible provider is selected.

## Local Engine

- Windows and macOS use the shared `tools/AifredEngine/Program.cs` runtime.
- Health reporting distinguishes local Ollama readiness from OpenAI route readiness.
- OpenAI provider settings correct stale local Ollama endpoint/model values when necessary.
- Engine logging falls back to the user application-data area when the install-root log path is not writable.
- Plugin health checks can attempt to relaunch the installed local runtime when the gateway is unavailable.

## Windows Packaging

Current Windows artifacts:

- `AIFRED-VST3-Setup.exe`
- `AIFRED-Uninstall.exe`
- `AIFRED-VST3-windows.zip`

The installer packages the VST3 and local engine, writes default local Ollama configuration, registers engine startup, and validates local readiness.

## macOS Packaging

Current macOS artifact:

- `AIFRED-VST3-macOS.pkg`

The package installs the VST3, cross-platform engine, default local configuration, LaunchAgent, setup/repair script, and `AIFRED Engine Control.command`.

The macOS package is not yet signed or notarized.

## Website And Delivery

The public website currently provides **free Windows plugin and catalog MP3 downloads** with no checkout or account requirement.

Current release metadata examples use:

```text
AIFRED_PLUGIN_REPO=kaeganscott26/AIFRED
AIFRED_PLUGIN_RELEASE_TAG=v0.3.6-installer-ai-alias
AIFRED_RELEASE_VERSION=v0.3.6-installer-ai-alias
```

The current plugin download flow uses allowlisted public Worker routes backed by versioned `AIFRED_DOWNLOADS` R2 objects. The payment pipeline is disabled.

Catalog audio uses `AIFRED_DOWNLOADS` R2 first and local files as a development fallback.

## Current CI Release Targets

GitHub Actions currently builds:

- Windows VST3 zip/installer/uninstaller.
- macOS VST3 pkg.
- Website JavaScript and repository validation checks.

Linux and Arch are not current GitHub Actions release targets. Historical references and generic UNIX packaging code remain pending separate verification.

The owner-only Android admin app is not a public release artifact.
