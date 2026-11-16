#!/usr/bin/env python3
"""test_explain.py -- synthetic fixtures for env/obs1/explain.py (python3 -I).

  python3 -I test_explain.py <explain.py> <reason_vocabulary.txt> <work dir>

Every fixture is a stub plan (sidecar, manifest, bindings), a stub relations row with its keys file, a stub facts view,
and a stub boot (dump text, run.meta, chain record, startup script) written into a new directory under <work dir>; the
script is run on it with python3 -I and its records are read back.  Covered (revision 3: a creator's callee whose returns
are not all followed, or whose success constant is only on a line marked not shown to reach the return, gives no success
constant; revision 2, after review): the link by
JR 3.2 axis 1 as written (an absence outcome stays unlinked; established only when the use's and the creator's out
ids, each read under its success constant, are the same instance), the publisher's out kept raw without the creator's
success constant, sidecar reasons with spaces read whole, the unbalanced-reason refusal, the stop budget in
budget.run, the plan executables from the instance list run.meta names, and a refused instance list.  Also: each need state of JR 3.5 (present,
value-missing, text-partial, not-reached, unknown-loss, unbound, not-planned, build-mismatch), each status of the link,
use_cond, order and effect axes, the intervention status the stage can produce (unconfirmed), a repeated
creation-argument value (both ties undetermined), and each JR 3.6 format rule (a record that breaks it is refused by
--check).  These check the script; they are not detection experiments.  Prints one line per case and a summary;
rc 0 only if every case passes.
"""
import copy
import hashlib
import json
import os
import subprocess
import sys

EXPLAIN, VOCAB, WORK = sys.argv[1], sys.argv[2], sys.argv[3]
FSHA = "f" * 64
RESULTS = []


def sha(path):
    with open(path, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()


def w(path, text):
    with open(path, "w") as f:
        f.write(text)


MANIFEST_SITES = """kzs-sites 3
site 1 label=s0001.actor-root op=entry.C_Main phase=before tu=c.c func=C_Main ref="actor C_Main (script-entry)"
site 2 label=s0002.actor-root op=entry.P_Main phase=before tu=p.c func=P_Main ref="actor P_Main (script-entry)"
site 3 label=s0003.actor-create op=call.pthread_create phase=point tu=t.c func=Create callee=pthread_create line=10 place=at-call ref="creation t.c:10"
value 3 0 role=start_routine from=arg2 type=u64
value 3 1 role=creation_arg from=arg3 type=u64
site 4 label=s0004.actor-create op=call.pthread_create phase=point tu=t.c func=Create callee=pthread_create line=10 place=after-call ref="creation t.c:10"
value 4 0 role=status from=ret type=s64
value 4 1 role=out0 from=out0 type=u64
site 5 label=s0005.actor-start op=entry.TaskEntry phase=before tu=t.c func=TaskEntry ref="start routine"
value 5 0 role=param0 from=arg0 type=u64
site 6 label=s0006.stop-event op=entry.EXIT phase=before tu=e.c func=EXIT ref="framework-layer data: app-exit"
value 6 0 role=param0 from=arg0 type=u64
site 7 label=s0007.stop-event op=entry.LOOP phase=before tu=e.c func=LOOP ref="framework-layer data: run-loop"
value 7 0 role=param0 from=arg0 type=u64
site 10 label=s0010.use-call op=call.LOOKUP phase=point tu=c.c func=C_Init callee=LOOKUP line=20 place=at-call ref="row a1b2c3d4e5f60718"
value 10 0 role=key from=arg1 type=ptr string=pointer-only
value 10 1 role=arg0 from=arg0 type=u64
site 11 label=s0011.handling-chain+use-call op=call.LOOKUP phase=point tu=c.c func=C_Init callee=LOOKUP line=20 place=after-call ref="row a1b2c3d4e5f60718"
value 11 0 role=status from=ret type=s64
value 11 1 role=out0 from=out0 type=u64
site 12 label=s0012.key-text op=call.FIND phase=point tu=o.c func=LOOKUP callee=FIND line=5 place=at-call ref="row a1b2c3d4e5f60718"
value 12 0 role=key_text from=arg1 type=ptr string=validated bound=64 ref="premise: NUL-terminated or the cut is flagged"
site 13 label=s0013.creator-call op=call.CREATE phase=point tu=p.c func=P_Init callee=CREATE line=30 place=at-call ref="row a1b2c3d4e5f60718"
value 13 0 role=key from=arg1 type=ptr string=pointer-only
value 13 1 role=arg0 from=arg0 type=u64
site 14 label=s0014.creator-call op=call.CREATE phase=point tu=p.c func=P_Init callee=CREATE line=30 place=after-call ref="row a1b2c3d4e5f60718"
value 14 0 role=status from=ret type=s64
value 14 1 role=out0 from=out0 type=u64
site 15 label=s0015.key-text op=call.memchr phase=point tu=o.c func=CREATE callee=memchr line=7 place=at-call ref="row a1b2c3d4e5f60718"
value 15 0 role=key_text from=arg0 type=ptr string=validated bound=20 ref="premise"
site 16 label=s0016.handling-chain op=call.EXIT phase=point tu=c.c func=C_Main callee=EXIT line=40 place=at-call ref="row a1b2c3d4e5f60718"
value 16 0 role=arg0 from=arg0 type=u64
"""

ROW_PARTIAL = """  A1 lookup c.c:20:5 in C_Init -> LOOKUP#1 namespace NS.name partition {3} key unresolved
      key: read through a pointer field
      actors: {C_Main (script-entry)}; result=tested
      creator: key unresolved (lookup): p.c:30:5 in P_Init -> CREATE key read through a pointer field actors {P_Main (script-entry)}
      handling: terminal EXIT at c.c:40:5 in C_Main: under (ne local:status 0) (the guard tests a local; the result of C_Init is used in C_Main: may) is argument 0 of it
      dependency: path A: 1 creator(s) of this partition in other actors
      feasibility: undecidable: the facts give no static order between the lookup's actor and the creators' actors
      flip: static-undetermined: the key of the lookup or of a creator is not resolved statically; search returns: FIND returns -17 [ERR_NOT_FOUND], FIND returns 0 [OK]
      incomplete: key_unresolved(pointer field without points-to); fixed_proof_open(no static order between the actors)
"""
ROW_DERIVED = """  A1 lookup c.c:20:5 in C_Init -> LOOKUP#1 namespace NS.name partition {3}
      actors: {C_Main (script-entry)}; result=tested
      creator: p.c:30:5 in P_Init -> CREATE key string literal actors {P_Main (script-entry)}
      handling: terminal EXIT at c.c:40:5 in C_Main: under (ne local:status 0) is argument 0 of it
      dependency: path A: 1 creator(s) of this partition in other actors
      feasibility: possible: the lookup's actor is created before the creator's actor
      flip: static-possible: FIND returns -17 [ERR_NOT_FOUND] with the earlier value (absent), not with the written value; FIND returns 0 [OK]
      incomplete: fixed_proof_open(no static order between the actors)
"""
ROW_NOTDERIVED = """  A1 lookup c.c:20:5 in C_Init -> LOOKUP#1 namespace NS.name partition {3}
      actors: {C_Main (script-entry)}; result=discarded
      dependency: path A: 1 creator(s) of this partition in other actors
      feasibility: undecidable: no static order
      flip: none: no constant
      incomplete: fixed_proof_open(no static order between the actors)
"""


def ev(actor, aseq, site, t, vals, s=None, flags="clock,pc"):
    v = ",".join(vals)
    st = ' str="%s"' % s if s is not None else ""
    if s is not None and "str" not in flags:
        flags += ",str"
    return "ev actor=%d aseq=%d site=%d[x] bind=1 link=0 t=%d pc=0x1 key=tid:%d v=[%s]%s flags=%s" % (
        actor, aseq, site, t, 100 + actor, v, st, flags)


def base_events(order="use-first", pub_key="K1", use_status="0xffffffffffffffef", use_out="0x0", second_arg="0x10002",
                trunc=False, after_use=True, use=True):
    """actor 1 creates 2 (consumer C) and 3 (publisher P)."""
    t_use = 2000000 if order == "use-first" else 6000000
    t_pub = 4000000
    E = [ev(1, 1, 3, 1000000, ["0xaaa", "0x10001"]), ev(1, 2, 4, 1000100, ["0x0", "0x77"]),
         ev(1, 3, 3, 1000200, ["0xaaa", second_arg]), ev(1, 4, 4, 1000300, ["0x0", "0x78"]),
         ev(2, 1, 5, 1100000, ["0x10001"]), ev(2, 2, 1, 1200000, ["-"])]
    if use:
        E += [ev(2, 3, 10, t_use, ["0x500", "0x600"]),
              ev(2, 4, 12, t_use + 10, ["0x500"], s="K1", flags="clock,pc,str" + (",str-trunc" if trunc else "")),
              ev(2, 5, 11, t_use + 20, [use_status, use_out])]
        if after_use:
            E += [ev(2, 6, 16, t_use + 30, ["0x3"]), ev(2, 7, 6, t_use + 40, ["0x3"])]
    E += [ev(3, 1, 5, 1150000, [second_arg]), ev(3, 2, 2, 1250000, ["-"]),
          ev(3, 3, 13, t_pub, ["0x900", "0x901"]), ev(3, 4, 15, t_pub + 10, ["0x900"], s=pub_key),
          ev(3, 5, 14, t_pub + 20, ["0x0", "0x30002"])]
    return E


ROW_DERIVED_CC = ROW_DERIVED.replace("      dependency:", "      callee-check: CREATE returns -5 [ERR_FULL], CREATE returns 0 [OK]\n      dependency:")


def make(case, row=ROW_PARTIAL, events=None, pool_dropped=0, unbind=(), mismatch=False, run=True, extra_need=True,
         budget=True, list_named=True):
    d = os.path.join(WORK, case)
    os.makedirs(d)
    side = ["kz-plan-sidecar 1", "input facts sha256=%s path=/stub/facts.kz" % FSHA,
            "input framework-layer-data sha256=%s path=/stub/fw.txt" % ("e" * 64),
            "rules plan rules revision 1", "priority method-priority: stub",
            'budget value=tier:D07 reason="stub" disclosure="stub" planned=1 truncation=0 rows cut unresolved="none" sensitivity="not measured"',
            "bound string KZ_RT_STR_MAX=64",
            "row a1b2c3d4e5f60718 A1 tier=1 position=1 label=R class=decided order=RELOC reach=CALLEE-ONLY disposition=D07 state=planned why=\"stub\"",
            "map a1b2c3d4e5f60718 10 11 12 13 14 15 16"]
    if extra_need:
        side.append('need row a1b2c3d4e5f60718 wait-result not-planned reason=plan_missing(need not planned) note="stub"')
        side.append('gap row a1b2c3d4e5f60718 "key text of LOOKUP parameter 1: stub" reason=record_insufficient(key text) rule=key-text')
    side += ["stop-event run-loop LOOP e.c:1 decl e.h:1", "stop-event app-exit EXIT e.c:2 decl e.h:2"]
    w(os.path.join(d, "plan.side"), "\n".join(side) + "\n")
    ss = sha(os.path.join(d, "plan.side"))
    w(os.path.join(d, "plan.kzs"), "# stub plan\n# sidecar sha256=%s\n" % (ss if not mismatch else "0" * 64) + MANIFEST_SITES)
    ms = sha(os.path.join(d, "plan.kzs"))
    w(os.path.join(d, "binds.kzs"), "".join("bind %d site=%d module=1 status=bound vals=0xf\n" % (1000 + n, n)
                                            for n in (1, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16) if n not in unbind))
    w(os.path.join(d, "relations.txt"), "kz-relations 1\nfacts %s\nrules relations rules revision 6 (stub)\n" % FSHA + row)
    w(os.path.join(d, "keys.txt"), "kz-relation-keys 1\nfacts %s\nrules stub\nA1 a1b2c3d4e5f60718 A|NS.name|C_Init|LOOKUP#1|1\n" % FSHA)
    w(os.path.join(d, "facts_view.txt"), 'facts /stub/facts.kz sha256=%s\ntool_sha256=%s\n  BASIS[1] name="extract.p0_table" value="P0 stub"\n  BASIS[2] name="p0.pthread_create" value="thread-create; stub"\n' % (FSHA, "c" * 64))
    w(os.path.join(d, "basis.kz"), "stub basis\n")
    bs = sha(os.path.join(d, "basis.kz"))
    args = ["python3", "-I", EXPLAIN, "--sidecar", d + "/plan.side", "--manifest", d + "/plan.kzs", "--bindings", d + "/binds.kzs",
            "--relations", d + "/relations.txt", "--keys", d + "/keys.txt", "--vocabulary", VOCAB, "--facts-view", d + "/facts_view.txt",
            "--bprime-basis", d + "/basis.kz", "--build-label", "stub", "--out", d + "/out"]
    if run:
        E = events if events is not None else base_events()
        dump = ["run abi=1 mode=stopped run_id=1 clock=1", "run manifest_sha256=%s" % ms,
                "run t_init=1000 t_attach=1 t_stop=9000000 t_dump=9100000",
                "runx pool_bytes=1 chunk_bytes=1 chunks_used=1 pool_dropped=%d boot_bytes_used=0 boot_bytes_dropped=0" % pool_dropped,
                "runx process=101 initial=tid:101 evidence=constructor", "runx end=in-process end_flags=- end_value=0 store=memory",
                "module 1 actor=? registrations=1 manifest_sha256=%s" % ms]
        for a in (1, 2, 3):
            dump.append("actor %d key=tid:%d overflow=0 partial=0 dropped_bytes=0" % (a, 100 + a))
        dump += E + ["format kzrt-dump 1.4 partial=0", "check ok (0 failures)"]
        w(os.path.join(d, "dump.txt"), "\n".join(dump) + "\n")
        w(os.path.join(d, "inst_files.sha256"), "%s  ./tx/core\n%s  ./tx/m.so\n%s  ./tx/scr\n" % ("d" * 64, "a" * 64, "b" * 64))
        ls = sha(os.path.join(d, "inst_files.sha256")) if list_named else "0" * 64
        w(os.path.join(d, "run.meta"), "file tx/core sha256=%s (build x)\nmanifest %s/plan.kzs sha256=%s\n"
          "instance_files dir=i files=3 other=0 list=%s/inst_files.sha256 list_sha256=%s\nstop=stub (CI L1)\ntx_exit=0 rx_exit=0\n" % ("d" * 64, d, ms, d, ls))
        w(os.path.join(d, "chain.txt"), "manifest\tconfirmed\tstub\nbprime\tconfirmed\tbasis=%s\n"
          "exe exe/tx/core\tconfirmed\trecord=dddddddddddddddd run.meta[tx/core]=dddddddddddddddd inst_list[tx/core]=dddddddddddddddd\n"
          "exe exe/tx/m.so\tconfirmed\trecord=aaaaaaaaaaaaaaaa inst_list[tx/m.so]=aaaaaaaaaaaaaaaa\n"
          "same_build\tunconfirmed(no build-id in dump)\tstub\nchain\tconfirmed\t3 links\n" % bs[:16])
        w(os.path.join(d, "stop_eval.tsv"), "# stub\napp\tentry\tfile\troot_sites\tactor\troot_ms\tstop_event\tstop_site\tstop_aseq\tstop_ms\tinside_window\tstatus\n"
          "CAPP\tC_Main\tc\t1\t2\t1.2\tEXIT\t6\t7\t2.0\tyes\treached\nPAPP\tP_Main\tp\t2\t3\t1.3\t-\t-\t-\t-\t-\tnot-reached\nsummary\tx\n")
        w(os.path.join(d, "boot_budget.tsv"), "# stub boot budget\nstop.value\t5 s + command + 5 s\nstop.source\tCI L1\nstop.reason\tstub\n"
          "stop.truncation\tafter the process end\nstop.sensitivity\tnot measured\nstop.rule\tnever failure or normal\nrun.value\t1 boot\nrun.reason\tstub\nrun.sensitivity\tnot measured\n")
        w(os.path.join(d, "scr"), "CFE_APP, c, C_Main, CAPP, 1, 1, 0x0, 0;\nCFE_APP, p, P_Main, PAPP, 1, 1, 0x0, 0;\n!\n")
        args += ["--dump-text", d + "/dump.txt", "--run-meta", d + "/run.meta", "--chain", d + "/chain.txt", "--startup-script", d + "/scr",
                 "--instance-list", d + "/inst_files.sha256", "--stop-eval", d + "/stop_eval.tsv"]
        if budget:
            args += ["--boot-budget", d + "/boot_budget.tsv"]
    p = subprocess.run(args, capture_output=True, text=True)
    recs = []
    rd = os.path.join(d, "out", "records")
    if os.path.isdir(rd):
        for fn in sorted(os.listdir(rd)):
            with open(os.path.join(rd, fn)) as f:
                recs.append(json.load(f))
    return p.returncode, recs, d, p.stdout + p.stderr


def need_state(rec, site):
    for n in rec["incompleteness"]["needs"]:
        if n["need"].startswith("site %d " % site):
            return n["state"]
    return None


def case(name, ok, detail):
    RESULTS.append((name, ok))
    print("%s %s: %s" % ("PASS" if ok else "FAIL", name, detail))


def ax(r):
    return (r["link"]["status"], r["use_cond"]["status"], r["order"]["status"], r["intervention"]["status"],
            r["effect"]["status"], r["tool_verdict"]["violation"]["value"], r["tool_verdict"]["odf"]["value"])


os.makedirs(WORK, exist_ok=True)
# --- control: present needs, unlinked link (absence outcome, JR 3.2 axis 1 as written), partial use condition, events
# only, failure sign; the sidecar reasons read whole; the stop budget and the plan executables in the record
rc, R, d, out = make("control")
r = R[0] if R else None
ok = rc == 0 and len(R) == 1 and ax(r) == ("unlinked", "partial", "events_only", "unconfirmed", "observed", "undetermined", "failure_sign") \
    and "link_unlinked(absence outcome: the use reached no object instance)" in r["link"]["missing"] \
    and "record_insufficient(no success constant of the creator: the creator's out id is not read)" in r["link"]["missing"] \
    and r["link"]["resource_key"]["generation"] is None and r["link"]["resource_key"]["name"] == "K1" \
    and r["link"]["instance_evidence"][0]["admitted"] is False \
    and r["order"]["publish_events"][0]["out"]["read"] is False and r["order"]["publish_events"][0]["out"]["raw"] == "0x30002" \
    and "L-ABS" not in [p["id"] for p in r["premises"]] \
    and [n["reason"] for n in r["incompleteness"]["needs"] if n["need"].startswith("wait-result")] == ["plan_missing(need not planned)"] \
    and [g["reason"] for g in r["incompleteness"]["static_gaps"]] == ["record_insufficient(key text)"] \
    and r["budget"]["run"]["stop"]["fired"].startswith("yes: run.meta 'stop=stub (CI L1)'") \
    and r["budget"]["run"]["stop"]["unresolved"] == "applications without their stop event inside the window: PAPP" \
    and r["budget"]["run"]["stop"]["value"] == "5 s + command + 5 s" and r["budget"]["run"]["run"]["value"] == "1 boot" \
    and sorted(r["build_id"]["executables_plan"]) == ["tx/core", "tx/m.so"] and r["build_id"]["executables_plan"]["tx/m.so"]["sha256"] == "a" * 64 \
    and all(need_state(r, s) == "present" for s in (10, 11, 12, 13, 14, 15, 16)) \
    and any(n["state"] == "not-planned" for n in r["incompleteness"]["needs"]) and r["tool_verdict"]["headline"] == "실패 징후" \
    and r["order"]["use_events"][0]["out"]["read"] is False and r["order"]["use_events"][0]["key"]["pointer_equal_to_key_text_argument"] is True \
    and r["consumer_actor"]["creation_tie"]["status"] == "determined" and r["actor_pair_kind"] == {"3": "app-app"} \
    and r["effect"]["recovery"] == "not_attempted"
case("control (present, not-planned; unlinked(absence outcome)/partial/events_only/unconfirmed/observed; undetermined, failure_sign; "
     "reasons read whole; stop budget; plan executables)", ok,
     "rc %d records %d %s" % (rc, len(R), ax(r) if r else out[-300:]))
CONTROL = d
# --- a 32-bit status recorded sign-extended against the row's unsigned constant
row32 = ROW_PARTIAL.replace("FIND returns -17 [ERR_NOT_FOUND]", "FIND returns 3221225473 [ERR_32]")
rc, R, _, out = make("status32", row=row32, events=base_events(use_status="0xffffffffc0000001"))
ok = rc == 0 and R and R[0]["order"]["use_events"][0]["status_name"] == "ERR_32"
case("a 32-bit status recorded sign-extended matches the row's unsigned constant", ok,
     str(R[0]["order"]["use_events"][0].get("status_name")) if R else out[-300:])
# --- refuted link
rc, R, _, out = make("refuted", events=base_events(pub_key="K2"))
ok = rc == 0 and R and R[0]["link"]["status"] == "refuted" and R[0]["tool_verdict"]["violation"]["value"] == "refuted_link"
case("link refuted (every publisher names another key) -> violation refuted_link", ok, str(ax(R[0])) if R else out[-300:])
# --- repeated creation argument: both ties undetermined, link unlinked
rc, R, _, out = make("collision", events=base_events(second_arg="0x10001"))
ok = rc == 0 and R and R[0]["consumer_actor"]["creation_tie"]["status"] == "undetermined" and R[0]["link"]["status"] == "unlinked" \
    and all(p["creation_tie"]["status"] == "undetermined" for p in R[0]["publisher_actors"]) \
    and any("collision" in m for m in R[0]["link"]["missing"])
case("repeated creation-argument value -> both ties undetermined, link unlinked", ok,
     (R[0]["consumer_actor"]["creation_tie"]["why"] if R else out[-300:]))
# --- derived use condition, use before the create -> unmet_state_observed; the absence outcome reaches no instance, so
# the link stays unlinked (JR 3.2 axis 1) and the violation is undetermined, not observed
rc, R, _, out = make("derived_unmet", row=ROW_DERIVED_CC)
ok = rc == 0 and R and ax(R[0])[:3] == ("unlinked", "derived", "unmet_state_observed") and R[0]["tool_verdict"]["violation"]["value"] == "undetermined" \
    and "link_unlinked(absence outcome: the use reached no object instance)" in R[0]["tool_verdict"]["violation"]["undetermined_reasons"] \
    and R[0]["order"]["publish_events"][0]["out"]["read"] is True and R[0]["tool_verdict"]["headline"] == "실패 징후"
case("use_cond derived, use before the create -> order unmet_state_observed; absence outcome -> link unlinked, violation undetermined", ok,
     str(ax(R[0])) if R else out[-300:])
# --- derived, create first, the use reads its id, the creator's out read under its success constant -> established,
# met_state_only, not_observed_in_runs
rc, R, _, out = make("derived_met", row=ROW_DERIVED_CC, events=base_events(order="pub-first", use_status="0x0", use_out="0x30002"))
ok = rc == 0 and R and ax(R[0])[:3] == ("established", "derived", "met_state_only") and R[0]["tool_verdict"]["violation"]["value"] == "not_observed_in_runs" \
    and R[0]["order"]["use_events"][0]["out"]["read"] is True and R[0]["link"]["resource_key"]["generation"] == "0x30002"
case("same instance reached (both out ids read under their success constants) -> established, met_state_only, not_observed_in_runs", ok,
     str(ax(R[0])) if R else out[-300:])
# --- the same, but the row gives no success constant of the creator: its out is not read -> unlinked, record_insufficient
rc, R, _, out = make("derived_met_nocc", row=ROW_DERIVED, events=base_events(order="pub-first", use_status="0x0", use_out="0x30002"))
ok = rc == 0 and R and ax(R[0])[:3] == ("unlinked", "derived", "record_insufficient") \
    and "record_insufficient(no success constant of the creator: the creator's out id is not read)" in R[0]["link"]["missing"] \
    and R[0]["link"]["resource_key"]["generation"] is None
case("no success constant of the creator: its out id is not read -> link unlinked, order record_insufficient", ok,
     str(ax(R[0])) if R else out[-300:])
# --- revision 3: the row gives the creator's callee constants, but a return of the callee is not followed: no success
# constant is taken from them (judgement rules 8.2 base requirement 4, 2026-11-16 addendum) -> not read, the named reason
ROW_DERIVED_OPEN = ROW_DERIVED_CC.replace("      dependency:", "      callee-check (returns not followed): CREATE: FINISH returns param [status] at p.c:50:5\n"
                                          "      callee-check (set): CREATE: not established (1 returns not followed, 0 constants not shown to reach the return)\n      dependency:")
rc, R, _, out = make("derived_met_open", row=ROW_DERIVED_OPEN, events=base_events(order="pub-first", use_status="0x0", use_out="0x30002"))
ok = rc == 0 and R and ax(R[0])[:3] == ("unlinked", "derived", "record_insufficient") \
    and "record_insufficient(no success constant of the creator: the callee's returns are not all followed)" in R[0]["link"]["missing"] \
    and R[0]["order"]["publish_events"][0]["out"]["read"] is False and R[0]["link"]["resource_key"]["generation"] is None
case("revision 3: a creator's callee with a return not followed gives no success constant -> out not read, named reason", ok,
     str(ax(R[0])) if R else out[-300:])
# --- revision 3: the creator's success constant only on a line marked "not shown to reach the return" -> not taken
ROW_DERIVED_MARK = ROW_DERIVED.replace("      dependency:", "      callee-check: CREATE returns -5 [ERR_FULL] when (lt param:n#0 0)\n"
                                       "      callee-check: CREATE returns 0 [OK] through CREATE -> ALLOC when (eq local:i 0); not shown to reach the return: CREATE returns the result of ALLOC at p.c:40:5 from the definition at p.c:31:9, redefined later on a path to the return\n      dependency:")
rc, R, _, out = make("derived_met_mark", row=ROW_DERIVED_MARK, events=base_events(order="pub-first", use_status="0x0", use_out="0x30002"))
ok = rc == 0 and R and ax(R[0])[:3] == ("unlinked", "derived", "record_insufficient") \
    and "record_insufficient(no success constant of the creator: the creator's out id is not read)" in R[0]["link"]["missing"] \
    and R[0]["order"]["publish_events"][0]["out"]["read"] is False
case("revision 3: a success constant only on a line marked not shown to reach the return is not taken -> out not read", ok,
     str(ax(R[0])) if R else out[-300:])
# --- no run: static_only; derived -> candidate; partial -> order-dependence candidate
rc, R, _, out = make("norun_derived", row=ROW_DERIVED, run=False)
ok1 = rc == 0 and R and R[0]["order"]["status"] == "static_only" and R[0]["tool_verdict"]["violation"]["value"] == "candidate" \
    and R[0]["effect"]["status"] == "outside_window_unknown"
rc2, R2, _, out2 = make("norun_partial", run=False)
ok2 = rc2 == 0 and R2 and R2[0]["order"]["status"] == "static_only" and R2[0]["tool_verdict"]["headline"] == "순서 의존 후보"
case("no run -> order static_only; derived -> violation candidate; partial -> 순서 의존 후보; effect outside_window_unknown", ok1 and ok2,
     "%s / %s" % (ax(R[0]) if R else out[-200:], ax(R2[0]) if R2 else out2[-200:]))
# --- use_cond not_derived
rc, R, _, out = make("notderived", row=ROW_NOTDERIVED)
ok = rc == 0 and R and R[0]["use_cond"]["status"] == "not_derived" and R[0]["use_cond"]["missing"]
case("use_cond not_derived (no failure constant, no flip, no handling, no writer)", ok, str(ax(R[0])) if R else out[-300:])
# --- value-missing
E = base_events(use_status="-")
rc, R, _, out = make("value_missing", events=E)
ok = rc == 0 and R and need_state(R[0], 11) == "value-missing" and R[0]["order"]["status"] == "record_insufficient" \
    and "record_insufficient(value)" in R[0]["order"]["missing"]
case("need value-missing -> order record_insufficient(value)", ok, str((need_state(R[0], 11), ax(R[0]))) if R else out[-300:])
# --- text-partial
rc, R, _, out = make("text_partial", events=base_events(trunc=True))
ok = rc == 0 and R and need_state(R[0], 12) == "text-partial" and "record_insufficient(key text)" in R[0]["link"]["missing"] \
    and R[0]["link"]["status"] == "unlinked"
case("need text-partial -> link unlinked, record_insufficient(key text)", ok, str((need_state(R[0], 12), R[0]["link"]["missing"])) if R else out[-300:])
# --- not-reached (the consumer actor exists, no use event)
rc, R, _, out = make("not_reached", events=base_events(use=False))
ok = rc == 0 and R and need_state(R[0], 10) == "not-reached" and R[0]["order"]["status"] == "record_insufficient" \
    and "target_not_reached" in R[0]["order"]["missing"] and R[0]["effect"]["status"] == "outside_window_unknown"
case("need not-reached -> order record_insufficient(target_not_reached), effect outside_window_unknown", ok, str(ax(R[0])) if R else out[-300:])
# --- unknown-loss
rc, R, _, out = make("unknown_loss", events=base_events(use=False), pool_dropped=1)
ok = rc == 0 and R and need_state(R[0], 10) == "unknown-loss" and "record_insufficient(loss marker)" in R[0]["order"]["missing"]
case("need unknown-loss (a loss marker, no event) -> record_insufficient(loss marker)", ok, str(need_state(R[0], 10)) if R else out[-300:])
# --- unbound
rc, R, _, out = make("unbound", unbind=(12,))
ok = rc == 0 and R and need_state(R[0], 12) == "unbound"
case("need unbound (no binding of a planned site)", ok, str(need_state(R[0], 12)) if R else out[-300:])
# --- effect none_in_window
rc, R, _, out = make("none_in_window", events=base_events(after_use=False))
ok = rc == 0 and R and R[0]["effect"]["status"] == "none_in_window" and R[0]["effect"]["missing"]
case("effect none_in_window (no handling or stop event after the use)", ok, str(ax(R[0])) if R else out[-300:])
# --- no boot budget record given: budget.run names it missing
rc, R, _, out = make("no_budget", budget=False)
ok = rc == 0 and R and R[0]["budget"]["run"].get("missing") == ["record_insufficient(no budget record of the boot given)"]
case("no boot budget record -> budget.run missing with a reason", ok, str(R[0]["budget"]["run"]) if R else out[-300:])
# --- an instance list run.meta does not name -> combination refused
rc, R, _, out = make("list_not_named", list_named=False)
ok = rc == 3 and R and "the instance list is not the one run.meta names" in R[0]["build_id"]["compat"]
case("an instance list run.meta does not name -> combination refused, rc 3", ok, "rc %d %s" % (rc, R[0]["build_id"]["compat"] if R else out[-300:]))
# --- build-mismatch
rc, R, _, out = make("mismatch", mismatch=True)
ok = rc == 3 and R and all(n["state"] == "build-mismatch" for n in R[0]["incompleteness"]["needs"] if n["need"].startswith("site ")) \
    and R[0]["build_id"]["compat"].startswith("refused") and R[0]["order"]["use_events"] == []
case("need build-mismatch (the manifest names another sidecar) -> combination refused, rc 3", ok,
     "rc %d %s" % (rc, R[0]["build_id"]["compat"] if R else out[-300:]))
# --- JR 3.6 rules: each mutation of the control record is refused by --check
with open(os.path.join(CONTROL, "out", "records", sorted(os.listdir(os.path.join(CONTROL, "out", "records")))[0])) as f:
    base = json.load(f)


def check(rec, tag):
    p = os.path.join(WORK, "check_%s.json" % tag)
    with open(p, "w") as f:
        json.dump(rec, f)
    q = subprocess.run(["python3", "-I", EXPLAIN, "--check", p, "--vocabulary", VOCAB], capture_output=True, text=True)
    return q.returncode, q.stdout


rc0, _ = check(base, "control")
muts = []
m = copy.deepcopy(base); m["tool_verdict"]["violation"]["value"] = "observed"; muts.append(("1", m))
m = copy.deepcopy(base); m["tool_verdict"]["violation"]["value"] = "observed"; m["use_cond"]["status"] = "derived"
for k in ("callee_check", "reach", "caller_dependence"):
    m["use_cond"][k]["sources"] = [{"kind": "static-row", "ref": "x:1"}]
for u in m["order"]["use_events"]:
    u["key"]["text"] = None; u["out"] = {"read": False}
muts.append(("2", m))
m = copy.deepcopy(base); m["use_cond"]["callee_check"]["sources"].append({"kind": "record-return", "ref": "dump"}); muts.append(("3", m))
m = copy.deepcopy(base); m["tool_verdict"]["odf"]["value"] = "shown"; m["intervention"]["plan_origin"] = "manual"; muts.append(("4", m))
m = copy.deepcopy(base); m["tool_verdict"]["inputs"].append("handling_label"); muts.append(("5", m))
m = copy.deepcopy(base); m["tool_verdict"]["inputs"].append("review.result"); muts.append(("6", m))
m = copy.deepcopy(base); m["use_cond"]["missing"] = []; muts.append(("7", m))
m = copy.deepcopy(base)
m["order"]["per_consumer"]["item2_uses_before_w_all_fail"]["events"].append({"actor": "9", "aseq": 1}); muts.append(("8", m))
m = copy.deepcopy(base); m["link"]["missing"] = ["no_such_reason(x)"]; muts.append(("3.5", m))
m = copy.deepcopy(base); m["incompleteness"]["needs"][-1]["reason"] = "plan_missing(need"; muts.append(("3.5", m))
allok = rc0 == 0
det = ["control rc %d" % rc0]
for tag, m in muts:
    rc, txt = check(m, "rule" + tag)
    want = ("JR 3.6 item %s" % tag) if tag != "3.5" else "JR 3.5"
    good = rc == 4 and want in txt
    allok = allok and good
    det.append("%s rc %d" % (tag, rc))
case("JR 3.6 items 1-8 and the JR 3.5 vocabulary: each broken record refused by --check (rc 4)", allok, "; ".join(det))
# --- the pipeline refuses too: an out directory that exists
rc, _, _, _ = make("exists_once")
q = subprocess.run(["python3", "-I", EXPLAIN, "--sidecar", "x", "--manifest", "x", "--bindings", "x", "--relations", "x", "--keys", "x",
                    "--vocabulary", VOCAB, "--facts-view", "x", "--bprime-basis", "x", "--build-label", "x",
                    "--out", os.path.join(WORK, "exists_once", "out")], capture_output=True, text=True)
case("an existing out directory -> rc 64", q.returncode == 64, "rc %d" % q.returncode)
nf = sum(1 for _, ok in RESULTS if not ok)
print("explain fixtures: %d of %d pass" % (len(RESULTS) - nf, len(RESULTS)))
sys.exit(1 if nf else 0)
