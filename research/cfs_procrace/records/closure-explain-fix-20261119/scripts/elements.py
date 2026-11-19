#!/usr/bin/env python3
"""elements.py OLD_TABLE DIR... -- the element values (c1)-(c4) per branch and path A row of static dry-run outputs, in the
columns of records/closure-c1prep-20261119/scratch/branches16/static_elements_16.tsv (the frozen judgement part's values
on the same static inputs), and every cell that differs from that table, with the depends_on and the sub-reasons the
revised judgement part writes.  Reads only."""
import json
import os
import sys

COLS = ["build", "branch", "row", "root", "c1_callee_check", "c2_reach", "c4_writers", "c3i_branch_i", "c3iii_by_context",
        "caller_dependence_text", "use_cond", "violation", "link"]
old = {}
with open(sys.argv[1]) as f:
    for ln in f.read().splitlines()[1:]:
        p = ln.split("\t")
        old[tuple(p[:4])] = p
new, extra = {}, []
for d in sys.argv[2:]:
    for br in sorted(os.listdir(d)):
        rd = os.path.join(d, br, "records")
        if not os.path.isdir(rd):
            continue
        for fn in sorted(os.listdir(rd)):
            r = json.load(open(os.path.join(rd, fn)))
            if r["path"] != "A":
                continue
            uc = r["use_cond"]
            root = (r["consumer_actor"].get("root_tie") or {}).get("root") or r["consumer_actor"].get("static_root") or "-"
            row = [r["build_id"]["build"], br, r["row_id"]["row_counter"], str(root), uc["callee_check"]["status"], uc["reach"]["status"],
                   uc["writers"]["status"], uc["branch_i"]["status"], ";".join("%s:%s" % (x["context"], x["status"]) for x in uc["branch_iii"]),
                   uc.get("caller_dependence", {}).get("text", ""), uc["status"], r["tool_verdict"]["violation"]["value"], r["link"]["status"]]
            new[tuple(row[:4])] = row
            deps = {k: len(uc[k].get("depends_on", [])) for k in ("callee_check", "reach", "writers", "branch_i")}
            pairs = r["link"]["name_binding"]["pairs"]
            extra.append("\t".join(row[:4] + ["depends_on c1 %d c2 %d c4 %d c3i %d" % (deps["callee_check"], deps["reach"], deps["writers"], deps["branch_i"]),
                                              "c3iii depends_on %s" % ",".join(str(len(x.get("depends_on", []))) for x in uc["branch_iii"]),
                                              "missing: " + " | ".join(uc["missing"])[:600], "pairs %d" % len(pairs)]))
print("\t".join(COLS))
for k in sorted(new):
    print("\t".join(new[k]))
print("# compare with %s: rows old %d new %d" % (os.path.basename(sys.argv[1]), len(old), len(new)))
nd = 0
for k in sorted(set(old) | set(new)):
    a, b = old.get(k), new.get(k)
    if a is None or b is None:
        print("# only in %s: %s" % ("new" if a is None else "old", " ".join(k)))
        nd += 1
        continue
    for i, c in enumerate(COLS[4:], 4):
        if a[i] != b[i]:
            print("# differs %s %s: %s -> %s" % (" ".join(k), c, a[i], b[i]))
            nd += 1
print("# differing cells %d" % nd)
print("# per row: the depends_on counts, the use_cond missing reasons, the pairs")
for x in extra:
    print("# " + x)
