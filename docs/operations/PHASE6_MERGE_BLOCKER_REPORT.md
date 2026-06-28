# Phase 6 Merge Blocker Report

This report documents current blockers before any merge to `main`.

## Blocker 1: Asset Strategy

Duplicated media exists in `website/` and `apps/website`.

A merge to `main` would bring large media into the canonical branch. The asset strategy must be explicitly accepted before merge, including whether duplicated media stays in Git for now or moves later to R2 or release storage.

## Blocker 2: Cloudflare Binding

The production Cloudflare Pages binding must be manually verified.

`apps/website` is not the production root. A non-production preview must succeed before any production path switch.

## Blocker 3: Existing Workflow Behavior

`.github/workflows/build.yml` still deploys from the old `website/` path.

Tag release publishing still exists. The release workflow must not be altered casually because it owns package and release behavior beyond the website migration.

## Blocker 4: Old Folder Removal Timing

`website/` cannot be removed until the `apps/website` preview and later production migration are proven.

`android_admin/` cannot be removed until the `apps/admin-android` build and release path is proven.

## Blocker 5: Plugin And Engine Path Migration

`plugin-aifred/` remains the current runtime plugin source.

`tools/AifredEngine/` remains the current local engine source.

Package and release scripts may still assume old paths. Moving these paths needs a separate explicit phase.

## Merge Recommendation

- Do not merge to `main` yet.
- Keep branches stacked.
- The next safe milestone is human approval for a non-production preview.
