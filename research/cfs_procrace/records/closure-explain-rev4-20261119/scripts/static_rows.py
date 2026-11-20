#!/usr/bin/env python3
"""static_rows.py DIR... -- the use-condition lines of every path A record of the static dry runs, per branch (reads only)."""
import json, os, sys
for d in sys.argv[1:]:
    for br in sorted(os.listdir(d)):
        rd = os.path.join(d, br, "records")
        if not os.path.isdir(rd):
            continue
        for fn in sorted(os.listdir(rd)):
            r = json.load(open(os.path.join(rd, fn)))
            if r["path"] != "A":
                continue
            uc = r["use_cond"]
            iii = "; ".join("ctx %s %s (%s)" % (x["context"], x["status"], (x.get("why") or "")[:90]) for x in uc["branch_iii"])
            print("%s\t%s\t%s\t%s\tuse_cond=%s\tc1=%s c2=%s c4=%s\t(i) %s (%s)\t(iii) %s\tuses=%d\tstored_field_rule=%d" % (
                os.path.basename(d), br, r["row_id"]["row_counter"], r["row_id"]["use_site_key"], uc["status"], uc["callee_check"]["status"],
                uc["reach"]["status"], uc["writers"]["status"], uc["branch_i"]["status"], (uc["branch_i"].get("why") or "")[:90], iii,
                r["uses"], len(uc["stored_field_rule"])))
