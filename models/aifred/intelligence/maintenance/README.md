# intelligence/maintenance

Purpose: perform autonomous maintenance of AIFRED-owned intelligence state.

This folder is the default home of agentic behavior. The maintenance agent manages context, memory, freshness, and integrity. It never manages the producer's mix.

Allowed autonomous work:
- validate profile/epoch/reference/session identity;
- expire stale context;
- compact completed session history;
- deduplicate events/records;
- rotate the ten-session memory window;
- evict obsolete active memory;
- rebuild bounded indexes;
- assemble/rebalance context budgets;
- verify/repair AIFRED-owned persistence metadata;
- hand completed data to export/archive systems without expanding active memory.

Forbidden:
- audio processing;
- DAW parameter changes;
- mixer/routing/automation changes;
- plugin insertion/removal/bypass;
- redefining measurements;
- allowing a provider to choose retention or deletion policy.

Maintenance must remain functional when no model provider is available. Provider failure must not corrupt or freeze AIFRED-owned state.
