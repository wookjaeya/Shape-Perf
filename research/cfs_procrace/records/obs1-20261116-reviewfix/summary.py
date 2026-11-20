#!/usr/bin/env python3
"""summary.py -- counts over the explanation records of one explain output directory (python3 -I summary.py DIR).

Revision 2 (after review) of records/obs1-20261116-explain/summary.py: a reason is counted once per record by its base
name (the set of bases per record), and the header says so; the occurrences of the sub-reasons are a separate count."""
import collections, glob, json, os, sys
D = sys.argv[1]
recs = [json.load(open(f)) for f in sorted(glob.glob(os.path.join(D, "records", "*.json")))]
print("records %d (rows %d)" % (len(recs), len({r["row_id"]["use_site_key"] for r in recs})))
C = collections.Counter
def show(title, c):
    print("== " + title)
    for k, n in sorted(c.items(), key=lambda x: (-x[1], str(x[0]))):
        print("  %4d  %s" % (n, k))
show("per row: records", C(r["row_id"]["row_counter"] for r in recs))
show("violation", C(r["tool_verdict"]["violation"]["value"] for r in recs))
show("odf", C(r["tool_verdict"]["odf"]["value"] for r in recs))
show("headline", C(r["tool_verdict"]["headline"] for r in recs))
for ax in ("link", "use_cond", "order", "intervention", "effect"):
    show(ax + ".status", C(r[ax]["status"] for r in recs))
VR = lambda r: r["tool_verdict"]["violation"]["undetermined_reasons"]
show("violation.undetermined_reasons: records per base reason (each record counted once per base)",
     C(b for r in recs for b in {x.split("(")[0] for x in VR(r)}))
show("violation.undetermined_reasons: occurrences per base reason (sub-reasons counted separately)",
     C(x.split("(")[0] for r in recs for x in VR(r)))
show("violation.undetermined_reasons with sub-reason: records", C(x for r in recs for x in set(VR(r))))
show("link.missing: records per base reason", C(b for r in recs for b in {x.split("(")[0] for x in r["link"]["missing"]}))
show("need states (needs over all records)", C(n["state"] for r in recs for n in r["incompleteness"]["needs"]))
show("need reasons (needs over all records)", C(n["reason"] for r in recs for n in r["incompleteness"]["needs"] if n.get("reason")))
show("odf failure_sign: row, consumer app, status name", C((r["row_id"]["row_counter"], r["consumer_actor"].get("app", {}).get("app"), u["name"]) for r in recs if r["tool_verdict"]["odf"]["value"] == "failure_sign" for u in r["tool_verdict"]["odf"]["failing_uses"][:1]))
show("build_id: plan executables per record", C(len(r["build_id"].get("executables_plan") or {}) for r in recs))
show("budget.run.stop.fired", C(str((r["budget"]["run"].get("stop") or {}).get("fired")) for r in recs))
