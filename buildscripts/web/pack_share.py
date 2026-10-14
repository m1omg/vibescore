#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Packs the application data (share/) of the web app into one file that the page downloads at startup.

Usage: pack_share.py SHARE_DIR OUT_DIR [--exclude NAME ...]
Writes OUT_DIR/share.data (the files one after the other) and OUT_DIR/share.json:
{"files": [{"path": "templates/...", "offset": 0, "size": 123}, ...]}
(see vibescore-web.js, which writes them to /files/share before the app starts)
"""
import argparse
import json
import os
import sys

parser = argparse.ArgumentParser()
parser.add_argument("share_dir")
parser.add_argument("out_dir")
parser.add_argument("--exclude", action="append", default=[], help="top-level folder of share/ to leave out")
args = parser.parse_args()

files = []
offset = 0
os.makedirs(args.out_dir, exist_ok=True)
with open(os.path.join(args.out_dir, "share.data"), "wb") as data:
    for root, dirs, names in os.walk(args.share_dir):
        rel_root = os.path.relpath(root, args.share_dir)
        if rel_root == ".":
            dirs[:] = sorted(d for d in dirs if d not in args.exclude)
        else:
            dirs.sort()
        for name in sorted(names):
            path = os.path.join(root, name)
            with open(path, "rb") as f:
                content = f.read()
            data.write(content)
            files.append({"path": os.path.relpath(path, args.share_dir).replace(os.sep, "/"),
                          "offset": offset, "size": len(content)})
            offset += len(content)

with open(os.path.join(args.out_dir, "share.json"), "w") as index:
    json.dump({"files": files}, index, separators=(",", ":"))

print(f"Packed {len(files)} files ({offset / 1e6:.1f} MB) into {args.out_dir}/share.data", file=sys.stderr)
