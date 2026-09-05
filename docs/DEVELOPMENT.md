# Development

Inspect branch, HEAD, upstream, working changes, remotes and source ownership before editing. Keep both repositories independently buildable. Do not copy dependencies or runtime implementation from an absolute sibling path. Separate construction work from DSP/GUI/model changes.

Use [architecture](ARCHITECTURE.md), [build](BUILD.md), [testing](TESTING.md), [installation](INSTALLATION.md), [distribution](DISTRIBUTION.md) and [channel ownership](COEXISTENCE.md). New documents need a clear owner; update these documents instead of adding phase logs. Git history holds superseded plans. Keep generated reports under out/<platform>/build/reports and outside canonical instructions.

CURRENT means source implements the feature; it does not certify a release. EXPERIMENTAL means code/tests exist outside the supported native runtime. PLANNED describes a design contract. UNIMPLEMENTED means no executable feature exists. Record skipped tests and untested platforms explicitly.

Preserve user settings, reference data, website assets, model files, deployment config and credentials. Examples should name configuration variables without values. System installation destinations differ from machine-specific checkout paths. User-profile editor settings and synthetic privacy-test paths are not product build configuration.


## Component development contracts

## Repository authorities

- `plugin-aifred/`: JUCE VST3 source.
- `tools/AifredEngine/`: .NET local engine.
- `apps/website/`: static site, `_worker.js`, Pages Functions, `/ops`.
- `apps/admin-android/`: private Kotlin/Compose app and Windows Desktop scripts.
- `apps/admin-desktop/`: shared desktop documentation and macOS source.
- `infra/cloudflare/`: non-authoritative operational Wrangler mirror.
- `integrations/forge/`: discovery manifest, schemas and export bridge.
- `tools/lib/aifred-archive.mjs`: archive implementation.
- `config/admin-commands.json`: command metadata authority.

`packages/plugin-juce/` and `packages/local-engine/` are path pointers, not runtime source. Dated operation reports are history, not configuration.

## Separation model

The plugin calls loopback AifredEngine; the engine calls Ollama or an explicitly configured compatible provider. The website/API runs on Cloudflare. Admin clients call Cloudflare and never replace the plugin's local path. FORGE consumes sanitized exports and archive metadata, not raw production credentials.

## Environment names

Website production/local names are documented in [Cloudflare Production Guide](CLOUDFLARE_PRODUCTION.md). Android build properties: `AIFRED_BASE_URL`, `AIFRED_API_TOKEN`, `AIFRED_ADMIN_USERNAME`, `AIFRED_ADMIN_PASSWORD`; local.properties equivalents are `aifredBaseUrl`, `aifredApiToken`, `aifredAdminUsername`, `aifredAdminPassword`. Desktop/local bridge: `AIFRED_API_BASE_URL`, `AIFRED_REPO_ROOT`, `AIFRED_ADMIN_SESSION_TOKEN`, `AIFRED_FORGE_ACTIVE_LOG_LIMIT_MB`. Never commit values.

## Command lifecycle

1. Edit `config/admin-commands.json`.
2. Add or update the backend implementation in `handleCommand` when behavior changes.
3. Run `node tools/generate-admin-command-reference.mjs`.
4. Generated outputs update backend registry metadata, Android local actions and `ADMIN_COMMAND_REFERENCE.md`.
5. `GET /api/v1/registry/actions` supplies Android; Android submits to the authenticated command endpoint or executes a generated local action.
6. Run `node tools/generate-admin-command-reference.mjs --check` and API tests.

Do not hand-edit generated command files. `/ops` and desktop have no text parser.

## Analytics, exports and archives

`apps/website/lib/activity-log.js` creates compact sanitized events; new KV events expire after 90 days. Web Analytics remains the traffic authority. Exports recursively remove secret-shaped keys and use versioned schemas under `integrations/forge/schemas/`. The bridge mirrors authenticated exports, evaluates the 25 MB default active threshold and calls the archive library. See [Archive Guide](ARCHIVE_GUIDE.md).

## Builds

Use [canonical builds](BUILD.md). Android remains a private SDK 35/JDK 17 Gradle product; run ./gradlew :app:assembleDebug from apps/admin-android. macOS admin uses apps/admin-desktop/macos/build.sh.

## Tests and deployment

```sh
node tools/generate-admin-command-reference.mjs --check
node --test tests/aifred-api.test.mjs tests/aifred-archive.test.mjs
bash tools/release/aifred_monorepo_validate.sh
npm --prefix apps run website:deploy
```

Deploy only `apps/website` to Pages project `aifred-site`. Generated `.app`, Android artifacts, local exports, archive data, `.forge` runtime databases and restore workspaces are ignored.

See [API Reference](API_REFERENCE.md), [Cloudflare Production Guide](CLOUDFLARE_PRODUCTION.md), and [Troubleshooting](TROUBLESHOOTING.md).

Run `python -B scripts/common/check_repository.py` for read-only canonical-path and Markdown-link checks. Generated developer reports belong below out/<platform>/build/reports.

The four tools/release inventory/workflow/admin/website Python entry points retain --check (read-only validation), --stdout (JSON), and default generated-report output. Completed migration parity/gate generators were removed; workflow checks preserve manual-only/read-only validation and release/deployment trigger boundaries. They do not deploy or request credentials.
