# AIFRED Wiki

This wiki documents the full AIFRED operating system: VST3 plugin, website, backend routes, private Android admin app, build pipeline, distribution model, and troubleshooting.

## Production Entry Points

- Website: https://www.north3rnlight3r.com
- Apex domain: https://north3rnlight3r.com
- GitHub release page: https://github.com/kaeganscott26/AIFRED/releases/latest

## Wiki Index

| Page | Purpose |
| --- | --- |
| [User Guide](User-Guide.md) | How artists and buyers use the website, beat catalog, analyzer, and VST |
| [Admin App Guide](Admin-App-Guide.md) | Owner-only Android app operation and command reference |
| [Developer Guide](Developer-Guide.md) | Local setup, builds, packages, CI, and deployment |
| [Backend Map](Backend-Map.md) | Cloudflare Pages Worker routes and data flow |
| [Function Map](Function-Map.md) | Software module and function-level responsibility map |
| [Troubleshooting](Troubleshooting.md) | Common failures and recovery steps |
| [Security And Distribution](Security-And-Distribution.md) | Privacy, release, admin app, and repository visibility rules |

## Product Principles

AIFRED is built around a small set of explicit decisions:

- Keep the public product polished and direct.
- Keep admin tooling private.
- Keep the website deployed on the North3rnLight3r domain, not a preview domain.
- Keep release packages easy to install.
- Keep analysis modes visually and technically distinct.
- Keep backend controls auditable and route-based.

