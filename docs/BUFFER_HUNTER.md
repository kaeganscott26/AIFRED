# BufferHunter

BufferHunter consumes `EngineSnapshot` values and owns bounded temporal observation. It does not remeasure audio. The processor-owned [Pipeline](../shared-dsp/src/Pipeline.cpp) survives editor close/reopen.

Profile/revision, sample-rate/channel, manual reset, incompatible publication gap, and major seek/loop changes start new epochs. Compatible stop/resume and silence retain useful history while freshness and signal state change.

The 300-frame ring reports latest, median, P10, P90, extrema, coverage/count, and conservative trend. Profile windows are 15, 20, 25, and 15 seconds. Integrated loudness, LRA, and true peak retain their programme values. `correlationBelowZeroSeconds` records persistent negative correlation; live correlation/width remain in `EngineSnapshot`.

## Related

- [DSP Configuration](DSP_CONFIGURATION.md)
- [AIFRED Filter](AIFRED_FILTER.md)
- [Shared DSP](../shared-dsp/README.md)
- [Testing](TESTING.md)
