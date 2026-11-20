#!/usr/bin/env python3
"""measure_ll.py EXTRACT_TXT LOOPDEFS_VIEW LOCKH_VIEW OLD_ORDER NEW_ORDER CDR RELATIONS -- the measurements of the loop flag and
lock handle step for one build (plan/full_stage_results_20261116.md 10.0), read from the kept outputs: the final round's
extract.txt, the views of LOOP_DEFS and LOCK_HANDLES (`kz-static show --section loopdefs` / `lockhandles`), the order text of
the result flow round (OLD) and of the final round (NEW), the caller-dependent report and the relations text of the final
round.  The two waits the step names (the entry wait of the second-generation task start, cfe_es_apps.c:508 or :573, and the
retry wait of the CF engine's semaphore lookup, cf_cfdp.c:1001) are listed when the build has them.  A measurement, no
verdict.  Prints `name: value` lines."""
import re
import sys
from collections import Counter

KNOWN = [r"cfe_es_apps\.c:(508|573):5 in CFE_ES_GetTaskFunction", r"cf_cfdp\.c:1001:13 in CF_CFDP_InitEngine"]


def read(p):
    return open(p, encoding="utf-8", errors="surrogateescape").read().split("\n")


def line_with(lines, prefix):
    for l in lines:
        if l.startswith(prefix):
            return l
    return ""


def num(pattern, text, default="-"):
    m = re.search(pattern, text)
    return m.group(1) if m else default


def main():
    if len(sys.argv) != 8:
        print(__doc__, file=sys.stderr)
        return 2
    ex, ld, lh, oo, no, cd, rel = (read(p) for p in sys.argv[1:])
    print("loop defs: " + line_with(ex, "loop defs ").split("loop defs", 1)[-1].strip())
    print("lock handles: " + line_with(ex, "lock handles ").split("lock handles", 1)[-1].strip())
    heads = [l for l in ld if l.startswith("  loop ")]
    st = Counter(re.match(r"^  loop \d+ \S+ in \S+ (\S+)", l).group(1) for l in heads if re.match(r"^  loop \d+ \S+ in \S+ (\S+)", l))
    print("loops examined: %d (%s)" % (len(heads), ", ".join("%s %d" % kv for kv in sorted(st.items()))))
    for l in heads:
        if " recognised form " in l:
            m = re.match(r"^  loop \d+ (\S+) in (\S+) recognised form (\S+)", l)
            print("recognised: %s in %s form %s" % (m.group(1), m.group(2), m.group(3)))
    for k in KNOWN:
        hit = [l for l in heads if re.search(k, l)]
        for l in hit:
            print("named wait: " + l.strip()[:600])
    # LOCK_HANDLES: by status, and the handle fields with their status
    rows = [l for l in lh if l.startswith("  lockop ")]
    by = Counter()
    fields = Counter()
    for l in rows:
        m = re.search(r" family \S+ (\S+) \{", l)
        s = m.group(1) if m else "?"
        by[s] += 1
        f = re.search(r" field (\S+)", l)
        fields[(f.group(1) if f else "(no field)", s)] += 1
    print("lock handle rows: %d (%s)" % (len(rows), ", ".join("%s %d" % kv for kv in sorted(by.items()))))
    for (f, s), n in sorted(fields.items(), key=lambda x: (-x[1], x[0]))[:12]:
        print("handle field: %s %s rows %d" % (f, s, n))
    # order: waits, HB-await, caller-side, HB-lock
    def summary(lines):
        w = line_with(lines, "summary waits ")
        e = line_with(lines, "summary edges listed ")
        return w, e
    ow, oe = summary(oo)
    nw, ne = summary(no)
    print("waits recognised: %s -> %s" % (num(r"predicate recognised (\d+)", ow), num(r"predicate recognised (\d+)", nw)))
    head4 = line_with(no, "rules with loop definitions and lock handles: ")
    print("through LOOP_DEFS: %s; budget values through a flag: %s; HB-lock by handle: %s" % (
        num(r"local definitions (\d+)", head4), num(r"through a flag (\d+)", head4), num(r"identified by its handle (\d+)", head4)))
    print("HB-await edges: %s -> %s (conditional %s, hard %s in all edges)" % (num(r"HB-await (\d+)", oe, "0"), num(r"HB-await (\d+)", ne, "0"),
                                                                            num(r"conditional (\d+)", ne, "0"), num(r"hard (\d+)", ne, "0")))
    orf, nrf = line_with(oo, "rules with result flow: "), line_with(no, "rules with result flow: ")
    print("caller-side uses of waits: %s -> %s; HB-await edges to them %s -> %s" % (
        num(r"caller-side uses of waits (\d+)", orf), num(r"caller-side uses of waits (\d+)", nrf),
        num(r"HB-await edges to them (\d+)", orf), num(r"HB-await edges to them (\d+)", nrf)))
    print("dependent uses: %s -> %s" % (num(r"dependent uses (.*)$", ow), num(r"dependent uses (.*)$", nw)))
    src = Counter(l.strip() for l in no if l.startswith("      lock identity source: "))
    print("HB-lock edges: %s; identity source: %s" % (num(r"HB-lock (\d+)", ne, "0"), ", ".join("%s %d" % (k.replace("lock identity source: ", "")[:40], n) for k, n in sorted(src.items())) or "-"))
    print("rows: " + "; ".join(l.replace("summary rows ", "") for l in no if l.startswith("summary rows ")))
    print("rows (result flow round): " + "; ".join(l.replace("summary rows ", "") for l in oo if l.startswith("summary rows ")))
    for l in cd[2:5]:
        print("caller-dependent: " + l)
    hl = line_with(rel, "rules with lock handles: ")
    print("relations path C by handle: %s, static-instance %s" % (num(r"identified by its handle (\d+)", hl), num(r"static-instance handle (\d+)", hl)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
