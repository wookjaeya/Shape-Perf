#!/usr/bin/env python3
# P6: which use-site keys of the kept relations would move if positions outside the bundle root were written by
# the source-tree token the type names already use ("<tree>/<path in tree>", the nearest directory above the file
# that holds a .git entry), and whether the same relation then has one key text in the defect and the fixed build.
import re, os, sys, hashlib, collections
W = "/home/user/work/redesign/obs1"
builds = ["c1_pre_tx", "c1_post_tx", "r2_core", "c5_641", "c5_642"]
cache = {}
def tok(path):
    if path in cache: return cache[path]
    out = path; d = os.path.dirname(path)
    while d and d != "/":
        if os.path.exists(os.path.join(d, ".git")):
            out = "<" + os.path.basename(d) + ">/" + os.path.relpath(path, d); break
        d = os.path.dirname(d)
    cache[path] = out; return out
absre = re.compile(r"(?<![A-Za-z0-9_.>-])/[A-Za-z0-9_./+-]+\.[ch]\b")
texts = {}
print("# build | rows | keys whose text holds an absolute position | by path A/B/C | of them with a source tree (.git above the file) | without | distinct absolute files")
for b in builds:
    rows = []
    for l in open("%s/ll_final_order/%s/keys.txt" % (W, b), errors="replace"):
        p = l.rstrip("\n").split(" ", 2)
        if len(p) == 3 and re.match(r"[ABC]\d+$", p[0]): rows.append(p)
    moved = []; files = set(); notree = 0
    newtext = {}
    for rid, key, text in rows:
        fs = absre.findall(text)
        nt = text
        for f in fs:
            t = tok(f); files.add(f)
            if t == f: notree += 1
            nt = nt.replace(f, t)
        newtext[rid] = (key, text, nt)
        if fs: moved.append(rid)
    texts[b] = newtext
    c = collections.Counter(r[0] for r in moved)
    print("%s | %d | %d | A %d B %d C %d | %d | %d | %d" % (b, len(rows), len(moved), c["A"], c["B"], c["C"], len(moved) - (1 if notree else 0) * 0 - sum(1 for r in moved if any(tok(f) == f for f in absre.findall(texts[b][r][1]))), sum(1 for r in moved if any(tok(f) == f for f in absre.findall(texts[b][r][1]))), len(files)))
# the same relation in the two C1 builds
a, p = texts["c1_pre_tx"], texts["c1_post_tx"]
old_a = set(v[1] for v in a.values()); old_p = set(v[1] for v in p.values())
new_a = set(v[2] for v in a.values()); new_p = set(v[2] for v in p.values())
print("# c1_pre_tx against c1_post_tx: key texts equal today %d of %d / %d; with the source-tree token %d of %d / %d" % (
    len(old_a & old_p), len(old_a), len(old_p), len(new_a & new_p), len(new_a), len(new_p)))
only_a = sorted(new_a - new_p); only_p = sorted(new_p - new_a)
print("# key texts still only in one build after the token: defect build %d, fixed build %d" % (len(only_a), len(only_p)))
for t in only_a[:6]: print("   defect only: " + t[:230])
for t in only_p[:6]: print("   fixed only:  " + t[:230])
for b in ("c1_pre_tx", "c1_post_tx"):
    for rid, (key, text, nt) in texts[b].items():
        if "initialize_throttling" in text and text.startswith("A|"):
            print("# probe row %s %s key %s\n   today: %s\n   token: %s" % (b, rid, key, text, nt))
dup = collections.Counter(v[2] for v in a.values()); print("# c1_pre_tx: new key texts that collide: %d" % sum(1 for k, v in dup.items() if v > 1))
