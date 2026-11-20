#!/usr/bin/env python3
"""measure_rf.py EXTRACT RELATIONS KEYS OLD_ORDER NEW_ORDER S1_PLAN S1_RULES S1_BEFORE S1_AFTER [--key K] -- what the result flow
step's option-on round of one build gives (plan/full_stage_results_20261116.md 9.0), read only from kept outputs:
  rows        the lines `result flow` and `result effect` of EXTRACT (extract.txt of rf_on_extract)
  labels      the handling_label lines of RELATIONS (relations.txt of rf_on_order) by path and label, and its header line
              `handling labels (`
  first path  (--key: the use-site key of the row, from KEYS) the row's lines absence, handling_label, callee-check
              (result flow) and callee-check (result flow, success), verbatim
  order       OLD_ORDER (sem_on_order) and NEW_ORDER (rf_on_order): the summary line of the spawn calls (child without
              success), the HB-await edges of the line `summary edges listed`, the header line `rules with result flow:`,
              the lines `use caller-side:` and the consequences of the lines `use the call of`
  T?          the line of disposition D11 of four outputs of the offline probe: S1_PLAN (s1_rev1.py on the relations.txt
              kept beside the tests_8c_fix facts, written by the relations rules of that time: the input of the earlier
              count), S1_RULES (s1_rev1.py on rf_relcheck's relations: the same facts, the relations rules of today),
              S1_BEFORE (s1_rev1.py on sem_on_order's relations: the earlier option-on round), S1_AFTER (s1_rev2.py on
              RELATIONS), and the [개정 2] lines of S1_AFTER
Counts of rows of a text; no verdict."""
import collections
import re
import sys


def lines(p):
    return open(p, encoding="utf-8", errors="surrogateescape").read().split("\n")


def d11(p):
    for l in lines(p):
        m = re.match(r"^   D11 \S.*?\s(\d+)  경로 ", l)
        if m:
            return int(m.group(1))
    return None


def order_facts(p):
    t = lines(p)
    cws = next((l for l in t if l.startswith("summary spawn calls with resolved threads ")), "-")
    edges = next((l for l in t if l.startswith("summary edges listed ")), "")
    m = re.search(r"HB-await (\d+)", edges)
    head = next((l for l in t if l.startswith("rules with result flow: ")), "-")
    side = sum(1 for l in t if l.startswith("      use caller-side: "))
    cons = collections.Counter()
    for l in t:
        if l.startswith("      use the call of "):
            c = l.rsplit("): ", 1)[-1]
            cons[c.split("(")[0]] += 1
    return cws, int(m.group(1)) if m else 0, head, side, dict(sorted(cons.items()))


def main():
    a = sys.argv[1:]
    key = None
    if "--key" in a:
        i = a.index("--key")
        key = a[i + 1]
        del a[i:i + 2]
    if len(a) != 9:
        print(__doc__, file=sys.stderr)
        return 2
    ext, rel, keys, oord, nord, s1p, s1r, s1b, s1a = a
    e = lines(ext)
    rf = next((l for l in e if l.startswith("result flow ")), "result flow -")
    re_ = next((l for l in e if l.startswith("result effect ")), "result effect -")
    print("rows: %s; %s" % (" ".join(rf.split()), " ".join(re_.split())))
    r = lines(rel)
    by = collections.Counter()
    row = None
    rows = {}
    for l in r:
        m = re.match(r"^  ([ABC])(\d+) ", l)
        if m:
            row = m.group(1) + m.group(2)
            rows[row] = []
            continue
        if row is not None and l.startswith("      "):
            rows[row].append(l)
        m = re.match(r"^      handling_label: (\S+)", l)
        if m and row is not None:
            by[(row[0], m.group(1))] += 1
    tot = collections.Counter()
    for (p, lab), n in by.items():
        tot[lab] += n
    print("handling labels: %s; path A %s; path B %s" % (dict(sorted(tot.items())),
          dict(sorted((k[1], n) for k, n in by.items() if k[0] == "A")), dict(sorted((k[1], n) for k, n in by.items() if k[0] == "B"))))
    print("relations header: %s" % next((l for l in r if l.startswith("handling labels (")), "-"))
    if key:
        rid = None
        for l in lines(keys):
            w = l.split()
            if len(w) >= 2 and w[1] == key:
                rid = w[0]
        if rid is None or rid not in rows:
            print("first path: key %s not in the keys listing or the relations" % key)
        else:
            sel = [l.strip() for l in rows[rid] if re.match(r"^      (absence|handling_label|callee-check \(result flow\)|callee-check \(result flow, success\)): ", l)]
            print("first path row %s (key %s): %d lines" % (rid, key, len(sel)))
            for l in sel:
                print("  " + l)
    for name, p in (("old", oord), ("new", nord)):
        cws, aw, head, side, cons = order_facts(p)
        print("order %s: %s" % (name, cws))
        print("order %s: HB-await edges listed %d; caller-side uses %d; use the call of: %s" % (name, aw, side, cons))
        if name == "new":
            print("order new: %s" % head)
    print("T? (D11 rows of the offline probe): earlier count's input (s1 rev 1, the relations kept with the tests_8c_fix facts) %s; the same facts, today's relations rules (s1 rev 1, rf_relcheck) %s; before (s1 rev 1, the earlier option-on round) %s; after (s1 rev 2, this round) %s" % (d11(s1p), d11(s1r), d11(s1b), d11(s1a)))
    for l in lines(s1a):
        if l.startswith("[개정 2]") or l.startswith("   T? 행: "):
            print("  s1 rev 2: " + l.strip())
    return 0


if __name__ == "__main__":
    sys.exit(main())
