# Shared DSP core 1.2.0

Official and Beta vendor identical `shared-dsp`, `AifredIntelligenceHost`, and host contract-test source. [shared-core.lock.json](../shared-core.lock.json) pins a CRLF-normalized SHA-256 inventory. Each repository builds without a sibling checkout. Run `python -B scripts/common/check_shared_core.py --peer <peer>` to compare explicit clones.

## Pipeline

```text
DAW audio
  -> Engine
  -> EngineSnapshot
  -> BufferHunter
  -> ObservationSnapshot
  -> Filter
  -> FilteredMixContext
```

[Contracts](include/aifred/Contracts.h) define physical values, profiles, measurement configuration, presentation configuration, identities, and metadata. [Pipeline](src/Pipeline.cpp) adapts processor-owned realtime publication to observation and filtered JSON.

## Measurement algorithms

### Level and crest

- Sample peak: maximum absolute sample per channel over the profile RMS window. The unrounded magnitude drives clip detection.
- RMS: rectangular mean-channel energy over 400 ms. A full-scale sine reads `-3.0103 dBFS`.
- Crest: sample peak minus RMS from the same 400 ms interval.

### True peak

[TruePeak.h](include/aifred/TruePeak.h) performs causal 64-tap Blackman-windowed sinc reconstruction. It uses 4x phases below 96 kHz, 2x below 192 kHz, and sample evaluation at 192 kHz. The filter delay is 32 input samples. This follows the oversampled-reconstruction approach in BS.1770 Annex 2 without claiming certification. Finite-filter edge behavior and the full official test set still require validation.

### Loudness

[Loudness.cpp](src/Loudness.cpp) implements BS.1770 K weighting, channel energy summation, 400 ms momentary loudness, and 3 s short-term loudness. Integrated loudness uses 400 ms blocks at 100 ms cadence with the `-70 LUFS` absolute gate and `-10 LU` relative gate. LRA samples 3 s loudness at 10 Hz, applies `-70 LUFS` and `-20 LU` gates, and reports P95 minus P10.

Bounded 0.01 LU histograms retain full-precision power sums. BufferHunter observation duration does not change programme integrated loudness or LRA.

### Stereo

- Correlation: `sum(L*R) / sqrt(sum(L*L)*sum(R*R))`, clamped to `-1..1`; silent denominators are unavailable.
- Balance: `10log10(right_energy/left_energy)`; positive values indicate more right-channel energy.
- Mid/Side: `M=(L+R)/2`, `S=(L-R)/2` with measured mean-square energy.
- Side-to-mid: `10log10(side_energy/mid_energy)`.
- Width: `100*side_energy/(mid_energy+side_energy)`. This is a documented AIFRED presentation scale, not an industry standard.
- Vectorscope: bounded sampled L/R pairs, with no derived quality score.

The active profile configures stereo integration. Stereo Phase Diagnostic uses 100 ms. Other profiles use 400 ms.

### High-resolution spectrum

[Spectrum.cpp](src/Spectrum.cpp) implements a radix-2 real-input STFT with a periodic Hann window and 75% overlap. It averages separate channel powers. One-sided normalization uses `N*sum(window^2)` and doubles interior bins, preserving Parseval energy. Power averaging and peak hold/release operate in the power domain.

The engine publishes all 1025 bins for a 2048 FFT or all 4097 bins for an 8192 FFT. It also publishes instantaneous, averaged, and peak power. No display floor changes those arrays. The frontend maps them into the selected `-120`, `-96`, `-72`, or `-48 dBFS` viewport.

### 30-band telemetry

The exact centres are:

```text
20, 30, 40, 50, 60, 70, 80, 90, 100, 150,
200, 250, 350, 450, 600, 750, 850, 1000, 1500, 2000,
3000, 4000, 6000, 8000, 10000, 12000, 14000, 16000, 18000, 20000 Hz
```

`850 Hz` is index 16. No `300 Hz` centre exists. [Spectrum::extractBands](src/Spectrum.cpp) forms geometric-midpoint regions, computes fractional overlap with FFT bin cells, accumulates power, then converts the result to dB. It does not sample the nearest bin. A region that extends beyond Nyquist remains unavailable.

## Profiles

[DSP Configuration](../docs/DSP_CONFIGURATION.md) provides the complete table and rationale. The four profiles configure one algorithm library:

- `MIX_BALANCED.r1`
- `SPECTRUM_SURGICAL.r1`
- `MASTERING_PRECISION.r1`
- `STEREO_PHASE_DIAGNOSTIC.r2`

There is no separate HQ or Linear Phase mode.

## Realtime publication

The engine publishes at 10 Hz through an eight-slot SPSC queue with seven usable entries. A full queue drops the new publication and increments a counter. The consumer drains at most seven items per 20 ms service callback. Any resulting sample-coverage gap starts a new observation epoch.

## Precision and formatting

Engine and observation calculations use `double`. Vectorscope and GUI positions use continuous float32. Text/model formatting rounds only at publication: whole dBFS/LUFS/dB/percent for most summaries, `0.1 dBTP`, and about `0.01` correlation. Raw snapshots keep full precision.

## Supported input

The engine accepts mono or stereo, finite sample rates from 32 to 192 kHz that are divisible by 10, including 44.1, 48, 88.2, 96, 176.4, and 192 kHz. Invalid configuration or nonfinite samples invalidate/reset analysis without changing audio pass-through.

## Standards references

- [ITU-R BS.1770](https://www.itu.int/rec/R-REC-BS.1770)
- [EBU Tech 3341](https://tech.ebu.ch/docs/tech/tech3341.pdf)
- [EBU Tech 3342](https://tech.ebu.ch/docs/tech/tech3342.pdf)

## Related

- [Architecture](../docs/ARCHITECTURE.md)
- [DSP Configuration](../docs/DSP_CONFIGURATION.md)
- [BufferHunter](../docs/BUFFER_HUNTER.md)
- [AIFRED Filter](../docs/AIFRED_FILTER.md)
- [Testing](../docs/TESTING.md)
