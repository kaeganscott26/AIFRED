# Phase 5 Admin Gradle Discovery Plan

> Historical migration record captured before the 2026-08-25 production promotion. Preserve as evidence; current authority is `website-cloudflare-production-2026-08-25.md`.

Phase 4 dry-run checks show `apps/admin-android` appears build-shape-compatible.

Phase 5 does not run Gradle by default.

Future safe command, only when the Java and Android environment is ready:

```sh
cd apps/admin-android
./gradlew tasks
```

This is task discovery only.

Do not run `assembleRelease` yet.

Do not sign APKs.

Do not upload APKs.

Do not change backend URLs.

Do not edit secrets.
