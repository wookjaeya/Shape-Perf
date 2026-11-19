#!/usr/bin/env python3
# Counts of the holder lines (relations rules revision 14, static_facts.md 5x item 3) in a relations text: per holder kind,
# writers per kind, FIELD_STATUS per-field states, and the reader verdicts.  holder_counts.py RELATIONS_TXT
import collections, re, sys
c = collections.Counter()
for l in open(sys.argv[1], errors="replace"):
    if not l.startswith("      holder: "): continue
    t = l[len("      holder: "):]
    kind = "field" if t.startswith("field ") else "local" if t.startswith("local ") else "other"
    c["holder lines"] += 1
    c["holder " + kind] += 1
    if kind != "field": continue
    for w in ("own out store", "clearing ", "other definition "):
        c["writers: " + w.strip()] += len(re.findall(r"(?:: |; )" + re.escape(w), t))
    m = re.search(r"FIELD_STATUS per-field=(\S+) type-level=(\S+?)[:;]", t)
    if m: c["per-field " + m.group(1)] += 1
    c["readers: tests the cleared value"] += t.count(": tests the cleared value before the dependent use at ")
    c["readers: uses the value without such a test"] += t.count(": uses the value without such a test")
    c["readers: not decided"] += t.count(": not decided: the guard tests the local ")
    m = re.search(r"branch-condition reads, the tests themselves: (\d+)", t)
    if m: c["test reads"] += int(m.group(1))
    if "other definition " not in t and "clearing " not in t: c["field holders with no writer but the own out store"] += 1
for k in sorted(c): print("%s %d" % (k, c[k]))
