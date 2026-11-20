#!/usr/bin/env python3
# Disk: kept output directories under the redesign root whose files are byte-identical to files of another kept
# directory (same relative path below the directory, same sha256).  Reads only; prints per directory the bytes
# whose identical copy exists in an earlier-named reference directory.
import os, hashlib, collections, sys
W = "/home/user/work/redesign/obs1"
dirs = sorted(d for d in os.listdir(W) if os.path.isdir(os.path.join(W, d)))
def sha(p):
    h = hashlib.sha256()
    with open(p, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""): h.update(b)
    return h.hexdigest()
idx = {}
for d in dirs:
    m = {}
    for root, _, fs in os.walk(os.path.join(W, d)):
        for f in fs:
            p = os.path.join(root, f)
            if os.path.islink(p): continue
            m[os.path.relpath(p, os.path.join(W, d))] = (os.path.getsize(p), p)
    idx[d] = m
hc = {}
def h(p):
    if p not in hc: hc[p] = sha(p)
    return hc[p]
mtime = {d: os.path.getmtime(os.path.join(W, d)) for d in dirs}
order = sorted(dirs, key=lambda d: mtime[d])
print("# directory | files | bytes | reference directory (older) | files identical at the same relative path | identical bytes | files not identical")
tot = 0
for i, d in enumerate(order):
    best = None
    for r in order[:i]:
        common = [k for k in idx[d] if k in idx[r] and idx[d][k][0] == idx[r][k][0]]
        if len(common) < max(1, len(idx[d]) // 2): continue
        same = [k for k in common if h(idx[d][k][1]) == h(idx[r][k][1])]
        b = sum(idx[d][k][0] for k in same)
        if best is None or b > best[1]: best = (r, b, len(same))
    n = len(idx[d]); by = sum(v[0] for v in idx[d].values())
    if best and best[1] > 0:
        print("%s | %d | %d | %s | %d | %d | %d" % (d, n, by, best[0], best[2], best[1], n - best[2]))
        if best[2] == n: tot += by
print("# bytes of directories whose every file is identical to the reference directory's file: %d" % tot)
