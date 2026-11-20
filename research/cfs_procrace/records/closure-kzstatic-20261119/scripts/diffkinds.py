#!/usr/bin/env python3
"""diffkinds.py OLD NEW [--tsv] [--samples FILE] -- two text outputs of one build (OLD: the kept output of the fixed tool,
NEW: the output of the scratch check build) compared as multisets of lines; every line that is only in one of them is
put under a label that names its kind, so that the declaration can state the allowed differences per output kind and
field.  A check of the closure step (results section 20), not a tool of the method.

The label of a text line: its indentation (the number of leading spaces), then for a line that is one absolute path
`path .../<last component>`, else the text before the first `: ` or ` = `
when that comes within the first 72 characters, else the first two words; digits are written N, a hex run of 16 or more
characters H, a bracketed part `[..]`, and a row id like A12 or B3 `<row>`.  With --tsv the label is the first column with digits as N (edges.tsv,
pair files: the edge id or row key) and the tenth column when the line has one (the value of a pair line).
Two masks are applied before the comparison, in every file kind, because they follow from the tool alone: the `facts
<hex>` line and the `tool kz-static ...` line are each written as their label only (a line of that kind that is in one
file and not in the other is still listed).

Output: line 1 `diffkinds old_lines=<n> new_lines=<n> only_old=<n> only_new=<n> labels=<n>`; then one line per label in
the order of first appearance (NEW, then OLD): `label removed=<r> added=<a> <label>`.  --samples FILE writes up to two
lines of each label and side (cut at 400 characters).  rc 0 when both files were read, 2 otherwise."""
import collections
import re
import sys


def label(line, tsv):
    if tsv:
        cols = line.split("\t")
        base = re.sub(r"\d+", "N", cols[0])
        if len(cols) >= 10:
            v = cols[9]
            v = v.split("(")[0]
            base += "\t" + v
        return base
    ind = len(line) - len(line.lstrip(" "))
    t = line.lstrip(" ")
    if t.startswith("/") and " " not in t:
        return "%d|path .../%s" % (ind, re.sub(r"\d+", "N", t.rsplit("/", 1)[-1]))
    cut = None
    for sep in (": ", " = "):
        i = t.find(sep)
        if 0 <= i <= 72 and (cut is None or i < cut):
            cut = i
    head = t[:cut] if cut is not None else " ".join(t.split(" ")[:2])
    head = re.sub(r"\[[^\]]*\]?", "[..]", head)
    head = re.sub(r"\b[ABC]\d+\b", "<row>", head)
    head = re.sub(r"[0-9a-f]{16,}", "H", head)
    head = re.sub(r"\d+", "N", head)
    return "%d|%s" % (ind, head)


def norm(line):
    if re.match(r"^facts [0-9a-f]{64}$", line):
        return "facts H"
    if line.startswith("tool kz-static "):
        return "tool kz-static T"
    return line


def read(path):
    with open(path, encoding="utf-8", errors="surrogateescape") as f:
        return [norm(l.rstrip("\n")) for l in f]


def main():
    a = sys.argv[1:]
    tsv = "--tsv" in a
    a = [x for x in a if x != "--tsv"]
    samples = None
    if "--samples" in a:
        i = a.index("--samples")
        samples = a[i + 1]
        del a[i:i + 2]
    if len(a) != 2:
        print(__doc__, file=sys.stderr)
        return 2
    try:
        old, new = read(a[0]), read(a[1])
    except OSError as e:
        print("diffkinds: %s" % e, file=sys.stderr)
        return 2
    co, cn = collections.Counter(old), collections.Counter(new)
    only_old = co - cn
    only_new = cn - co
    order, rem, add, smp = [], collections.Counter(), collections.Counter(), collections.defaultdict(list)
    for side, src, cnt in (("+", new, only_new), ("-", old, only_old)):
        left = collections.Counter(cnt)
        for l in src:
            if left[l] <= 0:
                continue
            left[l] -= 1
            k = label(l, tsv)
            if k not in rem and k not in add:
                order.append(k)
            (add if side == "+" else rem)[k] += 1
            if len(smp[(k, side)]) < 2:
                smp[(k, side)].append(l[:400])
    print("diffkinds old_lines=%d new_lines=%d only_old=%d only_new=%d labels=%d" % (len(old), len(new), sum(only_old.values()), sum(only_new.values()), len(order)))
    for k in order:
        print("label removed=%d added=%d %s" % (rem[k], add[k], k))
    if samples:
        with open(samples, "w", encoding="utf-8", errors="surrogateescape") as f:
            for k in order:
                for side in ("-", "+"):
                    for l in smp[(k, side)]:
                        f.write("%s %s\t%s\n" % (side, k, l))
    return 0


if __name__ == "__main__":
    sys.exit(main())
