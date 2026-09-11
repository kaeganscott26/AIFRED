#!/usr/bin/env bash

# AIFRED Beta 0.3.6
# macOS arm64 all-in-one build, test, stage, and local install script.
#
# Run from the root of the AIFRED Beta repository.

set -euo pipefail

echo "===================================================="
echo "       AIFRED Beta 0.3.6 macOS Build / Install      "
echo "===================================================="

# ----------------------------------------------------
# 1. Confirm repository root
# ----------------------------------------------------

if [[ ! -f "CMakeLists.txt" ]] ||
   [[ ! -f "CMakePresets.json" ]] ||
   [[ ! -d "plugin-aifred" ]]; then
    echo "ERROR: Run this script from the AIFRED Beta repository root."
    exit 1
fi

REPO_ROOT="$(pwd)"

CMAKE_PRESET="macos-release"
BUILD_DIR="$REPO_ROOT/out/macos-arm64/build"
STAGE_DIR="$REPO_ROOT/out/macos-arm64/stage"

PLUGIN_SOURCE="$BUILD_DIR/plugin-aifred/Aifred_artefacts/Release/VST3/Aifred.vst3"
PLUGIN_STAGE="$STAGE_DIR/AIFRED Beta/Aifred.vst3"

PLUGIN_INSTALL_ROOT="$HOME/Library/Audio/Plug-Ins/VST3/AIFRED Beta"
PLUGIN_INSTALL="$PLUGIN_INSTALL_ROOT/Aifred.vst3"

HOST_PROJECT="$REPO_ROOT/tools/AifredIntelligenceHost/AifredIntelligenceHost.csproj"
HOST_TEST_PROJECT="$REPO_ROOT/tools/AifredIntelligenceHost.Tests/AifredIntelligenceHost.ContractTests.csproj"

HOST_STAGE="$STAGE_DIR/AifredIntelligenceHost"
HOST_INSTALL="$HOME/Library/Application Support/Aifred/beta/IntelligenceHost"

# ----------------------------------------------------
# 2. macOS / architecture checks
# ----------------------------------------------------

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "ERROR: This script only supports macOS."
    exit 1
fi

ARCH="$(uname -m)"

if [[ "$ARCH" != "arm64" ]]; then
    echo "ERROR: Current macOS preset targets arm64."
    echo "Detected architecture: $ARCH"
    exit 1
fi

echo " -> macOS arm64 detected."

# ----------------------------------------------------
# 3. Xcode Command Line Tools
# ----------------------------------------------------

if ! xcode-select -p >/dev/null 2>&1; then
    echo "ERROR: Xcode Command Line Tools are required."
    echo "Run:"
    echo "    xcode-select --install"
    exit 1
fi

echo " -> Xcode Command Line Tools found."

# ----------------------------------------------------
# 4. Homebrew
# ----------------------------------------------------

if ! command -v brew >/dev/null 2>&1; then
    echo " -> Homebrew not found. Installing Homebrew..."

    /bin/bash -c \
        "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

    if [[ -x "/opt/homebrew/bin/brew" ]]; then
        eval "$(/opt/homebrew/bin/brew shellenv)"
    fi
fi

echo " -> Homebrew found."

# ----------------------------------------------------
# 5. Development dependencies
# ----------------------------------------------------

echo " -> Installing/verifying build dependencies..."

brew install git cmake ninja python@3.11

if ! command -v dotnet >/dev/null 2>&1; then
    echo " -> .NET SDK not found. Installing..."
    brew install --cask dotnet-sdk
fi

export PATH="/opt/homebrew/opt/python@3.11/bin:$PATH"

echo
echo "Toolchain:"
echo "  Git:    $(git --version)"
echo "  CMake:  $(cmake --version | head -n 1)"
echo "  Ninja:  $(ninja --version)"
echo "  Python: $(python3 --version)"
echo "  .NET:   $(dotnet --version)"
echo

# ----------------------------------------------------
# 6. Repository validation
# ----------------------------------------------------

echo " -> Validating repository..."

python3 -B scripts/common/check_repository.py
python3 -B scripts/common/check_shared_core.py

# ----------------------------------------------------
# 7. Configure
# ----------------------------------------------------

echo " -> Configuring macOS build..."

cmake --preset "$CMAKE_PRESET"

# ----------------------------------------------------
# 8. Compile AIFRED VST3
# ----------------------------------------------------

echo " -> Building AIFRED Beta VST3..."

cmake --build \
    --preset "$CMAKE_PRESET" \
    --target Aifred_VST3

# ----------------------------------------------------
# 9. Build/test remaining native targets
# ----------------------------------------------------

echo " -> Building native test targets..."

cmake --build --preset "$CMAKE_PRESET"

echo " -> Running native tests..."

ctest --preset "$CMAKE_PRESET"

# ----------------------------------------------------
# 10. Test IntelligenceHost
# ----------------------------------------------------

echo " -> Testing AifredIntelligenceHost..."

dotnet run \
    --project "$HOST_TEST_PROJECT" \
    -c Release

# ----------------------------------------------------
# 11. Verify exact plugin artifact
# ----------------------------------------------------

echo " -> Verifying compiled VST3..."

if [[ ! -d "$PLUGIN_SOURCE" ]]; then
    echo "ERROR: Expected VST3 bundle was not produced:"
    echo "    $PLUGIN_SOURCE"
    exit 1
fi

echo " -> Found:"
echo "    $PLUGIN_SOURCE"

# ----------------------------------------------------
# 12. Stage plugin
# ----------------------------------------------------

echo " -> Staging plugin..."

rm -rf "$STAGE_DIR"
mkdir -p "$(dirname "$PLUGIN_STAGE")"

cp -R "$PLUGIN_SOURCE" "$PLUGIN_STAGE"

# ----------------------------------------------------
# 13. Publish IntelligenceHost
# ----------------------------------------------------

echo " -> Publishing IntelligenceHost for osx-arm64..."

dotnet publish \
    "$HOST_PROJECT" \
    -c Release \
    -r osx-arm64 \
    --self-contained false \
    -o "$HOST_STAGE"

printf '{"channel":"beta"}\n' > "$HOST_STAGE/channel.json"

# ----------------------------------------------------
# 14. Install Beta VST3
# ----------------------------------------------------

echo " -> Installing AIFRED Beta VST3..."

mkdir -p "$PLUGIN_INSTALL_ROOT"

rm -rf "$PLUGIN_INSTALL"
cp -R "$PLUGIN_STAGE" "$PLUGIN_INSTALL"

# ----------------------------------------------------
# 15. Install IntelligenceHost
# ----------------------------------------------------

echo " -> Installing AIFRED Beta IntelligenceHost..."

mkdir -p "$HOST_INSTALL"
rm -rf "$HOST_INSTALL/bin"
mkdir -p "$HOST_INSTALL/bin"

cp -R "$HOST_STAGE/." "$HOST_INSTALL/bin/"

# ----------------------------------------------------
# 16. macOS bundle sanity check
# ----------------------------------------------------

echo " -> Inspecting VST3 bundle..."

if command -v codesign >/dev/null 2>&1; then
    codesign --verify --deep --strict "$PLUGIN_INSTALL" 2>/dev/null \
        && echo " -> Existing code signature verifies." \
        || echo " -> NOTICE: Plugin is unsigned/ad-hoc or signature validation failed."
fi

# ----------------------------------------------------
# 17. Result
# ----------------------------------------------------

echo
echo "===================================================="
echo "              AIFRED Beta Build Complete            "
echo "===================================================="
echo
echo "VST3:"
echo "  $PLUGIN_INSTALL"
echo
echo "IntelligenceHost:"
echo "  $HOST_INSTALL/bin"
echo
echo "Stage:"
echo "  $STAGE_DIR"
echo
echo "Host port:"
echo "  8787"
echo
echo "NOTE:"
echo "  macOS automatic host startup / LaunchAgent is not"
echo "  installed by this script yet."
echo
echo "Restart or rescan your DAW before testing AIFRED."
echo "===================================================="