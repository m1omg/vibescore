#!/usr/bin/env bash
# Install and start VibeScore on the connected device/emulator, then follow its log.
# Usage: buildscripts/mobile/android_run.sh [x86_64|arm64] [-- app arguments...]
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$HERE/env.sh"

ABI="${1:-x86_64}"
shift || true
[ "${1:-}" = "--" ] && shift
APP_ARGS="$*"

APP_ID="io.github.m1omg.vibescore"
BUILD="$VS_ROOT/build/android-$ABI"
APK="$(find "$BUILD" -name "*.apk" -path "*outputs*" -printf '%T@ %p\n' | sort -n | tail -1 | cut -d' ' -f2-)"

[ -n "$APK" ] || { echo "No APK found in $BUILD" >&2; exit 1; }
echo "Installing $APK"
adb install -r -g "$APK"

adb logcat -c
if [ -n "$APP_ARGS" ]; then
    adb shell am start -S -n "$APP_ID/org.qtproject.qt.android.bindings.QtActivity" --es applicationArguments "'$APP_ARGS'"
else
    adb shell am start -S -n "$APP_ID/org.qtproject.qt.android.bindings.QtActivity"
fi
