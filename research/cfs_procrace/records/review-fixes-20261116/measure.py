#!/usr/bin/env python3
"""measure.py OLD_ORDER NEW_ORDER -- the pair counts of every row (the `pairs:` lines, all pairs, not only the listed ones)
and the row values of two order.txt of one build, before and after the review fixes; and the values of the pairs named by
the review (given as the head text of a pair line, on stdin, one per line)."""
import collections, re, sys
PAIR = re.compile(r"      pair: (.*): (hard_before|fixed_early_proved|conditional_before|fixed_early_candidate|unordered|unknown)(?:: |; path |$)(.*)$")
def load(p):
    rows, tot, pairs = {}, collections.Counter(), {}
    cur = None
    for l in open(p, encoding="utf-8", errors="replace"):
        l = l.rstrip("\n")
        m = re.match(r"  ([ABC]\d+) key (\S+) order\.static (\S+)$", l)
        if m:
            cur = m.group(1); rows[cur] = m.group(3); continue
        if cur and l.startswith("      pairs: ") and not l.startswith("      pairs: none"):
            for v, n in re.findall(r"(hard_before|fixed_early_proved|conditional_before|fixed_early_candidate|unordered|unknown) (\d+)", l): tot[v] += int(n)
        m = PAIR.match(l)
        if m: pairs[m.group(1)] = m.group(2)
    return rows, tot, pairs
a, b = load(sys.argv[1]), load(sys.argv[2])
print("rows %d / %d; row values changed %d" % (len(a[0]), len(b[0]), sum(1 for k in a[0] if a[0][k] != b[0].get(k))))
print("row values after: " + ", ".join("%s %d" % kv for kv in sorted(collections.Counter("%s:%s" % (k[0], v) for k, v in b[0].items()).items())))
print("all pairs before: " + ", ".join("%s %d" % kv for kv in sorted(a[1].items())))
print("all pairs after:  " + ", ".join("%s %d" % kv for kv in sorted(b[1].items())))
for h in sys.stdin.read().split("\n"):
    if h.strip():
        print("named pair: %s ... : %s -> %s" % (h[:110], a[2].get(h, "(not listed)"), b[2].get(h, "(not listed)")))
