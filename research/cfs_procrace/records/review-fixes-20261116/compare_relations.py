#!/usr/bin/env python3
"""compare_relations.py OLD NEW -- relations.txt of one build before (relations rules revision 8) and after (revision 9) the
review fixes, with the differences the declaration allows (plan/full_stage_results_20261116.md 6.0 (나)):
  - line 3: `rules relations rules revision 8 (2026-11-16)` -> `... revision 9 (2026-11-16)`;
  - a `      callee-check: ...` line of a creator's callee may get the tail `; not shown to reach the return: <definition>`
    (the old line is the new line without that tail, at the same place);
  - new lines `      callee-check (set): <callee>: complete under these facts (...)` or `... not established (...)`, one per
    creator callee, after its callee-check lines.
Every other line must be the same.  Prints the counts per class and up to 10 other lines; rc 0 when nothing else differs."""
import collections
import difflib
import re
import sys

TAIL = "; not shown to reach the return: "
SET = re.compile(r"      callee-check \(set\): \S+: (complete under these facts \(every return is a constant or the result of a followed call, none through a "
                 r"definition redefined later\)|not established \(\d+ returns not followed, \d+ constants not shown to reach the return\): the constants listed "
                 r"are not all it returns, or not shown; no success constant is taken from them)")


def main():
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    a = open(sys.argv[1], encoding="utf-8", errors="surrogateescape").read().split("\n")
    b = open(sys.argv[2], encoding="utf-8", errors="surrogateescape").read().split("\n")
    c = collections.Counter()
    other = []
    sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
    for op, i1, i2, j1, j2 in sm.get_opcodes():
        if op == "equal":
            continue
        olds = list(a[i1:i2])
        for y in b[j1:j2]:
            if y == "rules relations rules revision 9 (2026-11-16)" and "rules relations rules revision 8 (2026-11-16)" in olds:
                olds.remove("rules relations rules revision 8 (2026-11-16)")
                c["rules"] += 1
                continue
            if SET.fullmatch(y):
                c["set " + ("complete" if "complete under" in y else "not established")] += 1
                continue
            if y.startswith("      callee-check: ") and TAIL in y and y.split(TAIL)[0] in olds:
                olds.remove(y.split(TAIL)[0])
                c["marked"] += 1
                continue
            other.append("+ " + y)
        other += ["- " + x for x in olds]
    print("classes: " + ", ".join("%s %d" % kv for kv in sorted(c.items())) + "; other %d" % len(other))
    for l in other[:10]:
        print("  " + l[:300])
    return 0 if not other and c["rules"] == 1 else 1


if __name__ == "__main__":
    sys.exit(main())
