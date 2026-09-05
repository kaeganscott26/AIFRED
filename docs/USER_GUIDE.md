# AIFRED Beta user guide

Install from verified current using [installation](INSTALLATION.md). DSP operates without chat. AifredIntelligenceHost on port 8787 requires .NET 10 and a configured available Ollama/OpenAI-compatible provider. No model is downloaded implicitly. macOS/Linux runtime remains unvalidated.

Select MIX_BALANCED for general mixing, SPECTRUM_SURGICAL for detailed FFT, MASTERING_PRECISION for programme metering, or STEREO_PHASE_DIAGNOSTIC for fast phase response. Profile switching starts a clean observation epoch. Live correlation/width follow current audio; other engineering values summarize observed measurements. Short observations remain insufficient for sustained conclusions. The FFT display is -24..0 dB while measurements retain full range.

Analyze observes Mix A; Reference uses compatible locally measured reference files; Compare has independent Mix A/B input pipelines. The first enabled reference slot supplies the selected distribution; slot controls do not average LUFS or fabricate FFT data. Chat sends explicit filtered observations only when asked. Four recent observation/question/response records support follow-up questions. Stated actions are not verified DAW edits.

The plugin does not require Cloudflare to measure audio. Website, Android/desktop administration and archive features have separate [admin](ADMIN_GUIDE.md), [API](API_REFERENCE.md) and [archive](ARCHIVE_GUIDE.md) contracts. Plugin observation history is bounded in memory, not a long-term session archive.
