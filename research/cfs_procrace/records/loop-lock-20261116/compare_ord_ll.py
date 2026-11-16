#!/usr/bin/env python3
"""compare_ord_ll.py [--tsv] OLD NEW -- order.txt (or with --tsv edges.tsv) of one build: OLD from the option-on round of the
result flow step (rf_on_order), NEW from the final option-on round of the loop flag and lock handle step (ll_final_order),
with the differences the declaration allows (plan/full_stage_results_20261116.md 10.0, order rules revision 4):
  header   line 2 (the facts hash); the new line `rules with loop definitions and lock handles: order rules revision 4`;
           in `rules with result flow:` only the numbers; `summary edges listed` the total, HB-await and conditional,
           and the HB-seq steps the closures followed not fewer (the closures with conditional edges reach the ends of the
           HB-await edges; hard and every other rule the same); `summary waits` the recognised count and the dependent
           uses; every other header line the same
  edges    every OLD edge in NEW with the same lines, an HB-lock edge with one added `lock identity source:` line; the
           edges only in NEW are HB-await edges
  waits    the same waits with the same head; per wait the lines the same apart from the `predicate:` and `awaited:`
           lines and the consequence of a `use the call of` line; added `use caller-side:` lines
  rows     the same bytes
  --tsv    the header and every OLD line in NEW; the lines only in NEW are HB-await edges
Prints one summary line, then at most 20 differences not allowed; rc 0 when only allowed differences, 1 otherwise."""
import re
import sys


def read(p):
    return open(p, encoding="utf-8", errors="surrogateescape").read().split("\n")


def counts(line):
    m = re.search(r"\((.*)\)", line)
    out = {}
    if not m:
        return out
    for part in re.split(r"[;,] ", m.group(1)):
        mm = re.match(r"^(.*?) (\d+)$", part.strip())
        if mm:
            out[mm.group(1)] = int(mm.group(2))
    return out


def split(lines):
    head, edges, waits, rows = [], {}, {}, []
    cur, sec = None, "head"
    order = {"edges": [], "waits": []}
    for l in lines:
        m = re.match(r"^\[(edges|waits|rows)\] \d+$", l)
        if m:
            sec = m.group(1)
            cur = None
            if sec == "rows":
                rows.append(l)
            continue
        if sec == "head":
            head.append(l)
        elif sec in ("edges", "waits"):
            if l.startswith("  ") and not l.startswith("      "):
                key = l.split()[0]
                cur = key
                (edges if sec == "edges" else waits)[key] = [l]
                order[sec].append(key)
            elif cur is not None:
                (edges if sec == "edges" else waits)[cur].append(l)
        else:
            rows.append(l)
    return head, edges, waits, rows


def main():
    args = sys.argv[1:]
    tsv = "--tsv" in args
    args = [a for a in args if a != "--tsv"]
    if len(args) != 2:
        print(__doc__, file=sys.stderr)
        return 2
    a, b = read(args[0]), read(args[1])
    bad = []
    if tsv:
        sa, sb = set(a[1:]), set(b[1:])
        if a[0] != b[0]:
            bad.append("header differs")
        lost = sa - sb
        extra = [l for l in sb - sa if l]
        if lost:
            bad.append("%d OLD lines not in NEW" % len(lost))
        others = [l for l in extra if l.split("\t")[1:2] != ["HB-await"]]
        if others:
            bad.append("%d NEW lines that are no HB-await edge" % len(others))
        if bad:
            print("BAD: " + "; ".join(bad))
            return 1
        print("only declared differences: every OLD line in NEW, new HB-await lines %d" % len(extra))
        return 0
    ha, ea, wa, ra = split(a)
    hb, eb, wb, rb = split(b)
    # header
    hb2 = [l for l in hb if not l.startswith("rules with loop definitions and lock handles: ")]
    newhead = len(hb) - len(hb2)
    if newhead != 1 or not any(l.startswith("rules with loop definitions and lock handles: order rules revision 4 (2026-11-16)") for l in hb):
        bad.append("the new header line is not there once")
    if len(ha) != len(hb2):
        bad.append("header line counts differ (%d, %d)" % (len(ha), len(hb2)))
    else:
        for i, (x, y) in enumerate(zip(ha, hb2)):
            if i == 1 or x == y:
                continue
            if x.startswith("rules with result flow: ") and re.sub(r"\d+", "N", x) == re.sub(r"\d+", "N", y):
                continue
            if x.startswith("summary edges listed ") and y.startswith("summary edges listed "):
                cx, cy = counts(x), counts(y)
                keys = set(cx) | set(cy)
                if all(cx.get(k) == cy.get(k) for k in keys if k not in ("HB-await", "conditional")) and \
                   re.sub(r"\(.*\)", "", re.sub(r"listed \d+", "listed N", x)) == re.sub(r"\(.*\)", "", re.sub(r"listed \d+", "listed N", y)):
                    m1 = re.search(r"followed (\d+)", x)
                    m2 = re.search(r"followed (\d+)", y)
                    if (m1 and m2 and int(m2.group(1)) >= int(m1.group(1))) or (not m1 and not m2):
                        continue
            if x.startswith("summary waits ") and y.startswith("summary waits "):
                if re.sub(r"predicate recognised \d+\); dependent uses .*$", "", x) == re.sub(r"predicate recognised \d+\); dependent uses .*$", "", y):
                    continue
            bad.append("header line %d: %s | %s" % (i + 1, x[:150], y[:150]))
    # edges
    n_lockline = 0
    for k, blk in ea.items():
        if k not in eb:
            bad.append("edge %s not in NEW" % k)
            continue
        nb = [l for l in eb[k] if not l.startswith("      lock identity source: ")]
        n_lockline += len(eb[k]) - len(nb)
        if nb != blk:
            bad.append("edge %s differs" % k)
        if len(eb[k]) - len(nb) and not blk[0].split()[1:2] == ["HB-lock"]:
            bad.append("edge %s: a lock identity line on an edge that is no HB-lock" % k)
    new_edges = [k for k in eb if k not in ea]
    for k in new_edges:
        if eb[k][0].split()[1:2] != ["HB-await"]:
            bad.append("new edge %s is no HB-await edge" % k)
    # waits
    n_pred = n_aw = n_use = n_cs = 0
    if set(wa) != set(wb):
        bad.append("the waits differ (%d, %d)" % (len(wa), len(wb)))
    for k in wa:
        if k not in wb:
            continue
        x = wa[k]
        y = [l for l in wb[k] if not l.startswith("      use caller-side: ")]
        n_cs += len(wb[k]) - len(y)
        if len(x) != len(y) or x[0] != y[0]:
            bad.append("wait %s: lines %d / %d or head differs" % (k, len(x), len(y)))
            continue
        for p, q in zip(x, y):
            if p == q:
                continue
            if p.startswith("      predicate: ") and q.startswith("      predicate: "):
                n_pred += 1
                continue
            if p.startswith("      awaited: ") and q.startswith("      awaited: "):
                n_aw += 1
                continue
            mp = re.match(r"^(      use the call of .*? \(result (tested|used)\)): ", p)
            mq = re.match(r"^(      use the call of .*? \(result (tested|used)\)): ", q)
            if mp and mq and mp.group(1) == mq.group(1):
                n_use += 1
                continue
            bad.append("wait %s: %s | %s" % (k, p[:140], q[:140]))
    if ra != rb:
        bad.append("[rows] differ")
    if bad:
        print("BAD: %d undeclared differences" % len(bad))
        for l in bad[:20]:
            print("  " + l)
        return 1
    print("only declared differences: the new header line; new HB-await edges %d; lock identity lines %d; waits: predicate lines %d, awaited lines %d, "
          "use-the-call consequences %d, caller-side use lines %d; rows the same" % (len(new_edges), n_lockline, n_pred, n_aw, n_use, n_cs))
    return 0


if __name__ == "__main__":
    sys.exit(main())
