# AIFRED Administration Guide

All live admin surfaces use `https://www.north3rnlight3r.com` and the protected `/api/v1/admin/*` API. Credentials and provider secrets are not embedded in `/ops` or desktop clients.

| Capability | Android | `/ops` | Windows | macOS |
| --- | --- | --- | --- | --- |
| Authenticated live status | Yes | Yes | Yes | Yes, through `/ops` WebKit |
| Analytics/downloads/logs/inquiries | Yes | Yes | Yes | Yes, through `/ops` |
| Catalog/reference/track analysis | Yes | Yes | Yes | Yes, through `/ops` |
| Site and track exports | Yes | Yes | Yes | Yes, through `/ops` |
| User-entered admin commands | Yes | No | No | No |
| Local diagnostic registry | Android only | No | No | No |
| Local archive status/search/restore | No | Metadata boundary only | Yes | Yes |
| Manual confirmed archive prune | No | No | Yes | Yes |

## Android Admin

The Compose app has Chat, Upload and Command tabs. It supports provider configuration, chat settings, catalog playback/uploads, reference and website asset uploads, file controls, operational summaries, exports and registered commands. UTC values are converted only for device presentation. Online login uses the backend signed session; saved offline owner credentials grant local-device features only.

## `/ops`

`/ops` has Overview, Analytics, Downloads, Track Analysis, API, Logs, Inquiries, Exports, FORGE and Archive panels. The shell is public static HTML but all operational data and exports require backend authentication. Responses are `no-store`; the page is `noindex`. It has no terminal parser.

## Desktop Admin

Windows preserves WinForms. macOS is an AppKit/WebKit shell. Live data comes from the same API or embedded `/ops`; archive controls call `tools/aifred-archive.mjs`. Offline mode covers local archives, never production administration. No direct Cloudflare token is required.

For every Android terminal command, use [Administrator Command Reference](ADMIN_COMMAND_REFERENCE.md). Archive CLI syntax is in [Archive Guide](ARCHIVE_GUIDE.md).
