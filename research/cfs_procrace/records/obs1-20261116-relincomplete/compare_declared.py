#!/usr/bin/env python3
# compare_declared.py -- the byte comparison declared before the build of relations rules revision 6.
#
#   python3 -I compare_declared.py relations OLD NEW        one relations text (or a relations golden) against its reference
#   python3 -I compare_declared.py keys OLD NEW             one use-site key listing against its reference
#
# relations: the declared changes are (1) the line "rules relations rules revision 5 (2026-10-30)" becomes
# "rules relations rules revision 6 (2026-11-16)", (2) one new line "summary incomplete (" right after the line
# "summary A ", (3) one new line "      incomplete: " right after every "      layers: " line of a row.  NEW with these
# three undone must equal OLD byte for byte; the script also checks that every row has one such line and that every A
# row whose first line ends in "key unresolved" names a key_unresolved reason other than the creator's key.
# keys: line 3 (the rules revision) may change; every other line must be equal.
# Prints one line "SAME ..." or "DIFF ..." and exits 0 / 1.
import re
import sys

OLD_RULES = "rules relations rules revision 5 (2026-10-30)"
NEW_RULES = "rules relations rules revision 6 (2026-11-16)"
ROW = re.compile(r"^  [ABC][0-9]+ ")


def relations(old: bytes, new: bytes) -> str:
    n = new.decode("utf-8", "surrogateescape").split("\n")
    out, problems = [], []
    rows = incs = summaries = 0
    a_unres_ok = a_unres = 0
    head = None
    i = 0
    while i < len(n):
        line = n[i]
        if line == NEW_RULES:
            out.append(OLD_RULES)
        elif line.startswith("summary incomplete ("):
            summaries += 1
            if not (out and out[-1].startswith("summary A ")):
                problems.append("summary incomplete not after the summary A line (line %d)" % (i + 1))
        elif line.startswith("      incomplete: "):
            incs += 1
            if not (out and out[-1].startswith("      layers: ")):
                problems.append("incomplete line not after a layers line (line %d)" % (i + 1))
            if head is not None and head.startswith("  A") and head.endswith(" key unresolved"):
                a_unres += 1
                items = line[len("      incomplete: "):].split("; ")
                if any(x.startswith("key_unresolved(") and x != "key_unresolved(creator key)" for x in items):
                    a_unres_ok += 1
            head = None
        else:
            if ROW.match(line):
                rows += 1
                head = line
            out.append(line)
        i += 1
    if summaries != 1:
        problems.append("summary incomplete lines: %d" % summaries)
    if incs != rows:
        problems.append("rows %d, incomplete lines %d" % (rows, incs))
    if a_unres != a_unres_ok:
        problems.append("A rows with key unresolved %d, of them with a key_unresolved reason %d" % (a_unres, a_unres_ok))
    same = "\n".join(out).encode("utf-8", "surrogateescape") == old
    if not same:
        problems.append("the text with the declared lines undone differs from the reference")
    tag = "SAME" if not problems else "DIFF"
    return "%s rows %d incomplete-lines %d a-key-unresolved %d/%d%s" % (tag, rows, incs, a_unres_ok, a_unres, "" if not problems else " : " + "; ".join(problems))


def keys(old: bytes, new: bytes) -> str:
    o = old.split(b"\n")
    n = new.split(b"\n")
    if len(o) != len(n):
        return "DIFF lines %d vs %d" % (len(o), len(n))
    diff = [k + 1 for k in range(len(o)) if o[k] != n[k]]
    ok = diff in ([], [3]) and (not diff or (o[2] == b"rules " + OLD_RULES[6:].encode() and n[2] == b"rules " + NEW_RULES[6:].encode()))
    return ("SAME" if ok else "DIFF") + " lines %d, differing lines %s" % (len(o), diff if diff else "none")


def main() -> int:
    if len(sys.argv) != 4 or sys.argv[1] not in ("relations", "keys"):
        print("usage: compare_declared.py relations|keys OLD NEW", file=sys.stderr)
        return 2
    old = open(sys.argv[2], "rb").read()
    new = open(sys.argv[3], "rb").read()
    r = relations(old, new) if sys.argv[1] == "relations" else keys(old, new)
    print(r)
    return 0 if r.startswith("SAME") else 1


if __name__ == "__main__":
    sys.exit(main())
