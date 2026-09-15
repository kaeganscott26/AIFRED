# Intelligence implementation phases

The intelligence layer is intentionally staged. Codex/agents must not collapse phases or implement later capabilities early.

## Phase 1 — Grounded conversational intelligence

Question answered: **What is happening in the mix?**

Inputs:
- one valid `FilteredMixContext`;
- current profile/configuration identity;
- observation validity/freshness;
- current reference compatibility/relationships;
- current conversation turn.

Implement:
- `IntelligenceCore` orchestration;
- provider-independent response contract;
- typed read-only mix/reference/profile tools;
- evidence policy;
- uncertainty classification;
- context-packet builder for the current question;
- provider router integration behind existing `AifredIntelligenceHost` transport.

Do not implement:
- long-lived session database;
- historical cross-session memory;
- DAW topology;
- DAW mutation;
- autonomous maintenance beyond request-local validation.

Completion gate:
- no unsupported claims from unavailable data;
- no reference claim when reference is incompatible;
- stale/insufficient observations are explicitly acknowledged;
- the system can say that no correction is indicated;
- equivalent evidence behavior is tested across supported providers.

## Phase 2 — Session context and ten-session memory

Question answered: **What has been happening?**

Implement:
- session identity/lifecycle;
- deterministic `SessionReducer`;
- meaningful event timeline rather than raw-frame persistence;
- comparison anchors;
- previous-question/advice/user-decision continuity;
- SQLite or equivalent downstream persistence outside realtime paths;
- current session + previous nine retained sessions;
- deterministic session compaction and eviction.

Rules:
- BufferHunter remains short-term observation authority;
- do not save high-frequency DSP frames as long-term memory;
- current session may be detailed; older sessions must be compact summaries;
- session 11 evicts session 1 from active reasoning memory;
- archived/exported records are not silently reintroduced into active memory.

Completion gate:
- reliable answers to “what changed?”, “did that help?”, “was this different before?”, and “what did you tell me earlier?”;
- bounded storage and deterministic eviction tests;
- no stale epoch/profile/reference context leaks into current reasoning.

## Phase 3 — Read-only DAW/session awareness

Question answered: **Where is it happening?**

Implement normalized adapters for whatever the host/DAW can legitimately expose:
- project/session identity;
- transport/section metadata;
- track/mixer identifiers and labels;
- buses/routes where available;
- plugin inventory where available;
- automation/parameter metadata only when exposed safely by the host.

All adapters normalize into AIFRED-owned contracts before reaching the model.

Allowed:
- read;
- search;
- inspect;
- correlate;
- navigate/highlight non-audio UI state when explicitly supported.

Forbidden:
- move faders;
- change gain;
- change plugin parameters;
- bypass/mute/solo audio state;
- change routing;
- write automation;
- insert/remove plugins;
- process audio.

Completion gate:
- missing host capabilities are reported as unavailable rather than guessed;
- normalized contracts work without provider-specific DAW knowledge;
- read-only/no-mutation integration tests pass.

## Phase 4 — Autonomous intelligence maintenance

Question answered: **Is AIFRED’s own understanding current, bounded, and useful?**

This is the only “agentic” phase by default. The agent manages AIFRED-owned knowledge, not the producer’s mix.

Implement:
- freshness checks;
- stale-context invalidation;
- deduplication;
- event/session compaction;
- ten-session rotation;
- memory eviction;
- relevant-context retrieval;
- context-packet budgeting;
- integrity/repair checks for AIFRED-owned session data;
- optional export/archive handoff that does not expand active memory.

Agent loop:

```text
observe AIFRED state
  -> validate freshness/identity
  -> update owned context
  -> compact/deduplicate
  -> evict stale/old memory
  -> build relevant context packet
  -> wait
```

The maintenance agent has no DAW mutation capability and no authority to reinterpret DSP.

Completion gate:
- bounded memory never exceeds ten active sessions;
- stale epochs/profiles/references are removed deterministically;
- context packets remain within configured budgets;
- maintenance can run without model/provider access;
- provider failure cannot corrupt stored context;
- no maintenance operation changes audio or DAW state.

## Final frontend gate

The final GUI/Babylon phase follows accepted intelligence contracts. It presents or navigates intelligence; it does not become a second intelligence implementation.
