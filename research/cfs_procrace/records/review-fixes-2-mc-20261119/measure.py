#!/usr/bin/env python3
"""measure.py OLD_ROOT NEW_ROOT BUILD -- the measurements results 12.10 asked of the kz-static build of the second
review-fix round, for one build, from kept outputs only (OLD: the kept outputs of kz-static 1a47abd0 under OLD_ROOT,
NEW: the outputs of the rebuilt tool under NEW_ROOT).  A measurement, not a judgement.  Reads:
  relations.txt of the final option-on round (ll_final_extract / rf2_final_extract): the handling-label header line, the
    handling_label line of every path A and B row (rows matched by row id and the row's header line), the result-flow
    header line (event send, application exit, terminal), the key-clearing lines and counts;
  extract.txt of the final round: the semantics, result flow and lock handles count lines;
  edges.tsv of the final round's order (ll_final_order / rf2_final_order) and of the T8CF order (rf2_pre_order /
    rf2_order): HB-await edges per writer actor (column from_actor);
  the revision-4 plans (mv_plans_v4 / rf2_plans_v4): the control-budget line, the sites by kind (the label after the
    site number), the needs by what= and state=, the gaps.
Output: labelled lines `<what> old=<...> new=<...>` and lists; rc 0 when every file was read, 2 otherwise."""
import collections
import os
import re
import sys


def rd(p):
    with open(p, encoding="utf-8", errors="surrogateescape") as f:
        return f.read().split("\n")


def rows_labels(lines):
    out, cur = collections.OrderedDict(), None
    for l in lines:
        m = re.match(r"^  ([AB][0-9]+) (.*)$", l)
        if m:
            cur = (m.group(1), m.group(2))
            continue
        m = re.match(r"^      handling_label: (\S+)", l)
        if m and cur:
            out[cur] = m.group(1)
    return out


def first(lines, rx):
    for l in lines:
        if re.match(rx, l):
            return l
    return ""


def per_actor(lines, rule="HB-await"):
    c = collections.Counter()
    for l in lines[1:]:
        f = l.split("\t")
        if len(f) > 4 and f[1] == rule:
            c[f[4]] += 1
    return c


def plan_counts(d):
    kzs, side = rd(os.path.join(d, "plan.kzs")), rd(os.path.join(d, "plan.side"))
    sites, needs, gaps = collections.Counter(), collections.Counter(), 0
    for l in kzs:
        m = re.match(r"^site \d+ label=s\d+\.(\S+)", l)
        if m:
            sites[m.group(1)] += 1
        m = re.match(r'^need \d+ .*what="([^"]+)" state=(\S+)', l)
        if m:
            needs["%s %s" % (m.group(1), m.group(2))] += 1
        if l.startswith("gap "):
            gaps += 1
    cb = first(side, r"^control-budget ")
    cb = " ".join(re.findall(r" ((?:rows|planned|cut|complete)=\S+)", cb))
    ctl = sum(1 for l in side if l.startswith("row ") and " control=" in l)
    return sites, needs, gaps, cb, ctl


def main():
    if len(sys.argv) != 4:
        print(__doc__, file=sys.stderr)
        return 2
    O, N, b = sys.argv[1:]
    try:
        ro = rd("%s/ll_final_extract/%s/%s/NP/relations.txt" % (O, b, b))
        rn = rd("%s/rf2_final_extract/%s/%s/NP/relations.txt" % (N, b, b))
        eo = rd("%s/ll_final_extract/%s/%s/NP/extract.txt" % (O, b, b))
        en = rd("%s/rf2_final_extract/%s/%s/NP/extract.txt" % (N, b, b))
        go = rd("%s/ll_final_order/%s/edges.tsv" % (O, b))
        gn = rd("%s/rf2_final_order/%s/edges.tsv" % (N, b))
        to = rd("%s/rf2_pre_order/%s/edges.tsv" % (O, b))
        tn = rd("%s/rf2_order/%s/edges.tsv" % (N, b))
        po = plan_counts("%s/mv_plans_v4/%s" % (O, b))
        pn = plan_counts("%s/rf2_plans_v4/%s" % (N, b))
    except OSError as e:
        print("measure: %s" % e, file=sys.stderr)
        return 2
    print("# measure.py %s (old: kz-static 1a47abd0 outputs; new: the rebuilt tool's)" % b)
    print("handling labels header old: %s" % first(ro, r"^handling labels "))
    print("handling labels header new: %s" % first(rn, r"^handling labels "))
    lo, ln = rows_labels(ro), rows_labels(rn)
    moved = [(k, lo[k], ln[k]) for k in lo if k in ln and lo[k] != ln[k]]
    print("rows with a handling_label old=%d new=%d matched=%d only-old=%d only-new=%d label-moved=%d" % (
        len(lo), len(ln), sum(1 for k in lo if k in ln), sum(1 for k in lo if k not in ln), sum(1 for k in ln if k not in lo), len(moved)))
    mv = collections.Counter("%s->%s" % (a, c) for _, a, c in moved)
    print("label moves by kind: %s" % (", ".join("%s %d" % kv for kv in sorted(mv.items())) or "none"))
    for (rid, head), a, c in moved:
        print("  moved %s %s -> %s | %s" % (rid, a, c, head[:200]))
    for k in (k for k in lo if k not in ln):
        print("  only-old %s | %s" % (k[0], k[1][:200]))
    for k in (k for k in ln if k not in lo):
        print("  only-new %s | %s" % (k[0], k[1][:200]))
    rx = re.compile(r"(\d+) rows \(open (\d+); event send (\d+), application exit (\d+), terminal (\d+)")
    for side, L in (("old", ro), ("new", rn)):
        m = rx.search(first(L, r"^rules with result flow: "))
        print("result effect rows %s: %s" % (side, "rows %s open %s event-send %s application-exit %s terminal %s" % m.groups() if m else "not found"))
    for lab in ("semantics", "result flow", "lock handles"):
        print("extract %s old: %s" % (lab, first(eo, r"^ *%s  " % lab).strip()))
        print("extract %s new: %s" % (lab, first(en, r"^ *%s  " % lab).strip()))
    kc = [l.strip() for l in rn if l.startswith("      key clearing: ")]
    print("key clearing lines new=%d (distinct %d)" % (len(kc), len(set(kc))))
    for l in sorted(set(kc)):
        print("  %s" % l[:300])
    for l in rn:
        m = re.search(r"key-clearing calls (\d+)", l)
        if m:
            print("namespace with key-clearing calls %s: %s" % (m.group(1), l.strip()[:200]))
    for tag, a, c in (("final round", go, gn), ("T8CF", to, tn)):
        ca, cc = per_actor(a), per_actor(c)
        print("HB-await edges %s old=%d new=%d" % (tag, sum(ca.values()), sum(cc.values())))
        for k in sorted(set(ca) | set(cc)):
            if ca[k] != cc[k]:
                print("  writer actor %s: %d -> %d" % (k, ca[k], cc[k]))
    (so, no, gpo, cbo, cto), (sn, nn, gpn, cbn, ctn) = po, pn
    print("v4 control-budget old: %s | rows with control= %d" % (cbo, cto))
    print("v4 control-budget new: %s | rows with control= %d" % (cbn, ctn))
    print("v4 sites old=%d new=%d; gaps old=%d new=%d" % (sum(so.values()), sum(sn.values()), gpo, gpn))
    for k in sorted(set(so) | set(sn)):
        if so[k] != sn[k]:
            print("  site kind %s: %d -> %d" % (k, so[k], sn[k]))
    print("v4 needs old=%d new=%d" % (sum(no.values()), sum(nn.values())))
    for k in sorted(set(no) | set(nn)):
        if no[k] != nn[k]:
            print("  need %s: %d -> %d" % (k, no[k], nn[k]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
