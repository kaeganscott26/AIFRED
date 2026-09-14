# intelligence/contracts

Purpose: define stable typed contracts used by the intelligence layer.

Contracts should cover request identity, evidence references, response plans, claim classes, tool arguments/results, session events, session summaries, memory records, DAW context, and maintenance status.

Rules:
- contracts describe data; they do not perform DSP or prose generation;
- upstream DSP/context schema names remain authoritative and are referenced rather than redefined;
- every contract carries enough identity to reject stale/incompatible data;
- schema/revision changes are explicit and testable;
- optional/unavailable fields remain explicit rather than defaulting to fabricated values.

Expected contract families:
- `IntelligenceRequest` / `IntelligenceResponse`;
- `EvidenceRef` / `Claim` / `ClaimConfidence`;
- `ToolRequest` / `ToolResult`;
- `SessionEvent` / `SessionSummary`;
- `MemoryRecord` / retention metadata;
- `DawContext` capability descriptors;
- `MaintenanceReport`.

No provider-specific wire shape belongs here unless it is isolated behind an adapter.
