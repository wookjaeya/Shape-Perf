#!/usr/bin/env python3
"""compare_rf.py OLD_VIEW NEW_VIEW -- the facts views (`kz-static show`) of one build: OLD from the option-on round of the
semantics step (--object-access, --semantics), NEW from this step's option-on round (and --result-flow), compared section
by section with the differences the declaration allows (plan/full_stage_results_20261116.md 9.0):
  [resultflow], [resulteffect]   new sections (NEW only), every line allowed
  [basis]   the header count; new items extract.result_flow, count.result_flow, count.result_effect, count.result_flow_depth_cut
            (NEW only); the item extract.units_from with another basis hash
every other section and line must be the same.  Prints a first summary line, then the differing lines that are not
allowed (at most 20); rc 0 when only allowed differences, 1 otherwise."""
import re
import sys

NEWITEM = re.compile(r"^  (extract\.result_flow|count\.result_flow|count\.result_effect|count\.result_flow_depth_cut) = .*$")
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
    for k in out:
        while out[k] and out[k][-1] == "":
            out[k].pop()
    return out


def main():
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    a, b = sections(sys.argv[1]), sections(sys.argv[2])
    bad, rows, new_items = [], {}, 0
    for name in sorted(set(a) | set(b)):
        x, y = a.get(name), b.get(name)
        if name in ("resultflow", "resulteffect"):
            if x is not None:
                bad.append("the old view has a [%s] section" % name)
            rows[name] = len([l for l in (y or [])[1:] if l.startswith("  ") and not l.startswith("      ")])
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
            continue
        if x != y:
            n = sum(1 for i in range(min(len(x), len(y))) if x[i] != y[i]) + abs(len(x) - len(y))
            bad.append("[%s] differs (%d lines)" % (name, n))
    if bad:
        print("BAD: %d undeclared differences" % len(bad))
        for l in bad[:20]:
            print("  " + l)
        return 1
    print("only declared differences: [resultflow] %d rows, [resulteffect] %d rows, basis items %d new, every other section the same" % (
        rows.get("resultflow", 0), rows.get("resulteffect", 0), new_items))
    return 0


if __name__ == "__main__":
    sys.exit(main())
