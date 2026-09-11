# intelligence/tests

Purpose: prove intelligence contracts without weakening upstream DSP validation.

Required test families by phase:

## Phase 1
- claim/evidence provenance;
- insufficient/stale observation handling;
- incompatible/no-reference handling;
- positive/no-change answers;
- provider-normalization/parity;
- tool allowlist and invalid argument rejection;
- no DSP reinterpretation.

## Phase 2
- session lifecycle/epoch isolation;
- deterministic event reduction;
- anchor comparison;
- persistence recovery;
- ten-session maximum;
- deterministic oldest-session eviction;
- archive/export isolation from active memory.

## Phase 3
- capability discovery;
- normalization across DAW adapters;
- explicit unavailable fields;
- read-only/no-mutation enforcement.

## Phase 4
- stale-context pruning;
- compaction/deduplication;
- memory rotation;
- context budget enforcement;
- maintenance without provider access;
- provider failure cannot corrupt state.

A passing language-model response is not sufficient evidence. Tests should assert structured contracts and invariants wherever possible.
