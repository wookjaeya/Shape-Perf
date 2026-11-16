#!/usr/bin/env python3
"""compare_rel_ll.py OLD NEW -- relations.txt of one build: OLD from the option-on round of the result flow step, NEW from the
final option-on round of the loop flag and lock handle step (plan/full_stage_results_20261116.md 10.0).  Allowed: line 2 (the
facts hash); one new header line `rules with lock handles: relations rules revision 11 (2026-11-16), over LOCK_HANDLES ...`
(NEW only).  Every other line must be the same, in order (the path C rows through a lock identified by its handle are 0 on the
five builds [I]; a row that changes is outside this allowance).  Prints one summary line and at most 20 differing lines; rc 0
when only allowed differences, 1 otherwise."""
import difflib
import sys


def main():
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    a = open(sys.argv[1], encoding="utf-8", errors="surrogateescape").read().split("\n")
    b = open(sys.argv[2], encoding="utf-8", errors="surrogateescape").read().split("\n")
    if len(a) < 2 or len(b) < 2:
        print("BAD: a file has no second line")
        return 1
    head = [l for l in b if l.startswith("rules with lock handles: relations rules revision 11 (2026-11-16), over LOCK_HANDLES")]
    bb = [l for i, l in enumerate(b) if i != 1 and not l.startswith("rules with lock handles: ")]
    aa = [l for i, l in enumerate(a) if i != 1]
    bad = [l for l in difflib.unified_diff(aa, bb, lineterm="", n=0) if l[:1] in "+-" and not l.startswith(("+++", "---"))]
    if len(head) != 1 or bad:
        print("BAD: new header lines %d; other differing lines %d" % (len(head), len(bad)))
        for l in bad[:20]:
            print("  " + l[:300])
        return 1
    print("only declared differences: line 2, the new header line (%s), every other line the same (%d lines)" % (head[0][:120] + " ...", len(aa)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
