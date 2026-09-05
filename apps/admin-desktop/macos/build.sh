#!/bin/sh
set -eu
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/../../.." && pwd)
APP="$REPO_ROOT/out/macos-arm64/build/admin/AIFRED Admin.app"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
swiftc "$SCRIPT_DIR/AIFREDAdmin.swift" -framework Cocoa -framework WebKit -o "$APP/Contents/MacOS/AIFREDAdmin"
printf '%s\n' "$REPO_ROOT" > "$APP/Contents/Resources/repo-root.txt"
printf '%s\n' '<?xml version="1.0" encoding="UTF-8"?>' '<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">' '<plist version="1.0"><dict><key>CFBundleExecutable</key><string>AIFREDAdmin</string><key>CFBundleIdentifier</key><string>com.north3rnlight3r.aifred.admin</string><key>CFBundleName</key><string>AIFRED Admin</string><key>CFBundlePackageType</key><string>APPL</string><key>CFBundleShortVersionString</key><string>1.0.0</string><key>NSHighResolutionCapable</key><true/></dict></plist>' > "$APP/Contents/Info.plist"
echo "$APP"
