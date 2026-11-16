#!/usr/bin/env python3
"""compare_order.py FIRST SECOND -- the order output of one build from the first build of order rules revision 1 (FIRST,
order_rev1/<b>) against that of the second build (SECOND, order_rev1b/<b>), with the differences the declaration addendum
allows (plan/full_stage_results_20261116.md 3.0a): line 2 (the facts hash, the same here) and, line by line, only
  - the notes of an HB-spawn edge: `      create primitive: ...`, `      status condition: ...`, `      child without success: ...`;
  - the summary line `summary spawn calls with resolved threads ...`;
  - the condition of an HB-lock edge `      condition: the write runs: ...`;
  - a row's pair line whose text before `; applies when the write runs:` is the same (the HB-lock condition it repeats);
  - in edges.tsv, a line of an HB-lock edge whose fields other than the condition are the same.
The two files must have the same number of lines.  Prints one line per file and a summary; rc 0 when nothing else differs."""
import os
import sys

PREFIXES = ("      create primitive: ", "      status condition: ", "      child without success: ", "summary spawn calls with resolved threads ",
            "      condition: the write runs: ")
APPLIES = "; applies when the write runs: "


def read(path):
    with open(path, encoding="utf-8", errors="surrogateescape") as f:
        return f.read().split("\n")


def allowed_text(a, b):
    for p in PREFIXES:
        if a.startswith(p) and b.startswith(p):
            return True
    if a.startswith("      pair: ") and APPLIES in a and APPLIES in b:
        return a.split(APPLIES, 1)[0] == b.split(APPLIES, 1)[0]
    return False


def allowed_tsv(a, b):
    x, y = a.split("\t"), b.split("\t")
    if len(x) != len(y) or len(x) < 11 or x[1] != "HB-lock" or y[1] != "HB-lock":
        return False
    return all(x[i] == y[i] for i in range(len(x)) if i != 7)


def main():
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 64
    bad = 0
    for name, rule in (("order.txt", allowed_text), ("edges.tsv", allowed_tsv)):
        a, b = read(os.path.join(sys.argv[1], name)), read(os.path.join(sys.argv[2], name))
        if len(a) != len(b):
            print("BAD %s: %d lines against %d" % (name, len(a), len(b)))
            bad += 1
            continue
        allowed, other = 0, []
        for i, (x, y) in enumerate(zip(a, b)):
            if x == y or (name == "order.txt" and i == 1):
                continue
            if rule(x, y):
                allowed += 1
            else:
                other.append(i + 1)
        print("%s %s: lines %d, allowed differences %d, other differences %d%s" % (
            "OK" if not other else "BAD", name, len(b), allowed, len(other), "" if not other else " (lines %s)" % other[:10]))
        bad += len(other)
    print("compare_order: %s" % ("only declared differences" if bad == 0 else "%d undeclared differences" % bad))
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
