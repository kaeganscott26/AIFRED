# AIFR3D / Aifred Ecosystem Rebuild Master Prompt
Version: 1.1
Target platform: Windows-first development and build environment
Primary goal: Rebuild the Aifred ecosystem from scratch in phased order using a clean, deterministic architecture.

## Scope Lock

Build ONLY these three surfaces:

1. A single VST3 plugin
2. A website
3. An Android admin app

Do NOT build:
- standalone desktop app
- vocal suite
- Aurora suite
- extra plugin variants
- unrelated experiments
- multiple plugin SKUs

The final product line is:
- one flagship VST3 plugin
- one website for branding, onboarding, commerce, updates, and distribution
- one Android admin app for internal control, diagnostics, content/config management, and backend operations

---

## Product Vision

Aifred is a VST3 mix-diagnostic plugin inside the broader DawAI/AIFR3D ecosystem.

It is not a gimmick plugin and not a fake “AI magic” layer.
It must be built as a real, measurable, reference-aware audio analysis system with a clear UI and explainable diagnostics.

The plugin’s job is to:
- analyze incoming mix audio in real time
- compare user metrics against calibrated reference behavior
- detect likely mix issues
- present a clear visual diagnosis layer
- provide meaningful, non-generic guidance
- remain performant and stable on real user systems

The website’s job is to:
- represent the brand professionally
- explain the product clearly
- host marketing and documentation content
- manage purchase flow
- manage download delivery and account/update flows as needed
- be deployed through Cloudflare

The Android admin app’s job is to:
- act as internal control surface for dataset, content, prompt/config, reference pool, product/admin operations, and deployment-related functions
- connect cleanly to the backend
- expose only admin-safe capabilities
- remain modular and production maintainable

---

## Identity and Behavioral Alignment

The system should preserve the following identity traits recovered from prior project work:

### Core values
- genuinely helpful, not fake-helpful
- precise, grounded, technically honest
- structured and explainable
- no fluff, no fake certainty, no corporate filler
- preserve founder intent
- reduce friction
- favor usefulness over polish theater

### Communication style
- sharp
- calm
- real
- technically competent
- not overbearing
- not tutorial-heavy unless needed

### Founder alignment
The system must be built to support a founder who:
- thinks quickly and in stacked layers
- moves across UI, DSP, architecture, product, and business in one stream
- rejects generic AI behavior
- wants real structure, not fake “vision language”
- needs systems that preserve intent under chaos

That means all architecture, naming, prompts, docs, and admin controls should prioritize:
- clarity
- maintainability
- explainability
- low-friction iteration
- strong ownership boundaries

---

## Windows-First Development Setup Requirements

Assume the machine is a fresh Windows development environment.
Before writing project code, set up and verify all dependencies required to build the full ecosystem.

### Phase 0A — Install required Windows tools

Install and verify:

1. Git
2. Git LFS
3. GitHub Desktop or GitHub CLI
4. Visual Studio 2022 Community or Professional
   - include Desktop development with C++
   - include MSVC build tools
   - include Windows SDK
   - include CMake tools for Windows
5. CMake (latest stable)
6. Ninja
7. Python 3.11+ with pip
8. Node.js LTS
9. pnpm or npm
10. Java JDK 17
11. Android Studio
    - Android SDK
    - platform tools
    - emulator optional
12. JUCE
13. VST3 SDK
14. 7-Zip
15. Inno Setup or NSIS for Windows installer packaging
16. FFmpeg if needed for media processing utilities
17. curl / PowerShell tooling as needed
18. Cloudflare CLI (Wrangler)
19. Firebase CLI only if explicitly needed later; otherwise do not introduce it
20. A code editor/IDE setup suitable for multi-language monorepo work

### Phase 0B — Verify all installations

Create a dependency verification checklist and verify:
- git works
- cmake works
- ninja works
- cl/MSVC toolchain works
- python works
- pip works
- node works
- package manager works
- java works
- adb works
- wrangler works
- JUCE path is known
- VST3 SDK path is known

Create a single script or set of scripts that verifies the Windows environment and prints pass/fail for every required dependency.

### Phase 0C — Define canonical folder layout

Create a clean monorepo with top-level folders like:

/docs
/apps/admin-android
/apps/website
/packages/backend
/packages/shared
/packages/config
/native/plugin-vst3
/scripts
/infra/cloudflare
/assets
/reference-data
/tests

Adjust names only if there is a stronger reason, but keep one monorepo and deterministic ownership.

---

## Non-Negotiable Architecture Rules

1. One monorepo
2. One source of truth for shared config
3. Clear separation between:
   - DSP/audio logic
   - UI layer
   - backend/API layer
   - admin app
   - website
   - deployment scripts
4. No mystery glue
5. No hidden business logic in the UI
6. No reading plugin visuals directly from unstable live UI state when a buffered/smoothed analysis layer is more correct
7. No fake AI claims
8. All diagnostics must trace back to real measurable conditions
9. Every subsystem must be documented
10. Every phase must leave the repo in a runnable state

---

## Plugin Canonical Spec

Build a single VST3 plugin named Aifred.

### Purpose
A real-time mix-diagnostic plugin with reference-aware analysis and a strong visual diagnosis system.

### Core analysis categories
At minimum support analysis and deviation framing for:
- tone / spectral balance
- stereo width / stereo behavior
- phase / correlation
- loudness behavior
- dynamics / crest / transient behavior
- low-end weight / distribution
- upper-mid harshness risk
- masking / density risk where feasible

### Reference-aware model
Implement a deterministic reference comparison system based on:
- user metric stream
- reference mean/target behavior
- tolerance bands
- deviation scoring
- smoothing/aggregation logic

Core concept:
Delta(t) = UserMetric(t) - ReferenceMeanMetric(t)

But do not stop at raw delta.
Design:
- metric normalization
- weighting
- tolerance windows
- time smoothing
- confidence/stability logic
- issue prioritization

### UI / Halo diagnostic system
Build the plugin UI around a central diagnosis layer.

The UI should include:
- a central alignment/readiness/diagnostic score or status indicator
- dominant diagnosis text such as:
  - Stereo Too Narrow
  - Low-End Overweight
  - Harsh Upper Mids
  - Dynamics Too Flat
  - Reference Close
  - Good Match
- category deviation arcs or equivalent visual structures for:
  - tone
  - stereo
  - loudness
  - dynamics
- a temporal pulse/history layer showing movement or recent behavior over time

Important:
The visual system must map cleanly from DSP state to normalized metrics to smoothing to UI state.
Avoid ambiguous decorative percentages.
If a score exists, define exactly:
- what it means
- what data feeds it
- how it is normalized
- how it should and should not be interpreted

### Performance requirements
- stable in real DAW usage
- low-latency analysis path
- no heavy UI polling
- no unnecessary memory growth
- safe threading model
- DSP state and UI state separated properly
- resilient when resized, docked, hidden, or reopened

### Deliverables for plugin
- compilable JUCE-based VST3 project
- deterministic DSP analysis engine
- documented metric pipeline
- structured parameter/state model
- explainable UI mapping
- preset/config scaffolding where useful
- automated build steps for Windows VST3 output
- packaging/install strategy

---

## Website Canonical Spec

Build a clean production website for Aifred and the DawAI ecosystem.

### Website goals
- landing page
- product explanation
- feature breakdown
- visuals/screenshots/video embeds if available
- pricing and purchase flow
- documentation/help section
- contact/about section
- update/download flow
- brand-consistent styling

### Deployment target
Use Cloudflare for hosting and edge/backend needs.

Prefer a setup like:
- Cloudflare Pages for frontend
- Cloudflare Workers for APIs when appropriate
- Cloudflare R2 for downloadable assets and protected media/files when appropriate
- Cloudflare DNS/routing for the domain

### Website architecture rules
- modern frontend stack
- production maintainable
- shared config with backend when appropriate
- environment variables documented
- no hardcoded secrets
- no path chaos
- relative asset handling fixed correctly
- no dependence on local machine file paths

### Commerce/distribution
Support a realistic purchase and delivery flow.
Build for a secure, maintainable path such as:
- purchase provider integration
- validated purchase success flow
- controlled download delivery
- release/version management
- update notes/changelog display

If PayPal is used, structure integration cleanly and document SKU/product handling.
Do not build a fake storefront with no real fulfillment path.

### Deliverables for website
- production frontend app
- deployable Cloudflare configuration
- environment template
- content structure
- asset pipeline
- purchase/download flow skeleton or implementation
- documentation for deployment and updates

---

## Android Admin App Canonical Spec

Build one Android admin app for internal ecosystem control.

### App goals
The admin app should support internal/admin tasks such as:
- managing reference pool metadata
- managing content/config data
- viewing system status
- managing product/admin settings
- invoking backend/admin workflows
- reviewing diagnostics/logs where appropriate
- handling controlled internal tools, not public end-user tools

### Technical direction
- Android-first
- maintainable architecture
- strong separation of UI, domain, and data layers
- authenticated admin access
- clean API integration
- no hacked-together mobile shell

### Deliverables for admin app
- Android Studio project
- structured architecture
- environment config
- authentication layer for admin-only use
- API client layer
- key admin screens
- release/debug build documentation

---

## Backend / Shared Systems

Build only the backend needed to support:
- website functions
- admin app functions
- product delivery/config/reference metadata needs

Do not overbuild a giant backend.
Only create the services that the three target surfaces actually need.

### Potential responsibilities
- admin authentication
- product metadata
- release/download metadata
- reference dataset metadata
- config/prompt/state management if required
- purchase verification hooks if required
- diagnostic/report persistence only if there is a real use case

### Shared packages
Create shared packages for:
- types/schemas
- API contracts
- config
- validation
- utility functions
- brand/content constants where appropriate

---

## Phase-by-Phase Execution Plan

### Phase 1 — Project initialization
Tasks:
1. Create monorepo
2. Initialize git
3. Create top-level structure
4. Add README and architecture docs
5. Add environment templates
6. Add dependency verification scripts
7. Add task runner/build scripts
8. Add CI placeholders

Definition of done:
- clean repo
- documented structure
- dependency checks pass
- all apps/packages bootstrapped minimally

### Phase 2 — Core architecture and source-of-truth docs
Tasks:
1. Write product brief
2. Write plugin technical spec
3. Write website technical spec
4. Write admin app technical spec
5. Write shared schema/contracts plan
6. Write deployment architecture
7. Write identity/alignment docs for internal agent/dev continuity
8. Freeze canonical naming and ownership boundaries

Definition of done:
- all architecture docs committed
- no ambiguity about scope
- one canonical source-of-truth set exists

### Phase 3 — Plugin foundation
Tasks:
1. Create JUCE VST3 project
2. Set up audio analysis engine skeleton
3. Define metric structs and processing pipeline
4. Implement buffered analysis path
5. Define DSP-to-UI state bridge
6. Create first working UI shell
7. Output debug metrics for validation
8. Add tests for core analysis math where feasible

Definition of done:
- plugin builds on Windows
- loads in at least one DAW
- metrics pipeline is structured and testable
- UI shell renders and updates correctly

### Phase 4 — Plugin diagnostics and Halo system
Tasks:
1. Implement category analysis
2. Implement reference comparison framework
3. Implement smoothing/aggregation
4. Implement dominant diagnosis logic
5. Implement deviation visuals
6. Implement pulse/history layer
7. Define and document any score meaning
8. Tune stability/performance

Definition of done:
- plugin produces explainable diagnoses
- UI maps cleanly to measured states
- documented thresholds and behaviors exist
- performance remains acceptable

### Phase 5 — Backend foundation
Tasks:
1. Create minimal backend services
2. Define schemas/contracts
3. Add auth model for admin use
4. Add product/release/config endpoints
5. Add deployment-ready environment config
6. Add local/dev mocks as needed

Definition of done:
- backend serves website/admin needs
- contracts are typed and documented
- no unnecessary services exist

### Phase 6 — Website foundation
Tasks:
1. Scaffold frontend
2. Build landing page
3. Build product/features pages
4. Build docs/help structure
5. Build pricing/purchase flow entry
6. Build changelog/download sections
7. Connect to backend/config where needed
8. Prepare Cloudflare deploy config

Definition of done:
- site runs locally
- core pages complete
- deployment path is defined
- asset handling is correct

### Phase 7 — Android admin app foundation
Tasks:
1. Scaffold Android app
2. Implement auth flow
3. Build dashboard
4. Build config/content/reference views
5. Build status/log views
6. Connect to backend APIs
7. Add debug/dev support
8. Validate admin-only flows

Definition of done:
- app builds in Android Studio
- login/auth works
- core admin screens work
- backend integration works

### Phase 8 — Commerce and distribution
Tasks:
1. Implement purchase flow
2. Implement release/download management
3. Implement protected delivery path
4. Add version metadata and changelog flow
5. Define installer hosting path
6. Validate download experience end-to-end

Definition of done:
- user can purchase
- user can receive correct deliverable
- release/version system is manageable
- flow is documented

### Phase 9 — Cloudflare deployment
Tasks:
1. Set up Cloudflare Pages
2. Set up Workers if required
3. Set up R2 if required
4. Configure DNS/domain routing
5. Configure secrets and environments
6. Deploy staging
7. Deploy production
8. Document rollback and update procedure

Definition of done:
- website is live
- backend services needed at edge are live
- assets/download paths are controlled
- deployment is reproducible

### Phase 10 — Packaging and installer creation
Tasks:
1. Produce VST3 release builds
2. Create Windows installer
3. Define install locations
4. Add uninstall support
5. Add release notes
6. Add checksum/versioning
7. Validate install on a clean Windows machine

Definition of done:
- installer works
- plugin lands in correct VST3 path
- uninstall works
- release is reproducible

### Phase 11 — QA and production hardening
Tasks:
1. Test plugin in multiple DAWs if possible
2. Test UI resize/open/close behavior
3. Test website on desktop/mobile
4. Test admin app auth and critical workflows
5. Test purchase/download flow
6. Audit performance and logs
7. Fix critical defects
8. Finalize release checklist

Definition of done:
- major flows validated
- critical issues resolved
- production checklist exists

### Phase 12 — Final distribution and documentation
Tasks:
1. Create release artifact structure
2. Publish website
3. Publish downloadable installer
4. Publish docs/help/changelog
5. Prepare internal admin deployment notes
6. Prepare maintenance/update checklist

Definition of done:
- ecosystem is publicly distributable
- internal admin path is stable
- docs are sufficient for future rebuilds

---

## Required Documentation Outputs

Generate and keep updated:
- master README
- architecture overview
- plugin DSP spec
- plugin UI mapping spec
- backend API/contracts doc
- website deployment doc
- Cloudflare deployment doc
- Android admin setup doc
- Windows setup doc
- release checklist
- installer packaging doc
- environment variable reference
- recovery/rebuild procedure

---

## Build Quality Standards

Every generated subsystem must follow these rules:
- readable names
- strong ownership boundaries
- comments only where useful
- avoid dead abstractions
- avoid overengineering
- avoid fake placeholder architecture
- each phase should compile/run where relevant
- each decision should favor long-term maintainability

---

## Codex Working Rules

When executing this build:
1. Work phase by phase
2. Do not skip architecture
3. Do not introduce extra products
4. Do not create a standalone desktop build
5. Do not create a vocal suite
6. Do not drift from the three-surface scope
7. At the end of each phase, summarize:
   - what was built
   - what files were created
   - what remains
   - what assumptions were made
8. When uncertain, choose the simplest architecture that preserves clarity and future stability
9. Prefer deterministic and explainable systems over flashy but vague implementations
10. Keep all secrets/configs externalized

---

## First action Codex should take

Start with:
1. dependency/install checklist for Windows
2. monorepo initialization
3. architecture/source-of-truth docs
4. plugin foundation
5. backend foundation
6. website foundation
7. Android admin foundation
8. commerce/distribution
9. Cloudflare deployment
10. installer/release hardening

Begin by generating:
- the monorepo directory tree
- Windows dependency install checklist
- bootstrap scripts
- root README
- architecture docs
- then move into implementation phase-by-phase

Do not ask to broaden scope.
Do not add extra products.
Build the single-plugin ecosystem only.
