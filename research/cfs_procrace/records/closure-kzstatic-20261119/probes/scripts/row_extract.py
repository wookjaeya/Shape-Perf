#!/usr/bin/env python3
# The lines of one row of a relations text, each with its line number (a reader's extract; no tool reads it).
#   row_extract.py RELATIONS ROW        e.g. row_extract.py relations.txt A21
import re, sys
rel, row = sys.argv[1], sys.argv[2]
on = False
for n, l in enumerate(open(rel, errors="replace"), 1):
    l = l.rstrip("\n")
    if re.match(r"^  [ABC]\d+ ", l):
        on = l.startswith("  %s " % row)
    elif not l.startswith("      "):
        on = False
    if on:
        print("%d: %s" % (n, l))
