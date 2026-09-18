#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/common.sh"
require_macos
[[ -x "$HOST_EXECUTABLE" ]] || { echo "Install the canonical current artifact first." >&2; exit 1; }
mkdir -p "$HOME/Library/LaunchAgents" "$DATA_PARENT/logs"
cat > "$LAUNCH_AGENT" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>Label</key><string>$HOST_LABEL</string>
<key>ProgramArguments</key><array><string>$HOST_EXECUTABLE</string><string>--channel</string><string>$channel</string></array>
<key>RunAtLoad</key><true/><key>KeepAlive</key><true/>
<key>StandardOutPath</key><string>$DATA_PARENT/logs/host.log</string>
<key>StandardErrorPath</key><string>$DATA_PARENT/logs/host-error.log</string>
</dict></plist>
PLIST
chmod 644 "$LAUNCH_AGENT"
launchctl bootout "gui/$(id -u)/$HOST_LABEL" 2>/dev/null || true
launchctl bootstrap "gui/$(id -u)" "$LAUNCH_AGENT"
launchctl kickstart -k "gui/$(id -u)/$HOST_LABEL"