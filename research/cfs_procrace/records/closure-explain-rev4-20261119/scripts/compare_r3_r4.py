#!/usr/bin/env python3
"""compare_r3_r4.py R3_DIR R4_BRANCH_DIR -- revision 3 records against revision 4 records of the same inputs:
the axis statuses and verdict values record by record, then every differing field path counted by its path class
(list indices removed).  Reads only."""
import json, os, re, sys
a, b = sys.argv[1], sys.argv[2]
fa = sorted(os.listdir(os.path.join(a, "records")))
fb = sorted(os.listdir(os.path.join(b, "records")))
print("records r3 %d r4 %d same names %s" % (len(fa), len(fb), fa == fb))
AX = [("link", "link.status"), ("use_cond", "use_cond.status"), ("order", "order.status"), ("intervention", "intervention.status"),
      ("effect", "effect.status"), ("violation", "tool_verdict.violation.value"), ("odf", "tool_verdict.odf.value"),
      ("headline", "tool_verdict.headline")]
def get(d, p):
    for k in p.split("."):
        d = d.get(k) if isinstance(d, dict) else None
    return d
def flat(x, p=""):
    if isinstance(x, dict):
        for k in x:
            yield from flat(x[k], p + "." + k if p else k)
    elif isinstance(x, list):
        for i, v in enumerate(x):
            yield from flat(v, "%s[%d]" % (p, i))
    else:
        yield p, x
same_axes, diff_axes = 0, []
cls = {}
for f in fa:
    if f not in fb:
        print("only in r3:", f); continue
    A, B = json.load(open(os.path.join(a, "records", f))), json.load(open(os.path.join(b, "records", f)))
    d = [(n, get(A, p), get(B, p)) for n, p in AX if get(A, p) != get(B, p)]
    if d:
        diff_axes.append((f, d))
    else:
        same_axes += 1
    FA, FB = dict(flat(A)), dict(flat(B))
    for k in set(FA) | set(FB):
        if FA.get(k, "<absent>") != FB.get(k, "<absent>"):
            c = re.sub(r"\[\d+\]", "[]", k)
            st = "added" if k not in FA else "removed" if k not in FB else "changed"
            cls.setdefault((c, st), set()).add(f)
print("axis statuses and verdict values equal in %d of %d records" % (same_axes, len(fa)))
for f, d in diff_axes:
    print("AXIS DIFF %s %s" % (f, d))
print("field path classes that differ (class, kind, records):")
for (c, st), fs in sorted(cls.items()):
    print("  %s\t%s\t%d" % (c, st, len(fs)))
sys.exit(1 if diff_axes else 0)
