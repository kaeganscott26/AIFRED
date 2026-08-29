#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source_path="$repo_root/integrations/forge"
destination="${1:-}"
if [[ -z "$destination" ]]; then
  echo "usage: $0 <FORGE_WORKSPACE/integrations/aifred>" >&2
  exit 2
fi
mkdir -p "$(dirname "$destination")"
if [[ -e "$destination" || -L "$destination" ]]; then
  echo "refusing to overwrite existing destination: $destination" >&2
  exit 1
fi
ln -s "$source_path" "$destination"
echo "$destination -> $source_path"
