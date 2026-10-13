#!/usr/bin/env bash
# Configure (first time) and build the VibeScore APK.
# Usage: buildscripts/mobile/android_build.sh [x86_64|arm64] [Release|Debug|RelWithDebInfo]
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$HERE/env.sh"

ABI="${1:-x86_64}"
BUILD_TYPE="${2:-Release}"
case "$ABI" in
    x86_64) QT_ANDROID="$QT_ANDROID_X86_64" ;;
    arm64) QT_ANDROID="$QT_ANDROID_ARM64" ;;
    *) echo "Unknown ABI: $ABI" >&2; exit 1 ;;
esac

SRC="$VS_ROOT/MuseScore"
BUILD="$VS_ROOT/build/android-$ABI"

if [ ! -f "$BUILD/build.ninja" ]; then
    vs_lowprio "$QT_ANDROID/bin/qt-cmake" -S "$SRC" -B "$BUILD" -G Ninja \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DMUSESCORE_BUILD_CONFIGURATION=app-mobile \
        -DCMAKE_AUTOGEN_PARALLEL="$VS_JOBS" \
        -DQT_HOST_PATH="$QT_HOST_PATH" \
        -DANDROID_SDK_ROOT="$ANDROID_SDK_ROOT" \
        -DANDROID_NDK_ROOT="$ANDROID_NDK_ROOT"
fi

# Low priority, VS_JOBS parallel jobs (see env.sh)
vs_lowprio cmake --build "$BUILD" -j"$VS_JOBS"
vs_lowprio cmake --build "$BUILD" -j"$VS_JOBS" --target apk

find "$BUILD" -name "*.apk" -path "*outputs*" -newer "$BUILD/build.ninja" -print
