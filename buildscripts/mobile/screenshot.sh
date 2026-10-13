#!/usr/bin/env bash
# Save a screenshot of the connected device/emulator.
# Usage: buildscripts/mobile/screenshot.sh <output.png>
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$HERE/env.sh"
adb exec-out screencap -p > "${1:-screenshot.png}"
echo "${1:-screenshot.png}"
