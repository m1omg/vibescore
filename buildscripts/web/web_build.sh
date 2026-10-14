#!/usr/bin/env bash
# Configure (first time) and build the VibeScore web app (WebAssembly, single thread).
# Usage: buildscripts/web/web_build.sh [Release|Debug|RelWithDebInfo]
# The result (index.html, VibeScore.js/.wasm, qtloader.js, share.data, sound/) is in ~/osm/build/web/webapp;
# serve it with serve.sh.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$HERE/env.sh"

BUILD_TYPE="${1:-Release}"
SRC="$VS_ROOT/MuseScore"
BUILD="$VS_ROOT/build/web"

if [ ! -f "$BUILD/build.ninja" ]; then
    vs_lowprio "$QT_WASM/bin/qt-cmake" -S "$SRC" -B "$BUILD" -G Ninja \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DMUSESCORE_BUILD_CONFIGURATION=app-web \
        -DCMAKE_AUTOGEN_PARALLEL="$VS_JOBS" \
        -DQT_HOST_PATH="$QT_HOST_PATH"
fi

# Low priority, VS_JOBS parallel jobs (see ../mobile/env.sh)
vs_lowprio cmake --build "$BUILD" -j"$VS_JOBS"
