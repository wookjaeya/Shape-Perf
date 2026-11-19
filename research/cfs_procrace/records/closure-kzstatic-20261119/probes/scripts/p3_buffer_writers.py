#!/usr/bin/env python3
# P3: for every `stored-name` line of a name-space data file, find the buffer field declared at decl=,
# its FIELD_STATUS row and its WRITERS rows in a facts view, and classify the writers.
import re, sys
nsd, view = sys.argv[1], sys.argv[2]
sec = {}
cur = None
for ln, l in enumerate(open(view, errors="replace"), 1):
    if l.startswith("["):
        cur = l.split("]")[0][1:]; sec[cur] = []
        continue
    if cur in ("ftypes", "fstatus", "writers", "locs", "targets", "atype"):
        sec[cur].append((ln, l.rstrip("\n")))
kind = None
out = []
for i, l in enumerate(open(nsd), 1):
    m = re.match(r"kind (\S+) value=(\d+)", l)
    if m: kind = (m.group(1), m.group(2)); continue
    m = re.match(r"\s+stored-name (\S+) at=(\S+) extent=(\S+) element=(\S+) decl=(\S+)", l)
    if not m: continue
    fn, at, ext, el, decl = m.groups()
    fld = [(n, t) for n, t in sec["ftypes"] if (" at " + decl + ":") in t]
    name = fld[0][1].split()[0] if fld else None
    row = {"nsd_line": i, "kind": kind[0], "value": kind[1], "create": fn, "copy_at": at, "decl": decl, "field": name}
    if name:
        fs = [(n, t) for n, t in sec["fstatus"] if t.split()[0] == name]
        ws = [(n, t) for n, t in sec["writers"] if t.split()[0] == name]
        tg = [(n, t) for n, t in sec["targets"] if name in t.split()]
        row["extent"] = re.search(r"extent (\S+ \d+)", fld[0][1]).group(1) if re.search(r"extent (\S+ \d+)", fld[0][1]) else "?"
        row["fstatus"] = (re.search(r"per-field=(\S+) type-level=(\S+)", fs[0][1]).groups() + (re.search(r"hits: ([^;]*)", fs[0][1]).group(1),)) if fs else None
        row["fstatus_line"] = fs[0][0] if fs else None
        row["target"] = bool(tg)
        cls = {"copy-at-entry": 0, "fill0-at-entry": 0, "fill0-elsewhere": 0, "other": 0}
        oth = []
        for n, t in ws:
            pos = re.search(r" at (\S+?):(\d+):\d+ in (\S+)", t)
            f, line = pos.group(1), pos.group(2)
            here = (f + ":" + line) == at
            fill0 = "value=const 0" in t
            if here and fill0: cls["fill0-at-entry"] += 1
            elif here: cls["copy-at-entry"] += 1
            elif fill0: cls["fill0-elsewhere"] += 1; oth.append("%d:%s:%s fill0 in %s" % (n, f, line, pos.group(3)))
            else: cls["other"] += 1; oth.append("%d:%s:%s in %s %s" % (n, f, line, pos.group(3), re.search(r"value=(\S+)", t).group(1)))
        row["writers"] = len(ws); row["cls"] = cls; row["oth"] = oth
        esc = [t for n, t in sec["atype"] if t.split()[0] == name and t.split()[1] != "untyped-write"]
        row["atype_hits"] = len(esc)
        row["atype_kinds"] = sorted(set(t.split()[1] for t in esc))
    out.append(row)
print("# name-space data: %s\n# facts view: %s" % (nsd, view))
print("# columns: nsd_line kind value create copy_at field extent target per-field type-level hits writers copy-at-entry fill0-at-entry fill0-elsewhere other atype-hits(non-untyped)")
held = 0
for r in out:
    if not r["field"]:
        print("%d %s %s %s %s field=NOT-IN-FIELD_TYPES" % (r["nsd_line"], r["kind"], r["value"], r["create"], r["copy_at"])); continue
    fs = r["fstatus"] or ("no-row", "no-row", "-")
    c = r["cls"]
    ok = fs[0] == "writer-set-complete" and c["other"] == 0 and c["copy-at-entry"] >= 1
    held += ok
    print("%d %s %s %s %s %s extent=%s target=%s per-field=%s type-level=%s hits[%s] writers=%d copy-at-entry=%d fill0-at-entry=%d fill0-elsewhere=%d other=%d atype=%d%s => condition-input %s" % (
        r["nsd_line"], r["kind"], r["value"], r["create"], r["copy_at"], r["field"], r["extent"], r["target"], fs[0], fs[1], fs[2],
        r["writers"], c["copy-at-entry"], c["fill0-at-entry"], c["fill0-elsewhere"], c["other"], r["atype_hits"], r["atype_kinds"] or "",
        "complete-and-only-create-copy-or-clear" if ok else "NOT-HELD"))
    for o in r["oth"]: print("      other writer: " + o)
print("# entries %d, with a buffer field in FIELD_TYPES %d, input held %d" % (len(out), sum(1 for r in out if r["field"]), held))
