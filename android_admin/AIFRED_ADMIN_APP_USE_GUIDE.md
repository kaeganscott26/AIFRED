# AIFRED Admin App Use Guide

## Login

The admin app supports offline login. Use the configured North3rnLight3r admin username and password. When the phone has no network, the app creates a local admin session so the command tab can run commands inside the Android app sandbox.

Online login uses `https://www.north3rnlight3r.com/api/v1/admin/login` and returns a 24-hour session token. Offline login returns a `local-admin-*` session token.

## Chat

The Chat tab loads the beat catalog from the live website and connects to:

`wss://www.north3rnlight3r.com/ws/chat`

If WebSocket is unavailable, the app falls back to:

`https://www.north3rnlight3r.com/api/v1/chat/ask`

Cloudflare Pages must have `OPENAI_API_KEY` or `OLLAMA_BASE_URL` configured for model answers. Without either setting, the route still responds but tells you the model provider is not configured.

## Beat Catalog

The app reads:

`https://www.north3rnlight3r.com/api/v1/catalog/list`

Use `Refresh Catalog` in the Chat tab to reload the website catalog. Select a beat to stream it from the site and view the live playback visualizer.

## Upload

The Upload tab can select local Android files and send them to configured admin API routes.

Catalog audio uses:

`POST /api/v1/admin/catalog/upload`

Reference audio uses:

`POST /api/v1/admin/reference/upload`

Website assets use:

`POST /api/v1/admin/files/upload`

Those upload storage routes need a backing storage implementation before they can permanently write large binary files from Cloudflare Pages.

## Command Tab

The command line has two modes.

Online admin session:

Commands are sent to `POST /api/v1/command/run`. The website backend only accepts safe allowlisted commands.

Offline local admin session:

Commands run through Android `sh -c` inside the app runtime environment. This is sandboxed by Android and does not provide root.

## Online Commands

`health`

Checks the live website API.

`catalog:list`

Returns the number of beat catalog tracks.

`models:list`

Shows whether OpenAI and Ollama routes are configured.

`reference:stats`

Shows whether accepted analyzer metadata can persist to the reference pool.

`deploy:status`

Shows the production website domains.

`deploy:site`

Dispatches GitHub Actions workflow `build.yml` when `GITHUB_TOKEN` is configured in Cloudflare Pages.

## Local Android Shell Commands

These work after offline admin login:

`pwd`

Shows the app process working directory.

`ls`

Lists the current app runtime directory.

`ls /sdcard`

Lists shared storage if Android permissions allow it.

`id`

Shows the Android Linux user assigned to the app.

`getprop ro.build.version.release`

Shows the Android OS version.

`df -h`

Shows filesystem capacity.

`pm list packages`

Lists installed Android packages visible to the app.

`input keyevent 3`

Sends the Android home key event if allowed by the runtime.

## Website File Control

Read deployed static files works without GitHub token for files that exist on the public site.

Write/update website files requires Cloudflare Pages environment variable:

`GITHUB_TOKEN`

The token needs repo contents write permission for `kaeganscott26/AIFRED`. When configured, `Save File` commits the edit to GitHub. GitHub Actions or the Cloudflare Pages Git integration then deploys the change.

## Website Analyzer

The public site now has a free browser analyzer. It decodes the uploaded mix locally, draws the visualizer, calculates tone balance, loudness, peak, crest factor, stereo width, low-end control, and harshness control, then sends metadata only to:

`POST /api/v1/analysis/submit`

Passing metadata can be stored when `AIFRED_REFERENCE_POOL` KV is bound. Failing metadata is disposed by the backend response.
