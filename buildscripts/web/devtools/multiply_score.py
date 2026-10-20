#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Makes a long score: repeats the measures of every staff of an .mscx K times. Usage: multiply_score.py IN OUT K
(demos/Dawn.mscx x 30: 570 measures on 13 staves, 9 MB, like a big orchestral score)"""
import re, sys
src, dst, k = sys.argv[1], sys.argv[2], int(sys.argv[3])
s = open(src, encoding="utf-8").read()
count = 0
def repeat(m):
    global count
    body = m.group(2)
    first, last = body.find("<Measure"), body.rfind("</Measure>")
    if first < 0:
        return m.group(0)
    last += len("</Measure>")
    count = body[first:last].count("<Measure")
    return f'<Staff id="{m.group(1)}">' + body[:first] + body[first:last] * k + body[last:] + "</Staff>"
s = re.sub(r'<Staff id="(\d+)">(.*?)</Staff>', repeat, s, flags=re.S)
s = s.replace("<metaTag name=\"workTitle\">Dawn</metaTag>", f"<metaTag name=\"workTitle\">Dawn x{k}</metaTag>")
open(dst, "w", encoding="utf-8").write(s)
print(f"{count} measures per staff x {k} = {count * k}")
