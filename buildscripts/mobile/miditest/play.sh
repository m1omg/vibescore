#!/usr/bin/env bash
# Plays MIDI notes on the test keyboard: play.sh 60 62 64
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$HERE/../env.sh"

notes="$(IFS=,; echo "$*")"
adb shell am broadcast -n io.github.m1omg.vibescore.miditest/.PlayReceiver --eia notes "$notes" > /dev/null
