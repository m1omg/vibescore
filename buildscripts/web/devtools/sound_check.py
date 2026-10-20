#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Does the web app make sound in a phone-like Chromium? Android's autoplay rule (audio only after a user gesture),
a phone (touch, mobile user agent), the live site through the proxy. An init script taps the player's output with an
AnalyserNode: the loudness (RMS) of what the browser plays is measured, with the AudioContext's state and whether
the sound font is there.

Usage: sound_check.py URL OUT_DIR STEP...
Steps: start, shot:NAME, tap:X,Y, open:X,Y=FILE (tap the "Open other" button at X,Y and give FILE), wait:S,
       waitsf:S (until the sound font is in the app, at most S seconds), measure:S (log every 0.5 s for S seconds),
       throttle:KBYTES_PER_S, offline, online, tapline (tap the page's sound line)
Environment: VS_STRICT_AUTOPLAY=1 (an AudioContext made before a user gesture stays suspended, as on Android),
             VS_LOCKED_AUDIO=1 (the browser never plays audio: the sound line must ask for a tap)

Example (phone layout: "Open other" at 330,738, Play at 216,26; needs a display, e.g. xvfb-run -a):
    sound_check.py https://m1omg.github.io/vibescore/ out start waitsf:240 open:330,738=demos/Dawn.mscx \\
        wait:15 tap:216,26 measure:4 shot:playing
"""
import os
import sys
import time

from playwright.sync_api import sync_playwright

url, out = sys.argv[1:3]
steps = sys.argv[3:]
os.makedirs(out, exist_ok=True)
t0 = time.time()


def note(text):
    print(f"{time.time() - t0:6.1f}s {text}", flush=True)


TAP_OUTPUT = """
(() => {
    const connect = AudioNode.prototype.connect;
    window.__vsAnalyser = null;
    window.__vsMaxRms = 0;
    AudioNode.prototype.connect = function (destination, ...rest) {
        const result = connect.call(this, destination, ...rest);
        if (destination instanceof AudioDestinationNode && !window.__vsAnalyser) {
            const analyser = this.context.createAnalyser();
            analyser.fftSize = 2048;
            connect.call(this, analyser);
            window.__vsAnalyser = analyser;
            const data = new Float32Array(analyser.fftSize);
            setInterval(() => {
                analyser.getFloatTimeDomainData(data);
                let sum = 0;
                for (const v of data) sum += v * v;
                window.__vsMaxRms = Math.max(window.__vsMaxRms, Math.sqrt(sum / data.length));
            }, 50);
        }
        return result;
    };
})();
"""

# Chrome on Android: an AudioContext made before the user touched the page is suspended, and resume() works only
# after the page had a user activation (this Chromium does not apply the rule itself)
STRICT_AUTOPLAY = """
(() => {
    const Native = window.AudioContext;
    window.AudioContext = class extends Native {
        constructor(...args) {
            super(...args);
            if (!navigator.userActivation.hasBeenActive) {
                Native.prototype.suspend.call(this);
            }
        }
        resume() {
            if (!navigator.userActivation.hasBeenActive) {
                return Promise.reject(new DOMException('no user activation yet', 'NotAllowedError'));
            }
            return Native.prototype.resume.call(this);
        }
    };
})();
"""

# The browser does not let the page play audio, whatever it does (resume() does nothing): counts the tries
LOCKED_AUDIO = """
(() => {
    const Native = window.AudioContext;
    window.__vsResumeTries = 0;
    window.AudioContext = class extends Native {
        constructor(...args) {
            super(...args);
            Native.prototype.suspend.call(this);
        }
        resume() {
            window.__vsResumeTries += 1;
            return Promise.resolve();
        }
    };
})();
"""

STATE = """() => {
    const i = window.vibescoreInstance;
    const audio = i?.vibescoreAudio;
    const rms = window.__vsMaxRms;
    window.__vsMaxRms = 0;
    let sf = null;
    try { sf = i ? i.FS.analyzePath('/files/share/sound/MS Basic.sf3').exists : null; } catch (e) {}
    return {
        ctx: audio ? audio.ctx.state : 'none',
        rms: rms,
        soundFont: sf,
        underruns: audio?.underrunFrames ? audio.underrunFrames() : null,
        app: (window.vibescoreTimings || {}).app || null,
        banner: document.querySelector('#banner')?.style.display === 'flex'
            ? document.querySelector('#banner span').textContent : '',
        line: document.querySelector('#sound')?.style.display === 'flex'
            ? document.querySelector('#sound').textContent.trim() : '',
    };
}"""

with sync_playwright() as p:
    device = dict(p.devices["Pixel 7"])
    proxy = {"server": os.environ["HTTPS_PROXY"]} if url.startswith("https:") else None
    context = p.chromium.launch_persistent_context(
        os.path.join(out, "profile"), headless=False, proxy=proxy,
        args=["--autoplay-policy=user-gesture-required", "--enable-unsafe-swiftshader", "--ignore-gpu-blocklist"],
        **{k: v for k, v in device.items() if k != "default_browser_type"})
    context.add_init_script("Object.defineProperty(window, 'showOpenFilePicker', { value: undefined });")
    if os.environ.get("VS_LOCKED_AUDIO"):
        context.add_init_script(LOCKED_AUDIO)
    if os.environ.get("VS_STRICT_AUTOPLAY"):
        context.add_init_script(STRICT_AUTOPLAY)
    context.add_init_script(TAP_OUTPUT)
    page = context.pages[0] if context.pages else context.new_page()
    logs = open(os.path.join(out, "console.log"), "a")
    page.on("console", lambda m: logs.write(f"{time.time() - t0:7.2f}s {m.text}\n"))
    cdp = context.new_cdp_session(page)

    for step in steps:
        kind, _, arg = step.partition(":")
        if kind == "start":
            page.goto(url, wait_until="commit")
            while True:
                time.sleep(1)
                try:
                    if page.evaluate("document.querySelector('#loading')?.style.display === 'none'"):
                        break
                except Exception:
                    pass
            time.sleep(3)
            note("started: " + str(page.evaluate(STATE)))
        elif kind == "shot":
            page.screenshot(path=os.path.join(out, arg + ".png"))
            note("shot " + arg)
        elif kind == "tap":
            x, y = (float(v) for v in arg.split(","))
            page.touchscreen.tap(x, y)
            note(f"tap {x:g},{y:g}")
        elif kind == "open":
            position, _, path = arg.partition("=")
            x, y = (float(v) for v in position.split(","))
            with page.expect_file_chooser() as chooser:
                page.touchscreen.tap(x, y)
            chooser.value.set_files(path)
            note("open " + os.path.basename(path))
        elif kind == "tapline":
            box = page.locator("#sound").bounding_box()
            page.touchscreen.tap(box["x"] + box["width"] / 2, box["y"] + box["height"] / 2)
            note("tap on the sound line; resume tries: " + str(page.evaluate("window.__vsResumeTries")))
        elif kind == "wait":
            time.sleep(float(arg))
        elif kind == "waitsf":
            end = time.time() + float(arg)
            while time.time() < end and not page.evaluate(STATE)["soundFont"]:
                time.sleep(1)
            note("sound font: " + str(page.evaluate(STATE)))
        elif kind == "measure":
            end = time.time() + float(arg)
            while time.time() < end:
                time.sleep(0.5)
                note(str(page.evaluate(STATE)))
        elif kind == "throttle":
            rate = float(arg) * 1000
            cdp.send("Network.enable")
            cdp.send("Network.emulateNetworkConditions", {"offline": False, "latency": 50,
                                                          "downloadThroughput": rate, "uploadThroughput": rate})
            note(f"throttled to {arg} kB/s")
        elif kind == "offline":
            context.set_offline(True)
            note("offline")
        elif kind == "online":
            context.set_offline(False)
            note("online")
    time.sleep(1)
    context.close()
