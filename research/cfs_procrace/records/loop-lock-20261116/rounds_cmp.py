#!/usr/bin/env python3
"""rounds_cmp.py ROUND_SHA... FINAL_SHA -- (plan 4.3: the intermediate re-extract rounds are not kept compressed; each kept the
record-text sha256 of every section, and the final option-on round decides) each intermediate option-on round's kept
per-section record-text sha256 (section_text_sha_2.py lines `<file> <section> count=<n> text_sha256=<h> lines=<k>`) against
the final round's, build by build (the build is the directory after the round's name).  Per round and build: the sections
of the round, those with the same record text in the final round, those that differ, and those the final round lacks.  A
difference is explained when it is BASIS (the items of the later options and the tool's identity) or FACT_EXPRS (the
expression roots the later sections append after the earlier rows: the prefix is checked from the containers by
section_text_cmp_ll.py in the step `rounds`); any other difference, or a section the final round lacks, is not.
Prints a summary line and one line per round and build; rc 0 when every difference is explained, 1 otherwise."""
import re
import sys

EXPLAINED = {"BASIS", "FACT_EXPRS"}


def load(path):
    out = {}
    for l in open(path, encoding="utf-8"):
        m = re.match(r"^(\S+) (\S+) count=(\d+) text_sha256=([0-9a-f]{64}) lines=(\d+)$", l.strip())
        if not m:
            continue
        f = m.group(1)
        mm = re.search(r"/obs1/([^/]+)/([^/]+)/", f)
        rnd, b = (mm.group(1), mm.group(2)) if mm else ("?", f)
        out.setdefault((rnd, b), {})[m.group(2)] = (m.group(4), int(m.group(3)))
    return out


def main():
    if len(sys.argv) < 3:
        print(__doc__, file=sys.stderr)
        return 2
    final = load(sys.argv[-1])
    fin_by_build = {b: s for (r, b), s in final.items()}
    lines, ok_all = [], True
    for p in sys.argv[1:-1]:
        for (rnd, b), secs in sorted(load(p).items()):
            f = fin_by_build.get(b)
            if f is None:
                lines.append("%s %s: no final round for this build" % (rnd, b))
                ok_all = False
                continue
            same = [s for s in secs if s in f and f[s][0] == secs[s][0]]
            diff = sorted(s for s in secs if s in f and f[s][0] != secs[s][0])
            lost = sorted(s for s in secs if s not in f)
            unexpl = [s for s in diff if s not in EXPLAINED] + lost
            ok = not unexpl
            ok_all &= ok
            lines.append("%s %s %s: sections %d, the same record text %d, differing %s, not in the final round %s%s" % (
                "OK" if ok else "BAD", rnd, b, len(secs), len(same), diff, lost, "" if ok else " (not explained: %s)" % unexpl))
    print("%s: %d rounds x builds compared with the final round; every difference %s" % ("OK" if ok_all else "BAD", len(lines), "explained (BASIS, FACT_EXPRS)" if ok_all else "NOT explained"))
    for l in lines:
        print(l)
    return 0 if ok_all else 1


if __name__ == "__main__":
    sys.exit(main())
