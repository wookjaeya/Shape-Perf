#!/usr/bin/env python3
"""measure_on.py OFF ON [--plans OFFPLAN ONPLAN] -- what the facts of --object-access change downstream, for one build: the
outputs of one kz-static on the facts extracted without the option (OFF: relations.txt, keys.txt, order.txt of the order
step's directory) and with it (ON), matched by use-site key (keys.txt; docs/static_facts.md 5l), not by row id (ids are
numbers in text order and move).  Prints:
  relations  rows off/on, keys in both, only off, only on (each listed with its key text), rows in both whose text
             differs (the row id written as its key), and per row line label (membership, key, writers, ...) how many
             rows in both differ in that line
  order      per key in both, the row value off -> on (a transition table), and the keys only on one side
  plans      (with --plans) sites off/on, site lines only off / only on, and the key values of the use-call and
             creator-call sites by string mode
A measurement: no allowance; rc 0 unless an input cannot be read."""
import collections
import os
import re
import sys


def read(p):
    with open(p, encoding="utf-8", errors="surrogateescape") as f:
        return f.read().split("\n")


def keys(d):
    out = {}
    for l in read(os.path.join(d, "keys.txt")):
        m = re.match(r"^([ABC]\d+) ([0-9a-f]{16}) (.*)$", l)
        if m:
            out[m.group(1)] = (m.group(2), m.group(3))
    return out


def rows(d, idkey):
    out, cur, buf = {}, None, []
    for l in read(os.path.join(d, "relations.txt")):
        m = re.match(r"^  ([ABC]\d+) ", l)
        if m and m.group(1) in idkey:
            if cur is not None:
                out[idkey[cur][0]] = buf
            cur, buf = m.group(1), [l.replace("  %s " % m.group(1), "  <%s> " % idkey[m.group(1)][0], 1)]
            continue
        if cur is not None:
            if l.startswith("      "):
                buf.append(l)
            else:
                out[idkey[cur][0]] = buf
                cur, buf = None, []
    if cur is not None:
        out[idkey[cur][0]] = buf
    return out


def order_values(d):
    out = {}
    for l in read(os.path.join(d, "order.txt")):
        m = re.match(r"^  ([ABC]\d+) key ([0-9a-f]{16}) order\.static (\S+)", l)
        if m:
            out[m.group(2)] = m.group(3)
    return out


def label(line):
    m = re.match(r"^      ([a-z][\w -]*?):", line)
    return m.group(1) if m else "(other)"


def main():
    a = sys.argv[1:]
    plans = None
    if "--plans" in a:
        i = a.index("--plans")
        plans = a[i + 1:i + 3]
        a = a[:i] + a[i + 3:]
    if len(a) != 2:
        print(__doc__, file=sys.stderr)
        return 2
    off, on = a
    ko, kn = keys(off), keys(on)
    so, sn = {v[0]: v[1] for v in ko.values()}, {v[0]: v[1] for v in kn.values()}
    ro, rn = rows(off, ko), rows(on, kn)
    both = sorted(set(so) & set(sn))
    print("relations rows off %d on %d; keys in both %d, only off %d, only on %d" % (len(so), len(sn), len(both), len(set(so) - set(sn)), len(set(sn) - set(so))))
    for k in sorted(set(so) - set(sn)):
        print("  only off %s %s" % (k, so[k]))
    for k in sorted(set(sn) - set(so)):
        print("  only on  %s %s" % (k, sn[k]))
    differ = [k for k in both if ro.get(k) != rn.get(k)]
    print("rows in both whose text differs: %d" % len(differ))
    lab = collections.Counter()
    for k in differ:
        a_, b_ = collections.Counter(ro.get(k, [])), collections.Counter(rn.get(k, []))
        labels = {label(l) for l in (a_ - b_).elements()} | {label(l) for l in (b_ - a_).elements()}
        for x in labels:
            lab[x] += 1
    for x, c in sorted(lab.items(), key=lambda t: (-t[1], t[0])):
        print("  line %-14s differs in %d rows" % (x, c))
    print("rows by path (off -> on): " + ", ".join("%s %d -> %d" % (p, sum(1 for v in ko.values() if v[1].startswith(p + "|")), sum(1 for v in kn.values() if v[1].startswith(p + "|"))) for p in "ABC"))
    if os.path.exists(os.path.join(off, "order.txt")) and os.path.exists(os.path.join(on, "order.txt")):
        vo, vn = order_values(off), order_values(on)
        t = collections.Counter((vo[k], vn[k]) for k in set(vo) & set(vn))
        print("order row values (off -> on), keys in both %d:" % len(set(vo) & set(vn)))
        for (x, y), c in sorted(t.items()):
            print("  %s -> %s %d" % (x, y, c))
    if plans:
        po, pn = read(os.path.join(plans[0], "plan.kzs")), read(os.path.join(plans[1], "plan.kzs"))
        def sites(lines):
            return [re.sub(r"^site \d+ label=s\d{4}\.", "site label=", l) for l in lines if l.startswith("site ")]
        a_, b_ = collections.Counter(sites(po)), collections.Counter(sites(pn))
        print("plan sites off %d on %d; site lines only off %d, only on %d (site numbers and label numbers masked)" % (
            sum(a_.values()), sum(b_.values()), sum((a_ - b_).values()), sum((b_ - a_).values())))
        for l in sorted((b_ - a_).elements())[:40]:
            print("  only on  " + l[:300])
        for l in sorted((a_ - b_).elements())[:40]:
            print("  only off " + l[:300])
        def keymodes(lines):
            c = collections.Counter()
            for l in lines:
                m = re.match(r"^value \d+ \d+ role=key from=\S+ type=\S+ string=(\S+)(?: bound=(\d+))?", l)
                if m:
                    c["%s%s" % (m.group(1), "" if m.group(2) is None else " bound=" + m.group(2))] += 1
            return c
        print("key values at use and creator calls, off: %s; on: %s" % (dict(sorted(keymodes(po).items())), dict(sorted(keymodes(pn).items()))))
    return 0


if __name__ == "__main__":
    sys.exit(main())
