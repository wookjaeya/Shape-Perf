#!/usr/bin/env python3
"""compare_sections_text.py OLD NEW -- (results 8.0b) the section_text_sha.py outputs of the earlier option-on round
(OLD: objacc_on_extract) and of this one (NEW: sem_on_extract), build by build: every section of OLD has the same count
and record-text sha256 in NEW, except STRINGS (the pool: the strings of the new section and items are added) and BASIS
(the new items, the basis hash of extract.units_from); NEW adds SEMANTICS and nothing else.  Prints one line per build;
rc 0 when nothing else differs."""
import collections
import re
import sys

ALLOWED = {"STRINGS", "BASIS"}


def read(path):
    out = collections.OrderedDict()
    for l in open(path):
        m = re.match(r"^(\S+) (\S+) count=(\d+) text_sha256=([0-9a-f]{64}) lines=(\d+)$", l.strip())
        if not m:
            continue
        b = re.search(r"/(c1_pre_tx|c1_post_tx|r2_core|c5_641|c5_642)/", m.group(1)).group(1)
        out.setdefault(b, collections.OrderedDict())[m.group(2)] = (m.group(3), m.group(4))
    return out


def main():
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    a, b = read(sys.argv[1]), read(sys.argv[2])
    bad = 0 if set(a) == set(b) and len(a) == 5 else 1
    for build in a:
        x, y = a[build], b.get(build, {})
        same = [s for s in x if s in y and x[s] == y[s]]
        diff = [s for s in x if s not in y or x[s] != y[s]]
        new = [s for s in y if s not in x]
        ok = set(diff) <= ALLOWED and new == ["SEMANTICS"]
        bad += 0 if ok else 1
        print("%s %s: sections of the earlier round %d, the same record text %d, differing %s, new %s (SEMANTICS count %s)" %
              ("OK" if ok else "BAD", build, len(x), len(same), diff, new, y.get("SEMANTICS", ("-",))[0]))
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
