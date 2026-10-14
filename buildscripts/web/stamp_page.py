#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Stamps the page of the web app with its build.

Usage: stamp_page.py WEB_DIR
Replaces in WEB_DIR/index.html:
- @VS_BUILD@: an id of this build (a hash of the app's files), which the page adds to the addresses of the files
  it loads, so that the browser does not mix files that it kept from an earlier build with the ones of this build;
- @VS_WASM_SIZE@: the size of VibeScore.wasm, for the progress of its download.
"""
import hashlib
import os
import sys

web_dir = sys.argv[1]
page = os.path.join(web_dir, "index.html")

digest = hashlib.sha256()
for name in ("VibeScore.wasm", "VibeScore.js", "qtloader.js", "vibescore-web.js", "share.data", "share.json"):
    with open(os.path.join(web_dir, name), "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            digest.update(block)

build = digest.hexdigest()[:12]
wasm_size = os.path.getsize(os.path.join(web_dir, "VibeScore.wasm"))

with open(page, encoding="utf-8") as f:
    text = f.read()
text = text.replace("@VS_BUILD@", build).replace("@VS_WASM_SIZE@", str(wasm_size))
with open(page, "w", encoding="utf-8") as f:
    f.write(text)

print(f"Stamped {page}: build {build}, wasm {wasm_size} bytes", file=sys.stderr)
