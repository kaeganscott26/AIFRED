# AIFRED Plugin Stabilization

Updated: 2026-05-28

This document records the focused VST3 stabilization pass for visible UI truthfulness and layout containment.

## Scope

- Plugin UI/source only.
- No installer or package scripts.
- No `aifred-site` changes.
- No fake meter values or canned AI responses.
- If a visible module is unavailable, the UI must say so instead of drawing default values as live data.

## Completed Stabilization Tasks

1. Chatbox layout collision
   - Chat controls now use the same right-column bounds as the painted chat panel.

2. Tone and Width meter truthfulness
   - `smoothed_.spectralTilt` now drives both Tone label aliases and Tone bar data.
   - `smoothed_.stereoWidth` now drives both Width label aliases and Width bar data.

3. Punch meter truthfulness
   - Punch label and bar now both use `state.metrics.punch01`.
   - Crest remains visible only as secondary context.

4. Loudness/LUFS meter truthfulness
   - Loudness label and normalized bar now both represent short-term LUFS.

5. Correlation meter truthfulness
   - Correlation fill/marker is hidden when signal or values are unavailable.

6. Spectrometer truthfulness
   - Spectrometer bands no longer draw minimum fake fill during silence or invalid analysis.

7. Candlestick/dynamics meter truthfulness
   - Current candle and empty history strips now report unavailable/empty states instead of default-value visuals.

8. Halo data truthfulness
   - Halo visuals are gated on valid signal analysis.

9. Mix Signature truthfulness
   - Live Mix Signature uses raw live metrics consistently and reports unavailable when no valid analysis exists.

10. Local engine status display correctness
    - Chat panel displays the actual engine client status text instead of forcing a ready message from a stale health flag.

11. Backend/chat availability state correctness
    - Duplicate chat requests report in-progress state.
    - Failed chat/settings routes clear cached availability.

12. Responsive layout, readability, and visible placeholder audit
    - Layout scaling no longer double-counts display DPI.
    - Long labels use fitted text where clipping was likely.
    - Visible unfinished placeholder copy was replaced with honest current behavior.

## Validation

Each task was validated before moving to the next task:

```sh
git diff --check
cmake --build build-mac --config Release
```

The macOS Release VST3 build passed after every task.

## Remaining Release Risk

Windows runtime regression has not been executed in this macOS environment. Before claiming release or beta readiness, run a Windows pass covering plugin load, UI scaling, chat/backend status, and audio-signal meter behavior.
