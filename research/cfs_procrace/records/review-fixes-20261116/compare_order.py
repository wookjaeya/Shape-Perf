#!/usr/bin/env python3
"""compare_order.py OLD_DIR NEW_DIR -- `kz-static order` of one build before (order rules revision 1) and after (revision 2)
the review fixes: order.txt and edges.tsv, with the differences the declaration allows (plan/full_stage_results_20261116.md
6.0 (나)).

order.txt
  head     line 1 (order rules revision 1 -> 2), line 3 (relations rules revision 8 -> 9), the `closure:` line (the
           caller-dependent sentence added), the `summary edges listed` line and the `summary spawn calls` line; every
           other head line the same.
  [edges]  an edge of both texts (by id): an HB-spawn edge may differ only in its `child without success:` line and a new
           `caller-dependent:` line; an HB-lock edge only in its `condition:` line, and there only after
           `child without success: `; every other edge the same.  An edge of one text only must be an HB-seq hard step (the
           listed steps follow the listed pairs).
  [waits]  the same.
  [rows]   the same rows (id, key) in the same order; a row's value may change only if every old pair whose value differs
           in the new listing had a path with a caller-dependent HB-spawn step from a thread the new edge note does not
           allow; within a row, the pair lines and the `pairs:` line may change, the other lines may differ only in the
           `child without success: ` tail of an HB-lock condition they quote.
  checks   (a) no listed hard_before or fixed_early_proved pair of the new text takes a caller-dependent HB-spawn edge from a
           thread its note does not allow; (b) a pair listed in both texts with another value: the old value was hard_before
           or fixed_early_proved and its old path had such a step; a pair unordered or unknown in the old text is not
           ordered in the new one.
edges.tsv  the header the same; rows by id: HB-spawn, L0-script and observed-only rows the same; HB-lock rows may differ only
           in the condition column after `child without success: `; HB-seq rows may be in one file only.
Prints the counts and up to 10 undeclared lines; rc 0 when nothing undeclared, 1 otherwise, 2 on a usage error."""
import collections
import os
import re
import sys

ACT = re.compile(r"([^\s{},]+ \((?:process|script-entry|spawn-entry|table-image-entry|table-walk-entry|[a-z-]+)\))")
PAIR = re.compile(r"      pair: (.*): (hard_before|fixed_early_proved|conditional_before|fixed_early_candidate|unordered|unknown)(?:: |; path |$)(.*)$")


def read(p):
    return open(p, encoding="utf-8", errors="surrogateescape").read().split("\n")


def split(lines):
    head, edges, waits, rows, cur = [], collections.OrderedDict(), [], [], "head"
    eid = None
    for l in lines:
        if l.startswith("[edges] "):
            cur = "edges"; continue
        if l.startswith("[waits] "):
            cur = "waits"; waits.append(l); continue
        if l.startswith("[rows] "):
            cur = "rows"; rows.append(l); continue
        if cur == "head":
            head.append(l)
        elif cur == "edges":
            m = re.match(r"  (E[0-9a-f]{16}) (\S+) (\S+)$", l)
            if m:
                eid = m.group(1); edges[eid] = [l]
            elif eid is not None:
                edges[eid].append(l)
        elif cur == "waits":
            waits.append(l)
        else:
            rows.append(l)
    return head, edges, waits, rows


def edge_meta(edges):
    meta = {}
    for eid, ls in edges.items():
        rule = ls[0].split()[1]
        to = next((ACT.findall(l)[-1] for l in ls if l.startswith("      to: ") and ACT.findall(l)), None)
        allowed = None
        for l in ls:
            if l.startswith("      caller-dependent: "):
                t = l.split("the closure follows this edge only from ", 1)[1]
                allowed = set(ACT.findall(t.split("this thread: ", 1)[1])) if "this thread: " in t else set()
        meta[eid] = (rule, to, allowed)
    return meta


def bad_step(head, value, path, old_meta, new_meta):
    """the first caller-dependent HB-spawn step of a hard/fixed path from a thread the NEW note does not allow"""
    ids = re.findall(r"(E[0-9a-f]{16}) ([A-Za-z0-9-]+)", path.split(";")[0])
    acts = ACT.findall(head)
    if not acts:
        return None
    prev = acts[0] if value == "hard_before" else acts[-1]
    for eid, rule in ids:
        if rule == "HB-spawn":
            al = new_meta.get(eid, (None, None, None))[2]
            if al is not None and prev not in al:
                return "%s from %s" % (eid, prev)
        nxt = old_meta.get(eid, (None, None, None))[1]
        prev = nxt or prev
    return None


def rows_of(rows):
    out = collections.OrderedDict()
    cur = None
    for l in rows[1:]:
        m = re.match(r"  ([ABC]\d+) key (\S+) order\.static (\S+)$", l)
        if m:
            cur = (m.group(1), m.group(2)); out[cur] = {"value": m.group(3), "lines": []}
            continue
        if cur is not None:
            out[cur]["lines"].append(l)
    return out


def strip_cws(l):
    return re.sub(r"child without success: .*?(?=; applies when |$)", "child without success: <flag>", l)


def main():
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    od, nd = sys.argv[1], sys.argv[2]
    oh, oe, ow, orows = split(read(os.path.join(od, "order.txt")))
    nh, ne, nw, nrows = split(read(os.path.join(nd, "order.txt")))
    om, nm = edge_meta(oe), edge_meta(ne)
    bad = []
    c = collections.Counter()
    # head
    allowed_head = (r"kz-static order \(order rules revision [12] \(2026-11-16\)\)", r"relations relations rules revision [89] \(2026-11-16\)",
                    r"closure: .*", r"summary edges listed .*", r"summary spawn calls with resolved threads .*")
    if len(oh) != len(nh):
        bad.append("head line count %d / %d" % (len(oh), len(nh)))
    for x, y in zip(oh, nh):
        if x != y:
            if any(re.fullmatch(p, x) and re.fullmatch(p, y) for p in allowed_head):
                c["head allowed"] += 1
            else:
                bad.append("head: - %s | + %s" % (x[:150], y[:150]))
    # edges
    for eid in set(oe) | set(ne):
        if eid in oe and eid in ne:
            x, y = oe[eid], ne[eid]
            if x == y:
                continue
            rule = om[eid][0]
            if rule == "HB-spawn":
                xs = [l for l in x if not l.startswith("      child without success: ")]
                ys = [l for l in y if not l.startswith("      child without success: ") and not l.startswith("      caller-dependent: ")]
                ok = xs == ys
            elif rule == "HB-lock":
                ok = [strip_cws(l) for l in x] == [strip_cws(l) for l in y]
            else:
                ok = False
            if ok:
                c["edge changed " + rule] += 1
            else:
                bad.append("edge %s %s changed beyond the declared lines" % (eid, rule))
        else:
            side = oe if eid in oe else ne
            rule, kind = side[eid][0].split()[1], side[eid][0].split()[2]
            if rule == "HB-seq" and kind == "hard":
                c["edge only %s HB-seq" % ("old" if eid in oe else "new")] += 1
            else:
                bad.append("edge %s %s %s only in the %s text" % (eid, rule, kind, "old" if eid in oe else "new"))
    if ow != nw:
        bad.append("waits differ")
    # rows
    ro, rn = rows_of(orows), rows_of(nrows)
    if list(ro) != list(rn):
        bad.append("rows differ in id or key or order")
    for k in ro:
        if k not in rn:
            continue
        a, b = ro[k], rn[k]
        xo = [strip_cws(l) for l in a["lines"] if not l.startswith("      pair: ") and not l.startswith("      pairs: ")]
        yo = [strip_cws(l) for l in b["lines"] if not l.startswith("      pair: ") and not l.startswith("      pairs: ")]
        if xo != yo:
            bad.append("row %s: lines other than the pairs differ" % k[0])
        op = {}
        for l in a["lines"]:
            m = PAIR.match(l)
            if m:
                op[m.group(1)] = (m.group(2), m.group(3))
        changed_pairs_ok = True
        for l in b["lines"]:
            m = PAIR.match(l)
            if not m:
                continue
            h, v, rest = m.group(1), m.group(2), m.group(3)
            if v in ("hard_before", "fixed_early_proved"):
                s = bad_step(h, v, rest, nm, nm)
                if s:
                    bad.append("row %s: new %s pair takes %s" % (k[0], v, s))
            if h in op and op[h][0] != v:
                ov, orest = op[h]
                c["pair value changed %s -> %s" % (ov, v)] += 1
                if ov not in ("hard_before", "fixed_early_proved") or not bad_step(h, ov, orest, om, nm):
                    changed_pairs_ok = False
                    bad.append("row %s: pair changed %s -> %s without a disallowed step in the old path: %s" % (k[0], ov, v, h[:160]))
        if a["value"] != b["value"]:
            c["row value changed"] += 1
            if not changed_pairs_ok:
                bad.append("row %s value %s -> %s" % (k[0], a["value"], b["value"]))
        if a["lines"] != b["lines"]:
            c["row lines changed"] += 1
    for l in nrows:
        m = PAIR.match(l)
        if m and m.group(2) in ("hard_before", "fixed_early_proved"):
            c["new listed hard/fixed pairs"] += 1
    for l in orows:
        m = PAIR.match(l)
        if m and m.group(2) in ("hard_before", "fixed_early_proved"):
            c["old listed hard/fixed pairs"] += 1
            if bad_step(m.group(1), m.group(2), m.group(3), om, nm):
                c["old listed hard/fixed pairs with a disallowed step"] += 1
    # edges.tsv
    ot = read(os.path.join(od, "edges.tsv")); nt = read(os.path.join(nd, "edges.tsv"))
    if ot[0] != nt[0]:
        bad.append("edges.tsv header differs")
    to = {l.split("\t")[0]: l.split("\t") for l in ot[1:] if l}
    tn = {l.split("\t")[0]: l.split("\t") for l in nt[1:] if l}
    for eid in set(to) | set(tn):
        if eid in to and eid in tn:
            x, y = to[eid], tn[eid]
            if x == y:
                continue
            if x[1] == "HB-lock" and x[:7] == y[:7] and x[8:] == y[8:] and strip_cws(x[7]) == strip_cws(y[7]):
                c["tsv HB-lock condition"] += 1
            else:
                bad.append("edges.tsv %s %s differs" % (eid, x[1]))
        else:
            r = (to.get(eid) or tn.get(eid))
            if r[1] == "HB-seq" and r[2] == "hard":
                c["tsv only %s HB-seq" % ("old" if eid in to else "new")] += 1
            else:
                bad.append("edges.tsv %s %s only in one file" % (eid, r[1]))
    print("compare_order: " + ", ".join("%s %d" % kv for kv in sorted(c.items())) + "; undeclared %d" % len(bad))
    for l in bad[:10]:
        print("  " + l[:400])
    return 0 if not bad else 1


if __name__ == "__main__":
    sys.exit(main())
