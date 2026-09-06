# aifred_filter

[Filter.cpp](../shared-dsp/src/Filter.cpp) converts `ObservationSnapshot` into deterministic `FilteredMixContext`. It publishes measured units, definitions, distributions, trends, source ownership, profile identity, and explicit reference compatibility. It does not create prose, genre opinions, artistic scores, mastered flags, or targets.

References must match schema, profile ID/revision, and sample rate. Context reports `no_reference`, unavailable, schema/profile/sample-rate mismatch, or compatible. Compatible distributions yield inside/outside relationships without fabricating high-resolution reference bins.

Only `aifred.filtered-mix.v1` reaches AifredIntelligenceHost. The host validates and transports context; it does not reinterpret DSP.

## Related

- [Architecture](ARCHITECTURE.md)
- [BufferHunter](BUFFER_HUNTER.md)
- [DSP Configuration](DSP_CONFIGURATION.md)
- [Future](FUTURE.md)
