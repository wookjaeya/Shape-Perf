#!/usr/bin/env python3
"""test_explain_eval.py -- fixtures of the falsifier and evaluation script (env/obs1/explain_eval.py; spec 11 item 13).

    python3 -I test_explain_eval.py OBS1_DIR WORK_DIR

Every fixture is a synthetic world (no case name, no app, function or key of a case): a relations text with two rows (a
control row with a found alternatives line and a holder with a second definition, and a tier+control row), a sidecar,
a pair file, a list of pre-defined uses, the static and per-boot sheet parts, an expectation file, a gate result and
an explanation output of two branches (D1-R0, D2-R0) whose records are written directly (the judgement part is not
run).  The base world passes every check; each case changes one thing and checks that the line of that check says what
it must (fail, not_evaluable, met, unmet, not_counted, undefined, a value) and that the class is the one of the rule.
Then: the script opens the expectation file only inside F1, and writes only its output (both checked by an open guard,
each with a negative control: a copy of the script that does otherwise is caught).  Prints one line per check and a
count; rc 0 when every check passes.
"""
import copy
import json
import os
import re
import shutil
import subprocess
import sys

OBS, WORK = sys.argv[1], sys.argv[2]
PY = sys.executable
RESULTS = []
K1, K2 = "00000000000000c1", "00000000000000c2"
REL = "relations.txt"


def ok(name, cond, detail=""):
    RESULTS.append((name, bool(cond)))
    print("%s %s%s" % ("PASS" if cond else "FAIL", name, ("" if cond else "  [%s]" % str(detail)[:400])))


def world():
    W = {}
    W["rel"] = {
        "C1": ["  C1 lookup app/u.c:10:5 in u_init -> Ns_Find#0 namespace Ns_t.name partition {2} key known",
               "      holder: field U_t.id; writers 2: other definition app/u.c:20:5 in u_x value=symbolic [assign]; own out store app/u.c:10:5; "
               "FIELD_STATUS per-field=writer-set-complete type-level=writer-set-complete; readers 1: u_run at app/u.c:30:5: uses the value without such a test (tests 0)",
               "      creator: key known: app/m.c:40:5 in m_init -> Ns_Make key string literal \"Q1\" actors {M_Main (script-entry)}",
               "      callee-check (success path): Ns_Make: missing(an implication between conditions is not decided): success value 0 [outcome class rule]; carried by 0",
               "      alternatives [context 1: U_Main@app/u.c:5:5 <- u_init@app/u.c:10:5]: found: retry: app/u.c:12:9; rests on: none; premises: none; decided calls: none"],
        "C2": ["  C2 lookup app/v.c:10:5 in v_init -> Ns_Find#0 namespace Ns_t.name partition {2} key known",
               "      creator: key known: app/m.c:50:5 in m_init -> Ns_Make key string literal \"Q2\" actors {M_Main (script-entry)}",
               "      callee-check (success path): Ns_Make: missing(an implication between conditions is not decided): success value 0 [outcome class rule]; carried by 0",
               "      alternatives [context 1: V_Main@app/v.c:5:5 <- v_init@app/v.c:10:5]: not established (open); rests on: none; premises: none; decided calls: none",
               "      alternatives [context 2: V_Cmd@app/v.c:7:5 <- v_init@app/v.c:10:5]: not established (open); rests on: none; premises: none; decided calls: none"],
    }
    W["sidecar"] = ["kz-plan-sidecar 2", "row %s C1 tier=- state=- control-state=planned" % K1, "row %s C2 tier=1 state=planned control-state=planned" % K2,
                    'context row=%s id=1 frame=50,51 chain="x"' % K2, 'context row=%s id=2 frame=52,53 chain="y"' % K2]
    W["pairs"] = [(K1, "A", "app/u.c:10:5", "u_init", "U_Main (script-entry)", "creator", "/abs/app/m.c:40:5", "m_init", "M_Main (script-entry)", "unordered", "-", "-", "creator"),
                  (K2, "A", "app/v.c:10:5", "v_init", "V_Main (script-entry)", "creator", "/abs/app/m.c:50:5", "m_init", "M_Main (script-entry)", "unordered", "-", "-", "creator")]
    W["uses"] = [("fx", "C1", K1, "A", "U_Main (script-entry)", "app/u.c", "10", "5", "Ns_Find", "10", "11", "-", "-", "control"),
                 ("fx", "C2", K2, "A", "V_Main (script-entry)", "app/v.c", "10", "5", "Ns_Find", "20", "21", "1", "50,51", "tier+control"),
                 ("fx", "C2", K2, "A", "V_Main (script-entry)", "app/v.c", "10", "5", "Ns_Find", "20", "21", "2", "52,53", "tier+control")]
    W["sheet_static_filled"] = False
    W["sheet_boot"] = None      # list of (row_key, consumer_root, context_id, reading, label)
    W["gate"] = "gate PASS\nlosses: none\n"
    W["expect"] = [("D1-R0", "fx", K1, "U_Main", "use_cond.status", "partial", "static"),
                   ("D1-R0", "fx", K1, "U_Main", "use[0].outcome", "absent", "boot")]
    W["recs"] = {"D1-R0": [rec_c1("D1"), rec_c2("D1")], "D2-R0": [rec_c1("D2"), rec_c2("D2")]}
    return W


def use_ev(at_site, after_site, t, outcome, context, key="Q1", out=None, status="-0x11"):
    u = {"at_call": {"actor": "2", "aseq": 3, "site": int(at_site), "t_ms": t}, "after_call": {"actor": "2", "aseq": 5, "site": int(after_site), "t_ms": t + 1},
         "status_value": status, "outcome": outcome, "context": context, "key": {"text": key, "truncated": False}, "kind_value": "0x2",
         "name_state": "undecided(the row's creator sites are not complete)", "link_result": "unlinked", "link_reasons": ["link_unlinked(x)"],
         "instance_link": {"status": "not_applicable"}}
    if out is not None:
        u["out"] = {"read": True, "value": out}
    return u


def base_rec(data, key, label, root, use_events, alts, holder_line=None):
    bi = {"status": "not_held", "why": "other definitions", "sources": [{"kind": "static-row", "ref": "%s:%d" % (REL, holder_line)}] if holder_line else [],
          "depends_on": []}
    biii = [{"status": "not_held" if v == "found" else "not_established", "why": "-", "context": c, "sources": [{"kind": "static-row", "ref": "%s:%d" % (REL, n)}],
             "depends_on": []} for c, (v, n) in sorted(alts.items())]
    return {"build_id": {"build": "fx"}, "row_id": {"use_site_key": key, "row_counter": label}, "path": "A",
            "consumer_actor": {"run_actor": "2", "root_tie": {"status": "determined", "root": root, "kind": "script-entry"}},
            "eval_branch": {"id": "%s-R0" % data, "data": data, "readings": ["R0"], "tag": None},
            "use_cond": {"status": "partial", "callee_check": {"status": "present", "depends_on": [], "sources": []},
                         "reach": {"status": "missing", "depends_on": [], "sources": []}, "writers": {"status": "missing", "depends_on": [], "sources": []},
                         "branch_i": bi, "branch_iii": biii},
            "order": {"use_events": use_events, "publish_events": []}, "link": {"status": "unlinked", "name_binding": {"pairs": []}},
            "tool_verdict": {"violation": {"value": "undetermined", "undetermined_reasons": ["link_unlinked(x)", "caller_dependence_unknown(y)"]},
                             "odf": {"value": "undetermined"}},
            "order_ground": ["order_basis_unknown(no linked pair)"], "same_build": {"main": "confirmed", "module:1": "confirmed"},
            "events": {"lifetime_ends": []}}


def rec_c1(data):
    return base_rec(data, K1, "C1", "U_Main", [use_ev(10, 11, 100.0, "absent", "1")], {"1": ("found", 8)}, holder_line=5)


def rec_c2(data):
    return base_rec(data, K2, "C2", "V_Main", [use_ev(20, 21, 200.0, "absent", "1", key="Q9")], {"1": ("not_established", 12), "2": ("not_established", 13)})


def write(W, d):
    os.makedirs(d)
    p = lambda n: os.path.join(d, n)
    rel = ["kz-relations 1", "facts " + "f" * 64, "[A] 2"]
    for k in ("C1", "C2"):
        rel.extend(W["rel"][k])
    with open(p(REL), "w") as f:
        f.write("\n".join(rel) + "\n")
    with open(p("plan.side"), "w") as f:
        f.write("\n".join(W["sidecar"]) + "\n")
    cols = "row_key\tpath\tuse_pos\tuse_function\tuse_root\tother_kind\tother_pos\tother_function\tother_root\tvalue\tedges\tpremises\trow_line"
    with open(p("pairs.tsv"), "w") as f:
        f.write("\n".join(["kz-order-pairs 1", "facts " + "f" * 64, "columns " + cols] + ["\t".join(x) for x in W["pairs"]] + ["rows 2", "end"]) + "\n")
    uc = "build\trow_label\trow_key\tpath\tconsumer_root\tuse_tu\tuse_line\tuse_col\tcallee\tat_site\tafter_site\tcontext_id\tframe_sites\tplan_state"
    with open(p("uses.tsv"), "w") as f:
        f.write("\n".join(["kz-uses 1", "rule synthetic", "columns " + uc] + ["\t".join(x) for x in W["uses"]] + ["lines %d rows 2" % len(W["uses"]), "end"]) + "\n")
    sc = "build\trow_label\trow_key\tconsumer_root\tuse_call\tcontext_id\tneeds_the_object\tpath_without_the_object\tcreating_task_and_kind\tforced_order"
    fill = "yes" if W["sheet_static_filled"] else "not filled: no reader"
    with open(p("sheets.tsv"), "w") as f:
        f.write("\n".join(["kz-sheets-static 1", "rule synthetic", "columns " + sc] + ["\t".join([u[0], u[1], u[2], u[4], "Ns_Find@x", u[11]] + [fill] * 4) for u in W["uses"]]
                          + ["lines %d" % len(W["uses"]), "end"]) + "\n")
    sb = "none"
    if W["sheet_boot"] is not None:
        sb = p("sheet_boot.tsv")
        with open(sb, "w") as f:
            f.write("\n".join(["kz-sheet-boot 1", "rule synthetic", "columns build\trow_key\tconsumer_root\tcontext_id\treading\tlabel"] +
                              ["\t".join(("fx",) + x) for x in W["sheet_boot"]] + ["lines %d" % len(W["sheet_boot"]), "end"]) + "\n")
    with open(p("expect.tsv"), "w") as f:
        f.write("\n".join(["kz-expect 1", "plan x"] + ["\t".join(("expect",) + x) for x in W["expect"]] + ["lines %d" % len(W["expect"]), "end"]) + "\n")
    g = "none"
    if W["gate"] is not None:
        g = p("gate.txt")
        with open(g, "w") as f:
            f.write(W["gate"])
    x = p("explain")
    for br, recs in W["recs"].items():
        os.makedirs(os.path.join(x, br, "records"))
        idx = ["build\tbranch\trow\tuse_site_key\tconsumer\tfile"]
        for i, r in enumerate(recs):
            fn = "fx.%s.%s.actor-%d.json" % (r["row_id"]["row_counter"], r["row_id"]["use_site_key"], i)
            with open(os.path.join(x, br, "records", fn), "w") as f:
                json.dump(r, f)
            idx.append("\t".join(["fx", br, r["row_id"]["row_counter"], r["row_id"]["use_site_key"], str(i), fn]))
        with open(os.path.join(x, br, "index.tsv"), "w") as f:
            f.write("\n".join(idx) + "\n")
    return ["--explain", x, "--build", "fx", "--boot", "b1", "--gate", g, "--pairs", p("pairs.tsv"), "--sidecar", p("plan.side"), "--relations", p(REL),
            "--uses", p("uses.tsv"), "--sheets-static", p("sheets.tsv"), "--sheets-boot", sb, "--expect", p("expect.tsv"), "--out", p("eval.txt")]


N = [0]


def run(W, tag, script=None):
    N[0] += 1
    d = os.path.join(WORK, "%03d_%s" % (N[0], tag))
    args = write(W, d)
    r = subprocess.run([PY, "-I", script or os.path.join(OBS, "explain_eval.py")] + args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
    out = os.path.join(d, "eval.txt")
    lines = []
    if os.path.exists(out):
        with open(out) as f:
            lines = f.read().splitlines()
    rows = []
    for ln in lines:
        p = ln.split("\t")
        if len(p) == 10 and not ln.startswith("columns "):
            rows.append(dict(zip(["check", "class", "branch", "build", "row_label", "row_key", "use", "result", "ground", "note"], p)))
    return r.returncode, rows, lines, r.stdout + r.stderr, args


def sel(rows, check, branch=None, key=None):
    return [x for x in rows if x["check"] == check and (branch is None or x["branch"] == branch) and (key is None or x["row_key"] == key)]


def case(tag, mod, checks):
    W = world()
    mod(W)
    rc, rows, lines, log, _ = run(W, tag)
    form = any(ln == "form ok" for ln in lines)
    ok("%s: rc 0 and form ok" % tag, rc == 0 and form, log[-300:])
    for name, f in checks:
        try:
            c = f(rows, lines)
        except Exception as e:
            c, log = False, "%s %s" % (e, log[-200:])
        ok("%s: %s" % (tag, name), c, [x for x in rows if x["result"] in ("fail", "not_evaluable", "met", "unmet", "not_counted")][:4])


MAIN = "branch D1-R0 main (declared before the boot: D1/R0): "
AUX = "branch D2-R0 auxiliary (data D2, readings R0): "


def tail_line(lines, prefix, what):
    return next((ln for ln in lines if ln.startswith(prefix + what)), "")


def boot_pass(line):
    m = re.search(r"boot: pass (\d+) / (\d+) \(fail (\d+), not evaluable (\d+)\)", line)
    return tuple(int(x) for x in m.groups()) if m else None


def only_fail(check, branch=None, key=None):
    """The check fails on the selected unit and no other check fails."""
    return lambda rows, lines: (any(x["result"] == "fail" for x in sel(rows, check, branch, key))
                                and all(x["check"] == check for x in rows if x["result"] == "fail"))


def main():
    if os.path.exists(WORK):
        shutil.rmtree(WORK)
    os.makedirs(WORK)
    # the base world: no check fails; classes by the rules
    case("base", lambda W: None, [
        ("no line fails", lambda rows, lines: not [x for x in rows if x["result"] == "fail"]),
        ("F1 lines carry the expectation's class", lambda rows, lines: {(x["class"], x["result"]) for x in sel(rows, "F1")} == {("static", "pass"), ("boot", "pass")}),
        ("F2a static (no success path line held: no generation can form)", lambda rows, lines: {x["class"] for x in sel(rows, "F2a")} == {"static"}),
        ("F2b format, F7 format, F5 static, F6 static pass (nothing to check)", lambda rows, lines: {x["class"] for x in sel(rows, "F2b") + sel(rows, "F7")} == {"format"}
         and {x["class"] for x in sel(rows, "F5")} == {"static"} and {x["result"] for x in sel(rows, "F6")} == {"pass"}),
        ("F3, F4, F8b, F9, F10 boot", lambda rows, lines: all({x["class"] for x in sel(rows, c)} == {"boot"} for c in ("F3", "F4", "F8b", "F9", "F10"))),
        ("F9 only on the row with two context lines", lambda rows, lines: {x["row_key"] for x in sel(rows, "F9")} == {K2}),
        ("F8b not evaluable without a linked pair", lambda rows, lines: {x["result"] for x in sel(rows, "F8b")} == {"not_evaluable"}),
        ("s1 on D1 control records only; s2-s4 on every branch", lambda rows, lines: {x["branch"] for x in sel(rows, "s1")} == {"D1-R0"}
         and {x["branch"] for x in sel(rows, "s2")} == {"D1-R0", "D2-R0"}),
        ("expect_discriminate: the control row met ((1) found line and (2) second definition), the tier+control row not counted",
         lambda rows, lines: {x["result"] for x in sel(rows, "expect_discriminate", key=K1)} == {"met"} and {x["result"] for x in sel(rows, "expect_discriminate", key=K2)} == {"not_counted"}),
        ("precision and recall undefined (no reader)", lambda rows, lines: {x["result"] for x in sel(rows, "precision") + sel(rows, "in_plan_recall")} == {"undefined(no reader)"}),
        ("undetermined split by kind", lambda rows, lines: any("structure 2" in x["note"] and "static input 2" in x["note"] for x in sel(rows, "undetermined_split", "D1-R0"))),
        ("tail per branch (protocol 14.13 item 3): the main D1-R0 block first, the auxiliary D2-R0 block named, F8 (a) on its own line, nothing pooled",
         lambda rows, lines: [ln for ln in lines if ln.startswith("branch ")][0].startswith(MAIN + "falsifiers F1-F10 by class")
         and MAIN + "expect_discriminate: met 1 / counted 1 (unmet 0; not_counted 1); by class: boot 0, static 2, format 0" in lines
         and AUX + "expect_discriminate: met 1 / counted 1 (unmet 0; not_counted 1); by class: boot 0, static 2, format 0" in lines
         and any(ln.startswith(AUX + "expect_safe s1-s4 by class") for ln in lines)
         and any(ln.startswith("branch * no branch (the pair file against the relations rows, F8 (a)): falsifiers") for ln in lines)
         and not any(ln.startswith(("falsifiers F1-F10", "expect_safe s1-s4", "expect_discriminate:")) for ln in lines)),
        ("count lines carry the branch", lambda rows, lines: all(len(ln.split("\t")) == 6 for ln in lines if ln.startswith("count\t"))),
        ("F4 not evaluable on records with no found use (protocol 14.13 item 4)", lambda rows, lines: {x["result"] for x in sel(rows, "F4")} == {"not_evaluable"}),
    ])

    def recs(W, br, i):
        return W["recs"][br][i]
    # F1
    case("F1", lambda W: W["expect"].__setitem__(0, W["expect"][0][:5] + ("derived", "static")), [("F1 fails on the changed line", only_fail("F1"))])
    case("F1-norecord", lambda W: W["expect"].append(("D1-R0", "fx", "0" * 16, "*", "use_cond.status", "partial", "boot")),
         [("F1 not evaluable without a record", lambda rows, lines: any(x["result"] == "not_evaluable" for x in sel(rows, "F1")))])
    # F2 (a) and its class
    def bound(W):
        recs(W, "D1-R0", 0)["link"]["name_binding"]["pairs"] = [{"use": 0, "result": "bound", "items": {}, "create_actor": "3", "create_aseq": 1}]
    case("F2a", bound, [("F2a fails (a bound pair in D1)", lambda rows, lines: any(x["result"] == "fail" for x in sel(rows, "F2a", "D1-R0", K1)))])

    def held_sp(W):
        W["rel"]["C1"][3] = W["rel"]["C1"][3].replace("missing(an implication between conditions is not decided)", "held")
    case("F2a-class", held_sp, [("a held success path line: F2a, s1, s3, s4 are boot", lambda rows, lines: {x["class"] for x in sel(rows, "F2a", key=K1) + sel(rows, "s1", key=K1)
                                                                                                            + sel(rows, "s3", key=K1) + sel(rows, "s4", key=K1)} == {"boot"}
                                 and {x["class"] for x in sel(rows, "F2a", key=K2)} == {"static"})])
    case("F2b", lambda W: recs(W, "D1-R0", 0)["use_cond"]["writers"].update(status="present", depends_on=["P0\tf\trole\tp0\t-\t1\tx\tk"]),
         [("F2b fails (present on a list entry in D1)", only_fail("F2b"))])
    # F3: a finished successful create of the same kind and text before an absent use
    def f3(W):
        r = recs(W, "D2-R0", 0)
        r["order"]["publish_events"] = [{"actor": "3", "call": "m_init -> Ns_Make line 40", "callee": "Ns_Make", "other_kind": "creator",
                                         "at_call": {"aseq": 1, "t_ms": 10.0}, "after_call": {"aseq": 2, "t_ms": 20.0}, "status_value": "0x0",
                                         "key": {"text": "Q1", "truncated": False}, "kind_value": "0x2", "out": {"read": False, "raw": "0x10001"}}]
    case("F3", f3, [("F3 fails (absence after a finished create)", only_fail("F3", "D2-R0", K1))])

    def f3_end(W):
        f3(W)
        recs(W, "D2-R0", 0)["events"]["lifetime_ends"] = [{"t_ms": 50.0, "names": "table", "form": "clear-table"}]
    case("F3-end", f3_end, [("a lifetime end between: F3 passes", lambda rows, lines: not [x for x in rows if x["result"] == "fail"])])

    def f3_slot(W):
        f3(W)
        recs(W, "D2-R0", 0)["events"]["lifetime_ends"] = [{"t_ms": 50.0, "names": "slot", "form": "clear-slot"}]
    case("F3-slot", f3_slot, [("a slot clear alone between: F3 passes and the note names the create as excused by a slot clear only (weak)",
                               lambda rows, lines: {x["result"] for x in sel(rows, "F3", "D2-R0", K1)} == {"pass"}
                               and all("slot clear only (weak" in x["note"] for x in sel(rows, "F3", "D2-R0", K1)))])
    case("F3-end-note", f3_end, [("a table clear between: the note names the create as excused by a lifetime end", lambda rows, lines: all(
        "excused by a lifetime end:" in x["note"] and "slot clear only" not in x["note"] for x in sel(rows, "F3", "D2-R0", K1)))])
    case("tail-branch", f3, [("a failure in D2-R0 only: the main line counts D1-R0 units and fails 0, the D2-R0 auxiliary line fails 1",
                              lambda rows, lines: boot_pass(tail_line(lines, MAIN, "falsifiers"))[2] == 0
                              and boot_pass(tail_line(lines, AUX, "falsifiers"))[2] == 1
                              and sum(boot_pass(tail_line(lines, MAIN, "falsifiers"))[1:])
                              == sum(1 for x in rows if x["branch"] == "D1-R0" and x["class"] == "boot" and re.fullmatch(r"F\d+[ab]?", x["check"])))])

    def f3_ne(W):
        f3(W)
        W["rel"]["C1"][3] = "      callee-check: Ns_Make returns 0 [OK]"
    case("F3-novalue", f3_ne, [("no success path line: F3 not evaluable", lambda rows, lines: any(x["result"] == "not_evaluable" for x in sel(rows, "F3", "D2-R0", K1)))])
    # protocol 14.13 item 4: a unit with no event is not evaluable, never a boot pass
    case("F3-pathB", lambda W: recs(W, "D2-R0", 0).__setitem__("path", "B"),
         [("a path B record: F3 not evaluable (8.2 applies to path A only)", lambda rows, lines: [(x["result"], x["class"]) for x in sel(rows, "F3", "D2-R0", K1)]
           == [("not_evaluable", "boot")] and "not a path A record" in sel(rows, "F3", "D2-R0", K1)[0]["note"]),
          ("the auxiliary line's boot pass count is the boot pass lines of its branch", lambda rows, lines: boot_pass(tail_line(lines, AUX, "falsifiers"))[0]
           == sum(1 for x in rows if x["branch"] == "D2-R0" and x["class"] == "boot" and x["result"] == "pass" and re.fullmatch(r"F\d+[ab]?", x["check"])))])

    def found_only(W):
        recs(W, "D2-R0", 0)["order"]["use_events"][0].update(outcome="found", out={"read": True, "value": "0x50001"}, status_value="0x0")
    case("F3-foundonly", found_only, [("a path A record with only a found use: F3 not evaluable, F4 counted", lambda rows, lines: {x["result"] for x in sel(rows, "F3", "D2-R0", K1)}
                                       == {"not_evaluable"} and {x["result"] for x in sel(rows, "F4", "D2-R0", K1)} == {"fail"})])
    case("noevents", lambda W: recs(W, "D2-R0", 1)["order"].__setitem__("use_events", []),
         [("a record with no use event: F3, F4 and F9 not evaluable", lambda rows, lines: all({x["result"] for x in sel(rows, c, "D2-R0", K2)} == {"not_evaluable"}
                                                                                           for c in ("F3", "F4", "F9")))])
    # F4
    def f4(W):
        u = recs(W, "D2-R0", 1)["order"]["use_events"][0]
        u.update(outcome="found", out={"read": True, "value": "0x20001"}, status_value="0x0")
    case("F4", f4, [("F4 fails (a found id with no recorded create)", only_fail("F4", "D2-R0", K2))])

    def f4p(W):
        f4(W)
        recs(W, "D2-R0", 1)["order"]["publish_events"] = [{"actor": "3", "call": "c", "callee": "Ns_Make", "other_kind": "creator", "at_call": {"t_ms": 5.0},
                                                          "after_call": {"t_ms": 6.0}, "status_value": "0x0", "out": {"read": False, "raw": "0x20001"}, "key": {}}]
    case("F4-pass", f4p, [("a recorded create with the id: F4 passes", lambda rows, lines: {x["result"] for x in sel(rows, "F4", "D2-R0", K2)} == {"pass"})])
    # F5 (and s2 with it), F6
    case("F5", lambda W: recs(W, "D2-R0", 0)["use_cond"]["branch_iii"][0].update(status="held"),
         [("F5 fails ((iii) held on a found line)", lambda rows, lines: any(x["result"] == "fail" for x in sel(rows, "F5", "D2-R0", K1))),
          ("s2 fails with it", lambda rows, lines: any(x["result"] == "fail" for x in sel(rows, "s2", "D2-R0", K1)))])
    case("F5-i", lambda W: recs(W, "D2-R0", 0)["use_cond"]["branch_i"].update(status="held"),
         [("F5 fails ((i) held against a second definition)", lambda rows, lines: any(x["result"] == "fail" and "(i) held" in x["note"] for x in sel(rows, "F5", "D2-R0", K1)))])
    case("F6", lambda W: W["rel"]["C1"].__setitem__(4, W["rel"]["C1"][4].replace("decided calls: none", "decided calls: app/u.c:11:5 result 0 by the stored-field rule")),
         [("F6 not evaluable with a decided context (the limit of spec 11 item 13; F6 has no failing input)",
           lambda rows, lines: {x["result"] for x in sel(rows, "F6", key=K1)} == {"not_evaluable"})])
    # F7
    case("F7", lambda W: recs(W, "D1-R0", 1)["use_cond"].update(status="derived"), [("F7 fails (derived in R0)", only_fail("F7", "D1-R0", K2))])
    case("records", lambda W: W["recs"]["D1-R0"].pop(1), [("a pre-defined row without a record: a format failure", lambda rows, lines: any(
        x["result"] == "fail" for x in sel(rows, "records", "D1-R0", K2)))])
    # F8
    case("F8a", lambda W: W["pairs"].pop(1), [("F8a fails (a creator location without a pair-file line)", lambda rows, lines: any(x["result"] == "fail" for x in sel(rows, "F8a", key=K2)))])

    def f8b(W):
        recs(W, "D2-R0", 0)["order_ground"] = [{"value": "hard_before", "src": ["pairs.tsv", 4]}]
    case("F8b", f8b, [("F8b fails (the pair-file line has another value)", only_fail("F8b", "D2-R0", K1))])

    def f8bp(W):
        recs(W, "D2-R0", 0)["order_ground"] = [{"value": "unordered", "src": ["pairs.tsv", 4]}]
    case("F8b-pass", f8bp, [("the same value: F8b passes", lambda rows, lines: {x["result"] for x in sel(rows, "F8b", "D2-R0", K1)} == {"pass"})])
    # F9, F10
    case("F9", lambda W: recs(W, "D2-R0", 1)["order"]["use_events"][0].__setitem__("context", None), [("F9 fails (context unknown)", only_fail("F9", "D2-R0", K2))])
    case("F10", lambda W: recs(W, "D2-R0", 1)["same_build"].__setitem__("module:1", "unconfirmed(no build-id in dump)"), [("F10 fails", only_fail("F10", "D2-R0", K2))])
    case("F10-gate", lambda W: W.__setitem__("gate", "gate FAIL control_ok=0\n"), [("F10 not evaluable on a boot that did not pass the gate", lambda rows, lines: {
        x["result"] for x in sel(rows, "F10")} == {"not_evaluable"})])
    # s1-s4
    case("s1", bound, [("s1 fails (a bound pair on a D1 control row)", lambda rows, lines: any(x["result"] == "fail" for x in sel(rows, "s1", "D1-R0", K1)))])
    case("s2", lambda W: recs(W, "D1-R0", 1)["use_cond"]["branch_iii"][1].update(status="held"), [("s2 fails ((iii) held on a control row)", only_fail("s2", "D1-R0", K2))])

    def s3(W):
        u = recs(W, "D2-R0", 1)["order"]["use_events"][0]
        u.update(outcome="found", out={"read": True, "value": "0x30001"}, instance_link={"status": "linked", "id": "0x30001"})
        recs(W, "D2-R0", 1)["order"]["publish_events"] = [{"actor": "3", "call": "c", "callee": "Ns_Make", "other_kind": "creator", "at_call": {"t_ms": 5.0},
                                                          "after_call": {"t_ms": 6.0}, "status_value": "0x0", "out": {"read": True, "value": "0x30001"}, "key": {}}]
        recs(W, "D2-R0", 1)["events"]["lifetime_ends"] = [{"t_ms": 100.0, "names": "id", "id": 0x30001, "form": "delete"}]
    case("s3", s3, [("s3 fails (an instance link across a lifetime end of its id)", only_fail("s3", "D2-R0", K2))])

    def s4(W):
        recs(W, "D2-R0", 0)["link"]["name_binding"]["pairs"] = [{"use": 0, "result": "refuted", "create_actor": "3", "create_aseq": 1,
                                                                 "items": {"basis4_creator_success": {"status": "missing"}}}]
    case("s4", s4, [("s4 fails (refuted without its conditions)", only_fail("s4", "D2-R0", K1))])
    # expect_discriminate
    case("ED-unmet", lambda W: recs(W, "D2-R0", 0)["use_cond"]["branch_iii"][0].update(status="not_established"),
         [("unmet when the found line's (iii) is not given", lambda rows, lines: {x["result"] for x in sel(rows, "expect_discriminate", "D2-R0", K1)} == {"unmet"})])

    def ed_none(W):
        W["rel"]["C1"][1] = "      holder: field U_t.id; writers 1: own out store app/u.c:10:5; FIELD_STATUS per-field=writer-set-complete type-level=writer-set-complete; readers 0"
        W["rel"]["C1"][4] = W["rel"]["C1"][4].replace("found: retry: app/u.c:12:9", "not established (open)")
        for br in ("D1-R0", "D2-R0"):
            recs(W, br, 0)["use_cond"]["branch_iii"][0]["status"] = "not_established"
    case("ED-nothing", ed_none, [("a row with nothing to require: unmet", lambda rows, lines: {x["result"] for x in sel(rows, "expect_discriminate", key=K1)} == {"unmet"}
                                  and all("nothing to require" in x["note"] for x in sel(rows, "expect_discriminate", key=K1)))])

    def ed_found(W):
        u = recs(W, "D2-R0", 0)["order"]["use_events"][0]
        u.update(outcome="found", out={"read": True, "value": "0x40001"}, instance_link={"status": "unlinked", "reason": "link_unlinked(no create of the id in the record)"})
    case("ED-found", ed_found, [("a found use on a row with requirements (1) and (2): still met (rule (3) only on rows without them, protocol 14.13 item 1)",
                                 lambda rows, lines: {x["result"] for x in sel(rows, "expect_discriminate", "D2-R0", K1)} == {"met"}),
                                ("its class static (no context line, one alternatives context)", lambda rows, lines: {x["class"] for x in sel(rows, "expect_discriminate", "D2-R0", K1)} == {"static"})])
    case("ED-found-heldsp", lambda W: (ed_found(W), held_sp(W)), [("(1) and (2) with a held success path line: still static (the requirements are static lines)", lambda rows, lines: {
        (x["result"], x["class"]) for x in sel(rows, "expect_discriminate", "D2-R0", K1)} == {("met", "static")})])

    def ed3_found(W):
        ed_none(W)
        ed_found(W)
    case("ED-3-found", ed3_found, [("rule (3) on a row with no (1) or (2): a found use with no generation is unmet, static", lambda rows, lines: {
        (x["result"], x["class"]) for x in sel(rows, "expect_discriminate", "D2-R0", K1)} == {("unmet", "static")}
        and all("rule (3): required: (3) use 0" in x["note"] for x in sel(rows, "expect_discriminate", "D2-R0", K1)))])

    def ed3_boot(W):
        ed_none(W)
        held_sp(W)
        recs(W, "D2-R0", 0)["order"]["use_events"][0].update(outcome="found", out={"read": True, "value": "0x40001"}, instance_link={"status": "linked", "id": "0x40001"})
    case("ED-3-boot", ed3_boot, [("rule (3) with a held success path line and a linked found use: met, boot", lambda rows, lines: {
        (x["result"], x["class"]) for x in sel(rows, "expect_discriminate", "D2-R0", K1)} == {("met", "boot")})])
    case("ED-3-boot-unmet", lambda W: (ed3_found(W), held_sp(W)), [("rule (3) with a held success path line and an unlinked found use: unmet, boot", lambda rows, lines: {
        (x["result"], x["class"]) for x in sel(rows, "expect_discriminate", "D2-R0", K1)} == {("unmet", "boot")})])

    def ed_ctx(W):   # a control row with two context lines: the record's use is in the other context
        W["sidecar"] += ['context row=%s id=1 frame=60,61 chain="p"' % K1, 'context row=%s id=2 frame=62,63 chain="q"' % K1]
        r = recs(W, "D2-R0", 0)
        r["order"]["use_events"][0]["context"] = "2"
        r["use_cond"]["branch_iii"] = [{"status": "not_established", "why": "no alternatives line of context 2", "context": "2", "sources": [], "depends_on": []}]
    case("ED-ctx", ed_ctx, [("(1) on a row whose (iii) follows the record's contexts: unmet when the found line's context has no use, boot", lambda rows, lines: {
        (x["result"], x["class"]) for x in sel(rows, "expect_discriminate", "D2-R0", K1)} == {("unmet", "boot")}),
                            ("the same row in a branch whose record carries the found context: met, boot", lambda rows, lines: {
        (x["result"], x["class"]) for x in sel(rows, "expect_discriminate", "D1-R0", K1)} == {("met", "boot")})])
    # precision and in-plan recall
    def observed(W):
        for br in ("D1-R0", "D2-R0"):
            recs(W, br, 0)["tool_verdict"]["violation"]["value"] = "observed"
    case("prec-noread", observed, [("a tool positive without a reader: undefined(no reader)", lambda rows, lines: {x["result"] for x in sel(rows, "precision")} == {"undefined(no reader)"})])
    case("prec-N0", lambda W: W.__setitem__("sheet_boot", [(K2, "V_Main (script-entry)", "1", "U", "negative")]),
         [("a reader and no tool positive: undefined(no tool positive)", lambda rows, lines: {x["result"] for x in sel(rows, "precision")} == {"undefined(no tool positive)"}),
          ("recall with k = 0: undefined(k = 0)", lambda rows, lines: {x["result"] for x in sel(rows, "in_plan_recall")} == {"undefined(k = 0)"})])
    case("prec-U", lambda W: (observed(W), W.__setitem__("sheet_boot", [(K1, "U_Main (script-entry)", "-", "U", "positive")])),
         [("only U: undefined(T + F = 0)", lambda rows, lines: {x["result"] for x in sel(rows, "precision")} == {"undefined(T + F = 0)"}),
          ("recall 1/1", lambda rows, lines: {x["result"] for x in sel(rows, "in_plan_recall")} == {"1/1"})])
    case("prec-TU", lambda W: (observed(W), W["uses"].append(("fx", "C1", K1, "A", "U_Main (script-entry)", "app/u.c", "10", "5", "Ns_Find", "10", "11", "1", "-", "control")),
                               W.__setitem__("sheet_boot", [(K1, "U_Main (script-entry)", "-", "T", "positive"), (K1, "U_Main (script-entry)", "1", "U", "positive")])),
         [("T and U: a value, not final", lambda rows, lines: all(x["result"].startswith("N 2 T 1 F 0 U 1") and "not final" in x["result"] for x in sel(rows, "precision")))])
    case("prec-TF", lambda W: (observed(W), W["uses"].append(("fx", "C1", K1, "A", "U_Main (script-entry)", "app/u.c", "10", "5", "Ns_Find", "10", "11", "1", "-", "control")),
                               W.__setitem__("sheet_boot", [(K1, "U_Main (script-entry)", "-", "T", "positive"), (K1, "U_Main (script-entry)", "1", "F", "negative")])),
         [("T and F: conditional 1/2", lambda rows, lines: all("conditional 1/2" in x["result"] and "not final" not in x["result"] for x in sel(rows, "precision")))])
    case("recall-0k", lambda W: W.__setitem__("sheet_boot", [(K1, "U_Main (script-entry)", "-", "U", "positive"), ("0" * 16, "X (script-entry)", "-", "U", "positive")]),
         [("a positive the tool did not find: 0/1", lambda rows, lines: {x["result"] for x in sel(rows, "in_plan_recall")} == {"0/1"}),
          ("beside: the undetermined positive by kind, one out of plan", lambda rows, lines: all("structure 1" in x["note"] and "out of plan positives 1" in x["note"]
                                                                                                for x in sel(rows, "in_plan_recall")))])
    # the open guard: the expectation file only inside F1, writes only to the output
    W = world()
    d = os.path.join(WORK, "guard")
    args = write(W, d)
    exp = os.path.abspath(dict(zip(args[::2], args[1::2]))["--expect"])
    outp = os.path.abspath(dict(zip(args[::2], args[1::2]))["--out"])

    def guarded(script, outname):
        a = list(args)
        a[a.index("--out") + 1] = os.path.join(d, outname)
        code = ("import builtins,sys,os,inspect\nreal=builtins.open\nEXP=%r\nOUT=%r\n"
                "def guard(f,*a,**k):\n    p=os.path.abspath(f) if isinstance(f,str) else f\n    m=(a[0] if a else k.get('mode','r'))\n"
                "    if p==EXP and not any(fr.function=='check_f1' for fr in inspect.stack()):\n        raise SystemExit('opened the expectation file outside F1')\n"
                "    if isinstance(p,str) and any(c in m for c in 'wax') and p!=OUT:\n        raise SystemExit('wrote '+p)\n    return real(f,*a,**k)\n"
                "builtins.open=guard\nimport importlib.util\nspec=importlib.util.spec_from_file_location('e',%r)\nm=importlib.util.module_from_spec(spec)\n"
                "spec.loader.exec_module(m)\nsys.exit(m.main(%r))\n") % (exp, os.path.join(d, outname), script, a)
        return subprocess.run([PY, "-I", "-c", code], stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
    g1 = guarded(os.path.join(OBS, "explain_eval.py"), "eval_guard.txt")
    ok("the script opens the expectation file only in F1 and writes only its output", g1.returncode == 0 and "opened" not in g1.stderr + g1.stdout
       and "wrote" not in g1.stderr + g1.stdout, (g1.stdout + g1.stderr)[-300:])
    with open(os.path.join(OBS, "explain_eval.py")) as f:
        src = f.read()
    bad1 = os.path.join(WORK, "eval_opens_expect.py")
    with open(bad1, "w") as f:
        f.write(src.replace("    rel_name = os.path.basename(o.relations)\n", "    rel_name = os.path.basename(o.relations)\n    read('expect_early', o.expect)\n", 1))
    g2 = guarded(bad1, "eval_guard2.txt")
    ok("negative control: a copy that opens the expectation file outside F1 is caught", g2.returncode != 0 and "outside F1" in g2.stderr + g2.stdout, (g2.stdout + g2.stderr)[-300:])
    bad2 = os.path.join(WORK, "eval_writes_more.py")
    with open(bad2, "w") as f:
        f.write(src.replace("    with open(o.out, \"w\", encoding=\"utf-8\") as f:\n", "    open(o.out + '.extra', 'w').close()\n    with open(o.out, \"w\", encoding=\"utf-8\") as f:\n", 1))
    g3 = guarded(bad2, "eval_guard3.txt")
    ok("negative control: a copy that writes beside its output is caught", g3.returncode != 0 and "wrote" in g3.stderr + g3.stdout, (g3.stdout + g3.stderr)[-300:])
    # the form check: a broken output is refused by the script's own check (tail counts)
    rc, rows, lines, log, a = run(world(), "form")
    ok("form: the written file says form ok and its tail counts equal its lines", "form ok" in lines and sum(int(ln.split("\t")[5]) for ln in lines if ln.startswith("count\t")) == len(rows))
    n_pass = sum(1 for _, c in RESULTS if c)
    print("eval fixtures: %d checks, %d passed, %d failed" % (len(RESULTS), n_pass, len(RESULTS) - n_pass))
    return 0 if n_pass == len(RESULTS) else 1


if __name__ == "__main__":
    sys.exit(main())
