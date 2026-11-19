#!/usr/bin/env python3
"""review27.py -- the counts that the corrections of results 27.4 cite (review of results 27, 2026-11-19).

Reads only; run with `python3 -I`.  What it reads and writes is the README of this record directory (written first).
Inputs: --records (records/closure-c1prep-20261119), --obs1 (the kept-output directory), --builds build=s,...,
--boots N, --tools (the directory of chain_check.py, whose build_id() is imported), --out (a new directory).
Every kept file is checked before it is read: the dump text, the explanation records and eval.txt against the keep lists
of their boot, the sidecar against the sha256 the eval head names, every executable of the build record against the
record's sha256.  One difference or one missing file ends the run with rc 1 and nothing written.
It counts and copies record fields; it does not judge, does not recompute a check or an expectation, and takes no row
name or case value as input (rows come from the records, the sidecar and the committed comparison files).
"""
import argparse
import collections
import hashlib
import json
import os
import re
import sys

TABLES = {
    "r1_buildid.tsv": "dump buildid lines, the chain.txt verdict, the record path of the chain_check.py 344-346 match, "
                      "the match without a leading './', and the readelf build-id of that file in the build tree",
    "r2_found_uses.tsv": "D1-R0 use events with outcome found: out, field_value, out0, instance-link reason, creates of the id",
    "r3_eval_lines.tsv": "eval.txt: D1-R0 F4 units per path and result, D1-R0 F3 pass notes, F8a notes, slot-clear and weak lines",
    "r4_order_ground.tsv": "records per order_ground value, fixed_early or hard_before records, order.pairs entries",
    "r5_plan_paths.tsv": "sidecar: rows and planned rows per path, planned path A rows with or without a namespace-entry line",
    "r6_expect_branch.tsv": "expect_compare.txt: lines per branch, build of the line, class and result",
}


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


class Kept:
    def __init__(self):
        self.lists, self.checked, self.bad = {}, [], []

    def load(self, base, listfile):
        m = {}
        with open(listfile, encoding="utf-8") as f:
            for line in f:
                line = line.rstrip("\n")
                if line:
                    sha, rel = line.split("  ", 1)
                    m[os.path.normpath(rel)] = sha
        self.lists[base] = (m, listfile)

    def check(self, base, rel, expected=None, src=None):
        path = os.path.join(base, rel)
        if expected is None:
            m, listfile = self.lists[base]
            expected = m.get(os.path.normpath("./" + rel))
            src = listfile
        if expected is None or not os.path.isfile(path):
            self.bad.append(f"{path}\tnot in {src} or missing")
            return None
        got = sha256(path)
        if got != expected:
            self.bad.append(f"{path}\tsha256 {got} != {expected} ({src})")
            return None
        self.checked.append(f"{got}  {path}")
        return path


def lines(path):
    with open(path, encoding="utf-8", errors="replace") as f:
        return f.read().split("\n")


def chain_args(chain_txt):
    """--build-record, --build-root, --dump-text and --main from the head line of a committed chain.txt."""
    head = lines(chain_txt)[0]
    out = {}
    for k in ("build-record", "build-root", "dump-text", "main"):
        m = re.search(r"--%s (\S+)" % k, head)
        out[k] = m.group(1) if m else None
    return out


def per_boot(cnt, boots):
    return ",".join(f"{n}:{cnt.get(n, 0)}" for n in boots)


def main():
    ap = argparse.ArgumentParser()
    for k in ("--records", "--obs1", "--builds", "--tools", "--out"):
        ap.add_argument(k, required=True)
    ap.add_argument("--boots", type=int, required=True)
    a = ap.parse_args()
    if os.path.exists(a.out):
        print(f"review27: {a.out} exists; not run", file=sys.stderr)
        return 2
    sys.path.insert(0, os.path.abspath(a.tools))
    import chain_check  # noqa: E402  (its build_record() and build_id(); nothing else)
    builds = [tuple(x.split("=")) for x in a.builds.split(",")]
    boots = list(range(1, a.boots + 1))
    kept = Kept()
    D = {}
    for b, s in builds:
        for n in boots:
            bd = os.path.join(a.records, "boot", b, f"boot{n}")
            d = {"bd": bd}
            bo = os.path.join(a.obs1, f"c1cyc_{s}_boot{n}")
            ex = os.path.join(a.obs1, f"c1cyc_{s}_explain{n}")
            ev = os.path.join(a.obs1, f"c1cyc_{s}_eval{n}")
            kept.load(bo, os.path.join(bd, f"c1cyc_{s}_boot{n}.sha256"))
            kept.load(ex, os.path.join(bd, f"c1cyc_{s}_explain{n}.sha256"))
            kept.load(ev, os.path.join(bd, f"c1cyc_{s}_eval{n}.sha256"))
            ca = chain_args(os.path.join(bd, "chain.txt"))
            d["chain_args"] = ca
            d["chain"] = lines(os.path.join(bd, "chain.txt"))
            rel_dump = os.path.relpath(ca["dump-text"], bo)
            d["dump"] = kept.check(bo, rel_dump)
            d["eval"] = kept.check(ev, "eval.txt")
            tail = lines(os.path.join(bd, "eval_tail.txt"))
            side = [l.split() for l in tail if l.startswith("input sidecar ")][0]
            d["sidecar"] = kept.check(os.path.dirname(side[3]), os.path.basename(side[3]), side[2], "the eval head")
            d["branches"] = collections.OrderedDict()
            for line in lines(os.path.join(bd, "explain.txt")):
                if line.startswith("branch "):
                    br = line.split()[1]
                    idx = kept.check(ex, f"{br}/index.tsv")
                    recs = collections.OrderedDict()
                    if idx:
                        il = [l.split("\t") for l in lines(idx) if l]
                        col = il[0].index("file")
                        for r in il[1:]:
                            p = kept.check(ex, f"{br}/records/{r[col]}")
                            if p:
                                with open(p, encoding="utf-8") as f:
                                    recs[r[col]] = json.load(f)
                    d["branches"][br] = recs
            D[(b, n)] = d
    # the build records and their executables, once per build (every boot of a build names the same record and tree)
    BR = {}
    for b, s in builds:
        ca = D[(b, boots[0])]["chain_args"]
        if any(D[(b, n)]["chain_args"]["build-record"] != ca["build-record"] or
               D[(b, n)]["chain_args"]["build-root"] != ca["build-root"] for n in boots):
            kept.bad.append(f"{b}\tthe boots of the build name different build records or trees")
        _, exes = chain_check.build_record(ca["build-record"])
        for p, h in exes:
            kept.check(ca["build-root"], p, h, ca["build-record"])
        BR[b] = (ca, exes)
    if kept.bad:
        print("review27: kept files differ from their lists; nothing written", file=sys.stderr)
        for l in kept.bad:
            print(l, file=sys.stderr)
        return 1
    os.makedirs(a.out)
    W = {}

    def out(name, line):
        if name not in W:
            W[name] = open(os.path.join(a.out, name), "w", encoding="utf-8")
            W[name].write(f"# review27.py (review of results 27, 2026-11-19): {TABLES[name]}\n")
        W[name].write(line + "\n")

    main_br = list(D[(builds[0][0], boots[0])]["branches"].keys())[0]

    # r1
    out("r1_buildid.tsv", "# columns build boot object dump_path dump_build_id note chain_same_build_object "
                          "record_path(chain_check.py:344-346) record_paths(leading './' removed) tree_build_id(readelf -n) equal")
    BUILDID = re.compile(r'buildid object=(main|module:\d+) path="((?:[^"\\]|\\.)*)" build_id=([0-9a-f]+|-) note=(\S+) ')
    for b, s in builds:
        ca, exes = BR[b]
        tree_ids = {}
        for n in boots:
            d = D[(b, n)]
            sbo = {}
            for l in d["chain"]:
                if l.startswith("same_build_object\t"):
                    x = l.split("\t")
                    sbo[x[2].split()[0]] = x[1]
            c = collections.Counter()
            for l in lines(d["dump"]):
                m = BUILDID.match(l)
                if not m:
                    continue
                obj, dpath, did, note = m.groups()

                def match(q):
                    return [p for p, _ in exes if q and (p == q or p.endswith("/" + q) or q.endswith("/" + p))]
                if obj == "main":
                    rule = "--main " + str(ca["main"])
                    strip = [ca["main"]]
                else:
                    hits = match(dpath)
                    rule = hits[0] if len(hits) == 1 else "None(%d matches)" % len(hits)
                    strip = match(dpath[2:] if dpath.startswith("./") else dpath)
                tid = None
                if len(strip) == 1:
                    if strip[0] not in tree_ids:
                        tree_ids[strip[0]] = chain_check.build_id(os.path.join(ca["build-root"], strip[0]))
                    tid = tree_ids[strip[0]]
                eq = "yes" if tid is not None and tid == did else "no"
                c["objects"] += 1
                c["present"] += note == "present"
                c["rule_match"] += not rule.startswith("None")
                c["strip_match_one"] += len(strip) == 1
                c["tree_equal"] += eq == "yes"
                c["chain_confirmed"] += sbo.get(obj) == "confirmed"
                out("r1_buildid.tsv", "\t".join([b, str(n), obj, dpath, did, note, sbo.get(obj, "-"), rule,
                                                 ",".join(strip) or "-", tid or "not read", eq]))
            out("r1_buildid.tsv", "# count %s boot %d: %s" % (b, n, ", ".join(f"{k} {c[k]}" for k in
                ("objects", "present", "chain_confirmed", "rule_match", "strip_match_one", "tree_equal"))))

    # r2
    out("r2_found_uses.tsv", "# columns build row path out_read out_value_null field_value_set after_call_out0 "
                             "instance_link_reason creates_of_the_id(publish_events: call; out.read; why) boots per_boot (branch %s)" % main_br)
    for b, s in builds:
        g = collections.defaultdict(collections.Counter)
        for n in boots:
            for fn, r in D[(b, n)]["branches"][main_br].items():
                row = fn.split(".")[1]
                for u in r["order"].get("use_events", []):
                    if u.get("outcome") != "found":
                        continue
                    o = u.get("out") or {}
                    fid = o.get("value")
                    vals = (u.get("after_call") or {}).get("values") or {}
                    cr = "-"
                    if r.get("path") == "A":
                        cs = [c for c in r["order"].get("publish_events", []) if fid is not None and
                              fid in {(c.get("out") or {}).get("value"), (c.get("out") or {}).get("raw")}]
                        cr = " | ".join(sorted({"%s; read=%s; %s" % (c.get("call"), (c.get("out") or {}).get("read"),
                                                                    (c.get("out") or {}).get("why", "-")) for c in cs})) or "none"
                    k = (row, r.get("path"), str(o.get("read")), "yes" if fid is None else "no",
                         "yes" if u.get("field_value") is not None else "no", "yes" if "out0" in vals else "no",
                         (u.get("instance_link") or {}).get("reason", "-"), cr)
                    g[k][n] += 1
        for k in sorted(g):
            out("r2_found_uses.tsv", "\t".join([b] + list(k) + [str(len(g[k])), per_boot(g[k], boots)]))
        for n in boots:
            c = collections.Counter()
            for k, v in g.items():
                c[(k[1], k[3], k[4])] += v.get(n, 0)
            out("r2_found_uses.tsv", "# count %s boot %d: %s" % (b, n, ", ".join(
                f"path {p} value_null {vn} field_value {fv}: {c[(p, vn, fv)]}" for p, vn, fv in sorted(c))))

    # r3
    out("r3_eval_lines.tsv", "# columns build boot what value lines (eval.txt; D1-R0 lines unless named)")
    for b, s in builds:
        for n in boots:
            d = D[(b, n)]
            paths = {fn: r.get("path") for fn, r in d["branches"][main_br].items()}
            ev = lines(d["eval"])
            cols = None
            c4, c3, c8, ns = collections.Counter(), collections.Counter(), collections.Counter(), collections.Counter()
            for l in ev:
                if l.startswith("columns "):
                    cols = l[len("columns "):].split("\t")
                    continue
                ns["lines containing 'slot clear'"] += "slot clear" in l
                ns["lines containing 'weak'"] += "weak" in l
                if cols is None:
                    continue
                x = l.split("\t")
                if len(x) != len(cols):
                    continue
                e = dict(zip(cols, x))
                if e["check"] == "F4" and e["branch"] == main_br:
                    fn = e["ground"].split(";")[-1].strip()
                    c4[(paths.get(fn, "?"), e["result"], e["note"] if e["result"] == "fail" else "-")] += 1
                if e["check"] == "F3" and e["branch"] == main_br and e["result"] == "pass":
                    c3[(e["row_label"], e["use"], e["note"])] += 1
                if e["check"] == "F8a":
                    c8[(e["branch"], e["result"], re.sub(r"^\d+ ", "<n> ", e["note"]))] += 1
            for (p, res, note), v in sorted(c4.items()):
                out("r3_eval_lines.tsv", "\t".join([b, str(n), "F4 path %s %s" % (p, res), note, str(v)]))
            for (row, use, note), v in sorted(c3.items()):
                out("r3_eval_lines.tsv", "\t".join([b, str(n), "F3 pass row %s use %s" % (row, use), note, str(v)]))
            for (br, res, note), v in sorted(c8.items()):
                out("r3_eval_lines.tsv", "\t".join([b, str(n), "F8a branch %s %s" % (br, res), note, str(v)]))
            for k in sorted(ns):
                out("r3_eval_lines.tsv", "\t".join([b, str(n), "whole file", k, str(ns[k])]))

    # r4
    out("r4_order_ground.tsv", "# columns build boot branch what value records")
    for b, s in builds:
        for n in boots:
            for br, recs in D[(b, n)]["branches"].items():
                c = collections.Counter()
                for r in recs.values():
                    og = [json.dumps(r.get("order_ground"), ensure_ascii=False),
                          json.dumps((r.get("order") or {}).get("order_ground"), ensure_ascii=False)]
                    c[("order_ground (record)", og[0])] += 1
                    c[("order.order_ground", og[1])] += 1
                    c[("names fixed_early or hard_before", "yes" if any(("fixed_early" in x or "hard_before" in x) for x in og) else "no")] += 1
                    c[("order.pairs entries", "sum")] += len((r.get("order") or {}).get("pairs") or [])
                for (w, v), k in sorted(c.items()):
                    out("r4_order_ground.tsv", "\t".join([b, str(n), br, w, v, str(k)]))

    # r5
    out("r5_plan_paths.tsv", "# columns build sidecar(sha256) path rows tier_planned control_planned both either; "
                             "then the path A rows planned by either: row key label namespace-entry line")
    ROW = re.compile(r"row ([0-9a-f]{16}) ([ABC])(\d+) tier=(\d) .*?state=(planned|cut)(?: control=(\S+) control-state=(planned|cut))? why=")
    for b, s in builds:
        side = D[(b, boots[0])]["sidecar"]
        sl = lines(side)
        rows = [m.groups() for m in (ROW.match(l) for l in sl if l.startswith("row ")) if m]
        nrow = sum(1 for l in sl if l.startswith("row "))
        ns_rows = {m.group(1) for m in (re.match(r"namespace-entry row=([0-9a-f]{16}) ", l) for l in sl) if m}
        tot = [0] * 5
        for p in "ABC":
            r = [x for x in rows if x[1] == p]
            v = [len(r), sum(x[4] == "planned" for x in r), sum(x[6] == "planned" for x in r),
                 sum(x[4] == "planned" and x[6] == "planned" for x in r), sum(x[4] == "planned" or x[6] == "planned" for x in r)]
            tot = [i + j for i, j in zip(tot, v)]
            out("r5_plan_paths.tsv", "\t".join([b, sha256(side), p] + [str(i) for i in v]))
        out("r5_plan_paths.tsv", "\t".join([b, sha256(side), "all"] + [str(i) for i in tot]))
        out("r5_plan_paths.tsv", "# %s: row lines %d, parsed %d" % (b, nrow, len(rows)))
        for x in rows:
            if x[1] == "A" and (x[4] == "planned" or x[6] == "planned"):
                out("r5_plan_paths.tsv", "\t".join([b, "path A planned", x[1] + x[2], x[0], "tier-state " + x[4],
                                                    "control-state " + str(x[6]), "namespace-entry " + ("yes" if x[0] in ns_rows else "no")]))

    # r6
    out("r6_expect_branch.tsv", "# columns build boot branch line_build class result lines (expect_compare.txt)")
    for b, s in builds:
        for n in boots:
            c = collections.Counter()
            for l in lines(os.path.join(D[(b, n)]["bd"], "expect_compare.txt")):
                if not l or l.startswith("#"):
                    continue
                x = l.split("\t")
                c[(x[0], x[1], x[-3], x[-2])] += 1
            for k in sorted(c, key=lambda k: (k[0] != main_br, k)):
                out("r6_expect_branch.tsv", "\t".join([b, str(n)] + list(k) + [str(c[k])]))

    for f in W.values():
        f.close()
    with open(os.path.join(a.out, "inputs.txt"), "w", encoding="utf-8") as f:
        f.write("# kept files read, each checked first against its keep list, the eval head or the build record\n")
        f.write("files %d sha256-of-sorted-lines %s\n" % (len(kept.checked), hashlib.sha256(
            "\n".join(sorted(kept.checked)).encode()).hexdigest()))
    print("review27 builds %s boots %d branches %d kept files checked %d differing 0" % (
        a.builds, a.boots, len(D[(builds[0][0], boots[0])]["branches"]), len(kept.checked)))
    for name in sorted(W):
        print("%s lines %d" % (name, sum(1 for _ in open(os.path.join(a.out, name), encoding="utf-8"))))
    return 0


if __name__ == "__main__":
    sys.exit(main())
