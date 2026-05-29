#!/bin/sh
set -eu

AIFRED_ROOT="${AIFRED_ROOT:-/Library/Application Support/Aifred}"
ENGINE="$AIFRED_ROOT/bin/AifredEngine"
PLIST="${AIFRED_PLIST:-/Library/LaunchAgents/com.aifred.engine.plist}"
MODELFILE="$AIFRED_ROOT/models/aifred/Modelfile"
GATEWAY_URL="${GATEWAY_URL:-http://127.0.0.1:8787}"
OLLAMA_URL="${OLLAMA_URL:-http://127.0.0.1:11434}"

log() { printf '[AIFRED] %s\n' "$1"; }

if ! command -v ollama >/dev/null 2>&1; then
  echo "Ollama is not installed. Install it from https://ollama.com/download/mac or with Homebrew, then rerun this script." >&2
  exit 1
fi

log "Starting Ollama if needed."
ollama serve >/dev/null 2>&1 &
sleep 2

if ! ollama list | grep -q '^llama3\.2:3b'; then
  log "Pulling llama3.2:3b."
  ollama pull llama3.2:3b
fi

if [ ! -f "$MODELFILE" ]; then
  echo "AIFRED Modelfile is missing at $MODELFILE." >&2
  exit 1
fi

log "Creating aifred:latest."
ollama create aifred -f "$MODELFILE"
log "Verifying Ollama model response at $OLLAMA_URL."
ollama run aifred:latest "Confirm AIFRED local AI is ready." >/dev/null

if [ ! -x "$ENGINE" ]; then
  echo "AifredEngine is missing or not executable at $ENGINE." >&2
  exit 1
fi

if [ -f "$PLIST" ]; then
  if command -v plutil >/dev/null 2>&1; then
    plutil -lint "$PLIST"
  fi
  user_name="$(stat -f '%Su' /dev/console 2>/dev/null || true)"
  if [ -n "$user_name" ] && [ "$user_name" != "root" ]; then
    user_id="$(id -u "$user_name")"
    launchctl bootout "gui/$user_id" "$PLIST" >/dev/null 2>&1 || true
    launchctl bootstrap "gui/$user_id" "$PLIST" >/dev/null 2>&1 || true
    launchctl kickstart -k "gui/$user_id/com.aifred.engine" >/dev/null 2>&1 || true
  fi
else
  log "LaunchAgent plist not found at $PLIST; starting engine directly for this session."
  "$ENGINE" >/dev/null 2>&1 &
fi

sleep 2
if ! curl -fsS "$GATEWAY_URL/health" | grep -q '"local_ai_ready":true'; then
  echo "AIFRED local AI verification failed. Check Ollama at $OLLAMA_URL, aifred:latest, and AifredEngine." >&2
  exit 1
fi

log "Local AI ready."
