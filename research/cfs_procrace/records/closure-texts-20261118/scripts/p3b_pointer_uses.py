#!/usr/bin/env python3
# P3b: the uses of the value of a pointer field (reads of FIELD in READS), and for call-argument uses the callee and
# the parameter position; says whether the callee has a body in the facts and what the facts hold about that parameter.
import re, sys, collections
view, field = sys.argv[1], sys.argv[2]
sec = collections.defaultdict(list); cur = None
for ln, l in enumerate(open(view, errors="replace"), 1):
    if l.startswith("["): cur = l.split("]")[0][1:]; continue
    if cur in ("reads", "calls", "functions", "paramflow", "semantics", "ftypes", "p0"): sec[cur].append((ln, l.rstrip("\n")))
site = {}
for n, t in sec["calls"]:
    m = re.match(r"\s+site (\d+) (\S+) (\S+) -> (\S+) (\S+)", t)
    if m: site[m.group(1)] = (m.group(4), m.group(5), m.group(2))
defined = set(t.split()[2] for n, t in sec["functions"] if "{defined" in t or " {defined}" in t)
uses = collections.Counter(); args = collections.Counter(); lines = []
for n, t in sec["reads"]:
    if (" reads field " + field + " ") not in t: continue
    u = re.search(r"use=\{([^}]*)\}", t).group(1)
    uses[u] += 1
    m = re.search(r"arg (\d+) of site (\d+)", t)
    if m and "call-argument" in u:
        cal = site.get(m.group(2), ("?", "?", "?"))
        args[(cal[0], m.group(1))] += 1
print("# view %s field %s" % (view, field))
ft = [t for n, t in sec["ftypes"] if t.split()[0] == field]
print("FIELD_TYPES: " + (ft[0].strip()[:200] if ft else "no row"))
print("reads of the field's value by use: " + ", ".join("%s %d" % kv for kv in sorted(uses.items())))
for (cal, a), c in sorted(args.items()):
    body = cal in defined
    pf = [t.strip()[:150] for n, t in sec["paramflow"] if re.match(r"\s+%s#%s " % (re.escape(cal), a), t)]
    kinds = collections.Counter(re.match(r"\S+ d\d+ (\S+)", x).group(1) for x in pf if re.match(r"\S+ d\d+ (\S+)", x))
    sm = [t for n, t in sec["semantics"] if re.search(r"\b%s\b" % re.escape(cal), t) and ("P0 " in t)]
    st = collections.Counter(re.search(r"(doc-collated|standard-collated|unverified|body-derived)", x).group(1) for x in sm if re.search(r"(doc-collated|standard-collated|unverified|body-derived)", x))
    print("  passed to %s parameter %s: %d site(s); callee body in facts: %s; PARAM_FLOW kinds of that parameter: %s; P0 semantics rows of the callee by state: %s" % (
        cal, a, c, "yes" if body else "no", dict(kinds) or "-", dict(st) or "-"))
