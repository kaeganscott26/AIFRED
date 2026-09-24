#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/common.sh"

endpoint="http://127.0.0.1:11434"
model="${1:-aifred:latest}"
modelfile="${2:-$DATA_PARENT/model/Modelfile}"
[[ -f "$modelfile" ]] || { echo "Canonical AIFRED Modelfile is missing: $modelfile" >&2; exit 1; }
base_model="$(awk 'toupper($1)=="FROM" {print $2; exit}' "$modelfile")"
[[ -n "$base_model" ]] || { echo "AIFRED Modelfile does not declare a base model." >&2; exit 1; }

ollama="$(command -v ollama || true)"
if [[ -z "$ollama" && -x /Applications/Ollama.app/Contents/Resources/ollama ]]; then ollama=/Applications/Ollama.app/Contents/Resources/ollama; fi
if [[ -z "$ollama" ]]; then
  setup_tmp="$(mktemp -d)"
  trap 'rm -rf "$setup_tmp"' EXIT
  curl -fsSL https://ollama.com/download/Ollama-darwin.zip -o "$setup_tmp/Ollama.zip"
  ditto -x -k "$setup_tmp/Ollama.zip" "$setup_tmp/unpacked"
  [[ ! -e /Applications/Ollama.app ]] || { echo "Existing Ollama.app is incomplete; repair it manually." >&2; exit 1; }
  cp -R "$setup_tmp/unpacked/Ollama.app" /Applications/Ollama.app
  ollama=/Applications/Ollama.app/Contents/Resources/ollama
fi
ready() { curl -fsS --max-time 2 "$endpoint/api/tags" >/dev/null 2>&1; }
if ! ready; then nohup "$ollama" serve >/tmp/aifred-ollama.log 2>&1 & fi
for attempt in $(seq 1 60); do ready && break; [[ "$attempt" == 60 ]] && { echo "Ollama did not become ready on port 11434." >&2; exit 1; }; sleep 1; done
"$ollama" show "$base_model" >/dev/null 2>&1 || "$ollama" pull "$base_model"
"$ollama" create "$model" -f "$modelfile"
"$ollama" show "$model" >/dev/null
echo "Ollama provider ready: created $model from $modelfile (base $base_model)."
