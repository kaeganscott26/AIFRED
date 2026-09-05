# Troubleshooting

Verify the exact current artifact with release.py, then [install](INSTALLATION.md) and rescan the DAW. Check [coexistence](COEXISTENCE.md) before removing any old global-slot installation. Never delete both channels' shared parent directory.

Beta host uses 8787; Official 8788. Confirm health identifies AifredIntelligenceHost and the expected channel. A different process on that port is not a valid host. Run scripts/windows/start-host.ps1 after installing; channel logs are under LocalAppData/Aifred/beta/logs. Provider unavailable does not stop DSP. Configure the provider/model and retain credentials privately.

Meter arcs use fractional values. Correlation/width are live; sustained observations take 15–25 seconds. Silence retains prior observation with inactive/stale flags. A flat -24..0 spectrum view can mean signal below its display floor; analytical values are not clamped. See [testing](TESTING.md) for diagnostic fixtures and manual host comparisons.
