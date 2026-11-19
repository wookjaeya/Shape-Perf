#!/usr/bin/env python3
"""open_other.py VIEW -- the open rows of a `kz-static show --section resulteffect` view whose reason has a part that the
header's reason classes of relations rules revision 14 (relations.cpp sf_class) put under `other`: the rows by the kind of
those parts, then the parts by their head (digits as N, positions and names cut after the first 6 words), with the number of rows that have such a part.
The reason text is the text in the parentheses after ` open (` on a row line.  A check of results section 20 (lead's
ruling (e)), not a tool of the method; it reads the view only."""
import collections, re, sys
def cls(p):
    if p.startswith("on a path through a branch this value does not decide"): return "branch"
    if " takes the address of " in p: return "address"
    if p.startswith("the outcome is "): return "outcome"
    if p.startswith("the values the callee returns are not known"): return "callee"
    return "other"
def reason(l):
    i = l.find(" open (")
    if i < 0: return None
    j, d = i + 7, 1
    k = j
    while k < len(l) and d:
        if l[k] == "(": d += 1
        elif l[k] == ")": d -= 1
        k += 1
    return l[j:k - 1]
def kind(p):
    if p.startswith("returns ") and "neither a constant nor this result" in p: return "a return of a value that is neither a constant nor the call's result"
    if p.count(")") > p.count("("): return "the tail of a parenthesised text cut at its '; ' (the part before it is of another class)"
    return "rest"
heads, rows, opened = collections.Counter(), 0, 0
kinds, only = collections.Counter(), collections.Counter()
for l in open(sys.argv[1], errors="replace"):
    if not l.startswith("  site "): continue
    r = reason(l.rstrip("\n"))
    if r is None: continue
    opened += 1
    parts = r.split("; ")
    oth = [p for p in parts if cls(p) == "other"]
    if not oth: continue
    rows += 1
    ks = set(kind(q) for q in oth)
    for k in ks: kinds[k] += 1
    if len(ks) == 1: only[next(iter(ks))] += 1
    for p in set(" ".join(re.sub(r"\d+", "N", q).split(" ")[:6]) for q in oth): heads[p] += 1
print("open rows %d; rows with a part of class other %d; heads %d" % (opened, rows, len(heads)))
for k, n in kinds.most_common(): print("kind rows %d (with no other kind of other part %d): %s" % (n, only[k], k))
for h, n in heads.most_common(): print("%6d  %s" % (n, h))
