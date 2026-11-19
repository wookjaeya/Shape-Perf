#!/usr/bin/env python3
"""test_explain4.py -- fixtures of the explanation script revision 4 (plan/c1_closure_explain_rev4_spec_20261118.md 6, 10).

    python3 -I test_explain4.py OBS1_DIR VOCABULARY WORK_DIR

OBS1_DIR is env/obs1 (explain.py, explain_read.py, explain_judge.py, explain_branches.py, explain_summary.py,
explain_expect.py).  Every fixture is a synthetic world (no case name, no app, function or key of a case): a path A row
of one name space with a consumer and a maker application, a process actor that starts them, a pair file, a name-space
data file, a candidate list and admission files, a dump text and a chain-check text.  Each case changes the world, runs
the driver into a new directory under WORK_DIR and checks the record.  Prints one line per check and a count; rc 0 when
every check passes.
"""
import copy
import hashlib
import json
import os
import shutil
import subprocess
import sys

OBS, VOC, WORK = sys.argv[1], sys.argv[2], sys.argv[3]
PY = sys.executable
F = "f" * 64
KEY = "00000000000000a1"
T = "a" * 64
RESULTS = []
FX = {}     # records kept for the format-check breaks of spec 11 items 1, 4 and 6


def ok(name, cond, detail=""):
    RESULTS.append((name, bool(cond)))
    print("%s %s%s" % ("PASS" if cond else "FAIL", name, ("" if cond else "  [%s]" % detail)))


def sha(path):
    with open(path, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()


def hx64(v):
    return "0x%x" % (v & 0xFFFFFFFFFFFFFFFF)


# ------------------------------------------------------------------------------------------------ the world
def world():
    W = {}
    W["rel"] = {
        "actors": "actors: {ConsumerMain (script-entry)}; result=tested",
        "holder": "holder: field Cons_t.obj_id; writers 1: own out store app/cons.c:50:9; FIELD_STATUS per-field=writer-set-complete "
                  "type-level=writer-set-complete; readers 1: cons_run at app/cons.c:80:5: uses the value without such a test (tests 0)",
        "creator": 'creator: key known: app/maker.c:30:5 in maker_init -> Lib_Create key string literal "W1" actors {MakerMain (script-entry)}',
        "success": "callee-check (success path): Lib_Create: held: success value 0 [outcome class rule]; carried by 1 alternative(s): "
                   "lib/ns.c:59:5 alt 0/1 = constant 0 under always; excluded: none; for alt 0/1 at lib/ns.c:59:5: name store",
        "keywriters": "writers: NS_rec_t.name_ptr: 2 (assign 2) from WRITERS; premises: FIELD_STATUS per-field writer-set-complete, "
                      "type-level writer-set-complete (writers 2)",
        "absence": "absence: -17 (no match: Lib_FindByName -> Lib_Search) at the call app/cons.c:50:9: reached; effects: exit-app at app/cons.c:95:5 complete",
        "origins": "absence origins: -17 [ERR_NOT_FOUND]: 2 origin(s): Lib_FindByName at lib/ns.c:10:5 when (not param:name#0) [other]; "
                   "Lib_Search at lib/ns.c:22:5 when (not (call-result Lib_Cmp)) [search no-match]",
        "alt1": "alternatives [context 1: ConsumerMain@app/cons.c:90:5 <- cons_init@app/cons.c:50:9]: none found (exploration complete); "
                "rests on: P0-unverified(xexit); premises: none; decided calls: none",
        "alt2": "alternatives [context 2: cons_cmd@app/cons.c:120:5 <- cons_init@app/cons.c:50:9]: found: retry: app/cons.c:122:9; "
                "rests on: none; premises: none; decided calls: none",
        "feas": "feasibility: possible: the facts give an order",
        "flip": "flip: static-possible: the absence value reaches the exit with the earlier value; search returns: Lib_Search returns -17 [ERR_NOT_FOUND], Lib_Search returns 0 [OK]",
        "layers": "layers: B P0 R",
    }
    W["rel_extra"] = []          # extra row body lines
    W["ns"] = {
        "w4": "writers [partition {4}] of NS_rec_t.name_ptr: 1: assign at lib/ns.c:60:9 in Lib_Create value=symbolic [rec->buf]",
        "wsh": "writers [shared by partitions {4} {5}] of NS_rec_t.name_ptr: 1: assign at lib/ns.c:70:9 in Lib_FindFree value=0 [NULL]",
        "buf": "stored-name buffer [partition {4}]: Widget_rec_t.wname extent 20 [MAX_NAME]; name stores 1: lib/ns.c:60:9 in Lib_Create; "
               "writers 2 from WRITERS: create copy lib/ns.c:60:9 in Lib_Create [memcpy]; clear lib/ns.c:5:5 in Lib_Init [fill 0]; "
               "FIELD_STATUS per-field=writer-set-complete type-level=writer-set-complete (writers 2); causes: none; address uses 0: "
               "discharged 0, not discharged 0 (the stored-name address uses line); complete",
        "lend": "lifetime end [all partitions]: clear-table lib/ns.c:5:5 in Lib_Init: names no id: every record of NS_table [fill 0] (NS_rec_t.name_ptr)",
    }
    W["head_oa"] = True
    W["entry"] = {"row": KEY, "data": None, "sha256": None, "line": "10", "kind": "KIND_WIDGET", "value": "4", "kind-at": "lib/ns.h:2",
                  "lookup": "Lib_FindByName:applies", "create": "Lib_Create:applies", "compare": "Lib_Cmp:applies",
                  "compare-call": "strcmp@lib/ns.c:12", "stored-name": "Widget_rec_t.wname", "decl": "lib/ns.h:3", "extent": "MAX_NAME",
                  "extent-value": "20", "extent-source": "FIELD_TYPES", "copy-call": "strncpy@lib/ns.c:60", "length-call": "memchr@lib/ns.c:11",
                  "success-path": "Lib_Create:held"}
    W["needs"] = [("kind-value", "planned", "sites=12,22"), ("object-delete", "planned", "sites=30,31"),
                  ("object-rollback", "planned", "sites=32"), ("object-clear", "planned", "sites=33")]
    W["lifetimes"] = ["lifetime KIND_WIDGET delete Lib_Delete lib/ns.c:80 data NSPATH line 12: applies sites=30,31",
                      "lifetime KIND_WIDGET rollback Lib_Create lib/ns.c:55 data NSPATH line 13: applies; create-finalize Lib_FinalizeNew at lib/ns.c:58:5, status arg0, out arg2; reentry=none sites=32",
                      "lifetime KIND_WIDGET clear-table Lib_Init lib/ns.c:3 data NSPATH line 14: applies; fill memset@lib/ns.c:5 sites=33"]
    W["lc"] = "lifetime-complete kind=KIND_WIDGET yes"
    W["contexts"] = ['context row=00000000000000a1 id=1 frame=40,41 chain="ConsumerMain@app/cons.c:90:5 <- cons_init@app/cons.c:50:9"',
                     'context row=00000000000000a1 id=2 frame=42,43 chain="cons_cmd@app/cons.c:120:5 <- cons_init@app/cons.c:50:9"']
    W["sites"] = {
        3: ("s0003.actor-create", "op=call.TaskSpawn phase=point tu=core/load.c func=main_loader callee=TaskSpawn line=10 place=at-call col=3 nth=1",
            [("creation_arg", "arg3", "u64")]),
        4: ("s0004.actor-create", "op=call.TaskSpawn phase=point tu=core/load.c func=main_loader callee=TaskSpawn line=10 place=after-call col=3 nth=1",
            [("status", "ret", "s64")]),
        5: ("s0005.actor-root+actor-start", 'op=entry.ConsumerMain phase=before tu=app/cons.c func=ConsumerMain ref="actor ConsumerMain (script-entry)"',
            [("param0", "param0", "u64")]),
        6: ("s0006.actor-root+actor-start", 'op=entry.MakerMain phase=before tu=app/maker.c func=MakerMain ref="actor MakerMain (script-entry)"',
            [("param0", "param0", "u64")]),
        10: ("s0010.use-call", "op=call.Lib_FindByName phase=point tu=app/cons.c func=cons_init callee=Lib_FindByName line=50 place=at-call col=9 nth=1 row=00000000000000a1",
             [("key", "arg0", "ptr")]),
        11: ("s0011.use-call", "op=call.Lib_FindByName phase=point tu=app/cons.c func=cons_init callee=Lib_FindByName line=50 place=after-call col=9 nth=1 row=00000000000000a1",
             [("status", "ret", "s64"), ("out0", "out1", "u64")]),
        12: ("s0012.key-text+kind-value", "op=call.Lib_Search phase=point tu=lib/ns.c func=Lib_FindByName callee=Lib_Search line=20 place=at-call col=5 nth=1 row=00000000000000a1",
             [("key_text", "arg1", "ptr string=validated bound=20"), ("kind", "arg0", "u64")]),
        20: ("s0020.creator-call", "op=call.Lib_Create phase=point tu=app/maker.c func=maker_init callee=Lib_Create line=30 place=at-call col=5 nth=1 row=00000000000000a1",
             [("key", "arg0", "ptr")]),
        21: ("s0021.creator-call", "op=call.Lib_Create phase=point tu=app/maker.c func=maker_init callee=Lib_Create line=30 place=after-call col=5 nth=1 row=00000000000000a1",
             [("status", "ret", "s64"), ("out0", "out1", "u64")]),
        22: ("s0022.key-text+kind-value", "op=call.Lib_Alloc phase=point tu=lib/ns.c func=Lib_Create callee=Lib_Alloc line=40 place=at-call col=5 nth=1 row=00000000000000a1",
             [("key_text", "arg1", "ptr string=validated bound=20"), ("kind", "arg0", "u64")]),
        30: ("s0030.object-delete", "op=entry.Lib_Delete phase=before tu=lib/ns.c func=Lib_Delete", [("param0", "param0", "u64")]),
        31: ("s0031.object-delete", "op=call.Lib_FinalizeDelete phase=point tu=lib/ns.c func=Lib_Delete callee=Lib_FinalizeDelete line=85 place=after-call col=5 nth=1",
             [("status", "ret", "s64")]),
        32: ("s0032.object-rollback", "op=call.Lib_FinalizeNew phase=point tu=lib/ns.c func=Lib_Create callee=Lib_FinalizeNew line=58 place=at-call col=5 nth=1",
             [("status", "arg0", "u64")]),
        33: ("s0033.object-clear", "op=call.memset phase=point tu=lib/ns.c func=Lib_Init callee=memset line=5 place=at-call col=5 nth=1", [("arg0", "arg0", "ptr")]),
        40: ("s0040.context-frame", "op=call.cons_init phase=point tu=app/cons.c func=ConsumerMain callee=cons_init line=90 place=at-call col=5 nth=1 row=00000000000000a1", []),
        41: ("s0041.context-frame", "op=call.cons_init phase=point tu=app/cons.c func=ConsumerMain callee=cons_init line=90 place=after-call col=5 nth=1 row=00000000000000a1", []),
        42: ("s0042.context-frame", "op=call.cons_init phase=point tu=app/cons.c func=cons_cmd callee=cons_init line=120 place=at-call col=5 nth=1 row=00000000000000a1", []),
        43: ("s0043.context-frame", "op=call.cons_init phase=point tu=app/cons.c func=cons_cmd callee=cons_init line=120 place=after-call col=5 nth=1 row=00000000000000a1", []),
    }
    W["map"] = [10, 11, 12, 20, 21, 22, 30, 31, 32, 33, 40, 41, 42, 43]
    W["unbound"] = set()
    # events: (actor, site, t, values, str, flags); aseq in actor order
    W["events"] = [
        ("1", 3, 100, [0xA], None, ""), ("1", 4, 110, [0], None, ""), ("1", 3, 120, [0xB], None, ""), ("1", 4, 130, [0], None, ""),
        ("2", 5, 200, [0xA], None, ""), ("2", 40, 900, [], None, ""), ("2", 10, 1000, [0x1000], None, ""),
        ("2", 12, 1050, [0x1000, 4], "W1", ""), ("2", 11, 1100, [-17, 0], None, ""), ("2", 41, 1200, [], None, ""),
        ("3", 6, 300, [0xB], None, ""), ("3", 20, 2000, [0x2000], None, ""), ("3", 22, 2050, [0x2000, 4], "W1", ""),
        ("3", 21, 2100, [0, 0x10001], None, ""),
    ]
    W["keys"] = {"1": "tid:100", "2": "tid:101", "3": "tid:102", "4": "tid:103"}
    W["buildids"] = ["buildid object=main path=\"core\" build_id=11 note=present source=start actor=- t=1",
                     "buildid object=module:1 path=\"core\" build_id=11 note=present source=registration actor=- t=2"]
    W["format"] = "format kzrt-dump 1.5 dump_flags=0x1 records=9 unknown=0 partial=0 pads=0 end=yes"
    W["chain"] = ["chain\tconfirmed\t3 links", "same_build_object\tconfirmed\tmain path=core tree=11 dump=11",
                  "same_build_object\tconfirmed\tmodule:1 path=core tree=11 dump=11", "same_build\tconfirmed\t2 objects: confirmed 2, refused 0, unconfirmed 0"]
    W["tok"] = []
    W["pairs"] = [(KEY, "A", "app/cons.c:50:9", "cons_init", "ConsumerMain (script-entry)", "creator", "/abs/app/maker.c:30:5", "maker_init",
                   "MakerMain (script-entry)", "unordered", "-", "-", "creator")]
    W["pairs_facts"], W["pairs_tool"], W["pairs_tail"] = F, T, None
    W["cands"] = [("P0", "strcmp", "source", "p0", "-", "1", "compare-call", "byte-string comparison"),
                  ("P0", "strncpy", "source", "p0", "-", "2", "copy-call", "data function"),
                  ("P0", "memchr", "source", "p0", "-", "3", "length-call", "data function"),
                  ("P0", "xexit", "source", "p0", "-", "4", "P0-unverified", "termination")]
    # the named-calls file (spec 11 item 2): function -> (named_by, records, unverified, layers); None writes no file
    W["named"] = {"strcmp": ("compare-call", "3", "1", "P0"), "strncpy": ("copy-call", "4", "2", "P0"),
                  "memchr": ("length-call", "3", "1", "P0")}
    W["named_over"] = {}        # head / tail / plan / drop overrides of the named-calls file
    W["admitted"] = []          # list of (branch data, kinds or None for all, overrides)
    W["readings"] = None        # None: the 8 of each data branch
    W["give_ns"] = True
    W["give_pairs"] = True
    W["give_cands"] = True
    W["run"] = True
    W["row_head"] = "  A1 lookup app/cons.c:50:9 in cons_init -> Lib_FindByName#0 namespace NS_rec_t.name_ptr partition {4} key known"
    return W


def write_world(W, d):
    os.makedirs(d)
    p = lambda n: os.path.join(d, n)
    with open(p("ns2.txt"), "w") as f:
        f.write("# synthetic name-space data\nsemantics-data 2\nrule coverage rule revision 2 (synthetic)\n")
    nsh = sha(p("ns2.txt"))
    e = dict(W["entry"])
    e["data"], e["sha256"] = e["data"] or p("ns2.txt"), e["sha256"] or nsh
    sc = ["kz-plan-sidecar 2", "generator tool=kz-static tool_sha256=%s tool_source=%s core_source=%s" % (T, "b" * 64, "c" * 64),
          "input facts sha256=%s path=/x/facts.kz" % F, "input namespace-data sha256=%s path=%s" % (nsh, p("ns2.txt")),
          "rules plan rules revision 5 (synthetic)", "manifest-revision 4 (kzs-sites 4)",
          "row 00000000000000a1 " + W.get("rid", "A1") + " tier=1 position=1 label=R fact-label=R class=decided order=RELOC reach=REACH disposition=D07 state=planned control=b control-state=planned",
          "map 00000000000000a1 " + " ".join(str(x) for x in W["map"])]
    for what, st, sites in W["needs"]:
        sc.append("need row 00000000000000a1 %s %s reason=- note=\"\" %s" % (what, st, sites))
    sc.extend(x.replace("NSPATH", p("ns2.txt")) for x in W["lifetimes"])
    if W["lc"]:
        sc.append(W["lc"])
    if W["entry"] is not None and W.get("entry_on", True):
        sc.append("namespace-entry " + " ".join("%s=%s" % (k, v) for k, v in e.items()))
    sc.extend(W["contexts"])
    with open(p("plan.side"), "w") as f:
        f.write("\n".join(sc) + "\n")
    m = ["kzs-sites 4", "# sidecar sha256=%s" % sha(p("plan.side"))]
    for n, (lab, rest, vals) in sorted(W["sites"].items()):
        m.append("site %d label=%s %s" % (n, lab, rest))
        for i, (role, frm, typ) in enumerate(vals):
            m.append("value %d %d role=%s from=%s type=%s" % (n, i, role, frm, typ))
    with open(p("plan.kzs"), "w") as f:
        f.write("\n".join(m) + "\n")
    with open(p("binds.kzs"), "w") as f:
        for n in sorted(W["sites"]):
            if n not in W["unbound"]:
                f.write("bind site=%d status=bound vals=0xf\n" % n)
    rel = ["kz-relations 1", "facts " + F, "rules relations rules revision 12 (synthetic)", "tool kz-static tool_sha256=%s tool_source=%s" % (T, "b" * 64)]
    if W["head_oa"]:
        rel.append("rules with object access: relations rules revision 14 (synthetic): with RESULT_FLOW and RESULT_EFFECT: the success path, absence origins, holder, alternatives, stored-name buffer and lifetime-end lines")
    rel.append("[A] 1")
    rel.append("  namespace NS_rec_t.name_ptr (record type NS_rec_t; tables NS_table)")
    rel.extend("      " + v for v in W["ns"].values() if v)
    rel.append(W["row_head"])
    rel.extend("      " + v for v in W["rel"].values() if v)
    rel.extend("      " + v for v in W["rel_extra"])
    with open(p("relations.txt"), "w") as f:
        f.write("\n".join(rel) + "\n")
    with open(p("keys.txt"), "w") as f:
        f.write("facts %s\n%s 00000000000000a1 app/cons.c:50:9 synthetic\n" % (F, W.get("rid", "A1")))
    with open(p("facts_view.txt"), "w") as f:
        f.write("facts /x/facts.kz sha256=%s\ntool_sha256=%s\n" % (F, T))
    with open(p("basis.kz"), "w") as f:
        f.write("synthetic basis\n")
    pl = ["kz-order-pairs 1", "facts " + W["pairs_facts"], "tool kz-static tool_sha256=%s tool_source=x" % W["pairs_tool"],
          "relations synthetic", "order order rules revision 8 (synthetic)", "domain synthetic",
          "columns row_key\tpath\tuse_pos\tuse_function\tuse_root\tother_kind\tother_pos\tother_function\tother_root\tvalue\tedges\tpremises\trow_line"]
    vals = {}
    for x in W["pairs"]:
        pl.append("\t".join(x))
        vals[x[9].split("(")[0]] = vals.get(x[9].split("(")[0], 0) + 1
    tail = W["pairs_tail"] or ("rows %d pairs %d hard_before %d conditional_before 0 fixed_early_proved 0 fixed_early_candidate 0 unordered %d unknown 0 same_thread %d no_pair 0"
                               % (len({x[0] for x in W["pairs"]}), len(W["pairs"]), vals.get("hard_before", 0), vals.get("unordered", 0), vals.get("same_thread", 0)))
    pl += [tail, "end"]
    with open(p("pairs.tsv"), "w") as f:
        f.write("\n".join(pl) + "\n")
    cl = ["kz-candidates 1", "rule judgement rules 8.13 item 3 (synthetic)", "plan %s %s" % (sha(p("plan.side")), p("plan.side")),
          "columns layer\tfunction\tcolumn\tdata_file\tdata_sha256\tdata_line\tnamed_by\tkind"]
    cl += ["\t".join(c) for c in W["cands"]]
    kinds = {}
    for c in W["cands"]:
        kinds[c[7]] = kinds.get(c[7], 0) + 1
    cl += ["lines %d kinds %d: %s" % (len(W["cands"]), len(kinds), "; ".join("%s %d" % x for x in sorted(kinds.items()))), "end"]
    with open(p("cand.txt"), "w") as f:
        f.write("\n".join(cl) + "\n")
    if W["named"] is not None:
        no = W["named_over"]
        rows = ["%s\t%s\t%s\t%s\t%s" % ((fn,) + tuple(v)) for fn, v in sorted(W["named"].items()) if fn != no.get("drop")]
        nl = [no.get("head", "kz-named-calls 1"), "rule judgement rules 8.14 item 5 (synthetic)",
              "plan %s %s" % (no.get("plan") or sha(p("plan.side")), p("plan.side")), "facts %s /x/facts.kz" % F,
              "columns function\tnamed_by\trecords\tunverified\tlayers"] + rows
        nl += [no.get("tail") or "lines %d no_entry %d" % (len(rows), sum(1 for r in rows if r.split("\t")[2] == "0")), "end"]
        with open(p("named.tsv"), "w") as f:
            f.write("\n".join(nl) + "\n")
    adm = []
    for i, (data, ks, ov) in enumerate(W["admitted"]):
        lines = [c for c in W["cands"] if ks is None or c[7] in ks]
        a = ["kz-admitted 1", "candidates %s" % (ov.get("candidates") or sha(p("cand.txt"))), "branch %s" % data]
        a += ["ground %s: none: evaluation branch written before the PI's answer" % k for k in sorted({c[7] for c in lines})]
        a.append("columns layer\tfunction\tcolumn\tdata_file\tdata_sha256\tdata_line\tnamed_by\tkind")
        body = ["\t".join(c) for c in lines] + ov.get("extra", [])
        if ov.get("drop_one"):
            body = body[1:]
        a += body + ["lines %d" % len(body), "end"]
        with open(p("adm%d.txt" % i), "w") as f:
            f.write("\n".join(a) + "\n")
        adm.append(p("adm%d.txt" % i))
    bargs = ["--out", p("branches.txt")] + (["--candidates", p("cand.txt")] if W["give_cands"] else [])
    for a in adm:
        bargs += ["--admitted", a]
    rc = subprocess.call([PY, "-I", os.path.join(OBS, "explain_branches.py")] + bargs, stdout=subprocess.DEVNULL)
    assert rc == 0, "branches"
    if W["readings"] is not None:
        with open(p("branches.txt")) as f:
            bl = f.read().splitlines()
        keep = [x for x in bl if not x.startswith("branch ") or any((" readings=%s " % r) in x for r in W["readings"])]
        n = sum(1 for x in keep if x.startswith("branch "))
        keep = [x if not x.startswith("lines ") else "lines %d" % n for x in keep]
        with open(p("branches.txt"), "w") as f:
            f.write("\n".join(keep) + "\n")
    if W["run"]:
        msha = sha(p("plan.kzs"))
        dl = ["run abi=1 mode=stopped run_id=1 instance=1 clock=1 runtime=\"kzrt 0.5.0\"", "run manifest_sha256=%s" % msha,
              "run t_init=0 t_attach=0 t_stop=100000 t_dump=100001", "runx pool_bytes=1 pool_dropped=0 boot_bytes_dropped=0",
              "runx process=100 initial=tid:100 evidence=constructor", "module 1 actor=? t=1 registrations=1 manifest_sha256=%s" % msha]
        dl += W["buildids"]
        aseq = {}
        evl = []
        for a, s, t, v, st, fl in W["events"]:
            aseq[a] = aseq.get(a, 0) + 1
            lab = W["sites"][s][0]
            vv = ",".join("-" if x is None else hx64(x) for x in v)
            evl.append("ev actor=%s aseq=%d site=%d[%s] bind=%d link=0 t=%d pc=0x0 key=%s v=[%s]%s flags=%s" % (
                a, aseq[a], s, lab, (1 << 16) | (s << 4), t, W["keys"][a], vv, (' str="%s"' % st) if st is not None else "", fl or "clock"))
        for a in sorted(aseq):
            dl.append("actor %s key=%s aseq=1..%d recorded=%d overflow=0 partial=0 dropped_bytes=0" % (a, W["keys"][a], aseq[a], aseq[a]))
        dl += evl + W["tok"] + [W["format"], "check ok (0 failures)"]
        with open(p("dump.txt"), "w") as f:
            f.write("\n".join(dl) + "\n")
        with open(p("run.meta"), "w") as f:
            f.write("manifest plan.kzs sha256=%s\nstop=budget\n" % msha)
        with open(p("chain.txt"), "w") as f:
            f.write("\n".join(W["chain"]) + "\n")
        with open(p("startup.scr"), "w") as f:
            f.write("CFE_APP, cons.so, ConsumerMain, CONS, 50, 16384, 0x0, 0;\nCFE_APP, maker.so, MakerMain, MAKR, 50, 16384, 0x0, 0;\n!\n")
    args = ["--sidecar", p("plan.side"), "--manifest", p("plan.kzs"), "--bindings", p("binds.kzs"), "--relations", p("relations.txt"),
            "--keys", p("keys.txt"), "--facts-view", p("facts_view.txt"), "--bprime-basis", p("basis.kz"), "--build-label", "fx",
            "--branches", p("branches.txt")]
    if W["give_pairs"]:
        args += ["--pairs", p("pairs.tsv")]
    if W["give_ns"]:
        args += ["--namespaces", p("ns2.txt")]
    if W["give_cands"]:
        args += ["--candidates", p("cand.txt")]
    if W["give_cands"] and W["named"] is not None:
        args += ["--named-calls", p("named.tsv")]
    if W["run"]:
        args += ["--dump-text", p("dump.txt"), "--run-meta", p("run.meta"), "--chain", p("chain.txt"), "--startup-script", p("startup.scr")]
    return args


N_RUN = [0]


def run(W, tag):
    N_RUN[0] += 1
    d = os.path.join(WORK, "%03d_%s" % (N_RUN[0], tag))
    args = write_world(W, d)
    out = os.path.join(d, "out")
    r = subprocess.run([PY, "-I", os.path.join(OBS, "explain.py")] + args + ["--vocabulary", VOC, "--out", out],
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
    return r.returncode, out, r.stdout + r.stderr


def rec(out, branch="D1-R0"):
    d = os.path.join(out, branch, "records")
    if not os.path.isdir(d):
        return None
    fs = [f for f in sorted(os.listdir(d)) if ".actor-2." in f or ".actor-2.json" in f or f.endswith(".actor-2.json")]
    fs = fs or sorted(os.listdir(d))
    if not fs:
        return None
    with open(os.path.join(d, fs[0])) as f:
        return json.load(f)


def pair0(r):
    ps = r["link"]["name_binding"]["pairs"] if r else []
    return ps[0] if ps else None


def item(r, k):
    p = pair0(r)
    return (p["items"][k]["status"], p["items"][k]["reason"]) if p else (None, None)


def full():
    W = world()
    W["admitted"] = [("D2", None, {})]
    return W


# ------------------------------------------------------------------------------------------------ cases
def main():
    if os.path.exists(WORK):
        shutil.rmtree(WORK)
    os.makedirs(WORK)
    # 1. the base world: every item held in D2, the pair bound, the record established by name binding, observed in D2-R0
    rc, out, log = run(full(), "base")
    ok("base rc 0", rc == 0, log[-400:])
    r2 = rec(out, "D2-R0")
    p = pair0(r2)
    ok("base D2: twelve items held", p is not None and p["missing_items"] == [], p and p["missing_items"])
    ok("base D2: pair bound", p is not None and p["result"] == "bound")
    ok("base D2: link established, basis name_binding", r2 and r2["link"]["status"] == "established" and r2["link"].get("basis") == "name_binding")
    ok("base D2: use_cond derived", r2 and r2["use_cond"]["status"] == "derived", r2 and r2["use_cond"]["missing"])
    ok("base D2: use name state unmet", r2 and r2["order"]["use_events"][0]["name_state"] == "unmet")
    ok("base D2: order unmet_state_observed, violation observed", r2 and r2["order"]["status"] == "unmet_state_observed"
       and r2["tool_verdict"]["violation"]["value"] == "observed")
    ok("base D2: order_ground is the pair-file line (unordered)", r2 and isinstance(r2["order_ground"][0], dict) and r2["order_ground"][0]["value"] == "unordered")
    ok("base D2: pair_kind app-app", r2 and set(r2["pair_kind"].values()) == {"app-app"})
    ok("base D2: context 1 assigned to the use", r2 and r2["order"]["use_events"][0]["context"] == "1")
    ok("base: uses.tsv written before aggregation", os.path.isfile(os.path.join(out, "D2-R0", "uses.tsv")))
    r1 = rec(out, "D1-R0")
    p1 = pair0(r1)
    ok("base D1: (na) semantics missing (unverified entries)", p1 and p1["missing_items"] == ["na_semantics"], p1 and p1["missing_items"])
    ok("base D1: pair unlinked, record unlinked", p1 and p1["result"] == "unlinked" and r1["link"]["status"] == "unlinked")
    ok("base D1: (iii) not established (rests on an unverified entry)", r1 and r1["use_cond"]["branch_iii"][0]["status"] == "not_established")
    ok("base D1: use_cond partial, violation undetermined", r1 and r1["use_cond"]["status"] == "partial" and r1["tool_verdict"]["violation"]["value"] == "undetermined")
    ok("base D1: eval_branch and sha256 fields", r1 and r1["eval_branch"]["id"] == "D1-R0" and r1["data_branch"]["admitted_sha256"] == "none"
       and len(r1["data_branch"]["candidates_sha256"]) == 64)
    rra = rec(out, "D2-Ra")
    ok("reading branch tag", rra and rra["eval_branch"]["tag"] == "4.2 의 읽기 갈래(PI 결정 전)")
    # 2. the judge opens only the normal form and the vocabulary
    def guard_code(allowed, outdir):
        return ("import builtins,sys,os\nsys.argv=['j','--normal',%r,'--vocabulary',%r,'--out',%r]\nreal=builtins.open\nallowed=set(%r)\n"
                "import importlib.util\nspec=importlib.util.spec_from_file_location('j',%r)\nm=importlib.util.module_from_spec(spec)\nspec.loader.exec_module(m)\n"
                "def guard(f,*a,**k):\n    p=os.path.abspath(f) if isinstance(f,str) else f\n    if isinstance(p,str) and 'r' in (a[0] if a else k.get('mode','r')) and p not in allowed and not p.startswith(%r):\n        raise SystemExit('opened '+p)\n    return real(f,*a,**k)\n"
                "builtins.open=guard\nsys.exit(m.main(sys.argv[1:]))\n") % (
            os.path.join(out, "normal.json"), VOC, outdir, sorted(allowed), os.path.join(OBS, "explain_judge.py"), outdir)
    jself = os.path.abspath(os.path.join(OBS, "explain_judge.py"))
    rj = subprocess.run([PY, "-I", "-c", guard_code([os.path.abspath(os.path.join(out, "normal.json")), os.path.abspath(VOC), jself],
                                                    os.path.join(WORK, "judge_only"))], stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
    ok("judge opens only the normal form and the vocabulary", rj.returncode == 0 and "opened" not in rj.stderr, rj.stderr[-300:])
    rj2 = subprocess.run([PY, "-I", "-c", guard_code([os.path.abspath(os.path.join(out, "normal.json")), jself], os.path.join(WORK, "judge_only2"))],
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
    ok("the open guard fails a judge that opens another file", rj2.returncode != 0 and "opened" in (rj2.stderr + rj2.stdout), rj2.stderr[-200:])

    def case(tag, mod, checks, branch="D2-R0", admitted=True):
        W = full() if admitted else world()
        mod(W)
        rc, out, log = run(W, tag)
        r = rec(out, branch)
        for name, f in checks:
            try:
                c = f(r, rc, out)
            except Exception as e:  # a missing field is a failure of the check
                c = False
                log = "%s %s" % (e, log[-300:])
            ok("%s: %s" % (tag, name), c, log[-300:] if not c else "")
        return r, out

    # 3. each item missing alone (D2, all admitted)
    case("basis1", lambda W: W.__setitem__("chain", [W["chain"][0], W["chain"][1], "same_build_object\tunconfirmed(no build-id in dump)\tmodule:1 path=- tree=not read dump=none"]),
         [("missing alone", lambda r, rc, o: pair0(r)["missing_items"] == ["basis1_same_build"]),
          ("sub-reason same build not confirmed", lambda r, rc, o: item(r, "basis1_same_build")[1] == "record_insufficient(same build not confirmed)")])
    case("basis1-refused", lambda W: W.__setitem__("chain", [W["chain"][0], W["chain"][1], "same_build_object\trefused\tmodule:1 path=core tree=12 dump=11"]),
         [("build_mismatch", lambda r, rc, o: item(r, "basis1_same_build")[1] == "build_mismatch")])
    case("basis1-dump14", lambda W: (W.__setitem__("buildids", []), W.__setitem__("format", "format kzrt-dump 1.4 records=1 unknown=0 partial=0 pads=0 end=yes"),
                                     W.__setitem__("chain", [W["chain"][0], "same_build\tunconfirmed(no build-id in dump)\t0 objects"])),
         [("no buildid line: basis 1 missing for the pair", lambda r, rc, o: pair0(r)["missing_items"] == ["basis1_same_build"])])

    def b2(W):
        W["events"][4] = ("2", 5, 200, [0xC], None, "")
    case("basis2", b2, [("missing alone", lambda r, rc, o: pair0(r)["missing_items"] == ["basis2_ties"]),
                        ("consumer creation tie reason", lambda r, rc, o: item(r, "basis2_ties")[1].startswith("link_unlinked(consumer creation tie"))])
    case("basis3-noorigins", lambda W: W["rel"].__setitem__("origins", None),
         [("missing alone", lambda r, rc, o: pair0(r)["missing_items"] == ["basis3_absence_constant"])])

    def b3null(W):
        W["events"][6] = ("2", 10, 1000, [0], None, "")
    case("basis3-nullkey", b3null, [("null key pointer: an origin not excluded", lambda r, rc, o: "basis3_absence_constant" in pair0(r)["missing_items"])])

    def b3other(W):
        W["rel"]["origins"] = W["rel"]["origins"].replace("when (not param:name#0) [other]", "when (eq field:G.x 3) [other]")
    case("basis3-otherorigin", b3other, [("an origin of another condition is not excluded", lambda r, rc, o: pair0(r)["missing_items"] == ["basis3_absence_constant"])])
    case("basis4-stored", lambda W: W["rel"].__setitem__("success", W["rel"]["success"].replace(": held:", ": missing(the stored value is not tied to the success path):")),
         [("missing alone", lambda r, rc, o: pair0(r)["missing_items"] == ["basis4_creator_success"]),
          ("sub-reason stored value", lambda r, rc, o: "stored value is not tied" in item(r, "basis4_creator_success")[1])])
    case("basis4-returns", lambda W: W["rel"].__setitem__("success", W["rel"]["success"].replace(": held:", ": missing(the returns are not all followed):")),
         [("sub-reason returns not followed", lambda r, rc, o: "returns are not all followed" in item(r, "basis4_creator_success")[1])])

    def b4st(W):
        W["events"][13] = ("3", 21, 2100, [-15, 0x10001], None, "")
    case("basis4-status", b4st, [("a failed create: out not read", lambda r, rc, o: "out id is not read" in item(r, "basis4_creator_success")[1])])

    def b5(W):
        W["events"][12] = ("3", 22, 2050, [0x2000, 4], "W1", "clock,str-trunc")
    case("basis5", b5, [("cut key text: basis 5 missing", lambda r, rc, o: "basis5_key_texts" in pair0(r)["missing_items"])])

    def b6over(W):
        W["events"][11] = ("3", 20, 1080, [0x2000], None, "")
        W["events"][12] = ("3", 22, 1090, [0x2000, 4], "W1", "")
        W["events"][13] = ("3", 21, 1150, [0, 0x10001], None, "")
    case("basis6-overlap", b6over, [("missing alone", lambda r, rc, o: pair0(r)["missing_items"] == ["basis6_create_after_use"]),
                                    ("name state undecided (overlap)", lambda r, rc, o: r["order"]["use_events"][0]["name_state"].startswith("undecided"))])

    def b6before(W):
        W["events"][11] = ("3", 20, 500, [0x2000], None, "")
        W["events"][12] = ("3", 22, 550, [0x2000, 4], "W1", "")
        W["events"][13] = ("3", 21, 600, [0, 0x10001], None, "")
    case("contradiction", b6before, [("basis 6 contradicted", lambda r, rc, o: item(r, "basis6_create_after_use")[0] == "contradicted"),
                                     ("absence after a live create", lambda r, rc, o: "absence after a live create" in item(r, "basis6_create_after_use")[1]),
                                     ("name state met", lambda r, rc, o: r["order"]["use_events"][0]["name_state"] == "met")])
    case("ga-noentry", lambda W: W.__setitem__("entry_on", False),
         [("(ga) missing", lambda r, rc, o: "ga_namespace_entry" in pair0(r)["missing_items"])])
    case("ga-nodata", lambda W: W.__setitem__("give_ns", False),
         [("missing alone", lambda r, rc, o: pair0(r)["missing_items"] == ["ga_namespace_entry"])])
    case("na-sem-nocall", lambda W: W["entry"].__setitem__("compare-call", "-"),
         [("missing alone", lambda r, rc, o: pair0(r)["missing_items"] == ["na_semantics"])])
    case("na-bound", lambda W: W["entry"].__setitem__("extent-value", "2"),
         [("missing alone", lambda r, rc, o: pair0(r)["missing_items"] == ["na_bound"])])
    case("da-buffer", lambda W: W["ns"].__setitem__("buf", W["ns"]["buf"].replace("; complete", "; not complete (FIELD_STATUS disabled; every cause discharged)")),
         [("missing alone in R0", lambda r, rc, o: pair0(r)["missing_items"] == ["da_no_name_mapping"])])
    case("da-buffer-Rf", lambda W: W["ns"].__setitem__("buf", W["ns"]["buf"].replace("; complete", "; not complete (FIELD_STATUS disabled; every cause discharged)")),
         [("held under reading Rf", lambda r, rc, o: pair0(r)["missing_items"] == [])], branch="D2-Rf")
    case("da-unassigned", lambda W: W["ns"].__setitem__("wx", "writers [no key-parameter function reaches Other_Bind] of NS_rec_t.name_ptr: 1: assign at lib/x.c:9:9 in Other_Bind value=symbolic [y->name]"),
         [("missing alone", lambda r, rc, o: pair0(r)["missing_items"] == ["da_no_name_mapping"])])
    case("da-fieldstatus", lambda W: W["rel"].__setitem__("keywriters", W["rel"]["keywriters"].replace("type-level writer-set-complete", "type-level DISABLED")),
         [("missing alone", lambda r, rc, o: pair0(r)["missing_items"] == ["da_no_name_mapping"])])

    def ra(W):
        W["events"][12] = ("3", 22, 2050, [0x2000, 5], "W1", "")
    case("ra-kind", ra, [("missing alone", lambda r, rc, o: pair0(r)["missing_items"] == ["ra_same_kind"])])
    case("ra-notplanned", lambda W: W.__setitem__("needs", [("kind-value", "not-planned", "")] + W["needs"][1:]),
         [("missing alone", lambda r, rc, o: pair0(r)["missing_items"] == ["ra_same_kind"])])
    case("ma-incomplete", lambda W: W.__setitem__("lc", "lifetime-complete kind=KIND_WIDGET no(rollback)"),
         [("missing alone", lambda r, rc, o: pair0(r)["missing_items"] == ["ma_no_lifetime_end"]),
          ("name state undecided", lambda r, rc, o: r["order"]["use_events"][0]["name_state"].startswith("undecided(lifetime"))])
    case("ma-unbound", lambda W: W["unbound"].add(32), [("missing alone", lambda r, rc, o: pair0(r)["missing_items"] == ["ma_no_lifetime_end"])])

    def ma_clear(W):
        W["events"].append(("4", 33, 1500, [0x9000], None, ""))
    case("ma-cleartable", ma_clear, [("(ma) seen: the table was cleared between", lambda r, rc, o: item(r, "ma_no_lifetime_end")[1].endswith("deleted or retired between the use and the create)"))])

    def ma_rollback(W):
        W["events"].extend([("4", 20, 1300, [0x3000], None, ""), ("4", 22, 1320, [0x3000, 4], "W1", ""), ("4", 32, 1340, [5], None, ""),
                            ("4", 21, 1360, [-5, 0], None, "")])
    case("ma-rollback", ma_rollback, [("(ma) seen: a rollback of the name between", lambda r, rc, o: any(
        p["items"]["ma_no_lifetime_end"]["reason"] and "deleted or retired" in p["items"]["ma_no_lifetime_end"]["reason"] for p in r["link"]["name_binding"]["pairs"]))])

    def ma_reentry(W):
        W["lifetimes"][1] = W["lifetimes"][1].replace("reentry=none", "reentry=possible")
        W["events"].extend([("4", 20, 1300, [0x3000], None, ""), ("4", 22, 1320, [0x3000, 4], "W9", ""), ("4", 32, 1340, [5], None, ""),
                            ("4", 21, 1360, [-5, 0], None, "")])
    case("ma-reentry", ma_reentry, [("reentry=possible: a rollback of another name counts for every name of the kind", lambda r, rc, o: any(
        p["create_actor"] == "3" and "deleted or retired" in (p["items"]["ma_no_lifetime_end"]["reason"] or "") for p in r["link"]["name_binding"]["pairs"]))])

    def ma_slot(W):
        W["sites"][34] = ("s0034.object-clear", "op=store.Lib_FindFree phase=effect tu=lib/ns.c func=Lib_FindFree line=70 place=store", [])
        W["map"].append(34)
        W["lifetimes"].append("lifetime KIND_WIDGET clear-slot Lib_FindFree lib/ns.c:66 data NSPATH line 15: applies; store at lib/ns.c:70 sites=34")
        W["events"].append(("4", 34, 1500, [], None, ""))
    case("ma-slotclear", ma_slot, [("a slot clear not shown: (ma) not excluded", lambda r, rc, o: item(r, "ma_no_lifetime_end")[1].endswith("is not excluded)"))])
    # 4. pair results and record results
    def refuted(W):
        W["events"][12] = ("3", 22, 2050, [0x2000, 4], "W2", "")
        W["rel"]["creator"] = W["rel"]["creator"].replace('"W1"', '"W2"')
    case("refuted", refuted, [("pair refuted", lambda r, rc, o: pair0(r)["result"] == "refuted"),
                              ("record refuted, violation refuted_link", lambda r, rc, o: r["link"]["status"] == "refuted" and r["tool_verdict"]["violation"]["value"] == "refuted_link"),
                              ("refuted_scope", lambda r, rc, o: bool(r["link"].get("refuted_scope")))])
    case("differs", refuted, [("D1: differs_from_use_key, not refuted", lambda r, rc, o: pair0(r)["result"] == "differs_from_use_key" and r["link"]["status"] == "unlinked")],
         branch="D1-R0")

    def refuted_some(W):
        refuted(W)
        W["rel"]["creator2"] = 'creator: key known: app/maker.c:30:5 in maker_init -> Lib_Create'
        W["events"].extend([("4", 20, 3000, [0x3000], None, ""), ("4", 22, 3050, [0x3000, 4], "W3", "clock,str-trunc"), ("4", 21, 3100, [0, 0x10003], None, "")])
    case("refuted-some", refuted_some, [("record unlinked: refuted for some creates only", lambda r, rc, o: r["link"]["status"] == "unlinked")])

    def refuted_inc(W):
        refuted(W)
        W["rel"]["inc"] = "incomplete: writer_set_incomplete(FIELD_STATUS not complete)"
    case("refuted-incomplete", refuted_inc, [("record not refuted when the creator set is incomplete", lambda r, rc, o: r["link"]["status"] == "unlinked" and pair0(r)["result"] == "refuted")])

    def two(W):
        W["events"].extend([("1", 3, 140, [0xD], None, ""), ("1", 4, 150, [0], None, ""), ("4", 6, 400, [0xD], None, "")])
        W["events"].extend([("4", 20, 3000, [0x3000], None, ""), ("4", 22, 3050, [0x3000, 4], "W1", ""), ("4", 21, 3100, [0, 0x10003], None, "")])
        W["chain"].append("same_build_object\tconfirmed\tmodule:1 path=core tree=11 dump=11")
    case("two-bound", two, [("more than one create bound: both unlinked", lambda r, rc, o: all(p["result"] == "unlinked" for p in r["link"]["name_binding"]["pairs"])
                                  and r["link"]["status"] == "unlinked")])

    def found(W):
        b6before(W)
        W["events"][8] = ("2", 11, 1100, [0, 0x10001], None, "")
        W["rel"]["flip"] = W["rel"]["flip"]
    FX["found"], _ = case("found-linked", found, [("a found use is instance-linked on an open generation", lambda r, rc, o: r["order"]["use_events"][0]["instance_link"]["status"] == "linked"),
                                 ("record established by instance", lambda r, rc, o: r["link"]["status"] == "established" and r["link"]["basis"] == "instance"),
                                 ("name state met on the open generation", lambda r, rc, o: r["order"]["use_events"][0]["name_state"] == "met")])

    def found_deleted(W):
        found(W)
        W["events"].extend([("4", 30, 800, [0x10001], None, ""), ("4", 31, 850, [0], None, "")])
        W["sites"][31] = (W["sites"][31][0], W["sites"][31][1], W["sites"][31][2])
    case("found-deleted", found_deleted, [("the generation was deleted before the use: unlinked", lambda r, rc, o: r["order"]["use_events"][0]["instance_link"]["status"] == "unlinked")])
    case("found-incomplete", lambda W: (found(W), W.__setitem__("lc", "lifetime-complete kind=KIND_WIDGET no(delete)")),
         [("lifetime ends incomplete: plan_missing", lambda r, rc, o: r["order"]["use_events"][0]["instance_link"]["reason"] == "plan_missing(need not planned)")])
    # 5. pair kinds
    def process_app(W):
        W["events"] = W["events"][:10] + [("1", 20, 2000, [0x2000], None, ""), ("1", 22, 2050, [0x2000, 4], "W1", ""), ("1", 21, 2100, [0, 0x10001], None, "")]
        W["pairs"][0] = W["pairs"][0][:8] + ("main (process)",) + W["pairs"][0][9:]
    case("process-app", process_app, [("pair kind process-app", lambda r, rc, o: r["pair_kind"].get("1") == "process-app"),
                                      ("basis 2 missing: the process initial thread has no creation tie (rule as written)",
                                       lambda r, rc, o: pair0(r)["missing_items"] == ["basis2_ties"] and r["order_ground"][0].startswith("order_basis_unknown"))])

    def same_actor(W):
        W["rel"]["creator"] = None
        W["rel"]["nap"] = 'not a pair: probe-create: app/maker.c:30:5 in maker_init -> Lib_Create (same function)'
        W["events"] = W["events"][:10] + [("2", 20, 2000, [0x2000], None, ""), ("2", 22, 2050, [0x2000, 4], "W1", ""), ("2", 21, 2100, [0, 0x10001], None, "")]
        W["pairs"][0] = W["pairs"][0][:5] + ("creator-not-a-pair(probe-create)",) + W["pairs"][0][6:8] + ("ConsumerMain (script-entry)", "same_thread", "-", "-", "not a pair")
    case("same-actor", same_actor, [("the probe-create of the consumer's own actor is a pair of kind same_actor", lambda r, rc, o: pair0(r)["pair_kind"] == "same_actor"),
                                    ("same_thread pair line read", lambda r, rc, o: pair0(r)["result"] == "bound" and r["order_ground"][0]["value"] == "same_thread")])

    def core_app(W):
        W["sites"][6] = ("s0006.actor-root+actor-start", 'op=entry.MakerMain phase=before tu=app/maker.c func=MakerMain ref="actor MakerMain (table-image-entry)"', [("param0", "param0", "u64")])
    case("core-app", core_app, [("pair kind core-app", lambda r, rc, o: r["pair_kind"].get("3") == "core-app")])

    def same_app(W):
        W["sites"][6] = ("s0006.actor-root+actor-start", 'op=entry.ConsumerMain phase=before tu=app/cons.c func=ConsumerMain ref="actor ConsumerMain (script-entry)"', [("param0", "param0", "u64")])
    case("same-app-other", same_app, [("pair kind same-app-other", lambda r, rc, o: r["pair_kind"].get("3") == "same-app-other")])
    # 6. order from the pair file only
    case("pairs-otherfacts", lambda W: W.__setitem__("pairs_facts", "e" * 64),
         [("pair file of other facts refused: order_basis_unknown", lambda r, rc, o: isinstance(r["order_ground"][0], str) and "refused" in r["order_ground"][0])])
    case("pairs-othertool", lambda W: W.__setitem__("pairs_tool", "d" * 64),
         [("pair file of another tool refused", lambda r, rc, o: "refused" in r["order_ground"][0])])
    case("pairs-absent", lambda W: W.__setitem__("pairs", [W["pairs"][0][:6] + ("/abs/app/other.c:1:1",) + W["pairs"][0][7:]]),
         [("no line of the pair: order_basis_unknown, no fallback", lambda r, rc, o: r["order_ground"][0] == "order_basis_unknown(no line of the pair in the pair file)"
           and r["order"]["static"] == "unknown")])
    case("pairs-badtail", lambda W: W.__setitem__("pairs_tail", "rows 1 pairs 9 unordered 9 no_pair 0"),
         [("a pair file whose tail does not count its lines is refused", lambda r, rc, o: "refused" in r["order_ground"][0])])
    case("hard-before-unmet", lambda W: W.__setitem__("pairs", [W["pairs"][0][:9] + ("hard_before", "E1", "-", "creator")]),
         [("hard_before with an unmet use flagged", lambda r, rc, o: r["order"].get("hard_before_unmet") == [0])])
    # 7. contexts and (c3)
    def ctx2(W):
        W["events"][5] = ("2", 42, 900, [], None, "")
        W["events"][9] = ("2", 43, 1200, [], None, "")
    case("context2", ctx2, [("use in context 2", lambda r, rc, o: r["order"]["use_events"][0]["context"] == "2"),
                            ("(iii) not held: found retry", lambda r, rc, o: r["use_cond"]["branch_iii"][0]["status"] == "not_held")])

    def noframe(W):
        del W["events"][9]
        del W["events"][5]
    case("context-unknown", noframe, [("no bracketing frame: context unknown", lambda r, rc, o: r["order"]["use_events"][0]["context"] is None),
                                      ("(iii) not established (context unknown)", lambda r, rc, o: any(x["why"] == "context unknown" for x in r["use_cond"]["branch_iii"])),
                                      ("caller_dependence_unknown(context unknown)", lambda r, rc, o: "caller_dependence_unknown(context unknown)" in r["use_cond"]["missing"])])
    hold_clear = "holder: field Cons_t.obj_id; writers 2: clearing app/cons.c:40:5 in cons_init value=0 [fill]; own out store app/cons.c:50:9; FIELD_STATUS per-field=writer-set-complete type-level=writer-set-complete; readers 1: cons_run at app/cons.c:80:5: tests the cleared value before the dependent use at app/cons.c:81:9 (tests 1, cleared 0)"
    case("i-clearing", lambda W: W["rel"].__setitem__("holder", hold_clear), [("(i) not held (a clearing definition)", lambda r, rc, o: r["use_cond"]["branch_i"]["status"] == "not_held")])
    case("i-clearing-Rb", lambda W: W["rel"].__setitem__("holder", hold_clear), [("(i) held under reading Rb", lambda r, rc, o: r["use_cond"]["branch_i"]["status"] == "held")], branch="D2-Rb")
    case("i-other", lambda W: W["rel"].__setitem__("holder", W["rel"]["holder"].replace("writers 1: own out store app/cons.c:50:9", "writers 2: own out store app/cons.c:50:9; other definition app/cons.c:60:5 in cons_x value=symbolic [assign]")),
         [("(i) not held (another definition)", lambda r, rc, o: r["use_cond"]["branch_i"]["status"] == "not_held")])
    case("i-incomplete", lambda W: W["rel"].__setitem__("holder", W["rel"]["holder"].replace("type-level=writer-set-complete", "type-level=DISABLED")),
         [("(i) not established (type-level DISABLED)", lambda r, rc, o: r["use_cond"]["branch_i"]["status"] == "not_established")])
    case("i-incomplete-Re", lambda W: W["rel"].__setitem__("holder", W["rel"]["holder"].replace("type-level=writer-set-complete", "type-level=DISABLED")),
         [("(i) held under reading Re, marked with its premise", lambda r, rc, o: r["use_cond"]["branch_i"]["status"] == "held" and r["use_cond"]["branch_i"]["reading"] == "Re")], branch="D2-Re")
    case("i-local", lambda W: W["rel"].__setitem__("holder", "holder: local id in cons_init; writers 1: own out store app/cons.c:50:9; local: definitions 1 (RESULT_FLOW); readers: not decided: the uses of a local are not READS rows"),
         [("(i) not established for a local holder", lambda r, rc, o: r["use_cond"]["branch_i"]["status"] == "not_established")])
    case("iii-premise", lambda W: W["rel"].__setitem__("alt1", W["rel"]["alt1"].replace("premises: none", "premises: (6) the writer set of G_t.run is complete under both rules")),
         [("(iii) not established on a premise", lambda r, rc, o: r["use_cond"]["branch_iii"][0]["status"] == "not_established")])
    case("iii-decided", lambda W: W["rel"].__setitem__("alt1", W["rel"]["alt1"].replace("decided calls: none", "decided calls: app/cons.c:91:5 result 0 by the stored-field rule (store app/cons.c:88:5 := 1)")),
         [("(iii) held with the stored-field rule marked", lambda r, rc, o: r["use_cond"]["branch_iii"][0]["status"] == "held" and r["use_cond"]["branch_iii"][0]["decided_by"] == "stored-field rule"),
          ("counted apart in use_cond.stored_field_rule", lambda r, rc, o: len(r["use_cond"]["stored_field_rule"]) == 1)])
    case("iii-Re-premise", lambda W: W["rel"].__setitem__("alt1", "alternatives [context 1: ConsumerMain@app/cons.c:90:5 <- cons_init@app/cons.c:50:9]: not established (open: exit at app/cons.c:95:5 [the stored-field rule at K 91:5: not decided: (5) or (6) rests on a premise]); rests on: none; premises: (6) the writer set of G_t.run is complete under both rules (FIELD_STATUS per-field=writer-set-complete type-level=DISABLED); decided calls: none"),
         [("Re cannot close (iii): no call result under the premise", lambda r, rc, o: "no call result under the premise" in r["use_cond"]["branch_iii"][0]["why"])], branch="D2-Re")
    case("Ra", lambda W: W["rel"].__setitem__("holder", hold_clear), [("(iii) alone derives under reading Ra", lambda r, rc, o: r["use_cond"]["status"] == "derived")], branch="D2-Ra")
    case("Ra-R0", lambda W: W["rel"].__setitem__("holder", hold_clear), [("R0 needs (i): partial", lambda r, rc, o: r["use_cond"]["status"] == "partial")])
    rcflip = "flip: static-undetermined: the key of the lookup or of a creator is not resolved statically; search returns: Lib_Search returns -17 [ERR_NOT_FOUND], Lib_Search returns 0 [OK]"
    case("Rc", lambda W: W["rel"].__setitem__("flip", rcflip), [("(c2) present from the absence line under Rc", lambda r, rc, o: r["use_cond"]["reach"]["status"] == "present")], branch="D2-Rc")
    case("Rc-R0", lambda W: W["rel"].__setitem__("flip", rcflip), [("(c2) missing in R0", lambda r, rc, o: r["use_cond"]["reach"]["status"] == "missing")])
    case("Rd", lambda W: W["rel"].__setitem__("inc", "incomplete: key_unresolved(creator key)"),
         [("(c4) present under Rd with its premise", lambda r, rc, o: r["use_cond"]["writers"]["status"] == "present" and r["use_cond"]["writers"]["reading"] == "Rd")], branch="D2-Rd")
    case("Rd-R0", lambda W: W["rel"].__setitem__("inc", "incomplete: key_unresolved(creator key)"),
         [("(c4) missing in R0", lambda r, rc, o: r["use_cond"]["writers"]["status"] == "missing")])
    # 8. data branches and admission files
    def partial(W):
        W["admitted"] = [("D2[byte-string comparison]", ["byte-string comparison"], {})]
    r_, o_ = case("partial", partial, [("(na) semantics still missing with the comparison kind only", lambda r, rc, o: "na_semantics" in pair0(r)["missing_items"])],
                  branch="D2_byte-string_comparison-R0", admitted=True)
    for tag, ov in (("adm-notincand", {"extra": ["P0\tother\tsource\tp0\t-\t9\tcopy-call\tdata function"]}), ("adm-partkind", {"drop_one": True}),
                    ("adm-candsha", {"candidates": "0" * 64})):
        W = full()
        W["admitted"] = [("D2", None, ov)]
        rc, out, log = run(W, tag)
        ok("%s: the run stops with rc 2 and writes no record" % tag, rc == 2 and not os.path.isdir(os.path.join(out, "D1-R0")), log[-200:])
    W = world()
    W["give_cands"] = False
    rc, out, log = run(W, "nocands")
    r = rec(out, "D1-R0")
    ok("no candidate list: only D1 branches, (na) semantics missing", rc == 0 and not os.path.isdir(os.path.join(out, "D2-R0")) and "na_semantics" in pair0(r)["missing_items"])
    # 9. tok lines, no run, a path B row (a shape that is no design row)
    def tok(W):
        W["tok"] = ["tok-create actor=1 aseq=9 token=7 site=3 t=5", "tok-return actor=1 aseq=10 token=7 status=0 handle=0x1 t=6", "tok-start actor=2 aseq=9 token=7 t=7"]
    case("tok", tok, [("tok lines counted only", lambda r, rc, o: r["events"]["tok_counts"] == {"tok-create": 1, "tok-return": 1, "tok-start": 1})])
    case("norun", lambda W: W.__setitem__("run", False), [("no run: static_only, rc 0", lambda r, rc, o: rc == 0 and r["order"]["status"] == "static_only")])

    def pathb(W):
        W["row_head"] = "  B1 G_t.flag -> Lib_Check#0 reader rows"
        W["rid"] = "B1"
        W["rel"] = {"actors": "actors: readers {ConsumerMain (script-entry)}", "field": "field: INIT 0",
                    "cc": "callee-check: Lib_Check returns -1 [ERR] when (eq param:x#0 0)", "w": "assign at app/maker.c:30:5 in maker_init value=1",
                    "feas": "feasibility: possible: the facts give an order", "flip": "flip: none"}
        W["rel_extra"] = []
        W["contexts"] = []      # (2026-11-19) the path A row's context lines are not of a path B row
        W["entry_on"] = False
    r, out = case("pathB", pathb, [("a path B row makes a record with no name-binding pair", lambda r, rc, o: rc == 0 and r["path"] == "B"
                                    and r["link"]["name_binding"]["pairs"] == [] and r["use_cond"]["branch_i"]["status"] == "not_established")])

    def second_use(W, status, out, text="W1"):
        W["events"].extend([("2", 10, 3000, [0x1000], None, ""), ("2", 12, 3050, [0x1000, 4], text, ""), ("2", 11, 3100, [status, out], None, "")])
    case("absent-then-found", lambda W: second_use(W, 0, 0x10001),
         [("the uses of the record differ in basis", lambda r, rc, o: r["link"]["status"] == "unlinked"
           and "link_unlinked(name binding: the uses of the record differ in basis)" in r["link"]["missing"])])
    case("two-texts", lambda W: second_use(W, -17, 0, "W7"),
         [("two key texts: multi_state_unlinked", lambda r, rc, o: any(x.startswith("multi_state_unlinked") for x in r["link"]["missing"]))])
    case("refuted-unplanned", lambda W: (refuted(W), W["unbound"].add(21)),
         [("record not refuted when a creator site is not bound", lambda r, rc, o: r["link"]["status"] != "refuted")])
    case("slot-shown", lambda W: W["lifetimes"].append("lifetime KIND_WIDGET clear-slot Lib_FindFree lib/ns.c:66 data NSPATH line 15: not-a-lifetime-end(the relations line shows the free-record condition)"),
         [("a slot clear shown to be no lifetime end plans no site: (ma) held", lambda r, rc, o: pair0(r)["missing_items"] == [])])
    case("note-absent", lambda W: W.__setitem__("chain", [W["chain"][0], W["chain"][1], "same_build_object\tunconfirmed(no build-id note)\tmodule:1 path=core tree=11 dump=none"]),
         [("an object without a build-id note: basis 1 missing", lambda r, rc, o: pair0(r)["missing_items"] == ["basis1_same_build"])])

    # 12. spec 11 (2026-11-19): shapes that are not the design rows (synthetic names only)
    # 12.1 item 1: the context of a row without context lines
    ctx_one = "alternatives [context 1: ConsumerMain@app/cons.c:90:5 <- cons_init@app/cons.c:50:9]: "
    tail_ = "; rests on: none; premises: none; decided calls: none"

    def noctx(body, extra=None):
        def m(W):
            W["contexts"] = []
            W["rel"]["alt1"] = ctx_one + body if body else None
            W["rel"]["alt2"] = extra
        return m
    use0 = lambda r: r["order"]["use_events"][0]
    case("ctx1-found", noctx("found: retry: app/cons.c:52:9" + tail_),
         [("no context line, one alternatives context: the use's context is that id", lambda r, rc, o: use0(r)["context"] == "1"),
          ("(iii) not_held from the found line", lambda r, rc, o: [x["status"] for x in r["use_cond"]["branch_iii"]] == ["not_held"]),
          ("uses.tsv carries the context", lambda r, rc, o: open(os.path.join(o, "D2-R0", "uses.tsv")).read().splitlines()[1].split("\t")[16] == "1")])
    case("ctx1-none", noctx("none found (exploration complete); rests on: P0-unverified(xexit); premises: none; decided calls: none"),
         [("context 1", lambda r, rc, o: use0(r)["context"] == "1"),
          ("(iii) held (D2: the entry it rests on admitted)", lambda r, rc, o: [x["status"] for x in r["use_cond"]["branch_iii"]] == ["held"]),
          ("use_cond derived", lambda r, rc, o: r["use_cond"]["status"] == "derived")])
    case("ctx1-none-D1", noctx("none found (exploration complete); rests on: P0-unverified(xexit); premises: none; decided calls: none"),
         [("D1: (iii) not established on the unverified entry", lambda r, rc, o: use0(r)["context"] == "1" and r["use_cond"]["branch_iii"][0]["status"] == "not_established"
           and r["use_cond"]["branch_iii"][0]["depends_on"] != [])], branch="D1-R0")
    case("ctx1-notest", noctx("not established (open: exit at app/cons.c:95:5: on a path through a branch this value does not decide)" + tail_),
         [("context 1, (iii) not established from the line", lambda r, rc, o: use0(r)["context"] == "1" and r["use_cond"]["branch_iii"][0]["status"] == "not_established"
           and r["use_cond"]["branch_iii"][0]["context"] == "1")])

    def ctx1_noat(W):
        noctx("found: retry: app/cons.c:52:9" + tail_)(W)
        del W["events"][9]
        del W["events"][6]
        del W["events"][5]
    case("ctx1-noat", ctx1_noat, [("no frame event and no at-call: the context is still the one id", lambda r, rc, o: use0(r)["context"] == "1"
                                    and "at_call" not in use0(r))])
    alt_two = "alternatives [context 2: cons_cmd@app/cons.c:120:5 <- cons_init@app/cons.c:50:9]: found: retry: app/cons.c:122:9" + tail_
    case("ctx2-nolines", noctx("found: retry: app/cons.c:52:9" + tail_, alt_two),
         [("no context line, two alternatives contexts: unknown", lambda r, rc, o: use0(r)["context"] is None),
          ("(iii) not established (context unknown)", lambda r, rc, o: any(x["why"] == "context unknown" and x["context"] is None for x in r["use_cond"]["branch_iii"])),
          ("caller_dependence_unknown(context unknown)", lambda r, rc, o: "caller_dependence_unknown(context unknown)" in r["use_cond"]["missing"])])
    case("ctx-none-lines", noctx(None),
         [("no context line and no alternatives line: unknown, no id made up", lambda r, rc, o: use0(r)["context"] is None
           and all(x["context"] is None for x in r["use_cond"]["branch_iii"]))])

    def ctx_dash(W):
        W["contexts"] = []
        W["rel"]["alt1"] = "alternatives [context -]: none found (exploration complete); rests on: P0-unverified(xexit); premises: none; decided calls: none"
        W["rel"]["alt2"] = None
    case("ctx-dash", ctx_dash, [("an alternatives line of context '-': the use's context is '-'", lambda r, rc, o: use0(r)["context"] == "-"
                                 and r["use_cond"]["branch_iii"][0]["context"] == "-" and r["use_cond"]["branch_iii"][0]["status"] == "held")])
    # 12.2 item 2: the named-calls file goes with the candidate list; a wrong one stops the run (rc 2, no record)
    for tag, mod in (("named-head", lambda W: W["named_over"].__setitem__("head", "kz-named-calls 2")),
                     ("named-tail", lambda W: W["named_over"].__setitem__("tail", "lines 9 no_entry 0")),
                     ("named-plan", lambda W: W["named_over"].__setitem__("plan", "0" * 64)),
                     ("named-drop", lambda W: W["named_over"].__setitem__("drop", "memchr")),
                     ("named-absent", lambda W: W.__setitem__("named", None))):
        W = full()
        mod(W)
        rc, out, log = run(W, tag)
        ok("%s: the run stops with rc 2 and writes no record" % tag, rc == 2 and not os.path.isdir(os.path.join(out, "D1-R0")), log[-200:])
    # 12.3 item 3: a named call with no entry, a call with records that are not unverified, an entry not read, address uses
    PART = "D2_byte-string_comparison_data_function-R0"

    def noentry(W):   # spec 11.5 item 2: a synthetic length call with no record, in no candidate line
        W["entry"]["length-call"] = "zlen@lib/ns.c:11"
        W["named"]["zlen"] = ("length-call", "0", "0", "-")
        W["admitted"] = [("D2", None, {}), ("D2[byte-string comparison,data function]", ["byte-string comparison", "data function"], {})]
    NOE = "link_unlinked(name binding: no entry: zlen)"
    for br_ in ("D1-R0", "D2-R0", PART, "D2-all"):
        FX["noentry-" + br_], _ = case("noentry-" + br_, noentry, [("(na) semantics missing, sub-reason no entry", lambda r, rc, o: item(r, "na_semantics") == ("missing", NOE)),
                                        ("the ground names the named-calls line", lambda r, rc, o: any(isinstance(g, dict) and g["file"][0] == "named.tsv"
                                                                                                       for g in pair0(r)["items"]["na_semantics"]["grounds"]))],
             branch=br_)

    def notunver(W):
        W["entry"]["length-call"] = "zlen@lib/ns.c:11"
        W["named"]["zlen"] = ("length-call", "2", "0", "P0")
    case("named-not-unverified", notunver, [("a call with records none of them unverified: (na) held, the pair bound", lambda r, rc, o: pair0(r)["missing_items"] == []
                                              and pair0(r)["result"] == "bound")])
    def noentry_cmp(W):   # a compare call with no record
        W["entry"]["compare-call"] = "zcmp@lib/ns.c:12"
        W["named"]["zcmp"] = ("compare-call", "0", "0", "-")
    for br_ in ("D1-R0", "D2-R0"):
        case("noentry-cmp-" + br_, noentry_cmp, [("a compare call with no entry: (na) semantics missing, sub-reason no entry: zcmp", lambda r, rc, o: item(r, "na_semantics")
                                                   == ("missing", "link_unlinked(name binding: no entry: zcmp)")),
                                                  ("its ground is the named-calls line", lambda r, rc, o: any(isinstance(g, dict) and g["file"][0] == "named.tsv"
                                                                                                         and "zcmp" in g["text"] for g in pair0(r)["items"]["na_semantics"]["grounds"]))],
             branch=br_)

    def noentry_list(W):   # two calls in one field, one of them with no record
        W["entry"]["copy-call"] = "zput@lib/ns.c:60,strncpy@lib/ns.c:60"
        W["named"]["zput"] = ("copy-call", "0", "0", "-")
    case("noentry-list", noentry_list, [("a comma list with one call of no entry: (na) missing in D2, naming only that call", lambda r, rc, o: item(r, "na_semantics")
                                         == ("missing", "link_unlinked(name binding: no entry: zput)") and pair0(r)["missing_items"] == ["na_semantics"])])
    case("entry-not-read", lambda W: W["rel"].__setitem__("alt1", W["rel"]["alt1"].replace("P0-unverified(xexit)", "P0-unverified(yexit)")),
         [("a rests-on entry in neither file: (iii) not established, its state not read", lambda r, rc, o: r["use_cond"]["branch_iii"][0]["status"] == "not_established"
           and "the state of the entry of yexit is not read" in r["use_cond"]["branch_iii"][0]["why"])])
    addr_ne = ("stored-name address uses (the reads of NS_rec_t.name_ptr, every partition): 2: Lib_Info at lib/ns.c:90:5: library: zcopy parameter 1: "
               "no entry says the parameter is not written (arg 0 unverified [arg 0 is written]); Lib_Cmp at lib/ns.c:12:5: test")
    for br_ in ("D2-R0", "D2-Rf"):
        case("addr-noentry-" + br_, lambda W: W["ns"].__setitem__("addr", addr_ne),
             [("an address use with no entry: (da) missing in every branch, sub-reason no entry", lambda r, rc, o: pair0(r)["missing_items"] == ["da_no_name_mapping"]
               and item(r, "da_no_name_mapping")[1] == "link_unlinked(name binding: no entry: zcopy)")], branch=br_)
    addr_uv = ("stored-name address uses (the reads of NS_rec_t.name_ptr, every partition): 1: Lib_Info at lib/ns.c:90:5: library: strncpy parameter 0: "
               "out-behaviour unverified [arg 0 is written]")
    SOB = ("P0", "strncpy", "out-behaviour", "p0", "-", "5", "copy-call", "data function")

    def addr_unver(W):
        W["ns"]["addr"] = addr_uv
        W["cands"].append(SOB)
        W["admitted"] = [("D2", None, {}), ("D2[byte-string comparison]", ["byte-string comparison"], {})]
    case("addr-unver-D2", addr_unver, [("an unverified address use: (da) held in D2, depends_on its candidate line", lambda r, rc, o: item(r, "da_no_name_mapping")[0] == "held"
                                         and pair0(r)["items"]["da_no_name_mapping"]["depends_on"] == ["\t".join(SOB)])])
    case("addr-unver-D1", addr_unver, [("D1: (da) missing, rests on an unverified entry", lambda r, rc, o: item(r, "da_no_name_mapping")[0] == "missing"
                                         and "rests on an unverified entry: strncpy:out-behaviour" in item(r, "da_no_name_mapping")[1]
                                         and pair0(r)["items"]["da_no_name_mapping"]["depends_on"] == ["\t".join(SOB)])], branch="D1-R0")
    case("addr-unver-part", addr_unver, [("a partial admission without the kind: (da) missing", lambda r, rc, o: item(r, "da_no_name_mapping")[0] == "missing")],
         branch="D2_byte-string_comparison-R0")
    # 12.4 item 4: a found use with no generation is undecided, never unmet; its instance link is unlinked
    def found_spm(W):
        found(W)
        W["rel"]["success"] = W["rel"]["success"].replace(": held:", ": missing(an implication between conditions is not decided):")
    case("found-nogen", found_spm, [("success path missing: no generation; name state undecided (no create of the found id)",
                                     lambda r, rc, o: use0(r)["name_state"] == "undecided(no create of the found id in the record)"),
                                    ("instance link unlinked, no create of the id", lambda r, rc, o: use0(r)["instance_link"] == {
                                        "status": "unlinked", "reason": "link_unlinked(no create of the id in the record)"}),
                                    ("record link unlinked", lambda r, rc, o: r["link"]["status"] == "unlinked")])
    case("found-nogen-inc", lambda W: (found_spm(W), W["rel"].__setitem__("inc", "incomplete: actor_not_extracted(spawn call with no resolved entry)")),
         [("creator sites not complete: undecided with that sub-reason", lambda r, rc, o: use0(r)["name_state"] == "undecided(the row's creator sites are not complete)")])

    def found_nocreate(W):
        found(W)
        W["events"] = W["events"][:10]
    case("found-nocreate", found_nocreate, [("no create event at all, complete creators: undecided, not unmet", lambda r, rc, o: use0(r)["outcome"] == "found"
                                              and use0(r)["name_state"] == "undecided(no create of the found id in the record)")])
    # 12.5 item 5: the list entries (c1), (c2), (c4), (c3)(i) rest on, per data branch
    LC = ("framework", "Lib_Create", "agreement", "fw", "-", "7", "sem-unverified", "framework:agreement")
    PART3 = "D2_byte-string_comparison_data_function_termination-R0"

    def dep_c4(W):
        W["rel"]["layers"] = "layers: B P0 R sem-unverified(Lib_Create:agreement)"
        W["cands"].append(LC)
        W["admitted"] = [("D2", None, {}), ("D2[byte-string comparison,data function,termination]", ["byte-string comparison", "data function", "termination"], {})]
    FX["dep_c4_D2"], _ = case("dep-c4-D2", dep_c4, [("(c4) present in D2 with depends_on the candidate line", lambda r, rc, o: r["use_cond"]["writers"]["status"] == "present"
                                 and r["use_cond"]["writers"]["depends_on"] == ["\t".join(LC)]),
                                ("(c1) depends_on none", lambda r, rc, o: r["use_cond"]["callee_check"]["depends_on"] == [])])
    FX["dep_c4_D1"], _ = case("dep-c4-D1", dep_c4, [("(c4) not present in D1, rests on an unverified entry", lambda r, rc, o: r["use_cond"]["writers"]["status"] == "missing"
                                 and "writer_set_incomplete(rests on an unverified entry: Lib_Create:agreement)" in r["use_cond"]["missing"]
                                 and r["use_cond"]["writers"]["depends_on"] == ["\t".join(LC)])], branch="D1-R0")
    case("dep-c4-part", dep_c4, [("(c4) not present in a partial admission without the kind", lambda r, rc, o: r["use_cond"]["writers"]["status"] == "missing"
                                   and r["use_cond"]["status"] == "partial")], branch=PART3)
    LS = ("P0", "Lib_Search", "role", "p0", "-", "8", "P0-unverified", "termination")

    def dep_c1(W):
        W["rel"]["layers"] = "layers: B P0 R P0-unverified(Lib_Search)"
        W["cands"].append(LS)
    case("dep-c1-D1", dep_c1, [("(c1) and (c2) rest on the flip line's listed entry: missing in D1", lambda r, rc, o: r["use_cond"]["callee_check"]["status"] == "missing"
                                 and r["use_cond"]["reach"]["status"] == "missing" and "absent_result_unevaluated(rests on an unverified entry: Lib_Search:role)" in r["use_cond"]["missing"])],
         branch="D1-R0")
    case("dep-c1-D2", dep_c1, [("(c1) present in D2 with its depends_on", lambda r, rc, o: r["use_cond"]["callee_check"]["status"] == "present"
                                 and r["use_cond"]["callee_check"]["depends_on"] == ["\t".join(LS)])])
    CR = ("framework", "cons_run", "agreement", "fw", "-", "9", "sem-unverified", "framework:agreement")

    def dep_ci(W):
        W["rel"]["layers"] = "layers: B P0 R sem-unverified(cons_run:agreement)"
        W["cands"].append(CR)
    case("dep-ci-D1", dep_ci, [("(c3)(i) not established in D1 on the holder line's listed entry", lambda r, rc, o: r["use_cond"]["branch_i"]["status"] == "not_established"
                                 and r["use_cond"]["branch_i"]["why"].startswith("rests on an unverified entry: cons_run:agreement"))], branch="D1-R0")
    case("dep-ci-D2", dep_ci, [("(c3)(i) held in D2 with its depends_on", lambda r, rc, o: r["use_cond"]["branch_i"]["status"] == "held"
                                 and r["use_cond"]["branch_i"]["depends_on"] == ["\t".join(CR)])])
    case("dep-none", lambda W: None, [("no listed entry named: every element and (da) depends_on none", lambda r, rc, o: all(
        r["use_cond"][k]["depends_on"] == [] for k in ("callee_check", "reach", "writers", "branch_i")) and pair0(r)["items"]["da_no_name_mapping"]["depends_on"] == [])])

    # 12.6 spec 11.5 item 1: the dump 1.5 kinds no-build-id and object-table lose no event; the link reason follows the cause
    noloss = lambda r: '"unknown-loss"' not in json.dumps(r)
    case("loss-nobuildid", lambda W: W["tok"].extend(["loss no-build-id actor=- key=9 count=1", "loss object-table actor=- key=0 count=2"]),
         [("only the bookkeeping kinds: no loss marker, the use unmet as without them", lambda r, rc, o: use0(r)["name_state"] == "unmet" and noloss(r)),
          ("(ma) held, the pair bound", lambda r, rc, o: pair0(r)["missing_items"] == [] and pair0(r)["result"] == "bound")])
    case("loss-event", lambda W: W["tok"].append("loss after-stop actor=2 key=0 count=1"),
         [("an event loss: the name state undecided on the loss markers", lambda r, rc, o: use0(r)["name_state"] == "undecided(lifetime ends not complete: loss markers in the record)"),
          ("needs not reached become unknown-loss", lambda r, rc, o: not noloss(r)),
          ("(ma) missing", lambda r, rc, o: "ma_no_lifetime_end" in pair0(r)["missing_items"])])
    case("found-loss", lambda W: (found(W), W["tok"].append("loss after-stop actor=2 key=0 count=1")),
         [("a found use under an event loss: unlinked, record_insufficient(loss marker)", lambda r, rc, o: use0(r)["instance_link"] == {
             "status": "unlinked", "reason": "record_insufficient(loss marker)", "why": "loss markers in the record"})])
    case("found-nobuildid", lambda W: (found(W), W["tok"].append("loss no-build-id actor=- key=1 count=1")),
         [("a found use with only a no-build-id loss line: linked on its open generation as without it", lambda r, rc, o: use0(r)["instance_link"]["status"] == "linked"
           and use0(r)["name_state"] == "met")])
    case("found-unbound-end", lambda W: (found(W), W["unbound"].add(30)),
         [("a lifetime-end site not bound: unlinked, record_insufficient(site not bound in the build)", lambda r, rc, o: use0(r)["instance_link"] == {
             "status": "unlinked", "reason": "record_insufficient(site not bound in the build)", "why": "lifetime-end sites not bound: 30"})])

    # 10. format checks: break each of 9-17 in a good record; --check refuses every one
    good = rec(os.path.join(WORK, "001_base", "out"), "D2-R0")
    gpath = os.path.join(WORK, "good.json")
    with open(gpath, "w") as f:
        json.dump(good, f)
    rcg = subprocess.call([PY, "-I", os.path.join(OBS, "explain.py"), "--check", gpath, "--vocabulary", VOC], stdout=subprocess.DEVNULL)
    ok("--check accepts the base record", rcg == 0)
    breaks = {
        "9 bound pair with an item not held": lambda r: r["link"]["name_binding"]["pairs"][0]["items"]["na_bound"].__setitem__("status", "missing"),
        "10 refuted without scope": lambda r: (r["link"].__setitem__("status", "refuted"), r["link"].pop("basis", None)),
        "11 basis without an established link": lambda r: r["link"].__setitem__("status", "unlinked"),
        "12 name-binding use not absent": lambda r: r["order"]["use_events"][0].__setitem__("outcome", "found"),
        "13 derived without (iii) held": lambda r: r["use_cond"]["branch_iii"][0].__setitem__("status", "not_established"),
        "14 a use without its name state": lambda r: r["order"]["use_events"][0].pop("name_state"),
        "14 uses_decided above uses_reached": lambda r: r.__setitem__("uses_decided", 9),
        "15 a held item on a hand reading": lambda r: r["link"]["name_binding"]["pairs"][0]["items"]["ga_namespace_entry"]["grounds"].append("hand reading"),
        "16 D1 record with an item on an unverified entry": lambda r: r["eval_branch"].__setitem__("data", "D1"),
        "16 order_ground of another form": lambda r: r.__setitem__("order_ground", ["the row value"]),
        "17 no eval_branch": lambda r: r.pop("eval_branch"),
        "17 a reading without its tag": lambda r: r["eval_branch"].update(readings=["Ra"], tag=None),
    }
    for name, br_ in breaks.items():
        bad = copy.deepcopy(good)
        br_(bad)
        bp = os.path.join(WORK, "bad.json")
        with open(bp, "w") as f:
            json.dump(bad, f)
        rcb = subprocess.call([PY, "-I", os.path.join(OBS, "explain.py"), "--check", bp, "--vocabulary", VOC], stdout=subprocess.DEVNULL)
        ok("format check %s: refused" % name, rcb == 4)
    # 10b. spec 11 items 1, 4 and 6: the added format checks 14 and 16 (a)-(d), each broken in a record that passes
    def uc_of(r):
        return r["use_cond"]
    breaks2 = [
        ("14 a use with a context of no alternatives line", good, lambda r: r["order"]["use_events"][0].__setitem__("context", "7"), "spec 11 item 1"),
        ("14 a found use whose name state is unmet", FX.get("found"), lambda r: r["order"]["use_events"][0].__setitem__("name_state", "unmet"), "spec 11 item 4"),
        ("16 (a) (c1) without depends_on", good, lambda r: uc_of(r)["callee_check"].pop("depends_on"), "(16) (a)"),
        ("16 (a) a pair's (da) without depends_on", good, lambda r: r["link"]["name_binding"]["pairs"][0]["items"]["da_no_name_mapping"].pop("depends_on"), "(16) (a)"),
        ("16 (b) D1: (c4) present on a list entry", FX.get("dep_c4_D1"), lambda r: uc_of(r)["writers"].__setitem__("status", "present"), "(16) (b)"),
        ("16 (c) D2[...]: (c4) present on a line of a kind not admitted", FX.get("dep_c4_D2"),
         lambda r: (r["eval_branch"].__setitem__("data", "D2[byte-string comparison]"), r["data_branch"].__setitem__("admitted_kinds", ["byte-string comparison"])), "(16) (c)"),
        ("16 (d) (na) held with the sub-reason no entry", FX.get("noentry-D2-R0"),
         lambda r: r["link"]["name_binding"]["pairs"][0]["items"]["na_semantics"].__setitem__("status", "held"), "(16) (d)"),
    ]
    for name, base_rec, br_, want in breaks2:
        if base_rec is None:
            ok("format check %s: refused" % name, False, "no base record")
            continue
        g2 = os.path.join(WORK, "good2.json")
        with open(g2, "w") as f:
            json.dump(base_rec, f)
        r0 = subprocess.run([PY, "-I", os.path.join(OBS, "explain.py"), "--check", g2, "--vocabulary", VOC], stdout=subprocess.PIPE, universal_newlines=True)
        bad = copy.deepcopy(base_rec)
        br_(bad)
        bp = os.path.join(WORK, "bad2.json")
        with open(bp, "w") as f:
            json.dump(bad, f)
        r1 = subprocess.run([PY, "-I", os.path.join(OBS, "explain.py"), "--check", bp, "--vocabulary", VOC], stdout=subprocess.PIPE, universal_newlines=True)
        ok("format check %s: the record passes, the broken one is refused by it" % name, r0.returncode == 0 and r1.returncode == 4 and want in r1.stdout,
           (r0.stdout[-200:], r1.stdout[-300:]))
    # 11. summary and expectation comparison (they change no record)
    before = sha(os.path.join(WORK, "001_base", "out", "D2-R0", "index.tsv"))
    sm = os.path.join(WORK, "summary.txt")
    ul = os.path.join(WORK, "uses_fx.tsv")     # a synthetic list of pre-defined uses: the fixture row's use in its two contexts
    with open(ul, "w") as f:
        f.write("kz-uses 1\nrule synthetic\ncolumns build\trow_label\trow_key\tpath\tconsumer_root\tuse_tu\tuse_line\tuse_col\tcallee\tat_site\t"
                "after_site\tcontext_id\tframe_sites\tplan_state\n")
        for cid, fr in (("1", "40,41"), ("2", "42,43")):
            f.write("\t".join(["fx", "A1", KEY, "A", "ConsumerMain (script-entry)", "app/cons.c", "50", "9", "Lib_FindByName", "10", "11", cid, fr, "tier+control"]) + "\n")
        f.write("lines 2 rows 1\nend\n")
    rcs = subprocess.call([PY, "-I", os.path.join(OBS, "explain_summary.py"), "--out", sm, "--uses", ul, "boot1=%s" % os.path.join(WORK, "001_base", "out"),
                           "boot2=%s" % os.path.join(WORK, "001_base", "out")], stdout=subprocess.DEVNULL)
    txt = open(sm).read() if os.path.exists(sm) else ""
    ok("summary rc 0, per-branch tables, no detection count", rcs == 0 and "table D2-R0" in txt and "violation x pair_kind x order" in txt and "detect" not in txt.lower())
    tl = txt.splitlines()
    heads = {}
    for i, ln in enumerate(tl):
        if ln.startswith("table "):
            heads[ln.split()[1]] = tl[i + 1]
    ok("summary (spec 11 item 7): main head on D1-R0 only, auxiliary heads with data and readings, the reading tag copied",
       heads.get("D1-R0", "").startswith("  head: main (declared before the boot: D1/R0)") and heads.get("D2-R0") == "  head: auxiliary (data D2, readings R0)"
       and heads.get("D2-Ra") == "  head: auxiliary (data D2, readings Ra) [4.2 의 읽기 갈래(PI 결정 전)]"
       and sum(1 for h in heads.values() if "main" in h) == 1, list(heads.items())[:3])
    ok("summary (spec 11 item 8): one row per pre-defined use, the unreached context included; per-boot reached / defined",
       sum(1 for ln in tl if ln.startswith("    use\t")) == 2 * len(heads) and "    boot boot1: reached 1 / defined 2; decided 1 / reached 1" in txt
       and any(ln.startswith("    use\tfx\tA1\t%s\tConsumerMain (script-entry)\tcontext 2\t2 boots: observed 0, not observed 0, refuted 0, undetermined 0, not reached 2" % KEY)
               for ln in tl), [ln for ln in tl if ln.startswith("    boot ")][:2])
    ok("summary (spec 11 item 9): the eight pair-kind lines, the between-app line",
       all(sum(1 for ln in tl if ln.startswith("    pair_kind\t%s\t" % k)) == len(heads) for k in ("app-app", "core-app", "process-app", "same-app-other",
                                                                                          "same-app-main-child", "same_actor", "core-core", "unknown"))
       and "    between apps (the headline line: app-app, both owners determined): 2" in txt)
    for name, f_ in (("two head lines in a table", lambda L: L.insert(L.index("table D2-R0") + 2, "  head: auxiliary (data D2, readings R0)")),
                     ("a main head on another branch", lambda L: L.__setitem__(L.index("table D2-R0") + 1, "  head: main (declared before the boot: D1/R0)")),
                     ("a use row missing", lambda L: L.remove(next(x for x in L if x.startswith("    use\t")))),
                     ("a pair-kind line missing", lambda L: L.remove(next(x for x in L if x.startswith("    pair_kind\tcore-core\t"))))):
        L = list(tl)
        f_(L)
        bp = os.path.join(WORK, "summary_bad.txt")
        with open(bp, "w") as f:
            f.write("\n".join(L) + "\n")
        rcb = subprocess.call([PY, "-I", os.path.join(OBS, "explain_summary.py"), "--check-summary", bp], stdout=subprocess.DEVNULL)
        rcg = subprocess.call([PY, "-I", os.path.join(OBS, "explain_summary.py"), "--check-summary", sm], stdout=subprocess.DEVNULL)
        ok("summary form check: %s refused (rc 3), the written summary passes" % name, rcb == 3 and rcg == 0)
    ex = os.path.join(WORK, "expect.txt")
    with open(ex, "w") as f:
        f.write("kz-expect 1\nplan x\n" + "\n".join("\t".join(x) for x in [
            ("expect", "D1-R0", "fx", KEY, "ConsumerMain", "pair[app/maker.c:30:5].missing_items", "na_semantics", "boot"),
            ("expect", "D2-R0", "fx", KEY, "*", "tool_verdict.violation.value", "observed", "boot"),
            ("expect", "D1-R0", "fx", KEY, "*", "use_cond.status", "derived", "static")]) + "\nlines 3\nend\n")
    eo = os.path.join(WORK, "expect_out.txt")
    rce = subprocess.call([PY, "-I", os.path.join(OBS, "explain_expect.py"), "--expect", ex, "--records", os.path.join(WORK, "001_base", "out"), "--out", eo],
                          stdout=subprocess.DEVNULL)
    et = open(eo).read() if os.path.exists(eo) else ""
    ok("expectation comparison: 2 match, 1 mismatch", rce == 0 and et.count("\tmatch\t") == 2 and et.count("\tmismatch\t") == 1, et[-300:])
    ok("expectation comparison (spec 11 item 12): the class on every line and the counts per class",
       "# by class: boot: match 1, mismatch 0, no_record 1; static: match 0, mismatch 1, no_record 0; format: match 0, mismatch 0, no_record 0" in et
       or "# by class: boot: match 2, mismatch 0, no_record 0; static: match 0, mismatch 1, no_record 0; format: match 0, mismatch 0, no_record 0" in et, et[-400:])
    for name, bad_line in (("no class column", "expect\tD2-R0\tfx\t%s\t*\tuse_cond.status\tpartial" % KEY),
                           ("a class outside the three", "expect\tD2-R0\tfx\t%s\t*\tuse_cond.status\tpartial\tmaybe" % KEY)):
        exb = os.path.join(WORK, "expect_bad.txt")
        with open(exb, "w") as f:
            f.write("kz-expect 1\nplan x\n%s\nlines 1\nend\n" % bad_line)
        eob = os.path.join(WORK, "expect_bad_out_%d.txt" % len(RESULTS))
        rcb = subprocess.call([PY, "-I", os.path.join(OBS, "explain_expect.py"), "--expect", exb, "--records", os.path.join(WORK, "001_base", "out"), "--out", eob],
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        ok("expectation file with %s: refused (rc 2)" % name, rcb == 2)
    ok("summary and comparison changed no record", sha(os.path.join(WORK, "001_base", "out", "D2-R0", "index.tsv")) == before)
    n_pass = sum(1 for _, c in RESULTS if c)
    print("fixtures: %d checks, %d passed, %d failed" % (len(RESULTS), n_pass, len(RESULTS) - n_pass))
    return 0 if n_pass == len(RESULTS) else 1


if __name__ == "__main__":
    sys.exit(main())
