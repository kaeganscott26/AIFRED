#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/common.sh"
require_macos
require_tools launchctl rsync python3 shasum curl
python3 -B "$ROOT/scripts/common/release.py" verify --platform macos-arm64

[[ -d "$OUT_ROOT/current/Aifred.vst3" && -d "$OUT_ROOT/current/IntelligenceHost" ]] || {
  echo "No verified current release found. Run scripts/macos/build.sh release first." >&2
  exit 1
}

launchctl bootout "gui/$(id -u)/$HOST_LABEL" 2>/dev/null || true
install_owned_tree "$OUT_ROOT/current/Aifred.vst3" "$PLUGIN_PARENT" Aifred.vst3
install_owned_tree "$OUT_ROOT/current/shared-dsp" "$PLUGIN_PARENT" shared-dsp
install_owned_tree "$OUT_ROOT/current/IntelligenceHost" "$DATA_PARENT" IntelligenceHost
install_owned_tree "$OUT_ROOT/current/model" "$DATA_PARENT" model
install_owned_tree "$OUT_ROOT/current/intelligence" "$DATA_PARENT" intelligence
mkdir -p "$HOME/Library/LaunchAgents"
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
"$SCRIPT_DIR/setup-ollama.sh"
"$SCRIPT_DIR/start-host.sh"
echo "AIFRED Beta installed from commit $(cat "$OUT_ROOT/commit.txt"). User settings retained."
