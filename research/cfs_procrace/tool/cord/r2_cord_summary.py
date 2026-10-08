#!/usr/bin/env python3
"""Case R2 (cFE #2663) summary of one CORD trace from the instrumented copy (tool/cord/patches/cfe_v701_r2.patch).

Uses cord_analyze.py's own loader and happens-before model (no second model), and adds what the R2 comparison with
the gdb result (results/R2_gdb_v701.md) needs:
  - verdict counts per key (sb.appid, sb.evsreg) and per using task, and the violating uses by SB function;
  - the outcome of each use: the next EVS NOTE on the same thread before that thread's next USE
    (evs.illegal_appid / evs.not_registered / none = accepted by EVS);
  - for every use judged "ordered", the happens-before chain that carries PUB to the USE, reconstructed from the
    analyzer's vector clocks (earliest event of each thread that knows the PUB, and the edge that brought it);
  - for every "predicted_violation", what the using thread knows of the publishing thread, to name the missing edge;
  - recorder meta lines (threads, overflow, dropped events).

  python3 r2_cord_summary.py TRACE.jsonl [--json OUT.json]
"""
import argparse
import collections
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cord_analyze as ca  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("trace")
    ap.add_argument("--json")
    a = ap.parse_args()
    meta, evs = ca.load(a.trace)
    findings, names, edges = ca.analyze(evs, key_prefix="sb.")
    by_seq = {e["seq"]: e for e in evs}
    per_tid = collections.defaultdict(list)
    for e in evs:
        per_tid[e["tid"]].append(e)
    name = lambda tid: names.get(tid, "tid%d" % tid)  # noqa: E731

    out = {"trace": os.path.basename(a.trace), "meta": meta, "events": len(evs), "threads_with_events": len(per_tid),
           "edges": dict(edges), "kinds": dict(collections.Counter(e["kind"] for e in evs))}
    print(f"trace {out['trace']}: {len(evs)} events, {len(per_tid)} threads with events, kinds {out['kinds']}")
    print(f"recorder meta: {meta}")
    dropped = [m for m in meta if m.get("meta") == 2]
    out["dropped"] = dropped
    print(f"dropped-event lines: {len(dropped)} {dropped if dropped else ''}")
    print(f"HB edges used by the analyzer: {dict(edges)}")

    pubs = {}
    for e in evs:
        if e["kind"] == "PUB" and e["key"] not in pubs:
            pubs[e["key"]] = e
    for k in ("sb.appid", "sb.evsreg"):
        p = pubs.get(k)
        print(f"PUB {k}: " + (f"seq {p['seq']} task {name(p['tid'])} value a={p['a']} at {p['file']}:{p['line']}"
                              if p else "NOT RECORDED"))
    out["pubs"] = {k: {"seq": p["seq"], "task": name(p["tid"]), "a": p["a"], "t": p["t"]} for k, p in pubs.items()}
    npub = sum(1 for e in evs if e["kind"] == "PUB" and e["key"] == "sb.appid")
    print(f"PUB sb.appid count: {npub}")

    # outcome of each USE: next NOTE evs.* on the same thread before the thread's next USE sb.appid
    outcome = {}
    for tid, lst in per_tid.items():
        pending = []
        for e in lst:
            if e["kind"] == "USE" and e["key"] == "sb.appid":
                for s in pending:
                    outcome[s] = "accepted"
                pending = [e["seq"]]
            elif e["kind"] == "NOTE" and e["key"].startswith("evs.") and pending:
                outcome[pending[0]] = e["key"]
                pending = []
        for s in pending:
            outcome[s] = "accepted"

    for f in findings:
        if f["key"] == "sb.appid":
            f["outcome"] = outcome.get(f["use_seq"], "?")
    res = {}
    for key in ("sb.appid", "sb.evsreg"):
        fs = [f for f in findings if f["key"] == key]
        vc = collections.Counter(f["verdict"] for f in fs)
        bytask = collections.Counter((f["verdict"], f["use_task"]) for f in fs)
        res[key] = {"verdicts": dict(vc), "by_task": {f"{v}|{t}": n for (v, t), n in sorted(bytask.items())}}
        print(f"\n== {key}: {len(fs)} uses; verdicts {dict(vc)}")
        for (v, t), n in sorted(bytask.items()):
            print(f"   {v:22s} {t:22s} {n}")
    out["by_key"] = res

    viol = [f for f in findings if f["key"] == "sb.appid" and f["verdict"] == "observed_violation"]
    g = collections.Counter((f["use_task"], f["use"].split()[1], f["use_status"], f["outcome"]) for f in viol)
    print(f"\n== sb.appid uses before the publication: {len(viol)}")
    for (t, fn, st, oc), n in sorted(g.items()):
        print(f"   {n:3d}  task={t:10s} sbfunc={fn:40s} AppId(a)={st} outcome={oc}")
    out["violations"] = [{"task": t, "sb_function": fn, "appid": st, "outcome": oc, "count": n}
                         for (t, fn, st, oc), n in sorted(g.items())]
    if viol and "sb.appid" in pubs:
        last = max(viol, key=lambda f: f["use_seq"])
        gap = (pubs["sb.appid"]["t"] - last["use_t"]) / 1e6
        print(f"   last violating use -> PUB sb.appid: {gap:.3f} ms (monotonic clock)")
        out["last_violation_to_pub_ms"] = gap
    between = [f for f in findings if f["key"] == "sb.evsreg" and f["verdict"] == "observed_violation"
               and "sb.appid" in pubs and f["use_seq"] > pubs["sb.appid"]["seq"]]
    print(f"   uses after PUB sb.appid but before PUB sb.evsreg: {len(between)}")
    out["between_pubs"] = len(between)

    oc_all = collections.Counter(f["outcome"] for f in findings if f["key"] == "sb.appid")
    notes = collections.Counter((e["key"], e["a"]) for e in evs if e["kind"] == "NOTE")
    print(f"\n== outcomes of sb.appid uses: {dict(oc_all)}")
    print(f"== NOTE events (key, a): {dict(notes)}")
    out["outcomes"] = dict(oc_all)
    out["notes"] = {f"{k}|{v}": n for (k, v), n in notes.items()}

    # HB chain for ordered uses
    def first_knowing(tid, ptid, clock, upto_seq):
        for e in per_tid[tid]:
            if e["seq"] > upto_seq:
                break
            if e["vc"].get(ptid, 0) >= clock:
                return e
        return None

    memo = {}

    def source_of(e):
        if e["seq"] not in memo:
            memo[e["seq"]] = _source_of(e)
        return memo[e["seq"]]

    def _source_of(e):
        if e["kind"] == "TSTART":
            cands = [x for x in evs if x["kind"] == "TCREATE" and x["key"] == e["key"] and x["seq"] < e["seq"]]
            return cands[-1] if cands else None
        if e["kind"] == "WAITRET":
            cand = [x for x in evs if x["kind"] == "SETSTATE" and x["key"] == e["key"] and x["seq"] < e["seq"]]
            exact = [x for x in cand if x["a"] == e["a"]]
            src = (exact or cand or [None])[-1]
            return src
        return None

    def chain(u, p):
        steps, cur, guard = [], u, 0
        ptid, clock = p["tid"], p["vc"][p["tid"]]
        while cur["tid"] != ptid and guard < 20:
            guard += 1
            e = first_knowing(cur["tid"], ptid, clock, cur["seq"])
            if e is None:
                return None
            src = source_of(e)
            steps.append(f"{e['kind']}({e['key']})@{name(e['tid'])}")
            if src is None:
                steps.append("?")
                return steps
            steps.append(f"{src['kind']}({src['key']}{'='+str(src['a']) if src['kind']=='SETSTATE' else ''})@{name(src['tid'])}")
            cur = src
        return list(reversed(steps))

    ordered = [f for f in findings if f["key"] == "sb.appid" and f["verdict"] == "ordered"]
    templ = collections.Counter()
    templ_tasks = collections.defaultdict(set)
    p = pubs.get("sb.appid")
    for f in ordered:
        u = by_seq[f["use_seq"]]
        ch = chain(u, p)
        # abstract the use-thread specific final TCREATE/TSTART keys into a template
        t = " -> ".join(ch) if ch else "NO CHAIN"
        tt = t
        if ch and len(ch) >= 2 and ch[-1].startswith("TSTART("):
            tt = " -> ".join(ch[:-2] + ["TCREATE(task:<X>)@" + ch[-2].split("@")[1], "TSTART(task:<X>)@<X>"])
        templ[tt] += 1
        templ_tasks[tt].add(f["use_task"])
    print(f"\n== happens-before chains carrying PUB sb.appid to the {len(ordered)} 'ordered' uses (templates):")
    out["ordered_chains"] = []
    for t, n in templ.most_common():
        print(f"   {n:4d}  {t}\n         use tasks: {sorted(templ_tasks[t])}")
        out["ordered_chains"].append({"count": n, "chain": t, "use_tasks": sorted(templ_tasks[t])})

    pred = [f for f in findings if f["verdict"] == "predicted_violation"]
    print(f"\n== predicted violations: {len(pred)}")
    out["predicted"] = []
    for f in pred:
        u = by_seq[f["use_seq"]]
        p = pubs[f["key"]]
        lst = per_tid[u["tid"]]
        has_tstart = any(e["kind"] == "TSTART" for e in lst if e["seq"] < u["seq"])
        kinds_before = collections.Counter(e["kind"] for e in lst if e["seq"] < u["seq"])
        rec = {"key": f["key"], "use_task": f["use_task"], "use": f["use"], "pub_task": f["pub_task"],
               "use_knows_pub_clock": u["vc"].get(p["tid"], 0), "pub_clock": p["vc"][p["tid"]],
               "use_thread_has_tstart": has_tstart, "use_thread_kinds_before": dict(kinds_before),
               "gap_ms": f.get("gap_ms")}
        out["predicted"].append(rec)
        print(f"   {rec}")
    allv = collections.Counter((f["key"], f["verdict"]) for f in findings)
    out["verdicts_all"] = {f"{k}|{v}": n for (k, v), n in allv.items()}
    if a.json:
        with open(a.json, "w") as fh:
            json.dump(out, fh, indent=1)


if __name__ == "__main__":
    sys.exit(main())
