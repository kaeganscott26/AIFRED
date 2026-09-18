#!/usr/bin/env bash
set -euo pipefail
endpoint="http://127.0.0.1:11434"
model="${1:-aifred:latest}"
ollama="$(command -v ollama || true)"
if [[ -z "$ollama" && -x /Applications/Ollama.app/Contents/Resources/ollama ]]; then ollama=/Applications/Ollama.app/Contents/Resources/ollama; fi
if [[ -z "$ollama" ]]; then
  tmp="$(mktemp -d)"
  trap 'rm -rf "$tmp"' EXIT
  curl -fsSL https://ollama.com/download/Ollama-darwin.zip -o "$tmp/Ollama.zip"
  ditto -x -k "$tmp/Ollama.zip" "$tmp/unpacked"
  rm -rf /Applications/Ollama.app
  cp -R "$tmp/unpacked/Ollama.app" /Applications/Ollama.app
  ollama=/Applications/Ollama.app/Contents/Resources/ollama
fi
ready() { curl -fsS --max-time 2 "$endpoint/api/tags" >/dev/null 2>&1; }
if ! ready; then nohup "$ollama" serve >/tmp/aifred-ollama.log 2>&1 & fi
for attempt in $(seq 1 60); do ready && break; [[ "$attempt" == 60 ]] && { echo "Ollama did not become ready on port 11434." >&2; exit 1; }; sleep 1; done
curl -fsS "$endpoint/api/tags" | grep -F '"name":"'$model'"' >/dev/null || "$ollama" pull "$model"
echo "Ollama is ready with $model."