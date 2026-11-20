#!/usr/bin/env python3
"""compare_sem.py OLD_VIEW NEW_VIEW -- the facts views (`kz-static show`) of one build: OLD from the option-on round of the
types and object access step (extract --object-access), NEW from this step's option-on round (--object-access and
--semantics), compared section by section with the differences the declaration allows
(plan/full_stage_results_20261116.md 8.0):
  [semantics]   a new section (NEW only), every line allowed
  [basis]       the header count; new items extract.semantics, extract.semantics[i], count.semantics,
                count.semantics_entries[i] (NEW only); the item extract.units_from with another basis hash
every other section and line must be the same.  Prints a first summary line, then the differing lines that are not
allowed (at most 20); rc 0 when only allowed differences, 1 otherwise."""
import re
import sys

NEWITEM = re.compile(r"^  (extract\.semantics(\[\d+\])?|count\.semantics(_entries\[\d+\])?) = .*$")
UNITS = re.compile(r"^  extract\.units_from = basis [0-9a-f]{16}  \(unset\)$")


def sections(path):
    out, cur = {}, None
    for line in open(path, encoding="utf-8", errors="surrogateescape").read().split("\n"):
        m = re.match(r"^\[([a-z0-9]+)\] \d+$", line)
        if m:
            cur = m.group(1)
            out[cur] = [line]
            continue
        if cur is not None:
            out[cur].append(line)
    for k in out:  # the end of the text (the last section's trailing empty line) is no line of a section
        while out[k] and out[k][-1] == "":
            out[k].pop()
    return out


def main():
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    a, b = sections(sys.argv[1]), sections(sys.argv[2])
    bad = []
    sem_rows = 0
    new_items = 0
    for name in sorted(set(a) | set(b)):
        x, y = a.get(name), b.get(name)
        if name == "semantics":
            if x is not None:
                bad.append("the old view has a [semantics] section")
            sem_rows = len([l for l in (y or [])[1:] if l.strip()])
            continue
        if x is None or y is None:
            bad.append("section [%s] only in %s" % (name, "new" if x is None else "old"))
            continue
        if name == "basis":
            xs = [l for l in x[1:] if not UNITS.match(l)]
            ys = [l for l in y[1:] if not UNITS.match(l)]
            new_items = len([l for l in ys if NEWITEM.match(l)])
            ys = [l for l in ys if not NEWITEM.match(l)]
            if xs != ys:
                bad.append("[basis] differs beyond the allowed items")
            if len([l for l in x[1:] if UNITS.match(l)]) != len([l for l in y[1:] if UNITS.match(l)]):
                bad.append("[basis] extract.units_from count differs")
            continue
        if x != y:
            n = sum(1 for i in range(min(len(x), len(y))) if x[i] != y[i]) + abs(len(x) - len(y))
            bad.append("[%s] differs (%d lines)" % (name, n))
    if bad:
        print("BAD: %d undeclared differences" % len(bad))
        for l in bad[:20]:
            print("  " + l)
        return 1
    print("only declared differences: [semantics] %d rows, basis items %d new, every other section the same" % (sem_rows, new_items))
    return 0


if __name__ == "__main__":
    sys.exit(main())
