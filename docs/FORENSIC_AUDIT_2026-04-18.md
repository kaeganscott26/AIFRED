# AIFRED Forensic Audit - 2026-04-18

Current workspace:

- Root: `C:\Users\North\Documents\Projects\AIFRED`
- Git remote: `https://github.com/kaeganscott26/AIFRED`
- Branch: `main`
- Working tree at audit start: clean
- GitHub auth: PASS, authenticated as `kaeganscott26`
- Current GitHub repo visibility: FAIL for the requested privacy model, `kaeganscott26/AIFRED` is public
- Live website health:
  - `https://www.north3rnlight3r.com/api/v1/health`: PASS, returned `{"ok":true,"service":"AIFRED website backend"}`
  - `https://north3rnlight3r.com/api/v1/health`: PASS, returned `{"ok":true,"service":"AIFRED website backend"}`

## Inventory

| Area | Existing source/artifact | Classification | Evidence |
| --- | --- | --- | --- |
| Plugin source | `plugin-aifred/` | PARTIAL | JUCE VST3 target exists and CI builds pass. DSP is K-weighted/RMS-style, not full EBU R128 integrated/short-term/momentary with gated LRA and 4x true peak. |
| Plugin installer | `tools/AifredWindowsInstaller/` and CI release assets | PARTIAL | Draft release `v0.3.4` has uploaded Windows/macOS/Arch assets, but release is draft and repo still mixes all products. |
| Local build tree | `build/`, `dist/` | UNKNOWN | No local build or dist artifacts were present at audit time. |
| Website source | `website/` | PARTIAL | Cloudflare Pages source exists and live health works. Purchase/download path points to GitHub draft/public release assets instead of verified paid R2 delivery. |
| Duplicate website source | `apps/website/` | FAIL | Contains editable copies of `index.html` and `styles.css` that differ from `website/`; not the active Cloudflare Pages source. |
| Website backend | `website/_worker.js`, `website/functions/` | PARTIAL | Health/catalog/chat/admin routes exist. Several admin/business routes return empty or local-only responses, and PayPal/R2 delivery is not implemented. |
| Admin app | `android_admin/` | PARTIAL | Android app source exists and CI builds debug APK. Local Java/ADB are not currently callable in this shell, and fake/empty analytics/sales/log routes remain. |
| GitHub CI | `.github/workflows/build.yml` | PARTIAL | Latest run `24600900490` succeeded for website, Android admin, VST3 Windows/macOS, Arch Linux, and Cloudflare deploy. Workflow still includes macOS despite the current prompt requiring Windows/Linux only. |
| GitHub release | Draft `v0.3.4` | PARTIAL | Draft has assets: `AIFRED-VST3-Setup.exe`, Windows zip, macOS zip, Arch tarball. It is not a published final release. |
| Cloudflare deploy | `website/wrangler.toml`, GitHub deploy job | PARTIAL | Latest CI deploy job succeeded and live health works. Local `wrangler` command is not installed globally; `npx wrangler` is available through Node/npm. |
| AI/chat | Plugin UI and website/backend chat routes | PARTIAL | Website has backend and BYO browser provider paths. Plugin currently stores endpoint/key and echoes chat input; no async provider abstraction or DSP-aware AI context exists in plugin. |
| Reference gate | Website `proGate()` | PARTIAL | Gate is broad but still returns binary accepted/disposed. It does not implement the requested Strong/Usable/Style-Specific/Technically Hot/Poor/Reject classification model. |
| Payment/delivery | PayPal links in website JS | FAKE/PARTIAL | Links open PayPal `_xclick`; there is no verified PayPal Orders capture, webhook, R2 token, or controlled installer delivery endpoint. |

## Dependency Verification

| Dependency | Status | Evidence |
| --- | --- | --- |
| Git | PASS | `git version 2.53.0.windows.3` |
| GitHub CLI | PASS | `gh version 2.90.0`, authenticated |
| CMake | PASS | `cmake version 4.3.1` |
| Visual Studio C++ toolchain | PASS with dev shell | `cl` works after launching `C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Launch-VsDevShell.ps1` |
| Ninja | FAIL | Not found |
| Python/pip | FAIL | Microsoft Store shim only; no usable Python interpreter/pip in PATH |
| Node/npm | PASS | Node `v25.9.0`, npm `11.12.1` |
| Java | FAIL | Not found locally |
| Android ADB | FAIL | Not found locally |
| Wrangler | PARTIAL | Global command not found; can be run through `npx wrangler` |
| .NET SDK | PASS | `10.0.202` |

## Source Of Truth Map

| Subsystem | Canonical source now | Required final canonical source |
| --- | --- | --- |
| Plugin | `plugin-aifred/` inside current monorepo | `aifred-plugin` repo |
| Admin app | `android_admin/` inside current monorepo | `aifred-admin` repo |
| Website | `website/` inside current monorepo | `aifred-site` repo |
| Duplicate website files | `apps/website/` | Archive/remove from active workflow |
| Installer | `tools/AifredWindowsInstaller/` and packaging scripts | Plugin repo only |
| Live site | Cloudflare Pages project `north3rnlight3r` | Cloudflare Pages from `aifred-site` repo |
| Release assets | Draft GitHub release in current mixed repo | Plugin repo releases only |

## Fake, Placeholder, Or Misleading Features

- Plugin chat button is not a real AI integration; it echoes prompt/file status.
- Plugin AI settings are saved in project state, not secure local config.
- Plugin tutorial is per-editor-session static state, not `AudioProcessorValueTreeState` session initialization.
- Plugin loudness is K-weighted block loudness, not full EBU R128 momentary/short-term/integrated/LRA.
- Plugin peak is sample peak, not 4x oversampled true peak.
- Website buy/download flow uses direct PayPal/GitHub links, not verified payment capture and protected delivery.
- Admin sales/log/inquiry APIs return empty arrays or generated IDs, so they must not be presented as live analytics.
- `apps/website/` is an editable shadow copy and not the Cloudflare Pages source.
- Current public GitHub repo contains admin app/backend/source docs, conflicting with security docs.

## Developer-Machine Dependencies

- Local build currently depends on Visual Studio dev shell setup.
- Local admin build cannot be proven without Java/Android SDK/ADB in PATH.
- Local deployment cannot be proven without Cloudflare auth and Wrangler via `npx`.
- Existing release packaging can be proven through CI, but clean-host VST loading in FL Studio cannot be proven from this shell without FL Studio or a VST3 test host.

## End-To-End Risks

- Public/private repo separation is not done.
- Current release assets are attached to a draft release under the mixed public repo.
- The website can claim downloads that are not tied to a verified installer publication flow.
- Payment fulfillment is not automated or verified.
- Plugin AI identity is not yet real in the VST binary.
- Full R128 and true-peak accuracy cannot be claimed yet.
- Admin app currently has control-plane UI backed by routes that are only partially real.
- Mac targets remain in CI/docs despite the current prompt requiring Windows/Linux focus and Mac target deletion.

## Phase 0 Result

Phase 0 classification: PARTIAL.

Real:

- Repository checkout and GitHub auth.
- Live Cloudflare health endpoint.
- CI build evidence for plugin, admin app, website checks, and Cloudflare Pages deploy.

Broken/missing:

- Local Python, Java, ADB, Ninja, and global Wrangler availability.
- Three-repo split.
- Full professional plugin DSP/AI architecture.
- Protected payment/download delivery.

Fake or misleading:

- Plugin chat behavior.
- Website paid delivery path.
- Admin analytics/sales/log proof.
- Duplicate editable website source.
