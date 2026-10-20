#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Makes some functions of the app's wasm too big for Safari's optimizing compiler.

JavaScriptCore (Safari; every browser on iOS and iPadOS) compiles a hot wasm function again with its optimizing
compiler (OMG). For some functions of the app (with Asyncify's instrumentation), that took gigabytes of memory, which
it did not give back for minutes: iOS ends such a page, and Safari loads it again (WebKit bug 304810). OMG leaves
alone the functions whose body is bigger than 100,000 bytes (JSC's maximumOMGCandidateCost): this puts a block that
is skipped (br over nops) at the start of each given function, so that its body is bigger than that. The function
runs as before (JavaScriptCore's baseline compiler; other browsers skip the padding when they compile it).

The functions are found by name in the symbol map of the link (--emit-symbol-map: "index:name" lines); names are
matched with fnmatch patterns. A function that is big enough already is left as it is, so running this again does
nothing.

Usage: pad_wasm_functions.py WASM SYMBOL_MAP PATTERN...
"""
import fnmatch
import re
import sys

# Bigger than JavaScriptCore's maximumOMGCandidateCost (100000), with a margin
PADDED_BODY_SIZE = 110000


def read_leb(data, pos):
    result = shift = 0
    while True:
        byte = data[pos]
        pos += 1
        result |= (byte & 0x7F) << shift
        shift += 7
        if byte < 0x80:
            return result, pos


def leb(value):
    out = bytearray()
    while True:
        byte = value & 0x7F
        value >>= 7
        out.append(byte | 0x80 if value else byte)
        if not value:
            return bytes(out)


def symbol_names(path):
    """Function index (imports included) -> name; the map escapes some characters as \\XX (hex)"""
    names = {}
    for line in open(path, encoding="utf-8", errors="replace"):
        index, _, name = line.rstrip("\n").partition(":")
        names[int(index)] = re.sub(r"\\([0-9a-fA-F]{2})", lambda m: chr(int(m.group(1), 16)), name)
    return names


def imported_function_count(data, pos, end):
    count, pos = read_leb(data, pos)
    functions = 0
    for _ in range(count):
        for _ in range(2):  # module, name
            length, pos = read_leb(data, pos)
            pos += length
        kind = data[pos]
        pos += 1
        if kind == 0:  # function: type index
            _, pos = read_leb(data, pos)
            functions += 1
        elif kind == 1:  # table: reference type, limits
            pos += 1
            flags, pos = read_leb(data, pos)
            _, pos = read_leb(data, pos)
            if flags & 1:
                _, pos = read_leb(data, pos)
        elif kind == 2:  # memory: limits
            flags, pos = read_leb(data, pos)
            _, pos = read_leb(data, pos)
            if flags & 1:
                _, pos = read_leb(data, pos)
        elif kind == 3:  # global: value type, mutability
            pos += 2
        elif kind == 4:  # tag: attribute, type index
            pos += 1
            _, pos = read_leb(data, pos)
        else:
            raise ValueError(f"unknown import kind {kind}")
    assert pos == end
    return functions


def padded_body(body):
    """The body (locals and code) with a skipped block of nops after the locals: block, br 0, nop..., end"""
    count, pos = read_leb(body, 0)
    for _ in range(count):
        _, pos = read_leb(body, pos)
        pos += 1  # value type
    nops = PADDED_BODY_SIZE - len(body) - 5
    return body[:pos] + b"\x02\x40\x0c\x00" + b"\x01" * nops + b"\x0b" + body[pos:]


def main():
    wasm_path, symbols_path, *patterns = sys.argv[1:]
    data = open(wasm_path, "rb").read()
    names = symbol_names(symbols_path)
    assert data[:8] == b"\0asm\x01\0\0\0", "not a wasm module"

    out = bytearray(data[:8])
    pos = 8
    imported = 0
    padded = []
    while pos < len(data):
        section_id = data[pos]
        size, start = read_leb(data, pos + 1)
        end = start + size
        if section_id == 2:
            imported = imported_function_count(data, start, end)
        if section_id != 10:
            out += data[pos:end]
            pos = end
            continue

        count, p = read_leb(data, start)
        code = bytearray(leb(count))
        for i in range(count):
            length, p = read_leb(data, p)
            body = data[p:p + length]
            p += length
            name = names.get(imported + i, "")
            if length < PADDED_BODY_SIZE and any(fnmatch.fnmatchcase(name, pattern) for pattern in patterns):
                body = padded_body(body)
                padded.append(name)
            code += leb(len(body)) + body
        assert p == end
        out += bytes([10]) + leb(len(code)) + code
        pos = end

    for pattern in patterns:
        if not any(fnmatch.fnmatchcase(name, pattern) for name in names.values()):
            sys.exit(f"pad_wasm_functions.py: no function matches {pattern}")
    if padded:
        open(wasm_path, "wb").write(out)
    for name in padded:
        print(f"pad_wasm_functions.py: padded {name}")


if __name__ == "__main__":
    main()
