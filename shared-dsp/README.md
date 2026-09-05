# Planned shared analyzer adapter

PLANNED / UNIMPLEMENTED. `adapters/` reserves Beta frontend integration ownership. No target or runtime uses this directory.

The authoritative initial design belongs to Official's [construction guide](https://github.com/kaeganscott26/AIFRED_Official-/blob/main/docs/REPOSITORY_CONSTRUCTION.md); that construction change must be published before the remote link resolves. The current Beta DSP remains a behavioral baseline. Do not port its implementation into the new engine.

The future dependency provides aifred_engine (analysis/spectrum/profiles/snapshots), BufferHunter (non-realtime observations) and aifred_filter (factual context boundary). Choose and pin a versioned dependency before implementing this adapter. Never consume a sibling source tree or maintain a second Beta copy of the algorithm implementation. No model integration belongs in this scaffold.
