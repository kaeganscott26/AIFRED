# AIFRED User Guide

## What AIFRED does

AIFRED analyzes a DAW signal, presents tone, width, punch, loudness, dynamics and reference-alignment information, and can send the current interpreted analysis context to a local model through AifredEngine. Version 0.3.6 is a VST3; no AU/AAX build is registered.

## Install and start

Windows distribution includes the VST3, AifredEngine, installer/uninstaller and local Ollama setup. The published macOS artifact is a manual-install ZIP; the repository can build a local pkg. Start Ollama with the `aifred:latest` model, then confirm:

```sh
curl http://127.0.0.1:8787/health
```

The plugin detects the engine off the audio thread. Audio analysis remains usable when model chat is unavailable.

## Plugin workflow

- **Analyze** shows the current mix signature and history.
- **Reference** compares the mix with selected reference material.
- **Compare** separates Mix A and Mix B analysis.
- **Chat** sends a request only when asked and includes the current canonical interpreted snapshot.

The plugin preserves local-first operation. It does not send audio to Cloudflare and does not require the website to analyze a track.

## Website and downloads

Use `https://www.north3rnlight3r.com` for the public site, browser analyzer, beat catalog and free plugin downloads. Catalog streams and downloads are served through controlled API routes backed by R2, with byte-range support. Commercial licensing is inquiry-based; the old PayPal flow is disabled.

## Administration

Android Admin is the private mobile control surface. `/ops` is the authenticated browser dashboard. Windows and macOS Desktop Admin mirror live operations and add local archive management. See [Admin Guide](ADMIN_GUIDE.md) and the exact [Command Reference](ADMIN_COMMAND_REFERENCE.md).

## Exports and history

**Export Site Data** and **Export Track Analysis** produce authenticated, sanitized JSON snapshots. FORGE retains bounded current context. Desktop/local storage retains verified historical bundles; see [Archive Guide](ARCHIVE_GUIDE.md).

## Common problems

If engine health fails, confirm the engine process and port 8787. If the engine reports Ollama unavailable or the model missing, verify port 11434 and `ollama list`. For admin, export, archive, connectivity and timestamp issues, use [Troubleshooting](TROUBLESHOOTING.md).
