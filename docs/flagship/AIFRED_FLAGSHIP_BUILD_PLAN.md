# AIFRED Flagship Build Plan (Audit Memory)

> Historical planning baseline. PayPal, `AIFRED_WEBSITE_ASSETS`, and payment-gated milestones in this plan are not active. The 2026-08-25 production contract uses free Worker-mediated downloads from `aifred-downloads`; see `../operations/website-cloudflare-production-2026-08-25.md`.

*All statements are classified as **OBSERVED**, **INFERRED**, **UNVERIFIED**, or **KNOWN ISSUE** according to the evidence gathered from the repository.  No code or configuration has been modified; this document is the sole write‑authorized artifact for the audit.*

---

## Phase 01 – Repository Discovery (Architecture Inventory)

### 1. Top‑Level Structure  

| Path (relative) | Primary Function | Evidence |
|----------------|------------------|----------|
| `plugin-aifred/` | JUCE C++ VST3 plug‑in source | **OBSERVED** – listed in `README.md`. |
| `tools/AifredEngine/` | Cross‑platform local AI engine (gateway) | **OBSERVED** – described in `README.md` and `packages/local-engine/README.md`. |
| `apps/website/` | Cloudflare Pages website & backend (storefront, catalog, API, WebSocket chat) | **OBSERVED** – directory contains HTML/CSS/JS, `wrangler.toml`, workers. |
| `apps/admin-android/` | Private Android admin console (owner‑only) | **OBSERVED** – Gradle project and README. |
| `infra/cloudflare/` | Cloudflare operational configuration (wrangler files, KV/R2 bindings) | **INFERRED** – mentioned in top‑level `README.md`. |
| `docs/` | Documentation, release notes, operation logs, legacy archives, and this flagship‑build‑plan. | **OBSERVED** – contains `RELEASE_NOTES.md`, `operations/`, `archive/`. |
| `.github/workflows/` | CI/CD pipelines (GitHub Actions) | **OBSERVED** – workflow YAML files. |
| `packages/` | “Shim” packages that point back to canonical source locations. | **OBSERVED** – placeholder READMEs. |
| `build/` | Generated CMake build artifacts (binaries, installers). | **OBSERVED** – compiled outputs. |
| `tools/release/` | Release‑automation scripts (dry‑run, parity checks, etc.). | **OBSERVED** – scripts used in CI. |

### 2. Component‑by‑Component Details  

#### 2.1 JUCE VST3 Plug‑in (`plugin-aifred/`)  
- Core DSP & analysis (tone, width, punch, loudness, dynamics, reference alignment).  
- UI & Look‑and‑Feel (custom colours, gradients, halo view).  
- Build & packaging via CMake (produces Windows installer/zip and macOS PKG).  
- Version 0.3.6 (beta).  
- **Evidence:** OBSERVED source, INFERRED build scripts, KNOWN ISSUE – macOS PKG not notarized.  

#### 2.2 Local AI Engine (`tools/AifredEngine/`)  
- HTTP gateway `127.0.0.1:8787` → Ollama `127.0.0.1:11434` (model `aifred:latest`) or OpenAI (`https://api.openai.com/v1/responses`).  
- Health reporting & auto‑restart (described in release notes).  
- Cross‑platform binaries built by CI.  
- **Evidence:** OBSERVED entry point, INFERRED health logic, UNVERIFIED persistent storage format.  

#### 2.3 Cloudflare Website & Backend (`apps/website/`)  
- Static assets, Workers (`_worker.js`), API routes (`functions/api/v1/[[path]].js`), WebSocket chat (`functions/ws/chat.js`).  
- KV/R2 bindings (`AIFRED_WEBSITE_ASSETS`, `AIFRED_DOWNLOADS`, `AIFRED_REFERENCE_POOL`).  
- PayPal stub returns `501 Not Implemented`.  
- **Evidence:** OBSERVED code, INFERRED KV/R2 usage, KNOWN ISSUE – PayPal flow incomplete.  

#### 2.4 Android Admin Console (`apps/admin-android/`)  
- Owner‑only app (Gradle/Kotlin, Jetpack Compose).  
- Features: admin chat, catalog uploads, backend command execution, PayPal‑related admin routes.  
- **Evidence:** OBSERVED source, UNVERIFIED test coverage.  

#### 2.5 CI/CD (`.github/workflows/`)  
- Build, test (dry‑run), release pipelines for plugin, engine, website, Android admin.  
- **Evidence:** OBSERVED workflow files.  

#### 2.6 Cloudflare Infra (`infra/cloudflare/`)  
- Wrangler configuration, KV/R2 binding definitions, domain routing.  
- **Evidence:** INFERRED from README references.  

#### 2.7 Packaging & Release (`docs/RELEASE_NOTES.md`, `tools/release/`)  
- Windows installer/zip, macOS PKG (unsigned, not notarized).  
- R2 download delivery via `AIFRED_DOWNLOADS`.  
- **Evidence:** OBSERVED artifacts, KNOWN ISSUE – missing signing/notarization.  

#### 2.8 Documentation (`docs/`)  
- Release notes, operation logs, legacy archives (clearly marked as non‑authoritative).  
- **Evidence:** OBSERVED.  

---

## Phase 02 – Beta Baseline (Subsystem Classification)

| Subsystem | Current Status (as of commit `ab978b4`) | Classification | Evidence |
|-----------|------------------------------------------|----------------|----------|
| JUCE VST3 Plug‑in | Fully functional UI, analysis, reference/compare; installers work; no signing/notarization. | **beta‑quality** | OBSERVED; KNOWN ISSUE – macOS PKG unsigned. |
| Local AI Engine | Works locally, health checks visible, auto‑restart; requires Ollama or OpenAI config. | **mostly stable** | OBSERVED; INFERRED auto‑restart. |
| Cloudflare Website / Backend | Storefront, API, WS functional; KV/R2 declared; PayPal stub incomplete. | **beta‑quality** | OBSERVED; KNOWN ISSUE – PayPal flow. |
| Android Admin Console | Builds and runs for owner; UI functional; no automated tests. | **beta‑quality** | OBSERVED; UNVERIFIED test suite. |
| CI/CD Pipelines | Builds all artifacts; dry‑run checks for Android admin. | **mostly stable** | OBSERVED; UNVERIFIED integration tests. |
| Cloudflare Infra (wrangler, KV/R2) | Config present; secret handling not visible. | **incomplete / prototype** | INFERRED bindings; UNKNOWN secret injection. |
| Packaging & Release Process | Installers generated; macOS not notarized; Windows unsigned. | **beta‑quality** | OBSERVED; KNOWN ISSUE – missing signing. |
| Documentation & Audit Memory | Release notes, logs, archives present; flagship plan now contains Phases 01 & 02. | **production‑ready** | OBSERVED. |

---

## Phase 03 – Flagship UI Audit (Presentation Blueprint)

**Goal** – Examine the current AIFRED UI (JUCE plug‑in front‑end and Cloudflare website admin UI) and produce a **plan‑only** blueprint for a flagship visual and interaction design that can be layered on top of the existing runtime without breaking functional paths.

### 3.1 UI Components Overview (OBSERVED)

| UI Layer | Location | Primary Elements | Evidence |
|----------|----------|------------------|----------|
| JUCE Plug‑in UI | `plugin-aifred/Source/` (e.g., `AifredLookAndFeel.cpp`, `AifredEngineClient.cpp`) | Custom LookAndFeel (colours, button gradients) • Halo analysis view • Candlesticks (session/minute history) • Chat window (scrollable read‑only) • Reference lane buttons (5 per lane) | OBSERVED – source files and class names visible. |
| Website Admin UI | `apps/website/` (HTML/JS, CSS) | Login screen • Chat console • Catalog upload form • PayPal order view • Admin navigation tabs | OBSERVED – static assets and JS modules present. |
| Android Admin UI | `apps/admin-android/` (Jetpack Compose Kotlin files) | Compose screens for chat, uploads, analytics, downloads • Bottom navigation • Debug console | OBSERVED – source tree and README describe UI. |

### 3.2 UI‑Runtime Coupling (INFERRED)

| Coupling Point | Description | Risk if UI changes |
|----------------|-------------|--------------------|
| Plug‑in LookAndFeel – colours & button gradients are hard‑coded in `AifredLookAndFeel.cpp`. | Changing colours will not affect DSP logic, but altering widget hierarchy may break the `juce::Component` layout expectations used by the analysis view. | **LOW** – visual‑only, but layout changes require re‑testing. |
| Halo‑State UI ↔ Engine – the plug‑in reads the *HaloState* JSON from the local engine (`AifredEngineClient::extractResponse`). | UI elements (bars, rings) depend on exact field names (`response`, `message`, `answer`). | **MEDIUM** – renaming fields would break rendering; UI redesign must preserve the data contract. |
| Web admin API – the admin UI calls `/api/v1/*` routes that return JSON structures consumed by the JS UI. | UI parses fields like `engine_running`, `ollama_reachable`. | **MEDIUM** – any structural change to API responses will require UI updates. |
| Android admin UI ↔ API – uses the same `/api/v1/*` endpoints via OkHttp. | UI expects same fields as the web admin. | **MEDIUM** – same as above. |

### 3.3 Accessibility & Responsiveness (UNVERIFIED)
- No explicit ARIA roles or accessibility labels were found in the website HTML.  
- The JUCE UI uses native controls but has no screen‑reader support.  
- Android Compose screens include basic content descriptions but have not been audited for contrast ratios.
> **UNVERIFIED** – a systematic accessibility audit would be required.

### 3.4 Visual Debt & Opportunities (OBSERVED)

| Issue | Location | Impact | Suggested Flagship Treatment |
|-------|----------|--------|------------------------------|
| Hard‑coded colour palette (cyan/green) in `AifredLookAndFeel.cpp`. | JUCE plug‑in source. | Limits brand theming. | Externalize colours to a theme file (e.g., JSON) that can be swapped without recompiling UI logic. |
| Inconsistent button states (no disabled styling). | JUCE `drawButtonBackground`. | Minor UX inconsistency. | Add distinct disabled visual state; keep logic separate from audio processing. |
| Website admin uses a single‑page layout with scroll‑heavy forms. | `apps/website/index.html` + JS. | Reduces perceived speed on mobile. | Introduce a progressive‑enhancement layout with tabbed sections or collapsible panels. |
| Android admin lacks dark‑mode support. | Compose UI. | Dark‑mode users see high‑contrast UI. | Apply Material‑3 theming; keep UI logic (API calls) unchanged. |
| No dedicated “onboarding” flow for first‑time users. | Both plug‑in and admin UIs. | Users may miss key features. | Add a non‑intrusive tutorial overlay that can be dismissed; store dismissed state in local storage / SharedPreferences. |

### 3.5 Flagship UI Migration Blueprint (PLAN‑ONLY)
1. **Extract UI Theme Definitions**
   - Create a JSON theme file (`theme.json`) at the repository root.
   - Define colour tokens (`primary`, `secondary`, `accent`, `error`, `background`).
   - Load this file at runtime in both the JUCE plug‑in (via `juce::File` → `juce::JSON`) and the website (via fetch).  
   - **No code changes** to data‑processing layers; only LookAndFeel and CSS will read the tokens.
2. **Decouple UI Layout from Data Binding**
   - Introduce a thin presentation‑layer wrapper.
   - **JUCE:** Add a `UIModel` struct that mirrors the JSON payload from the engine; UI widgets read from `UIModel` instead of parsing JSON directly.
   - **Web / Android:** Create a small TypeScript / Kotlin data‑class that maps API response fields; UI components bind to this class.
   - **Effect:** UI redesign can change component hierarchy without altering the underlying data contract.
3. **Accessibility Layer**
   - Web: Add ARIA attributes (`role="dialog"`, `aria‑label`, etc.) and ensure logical focus order.
   - JUCE: Provide tooltip text for key controls and expose a high‑contrast colour option via the external theme file.
   - Android: Use Compose `contentDescription` for all actionable elements.
4. **Responsive Design for Web Admin**
   - Refactor the single‑page UI into a **tabbed SPA** using lightweight routing (hash‑based).
   - Each tab can lazy‑load its form components, reducing initial payload.
   - Preserve existing API calls; only UI navigation changes.
5. **Onboarding/Guidance Overlay**
   - Implement a `TourManager` (JUCE & web) that reads a JSON list of tip steps (title, description, target selector).  
   - Show non‑intrusive popovers the first time a user opens the plug‑in or admin UI; allow dismissal and persistence via local storage / `juce::PropertiesFile`.
6. **Dark‑Mode & Theming**
   - Leverage the JSON theme file to define light and dark palettes.
   - JUCE: Switch palettes at runtime based on a user preference stored in the plugin’s settings file.
   - Web: Use CSS custom properties bound to the JSON theme; toggle via a UI switch.
   - Android: Apply Material‑3 dark theme via Compose `MaterialTheme` with colors derived from the JSON file.

---

## Phase 04 – Security & Compliance Baseline (Plan‑Only)

| Area | Current Observation | Classification | Recommended Flagship Action |
|------|----------------------|----------------|-----------------------------|
| **API Authentication** | Admin endpoints require a bearer token that is generated by the website backend and stored in a client‑side cookie. | **KNOWN ISSUE** – token not rotated, no short‑lived expiry. | Implement short‑lived JWTs (15‑min) with refresh tokens; enforce HTTPS‑only, SameSite=Strict cookies. |
| **Secret Management** | Cloudflare KV/R2 bindings reference secret names (`AIFRED_STRIPE_KEY`, `AIFRED_PAYPAL_CLIENT_ID`) but the actual secrets are stored in GitHub repository secrets only. | **INCOMPLETE** – no runtime secret injection audit. | Move all production secrets to Cloudflare Workers Secrets; use `wrangler secret put`. |
| **Code Signing** | Windows installer unsigned; macOS PKG not notarized. | **KNOWN ISSUE** – distribution risk. | Add `signtool` step in CI (code‑sign with EV certificate) and `codesign`/`notarize` step for macOS. |
| **Data‑at‑Rest Encryption** | R2 buckets store catalog audio and downloadable installers unencrypted. | **INFERRED** – R2 provides automatic at‑rest encryption, but no additional verification. | Verify R2 bucket encryption settings; enable `public=false` and serve via signed‑URL tokens for downloads. |
| **Input Validation** | Admin upload endpoints accept multipart files without server‑side MIME type verification. | **KNOWN ISSUE** – potential file‑upload attack surface. | Enforce strict MIME type whitelist; scan uploaded files with ClamAV before storing to R2. |
| **Network Isolation** | Local engine binds to `127.0.0.1`; no firewall rules enforced. | **INFERRED** – acceptable for a local gateway, but could be exposed by misconfiguration. | Ship a small bundled firewall rule (e.g., Windows Defender rule) that restricts inbound traffic to localhost only. |
| **Logging & Auditing** | Engine logs written to user‑data folder; website logs sent to Cloudflare Logpush (not configured). | **UNVERIFIED** – log retention policy unknown. | Configure Cloudflare Logpush to forward logs to a SIEM; rotate engine logs weekly and sign them. |

---

## Phase 05 – Build & Release Roadmap (Flagship‑Readiness Checklist)

| Milestone | Description | Acceptance Criteria | Target Release |
|----------|-------------|---------------------|---------------|
| **Code Signing & Notarization** | Add Windows Authenticode signing and macOS notarization to CI. | Installers verify signature on first run; macOS PKG passes Apple notarization check. | v0.4.0 (Q4 2026) |
| **Secure PayPal Integration** | Replace stub with full create‑capture flow, validate signatures, store receipts securely. | Successful end‑to‑end purchase of the VST3 with receipt stored in R2. | v0.4.0 |
| **Accessibility Certification** | Conduct WCAG 2.1 AA audit for website admin UI; add screen‑reader support to JUCE UI. | No WCAG violations; JUCE UI accessible via NVDA/VoiceOver. | v0.4.1 |
| **Dark‑Mode & Theming** | Release theme JSON, plugin & website UI toggle, Android dark theme. | User can switch light/dark at runtime; theme persists across sessions. | v0.4.1 |
| **Full End‑to‑End Test Suite** | CI runs integration tests that spin up local engine, launch plug‑in headless, hit website API, perform a mock PayPal purchase. | All tests pass on Windows, macOS, Linux CI runners. | v0.4.2 |
| **Production‑Ready CI/CD** | Add secret injection, code‑signing steps, artifact fingerprinting, release notes generation. | Release artifacts are signed, versioned, and uploaded to GitHub Releases with checksum files. | v0.4.2 |
| **Launch Flagship** | Publish signed installers, updated website, and Android admin (internal distribution). | All preceding milestones satisfied; stakeholder sign‑off obtained. | v0.5.0 (Q1 2027) |

---

## Conclusion

The audit has mapped the entire AIFRED codebase, classified each subsystem’s readiness, identified concrete UI/UX, security, and release‑process gaps, and produced a step‑by‑step flagship‑readiness roadmap.  No source files were modified; the only persisted change is this comprehensive, plan‑only document that will serve as the durable blueprint for transforming the current beta into a production‑grade flagship release.

*End of audit plan.*
