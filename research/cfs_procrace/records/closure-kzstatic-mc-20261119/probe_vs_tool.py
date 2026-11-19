#!/usr/bin/env python3
"""probe_vs_tool.py PROBES_OUT RELDIR -- the kept probe outputs of the fixed tool's facts
(records/closure-kzstatic-20261119/probes/out) beside the lines the declared tool writes for the same objects in the
final-option relations texts RELDIR/<build>/relations.txt (results 21.0 (사) 3).  For each probe it prints the probe's
outcome and the tool's line, one pair per object; it does not judge (the results section reads the pairs).
  P1  alternatives: per probed row, the probe's `alternatives` lines and the tool's `alternatives` lines of that row
  P2  success path: per creator callee, the probe's `basis 4` outcome and the tool's `callee-check (success path)` state
  P4  holder: per probed row, the probe's `branch (i)` line and the tool's `holder:` line of that row
  P6  keys: the probe's key-text summary (the tool side is exit/keys_compare.txt)
  OR  open-reason population: the probe's call-site lines (the tool side is the relations header and exit/address_passing_*)
A check, not a tool of the method.  rc 0 on readable input, 2 otherwise."""
import glob
import os
import re
import sys


def rows_of(rel):
    out, cur = {}, None
    for l in open(rel, errors="replace"):
        m = re.match(r"  ([ABC]\d+) ", l)
        if m:
            cur = m.group(1)
            out[cur] = []
            continue
        if cur and l.startswith("    "):
            out[cur].append(l.rstrip("\n"))
        elif cur and not l.startswith(" "):
            cur = None
    return out


def short(s, n=260):
    return s if len(s) <= n else s[:n] + " ..."


def main():
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    po, rd = sys.argv[1], sys.argv[2]
    rels = {}
    for b in ("c1_pre_tx", "c1_post_tx"):
        rels[b] = rows_of(os.path.join(rd, b, "relations.txt"))
    print("== P1 alternatives")
    for f in sorted(glob.glob(os.path.join(po, "p1_alternatives_*.txt"))):
        m = re.match(r"p1_alternatives_(c1_\w+?_tx)_(A\d+)\.txt", os.path.basename(f))
        b, row = m.group(1), m.group(2)
        print("-- %s %s" % (b, row))
        for l in open(f, errors="replace"):
            if "alternatives [" in l:
                print("  probe: " + short(l.strip()))
        for l in rels[b].get(row, []):
            if l.strip().startswith("alternatives ["):
                print("  tool:  " + short(l.strip()))
    print("== P2 success path")
    for b in ("c1_pre_tx", "c1_post_tx"):
        tool = {}
        for l in open(os.path.join(rd, b, "relations.txt"), errors="replace"):
            m = re.match(r"^ +callee-check \(success path\): ([A-Za-z_0-9]+): (held|missing)(\([^)]*\))?", l)
            if m:
                tool.setdefault(m.group(1), set()).add(m.group(2) + (m.group(3) or ""))
        print("-- %s" % b)
        for l in open(os.path.join(po, "p2_success_path_%s.txt" % b), errors="replace"):
            if l.startswith("#"):
                continue
            callee = re.match(r"([A-Za-z_0-9]+)", l).group(1)
            m = re.search(r"=> basis 4 (static line: )?(\w+)( \([^)]*\))?", l)
            probe = (m.group(2) + (m.group(3) or "")) if m else "?"
            print("  %s: probe %s | tool %s" % (callee, probe, "; ".join(sorted(tool.get(callee, {"(no line)"})))))
    print("== P4 holder")
    for f in sorted(glob.glob(os.path.join(po, "p4_branch_i_*.txt"))):
        m = re.match(r"p4_branch_i_(c1_\w+?_tx)_(A\d+)\.txt", os.path.basename(f))
        b, row = m.group(1), m.group(2)
        print("-- %s %s" % (b, row))
        for l in open(f, errors="replace"):
            if "branch (i)" in l:
                print("  probe: " + short(l.strip()))
        for l in rels[b].get(row, []):
            if l.strip().startswith("holder:"):
                print("  tool:  " + short(l.strip(), 600))
    print("== P6 keys (probe)")
    for l in open(os.path.join(po, "p6_keys.txt"), errors="replace"):
        print("  " + l.rstrip("\n"))
    print("== open-reason population (probe)")
    for l in open(os.path.join(po, "open_reason_population.txt"), errors="replace"):
        print("  " + l.rstrip("\n"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
