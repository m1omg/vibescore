# Handover: VibeScore web app, open issues after 2026-10-20

This file hands the work over to a new Claude session. Read it, then `docs/vibescore-porting.md` (sections "Audio
and MIDI" and "The web app"), which describes how everything works.

## The project

- **What it is.** VibeScore is a fork of MuseScore Studio 4.7 for phones and tablets: Android, iOS/iPadOS, and the web.
  - It uses Qt 6.10.2.
  - The mobile interface is in `src/mobile/`.
  - The web app is built with Qt for WebAssembly and Emscripten 4.0.7.
- **Where it lives.**
  - Repository: `m1omg/vibescore`. The default branch is **`vibescore`**.
  - Live web app: https://m1omg.github.io/vibescore/
- **CI.**
  - A push to `vibescore` runs `Build: Web` (`.github/workflows/build_web.yml`). It builds the app without threads and the app with threads (`threads/`), then publishes both to GitHub Pages. It takes about 30 minutes.
  - When `Build: Web` finishes, `Test: Web start` (`test_web.yml`, which runs `buildscripts/web/smoke_test.py`) starts the published app in Chromium and Firefox.
  - `Build: Android` and `Build: iOS` also run on every push.
  - Changes to `**.md` files do not trigger `Build: Web`.
- **Which app a browser gets.**
  - When the page is cross-origin isolated (the service worker adds COOP/COEP), it runs the app with threads. Chrome on Android and desktop Chromium/Firefox get that one.
  - `?threads=0` or `?threads=1` forces one of the two apps.

## Working with this user

- **Merge and deploy without asking.** The user gave standing permission to merge Claude's own PRs into `vibescore` and deploy them to GitHub Pages. Asking each time wastes their time.
- **Their preferences:**
  - Software must not depend on the display's refresh rate.
  - Use web search for current facts about apps and browsers.
  - Keep Linux help simple.
  - Be truthful, and say when something could not be tested.
- **Where they test:**
  - A Samsung Galaxy S24, in a normal Chrome tab.
  - Firefox on a desktop.
  - A friend will try the app on an iPad as a Home Screen app. That result is still pending. Build TestFlight only if the user asks.
- **Conventions:**
  - Fill in the PR template (`.github/pull_request_template.md`) in PR bodies.
  - Merge PRs with rebase.
  - Write commit messages that explain the problem and the fix.
  - Use the attribution lines your own session gives you.
  - Never put model names in commits or PRs.

## What the user reported, and what was done about it (2026-10-20, second session)

The user's words: "It's not silent anymore, but it's kind of glitchy in that one time it didn't even load the
Beethoven song. And then the other, when I backed out of it, it still continued playing." Asked, they said: "backed
out" is the ← arrow at the top left of the score, and when the Beethoven did not load, "it just froze".

### Done: leaving a score keeps it playing (fixed)

- The ← arrow (and Android's Back key) open the Scores page through `interactive->open("musescore://home")`, which
  calls `MobileShellView::loadPage`: that now pauses playback when a page that does not show the score replaces one
  that does (the score stays open as a tab, to come back to). `MobileShellView::isNotationPage` decides which pages
  show the score (the QML copy of that check is gone).
- On the web, playback also went on while the page was hidden (another app or tab in front): Qt for WebAssembly
  reports no application state changes of its own (checked in qtbase 6.10, `src/plugins/platforms/wasm`:
  no `handleApplicationStateChanged`), the page reports itself hidden as `Qt::ApplicationInactive`
  (`web::watchPageVisibility`), and `MobileLifecycleController` paused only on `Qt::ApplicationSuspended`. On the
  web, inactive pauses too now.
- Not changed: the ← arrow still leaves the score open. The tab bar's × closes it. Opening another score from the
  Scores page closes the current one after asking ("Do you want to save changes…"); that dialog's third button is
  cut off on a phone (412 px wide; see "Other findings").

### Done: the play button did nothing while the sounds loaded

After a score opened, its instruments' sounds are loaded for a while (`PlaybackController::setupSequenceTracks`,
"Loading audio samples": ~0.1-1 s per instrument in the browser, more on a phone, so ~10-30 s for an orchestra),
and `isPlayAllowed()` is false until then: a tap on the phone layout's play button did nothing ("playback not
allowed" in the log). The button is disabled meanwhile now (`MobileShellView::isPlayAllowed`), like the tablet
layout's playback toolbar always was. This is likely part of "glitchy".

### Done: the "Opening…" box stood still (the page looked frozen)

While a score loads, the app's thread is busy and nothing of the app can be drawn, so the "Opening “name”…" box
stood still: for minutes on a phone with a big score. On the web, the box is now the page's (`web::showBusy`,
`vibescoreBusy`, `#busy` in `index.html`) with a spinner that the browser keeps turning on its compositor thread (a
CSS transform animation runs while the main thread is blocked). The native apps keep the QML busy dialog. The box
also takes the taps meanwhile: before, taps during the load were queued (`ExcludeUserInputEvents`) and applied after
it, wherever they landed on the score page.

### Not found: why the Beethoven once "just froze"

Not reproduced. What was measured (desktop Chromium, Pixel 7 profile, `navigator.hardwareConcurrency` 8 like the S24,
the live threads app, `devtools/load_check.py`; the stand-ins are `devtools/multiply_score.py demos/Dawn.mscx`):

| Score (measures × staves) | Load, page's thread busy | Page process (RSS) after |
|---|---|---|
| start, no score | | ~590 MB |
| Dawn x30 (570 × 13, 9 MB) | ~9 s | 920 MB; 1160 MB after the sounds loaded |
| Dawn x60 (1140 × 13, 18 MB) | 16 s (12 s on the second start, 18-20 s on the first) | 1260 MB; 1420 MB with sounds |
| Dawn x150 (2850 × 13, 46 MB) | 44 s | 2000 MB; 2290 MB with sounds (no error) |

- The Beethoven finale is roughly 940 measures on ~25 staves, i.e. between Dawn x60 and x150 in staff-measures, but
  denser (chorus with lyrics, more notes per measure). A phone is a few times slower than this desktop CPU: a load
  of minutes is normal. The first start after a new build runs the app on Chrome's baseline compiler while the
  optimizing one works in the background (`--no-wasm-tier-up` made x60 take 20 s instead of 12-16 s here; on a phone
  the compile threads also compete with the app's thread). So "froze" was most likely a load of several minutes with
  nothing moving on the screen; the spinner now shows the page is alive. The user tested right after the deploy of
  PR #5, i.e. a first start.
- Memory: the threads app's wasm memory is capped at 2 GB (`QT_WASM_MAXIMUM_MEMORY` in `src/app/CMakeLists.txt`);
  Dawn x150 still fit, so the Beethoven should too. An out-of-memory abort in a worker thread (e.g. the audio engine
  thread loading samples) would leave the main thread blocked on a lock: a real freeze. Not seen.
- Emscripten's thread pool: `PTHREAD_POOL_SIZE` 8, `PTHREAD_POOL_SIZE_STRICT` 1 (a 9th thread gets a new worker
  that starts only when the page's thread returns to the event loop). The page never had more than 8 workers in the
  flows tested (start, open, play, home). With 8 cores the mixer pool has 4 threads (`desiredAudioThreadNumber`),
  plus the audio driver's thread, the QML loader thread, `QQuickPixmapReader`, and `QtConcurrent` threads for the
  Scores page's thumbnails (`RecentFilesController::thumbnail`, up to 8 at once with many scores): a 9th thread is
  possible there. Nothing on the main thread waits synchronously for such a thread that could be found (the QML
  loader thread is started at the first QML load, early; `clearComponentCache`, which would restart it, is never
  called).
- Next steps if it happens again: ask what the screen showed (the spinner turning = loading; stopped = a real
  freeze), and whether it was the first start after an update. Get the Beethoven file and measure it with
  `load_check.py --cpu-slowdown 4`. On the phone, Chrome's remote debugging (chrome://inspect over USB) would show the
  console. A memory estimate: ~40-45 KB of wasm memory per staff-measure of Dawn.

### Other findings

- The "Do you want to save changes to the score … before closing?" dialog (three buttons) was wider than a phone
  screen in portrait, with the third button ("Save") off the screen: fixed in
  [m1omg/vibescore#8](https://github.com/m1omg/vibescore/pull/8) (the standard dialogs adapt to the width of a
  single window, `StyledDialogView.availableWidth`; `ButtonBox` stacks its buttons where their row does not fit).
- "Glitchy": the user says the sound did not crackle on the phone when playing normally with the notation open.
  `Module.vibescoreAudio.underrunFrames()` counts missing frames (`devtools/sound_check.py` logs it). During a load
  the engine thread waits for the main thread (file access from threads goes through it), so the counter grows then;
  nothing plays at that time.
- `load_check.py --chrome-arg "--js-flags=--no-liftoff"` is not "optimized code only": functions are then compiled
  lazily on first call, which is counted as busy time (47 s for x60).

## What the sessions did

### First session (2026-10-20)

- [m1omg/vibescore#3](https://github.com/m1omg/vibescore/pull/3): Play shows a pause icon while playing. The "Opening…" box now appears in Firefox before a big score loads.
- [m1omg/vibescore#4](https://github.com/m1omg/vibescore/pull/4): the web app as an installed Home Screen app:
  - manifest, icons, the iPad "Add to Home Screen" hint, and persistent storage when installed
  - page visibility reported to Qt (autosave when hidden)
  - after the system ended a hidden page, the next start reopens the scores that were open
  - Safari/WebKit findings documented: JavaScriptCore's optimizing compiler uses gigabytes on Asyncify code (WebKit bug 304810). This cannot be fixed from the page.
- [m1omg/vibescore#5](https://github.com/m1omg/vibescore/pull/5): no sound on the S24. Fixed and confirmed by the user ("not silent anymore"):
  - The sound font is kept in Cache Storage like the wasm, with a version, stall detection, 3 tries and progress. It is no longer kept in IndexedDB.
  - A status line (`#sound` in `index.html`, `vsShowSound` in `vibescore-web.js`) says "Loading the sounds… N%", "No sound: … Try again", or "Tap here to turn on the sound".
  - The AudioContext also resumes on `pointerup`/`touchend`/`click` (`src/framework/audio/driver/platform/web/webaudiodriver.cpp`).

### Second session (2026-10-20)

- [m1omg/vibescore#7](https://github.com/m1omg/vibescore/pull/7): playback pauses when the score page is left (← arrow, Android Back) and, on the web, when the page
  is hidden; the top bar's play button is disabled while the sounds load; the web "Opening…" box is the page's,
  with a spinner that keeps turning while the app's thread is busy; `devtools/load_check.py` (threads, memory, busy
  time, Chrome's compilers, video), `devtools/serve_isolated.py`, and `hide`/`show` steps in `sound_check.py`.
- [m1omg/vibescore#8](https://github.com/m1omg/vibescore/pull/8): the "save changes?" dialog fits a phone's screen
  (text wrapped, buttons stacked); `load_check.py --device`.

### Third session (2026-10-21)

- **The user's report.** A minuet saved by MuseScore 2.0.2 stayed at "Opening…" for good.
- **Cause.** MuseScore asked about updating the old score (the migration dialog, `openSync`) *during* the load. The page's "Opening…" box from #7 lay over it and took every tap.
- **Fix 1.** The page's box now steps aside for any dialog that waits for an answer, and comes back afterwards: `MobileInteractive::waitForUser`, used by `openSync`, `questionSync` and the other `…Sync` methods.
- **Fix 2.** Phones and tablets no longer ask about migrating. The 590 px desktop dialog is skipped, and the score is updated with the dialog's defaults (`ProjectMigrator::askAboutMigration`).
- **Requested by the user: the file chooser shows MuseScore files first.** The browser's chooser now gets the app's name filters (`browserNameFilter`), with "MuseScore files (*.mscz)" first.
  - Chrome on Android still shows all files: Android does not know `.mscz`.

## Building and testing in a cloud container

- **Toolchain** (about 3.6 GB, under a minute to install), as in `build_web.yml`:
  - Qt 6.10.2 through aqtinstall 3.3.0 (in a venv): `linux_gcc_64` for the host tools, plus `wasm_singlethread` (and `wasm_multithread` for the threads app), with modules `qt5compat` and `qtshadertools`.
  - emsdk 4.0.7: `emsdk install 4.0.7 && emsdk activate 4.0.7`.
  - The host tools need `apt-get install libegl1 libgl1` (qsb), and `LANG=C.UTF-8`.
  - The session's proxy refuses GitHub's archive downloads (`codeload.github.com`), which Emscripten's zlib port
    needs (`-s USE_ZLIB=1`, from Qt): `git clone --branch v1.3.1 https://github.com/madler/zlib.git` into
    `$(em-config CACHE)/ports/zlib/zlib-1.3.1` (without `.git`), and write the port's URL
    (`https://github.com/madler/zlib/archive/refs/tags/v1.3.1.tar.gz`) into `$(em-config CACHE)/ports/zlib/.emscripten_url`.
  - Configure with `-DCMAKE_AUTORCC_OPTIONS=--no-zstd`: the host `rcc` here compressed KDDockWidgets' AUTORCC
    resources with zstd, which the wasm Qt lacks (`undefined symbol: qResourceFeatureZstd()` at the link). CI does
    not hit this (its log compiles the same files; why was not found).
- **Configure and build:**
  - Source `emsdk_env.sh`.
  - Run `<qt>/wasm_singlethread/bin/qt-cmake -S . -B build-web -G Ninja -DMUSESCORE_BUILD_CONFIGURATION=app-web -DVS_WEB_THREADS=OFF -DCMAKE_BUILD_TYPE=Release -DQT_HOST_PATH=<qt>/gcc_64`.
  - Then run `cmake --build build-web`. The site is in `build-web/webapp`, and the build folder takes about 1 GB.
  - A full first build takes long (about 50 minutes on 4 cores, 10 of them the final link). Rebuilding after a change to a few files is quick.
  - Changes to only `buildscripts/web/*.js`/`index.html` need no compile:
    1. Copy `buildscripts/web/index.html` and the changed scripts into a copy of `build-web/webapp`.
    2. Run `buildscripts/web/stamp_page.py <that folder>`, as `SetupWebPackaging.cmake` does.
- **Serving locally.** Serve the folder with `python3 -m http.server -d <folder> <port>`. `serve.sh` expects the `~/osm` layout of a local machine. A build with threads alone (its `webapp` folder has no `threads/`) needs the COOP/COEP headers: `buildscripts/web/devtools/serve_isolated.py <folder> <port>`.
- **Chromium (phone):**
  - Playwright's Chromium is preinstalled (`/opt/pw-browsers`). Do not run `playwright install`.
  - `pip install playwright==1.56.0` in a venv, then run under `xvfb-run -a`.
  - `buildscripts/web/devtools/sound_check.py` emulates a Pixel 7 with Android's autoplay rule. It measures what the page plays with an AnalyserNode (RMS), along with the AudioContext state, whether the sound font is loaded, and the status line. The usage and an example are at the top of the file.
  - In the phone layout, "Open other" on the Scores page is at (330,738), Play at (216,26) and the ← arrow at (24,26).
  - `buildscripts/web/devtools/load_check.py` opens a big score and logs the workers (threads), the memory, the busy time of the page's thread and the console, with the number of cores of a phone, Chrome's compilers, a second start, and a video (usage at the top of the file).
- **Firefox:**
  - `buildscripts/web/devtools/firefox_sound.cjs` needs `npm i puppeteer-core` and a Firefox release (the tarball from Mozilla).
  - The container has no sound device. Install PulseAudio and run `pulseaudio --start --exit-idle-time=-1`, or Firefox's AudioContext never runs.
  - In Puppeteer the space key is `' '`, not `'Space'`.
- **Pitfalls:**
  - Playwright's `page.evaluate` gives the page a user activation, so it unlocks audio by itself.
  - `pkill -f pattern` can kill your own shell. Anchor the pattern (`'^python3 ...'`) or use PIDs.
  - Outbound HTTPS goes through the session's proxy (`HTTPS_PROXY`). Playwright gets `proxy={"server": HTTPS_PROXY}`. Never turn off TLS verification.
  - Opening an old-format score shows a migration dialog first.
- **Checking a deploy.**
  - GitHub Pages serves pages with `max-age=600`, so phones may keep the old page for up to 10 minutes.
  - The page lists each file's version in `VS_VERSIONS`. Grep the live `index.html` for it, or for new code, to see when the deploy is live.
