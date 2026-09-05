# AifredEngine

AifredEngine 1.0.0 is the loopback-only model and analysis companion for the AIFRED VST3. It listens on `http://127.0.0.1:8787`, defaults to Ollama at `http://127.0.0.1:11434` with `aifred:latest`, and can use an explicitly configured OpenAI-compatible provider.

Routes: `GET /health`, `POST /analyze`, `POST /chat`, `GET|POST /v1/settings`, `POST /v1/restart`. It writes `no-store` JSON and logs to the installed engine `logs/engine.log`. Settings live in platform application data; secrets are local and must not enter Git.

```sh
dotnet build tools/AifredEngine/AifredEngine.csproj -c Release
dotnet build tools/AifredEngine/AifredEngine.Mac.csproj -c Release
```

The engine does not serve the production website and the VST does not call Cloudflare directly. See [API Reference](../../docs/API_REFERENCE.md) and [Backend Separation Contract](../../docs/ARCHITECTURE.md).
