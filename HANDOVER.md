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

## Open issues (the user is "still not satisfied")

The user's words: "It's not silent anymore, but it's kind of glitchy in that one time it didn't even load the Beethoven song. And then the other, when I backed out of it, it still continued playing."

### 1. Leaving a score keeps it playing

"Backed out" can mean three things. Ask which one, or fix all three:

1. **The arrow at the top left of the score.** `MobileNotationPage.qml`, `homeButton`, calls `api.launcher.open("musescore://home")`.
   - It only shows the Scores page. The score stays open (as a tab), and so does its playback.
   - Android's Back key in the native app does the same: `BackNavigationController::goBack()` in `src/mobile/internal/backnavigationcontroller.cpp` ends by opening `musescore://home`.
   - Fix direction: stop (or pause) playback whenever the notation page is left. Do it in one place both paths go through, e.g. when the interactive's current URI changes away from `musescore://notation`. The native apps share this code, so they get the same fix.
2. **The phone's Back gesture in Chrome.** The page has no `history`/`popstate` handling. Back therefore leaves the page, or closes the tab and puts Chrome in the background.
3. **Switching to another app, or the phone's home screen, while the page plays.**
   - `MobileLifecycleController::onApplicationStateChanged` (`src/mobile/internal/mobilelifecyclecontroller.cpp`) pauses playback only on `Qt::ApplicationSuspended`.
   - The web app reports a hidden page as `Qt::ApplicationInactive`: see `web::watchPageVisibility()` in `src/mobile/internal/platform/web/webplatform.cpp`, which is called from `mobilemodule.cpp`. So **on the web, playback never pauses when the page is hidden**.
   - Chrome on Android keeps Web Audio playing in a background tab.
   - Probably also pause there when the page is hidden. Check that the autosave on that state change (`ProjectAutoSaver`) still works.

The play button's icon follows `playbackController()->isPlayingChanged()` (`mobileshellview.cpp`), so it will stay right.

### 2. "One time it didn't even load the Beethoven"

- **The score.** It is the user's own file, "Beethoven_Op.125_4" (Symphony No. 9, finale): a big orchestral score, not in the repo. Earlier it took long to open in Firefox. The fix then was to draw the "Opening…" box two frames before loading (`MobileInteractive::drawWindowsNow`, `src/mobile/internal/mobileinteractive.cpp`).
- **Still unknown.** Ask the user:
  - What did they see? The page starting over from the loading screen (the tab was killed, likely for memory), an "Opening…" box that stayed, an error, or nothing at all?
  - How often does it happen?
  - Can they share the file?
- **Facts to start from:**
  - In desktop Chromium the start peaks at about 650 MB. `smoke_test.py` fails above 1.4 GB.
  - The sound font (`share/sound/MS Basic.sf3`, 51 MB) is now in memory once. It is written into MEMFS with `canOwn`.
  - When Chrome kills a hidden tab, the next start reopens the scores that were open. That happens only if the page ended in the background:
    - `vibescoreEndedInBackground` in `buildscripts/web/vibescore-web.js`
    - `StartupScenario` in `src/appshell/internal/startupscenario.cpp`
  - A tab killed while on screen starts on the Scores page.
- **A stand-in score.** `buildscripts/web/devtools/multiply_score.py demos/Dawn.mscx /tmp/Dawn_x30.mscx 30` makes 570 measures on 13 staves (9 MB).
- **Next steps.** Open such a score in the phone profile (below) and measure memory and time. For memory, use the RSS of the page process, as `smoke_test.py`'s `page_process_memory_mb()` reads it from `/proc`.

### 3. "Glitchy"

Ask whether the user also heard crackles or dropouts during playback. In the threads app, `Module.vibescoreAudio.underrunFrames()` counts missing audio frames. `devtools/sound_check.py` logs it. It stayed constant while playing in the phone profile in Chromium.

## What the last session did

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

## Building and testing in a cloud container

- **Toolchain** (about 3 GB), as in `build_web.yml`:
  - Qt 6.10.2 through aqtinstall 3.3.0 (in a venv): `linux_gcc_64` for the host tools, plus `wasm_singlethread` (and `wasm_multithread` for the threads app), with modules `qt5compat` and `qtshadertools`.
  - emsdk 4.0.7: `emsdk install 4.0.7 && emsdk activate 4.0.7`.
- **Configure and build:**
  - Source `emsdk_env.sh`.
  - Run `<qt>/wasm_singlethread/bin/qt-cmake -S . -B build-web -G Ninja -DMUSESCORE_BUILD_CONFIGURATION=app-web -DVS_WEB_THREADS=OFF -DCMAKE_BUILD_TYPE=Release -DQT_HOST_PATH=<qt>/gcc_64`.
  - Then run `cmake --build build-web`. The site is in `build-web/webapp`, and the build folder takes about 1 GB.
  - A full first build takes long. Rebuilding after a change to a few files is quick.
  - Changes to only `buildscripts/web/*.js`/`index.html` need no compile:
    1. Copy `buildscripts/web/index.html` and the changed scripts into a copy of `build-web/webapp`.
    2. Run `buildscripts/web/stamp_page.py <that folder>`, as `SetupWebPackaging.cmake` does.
- **Serving locally.** Serve the folder with `python3 -m http.server -d <folder> <port>`. `serve.sh` expects the `~/osm` layout of a local machine.
- **Chromium (phone):**
  - Playwright's Chromium is preinstalled (`/opt/pw-browsers`). Do not run `playwright install`.
  - `pip install playwright==1.56.0` in a venv, then run under `xvfb-run -a`.
  - `buildscripts/web/devtools/sound_check.py` emulates a Pixel 7 with Android's autoplay rule. It measures what the page plays with an AnalyserNode (RMS), along with the AudioContext state, whether the sound font is loaded, and the status line. The usage and an example are at the top of the file.
  - In the phone layout, "Open other" on the Scores page is at (330,738) and Play at (216,26).
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
