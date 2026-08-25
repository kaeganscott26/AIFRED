# AIFRED System Requirements

## Windows

- 64-bit Windows 10 or newer.
- VST3 host such as FL Studio.
- Enough local disk space for the plugin, AIFRED Engine, and optional Ollama models.
- Ollama for the default local-AI route.

## macOS

- Apple Silicon macOS target for the current published plugin build.
- VST3 host.
- Ollama installed before using the default local-AI route.
- The published macOS ZIP is a manual plugin install and is not notarized; no signed pkg or dmg is currently published.

## Local AI

- AIFRED Engine gateway: `http://127.0.0.1:8787`
- Ollama endpoint: `http://127.0.0.1:11434`
- Default local model: `aifred:latest`

## OpenAI

Optional OpenAI routing uses:

- Endpoint: `https://api.openai.com/v1/responses`
- Default model: `gpt-5.6-luna`
- User-supplied API key

## Website Backend

The public website/backend runs on Cloudflare Pages/Workers infrastructure with the configured KV and R2 bindings. No service binding is part of the current production contract.
