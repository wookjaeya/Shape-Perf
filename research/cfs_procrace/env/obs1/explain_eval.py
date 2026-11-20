#!/usr/bin/env python3
"""explain_eval.py -- the falsifier and evaluation script of the C1 cycle (plan/c1_closure_explain_rev4_spec_20261118.md
11 item 13 and 11.4; evaluation protocol plan/c1_closure_evaluation_protocol_20261118.md 6, 7, 14.3, 14.6, 14.7;
results 26.0 (e), (f)).

    python3 -I explain_eval.py --explain EXPLAIN_OUT_DIR --build NAME --boot LABEL --gate GATE_TXT|none --pairs PAIRS.tsv \
        --sidecar PLAN.side --relations RELATIONS.txt --uses USES.tsv --sheets-static SHEETS.tsv --sheets-boot FILE|none \
        --expect EXPECT.tsv --out NEW_FILE

One boot of one build: it reads the explanation output of that boot (every branch directory's index.tsv and the records
it names), the boot's gate result, the pair file, the plan sidecar, the relations text, the list of pre-defined uses,
the static and per-boot parts of the adjudication sheets, and, for F1 only, the expectation file.  It opens only the
files named on the command line (and the records the index files name), hashes each into the head of the output, writes
only NEW_FILE, changes no input and no record, and its output is the input of no part of Kairos.  No row id, key, name
or expected verdict is an input: rows are chosen by the list of pre-defined uses and its plan_state column, contexts by
the sidecar, requirements by the static lines of the relations text.  Standard library only; imports no other part.

Output kz-eval 1: one line per unit of each check (check, class, branch, build, row_label, row_key, use, result, ground,
note); class is boot (a boot can make it fail), static (the static inputs fix it), format (only a script error can
make it fail) by the rules of spec 11.4 and 11.5 (evaluation protocol 14.6, 14.13); result pass / fail / not_evaluable
for a check (a unit with no event to check is not_evaluable, never pass: protocol 14.13 item 4), met / unmet /
not_counted for expect_discriminate, a value or undefined(...) for a number.  The tail (protocol 14.13 item 3) counts
check x class x result x branch, then per branch -- D1-R0 first, labelled main (declared before the boot: D1/R0), every
other branch auxiliary with its data branch and readings, the branchless units on their own line -- the falsifiers
and expect_safe by class (only the boot class is a falsifier or safety result; static, format and not evaluable apart)
and expect_discriminate met / counted; nothing is pooled across branches; then precision, in-plan recall and the
undetermined split per branch.  The form is checked at the end (every unit has its line, the class and result values
are in their sets, the tail counts equal the lines); rc 0 written and checked, 3 the form check failed, 2 an input not
of its form, 64 usage.  Revision 2 (spec 11.5 item 3): expect_discriminate rule (3) only on a control row with no
requirement (1) or (2), and its class from the static inputs; the tail per branch; units with no event not_evaluable;
F3 notes the creates a lifetime end excused, and those a slot clear alone excused.
"""
import argparse
import hashlib
import json
import os
import re
import sys

EVAL_REV = "explain_eval.py revision 2 (2026-11-19)"
MAIN = "D1-R0"   # the main result, declared before the boot (evaluation protocol 14.1; a constant, not a row or a key)
INPUTS = []
CLASSES = ("boot", "static", "format")
RESULTS = ("pass", "fail", "not_evaluable", "met", "unmet", "not_counted")
DECIDED = ("observed", "not_observed_in_runs", "refuted_link")
KIND = {"no_run": "run evidence", "target_not_reached": "run evidence", "record_insufficient": "run evidence",
        "absent_result_unevaluated": "static input", "key_unresolved": "static input", "writer_set_incomplete": "static input",
        "fixed_proof_open": "static input", "caller_dependence_unknown": "static input", "actor_not_extracted": "static input",
        "plan_missing": "resource", "budget_truncated": "resource", "link_unlinked": "structure", "multi_state_unlinked": "structure",
        "build_mismatch": "structure", "intervention_out_of_scope": "structure"}
KINDS = ["run evidence", "static input", "resource", "structure", "contradiction", "other"]
NOT_FILLED = "not filled: no reader"
FIVE = ("ga_namespace_entry", "na_semantics", "na_bound", "da_no_name_mapping", "ra_same_kind", "ma_no_lifetime_end")


def reason_kind(r):
    if "absence after a live create of the same name" in r:
        return "contradiction"
    return KIND.get(r.split("(", 1)[0], "other")


def read(name, path):
    with open(path, "rb") as f:
        data = f.read()
    INPUTS.append((name, hashlib.sha256(data).hexdigest(), path))
    return data.decode("utf-8", errors="replace")


# ------------------------------------------------------------------------------------------------ the static inputs
def parse_relations(text, rel_name, wanted):
    rows, cur = {}, None
    pos = re.compile(r"(\S+:\d+:\d+)")
    for n, ln in enumerate(text.splitlines(), 1):
        m = re.match(r"^  ([ABC]\d+) ", ln)
        if m:
            cur = None
            if m.group(1) in wanted:
                cur = {"id": m.group(1), "line": n, "success": [], "alts": {}, "holders": [], "creators": [], "writers": []}
                rows[m.group(1)] = cur
            continue
        if not ln.startswith("      "):
            cur = None
            continue
        if cur is None:
            continue
        t = ln.strip()
        if t.startswith("callee-check (success path): "):
            mm = re.match(r"callee-check \(success path\): (\S+): (held|missing\(.*?\)): success value (-?\d+) ", t)
            if mm:
                cur["success"].append({"callee": mm.group(1), "held": mm.group(2) == "held", "value": int(mm.group(3)), "n": n})
        elif t.startswith("alternatives [context "):
            mm = re.match(r"alternatives \[context (\d+|-)(?::[^\]]*)?\]: (.*)$", t)
            if mm:
                body = mm.group(2)
                v = "found" if body.startswith("found: ") else "none_found" if body.startswith("none found") else "not_established"
                dm = re.search(r"; decided calls: (.*)$", body)
                cur["alts"][mm.group(1)] = {"verdict": v, "n": n, "decided": dm.group(1) if dm else "none"}
        elif t.startswith("holder: "):
            cur["holders"].append((n, t))
        elif t.startswith("creator:"):
            mm = pos.search(t)
            cur["creators"].append((n, mm.group(1) if mm else None))
        elif re.match(r"(out-pointer|assign|memset|call|store|copy) at ", t):
            mm = pos.search(t)
            cur["writers"].append((n, mm.group(1) if mm else None))
    for r in rows.values():
        r["ref"] = lambda k, rn=rel_name: "%s:%d" % (rn, k)
    return rows


def other_definitions(holder_text, reads_rb):
    """The writers of a holder line other than the own out store (a clearing writer is no other definition under the
    reading Rb: judgement rules 8.13 item 5)."""
    m = re.search(r"; writers \d+: (.*?); FIELD_STATUS ", holder_text)
    ws = m.group(1).split("; ") if m else []
    return [w for w in ws if not w.startswith("own out store ") and not (reads_rb and w.startswith("clearing "))]


def parse_sidecar(text):
    rows, ctx = {}, {}
    for ln in text.splitlines():
        if ln.startswith("row "):
            p = ln.split()
            rows[p[1]] = p[2]
        m = re.match(r"context row=(\S+) id=(\d+) ", ln)
        if m:
            ctx.setdefault(m.group(1), []).append(m.group(2))
    return rows, ctx


def parse_pairs(text):
    lines = text.splitlines()
    if not lines or lines[0] != "kz-order-pairs 1":
        return None
    cols = None
    out = {}
    for n, ln in enumerate(lines, 1):
        if ln.startswith("columns "):
            cols = ln[len("columns "):].split("\t")
            continue
        f = ln.split("\t")
        if cols and len(f) == len(cols):
            d = dict(zip(cols, f))
            d["n"] = n
            out.setdefault(d.get("row_key"), []).append(d)
    return out


USE_COLS = ["build", "row_label", "row_key", "path", "consumer_root", "use_tu", "use_line", "use_col", "callee", "at_site",
            "after_site", "context_id", "frame_sites", "plan_state"]


def parse_uses(text):
    lines = text.splitlines()
    if not lines or lines[0] != "kz-uses 1" or lines[-1] != "end":
        return None
    body = [ln.split("\t") for ln in lines[1:-1] if not ln.startswith(("rule ", "columns ", "lines "))]
    if any(len(x) != 14 for x in body):
        return None
    return [dict(zip(USE_COLS, x)) for x in body]


def parse_sheet(text, kind):
    """kz-sheets-static 1 (committed) or kz-sheet-boot 1 (spec 11.4: build, row_key, consumer_root, context_id, reading
    T|F|U, label positive|negative|unreadable)."""
    lines = text.splitlines()
    want = "kz-sheets-static 1" if kind == "static" else "kz-sheet-boot 1"
    if not lines or lines[0] != want or lines[-1] != "end":
        return None
    cols = next((ln[len("columns "):].split("\t") for ln in lines if ln.startswith("columns ")), None)
    if cols is None:
        return None
    rows = [dict(zip(cols, ln.split("\t"))) for ln in lines[1:-1] if not ln.startswith(("rule ", "columns ", "lines "))]
    if kind == "boot" and any(r.get("reading") not in ("T", "F", "U") or r.get("label") not in ("positive", "negative", "unreadable")
                              for r in rows):
        return None
    return rows


# ------------------------------------------------------------------------------------------------ the records
def root_of(r):
    ca = r.get("consumer_actor") or {}
    rt = ca.get("root_tie") or {}
    if rt.get("root"):
        return "%s (%s)" % (rt["root"], rt.get("kind"))
    if ca.get("static_root"):
        return "%s (%s)" % (ca["static_root"], ca.get("static_kind"))
    return "none"


def short_root(r):
    return (r["consumer_actor"].get("root_tie") or {}).get("root") or r["consumer_actor"].get("static_root")


def load_records(d, build):
    per = {}
    for br in sorted(os.listdir(d)):
        idx = os.path.join(d, br, "index.tsv")
        if not os.path.isfile(idx):
            continue
        txt = read("index:%s" % br, idx).splitlines()[1:]
        per[br] = []
        for ln in txt:
            fn = ln.split("\t")[-1]
            r = json.loads(read("record:%s/%s" % (br, fn), os.path.join(d, br, "records", fn)))
            if r["build_id"]["build"] == build:
                per[br].append((fn, r))
    return per


def field(rec, f):
    m = re.match(r"pair\[(.+)\]\.(missing_items|result)$", f)
    if m:
        ps = [p for p in rec["link"].get("name_binding", {}).get("pairs", []) if p.get("create_pos") == m.group(1)]
        if not ps:
            return None
        if m.group(2) == "missing_items":
            return ",".join(sorted(set(x for p in ps for x in p["missing_items"])))
        return ",".join(sorted({p["result"] for p in ps}))
    m = re.match(r"use\[(\d+)\]\.(\w+)$", f)
    if m:
        ue = rec["order"]["use_events"]
        i = int(m.group(1))
        return ue[i].get(m.group(2)) if i < len(ue) else None
    d = rec
    for k in f.split("."):
        d = d.get(k) if isinstance(d, dict) else None
    return d


def norm(f, v):
    if v is None:
        return None
    if f.endswith(".missing_items"):
        return ",".join(sorted(x for x in str(v).split(",") if x))
    return str(v)


def hx(v):
    return None if v is None else ("-0x%x" % -v if v < 0 else "0x%x" % v)


def t_of(e, k):
    return (e.get(k) or {}).get("t_ms")


def complete(k):
    return bool(k) and k.get("text") is not None and not k.get("truncated")


def ends_between(rec, t0, t1, name, ids):
    out = []
    for x in rec.get("events", {}).get("lifetime_ends", []):
        t = x.get("t_ms")
        if t is None or not (t0 <= t <= t1):
            continue
        if x.get("names") in ("table", "kind") or (x.get("names") == "name" and x.get("name") == name) or \
                (x.get("names") == "id" and hx(x.get("id")) in ids) or x.get("names") == "slot":
            out.append(x)
    return out


# ------------------------------------------------------------------------------------------------ main
def main(argv):
    ap = argparse.ArgumentParser()
    for a in ("explain", "build", "boot", "gate", "pairs", "sidecar", "relations", "uses", "sheets-static", "sheets-boot", "expect", "out"):
        ap.add_argument("--" + a)
    o = ap.parse_args(argv)
    if any(getattr(o, a.replace("-", "_")) is None for a in ("explain", "build", "boot", "gate", "pairs", "sidecar", "relations", "uses",
                                                              "sheets-static", "sheets-boot", "expect", "out")):
        print("explain_eval.py: every argument is required (none for --gate and --sheets-boot when absent)", file=sys.stderr)
        return 64
    if os.path.exists(o.out):
        print("explain_eval.py: %s exists" % o.out, file=sys.stderr)
        return 64
    try:
        with open(os.path.abspath(__file__), "rb") as f:
            self_sha = hashlib.sha256(f.read()).hexdigest()
    except OSError:
        self_sha = None
    rel_name = os.path.basename(o.relations)
    uses = parse_uses(read("uses", o.uses))
    if uses is None:
        print("explain_eval.py: the uses list is not of its form", file=sys.stderr)
        return 2
    uses = [u for u in uses if u["build"] == o.build]
    labels = {u["row_label"] for u in uses}
    side_rows, side_ctx = parse_sidecar(read("sidecar", o.sidecar))
    rel = parse_relations(read("relations", o.relations), rel_name, labels)
    pairs = parse_pairs(read("pairs", o.pairs))
    if pairs is None:
        print("explain_eval.py: the pair file is not of its form", file=sys.stderr)
        return 2
    gate = "none"
    if o.gate != "none":
        g = read("gate", o.gate)
        m = re.match(r"gate (PASS|FAIL)", g)
        gate = m.group(1) if m else "unreadable"
    sst = parse_sheet(read("sheets_static", o.sheets_static), "static")
    if sst is None:
        print("explain_eval.py: the static sheets are not of their form", file=sys.stderr)
        return 2
    sbt = None
    if o.sheets_boot != "none":
        sbt = parse_sheet(read("sheets_boot", o.sheets_boot), "boot")
        if sbt is None:
            print("explain_eval.py: the per-boot sheet is not of its form (spec 11.4)", file=sys.stderr)
            return 2
    per = load_records(o.explain, o.build)
    rows_of_key = {}
    for u in uses:
        rows_of_key.setdefault(u["row_key"], {"label": u["row_label"], "states": set()})["states"].add(u["plan_state"])
    control = {k for k, v in rows_of_key.items() if any("control" in s for s in v["states"])}
    only_control = {k for k in control if all(s == "control" for s in rows_of_key[k]["states"])}

    def srow(key):
        return rel.get(rows_of_key.get(key, {}).get("label")) or {"success": [], "alts": {}, "holders": [], "creators": [], "writers": [],
                                                                 "ref": lambda k: "%s:%d" % (rel_name, k)}

    def gen_possible(key):    # a generation (and so bound, refuted, an instance link) needs a held success path line
        return any(s["held"] for s in srow(key)["success"])

    L = []
    units = set()

    def emit(check, cls, branch, key, use, result, ground="-", note="-", unit=None):
        L.append({"check": check, "class": cls, "branch": branch, "row_label": rows_of_key.get(key, {}).get("label", "-") if key != "*" else "*",
                  "row_key": key, "use": str(use), "result": result, "ground": ground or "-", "note": (note or "-").replace("\t", " ")[:600]})
        if unit is not None:
            units.add(unit)

    expected = set()
    # ---- F1 (the expectation file is opened here only)
    def check_f1():
        ex = read("expect", o.expect).splitlines()
        if not ex or ex[0] != "kz-expect 1" or ex[-1] != "end":
            return None
        lines = [x.split("\t") for x in ex if x.startswith("expect\t")]
        if any(len(x) != 8 for x in lines):
            return None
        return lines
    exl = check_f1()
    if exl is None:
        print("explain_eval.py: the expectation file is not kz-expect 1 with eight columns (spec 11 item 12)", file=sys.stderr)
        return 2
    for i, (_, br, build, key, root, fld, val, cls) in enumerate(exl):
        if build != o.build:
            continue
        expected.add(("F1", i))
        recs = [(fn, r) for fn, r in per.get(br, []) if r["row_id"]["use_site_key"] == key and root in ("*", short_root(r))]
        if not recs:
            emit("F1", cls, br, key, "*", "not_evaluable", "expectation line %d" % (i + 1), "no record", unit=("F1", i))
            continue
        fv = sorted({norm(fld, field(r, fld)) or "-" for _, r in recs})
        emit("F1", cls, br, key, "*", "pass" if fv == [norm(fld, val)] else "fail",
             "expectation line %d; %s" % (i + 1, ",".join(fn for fn, _ in recs))[:300], "%s: expected %s, found %s" % (fld, val, "|".join(fv)), unit=("F1", i))
    # ---- per branch and record
    for br in sorted(per):
        recs = per[br]
        if not recs:
            continue
        eb = recs[0][1].get("eval_branch", {})
        data, readings = eb.get("data"), set(eb.get("readings") or [])
        keys_here = {r["row_id"]["use_site_key"] for _, r in recs}
        for key in sorted(rows_of_key):
            if key not in keys_here:     # a pre-defined row with no record in this branch: a format failure of the judgement part
                emit("records", "format", br, key, "*", "fail", "-", "no record of a pre-defined row in this branch")
        for fn, r in recs:
            key = r["row_id"]["use_site_key"]
            S = srow(key)
            uc = r["use_cond"]
            ue = r["order"]["use_events"]
            pe = r["order"]["publish_events"]
            pairs_r = r["link"].get("name_binding", {}).get("pairs", [])
            u_ = (fn,)
            gp = gen_possible(key)
            # F2 (D1 branches)
            if data == "D1":
                expected.add(("F2a", br, fn))
                nb = [p for p in pairs_r if p.get("result") == "bound"]
                emit("F2a", "boot" if gp else "static", br, key, "*", "fail" if nb else "pass", fn,
                     "%d bound pairs in D1%s" % (len(nb), "" if gp else "; no success path line of the row is held (no generation can form)"), unit=("F2a", br, fn))
                bad = []
                things = [("use_cond.%s" % k, uc.get(k)) for k in ("callee_check", "reach", "writers", "branch_i")] + \
                         [("use_cond.branch_iii[%d]" % i, x) for i, x in enumerate(uc.get("branch_iii", []))] + \
                         [("pair %d %s" % (i, k), v) for i, p in enumerate(pairs_r) for k, v in p.get("items", {}).items()]
                for nm, e in things:
                    if isinstance(e, dict) and e.get("status") in ("held", "present") and (
                            e.get("depends_on") or (e.get("premises") and e.get("reading") not in readings)):
                        bad.append(nm)
                expected.add(("F2b", br, fn))
                emit("F2b", "format", br, key, "*", "fail" if bad else "pass", fn, "; ".join(bad) or "nothing held or present on a list entry or a premise",
                     unit=("F2b", br, fn))
            # F3 (path A absence uses)
            expected.add(("F3", br, fn))
            n3 = 0
            if r.get("path") == "A":
                succ = {s["callee"]: s for s in S["success"]}
                for i, u in enumerate(ue):
                    if u.get("outcome") != "absent":
                        continue
                    n3 += 1
                    ut = t_of(u, "at_call")
                    hit, ne, exc, weak = [], [], [], []
                    for c in pe:
                        if c.get("other_kind") == "writer":
                            continue
                        if not complete(u.get("key")) or not complete(c.get("key")) or u["key"]["text"] != c["key"]["text"]:
                            continue
                        if u.get("kind_value") is None or u.get("kind_value") != c.get("kind_value"):
                            continue
                        ct = t_of(c, "after_call")
                        if ut is None or ct is None or not ct < ut:
                            continue
                        sp = succ.get(c.get("callee"))
                        if sp is None:
                            ne.append(c.get("call"))
                            continue
                        if c.get("status_value") != hx(sp["value"]):
                            continue
                        ids = {(c.get("out") or {}).get("value"), (c.get("out") or {}).get("raw")} - {None}
                        eb_ = ends_between(r, ct, ut, u["key"]["text"], ids)
                        if eb_:   # excused by a lifetime end between (judgement rules 8.13 item 13); a slot clear alone is weak
                            (weak if all(x.get("names") == "slot" for x in eb_) else exc).append(
                                "%s (%d end(s): %s)" % (c.get("call"), len(eb_), ",".join(sorted({str(x.get("names")) for x in eb_}))))
                            continue
                        hit.append("%s (%s)" % (c.get("call"), S["ref"](sp["n"])))
                    if hit:
                        emit("F3", "boot", br, key, i, "fail", fn, "absent after a finished successful create of the same kind and text: " + "; ".join(hit),
                             unit=("F3", br, fn))
                    elif ne:
                        emit("F3", "boot", br, key, i, "not_evaluable", fn, "the success value of %s is not read (no success path line)" % "; ".join(ne),
                             unit=("F3", br, fn))
                    else:
                        emit("F3", "boot", br, key, i, "pass", fn,
                             ("no earlier finished successful create of the same kind and text" if not (exc or weak) else
                              "every earlier finished successful create of the same kind and text has a lifetime end before the use")
                             + ("; excused by a lifetime end: " + "; ".join(exc) if exc else "")
                             + ("; excused by a slot clear only (weak: judgement rules 8.13 item 13): " + "; ".join(weak) if weak else ""),
                             unit=("F3", br, fn))
            if r.get("path") != "A":
                emit("F3", "boot", br, key, "*", "not_evaluable", fn, "not a path A record (judgement rules 8.2 applies to path A only)", unit=("F3", br, fn))
            elif n3 == 0:
                emit("F3", "boot", br, key, "*", "not_evaluable", fn, "no absent use event in this record", unit=("F3", br, fn))
            # F4 (found uses)
            expected.add(("F4", br, fn))
            n4 = 0
            for i, u in enumerate(ue):
                if u.get("outcome") != "found":
                    continue
                n4 += 1
                fid = (u.get("out") or {}).get("value")
                ut = t_of(u, "at_call") if t_of(u, "at_call") is not None else t_of(u, "after_call")
                cs = [c for c in pe if fid is not None and fid in {(c.get("out") or {}).get("value"), (c.get("out") or {}).get("raw")}
                      and t_of(c, "at_call") is not None and ut is not None and t_of(c, "at_call") < ut]
                emit("F4", "boot", br, key, i, "pass" if cs else "fail", fn,
                     ("create of the id %s recorded: %s" % (fid, cs[0].get("call"))) if cs else "no recorded create whose out value is the found id %s" % fid,
                     unit=("F4", br, fn))
            if n4 == 0:
                emit("F4", "boot", br, key, "*", "not_evaluable", fn, "no found use event in this record", unit=("F4", br, fn))
            # F5, s2-s4, expect_discriminate (control rows)
            if key in control:
                expected.add(("F5", br, fn))
                bad, note = [], []
                for cid, a in sorted(S["alts"].items()):
                    if a["verdict"] != "found":
                        continue
                    xs = [x for x in uc.get("branch_iii", []) if x.get("context") == cid]
                    if any(x.get("status") == "held" for x in xs):
                        bad.append("(iii) of context %s held against the found line %s" % (cid, S["ref"](a["n"])))
                    note.append("context %s found (%s)" % (cid, S["ref"](a["n"])))
                for n, t in S["holders"]:
                    if other_definitions(t, "Rb" in readings) and uc.get("branch_i", {}).get("status") == "held":
                        bad.append("(i) held against the other definitions of %s" % S["ref"](n))
                emit("F5", "static", br, key, "*", "fail" if bad else "pass", fn, "; ".join(bad or note) or "no found line and no other definition",
                     unit=("F5", br, fn))
                expected.add(("s2", br, fn))
                h = [x.get("context") for x in uc.get("branch_iii", []) if x.get("status") == "held"]
                hi = uc.get("branch_i", {}).get("status") == "held"
                emit("s2", "static", br, key, "*", "fail" if (h or hi) else "pass", fn,
                     ("held: %s%s" % ("(i) " if hi else "", " ".join("(iii) context %s" % c for c in h))) if (h or hi) else "no held (c3) branch",
                     unit=("s2", br, fn))
                if data == "D1":
                    expected.add(("s1", br, fn))
                    nb = [p for p in pairs_r if p.get("result") == "bound"]
                    emit("s1", "boot" if gp else "static", br, key, "*", "fail" if nb else "pass", fn, "%d bound pairs" % len(nb), unit=("s1", br, fn))
                expected.add(("s3", br, fn))
                bad = []
                for i, u in enumerate(ue):
                    il = u.get("instance_link") or {}
                    if il.get("status") != "linked":
                        continue
                    ut = t_of(u, "at_call")
                    cs = [c for c in pe if il.get("id") in {(c.get("out") or {}).get("value")} and t_of(c, "at_call") is not None
                          and ut is not None and t_of(c, "at_call") < ut]
                    if not cs or ends_between(r, t_of(cs[0], "after_call") or 0, ut, (u.get("key") or {}).get("text"), {il.get("id")}):
                        bad.append("use %d linked to %s without an open generation in the record" % (i, il.get("id")))
                emit("s3", "boot" if gp else "static", br, key, "*", "fail" if bad else "pass", fn,
                     "; ".join(bad) or ("no instance link without an open generation" + ("" if gp else " (vacuous: no generation can form)")), unit=("s3", br, fn))
                expected.add(("s4", br, fn))
                bad = []
                for i, p in enumerate(pairs_r):
                    if p.get("result") != "refuted":
                        continue
                    it = p.get("items", {})
                    need = ["basis1_same_build", "basis2_ties", "basis4_creator_success", "basis5_key_texts"] + list(FIVE)
                    ok_ = all((it.get(k) or {}).get("status") == "held" for k in need)
                    uk = (ue[p["use"]].get("key") if p.get("use") is not None and p["use"] < len(ue) else None) or {}
                    ck = next((c.get("key") for c in pe if c.get("actor") == p.get("create_actor") and (c.get("at_call") or {}).get("aseq") == p.get("create_aseq")), None) or {}
                    if not (ok_ and complete(uk) and complete(ck) and uk.get("text") != ck.get("text")):
                        bad.append("pair %d refuted without the five conditions, bases 1, 2, 4, 5 and two complete different key texts" % i)
                emit("s4", "boot" if gp else "static", br, key, "*", "fail" if bad else "pass", fn, "; ".join(bad) or "no refuted pair without its conditions",
                     unit=("s4", br, fn))
                # expect_discriminate (evaluation protocol 14.7 rules (1)-(3))
                expected.add(("ED", br, fn))
                req, got = [], []
                for cid, a in sorted(S["alts"].items()):
                    if a["verdict"] == "found":
                        req.append("(1) context %s (iii) not_held on %s" % (cid, S["ref"](a["n"])))
                        if any(x.get("context") == cid and x.get("status") == "not_held" and any(s.get("ref") == S["ref"](a["n"]) for s in x.get("sources", []))
                               for x in uc.get("branch_iii", [])):
                            got.append(req[-1])
                for n, t in S["holders"]:
                    if other_definitions(t, "Rb" in readings):
                        req.append("(2) (i) not_held on %s" % S["ref"](n))
                        bi = uc.get("branch_i", {})
                        if bi.get("status") == "not_held" and any(s.get("ref") == S["ref"](n) for s in bi.get("sources", [])):
                            got.append(req[-1])
                # evaluation protocol 14.13 items 1-2: rule (3) only on a control row with no requirement (1) or (2);
                # the class from the static inputs: (1) on a row whose (iii) entries follow the record's contexts (context
                # lines in the sidecar, or not exactly one alternatives context) -> boot; no (1)/(2): boot when a success
                # path line is held (a generation can form); else static
                has1 = any(x.startswith("(1) ") for x in req)
                if req:
                    rules_ = "rules (1)-(2)"
                    cls = "boot" if has1 and (side_ctx.get(key) or len(S["alts"]) != 1) else "static"
                else:
                    rules_ = "rule (3)"
                    cls = "boot" if gp else "static"
                    for i, u in enumerate(ue):
                        if u.get("outcome") == "found":
                            req.append("(3) use %d instance-linked on an open generation" % i)
                            if (u.get("instance_link") or {}).get("status") == "linked":
                                got.append(req[-1])
                other = [x for x in ("refuted_link", "not_observed_in_runs") if r["tool_verdict"]["violation"]["value"] == x]
                if key not in only_control:
                    res = "not_counted"
                elif not req:
                    res = "unmet"
                else:
                    res = "met" if len(got) == len(req) else "unmet"
                emit("expect_discriminate", cls, br, key, "*", res, fn, "%s: " % rules_ +
                     ("required: %s; given: %s" % (" | ".join(req), " | ".join(got) or "none") if req else "nothing to require from the static lines and the record")
                     + ("; other grounds: " + ", ".join(other) if other else "") + ("; plan_state tier+control: not counted" if res == "not_counted" else ""),
                     unit=("ED", br, fn))
            # F6
            expected.add(("F6", br, fn))
            dec = [(cid, a) for cid, a in sorted(S["alts"].items()) if a["decided"] not in ("none", "")]
            emit("F6", "static", br, key, "*", "not_evaluable" if dec else "pass", fn,
                 ("contexts decided by the stored-field rule: %s; the plan names no site in the excluded branch (limit)" % ", ".join(c for c, _ in dec))
                 if dec else "nothing to check: no context decided by the stored-field rule", unit=("F6", br, fn))
            # F7 (R0 branches, every pre-defined row)
            if readings == {"R0"} and key in rows_of_key:
                expected.add(("F7", br, key))
                bad = uc.get("status") == "derived" or r["tool_verdict"]["violation"]["value"] == "observed"
                emit("F7", "format", br, key, "*", "fail" if bad else "pass", fn, "use_cond %s, violation %s" % (uc.get("status"), r["tool_verdict"]["violation"]["value"]),
                     unit=("F7", br, key))
            # F8 (b)
            expected.add(("F8b", br, fn))
            og = [g for g in r.get("order_ground", []) if isinstance(g, dict)]
            if not og:
                emit("F8b", "boot", br, key, "*", "not_evaluable", fn, "no linked pair: the order ground is order_basis_unknown", unit=("F8b", br, fn))
            for g in og:
                n = (g.get("src") or [None, None])[1]
                line = [x for x in pairs.get(key, []) if x["n"] == n]
                emit("F8b", "boot", br, key, "*", "pass" if line and line[0].get("value") == g.get("value") else "fail", "%s:%s" % (os.path.basename(o.pairs), n),
                     "pair-file line %s value %s, the record's %s" % (n, line[0].get("value") if line else "absent", g.get("value")), unit=("F8b", br, fn))
            # F9 (rows with two or more context lines)
            if len(side_ctx.get(key, [])) >= 2:
                expected.add(("F9", br, fn))
                unk = [i for i, u in enumerate(ue) if u.get("context") is None]
                emit("F9", "boot", br, key, "*", "fail" if unk else ("pass" if ue else "not_evaluable"), fn,
                     ("uses with context unknown: %s" % unk) if unk else ("%d use events, each with one context" % len(ue) if ue else "no use event in this record"),
                     unit=("F9", br, fn))
            # F10
            expected.add(("F10", br, fn))
            if gate != "PASS":
                emit("F10", "boot", br, key, "*", "not_evaluable", o.gate, "the gate is %s: F10 is read on a boot that passed the gate" % gate, unit=("F10", br, fn))
            else:
                sb = r.get("same_build") or {}
                badb = sorted(k for k, v in sb.items() if v != "confirmed")
                emit("F10", "boot", br, key, "*", "fail" if (badb or not sb) else "pass", fn,
                     ("objects not confirmed: %s" % ", ".join(badb)) if badb else ("no same_build_object line in the record" if not sb else "every object confirmed"),
                     unit=("F10", br, fn))
    # ---- F8 (a) per pre-defined row (static: the pair file against the relations row)
    for key in sorted(rows_of_key):
        expected.add(("F8a", key))
        S = srow(key)
        locs = [p for _, p in (S["creators"] or S["writers"]) if p]
        miss = [p for p in locs if not any(x.get("other_pos", "").endswith(p) for x in pairs.get(key, []))]
        emit("F8a", "static", "*", key, "*", "fail" if miss else "pass", os.path.basename(o.pairs),
             ("no pair-file line for %s" % ", ".join(miss)) if miss else "%d creator or writer locations, each with a pair-file line; the text output "
             "(order.txt) and the value side are not inputs of this script and are not compared" % len(locs), unit=("F8a", key))
    # ---- precision, in-plan recall, undetermined split (evaluation protocol 14.3), per branch
    reader = any(any(v != NOT_FILLED for k, v in r.items() if k not in ("build", "row_label", "row_key", "consumer_root", "use_call", "context_id"))
                 for r in sst if r.get("build") == o.build) or sbt is not None
    sheet = {}
    for r in sbt or []:
        if r.get("build") == o.build:
            sheet[(r["row_key"], r["consumer_root"], r["context_id"])] = r
    metrics = []
    for br in sorted(per):
        recs = per[br]
        pos_, rec_of = [], {}
        for u in uses:
            hit = []
            for fn, r in recs:
                if r["row_id"]["use_site_key"] != u["row_key"] or root_of(r) != u["consumer_root"]:
                    continue
                ev = [e for e in r["order"]["use_events"] if {u["at_site"], u["after_site"]} & {str((e.get("at_call") or {}).get("site")), str((e.get("after_call") or {}).get("site"))}
                      and (u["context_id"] == "-" or str(e.get("context")) == u["context_id"])]
                if ev:
                    hit.append(r)
            rec_of[(u["row_key"], u["consumer_root"], u["context_id"])] = hit
            if any(r["tool_verdict"]["violation"]["value"] == "observed" for r in hit):
                pos_.append((u["row_key"], u["consumer_root"], u["context_id"]))
        N = len(pos_)
        T = sum(1 for k in pos_ if sheet.get(k, {}).get("reading") == "T")
        F = sum(1 for k in pos_ if sheet.get(k, {}).get("reading") == "F")
        U = N - T - F
        if not reader:
            pv = "undefined(no reader)"
        elif N == 0:
            pv = "undefined(no tool positive)"
        elif T + F == 0:
            pv = "undefined(T + F = 0)"
        else:
            pv = "N %d T %d F %d U %d; completed %d/%d; conditional %d/%d; range %d/%d-%d/%d%s" % (
                N, T, F, U, T + F, N, T, T + F, T, N, T + U, N, "; not final (U > 0)" if U else "")
        metrics.append(("precision", br, pv))
        L.append({"check": "precision", "class": "boot", "branch": br, "row_label": "*", "row_key": "*", "use": "*", "result": pv, "ground": "-",
                  "note": "tool positives are the pre-defined uses whose record's violation is observed"})
        kpos = [k for k, s in sheet.items() if s.get("label") == "positive" and k in rec_of]
        out_plan = [k for k, s in sheet.items() if s.get("label") == "positive" and k not in rec_of]
        unread = [k for k, s in sheet.items() if s.get("label") == "unreadable"]
        found = [k for k in kpos if any(r["tool_verdict"]["violation"]["value"] == "observed" for r in rec_of[k])]
        und_k, neg_k, unr_k = {}, 0, 0
        for k in kpos:
            vs = {r["tool_verdict"]["violation"]["value"] for r in rec_of[k]}
            if not rec_of[k]:
                unr_k += 1
            elif "undetermined" in vs and "observed" not in vs:
                for kk in {reason_kind(x) for r in rec_of[k] for x in r["tool_verdict"]["violation"]["undetermined_reasons"]}:
                    und_k[kk] = und_k.get(kk, 0) + 1
            elif vs & {"not_observed_in_runs", "refuted_link"} and "observed" not in vs:
                neg_k += 1
        if not reader:
            rv = "undefined(no reader)"
        elif not kpos:
            rv = "undefined(k = 0)"
        else:
            rv = "%d/%d" % (len(found), len(kpos))
        side = "beside: undetermined by kind %s; explicit negatives %d; not reached %d; out of plan positives %d; unreadable %d" % (
            ", ".join("%s %d" % (k, und_k.get(k, 0)) for k in KINDS), neg_k, unr_k, len(out_plan), len(unread))
        metrics.append(("in_plan_recall", br, rv + "; " + side))
        L.append({"check": "in_plan_recall", "class": "boot", "branch": br, "row_label": "*", "row_key": "*", "use": "*", "result": rv, "ground": "-", "note": side})
        split = {}
        nu = 0
        for k, hit in rec_of.items():
            if hit and all(r["tool_verdict"]["violation"]["value"] == "undetermined" for r in hit):
                nu += 1
                for kk in {reason_kind(x) for r in hit for x in r["tool_verdict"]["violation"]["undetermined_reasons"]}:
                    split[kk] = split.get(kk, 0) + 1
        sv = "uses undetermined %d: %s (a use counts once in each kind its record's reasons have; explicit negatives apart)" % (
            nu, ", ".join("%s %d" % (k, split.get(k, 0)) for k in KINDS))
        metrics.append(("undetermined_split", br, sv))
        L.append({"check": "undetermined_split", "class": "boot", "branch": br, "row_label": "*", "row_key": "*", "use": "*", "result": "%d" % nu,
                  "ground": "-", "note": sv})
    # ---- tail and the form check
    counts = {}
    for x in L:
        k = (x["check"], x["class"], x["result"] if x["result"] in RESULTS else "value", x["branch"])
        counts[k] = counts.get(k, 0) + 1
    eb_of = {}
    for br in per:
        if per[br]:
            e = per[br][0][1].get("eval_branch", {})
            eb_of[br] = (e.get("data") or "?", ",".join(e.get("readings") or ["R0"]))

    def label(br):   # evaluation protocol 14.1 and 14.13 item 3
        if br == MAIN:
            return "main (declared before the boot: D1/R0)"
        if br == "*":
            return "no branch (the pair file against the relations rows, F8 (a))"
        d, rd = eb_of.get(br, ("?", "?"))
        return "auxiliary (data %s, readings %s)" % (d, rd)
    order_ = ([MAIN] if any(x["branch"] == MAIN for x in L) else []) + sorted({x["branch"] for x in L} - {MAIN, "*"}) + \
        (["*"] if any(x["branch"] == "*" for x in L) else [])

    def by_class(xs):
        out = []
        for c in CLASSES:
            ys = [x for x in xs if x["class"] == c]
            out.append("%s: pass %d / %d (fail %d, not evaluable %d)" % (c, sum(1 for y in ys if y["result"] == "pass"),
                                                                       sum(1 for y in ys if y["result"] in ("pass", "fail")),
                                                                       sum(1 for y in ys if y["result"] == "fail"), sum(1 for y in ys if y["result"] == "not_evaluable")))
        return "; ".join(out)
    head = ["kz-eval 1", "script %s sha256 %s" % (EVAL_REV, self_sha), "boot %s build %s gate %s" % (o.boot, o.build, gate)]
    head += ["input %s %s %s" % x for x in INPUTS if not x[0].startswith("record:")]
    nrec = sum(1 for x in INPUTS if x[0].startswith("record:"))
    head.append("records %d (sha256 of every record: %s)" % (nrec, hashlib.sha256("".join(x[1] for x in INPUTS if x[0].startswith("record:")).encode()).hexdigest()))
    head.append("columns check\tclass\tbranch\tbuild\trow_label\trow_key\tuse\tresult\tground\tnote")
    body = ["\t".join([x["check"], x["class"], x["branch"], o.build, x["row_label"], x["row_key"], x["use"], x["result"], x["ground"], x["note"]]) for x in L]
    tail = ["count\t%s\t%s\t%s\t%s\t%d" % (k[0], k[1], k[2], k[3], n) for k, n in sorted(counts.items(), key=lambda kv: (order_.index(kv[0][3]), kv[0]))]
    for br in order_:
        lb = "branch %s %s: " % (br, label(br))
        fb = [x for x in L if x["branch"] == br and re.fullmatch(r"F\d+[ab]?", x["check"])]
        sb = [x for x in L if x["branch"] == br and x["check"] in ("s1", "s2", "s3", "s4")]
        eb = [x for x in L if x["branch"] == br and x["check"] == "expect_discriminate"]
        tail.append(lb + "falsifiers F1-F10 by class (only the boot class is a falsifier result): " + by_class(fb))
        if br != "*":
            tail.append(lb + "expect_safe s1-s4 by class (only the boot class is a safety result): " + by_class(sb))
            tail.append(lb + "expect_discriminate: met %d / counted %d (unmet %d; not_counted %d); by class: %s" % (
                sum(1 for x in eb if x["result"] == "met"), sum(1 for x in eb if x["result"] in ("met", "unmet")), sum(1 for x in eb if x["result"] == "unmet"),
                sum(1 for x in eb if x["result"] == "not_counted"),
                ", ".join("%s %d" % (c, sum(1 for x in eb if x["class"] == c)) for c in CLASSES)))
    for m in metrics:
        tail.append("%s %s %s: %s" % (m[0], m[1], label(m[1]), m[2]))
    # form check
    bad = []
    for x in L:
        if x["class"] not in CLASSES:
            bad.append("class %r" % x["class"])
        if x["check"] not in ("precision", "in_plan_recall", "undetermined_split") and x["result"] not in RESULTS:
            bad.append("result %r of %s" % (x["result"], x["check"]))
    miss = sorted(expected - units, key=str)
    if miss:
        bad.append("units without a line: %s" % ", ".join(str(m) for m in miss[:10]))
    if sum(counts.values()) != len(body):
        bad.append("the tail counts %d lines, the body has %d" % (sum(counts.values()), len(body)))
    tail.append("form %s%s" % ("ok" if not bad else "FAILED: ", "; ".join(bad)))
    tail += ["lines %d" % len(body), "end"]
    with open(o.out, "w", encoding="utf-8") as f:
        f.write("\n".join(head + body + tail) + "\n")
    print("eval %s: lines %d; form %s" % (o.out, len(body), "ok" if not bad else "FAILED"))
    for t in tail:
        if t.startswith("branch %s main " % MAIN):
            print(t)
    return 3 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
