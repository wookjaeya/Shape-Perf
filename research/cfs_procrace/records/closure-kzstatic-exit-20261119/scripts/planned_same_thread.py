#!/usr/bin/env python3
"""planned_same_thread.py PAIRS_TSV PLAN_SIDE -- per planned row of the sidecar (state=planned or control-state=planned):
the pair-file lines of value same_thread by other kind and path, and whether the text counts each (paths B and C, the
only line of its two positions: counted as a one-thread unknown; otherwise not counted).  Reads only."""
import collections, re, sys
pf = open(sys.argv[1], errors="replace").read().split("\n")
cols = next(i for i, l in enumerate(pf) if l.startswith("columns "))
lines = [l.split("\t") for l in pf[cols + 1:] if "\t" in l and not l.startswith("edge\t")]
grp = collections.Counter((f[0], f[6], f[2]) for f in lines)
planned = []
for l in open(sys.argv[2], errors="replace"):
    m = re.match(r"row ([0-9a-f]{16}) (\S+) ", l)
    if m and (" state=planned" in l or " control-state=planned" in l): planned.append((m.group(1), m.group(2)))
for k, rid in planned:
    c = collections.Counter()
    for f in lines:
        if f[0] != k or not f[9].startswith("same_thread"): continue
        counted = f[5] in ("creator", "writer") and f[1] != "A" and grp[(f[0], f[6], f[2])] == 1
        c[(f[5], f[1], "counted" if counted else "not counted", "single" if grp[(f[0], f[6], f[2])] == 1 else "group")] += 1
    if c: print("%s %s: " % (rid, k) + "; ".join("%s path %s %s (%s) %d" % (a, b, s, g, n) for (a, b, s, g), n in sorted(c.items())))
