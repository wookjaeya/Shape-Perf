#!/usr/bin/env python3
"""CORD analyzer: find observed and predicted order violations in a CORD trace.

Ordering model (see tool/DESIGN.md §3): happens-before is program order plus
  - TCREATE(token) -> TSTART(token)
  - SIG(o) -> WAIT(o), matched FIFO per sync object
  - SETSTATE(v, x) -> WAITRET(v, x) (reads-from on designated state variables)
Lock release -> acquire is not an ordering edge, unless --locks-hb is given
(ablation).

For every USE(k) the analyzer looks at the first PUB(k) in the run:
  - no PUB before the USE              -> observed violation (USE ran first)
  - PUB before USE, but not PUB -> USE -> predicted violation (some feasible
                                          schedule runs the USE first)
  - PUB -> USE                         -> ordered (with the edge kind recorded)

  python3 cord_analyze.py TRACE.jsonl [--json OUT.json] [--locks-hb] [--key-prefix P]
"""
import argparse
import collections
import json
import sys

K = {1: "PUB", 2: "USE", 3: "TCREATE", 4: "TSTART", 5: "SIG", 6: "WAIT", 7: "SETSTATE",
     8: "WAITRET", 9: "LOCK", 10: "UNLOCK", 11: "NOTE", 12: "TASKNAME"}


def load(path):
    meta, evs = [], []
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            ln = ln.strip()
            if not ln:
                continue
            try:
                j = json.loads(ln)
            except json.JSONDecodeError:
                continue
            if "meta" in j:
                meta.append(j)
            else:
                j["kind"] = K.get(j["k"], str(j["k"]))
                evs.append(j)
    evs.sort(key=lambda e: e["seq"])
    return meta, evs


def vc_join(a, b):
    for t, c in b.items():
        if a.get(t, -1) < c:
            a[t] = c


def analyze(evs, locks_hb=False, key_prefix=None):
    vc = collections.defaultdict(dict)          # tid -> vector clock
    names = {}
    tokens = {}                                 # TCREATE token -> VC snapshot
    sigq = collections.defaultdict(collections.deque)
    states = collections.defaultdict(list)      # key -> [(seq, value, vc)]
    lastrel = {}
    edges = collections.Counter()
    for e in evs:
        t = e["tid"]
        v = vc[t]
        v[t] = v.get(t, 0) + 1
        k, key = e["kind"], e["key"]
        if k == "TASKNAME":
            names[t] = key
        elif k == "TCREATE":
            tokens[e["a"]] = dict(v)
        elif k == "TSTART":
            if e["a"] in tokens:
                vc_join(v, tokens[e["a"]])
                edges["create"] += 1
        elif k == "SIG":
            sigq[key].append(dict(v))
        elif k == "WAIT":
            if sigq[key]:
                vc_join(v, sigq[key].popleft())
                edges["signal"] += 1
        elif k == "SETSTATE":
            states[key].append((e["seq"], e["a"], dict(v)))
        elif k == "WAITRET":
            cand = [s for s in states[key] if s[0] < e["seq"]]
            exact = [s for s in cand if s[1] == e["a"]]
            src = (exact or cand or [None])[-1]
            if src:
                vc_join(v, src[2])
                edges["state"] += 1
        elif k == "UNLOCK" and locks_hb:
            lastrel[key] = dict(v)
        elif k == "LOCK" and locks_hb:
            if key in lastrel:
                vc_join(v, lastrel[key])
                edges["lock"] += 1
        e["vc"] = dict(v)

    pubs = collections.defaultdict(list)
    for e in evs:
        if e["kind"] == "PUB":
            pubs[e["key"]].append(e)
    findings = []
    for u in evs:
        if u["kind"] != "USE":
            continue
        key = u["key"]
        if key_prefix and not key.startswith(key_prefix):
            continue
        ps = pubs.get(key, [])
        before = [p for p in ps if p["seq"] < u["seq"]]
        rec = {"key": key, "use": site(u), "use_task": names.get(u["tid"], str(u["tid"])),
               "use_status": u["a"], "use_seq": u["seq"], "use_t": u["t"]}
        if not ps:
            rec["verdict"] = "no_publication_in_run"
        elif not before:
            p = ps[0]
            rec.update(pub=site(p), pub_task=names.get(p["tid"], str(p["tid"])), pub_seq=p["seq"],
                       gap_ms=(p["t"] - u["t"]) / 1e6, verdict="observed_violation")
        else:
            p = before[0]
            rec.update(pub=site(p), pub_task=names.get(p["tid"], str(p["tid"])), pub_seq=p["seq"],
                       gap_ms=(u["t"] - p["t"]) / 1e6)
            if p["tid"] == u["tid"]:
                rec["verdict"] = "same_task"
            elif u["vc"].get(p["tid"], 0) >= p["vc"][p["tid"]]:
                rec["verdict"] = "ordered"
            else:
                rec["verdict"] = "predicted_violation"
        findings.append(rec)
    return findings, names, edges


def site(e):
    return f'{e["file"]}:{e["line"]} {e["func"]}'


def summarize(findings):
    groups = collections.OrderedDict()
    for f in findings:
        g = (f["verdict"], f["key"].split(":")[0], f.get("pub", "-"), f["use"], f.get("pub_task", "-"), f["use_task"])
        if g not in groups:
            groups[g] = {"verdict": g[0], "class": g[1], "pub": g[2], "use": g[3], "pub_task": g[4],
                         "use_task": g[5], "keys": [], "count": 0, "statuses": collections.Counter(),
                         "min_gap_ms": None}
        r = groups[g]
        r["count"] += 1
        r["statuses"][f["use_status"]] += 1
        if f["key"] not in r["keys"] and len(r["keys"]) < 5:
            r["keys"].append(f["key"])
        gap = f.get("gap_ms")
        if gap is not None and (r["min_gap_ms"] is None or gap < r["min_gap_ms"]):
            r["min_gap_ms"] = gap
    out = list(groups.values())
    for r in out:
        r["statuses"] = dict(r["statuses"])
    order = {"observed_violation": 0, "predicted_violation": 1, "no_publication_in_run": 2, "ordered": 3, "same_task": 4}
    out.sort(key=lambda r: (order.get(r["verdict"], 9), -r["count"]))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("trace")
    ap.add_argument("--json")
    ap.add_argument("--locks-hb", action="store_true")
    ap.add_argument("--key-prefix")
    ap.add_argument("--all", action="store_true", help="also print ordered/same-task groups")
    a = ap.parse_args()
    meta, evs = load(a.trace)
    findings, names, edges = analyze(evs, a.locks_hb, a.key_prefix)
    groups = summarize(findings)
    print(f"events={len(evs)} tasks={len(set(e['tid'] for e in evs))} edges={dict(edges)} meta={meta}")
    for r in groups:
        if not a.all and r["verdict"] in ("ordered", "same_task"):
            continue
        print(f'[{r["verdict"]}] x{r["count"]} {r["class"]} keys={r["keys"]} status={r["statuses"]} gap_ms={r["min_gap_ms"]}\n'
              f'    pub: {r["pub_task"]} {r["pub"]}\n    use: {r["use_task"]} {r["use"]}')
    counts = collections.Counter(r["verdict"] for r in groups)
    print("groups:", dict(counts))
    if a.json:
        with open(a.json, "w") as f:
            json.dump({"meta": meta, "edges": dict(edges), "tasks": names, "groups": groups,
                       "findings": findings}, f, indent=1)


if __name__ == "__main__":
    sys.exit(main())
