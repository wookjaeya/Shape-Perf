#!/usr/bin/env python3
"""measure_paths.py PLAN [PLAN...] -- per path (A, B, C) of one build's plan (a directory with plan.side; any plan rules
revision): the relation rows, the rows planned by the tier order (row line state=planned), the rows planned by the
control selection (control-state=planned; revision 3 only), by either, and the rows planned by neither, grouped by the
reason of the row's own `need row <key> observation not-planned reason=...` line (and of its `control` need line when
it has one) together with the tier, the disposition and the label the priority rule gave the row.  Also the table
path x label x tier x order class of every row, whatever its state: which labels and tiers the rows of each path have
on this build (to be read beside the label and tier assignment of kairos/lib/llvm/plan_gen.cpp).
Counts only; every number from the sidecar named.  No tool is changed by this script and it gives no input to one.
rc 0; 2 on a usage error; 1 when a row line does not parse or a cut row has no need line."""
import collections
import os
import re
import sys

ROW = re.compile(r"row ([0-9a-f]{16}) ([ABC])(\d+) tier=(\d) position=\d+ label=(\S+) class=(\S+) order=(\S+) reach=(\S+) disposition=(\S+) "
                 r"state=(planned|cut)(?: control=(\S+) control-state=(planned|cut))? why=")
NEED = re.compile(r"need row ([0-9a-f]{16}) (observation|control) not-planned reason=(.*?)(?: note=.*)?$")


def read(p):
    with open(p, encoding="utf-8", errors="surrogateescape") as f:
        return f.read().split("\n")


def one(d):
    side = read(os.path.join(d, "plan.side"))
    rows, bad = [], 0
    needs = collections.defaultdict(dict)
    for l in side:
        if l.startswith("row "):
            m = ROW.match(l)
            if not m:
                bad += 1
                print("UNPARSED " + l[:120])
                continue
            rows.append(dict(key=m.group(1), path=m.group(2), id=m.group(2) + m.group(3), tier=m.group(4), label=m.group(5), order=m.group(7),
                             disp=m.group(9), state=m.group(10), control=m.group(11), cstate=m.group(12)))
        m = NEED.match(l)
        if m:
            needs[m.group(1)][m.group(2)] = m.group(3)
    print("plan %s" % d)
    for l in side:
        if l.startswith(("manifest-revision ", "plan-id ", "classes ", "budget value=", "control-budget ")):
            print("  " + (l if len(l) < 200 else l[:l.find(" reason=")] + " ..." + (l[l.find(" planned="):l.find(" unresolved=")] if " planned=" in l else "")))
    print("path rows tier-planned control-planned either neither")
    tot = [0] * 5
    for p in "ABC":
        r = [x for x in rows if x["path"] == p]
        t = sum(x["state"] == "planned" for x in r)
        c = sum(x["cstate"] == "planned" for x in r)
        e = sum(x["state"] == "planned" or x["cstate"] == "planned" for x in r)
        v = [len(r), t, c, e, len(r) - e]
        tot = [a + b for a, b in zip(tot, v)]
        print("%s %d %d %d %d %d" % tuple([p] + v))
    print("all %d %d %d %d %d" % tuple(tot))
    print("rows planned by neither, by path, reason, tier, disposition, label:")
    g = collections.Counter()
    for x in rows:
        if x["state"] == "planned" or x["cstate"] == "planned":
            continue
        n = needs.get(x["key"], {})
        if "observation" not in n:
            bad += 1
            print("NO-NEED-LINE %s %s" % (x["key"], x["id"]))
        why = n.get("observation", "?") + ("; control: " + n["control"] if "control" in n else "")
        g[(x["path"], why, x["tier"], x["disp"], x["label"])] += 1
    for k, v in sorted(g.items()):
        print("  %s reason=%s tier=%s disposition=%s label=%s: %d" % (k + (v,)))
    print("every row by path, label, tier, order class (planned by the tier order / rows):")
    t = collections.Counter()
    tp = collections.Counter()
    for x in rows:
        k = (x["path"], x["label"], x["tier"], x["order"])
        t[k] += 1
        tp[k] += x["state"] == "planned"
    for k, v in sorted(t.items()):
        print("  %s label=%s tier=%s order=%s: %d / %d" % (k + (tp[k], v)))
    labs = collections.defaultdict(set)
    tiers = collections.defaultdict(set)
    for x in rows:
        labs[x["path"]].add(x["label"])
        tiers[x["path"]].add(x["tier"])
    for p in "ABC":
        print("path %s: labels seen {%s}; tiers seen {%s}" % (p, ", ".join(sorted(labs[p])), ", ".join(sorted(tiers[p]))))
    return bad


def main():
    if len(sys.argv) < 2:
        print(__doc__, file=sys.stderr)
        return 2
    bad = 0
    for d in sys.argv[1:]:
        bad += one(d)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
