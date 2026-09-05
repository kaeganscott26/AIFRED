# Testing

Run `pwsh -NoProfile -File scripts/windows/build.ps1 -Action test`. It builds the plugin/tests, runs CTest, Intelligence Host contracts, shared checksums, repository links and Python release-safety tests. Beta additionally runs API/archive Node suites and website checks. Python owns release validation only.

```powershell
ctest --preset windows-release
dotnet run --project tools/AifredIntelligenceHost.Tests/AifredIntelligenceHost.ContractTests.csproj -c Release
python -B -m unittest discover -s scripts/tests
python -B scripts/common/check_shared_core.py
python -B scripts/common/validate_meter_reference.py
```

The last command requires FFmpeg and compares identical generated 48 kHz stereo audio against its ebur128 implementation. Exact values/version go to out/windows-x64/build/reports/meter-reference.json. Tolerances: 0.15 LU integrated, 0.3 dBTP peak, 1 LU LRA. This is independent implementation comparison, not proprietary-meter or full standards certification.

Executed independent result (FFmpeg 9.0, generated 40 s plateau fixture): AIFRED integrated -22.58966596 LUFS versus FFmpeg -22.6; true peak -19.99999987 dBTP versus -20.0; LRA 10 LU in both. Both channels produced identical results. This limited fixture passed the stated tolerances.

C++ tests cover peak/RMS/crest, clipping/silence, timing/gating/reset, analytic intersample peak, FFT mapping/Parseval/850 Hz/high resolution, stereo phase/energy, profiles/sample rates, bounded observation statistics/freshness/epochs, filter units/frequency/reference facts, concurrent SPSC ordering, real plugin profile-state roundtrip/backward default and audio pass-through, fractional GUI projection and previous observation/action/response continuity. Host tests use strict FilteredMixContext and mocked providers; no paid request is required. Release tests exercise failure preservation, recovery and path ownership.

The LRA plateau fixture follows [EBU Tech 3342](https://tech.ebu.ch/docs/tech/tech3342.pdf). The full [EBU test set](https://tech.ebu.ch/publications/ebu_loudness_test_set) remains unvalidated.

## Manual host checklist — NOT PERFORMED

1. Load exact current bundles; record manifests. Test separately then simultaneously. Remove identified old global-slot duplicates from DAW scanning.
2. Use identical audio, insert position, playback region and sample rate in AIFRED and available Waves, SPAN, FabFilter, Ozone or FL Studio meters. Record tool versions/configuration; disable processing between meters.
3. Compare sample peak/clipping; RMS only with compatible 400 ms rectangular mean-channel/sine calibration; crest with matching operands. Never insert arbitrary offsets.
4. Reset programmes together. Compare 400 ms momentary, 3 s short-term, integrated gates, true peak and LRA. Record integration boundaries, interpolation factor/filter and gate tolerance. Short-programme LRA is provisional.
5. Compare tones/broadband frequency placement and relative energy. Match FFT/Hann/overlap, averaging/release/hold and bin-versus-band representation. -24..0 GUI range must not alter context below -24 dB.
6. Test identical, inverted, unrelated and panned channels. Check correlation, balance, M/S and derived width. Diagnostic stereo is 100 ms; other profiles 400 ms. Verify continuous response to Stereo Shaper.
7. Switch all four profiles; save/reload state; reset; stop/resume; seek/loop >1 s; close/reopen editor; play silence; disconnect provider. Check epochs/freshness and audio glitches.
8. Confirm separate hosts/instance IDs and “I changed 6 kHz” → “how about now?” context. Test install/update/uninstall in disposable Windows environment.

JUCE moduleinfo generation enumerates a loaded VST3 factory. It does not establish DAW scan/rendering, realtime CPU, installed-product or professional-meter validation. macOS/Linux runtime/package status: SCAFFOLDED / NOT VALIDATED.
