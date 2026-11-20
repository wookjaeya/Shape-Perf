#!/usr/bin/env python3
"""expect_changes.py -- the list of what changed between two kz-expect 1 files (results 26.4): one line per expectation
key (branch, build, row key, root, field) with the old value, the new value, the new class and the change (same, changed,
added, removed), and a tail of counts per field and change.  Reads only the two files; run with python3 -I.

    expect_changes.py OLD NEW OUT
"""
import sys


def load(p):
    out = {}
    for l in open(p, encoding="utf-8"):
        if l.startswith("expect\t"):
            r = l.rstrip("\n").split("\t")
            out[tuple(r[1:6])] = (r[6], r[7] if len(r) > 7 else "-")
    return out


def main(argv):
    if len(argv) != 3:
        print(__doc__, file=sys.stderr)
        return 64
    old, new = load(argv[0]), load(argv[1])
    rows, cnt = [], {}
    for k in sorted(set(old) | set(new)):
        o, n = old.get(k), new.get(k)
        ch = "added" if o is None else "removed" if n is None else "same" if o[0] == n[0] else "changed"
        rows.append("\t".join(list(k) + [o[0] if o else "-", n[0] if n else "-", n[1] if n else "-", ch]))
        cnt[(k[4].split("[")[0] if k[4].startswith("pair[") else k[4], ch)] = cnt.get((k[4].split("[")[0] if k[4].startswith("pair[") else k[4], ch), 0) + 1
    with open(argv[2], "w", encoding="utf-8") as f:
        f.write("# expect_changes.py: %s -> %s\n" % (argv[0].rsplit("/", 1)[-1], argv[1].rsplit("/", 1)[-1]))
        f.write("branch\tbuild\trow_key\troot\tfield\told\tnew\tclass\tchange\n")
        f.write("\n".join(rows) + "\n")
        for (fld, ch), v in sorted(cnt.items()):
            f.write("# count\t%s\t%s\t%d\n" % (fld, ch, v))
        tot = {}
        for (fld, ch), v in cnt.items():
            tot[ch] = tot.get(ch, 0) + v
        f.write("# total\t" + "\t".join("%s %d" % x for x in sorted(tot.items())) + "\n")
    print("changes: " + ", ".join("%s %d" % x for x in sorted(tot.items())))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
