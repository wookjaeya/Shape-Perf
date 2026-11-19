#!/usr/bin/env python3
"""keys_compare.py OLD_DIR NEW_DIR BUILD... : the use-site key listings (`kz-static relations --keys`) of the same builds
by two tools: OLD_DIR/<build>/keys.txt against NEW_DIR/<build>.keys.txt.  Per build: rows, rows whose key moved (with
the old and new key text), keys shared by different texts; then the key texts of the first two builds against each other
(rows with the same row id: equal texts n of n; key sets: equal keys n).  rc 0 always on readable input, 2 otherwise."""
import sys
def rows(p):
    out = {}
    for l in open(p, errors="replace"):
        f = l.rstrip("\n").split(" ", 2)
        if len(f) == 3 and f[0][:1] in "ABC" and f[0][1:].isdigit(): out[f[0]] = (f[1], f[2])
    return out
if len(sys.argv) < 4: print(__doc__); sys.exit(2)
old, new, builds = sys.argv[1], sys.argv[2], sys.argv[3:]
got = {}
for b in builds:
    o = rows("%s/%s/keys.txt" % (old, b)); n = rows("%s/%s.keys.txt" % (new, b)); got[b] = n
    moved = [r for r in o if r in n and o[r][1] != n[r][1]]
    print("%s: rows old %d new %d; rows whose key text moved %d; rows only in one %d" % (b, len(o), len(n), len(moved), len(set(o) ^ set(n))))
    for r in moved: print("  %s %s %s -> %s %s" % (r, o[r][0], o[r][1], n[r][0], n[r][1]))
    texts = {}
    for r, (k, t) in n.items(): texts.setdefault(k, set()).add(t)
    print("  keys shared by different texts (new): %d" % sum(1 for k, ts in texts.items() if len(ts) > 1))
if len(builds) >= 2:
    a, b = got[builds[0]], got[builds[1]]
    common = [r for r in a if r in b]
    eq = [r for r in common if a[r][1] == b[r][1]]
    print("%s against %s: rows with the same id %d; equal key texts %d of %d; equal key sets: %s" % (builds[0], builds[1], len(common), len(eq), len(common),
          sorted(k for k, _ in a.values()) == sorted(k for k, _ in b.values())))
    for r in common:
        if a[r][1] != b[r][1]: print("  differs %s: %s | %s" % (r, a[r][1], b[r][1]))
