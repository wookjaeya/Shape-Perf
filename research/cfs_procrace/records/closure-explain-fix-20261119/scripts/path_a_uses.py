#!/usr/bin/env python3
"""path_a_uses.py DIR... -- per explanation output, per branch given by PATH_A_BRANCHES (default D1-R0,D2-R0): every path A
record's use events (outcome, context, name state, instance link), its (c3)(iii) entries, its pairs' result and missing
items with the (na) and (da) reasons, use_cond and violation.  Reads only (for the synthetic dry runs of results 26.3)."""
import json
import os
import sys

brs = os.environ.get("PATH_A_BRANCHES", "D1-R0,D2-R0").split(",")
for d in sys.argv[1:]:
    for br in brs:
        rd = os.path.join(d, br, "records")
        if not os.path.isdir(rd):
            continue
        for fn in sorted(os.listdir(rd)):
            r = json.load(open(os.path.join(rd, fn)))
            if r["path"] != "A":
                continue
            uc = r["use_cond"]
            ues = r["order"]["use_events"]
            print("%s\t%s\t%s\t%s\tuse_cond=%s violation=%s link=%s\tuses %d: %s" % (
                d.rstrip("/").split("/")[-2] if d.endswith("explain") or d.endswith("explain/") else os.path.basename(d.rstrip("/")), br,
                r["row_id"]["row_counter"], fn.split(".actor-")[-1][:-5], uc["status"], r["tool_verdict"]["violation"]["value"], r["link"]["status"],
                len(ues), " | ".join("%s ctx=%s name=%s link=%s" % (u["outcome"], u["context"], u.get("name_state"), (u.get("instance_link") or {}).get("status"))
                                     for u in ues)))
            print("\t\t(iii): %s" % "; ".join("%s:%s" % (x["context"], x["status"]) for x in uc["branch_iii"]))
            for p in r["link"]["name_binding"]["pairs"][:3]:
                it = p["items"]
                print("\t\tpair use %s create %s %s: %s; missing %s; (na) %s; (da) %s" % (p["use"], p["create_actor"], p["pair_kind"], p["result"],
                                                                                     ",".join(p["missing_items"]), it["na_semantics"]["reason"], it["da_no_name_mapping"]["reason"]))
