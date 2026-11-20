#!/usr/bin/env python3
"""compare_tools_2.py OLD NEW -- the outputs of regress_tools.sh before and after the build, file by file, with the
allowed differences of compare_tools.py and those of addendum 2 of the declaration (results 11.0.2):
  rc.txt                    the first line (the tool hashes)
  c1.run.kzt.txt, c5.run.kzt.txt
                            the header line `tool_sha256=..` and the line `artifact_sha256=..` (the sha256 of the whole
                            container file, kairos/lib/core/container_reader.cpp:136, which holds the tool hash in its header)
  c1.convert.log, c5.convert.log
                            the line `wrote <OUT>/<c>.run.kzt artifact_sha256 ..` (the same hash)
  c1.run.kzt, c5.run.kzt    compared as bytes: the same size, and every differing byte inside the header fields
                            tool_sha256 and header_crc (file offsets 120..155, kairos/include/kz/schema/kz_schema.h:279-293:
                            magic 8, six uint32, three uint64, basis_sha256 32, profile_sha256 32, then tool_sha256 32 and
                            header_crc 4)
Every other line must be the same, and the line counts.  Prints one line per file and a summary; rc 0 when nothing else
differs, 1 otherwise, 2 on a usage error."""
import os
import re
import sys

ALLOW = {
    "rc.txt": [re.compile(r"kz-rt-dump [0-9a-f]{64} kz [0-9a-f]{64} kz-dump [0-9a-f]{64}")],
    "c1.run.kzt.txt": [re.compile(r"tool_sha256=[0-9a-f]{64}"), re.compile(r"artifact_sha256=[0-9a-f]{64}")],
    "c5.run.kzt.txt": [re.compile(r"tool_sha256=[0-9a-f]{64}"), re.compile(r"artifact_sha256=[0-9a-f]{64}")],
    "c1.convert.log": [re.compile(r"wrote <OUT>/c1\.run\.kzt artifact_sha256 [0-9a-f]{64}")],
    "c5.convert.log": [re.compile(r"wrote <OUT>/c5\.run\.kzt artifact_sha256 [0-9a-f]{64}")],
}
FILES = ["rc.txt", "c1.dump.txt", "c1.convert.log", "c1.run.kzt.txt", "c5.dump.txt", "c5.convert.log", "c5.run.kzt.txt"]
BIN = ["c1.run.kzt", "c5.run.kzt"]
LO, HI = 120, 156  # tool_sha256 (120..151) and header_crc (152..155)


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
    for name in BIN:
        with open(os.path.join(old, name), "rb") as f:
            a = f.read()
        with open(os.path.join(new, name), "rb") as f:
            b = f.read()
        if len(a) != len(b):
            print("BAD %s: size %d / %d" % (name, len(a), len(b)))
            bad += 1
            continue
        d = [i for i in range(len(a)) if a[i] != b[i]]
        out = [i for i in d if not LO <= i < HI]
        print("%s %s: size %d, differing bytes %d (offsets %s), outside tool_sha256 and header_crc %d" % (
            "OK" if not out else "BAD", name, len(b), len(d), "%d..%d" % (d[0], d[-1]) if d else "-", len(out)))
        bad += len(out)
    print("compare_tools_2: %s" % ("only declared differences" if bad == 0 else "%d undeclared differences" % bad))
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
