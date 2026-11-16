#!/usr/bin/env python3
"""compare_tools.py OLD NEW -- the outputs of regress_tools.sh before and after the build, file by file.
Allowed differences, and only these: rc.txt's first line (the tool hashes); in c1.run.kzt.txt and c5.run.kzt.txt the
container header line `tool_sha256=..` (kz's own hash, written into every container it makes) and the `container ...
size=` line only if the size is the same (it must be); c1.run.kzt and c5.run.kzt are not compared as bytes (their
header holds the tool hash), their kz-dump text is.  Every other line must be the same, and the line counts.  Prints one
line per file and a summary; rc 0 when nothing else differs, 1 otherwise, 2 on a usage error."""
import os
import re
import sys

ALLOW = {
    "rc.txt": [re.compile(r"kz-rt-dump [0-9a-f]{64} kz [0-9a-f]{64} kz-dump [0-9a-f]{64}")],
    "c1.run.kzt.txt": [re.compile(r"tool_sha256=[0-9a-f]{64}")],
    "c5.run.kzt.txt": [re.compile(r"tool_sha256=[0-9a-f]{64}")],
}
FILES = ["rc.txt", "c1.dump.txt", "c1.convert.log", "c1.run.kzt.txt", "c5.dump.txt", "c5.convert.log", "c5.run.kzt.txt"]


def lines(p):
    with open(p, encoding="utf-8", errors="surrogateescape") as f:
        return f.read().split("\n")


def main():
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    old, new = sys.argv[1:]
    bad = 0
    for name in FILES:
        a, b = lines(os.path.join(old, name)), lines(os.path.join(new, name))
        diffs, allowed = [], 0
        if len(a) != len(b):
            diffs.append("line count %d / %d" % (len(a), len(b)))
        for i in range(min(len(a), len(b))):
            if a[i] == b[i]:
                continue
            if any(p.fullmatch(a[i]) and p.fullmatch(b[i]) for p in ALLOW.get(name, [])):
                allowed += 1
                continue
            diffs.append(i + 1)
        print("%s %s: lines %d, allowed differences %d, other differences %d%s" % (
            "OK" if not diffs else "BAD", name, len(b), allowed, len(diffs), "" if not diffs else " (%s)" % diffs[:10]))
        bad += len(diffs)
    print("compare_tools: %s" % ("only declared differences" if bad == 0 else "%d undeclared differences" % bad))
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
