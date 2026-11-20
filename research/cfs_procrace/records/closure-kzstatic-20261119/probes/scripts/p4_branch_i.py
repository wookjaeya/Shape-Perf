#!/usr/bin/env python3
# P4: branch (i) of the caller-dependence element from facts that exist: the row's `id holder:` line of the lookup,
# the writer set of the holder field (WRITERS, FIELD_STATUS) and the readers of it (READS).
import re, sys, collections
view, rel, rowid = sys.argv[1], sys.argv[2], sys.argv[3]
S = collections.defaultdict(list); cur = None
for ln, l in enumerate(open(view, errors="replace"), 1):
    if l.startswith("["): cur = l.split("]")[0][1:]; continue
    if cur in ("writers", "fstatus", "reads", "calls", "atype"): S[cur].append((ln, l.rstrip("\n")))
site = {}
for n, t in S["calls"]:
    m = re.match(r"\s+site (\d+) (\S+) (\S+) -> (\S+) ", t)
    if m: site[m.group(1)] = (m.group(4), m.group(2), m.group(3))
rows = open(rel, errors="replace").read().split("\n")
h = [i for i, l in enumerate(rows) if re.match(r"  %s lookup " % rowid, l)][0]
body = []
for l in rows[h:]:
    if body and re.match(r"  [ABC]\d+ ", l): break
    body.append(l)
upos, ufn, ucal = re.match(r"  \S+ lookup (\S+) in (\S+) -> (\S+?)#", body[0]).groups()
ih = [(h + i + 1, l) for i, l in enumerate(body) if l.strip().startswith("id holder: the lookup's")]
print("row %s (relations line %d) use %s -> %s" % (rowid, h + 1, upos, ucal))
if not ih: print("  branch (i): not established (the row has no id holder line of the lookup)"); sys.exit(0)
m = re.search(r"\[(.*?)\] address (\S+) base ([\w-]+(?: [\w-]+)?) ?(?:object (\S+))?", ih[0][1])
if not m: print("  branch (i): not established by this probe (the holder is a local or has no field: " + ih[0][1].strip()[:110] + ")"); sys.exit(0)
expr, fld, base = m.group(1), m.group(2), m.group(3)
print("  id holder (relations line %d): %s field %s base %s" % (ih[0][0], expr, fld, base))
if not re.match(r"[A-Za-z_].*\.\w+", fld) or fld == "base":
    print("  branch (i): not established by this probe (the holder is %s, not a field: its later uses are the RESULT_FLOW rows of the local, not read here)" % ("a local" if "local" in ih[0][1] else "not a field")); sys.exit(0)
ws = [(n, t) for n, t in S["writers"] if t.split()[0] == fld]
fs = [(n, t) for n, t in S["fstatus"] if t.split()[0] == fld]
pf = re.search(r"per-field=(\S+) type-level=(\S+).*?hits: ([^;]*)", fs[0][1]).groups() if fs else ("no-row", "no-row", "-")
own = []; clear = []; other = []
for n, t in ws:
    pos = re.search(r" at (\S+) in (\S+)", t).groups()
    if "value=const 0" in t: clear.append((n, pos))
    elif pos[0] == upos and ("key=*%s argument" % ucal) in t: own.append((n, pos))
    else:
        via = re.search(r"key=\*(\S+) argument", t); other.append((n, pos, via.group(1) if via else re.search(r"value=(\S+)", t).group(1)))
rd = []
for n, t in S["reads"]:
    if (" reads field " + fld + " ") not in t: continue
    u = re.search(r"use=\{([^}]*)\}", t).group(1); fn = t.split()[0]
    a = re.search(r"arg (\d+) of site (\d+)", t)
    cal = site.get(a.group(2), ("?",))[0] if a else None
    if cal == ucal and a and site[a.group(2)][1] == upos: continue
    rd.append((n, fn, u, cal))
nonun = [t for n, t in S["atype"] if t.split()[0] == fld and t.split()[1] != "untyped-write"]
print("  writers of %s: %d rows [WRITERS]: the lookup's own out store %d (view line %s); clearing or initial (constant 0) %d; other %d" % (fld, len(ws), len(own), [n for n, _ in own], len(clear), len(other)))
for n, pos, via in other: print("    other definition: view line %d at %s in %s through %s%s" % (n, pos[0], pos[1].split("@")[0], via, " (the same function as the use)" if pos[1] == ufn else ""))
print("  FIELD_STATUS (view line %s): per-field=%s type-level=%s hits %s; address-type hits on the field other than untyped writes: %d %s" % (fs[0][0] if fs else "-", pf[0], pf[1], pf[2], len(nonun), sorted(set(t.split()[1] + "@" + re.search(r" at (\S+)", t).group(1).split("/")[-1] for t in nonun))))
c = collections.Counter((fn.split("@")[0], cal or u) for n, fn, u, cal in rd)
print("  later readers of %s: %d rows [READS]: %s" % (fld, len(rd), "; ".join("%s -> %s x%d" % (k[0], k[1], v) for k, v in sorted(c.items()))))
if other: res = "NOT HELD: the holder has another definition (%s)" % "; ".join("%s through %s" % (p[0].split("/")[-1], via) for n, p, via in other)
elif not rd: res = "NOT HELD: no later reader of the holder"
elif pf[0] != "writer-set-complete": res = "NOT ESTABLISHED: no other definition among the %d listed writers and %d later readers, but the writer set of the holder is not complete (per-field=%s)" % (len(ws), len(rd), pf[0])
else: res = "HELD: the only non-clearing writer is the lookup's out store, the writer set is complete (per-field), later readers %d" % len(rd)
print("  branch (i): " + res)
