#!/bin/sh
set -eu

AIFRED_ROOT="${AIFRED_ROOT:-/Library/Application Support/Aifred}"
ENGINE="$AIFRED_ROOT/bin/AifredEngine"
PLIST="${AIFRED_PLIST:-/Library/LaunchAgents/com.aifred.engine.plist}"
MODELFILE="$AIFRED_ROOT/models/aifred/Modelfile"
CONFIG_DIR="$AIFRED_ROOT/config"
USER_SETTINGS_DIR="${HOME}/Library/Application Support/Aifred"
GATEWAY_URL="${GATEWAY_URL:-http://127.0.0.1:8787}"
OLLAMA_URL="${OLLAMA_URL:-http://127.0.0.1:11434}"
MODEL_NAME="${MODEL_NAME:-aifred:latest}"

log() { printf '[AIFRED] %s\n' "$1"; }

if mkdir -p "$CONFIG_DIR" 2>/dev/null && [ -w "$CONFIG_DIR" ]; then
  cat > "$CONFIG_DIR/config.json" <<JSON
{
  "mode": "local",
  "port": 8787,
  "gateway_url": "$GATEWAY_URL",
  "provider": "ollama",
  "model_path": "models/aifred-assistant-q4.gguf",
  "model_name": "$MODEL_NAME",
  "openai_api_key": "",
  "ollama_url": "$OLLAMA_URL",
  "custom_endpoint": "$OLLAMA_URL",
  "timeout_ms": 420000
}
JSON
else
  log "System config is not writable at $CONFIG_DIR; keeping the installed engine config."
fi

mkdir -p "$USER_SETTINGS_DIR"
cat > "$USER_SETTINGS_DIR/user_settings.json" <<JSON
{
  "provider_override_enabled": false,
  "provider_mode": "ollama",
  "api_key": "",
  "ollama_url": "$OLLAMA_URL",
  "custom_endpoint": "$OLLAMA_URL",
  "model_name": "$MODEL_NAME"
}
JSON

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

log "Creating $MODEL_NAME."
ollama create "$MODEL_NAME" -f "$MODELFILE"
log "Verifying Ollama model response at $OLLAMA_URL."
ollama run "$MODEL_NAME" "Confirm AIFRED local AI is ready." >/dev/null

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
  echo "AIFRED local AI verification failed. Check Ollama at $OLLAMA_URL, $MODEL_NAME, and AifredEngine." >&2
  exit 1
fi

log "Local AI ready."
