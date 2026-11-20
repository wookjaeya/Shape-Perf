#!/usr/bin/env python3
# P5: pairs per build in the kept order outputs: all pairs (the `pairs:` counts) against listed `pair:` lines,
# an estimate of a complete pair output, and for the rows planned in the revision-4 plans whether every pair is
# listed and whether the actors its pairs name are roots that have a root site in the plan (and, for the one kept
# boot record, an actor with that root).
import re, sys, os, collections
W = "/home/user/work/redesign/obs1"
builds = ["c1_pre_tx", "c1_post_tx", "r2_core", "c5_641", "c5_642"]
def rows_of(order):
    rows = []; cur = None
    for ln, l in enumerate(open(order, errors="replace"), 1):
        m = re.match(r"  ([ABC]\d+) key ([0-9a-f]{16}) order\.static (\S+)", l)
        if m: cur = {"id": m.group(1), "key": m.group(2), "val": m.group(3), "listed": [], "total": {}, "line": ln}; rows.append(cur); continue
        if cur is None: continue
        if l.startswith("      pair: "): cur["listed"].append(l.rstrip("\n"))
        elif l.startswith("      pairs: "):
            for v, c in re.findall(r"(\w+) (\d+)", re.sub(r"\(listed \d+\)", "", l[len("      pairs: "):])): cur["total"][v] = cur["total"].get(v, 0) + int(c)
    return rows
def actors_of(pairline):
    return re.findall(r" ((?:[A-Za-z_][A-Za-z0-9_]*|main) \((?:process|spawn-entry|script-entry|table-image-entry|table-walk-entry)\))", pairline)
run_roots = collections.Counter()
dump = W + "/c1pre_boot/tx/tx.dump.txt"; oldplan = W + "/plangen/c1_pre_tx/plan.kzs"
site_root = {}
for l in open(oldplan, errors="replace"):
    m = re.match(r"site (\d+) label=s\d+\.actor-root op=entry\.(\S+)", l)
    if m: site_root[m.group(1)] = m.group(2)
actors_seen = set()
for l in open(dump, errors="replace"):
    m = re.match(r"ev actor=(\d+) aseq=\d+ site=(\d+)\[s\d+\.actor-root\]", l)
    if m: run_roots[site_root.get(m.group(2), "?")] += 1
    m = re.match(r"ev actor=(\d+) ", l)
    if m: actors_seen.add(m.group(1))
print("# kept boot record %s: actors %d, actors with a root event %d, roots: %s" % (dump, len(actors_seen), sum(run_roots.values()), dict(run_roots)))
print("# build | rows | pairs (sum of the `pairs:` lines) | listed pair lines | rows with pairs not all listed | mean bytes of a listed pair line | estimate of a complete pair text = pairs x mean | order.txt bytes")
for b in builds:
    order = "%s/rf2_final_order/%s/order.txt" % (W, b)
    rows = rows_of(order)
    tot = sum(sum(r["total"].values()) for r in rows); lst = sum(len(r["listed"]) for r in rows)
    notall = sum(1 for r in rows if sum(r["total"].values()) > len(r["listed"]))
    mean = (sum(len(x) + 1 for r in rows for x in r["listed"]) / lst) if lst else 0
    print("%s | %d | %d | %d | %d | %.0f | %.0f | %d" % (b, len(rows), tot, lst, notall, mean, tot * mean, os.path.getsize(order)))
    side = "%s/rf2_plans_v4/%s/plan.side" % (W, b); plan = "%s/rf2_plans_v4/%s/plan.kzs" % (W, b)
    planned = {}
    for l in open(side, errors="replace"):
        m = re.match(r"row ([0-9a-f]{16}) (\S+) .*", l)
        if m and (" state=planned" in l or " control-state=planned" in l): planned[m.group(1)] = l
    plan_roots = set(m.group(1) for m in (re.match(r"site \d+ label=s\d+\.actor-root op=entry\.(\S+)", l) for l in open(plan, errors="replace")) if m)
    bykey = {r["key"]: r for r in rows}
    n_all = n_not = n_norow = 0; pr_tot = pr_lst = 0; unm = collections.Counter(); val = collections.Counter(); act_missing_run = collections.Counter()
    for k in planned:
        r = bykey.get(k)
        if not r: n_norow += 1; continue
        t = sum(r["total"].values()); pr_tot += t; pr_lst += len(r["listed"]); val[r["val"]] += 1
        if t > len(r["listed"]): n_not += 1
        else: n_all += 1
        for x in r["listed"]:
            for a in actors_of(x):
                nm = a.split(" (")[0]
                if nm != "main" and nm not in plan_roots: unm[a] += 1
                if b == "c1_pre_tx" and nm != "main" and run_roots.get(nm, 0) == 0: act_missing_run[a] += 1
    print("   planned rows %d (rows of the sidecar with state=planned): order row found %d; all pairs listed %d, not all listed %d; pairs of the planned rows %d, listed %d; row values %s" % (
        len(planned), len(planned) - n_norow, n_all, n_not, pr_tot, pr_lst, dict(val)))
    print("   actors named in the listed pairs of planned rows without a root site in the plan: %s" % (dict(unm) or "none"))
    if b == "c1_pre_tx": print("   actors named in the listed pairs of planned rows with no actor of that root in the kept boot record: %s" % (dict(act_missing_run) or "none"))
