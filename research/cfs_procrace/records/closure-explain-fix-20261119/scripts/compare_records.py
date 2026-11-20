#!/usr/bin/env python3
"""compare_records.py OLD_DIR NEW_DIR [BRANCH ...] -- the records of two explanation outputs of the same inputs, branch by
branch: the record file names, the axis statuses and verdict values record by record, the uses.tsv lines (column by
column), then every differing field path counted by its class (list indices removed) with one example each.  Field paths
whose only difference is the script's identity (script.judge, script.judge_sha256, script.reader, script.reader_sha256,
inputs) are counted apart.  Reads only."""
import json
import os
import re
import sys

a, b = sys.argv[1], sys.argv[2]
brs = sys.argv[3:] or sorted(x for x in os.listdir(b) if os.path.isfile(os.path.join(b, x, "index.tsv")))
AX = [("link", "link.status"), ("use_cond", "use_cond.status"), ("order", "order.status"), ("intervention", "intervention.status"),
      ("effect", "effect.status"), ("violation", "tool_verdict.violation.value"), ("odf", "tool_verdict.odf.value"),
      ("headline", "tool_verdict.headline")]
IDENT = ("script.", "inputs.", "inputs")


def get(d, p):
    for k in p.split("."):
        d = d.get(k) if isinstance(d, dict) else None
    return d


def flat(x, p=""):
    if isinstance(x, dict):
        if not x and p:
            yield p, "{}"
        for k in x:
            yield from flat(x[k], p + "." + k if p else k)
    elif isinstance(x, list):
        if not x and p:
            yield p, "[]"
        for i, v in enumerate(x):
            yield from flat(v, "%s[%d]" % (p, i))
    else:
        yield p, x


for br in brs:
    da, db = os.path.join(a, br, "records"), os.path.join(b, br, "records")
    fa, fb = sorted(os.listdir(da)), sorted(os.listdir(db))
    print("== branch %s: records old %d new %d same names %s" % (br, len(fa), len(fb), fa == fb))
    same_axes, diff_axes, cls, ident = 0, [], {}, {}
    for f in fa:
        if f not in fb:
            print("only in old:", f)
            continue
        ra, rb = json.load(open(os.path.join(da, f))), json.load(open(os.path.join(db, f)))
        d = [(n, get(ra, p), get(rb, p)) for n, p in AX if get(ra, p) != get(rb, p)]
        if d:
            diff_axes.append((f, d))
        else:
            same_axes += 1
        xa, xb = dict(flat(ra)), dict(flat(rb))
        for p in sorted(set(xa) | set(xb)):
            if xa.get(p, "<absent>") == xb.get(p, "<absent>"):
                continue
            c = re.sub(r"\[\d+\]", "[]", p)
            tgt = ident if c.startswith(IDENT) else cls
            kind = "added" if p not in xa else "removed" if p not in xb else "changed"
            k = (c, kind)
            if k not in tgt:
                tgt[k] = [0, f, xa.get(p, "<absent>"), xb.get(p, "<absent>")]
            tgt[k][0] += 1
    print("axis statuses and verdict values equal: %d of %d records" % (same_axes, len(fa)))
    for f, d in diff_axes:
        print("  axes differ %s: %s" % (f, "; ".join("%s %s -> %s" % x for x in d)))
    ua, ub = os.path.join(a, br, "uses.tsv"), os.path.join(b, br, "uses.tsv")
    if os.path.isfile(ua) and os.path.isfile(ub):
        la, lb = open(ua).read().splitlines(), open(ub).read().splitlines()
        hd = la[0].split("\t") if la else []
        coldiff = {}
        for x, y in zip(la[1:], lb[1:]):
            for i, (p, q) in enumerate(zip(x.split("\t"), y.split("\t"))):
                if p != q:
                    k = (hd[i] if i < len(hd) else str(i), p, q)
                    coldiff[k] = coldiff.get(k, 0) + 1
        print("uses.tsv lines old %d new %d; differing cells (column, old -> new: lines):" % (len(la) - 1, len(lb) - 1))
        for k, n in sorted(coldiff.items()):
            print("  %s: %s -> %s: %d" % (k[0], k[1][:100], k[2][:100], n))
        if not coldiff:
            print("  none")
    print("differing field paths by class (path class, kind: records; one example: old -> new):")
    for (c, kind), (n, f, x, y) in sorted(cls.items()):
        print("  %s [%s]: %d (%s: %s -> %s)" % (c, kind, n, f, json.dumps(x, ensure_ascii=False)[:140], json.dumps(y, ensure_ascii=False)[:140]))
    if not cls:
        print("  none")
    print("script identity paths (counted apart): %s" % ", ".join("%s [%s] %d" % (c, k, v[0]) for (c, k), v in sorted(ident.items())))
