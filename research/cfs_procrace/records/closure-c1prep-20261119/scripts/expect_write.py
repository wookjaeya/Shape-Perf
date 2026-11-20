#!/usr/bin/env python3
"""expect_write.py -- the filled expectation file of the C1 cycle (kz-expect 1, plan/c1_closure_explain_rev4_spec_20261118.md
10.6; evaluation protocol 5; results 25.11), written before any boot.  Run with python3 -I.

    expect_write.py --branches BRANCHES --static STATIC_ELEMENTS.tsv --predictions PREDICTIONS.tsv
                    --uses BUILD:USES.tsv:SIDECAR ... -o OUT

No row id, key or value is a constant here.  Every line comes from one of three files:
  static       the frozen explanation script's record fields on the static inputs (no run), per build, branch and row:
               use_cond.status, tool_verdict.violation.value, use_cond.callee_check.status (c1), use_cond.reach.status
               (c2), use_cond.writers.status (c4), use_cond.branch_i.status (c3)(i) -- the values a boot cannot move
  predictions  the hand-written boot predictions [I] per build and row label (what a boot adds: link status, use
               outcome, name state, context, link result, pair items and result); a value per data branch (D1, D2);
               the field iii_boot is (c3)(iii) at a boot, composed into use_cond.caller_dependence.text as
               "(i) <c3i>; (iii) <iii_boot>" (the judgement part's text, explain_judge.py)
  uses         the pre-defined use list of the build (results 25.8): the row key and the consumer root of each label
One line per (branch of the branch file, build, row, field).  The root is the use list's consumer root; a row whose
consumer is "none" takes "*" (every record of the row).
"""
import argparse
import hashlib
import sys

STATIC_FIELDS = [("use_cond.status", "use_cond"), ("tool_verdict.violation.value", "violation"),
                 ("use_cond.callee_check.status", "c1_callee_check"), ("use_cond.reach.status", "c2_reach"),
                 ("use_cond.writers.status", "c4_writers"), ("use_cond.branch_i.status", "c3i_branch_i")]


def tsv(path):
    rows = [l.rstrip("\n").split("\t") for l in open(path, encoding="utf-8") if l.strip() and not l.startswith("#")]
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--branches", required=True)
    ap.add_argument("--static", required=True)
    ap.add_argument("--predictions", required=True)
    ap.add_argument("--uses", action="append", required=True)
    ap.add_argument("-o", required=True)
    a = ap.parse_args()
    branches = [l.split()[1] for l in open(a.branches, encoding="utf-8") if l.startswith("branch ")]
    data_of = {l.split()[1]: l.split()[2].split("=", 1)[1] for l in open(a.branches, encoding="utf-8") if l.startswith("branch ")}
    st = tsv(a.static)
    head, st = st[0], st[1:]
    ix = {h: i for i, h in enumerate(head)}
    static = {(r[ix["build"]], r[ix["branch"]], r[ix["row"]]): r for r in st}
    pred = {}
    for r in tsv(a.predictions):
        pred.setdefault((r[0], r[1]), []).append(r)
    plans, rows = [], []
    for spec in a.uses:
        build, upath, side = spec.split(":")
        plans.append("plan %s" % hashlib.sha256(open(side, "rb").read()).hexdigest())
        seen = set()
        for r in tsv(upath):
            if len(r) < 14 or r[0] != build or r[1] in seen:
                continue
            seen.add(r[1])
            root = r[4].split(" (")[0]
            rows.append((build, r[1], r[2], "*" if root == "none" else root))
    out = []
    for br in branches:
        dcol = 4 if data_of[br] == "D1" else 5
        for build, label, key, root in rows:
            s = static.get((build, br, label))
            if s is None:
                sys.stderr.write("expect_write.py: no static line for %s %s %s\n" % (build, br, label))
                return 1
            for field, col in STATIC_FIELDS:
                out.append(["expect", br, build, key, root, field, s[ix[col]]])
            ps = pred.get((build, label), []) + pred.get(("*", label), [])
            iii = [p for p in ps if p[3] == "iii_boot"]
            if len(iii) != 1:
                sys.stderr.write("expect_write.py: %s %s: %d iii_boot lines\n" % (build, label, len(iii)))
                return 1
            out.append(["expect", br, build, key, root, "use_cond.caller_dependence.text",
                        "(i) %s; (iii) %s" % (s[ix["c3i_branch_i"]], iii[0][dcol])])
            for p in ps:
                if p[3] == "iii_boot":
                    continue
                out.append(["expect", br, build, key, p[2] if p[2] != "*" else root, p[3], p[dcol]])
    with open(a.o, "w", encoding="utf-8") as f:
        f.write("kz-expect 1\n")
        for p in plans:
            f.write(p + "\n")
        for r in out:
            f.write("\t".join(r) + "\n")
        f.write("lines %d\nend\n" % len(out))
    print("expectations: %d lines over %d branches, %d rows" % (len(out), len(branches), len(rows)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
