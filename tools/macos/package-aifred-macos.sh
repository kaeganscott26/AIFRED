#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)"
BUILD_ROOT="${BUILD_ROOT:-$REPO_ROOT/build-mac}"
DIST_DIR="${DIST_DIR:-$REPO_ROOT/dist/macos}"
RUNTIME="${RUNTIME:-osx-arm64}"
PKGROOT="$DIST_DIR/pkgroot"
SCRIPTS_DIR="$DIST_DIR/scripts"
PRODUCT_ROOT="$PKGROOT/Library/Application Support/Aifred"
PLUGIN_ROOT="$PKGROOT/Library/Audio/Plug-Ins/VST3"

PLUGIN_PATH="$(find "$BUILD_ROOT" -type d -name Aifred.vst3 -path '*/Release/VST3/*' | head -n 1)"
if [ -z "$PLUGIN_PATH" ]; then
  echo "Aifred.vst3 was not found under $BUILD_ROOT" >&2
  exit 1
fi

rm -rf "$DIST_DIR"
mkdir -p "$PRODUCT_ROOT/bin" "$PRODUCT_ROOT/config" "$PRODUCT_ROOT/logs" "$PRODUCT_ROOT/models/aifred" "$PLUGIN_ROOT" "$PKGROOT/Library/LaunchAgents" "$SCRIPTS_DIR"

cp -R "$PLUGIN_PATH" "$PLUGIN_ROOT/Aifred.vst3"

dotnet publish "$REPO_ROOT/tools/AifredEngine/AifredEngine.Mac.csproj" -c Release -r "$RUNTIME" -o "$DIST_DIR/engine/$RUNTIME"
cp "$DIST_DIR/engine/$RUNTIME/AifredEngine" "$PRODUCT_ROOT/bin/AifredEngine"
chmod 755 "$PRODUCT_ROOT/bin/AifredEngine"

cat > "$PRODUCT_ROOT/config/config.json" <<'JSON'
{
  "mode": "local",
  "port": 8787,
  "gateway_url": "http://127.0.0.1:8787",
  "provider": "ollama",
  "model_path": "models/aifred-assistant-q4.gguf",
  "model_name": "aifred:latest",
  "openai_api_key": "",
  "ollama_url": "http://127.0.0.1:11434",
  "custom_endpoint": "http://127.0.0.1:11434",
  "timeout_ms": 420000
}
JSON

cat > "$PRODUCT_ROOT/models/README.txt" <<'TXT'
Optional offline model slot: place a licensed GGUF model here as aifred-assistant-q4.gguf.
AIFRED works out of the box with local Ollama chat when Ollama is installed and the aifred:latest model alias has been created.
TXT

cp "$SCRIPT_DIR/com.aifred.engine.plist" "$PKGROOT/Library/LaunchAgents/com.aifred.engine.plist"
cp "$REPO_ROOT/models/aifred/Modelfile" "$PRODUCT_ROOT/models/aifred/Modelfile"
cp "$REPO_ROOT/website/assets/docs/aifred-beta-release-notes.txt" "$PRODUCT_ROOT/AIFRED-BETA-NOTES.txt"
cp "$SCRIPT_DIR/postinstall" "$SCRIPTS_DIR/postinstall"
chmod 755 "$SCRIPTS_DIR/postinstall"

pkgbuild \
  --root "$PKGROOT" \
  --scripts "$SCRIPTS_DIR" \
  --identifier "com.aifred.vst3" \
  --version "0.3.6" \
  --install-location "/" \
  "$DIST_DIR/AIFRED-VST3-macOS.pkg"

echo "Created $DIST_DIR/AIFRED-VST3-macOS.pkg"
