#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
case "${1:-}" in
  uninstall) exec "$SCRIPT_DIR/uninstall.sh" ;;
  update) "$SCRIPT_DIR/build.sh" release; exec "$SCRIPT_DIR/install.sh" ;;
  *) echo "Usage: $0 [uninstall|update]" >&2; exit 2 ;;
esac
