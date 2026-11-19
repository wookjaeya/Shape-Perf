#!/usr/bin/env python3
"""pair_check.py ORDER_TXT PAIRS_TSV EDGES_TSV [PLAN_SIDE ...]

Checks of the complete pair file (order rules revision 8, kairos/docs/static_facts.md 5x and its addendum) against the
order text of the same facts and tool:
  1. the header: facts and tool lines equal to the order text's; the column line;
  2. the tail: rows, pairs and each value count equal to the lines of the file;
  3. every listed `pair:` line of the text has a line of the same row key, positions, roots and value
     (a text `unknown: one thread: ...` pair is a `same_thread` line);
  4. per row, the text's `pairs:` counts equal the file's lines of kind creator or writer (a same_thread line is counted as
     unknown on paths B and C when it is the only line of its two positions, the text's one-thread pair, and left out
     otherwise and on path A: the text pairs no event of the same thread then);
  6. every edge id of the `edges` column is in EDGES_TSV or on an `edge` line of the file, and the tail's
     `edges_not_in_tsv` equals the edge lines;
  5. with plan sidecars: every row key of a row line with `state=planned` or `control-state=planned` has at least one
     line in the file.
Prints one line per check and the counts; rc 0 when every check holds, 1 otherwise, 2 on a usage error.
"""
import re, sys
if len(sys.argv) < 4: print(__doc__); sys.exit(2)
tsv_ids = set(l.split("\t")[0] for l in open(sys.argv[3], errors="replace") if "\t" in l)
text = open(sys.argv[1], errors="replace").read().split("\n")
pf = open(sys.argv[2], errors="replace").read().split("\n")
bad = 0
def check(ok, what):
    global bad
    print(("ok   " if ok else "FAIL ") + what)
    if not ok: bad += 1
# 1 header
tfacts = next((l for l in text if l.startswith("facts ")), None)
ttool = next((l for l in text if l.startswith("tool kz-static ")), None)
check(pf[0] == "kz-order-pairs 1", "first line kz-order-pairs 1")
check(pf[1] == tfacts, "facts line equal to the order text's")
check(pf[2] == ttool and ttool is not None, "tool line equal to the order text's")
cols = "columns row_key\tpath\tuse_pos\tuse_function\tuse_root\tother_kind\tother_pos\tother_function\tother_root\tvalue\tedges\tpremises\trow_line"
check(cols in pf, "column line")
body = pf[pf.index(cols) + 1:]
tail = [l for l in body if l.startswith("rows ")]
lines = [l.split("\t") for l in body if "\t" in l and not l.startswith("edge\t")]
edge_lines = [l.split("\t") for l in body if l.startswith("edge\t")]
check(all(len(f) == 13 for f in lines), "13 fields on every line (%d lines)" % len(lines))
# 2 tail
cnt = {}
for f in lines:
    v = f[9].split("(")[0]; cnt[v] = cnt.get(v, 0) + 1
t = dict(zip(tail[0].split()[0::2], map(int, tail[0].split()[1::2]))) if tail else {}
keys = set(f[0] for f in lines)
check(t.get("rows") == len(keys), "tail rows %s = row keys %d" % (t.get("rows"), len(keys)))
check(t.get("pairs") == sum(n for v, n in cnt.items() if v != "no_pair"), "tail pairs %s = lines without no_pair %d" % (t.get("pairs"), sum(n for v, n in cnt.items() if v != "no_pair")))
for v in ("hard_before", "conditional_before", "fixed_early_proved", "fixed_early_candidate", "unordered", "unknown", "same_thread", "no_pair"):
    check(t.get(v, -1) == cnt.get(v, 0), "tail %s %s = lines %d" % (v, t.get(v), cnt.get(v, 0)))
check(body[-2:] == ["end", ""] or body[-1:] == ["end"], "last line end")
# 6 edges
eids = set(e[1] for e in edge_lines)
named = set(x for f in lines if f[10] != "-" for x in f[10].split(","))
unres = [x for x in named if x not in tsv_ids and x not in eids]
check(not unres, "edge ids of the pair lines resolved by edges.tsv or edge lines: %d of %d (edge lines %d)" % (len(named) - len(unres), len(named), len(edge_lines)))
check(t.get("edges_not_in_tsv") == len(edge_lines) and all(len(e) == 12 for e in edge_lines) and not (eids & tsv_ids), "tail edges_not_in_tsv %s = edge lines %d, 12 fields each, none in edges.tsv" % (t.get("edges_not_in_tsv"), len(edge_lines)))
# 3 listed pairs
idx = {}
for f in lines:
    idx.setdefault((f[0], f[6], f[8], f[2], f[4]), []).append(f[9])
row_re = re.compile(r"^  ([ABC])(\d+) key (\S+) order\.static ")
pr = re.compile(r"^      pair: (?:call|write|read) (\S+) in .*? (\S+ \([a-z-]+\)) -> (?:call|write|read) (\S+) in .*? (\S+ \([a-z-]+\)): (\w+)(: one thread)?")
pr_un = re.compile(r"^      pair: (?:call|write|read) (\S+) in .*-> (?:call|write|read) (\S+) in .*: unknown: no root reaches (\S+)")
key = None; path = None; listed = 0; matched = 0; missing = []
rowcnt = {}
for l in text:
    m = row_re.match(l)
    if m: path, key = m.group(1), m.group(3); continue
    if key is None: continue
    if l.startswith("      pairs: ") and not l.startswith("      pairs: none"):
        c = {}
        for part in l[len("      pairs: "):].split(", "):
            w = part.split(" ")
            c[w[0]] = c.get(w[0], 0) + int(w[1])
        rowcnt[key] = (path, c)
    m = pr.match(l)
    if m:
        listed += 1
        v = "same_thread" if m.group(6) else m.group(5)
        got = idx.get((key, m.group(1), m.group(2), m.group(3), m.group(4)), [])
        if any(g.split("(")[0] == v for g in got): matched += 1
        else: missing.append(l.strip()[:200])
        continue
    m = pr_un.match(l)
    if m:
        listed += 1
        got = [f[9] for f in lines if f[0] == key and f[6] == m.group(1) and f[2] == m.group(2)]
        if any(g == "unknown(no root reaches %s)" % m.group(3) for g in got): matched += 1
        else: missing.append(l.strip()[:200])
check(listed == matched, "listed pair lines of the text with a line of the same value: %d of %d" % (matched, listed))
for x in missing[:10]: print("  missing:", x)
# 4 per-row counts
diff = []
for k, (p, c) in rowcnt.items():
    fc = {}
    grp = {}
    for f in lines:
        if f[0] == k: grp[(f[6], f[2])] = grp.get((f[6], f[2]), 0) + 1
    for f in lines:
        if f[0] != k or f[5] not in ("creator", "writer"): continue
        v = f[9].split("(")[0]
        if v == "same_thread":
            if p == "A" or grp[(f[6], f[2])] != 1: continue
            v = "unknown"
        fc[v] = fc.get(v, 0) + 1
    if fc != c: diff.append((k, c, fc))
check(not diff, "rows whose text pairs counts equal the file's creator and writer lines: %d of %d" % (len(rowcnt) - len(diff), len(rowcnt)))
for x in diff[:10]: print("  differs:", x)
# 5 planned rows
for side in sys.argv[4:]:
    planned = set()
    for l in open(side, errors="replace"):
        if l.startswith("row ") and (" state=planned" in l or " control-state=planned" in l):
            m = re.search(r"\bkey=([0-9a-f]{16})", l) or re.search(r"^row (\S+)", l)
            if m: planned.add(m.group(1))
    have = [k for k in planned if k in keys]
    check(len(have) == len(planned) and planned, "%s: planned rows with lines in the pair file %d of %d" % (side, len(have), len(planned)))
    print("  pair lines of the planned rows %d" % sum(1 for f in lines if f[0] in planned))
print("counts " + " ".join("%s %d" % (v, n) for v, n in sorted(cnt.items())) + "; lines %d; other_kind " % len(lines) +
      " ".join("%s %d" % (k, sum(1 for f in lines if f[5] == k)) for k in sorted(set(f[5] for f in lines))))
print("result " + ("PASS" if bad == 0 else "FAIL %d" % bad))
sys.exit(0 if bad == 0 else 1)
