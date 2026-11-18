#!/usr/bin/env python3
# What the present reader of the name-space data does with a line: a mirror of the three tests of
# kairos/lib/llvm/plan_gen.cpp read_namespaces (the `file` and operation expressions and the two word sets, :443-446;
# the refusals :469 and :481).  A mirror in Python is a reading aid, not the tool: the kz-static step checks the real
# reader.  Input: lines on stdin.  Output per line: skipped | file | entry | operation | REFUSED(<text>).
import re, sys
re_file = re.compile(r"^file (\S+) sha256=([0-9a-f]{64})$")
re_op = re.compile(r"^  (lookup|create|copy|create-check|delete|clear) (\S+) def=(\S+):(\d+)(?: at=(\S+):(\d+))?(?: public=\S+)?$")
other = {"semantics-data", "layer", "component", "rule", "generation", "compare", "chain-status", "end"}
other_sub = {"stored-name", "extent", "other-store", "compare"}
cur = False
for l in sys.stdin:
    l = l.rstrip("\n")
    if not l or l[0] == "#":
        r = "skipped (empty or comment)"
    elif re_file.match(l):
        r = "file"
    else:
        w = l.split(" ")[0]
        if w in ("kind", "table"):
            cur = True; r = "entry"
        elif re_op.match(l):
            r = "operation" if cur else "REFUSED(an operation before its kind or table)"
        else:
            sub = l[2:].split(" ")[0] if len(l) > 2 and l[:2] == "  " else ""
            r = "skipped (statement the reader does not use)" if (w in other or (sub and sub in other_sub)) else "REFUSED(is no statement of the namespace data)"
    print("%-50s | %s" % (r, l))
