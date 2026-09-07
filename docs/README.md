# AIFRED Beta Documentation

> Navigation hub for the public Beta source, architecture, validation, and release workflow.

---

# START HERE

| If you want to... | Open... |
| --- | --- |
| understand the whole runtime | [Architecture](ARCHITECTURE.md) |
| understand the repository layout | [Repository Map](REPOSITORY_MAP.md) |
| inspect measurement/observation/filter truth | [Shared DSP](../shared-dsp/README.md) |
| understand the four analysis profiles | [DSP Configuration](DSP_CONFIGURATION.md) |
| understand temporal observation | [BufferHunter](BUFFER_HUNTER.md) |
| understand deterministic model-ready context | [AIFRED Filter](AIFRED_FILTER.md) |
| build the Beta | [Build](BUILD.md) |
| understand what has actually been validated | [Testing](TESTING.md) |
| install/update/uninstall | [Installation](INSTALLATION.md) |
| understand artifacts/promotion | [Distribution](DISTRIBUTION.md) |
| keep Beta and Official separated | [Coexistence](COEXISTENCE.md) |
| contribute safely | [Development](DEVELOPMENT.md) |
| see structural repository rules | [Repository Construction](REPOSITORY_CONSTRUCTION.md) |
| see explicitly unimplemented/future work | [Future](FUTURE.md) |

---

# Architecture Path

```text
README
  -> ARCHITECTURE
  -> shared-dsp/README
  -> DSP_CONFIGURATION
  -> BUFFER_HUNTER
  -> AIFRED_FILTER
```

Use this path before making DSP, snapshot, observation, filtering, or model-context changes.

---

# Operator Path

```text
BUILD
  -> TESTING
  -> DISTRIBUTION
  -> INSTALLATION
  -> COEXISTENCE
```

A successful compiler invocation is not the same thing as DAW validation. Each document states its own evidence boundary.

---

# Contributor Path

```text
REPOSITORY_MAP
  -> DEVELOPMENT
  -> REPOSITORY_CONSTRUCTION
  -> ARCHITECTURE
  -> TESTING
```

Do not add alternate measurement implementations or resurrect removed runtime paths because an archived document happens to mention them.

---

# Public Beta Boundary

This documentation describes the public Beta. Owner-only administration/control-plane software and private flagship intelligence implementation are not part of the public Beta contract.

No documentation example should contain a real provider key, Cloudflare token, owner password, or other secret value.

---

# Main Doorways

**[Product README](../README.md)** · **[Repository Map](REPOSITORY_MAP.md)** · **[Architecture](ARCHITECTURE.md)** · **[Shared DSP](../shared-dsp/README.md)** · **[DSP Configuration](DSP_CONFIGURATION.md)** · **[BufferHunter](BUFFER_HUNTER.md)** · **[AIFRED Filter](AIFRED_FILTER.md)** · **[Build](BUILD.md)** · **[Testing](TESTING.md)** · **[Installation](INSTALLATION.md)** · **[Distribution](DISTRIBUTION.md)** · **[Coexistence](COEXISTENCE.md)** · **[Development](DEVELOPMENT.md)** · **[Future](FUTURE.md)**
