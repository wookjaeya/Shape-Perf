#!/usr/bin/env python3
# Counts of the alternatives lines of a relations text (relations rules revision 14, static_facts.md 5x item 4 and its
# seventh addendum): lines per result class, rows with lines, the contexts per path A row, the premises, the decided calls,
# and the header line of the stored-field rule.  A check, not a tool of the method.   alt_counts.py RELATIONS_TXT
import re, sys, collections
rows = set(); cls = collections.Counter(); prem = collections.Counter(); dec = 0; per_row = collections.Counter(); rests = collections.Counter()
cur = None; hdr = ""
for l in open(sys.argv[1], errors="replace"):
    m = re.match(r"  ([ABC]\d+) ", l)
    if m: cur = m.group(1)
    if l.startswith("rules with decided call results:"): hdr = l.rstrip("\n")
    t = l.strip()
    if not t.startswith("alternatives"): continue
    rows.add(cur); per_row[cur] += 1
    r = t.split("]: ", 1)[1] if "]: " in t else t
    if r.startswith("found: "):
        k = "found (" + ", ".join(sorted(set(re.findall(r"(retry|creation in the same name space and partition by the same function|another path)", r.split("; rests on:")[0])))) + ")"
        if " [not established (" in r.split("; rests on:")[0]: k += " with a context not established"
    elif r.startswith("none found (exploration complete)"): k = "none found (exploration complete)"
    elif r.startswith("not established (the lookup's absence value is not evaluated)"): k = "not established: absence value not evaluated"
    elif r.startswith("not established (open:"): k = "not established: an open effect"
    elif r.startswith("not established (the value's effects end in"): k = "not established: effects end in a function that returns to its caller"
    elif r.startswith("not established"): k = "not established: other (" + r[17:60] + ")"
    else: k = "other"
    if t.startswith("alternatives: the walk stopped"): k = "walk bound reached"
    cls[k] += 1
    pm = re.search(r"; premises: (.*?); decided calls: (.*)$", t)
    if pm:
        if pm.group(1) != "none":
            for p in pm.group(1).split("; ("): prem[re.sub(r"[A-Za-z_0-9]+\.[A-Za-z_0-9]+", "<field>", p.split(" (FIELD_STATUS")[0].lstrip("("))] += 1
        if pm.group(2) != "none": dec += 1
    rs = re.search(r"; rests on: (.*?); premises:", t)
    if rs and rs.group(1) != "none":
        for x in rs.group(1).split(" "): rests[x.split("(")[0]] += 1
print("rows with alternatives lines %d; lines %d; contexts per row: %s" % (len(rows), sum(cls.values()), dict(sorted(collections.Counter(per_row.values()).items()))))
for k, v in sorted(cls.items()): print("  %s: %d" % (k, v))
print("lines with premises: %s" % (dict(prem) or "none"))
print("lines with decided calls: %d" % dec)
print("rests on (lines naming the kind): %s" % (dict(rests) or "none"))
print("header: %s" % hdr)
