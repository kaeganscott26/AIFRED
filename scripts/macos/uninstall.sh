#!/usr/bin/env bash
set -euo pipefail

[[ "$(uname -s)" == Darwin ]] || { echo "This script requires macOS." >&2; exit 1; }

HOST_LABEL="com.north3rnlight3r.aifred-intelligence-host"
PLUGIN_TARGET="$HOME/Library/Audio/Plug-Ins/VST3/AIFRED Beta/Aifred.vst3"
DATA_TARGET="$HOME/Library/Application Support/Aifred/beta"
LAUNCH_AGENT="$HOME/Library/LaunchAgents/$HOST_LABEL.plist"

launchctl bootout "gui/$(id -u)/$HOST_LABEL" 2>/dev/null || true
rm -f "$LAUNCH_AGENT"
rm -rf "$PLUGIN_TARGET" "$DATA_TARGET/IntelligenceHost" "$DATA_TARGET/model" "$DATA_TARGET/intelligence" "$DATA_TARGET/shared-dsp"
rmdir "$HOME/Library/Audio/Plug-Ins/VST3/AIFRED Beta" 2>/dev/null || true
rmdir "$DATA_TARGET" 2>/dev/null || true
echo "AIFRED Beta binaries and intelligence assets removed. User settings were retained."
