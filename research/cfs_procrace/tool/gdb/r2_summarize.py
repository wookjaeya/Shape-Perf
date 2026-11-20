#!/usr/bin/env python3
"""Summarize an R2 gdb trace (r2_trace.py output) into per-use verdicts.

A "use" is a SEND_WITH_APPID hit whose immediate caller is a cFE SB function
(the SB API reporting an event with SB's own AppId). Its outcome is the next
OUT_* hit on the same LWP before that LWP's next SEND_WITH_APPID, if any.
Verdicts (cases/mapping_v701.md §1.4; oracle per decision P4 (a), pending):
  violation_appid        use before SB_PUB (SB AppId not yet stored)
  violation_registration use after SB_PUB, before SB_EVS_REGISTERED
  ordered                use after both
The task of a hit is the outermost "*TaskMain"/"*AppMain"/"*_Main" frame in
its recorded stack, else the gdb thread name. Observed order is the gdb hit
order (seq); it is the order of breakpoint hits under gdb, not an unperturbed
schedule.

  python3 r2_summarize.py TRACE.jsonl [--json OUT.json]
"""
import argparse
import collections
import json
import sys


def task_of(rec):
    for fr in reversed(rec.get("stack") or []):
        f = fr.get("func") or ""
        if f.endswith("TaskMain") or f.endswith("AppMain") or f.endswith("_Main"):
            return f
    return rec.get("thread_name") or str(rec.get("lwp"))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("trace")
    ap.add_argument("--json")
    a = ap.parse_args()
    recs, meta = [], []
    with open(a.trace, encoding="utf-8", errors="replace") as f:
        for ln in f:
            ln = ln.strip()
            if not ln:
                continue
            try:
                j = json.loads(ln)
            except json.JSONDecodeError:
                continue
            (meta if "meta" in j else recs).append(j)
    recs.sort(key=lambda r: r["seq"])

    pub = next((r for r in recs if r["kind"] == "SB_PUB"), None)
    reg = next((r for r in recs if r["kind"] == "SB_EVS_REGISTERED"), None)
    uses, pending = [], {}
    for r in recs:
        k, lwp = r["kind"], r["lwp"]
        if k == "SEND_WITH_APPID":
            st = r.get("stack") or []
            caller = st[1]["func"] if len(st) > 1 else None
            pending.pop(lwp, None)
            if caller and caller.startswith("CFE_SB_"):
                u = {"seq": r["seq"], "t_ns": r["t_ns"], "lwp": lwp, "task": task_of(r),
                     "caller": caller, "event_id": r.get("event_id"), "app_id": r.get("app_id"),
                     "sb_appid_now": r.get("sb_appid_now"), "outcome": "accepted_or_filtered",
                     "site": " < ".join(f'{x.get("func")}:{x.get("line")}' for x in st[1:5])}
                uses.append(u)
                pending[lwp] = u
        elif k in ("OUT_ILLEGAL_APP_ID", "OUT_NOT_REGISTERED") and lwp in pending:
            pending.pop(lwp)["outcome"] = k[4:].lower()
    for u in uses:
        if pub is None or u["seq"] < pub["seq"]:
            u["verdict"] = "violation_appid"
        elif reg is None or u["seq"] < reg["seq"]:
            u["verdict"] = "violation_registration"
        else:
            u["verdict"] = "ordered"

    kinds = collections.Counter(r["kind"] for r in recs)
    print(f"hits={dict(kinds)}")
    print(f"SB_PUB: {'seq %d lwp %d %s value %s' % (pub['seq'], pub['lwp'], task_of(pub), pub.get('sb_appid')) if pub else 'NOT HIT'}")
    print(f"SB_EVS_REGISTERED: {'seq %d status %s' % (reg['seq'], reg.get('status')) if reg else 'NOT HIT'}")
    groups = collections.Counter((u["verdict"], u["outcome"], u["task"], u["caller"], u["event_id"]) for u in uses)
    for (v, o, t, c, e), n in sorted(groups.items(), key=lambda x: (x[0][0], -x[1])):
        print(f"  [{v}] outcome={o} x{n}  task={t} caller={c} event_id={e}")
    print("verdicts:", dict(collections.Counter(u["verdict"] for u in uses)))
    print("outcomes:", dict(collections.Counter(u["outcome"] for u in uses)))
    if a.json:
        with open(a.json, "w") as f:
            json.dump({"meta": meta, "hits": dict(kinds), "sb_pub": pub, "sb_evs_registered": reg,
                       "uses": uses}, f, indent=1)


if __name__ == "__main__":
    sys.exit(main())
