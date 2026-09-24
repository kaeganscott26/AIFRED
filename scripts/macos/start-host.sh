#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/common.sh"
require_macos
require_tools launchctl curl python3
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

health_url="http://127.0.0.1:$HOST_PORT/health"
health=""
for attempt in $(seq 1 30); do
  health="$(curl -fsS --max-time 3 "$health_url" 2>/dev/null || true)"
  [[ -n "$health" ]] && break
  sleep 1
done
[[ -n "$health" ]] || { echo "Intelligence Host unavailable at $health_url." >&2; exit 1; }
HEALTH_JSON="$health" python3 - <<'PY'
import json
import os
import sys

health = json.loads(os.environ['HEALTH_JSON'])
if health.get('host_identity') != 'AifredIntelligenceHost' or health.get('product_channel') != 'beta':
    raise SystemExit('Intelligence Host responded with the wrong identity or channel.')
if not health.get('ai_available'):
    error = health.get('last_error') or 'configured provider/model is unavailable'
    print(f"Intelligence Host available, but provider/model unavailable: {error}", file=sys.stderr)
    raise SystemExit(2)
print(f"Intelligence Host available; provider/model ready: {health.get('provider')} / {health.get('model_name')}")
PY
