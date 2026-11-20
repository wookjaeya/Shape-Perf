#!/usr/bin/env python3
"""plan_rev5_compare.py KEPT NEW -- a plan of plan rules revision 5 (NEW: a directory with plan.side, plan.kzs, plan.req)
against a kept revision-4 plan of the same build (KEPT), with the site ids mapped to the site keys of each plan.

Writes to stdout:
  rows       the row lines (selection, tiers, positions, labels, control): equal / different, matched by the row's
             number (A21, B13, ...) so that a row whose use-site key moved with the facts is reported as such
  selection  the rows planned by the tier order and by the control selection, in each plan
  sites      site keys only in KEPT (removed), only in NEW (added, by rule), and common sites whose rules or values differ
  needs      need lines by (subject, what): equal, changed (state, reason, note or sites), only in NEW, only in KEPT
  lines      the other sidecar line kinds counted in each plan
  requests   the request lines with their site ids mapped to keys: equal / different
A check of the step (results section 19), not a tool of the method.  rc 0 always when both plans were read."""
import re, sys, collections

def read(p):
    with open(p, encoding="utf-8") as f:
        return f.read().splitlines()

def sidecar(d):
    L = read(d + "/plan.side")
    sites = {}
    for l in L:
        m = re.match(r'^site (\d+) (\S+) key="(.*)" refs=', l)
        if m: sites[m.group(1)] = (m.group(3), m.group(2).split(".", 1)[1])
    return L, sites

def manifest(d, sites):
    vals = collections.defaultdict(list)
    for l in read(d + "/plan.kzs"):
        m = re.match(r'^value (\d+) \d+ (.*)$', l)
        if m and m.group(1) in sites: vals[sites[m.group(1)][0]].append(m.group(2))
    return vals

def ids_to_keys(s, sites):
    return ",".join(sorted(sites[i][0] for i in s.split(",") if i in sites)) if s else ""

def main():
    kd, nd = sys.argv[1], sys.argv[2]
    kL, ks = sidecar(kd)
    nL, ns = sidecar(nd)
    out = []
    # rows
    def rows(L):
        r = {}
        for l in L:
            m = re.match(r'^row (\S+) (\S+) (.*)$', l)
            if m: r[m.group(2)] = (m.group(1), m.group(3))
        return r
    kr, nr = rows(kL), rows(nL)
    same = [i for i in kr if i in nr and kr[i] == nr[i]]
    keymoved = [i for i in kr if i in nr and kr[i][1] == nr[i][1] and kr[i][0] != nr[i][0]]
    diff = [i for i in kr if i in nr and kr[i][1] != nr[i][1]]
    out.append(f"rows kept {len(kr)} new {len(nr)}: equal {len(same)}; same line but the use-site key moved {len(keymoved)} {' '.join(keymoved)}; "
               f"different {len(diff)} {' '.join(diff)}; only kept {len(set(kr)-set(nr))}; only new {len(set(nr)-set(kr))}")
    def sel(r):
        t = sorted((i for i, (_, x) in r.items() if " state=planned" in x), key=lambda s: (s[0], int(s[1:])))
        c = sorted((i for i, (_, x) in r.items() if "control-state=planned" in x), key=lambda s: (s[0], int(s[1:])))
        tiers = {i: re.search(r"tier=(\d)", x).group(1) for i, (_, x) in r.items()}
        return t, c, tiers
    kt, kc, ktier = sel(kr)
    nt, nc, ntier = sel(nr)
    out.append(f"selection tier-planned kept {len(kt)} new {len(nt)} equal={'yes' if kt == nt else 'no'}: {' '.join(nt)}")
    out.append(f"selection control-planned kept {len(kc)} new {len(nc)} equal={'yes' if kc == nc else 'no'}: {' '.join(nc)}")
    out.append(f"selection tiers equal={'yes' if ktier == ntier else 'no'}")
    # sites
    kk = {v[0]: v[1] for v in ks.values()}
    nk = {v[0]: v[1] for v in ns.values()}
    kv, nv = manifest(kd, ks), manifest(nd, ns)
    removed = sorted(set(kk) - set(nk))
    added = sorted(set(nk) - set(kk))
    out.append(f"sites kept {len(kk)} new {len(nk)}: removed {len(removed)}; added {len(added)}")
    byrule = collections.Counter()
    for k in added:
        for r in nk[k].split("+"): byrule[r] += 1
    out.append("sites added by rule (a site may carry several): " + "; ".join(f"{r} {n}" for r, n in sorted(byrule.items())))
    for k in removed: out.append(f"  removed site {k} [{kk[k]}]")
    ch_rules, ch_vals = [], []
    for k in sorted(set(kk) & set(nk)):
        if kk[k] != nk[k]: ch_rules.append(f"  rules {kk[k]} -> {nk[k]}: {k}")
        if kv.get(k) != nv.get(k):
            ch_vals.append(f"  values: {k}\n    kept: {' | '.join(kv.get(k, []))}\n    new:  {' | '.join(nv.get(k, []))}")
    out.append(f"common sites {len(set(kk) & set(nk))}: rules changed {len(ch_rules)}; values changed {len(ch_vals)}")
    out += ch_rules + ch_vals
    # needs
    def needs(L, sites):
        d = {}
        for l in L:
            m = re.match(r'^need (\S+(?: \S+)?) (\S+(?: \d+)?) (planned|not-planned) reason=(\S+(?:\([^)]*\))?) note=("[^"]*")(?: sites=(\S+))?$', l)
            if m: d[(m.group(1), m.group(2))] = (m.group(3), m.group(4), m.group(5), ids_to_keys(m.group(6) or "", sites))
        return d
    kn, nn = needs(kL, ks), needs(nL, ns)
    eq = [x for x in kn if x in nn and kn[x] == nn[x]]
    chg = [x for x in kn if x in nn and kn[x] != nn[x]]
    onew = [x for x in nn if x not in kn]
    okept = [x for x in kn if x not in nn]
    out.append(f"needs kept {len(kn)} new {len(nn)}: equal {len(eq)}; changed {len(chg)}; only new {len(onew)}; only kept {len(okept)}")
    out.append("needs only new by what: " + "; ".join(f"{w} {n}" for w, n in sorted(collections.Counter(re.sub(r' \d+$', '', x[1]) for x in onew).items())))
    for x in chg:
        a, b = kn[x], nn[x]
        parts = [n for n, i in (("state", 0), ("reason", 1), ("note", 2), ("sites", 3)) if a[i] != b[i]]
        out.append(f"  changed need {x[0]} {x[1]}: {', '.join(parts)}: {a[0]} {a[1]} -> {b[0]} {b[1]}")
    for x in okept: out.append(f"  only kept need {x[0]} {x[1]}")
    # other line kinds
    def kinds(L):
        return collections.Counter(l.split(" ", 1)[0] for l in L)
    kc_, nc_ = kinds(kL), kinds(nL)
    out.append("sidecar line kinds (kept -> new): " + "; ".join(f"{w} {kc_.get(w, 0)}->{nc_.get(w, 0)}" for w in sorted(set(kc_) | set(nc_)) if kc_.get(w, 0) != nc_.get(w, 0)))
    def plain(L, skip):
        return [l for l in L if l.split(" ", 1)[0] not in skip]
    skip = {"row", "site", "map", "need", "gap", "generator", "namespace-data", "namespace-layer", "namespace-entry", "context", "lifetime", "lifetime-complete", "input", "bound", "requests", "plan-id", "facts-artifact", "basis-artifact", "spawn"}
    pk, pn = plain(kL, skip), plain(nL, skip)
    out.append(f"other sidecar lines equal={'yes' if pk == pn else 'no'} ({len(pk)} lines)")
    if pk != pn:
        for l in sorted(set(pk) ^ set(pn))[:20]: out.append("  differs: " + l[:300])
    # requests
    def reqs(d, sites):
        r = []
        for l in read(d + "/plan.req"):
            m = re.match(r'^(request \d+ .*?) site=(\S+) (.*)$', l)
            if m: r.append(m.group(1) + " site=" + (sites[m.group(2)][0] if m.group(2) in sites else m.group(2)) + " " + m.group(3))
        return r
    rk, rn = reqs(kd, ks), reqs(nd, ns)
    out.append(f"requests kept {len(rk)} new {len(rn)} equal after mapping the site ids={'yes' if rk == rn else 'no'}")
    print("\n".join(out))

main()
