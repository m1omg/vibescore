#!/usr/bin/env bash
# Serves the web app that web_build.sh built, for testing: http://localhost:8000/
# Usage: buildscripts/web/serve.sh [port]
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$HERE/env.sh"
PORT="${1:-8000}"
cd "$VS_ROOT/build/web/webapp"
echo "VibeScore: http://localhost:$PORT/"
exec python3 -m http.server "$PORT" --bind 127.0.0.1
