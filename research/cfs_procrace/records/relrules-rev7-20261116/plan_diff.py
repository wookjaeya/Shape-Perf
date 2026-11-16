#!/usr/bin/env python3
"""plan_diff.py -- what changed in a generated plan between two generator builds: the rows (by use-site key: tier,
disposition, label, order class, reach, state), the sites (by site key: added, removed; the rows a site serves), the
need and gap lines (by kind and reason) and the header lines.  Run with python3 -I.  Measurement only: no verdict, rc 0
unless an input cannot be read.

  plan_diff.py OLD_SIDECAR NEW_SIDECAR OUT
"""
import collections
import re
import sys

ROWL = re.compile(r'^row (\S+) (\S+) (.*)$')
SITE = re.compile(r'^site (\d+) (\S+) key="([^"]*)" refs="([^"]*)"')


def load(path):
    rows, sites, needs, gaps, head = {}, {}, collections.Counter(), collections.Counter(), []
    for l in open(path, encoding="utf-8"):
        l = l.rstrip("\n")
        m = ROWL.match(l)
        if m:
            kv = dict(re.findall(r'(\w+)=("[^"]*"|\S+)', m.group(3)))
            rows[m.group(1)] = (m.group(2), kv)
            continue
        m = SITE.match(l)
        if m:
            sites[m.group(3)] = (m.group(2), m.group(4))
            continue
        if l.startswith("need "):
            m2 = re.search(r" (\S+) (not-planned|planned|\S+) reason=(\S+\([^)]*\))", l)
            needs[(m2.group(1), m2.group(3)) if m2 else ("?", l[:60])] += 1
            continue
        if l.startswith("gap "):
            m2 = re.search(r"reason=(\S+\([^)]*\))", l)
            gaps[m2.group(1) if m2 else l[:60]] += 1
            continue
        head.append(l)
    return rows, sites, needs, gaps, head


def main():
    old, new, out = sys.argv[1:4]
    orows, osites, oneeds, ogaps, ohead = load(old)
    nrows, nsites, nneeds, ngaps, nhead = load(new)
    o = []
    o.append("# plan_diff: %s -> %s" % (old, new))
    o.append("rows: old %d, new %d; only old %d, only new %d" % (len(orows), len(nrows), len(set(orows) - set(nrows)), len(set(nrows) - set(orows))))
    for state in ("planned",):
        a = {k for k, v in orows.items() if v[1].get("state") == state}
        b = {k for k, v in nrows.items() if v[1].get("state") == state}
        o.append("planned rows: old %d (%s), new %d (%s)" % (len(a), " ".join(sorted(orows[k][0] for k in a)), len(b), " ".join(sorted(nrows[k][0] for k in b))))
        for k in sorted(a - b):
            nv = nrows.get(k)
            o.append("  planned before only: %s %s -> %s" % (k, orows[k][0], ("%s tier=%s disposition=%s label=%s order=%s reach=%s state=%s why=%s" % (
                nv[0], nv[1].get("tier"), nv[1].get("disposition"), nv[1].get("label"), nv[1].get("order"), nv[1].get("reach"), nv[1].get("state"), nv[1].get("why"))) if nv else "no row (left out)"))
        for k in sorted(b - a):
            ov = orows.get(k)
            o.append("  planned after only: %s %s (before: %s)" % (k, nrows[k][0], "%s %s" % (ov[0], ov[1].get("disposition")) if ov else "no row"))
    tr = collections.Counter()
    for k in set(orows) & set(nrows):
        a, b = orows[k][1], nrows[k][1]
        for f in ("tier", "disposition", "label", "order", "reach"):
            if a.get(f) != b.get(f):
                tr["%s %s -> %s" % (f, a.get(f), b.get(f))] += 1
    o.append("kept rows whose fields changed (field old -> new: rows):")
    for t, c in sorted(tr.items()):
        o.append("  %s: %d" % (t, c))
    for name, src in (("tier", orows), ):
        pass
    for label_, rows in (("old", orows), ("new", nrows)):
        c = collections.Counter(v[1].get("tier") for v in rows.values())
        d = collections.Counter(v[1].get("disposition") for v in rows.values())
        o.append("%s tiers %s; dispositions %s" % (label_, dict(sorted(c.items())), dict(sorted(d.items()))))
    o.append("sites: old %d, new %d; only old %d, only new %d, same key %d" % (len(osites), len(nsites), len(set(osites) - set(nsites)), len(set(nsites) - set(osites)), len(set(osites) & set(nsites))))
    rule = lambda lab: lab.split(".", 1)[1] if "." in lab else lab
    for k in sorted(set(osites) - set(nsites)):
        o.append("  site only old: %s %s refs=%s" % (rule(osites[k][0]), k, osites[k][1][:160]))
    for k in sorted(set(nsites) - set(osites)):
        o.append("  site only new: %s %s refs=%s" % (rule(nsites[k][0]), k, nsites[k][1][:160]))
    rc_o = collections.Counter(rule(v[0]) for v in osites.values())
    rc_n = collections.Counter(rule(v[0]) for v in nsites.values())
    o.append("sites per rule: " + ", ".join("%s %d -> %d" % (r, rc_o[r], rc_n[r]) for r in sorted(set(rc_o) | set(rc_n))))
    o.append("need lines per (kind, reason): " + "; ".join("%s %s %d -> %d" % (k[0], k[1], oneeds[k], nneeds[k]) for k in sorted(set(oneeds) | set(nneeds))))
    o.append("gap lines per reason: " + "; ".join("%s %d -> %d" % (k, ogaps[k], ngaps[k]) for k in sorted(set(ogaps) | set(ngaps))))
    hd = [(a, b) for a, b in zip(ohead, nhead) if a != b]
    o.append("header lines that differ: %d (of %d / %d)" % (len(hd), len(ohead), len(nhead)))
    for a, b in hd:
        o.append("  - %s\n  + %s" % (a[:220], b[:220]))
    open(out, "w", encoding="utf-8").write("\n".join(o) + "\n")
    print("\n".join(o[:6]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
