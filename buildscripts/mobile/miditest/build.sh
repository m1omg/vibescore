#!/usr/bin/env bash
# Builds and installs the MIDI test device app (see README.md), with Qt's Gradle wrapper
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$HERE/../env.sh"

BUILD="$VS_ROOT/build/miditest"
rm -rf "$BUILD"
mkdir -p "$BUILD"
cp -r "$HERE/src" "$HERE/build.gradle" "$HERE/settings.gradle" "$BUILD/"
cp -r "$QT_ANDROID_X86_64/src/3rdparty/gradle/." "$BUILD/"
echo "sdk.dir=$ANDROID_SDK_ROOT" > "$BUILD/local.properties"

cd "$BUILD"
vs_lowprio ./gradlew --no-daemon -q assembleDebug
adb install -r "$(find build/outputs/apk/debug -name "*.apk" | head -1)"
