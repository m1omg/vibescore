#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Adds the app with threads to the web app: the site is the app without threads (the page, its data, the sound font)
with the app with threads in threads/ (the page chooses, see index.html).

Usage: add_threads_app.py WEB_DIR THREADS_WEB_DIR
WEB_DIR: the web app without threads (built with VS_WEB_THREADS=OFF); THREADS_WEB_DIR: the one with threads.
The page in WEB_DIR is stamped again (stamp_page.py), from the page of the sources.
"""
import os
import shutil
import subprocess
import sys

web_dir, threads_web_dir = sys.argv[1], sys.argv[2]
here = os.path.dirname(os.path.abspath(__file__))

target = os.path.join(web_dir, "threads")
os.makedirs(target, exist_ok=True)
for name in ("VibeScore.js", "VibeScore.wasm"):
    shutil.copy2(os.path.join(threads_web_dir, name), os.path.join(target, name))

subprocess.run([sys.executable, os.path.join(here, "stamp_page.py"), web_dir,
                "--template", os.path.join(here, "index.html")], check=True)
print(f"Added the app with threads from {threads_web_dir} to {web_dir}/threads", file=sys.stderr)
