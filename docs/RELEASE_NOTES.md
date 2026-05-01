# AIFRED v0.3.3 JUCE Final

This release restores the distributable JUCE VST3 line and removes the stale public download assumptions from the website flow.

## Plugin

- JUCE VST3 build target versioned as `0.3.3`.
- Resizable Halo interface with quadrant scale ticks and readable labels.
- Switchable Halo center display for multiband, waveform, and combined spectrometer views.
- Session candlestick meter shows one session candle with open, close, and delta dB.
- Minute history meter shows one candle per minute for the latest 10 minutes.
- Reference mode shows a default reference-pool ring plus five per-reference rings and five volume lanes.
- Compare mode keeps separate Mix A and Mix B analysis routes for FL Studio sidechain comparison.
- Local engine health and BYO endpoint settings remain separated from core DSP metering.

## Website And Delivery

- Website config uses the $5 PayPal purchase path.
- Cloudflare backend records PayPal IPN sales and issues tokenized download links for the configured release package.
- Download defaults target `kaeganscott26/AIFRED` tag `v0.3.3-juce-final`.
- Notification email support remains environment-variable driven.

## Packaging

- Windows installer packages the VST3 and local AIFRED engine.
- The installer can save an OpenAI-compatible endpoint, API key, and model into `%AppData%\Aifred\user_settings.json`.
- The engine defaults to relative model paths so packages are not tied to a developer machine.
- GitHub Actions build Windows, macOS, Linux, and Arch zip packages and publish release assets from tags.

