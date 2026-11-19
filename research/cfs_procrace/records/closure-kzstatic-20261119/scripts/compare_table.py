#!/usr/bin/env python3
"""compare_table.py CMP -- the reports of scratch6_compare.sh in CMP folded into one table per output kind: for each
(step, file) the labels of diffkinds.py, or the differing header fields and sections of sections_cmp.py, with the counts
of the five builds in the order c1_pre_tx/c1_post_tx/r2_core/c5_641/c5_642 (`-removed/+added`, `.` when the build has
none).  A kind whose five reports have no differing line is one `same` line.  A check of results section 20, not a tool
of the method."""
import collections
import os
import re
import sys

B = ["c1_pre_tx", "c1_post_tx", "r2_core", "c5_641", "c5_642"]


def main():
    cmp_dir = sys.argv[1]
    kinds = collections.OrderedDict()
    for f in sorted(os.listdir(cmp_dir)):
        if not f.endswith(".txt"):
            continue
        stem = f[:-4]
        b = next((x for x in B if stem.endswith("_" + x)), None)
        if b is None:
            continue
        kind = stem[: -len(b) - 1]
        rows = kinds.setdefault(kind, collections.OrderedDict())
        with open(os.path.join(cmp_dir, f), encoding="utf-8", errors="replace") as fh:
            lines = fh.read().split("\n")
        if lines[0].startswith("diffkinds"):
            for l in lines[1:]:
                m = re.match(r"^label removed=(\d+) added=(\d+) (.*)$", l)
                if m:
                    rows.setdefault(m.group(3), {})[b] = "-%s/+%s" % (m.group(1), m.group(2))
        else:
            for l in lines:
                m = re.match(r"^header same=\d+ differing=(\d+) \[(.*)\]$", l)
                if m and m.group(1) != "0":
                    rows.setdefault("header fields: " + m.group(2), {})[b] = "x"
                m = re.match(r"^section (\S+) old=(\S+) new=(\S+) differs removed=(\d+) added=(\d+)", l)
                if m:
                    rows.setdefault("section " + m.group(1), {})[b] = "-%s/+%s (%s->%s)" % (m.group(4), m.group(5), m.group(2), m.group(3))
                m = re.match(r"^section (\S+) .*(only-old|only-new)", l)
                if m:
                    rows.setdefault("section " + m.group(1), {})[b] = m.group(2)
                m = re.match(r"^count (\S+) old=(\d+) new=(\d+)", l)
                if m:
                    rows.setdefault("count " + m.group(1), {})[b] = "%s->%s" % (m.group(2), m.group(3))
    for kind, rows in kinds.items():
        if not rows:
            print("%s\tsame (no differing line in the five builds)" % kind)
            continue
        for lab, per in rows.items():
            print("%s\t%s\t%s" % (kind, lab, " ".join(per.get(x, ".") for x in B)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
