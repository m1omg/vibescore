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
| Web app | `app-web` | `buildscripts/cmake/SetupConfigureWeb.cmake` (the mobile app, minus what browsers lack) | Qt for WebAssembly: multithread (`VS_WEB_THREADS=ON`) and single thread |

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
`wasm_singlethread` and `wasm_multithread` (with `qt5compat`, `qtshadertools`).

```sh
buildscripts/web/web_build.sh [Release|Debug]   # configure (first time) and build into ~/osm/build/web/webapp
VS_WEB_THREADS=1 buildscripts/web/web_build.sh  # the app with threads (~/osm/build/web-threads), put into webapp/threads
buildscripts/web/serve.sh [port]                # http://localhost:8000/
```

The site is the app without threads, with the app with threads in `threads/` (`add_threads_app.py`); the page
chooses (`?threads=0` or `?threads=1` in its address forces one).

CI: `.github/workflows/build_web.yml` builds both apps (in parallel), puts them together and publishes them on GitHub
Pages; then
`.github/workflows/test_web.yml` starts the published app in Chromium and Firefox on x86-64 and ARM64 (and on a
slowed-down ARM64 tablet) with `buildscripts/web/smoke_test.py`, which notes the time of each step of the start and
fails when the page's process needs more than 1.4 GB of memory.

### Releases

Pushing a tag `vX.Y.Z` runs `.github/workflows/release.yml`: it builds the Android APK and the iOS IPA (through the
build workflows) and publishes them on GitHub Releases, with checksums and the notes of
`buildscripts/release/release_notes.md`. Android installs an update only when it is signed with the same key as the
installed app, so releases need the release key in the repository's secrets `ANDROID_KEYSTORE_BASE64` (the keystore,
base64-encoded; key alias `vibescore`) and `ANDROID_KEYSTORE_PASS`. The version is set in `MobileBranding.cmake`.
Tag a commit without `[skip ci]` in its message: GitHub runs no workflow for a pushed tag that points to one.

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
- Web: `WebAudioDriver`. With threads, the engine runs in the driver mode on a thread of the driver, which keeps a
  ring of audio in the shared memory filled ~64 ms ahead; the AudioWorklet plays it from there (the page's thread does
  nothing for the audio), and the mixer renders the instruments in a pool of up to 4 threads. Without threads the
  engine renders on the main thread, a little ahead of time, and the blocks are sent to the AudioWorklet. Browsers
  play audio after a user gesture; until then the engine runs in real time and its audio is dropped. The page downloads the
  sound font after the app started (once: it is kept in IndexedDB); instruments that find no sound font wait for
  one (`EventAudioSource`), so a score opened before the download finished gets its sounds then. An audio export
  renders in the driver's callback too: every 150 ms `SoundTrackWriter` hands the app what the engine sent it and
  lets the app and the browser run for a moment (a nested event loop, on the app's side of the RPC channel), so that
  the progress shows and Cancel works; the driver does not call the engine again meanwhile.

The app pauses playback and stops the audio stream when it goes to the background, and saves an autosave.

### The web app

`buildscripts/web/index.html` and `vibescore-web.js` download the app with a progress bar and say which step of
the start takes too long or failed. The page is stamped by `stamp_page.py` with the version of each file (a hash of
its content), which is in the file's address (`?v=`): the browser does not mix files of different builds, and a new
build downloads only the files that changed (Qt's resources get a fixed time, `QT_RCC_SOURCE_DATE_OVERRIDE` in
`build_web.yml`: rcc wrote the time of the checkout, so the wasm differed at every build). The page keeps the app (the wasm, `share.data`) in Cache Storage itself
(`vsFetchKept`), so later starts do not download it (the browser's own cache may not keep files this big, and GitHub
Pages changes every file's ETag at every deployment). It downloads them past the service worker (`cache:
'no-store'`): browsers stop a service worker that works too long (Firefox a minute after the page's last request,
`dom.serviceWorkers.idle_timeout` + `idle_extended_timeout`), which stopped such a download, or the keeping of it, on
slower connections. A file that is kept is still taken through the service worker, which gives it from the cache:
Chromium keeps the compiled code of a wasm only with such a response. They prepare the file system before the app
starts: the user's
folders (`/home/web_user`, `/files/data`) and the downloaded sound font (`/files/share/sound`) are mounted from
IndexedDB, and the application data (`/files/share`) is unpacked from `share.data` (made by `pack_share.py` at build
time). The folders are written back to IndexedDB
regularly and when the page is hidden. The start waits for IndexedDB (the app's runtime opens a database too); a
database that the browser does not open in 10 seconds fails instead, and the app starts without the saved data
and says so. The app is linked with Asyncify, so that dialogs that wait for an answer
(nested event loops) work; `web::fixNestedEventLoops()` works around a Qt 6.10 bug that stopped the event processing
after such a dialog. Nothing is drawn while the app's thread is busy, e.g. opening a big score: a dialog that was just
answered stayed on the screen meanwhile, as if the app had frozen. Opening a score shows "Opening “name”…" first
(`ProjectActionsController::doOpenProject`, `MobileInteractive::showProgress`), and waits until each window has drawn
two frames (an event loop until `QQuickWindow::frameSwapped`, at most 1 s) before it starts: a browser shows a drawn
frame at its next refresh, which can come after a task that starts at once (Firefox), and asks for the next frame then.

Threads: the app with threads needs shared memory (SharedArrayBuffer), which browsers give only to a cross-origin
isolated page (headers COOP/COEP, which GitHub Pages does not send). The service worker `vibescore-sw.js` adds them
to the page and to the workers' scripts; the first time, the page registers it and loads itself again. Where that
does not work (no service workers, e.g. Firefox's private windows), or the app with threads failed to start with this
build, the page starts the one without threads. The service worker also gives the app's files from Cache Storage
(it keeps the scripts there; the page keeps the rest), so the app starts offline; the page itself comes from the
network when there is one. Each thread is a Web Worker with an
instance of the app's code (a pool of 8 is started with the app, `QT_WASM_PTHREAD_POOL_SIZE`), the shared memory is
reserved up to 2 GB, and the threads get 1 MB stacks. File access from a thread is done by the page's thread.

Start (measured in Chromium slowed down 4x): `main.cpp` set `QML_DISABLE_DISK_CACHE`, which in Qt 6 also turns off
the QML compiled when the app is built, so all the QML was compiled at every start; on mobile platforms and the web
it sets `QML_DISK_CACHE=aot`, and the QML modules are compiled to byte code only (`--only-bytecode`, see
`muse_create_qml_module`: the same byte code as at run time, without the C++ that qmlcachegen can generate; the module
is 18 MB smaller). The music fonts are added to the font database at the start, but a font reads its metadata when
it is used first, and each glyph is measured when its symbol is used first (measuring the thousands of glyphs of all
the fonts was a third of the start). The palettes with all the elements and the default ones (for "Reset palette")
are made when they are needed. The sound font is read from IndexedDB after the start. The download's progress is
shown a few times a second.

Opening a big score: a lyrics line (the hyphens and melismas, which the layout makes) made the score's map of spanners
rebuild its lookup tree at its next search, for every sung syllable; now only spanners that are in the map do.

Memory (what phones and tablets give a tab is limited): the module is ~115 MB, ~90 MB of it code. The link keeps
functions with a single caller apart when they are big (`--one-caller-inline-max-function-size`): otherwise all the
static initializers end up in `__wasm_call_ctors` (1.9 MB), which Chromium's optimizing compiler compiles again after
the start for more than a gigabyte of memory (the page's process went from ~0.65 GB to ~1.9 GB; the start froze on a
tablet). Only the Qt Quick Controls style that the app uses is linked (`SetupQt6.cmake`). On mobile platforms and
the web, `Muse.GraphicalEffects` uses QtQuick.Effects (`GraphicalEffects/mobile/`): Qt5Compat's effects compile their
shaders when the app runs, which fails on WebGL 2 and OpenGL ES 3, and needs Qt Shader Tools in the app. JSPI instead of Asyncify
would halve the code, but it needs a Qt built with WebAssembly exceptions: with the prebuilt Qt (JavaScript
exceptions) every `invoke_*` import becomes a suspending import, which fails outside `WebAssembly.promising`
(e.g. in the static initializers).

Speed: everything runs on the page's one thread: the engraving, painting the score (now on the GPU: the view's
QQuickPaintedItem uses the FramebufferObject render target with Qt's OpenGL paint engine on WebGL), the QML interface
(interpreted: no JIT in WebAssembly) and the audio engine (FluidSynth, the reverb; see WebAudioDriver). What was done,
measured with a 64-measure string quartet in Chromium (a desktop CPU, and slowed down 4x with device pixel ratio 2 as
a tablet): the link is `-O3` (Asyncify's code is optimized much better than with `-Os`); the audio engine uses
WebAssembly SIMD (`-msimd128 -msse2`: the reverb's SSE2 code); the audio player (AudioWorklet) is sent each block with
the frame of the AudioContext's time at which it plays, and no silence, and sends nothing back (the page follows it
with `currentTime`): the idle app takes ~30 instead of ~105 ms of CPU a second on the slowed-down tablet; the engine
renders every 20 ms; no timing of traced functions; no walk through all objects on every mouse press
(NavigationController); a selection change paints only the old and new selection; the background is painted only
around the pages; the note input bar does not make its Add menu again (and leak it) on every change. On the
slowed-down tablet, from a key press to the new frame: a note entered 192 -> 120 ms, a pitch change 200 -> 120 ms, a
selection change 128 -> 48 ms; on the desktop CPU an edit takes 35-50% less of the CPU. What remains per edit: the
layout (no single hot spot; Asyncify makes it ~30% slower than without), the audio engine while notes sound (~25% of
the CPU, but spread over time; with threads it runs off the page's thread), QML updates. A bigger step would be JSPI
(above). Measuring: a CPU profile of the page per group of actions (Chrome
DevTools Protocol), with the Event Timing API for the latency; the wasm functions' names come from a link with
`--emit-symbol-map`.

Two traps of the WebAssembly build: `size_t` has 32 bits, and the queues' `nextPowerOfTwo()` shifted it by 32
(undefined; Clang made every queue's capacity 1, so a score with many instruments queued its playback for minutes);
and Qt arms its browser timer again only when a Qt timer was due when it fired, which Firefox's coarser clock made
fail (Qt's timers stopped until the next input event): `vsLengthenQtTimers()` makes the page's timers 2 ms longer.
Chromium keeps the compiled app in its code cache only when the fetched response itself is compiled (the loader
counts the download on a copy).

Installed app (from the Home Screen of an iPhone or iPad, or installed from Chrome/Edge; `vibescoreIsInstalled()`:
`display-mode: standalone` or `navigator.standalone`): phones and tablets close such an app in the background to
free memory, like any app, so it behaves like the mobile apps. The page tells the app when it is hidden or shown
(`web::watchPageVisibility`: the application state becomes inactive or active, through Qt's queue of browser events),
and the changes of the open score are autosaved when it is hidden (`ProjectAutoSaver`, as on Android and iOS; also in
a browser tab, which a phone's browser may close too) and written to IndexedDB. At the next start, an installed app
opens the scores that were open (`StartupScenario::restoreLastSession`); in a browser tab, closing the page is the
usual way to quit, so it starts with the Scores page. An installed app asks for persistent storage at the start
(browsers give it to such apps without asking; Safari also does not remove the data of a Home Screen app after 7
days without use, as it does for a site); in a tab it is asked after the first save. iOS needs PNG icons
(`apple-touch-icon.png`, made from the iOS app's icon) and, before iOS 26, the `apple-mobile-web-app-*` meta tags;
in Safari on an iPhone or iPad (not from the Home Screen) the loading screen says how to add the app there.

Keyboard focus: Qt for WebAssembly can leave the browser's focus on the page itself (seen when the Properties
panel was opened), and the app then got no keys; `vibescoreKeepKeyboardFocus` gives the focus back to the
window that is clicked, or that lost it to no element.

## Platform checks

Qt defines `Q_OS_LINUX` on Android and the deprecated `Q_OS_MAC` on iOS; CMake's host platform checks are wrong
for cross builds. `buildscripts/cmake/GetPlatformInfo.cmake` sets `OS_IS_ANDROID`, `OS_IS_IOS`, `OS_IS_MOBILE`
and `OS_IS_WASM` from the target system, and code checks `Q_OS_ANDROID` / `Q_OS_IOS` / `Q_OS_WASM` before the
desktop platforms.

## Not available

- Muse Sounds (a proprietary library), VST and other audio plugins.
- Video export (FFmpeg), AAC export (fdk-aac is not compatible with the GPL).
- On the web: MIDI devices, language packs, Learn and plugins (for now).
