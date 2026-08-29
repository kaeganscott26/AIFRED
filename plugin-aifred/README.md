# AIFRED VST3

Canonical JUCE/C++ plugin source. Current version: 0.3.6; current registered format: VST3 for Windows and macOS builds.

The plugin measures loudness/headroom, tone, stereo width/correlation, punch/transients, dynamics/crest factor and reference alignment. Analyze, Reference and Compare are distinct views backed by the canonical interpreted `HaloState` snapshot. Chat is request-driven and sends structured current context to loopback AifredEngine; no model work occurs on the audio thread.

Default gateway: `http://127.0.0.1:8787`. The plugin remains local-first and does not require Cloudflare. Engine/model failure disables model answers, not core audio analysis.

```sh
cmake -S . -B build/aifred -DCMAKE_BUILD_TYPE=Release
cmake --build build/aifred --config Release --parallel
```

Windows packaging uses `tools/package-aifred.ps1` and the .NET installer projects. macOS packaging uses `tools/macos/package-aifred-macos.sh`. See [User Guide](../docs/USER_GUIDE.md), [Developer Guide](../docs/DEVELOPER_GUIDE.md), and [Troubleshooting](../docs/TROUBLESHOOTING.md).
