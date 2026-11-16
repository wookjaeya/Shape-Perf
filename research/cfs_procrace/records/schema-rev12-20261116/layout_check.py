#!/usr/bin/env python3
"""layout_check.py OLD NEW -- `kz-dump --layout` of schema revision 11 (OLD) against that of the new build (NEW): the first
line (kz-layout abi=1 revision=N) goes from 11 to 12, the last line (fingerprint=) changes, and every other line of OLD is in
NEW in the same order (NEW only adds lines).  Prints the counts of added lines by kind (record, field, enum, item, section,
format) and the names of the new sections; rc 0 when the rule holds, 1 otherwise."""
import re
import sys


def main():
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 64
    a = open(sys.argv[1]).read().split("\n")
    b = open(sys.argv[2]).read().split("\n")
    bad = 0
    if a[0] != "kz-layout abi=1 revision=11" or b[0] != "kz-layout abi=1 revision=12":
        print("BAD first lines: %r -> %r" % (a[0], b[0]))
        bad += 1
    fa = [x for x in a if x.startswith("fingerprint=")]
    fb = [x for x in b if x.startswith("fingerprint=")]
    if len(fa) != 1 or len(fb) != 1 or fa == fb:
        print("BAD fingerprint lines: %r -> %r" % (fa, fb))
        bad += 1
    old = [x for x in a[1:] if not x.startswith("fingerprint=")]
    new = [x for x in b[1:] if not x.startswith("fingerprint=")]
    j, added = 0, []
    for x in new:
        if j < len(old) and x == old[j]:
            j += 1
        else:
            added.append(x)
    if j != len(old):
        print("BAD old line %d is not in the new layout in order: %r" % (j + 1, old[j]))
        bad += 1
    kinds = {}
    for x in added:
        k = ("record" if x.startswith("record ") else "section" if x.startswith("section ") else "format" if x.startswith("format ")
             else "enum" if x.startswith("enum ") or x.startswith("flags ") else "item" if re.match(r"^  0x", x) else "field" if x.startswith("  ") else "other")
        kinds[k] = kinds.get(k, 0) + 1
    print("old lines kept in order: %d of %d; added %d: %s" % (j, len(old), len(added), dict(sorted(kinds.items()))))
    print("first line: %s -> %s" % (a[0], b[0]))
    print("fingerprint: %s -> %s" % (fa[0] if fa else "-", fb[0] if fb else "-"))
    for x in added:
        if x.startswith("section "):
            print("new " + x)
    print("layout_check: %s" % ("only additions" if bad == 0 else "%d failures" % bad))
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
