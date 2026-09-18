#!/usr/bin/env bash

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT_ROOT="$ROOT/out/macos-arm64"
SOURCE_ROOT="$ROOT"
BUILD_ROOT="$OUT_ROOT/build"
STAGE_ROOT="$OUT_ROOT/stage"
PACKAGE_ROOT="$OUT_ROOT/package"
CURRENT_ROOT="$OUT_ROOT/current"
PLUGIN_BUILD="$BUILD_ROOT/plugin-aifred/Aifred_artefacts/Release/VST3/Aifred.vst3"

channel="beta"
display_channel="Beta"
PLUGIN_PARENT="$HOME/Library/Audio/Plug-Ins/VST3/AIFRED $display_channel"
DATA_PARENT="$HOME/Library/Application Support/Aifred/$channel"
PLUGIN_TARGET="$PLUGIN_PARENT/Aifred.vst3"
SHARED_DSP_TARGET="$PLUGIN_PARENT/shared-dsp"
HOST_TARGET="$DATA_PARENT/IntelligenceHost"
HOST_EXECUTABLE="$HOST_TARGET/AifredIntelligenceHost"
HOST_LABEL="com.north3rnlight3r.aifred-intelligence-host"
LAUNCH_AGENT="$HOME/Library/LaunchAgents/$HOST_LABEL.plist"

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
  rm -rf "$BUILD_ROOT" "$STAGE_ROOT" "$PACKAGE_ROOT" "$OUT_ROOT/current" "$OUT_ROOT/previous"
  printf '%s\n' "$(git -C "$ROOT" rev-parse HEAD)" > "$OUT_ROOT/commit.txt"
}

assert_owned_path() {
  local target="$1" parent="$2"
  case "$target" in "$parent"/*) ;; *) echo "Install target escaped owner: $target" >&2; exit 1 ;; esac
  [[ -L "$target" ]] && { echo "Refusing symlink install target: $target" >&2; exit 1; }
}

install_owned_tree() {
  local source="$1" parent="$2" name="$3" target="$parent/$name" candidate="$parent/$name.candidate" previous="$parent/$name.previous"
  assert_owned_path "$target" "$parent"; assert_owned_path "$candidate" "$parent"; assert_owned_path "$previous" "$parent"
  [[ ! -e "$candidate" && ! -e "$previous" ]] || { echo "Retained installation recovery requires inspection: $target" >&2; exit 1; }
  mkdir -p "$parent"
  cp -R "$source" "$candidate"
  while IFS= read -r -d '' file; do
    local relative="$file"
    relative="${relative#"$source"/}"
    [[ "$(shasum -a 256 "$file" | awk '{print $1}')" == "$(shasum -a 256 "$candidate/$relative" | awk '{print $1}')" ]] || { rm -rf "$candidate"; echo "Installed hash mismatch: $relative" >&2; exit 1; }
  done < <(find "$source" -type f -print0)
  [[ ! -e "$target" ]] || mv "$target" "$previous"
  if ! mv "$candidate" "$target"; then
    [[ -e "$previous" ]] && mv "$previous" "$target"
    exit 1
  fi
  rm -rf "$previous"
}

remove_owned_tree() {
  local target="$1" parent="$2"
  assert_owned_path "$target" "$parent"
  [[ ! -L "$target" ]] && rm -rf "$target"
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
  local package_source="$STAGE_ROOT"
  [[ -d "$CURRENT_ROOT" ]] && package_source="$CURRENT_ROOT"
  mkdir -p "$PACKAGE_ROOT"
  COPYFILE_DISABLE=1 tar -czf "$PACKAGE_ROOT/AIFRED-Beta-macos-arm64.tar.gz" -C "$package_source" .
  cp "$OUT_ROOT/commit.txt" "$PACKAGE_ROOT/commit.txt"
  echo "Created $PACKAGE_ROOT/AIFRED-Beta-macos-arm64.tar.gz"
}
