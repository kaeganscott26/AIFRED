#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/common.sh"
require_macos
require_tools launchctl rsync

PLUGIN_PARENT="$HOME/Library/Audio/Plug-Ins/VST3/AIFRED Beta"
DATA_PARENT="$HOME/Library/Application Support/Aifred/beta"
PLUGIN_TARGET="$PLUGIN_PARENT/Aifred.vst3"
HOST_TARGET="$DATA_PARENT/IntelligenceHost"
HOST_EXECUTABLE="$HOST_TARGET/AifredIntelligenceHost"
HOST_LABEL="com.north3rnlight3r.aifred-intelligence-host"
LAUNCH_AGENT="$HOME/Library/LaunchAgents/$HOST_LABEL.plist"

[[ -d "$STAGE_ROOT/Aifred.vst3" && -d "$STAGE_ROOT/IntelligenceHost" ]] || {
  echo "No staged release found. Run scripts/macos/build.sh release first." >&2
  exit 1
}

launchctl bootout "gui/$(id -u)/$HOST_LABEL" 2>/dev/null || true
mkdir -p "$PLUGIN_PARENT" "$DATA_PARENT" "$HOME/Library/LaunchAgents"
rm -rf "$PLUGIN_TARGET" "$HOST_TARGET" "$DATA_PARENT/model" "$DATA_PARENT/intelligence" "$DATA_PARENT/shared-dsp"
cp -R "$STAGE_ROOT/Aifred.vst3" "$PLUGIN_TARGET"
cp -R "$STAGE_ROOT/IntelligenceHost" "$HOST_TARGET"
cp -R "$STAGE_ROOT/model" "$DATA_PARENT/model"
cp -R "$STAGE_ROOT/intelligence" "$DATA_PARENT/intelligence"
cp -R "$STAGE_ROOT/shared-dsp" "$DATA_PARENT/shared-dsp"
chmod 755 "$HOST_EXECUTABLE"

cat > "$LAUNCH_AGENT" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>Label</key><string>$HOST_LABEL</string>
<key>ProgramArguments</key><array><string>$HOST_EXECUTABLE</string><string>--channel</string><string>beta</string></array>
<key>RunAtLoad</key><true/><key>KeepAlive</key><true/>
</dict></plist>
PLIST
chmod 644 "$LAUNCH_AGENT"
launchctl bootstrap "gui/$(id -u)" "$LAUNCH_AGENT"
launchctl kickstart -k "gui/$(id -u)/$HOST_LABEL"
echo "AIFRED Beta installed from origin/main commit $(cat "$OUT_ROOT/commit.txt")."
