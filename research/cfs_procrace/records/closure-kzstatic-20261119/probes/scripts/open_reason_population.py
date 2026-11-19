#!/usr/bin/env python3
# Population of the open reason the stored-field call-result rule answers ("... takes the address of field:X ..."),
# per build, in kept relations texts: rows whose text carries the reason, and the distinct (callee, call position,
# field) triples.  Reads only.   open_reason_population.py ORDER_ROOT BUILD...
import re, sys
root = sys.argv[1]
print("# build | rows whose text carries the reason | distinct call sites (callee, position, field) | the sites")
for b in sys.argv[2:]:
    rows = 0; sites = set()
    for l in open("%s/%s/relations.txt" % (root, b), errors="replace"):
        hits = re.findall(r"(\S+) at (\S+) takes the address of (field:[A-Za-z_0-9.]+)", l)
        if hits:
            rows += 1
            sites.update(hits)
    print("%s | %d | %d | %s" % (b, rows, len(sites), "; ".join("%s at %s %s" % s for s in sorted(sites)) or "-"))
