# Distribution

[Build](BUILD.md) owns compiler output; [installation](INSTALLATION.md) owns installed files. out/windows-x64/stage is a candidate; current is the single verified release. release.py checks ownership, exact SHA-256 inventory, VST3 source equality and host channel before promotion. Failed validation retains current. Promotion verifies again before recycling superseded current; retained recovery blocks another promotion.

Manifest v2 includes product/channel/version, Git/source-tree identity, shared core version, DSP profile schema/revisions, context schema, port and exact paths. Prior manifest-v1 releases are inventory-verified only for safe promotion; no old runtime is packaged. Never commit generated output, recovery archives or credentials.

Beta additionally builds ZIP and Windows installer/uninstaller with channel ownership and retained settings. Signing, website publication and installed/DAW testing are separate gates. Website downloads are not published by this task. macOS/Linux distribution: SCAFFOLDED / NOT VALIDATED.
