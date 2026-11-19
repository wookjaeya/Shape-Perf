#!/usr/bin/env python3
"""synth_record.py -- a SYNTHETIC record of use events for a dry run of the explanation script on a real C1 plan
(explanation spec 11 item 16; results 26.0 (b)).  It is a test input, never evidence, and is kept under a test name.

    python3 -I synth_record.py --manifest plan.kzs --sidecar plan.side --relations relations.txt --pairs pairs.tsv \
        --bindings binds.kzs --startup-script cfe_es_startup.scr --order script|reverse --out NEW_DIR

Rule (no row id, key, case name or expected verdict is an input; every value comes from the plan, the relations text,
the pair file, the binding file and the startup script):
  actors    the process initial thread (key tid:100) and one task actor per root that a planned row's pair-file lines
            name as the use root or a creator root ('main (process)' is the initial thread; a root text that names no
            single root, such as '-' or a set in braces, gets no actor); the framework roots
            (table-image-entry) start first, then the startup script's applications in its order (--order script) or
            in the reverse order (--order reverse); other roots after them.  The initial thread makes each task actor
            through the plan's actor-create sites (creation argument 0xA000+k) and the actor runs the actor-start and
            its actor-root entry sites first.
  calls     for every planned row (sidecar state=planned or control-state=planned): every creator-call site pair
            (at-call, after-call of one call) in the actor of the creator root the pair file gives for its position,
            and every use-call site pair in the actor of the use root; inside a call the row's key-text and kind-value
            sites whose function is the call's callee.  Within an actor the calls are in the order (source file, line).
            A row with context lines has its use inside the frame of its first context (--order script) or its last
            (--order reverse).
  values    a creator's key text is the first quoted string of the relations creator line at its position; a use's key
            text is the first such key text of the row's creator lines (so that every use has a create of its name in
            the record); the kind value is the row's partition; a create returns the success value of the row's
            success-path line of its callee (0 when the line gives none) and a new id; a use is found (the row's
            non-failure constant of its flip line, the id) when a create of the same partition and key text ended
            before it in the record, else absent (the row's absence value).  Only sites bound in the binding file get
            events.  No lifetime end, effect or stop event is written.
  output    NEW_DIR/tx.dump.txt (kzrt-dump text 1.5 with buildid lines of main and module 1), run.meta (the manifest's
            sha256), chain.txt (stand-in lines: chain and both same_build_object lines confirmed), startup.scr (a copy).
"""
import argparse
import hashlib
import os
import re
import shutil
import sys

KV = re.compile(r'(\w[\w-]*)=("(?:[^"\\]|\\.)*"|\S+)')


def kv(t):
    return {k: (v[1:-1] if v.startswith('"') else v) for k, v in KV.findall(t)}


def sha(p):
    with open(p, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()


def hx64(v):
    return "0x%x" % (v & 0xFFFFFFFFFFFFFFFF)


def main():
    ap = argparse.ArgumentParser()
    for a in ("manifest", "sidecar", "relations", "pairs", "bindings", "startup-script", "order", "out"):
        ap.add_argument("--" + a, required=True)
    o = ap.parse_args()
    if o.order not in ("script", "reverse") or os.path.exists(o.out):
        print("synth_record.py: --order script|reverse and a new --out", file=sys.stderr)
        return 64
    sites, vals = {}, {}
    for ln in open(o.manifest, encoding="utf-8"):
        if ln.startswith("site "):
            p = ln.split(" ", 2)
            a = kv(p[2])
            a["rules"] = a.get("label", "").split(".", 1)[-1].split("+")
            a["entry"] = a["op"][len("entry."):] if a.get("op", "").startswith("entry.") else None
            sites[int(p[1])] = a
        elif ln.startswith("value "):
            p = ln.split()
            vals.setdefault(int(p[1]), {})[int(p[2])] = kv(ln).get("role")
    bound = set()
    for ln in open(o.bindings, encoding="utf-8"):
        if ln.startswith("bind "):
            a = kv(ln)
            if a.get("status") in ("bound", "duplicated", "inlined") and a.get("site", "").isdigit():
                bound.add(int(a["site"]))
    rows, maps, ctx = {}, {}, {}
    for ln in open(o.sidecar, encoding="utf-8"):
        if ln.startswith("row ") and (" state=planned" in ln or " control-state=planned" in ln):
            p = ln.split()
            rows[p[1]] = p[2]
        elif ln.startswith("map "):
            p = ln.split()
            maps[p[1]] = [int(x) for x in p[2:]]
        m = re.match(r"context row=(\S+) id=(\d+) frame=(\d+),(\d+)", ln)
        if m:
            ctx.setdefault(m.group(1), []).append((int(m.group(3)), int(m.group(4))))
    rel = {}
    cur = None
    for ln in open(o.relations, encoding="utf-8"):
        m = re.match(r"^  ([ABC]\d+) ", ln)
        if m:
            cur = m.group(1) if m.group(1) in rows.values() else None
            if cur:
                mp = re.search(r"partition \{(\d*)\}", ln)
                rel[cur] = {"partition": int(mp.group(1)) if mp and mp.group(1) else 0, "creators": [], "success": {}, "absence": None, "ok": None}
            continue
        if not ln.startswith("      "):
            cur = None
            continue
        if cur is None:
            continue
        t = ln.strip()
        if t.startswith("creator:"):
            mp = re.search(r"(\S+:\d+:\d+) in ", t)
            mk = re.search(r'"([^"]*)"', t)
            rel[cur]["creators"].append((mp.group(1) if mp else None, mk.group(1) if mk else None))
        elif t.startswith("callee-check (success path): "):
            mm = re.match(r"callee-check \(success path\): (\S+): \S+: success value (-?\d+)", t)
            if mm:
                rel[cur]["success"][mm.group(1)] = int(mm.group(2))
        elif t.startswith("absence: "):
            mm = re.match(r"absence: (-?\d+) ", t)
            rel[cur]["absence"] = int(mm.group(1)) if mm else None
        elif t.startswith("flip: "):
            cs = [int(v) for v in re.findall(r"returns (-?\d+) \[", t)]
            ok = [v for v in cs if not (v < 0 or 0xC0000000 <= v <= 0xFFFFFFFF)]
            rel[cur]["ok"] = ok[0] if ok else 0
    pairs = {}
    cols = None
    for ln in open(o.pairs, encoding="utf-8"):
        ln = ln.rstrip("\n")
        if ln.startswith("columns "):
            cols = ln[len("columns "):].split("\t")
            continue
        f = ln.split("\t")
        if cols and len(f) == len(cols):
            d = dict(zip(cols, f))
            if d["row_key"] in rows:
                pairs.setdefault(d["row_key"], []).append(d)
    apps = []
    for ln in open(o.startup_script, encoding="utf-8"):
        if ln.startswith("!"):
            break
        f = [x.strip() for x in ln.split(",")]
        if len(f) >= 4 and f[0] == "CFE_APP":
            apps.append(f[2])
    if o.order == "reverse":
        apps = apps[::-1]
    root_site = {}
    for n, s in sorted(sites.items()):
        if "actor-root" in s["rules"] and n in bound:
            m = re.search(r"actor (\S+) \(([^)]*)\)", s.get("ref", ""))
            if m:
                root_site.setdefault("%s (%s)" % (m.group(1), m.group(2)), n)
    create_at = [n for n, s in sorted(sites.items()) if "actor-create" in s["rules"] and s.get("place") == "at-call" and n in bound]
    create_after = [n for n, s in sorted(sites.items()) if "actor-create" in s["rules"] and s.get("place") == "after-call" and n in bound]
    start = [n for n, s in sorted(sites.items()) if "actor-start" in s["rules"] and n in bound]

    def tail(pos, s):
        return pos is not None and pos.endswith("%s:%s:%s" % (s.get("tu"), s.get("line"), s.get("col")))

    def call_pairs(key, rule):
        ats = [n for n in maps.get(key, []) if rule in sites[n]["rules"] and sites[n].get("place") == "at-call"]
        out = []
        for a in ats:
            s = sites[a]
            af = [n for n in maps.get(key, []) if rule in sites[n]["rules"] and sites[n].get("place") == "after-call" and
                  all(sites[n].get(k) == s.get(k) for k in ("tu", "func", "callee", "line", "col"))]
            out.append((a, af[0] if af else None))
        return out
    calls = {}      # root -> list of call dicts
    for key, label in sorted(rows.items()):
        R = rel.get(label)
        if R is None:
            continue
        keys = [k for _, k in R["creators"] if k is not None]
        for a, af in call_pairs(key, "creator-call"):
            s = sites[a]
            roots = sorted({d["other_root"] for d in pairs.get(key, []) if tail(d["other_pos"], s)})
            ktxt = next((k for p, k in R["creators"] if p and p == "%s:%s:%s" % (s["tu"], s["line"], s["col"]) or tail(p, s)), None)
            for r in roots:
                calls.setdefault(r, []).append({"kind": "create", "row": key, "at": a, "after": af, "key": ktxt, "R": R, "s": s})
        for a, af in call_pairs(key, "use-call"):
            s = sites[a]
            roots = sorted({d["use_root"] for d in pairs.get(key, []) if d["use_pos"].endswith("%s:%s:%s" % (s["tu"], s["line"], s["col"]))
                            or tail(d["use_pos"], s)})
            for r in roots:
                cx = ctx.get(key, [])
                fr = (cx[0] if o.order == "script" else cx[-1]) if cx else None
                calls.setdefault(r, []).append({"kind": "use", "row": key, "at": a, "after": af, "key": keys[0] if keys else None, "R": R, "s": s,
                                                "frame": fr})
    for r in [r for r in calls if not re.fullmatch(r"\S+ \([^()]+\)", r)]:   # a pair-file root that names no single root ('-', a set)
        del calls[r]
    core = sorted(r for r in calls if r.endswith("(table-image-entry)"))
    appr = [r for a in apps for r in calls if r == "%s (script-entry)" % a]
    rest = sorted(r for r in calls if r not in core and r not in appr and not r.endswith("(process)"))
    order = core + appr + rest
    T = [1000000]
    ev = []
    aseq = {}

    def emit(actor, site, values=None, s_=None):
        if site is None or site not in bound:
            return
        aseq[actor] = aseq.get(actor, 0) + 1
        T[0] += 1000
        roles = vals.get(site, {})
        v = [values.get(roles[i], 0) if values else 0 for i in sorted(roles)]
        ev.append((actor, aseq[actor], site, T[0], v, s_))
    keyof = {"1": "tid:100"}
    created = {}
    nid = [0x10000]

    def run_calls(actor, cl):
        for c in sorted(cl, key=lambda c: (c["s"].get("tu", ""), int(c["s"].get("line", "0") or 0))):
            R, s = c["R"], c["s"]
            inner = [n for n in maps.get(c["row"], []) if ({"key-text", "kind-value"} & set(sites[n]["rules"])) and sites[n].get("func") == s.get("callee")]
            ptr = 0x7000 + int(actor)
            if c["kind"] == "use" and c["frame"]:
                emit(actor, c["frame"][0])
            emit(actor, c["at"], {"key": ptr, "arg0": ptr, "arg1": ptr})
            for n in inner:
                emit(actor, n, {"key_text": ptr, "kind": R["partition"], "arg0": R["partition"], "arg1": ptr}, c["key"])
            if c["kind"] == "create":
                nid[0] += 1
                st = R["success"].get(s.get("callee"), 0)
                emit(actor, c["after"], {"status": st, "out0": nid[0], "out2": nid[0]})
                if c["key"] is not None:
                    created[(R["partition"], c["key"])] = nid[0]
            else:
                hit = created.get((R["partition"], c["key"]))
                if hit is not None:
                    emit(actor, c["after"], {"status": R["ok"] or 0, "out0": hit})
                else:
                    emit(actor, c["after"], {"status": R["absence"] if R["absence"] is not None else -1, "out0": 0})
            if c["kind"] == "use" and c["frame"]:
                emit(actor, c["frame"][1])
    if "main (process)" in calls:
        run_calls("1", calls["main (process)"])
    for k, r in enumerate(order, 2):
        a = str(k)
        keyof[a] = "tid:%d" % (100 + k)
        arg = 0xA000 + k
        for n in create_at:
            emit("1", n, {"creation_arg": arg, "start_routine": 0x5000, "arg0": arg})
        for n in create_after:
            emit("1", n, {"status": 0, "out0": arg})
        for n in start:
            emit(a, n, {"param0": arg})
        emit(a, root_site.get(r), {"param0": arg})
        run_calls(a, calls[r])
    msha = sha(o.manifest)
    os.makedirs(o.out)
    lines = ["# synthetic record of use events for a dry run (records/closure-explain-fix-20261119/scripts/synth_record.py, order %s): a test input, never evidence" % o.order,
             'run abi=1 mode=stopped run_id=1 instance=1 clock=1 runtime="synthetic"', "run manifest_sha256=%s" % msha,
             "run t_init=0 t_attach=0 t_stop=%d t_dump=%d" % (T[0] + 1000000, T[0] + 1000001), "runx pool_bytes=4194304 pool_dropped=0 boot_bytes_dropped=0",
             "runx process=100 initial=tid:100 evidence=synthetic", "module 1 actor=1 t=1 registrations=1 manifest_sha256=%s" % msha,
             'buildid object=main path="synthetic" build_id=00 note=present source=start actor=- t=1',
             'buildid object=module:1 path="synthetic" build_id=00 note=present source=registration actor=- t=2']
    for a in sorted(aseq, key=int):
        lines.append("actor %s key=%s aseq=1..%d recorded=%d overflow=0 partial=0 dropped_bytes=0" % (a, keyof[a], aseq[a], aseq[a]))
    for a, q, site, t, v, st in ev:
        lines.append("ev actor=%s aseq=%d site=%d[%s] bind=%d link=0 t=%d pc=0x0 key=%s v=[%s]%s flags=clock" % (
            a, q, site, sites[site].get("label", "s"), (1 << 16) | (site << 4), t, keyof[a], ",".join(hx64(x) for x in v),
            (' str="%s"' % st) if st is not None else ""))
    lines += ["format kzrt-dump 1.5 dump_flags=0x1 records=%d unknown=0 partial=0 pads=0 end=yes" % len(ev), "check ok (0 failures)"]
    with open(os.path.join(o.out, "tx.dump.txt"), "w") as f:
        f.write("\n".join(lines) + "\n")
    with open(os.path.join(o.out, "run.meta"), "w") as f:
        f.write("# synthetic stand-in (test input)\nmanifest plan.kzs sha256=%s\nstop=synthetic\n" % msha)
    with open(os.path.join(o.out, "chain.txt"), "w") as f:
        f.write("# synthetic stand-in chain lines (test input, not a chain check)\nchain\tconfirmed\tsynthetic\n"
                "same_build_object\tconfirmed\tmain synthetic\nsame_build_object\tconfirmed\tmodule:1 synthetic\n"
                "same_build\tconfirmed\tsynthetic: 2 objects\n")
    shutil.copyfile(o.startup_script, os.path.join(o.out, "startup.scr"))
    print("synthetic record: order %s, actors %d (%s), events %d, uses %d, creates %d" % (
        o.order, len(aseq), ", ".join(order), len(ev), sum(1 for r in calls.values() for c in r if c["kind"] == "use"),
        sum(1 for r in calls.values() for c in r if c["kind"] == "create")))
    return 0


if __name__ == "__main__":
    sys.exit(main())
