#!/usr/bin/env bash
# Configure (first time) and build the VibeScore web app (WebAssembly).
# Usage: buildscripts/web/web_build.sh [Release|Debug|RelWithDebInfo]
# The result (index.html, VibeScore.js/.wasm, qtloader.js, share.data, sound/) is in ~/osm/build/web/webapp;
# serve it with serve.sh.
# VS_WEB_THREADS=1: the app with threads (Qt for WebAssembly multithread), in ~/osm/build/web-threads; then
# add_threads_app.py puts it into the web app above (threads/), see the end of this script.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$HERE/env.sh"

BUILD_TYPE="${1:-Release}"
SRC="$VS_ROOT/MuseScore"
if [ "${VS_WEB_THREADS:-0}" = "1" ]; then
    BUILD="$VS_ROOT/build/web-threads"
    QT_KIT="$QT_WASM_THREADS"
    THREADS=ON
else
    BUILD="$VS_ROOT/build/web"
    QT_KIT="$QT_WASM"
    THREADS=OFF
fi

if [ ! -f "$BUILD/build.ninja" ]; then
    vs_lowprio "$QT_KIT/bin/qt-cmake" -S "$SRC" -B "$BUILD" -G Ninja \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DMUSESCORE_BUILD_CONFIGURATION=app-web \
        -DVS_WEB_THREADS="$THREADS" \
        -DCMAKE_AUTOGEN_PARALLEL="$VS_JOBS" \
        -DQT_HOST_PATH="$QT_HOST_PATH"
fi

# Low priority, VS_JOBS parallel jobs (see ../mobile/env.sh)
vs_lowprio cmake --build "$BUILD" -j"$VS_JOBS"

# The web app gets the app with threads, when that was built
if [ -f "$VS_ROOT/build/web-threads/webapp/VibeScore.wasm" ] && [ -f "$VS_ROOT/build/web/webapp/VibeScore.wasm" ]; then
    python3 "$HERE/add_threads_app.py" "$VS_ROOT/build/web/webapp" "$VS_ROOT/build/web-threads/webapp"
fi
