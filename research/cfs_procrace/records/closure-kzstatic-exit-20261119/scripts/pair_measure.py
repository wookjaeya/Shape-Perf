#!/usr/bin/env python3
"""pair_measure.py ORDER_DIR [PLAN_SIDE ...] -- the kept pair measure of one build against the sum of the `pairs:` lines
of its order text and against its complete pair file, all three written by the same kz-static run into ORDER_DIR
(order.txt, cdr.txt, pairs.tsv).

  K  the kept pair measure: the `pairs of the rows N` line of cdr.txt (the caller-dependent measurement of order.cpp,
     which records a pair in pair_values_ only when pair() was computed for it, i.e. both events have a root);
  T  the sum of the `pairs:` lines of order.txt by value;
  F  the lines of pairs.tsv by value, other kind and path.

It prints the decomposition and checks three identities, each on the counted lines (no constant of any build):
  (1) T - K = the pair-file lines whose value is `unknown(no root reaches ...)` (the text counts them as unknown, the
      measurement does not record them);
  (2) F(pairs) - T = the same_thread lines the text does not count: a same_thread line is counted by the text as an
      unknown one-thread pair only on paths B and C when it is the only line of its two positions (the rule of
      records/closure-kzstatic-20261119/scripts/pair_check.py check 4); every other same_thread line (path A, a
      not-a-pair creator, a group of several lines) is outside the text's count;
  (3) K = hard_before + fixed_early_proved + unordered + conditional values + the same_thread lines the text counts as
      unknown (the measurement records the one-thread pairs, since pair() is computed for them).
With PLAN_SIDE files: per planned row (state=planned or control-state=planned) the text sum, the file's lines and the
same split, so that a difference between the two on the planned rows is accounted row by row.
rc 0 when every identity holds, 1 otherwise, 2 on a usage error.  Reads only; judges nothing.
"""
import collections, re, sys

if len(sys.argv) < 2:
    print(__doc__); sys.exit(2)
d = sys.argv[1]
bad = 0
def check(ok, what):
    global bad
    print(("ok   " if ok else "FAIL ") + what)
    if not ok: bad += 1

cdr = open(d + "/cdr.txt", errors="replace").read()
m = re.search(r"^pairs of the rows (\d+) \(unordered (\d+)\)", cdr, re.M)
K, KU = int(m.group(1)), int(m.group(2))

row_re = re.compile(r"^  ([ABC])(\d+) key ([0-9a-f]{16}) order\.static ")
T = collections.Counter(); trow = {}
key = None
for l in open(d + "/order.txt", errors="replace"):
    mm = row_re.match(l)
    if mm: key = mm.group(3); trow.setdefault(key, collections.Counter()); continue
    if key and l.startswith("      pairs: ") and not l.startswith("      pairs: none"):
        for v, c in re.findall(r"(\w+) (\d+)", re.sub(r"\(listed \d+\)", "", l[len("      pairs: "):])):
            T[v] += int(c); trow[key][v] += int(c)

pf = open(d + "/pairs.tsv", errors="replace").read().split("\n")
cols = next(i for i, l in enumerate(pf) if l.startswith("columns "))
lines = [l.split("\t") for l in pf[cols + 1:] if "\t" in l and not l.startswith("edge\t")]
grp = collections.Counter((f[0], f[6], f[2]) for f in lines)
def cls(f):
    v = f[9].split("(")[0]
    if v == "same_thread":
        counted = f[5] in ("creator", "writer") and f[1] != "A" and grp[(f[0], f[6], f[2])] == 1
        return v, ("counted-as-unknown" if counted else "not-counted")
    if v == "unknown":
        return v, ("no-root" if f[9].startswith("unknown(no root reaches") else "other-unknown")
    return v, "-"
F = collections.Counter(); Fd = collections.Counter(); frow = collections.defaultdict(collections.Counter)
for f in lines:
    v, sub = cls(f)
    F[v] += 1; Fd[(v, sub, f[5], f[1])] += 1; frow[f[0]][(v, sub)] += 1

Tsum = sum(T.values()); Fp = sum(n for v, n in F.items() if v != "no_pair")
st_cnt = sum(n for (v, s, k, p), n in Fd.items() if v == "same_thread" and s == "counted-as-unknown")
st_not = sum(n for (v, s, k, p), n in Fd.items() if v == "same_thread" and s == "not-counted")
noroot = sum(n for (v, s, k, p), n in Fd.items() if v == "unknown" and s == "no-root")
oth_unk = sum(n for (v, s, k, p), n in Fd.items() if v == "unknown" and s == "other-unknown")
print("build %s" % d)
print("K kept pair measure (cdr.txt): %d (unordered %d)" % (K, KU))
print("T sum of the pairs: lines: %d; " % Tsum + ", ".join("%s %d" % kv for kv in sorted(T.items())))
print("F pair-file lines: %d (pairs %d, no_pair %d); " % (len(lines), Fp, F.get("no_pair", 0)) + ", ".join("%s %d" % kv for kv in sorted(F.items())))
print("  unknown lines: no root reaches %d, other %d" % (noroot, oth_unk))
print("  same_thread lines: counted by the text as one-thread unknown %d, not counted %d" % (st_cnt, st_not))
print("  same_thread lines not counted, by other kind and path:")
for (v, s, k, p), n in sorted(Fd.items()):
    if v == "same_thread" and s == "not-counted": print("    %s path %s: %d" % (k, p, n))
print("  same_thread lines counted, by other kind and path:")
for (v, s, k, p), n in sorted(Fd.items()):
    if v == "same_thread" and s == "counted-as-unknown": print("    %s path %s: %d" % (k, p, n))
check(Tsum - K == noroot, "(1) T - K = %d - %d = %d = unknown(no root reaches) lines %d" % (Tsum, K, Tsum - K, noroot))
check(T.get("unknown", 0) == noroot + oth_unk + st_cnt, "    text unknown %d = no-root %d + other unknown %d + one-thread same_thread %d" % (T.get("unknown", 0), noroot, oth_unk, st_cnt))
check(Fp - Tsum == st_not, "(2) F(pairs) - T = %d - %d = %d = same_thread lines not counted by the text %d" % (Fp, Tsum, Fp - Tsum, st_not))
ordered = sum(F.get(v, 0) for v in ("hard_before", "conditional_before", "fixed_early_proved", "fixed_early_candidate", "unordered"))
check(K == ordered + st_cnt + oth_unk, "(3) K %d = hard_before + conditional + fixed_early + unordered %d + counted one-thread %d + other unknown %d" % (K, ordered, st_cnt, oth_unk))
check(KU == F.get("unordered", 0) and KU == T.get("unordered", 0), "    unordered: measure %d, text %d, file %d" % (KU, T.get("unordered", 0), F.get("unordered", 0)))

for side in sys.argv[2:]:
    planned = []
    for l in open(side, errors="replace"):
        mm = re.match(r"row ([0-9a-f]{16}) (\S+) ", l)
        if mm and (" state=planned" in l or " control-state=planned" in l): planned.append((mm.group(1), mm.group(2), "+".join(x for x, on in (("planned", " state=planned" in l), ("control", " control-state=planned" in l)) if on)))
    print("plan %s: planned rows %d" % (side, len(planned)))
    print("  row id | selection | text sum | file lines | file no_pair | same_thread not counted | file - text")
    ts = fs = 0; expl = 0
    for k, rid, sel in planned:
        t = sum(trow.get(k, {}).values()); fr = frow.get(k, collections.Counter())
        fl = sum(fr.values()); npair = sum(n for (v, s), n in fr.items() if v == "no_pair")
        nc = sum(n for (v, s), n in fr.items() if v == "same_thread" and s == "not-counted")
        ts += t; fs += fl
        if fl - t == nc + npair: expl += 1
        print("  %s | %s | %d | %d | %d | %d | %d" % (rid, sel, t, fl, npair, nc, fl - t))
    print("  sums: text %d, file lines %d, difference %d" % (ts, fs, fs - ts))
    check(expl == len(planned), "  rows whose difference file - text = same_thread not counted + no_pair: %d of %d" % (expl, len(planned)))
print("result " + ("PASS" if bad == 0 else "FAIL %d" % bad))
sys.exit(0 if bad == 0 else 1)
