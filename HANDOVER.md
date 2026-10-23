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

## 2026-10-23: the app was restored to the build of 2026-10-20 15:53 (`1a62de05`), at the user's request

**Why.** On the user's Xiaomi Pad 5 (Chrome), opening any score stays at "Opening…" for good, in the app with threads
and in the one without (`?threads=0`), also an hour after the first start. The Galaxy S24 is fine. The user said that
the Pad 5 worked before "the dialogue fix" and asked to go back to the version before it; of the three versions that
were offered (before #11 / before #8 / only the Oct 22 changes) they chose **before #8**.

**What was not found.** The failure was not reproduced here, in Chromium on Linux (software GL), with: a phone profile,
a tablet profile (1138×712 and 1463×914, touch, mobile and desktop user agent), a desktop profile with a mouse, the
app with and without threads, first start / later start / update from the previous deploy, a 2 MB/s network, 4 cores
with a 5× slower CPU, scores in the formats MuseScore 2.0.2, 3.0, 4.2, 4.6 and 4.7 (27 to 34 KB), opening a second
score (the "save changes?" question), a score from the library, a session restored after the system ended the page,
and Firefox 158. The tools for that are in `buildscripts/web/devtools` (`load_check.py`, with `--device`; the others
of that run were in a scratchpad: a mouse variant, a tablet variant that logs the page's `#busy` box every 2 s, a page
crash/restore variant). CI's "Test: Web start" never opens a score, so nothing in CI would have caught it.

**What the restore is.** Every file outside `HANDOVER.md`, `docs/` and `buildscripts/web/devtools/` is as it was at
`1a62de05` (check: `git diff 1a62de05 -- . ':!HANDOVER.md' ':!docs' ':!buildscripts/web/devtools'` is empty). The page
(`index.html`, `vibescore-web.js`, the service worker) did not change in these days, so only the app changed. The
restore is **one commit**: `git revert` of it brings everything below back at once.

**What is gone, and what it costs** (each change can be put back alone with `git checkout <commit> -- <files>`; the
Pad 5 should be tried after each one, in this order, to find the one that breaks it):

1. `b27e48fa` (#8): the standard dialogs adapt to the width of a single window (`StyledDialogView.availableWidth`,
   `InWindowHost.qml`, `ButtonBox.qml` stacks its buttons, `StandardDialog*.qml`). Without it the "Do you want to save
   changes…" dialog is wider than a phone in portrait, and its third button ("Save") is off the screen.
2. `a29ece18` (#11): the page's "Opening…" box steps aside for dialogs that wait for an answer
   (`MobileInteractive::waitForUser`, `m_busyTitle`), the migration question fits a phone (`MigrationDialog.qml`, the two
   `MigrationContentFor*.qml`), and the browser's file chooser gets the app's name filters (`browserNameFilter`). Without
   it a score saved by MuseScore 2 or 3 (the user's minuet) asks about updating its engraving under the page's box and
   stays at "Opening…" for good. Then `7c9ece88`: the example picture's box of the migration dialog.
3. `a52f5111` (#13): the queue of Qt's pending browser events keeps target, current/related target, path and offset on
   each event (`vs_guard_pending_events` in `webplatform.cpp`). Without it, answering the migration question with a
   mouse on a computer freezes the app (Qt 6.10 reads the target of a queued pointer event late, it is null in the
   shadow DOM, `dom::mapPoint` throws).
4. `c07792d2`, `c0d978bd` (#13): the audio kept ready by the app with threads grows when the player runs out while music
   plays (`webaudiodriver.cpp/.h`, `vibescoreAudio.stats()`, `?audioahead=`). Without it a weak or busy device (the
   user's 2017 MacBook Air) may stutter a bit.

Item 4 is compiled only into the app with threads (`MUSE_THREADS_SUPPORT`), and the Pad 5 fails in the app without
threads as well, so the cause is in 1, 2 or 3. Which one: not known. Of those, only 2 and 3 run during a plain load
without any dialog (2: the file chooser's filter and `showProgress`; 3: every queued browser event), so look at 2 and 3
first if the restore turns out to fix the Pad 5 and 1 is cleared.

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
  - Cause: MuseScore asks whether to update an old score (the migration dialog, `openSync`) *during* the load, and the page's "Opening…" box from #7 lay over it and took every tap.
- **First fix, reverted.** [m1omg/vibescore#9](https://github.com/m1omg/vibescore/pull/9) updated old scores without asking on mobile. The user then reported slow loading and stuttering sound, and asked to undo it at once: [m1omg/vibescore#10](https://github.com/m1omg/vibescore/pull/10).
  - The revert's build was byte-identical to the build before #9: the builds are reproducible, and the wasm versions are the same.
- **Measured afterwards.** The CI artifacts of both builds, served locally, under the same phone-like conditions (`load_check.py --cores 8 --cpu-slowdown 4`):
  - first start: 26.2 s for both;
  - opening Dawn: 11.4 s vs 12.4 s on the first start, 7.4 s for both on the second start;
  - audio: 0 missing frames during 15 s of playback in both.
  - So #9 was not slower. What the user saw was most likely the first start after an update, which **every deploy** costs a phone: Chrome downloads the ~124 MB threads wasm again, and runs it on its baseline compiler until the optimizing compiler is done, which takes minutes on a phone (slower loading, and possibly stuttering sound meanwhile).
  - Keep deploys few, and judge speed on the second start after one.
- **Second fix**, as the user chose:
  - The migration question stays and fits a phone (`MigrationDialog` uses `availableWidth`, and its texts wrap).
  - The page's "Opening…" box steps aside for any dialog that waits for an answer (`MobileInteractive::waitForUser`).
  - The browser's file chooser shows MuseScore files first (`browserNameFilter`).
  - Merged as [m1omg/vibescore#11](https://github.com/m1omg/vibescore/pull/11). Starting "Build: Web" on a branch (`workflow_dispatch`) is refused to this integration (403), so the threads build could only be measured after the merge, from its CI artifacts. It starts, opens and plays like the previous one: first-start opening of Dawn took 13.7 s, 11.5 s and 14.2 s vs 14.3 s, 11.4 s and 13.4 s; there were 0 missing frames during playback.
- **"Playback stuttered like Quake on a Cyrix 486"** after #9's deploy, on the user's Xiaomi Pad 5 (Snapdragon 860, Android, Chrome; not the S24). This was not reproduced here (also not in a 1280×800, DPR 2 tablet emulation: the tablet layout's slow frames there were the container's software GPU; the page's thread was ~73% idle while playing).
  - `devtools/gap_check.py` taps Play as soon as the button is enabled after a fresh first start, and records the output's loudness every 20 ms. It found 0 gaps and 0 missing frames in either build (4× CPU slowdown, 8 cores).
  - Underruns that `load_check.py` counts right after a score opens are the instruments' sounds loading, while Play is still disabled.
  - Most likely cause: a phone's first start after an update. Chrome runs the new wasm on its baseline compiler while it optimizes in the background.
  - A suspect for a real phone: the threads player keeps only ~64 ms of audio ready (`AHEAD_FRAMES` in `webaudiodriver.cpp`), so any hiccup of the render thread longer than that is a gap. If the user still hears stutter on the second start after an update, a reserve that grows after underruns (with the cursor's lead in mind) is the next step.

### Third session, continued (2026-10-22): MacBook Air 2017 in Chrome

- **The user's report.** On a MacBook Air 2017 (i5-5350U, 2 cores / 4 threads, Chrome): opening a score takes long, and the sound "still stutters a bit" even after the app loaded and sat idle.
- **Emulation.** `mac_check`-style runs in the scratchpad (not kept): 1440×900 at DPR 1, the second start, Dawn.
  - The page's renderer and Chrome's audio service were pinned to 2 cores; also everything on 2 cores, also with two busy processes on those cores.
  - Playback: 0 missing frames and 0 gaps in 60 s in every case. The audio threads used ~14% of one core. So the stutter was not reproduced.
  - On a Mac, Chrome's `'playback'` output buffer is ~20 ms (1024 frames: `AudioLatency::GetHighLatencyBufferSize`), so it is not larger than the reserve.
  - The user was asked to run a console check on the MacBook: `underrunFrames()` over 30 s while playing. With the new diagnostics, `vibescoreInstance.vibescoreAudio.stats()` tells more.
- **Opening Dawn the first time, in the box** (temporary `VSPERF` timers in a local build):
  - read 0.22 s, layout 0.75 s, `PlaybackModel::load` 0.38 s
  - creating the score page 0.73 s, **the palettes panel 0.76 s**
  - then 2.6 s until the sounds are loaded and Play is enabled
  - A small score (the minuet) needs ~0.3 s for the score itself, and ~1.1 s for the page and the palettes.
- **Found: a freeze on computers.** After the migration question was answered with the mouse in the desktop layout, the app froze: no more redraws or clicks. The engine still loaded the sounds. The live builds #11/#12 have it.
  - Cause: a Qt 6.10 wasm bug. Queued browser events are handled after delivery. In Qt's shadow DOM their `target` is then null, `dom::mapPoint` throws ("Cannot read properties of null (reading 'getBoundingClientRect')"), and the event processing stops.
  - Touch doesn't take that path, so phones and tablets were fine.
  - Found with a build linked with `--profiling-funcs` (function names in the stack) and Playwright's `pageerror`.
- **Changes:**
  - **Freeze:** `vs_guard_pending_events` keeps target, current and related target, `composedPath()` and a mouse event's offset on each event when it is queued.
  - **Palettes, tried and dropped:** the side sheet's panels loaded asynchronously (a `Loader` with `asynchronous`). The panels then appeared complete within 0.15 s of a click, but opening was not faster: Dawn's box 3.5–4.4 s against 3.7–5.0 s (2 cores for the page), ~5.8 s against 5.5–5.9 s (everything on 2 cores); a small score 2.7–2.9 s against 2.6–3.3 s. Why was not checked. A guess: the panel's costly part is one piece of work that the incubator cannot split, which then runs right after the box went and before the browser draws. A performance profile of that moment is the next step if opening is still too slow.
  - **Audio reserve:** `browser output buffer (rounded up to 512) + 3072` frames, plus 1024 each time a full ring ran empty while music played (up to 8192; it never shrinks); the ring is 16384 frames. `vibescoreAudio.stats()/resetStats()` and `?audioahead=FRAMES` (1024 to 8192) for diagnosis.
  - **Not done:** serial mixing on ≤4 cores. It would have been a risk for cheap quad-core phones, and it did not help in the emulation.
- **Review.** A separate review agent read the diff. It confirmed two defects, both fixed. One hiccup grew the reserve two or three times, because the player kept running short while the ring was being refilled; the counter is now taken again after the refill. And `?audioahead` below two blocks guaranteed underruns. Its other points were risks it could not confirm, or nits.
- **Measured after the changes** (a local build with threads, `crossOriginIsolated`, 2 cores, Dawn): 0 underruns in 60 s, the least the ring held while playing 70 ms, the reserve stayed at its start (93 ms here: Chrome on Linux has a 1024-frame buffer too). The old minuet with its question answered by mouse opens and the app goes on answering.
  - `stats().underrunMs` counts from the start, including the silence while a score's sounds load (seconds; `longestBlockMs` is then the loading, 2–4 s). For the playback alone: `resetStats()`, play, then `stats()`.
- **Local build with threads works in the container:** `qt-cmake` from `toolchain/qt/6.10.2/wasm_multithread` with `-DVS_WEB_THREADS=ON` (the other options as for the single-thread build), ~22 min from scratch on 4 cores; `add_threads_app.py` puts it in a single-thread site's `threads/`.

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
