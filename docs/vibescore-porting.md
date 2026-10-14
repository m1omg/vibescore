# VibeScore: porting notes

VibeScore is MuseScore Studio 4.7 for Android, iOS/iPadOS and the web browser. This document explains how the port
is built and where its parts are, for people who work on the code. Installing and using the app is described in
[README.md](../README.md).

The work is kept additive where possible, so that upstream MuseScore releases (4.7.x, later 5.x) can be merged:
new code is in new files (`src/mobile`, `platform/android|ios|web` folders), and changes to existing files are
behind `MUE_CONFIGURATION_IS_APPMOBILE`, `MUE_CONFIGURATION_IS_APPWEB`, `MUSE_CONFIGURATION_IS_MOBILE` or
`Q_OS_ANDROID` / `Q_OS_IOS` / `Q_OS_WASM`.

## Build configurations

| Configuration | `MUSESCORE_BUILD_CONFIGURATION` | CMake setup | Platforms |
|---|---|---|---|
| Mobile app | `app-mobile` | `buildscripts/cmake/SetupConfigureMobile.cmake` | Android, iOS (also the desktop, to try the touch UI) |
| Web app | `app-web` | `buildscripts/cmake/SetupConfigureWeb.cmake` (the mobile app, minus what browsers lack) | Qt for WebAssembly, single thread |

Branding (name, identifiers, URLs, version) is in one place: `buildscripts/cmake/MobileBranding.cmake`.

Services operated by Muse Group (MuseScore.com, Audio.com, Muse Sounds, the updater, crash reports, telemetry,
the welcome dialog) are turned off. Learn videos and language packs are kept.

## Building

All builds run at idle CPU and I/O priority with few parallel jobs (`vs_lowprio` in `buildscripts/mobile/env.sh`;
`VS_JOBS=N` changes the number of jobs). The toolchain is installed without root into `~/osm/toolchain`, where
`~/osm` is a link to the project folder without spaces in its path (Gradle, the NDK and Emscripten fail on spaces).

### Android

Toolchain: Qt 6.10.2 for `linux_gcc_64` (host tools), `android_arm64_v8a` and `android_x86_64` (with `qt5compat`,
`qtshadertools`), Android SDK with NDK 27.2.12479018, JDK 21.

```sh
buildscripts/mobile/android_build.sh [x86_64|arm64] [Release|Debug]   # configure (first time) and build the APK
buildscripts/mobile/emulator.sh VS_Tablet                             # boot an emulator (low priority, 2 cores)
buildscripts/mobile/android_run.sh x86_64                             # install, start, follow the log
buildscripts/mobile/screenshot.sh out.png
buildscripts/mobile/touch.sh tap|doubletap|longpress|drag|pinch ...   # multi-touch input (needs adb root)
buildscripts/mobile/emulator.sh stop
```

CI: `.github/workflows/build_android.yml` builds the arm64 APK; it is signed with the release key when the
`ANDROID_KEYSTORE_BASE64` and `ANDROID_KEYSTORE_PASS` secrets exist, otherwise with a throwaway key.

### iOS / iPadOS

There is no local iOS build (no Mac): `.github/workflows/build_ios.yml` builds an unsigned IPA for devices (to sideload)
and an x86_64 simulator app, which `buildscripts/mobile/ios_simulator_smoke_test.sh` starts on iPhone and iPad
simulators (an Intel Mac runner: Qt's iOS package has no arm64 simulator code). The screenshots and logs are kept
as an artifact.

### Web

Toolchain: Emscripten 4.0.7 (the version Qt 6.10 is built with) in `~/osm/toolchain/emsdk`, and Qt 6.10.2 for
`wasm_singlethread` (with `qt5compat`, `qtshadertools`).

```sh
buildscripts/web/web_build.sh [Release|Debug]   # configure (first time) and build into ~/osm/build/web/webapp
buildscripts/web/serve.sh [port]                # http://localhost:8000/
```

CI: `.github/workflows/build_web.yml` builds the web app and publishes it on GitHub Pages.

### Releases

Pushing a tag `vX.Y.Z` runs `.github/workflows/release.yml`: it builds the Android APK and the iOS IPA (through the
build workflows) and publishes them on GitHub Releases, with checksums and the notes of
`buildscripts/release/release_notes.md`. Android installs an update only when it is signed with the same key as the
installed app, so releases need the release key in the repository's secrets `ANDROID_KEYSTORE_BASE64` (the keystore,
base64-encoded; key alias `vibescore`) and `ANDROID_KEYSTORE_PASS`. The version is set in `MobileBranding.cmake`.

### Unit tests

Host tests use the `utest` configuration (Debug, ASAN) in `~/osm/build/host-utest`; run them with
`QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0`. Tests written for the port: the touch controller
(`notationviewtouchcontroller_tests.cpp`) and software keyboard input (`notationviewinputcontroller_tests.cpp`).

## Architecture

### One window

Mobile systems and browsers have one window. Everything that the desktop app shows in separate windows is shown
inside the main window instead:

- **Popups, menus, dropdowns, tooltips and QML dialogs** (`WindowView` and its subclasses) are hosted by
  `InWindowHost.qml` (a `T.Popup` on the window's overlay) when `MUSE_CONFIGURATION_IS_MOBILE` is set. Dialogs stay
  inside the safe area; dialogs with `fillsCompactWindow` take the whole window on phones.
- **QWidget dialogs** (Style, Page settings, Transpose, Staff properties, ...) are shown in
  `MobileWidgetDialogFrame`: the whole window, a title bar with a close button, kinetic scrolling, and the dialog's
  buttons kept in sight.
- **Touch versions of dialogs**: Transpose, Measures per system, Create tuplet and Select (More…) have QML versions
  with large controls (`TransposeDialog.qml`, `BreaksDialog.qml`, `TupletDialog.qml`, `SelectDialog.qml` with their
  models in `notationscene`), which
  mobile and web builds register for the same URIs instead of the QWidget dialogs.
- **"Open in a new window"** replaces the current score (`MobileWindowsProvider`).
- **Keyboard focus**: in-window dialogs and menus give nothing the focus back when they close, so the mobile shell
  gives it to the score when nothing else has it (`MobileShellView::focusScoreIfNothingActive`); the score's shortcuts
  then work from a hardware keyboard.

### The mobile shell (`src/mobile/qml/MuseScore/Mobile`)

`guiapp.cpp` loads `MuseScore/Mobile/Main.qml` instead of the desktop window (`MUSE_DESKTOP_SHELL=1` loads the
desktop window, for comparison). `MobileShellView` implements `IDockWindow`, so the View menu, the `dock-*`
actions and panels that the app opens by itself work with the shell's sheets:

- Tablets: panels beside the score (side sheet: palettes, layout, properties, selection filter, history) and below
  it (bottom sheet: keypad, mixer, piano keyboard, timeline, percussion).
- Phones (either orientation): the same panels over the score, one at a time; the app menu is a drawer that shows
  one menu level at a time; the note input keypad comes with note input; Home has a bottom bar.

### Touch input on the score

`NotationViewTouchController` turns touches into the mouse events that `NotationViewInputController` already
handles: a tap is a click (with a larger hit tolerance), a double tap a double click, a drag pans the score (with
momentum) unless it starts on a selected element or a grip, a long press opens the context menu (in note input:
a preview of the note under the finger), and two fingers zoom and pan. A stylus keeps the precise mouse behavior.
`ITouchModifiers` holds the Shift and Ctrl toggles that the shell shows over the score.
While a gesture moves the whole score (pan, pinch, fling), the score is not painted again for every finger movement:
the GPU shows the last painting moved and scaled (a transform node around the painted item), and the score is painted
again every 120 ms and when the gesture ends (`AbstractNotationPaintView::setMovingByGesture`).

### Software keyboards

When text editing starts on a touch screen, the keyboard is shown. Input methods see the text of the line with the
cursor (without the text being composed), get hints (no corrections or capitals in lyrics, chord symbols,
fingerings), and their replacements, cursor moves and suggestions are applied (`NotationInteraction::editText`).
Qt for Android only sends keyboard input to an object with an `inputMethodHints` property, which the score view
therefore has. A space or hyphen that a keyboard sends as text in lyrics moves on, as the keys do.
While text is edited, `TextEditBar.qml` (above the keyboard, in place of the status bar) has the actions of the keys
that software keyboards lack (next syllable, word, verse, beat; bold, italic, underline) and Done; its buttons are not
in the keyboard navigation, so that tapping them leaves the focus (and the typed text) on the score.

### Files

Scores live in a library folder (`userDataPath()/Scores`): Android app-specific external storage (reachable over
USB), iOS `Documents` (shown by the Files app), the web: IndexedDB. Save, autosave and recent files work as on the
desktop. `MobileInteractive` (on top of the default `IInteractive`) opens files with the system's document picker
(the browser's file chooser on the web) and imports them, saves scores into the library, and shares exports
(Android share sheet, iOS activity view; downloads on the web). Home has a Library tab with all the library's
scores (rename, duplicate, delete, share).

### Audio and MIDI

- Android: `OboeAudioDriver` (AAudio, OpenSL ES fallback); MIDI devices through `android.media.midi`. Bluetooth LE
  MIDI devices are chosen in the system's companion device chooser (no location permission) and opened with
  `MidiManager.openBluetoothDevice` (`VibeScoreBluetoothMidi.java`); then they are MIDI devices like the others.
- iOS: `IosAudioDriver` (RemoteIO, AVAudioSession); MIDI through CoreMIDI; Bluetooth MIDI devices are connected in
  Apple's panel (`CABTMIDICentralViewController`).
- Both: Preferences → Audio & MIDI → "Connect Bluetooth MIDI device…" runs the action `midi-connect-bluetooth`
  (`BluetoothMidiController` in the mobile module).
- Web: `WebAudioDriver`. There is no audio thread: the engine renders on the main thread, a little ahead of time,
  and the blocks are played by an AudioWorklet. Browsers play audio after a user gesture. The page downloads the
  sound font after the app started (once: it is kept in IndexedDB); instruments that find no sound font wait for
  one (`EventAudioSource`), so a score opened before the download finished gets its sounds then.

The app pauses playback and stops the audio stream when it goes to the background, and saves an autosave.

### The web app

`buildscripts/web/index.html` and `vibescore-web.js` download the app with a progress bar and say which step of
the start takes too long or failed (the page is stamped with the build by `stamp_page.py`, so that the browser
does not mix files of different builds). They prepare the file system before the app starts: the user's
folders (`/home/web_user`, `/files/data`) and the downloaded sound font (`/files/share/sound`) are mounted from
IndexedDB, and the application data (`/files/share`) is unpacked from `share.data` (made by `pack_share.py` at build
time). The folders are written back to IndexedDB
regularly and when the page is hidden. The start waits for IndexedDB (the app's runtime opens a database too); a
database that the browser does not open in 10 seconds fails instead, and the app starts without the saved data
and says so. The app is linked with Asyncify, so that dialogs that wait for an answer
(nested event loops) work; `web::fixNestedEventLoops()` works around a Qt 6.10 bug that stopped the event processing
after such a dialog.

## Platform checks

Qt defines `Q_OS_LINUX` on Android and the deprecated `Q_OS_MAC` on iOS; CMake's host platform checks are wrong
for cross builds. `buildscripts/cmake/GetPlatformInfo.cmake` sets `OS_IS_ANDROID`, `OS_IS_IOS`, `OS_IS_MOBILE`
and `OS_IS_WASM` from the target system, and code checks `Q_OS_ANDROID` / `Q_OS_IOS` / `Q_OS_WASM` before the
desktop platforms.

## Not available

- Muse Sounds (a proprietary library), VST and other audio plugins.
- Video export (FFmpeg), AAC export (fdk-aac is not compatible with the GPL).
- On the web: MIDI devices, language packs, Learn, plugins and audio export (for now).
