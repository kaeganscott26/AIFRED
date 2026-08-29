# AIFRED Local Engine Package

Path authority pointer; this directory does not contain runtime source.

The current canonical local engine source remains:

```text
../../tools/AifredEngine
```

The plugin, installers and local setup scripts depend on that canonical path. See the current [engine README](../../tools/AifredEngine/README.md).

The local engine is separate from the Cloudflare website backend. It serves the plugin at `http://127.0.0.1:8787` and talks to Ollama at `http://127.0.0.1:11434` with model `aifred:latest`.
