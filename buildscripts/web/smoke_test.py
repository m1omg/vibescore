#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Starts the web app in a browser and reports how the start goes.

Usage: smoke_test.py URL OUT_DIR [--engine chromium|firefox] [--timeout SECONDS] [--cpu-slowdown N] [--tablet]
                     [--max-memory MB] [--headed]

Writes to OUT_DIR, with the engine as prefix: the loading steps with their times (timeline.txt), the browser's console
(console.log) and screenshots. After the app started, the page is loaded again (the second start), which must not
download the app again (the browser keeps it). Exits with 1 when the app did not start in time (either time), when the
second start downloaded the app, or (on Linux) when the page's process used more memory than --max-memory.
- --cpu-slowdown N (Chromium): runs the page N times slower, like a slower device.
- --tablet (Chromium): a touch tablet (mobile viewport).
"""
import argparse
import asyncio
import json
import os
import platform
import re
import sys
import time

from playwright.async_api import async_playwright


def page_process_memory_mb():
    """The memory (RSS) of the biggest page process (renderer, content process) that this script started (Linux)."""
    children = {}
    for pid in os.listdir("/proc"):
        if not pid.isdigit():
            continue
        try:
            with open(f"/proc/{pid}/stat") as f:
                ppid = int(f.read().rsplit(")", 1)[1].split()[1])
            children.setdefault(ppid, []).append(int(pid))
        except (OSError, ValueError, IndexError):
            pass
    biggest = 0
    todo = [os.getpid()]
    while todo:
        pid = todo.pop()
        todo += children.get(pid, [])
        try:
            with open(f"/proc/{pid}/cmdline", "rb") as f:
                cmdline = f.read()
            if b"--type=renderer" not in cmdline and b"-contentproc" not in cmdline:
                continue
            with open(f"/proc/{pid}/status") as f:
                for line in f:
                    if line.startswith("VmRSS:"):
                        biggest = max(biggest, int(line.split()[1]) // 1024)
        except OSError:
            pass
    return biggest

# Before the page's scripts: notes each step of the loading screen, and how long the page's thread was busy
# (the longest time between two ticks of a 100 ms timer)
PAGE_PROBE = """
(() => {
    const probe = window.__vsProbe = { steps: [], longestBusyMs: 0 };
    let lastTick = performance.now();
    setInterval(() => {
        const now = performance.now();
        probe.longestBusyMs = Math.max(probe.longestBusyMs, now - lastTick - 100);
        lastTick = now;
    }, 100);
    document.addEventListener('DOMContentLoaded', () => {
        const status = document.querySelector('#status');
        let lastStep = '';
        const note = () => {
            // "Downloading VibeScore… 12 of 140 MB" is one step
            const step = status.textContent.replace(/[0-9.]+/g, '#');
            if (step !== lastStep) {
                lastStep = step;
                probe.steps.push([Math.round(performance.now()), status.textContent]);
            }
        };
        note();
        new MutationObserver(note).observe(status, { childList: true, characterData: true, subtree: true });
    });
})();
"""

STATE = """JSON.stringify({
    status: document.querySelector('#status')?.textContent,
    problem: document.querySelector('#problem')?.textContent,
    started: document.querySelector('#loading')?.style.display === 'none',
    steps: window.__vsProbe?.steps,
    longestBusyMs: Math.round(window.__vsProbe?.longestBusyMs ?? 0),
    heapMB: performance.memory ? Math.round(performance.memory.usedJSHeapSize / 1e6) : null,
})"""


async def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("url")
    parser.add_argument("out")
    parser.add_argument("--engine", default="chromium", choices=["chromium", "firefox"])
    parser.add_argument("--timeout", type=float, default=300)
    parser.add_argument("--cpu-slowdown", type=float, default=0)
    parser.add_argument("--tablet", action="store_true")
    parser.add_argument("--max-memory", type=float, default=0)
    parser.add_argument("--headed", action="store_true")
    args = parser.parse_args()
    has_proc = os.path.isdir("/proc/self")
    peak_memory = 0

    os.makedirs(args.out, exist_ok=True)
    prefix = os.path.join(args.out, args.engine + ("-tablet" if args.tablet else "")
                          + (f"-slow{args.cpu_slowdown:g}" if args.cpu_slowdown else "") + "-")
    timeline = []
    console = []
    t0 = time.time()

    def note(text):
        line = f"{time.time() - t0:7.1f}s  {text}"
        print(line, flush=True)
        timeline.append(line)

    async with async_playwright() as p:
        if args.engine == "firefox":
            # Without a GPU (CI), Firefox shows WebGL only when asked to, with Mesa's software renderer (in a virtual
            # display: --headed under xvfb-run)
            browser = await p.firefox.launch(headless=not args.headed,
                                             firefox_user_prefs={"webgl.force-enabled": True, "webgl.forbid-software": False})
        else:
            # VS_CHROMIUM: another Chromium than Playwright's own, e.g. /usr/bin/google-chrome-stable
            browser = await p.chromium.launch(executable_path=os.environ.get("VS_CHROMIUM"), headless=not args.headed,
                                              args=["--enable-unsafe-swiftshader", "--ignore-gpu-blocklist"])
        note(f"{args.engine} {browser.version} on {platform.machine()}, {os.cpu_count()} CPUs")

        options = {"viewport": {"width": 1280, "height": 800}}
        if args.tablet:
            options = dict(p.devices["Galaxy Tab S4 landscape"])
        context = await browser.new_context(**options)
        page = await context.new_page()
        await page.add_init_script(PAGE_PROBE)
        page.on("console", lambda m: console.append(f"{time.time() - t0:7.1f}s [{m.type}] {m.text}"))
        page.on("pageerror", lambda e: console.append(f"{time.time() - t0:7.1f}s [pageerror] {e}"))
        page.on("crash", lambda _: note("THE PAGE CRASHED"))
        page.on("requestfailed", lambda r: console.append(f"{time.time() - t0:7.1f}s [request failed] {r.url} {r.failure}"))

        if args.cpu_slowdown and args.engine == "chromium":
            cdp = await context.new_cdp_session(page)
            await cdp.send("Emulation.setCPUThrottlingRate", {"rate": args.cpu_slowdown})
            note(f"CPU slowed down {args.cpu_slowdown:g} times")

        async def screenshot(name):
            try:
                await asyncio.wait_for(page.screenshot(path=prefix + name), 30)
            except Exception as e:
                note(f"no screenshot: {e}")

        def note_steps(state, title):
            timeline.append(title)
            for at, text in state.get("steps") or []:
                timeline.append(f"  page step at {at / 1000:7.1f}s: {text}")
            timeline.append(f"  longest time the page's thread was busy: {state.get('longestBusyMs')} ms")
            timeline.append(f"  problem shown: {state.get('problem')!r}")

        async def wait_for_start(start_time):
            """Waits until the app started (True) or could not start (False); returns it with the page's last state."""
            nonlocal peak_memory
            last_status = None
            state = {}
            started = False
            while time.time() - start_time < args.timeout:
                await asyncio.sleep(1)
                if has_proc:
                    peak_memory = max(peak_memory, page_process_memory_mb())
                try:
                    state = json.loads(await asyncio.wait_for(page.evaluate(STATE), 5))
                except asyncio.TimeoutError:
                    note("the page does not answer (its thread is busy)")
                    continue
                except Exception as e:
                    # The page loads itself again once, the first time (see vibescoreIsolatePage in vibescore-web.js)
                    if "context was destroyed" in str(e) or "navigat" in str(e):
                        note("the page loads again")
                        continue
                    note(f"could not read the page: {e}")
                    break
                status = re.sub(r"[0-9.]+ of", "# of", state.get("status") or "")
                if status != last_status:
                    last_status = status
                    note(f"status: {state.get('status')!r}  problem: {state.get('problem')!r}  heap: {state.get('heapMB')} MB")
                if "could not start" in (state.get("status") or ""):
                    note("the page says that the app could not start")
                    break
                if state.get("started"):
                    started = True
                    note(f"the app started in {time.time() - start_time:.1f} s")
                    # The memory is watched a while longer (the browser keeps compiling in the background, and keeps
                    # the app's files)
                    for _ in range(20):
                        await asyncio.sleep(1)
                        if has_proc:
                            peak_memory = max(peak_memory, page_process_memory_mb())
                    break
            if not started:
                note(f"the app did not start in {args.timeout:g} s")
            try:
                state = json.loads(await asyncio.wait_for(page.evaluate(STATE), 10))
            except Exception:
                pass
            return started, state

        await page.goto(args.url, wait_until="commit")
        note("page requested")
        started, state = await wait_for_start(t0)
        note_steps(state, "The first start:")

        # The second start: the browser kept the app (vsFetchKept in vibescore-web.js), so it is not downloaded again
        is_second_start = started
        downloaded_again = False
        if is_second_start:
            await screenshot("screen.png")
            note("the page is loaded again (the second start)")
            await page.reload(wait_until="commit")
            started, state = await wait_for_start(time.time())
            note_steps(state, "The second start:")
            downloaded_again = any("Downloading VibeScore" in text for _, text in state.get("steps") or [])
            if downloaded_again:
                note("the second start downloaded the app again: the browser did not keep it")

        if has_proc:
            note(f"peak memory of the page's process: {peak_memory} MB")
        too_much_memory = args.max_memory and peak_memory > args.max_memory
        if too_much_memory:
            note(f"the page used more than {args.max_memory:g} MB")
        await screenshot("screen-second.png" if is_second_start else "screen.png")

        with open(prefix + "timeline.txt", "w") as f:
            f.write("\n".join(timeline) + "\n")
        with open(prefix + "console.log", "w") as f:
            f.write("\n".join(console) + "\n")
        await browser.close()
    return 0 if started and not downloaded_again and not too_much_memory else 1


if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
