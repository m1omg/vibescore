#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Stamps the page of the web app with its build.

Usage: stamp_page.py WEB_DIR [--template PAGE]
Writes WEB_DIR/index.html from PAGE (default: WEB_DIR/index.html itself), replacing:
- @VS_BUILD@: an id of this build (a hash of the app's files), which the page adds to the addresses of the files
  it loads, so that the browser does not mix files that it kept from an earlier build with the ones of this build;
- @VS_WASM_SIZE@: the size of VibeScore.wasm, for the progress of its download;
- @VS_THREADS_WASM_SIZE@: the size of threads/VibeScore.wasm, the app with threads (0 when it is not there).
"""
import argparse
import hashlib
import os
import sys

parser = argparse.ArgumentParser()
parser.add_argument("web_dir")
parser.add_argument("--template")
args = parser.parse_args()

web_dir = args.web_dir
page = os.path.join(web_dir, "index.html")
threads_wasm = os.path.join(web_dir, "threads", "VibeScore.wasm")
has_threads = os.path.exists(threads_wasm)

names = ["VibeScore.wasm", "VibeScore.js", "qtloader.js", "vibescore-web.js", "share.data", "share.json"]
if has_threads:
    names += ["threads/VibeScore.wasm", "threads/VibeScore.js"]

digest = hashlib.sha256()
for name in names:
    with open(os.path.join(web_dir, name), "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            digest.update(block)

build = digest.hexdigest()[:12]
wasm_size = os.path.getsize(os.path.join(web_dir, "VibeScore.wasm"))
threads_wasm_size = os.path.getsize(threads_wasm) if has_threads else 0

with open(args.template or page, encoding="utf-8") as f:
    text = f.read()
text = (text.replace("@VS_BUILD@", build)
            .replace("@VS_WASM_SIZE@", str(wasm_size))
            .replace("@VS_THREADS_WASM_SIZE@", str(threads_wasm_size)))
with open(page, "w", encoding="utf-8") as f:
    f.write(text)

print(f"Stamped {page}: build {build}, wasm {wasm_size} bytes, with threads {threads_wasm_size} bytes",
      file=sys.stderr)
