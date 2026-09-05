# Beta architecture

The current [shared core contract](../shared-dsp/README.md) defines implemented ownership and algorithms. Source is vendored, versioned and checksum-verified, with no sibling-clone dependency.

The processor owns DSP, bounded SPSC publication, BufferHunter, observation lifetime and bounded context. The editor renders full-resolution power and continuous float32 meters and selects one of four implemented profiles. aifred_filter is the only structured analysis boundary. AifredIntelligenceHost routes provider transport on channel beta, port 8787. No Python runtime or duplicate analyzer participates.

Profile selection adds optional dsp_profile XML state. Missing/unknown values select MIX_BALANCED. Existing plugin/state identities are retained. Profile changes activate at the next audio block, reset incompatible epochs and propagate revision to context. Editor close/reopen does not reset observation.

Future tools/profiles in the workspace design remain unimplemented. The current scope implements the four initial profiles. [Testing](TESTING.md) records limits; [coexistence](COEXISTENCE.md) describes installation boundaries.
