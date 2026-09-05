# Build

CURRENT: CMakePresets.json owns the native Release configuration. Directory.Build.props routes .NET compiler/intermediate output into the same platform build root. Run commands from this repository, never a sibling checkout.

| Platform | Preset | Build | Candidate | Authoritative release |
|---|---|---|---|---|
| Windows x64 | windows-release | out/windows-x64/build | out/windows-x64/stage | out/windows-x64/current |
| macOS ARM64 | macos-release | out/macos-arm64/build | out/macos-arm64/stage | out/macos-arm64/current |
| Linux x64 | linux-release | out/linux-x64/build | out/linux-x64/stage | out/linux-x64/current |

Windows prerequisites: PowerShell 7, Visual Studio C++ Build Tools and Windows SDK, CMake 3.24+, Ninja, .NET SDK 10 and Python 3.12+. Beta validation also requires Node/npm. The wrapper discovers MSVC with vswhere and selects x64. Dependencies remain pinned by the existing CMake definitions; do not substitute another repository's JUCE tree.

```powershell
pwsh -NoProfile -File scripts/windows/build.ps1 -Action configure
pwsh -NoProfile -File scripts/windows/build.ps1 -Action build
pwsh -NoProfile -File scripts/windows/build.ps1 -Action test
pwsh -NoProfile -File scripts/windows/build.ps1 -Action release
```

Actions run prerequisites: build includes configure; test includes build; stage/package assemble and validate a candidate after tests; release also promotes it. Configure creates a fresh cache at the canonical location on the first run. Subsequent builds reuse it. The former ninja-release preset has been replaced by windows-release/macos-release/linux-release; do not patch moved caches or create a second permanent configuration directory.

Native direct commands require the platform compiler environment:

```sh
cmake --preset windows-release
cmake --build --preset windows-release --target Aifred_VST3
```

On macOS use macos-release and Xcode command-line tools; Ninja and CMake must be available. Beta also needs .NET 10 and pkgbuild for its existing packaging route. On Linux use linux-release, GCC/Clang, Ninja, CMake and JUCE's Linux development libraries. Debian/Ubuntu use apt packages such as build-essential, libasound2-dev, libx11-dev, libxext-dev, libxinerama-dev, libxrandr-dev, libxcursor-dev and libfreetype-dev. Arch uses pacman packages such as base-devel, alsa-lib, libx11, libxext, libxinerama, libxrandr, libxcursor and freetype2. Official WebView support needs platform-specific design/validation beyond this list. Check distro package availability before installing; wrappers install no dependencies.

macOS and Linux: SCAFFOLDED / NOT VALIDATED in this Windows pass. Linux has no supported complete companion/installer contract. Official's companion targets net10.0-windows; a Linux/macOS preset is not proof that the whole product works there.

All out/ content is generated and ignored. Sources, configuration, user data, models and reference audio stay in their owned locations. Build identity uses Git SHA plus dirty-state metadata; a build made from uncommitted changes is not a pristine release of that SHA. See [distribution](DISTRIBUTION.md) and [testing](TESTING.md).
