#!/usr/bin/env python3
"""compare_rel_rf.py OLD NEW -- the relations text of one build: OLD from facts without RESULT_FLOW and RESULT_EFFECT (the
semantics step's option-on round), NEW from this step's option-on round, compared with the differences the declaration
allows (plan/full_stage_results_20261116.md 9.0): line 2 (the facts hash); two new header lines `rules with result flow:`
and `handling labels (` (NEW only, one each); in the rows, new lines `absence:`, `handling_label:`, `callee-check (result
flow):` and `callee-check (result flow, success):` (NEW only).  With those lines removed, every line must be the same.
Prints a summary line (the new lines per kind); rc 0 when nothing else differs."""
import collections
import re
import sys

ROWLINE = re.compile(r"^      (absence|handling_label|callee-check \(result flow\)|callee-check \(result flow, success\)): ")
HEAD = ("rules with result flow: ", "handling labels (")


def main():
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    a = open(sys.argv[1], encoding="utf-8", errors="surrogateescape").read().split("\n")
    b = open(sys.argv[2], encoding="utf-8", errors="surrogateescape").read().split("\n")
    heads = [l for l in b if l.startswith(HEAD)]
    if len(heads) != 2 or len([l for l in b if l.startswith(HEAD[0])]) != 1:
        print("BAD: %d new header lines" % len(heads))
        return 1
    kinds = collections.Counter()
    bb = []
    for l in b:
        if l.startswith(HEAD):
            continue
        m = ROWLINE.match(l)
        if m:
            kinds[m.group(1)] += 1
            continue
        bb.append(l)
    if any(ROWLINE.match(l) for l in a) or any(l.startswith(HEAD) for l in a):
        print("BAD: the old text has a line of revision 10")
        return 1
    if len(a) != len(bb):
        print("BAD: line count %d / %d (after the new lines)" % (len(a), len(bb)))
        return 1
    diff = [i + 1 for i in range(len(a)) if a[i] != bb[i] and i != 1]
    if diff:
        print("BAD: %d other differing lines (first %s)" % (len(diff), diff[:10]))
        return 1
    print("only declared differences: line 2, 2 header lines, new row lines %s" % dict(sorted(kinds.items())))
    return 0


if __name__ == "__main__":
    sys.exit(main())
