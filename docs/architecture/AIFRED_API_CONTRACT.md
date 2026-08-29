# AIFRED API Contract

The production Cloudflare API and local AifredEngine are intentionally separate.

Production origin `https://www.north3rnlight3r.com` provides `/health`, `/v1/models`, and `/v1/chat/completions`, plus website/admin routes under `/api/v1`. Streaming chat uses OpenAI-compatible SSE and `data: [DONE]`. Reserved `/v1/embeddings` and `/v1/responses` return 501.

Local AifredEngine at `http://127.0.0.1:8787` provides `/health`, `/analyze`, `/chat`, `/v1/settings`, and `/v1/restart`. It does not implement cloud `/v1/models` or `/v1/chat/completions`.

See [API Reference](../API_REFERENCE.md).
