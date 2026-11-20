#!/usr/bin/env python3
"""measure_v4.py V4 V3 [V4ALL] -- the measurements of one build's plans of plan rules revision 3 (a directory of plans.sh
v4: plan.kzs, plan.side, plan.req), against the revision-2 plan of the same facts (V3: ll_plans_final/<b>) and, given, the
measurement plan with the tier budget all (V4ALL).  Counts only; every number from the files named.
  rows            path A rows of the relations (row lines of the sidecar with an A id), the control set by kind, planned
                  and cut; the path A rows planned by the tier order, by the control selection, by either; of them the
                  rows outside relation 1 (use-site key 17975444d83f6589, the relation the name-binding rule was fitted
                  on: judgement rules 8.0-8.8); the probe-create rows (control a) and whether each is planned
  revision 2      the path A rows the revision-2 plan of the same facts plans (state=planned)
  sites           sites by rule name (a site with two rule names counts under each), place=load sites, spawn statements
  needs           need statements by what (the first word) and state; gap statements by reason
  requests        rows, requests, planned (the end line), and by value kind and state
  lifetime        the sidecar's lifetime lines (applies or not)
Prints the measurement; rc 0, 2 on a usage error."""
import collections
import os
import re
import sys

REL1 = "17975444d83f6589"


def read(p):
    with open(p, encoding="utf-8", errors="surrogateescape") as f:
        return f.read().split("\n")


def rows_of(side):
    out = []
    for l in side:
        m = re.match(r"row ([0-9a-f]{16}) ([ABC])(\d+) tier=(\d) position=\d+ .* state=(planned|cut)(?: control=(\S+) control-state=(planned|cut))? why=", l)
        if m:
            out.append(dict(key=m.group(1), path=m.group(2), id=m.group(2) + m.group(3), tier=m.group(4), state=m.group(5), control=m.group(6), cstate=m.group(7)))
    return out


def sites_report(man, title):
    rules = collections.Counter()
    load = spawn = 0
    for l in man:
        m = re.match(r"site \d+ label=s\d{4}\.(\S+) .*", l)
        if m:
            for r in m.group(1).split("+"):
                rules[r] += 1
            if " place=load " in l:
                load += 1
        if l.startswith("spawn "):
            spawn += 1
    n = sum(1 for l in man if l.startswith("site "))
    print("%s sites %d (load %d, spawn statements %d); by rule: %s" % (title, n, load, spawn, ", ".join("%s %d" % kv for kv in sorted(rules.items()))))


def main():
    if len(sys.argv) not in (3, 4):
        print(__doc__, file=sys.stderr)
        return 2
    v4, v3 = sys.argv[1], sys.argv[2]
    side, man, req = read(os.path.join(v4, "plan.side")), read(os.path.join(v4, "plan.kzs")), read(os.path.join(v4, "plan.req"))
    rows = rows_of(side)
    a = [r for r in rows if r["path"] == "A"]
    ctl = [r for r in a if r["control"]]
    print("plan %s" % v4)
    for l in side:
        if l.startswith(("plan-id ", "control-budget ", "requests ", "budget ")):
            print("  " + l)
    print("rows path A %d; control set %d (a %d, b %d, a+b %d), planned %d, cut %d" % (
        len(a), len(ctl), sum(r["control"] == "a" for r in ctl), sum(r["control"] == "b" for r in ctl), sum(r["control"] == "a+b" for r in ctl),
        sum(r["cstate"] == "planned" for r in ctl), sum(r["cstate"] == "cut" for r in ctl)))
    tier = [r for r in a if r["state"] == "planned"]
    ctlp = [r for r in a if r["cstate"] == "planned"]
    either = [r for r in a if r["state"] == "planned" or r["cstate"] == "planned"]
    print("path A planned: tier %d [%s]; control %d [%s]; either %d" % (len(tier), " ".join(r["id"] for r in tier), len(ctlp), " ".join(r["id"] for r in ctlp), len(either)))
    out = [r for r in either if r["key"] != REL1]
    print("path A planned outside relation 1 (key %s): %d [%s]; relation 1 planned: %s" % (
        REL1, len(out), " ".join("%s(%s)" % (r["id"], r["key"]) for r in out), "yes" if any(r["key"] == REL1 for r in either) else "no (not a row of these facts or not planned)"))
    probe = [r for r in a if r["control"] in ("a", "a+b")]
    print("probe-create rows (control a): %s" % (" ".join("%s(%s) control-state=%s tier-state=%s" % (r["id"], r["key"], r["cstate"], r["state"]) for r in probe) or "none"))
    v3rows = rows_of(read(os.path.join(v3, "plan.side")))
    v3a = [r for r in v3rows if r["path"] == "A" and r["state"] == "planned"]
    print("revision 2 (%s): path A planned %d [%s]" % (v3, len(v3a), " ".join(r["id"] for r in v3a)))
    sites_report(man, "revision 3")
    sites_report(read(os.path.join(v3, "plan.kzs")), "revision 2")
    needs = collections.Counter()
    gaps = collections.Counter()
    for l in man:
        m = re.match(r'need \d+ subject="[^"]*" what="([^" ]+)[^"]*" state=(\S+)', l)
        if m:
            needs[(m.group(1), m.group(2))] += 1
        m = re.match(r'gap \d+ subject="[^"]*" what="[^"]*" reason="([^"]*)"', l)
        if m:
            gaps[m.group(1)] += 1
    print("needs: %s" % ", ".join("%s %s %d" % (k[0], k[1], v) for k, v in sorted(needs.items())))
    print("gaps %d: %s" % (sum(gaps.values()), "; ".join("%s %d" % kv for kv in sorted(gaps.items()))))
    end = [l for l in req if l.startswith("end ")]
    kinds = collections.Counter()
    for l in req:
        m = re.match(r"request \d+ row=\S+ id=\S+ label=\S+ value=(\S+) state=(\S+)", l)
        if m:
            kinds[(re.sub(r"\d+$", "", m.group(1)), m.group(2))] += 1
    print("requests %s; %s" % (end[0] if end else "-", ", ".join("%s %s %d" % (k[0], k[1], v) for k, v in sorted(kinds.items()))))
    for l in side:
        if l.startswith("lifetime "):
            print("  " + l)
    if len(sys.argv) == 4:
        va = sys.argv[3]
        mall = read(os.path.join(va, "plan.kzs"))
        sites_report(mall, "measurement (tier budget all)")
        na = collections.Counter()
        for l in mall:
            m = re.match(r'need \d+ subject="[^"]*" what="([^" ]+)[^"]*" state=(\S+)', l)
            if m:
                na[(m.group(1), m.group(2))] += 1
        print("measurement needs: %s" % ", ".join("%s %s %d" % (k[0], k[1], v) for k, v in sorted(na.items())))
    return 0


if __name__ == "__main__":
    sys.exit(main())
