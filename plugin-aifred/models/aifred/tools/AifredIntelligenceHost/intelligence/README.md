# AIFRED Intelligence Layer

This directory is the authoritative map for the post-DSP intelligence architecture. It begins **after** `FilteredMixContext` and must never redefine measurements owned by `aifred_engine`, `BufferHunter`, or `aifred_filter`.

## Architecture law

```text
DAW audio
  -> aifred_engine
  -> EngineSnapshot
  -> BufferHunter
  -> ObservationSnapshot
  -> aifred_filter
  -> FilteredMixContext
  -> IntelligenceCore
     -> context assembly
     -> session state
     -> bounded memory
     -> read-only tools
     -> provider reasoning
     -> evidence-bound response
```

The LLM is a replaceable reasoning provider. It is not measurement, memory, session state, or authority.

AIFRED may autonomously maintain its own context. It may not autonomously alter the mix.

## Permanent non-negotiables

- DSP truth is upstream and immutable from this layer.
- `FilteredMixContext` is the only measurement/observation input boundary.
- No intelligence code runs in `processBlock`.
- No model/provider call may block the audio thread.
- No intelligence component may fabricate measurements, reference detail, DAW topology, plugin state, or history.
- Read-only DAW/session inspection may be added behind explicit adapters.
- Audio, mixer, routing, automation, plugin parameters, and production decisions are never modified by AIFRED.
- Agentic behavior is reserved for maintaining AIFRED-owned state: freshness, compaction, pruning, rotation, retrieval, indexing, and context-packet assembly.
- Active memory is bounded to the current session plus the previous nine retained sessions.
- Session 11 evicts session 1 from active intelligence memory. Export/archive features may preserve user-owned records, but active reasoning must still obey the ten-session bound.
- Provider output must distinguish fact, supported inference, possible explanation, and unknown.

## Directory map

- [`core/`](core/) — orchestration entry point; coordinates context, tools, policy, and providers.
- [`contracts/`](contracts/) — typed schemas shared across intelligence components.
- [`context/`](context/) — question-scoped context-packet assembly.
- [`session/`](session/) — current-session lifecycle, events, anchors, and state reduction.
- [`memory/`](memory/) — bounded ten-session retention and promotion/eviction rules.
- [`maintenance/`](maintenance/) — autonomous freshness, compaction, pruning, rotation, deduplication, and repair.
- [`storage/`](storage/) — downstream, schema-versioned persistence; never part of the realtime path.
- [`tools/`](tools/) — typed read-only capabilities available to reasoning.
- [`daw/`](daw/) — normalized read-only DAW/session adapters; no mix mutation.
- [`policy/`](policy/) — evidence, uncertainty, safety, and recommendation rules.
- [`personality/`](personality/) — conversational style only; never evidence or analysis policy.
- [`providers/`](providers/) — OpenAI/Ollama-compatible model adapters behind one interface.
- [`prompts/`](prompts/) — provider-neutral prompt assembly from policy + personality + context.
- [`response/`](response/) — structured response planning and evidence validation before user-visible prose.
- [`tests/`](tests/) — contract, evidence, memory, freshness, provider-parity, and no-mutation tests.
- [`PHASES.md`](PHASES.md) — implementation order and hard completion gates.

## Build order

Do not implement this tree opportunistically. Follow [`PHASES.md`](PHASES.md).

The intended progression is:

1. grounded conversational intelligence;
2. session context and ten-session memory;
3. read-only DAW/session awareness;
4. autonomous intelligence maintenance.

Later GUI work consumes intelligence output; it does not own intelligence state.
