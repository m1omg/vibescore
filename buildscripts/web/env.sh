# VibeScore web toolchain environment (bash/zsh): source buildscripts/web/env.sh
# Emscripten (emsdk) and Qt for WebAssembly live in ~/osm/toolchain, see docs/vibescore-porting.md
HERE_WEB="$(cd "$(dirname "${BASH_SOURCE[0]:-${(%):-%x}}")" && pwd)"
source "$HERE_WEB/../mobile/env.sh"

export VS_EMSDK_VERSION="4.0.7" # the version Qt 6.10 is built with
export EMSDK="$VS_TOOLCHAIN/emsdk"
# The emsdk configuration finds everything from this path (keep it space-free: ~/osm)
export EM_CONFIG="$EMSDK/.emscripten"
export EMSDK_NODE="$(ls -d "$EMSDK"/node/*/bin/node 2>/dev/null | head -1)"
export PATH="$EMSDK:$EMSDK/upstream/emscripten:$(dirname "${EMSDK_NODE:-/}"):$PATH"
export QT_WASM="$VS_TOOLCHAIN/Qt/$QT_VERSION_VS/wasm_singlethread"
