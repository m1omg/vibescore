#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""How the web app takes a big score in a phone-like Chromium (Pixel 7, with the number of CPU cores of a phone):
opens scores and logs the page's workers (Emscripten's threads; a 9th would mean the thread pool of 8 ran out), the
memory of the page's process, how long the page's thread was busy (the longest gap between ticks of a 100 ms timer,
and their sum since the last mark), and the console (errors, a thread pool exhausted, an abort).

Usage: load_check.py URL OUT_DIR [--cores N] [--cpu-slowdown N] [--chrome-arg ARG]... [--video 1] STEP...
Steps: start, reload (the page again, in the same browser profile: the second start), mark (the busy time from
       here), open:X,Y=FILE (tap the "Open other" button at X,Y and give FILE), waitload:S (until the page's thread
       was busy and is idle again, at most S seconds; a dialog that waits, e.g. "save changes?", looks loaded too),
       tap:X,Y, wait:S, waitidle:S, shot:NAME, state
Writes to OUT_DIR: run.log, console.log, the screenshots, and with --video 1 a recording (video/*.webm), e.g. to see
that the busy dialog's spinner keeps turning while the app's thread is busy.
Phone layout: "Open other" at 330,738, Play at 216,26, the Home arrow at 24,26. A big score: devtools/multiply_score.py.
Example: load_check.py https://m1omg.github.io/vibescore/ out --cores 8 start mark open:330,738=/tmp/Dawn_x60.mscx \
         waitload:600 state
Chrome's compilers (the first start after a new build runs the app on the baseline compiler while the optimizing one
works in the background): --chrome-arg "--js-flags=--no-wasm-tier-up --no-wasm-dynamic-tiering" (baseline only),
--chrome-arg "--js-flags=--no-liftoff" (optimized only); start, then reload, compares the first and the second start.
"""
import os
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from smoke_test import page_process_memory_mb  # noqa: E402

from playwright.sync_api import sync_playwright  # noqa: E402

args = sys.argv[1:]
url, out = args[0], args[1]
steps = args[2:]
cores = 8
slowdown = 1
chrome_args = []
video = False
while steps and steps[0].startswith("--"):
    if steps[0] == "--cores":
        cores = int(steps[1])
    elif steps[0] == "--cpu-slowdown":
        slowdown = float(steps[1])
    elif steps[0] == "--chrome-arg":
        chrome_args.append(steps[1])
    elif steps[0] == "--video":
        video = steps[1] == "1"
    steps = steps[2:]
os.makedirs(out, exist_ok=True)
t0 = time.time()
log = open(os.path.join(out, "run.log"), "a")


def note(text):
    line = f"{time.time() - t0:6.1f}s {text}"
    print(line, flush=True)
    log.write(line + "\n")
    log.flush()


STATE = """() => {
    const i = window.vibescoreInstance;
    const audio = i?.vibescoreAudio;
    return {
        ctx: audio ? audio.ctx.state : 'none',
        underruns: audio?.underrunFrames ? audio.underrunFrames() : null,
        hc: navigator.hardwareConcurrency,
        loading: document.querySelector('#loading')?.style.display,
        status: document.querySelector('#status')?.textContent,
        line: document.querySelector('#sound')?.style.display === 'flex'
            ? document.querySelector('#sound').textContent.trim() : '',
        busyMs: Math.round(window.__vsProbe.busyMs), longestMs: Math.round(window.__vsProbe.longestMs),
    };
}"""


def state(page):
    try:
        s = page.evaluate(STATE)
    except Exception as e:
        s = f"(no answer: {str(e).splitlines()[0]})"
    return f"workers={len(page.workers)} mem={page_process_memory_mb()}MB {s}"


with sync_playwright() as p:
    device = dict(p.devices["Pixel 7"])
    proxy = {"server": os.environ["HTTPS_PROXY"]} if url.startswith("https:") else None
    context = p.chromium.launch_persistent_context(
        os.path.join(out, "profile"), headless=False, proxy=proxy,
        args=["--autoplay-policy=no-user-gesture-required", "--enable-unsafe-swiftshader", "--ignore-gpu-blocklist"] + chrome_args,
        **{k: v for k, v in device.items() if k != "default_browser_type"},
        **({"record_video_dir": os.path.join(out, "video"), "record_video_size": device["viewport"]} if video else {}))
    context.add_init_script("Object.defineProperty(window, 'showOpenFilePicker', { value: undefined });")
    context.add_init_script("""
(() => {
    const probe = window.__vsProbe = { busyMs: 0, longestMs: 0, mark() { this.busyMs = 0; this.longestMs = 0; } };
    let last = performance.now();
    setInterval(() => {
        const now = performance.now();
        const gap = now - last - 100;
        if (gap > 50) { probe.busyMs += gap; probe.longestMs = Math.max(probe.longestMs, gap); }
        last = now;
    }, 100);
})();
""")
    page = context.pages[0] if context.pages else context.new_page()
    console = open(os.path.join(out, "console.log"), "a")

    def on_console(m):
        console.write(f"{time.time() - t0:7.2f}s [{m.type}] {m.text}\n")
        console.flush()
        text = m.text
        if "thread pool" in text or "exhausted" in text or m.type == "error" or "abort" in text.lower():
            note(f"CONSOLE [{m.type}] {text[:300]}")

    page.on("console", on_console)
    page.on("pageerror", lambda e: note(f"PAGEERROR {str(e)[:300]}"))
    page.on("worker", lambda w: note(f"worker started: {len(page.workers)} now"))
    cdp = context.new_cdp_session(page)
    cdp.send("Emulation.setHardwareConcurrencyOverride", {"hardwareConcurrency": cores})
    if slowdown > 1:
        cdp.send("Emulation.setCPUThrottlingRate", {"rate": slowdown})

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
            note("started: " + state(page))
        elif kind == "reload":
            page.goto(url, wait_until="commit")
            while True:
                time.sleep(1)
                try:
                    if page.evaluate("document.querySelector('#loading')?.style.display === 'none'"):
                        break
                except Exception:
                    pass
            time.sleep(3)
            note("started again: " + state(page))
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
        elif kind == "wait":
            time.sleep(float(arg))
            note("waited: " + state(page))
        elif kind == "waitidle":
            end = time.time() + float(arg)
            t = time.time()
            while time.time() < end:
                try:
                    page.evaluate("1", )
                    break
                except Exception:
                    time.sleep(0.5)
            note(f"idle after {time.time() - t:.1f}s: " + state(page))
        elif kind == "state":
            note(state(page))
        elif kind == "mark":
            page.evaluate("window.__vsProbe.mark()")
            note("mark")
        elif kind == "waitload":
            # Until the page's thread was busy and is idle again (evaluate blocks while it is busy)
            end = time.time() + float(arg)
            t = time.time()
            time.sleep(3)
            seen = 0
            while time.time() < end:
                busy = page.evaluate("window.__vsProbe.busyMs")
                if busy > 500 and busy == seen:
                    break
                seen = busy
                time.sleep(2)
            note(f"loaded after {time.time() - t:.1f}s: " + state(page))
    time.sleep(1)
    context.close()
