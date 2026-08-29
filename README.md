# AIFRED

AIFRED is a private audio-analysis ecosystem: a JUCE VST3 and local model gateway, a Cloudflare-hosted website/API, owner-only admin clients, and a bounded FORGE integration with desktop-owned cold storage.

## Runtime components

| Component | Authority | Current role |
| --- | --- | --- |
| VST3 0.3.6 | `plugin-aifred/` | DAW analysis, reference comparison, interpreted mix state, local-engine chat |
| AifredEngine 1.0.0 | `tools/AifredEngine/` | Loopback gateway on `127.0.0.1:8787`; Ollama/OpenAI-compatible routing |
| Website/API | `apps/website/` | Public site, analyzer, catalog/downloads, Pages Functions API, `/ops` |
| Android Admin 2.3.0 | `apps/admin-android/` | Private Compose client, uploads, live operations, exports, command terminal |
| Desktop Admin | `apps/admin-android/tools/windows-admin/`, `apps/admin-desktop/` | Live admin controls plus local archive management |
| FORGE bridge 1.1.0 | `integrations/forge/` | Credential-free discovery, bounded current export mirror, archive pointers |
| Archive schema 1.0.0 | `tools/aifred-archive.mjs` | Verified gzip JSONL cold storage under ignored `runtime/aifred-archive/` |

## Runtime flow

```text
VST3 -> AifredEngine (127.0.0.1:8787) -> Ollama (127.0.0.1:11434)
                                      -> configured OpenAI-compatible provider

Browser / Android / Desktop -> https://www.north3rnlight3r.com
                            -> Cloudflare Pages + Functions
                            -> KV activity/reference data + private R2 objects

FORGE -> latest/current sanitized exports
      -> configurable 25 MB active threshold
      -> verified desktop/local archive
      -> lightweight manifest retained for bounded retrieval
```

The plugin is intentionally local-first and does not require Cloudflare. Admin clients use the production API and do not need direct Cloudflare credentials.

## Production

- Pages project: `aifred-site`
- Domains: `north3rnlight3r.com`, `www.north3rnlight3r.com`, and the protected Pages hostname
- Canonical public API: `/health`, `/v1/models`, `/v1/chat/completions`
- Protected administration: `/api/v1/admin/*`, `/api/v1/command/run`
- Free downloads: R2-backed plugin packages and catalog media; PayPal routes are retired

## Build and validation

```sh
cmake -S . -B build/aifred -DCMAKE_BUILD_TYPE=Release
cmake --build build/aifred --config Release --parallel
npm ci --prefix apps
npm --prefix apps run website:check
node --test tests/aifred-archive.test.mjs
bash tools/release/aifred_monorepo_validate.sh
```

Android requires SDK 35 and JDK 17. The macOS Desktop Admin is built locally with `./apps/admin-desktop/macos/build.sh`; generated bundles are ignored. See the component READMEs and [Developer Guide](docs/DEVELOPER_GUIDE.md).

## Documentation

Start with the [documentation index](docs/README.md). Historical consolidation and preview reports under `docs/operations/` preserve decision evidence but are not current runtime authority.
