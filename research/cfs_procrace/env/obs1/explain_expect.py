#!/usr/bin/env python3
"""explain_expect.py -- compare a filled expectation file (kz-expect 1, spec 10.6) with explanation records of revision 4.

    python3 -I explain_expect.py --expect FILE --records EXPLAIN_OUT_DIR --out FILE

One output line per expectation: match, mismatch (with the value found) or no_record, with the line's class.  It never
changes a record.  rc 0 written (whatever the matches), 2 the expectation file is not of its form, 64 usage.
Spec 11 item 12 (2026-11-19): every expect line has an eighth column class, one of boot (a boot can make it fail),
static (the frozen judgement part gives it from the static inputs alone), format (only a script error can make it
fail) (evaluation protocol 14.6); a line without it or with another value refuses the file (rc 2).  The tail gives the
counts per class and result.
"""
import argparse
import json
import os
import re
import sys

CLASSES = ("boot", "static", "format")

def get(rec, field):
    m = re.match(r"pair\[(.+)\]\.(missing_items|result)$", field)
    if m:
        ps = [p for p in rec["link"].get("name_binding", {}).get("pairs", []) if p.get("create_pos") == m.group(1)]
        if not ps:
            return None
        if m.group(2) == "missing_items":
            return ",".join(sorted(set(x for p in ps for x in p["missing_items"])))
        return ",".join(sorted({p["result"] for p in ps}))
    m = re.match(r"use\[(\d+)\]\.(\w+)$", field)
    if m:
        ue = rec["order"]["use_events"]
        i = int(m.group(1))
        return ue[i].get(m.group(2)) if i < len(ue) else None
    d = rec
    for k in field.split("."):
        d = d.get(k) if isinstance(d, dict) else None
    return d


def norm(field, v):
    if v is None:
        return None
    if field.endswith(".missing_items"):
        return ",".join(sorted(x for x in str(v).split(",") if x))
    return str(v)


def main(argv):
    ap = argparse.ArgumentParser()
    for a in ("expect", "records", "out"):
        ap.add_argument("--" + a, required=True)
    o = ap.parse_args(argv)
    if os.path.exists(o.out):
        print("explain_expect.py: %s exists" % o.out, file=sys.stderr)
        return 64
    with open(o.expect, encoding="utf-8") as f:
        lines = f.read().splitlines()
    if not lines or lines[0] != "kz-expect 1" or lines[-1] != "end":
        print("explain_expect.py: not a kz-expect 1 file", file=sys.stderr)
        return 2
    ex = [ln.split("\t") for ln in lines if ln.startswith("expect\t")]
    tail = [ln for ln in lines if ln.startswith("lines ")]
    if not tail or tail[0] != "lines %d" % len(ex) or any(len(x) != 8 for x in ex):
        print("explain_expect.py: tail count or columns wrong (eight columns: spec 11 item 12)", file=sys.stderr)
        return 2
    if any(x[7] not in CLASSES for x in ex):
        print("explain_expect.py: a class outside %s" % ", ".join(CLASSES), file=sys.stderr)
        return 2
    out = ["# explain_expect.py revision 4, spec 11 item 12 (2026-11-19): expectation\tclass\tresult\tfound"]
    counts = {}
    per_class = {}
    for _, br, build, key, root, field, val, cls in ex:
        d = os.path.join(o.records, br, "records")
        recs = []
        if os.path.isdir(d):
            for fn in sorted(os.listdir(d)):
                if not fn.startswith("%s." % build) or (".%s." % key) not in fn:
                    continue
                with open(os.path.join(d, fn)) as f:
                    r = json.load(f)
                rr = (r["consumer_actor"].get("root_tie") or {}).get("root") or r["consumer_actor"].get("static_root")
                if root in ("*", rr):
                    recs.append(r)
        if not recs:
            res, found = "no_record", "-"
        else:
            fv = sorted({norm(field, get(r, field)) or "-" for r in recs})
            found = "|".join(fv)
            res = "match" if fv == [norm(field, val)] else "mismatch"
        counts[res] = counts.get(res, 0) + 1
        per_class[(cls, res)] = per_class.get((cls, res), 0) + 1
        out.append("\t".join([br, build, key, root, field, val, cls, res, found]))
    out.append("# by class: " + "; ".join("%s: %s" % (c, ", ".join("%s %d" % (r, per_class.get((c, r), 0)) for r in ("match", "mismatch", "no_record")))
                                         for c in CLASSES))
    out.append("# " + ", ".join("%s %d" % x for x in sorted(counts.items())))
    with open(o.out, "w") as f:
        f.write("\n".join(out) + "\n")
    print(out[-1][2:])
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
