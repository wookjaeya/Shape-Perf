#!/usr/bin/env python3
"""normcmp.py OLD_DIR NEW_DIR: the probe outputs of the same scripts compared after masking what follows from
the input change alone (view line numbers, relations line numbers, a leading line number of a listed row, the input directory names).  Prints per file
`same`, `same-after-mask` or `differs` with the differing masked lines (old '<', new '>')."""
import os, re, sys
def mask(l):
    l = l.replace("ll_final_order", "<order-dir>").replace("rf2_final_order", "<order-dir>")
    l = l.replace("mv_plans_v4", "<plan-dir>").replace("rf2_plans_v4", "<plan-dir>")
    l = re.sub(r"view lines? \[?[0-9][0-9, -]*\]?", "view line <n>", l)
    l = re.sub(r"\(view line \[[0-9, ]*\]\)", "(view line <n>)", l)
    l = re.sub(r"relations line [0-9]+", "relations line <n>", l)
    l = re.sub(r"view row [0-9]+", "view row <n>", l)
    l = re.sub(r"^([ ]*(?:[a-z -]+: )?)[0-9]+:", r"\1<n>:", l)
    return l
old, new = sys.argv[1], sys.argv[2]
for n in sorted(os.listdir(new)):
    a = open(os.path.join(old, n), errors="replace").read().split("\n")
    b = open(os.path.join(new, n), errors="replace").read().split("\n")
    if a == b: print("same", n); continue
    ma, mb = [mask(x) for x in a], [mask(x) for x in b]
    if ma == mb: print("same-after-mask", n); continue
    sa, sb = set(ma), set(mb)
    print("differs", n, "old-only", len([x for x in ma if x not in sb]), "new-only", len([x for x in mb if x not in sa]))
    for x in ma:
        if x not in sb: print("  <", x[:400])
    for x in mb:
        if x not in sa: print("  >", x[:400])
