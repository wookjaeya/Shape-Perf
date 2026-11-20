#!/usr/bin/env python3
"""design_rows.py RELATIONS_TXT PLAN_SIDE -- for every planned row of the sidecar (state=planned or control-state=planned):
the row's header and, cut to 240 characters, each of its lines of the closure build's new kinds (success path, absence
origins, holder, alternatives) and the stored-name buffer and lifetime-end lines of the namespace block the header
names.  The rows are taken from the sidecar, not given; reads only, judges nothing."""
import re, sys
text = open(sys.argv[1], errors="replace").read().split("\n")
planned = []
for l in open(sys.argv[2], errors="replace"):
    m = re.match(r"row ([0-9a-f]{16}) (\S+) ", l)
    if m and (" state=planned" in l or " control-state=planned" in l):
        planned.append((m.group(2), "+".join(x for x, on in (("planned", " state=planned" in l), ("control", " control-state=planned" in l)) if on)))
rows, ns = {}, {}
cur = None; curns = None
for l in text:
    m = re.match(r"^  ([ABC]\d+) ", l)
    if m: cur = m.group(1); rows[cur] = [l]; curns = None; continue
    m = re.match(r"^  namespace (\S+) ", l)
    if m: curns = m.group(1); ns[curns] = []; cur = None; continue
    if l.startswith("  ") and not l.startswith("   "): cur = None; curns = None; continue
    if cur: rows[cur].append(l)
    elif curns and re.match(r"^ +(stored-name buffer \[|lifetime end \[)", l): ns[curns].append(l.strip())
kinds = (("success path", r"^ +callee-check \(success path\): "), ("absence origins", r"^ +absence origins: "),
         ("holder", r"^ +holder: "), ("alternatives", r"^ +alternatives"))
for rid, sel in planned:
    b = rows.get(rid)
    if not b: print("%s (%s): no row in the text" % (rid, sel)); continue
    print("%s (%s): %s" % (rid, sel, b[0].strip()[:240]))
    for name, rx in kinds:
        got = [x.strip() for x in b if re.match(rx, x)]
        print("  %s: %d line(s)" % (name, len(got)))
        for x in got: print("    " + x[:240])
    m = re.search(r" namespace (\S+) ", b[0])
    if m:
        got = ns.get(m.group(1), [])
        print("  namespace %s: stored-name buffer / lifetime end lines %d" % (m.group(1), len(got)))
        for x in got: print("    " + x[:240])
