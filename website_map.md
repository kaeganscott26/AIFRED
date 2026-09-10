# Beta website and Cloudflare map

This repository contains the website and Pages Functions source that historically served production. Preserve it as rollback source until the Official site and dedicated API Worker pass production validation.

## Historical production path

```text
north3rnlight3r.com
  -> Cloudflare Pages project aifred-site
  -> Git source kaeganscott26/AIFRED
  -> apps/website
  -> Pages Functions / _worker.js backend
```

The production download failure at `/api/v1/downloads/plugin` is replaced by the Official dedicated Worker. The Beta source remains available for rollback but must not resume automatic production deployment after ownership moves.

## Target production path

```text
north3rnlight3r.com/*      -> aifred-site Pages content deployed from AIFRED_Official-
north3rnlight3r.com/api/*  -> aifred-api Worker deployed from AIFRED_Official-
```

Official owns the website, Android Admin, desktop Admin, `/ops`, Worker, production bindings, and deployment operations. Beta keeps the free plugin, its required shared measurement/filter/host code, release artifacts, tests, scripts, and public user documentation.

The latest Beta release remains public at `kaeganscott26/AIFRED`. Its verified installer and ZIP are mirrored under immutable R2 keys and delivered through the canonical website API. Release Notes links to the public repository front page.

## Shared public services

Beta and Official use `https://north3rnlight3r.com/api/v1/references` for the shared sanitized reference catalog. Remote actions occur only on explicit user request. The VST3 measurement path remains local and does not poll Cloudflare while idle.

Download and reference-upload activity is recorded by the Official Worker through its bounded queue, D1 rollups, and Analytics Engine. Admin clients read those contracts; they do not mutate R2, D1, or arbitrary files directly.

See [Backend Map](backend_map.md), [Repository Construction](docs/REPOSITORY_CONSTRUCTION.md), and [API Reference](docs/API_REFERENCE.md).
