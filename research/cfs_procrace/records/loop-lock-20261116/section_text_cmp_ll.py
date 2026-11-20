#!/usr/bin/env python3
"""section_text_cmp_ll.py KZDUMP OLD NEW [--new A,B,...] [--expect-prefix] -- the facts container of one build in an earlier
option-on round (OLD) and in the final option-on round of the loop flag and lock handle step (NEW: ll_final_extract), section by
section by record text (the record lines of kz-dump, string references written as text; the `section` line with its crc32 is
left out, as section_text_sha_2.py): every section of OLD has the same record lines in NEW, except STRINGS and BASIS (may
differ) and FACT_EXPRS, whose OLD record lines must be the first lines of NEW's (the expression roots of the sections written
after them are appended after all the others); NEW adds the sections --new names (default LOCK_HANDLES,LOOP_DEFS: those of
this step against the result flow round) and no other.  (A copy of the result flow step's section_text_cmp.py with the new
sections as an argument; that script is unchanged.)  Prints one line; rc 0 when nothing else differs."""
import subprocess
import sys

ALLOWED = {"STRINGS", "BASIS"}


def sections(kd, path):
    text = subprocess.run([kd, path], capture_output=True, check=True).stdout.decode("utf-8", "surrogateescape").split("\n")
    out, cur = {}, None
    for l in text:
        if l.startswith("section "):
            cur = l.split()[1]
            out[cur] = []
        elif cur is not None and l.startswith("  "):
            out[cur].append(l)
    return out


def main():
    args = sys.argv[1:]
    new_names = ["LOCK_HANDLES", "LOOP_DEFS"]
    if "--new" in args:
        i = args.index("--new")
        new_names = sorted(x for x in args[i + 1].split(",") if x)
        del args[i:i + 2]
    if len(args) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    a, b = sections(args[0], args[1]), sections(args[0], args[2])
    same, diff = [], []
    for s in a:
        if s in b and a[s] == b[s]:
            same.append(s)
        elif s == "FACT_EXPRS" and s in b and b[s][:len(a[s])] == a[s]:
            same.append(s + "(prefix, +%d rows)" % (len(b[s]) - len(a[s])))
        else:
            diff.append(s)
    new = sorted(s for s in b if s not in a)
    ok = set(diff) <= ALLOWED and new == new_names and "FACT_EXPRS" in a and any(x.startswith("FACT_EXPRS") for x in same)
    print("%s %s: sections of the earlier round %d, the same record text %d (%s), differing %s, new %s (%s)" %
          ("OK" if ok else "BAD", args[2], len(a), len(same), [x for x in same if x.startswith("FACT_EXPRS")], diff, new,
           ", ".join("%s %d record lines" % (n, len(b.get(n, []))) for n in new)))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
