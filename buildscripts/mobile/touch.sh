#!/usr/bin/env bash
# Touch input for the Android emulator, written straight to its multi-touch device
# (`adb shell input` can neither pinch nor tap twice quickly enough for a double tap).
# Coordinates are screen pixels in the device's natural orientation.
# Writing to the input device needs root: run `adb root` first (google_apis emulator images allow it).
#
# Usage:
#   touch.sh tap X Y
#   touch.sh doubletap X Y
#   touch.sh longpress X Y [MS]
#   touch.sh drag X1 Y1 X2 Y2 [STEPS]
#   touch.sh pinch CX CY D1 D2 [STEPS]    two fingers on a horizontal line, D1 -> D2 pixels apart
#
# VS_TOUCH_DEVICE overrides the input device (default: /dev/input/event2, the emulator's first touch screen).
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$HERE/env.sh"

DEV="${VS_TOUCH_DEVICE:-/dev/input/event2}"
SIZE="$(adb shell wm size | tail -1 | sed 's/.*: //' | tr -d '\r')"
W="${SIZE%x*}"
H="${SIZE#*x}"

CMDS=""
ev() { CMDS+="sendevent $DEV $1 $2 $3;"; }
sync_ev() { ev 0 0 0; }
pause() { CMDS+="sleep $1;"; }
tx() { echo $(( $1 * 32767 / (W - 1) )); }
ty() { echo $(( $1 * 32767 / (H - 1) )); }

NEXT_ID=$(( RANDOM % 1000 + 1 ))
finger_down() { # slot x y
    ev 3 47 "$1"; ev 3 57 "$NEXT_ID"; ev 3 53 "$(tx "$2")"; ev 3 54 "$(ty "$3")"; ev 3 58 50
    NEXT_ID=$(( NEXT_ID + 1 ))
}
finger_move() { # slot x y
    ev 3 47 "$1"; ev 3 53 "$(tx "$2")"; ev 3 54 "$(ty "$3")"
}
finger_up() { # slot
    ev 3 47 "$1"; ev 3 57 -1
}

tap() { # x y
    finger_down 0 "$1" "$2"; sync_ev
    pause 0.05
    finger_up 0; sync_ev
}

cmd="${1:-}"
shift || true

case "$cmd" in
tap)
    tap "$1" "$2"
    ;;
doubletap)
    tap "$1" "$2"
    pause 0.1
    tap "$1" "$2"
    ;;
longpress)
    finger_down 0 "$1" "$2"; sync_ev
    pause "$(awk "BEGIN { print ${3:-900} / 1000 }")"
    finger_up 0; sync_ev
    ;;
drag)
    steps="${5:-15}"
    finger_down 0 "$1" "$2"; sync_ev
    for ((i = 1; i <= steps; i++)); do
        pause 0.016
        finger_move 0 $(( $1 + ($3 - $1) * i / steps )) $(( $2 + ($4 - $2) * i / steps )); sync_ev
    done
    finger_up 0; sync_ev
    ;;
pinch)
    cx="$1"; cy="$2"; d1="$3"; d2="$4"; steps="${5:-15}"
    finger_down 0 $(( cx - d1 / 2 )) "$cy"
    finger_down 1 $(( cx + d1 / 2 )) "$cy"; sync_ev
    for ((i = 1; i <= steps; i++)); do
        pause 0.016
        d=$(( d1 + (d2 - d1) * i / steps ))
        finger_move 0 $(( cx - d / 2 )) "$cy"
        finger_move 1 $(( cx + d / 2 )) "$cy"; sync_ev
    done
    finger_up 1; sync_ev
    finger_up 0; sync_ev
    ;;
*)
    sed -n '2,14p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
    exit 1
    ;;
esac

adb shell "$CMDS"
