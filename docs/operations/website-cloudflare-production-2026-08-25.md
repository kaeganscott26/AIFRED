# Website, Cloudflare, R2, and Android Production Audit — 2026-08-25

## Executive Summary

Production is operational on Cloudflare Pages project `aifred-site` from canonical source `apps/website` in the AIFRED monorepo. The final deployed source is commit `68fc41c72e7fbc513bd5505d24d0bd9b4a58e5dd`.

The website now distributes the Windows installer, Windows portable ZIP, published macOS VST3 ZIP, and all 53 unique catalog MP3s for free through controlled Pages/Worker routes backed by the private `aifred-downloads` R2 bucket. PayPal is disabled. Public activity and inquiries write to KV and do not mutate GitHub.

The private Android admin app was updated, built, linted, installed, and launched as version `2.3.0` (`versionCode 243`). It includes safe local Linux/Termux/Android actions, R2-backed catalog playback through the same absolute HTTPS paths as the website, app-private Website/Ollama/OpenAI API profiles, and authenticated controls matching the Cloudflare runtime API controls in `/ops`. Android loopback cleartext discovery/chat passed an on-device instrumentation test.

The website and Android code paths are ready for Ollama, including bearer-token or Cloudflare Access service-token headers. Production Pages cannot yet reach Ollama because no public HTTPS Ollama gateway/tunnel exists and the available Cloudflare OAuth grant does not include DNS write/Access policy administration. `OLLAMA_BASE_URL` was deliberately not set to `127.0.0.1`, because that address would refer to Cloudflare's runtime rather than the owner's machine and would leave chat broken.

Final production deployment:

- Deployment ID: `f7d9e13d-3a99-4720-9cf7-8d153aaa41bc`
- Immutable URL: `https://f7d9e13d.aifred-site.pages.dev`
- Created: `2026-08-25T09:19:31.458678Z`
- Completed: `2026-08-25T09:19:33.304336Z`
- Source branch: `main`
- Source commit: `68fc41c72e7fbc513bd5505d24d0bd9b4a58e5dd`
- Public aliases: `https://aifred-site.pages.dev`, `https://north3rnlight3r.com`, `https://www.north3rnlight3r.com`

## Previous Production Architecture

The last working production deployment before this cleanup was:

- Deployment ID: `6df05b80-3fde-4db1-9be1-a4f0203dfdd0`
- URL: `https://6df05b80.aifred-site.pages.dev`
- Created: `2026-05-04T06:28:25.197203Z`
- Last modified: `2026-05-07T09:04:09.173741Z`
- Source commit: `1c87bfb...`
- Deployment snapshot source: GitHub repository `kaeganscott26/aifred-site`, branch `main`

The Pages project itself was already named `aifred-site` and already served the current Pages/custom domains. Its project-level Git configuration had subsequently drifted toward the AIFRED repository but still used root `website` and a recursive `npx wrangler pages deploy ...` build command. This mixed an obsolete source tree with a deployment command as a build command and produced many failed historical deployment attempts.

Previous production bindings found through Cloudflare inspection:

- KV `AIFRED_REFERENCE_POOL` -> `8a120701767e474f928d1af7037cd68a`
- KV `AIFRED_SALES_LOG` -> `2c66da7795b54135a4d67e514b97491f`
- R2 `AIFRED_DOWNLOADS` -> `aifred-downloads`
- R2 `AIFRED_REFERENCE_BUCKET` -> `aifred-reference-pool`
- Service `MAILER` -> missing service `aifred-mailer`
- Compatibility date `2026-04-22`
- No compatibility flags

Only `aifred-downloads` and `aifred-reference-pool` exist in the account. The previously referenced `aifred-website-assets` bucket does not exist. Cloudflare did not expose active PayPal secret names in the production deployment configuration inspected during this work.

## New Architecture

Website delivery:

```text
GitHub main
-> CI syntax, API, path, and monorepo validation
-> explicit repository-defined Wrangler deployment
-> Cloudflare Pages project aifred-site
-> Pages/Worker API allowlist
-> private R2 bucket aifred-downloads
-> browser download or byte-range audio stream
```

Website activity:

```text
website activity or inquiry
-> Pages/Worker API
-> AIFRED_SALES_LOG KV
```

Runtime model administration:

```text
authenticated /ops or Android admin app
-> secret-safe admin API
-> provider/endpoint/model selection in AIFRED_SALES_LOG KV
-> Pages chat router
-> OpenAI HTTPS API or protected Ollama HTTPS gateway
```

Provider keys and Cloudflare Access service-token values stay in Pages secrets and are never returned by the admin API.

The KV name is historical. It now stores ordinary website activity and inquiries as well as retaining read compatibility for historical sales records. Public activity does not fall back to GitHub writes. Explicit authenticated admin file/catalog operations may still intentionally write approved repository paths; deployment remains a separate action.

## Current Production vs New Repository Configuration

| Concern | Previous active/drifted state | Final repository/production state |
|---|---|---|
| Pages project | `aifred-site` | `aifred-site` |
| Deployment snapshot source | old `kaeganscott26/aifred-site` repository | `kaeganscott26/AIFRED` |
| Website authority | obsolete `website` references | `apps/website` |
| Wrangler authority | divergent root/app/infra files | `apps/website/wrangler.toml`; root convenience and infra mirror aligned |
| Wrangler version | ad-hoc/latest use existed | pinned `4.125.0` in `apps/package-lock.json` |
| Local deploy | inconsistent/ad-hoc | `npm --prefix apps run website:deploy` |
| CI deploy | recursive/duplicated Pages behavior | same repository script; deploy job is manual-only |
| Native Git deployment | enabled/drifted | disabled to prevent double deployment; Git pushes validate/package |
| Compatibility | `2026-04-22`, no flags | `2026-08-25`, `nodejs_compat` |
| Obsolete service | `MAILER` -> missing `aifred-mailer` | removed |
| Website-assets R2 | nonexistent `aifred-website-assets` referenced | removed; `aifred-downloads` is canonical |
| Plugin distribution | payment-gated/PayPal-oriented code and UX | free allowlisted R2 routes |
| macOS distribution | absent from website module | verified published macOS ZIP exposed as manual install |
| Catalog | price/checkout language and one stale WAV route | 53 unique free MP3 cards, each resolving to a checked-in/R2 object |
| Public telemetry | GitHub fallback could create commits | KV-only writes |
| Operations auth | incomplete/legacy configuration | Cloudflare username, password verifier, and session-signing secrets configured |
| Runtime model administration | environment-only and redeploy-dependent | authenticated `/ops` and Android controls; non-secret routing stored in KV |
| Android local Ollama | Android blocked cleartext loopback; provider inferred from URL shape | private/loopback HTTP allowed by app policy; provider explicit; arbitrary public HTTP rejected |
| Pages Ollama | loopback example was not remotely reachable | HTTPS-only protected-gateway contract implemented; endpoint remains blocked pending tunnel/DNS/Access setup |

Direct Wrangler uploads leave the Cloudflare project build root/destination empty; this is expected because source/build authority is in the repository scripts rather than Cloudflare's native build system. Cloudflare's connected Git source is retained but automatic production and preview deployments are disabled.

## PayPal Removal

Final runtime contract:

```text
distribution.mode = free
payment_pipeline = disabled
```

Removed from active runtime:

- Client-side PayPal SDK and button initialization.
- PayPal configuration, order creation, capture, IPN/webhook, and payment-token download routes.
- Payment-gated download token behavior.
- Checkout, `$5 beta`, `$100`, and `$200` active UX.
- Android manual sale-entry/receipt controls.
- PayPal/service secrets as deployment requirements.

Retained only for historical compatibility:

- Read-only historical sale record readers.
- Android labels and event classifiers that can display old PayPal/sale events already stored.
- Dated migration documents, now marked historical and superseded.

Documentation:

- Current README, state, release notes, wiki, Cloudflare guides, Android manuals, smoke tests, and status records describe free distribution.
- Historical phase documents carry a banner directing readers to current authority.

Dead/unused payment implementation was deleted where safe. Historical business records were not deleted.

## Cloudflare Bindings

Final production contract:

- Project: `aifred-site`
- Production branch: `main`
- Compatibility date: `2026-08-25`
- Compatibility flags: `nodejs_compat`
- KV `AIFRED_REFERENCE_POOL`: `8a120701767e474f928d1af7037cd68a`
- KV `AIFRED_SALES_LOG`: `2c66da7795b54135a4d67e514b97491f`
- R2 `AIFRED_DOWNLOADS`: `aifred-downloads`
- R2 `AIFRED_REFERENCE_BUCKET`: `aifred-reference-pool`
- Service bindings: none
- Operator configuration: `AIFRED_ADMIN_USERNAME`, `AIFRED_ADMIN_PASSWORD_SHA256`, and `AIFRED_ADMIN_SESSION_SECRET` are production secrets; values are intentionally omitted.
- Optional model secrets: `OPENAI_API_KEY`, `OLLAMA_API_TOKEN`, `OLLAMA_ACCESS_CLIENT_ID`, and `OLLAMA_ACCESS_CLIENT_SECRET`; none of their values are exposed through `/ops` or the API.
- Runtime provider/endpoint/model selection: KV key `admin:config:api-runtime` under `AIFRED_SALES_LOG`.
- PayPal variables: none required.

Cloudflare still reports two malformed secret-name entries and one unreferenced legacy Cloudflare-named variable. Their names/values are intentionally not reproduced. They are not referenced by the runtime, but the owner should identify, remove, and rotate them through the Cloudflare dashboard/API. Preview deployments are disabled; the inactive preview configuration still has an older compatibility date and should be normalized before previews are re-enabled.

## R2 Inventory

The bucket remains private. Current source contains 63 meaningful website assets totaling `276,250,630` bytes. Initial upload established complete source/R2 key and size parity; later changed data/doc objects were uploaded with `--remote` and verified incrementally. One superseded system-requirements TXT object remains in R2 for legacy compatibility after the source document was renamed to Markdown.

### Release objects

| Local verified source | R2 key | Bytes | Content type | Result |
|---|---|---:|---|---|
| `/tmp/aifred-release-audit.g651Me/github/AIFRED-VST3-Setup.exe` | `releases/v0.3.6-installer-ai-alias/AIFRED-VST3-Setup.exe` | 86,152,274 | `application/vnd.microsoft.portable-executable` | uploaded/verified |
| `/tmp/aifred-release-audit.g651Me/github/AIFRED-VST3-windows.zip` | `releases/v0.3.6-installer-ai-alias/AIFRED-VST3-windows.zip` | 34,498,150 | `application/zip` | replaced mismatched object with official release asset |
| `/tmp/aifred-release-audit.g651Me/github/AIFRED-VST3-macos.zip` | `releases/v0.3.6-installer-ai-alias/AIFRED-VST3-macos.zip` | 1,701,201 | `application/zip` | uploaded/verified |
| prior R2 Windows ZIP | `releases/v0.3.6-installer-ai-alias/legacy/AIFRED-VST3-windows-pre-20260825.zip` | 34,925,966 | `application/zip` | preserved for recovery |

Verified official hashes:

- Setup: `619ec7f22c1d55c1f1c458b3e0f897ada51e9ad66f8698fc9a1c0d1abd427448`
- Windows ZIP: `36290a1dbfbe18b1db67305895ee3dfb211a0ff013c05668b897a7e11ab65d30`
- macOS ZIP: `3ffc75d77cb4f4a47f4a388944665e21a6ca1fbee550755b9cf7dd4c03cf9461`

No `.pkg` or `.dmg` exists in the repository, local build outputs, or published GitHub release inspected. The website therefore accurately labels the macOS artifact as **macOS Plugin (ZIP)** and documents manual VST3 installation; it is not mislabeled as a signed installer.

### Catalog MP3 objects

All keys below are under `assets/audio/catalog/` and were matched to local source by key and size:

```text
3amDrill.mp3|5423063
3amDrillinst.mp3|5423063
3peattype.mp3|4521525
Bazar.mp3|5383566
Bergundy Remix North3rnLight3r.mp3|6579138
Celestial Prequal.mp3|7243693
Cocaina Remix.mp3|4243791
Color of Your Soul Remix North3rnLight3er.mp3|4115896
Dope prod North3rnLight3r.mp3|2838194
IzotopeMix.mp3|6080095
Knockin Knoggins Rmix.mp3|5441244
LIfe is Reluctant - North3rn Light3r version.mp3|5071350
LiquidBeat.mp3|5048181
Lullaby.mp3|4938439
Mongolia Throat Beat.mp3|3784872
No Complaints.mp3|5592963
No Complaintsinst.mp3|9320698
North3rnLight3r instru.mp3|6320914
Northernlighterbeatmastewr.mp3|4500851
Phantom Dear mixed by North3rnLight3r.mp3|4712741
Pink Floyd Money North3rnLight3r Flip.mp3|4253195
Put It Downprev.mp3|4664482
REVENANTpremaster.mp3|3964176
Rude Boy remix.mp3|2480212
SickAF beat prod by North3rnLight3r.mp3|9438209
Sierra Leone Remix prod. North3rnLight3r.mp3|5269463
SpanishMoss master.mp3|4376702
Streets of Mongolia [mastered].mp3|10605266
Televeve Remix.mp3|5256297
TrapDubMix.mp3|3855716
Turn The Page cover.mp3|7149652
When Im With You prod. North3rnLight3r.mp3|3457610
astromaster.mp3|6100159
coolvoxboombapbeat.mp3|6828466
coolvoxboombapbeatmaster.mp3|4097714
gucciplukcs.mp3|3517796
infinite prod. North3rnLight3r.mp3|4377956
june27remixwav.mp3|7091974
losing faith premaster.mp3|4213711
newtrapshit2.mp3|3620664
rnbINST.mp3|2724105
smokin green screwed instrummp3.mp3|8850576
sound of my dreams dubstep.mp3|2634439
sound of my dreams master dubstep.mp3|1977408
spacemaster.mp3|5096428
sugarplum beat.mp3|4768539
suicideboystypewav.mp3|6579138
swingtrap.mp3|1209408
take off.mp3|3845058
timeforthatmp3.mp3|6056311
uoeno.mp3|4534064
weendinstrumental.mp3|6147177
yothisbeatiscrazy.mp3|5764168
```

The final catalog contains exactly 53 cards, 53 unique asset keys, no missing local objects, and free-distribution language on every card. The stale `Throat_beat2_no_limiter.wav` route was repointed to the existing `Streets of Mongolia [mastered].mp3`; a duplicate historical `Lullaby.wav` card was removed.

## Download API

Stable plugin routes:

```text
GET|HEAD /api/v1/downloads/plugin?asset=setup
GET|HEAD /api/v1/downloads/plugin?asset=zip
GET|HEAD /api/v1/downloads/plugin?asset=macos
```

Catalog routes:

```text
GET|HEAD /api/v1/assets/audio/catalog/<allowlisted-catalog-file>
GET|HEAD /api/v1/assets/audio/catalog/<allowlisted-catalog-file>?download=1
```

The Worker rejects traversal, does not expose arbitrary bucket keys, returns 404 for missing objects, returns a distinct unavailable response when the R2 binding is absent, supports HEAD, sets MIME/length/disposition/range/cache headers, and handles valid/invalid byte ranges with 206/416 responses. Audio seeking works through byte ranges.

## Android Admin 2.3.0

- Application ID: `com.aifred.admin`
- Version: `2.3.0`
- Version code: `243` (higher than the previous 241 so Android accepts an upgrade)
- APK: `apps/admin-android/app/build/outputs/apk/debug/app-debug.apk`
- APK bytes: `27,937,574`
- APK SHA-256: `930e4bd8c290f4c481134d50b9c9680de9b3f7157bbf0fc298f20bcd4a916b61`
- Connected device: Motorola moto g play (2026)
- Install result: `adb install -r` success
- Installed timestamp: `2026-08-25 04:22:17` local
- Launch result: cold launch succeeded; process remained running; no matching fatal exception found.

Android changes:

- Removed active/manual payment-entry controls; historical sales remain read-only.
- Added 15 local-only read-only/non-root Linux, Termux, and Android diagnostic actions.
- Kept backend actions server allowlisted and separate from local shell actions.
- Resolved website-relative catalog URLs against the production base before `MediaPlayer.setDataSource`, eliminating the `/api/... ENOENT` local-path failure.
- Added Website, direct Ollama, and direct OpenAI API profiles with editable endpoint/model/key, connection test, model discovery, and apply/save controls.
- Added authenticated **Save Website Route** and **Test Website Route** controls matching `/ops`; Cloudflare routing values persist to KV while secrets remain deployment-managed.
- Enabled Android cleartext transport for local-model compatibility, with app validation limiting HTTP API profiles to loopback/RFC1918 private hosts and rejecting public cleartext endpoints.
- Made provider selection explicit so custom Ollama HTTPS tunnels and non-default local ports use `/api/tags` and `/api/chat` correctly.
- Removed the tracked hardcoded password verifier; offline unlock compares only with credentials explicitly saved in private, non-backed-up app storage.
- API profile/key data stays in app-private preferences; Android backup was disabled. No API key was committed.
- Direct Ollama guidance explains that phone `127.0.0.1` is the phone itself and a remote host requires a reachable LAN endpoint.

## Tests

Commands and results:

| Command/check | Result |
|---|---|
| `git status`, `git diff`, `git log --oneline -15` | current worktree/history audited before edits |
| `npm ci --prefix apps` | passed; 0 vulnerabilities |
| `npm --prefix apps run website:check` | passed; final 23/23 tests |
| `tools/release/aifred_monorepo_validate.sh --gradle` | passed |
| `git diff --check` | passed before each commit |
| `./gradlew :app:testDebugUnitTest :app:lintDebug :app:assembleDebug` | passed; API profile and catalog URL unit tests passed; lint passed; APK assembled |
| `./gradlew connectedDebugAndroidTest` | passed on the connected phone; 1/1 test verified loopback cleartext Ollama discovery and chat |
| JSON validation for `api/v1/registry/actions` | passed |
| Static catalog parity check | 53 tracks, 53 unique assets, 0 missing |
| `npx --prefix apps wrangler whoami` | authenticated to expected Cloudflare account |
| Pages project/deployment inspection | passed |
| R2 bucket/object inspection | passed |
| Remote R2 download/hash comparison for changed docs | exact match |
| Full setup/Windows ZIP/macOS ZIP production downloads | exact source size/hash; files identified as PE/ZIP, not HTML |
| Plugin and MP3 range requests | 206, exact 1,024-byte bodies and correct `Content-Range` |
| Invalid MP3 range | 416 |
| GitHub `Website checks` on final source commit | passed |
| GitHub automatic `Dependency Graph` after filename fix | passed |
| APK `adb install -r`, package/version inspection, launch, fatal-log scan | passed |

Website tests cover health, models, malformed/streaming chat, admin authorization, secret-safe KV-backed API configuration, protected Ollama gateway headers/chat, Cloudflare loopback rejection, free content, complete catalog parity, free setup/ZIP/macOS resolution, HEAD, missing object/binding behavior, beat attachment, ranges, invalid ranges, traversal, KV-only activity/inquiries, and removed PayPal routes.

## Git

Implementation commits pushed in deployment order:

```text
70caabd fix website distribution and Cloudflare deployment pipeline
1857d7f fix static free download actions
c33f77e add free macOS plugin download
121775c fix Pages Wrangler validation
a913ac7 fix R2 byte range responses
686ea9e update Android admin 2.3.0 and repository documentation
5d1746f fix dependency graph document detection
20d12b4 fix catalog download asset parity
d46d6bf remove tracked operator credentials
dfad5d4 fix Android catalog playback and add API profiles
68fc41c add secure runtime model configuration
```

`HEAD` and `origin/main` matched `68fc41c72e7fbc513bd5505d24d0bd9b4a58e5dd` before final production deployment. Pushes succeeded except for one normal non-fast-forward rejection caused by a concurrent owner commit; that commit was fetched/rebased without force-pushing or discarding it.

The concurrent commit `d135c1e` placed operator credential values in tracked `.dev.vars.example`. Commit `d46d6bf` immediately restored empty example values and the correct password-hash key. The values remain recoverable from Git history and therefore require rotation; this report does not reproduce them.

## Deployment

Final deployment command:

```bash
npm --prefix apps run website:deploy
```

Final result:

- Deployment ID: `f7d9e13d-3a99-4720-9cf7-8d153aaa41bc`
- Deployment URL: `https://f7d9e13d.aifred-site.pages.dev`
- Production URLs: `https://aifred-site.pages.dev`, `https://north3rnlight3r.com`, `https://www.north3rnlight3r.com`
- Commit: `68fc41c72e7fbc513bd5505d24d0bd9b4a58e5dd`
- Timestamp: created `2026-08-25T09:19:31.458678Z`; success `2026-08-25T09:19:33.304336Z`
- Result: success

An initial deployment of the same commit succeeded as `b8b5d377-671e-4950-97be-0e9e73fd6eb5`, but Cloudflare metadata marked it dirty because the unfinished, untracked audit draft was present outside the deployed website directory. The draft was temporarily stashed and the exact pushed source was redeployed; the final deployment above records `commit_dirty=false`.

The immutable deployment-specific hostname returns 302 to Cloudflare Access for anonymous requests. Public Pages/custom aliases return 200 and serve this deployment. This Access behavior was retained rather than weakening deployment-host protection.

## Production Verification

Final public checks:

| Endpoint/behavior | Result |
|---|---|
| `aifred-site.pages.dev/` | 200; free/macOS module present |
| `north3rnlight3r.com/` | 200; free/macOS module present |
| `www.north3rnlight3r.com/` | 200; free/macOS module present |
| Homepage CSS/JS/config | 200 |
| `/api/v1/health` | 200 |
| `/api/v1/content/get` | 200; free distribution |
| `/api/v1/catalog/list` | 200; 53 unique tracks, all free, no missing path |
| Release notes | 200 |
| `/ops` | 200 |
| Valid operator login | 200; authenticated `/api/v1/admin/ops/status` 200 |
| Authenticated `/api/v1/admin/api/config` | 200; provider/model config stored in KV; `secret_values_exposed=false` |
| Authenticated Website provider test | 200 |
| Authenticated Ollama/OpenAI provider tests | 503 with explicit missing configuration; no secret leakage |
| Attempted Pages Ollama loopback config | 400; insecure/unreachable HTTP endpoint rejected |
| Invalid operator login | 401 |
| Unauthenticated admin ops/dashboard | 401 |
| PayPal config/create/capture/IPN and sale-download routes | 404 |
| Windows setup HEAD | 200; 86,152,274 bytes; PE MIME; attachment; ranges |
| Windows ZIP HEAD | 200; 34,498,150 bytes; ZIP MIME; attachment; ranges |
| macOS ZIP HEAD | 200; 1,701,201 bytes; ZIP MIME; attachment; ranges |
| Setup/Windows/macOS byte ranges | 206; 1,024 bytes; correct total |
| `3amDrill.mp3` HEAD/range/download | 200/206/200 attachment; audio MIME; seeking headers |
| `Streets of Mongolia [mastered].mp3` | 200; range 206; 10,605,266-byte total |
| Invalid MP3 range | 416 |
| Full plugin bodies | source hashes matched exactly |
| Contact smoke submission | KV stored `true`; email sent `false` because no EMAIL binding |

Contact smoke inquiry ID: `49904531-2a1c-4f55-89b8-e154823d3356`. This is an intentional production smoke record.

## Blockers

### Fixed

1. **Invalid Pages Wrangler field** — Pages rejected `observability`; removed from Pages config and added a regression validation. Deploy then succeeded.
2. **Divergent deploy contracts** — root/app/infra/CI used different paths/project behavior; normalized on `aifred-site`, `apps/website`, and pinned Wrangler.
3. **Recursive native build/deploy** — Cloudflare native Git automation was disabled; repository scripts now own deployment.
4. **Missing MAILER service** — obsolete binding and associated production secret requirement removed.
5. **Nonexistent website-assets bucket** — removed; all distribution uses existing `aifred-downloads`.
6. **R2 Windows ZIP drift** — prior object differed from official release; preserved under `legacy/`, then canonical key replaced with official asset.
7. **R2 upload transient failures** — six network failures were retried; final object parity succeeded.
8. **Wrangler local R2 default** — two doc updates initially went to local emulation; rerun with `--remote`, downloaded again, and hashes matched.
9. **R2 byte-range semantics** — live testing exposed incorrect range reads; implemented explicit offset/length handling and 416 validation.
10. **Invisible static download controls** — installer/ZIP/macOS/release-note links now exist directly in `index.html`, not only injected JavaScript.
11. **Missing macOS website option** — verified published macOS ZIP uploaded and exposed through an allowlisted API route.
12. **Stale catalog WAV route and duplicate card** — repointed Streets of Mongolia to its real MP3 and removed duplicate Lullaby metadata; parity test prevents recurrence.
13. **GitHub dependency graph failure** — `aifred-system-requirements.txt` was misclassified as Python requirements; renamed to Markdown. Next dependency graph run passed.
14. **Android SDK/toolchain absent** — JDK 17, command-line tools, platform tools, API 35, and required build tools installed.
15. **Android compile/lint issues** — nullable JSON handling and notification permission race fixed; build/lint now pass without a baseline.
16. **ADB authorization** — device authorization prompt accepted; install/launch passed.
17. **Android ENOENT playback** — relative API paths are now resolved to full website HTTPS paths before MediaPlayer opens them.
18. **Operations credentials missing** — username, password verifier, and random session-signing secret configured in Cloudflare; live login verified.
19. **Android cleartext policy failure** — manifest permits local-model cleartext; runtime validation rejects public HTTP endpoints; on-device Ollama-style discovery/chat instrumentation passed.
20. **Custom Ollama endpoint inference** — Android now carries the selected provider explicitly, so non-default ports and HTTPS tunnel hostnames use Ollama routes correctly.
21. **Admin model-routing parity** — `/ops` and Android now read/save/test the same authenticated, KV-backed Cloudflare runtime route without exposing secrets.
22. **Tracked Android password verifier** — removed; no reusable password or verifier remains in current APK source.

### Remaining/remediation

1. **No native macOS installer exists.** Current deliverable is a manual VST3 ZIP. Build a real pkg/dmg, sign with Developer ID, notarize/staple, publish it as a release asset, upload the exact artifact to R2, then add a distinct allowlisted asset key and tests.
2. **Operator credentials appeared in Git history in concurrent commit `d135c1e`.** Current tracked files are clean, but removing the file contents does not erase history. Rotate the operator password and session secret, update Cloudflare secrets, redeploy, then coordinate a history rewrite with `git filter-repo` and protected `--force-with-lease` only if repository exposure policy requires complete removal.
3. **Cloudflare malformed/legacy secret names remain.** Identify the three unreferenced entries in the Pages production environment, remove them, and rotate any value that resembles a credential. Do not copy their names/values into issues or docs.
4. **Contact email delivery is not configured.** Inquiry persistence works in KV. To send notifications, verify a sender/domain, configure the supported EMAIL binding, and repeat contact smoke verification.
5. **Immutable deployment URLs are Access-protected.** This is intentional. Use public aliases for anonymous verification or create a narrowly scoped Access service token if immutable-host CI verification is required.
6. **Inactive preview configuration is older.** Preview deployment is disabled. Before re-enabling, align compatibility/bindings and validate on a non-production branch.
7. **Cloudflare cannot reach local Ollama yet.** Local Ollama listens on `127.0.0.1:11434`; no `cloudflared` connector/HTTPS hostname currently exists, and the available OAuth grant has zone read but not DNS write or Access application policy administration. Create a named tunnel such as `ollama.north3rnlight3r.com -> http://127.0.0.1:11434`, protect it with a Cloudflare Access service-token policy (or an authenticated Ollama gateway), run the connector as a supervised service, then set `OLLAMA_BASE_URL`, `OLLAMA_ACCESS_CLIENT_ID`, and `OLLAMA_ACCESS_CLIENT_SECRET` in Pages production and use `/ops` **Test selected provider** before switching the provider to Ollama. Do not use an unauthenticated Quick Tunnel.
8. **OpenAI is not configured in Pages.** `/ops` correctly reports `OPENAI_API_KEY is not configured`. Add it as a Pages production secret, test through `/ops`, then select OpenAI only after a successful test.

## Remaining Technical Debt

- Rename/migrate historically named `AIFRED_SALES_LOG` to a neutral activity namespace while preserving historical records.
- Add rate limiting/Turnstile to public activity and inquiry endpoints.
- Add a verified email binding if notification delivery is required.
- Produce and notarize a real macOS installer with the packaged AIFRED Engine.
- Decide whether to delete the legacy R2 Windows ZIP and old TXT requirements object after a retention period.
- Audit/remove the malformed Cloudflare variable names and rotate the operator credential exposed in Git history.
- Consider app-level encrypted storage backed by Android Keystore for API/admin credentials; current preferences are private and backups are disabled but are not hardware-backed encryption.
- Provision and supervise an authenticated HTTPS Ollama gateway before selecting Ollama for the public Pages runtime.
- Split full VST plugin builds from documentation-only pushes to reduce CI time; website checks already complete quickly and deployment is manual.
- Keep explicit authenticated admin GitHub writes constrained to approved low-frequency file/catalog operations.
- If preview deployments return, normalize the inactive preview environment before enabling them.

## Final State

At report creation:

- Production website: operational.
- Free Windows/macOS plugin delivery: operational.
- Free catalog playback/download: operational for 53 unique MP3s.
- R2 bucket: private and operational through the Worker/API.
- PayPal: disabled and not required.
- Activity/inquiry persistence: KV-first, no public GitHub writes.
- `/ops`: operator authentication configured and verified.
- `/ops` runtime API module: deployed, authenticated, KV-backed, and secret-safe; Website profile test passes.
- Cloudflare Ollama/OpenAI model generation: code ready but blocked until reachable/provider secrets are configured; not falsely reported operational.
- Android 2.3.0: built, tested, installed, and launched.
- Git deployment source: pushed and matched `origin/main` before deployment.
