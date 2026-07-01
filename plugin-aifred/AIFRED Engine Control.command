#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
REPO_STAGED_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)/dist/macos/pkgroot/Library/Application Support/Aifred"
AIFRED_ROOT="${AIFRED_ROOT:-/Library/Application Support/Aifred}"
if [ ! -x "$AIFRED_ROOT/bin/AifredEngine" ] && [ -x "$REPO_STAGED_ROOT/bin/AifredEngine" ]; then
  AIFRED_ROOT="$REPO_STAGED_ROOT"
fi
ENGINE="$AIFRED_ROOT/bin/AifredEngine"
SETUP="$AIFRED_ROOT/setup-aifred-local-ai.sh"
PLIST="${AIFRED_PLIST:-/Library/LaunchAgents/com.aifred.engine.plist}"
LABEL="com.aifred.engine"
GATEWAY_URL="${GATEWAY_URL:-http://127.0.0.1:8787}"

log() { printf '[AIFRED] %s\n' "$1"; }

console_user_id() {
  id -u "$(id -un)"
}

bootout_agent() {
  user_id="$(console_user_id)"
  launchctl bootout "gui/$user_id" "$PLIST" >/dev/null 2>&1 || true
}

bootstrap_agent() {
  if [ ! -f "$PLIST" ]; then
    log "LaunchAgent plist is missing at $PLIST."
    return 1
  fi
  user_id="$(console_user_id)"
  launchctl bootstrap "gui/$user_id" "$PLIST" >/dev/null 2>&1 || true
  launchctl kickstart -k "gui/$user_id/$LABEL" >/dev/null 2>&1 || true
}

start_direct() {
  if command -v ollama >/dev/null 2>&1; then
    ollama serve >/dev/null 2>&1 &
  fi
  if [ -x "$ENGINE" ]; then
    "$ENGINE" >/dev/null 2>&1 &
  else
    log "AifredEngine is missing or not executable at $ENGINE."
    return 1
  fi
}

start_or_repair() {
  if [ -x "$SETUP" ]; then
    "$SETUP" || {
      log "Full repair did not complete; trying to start the installed engine."
      bootstrap_agent || start_direct
    }
  else
    bootstrap_agent || start_direct
  fi
}

stop_engine() {
  bootout_agent
  pkill -f "$ENGINE" >/dev/null 2>&1 || true
  log "AIFRED engine stopped for this login session."
}

status() {
  if curl -fsS "$GATEWAY_URL/health" 2>/dev/null; then
    printf '\n'
  else
    log "Gateway is not reachable at $GATEWAY_URL."
  fi
}

printf '\nAIFRED Engine Control\n'
printf '1. Start or repair local AI\n'
printf '2. Restart engine\n'
printf '3. Stop engine for this login\n'
printf '4. Show status\n'
printf '5. Quit\n'
printf 'Choose: '
read choice

case "$choice" in
  1) start_or_repair ;;
  2) stop_engine; start_or_repair ;;
  3) stop_engine ;;
  4) status ;;
  *) log "No changes made." ;;
esac

printf '\nPress return to close this window.'
read _unused
