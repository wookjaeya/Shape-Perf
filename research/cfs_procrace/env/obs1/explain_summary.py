#!/usr/bin/env python3
"""explain_summary.py -- the summary of the explanation records of revision 4 (spec 3 item 13 and 11 items 7-9, 11;
evaluation protocol plan/c1_closure_evaluation_protocol_20261118.md sections 8, 10 and 14.1-14.4).

    python3 -I explain_summary.py --out FILE --uses USES.tsv [--uses USES.tsv ...] BOOT_LABEL=EXPLAIN_OUT_DIR [...]
    python3 -I explain_summary.py --check-summary FILE

Reads the index.tsv and records of every branch of every boot given, and the lists of pre-defined uses (kz-uses 1, the
denominator of evaluation protocol 14.2); writes, per branch:
  - one head line: "main (declared before the boot: D1/R0)" for the branch D1-R0 only (evaluation protocol 14.1; the
    constant D1-R0 is a branch id, not a row or key), "auxiliary (data <data>, readings <readings>)" for every other
    branch; the judgement part's eval_branch.tag is copied unchanged;
  - verdict x pair kind x order value, per-boot distributions of every record (row, consumer root), the undetermined
    reasons by kind, the records with a (c3)(iii) branch decided by the stored-field rule, other_order_seen_in_boots;
  - the per-use table (spec 11 item 8): per boot reached / defined and decided / reached with the undetermined uses by
    kind (spec 10.7-8), per use its distribution over the boots and its event count, and the records whose uses share
    one verdict;
  - the pair table (spec 11 item 9): the eight pair kinds each on its own line (0 included), the unknown kind split by
    the actors' owner state, the between-app line (app-app), the control lines (one app, one actor), the failure
    signs by pair-kind set and by the consumer's affiliation, and the legend of the static unordered value.
It writes no "detection" number and no sentence that words a claim; decided = observed, not_observed_in_runs,
refuted_link.  It reads no adjudication sheet and no expectation file, and changes no record.  The form of its output
is checked at the end (and by --check-summary): one head line per branch table, main only on D1-R0, one row per
pre-defined use, the eight pair-kind lines.  rc 0 written and checked; 3 the written file fails its form check;
2 a uses list is not of its form; 64 usage.
"""
import argparse
import json
import os
import re
import sys

REV = "explain_summary.py revision 4, spec 11 items 7-9 (2026-11-19)"
MAIN = "D1-R0"
MAIN_HEAD = "main (declared before the boot: D1/R0)"
DECIDED = ("observed", "not_observed_in_runs", "refuted_link")
KIND = {"no_run": "run evidence", "target_not_reached": "run evidence", "record_insufficient": "run evidence",
        "absent_result_unevaluated": "static input", "key_unresolved": "static input", "writer_set_incomplete": "static input",
        "fixed_proof_open": "static input", "caller_dependence_unknown": "static input", "actor_not_extracted": "static input",
        "plan_missing": "resource", "budget_truncated": "resource", "link_unlinked": "structure", "multi_state_unlinked": "structure",
        "build_mismatch": "structure", "intervention_out_of_scope": "structure"}
KINDS = ["run evidence", "static input", "resource", "structure", "contradiction", "other"]
PAIR_KINDS = ["app-app", "core-app", "process-app", "same-app-other", "same-app-main-child", "same_actor", "core-core", "unknown"]
PAIR_GROUP = {"app-app": "between apps (both owners determined)", "core-app": "core and app", "process-app": "process and app",
              "core-core": "core and core", "same-app-other": "control (one app)", "same-app-main-child": "control (one app)",
              "same_actor": "control (one actor)", "unknown": "own line (see the split below)"}
USE_COLS = ["build", "row_label", "row_key", "path", "consumer_root", "use_tu", "use_line", "use_col", "callee", "at_site",
            "after_site", "context_id", "frame_sites", "plan_state"]


def reason_kind(r):
    if "absence after a live create of the same name" in r:
        return "contradiction"
    return KIND.get(r.split("(", 1)[0], "other")


def read_uses(path):
    with open(path, encoding="utf-8") as f:
        lines = f.read().splitlines()
    if not lines or lines[0] != "kz-uses 1" or lines[-1] != "end":
        return None, "%s is not a kz-uses 1 file" % path
    cols = [ln for ln in lines if ln.startswith("columns ")]
    if not cols or cols[0][len("columns "):].split("\t") != USE_COLS:
        return None, "%s: the columns line is not the fourteen columns" % path
    body = [ln.split("\t") for ln in lines[1:-1] if not ln.startswith(("rule ", "columns ", "lines "))]
    tail = [ln for ln in lines if ln.startswith("lines ")]
    if not tail or not tail[0].startswith("lines %d " % len(body)) or any(len(x) != 14 for x in body):
        return None, "%s: the tail count or the columns of a line are wrong" % path
    return [dict(zip(USE_COLS, x)) for x in body], None


def root_of(r):
    ca = r.get("consumer_actor") or {}
    rt = ca.get("root_tie") or {}
    if rt.get("root"):
        return "%s (%s)" % (rt["root"], rt.get("kind"))
    if ca.get("static_root"):
        return "%s (%s)" % (ca["static_root"], ca.get("static_kind"))
    return "none"


def owner_class(view):
    a = (view or {}).get("app") or {}
    if a.get("status") != "determined":
        return "unknown"
    return {"app": "app", "child": "app", "core": "core", "process": "process"}.get(a.get("class"), "unknown")


def use_events_of(r, u):
    """The record's use events of one pre-defined use: its use call (at or after site) and its context ('-' in the list:
    the row has no context line, every context of the record counts)."""
    out = []
    for e in r["order"]["use_events"]:
        sites = {str((e.get("at_call") or {}).get("site")), str((e.get("after_call") or {}).get("site"))}
        if not ({u["at_site"], u["after_site"]} & sites):
            continue
        if u["context_id"] != "-" and str(e.get("context")) != u["context_id"]:
            continue
        out.append(e)
    return out


def check_summary(lines):
    """Spec 11 items 7-9: the form of a summary text.  Returns the list of failures."""
    bad = []
    tables, cur = [], None
    for ln in lines:
        if ln.startswith("table "):
            cur = {"id": ln.split()[1], "heads": [], "uses_defined": None, "use_rows": 0, "pair_lines": set()}
            tables.append(cur)
        elif cur is not None:
            if ln.startswith("  head: "):
                cur["heads"].append(ln[len("  head: "):])
            elif ln.startswith("  per use (defined "):
                m = re.match(r"  per use \(defined (\d+)\)", ln)
                cur["uses_defined"] = int(m.group(1)) if m else None
            elif ln.startswith("    use\t"):
                cur["use_rows"] += 1
            elif ln.startswith("    pair_kind\t"):
                cur["pair_lines"].add(ln.split("\t")[1])
    if not tables:
        bad.append("no branch table")
    for t in tables:
        if len(t["heads"]) != 1:
            bad.append("table %s: %d head lines (one is required)" % (t["id"], len(t["heads"])))
        for h in t["heads"]:
            if h.startswith("main") and t["id"] != MAIN:
                bad.append("table %s: a main head on a branch that is not %s" % (t["id"], MAIN))
            if t["id"] == MAIN and not h.startswith(MAIN_HEAD):
                bad.append("table %s: the main branch without the main head" % t["id"])
            if t["id"] != MAIN and not h.startswith("auxiliary (data "):
                bad.append("table %s: an auxiliary branch without the auxiliary head" % t["id"])
        if t["uses_defined"] is None or t["use_rows"] != t["uses_defined"]:
            bad.append("table %s: %d use rows for %s pre-defined uses" % (t["id"], t["use_rows"], t["uses_defined"]))
        miss = [k for k in PAIR_KINDS if k not in t["pair_lines"]]
        if miss:
            bad.append("table %s: no pair_kind line of %s" % (t["id"], ", ".join(miss)))
    return bad


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("--out")
    ap.add_argument("--uses", action="append", default=[])
    ap.add_argument("--check-summary")
    ap.add_argument("boots", nargs="*")
    o = ap.parse_args(argv)
    if o.check_summary:
        with open(o.check_summary, encoding="utf-8") as f:
            bad = check_summary(f.read().splitlines())
        for x in bad:
            print("form: " + x)
        print("summary form %s (%d failures)" % ("ok" if not bad else "FAILED", len(bad)))
        return 3 if bad else 0
    if not o.out or not o.boots or not o.uses:
        print("explain_summary.py: --out, --uses and at least one BOOT=DIR are required", file=sys.stderr)
        return 64
    if os.path.exists(o.out):
        print("explain_summary.py: %s exists" % o.out, file=sys.stderr)
        return 64
    uses = []
    for p in o.uses:
        u, why = read_uses(p)
        if u is None:
            print("explain_summary.py: " + why, file=sys.stderr)
            return 2
        uses.extend(u)
    per = {}    # branch -> list of (boot, record, file)
    boots_all = []
    for spec in o.boots:
        boot, d = spec.split("=", 1)
        boots_all.append(boot)
        for br in sorted(os.listdir(d)):
            idx = os.path.join(d, br, "index.tsv")
            if not os.path.isfile(idx):
                continue
            with open(idx) as f:
                rows = f.read().splitlines()[1:]
            per.setdefault(br, [])
            for ln in rows:
                fn = ln.split("\t")[-1]
                with open(os.path.join(d, br, "records", fn)) as f:
                    per[br].append((boot, json.load(f), fn))
    builds = sorted({r["build_id"]["build"] for recs in per.values() for _, r, _ in recs})
    my_uses = [u for u in uses if u["build"] in builds] if builds else uses
    out = ["# %s: tables per evaluation branch; decided = observed, not_observed_in_runs, refuted_link; "
           "unordered (static) = this revision found no edge; uses lists: %s" % (REV, ", ".join(os.path.basename(p) for p in o.uses))]
    for br in sorted(per):
        recs = per[br]
        eb = recs[0][1].get("eval_branch", {}) if recs else {"id": br}
        tag = eb.get("tag")
        out.append("")
        out.append("table %s" % br)
        if br == MAIN:
            head = MAIN_HEAD
        else:
            head = "auxiliary (data %s, readings %s)" % (eb.get("data", "?"), ",".join(eb.get("readings") or ["?"]))
        out.append("  head: %s%s" % (head, (" [%s]" % tag) if tag else ""))
        boots = sorted({b for b, _, _ in recs}) or sorted(boots_all)
        out.append("boots %d: %s" % (len(boots), " ".join(boots)))
        cube = {}
        for b, r, _ in recs:
            v = r["tool_verdict"]["violation"]["value"]
            pks = sorted(set(r.get("pair_kind", {}).values())) or ["-"]
            ov = r["order"]["static"]
            for pk in pks:
                cube[(v, pk, ov)] = cube.get((v, pk, ov), 0) + 1
        out.append("violation x pair_kind x order (records; a record with publishers of several kinds counts once per kind):")
        for k in sorted(cube):
            out.append("  %s\t%s\t%s\t%d" % (k[0], k[1], k[2], cube[k]))
        dist = {}
        for b, r, _ in recs:
            key = (r["row_id"]["row_counter"], r["row_id"]["use_site_key"], str((r["consumer_actor"].get("root_tie") or {}).get("root") or r["consumer_actor"].get("static_root")))
            dist.setdefault(key, {})
            v = r["tool_verdict"]["violation"]["value"]
            dist[key][v] = dist[key].get(v, 0) + 1
        out.append("per record over boots (row, use-site key, consumer root: values):")
        for k in sorted(dist):
            n = sum(dist[k].values())
            out.append("  %s %s %s: %d boots: %s" % (k[0], k[1], k[2], n, ", ".join("%s %d" % x for x in sorted(dist[k].items()))))
        und = {}
        for b, r, _ in recs:
            if r["tool_verdict"]["violation"]["value"] in DECIDED:
                continue
            for x in r["tool_verdict"]["violation"]["undetermined_reasons"]:
                und[reason_kind(x)] = und.get(reason_kind(x), 0) + 1
        sfr = sum(1 for b, r, _ in recs if r["use_cond"].get("stored_field_rule"))
        out.append("records with a (c3)(iii) branch decided by the stored-field rule (counted apart; not for statements outside "
                   "the design rows): %d" % sfr)
        out.append("undetermined reasons by kind (reasons, over records): " + (", ".join("%s %d" % x for x in sorted(und.items())) or "none"))
        seen = {}
        for b, r, _ in recs:
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
        # ---- per-use table (spec 11 item 8; evaluation protocol 14.2)
        out.append("  per use (defined %d): the unit of evaluation protocol 14.2 is one pre-defined use in one boot of one build; "
                   "its verdict is the violation of the record (build, row, consumer actor) it is in" % len(my_uses))
        cells = {}      # (use index, boot) -> list of (verdict, events, file, reasons)
        for i, u in enumerate(my_uses):
            for b, r, fn in recs:
                if r["build_id"]["build"] != u["build"] or r["row_id"]["use_site_key"] != u["row_key"] or root_of(r) != u["consumer_root"]:
                    continue
                ev = use_events_of(r, u)
                cells.setdefault((i, b), []).append((r["tool_verdict"]["violation"]["value"], len(ev), fn,
                                                     r["tool_verdict"]["violation"]["undetermined_reasons"]))
        for b in boots:
            reached = [i for i in range(len(my_uses)) if any(x[1] > 0 for x in cells.get((i, b), []))]
            decided = [i for i in reached if any(x[0] in DECIDED for x in cells[(i, b)] if x[1] > 0)]
            kinds = {}
            for i in reached:
                if i in decided:
                    continue
                ks = {reason_kind(r) for x in cells[(i, b)] if x[1] > 0 for r in x[3]}
                for k in ks:
                    kinds[k] = kinds.get(k, 0) + 1
            out.append("    boot %s: reached %d / defined %d; decided %d / reached %d; undetermined uses by kind (a use counts once in "
                       "each kind its record's reasons have): %s" % (b, len(reached), len(my_uses), len(decided), len(reached),
                                                                    ", ".join("%s %d" % (k, kinds.get(k, 0)) for k in KINDS)))
        shared = {}
        for i, u in enumerate(my_uses):
            dist_u, evn = {}, 0
            for b in boots:
                xs = cells.get((i, b), [])
                hit = [x for x in xs if x[1] > 0]
                evn += sum(x[1] for x in hit)
                if not hit:
                    v = "not reached"
                else:
                    vs = sorted({x[0] for x in hit})
                    v = vs[0] if len(vs) == 1 else "mixed(%s)" % ",".join(vs)
                    for x in hit:
                        shared.setdefault((b, x[2]), set()).add(i)
                dist_u[v] = dist_u.get(v, 0) + 1
            out.append("    use\t%s\t%s\t%s\t%s\tcontext %s\t%d boots: observed %d, not observed %d, refuted %d, undetermined %d, not reached %d%s\tevents %d" % (
                u["build"], u["row_label"], u["row_key"], u["consumer_root"], u["context_id"], len(boots),
                dist_u.get("observed", 0), dist_u.get("not_observed_in_runs", 0), dist_u.get("refuted_link", 0), dist_u.get("undetermined", 0),
                dist_u.get("not reached", 0), "".join(", %s %d" % (k, n) for k, n in sorted(dist_u.items()) if k.startswith("mixed")), evn))
        sh = sorted({fn for (b, fn), idx in shared.items() if len(idx) > 1})
        out.append("    the uses of one record share its one verdict (evaluation protocol 14.2): %s" % (", ".join(sh) if sh else "no record holds two reached uses"))
        # ---- pair table (spec 11 item 9; evaluation protocol 14.4)
        out.append("  pairs (actor pairs consumer x publisher of every record, over the boots; legend: static unordered = this revision found no edge, "
                   "not that the other order is feasible):")
        pk_n = {k: 0 for k in PAIR_KINDS}
        unk = {"a owner not determined": 0, "b core-process with both owners determined": 0, "c other": 0}
        for b, r, _ in recs:
            views = {v.get("run_actor"): v for v in r.get("publisher_actors", [])}
            cv = r.get("consumer_actor") or {}
            for a, k in sorted(r.get("pair_kind", {}).items()):
                pk_n[k] = pk_n.get(k, 0) + 1
                if k != "unknown":
                    continue
                pv = cv if a == cv.get("run_actor") else views.get(a, {})
                oc, op = owner_class(cv), owner_class(pv)
                if "unknown" in (oc, op):
                    unk["a owner not determined"] += 1
                elif {oc, op} == {"core", "process"}:
                    unk["b core-process with both owners determined"] += 1
                else:
                    unk["c other"] += 1
        for k in PAIR_KINDS:
            out.append("    pair_kind\t%s\t%d\t%s" % (k, pk_n.get(k, 0), PAIR_GROUP[k]))
        out.append("    between apps (the headline line: app-app, both owners determined): %d" % pk_n["app-app"])
        out.append("    controls (not between-app results): same-app-other %d, same-app-main-child %d, same_actor %d" % (
            pk_n["same-app-other"], pk_n["same-app-main-child"], pk_n["same_actor"]))
        out.append("    unknown split by the actors' owner state: " + ", ".join("%s %d" % x for x in unk.items()))
        fs_pk, fs_aff = {}, {"app": 0, "core": 0, "process": 0, "unknown": 0}
        for b, r, _ in recs:
            if r["tool_verdict"]["odf"]["value"] != "failure_sign":
                continue
            k = ",".join(sorted(set(r.get("pair_kind", {}).values()))) or "-"
            fs_pk[k] = fs_pk.get(k, 0) + 1
            fs_aff[owner_class(r.get("consumer_actor"))] += 1
        out.append("    failure signs (records with odf failure_sign) by pair-kind set: " + (", ".join("%s %d" % x for x in sorted(fs_pk.items())) or "none"))
        out.append("    failure signs by the consumer's affiliation (a core consumer's sign is no between-app signal): " +
                   ", ".join("%s %d" % x for x in fs_aff.items()))
    with open(o.out, "w") as f:
        f.write("\n".join(out) + "\n")
    bad = check_summary(out)
    for x in bad:
        print("form: " + x)
    print("summary branches %d uses %d form %s" % (len(per), len(my_uses), "ok" if not bad else "FAILED"))
    return 3 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
