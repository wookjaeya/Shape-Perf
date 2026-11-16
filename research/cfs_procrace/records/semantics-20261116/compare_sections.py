#!/usr/bin/env python3
"""compare_sections.py OLD NEW -- the per-section sha256 of two option-on rounds (section_sha.py outputs: OLD the types and
object access step's records/object-access-20261116/section_sha_on.txt, NEW this step's), build by build: every section
of OLD has the same count, crc32 and sha256 in NEW, except STRINGS (the strings of the new section and items are added)
and BASIS (the new items, the basis hash of extract.units_from); NEW may add SEMANTICS, and nothing else.  Prints one
line per build; rc 0 when nothing else differs."""
import collections
import re
import sys

ALLOWED = {"STRINGS", "BASIS"}


def read(path):
    out = collections.OrderedDict()
    for l in open(path):
        m = re.match(r"^(\S+) (\S+) count=(\d+) crc32=(\S+) sha256=([0-9a-f]{64})$", l.strip())
        if not m:
            continue
        b = re.search(r"/(c1_pre_tx|c1_post_tx|r2_core|c5_641|c5_642)/", m.group(1)).group(1)
        out.setdefault(b, collections.OrderedDict())[m.group(2)] = m.groups()[2:]
    return out


def main():
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    a, b = read(sys.argv[1]), read(sys.argv[2])
    bad = 0
    for build in a:
        x, y = a[build], b.get(build, {})
        same = [s for s in x if s in y and x[s] == y[s]]
        diff = [s for s in x if s not in y or x[s] != y[s]]
        new = [s for s in y if s not in x]
        ok = set(diff) <= ALLOWED and set(new) <= {"SEMANTICS"} and "SEMANTICS" in new
        bad += 0 if ok else 1
        print("%s %s: sections equal %d, differing %s, new %s (SEMANTICS count %s)" % ("OK" if ok else "BAD", build, len(same), diff, new,
                                                                                         y.get("SEMANTICS", ("-",))[0]))
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
