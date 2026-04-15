# AIFR3D / Aifred VST3 Canonical Build Document for Codex
Version: 1.0  
Target: Build the **plugin only** from scratch  
Primary dev OS: Windows  
Required plugin targets: Windows + macOS  
Optional target after stabilization: Linux  
Input references provided to Codex: concept images only, website files, Android admin app files  
Constraint: **Do not reuse old plugin source code. Build a new plugin implementation from scratch.**

---

# 1. Mission

Build a single flagship VST3 plugin named **Aifred** inside the AIFR3D ecosystem.

Aifred is a **real-time, reference-aware mix diagnostic plugin**.  
It is not a generic AI toy. It is not a mastering robot. It is not a suite. It is not a standalone app.

Its job is to:

- listen to live mix audio in real time
- extract measurable DSP features
- compare the user mix against an internal or loaded reference profile
- detect deviations and likely mix issues
- present the result through a **high-clarity visual diagnostic UI**
- provide an **explainable fix list** and optional AI-assisted interpretation
- remain stable and performant in real DAW sessions

This document is the canonical plugin build spec for Codex.  
The website and Android admin app may be used for style alignment and ecosystem integration, but **the plugin code must be rebuilt from scratch**.

---

# 2. Hard Scope Lock

Build exactly one plugin product:

- **Aifred VST3**

Do **not** build:

- standalone desktop app
- vocal plugin
- plugin suite
- extra editions
- AU/AAX unless requested later
- cloud-dependent real-time audio analysis
- fake “one-click fix my mix” functionality

The plugin must stand on three user-facing operating modules:

1. **REFERENCE**
2. **ANALYZE**
3. **COMPARE**

These are the three modules referred to in the concept history.

---

# 3. Concept Image Mapping

The user will provide three concept images. Use them as visual-direction references only.

## Image mapping
- **Reference image** = REFERENCE module visual direction
- **Analyze image** = ANALYZE module visual direction
- **Compare image** = COMPARE module visual direction

These images define:
- layout hierarchy
- halo style direction
- meter framing
- panel density
- visual tone
- diagnostic feel

They do **not** define exact code structure or internal math.

Codex must translate the concepts into a modern, clean, technically stable JUCE implementation.

---

# 4. Product Definition

Aifred is a **diagnostic plugin**, not a processor that changes the audio.

The plugin should:
- receive stereo input audio
- analyze it
- optionally compare it against a reference target/profile
- surface problems such as:
  - stereo too narrow / too wide
  - low-end overweight / underweight
  - upper-mid harshness
  - excessive true peak risk
  - dynamics too flat / too spiky
  - crest mismatch
  - tonal tilt mismatch
  - masking / density risk
  - mono compatibility problems
  - headroom / loudness mismatch

The plugin should **not**:
- process audio in the signal path
- apply EQ/compression automatically
- make fake genre claims without defined references
- generate random advice not tied to measured states

---

# 5. Core User Experience

The user experience is built around **three modules**:

## 5.1 REFERENCE module
Purpose:
- define or load the target profile
- display the target fingerprint
- allow the user to understand what “good” looks like for the current comparison context

Reference can come from:
- built-in genre/reference presets
- loaded reference audio analyzed offline/non-real-time
- saved custom reference snapshots
- future admin-managed reference packs

What the user sees:
- target spectral/fingerprint shape
- target loudness / true peak / crest ranges
- target stereo width / phase tendencies
- target dynamics window
- target status summary

This module answers:
**“What am I aiming at?”**

## 5.2 ANALYZE module
Purpose:
- analyze the live incoming mix in real time
- compute feature streams
- map them to stable user-facing metrics
- generate dominant diagnoses and issue severity

What the user sees:
- live fingerprint
- halo meter
- current measured values
- session trend views
- dominant diagnosis text
- fix list ranked by severity

This module answers:
**“What is my mix doing right now?”**

## 5.3 COMPARE module
Purpose:
- compare A vs B or current mix vs target
- show relative differences clearly
- support dual halo / dual DSP compare logic

Compare should support:
- Mix A vs Mix B
- Live mix vs reference profile
- previous snapshot vs current state

What the user sees:
- dual halo layout
- A/B metric deltas
- band-by-band energy comparison
- crest / loudness / width difference readouts
- “closer to target / farther from target” style summaries

This module answers:
**“How does this source differ from the other one?”**

---

# 6. Canonical Architecture

Build the plugin with clean, explicit separation of layers.

## 6.1 Top-level layers
1. **Audio Input Layer**
2. **DSP Feature Extraction Layer**
3. **Analysis Engine**
4. **Reference Engine**
5. **Comparison Engine**
6. **Diagnosis Engine**
7. **AI Explanation Layer**
8. **UI State / Presentation Layer**
9. **Persistence / Preset / Snapshot Layer**

## 6.2 Strict rules
- real-time audio thread must remain lightweight and deterministic
- AI must **never** run on the audio thread
- UI must not compute heavy DSP
- diagnosis logic must operate on smoothed analysis state, not raw jittery frame data
- compare logic must use normalized metrics, not direct raw-unit mixing across domains
- each subsystem must have a defined owner and explicit interface

---

# 7. Recommended Code Structure

Use JUCE with a modular internal architecture.

Suggested tree:

```text
/native/plugin-vst3/
  CMakeLists.txt
  /Source
    PluginProcessor.h
    PluginProcessor.cpp
    PluginEditor.h
    PluginEditor.cpp

    /core
      AppConfig.h
      Types.h
      Constants.h
      Units.h

    /audio
      AudioRingBuffer.h
      WindowFunctions.h
      FFTProcessor.h
      LoudnessMeter.h
      TruePeakEstimator.h
      CorrelationMeter.h
      CrestFactorMeter.h
      StereoWidthMeter.h
      SpectralAnalyzer.h
      TransientAnalyzer.h
      BandEnergyAnalyzer.h

    /engine
      MetricFrame.h
      MetricNormalizer.h
      MetricSmoother.h
      AnalysisEngine.h
      ReferenceProfile.h
      ReferenceEngine.h
      ComparisonEngine.h
      DiagnosisEngine.h
      SnapshotEngine.h

    /ai
      ExplanationInput.h
      ExplanationEngine.h
      PromptTemplates.h
      RuleExplainer.h

    /ui
      Theme.h
      Layout.h
      HaloMeterComponent.h
      DualHaloCompareComponent.h
      RadarFingerprintComponent.h
      SessionTrendComponent.h
      CandleHistoryComponent.h
      FixListComponent.h
      ValueCardComponent.h
      HeaderComponent.h
      FooterToolbarComponent.h

    /state
      PluginState.h
      ParameterRegistry.h
      SessionStore.h
      SnapshotStore.h

    /util
      MathUtils.h
      SmoothingUtils.h
      EnumStrings.h
      Logger.h
```

This is a guide, not a prison, but the same separation principles must be preserved.

---

# 8. DSP Philosophy

The plugin is **measurement-first**.

Every meaningful on-screen conclusion must trace back to measurable features.

The DSP system must be:
- deterministic
- explainable
- stable
- low-overhead
- modular
- not dependent on cloud AI

The plugin’s intelligence comes from **three stacked layers**:

1. **Raw DSP measurement**
2. **Normalized comparison / diagnosis logic**
3. **Optional AI explanation of diagnosis**

This order is critical.

---

# 9. DSP Pipeline Overview

## 9.1 Audio thread responsibilities
On the audio thread, do only what must be done in real time:

- read stereo input buffers
- update rolling windows/ring buffers
- compute frame-level lightweight feature data or enqueue windows for worker analysis
- store recent raw/audio-derived measurements in thread-safe structures

Never do:
- network calls
- LLM inference
- heavy string generation
- expensive UI recomputation
- complicated dynamic allocation loops

## 9.2 Worker / analysis thread responsibilities
On a background thread or controlled analysis scheduler:

- FFT analysis
- band energy calculations
- loudness aggregation
- true peak estimation
- crest and transient analysis
- width/correlation summaries
- normalization
- smoothing
- diagnosis generation
- AI explanation preparation

## 9.3 UI thread responsibilities
On the UI thread:

- render already-prepared presentation state
- animate halo transitions smoothly
- show cards, trends, and fix list
- handle user interactions
- avoid owning DSP logic

---

# 10. Core Metrics

At minimum implement these metrics.

## 10.1 Loudness metrics
- Momentary LUFS
- Short-term LUFS
- Integrated LUFS
- Loudness range if feasible

## 10.2 Peak metrics
- sample peak
- estimated true peak / dBTP
- headroom to 0 dBFS

## 10.3 Dynamics metrics
- crest factor
- transient density
- transient sharpness / attack tendency
- dynamic flatness score or dynamic variance measure

## 10.4 Stereo metrics
- stereo width estimate
- mid/side energy ratio
- inter-channel correlation
- mono compatibility indicator

## 10.5 Spectral / tonal metrics
- low/sub energy
- low-mid energy
- mid energy
- upper-mid energy
- air/high energy
- spectral tilt
- tonal centroid / weighted brightness
- band balance vs reference

## 10.6 Density / masking signals
Implement practical proxies, not fantasy metrics:
- overlapping band energy congestion
- persistent midrange crowding score
- low-end congestion score
- high-mid harsh density indicator

---

# 11. Frequency Band Model

Use a fixed band strategy for the first release.  
Do not overcomplicate with too many bands.

Recommended canonical bands:
- **Sub:** 20–60 Hz
- **Low:** 60–120 Hz
- **Low-Mid:** 120–400 Hz
- **Mid:** 400 Hz – 2.5 kHz
- **High-Mid:** 2.5 – 6 kHz
- **Air:** 6 – 20 kHz

Optionally split further internally, but preserve this user-facing band model.

---

# 12. Reference Engine

The reference engine defines what the current source should be compared against.

## 12.1 Reference profile content
A reference profile should store:
- target loudness ranges
- target peak ranges
- target crest ranges
- target stereo width/correlation ranges
- target band-energy means
- target spectral tilt tendencies
- target transient behavior ranges
- optional textual label (e.g. “Modern EDM Wide”, “Tight Mono Club”, etc.)

## 12.2 Reference types
Support:
1. built-in preset references
2. user-generated references from analyzed files
3. session snapshots

## 12.3 Reference ingestion
If loading external audio:
- analyze it offline or on a controlled background process
- generate normalized summary profile
- do not attempt real-time heavy reference analysis in the audio callback

---

# 13. Compare Engine

This is a critical subsystem.

The compare engine must support both:
- **live mix vs reference**
- **mix A vs mix B**

This is the basis of the **dual DSP / dual halo compare logic**.

## 13.1 Compare principles
Every compared source should produce its own metric frame:
- source A metric frame
- source B metric frame

Then the compare engine computes:
- direct deltas
- signed differences
- normalized severity differences
- “closer / farther” judgments relative to target

## 13.2 Compare formula base
For any metric:

```text
delta = sourceMetric - targetMetric
```

But do not stop there.

Also compute:
- absolute deviation
- tolerance-normalized deviation
- weighted importance
- direction (above / below / narrower / wider / louder / duller)

## 13.3 Compare output
The compare engine should emit a structured comparison result:

- summary differences
- per-category differences
- per-band differences
- overall closeness value
- dominant delta label
- UI-ready compare state

---

# 14. Metric Normalization

Raw units cannot be mixed blindly.  
Normalize them before diagnosis.

## 14.1 Why normalization matters
LUFS, dBTP, correlation, and band-energy ratios live in different units and scales.  
Codex must not combine them naively.

## 14.2 Normalization strategy
For each metric, define:
- raw unit
- expected range
- healthy tolerance range
- clamp range
- normalization curve

Output:
- normalized score in a controlled space, e.g. `[-1.0, +1.0]` or `[0.0, 1.0]`

Recommended:
- keep **signed normalized deviation** for diagnosis
- keep **absolute normalized severity** for fix ranking

## 14.3 Example
For stereo width:
- target width = 0.42
- tolerance = ±0.08
- measured width = 0.15

Then:
- signed deviation = measured - target
- severity = function(abs(deviation), tolerance)

Do this systematically for every diagnosis-driving metric.

---

# 15. Smoothing and Stability

The plugin must not flicker or overreact.

Implement smoothing at multiple levels:

## 15.1 Frame smoothing
Smooth raw feature values over short windows.

## 15.2 Presentation smoothing
Smooth UI values separately so the halo and cards move cleanly.

## 15.3 Diagnosis hysteresis
Do not swap dominant diagnoses on tiny fluctuations.  
Use thresholds and hold times.

Example:
- require a diagnosis to exceed a severity threshold for N frames
- require an alternative diagnosis to meaningfully exceed the current one before replacing it

This is essential to avoid the plugin feeling unstable or random.

---

# 16. Diagnosis Engine

This is the heart of user-facing intelligence.

The diagnosis engine consumes smoothed, normalized metric state and emits:

- dominant diagnosis
- issue list
- severity ranking
- confidence/stability estimate
- summary status

## 16.1 Diagnosis categories
At minimum implement:
- Stereo Too Narrow
- Stereo Too Wide
- Low-End Overweight
- Low-End Weak
- Harsh Upper Mids
- Dull High End
- Dynamics Too Flat
- Dynamics Too Spiky
- Excessive True Peak Risk
- Crest Mismatch
- Phase / Mono Compatibility Risk
- Reference Close
- Good Match

## 16.2 Diagnosis ranking
Each diagnosis should have:
- trigger conditions
- weighted metrics
- severity score
- explanation data

## 16.3 Fix list
The diagnosis engine must also generate a ranked fix list.

Each fix entry should contain:
- issue name
- severity band (High / Mid / Low)
- brief explanation
- supporting metric evidence
- suggested action style

Example:
- **HIGH — Harsh Upper Mids**  
  Strong persistent energy surplus in 2.5–6 kHz vs target.  
  Signal clarity risk and fatigue tendency elevated.  
  Consider controlled upper-mid taming and re-check transient emphasis.

This is not generic prose. It must be derived from measured evidence.

---

# 17. AI Layer

The AI layer is **not** the real-time DSP brain.

The AI layer is a **secondary explanation and guidance system**.

## 17.1 Rules
- AI must never replace measured DSP analysis
- AI must never invent issues not supported by metrics
- AI must not run on the audio thread
- AI should consume structured diagnosis data and convert it into useful human guidance

## 17.2 AI inputs
The AI layer should receive a compact structured object such as:

- dominant diagnosis
- top 3–5 issues
- measured values
- normalized severities
- reference label
- confidence/stability values
- recent trend information

## 17.3 AI outputs
It may generate:
- a natural-language explanation
- action suggestions
- “why this matters” notes
- compare summaries
- guidance for next steps

## 17.4 AI fallback
The plugin must still function fully without external AI access.  
Implement a **rule-based explainer** first so the product always works offline.

Optional later:
- local model support
- remote model support
- admin-configurable inference path

## 17.5 Canonical AI architecture
Phase 1:
- deterministic rule-based explanation templates

Phase 2:
- optional asynchronous LLM summarizer using structured prompt input

The rule engine is mandatory.  
The LLM layer is optional.

---

# 18. Halo System

The halo is the plugin’s signature visual system.

There are two core halo modes:

1. **Single Halo** for ANALYZE / REFERENCE-driven views
2. **Dual Halo** for COMPARE mode

## 18.1 Single Halo
Represents:
- central measured state
- outer deviation arcs
- recent movement / pulse
- live/current status

Recommended structure:
- outer segmented ring = category deviation intensity
- inner ring = smoothed motion / pulse / activity
- center card = key values + dominant diagnosis

## 18.2 Dual Halo
Represents:
- source A
- source B
- or source vs target

Each side gets:
- its own metric state
- its own color identity
- comparable ring logic
- shared bottom comparison panel

Do not fake it by simply duplicating visuals.  
There must be **true dual source metric state** underneath.

## 18.3 Halo mapping rules
Each halo element must map from real internal state:
- arc color
- arc fill amount
- pulse speed
- pulse intensity
- center values
- compare delta text

No decorative motion without meaning.

---

# 19. UI Layout

Use the concept images as layout inspiration.

## 19.1 Recommended ANALYZE layout
- top header with branding and module buttons
- left panel: fingerprint / radar / band signature
- mid cards: LUFS, dBTP, crest, etc.
- right panel: single halo
- bottom row: last 10 snapshots / candle / trend / history
- right or lower-right: fix list
- footer controls: snapshot, clear fixes, attach reference, debug, ask AI

## 19.2 Recommended COMPARE layout
- two large halos, left and right
- shared summary band at bottom
- energy comparison bar block
- source A vs source B stats in a clean comparative layout

## 19.3 Visual design direction
Preserve:
- dark premium UI
- neon halo language
- sci-fi polish
- but keep it readable and professional
- use restrained glow, not visual chaos

---

# 20. Session and Snapshot Logic

The plugin should support snapshots and recent-session trend tracking.

## 20.1 Snapshot contents
A snapshot should capture:
- timestamp
- summary metric frame
- dominant diagnosis
- issue ranking
- optional label

## 20.2 Use cases
Snapshots power:
- recent 10 session view
- compare against previous state
- trend lines
- reference creation from good states

## 20.3 Persistence
Use lightweight local persistence appropriate for plugin state.  
Do not overcomplicate with a database inside the plugin.

---

# 21. Parameters and Controls

The plugin does not need huge parameter count.  
Focus on meaningful controls.

At minimum consider:
- module selection
- reference source selection
- snapshot capture
- compare source selection
- AI explanation toggle
- analysis sensitivity / smoothing amount
- show/hide detail panes

Avoid parameter bloat.

---

# 22. Threading Model

## 22.1 Audio thread
- capture input
- maintain rolling buffers
- compute lightweight immediate features if needed
- write to lock-safe queues/buffers

## 22.2 Analysis thread
- heavy metric computation
- smoothing
- diagnosis
- comparison
- snapshot updates

## 22.3 UI thread
- render prepared state only

## 22.4 AI thread/task
- explanation generation
- never block audio or UI

Use lock-minimized data handoff patterns:
- atomics where sufficient
- lock-free queues/ring buffers where practical
- double-buffered state snapshots for UI handoff

---

# 23. Build Technology

## 23.1 Mandatory framework
- JUCE
- CMake
- VST3 SDK

## 23.2 Language
- modern C++ (C++20 acceptable if stable in the chosen toolchain)

## 23.3 Windows build
- Visual Studio 2022
- CMake + Ninja or Visual Studio generator

## 23.4 macOS build
- Xcode
- proper signing/notarization plan documented

## 23.5 Linux
Add only after Windows and macOS are validated.

---

# 24. Step-by-Step Build Plan for Codex

## Phase 1 — Project bootstrap
1. Create a new JUCE+CMake VST3 project named Aifred
2. Confirm Windows build
3. Create modular directory structure
4. Add basic processor/editor scaffolding
5. Add build scripts and README

**Done when:** the plugin builds and opens in a host with placeholder UI.

## Phase 2 — Core DSP infrastructure
1. Add ring buffer and frame scheduler
2. Add FFT infrastructure
3. Add loudness meter
4. Add true peak estimator
5. Add correlation/width meters
6. Add crest/transient analyzers
7. Add band energy analyzer
8. Create `MetricFrame` type

**Done when:** the plugin produces a valid internal metric frame from live audio.

## Phase 3 — Analysis engine
1. Implement normalization layer
2. Implement metric smoothing
3. Implement time aggregation
4. Implement state export for UI
5. Add debug metrics panel

**Done when:** stable live values can be displayed without flicker.

## Phase 4 — Reference engine
1. Define `ReferenceProfile`
2. Implement built-in target presets
3. Implement offline reference generation from audio file
4. Store normalized target metrics
5. Add target display state

**Done when:** the plugin can load/select a target profile and compare against it.

## Phase 5 — Comparison engine
1. Implement A vs target compare
2. Implement A vs B compare
3. Add delta calculations
4. Add per-band comparison
5. Add closeness/severity outputs

**Done when:** compare mode has true dual-state operation.

## Phase 6 — Diagnosis engine
1. Define diagnosis rules
2. Create severity scoring
3. Implement dominant diagnosis logic
4. Implement fix list generation
5. Add confidence/stability values

**Done when:** the plugin can explain “what is wrong” based on measured state.

## Phase 7 — Rule-based AI explanation layer
1. Build structured explanation input
2. Build template-based explanation engine
3. Generate concise user guidance
4. Add “Ask AI” panel/action using rule engine first

**Done when:** the plugin can output useful explanations without external AI.

## Phase 8 — UI build
1. Build header/module switcher
2. Build fingerprint/radar component
3. Build value cards
4. Build single halo
5. Build dual halo compare component
6. Build session trend and candle history
7. Build fix list
8. Connect UI to smoothed presentation state

**Done when:** ANALYZE, REFERENCE, and COMPARE all have working UI surfaces.

## Phase 9 — Snapshot/session systems
1. Add snapshot capture
2. Add session history
3. Add recent trend components
4. Add compare-against-snapshot flow

**Done when:** the plugin has meaningful recent-session memory.

## Phase 10 — Performance and polish
1. Profile CPU usage
2. Reduce allocations
3. optimize FFT/update cadence
4. tune smoothing
5. add diagnosis hysteresis
6. harden UI resizing and repaint behavior

**Done when:** plugin feels smooth and stable in a real DAW session.

## Phase 11 — Windows release
1. Create release build
2. package VST3 for Windows
3. validate install path
4. test in at least two hosts if possible

**Done when:** Windows VST3 is distributable.

## Phase 12 — macOS release
1. Build on Mac
2. validate host loading
3. document signing/notarization
4. package macOS VST3

**Done when:** macOS VST3 is distributable.

## Phase 13 — Optional Linux
1. port build if architecture remains clean
2. validate host compatibility
3. document caveats

**Done when:** Linux build works without compromising required targets.

---

# 25. Acceptance Criteria

The plugin is successful when:

- it builds from scratch with no old plugin code reused
- it has three real modules: REFERENCE, ANALYZE, COMPARE
- the DSP pipeline is deterministic and measurable
- the compare engine truly supports dual-source logic
- the diagnosis engine produces stable ranked issues
- the AI layer explains rather than invents
- the halo reflects actual state, not decorative nonsense
- Windows and macOS builds work
- the architecture is documented clearly enough for future maintenance

---

# 26. What Codex Must Not Do

- do not invent extra products
- do not turn this into a processor that alters audio
- do not place AI in the real-time audio path
- do not create fake scores with no exact definition
- do not combine raw metrics without normalization
- do not make the UI read directly from chaotic raw DSP state
- do not use old plugin source code as implementation reference
- do not let visuals drift away from measurable meaning
- do not overengineer unnecessary microservices around the plugin
- do not ship compare mode without true dual-state logic

---

# 27. Final Instruction to Codex

Build Aifred as a **measurement-first, reference-aware, three-module diagnostic VST3 plugin** with a signature halo UI and true dual compare logic.

The plugin must be:
- modern
- explainable
- stable
- cross-platform
- visually premium
- technically honest

Use the supplied concept images for visual direction.  
Use the website and Android admin app for ecosystem/brand alignment only.  
Do not reuse any old plugin code.  
Create a fresh implementation with strong architecture and documentation.

At the end of every phase, report:
- what was built
- which files were added/changed
- what assumptions remain
- what the next concrete step is
