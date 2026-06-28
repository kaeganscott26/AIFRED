# AIFRED Local Engine Package

Phase 1 consolidation placeholder.

The current canonical local engine source remains:

```text
../../tools/AifredEngine
```

Do not move or rewrite the runtime engine during Phase 1. The plugin, installers, and local AI setup scripts still depend on the existing path.

The local engine is separate from the Cloudflare website backend. It serves the plugin at `http://127.0.0.1:8787` and talks to Ollama at `http://127.0.0.1:11434` with model `aifred:latest`.
