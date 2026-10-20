#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Serves a web app folder with the headers that make the page cross-origin isolated (COOP/COEP), which the app with
threads needs: for a build with threads alone (VS_WEB_THREADS=ON, its webapp folder, with no threads/ inside), which
the page would otherwise start without shared memory. Usage: serve_isolated.py DIR PORT"""
import http.server
import sys

directory, port = sys.argv[1], int(sys.argv[2])


class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=directory, **kwargs)

    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cache-Control", "no-store")
        super().end_headers()

    def log_message(self, format, *args):
        pass


http.server.ThreadingHTTPServer.allow_reuse_address = True
http.server.ThreadingHTTPServer(("127.0.0.1", port), Handler).serve_forever()
