#!/usr/bin/env bash

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT_ROOT="$ROOT/out/macos-arm64"
SOURCE_ROOT="$OUT_ROOT/source"
BUILD_ROOT="$OUT_ROOT/build"
STAGE_ROOT="$OUT_ROOT/stage"
PACKAGE_ROOT="$OUT_ROOT/package"
PLUGIN_BUILD="$BUILD_ROOT/plugin-aifred/Aifred_artefacts/Release/VST3/Aifred.vst3"

require_macos() {
  [[ "$(uname -s)" == Darwin && "$(uname -m)" == arm64 ]] || {
    echo "AIFRED macOS scripts require macOS arm64." >&2
    exit 1
  }
}

require_tools() {
  local tool
  for tool in "$@"; do
    command -v "$tool" >/dev/null || { echo "Missing required tool: $tool" >&2; exit 1; }
  done
}

prepare_origin_source() {
  mkdir -p "$OUT_ROOT"
  local lock_dir="$OUT_ROOT/pipeline.lock.d"
  mkdir "$lock_dir" 2>/dev/null || { echo "Another AIFRED macOS pipeline is running." >&2; exit 1; }
  trap 'rmdir "$OUT_ROOT/pipeline.lock.d" 2>/dev/null || true' EXIT
  git -C "$ROOT" fetch --quiet origin main
  rm -rf "$SOURCE_ROOT" "$BUILD_ROOT" "$STAGE_ROOT" "$PACKAGE_ROOT"
  mkdir -p "$SOURCE_ROOT"
  git -C "$ROOT" archive origin/main | tar -x -C "$SOURCE_ROOT"
  printf '%s\n' "$(git -C "$ROOT" rev-parse origin/main)" > "$OUT_ROOT/commit.txt"
}

stage_release() {
  [[ -d "$PLUGIN_BUILD" ]] || { echo "Built VST3 bundle is missing: $PLUGIN_BUILD" >&2; exit 1; }
  mkdir -p "$STAGE_ROOT"
  cp -R "$PLUGIN_BUILD" "$STAGE_ROOT/Aifred.vst3"
  cp -R "$SOURCE_ROOT/shared-dsp" "$STAGE_ROOT/shared-dsp"
  cp -R "$SOURCE_ROOT/models/aifred" "$STAGE_ROOT/model"
  cp -R "$SOURCE_ROOT/tools/AifredIntelligenceHost/intelligence" "$STAGE_ROOT/intelligence"
  dotnet publish "$SOURCE_ROOT/tools/AifredIntelligenceHost/AifredIntelligenceHost.csproj" \
    -c Release -r osx-arm64 --self-contained true \
    -p:PublishSingleFile=false -p:IncludeNativeLibrariesForSelfExtract=true \
    -o "$STAGE_ROOT/IntelligenceHost"
  printf '{"channel":"beta","commit":"%s"}\n' "$(cat "$OUT_ROOT/commit.txt")" > "$STAGE_ROOT/IntelligenceHost/channel.json"
  cp "$SOURCE_ROOT/config/distribution/aifred-settings.example.json" "$STAGE_ROOT/aifred-settings.example.json"
  cp "$SOURCE_ROOT/README.md" "$STAGE_ROOT/README.md"
  find "$STAGE_ROOT" \( -name '._*' -o -name '.DS_Store' \) -delete
}

package_release() {
  mkdir -p "$PACKAGE_ROOT"
  COPYFILE_DISABLE=1 tar -czf "$PACKAGE_ROOT/AIFRED-Beta-macos-arm64.tar.gz" -C "$STAGE_ROOT" .
  cp "$OUT_ROOT/commit.txt" "$PACKAGE_ROOT/commit.txt"
  echo "Created $PACKAGE_ROOT/AIFRED-Beta-macos-arm64.tar.gz"
}
