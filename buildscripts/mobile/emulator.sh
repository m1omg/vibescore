#!/usr/bin/env bash
# Boot an Android emulator (headless by default) and wait until it is ready.
# Usage: buildscripts/mobile/emulator.sh [VS_Tablet|VS_Phone] [--window]
# Note: the default "swiftshader_indirect" GPU mode renders Qt Quick shapes incorrectly; use swangle_indirect.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$HERE/env.sh"

AVD="${1:-VS_Tablet}"
WINDOW_ARGS="-no-window"
[ "${2:-}" = "--window" ] && WINDOW_ARGS=""

if adb get-state >/dev/null 2>&1; then
    echo "A device is already connected"
    exit 0
fi

nohup emulator -avd "$AVD" $WINDOW_ARGS -no-audio -no-snapshot-save -no-boot-anim -gpu swangle_indirect \
    > "$VS_TOOLCHAIN/emulator_$AVD.log" 2>&1 &

adb wait-for-device
until [ "$(adb shell getprop sys.boot_completed 2>/dev/null | tr -d '\r')" = "1" ]; do
    sleep 2
done
echo "$AVD is ready"
