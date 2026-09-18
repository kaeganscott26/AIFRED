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
require_tools cmake ninja dotnet python3 git tar
prepare_origin_source

cmake -S "$SOURCE_ROOT" -B "$BUILD_ROOT" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_STANDARD=20 \
  -DCMAKE_CXX_STANDARD_REQUIRED=ON \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DJUCE_BUILD_EXAMPLES=OFF \
  -DJUCE_BUILD_EXTRAS=OFF
[[ "$action" == configure ]] && exit 0

cmake --build "$BUILD_ROOT" --target \
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
ctest --test-dir "$BUILD_ROOT" --output-on-failure
python3 -B "$SOURCE_ROOT/scripts/common/check_shared_core.py"
[[ "$action" == test ]] && exit 0

stage_release
[[ "$action" == stage ]] && exit 0

package_release
[[ "$action" == package || "$action" == release ]] && exit 0
