# Troubleshooting

Verify the exact current artifact with `release.py`, then [install](INSTALLATION.md) and rescan the DAW. Check [coexistence](COEXISTENCE.md) before removing an old global-slot installation. Do not delete both channels' parent directory.

Beta host uses port 8787; Official uses 8788. Confirm `/health` identifies `AifredIntelligenceHost` and the expected channel. A different process on the port is not a valid host. Run [start-host.ps1](../scripts/windows/start-host.ps1) after installing. Beta logs live under `%LOCALAPPDATA%/Aifred/beta/logs`. Provider failure does not stop DSP.

Meter arcs use continuous float values. Correlation and width are live; sustained observations take 15 to 25 seconds. Silence retains prior observation with inactive or stale flags. A flat spectrum can mean that the signal falls below the selected `-120`, `-96`, `-72`, or `-48 dBFS` viewport. Analytical values are not clamped.

Trace measurement problems through DAW buffer, engine snapshot, observation snapshot, filter/view projection, then rendering. Do not modify a DSP formula to repair presentation.

## Related

- [Testing](TESTING.md)
- [DSP Configuration](DSP_CONFIGURATION.md)
- [Architecture](ARCHITECTURE.md)
- [Installation](INSTALLATION.md)
