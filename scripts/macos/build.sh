#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/common.sh"

action="${1:-release}"
case "$action" in
  configure|build|test|stage|package|release) ;;
  *) echo "Usage: $0 [configure|build|test|stage|package|release]" >&2; exit 2 ;;
esac

require_macos
require_tools cmake ninja dotnet python3 git tar shasum
prepare_origin_source

cmake --preset macos-release
[[ "$action" == configure ]] && exit 0

cmake --build --preset macos-release --target \
  Aifred_VST3 \
  aifred_frontend_contract_tests \
  aifred_fixture_meter \
  aifred_state_contract_tests \
  aifred_gui_layout_tests \
  aifred_core_tests \
  aifred_reference_pool_contract_tests
[[ "$action" == build ]] && exit 0

python3 -B "$SOURCE_ROOT/scripts/common/check_repository.py"
python3 -B -m unittest discover -s "$SOURCE_ROOT/scripts/tests"
dotnet run --project "$SOURCE_ROOT/tools/AifredIntelligenceHost.Tests/AifredIntelligenceHost.ContractTests.csproj" -c Release
ctest --preset macos-release
python3 -B "$SOURCE_ROOT/scripts/common/check_shared_core.py"
[[ "$action" == test ]] && exit 0

python3 -B "$ROOT/scripts/common/release.py" prepare --platform macos-arm64
stage_release
python3 -B "$ROOT/scripts/common/release.py" manifest --platform macos-arm64
python3 -B "$ROOT/scripts/common/release.py" verify --platform macos-arm64 --location stage
[[ "$action" == stage || "$action" == package ]] && exit 0

python3 -B "$ROOT/scripts/common/release.py" promote --platform macos-arm64

package_release
[[ "$action" == release ]] && exit 0
