#!/usr/bin/env python3
"""elements_cross.py -- after the expectations were committed (results 26.4): the element values by rule
(expect/elements_by_rule_2.tsv) against the values the fixed judgement part wrote on the static inputs in results 26.3
(closure-explain-fix-20261119/out/static_elements_16_fixed.tsv), cell by cell.  A check only; it changes no expectation.

    elements_cross.py BY_RULE FIXED OUT
"""
import sys


def rd(p):
    L = [l.rstrip("\n").split("\t") for l in open(p, encoding="utf-8") if l.strip() and not l.startswith("#")]
    return {(r[0], r[1], r[2]): dict(zip(L[0], r)) for r in L[1:]}


a, b = rd(sys.argv[1]), rd(sys.argv[2])
m = {"c1": "c1_callee_check", "c2": "c2_reach", "c4": "c4_writers", "c3i": "c3i_branch_i", "c3iii_by_context": "c3iii_by_context",
     "use_cond": "use_cond"}
out, n = [], 0
for k in sorted(set(a) | set(b)):
    for x, y in m.items():
        n += 1
        va, vb = (a.get(k) or {}).get(x), (b.get(k) or {}).get(y)
        if va != vb:
            out.append("\t".join(list(k) + [x, str(va), str(vb)]))
with open(sys.argv[3], "w", encoding="utf-8") as f:
    f.write("# elements_cross.py: %s vs %s\n" % (sys.argv[1].rsplit("/", 1)[-1], sys.argv[2].rsplit("/", 1)[-1]))
    f.write("\n".join(out) + ("\n" if out else ""))
    f.write("rows %d %d cells %d differing %d\n" % (len(a), len(b), n, len(out)))
print("rows %d %d cells %d differing %d" % (len(a), len(b), n, len(out)))
