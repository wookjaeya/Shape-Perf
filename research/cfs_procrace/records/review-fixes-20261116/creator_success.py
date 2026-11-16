#!/usr/bin/env python3
"""creator_success.py EXPLAIN RELATIONS ROW... -- the success constant of each creator callee of the named path A rows
(row ids of RELATIONS, e.g. A20 A21), read by the functions of the explanation script EXPLAIN itself (parse_relations,
row_static, publisher_out): the rule of the creator's out-read of that script revision.  Writes one line per (row, creator
callee): the constants the rule takes, the lines it does not take, and whether a creator's out would be read under a
success constant.  Re-measures records/relrules-rev7-20261116/creator_success.txt (which applied revision 2's rule by
hand) after the review (plan/full_stage_results_20261116.md 6.7 rules-01)."""
import importlib.util
import sys


class Inst:
    def __init__(self, callee):
        self.callee = callee


def main():
    if len(sys.argv) < 4:
        print(__doc__, file=sys.stderr)
        return 2
    spec = importlib.util.spec_from_file_location("explain", sys.argv[1])
    ex = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(ex)
    text = open(sys.argv[2], encoding="utf-8", errors="replace").read()
    _, rows = ex.parse_relations(text)
    print("# %s; relations %s" % (ex.SCRIPT_REV, sys.argv[2]))
    for rid in sys.argv[3:]:
        row = rows.get(rid)
        if row is None:
            print("%s: no such row" % rid)
            continue
        st = ex.row_static(row, "relations.txt")
        callees = sorted(set(st["callee_consts"]) | set(st.get("callee_open", {})))
        for c in callees:
            cs = st["callee_consts"].get(c, [])
            succ = sorted({(v, nm) for v, nm, _ in cs if not ex.fail_class(v)})
            marked = sum(1 for _, t in row["body"] if t.startswith("callee-check: %s returns" % c) and "; not shown to reach the return:" in t)
            out = ex.publisher_out(st, Inst(c), 0, "0x1")
            print("%s: callee %s: constants taken %d (not of failure class %s); lines marked not shown to reach the return %d; %s; "
                  "a creator's out under status 0: %s (%s)" % (rid, c, len(cs), succ, marked,
                                                              "returns not all followed or not shown (%s)" % st["callee_open"][c] if c in st.get("callee_open", {}) else "set not marked open",
                                                              "read" if out.get("read") else "not read", out.get("why", out.get("under", ""))))
    return 0


if __name__ == "__main__":
    sys.exit(main())
