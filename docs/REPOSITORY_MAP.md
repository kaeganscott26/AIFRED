# AIFRED Beta Repository Map

This document is the public navigation map for the Beta repository. It explains **where to look**, **which tree owns which responsibility**, and **which paths are generated or operational rather than product source**.

The public Beta should remain understandable without private flagship/control-plane context.

---

# START HERE

```text
Need the runtime architecture?  -> docs/ARCHITECTURE.md
Need DSP truth?                 -> shared-dsp/README.md
Need the plugin adapter/UI?     -> plugin-aifred/
Need tests?                     -> tests/ + docs/TESTING.md
Need to build?                  -> scripts/windows/ + docs/BUILD.md
Need install/release behavior?  -> docs/INSTALLATION.md + docs/DISTRIBUTION.md
Need contribution rules?        -> docs/DEVELOPMENT.md
```

---

# Authoritative Runtime Path

```text
plugin-aifred/Source
       │
       ▼
shared-dsp
  aifred_engine
       ↓
EngineSnapshot
       ↓
BufferHunter
       ↓
ObservationSnapshot
       ↓
aifred_filter
       ↓
FilteredMixContext
       ↓
AifredIntelligenceHost transport
```

One implementation owns each responsibility. Do not add a second analyzer, second snapshot serializer, fallback measurement path, or frontend-owned DSP implementation.

---

# Top-Level Map

## Native product source

### [`plugin-aifred/`](../plugin-aifred/)
JUCE VST3 product frontend and DAW adapter.

- [`Source/`](../plugin-aifred/Source/) — processor/editor/client integration.
- [`Assets/`](../plugin-aifred/Assets/) — product assets used by the Beta frontend.
- [`CMakeLists.txt`](../plugin-aifred/CMakeLists.txt) — plugin target and dependencies.
- [`README.md`](../plugin-aifred/README.md) — frontend-specific ownership notes.

The frontend consumes shared-core results. It does not own alternate measurement algorithms.

### [`shared-dsp/`](../shared-dsp/)
Authoritative measurement, observation, filtering, profile, and pipeline implementation shared with the flagship channel through a pinned inventory.

Start with [`shared-dsp/README.md`](../shared-dsp/README.md).

Key conceptual ownership:

```text
Engine           measurement
Contracts        snapshot/profile identities
BufferHunter     bounded temporal observation
Filter           deterministic context projection
Pipeline         processor-side coordination outside realtime publication
```

### [`shared-core.lock.json`](../shared-core.lock.json)
Pinned shared-core inventory used to prove parity and prevent accidental channel drift.

---

# Verification

### [`tests/`](../tests/)
Native/product contract tests.

Important examples:
- `frontend_contract_tests.cpp`
- `state_contract_tests.cpp`
- shared DSP tests referenced from the shared-core build

Use [`TESTING.md`](TESTING.md) for the distinction between automated evidence and manual DAW/professional-meter validation.

### [`scripts/`](../scripts/)
Build, release, install, promotion, shared-core, and repository verification.

Start with:
- [`scripts/windows/`](../scripts/windows/) for Windows operations;
- [`scripts/common/`](../scripts/common/) for cross-cutting release/core checks.

### [`tools/`](../tools/)
Auxiliary validation/release tooling. Tools support the product; they are not alternate runtime implementations.

---

# Build Entry Points

### [`CMakeLists.txt`](../CMakeLists.txt)
Root CMake entry. The Beta plugin is built through `plugin-aifred` and contract tests are registered here.

### [`CMakePresets.json`](../CMakePresets.json)
Canonical configure/build presets.

### [`Directory.Build.props`](../Directory.Build.props)
Shared .NET build settings for host/supporting projects.

Generated output belongs under `out/` and is not source.

---

# Public Website / Distribution Surface

### [`apps/website/`](../apps/website/)
Public Beta web surface and Pages/Function runtime used for the Beta site, controlled package downloads, catalog playback, and public API compatibility.

The public site may contain operational routing code required to serve the Beta. Owner-only administration/control-plane UI is not part of the public product contract.

### [`infra/cloudflare/`](../infra/cloudflare/)
Cloudflare deployment/storage configuration required by the public Beta website/runtime.

No credential value belongs here. Configuration may name bindings/secrets, but values live in Cloudflare/GitHub secret stores.

### [`wrangler.jsonc`](../wrangler.jsonc)
Root convenience configuration. Canonical website deployment behavior is documented in the website/Cloudflare documentation that remains public.

---

# Supporting Public Trees

### [`integrations/`](../integrations/)
Public integration contracts/adapters retained by the Beta.

### [`packages/`](../packages/)
Supporting packages used by the public Beta tree.

### [`models/`](../models/)
Public model/configuration resources when tracked. Model weights and private provider credentials are not committed.

### [`api/`](../api/)
Public API/registry compatibility resources that are safe and required for the Beta surface.

### [`config/`](../config/)
Public non-secret configuration contracts. Secret values do not belong here.

---

# Documentation Map

## Product behavior
- [Architecture](ARCHITECTURE.md)
- [Shared DSP](../shared-dsp/README.md)
- [DSP Configuration](DSP_CONFIGURATION.md)
- [BufferHunter](BUFFER_HUNTER.md)
- [AIFRED Filter](AIFRED_FILTER.md)

## Operating the Beta
- [Build](BUILD.md)
- [Testing](TESTING.md)
- [Installation](INSTALLATION.md)
- [Distribution](DISTRIBUTION.md)
- [Coexistence](COEXISTENCE.md)

## Development boundaries
- [Development](DEVELOPMENT.md)
- [Repository Construction](REPOSITORY_CONSTRUCTION.md)
- [Future](FUTURE.md)

Historical material is evidence, not a license to resurrect removed architecture.

---

# Public Boundary

The public repository exists to make the Beta itself inspectable and reproducible.

Keep private elsewhere:
- owner-only admin applications;
- production control-plane UI;
- operational credentials/tokens;
- private analytics/inquiry exports;
- flagship-only intelligence implementation and plans beyond deliberately published public boundaries.

Keep public here when required for the Beta:
- VST3 source;
- shared measurement/observation/filter source;
- tests/build/install/release tooling;
- public Beta website and package-serving runtime;
- non-secret deployment contracts needed to reproduce that public surface.

---

# Repo Loop

```text
README
  ↓
REPOSITORY_MAP
  ↓
ARCHITECTURE
  ↓
shared-dsp/README
  ↓
BUILD + TESTING
  ↓
INSTALLATION + DISTRIBUTION
  ↓
DEVELOPMENT
  ↓
README
```

When source, tests, and prose disagree, stop and resolve the mismatch. Do not “average” contradictory architecture descriptions into a new implementation.
