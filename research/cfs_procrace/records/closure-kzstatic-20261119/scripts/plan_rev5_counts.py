#!/usr/bin/env python3
"""plan_rev5_counts.py DIR... -- the counts of plan rules revision 5 (docs/plan_manifest.md 10.7) in the sidecar of each
plan directory: the planned path A rows and how many have a namespace-entry line (and the not-planned ones by reason),
the namespace-layer lines, the sites per new rule, the contexts and frames, the lifetime lines and lifetime-complete per
kind, the needs of the new rules by state and reason, the key texts bounded by an extent.  Every count is read from the
sidecar; a check of the step (results section 19), not a tool of the method."""
import re, sys, collections

for d in sys.argv[1:]:
    L = open(d + "/plan.side", encoding="utf-8").read().splitlines()
    rows = {}
    for l in L:
        m = re.match(r'^row (\S+) (\S+) .*', l)
        if m and m.group(2).startswith("A") and (" state=planned" in l or "control-state=planned" in l): rows[m.group(1)] = m.group(2)
    ent = [l for l in L if l.startswith("namespace-entry ")]
    ent_rows = {re.search(r"row=(\S+)", l).group(1) for l in ent}
    print(f"== {d}")
    print(f"planned path A rows {len(rows)}: with a namespace-entry line {len(ent_rows & set(rows))} ({' '.join(sorted(rows[k] for k in ent_rows & set(rows)))}); "
          f"without {len(set(rows) - ent_rows)} ({' '.join(sorted(rows[k] for k in set(rows) - ent_rows))})")
    print(f"namespace-entry lines {len(ent)}; namespace-layer lines {sum(1 for l in L if l.startswith('namespace-layer '))}")
    for l in ent:
        f = dict(x.split("=", 1) for x in l.split(" ")[1:] if "=" in x)
        print(f"  entry {rows.get(f['row'], f['row'])}: kind={f['kind']} value={f['value']} stored-name={f['stored-name']} extent-value={f['extent-value']} "
              f"compare-call={f['compare-call']} copy-call={f['copy-call']} length-call={f['length-call']} success-path={f['success-path']}")
    sites = collections.Counter()
    for l in L:
        m = re.match(r'^site \d+ s\d+\.(\S+) key=', l)
        if m:
            for r in m.group(1).split("+"):
                if r in ("kind-value", "context-frame", "object-rollback", "object-clear", "object-delete"): sites[r] += 1
    print("sites by rule (object-delete includes revision 3's): " + "; ".join(f"{r} {n}" for r, n in sorted(sites.items())))
    ctx = [l for l in L if l.startswith("context row=")]
    ctx_rows = collections.Counter(re.search(r"row=(\S+)", l).group(1) for l in ctx)
    print(f"context lines {len(ctx)} in rows {len(ctx_rows)} ({' '.join(sorted(rows.get(k, k) for k in ctx_rows))}); frames found {sum(1 for l in ctx if ' frame=-' not in l)}, none {sum(1 for l in ctx if ' frame=-' in l)}")
    lt = collections.Counter()
    for l in L:
        m = re.match(r'^lifetime \S+ (\S+) .*?: (applies|does not apply|not-a-lifetime-end)', l)
        if m: lt[(m.group(1), m.group(2))] += 1
    print("lifetime lines by operation and state: " + "; ".join(f"{a} {b} {n}" for (a, b), n in sorted(lt.items())))
    for l in L:
        if l.startswith("lifetime-complete "): print("  " + l)
    nd = collections.Counter()
    for l in L:
        m = re.match(r'^need row \S+ (kind-value|object-rollback|object-clear|object-delete|context-frame)(?: \d+)? (planned|not-planned) reason=([a-z_]+(?:\([^)]*\))?|-)', l)
        if m: nd[(m.group(1), m.group(2), m.group(3))] += 1
    print("needs of the rules: " + "; ".join(f"{a} {b} {c} {n}" for (a, b, c), n in sorted(nd.items())))
    b = [l for l in L if l.startswith("bound string-namespace-extent ")]
    print("key texts bounded by an extent: " + (re.search(r"sites=(\d+)", b[0]).group(1) if b else "0"))
