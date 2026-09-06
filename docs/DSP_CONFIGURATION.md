# DSP configuration

[Contracts.h](../shared-dsp/include/aifred/Contracts.h) separates measurement configuration from presentation configuration. The four profiles are the only analyzer modes. AIFRED Beta does not expose separate HQ or Linear Phase switches.

| Setting | Mix Balanced | Spectrum Surgical | Mastering Precision | Stereo Phase Diagnostic |
|---|---:|---:|---:|---:|
| Identity | `MIX_BALANCED.r1` | `SPECTRUM_SURGICAL.r1` | `MASTERING_PRECISION.r1` | `STEREO_PHASE_DIAGNOSTIC.r2` |
| FFT / bins | 2048 / 1025 | 8192 / 4097 | 8192 / 4097 | 2048 / 1025 |
| Overlap / window | 75% / periodic Hann | 75% / periodic Hann | 75% / periodic Hann | 75% / periodic Hann |
| Average / release / hold | 0.4 / 0.5 / 2 s | 2 / 1.5 / 4 s | 3 / 2 / 5 s | 0.4 / 0.5 / 2 s |
| RMS / stereo window | 400 / 400 ms | 400 / 400 ms | 400 / 400 ms | 400 / 100 ms |
| Observation | 15 s | 20 s | 25 s | 15 s |
| CPU / reaction | moderate / balanced | high / deliberate | high / stable | moderate / fast stereo |
| Peak spectrum trace | off | on | on | off |

All profiles retain sample peak, RMS, true peak, M/ST/I loudness, LRA, crest, correlation, balance, M/S, side-to-mid, width, vectorscope, high-resolution FFT, and 30-band telemetry. A required-metric policy marks each profile's focus.

Spectrum Surgical supplies the high-resolution behavior that a separate HQ switch would duplicate. Linear phase has no technical role in the active FFT/STFT measurement. A legitimate future linear-phase feature would require a separate documented analysis-only FIR/filter-bank algorithm.

The default viewport is `-96..0 dBFS`. The Options panel offers `-120`, `-72`, and `-48 dBFS` floors. Viewport changes persist in state and do not reset measurement or observation epochs.

## Related

- [Shared DSP](../shared-dsp/README.md)
- [Architecture](ARCHITECTURE.md)
- [BufferHunter](BUFFER_HUNTER.md)
- [Testing](TESTING.md)
