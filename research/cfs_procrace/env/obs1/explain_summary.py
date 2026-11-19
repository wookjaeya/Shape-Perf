#!/usr/bin/env python3
"""explain_summary.py -- the summary of the explanation records of revision 4 (spec 3 item 13; evaluation protocol
plan/c1_closure_evaluation_protocol_20261118.md sections 8 and 10).

    python3 -I explain_summary.py --out FILE BOOT_LABEL=EXPLAIN_OUT_DIR [BOOT_LABEL=EXPLAIN_OUT_DIR ...]

Reads the index.tsv and records of every branch of every boot given; writes, per branch: verdict x pair kind x order
value, per-boot distributions of every record (row, consumer root), the undetermined reasons by kind, and
other_order_seen_in_boots per pair (the order of the use and the create seen across boots).  It writes no "detection"
number and no sentence that words a claim; decided = observed, not_observed_in_runs, refuted_link.  It changes no record.
"""
import argparse
import json
import os
import sys

DECIDED = ("observed", "not_observed_in_runs", "refuted_link")
KIND = {"no_run": "run evidence", "target_not_reached": "run evidence", "record_insufficient": "run evidence",
        "absent_result_unevaluated": "static input", "key_unresolved": "static input", "writer_set_incomplete": "static input",
        "fixed_proof_open": "static input", "caller_dependence_unknown": "static input", "actor_not_extracted": "static input",
        "plan_missing": "resource", "budget_truncated": "resource", "link_unlinked": "structure", "multi_state_unlinked": "structure",
        "build_mismatch": "structure", "intervention_out_of_scope": "structure"}


def reason_kind(r):
    if "absence after a live create of the same name" in r:
        return "contradiction"
    return KIND.get(r.split("(", 1)[0], "other")


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("boots", nargs="+")
    o = ap.parse_args(argv)
    if os.path.exists(o.out):
        print("explain_summary.py: %s exists" % o.out, file=sys.stderr)
        return 64
    per = {}    # branch -> list of (boot, record)
    for spec in o.boots:
        boot, d = spec.split("=", 1)
        for br in sorted(os.listdir(d)):
            idx = os.path.join(d, br, "index.tsv")
            if not os.path.isfile(idx):
                continue
            with open(idx) as f:
                rows = f.read().splitlines()[1:]
            for ln in rows:
                fn = ln.split("\t")[-1]
                with open(os.path.join(d, br, "records", fn)) as f:
                    per.setdefault(br, []).append((boot, json.load(f)))
    out = ["# explain_summary.py revision 4 (2026-11-19): tables per evaluation branch; decided = observed, "
           "not_observed_in_runs, refuted_link; unordered (static) = this revision found no edge"]
    for br in sorted(per):
        recs = per[br]
        tag = recs[0][1].get("eval_branch", {}).get("tag")
        out.append("")
        out.append("branch %s%s" % (br, (" [%s]" % tag) if tag else ""))
        boots = sorted({b for b, _ in recs})
        out.append("boots %d: %s" % (len(boots), " ".join(boots)))
        cube = {}
        for b, r in recs:
            v = r["tool_verdict"]["violation"]["value"]
            pks = sorted(set(r.get("pair_kind", {}).values())) or ["-"]
            ov = r["order"]["static"]
            for pk in pks:
                cube[(v, pk, ov)] = cube.get((v, pk, ov), 0) + 1
        out.append("violation x pair_kind x order (records; a record with publishers of several kinds counts once per kind):")
        for k in sorted(cube):
            out.append("  %s\t%s\t%s\t%d" % (k[0], k[1], k[2], cube[k]))
        dist = {}
        for b, r in recs:
            key = (r["row_id"]["row_counter"], r["row_id"]["use_site_key"], str((r["consumer_actor"].get("root_tie") or {}).get("root") or r["consumer_actor"].get("static_root")))
            dist.setdefault(key, {})
            v = r["tool_verdict"]["violation"]["value"]
            dist[key][v] = dist[key].get(v, 0) + 1
        out.append("per record over boots (row, use-site key, consumer root: values):")
        for k in sorted(dist):
            n = sum(dist[k].values())
            out.append("  %s %s %s: %d boots: %s" % (k[0], k[1], k[2], n, ", ".join("%s %d" % x for x in sorted(dist[k].items()))))
        und = {}
        for b, r in recs:
            if r["tool_verdict"]["violation"]["value"] in DECIDED:
                continue
            for x in r["tool_verdict"]["violation"]["undetermined_reasons"]:
                und[reason_kind(x)] = und.get(reason_kind(x), 0) + 1
        sfr = sum(1 for b, r in recs if r["use_cond"].get("stored_field_rule"))
        out.append("records with a (c3)(iii) branch decided by the stored-field rule (counted apart; not for statements outside "
                   "the design rows): %d" % sfr)
        out.append("undetermined reasons by kind (reasons, over records): " + (", ".join("%s %d" % x for x in sorted(und.items())) or "none"))
        seen = {}
        for b, r in recs:
            for p in r["link"].get("name_binding", {}).get("pairs", []):
                ui = p["use"]
                ue = r["order"]["use_events"][ui] if ui < len(r["order"]["use_events"]) else {}
                ut = (ue.get("at_call") or {}).get("t_ms")
                ct = next((c.get("at_call", {}).get("t_ms") for c in r["order"]["publish_events"]
                           if c.get("actor") == p["create_actor"] and (c.get("at_call") or {}).get("aseq") == p["create_aseq"]), None)
                if ut is None or ct is None:
                    continue
                k = (r["row_id"]["use_site_key"], p["create_pos"], p["pair_kind"])
                seen.setdefault(k, set()).add("use first" if ut < ct else "create first")
        out.append("other_order_seen_in_boots per pair (row key, create site, pair kind):")
        for k in sorted(seen):
            out.append("  %s %s %s: %s; other order seen: %s" % (k[0], k[1], k[2], " / ".join(sorted(seen[k])), "yes" if len(seen[k]) > 1 else "no"))
        if not seen:
            out.append("  none (no pair with both at-calls recorded)")
    with open(o.out, "w") as f:
        f.write("\n".join(out) + "\n")
    print("summary branches %d" % len(per))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
