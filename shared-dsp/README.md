# Planned shared analyzer ownership

PLANNED / UNIMPLEMENTED. This directory is not compiled or called by either current product.
Official owns the initial design contract in [the construction guide](../docs/REPOSITORY_CONSTRUCTION.md). Develop the new engine from documented algorithms and standards; current products are behavioral baselines, not implementation sources.

| Folder | Future responsibility |
|---|---|
| aifred_engine/analysis | Authoritative sample peak, RMS, loudness, stereo and other documented algorithms |
| aifred_engine/spectrum | Precise FFT/STFT bins and derived telemetry energy |
| aifred_engine/profiles | Versioned configurations of shared algorithm implementations |
| aifred_engine/snapshots | Bounded measurement schema, units, validity, sample-time, profile and epoch |
| BufferHunter | Non-realtime observation windows, coverage, freshness and trends |
| aifred_filter | Factual semantic/context relationships; no canned chatbot responses |
| adapters | Frontend integration contracts for Beta and Official |

No runtime classes, Python brain or CMake target are introduced. Before linking either frontend, choose a versioned distribution mechanism (for example a pinned dependency release). Both consumers must pin the same reviewed contract. Never add a sibling checkout path or copy old analysis implementations. Model identity files and tools remain future documentation only.
