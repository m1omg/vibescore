# VibeScore MIDI test device

A tiny Android app that provides a virtual MIDI keyboard (a `MidiDeviceService`), to test
MIDI input on an emulator without hardware.

```sh
buildscripts/mobile/miditest/build.sh            # builds and installs the test app
# In VibeScore: Preferences > Audio & MIDI > MIDI input: "VibeScore Test Keyboard"
buildscripts/mobile/miditest/play.sh 60 62 64    # plays C4, D4, E4 (MIDI note numbers)
```
