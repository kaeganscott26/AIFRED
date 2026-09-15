# intelligence/tools

Purpose: expose typed, allowlisted, read-only capabilities to `IntelligenceCore`.

Phase 1 tool families may include:
- current mix state;
- metric detail;
- spectrum band/region detail;
- stereo/loudness/dynamics state;
- reference status/relationship;
- profile/observation status.

Later phases may add:
- session timeline/change lookup;
- previous advice/anchors;
- read-only DAW/session search and inspection.

Rules:
- tool schemas are AIFRED-owned and provider-neutral;
- tool results preserve source identity and availability;
- unavailable data returns an explicit unavailable result;
- tools do not write DSP, DAW, mixer, routing, automation, or plugin state;
- no generic shell/filesystem/network tool is exposed to the model from this layer without a separately approved architecture change;
- tool execution is bounded, auditable, and cancellable.

A model may request facts through tools; it may not create new facts.
