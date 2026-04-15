# AIFR3D / Aifred Rebuild Master Prompt — Revised Scope
Version: 2.0
Primary build machine: Windows
Primary rebuild target: VST plugin from scratch
Preserve and modernize: Website + Android admin app
Required plugin targets: Windows and macOS
Desired plugin target: Linux if practical after Windows and macOS are stable

## Scope Lock

Build and maintain exactly these three product surfaces:

1. A single VST3 plugin named Aifred
2. The website
3. The Android admin app

### Critical scope clarification

- Rebuild the **VST plugin from scratch**
- **Do not** rebuild the website concept from zero unless inspection proves it is necessary
- **Do not** rebuild the Android admin app concept from zero unless inspection proves it is necessary
- Use the existing website source files as a **blueprint/reference**
- Use the existing Android admin app as a **blueprint/reference**
- Modernize both by removing hardcoded paths, fixing deployment assumptions, and reconnecting them cleanly to new infrastructure
- Recreate Cloudflare routing, deployment, and storage configuration cleanly
- Do **not** add:
  - standalone desktop app
  - vocal suite plugin
  - Aurora suite
  - extra plugin SKUs
  - unrelated experimental products

---

## Product Intent

Aifred is a real-time, reference-aware mix diagnostic VST3 plugin within the broader AIFR3D ecosystem.

This is not a fake AI magic product.
It must be a real, explainable analysis system built on measurable DSP and deterministic logic.

The surrounding ecosystem supports it:

- the website presents, sells, documents, and distributes the product
- the Android admin app controls and manages operational/admin workflows
- Cloudflare provides the modern deployment spine for web and edge infrastructure

---

## Revised Build Strategy

### Rebuild from scratch
- VST3 plugin codebase
- plugin DSP pipeline
- plugin UI implementation
- plugin packaging/build scripts
- plugin cross-platform build configuration

### Preserve as blueprint, modernize implementation
- website source structure and design intent
- website branding, layout, page structure, copy direction, media placement
- Android admin app screen flow, module layout, and operational feature set

### Rebuild/restore infrastructure
- Cloudflare DNS
- Cloudflare Pages
- Cloudflare Workers where needed
- Cloudflare R2 where needed
- environment variables
- routing
- asset delivery
- purchase/download flow integration
- any hardcoded path/file assumptions

---

## Non-Negotiable Requirements

1. One flagship VST3 plugin only
2. Windows and macOS plugin builds are mandatory
3. Linux plugin build is desirable and should be added if architecture allows without destabilizing the priority targets
4. Windows is the main development environment
5. Website and admin app must stop depending on hardcoded local file paths
6. All file access and deployment logic must use clean relative paths or environment-driven paths
7. Cloudflare configuration must be recreated cleanly and documented
8. Shared config must be centralized
9. Every subsystem must have clear ownership boundaries
10. The final system must be reproducible from a clean Windows machine

---

## Identity and Alignment Requirements

Preserve the recovered identity traits from project history:

### Core values
- useful over performative
- direct and technically honest
- structured and explainable
- no fluff
- no fake certainty
- preserve founder intent
- reduce friction
- build systems that are real, measurable, and maintainable

### Founder alignment
The system must support a founder who:
- thinks fast
- stacks multiple layers of thought at once
- moves between DSP, UI, architecture, product, business, and branding rapidly
- rejects generic or fake AI behavior
- wants strong structure without losing creativity

That means docs, code, naming, prompts, and admin controls should favor:
- clarity
- low friction
- continuity
- maintainability
- explicit system truth

---

## Windows Development Environment Requirements

Assume a clean Windows machine and install everything needed for:

- plugin development
- website maintenance
- Android admin app maintenance
- Cloudflare deployment
- packaging and distribution

### Required Windows installations

1. Git
2. Git LFS
3. Visual Studio 2022
   - Desktop development with C++
   - MSVC toolchain
   - Windows SDK
   - CMake tools
4. CMake
5. Ninja
6. Python 3.11+
7. Node.js LTS
8. pnpm or npm
9. Java JDK 17
10. Android Studio
11. JUCE
12. VST3 SDK
13. 7-Zip
14. Inno Setup or NSIS
15. Wrangler CLI
16. Xcode access is required later on a Mac build machine for signed macOS plugin builds
17. Optional Linux build environment after Windows/macOS targets are stable

### Verification tasks
Create scripts and checklists to verify:
- git
- cmake
- ninja
- MSVC
- python/pip
- node/package manager
- java
- adb
- wrangler
- JUCE path
- VST3 SDK path

---

## Canonical Monorepo Structure

Create or normalize to a clean monorepo structure like:

/docs
/apps/website
/apps/admin-android
/native/plugin-vst3
/packages/backend
/packages/shared
/packages/config
/infra/cloudflare
/scripts
/assets
/reference-data
/tests

Reuse existing website/admin sources where appropriate, but reorganize into deterministic ownership if currently messy.

---

## Plugin Canonical Spec

Build one VST3 plugin named Aifred.

### Plugin goals
- real-time mix diagnostics
- reference-aware comparison
- clear and explainable UI
- stable cross-platform architecture
- performance suitable for real DAW usage

### Core analysis categories
Support analysis for:
- tone / spectral balance
- stereo width
- phase / correlation
- loudness behavior
- dynamics / crest / transient behavior
- low-end weight
- upper-mid harshness risk
- masking/density risk where practical

### Reference-aware compare model
Base the system on:
Delta(t) = UserMetric(t) - ReferenceMeanMetric(t)

But implement the full real pipeline:
- metric extraction
- normalization
- tolerance bands
- weighting
- smoothing
- issue prioritization
- stability/confidence logic

### Preserve these product truths
- dual DSP logic
- compare mode
- double halo / dual halo visual idea if that remains the chosen direction
- diagnosis-first UI, not vague decoration
- clear relationship between measured state and displayed state

### UI requirements
The UI should support:
- central diagnosis/alignment state
- dominant diagnosis text
- category deviation visuals
- temporal movement/history layer
- clean mapping from DSP state -> normalized values -> smoothing -> visual presentation

### Platform targets
Required:
- Windows VST3
- macOS VST3

Desired:
- Linux VST3 build path after required targets are stable

### Plugin deliverables
- JUCE-based VST3 source
- deterministic DSP engine
- documented metrics and thresholds
- cross-platform build config
- Windows packaging path
- macOS build/signing/notarization plan
- Linux build notes if implemented

---

## Website Preservation and Modernization Spec

The website already exists conceptually and should be preserved as the product/public face.

### Preserve
- branding
- page structure
- visual identity
- product positioning
- layout logic
- core content model

### Rework
- text where needed
- hardcoded file paths
- asset linking
- API routing
- deployment assumptions
- Cloudflare integration
- purchase/download delivery plumbing

### Website goals
- landing page
- product explanation
- feature breakdown
- screenshots/media
- pricing/purchase flow
- download/update flow
- docs/help
- contact/about

### Deployment target
Use Cloudflare for the live web stack:
- Pages for frontend
- Workers for needed APIs
- R2 for assets/downloadable artifacts where appropriate
- DNS and routing configured cleanly

### Website deliverables
- cleaned website codebase
- relative/environment-driven asset paths
- Cloudflare deployment config
- documented env vars
- purchase/download flow structure
- update/release content flow

---

## Android Admin App Preservation and Modernization Spec

The Android admin app should be kept conceptually and modernized, not casually discarded.

### Preserve
- command center concept
- screen layout logic
- admin-only orientation
- chat/upload/command split
- website control features
- catalog/reference management idea
- sales/inquiries/admin log sections
- settings/config control surfaces

### Rework
- hardcoded endpoints
- brittle auth/session assumptions
- local-path dependencies
- backend contract mismatches
- deployment/env assumptions

### Admin app role
An authenticated Android control plane for:
- website control
- file/content operations
- catalog/reference admin
- AI/system settings
- backend diagnostics
- sales/inquiry/admin operations

### Deliverables
- cleaned Android codebase
- stable API integration
- env/config separation
- admin auth flow
- documented backend dependencies

---

## Backend and Shared Services

Build only what the three retained surfaces actually need.

### Responsibilities
- admin authentication
- product/release metadata
- website/admin app API needs
- download/release metadata
- reference/catalog metadata
- diagnostics/status endpoints if needed

### Avoid
- giant unnecessary backend
- vague microservices
- overbuilt architecture

### Shared packages
Create shared packages for:
- schemas/types
- config
- validation
- API contracts
- shared constants

---

## Phase-by-Phase Execution Plan

### Phase 1 — Audit and freeze current assets
Tasks:
1. Freeze current website source
2. Freeze current Android admin app source
3. Inventory branding assets
4. Inventory plugin visual targets and halo screenshots
5. Identify any useful backend contracts still worth preserving
6. Document what is source-truth vs obsolete

Definition of done:
- preserved blueprint assets are backed up
- keep/rework/rebuild decisions are documented

### Phase 2 — Windows environment setup
Tasks:
1. Install all required dependencies
2. Verify toolchain
3. Document machine setup
4. Create bootstrap scripts

Definition of done:
- clean Windows machine can build repo prerequisites

### Phase 3 — Monorepo normalization
Tasks:
1. Create/normalize repo structure
2. Place website/admin/plugin in proper ownership boundaries
3. Add shared config and docs folders
4. Add scripts and env templates

Definition of done:
- monorepo is structured and documented

### Phase 4 — Architecture and source-of-truth docs
Tasks:
1. Write plugin spec
2. Write website modernization spec
3. Write admin app modernization spec
4. Write Cloudflare deployment spec
5. Freeze naming and boundaries

Definition of done:
- scope is explicit and locked

### Phase 5 — Plugin rebuild from scratch
Tasks:
1. Create fresh JUCE VST3 project
2. Define DSP engine
3. Implement dual-DSP/compare architecture
4. Implement metric pipeline
5. Build UI shell
6. Implement halo/diagnosis system
7. Validate in DAW
8. Prepare cross-platform build configuration

Definition of done:
- plugin builds and runs on Windows
- architecture is clean and documented

### Phase 6 — Cross-platform plugin targets
Tasks:
1. Prepare macOS build machine workflow
2. Build and validate macOS VST3
3. Document signing/notarization path
4. Add Linux build path if practical

Definition of done:
- Windows and macOS are supported
- Linux path exists if feasible

### Phase 7 — Website modernization
Tasks:
1. Refactor hardcoded paths
2. Normalize assets
3. Update text/content where needed
4. Connect to new backend/env config
5. Prepare Cloudflare deployment files

Definition of done:
- website runs cleanly without brittle path assumptions

### Phase 8 — Android admin app modernization
Tasks:
1. Refactor endpoint/config assumptions
2. Clean auth/session flow
3. Reconnect to backend contracts
4. Preserve operational screens and workflows
5. Validate on current Android environment

Definition of done:
- admin app works against the restored backend/web stack

### Phase 9 — Cloudflare restore/rebuild
Tasks:
1. Recreate DNS
2. Recreate Pages
3. Recreate Workers as needed
4. Recreate R2 as needed
5. Set secrets and env vars
6. Connect domain routing
7. Validate health endpoints and asset delivery

Definition of done:
- website and supporting services are live and routable

### Phase 10 — Commerce and distribution
Tasks:
1. Restore or implement purchase flow
2. Connect release/download flow
3. Define delivery path for plugin installers
4. Add version metadata and changelog process

Definition of done:
- product can be sold and delivered realistically

### Phase 11 — Packaging and QA
Tasks:
1. Build Windows installer
2. Define macOS distribution path
3. Test plugin in target DAWs
4. Test website on desktop/mobile
5. Test admin app against live services
6. Fix critical defects

Definition of done:
- ecosystem is stable enough for release

### Phase 12 — Release and maintenance docs
Tasks:
1. Create release checklist
2. Create rebuild/recovery doc
3. Create deployment rollback doc
4. Create future maintenance notes

Definition of done:
- future rebuilds do not depend on memory alone

---

## Documentation Outputs Required

Maintain:
- README
- Windows setup guide
- plugin DSP spec
- plugin UI mapping spec
- website modernization doc
- admin app modernization doc
- Cloudflare deployment doc
- backend/API contract doc
- release checklist
- recovery/rebuild guide

---

## Codex Working Rules

1. Respect the revised scope
2. Rebuild only the plugin from scratch
3. Treat website and Android admin app as preserved blueprints
4. Remove hardcoded paths everywhere
5. Use relative or environment-driven configuration
6. Prioritize Windows development workflow
7. Deliver Windows and macOS plugin support as mandatory
8. Add Linux plugin support only after mandatory targets are stable
9. At the end of each phase report:
   - what changed
   - what files were touched
   - what assumptions remain
   - what still blocks release

---

## First Actions

Start by:
1. auditing and backing up the current website/admin assets
2. setting up the Windows environment
3. normalizing the monorepo
4. writing the locked architecture docs
5. rebuilding the VST plugin from scratch
6. modernizing website and admin app integration
7. restoring Cloudflare
8. validating Windows/macOS plugin distribution

Do not broaden scope.
Do not add extra products.
Do not rebuild solved surfaces unnecessarily.
Build the cleanest version of the retained ecosystem.
