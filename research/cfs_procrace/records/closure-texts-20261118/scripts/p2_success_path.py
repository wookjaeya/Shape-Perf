#!/usr/bin/env python3
# P2: for each creator callee named by the path A rows of a relations text, the static line of basis 4:
# which return alternatives can carry the success value, and whether each such path passes the name-initialising
# store and the store of the out id.  Inputs: RETURNS, RESULT_FLOW, PARAM_FLOW, WRITERS of a facts view.
import re, sys, collections
view, rel = sys.argv[1], sys.argv[2]
S = collections.defaultdict(list); cur = None
for ln, l in enumerate(open(view, errors="replace"), 1):
    if l.startswith("["): cur = l.split("]")[0][1:]; continue
    if cur in ("returns", "resultflow", "paramflow", "writers", "calls"): S[cur].append((ln, l.rstrip("\n")))
rows = open(rel, errors="replace").read().split("\n")
cre = collections.OrderedDict(); keyf = {}
curns = None
for l in rows:
    m = re.match(r"  A\d+ lookup .* namespace (\S+) partition", l)
    if m: curns = m.group(1)
    m = re.match(r"      (?:creator: .*?|not a pair: .*?): (\S+) in \S+ -> (\S+) key", l)
    if m and curns: cre.setdefault(m.group(2), set()).add(curns)
    m = re.match(r"      callee-check \(result flow, success\): (\S+): shown to reach the return: (.*?) \(", l)
    if m: keyf[m.group(1)] = m.group(2)
def rets(fn):
    out = []; cur = None
    for n, t in S["returns"]:
        m = re.match(r"\s{2,3}%s returns at (\S+) alt (\d+)/(\d+) (\S+) (.*)$" % re.escape(fn), t)
        if m: cur = dict(pos=m.group(1), alt=m.group(2), of=m.group(3), kind=m.group(4), rest=m.group(5), ln=n, sub=[]); out.append(cur); continue
        if cur is not None and t.startswith("      "): cur["sub"].append(t.strip())
        elif not t.startswith("      "): cur = None
    return out
print("# view %s\n# relations %s" % (view, rel))
for fn, nss in cre.items():
    sv = keyf.get(fn)
    R = rets(fn)
    if not R: print("%s: no RETURNS rows (no body in these facts) => basis 4 missing: the callee's returns are not all followed" % fn); continue
    if sv is None or not re.match(r"-?\d+$", sv.strip()):
        print("%s: the row gives no single value on its `callee-check (result flow, success)` line (%s) => basis 4 missing" % (fn, sv)); continue
    v = int(sv); carry = []; excl = []
    for r in R:
        when = next((re.search(r"when=(.*)$", s).group(1) for s in r["sub"] if "when=" in s), "")
        if r["kind"] == "const":
            c = int(r["rest"].split()[0])
            (carry if c == v else excl).append((r, "constant %d" % c)); continue
        if r["kind"] == "call-result":
            site = re.search(r"call site (\d+) (\S+)", r["rest"]); sid, spos = site.group(1), site.group(2)
            dfn = next((re.search(r"definition of (\w+) at (\S+)", s) for s in r["sub"] if s.startswith("definition of")), None)
            var, dpos = (dfn.group(1), dfn.group(2)) if dfn else (None, None)
            # the definition's own value tested by a unique branch: does `when` exclude v ?
            dshort = ":".join(dpos.split(":")[-2:]) if dpos else ""
            rf = [(n, t) for n, t in S["resultflow"] if (" site %s " % sid) in t and "-> branch" in t and " unique " in t and ("local:%s@def:%s" % (var, dshort)) in t]
            ex = False
            if rf and "redefined-later" in r["rest"]:
                cond = re.search(r"cond (\(.*\)) \{", rf[0][1]).group(1).replace("@def:%s" % dshort, "")
                neg = "(not %s)" % cond if False else None
                # `when` holds (not (and ... (eq local:var v))) : the definition reaches the return only when its branch on v is not taken
                if re.search(r"\(not \(and .*\(eq local:%s %d\)\)\)" % (re.escape(var), v), when) and cond == "(eq local:%s %d)" % (var, v): ex = True
            (excl if ex else carry).append((r, "result of the call at %s%s" % (":".join(spos.split(":")[-3:]), " (its own branch on the value at view line %d leaves no path of %d to the return)" % (rf[0][0], v) if ex else "")))
            r["site"] = sid; r["when"] = when
            continue
        carry.append((r, r["kind"] + " " + r["rest"][:40]))
    line = "%s (name spaces %s): success value %d; returns %d; " % (fn, ",".join(sorted(nss)), v, len(R))
    line += "cannot carry it: " + "; ".join("%s alt %s/%s %s" % (":".join(r["pos"].split(":")[-3:]), r["alt"], r["of"], why) for r, why in excl) + ". "
    ok = True; parts = []
    for r, why in carry:
        if r["kind"] != "call-result": ok = False; parts.append("%s alt %s: %s: not analysed" % (r["pos"], r["alt"], why)); continue
        when = r["when"]; sid = r["site"]
        callee = re.search(r"\(call-result (\S+) ", r["rest"]).group(1)
        cr = rets(callee)
        passthru = len(cr) == 1 and cr[0]["kind"] == "param"
        pidx = re.search(r"#(\d+)", cr[0]["rest"]).group(1) if passthru else None
        ns_ok = []
        for ns in sorted(nss):
            w = [(n, t) for n, t in S["writers"] if t.split()[0] == ns and (" in %s " % fn) in t]
            g = [(n, re.search(r"guard=(.*?) \{", t).group(1) if "guard=" in t else "-") for n, t in w]
            ns_ok.append((ns, [n for n, gg in g if gg == when], len(w)))
        pf = [(n, t) for n, t in S["paramflow"] if re.match(r"\s+%s#(\d+) d0 call-argument -> %s#(\d+) at site %s " % (re.escape(fn), re.escape(callee), sid), t)]
        outs = []
        for n, t in pf:
            m = re.match(r"\s+\S+#(\d+) d0 call-argument -> \S+#(\d+)", t); a, b = m.group(1), m.group(2)
            ws = [(wn, wt) for wn, wt in S["writers"] if (" via %s " % callee) in wt and ("key=*%s argument %s " % (fn, a)) in wt]
            if ws:
                g = re.search(r"guard=(.*?) \{", ws[0][1]).group(1); pname = re.search(r"\(ne (param:\w+#%s) 0\)" % b, g)
                srcname = re.search(r"\[(\w+)\]", t).group(1)
                nonnull = ("(not (not (ne param:%s#%s 0)))" % (srcname, a)) in when
                outs.append((a, b, len(ws), g, nonnull, ws[0][0]))
        good = passthru and all(len(x[1]) >= 1 for x in ns_ok) and any(o[4] for o in outs)
        ok = ok and good
        parts.append("carries it: alt %s/%s = result of %s at %s under %s; %s returns its parameter %s unchanged: %s; name store on that path: %s; out id store through parameter %s -> %s#%s at osapi line of view row %s under %s, non-null on the path: %s" % (
            r["alt"], r["of"], callee, ":".join(r["pos"].split(":")[-3:]), when, callee, pidx, "yes" if passthru else "no",
            ", ".join("%s view line(s) %s of %d" % (ns, x or "NONE", c) for ns, x, c in ns_ok),
            outs[0][0] if outs else "?", callee, outs[0][1] if outs else "?", outs[0][5] if outs else "?", outs[0][3] if outs else "?", "yes" if outs and outs[0][4] else "no"))
    print(line + " ".join(parts) + " => basis 4 static line: %s" % ("HELD" if ok and carry else "MISSING (the callee's returns are not all followed)"))
