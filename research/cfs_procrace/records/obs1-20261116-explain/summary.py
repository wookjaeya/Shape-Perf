#!/usr/bin/env python3
"""summary.py -- counts over the explanation records of one explain output directory (python3 -I summary.py DIR)."""
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
show("violation.undetermined_reasons (reason, records)", C(x.split("(")[0] for r in recs for x in set(r["tool_verdict"]["violation"]["undetermined_reasons"])))
show("violation.undetermined_reasons with sub-reason (records)", C(x for r in recs for x in set(r["tool_verdict"]["violation"]["undetermined_reasons"])))
show("need states (needs over all records)", C(n["state"] for r in recs for n in r["incompleteness"]["needs"]))
show("odf failure_sign: row, consumer app, status name", C((r["row_id"]["row_counter"], r["consumer_actor"].get("app", {}).get("app"), u["name"]) for r in recs if r["tool_verdict"]["odf"]["value"] == "failure_sign" for u in r["tool_verdict"]["odf"]["failing_uses"][:1]))
