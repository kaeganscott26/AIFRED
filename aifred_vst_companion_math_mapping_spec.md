# AIFR3D / Aifred VST3 Companion Math & Mapping Spec
Version: 1.0
Companion to: `aifred_vst_canonical_build_document`
Purpose: Lock down metric math, normalization, thresholds, diagnosis triggers, and halo/UI mapping rules so the plugin can be built consistently.

---

# 1. Purpose of This File

This file exists to reduce ambiguity.

The canonical build document explains:
- what the plugin is
- what modules exist
- how the system should be architected

This companion file explains:
- what the major metrics are
- how they should be normalized
- how diagnosis severity should be computed
- how thresholds should work
- how halo visuals should map from internal state

This is a practical first-release spec.
It does not need to be academically perfect.
It needs to be:
- consistent
- explainable
- stable
- implementable
- tunable later

---

# 2. Canonical Metric Categories

Use these top-level categories:

1. Loudness
2. Peaks / Headroom
3. Dynamics
4. Stereo
5. Tone / Spectral Balance
6. Density / Masking
7. Compare / Distance-to-Target

Each category should produce:
- raw measurements
- normalized deviations
- a category severity
- optional explanation evidence

---

# 3. Raw Metric Definitions

## 3.1 Loudness

### Momentary LUFS
Definition:
- short-window perceptual loudness estimate
- rolling window approximately 400 ms

Use:
- immediate loudness feedback
- session candle/history
- fast compare movement

### Short-Term LUFS
Definition:
- rolling loudness estimate
- window approximately 3 seconds

Use:
- stable current loudness state
- loudness compare vs target
- diagnosis support

### Integrated LUFS
Definition:
- accumulated program loudness over session using gated integrated loudness logic where practical

Use:
- broad loudness target alignment
- final session trend
- reference comparison

### Loudness Range (optional for first release)
Definition:
- variation range across short-term loudness over time

Use:
- dynamics characterization
- “too flat” support signal

---

## 3.2 Peaks / Headroom

### Sample Peak
Definition:
- maximum absolute sample value observed over the analysis window

Use:
- safety / clipping risk support

### Estimated True Peak / dBTP
Definition:
- oversampled or interpolated peak estimate approximating inter-sample peak behavior

Use:
- overs risk / headroom diagnosis
- high-severity fix list items

### Headroom
Definition:
- headroom_db = 0 dBFS - current_true_peak_dBFS

Use:
- margin assessment
- “true peak risk” and final loudness safety

---

## 3.3 Dynamics

### Crest Factor
Definition:
- crest_factor_db = peak_dbfs - rms_dbfs
or
- crest_factor_db = true_peak_dbfs - short_term_rms_dbfs

Use:
- macro punch / dynamics characterization
- flat vs spiky judgments

### Dynamic Variance
Definition:
- variance or spread of short-term loudness / RMS over a rolling time window

Use:
- “Dynamics Too Flat” support

### Transient Density
Definition:
- count or weighted score of transient events per unit time

Use:
- attack activity
- over-spiky / under-defined transient judgments

### Transient Sharpness
Definition:
- weighted average steepness or local attack prominence of transient events

Use:
- sharpness vs softness characterization

---

## 3.4 Stereo

### Stereo Width Estimate
Definition:
One practical first-release option:
- width = side_energy / (mid_energy + side_energy + epsilon)

Range:
- approximately 0.0 to 1.0

Use:
- narrow vs wide diagnoses

### Mid/Side Ratio
Definition:
- ms_ratio = side_energy / (mid_energy + epsilon)

Use:
- supporting stereo evidence

### Correlation
Definition:
- inter-channel correlation coefficient over rolling window

Range:
- -1.0 to +1.0

Use:
- mono compatibility
- phase risk diagnosis

### Mono Compatibility Indicator
Definition:
Derived from:
- correlation
- side dominance
- null loss on mono sum if implemented

Use:
- fix list and risk labeling

---

## 3.5 Tone / Spectral Balance

### Band Energy
Use canonical bands:
- Sub: 20–60 Hz
- Low: 60–120 Hz
- Low-Mid: 120–400 Hz
- Mid: 400 Hz – 2.5 kHz
- High-Mid: 2.5–6 kHz
- Air: 6–20 kHz

Definition:
- RMS-like or integrated energy per band over rolling analysis window

Use:
- fingerprint/radar
- tonal compare
- issue detection

### Spectral Tilt
Definition:
- weighted slope from low-frequency energy to high-frequency energy
- can be approximated by linear regression over log-frequency band energies

Use:
- bright vs dull / dark vs tilted judgments

### Brightness / Tonal Centroid
Definition:
- centroid-like weighted spectral center

Use:
- supporting signal for dull / harsh judgments

### Band Delta vs Reference
Definition:
- per-band difference between normalized current band energy and reference band energy

Use:
- tonal issue engine
- compare display
- fix list evidence

---

## 3.6 Density / Masking

These should remain practical proxies.

### Midrange Congestion Score
Derived from:
- persistent surplus in low-mid + mid bands
- low dynamic movement in those bands
- broad overlapping energy

Use:
- density/masking warnings

### Low-End Congestion Score
Derived from:
- excess sub/low energy vs target
- unstable crest/headroom relationship
- mono/sum pressure

Use:
- low-end overweight diagnosis

### Harsh Density Score
Derived from:
- persistent high-mid surplus
- transient sharpness
- true peak strain / presence intensity

Use:
- harsh upper mids diagnosis

---

# 4. Canonical Analysis Windows

These are suggested defaults, tunable later.

- frame hop: 20–50 ms
- FFT visual update: 20–30 fps equivalent
- momentary loudness window: ~400 ms
- short-term loudness window: ~3 s
- diagnosis smoothing window: ~1.5–4 s depending on category
- session snapshot interval: user-driven or periodic every few seconds
- compare state refresh: tied to smoothed engine state, not raw buffer rate

---

# 5. Reference Profile Structure

A reference profile should contain:

- label
- genre/style tag (optional)
- target_integrated_lufs
- target_short_term_lufs_range
- target_true_peak_range
- target_crest_range
- target_width_range
- target_correlation_floor
- target_band_energy_means
- target_band_energy_tolerances
- target_spectral_tilt_range
- target_transient_density_range
- target_dynamic_variance_range

For first release, store ranges as:
- min
- target
- max

This keeps comparison explainable.

---

# 6. Deviation Math

For each metric:

```text
raw_deviation = measured - target
absolute_deviation = abs(raw_deviation)
```

For metrics with a tolerance:

```text
tolerance_normalized = absolute_deviation / max(tolerance, epsilon)
```

Then clamp to a manageable range:

```text
severity = clamp(tolerance_normalized, 0.0, severity_ceiling)
```

Recommended:
- severity_ceiling = 2.0 or 3.0 internally

For user-facing category severity, remap to:

```text
display_severity = clamp(severity / severity_norm_max, 0.0, 1.0)
```

Recommended:
- severity_norm_max = 2.0

Interpretation:
- 0.0 = on target
- 0.5 = moderately off
- 1.0 = strongly off

---

# 7. Signed Direction Model

Keep signed direction where meaningful.

Examples:
- loudness: negative = below target, positive = above target
- width: negative = narrower than target, positive = wider
- spectral tilt: negative = darker than target, positive = brighter
- crest: negative = flatter than target, positive = spikier

This matters because the same magnitude can imply opposite fixes.

---

# 8. Category Severity Computation

Each top-level category should aggregate multiple supporting metrics.

## 8.1 Loudness category severity
Recommended weighted inputs:
- short-term loudness deviation: 0.45
- integrated loudness deviation: 0.35
- true peak risk contribution: 0.20

## 8.2 Dynamics category severity
Recommended weighted inputs:
- crest deviation: 0.40
- dynamic variance deviation: 0.30
- transient density deviation: 0.20
- transient sharpness deviation: 0.10

## 8.3 Stereo category severity
Recommended weighted inputs:
- width deviation: 0.45
- correlation risk: 0.30
- mono compatibility risk: 0.25

## 8.4 Tone category severity
Recommended weighted inputs:
- per-band energy deltas aggregated: 0.60
- spectral tilt deviation: 0.25
- brightness/centroid deviation: 0.15

## 8.5 Density category severity
Recommended weighted inputs:
- mid congestion: 0.40
- low-end congestion: 0.30
- harsh density: 0.30

These weights are starting points, not sacred law.

---

# 9. Diagnosis Trigger Model

A diagnosis should not be emitted from one metric alone unless the case is obvious.

Use:
- primary trigger metrics
- supporting evidence metrics
- minimum severity threshold
- hold time / hysteresis

Generic structure:

```text
diagnosis_score =
  Σ(weight_i * normalized_metric_i)
```

Diagnosis becomes active if:
- score >= activation_threshold
- enough supporting evidence exists
- it remains active long enough to pass hold criteria

Recommended defaults:
- activation_threshold = 0.55
- strong diagnosis threshold = 0.75
- hold time = 0.75–2.0 seconds depending on category

---

# 10. Canonical Diagnosis Definitions

## 10.1 Stereo Too Narrow
Primary drivers:
- width below target
- high correlation near mono
- low side energy

Suggested trigger:
- width deviation severity >= 0.65
- correlation > 0.85
- negative signed width deviation

## 10.2 Stereo Too Wide
Primary drivers:
- width above target
- low/unstable correlation
- mono compatibility risk

Suggested trigger:
- width severity >= 0.65
- correlation risk >= 0.45
- positive signed width deviation

## 10.3 Low-End Overweight
Primary drivers:
- sub + low band surplus
- low-end congestion score
- headroom pressure / true peak proximity

Suggested trigger:
- sub/low combined surplus >= 0.60
- low-end congestion >= 0.50

## 10.4 Low-End Weak
Primary drivers:
- sub + low deficit vs target
- low overall low-band presence
- reduced punch support

Suggested trigger:
- combined low-band deficit >= 0.60

## 10.5 Harsh Upper Mids
Primary drivers:
- high-mid band surplus
- harsh density
- transient sharpness elevation

Suggested trigger:
- high-mid surplus >= 0.60
- harsh density >= 0.50

## 10.6 Dull High End
Primary drivers:
- air/high deficit
- negative brightness deviation
- negative spectral tilt

Suggested trigger:
- air deficit >= 0.55
- brightness/tone support present

## 10.7 Dynamics Too Flat
Primary drivers:
- crest below target
- low dynamic variance
- transient density below desired character

Suggested trigger:
- negative crest severity >= 0.55
- dynamic variance low >= 0.45

## 10.8 Dynamics Too Spiky
Primary drivers:
- crest above target
- transient sharpness high
- unstable peaks

Suggested trigger:
- positive crest severity >= 0.55
- transient sharpness high >= 0.45

## 10.9 Excessive True Peak Risk
Primary drivers:
- true peak near or above target ceiling
- headroom shortage

Suggested trigger:
- true peak severity >= 0.65
This diagnosis can override many others in ranking.

## 10.10 Phase / Mono Compatibility Risk
Primary drivers:
- low/negative correlation
- excessive side energy
- mono loss estimate

Suggested trigger:
- correlation risk >= 0.60

## 10.11 Reference Close
Suggested trigger:
- all category severities below ~0.30
- no critical risk present

## 10.12 Good Match
Suggested trigger:
- all category severities below ~0.20
- no peak or phase red flags
- stable state maintained for hold duration

---

# 11. Diagnosis Priority Rules

Not all issues should rank equally.

Recommended priority order when severity is similar:
1. Excessive True Peak Risk
2. Phase / Mono Compatibility Risk
3. Low-End Overweight
4. Harsh Upper Mids
5. Dynamics Too Flat / Too Spiky
6. Stereo Too Narrow / Too Wide
7. Dull High End / Low-End Weak
8. Reference Close / Good Match

Rationale:
- safety and translation problems should beat aesthetic issues

---

# 12. Fix List Severity Bands

Convert severity to labels:

- **High**: severity >= 0.75
- **Mid**: 0.45 <= severity < 0.75
- **Low**: 0.20 <= severity < 0.45
- below 0.20: usually omit unless useful context

Each fix item must include:
- issue title
- severity band
- concise explanation
- measured evidence
- suggested next move

---

# 13. Overall Alignment / Closeness Score

If a center score is shown, define it exactly.

Recommended:
- compute weighted category penalties
- convert penalty to closeness

Example:

```text
penalty =
  0.22 * loudness_severity +
  0.18 * peaks_severity +
  0.18 * dynamics_severity +
  0.16 * stereo_severity +
  0.18 * tone_severity +
  0.08 * density_severity

closeness = clamp(1.0 - penalty, 0.0, 1.0)
display_percent = round(closeness * 100)
```

Important:
This score is **not “quality”**.
It is **distance to the current target/reference**.

Label it clearly:
- Alignment
- Reference Match
- Target Closeness

Avoid labels like:
- approval
- beauty
- quality

---

# 14. Halo Mapping Spec

The halo must map to actual state.

## 14.1 Single Halo mapping

### Outer ring segments
Map to category severities:
- tone
- stereo
- loudness
- dynamics
- peaks/headroom
- density

Segment fill amount:
- proportional to category severity

Segment color intent:
- cyan/blue = stable/good
- green = near target
- yellow/orange = moderate warning
- red/magenta = critical/high deviation

Exact colors can be tuned, but meaning must remain consistent.

### Inner ring
Map to recent temporal activity:
- pulse intensity = recent transient/loudness movement
- pulse speed = change rate
- pulse brightness = confidence/activity blend

### Center panel
Show:
- 2–4 key metrics
- dominant diagnosis or alignment label
- optional closeness score

---

## 14.2 Dual Halo mapping

Each halo represents a source:
- Mix A
- Mix B
or
- Live vs Reference

Recommended:
- left halo = source A
- right halo = source B or target-relative view

Each side gets:
- its own arc fills
- its own center metrics
- its own color identity

Bottom comparison panel should show:
- band-by-band differences
- overall delta / closeness
- dominant compare insight

---

# 15. Radar / Fingerprint Mapping

The fingerprint/radar should display normalized band-state or category-state values.

Recommended axes for first release:
- Sub
- Low-Mid
- Mid
- Air
- Dyn
- Trans
- Width / Stereo
- Presence / High-Mid

Display:
- live/current fingerprint
- target/reference fingerprint overlay

This is for pattern recognition, not exact reading.
Precise values should remain in cards/tooltips.

---

# 16. Candle / Session History Mapping

The session candles or bars should represent recent snapshot trends.

Recommended candle color logic:
- blue/green = moving toward target
- red/orange = moving away from target
- neutral = stable / minimal change

Height could map to:
- net deviation magnitude
or
- loudness / motion amplitude

Choose one mapping and document it clearly.

---

# 17. Confidence / Stability Score

This should not be fake AI confidence.

Define confidence/stability as:
- how stable the diagnosis has been over recent windows
- how complete/consistent the supporting evidence is

Example inputs:
- diagnosis hold duration
- variance of supporting metrics
- agreement across primary/supporting signals

Range:
- 0.0 to 1.0

Use:
- diagnosis label confidence
- AI explanation certainty wording
- reducing overconfident phrasing during unstable states

---

# 18. AI Explanation Rules

The rule-based AI explainer must translate measured state into user guidance.

## 18.1 Wording rules
- no fake certainty
- no invented causes
- no genre claims unless reference explicitly supports it
- no generic “make it sound better” filler

## 18.2 Example template structure
- observation
- why it matters
- what to check next

Example:
“High-mid energy is consistently above the current target window, and transient sharpness is also elevated. This can push the mix toward fatigue or brittle presence. Check lead brightness, cymbal edge, and transient emphasis before broad high-end changes.”

This is acceptable because it follows evidence.

---

# 19. Suggested Initial Tolerance Defaults

These are starting points only.

## Loudness
- short-term LUFS tolerance: ±1.5 LU
- integrated LUFS tolerance: ±1.0 to ±2.0 LU depending on reference design

## True peak
- safe tolerance window: ±0.5 dB around target ceiling

## Crest factor
- tolerance: ±1.5 dB

## Stereo width
- tolerance: ±0.08 normalized width units

## Correlation
- warning below ~0.30
- risk below ~0.0
- very narrow near +0.90 to +1.0 depending on target

## Band energy
- per-band tolerance: start with normalized ±10–20% band-relative tolerance or dB-derived equivalent

## Spectral tilt
- tune empirically by style, start moderate

These must be configurable in code and documented.

---

# 20. Recommended First-Release Simplifications

To avoid Codex overcomplicating the build:

- use rule-based AI first
- use fixed bands first
- use preset references first, then custom reference import
- use lightweight local persistence for snapshots
- use one clean halo system before adding fancy variants
- keep compare mode structurally simple but real

The plugin wins by being:
- stable
- interpretable
- elegant
not by pretending to solve every audio problem in v1

---

# 21. Validation Checklist for Codex

Codex should verify:

- every visible warning maps back to measurable metrics
- every center score has a clear formula
- compare mode uses two real metric states
- diagnosis does not flicker excessively
- fix list ranks by severity correctly
- AI explanation never contradicts the metrics
- halo segments are tied to category state
- all thresholds are centralized and easy to tune

---

# 22. Final Instruction to Codex

Use this file to lock down:
- metric math
- deviation logic
- diagnosis thresholds
- severity bands
- halo mapping
- center score meaning

Do not invent hidden scoring systems.
Do not use vague “AI confidence” without definition.
Do not let the UI drift away from the measurement engine.

Build the plugin so every meaningful visual and every fix item can be traced back to:
1. a measured signal
2. a normalized deviation
3. a diagnosis rule
4. a documented mapping
