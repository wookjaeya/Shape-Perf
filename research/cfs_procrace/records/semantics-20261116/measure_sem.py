#!/usr/bin/env python3
"""measure_sem.py VIEW RELATIONS KEYS PLAN [--key K] -- what the SEMANTICS section of one build holds, and what the first
observation path uses of it (a measurement, no judgement; plan/full_stage_results_20261116.md 8.0):
  rows        SEMANTICS rows per layer and verification state, the framework rows per column, the agreement classes
  entries     the BASIS count.semantics_entries lines (per data file: entries, applied, pins not held)
  relations   the relations header line `semantics of these facts`, and the layers lines with sem-unverified(...)
  first path  (--key: the use-site key of the row, from KEYS) the functions the row names that have SEMANTICS rows, per
              layer, each with its verification states; the functions of the plan's site lines (callee= and fn=) that have
              SEMANTICS rows
VIEW is `kz-static show` of the facts (with the semantics section); RELATIONS and KEYS the relations of those facts;
PLAN the plan.kzs generated from them."""
import collections
import re
import sys

ROW = re.compile(r"^  (P0|framework|status-space) (\S+) (\S+)(?: arg \d+)?(?: value -?\d+)? (unverified|body-derived|doc-collated|standard-collated) "
                 r"(\S+) \{[^}]*\} \[(.*?)\] \(")


def main():
    a = sys.argv[1:]
    key = None
    if "--key" in a:
        i = a.index("--key")
        key = a[i + 1]
        a = a[:i] + a[i + 2:]
    if len(a) != 4:
        print(__doc__, file=sys.stderr)
        return 2
    view, rel, keys, plan = a
    rows = []
    entries = []
    sec = None
    for line in open(view, errors="surrogateescape"):
        line = line.rstrip("\n")
        if line.startswith("["):
            sec = line.split("]")[0][1:]
            continue
        if sec == "semantics":
            m = ROW.match(line)
            if m:
                rows.append(m.groups())
            elif line.strip():
                print("UNPARSED " + line[:120])
        if sec == "basis" and "count.semantics_entries[" in line:
            entries.append(line.strip())
    print("rows %d" % len(rows))
    by = collections.Counter((r[0], r[3]) for r in rows)
    for layer in ("P0", "framework", "status-space"):
        n = sum(v for (l, s), v in by.items() if l == layer)
        c = n - by.get((layer, "unverified"), 0)
        print("  layer %s: rows %d, collated %d (%s)" % (layer, n, c, ", ".join("%s %d" % (s, v) for (l, s), v in sorted(by.items()) if l == layer)))
    col = collections.Counter((r[1], r[3]) for r in rows if r[0] == "framework")
    for c in sorted({k[0] for k in col}):
        print("  framework column %s: %s" % (c, ", ".join("%s %d" % (s, v) for (cc, s), v in sorted(col.items()) if cc == c)))
    agr = collections.Counter(re.sub(r"[:(].*", "", r[5]).strip() for r in rows if r[0] == "framework" and r[1] == "agreement")
    for k, v in sorted(agr.items()):
        print("  agreement: %s %d" % (k, v))
    apis = collections.defaultdict(lambda: collections.Counter())
    for r in rows:
        if r[2] != "-":
            apis[(r[0], r[2])][r[3]] += 1
    print("functions with SEMANTICS rows: P0 %d, framework %d" % (len([k for k in apis if k[0] == "P0"]), len([k for k in apis if k[0] == "framework"])))
    for e in entries:
        print("entries " + e)
    text = open(rel, errors="surrogateescape").read().split("\n")
    for l in text:
        if l.startswith("semantics of these facts"):
            print("relations " + l)
    tagged = [l for l in text if re.search(r" sem-unverified\([^()]*\)$", l)]
    print("relations layers lines with sem-unverified %d (rows %d)" % (len(tagged), len([l for l in tagged if l.startswith("      layers: ")])))
    if key:
        rid = None
        for l in open(keys, errors="surrogateescape"):
            w = l.split()
            if len(w) >= 2 and w[1] == key:
                rid = w[0]
        if rid is None:
            print("first path: key %s not in the keys listing" % key)
        else:
            block, on = [], False
            for l in text:
                if l.startswith("  %s " % rid):
                    on = True
                    block.append(l)
                    continue
                if on and (re.match(r"^  [A-Z]\d+ ", l) or l.startswith("[") or l.startswith("  namespace ")):
                    break
                if on:
                    block.append(l)
            words = set(re.findall(r"[A-Za-z_][A-Za-z0-9_]*", "\n".join(block)))
            for layer in ("framework", "P0"):
                named = sorted(n for (ly, n) in apis if ly == layer and n in words)
                print("first path row %s (key %s): %s functions with SEMANTICS rows named %d: %s" % (rid, key, layer, len(named),
                      "; ".join("%s (%s)" % (n, ", ".join("%s %d" % kv for kv in sorted(apis[(layer, n)].items()))) for n in named)))
    sites = set()
    for l in open(plan, errors="surrogateescape"):
        if l.startswith("site "):
            sites |= set(re.findall(r"\b(?:callee|fn)=([A-Za-z_][A-Za-z0-9_]*)", l))
    for layer in ("framework", "P0"):
        named = sorted(n for (ly, n) in apis if ly == layer and n in sites)
        print("plan sites: %s functions with SEMANTICS rows %d of %d named by the sites: %s" % (layer, len(named), len(sites), ", ".join(named)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
