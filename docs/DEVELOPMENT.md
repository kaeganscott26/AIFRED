# Development

Read [architecture](ARCHITECTURE.md), [shared algorithms](../shared-dsp/README.md), [build](BUILD.md) and [testing](TESTING.md). Preserve frontends and audio/state IDs. Measurements belong only to aifred_engine, observation to BufferHunter, semantics to aifred_filter. Network/model/file operations never reach processBlock.

Both repos vendor identical core/host source and checksum lock. Review/version both copies together; optional --peer checks explicit clones. Git/external recovery bundles are archives; retain no dead alternative analyzer or serializer.

Before work inspect clean main, local/tracking/live remote HEAD and ahead/behind. Update using git switch main and git pull --ff-only origin main. Before push review complete diff, run canonical tests/release, fetch and reconcile unexpected advancement. No force push, output or secrets. Distinguish Windows automation from installed/DAW validation and unvalidated platforms.
