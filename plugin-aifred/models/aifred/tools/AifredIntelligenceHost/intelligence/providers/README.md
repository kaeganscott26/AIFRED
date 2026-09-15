# intelligence/providers

Purpose: isolate model/provider transport and protocol differences behind one intelligence-provider interface.

Supported provider families may include local Ollama and OpenAI-compatible endpoints, but provider identity must never change AIFRED's evidence, memory, tool, or no-mutation rules.

Responsibilities:
- translate AIFRED request/prompt/tool contracts to provider wire format;
- normalize provider responses/tool calls;
- enforce timeouts/cancellation/size limits;
- report provider availability/failure explicitly;
- avoid leaking provider-specific objects into `core/`.

Rules:
- no provider owns memory;
- no provider persists authoritative session state;
- no provider receives secrets from plugin/DSP contracts;
- provider failure leaves DSP and maintenance operational;
- switching provider must not reinterpret measurement truth.

Existing `AifredIntelligenceHost` remains lifecycle/HTTP/settings transport; this folder is the future reasoning-provider boundary behind it.
