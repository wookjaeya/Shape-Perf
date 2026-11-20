#!/usr/bin/env python3
"""compare_rel_sem.py OLD NEW -- the relations text of one build: OLD from facts without SEMANTICS (the option-on round of
the types and object access step), NEW from this step's option-on round (facts with SEMANTICS), compared line by line with
the differences the declaration allows (plan/full_stage_results_20261116.md 8.0): line 2 (the facts hash); one new
header line `semantics of these facts (SEMANTICS, ...` (NEW only); ` sem-unverified(...)` at the end of a layers line
(rows and name-space blocks).  Every other line must be the same, and the line count after the header line is dropped.
Prints a summary line (tagged layers lines, of them row lines and name-space lines); rc 0 when nothing else differs."""
import re
import sys

TAG = re.compile(r" sem-unverified\([^()]*\)$")


def main():
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    a = open(sys.argv[1], encoding="utf-8", errors="surrogateescape").read().split("\n")
    b = open(sys.argv[2], encoding="utf-8", errors="surrogateescape").read().split("\n")
    head = [i for i, l in enumerate(b) if l.startswith("semantics of these facts (SEMANTICS, ")]
    if len(head) != 1:
        print("BAD: %d header lines `semantics of these facts`" % len(head))
        return 1
    bb = b[:head[0]] + b[head[0] + 1:]
    tagged = rows = ns = 0
    stripped = []
    for l in bb:
        if TAG.search(l):
            tagged += 1
            if l.startswith("      layers: "):
                rows += 1
            elif l.startswith("      namespace layers: "):
                ns += 1
            l = TAG.sub("", l)
        stripped.append(l)
    if len(a) != len(stripped):
        print("BAD: line count %d / %d (after the header line)" % (len(a), len(stripped)))
        return 1
    diff = [i + 1 for i in range(len(a)) if a[i] != stripped[i] and i != 1]
    if diff:
        print("BAD: %d other differing lines (first %s)" % (len(diff), diff[:10]))
        return 1
    print("only declared differences: line 2, the header line, %d tagged layers lines (rows %d, name-space blocks %d)" % (tagged, rows, ns))
    return 0


if __name__ == "__main__":
    sys.exit(main())
