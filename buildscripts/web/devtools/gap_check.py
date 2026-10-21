#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Audible gaps in playback, in a phone-like Chromium (Pixel 7 with 8 cores like a Galaxy S24, the CPU slowed down):
starts the app (a fresh profile: the first start, as after an update; --second-start: the start after it), opens a
score, taps Play as soon as the play button is enabled (it is greyed out while the instruments' sounds load), then
records the loudness of what the page plays every 20 ms (an AnalyserNode on the player) and the player's underrun
counter (the threads app). A gap is a 20 ms window of silence after one of sound.
Usage: gap_check.py URL OUT_DIR SCORE [--slowdown N] [--seconds S] [--second-start]
Needs Pillow (the play button's state is read from a screenshot) and a display (xvfb-run -a).
Example: gap_check.py https://m1omg.github.io/vibescore/ out demos/Dawn.mscx --slowdown 4 --seconds 30
"""
import io
import json
import os
import sys
import time

from PIL import Image
from playwright.sync_api import sync_playwright

url, out, score = sys.argv[1:4]
args = sys.argv[4:]
slowdown = float(args[args.index("--slowdown") + 1]) if "--slowdown" in args else 4
seconds = float(args[args.index("--seconds") + 1]) if "--seconds" in args else 30
second_start = "--second-start" in args
os.makedirs(out, exist_ok=True)

TAP_OUTPUT = """
(() => {
    Object.defineProperty(navigator, 'hardwareConcurrency', { value: 8 });
    const connect = AudioNode.prototype.connect;
    window.__vsLevels = [];
    AudioNode.prototype.connect = function (destination, ...rest) {
        const result = connect.call(this, destination, ...rest);
        if (destination instanceof AudioDestinationNode && !window.__vsAnalyser) {
            const analyser = this.context.createAnalyser();
            analyser.fftSize = 1024; // ~21 ms at 48 kHz
            connect.call(this, analyser);
            window.__vsAnalyser = analyser;
            const data = new Float32Array(analyser.fftSize);
            const take = () => {
                analyser.getFloatTimeDomainData(data);
                let sum = 0;
                for (const v of data) sum += v * v;
                const audio = window.vibescoreInstance?.vibescoreAudio;
                window.__vsLevels.push([performance.now(), Math.sqrt(sum / data.length),
                                        audio?.underrunFrames ? audio.underrunFrames() : null]);
            };
            setInterval(take, 20);
        }
        return result;
    };
})();
"""


def wait_started(page):
    while True:
        time.sleep(1)
        try:
            if page.evaluate("document.querySelector('#loading')?.style.display === 'none'"):
                return
        except Exception:
            pass


with sync_playwright() as p:
    device = dict(p.devices["Pixel 7"])
    proxy = {"server": os.environ["HTTPS_PROXY"]} if url.startswith("https:") else None
    context = p.chromium.launch_persistent_context(
        os.path.join(out, "profile"), headless=False, proxy=proxy,
        args=["--autoplay-policy=user-gesture-required", "--enable-unsafe-swiftshader", "--ignore-gpu-blocklist"],
        **{k: v for k, v in device.items() if k != "default_browser_type"})
    context.add_init_script("Object.defineProperty(window, 'showOpenFilePicker', { value: undefined });")
    context.add_init_script(TAP_OUTPUT)
    page = context.pages[0] if context.pages else context.new_page()
    cdp = context.new_cdp_session(page)
    cdp.send("Emulation.setCPUThrottlingRate", {"rate": slowdown})
    t0 = time.time()
    page.goto(url, wait_until="commit")
    wait_started(page)
    if second_start:
        time.sleep(20)
        page.reload(wait_until="commit")
        cdp = context.new_cdp_session(page)
        cdp.send("Emulation.setCPUThrottlingRate", {"rate": slowdown})
        wait_started(page)
    print(f"{time.time() - t0:6.1f}s started", flush=True)
    time.sleep(2)
    with page.expect_file_chooser() as chooser:
        page.touchscreen.tap(330, 738)
    chooser.value.set_files(score)
    # Until the score is shown: the page's thread is busy meanwhile
    time.sleep(3)
    page.evaluate("1")
    print(f"{time.time() - t0:6.1f}s opened", flush=True)
    page.evaluate("window.__vsLevels = []")
    # Play is allowed once the instruments' sounds are loaded: the play button (phone layout) is greyed out until then
    def play_darkness():
        png = page.screenshot(clip={"x": 206, "y": 16, "width": 20, "height": 20})
        return Image.open(io.BytesIO(png)).convert("L").getextrema()[0]

    for i in range(600):
        darkness = play_darkness()
        if darkness < 80:
            break
        time.sleep(0.25)
    print(f"{time.time() - t0:6.1f}s play enabled (icon {darkness})", flush=True)
    page.touchscreen.tap(216, 26)
    print(f"{time.time() - t0:6.1f}s tapped Play", flush=True)
    page.evaluate("window.__vsLevels = []")
    time.sleep(seconds)
    levels = page.evaluate("window.__vsLevels")
    page.touchscreen.tap(216, 26)
    context.close()

json.dump(levels, open(os.path.join(out, "levels.json"), "w"))
sound = [level > 1e-4 for _, level, _ in levels]
gaps = sum(1 for i in range(1, len(sound)) if sound[i - 1] and not sound[i])
silent = sum(1 for s in sound if not s)
under = [u for _, _, u in levels if u is not None]
print(f"windows: {len(sound)}, silent: {silent} ({100 * silent / max(len(sound), 1):.1f}%), gaps begun: {gaps}, "
      f"underrun frames during playback: {under[-1] - under[0] if under else None}")
# Silence per 5 s
per = []
for k in range(0, len(sound), 250):
    chunk = sound[k:k + 250]
    per.append(f"{100 * chunk.count(False) / len(chunk):.0f}%")
print("silent per 5 s:", " ".join(per))
