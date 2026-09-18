#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT="$ROOT/out/macos-arm64"
BUILD="$OUT/build"
PACKAGE="$OUT/package"
PAYLOAD="$PACKAGE/payload"
SCRIPTS="$PACKAGE/scripts"
DMG_ROOT="$PACKAGE/dmg"
VERSION="${AIFRED_VERSION:-0.3.6}"
IDENTIFIER="com.north3rnlight3r.aifred.beta"
CODESIGN_IDENTITY="${AIFRED_CODESIGN_IDENTITY:-}"
PKG_SIGNING_IDENTITY="${AIFRED_PKG_SIGNING_IDENTITY:-}"
DMG="$OUT/AIFRED-Beta-${VERSION}-macos-arm64.dmg"
PKG="$DMG_ROOT/AIFRED Beta Installer.pkg"
PLUGIN="$BUILD/plugin-aifred/Aifred_artefacts/Release/VST3/Aifred.vst3"
HOST_PROJECT="$ROOT/tools/AifredIntelligenceHost/AifredIntelligenceHost.csproj"
HOST_PUBLISH="$PACKAGE/host"

if [[ "$(uname -s)" != "Darwin" || "$(uname -m)" != "arm64" ]]; then
  echo "This package must be built on macOS arm64." >&2
  exit 1
fi

for tool in cmake dotnet pkgbuild hdiutil; do
  command -v "$tool" >/dev/null || { echo "Missing required tool: $tool" >&2; exit 1; }
done

if [[ ! -f "$ROOT/CMakeLists.txt" || ! -f "$ROOT/CMakePresets.json" ]]; then
  echo "Run this script from an AIFRED repository checkout." >&2
  exit 1
fi

echo "Packaging AIFRED Beta $(git -C "$ROOT" rev-parse --short HEAD)"
cmake --preset macos-release
cmake --build --preset macos-release --target Aifred_VST3

rm -rf "$PACKAGE"
mkdir -p "$PAYLOAD/Library/Audio/Plug-Ins/VST3" \
  "$PAYLOAD/Library/Application Support/Aifred/beta/IntelligenceHost/bin" \
  "$PAYLOAD/Library/LaunchAgents" "$SCRIPTS" "$DMG_ROOT"

if [[ ! -d "$PLUGIN" ]]; then
  echo "VST3 build output was not found: $PLUGIN" >&2
  exit 1
fi
cp -R "$PLUGIN" "$PAYLOAD/Library/Audio/Plug-Ins/VST3/Aifred.vst3"
find "$PAYLOAD" \( -name '._*' -o -name '.DS_Store' \) -delete

dotnet publish "$HOST_PROJECT" -c Release -r osx-arm64 --self-contained true \
  -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true \
  -o "$HOST_PUBLISH"
cp "$HOST_PUBLISH/AifredIntelligenceHost" \
  "$PAYLOAD/Library/Application Support/Aifred/beta/IntelligenceHost/bin/"
printf '{"channel":"beta"}\n' > \
  "$PAYLOAD/Library/Application Support/Aifred/beta/IntelligenceHost/bin/channel.json"
xattr -rc "$PAYLOAD" 2>/dev/null || true

cat > "$PAYLOAD/Library/LaunchAgents/com.north3rnlight3r.aifred-intelligence-host.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>Label</key><string>com.north3rnlight3r.aifred-intelligence-host</string>
  <key>ProgramArguments</key>
  <array><string>/Library/Application Support/Aifred/beta/IntelligenceHost/bin/AifredIntelligenceHost</string><string>--channel</string><string>beta</string></array>
  <key>RunAtLoad</key><true/>
  <key>KeepAlive</key><true/>
  <key>ProcessType</key><string>Interactive</string>
  <key>StandardOutPath</key><string>/tmp/aifred-intelligence-host.log</string>
  <key>StandardErrorPath</key><string>/tmp/aifred-intelligence-host.error.log</string>
</dict>
</plist>
PLIST

cp "$ROOT/scripts/macos/postinstall" "$SCRIPTS/postinstall"
chmod 755 "$SCRIPTS/postinstall"
xattr -c "$SCRIPTS/postinstall" 2>/dev/null || true

if [[ -n "$CODESIGN_IDENTITY" ]]; then
  codesign --force --deep --options runtime --timestamp \
    --sign "$CODESIGN_IDENTITY" \
    "$PAYLOAD/Library/Audio/Plug-Ins/VST3/Aifred.vst3"
  codesign --force --options runtime --timestamp \
    --sign "$CODESIGN_IDENTITY" \
    "$PAYLOAD/Library/Application Support/Aifred/beta/IntelligenceHost/bin/AifredIntelligenceHost"
fi

pkgbuild_args=(--root "$PAYLOAD" --scripts "$SCRIPTS"
  --identifier "$IDENTIFIER" --version "$VERSION" --install-location / "$PKG")
if [[ -n "$PKG_SIGNING_IDENTITY" ]]; then
  pkgbuild_args+=(--sign "$PKG_SIGNING_IDENTITY")
fi
COPYFILE_DISABLE=1 pkgbuild "${pkgbuild_args[@]}"
EXPANDED_PKG="$PACKAGE/expanded-pkg"
pkgutil --expand "$PKG" "$EXPANDED_PKG"
find "$EXPANDED_PKG" \( -name '._*' -o -name '.DS_Store' \) -delete
rm -f "$PKG"
pkgutil --flatten "$EXPANDED_PKG" "$PKG"

cat > "$DMG_ROOT/Install AIFRED Beta.command" <<'INSTALLER'
#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
open "$SCRIPT_DIR/AIFRED Beta Installer.pkg"
INSTALLER
chmod 755 "$DMG_ROOT/Install AIFRED Beta.command"

printf 'AIFRED Beta %s for macOS arm64\n\nOpen the installer package.\n' "$VERSION" > "$DMG_ROOT/README.txt"

rm -f "$DMG"
hdiutil create -volname "AIFRED Beta ${VERSION}" -srcfolder "$DMG_ROOT" \
  -format UDZO -ov "$DMG"

echo "Created: $DMG"
echo "Commit:  $(git -C "$ROOT" rev-parse HEAD)"