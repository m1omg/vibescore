#!/usr/bin/env bash
# Smoke test of the iOS app on simulators (CI, macOS): installs the app on an iPad and an iPhone simulator,
# starts it, opens a score from the bundled templates, saves screenshots and logs, and fails if it crashed.
# Usage: ios_simulator_smoke_test.sh <path/to/VibeScore.app> <output dir>
set -euo pipefail

APP="$1"
OUT="$2"
mkdir -p "$OUT"
# Absolute: simctl hands the --stdout/--stderr paths to the simulator's launchd, which resolves relative ones from /
OUT="$(cd "$OUT" && pwd)"

BUNDLE_ID="$(/usr/libexec/PlistBuddy -c "Print CFBundleIdentifier" "$APP/Info.plist")"

# Built without signing: sign it to run locally, as Xcode does for simulators
codesign --force --sign - --timestamp=none "$APP"

EXECUTABLE="$(/usr/libexec/PlistBuddy -c "Print CFBundleExecutable" "$APP/Info.plist")"
{
    echo "== $APP"
    ls -la "$APP" | head -30
    file "$APP/$EXECUTABLE"
    lipo -info "$APP/$EXECUTABLE" || true
    codesign -dv "$APP" 2>&1 || true
    /usr/libexec/PlistBuddy -c "Print" "$APP/Info.plist"
} > "$OUT/app-info.txt" 2>&1

SCORE="share/templates/02-Choral/07-Voice_+_Piano/07-Voice_+_Piano.mscx"
STARTUP_SECONDS=90

# The first available device of the newest iOS runtime whose name starts with the prefix
find_device() {
    xcrun simctl list devices available -j | python3 -c '
import json, sys
prefix = sys.argv[1]
devices = json.load(sys.stdin)["devices"]
runtimes = sorted((r for r in devices if "iOS" in r), reverse=True)
for runtime in runtimes:
    for device in devices[runtime]:
        if device["name"].startswith(prefix):
            print(device["udid"], device["name"], sep="|")
            sys.exit(0)
sys.exit(1)
' "$1"
}

is_running() {
    xcrun simctl spawn "$1" launchctl list 2>/dev/null | grep -q "UIKitApplication:$BUNDLE_ID"
}

failed=0

run_on() {
    local prefix="$1" label="$2"
    local entry udid name container
    entry="$(find_device "$prefix")" || { echo "No $prefix simulator"; failed=1; return; }
    udid="${entry%%|*}"
    name="${entry#*|}"
    echo "== $label: $name ($udid)"

    xcrun simctl boot "$udid" 2>/dev/null || true
    xcrun simctl bootstatus "$udid" -b
    xcrun simctl install "$udid" "$APP"

    # 1. Start: Home page
    if ! xcrun simctl launch --terminate-running-process --stdout="$OUT/$label-1-home.log" --stderr="$OUT/$label-1-home.log" \
        "$udid" "$BUNDLE_ID"; then
        echo "::error::$label: the app could not be started"
        xcrun simctl spawn "$udid" log show --last 5m --style compact --predicate 'process == "SpringBoard" OR process == "runningboardd" OR process == "launchd_sim" OR process CONTAINS "VibeScore"' \
            > "$OUT/$label-system.log" 2>&1 || true
        # Started directly, the binary tells why it does not run (dyld errors, missing libraries, ...)
        container="$(xcrun simctl get_app_container "$udid" "$BUNDLE_ID" app 2>/dev/null || true)"
        if [ -n "$container" ]; then
            ( xcrun simctl spawn "$udid" "$container/$EXECUTABLE" > "$OUT/$label-spawn.log" 2>&1 & pid=$!; sleep 30; kill $pid 2>/dev/null ) || true
        fi
        failed=1
        return
    fi
    sleep "$STARTUP_SECONDS"
    xcrun simctl io "$udid" screenshot "$OUT/$label-1-home.png"
    if ! is_running "$udid"; then
        echo "::error::$label: the app is not running after start"
        failed=1
    fi

    # 2. A score passed on the command line
    container="$(xcrun simctl get_app_container "$udid" "$BUNDLE_ID" app)"
    xcrun simctl launch --terminate-running-process --stdout="$OUT/$label-2-score.log" --stderr="$OUT/$label-2-score.log" \
        "$udid" "$BUNDLE_ID" "$container/$SCORE"
    sleep "$STARTUP_SECONDS"
    xcrun simctl io "$udid" screenshot "$OUT/$label-2-score.png"
    if ! is_running "$udid"; then
        echo "::error::$label: the app is not running after opening a score"
        failed=1
    fi

    # Crash reports of the app, if any
    find "$HOME/Library/Logs/DiagnosticReports" -name "VibeScore*" -newer "$OUT" -exec cp {} "$OUT/" \; 2>/dev/null || true

    xcrun simctl terminate "$udid" "$BUNDLE_ID" 2>/dev/null || true
    xcrun simctl shutdown "$udid" 2>/dev/null || true
}

run_on "iPad" "ipad"
run_on "iPhone" "iphone"

ls -la "$OUT"
exit $failed
