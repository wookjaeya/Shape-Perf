#!/usr/bin/env python3
# P1: context-qualified `alternatives:` line of a path A row, from a facts view (kz-static show) and the row's text.
# The stored-field call-result rule (the result of a call that takes the address of a field is decided from the value
# stored to that field on the path) is stated in judgement rules 8.13 (plan/ovm_rev1_20261116/JUDGEMENT_RULES_REV1.md).
import re, sys, collections
view, rel, rowid = sys.argv[1], sys.argv[2], sys.argv[3]
S = collections.defaultdict(list); cur = None
for ln, l in enumerate(open(view, errors="replace"), 1):
    if l.startswith("["): cur = l.split("]")[0][1:]; continue
    if cur in ("functions", "calls", "writers", "retconsts", "paramflow", "fstatus", "resultflow", "resulteffect"):
        S[cur].append((ln, l.rstrip("\n")))
calls = {}; callers = collections.defaultdict(list)
for n, t in S["calls"]:
    m = re.match(r"\s+site (\d+) (\S+) (\S+) -> (\S+) (\S+) \{([^}]*)\}", t)
    if m: calls[m.group(1)] = dict(pos=m.group(2), fn=m.group(3), callee=m.group(4), kind=m.group(5), flags=m.group(6), ln=n); callers[m.group(4)].append(m.group(1))
fdef = {}
for n, t in S["functions"]:
    m = re.match(r"\s+fn \d+ (\S+) (\S+) params=\d+ \{([^}]*)\}", t)
    if m: fdef[m.group(1)] = (m.group(2), m.group(3), n)
eff = collections.defaultdict(list)
for n, t in S["resulteffect"]:
    m = re.match(r"\s+site (\d+) \S+ in \S+ (\w+) (-?\d+) (\S+) -> (\S+)(.*)$", t)
    if m:
        rest = m.group(6); st = "open" if re.search(r"\] open \(| open \(", rest) else ("complete" if " complete flow" in rest else "?")
        eff[(m.group(1), m.group(3))].append(dict(kind=m.group(5), rest=rest, state=st, ln=n, cls=m.group(2), reach=m.group(4)))
def short(p): return re.sub(r"^/home/user/work/procrace/c1_build/(pre|post)/src/", "<\\1>/", p)
# --- three-valued evaluation of a condition text
def parse(s):
    s = s.strip(); toks = re.findall(r"\(|\)|[^\s()]+", s); pos = [0]
    def rd():
        t = toks[pos[0]]; pos[0] += 1
        if t != "(": return t
        lst = []
        while toks[pos[0]] != ")": lst.append(rd())
        pos[0] += 1; return lst
    return rd()
def flat(x): return x if isinstance(x, str) else "(" + " ".join(flat(y) for y in x) + ")"
def ev(x, env):
    if isinstance(x, str):
        if x in env: return env[x]
        if re.match(r"-?\d+$", x): return int(x)
        return None
    k = flat(x)
    if k in env: return env[k]
    op = x[0]
    if op == "not":
        v = ev(x[1], env); return None if v is None else (0 if v else 1)
    if op in ("and", "or"):
        vs = [ev(y, env) for y in x[1:]]
        if op == "and": return 0 if any(v == 0 for v in vs) else (None if any(v is None for v in vs) else 1)
        return 1 if any(v not in (None, 0) for v in vs) else (None if any(v is None for v in vs) else 0)
    if op in ("ne", "eq", "lt", "gt", "le", "ge") and len(x) == 3:
        a, b = ev(x[1], env), ev(x[2], env)
        if a is None or b is None: return None
        return int({"ne": a != b, "eq": a == b, "lt": a < b, "gt": a > b, "le": a <= b, "ge": a >= b}[op])
    return None
def close_by_field(site_up, val, out):
    """The stored-field call-result rule at the frame whose call site is site_up, for the value val. Returns (decided, text)."""
    es = eff[(site_up, val)]
    op = [e for e in es if e["state"] == "open" and "takes the address of field:" in e["rest"]]
    if not op: return None
    m = re.search(r"; (\S+) at (\S+) takes the address of field:(\S+) on these paths|\((?:[^()]*; )?(\S+) at (\S+) takes the address of field:(\S+) on these paths", op[0]["rest"])
    g = [x for x in m.groups() if x]; F, kpos, X = g[0], g[1], g[2]
    st = [e for e in es if e["kind"] == "store" and e["state"] == "complete" and ("field:%s := " % X) in e["rest"]]
    if not st: return (False, "no decided store to field:%s on this path" % X)
    c = int(re.search(r"value (-?\d+) \[", st[0]["rest"]).group(1)); spos = re.search(r"store at (\S+)", " store" + st[0]["rest"]).group(1)
    ksite = [s for s, d in calls.items() if d["pos"] == kpos and d["callee"] == F]
    caller = calls[ksite[0]]["fn"]
    rf = [(n, t) for n, t in S["resultflow"] if (" site %s " % ksite[0]) in t and "-> branch" in t]
    cond = re.search(r"cond (\(.*\)) \{", rf[0][1]).group(1) if rf else None
    k = None
    mm = re.search(r"\(call-result %s \(seq (.*)\)\)" % re.escape(F), cond or "")
    args = mm.group(1) if mm else ""
    k = 0 if args.startswith("(addr field:%s)" % X) else None
    rcs = [(n, t) for n, t in S["retconsts"] if re.match(r"\s+%s returns " % re.escape(F), t)]
    pn = None; res = []
    for n, t in rcs:
        w = re.search(r"when=(.*)$", t).group(1); cst = int(re.match(r"\s+\S+ returns (-?\d+)", t).group(1))
        if pn is None:
            q = re.search(r"\(deref (param:\w+#%d)\)" % (k if k is not None else 0), w); pn = q.group(1) if q else None
        res.append((cst, w, n, "only-constants" in t))
    env = {pn: 1, "(deref %s)" % pn: c} if pn else {}
    vals = [(cst, ev(parse(w), env), n) for cst, w, n, oc in res]
    possible = sorted(set(cst for cst, v, n in vals if v != 0)); certain = [cst for cst, v, n in vals if v == 1]
    allconst = all(oc for *_, oc in res) and len(res) > 0
    # stores through the parameter in the callee: their guards under the same assignment
    outst = [(n, t) for n, t in S["paramflow"] if re.match(r"\s+%s#%s d\d+ out-store" % (re.escape(F), k), t)]
    outg = [(n, ev(parse(re.search(r"guard=(.*)$", t).group(1)), env) if "guard=" in t else 1) for n, t in outst]
    passed = [(n, t) for n, t in S["paramflow"] if re.match(r"\s+%s#%s d\d+ call-argument" % (re.escape(F), k), t)]
    ws = [(n, t) for n, t in S["writers"] if t.split()[0] == X]
    sline = int(spos.split(":")[-1]); kline = int(kpos.split(":")[-2]); sfile = spos.rsplit(":", 1)[0]
    between = [n for n, t in ws if (" at %s:" % sfile) in t and (" in %s " % caller) in t and sline < int(re.search(r" at \S+?:(\d+):\d+ in", t).group(1)) < kline]
    cbetween = [s for s, d in calls.items() if d["fn"] == caller and d["pos"].startswith(sfile + ":") and sline < int(d["pos"].split(":")[-2]) < kline]
    wf = sorted(set(re.search(r" in (\S+)", t).group(1) for n, t in ws))
    fs = [(n, t) for n, t in S["fstatus"] if t.split()[0] == X]
    pf = re.search(r"per-field=(\S+) type-level=(\S+)", fs[0][1]).groups() if fs else ("no-row", "no-row")
    decided = allconst and len(possible) == 1 and k is not None and not between and not cbetween and all(v == 0 for n, v in outg) and not passed and pf[0] == "writer-set-complete"
    bval = ev(parse(cond.replace(mm.group(0), "RES")), {"RES": possible[0]}) if (cond and mm and len(possible) == 1) else None
    out.append("      stored-field call-result rule: store field:%s := %d at %s [RESULT_EFFECT view line %d, complete]; call %s at %s passes (addr field:%s) as argument %s [RESULT_FLOW view line %d: cond %s]" % (X, c, short(spos), st[0]["ln"], F, short(kpos), X, k, rf[0][0] if rf else -1, cond))
    out.append("        RET_CONSTS of %s under %s != 0 and (deref %s) = %d: %s; only-constants on every row: %s" % (F, pn, pn, c, "; ".join("returns %d: when is %s [view line %d]" % (cst, {1: "true", 0: "false", None: "undecided"}[v], n) for cst, v, n in vals), "yes" if allconst else "no"))
    out.append("        stores through parameter %s inside %s under that assignment: %s; the parameter passed on to another call: %s" % (k, F, ", ".join("view line %d guard %s" % (n, {1: "true", 0: "false", None: "undecided"}[v]) for n, v in outg) or "none", len(passed)))
    out.append("        writers of %s: %d rows in functions {%s}; FIELD_STATUS per-field=%s type-level=%s [view line %d]; writers of it between the store and the call in %s: %d; calls between them: %d" % (X, len(ws), ", ".join(wf), pf[0], pf[1], fs[0][0] if fs else -1, caller, len(between), len(cbetween)))
    out.append("        => call result %s; branch %s is %s" % ("decided " + str(possible[0]) if decided else "NOT decided (possible %s)" % possible, cond, {1: "taken", 0: "not taken", None: "undecided"}[bval] if decided else "undecided"))
    return (decided and bval == 0, "the loop at %s is not entered; the exit effect is reached" % short(kpos))
def describe(site, val):
    es = eff[(site, val)]; c = collections.Counter("%s:%s" % (e["kind"], e["state"]) for e in es)
    return ", ".join("%s %d" % kv for kv in sorted(c.items())) or "no RESULT_EFFECT row of this value"
# --- the row
rows = open(rel, errors="replace").read().split("\n")
hdr = [i for i, l in enumerate(rows) if re.match(r"  %s lookup " % rowid, l)][0]
body = [];
for l in rows[hdr:]:
    if body and re.match(r"  [ABC]\d+ ", l): break
    body.append(l)
upos = re.match(r"  \S+ lookup (\S+) in (\S+) -> (\S+?)#", body[0]).groups()
ab = [l for l in body if l.strip().startswith("absence:")]
val = re.search(r"absence: (-?\d+) ", ab[0]).group(1) if ab and re.search(r"absence: (-?\d+) ", ab[0]) else None
usite = [s for s, d in calls.items() if d["pos"] == upos[0] and d["callee"] == upos[2]]
print("row %s use %s in %s -> %s; absence value %s; use call site %s (view line %s)" % (rowid, short(upos[0]), upos[1], upos[2], val, usite, calls[usite[0]]["ln"] if usite else "-"))
if val is None or not usite:
    print("  alternatives: not established (the row has no evaluated absence value or the use site is not a call row)"); sys.exit(0)
probe = [l for l in body if l.strip().startswith("not a pair: probe-create")]
def walk(site, chain, out, depth=0):
    d = calls[site]; es = eff[(site, val)]
    out.append("    frame %s in %s (call flags {%s}): effects of %s: %s" % (short(d["pos"]), d["fn"], d["flags"], val, describe(site, val)))
    found = []
    if any(e["kind"] == "retry" for e in es): found.append("retry (the call at %s runs again; RESULT_EFFECT view line %d, %s)" % (short(d["pos"]), [e for e in es if e["kind"] == "retry"][0]["ln"], [e for e in es if e["kind"] == "retry"][0]["state"]))
    if depth == 0 and probe:
        m = re.search(r"\): (\S+) in \S+ -> (\S+) key", probe[0]); found.append("creation in the same name space and partition by the same function (%s -> %s; the row's not-a-pair line)" % (short(m.group(1)), m.group(2)) if m else "creation by the same function")
    opened = [e for e in es if e["state"] == "open"]
    closing = close_by_field(site, val, out) if opened else None
    rets = [e for e in es if e["kind"] in ("return-result",)]
    state = None
    if closing is not None and closing[0]:
        state = "complete"
    elif opened and not found:
        kinds = sorted(set(e["kind"] for e in opened))
        state = "open: " + ", ".join(kinds)
    if rets and not (closing and closing[0]):
        fn = d["fn"]; cs = callers.get(fn, [])
        direct = [s for s in cs if calls[s]["kind"] == "direct"]
        res = []
        if not direct:
            res.append((chain + [site], found, "not established (the function %s returns the value and has no direct caller in these facts: flags {%s})" % (fn, fdef.get(fn, ("", "?"))[1])))
        for s in direct:
            res.extend(walk(s, chain + [site], out, depth + 1))
        return [(c, found + f, st) for c, f, st in res]
    if not rets and not es:
        return [(chain + [site], found, "not established (no RESULT_EFFECT row of the value at this frame)")]
    loopframe = [s for s in chain + [site] if "in-loop" in calls[s]["flags"]]
    root = d["fn"]
    cs = [s for s in callers.get(root, []) if calls[s]["kind"] == "direct"]
    tail = None
    if not (closing and closing[0]) and not rets:
        # the value is consumed here and the function goes on: where the path continues is the caller's
        tail = "the value's effects end in %s and the function returns to its caller: direct callers %d, flags {%s}" % (root, len(cs), fdef.get(root, ("", "?"))[1])
        return [(chain + [site], found, ("open: " if opened else "") + "not established (%s)" % tail)]
    return [(chain + [site], found, state or "complete")]
out = []
res = walk(usite[0], [], out)
for l in out: print(l)
for chain, found, state in res:
    ctx = " <- ".join("%s@%s" % (calls[s]["fn"], short(calls[s]["pos"])) for s in reversed(chain))
    if found: txt = "found: " + "; ".join(found) + ((" [" + state + "]") if state != "complete" else "")
    elif state == "complete": txt = "none found (exploration complete)"
    else: txt = state if state.startswith("not established") else "not established (%s)" % state
    print("  alternatives [context %s]: %s" % (ctx, txt))
