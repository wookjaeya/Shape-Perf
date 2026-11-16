#!/usr/bin/env python3
"""compare_ord_rf.py OLD NEW -- the order text of one build: OLD from facts without RESULT_FLOW and RESULT_EFFECT (the
semantics step's option-on round), NEW from this step's option-on round, compared with the differences the declaration
allows (plan/full_stage_results_20261116.md 9.0): line 2 (the facts hash); one new header line `rules with result flow:`
(NEW only); a line pair that differs only where both lines hold `child without success:` (the summary line and the edge
notes: order rules revision 3 reads the inner spawn call's effects); a `use the call of` line whose consequence
`unknown(the caller tests|uses the result; the conditions of its calls are not in these facts)` becomes another
`unknown(...)` or `caller-side(...)`; new `use caller-side:` lines (NEW only).  Every other line must be the same (a
new HB-await edge or pair line is not allowed by this script: the declaration expects none on the five builds [I]).
Prints a summary line; rc 0 when nothing else differs.
compare_ord_rf.py --tsv OLD NEW -- the edge listings (edges.tsv) of the same two runs: the same lines, apart from lines
whose condition field differs only after `child without success:` (an HB-lock edge's condition ends with the spawn call's
flag); every other field the same."""
import re
import sys

OLDCONS = re.compile(r"^(      use the call of .*\(result (tested|used)\)): unknown\(the caller (tests|uses) the result; the conditions of its calls are not in these facts\)$")
NEWCONS = re.compile(r"^(      use the call of .*\(result (tested|used)\)): (unknown|caller-side)\(.*\)$")


def tsv(pa, pb):
    a = open(pa, encoding="utf-8", errors="surrogateescape").read().split("\n")
    b = open(pb, encoding="utf-8", errors="surrogateescape").read().split("\n")
    if len(a) != len(b):
        print("BAD: line count %d / %d" % (len(a), len(b)))
        return 1
    cws, other = 0, []
    for i, (x, y) in enumerate(zip(a, b)):
        if x == y:
            continue
        fx, fy = x.split("\t"), y.split("\t")
        if len(fx) == len(fy) == 11 and fx[:7] == fy[:7] and fx[8:] == fy[8:] and "child without success:" in fx[7] and "child without success:" in fy[7] \
                and fx[7].split("child without success:")[0] == fy[7].split("child without success:")[0]:
            cws += 1
            continue
        other.append(i + 1)
    if other:
        print("BAD: %d other differing lines (first %s)" % (len(other), other[:10]))
        return 1
    print("only declared differences: %d lines, conditions differing after child without success %d" % (len(b), cws))
    return 0


def main():
    if len(sys.argv) == 4 and sys.argv[1] == "--tsv":
        return tsv(sys.argv[2], sys.argv[3])
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    a = open(sys.argv[1], encoding="utf-8", errors="surrogateescape").read().split("\n")
    b = open(sys.argv[2], encoding="utf-8", errors="surrogateescape").read().split("\n")
    heads = [i for i, l in enumerate(b) if l.startswith("rules with result flow: ")]
    if len(heads) != 1:
        print("BAD: %d header lines `rules with result flow:`" % len(heads))
        return 1
    side = [l for l in b if l.startswith("      use caller-side: ")]
    bb = [l for i, l in enumerate(b) if i != heads[0] and not l.startswith("      use caller-side: ")]
    if len(a) != len(bb):
        print("BAD: line count %d / %d (after the header line and the caller-side uses)" % (len(a), len(bb)))
        return 1
    cws = cons = 0
    other = []
    for i in range(len(a)):
        if a[i] == bb[i] or i == 1:
            continue
        if "child without success:" in a[i] and "child without success:" in bb[i]:
            x, y = a[i].split("child without success:")[0], bb[i].split("child without success:")[0]
            if x == y:
                cws += 1
                continue
        m, n = OLDCONS.match(a[i]), NEWCONS.match(bb[i])
        if m and n and m.group(1) == n.group(1):
            cons += 1
            continue
        other.append(i + 1)
    if other:
        print("BAD: %d other differing lines (first %s)" % (len(other), other[:10]))
        return 1
    print("only declared differences: line 2, the header line, child without success lines %d, use the call of consequences %d, caller-side uses %d" % (cws, cons, len(side)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
