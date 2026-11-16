#!/usr/bin/env python3
"""section_text_cmp.py KZDUMP OLD NEW -- the facts container of one build in the earlier option-on round (OLD: the semantics
step's sem_on_extract) and in this step's (NEW: rf_on_extract), section by section by record text (the record lines of
kz-dump, string references written as text; the `section` line with its crc32 is left out, as section_text_sha_2.py):
every section of OLD has the same record lines in NEW, except STRINGS and BASIS (may differ) and FACT_EXPRS, whose OLD
record lines must be the first lines of NEW's (the expression roots of RESULT_FLOW are appended after all the others);
NEW adds RESULT_FLOW and RESULT_EFFECT and no other section.  Prints one line; rc 0 when nothing else differs."""
import re
import subprocess
import sys

ALLOWED = {"STRINGS", "BASIS"}
NEW = ["RESULT_EFFECT", "RESULT_FLOW"]


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
    if len(sys.argv) != 4:
        print(__doc__, file=sys.stderr)
        return 2
    a, b = sections(sys.argv[1], sys.argv[2]), sections(sys.argv[1], sys.argv[3])
    same, diff = [], []
    for s in a:
        if s in b and a[s] == b[s]:
            same.append(s)
        elif s == "FACT_EXPRS" and s in b and b[s][:len(a[s])] == a[s]:
            same.append(s + "(prefix, +%d rows)" % (len(b[s]) - len(a[s])))
        else:
            diff.append(s)
    new = sorted(s for s in b if s not in a)
    ok = set(diff) <= ALLOWED and new == NEW and any(x.startswith("FACT_EXPRS") for x in same)
    print("%s %s: sections of the earlier round %d, the same record text %d (%s), differing %s, new %s (RESULT_FLOW %d, RESULT_EFFECT %d record lines)" %
          ("OK" if ok else "BAD", sys.argv[3], len(a), len(same), [x for x in same if x.startswith("FACT_EXPRS")], diff, new,
           len(b.get("RESULT_FLOW", [])), len(b.get("RESULT_EFFECT", []))))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
