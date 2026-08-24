# AIFRED API Contract

The canonical AI base path is `/v1`. Clients configure only the origin/base URL:

- Local: `http://127.0.0.1:8787/v1`
- Cloud: `https://<AIFRED_API_HOST>/v1`

`GET /v1/models` returns an OpenAI-style model list. `POST /v1/chat/completions` accepts `model`, `messages`, `stream`, `temperature`, `top_p`, `max_tokens`, and provider-supported tool fields. Streaming uses SSE with OpenAI-style chunks and `data: [DONE]`. Errors use `{error:{message,type,code,param}}`.

`POST /v1/embeddings` and `POST /v1/responses` are reserved and return explicit `501` responses until providers are configured. The old `/api/v1/chat/ask` route is a deprecated adapter to the canonical implementation.
