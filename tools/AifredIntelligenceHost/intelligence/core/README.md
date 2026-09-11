# intelligence/core

Purpose: own the provider-independent orchestration entry point for AIFRED intelligence.

`IntelligenceCore` coordinates validated context, typed tools, policy, personality, provider routing, and response validation. It does not measure audio, persist raw DSP frames, own DAW state, or contain provider-specific business logic.

Expected responsibilities:
- accept validated request identity + `FilteredMixContext`/session context;
- ask the context builder for the smallest relevant packet;
- expose only allowlisted read-only tools;
- apply evidence/uncertainty policy before provider execution;
- call the selected provider through `providers/`;
- validate structured provider output;
- return an evidence-bound response to the host/frontend.

Forbidden:
- DSP formulas;
- direct `processBlock` access;
- direct SQLite writes from the audio thread;
- hidden global memory;
- DAW mutation;
- provider SDK code outside `providers/`.

Dependencies flow inward from contracts/policy/context/tools/providers. Other folders should not duplicate orchestration.
