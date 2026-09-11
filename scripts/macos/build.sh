#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
cd "$ROOT"
ACTION="${1:-help}"
case "$ACTION" in
  configure) cmake --preset macos-release ;;
  build) cmake --preset macos-release; cmake --build --preset macos-release --target Aifred_VST3 ;;
  test) python3 -B scripts/common/check_repository.py; cmake --build --preset macos-release; ctest --preset macos-release ;;
  *) echo 'SCAFFOLDED / NOT VALIDATED. configure/build/test are developer entry points. stage/package/install/uninstall/update/rollback require the documented platform and channel gates.' >&2; exit 2 ;;
esac
