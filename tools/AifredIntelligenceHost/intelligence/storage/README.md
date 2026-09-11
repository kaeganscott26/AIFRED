# intelligence/storage

Purpose: persist intelligence-owned session/memory data outside every realtime path.

This folder will own the storage abstraction and migrations for session events, summaries, anchors, and bounded active memory. SQLite is the expected local-first implementation unless a later architecture decision changes it.

Rules:
- never access storage from `processBlock`;
- never persist raw high-frequency DSP frames as long-term intelligence memory;
- writes are downstream, bounded, recoverable, and schema-versioned;
- storage failure must not interrupt audio analysis;
- active-memory retention is still controlled by `memory/`, not database capacity;
- exports/archives are user-owned records and do not expand the ten-session active window;
- provider output is not authoritative storage state until validated and normalized by AIFRED.

Storage implements persistence. It does not decide relevance, freshness, retention, or interpretation.
