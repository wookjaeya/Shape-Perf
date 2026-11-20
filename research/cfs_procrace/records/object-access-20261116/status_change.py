#!/usr/bin/env python3
"""status_change.py OFF ON [--build B] -- the writer-set status of every target location in the facts views (kz-static
show) of one build extracted without and with --object-access: the count of each status in both views, the
transitions, and for the locations whose status moved from writer-set-complete to DISABLED the change of their A-TYPE
counts (which hit made them DISABLED: a new address-escape hit of the array-decay rule, and the untyped writes of the
program that every escaped location then counts).  The address-escape lines only in ON are put in kinds by the text
the option writes after "the array decays to a pointer to its first element" (passed to a call, converted to an
integer, stored, ...), and the callees of the "passed to" kind are counted.  A measurement, not a verdict
(plan/full_stage_results_20261116.md section 7.5)."""
import collections
import re
import sys

LOCS = re.compile(r"^  (\S+) (writer-set-complete|DISABLED|\S+) writers=(\d+) \(direct (\d+), data-fn (\d+), out-pointer (\d+)\)"
                  r" atype: untyped=(\d+) escape=(\d+) cast=(\d+) union=(\d+) unknown-effect=(\d+) init=(\d+) unresolved-indirect=(\d+)$")
ESC = re.compile(r"^  (\S+) address-escape at (\S+) in (\S+): (.*)$")
DECAY = "the array decays to a pointer to its first element"


def section(lines, name):
    out, on = [], False
    for l in lines:
        if l.startswith("["):
            on = l.split()[0] == "[" + name + "]"
            continue
        if on:
            out.append(l)
    return out


def locs(lines):
    d = {}
    for l in section(lines, "locs"):
        m = LOCS.match(l)
        if m:
            g = m.groups()
            d[g[0]] = {"status": g[1], "writers": int(g[2]), "outptr": int(g[5]), "untyped": int(g[6]), "escape": int(g[7]),
                       "cast": int(g[8]), "union": int(g[9]), "unknown": int(g[10])}
    return d


def escapes(lines):
    return [l for l in section(lines, "atype") if ESC.match(l)]


def kind_of(detail):
    if DECAY not in detail:
        return "other (not the array-decay rule)", None
    rest = detail.split(DECAY, 1)[1].lstrip(",: ")
    m = re.match(r"passed to (\S+?)[:,]", rest + ":")
    if m:
        return "passed to a call", m.group(1)
    return rest.split(" (")[0].split(":")[0].strip() or "(no text)", None


def main():
    a = sys.argv[1:]
    build = "?"
    if "--build" in a:
        i = a.index("--build"); build = a[i + 1]; del a[i:i + 2]
    if len(a) != 2:
        print(__doc__, file=sys.stderr)
        return 2
    off = open(a[0], encoding="utf-8", errors="surrogateescape").read().split("\n")
    on = open(a[1], encoding="utf-8", errors="surrogateescape").read().split("\n")
    lo, ln = locs(off), locs(on)
    print("status_change %s: target locations off %d on %d, in both %d" % (build, len(lo), len(ln), len(set(lo) & set(ln))))
    for name, d in (("off", lo), ("on", ln)):
        c = collections.Counter(v["status"] for v in d.values())
        print("  %s status: %s" % (name, ", ".join("%s %d" % kv for kv in sorted(c.items()))))
    tr = collections.Counter((lo[k]["status"], ln[k]["status"]) for k in set(lo) & set(ln))
    for (x, y), n in sorted(tr.items()):
        print("  transition %s -> %s: %d" % (x, y, n))
    moved = sorted(k for k in set(lo) & set(ln) if lo[k]["status"] == "writer-set-complete" and ln[k]["status"] == "DISABLED")
    why = collections.Counter()
    for k in moved:
        o, n = lo[k], ln[k]
        e = n["escape"] - o["escape"]
        u = n["untyped"] - o["untyped"]
        why[("new escape" if e > 0 else "no new escape") + (", untyped exposed" if o["untyped"] == 0 and n["untyped"] > 0 else "")
            + (", other A-TYPE" if (n["cast"], n["union"], n["unknown"]) != (o["cast"], o["union"], o["unknown"]) else "")] += 1
    print("  complete -> DISABLED: %d; by the A-TYPE change: %s" % (len(moved), "; ".join("%s %d" % kv for kv in sorted(why.items()))))
    for k in moved:
        o, n = lo[k], ln[k]
        print("    %s escape %d->%d untyped %d->%d writers %d->%d (out-pointer %d->%d)" % (
            k, o["escape"], n["escape"], o["untyped"], n["untyped"], o["writers"], n["writers"], o["outptr"], n["outptr"]))
    eo, en = collections.Counter(escapes(off)), collections.Counter(escapes(on))
    added = list((en - eo).elements())
    removed = list((eo - en).elements())
    kinds, callees = collections.Counter(), collections.Counter()
    for l in added:
        kk, callee = kind_of(ESC.match(l).group(4))
        kinds[kk] += 1
        if callee:
            callees[callee] += 1
    print("  address-escape lines: off %d on %d, only on %d, only off %d" % (sum(eo.values()), sum(en.values()), len(added), len(removed)))
    for kk, n in sorted(kinds.items(), key=lambda kv: (-kv[1], kv[0])):
        print("    only on, %s: %d" % (kk, n))
    print("    callees of the 'passed to a call' lines only on (%d callees): %s" % (
        len(callees), ", ".join("%s %d" % kv for kv in sorted(callees.items(), key=lambda kv: (-kv[1], kv[0])))))
    return 0


if __name__ == "__main__":
    sys.exit(main())
