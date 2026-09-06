# Future architecture

The current completion boundary is:

```text
DAW -> EngineSnapshot -> ObservationSnapshot -> FilteredMixContext
```

The next project may build a new intelligence layer behind that contract. Babylon remains the final GUI phase. Current code does not implement new intelligence tooling, personality/memory systems, delivery-compliance policies, extra profiles, or clickable detail views.

`MetricDetail` and the full-resolution spectrum/telemetry contracts prepare future meter clicks without adding DSP.

## Related

- [Architecture](ARCHITECTURE.md)
- [DSP Configuration](DSP_CONFIGURATION.md)
- [AIFRED Filter](AIFRED_FILTER.md)
- [Testing](TESTING.md)
