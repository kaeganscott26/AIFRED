# AIFRED Reference Profile Sync Foundation

Status: roadmap/flagship foundation. Do not present as live until implemented and verified end to end.

## Product Boundaries

- Plugin Local Reference Mode: user loads up to five local reference tracks inside the plugin. This is the beta reference workflow.
- Website Reference Intake: public upload flow for free limited analysis. Accepted candidate metadata can help build the curated reference pool.
- Cloud Reference Profile Sync: future system where the plugin fetches profile data, not copyrighted songs.

## Source Of Truth

The website intake route gates uploaded analysis metadata. Accepted records persist in Cloudflare KV and can mirror metadata JSON into R2 when `AIFRED_REFERENCE_POOL` and `AIFRED_REFERENCE_BUCKET` are bound.

Future profile sync should publish two versioned artifacts:

- Reference Pool Manifest
- Reference Profile Pack

## Manifest Shape

```json
{
  "schema_version": 1,
  "pool_version": "2026.05.29.1",
  "updated_at": "2026-05-29T00:00:00Z",
  "accepted_count": 0,
  "profile_pack_url": "https://example.invalid/aifred/reference-profiles.json",
  "checksum": "sha256-placeholder",
  "minimum_plugin_version": "0.1.0"
}
```

## Profile Shape

Profiles should contain no private user-identifying data and no full audio:

- profile id
- accepted timestamp
- genre/tags when available
- integrated LUFS
- true peak
- short-term/momentary summary when available
- spectral tilt/tone
- stereo width
- correlation
- punch score
- crest factor
- band energy summary
- quality gate score
- confidence score

## Plugin Sync Behavior

- Check the manifest on startup or a safe background interval.
- Compare cached `pool_version` to the manifest.
- Download a profile pack only when newer.
- Verify checksum before replacing the local cache.
- Fall back to cached or bundled profiles when offline.
- Never run network work on the realtime audio thread.
- Never download full songs to customer machines.

## Safety Note

If full uploaded audio is ever stored for intake review, it should stay private/restricted. Public plugin sync should distribute extracted reference profiles only.
