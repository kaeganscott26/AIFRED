# Beta repository construction

## Current ownership during production migration

The public Beta repository must remain independently buildable and must keep its current website, Pages Functions, administration surfaces, and Cloudflare configuration while they support production. The Official replacement has not assumed production ownership yet.

After the Official website and dedicated API Worker pass staging and production acceptance, a separate authorized cleanup may reduce Beta to:

- the Beta VST3 source and frontend;
- the vendored shared DSP, BufferHunter, filtering pipeline, and required host transport;
- plugin-side API/provider configuration;
- Beta build, install, package, release, test, and channel documentation.

Do not remove production website, backend, Android Admin, desktop Admin, `/ops`, or deployment files from Beta during source preparation. Git history alone does not replace a working rollback source.

## Authoritative product paths

| Responsibility | Location |
| --- | --- |
| Beta plugin adapter and frontend | [`plugin-aifred`](../plugin-aifred) |
| Shared measurement and filtering contracts | [`shared-dsp`](../shared-dsp) |
| Host transport | [`tools/AifredIntelligenceHost`](../tools/AifredIntelligenceHost) |
| Build, install, and release automation | [`scripts`](../scripts) |
| Current production fallback website/API | [`apps/website`](../apps/website) |
| Current production fallback infrastructure | [`infra/cloudflare`](../infra/cloudflare) and [`wrangler.jsonc`](../wrangler.jsonc) |

[`shared-core.lock.json`](../shared-core.lock.json) pins the shared source inventory. Beta must not depend on an Official sibling checkout or a machine-specific path.

## Generated output and secrets

Generated artifacts belong under `out/` and must stay untracked. Keep provider keys, Cloudflare credentials, GitHub tokens, owner passwords, and deployment secrets outside Git. Checked-in examples may name required variables but must not contain values.

## Phase boundary

The active plugin path ends at `FilteredMixContext -> AifredIntelligenceHost`. This migration does not add personality, memory, DAW intelligence, Babylon, or a new reasoning layer.

## Related

- [Repository map](REPOSITORY_MAP.md)
- [Architecture](ARCHITECTURE.md)
- [Development](DEVELOPMENT.md)
- [Testing](TESTING.md)
- [Shared DSP](../shared-dsp/README.md)
