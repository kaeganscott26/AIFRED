#!/bin/sh
set -eu

AIFRED_ROOT="${AIFRED_ROOT:-$HOME/.local/share/aifred}"
ENGINE="$AIFRED_ROOT/bin/AifredEngine"
MODELFILE="$AIFRED_ROOT/models/aifred/Modelfile"
SERVICE_DIR="$HOME/.config/systemd/user"
SERVICE_FILE="$SERVICE_DIR/aifred-engine.service"
GATEWAY_URL="${GATEWAY_URL:-http://127.0.0.1:8787}"

log() { printf '[AIFRED] %s\n' "$1"; }

if ! command -v ollama >/dev/null 2>&1; then
  echo "Ollama is not installed. Official install path: curl -fsSL https://ollama.com/install.sh | sh" >&2
  exit 1
fi

log "Starting Ollama if needed."
if command -v systemctl >/dev/null 2>&1; then
  systemctl --user start ollama >/dev/null 2>&1 || systemctl start ollama >/dev/null 2>&1 || ollama serve >/dev/null 2>&1 &
else
  ollama serve >/dev/null 2>&1 &
fi
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
log "Verifying Ollama model response."
ollama run aifred:latest "Confirm AIFRED local AI is ready." >/dev/null

if [ ! -x "$ENGINE" ]; then
  echo "AifredEngine is missing or not executable at $ENGINE." >&2
  exit 1
fi

mkdir -p "$SERVICE_DIR"
cat > "$SERVICE_FILE" <<EOF
[Unit]
Description=AIFRED local AI engine
After=network.target

[Service]
Type=simple
ExecStart=$ENGINE
Restart=on-failure
RestartSec=2

[Install]
WantedBy=default.target
EOF

if command -v systemctl >/dev/null 2>&1; then
  systemctl --user daemon-reload
  systemctl --user enable --now aifred-engine.service
else
  log "systemctl unavailable; starting engine directly for this session."
  "$ENGINE" >/dev/null 2>&1 &
fi

sleep 2
if ! curl -fsS "$GATEWAY_URL/health" | grep -q '"local_ai_ready":true'; then
  echo "AIFRED local AI verification failed. Check Ollama, aifred:latest, and AifredEngine." >&2
  exit 1
fi

log "Local AI ready."
