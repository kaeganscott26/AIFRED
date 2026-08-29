# `/ops` Operations Console

Open `https://www.north3rnlight3r.com/ops` and sign in with the configured AIFRED administrator account. The page stores the signed session token in browser session storage and sends it only as a bearer token to same-origin APIs.

Panels: Overview, Analytics, Downloads, Track Analysis, API, Logs, Inquiries, Exports, FORGE and Archive. Refresh is manual to avoid wasteful polling. Tables are bounded; log filtering is client-side over the returned window. Times arrive as UTC ISO-8601 and display in the browser timezone.

Exports download the backend-produced JSON using `/api/v1/admin/export/site` or `/api/v1/admin/export/tracks`. The Archive panel is informational because a Cloudflare page cannot read local desktop archives.

`/ops` does not implement a text terminal or the `/api/v1/command/run` parser. Use Android Admin for registered admin commands.
