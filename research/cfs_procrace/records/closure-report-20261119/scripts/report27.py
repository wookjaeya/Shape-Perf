#!/usr/bin/env python3
"""report27.py -- the counts of results 27.4 (C1 cycle boots), by the order of results 27.3 (2026-11-19).

Reads only.  Inputs: the committed records of the boots (--records: records/closure-c1prep-20261119), and the kept
outputs under --obs1 (c1cyc_<s>_explain<N>/<branch>/{index.tsv,uses.tsv,records/*.json}, c1cyc_<s>_eval<N>/eval.txt,
the relations text the eval head names).  Every kept file is checked against the keep list of its boot
(boot/<build>/boot<N>/c1cyc_<s>_{explain,eval}<N>.sha256) or, for the relations text, against the eval head, before
it is read; one difference or one missing file ends the run with rc 1 and nothing written.
It copies and counts record fields; it does not judge, does not recompute an expectation or a check, and takes no row
name or case value as input (the rows come from the uses lists and the records).
Output: a new directory --out (refused with rc 2 if it exists), one text file per table (1)-(11) of results 27.3.
"""
import argparse
import collections
import hashlib
import json
import os
import sys

TABLES = {
    "t01_boots.tsv": "(1) gate, losses, chain, same_build and same_build_object per build and boot",
    "t02_uses.tsv": "(2) per branch (D1-R0 first) and pre-defined use: per boot reached, use events, record violation",
    "t03_elements.tsv": "(3) per branch and path A record: use-condition elements, use_cond, violation, link per boot",
    "t04_pairs.tsv": "(4) D1-R0: name-binding items of every pair of the path A records per boot",
    "t05_checks.tsv": "(5) falsifiers, expect_safe and the rest of eval.txt: units per branch, class, check, result",
    "t05b_units.tsv": "(5) D1-R0: fail and not_evaluable units per check, row and note, per boot",
    "t06_discriminate.tsv": "(6) expect_discriminate lines of eval_tail.txt per boot",
    "t07_expect.tsv": "(7) expect_compare.txt: lines per class and result, and every mismatch with its item difference",
    "t08_pairkinds.tsv": "(8) D1-R0: pair kinds, failure signs, consumer affiliation, order lines per pair, per boot",
    "t09_aux.tsv": "(9) auxiliary branches: changed conditions and the record fields that differ from D1-R0",
    "t10_compare11.tsv": "(10) evaluation protocol 11: (a) failure state, (b) with another actor creating the same text, (c)",
    "t11_rows_c3.tsv": "(11) path A records: (c3) static lines (relations text) and boot record columns",
}


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


class Kept:
    """sha256 lists of the keep step; every read of a kept file goes through check()."""

    def __init__(self):
        self.lists = {}
        self.checked = []
        self.bad = []

    def load(self, base, listfile):
        m = {}
        with open(listfile, encoding="utf-8") as f:
            for line in f:
                line = line.rstrip("\n")
                if not line:
                    continue
                sha, rel = line.split("  ", 1)
                m[os.path.normpath(rel)] = sha
        self.lists[base] = (m, listfile)

    def check(self, base, rel, expected=None):
        path = os.path.join(base, rel)
        if expected is None:
            m, listfile = self.lists[base]
            expected = m.get(os.path.normpath("./" + rel))
            src = listfile
        else:
            src = "the eval head"
        if expected is None or not os.path.isfile(path):
            self.bad.append(f"{path}\tnot in {src} or missing")
            return None
        got = sha256(path)
        if got != expected:
            self.bad.append(f"{path}\tsha256 {got} != {expected} ({src})")
            return None
        self.checked.append(f"{got}  {path}")
        return path


def read_tsv(path):
    with open(path, encoding="utf-8") as f:
        rows = [l.rstrip("\n").split("\t") for l in f if l.strip()]
    head = rows[0]
    return [dict(zip(head, r)) for r in rows[1:]]


def read_uses_list(path):
    out = []
    cols = None
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith("columns "):
                cols = line[len("columns "):].split("\t")
            elif cols and "\t" in line and not line.startswith(("kz-uses", "rule ", "lines ", "end")):
                out.append(dict(zip(cols, line.split("\t"))))
    return out


def j(v):
    return json.dumps(v, ensure_ascii=False, sort_keys=True)


def root_of(rec):
    rt = (rec.get("consumer_actor") or {}).get("root_tie") or {}
    if rt.get("root"):
        return f"{rt['root']} ({rt.get('kind')})"
    return "none"


def elements(rec):
    uc = rec.get("use_cond") or {}
    e = collections.OrderedDict()
    e["(c1) callee_check"] = (uc.get("callee_check") or {}).get("status")
    e["(c2) reach"] = (uc.get("reach") or {}).get("status")
    e["(c3)(i) branch_i"] = (uc.get("branch_i") or {}).get("status")
    e["(c3)(iii) branch_iii"] = ";".join(f"{b.get('context')}:{b.get('status')}" for b in uc.get("branch_iii") or []) or "-"
    e["(c4) writers"] = (uc.get("writers") or {}).get("status")
    e["caller_dependence.text"] = (uc.get("caller_dependence") or {}).get("text")
    e["use_cond.status"] = uc.get("status")
    e["violation"] = ((rec.get("tool_verdict") or {}).get("violation") or {}).get("value")
    e["violation.kind"] = ((rec.get("tool_verdict") or {}).get("violation") or {}).get("kind")
    e["odf"] = ((rec.get("tool_verdict") or {}).get("odf") or {}).get("value")
    e["link.status"] = (rec.get("link") or {}).get("status")
    return e


def element_sources(rec):
    uc = rec.get("use_cond") or {}
    out = []
    for name in ("callee_check", "reach", "branch_i", "writers"):
        refs = [s.get("ref") for s in (uc.get(name) or {}).get("sources") or []]
        out.append(f"{name}={','.join(r for r in refs if r) or '-'}")
    for b in uc.get("branch_iii") or []:
        refs = [s.get("ref") for s in b.get("sources") or []]
        out.append(f"branch_iii[{b.get('context')}]={','.join(r for r in refs if r) or '-'}")
    return " ".join(out)


def pair_rows(rec):
    nb = (rec.get("link") or {}).get("name_binding") or {}
    for p in nb.get("pairs") or []:
        yield p


def use_events(rec):
    return ((rec.get("order") or {}).get("use_events")) or []


def reason_prefixes(rec):
    rs = ((rec.get("tool_verdict") or {}).get("violation") or {}).get("undetermined_reasons") or []
    c = collections.Counter(r.split("(", 1)[0] for r in rs)
    return ",".join(f"{k}x{v}" for k, v in sorted(c.items())) or "-"


def dist(values):
    c = collections.Counter(values)
    return ", ".join(f"{k} {v}" for k, v in sorted(c.items()))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--records", required=True)
    ap.add_argument("--obs1", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--builds", required=True, help="build=s,build=s (s: the work directory infix)")
    ap.add_argument("--boots", type=int, required=True)
    a = ap.parse_args()
    if os.path.exists(a.out):
        print(f"report27: {a.out} exists; not run", file=sys.stderr)
        return 2
    builds = [tuple(x.split("=")) for x in a.builds.split(",")]
    boots = list(range(1, a.boots + 1))
    kept = Kept()
    data = {}   # (build, n) -> dict
    for b, s in builds:
        for n in boots:
            bd = os.path.join(a.records, "boot", b, f"boot{n}")
            ex = os.path.join(a.obs1, f"c1cyc_{s}_explain{n}")
            ev = os.path.join(a.obs1, f"c1cyc_{s}_eval{n}")
            kept.load(ex, os.path.join(bd, f"c1cyc_{s}_explain{n}.sha256"))
            kept.load(ev, os.path.join(bd, f"c1cyc_{s}_eval{n}.sha256"))
            d = {"bd": bd, "branches": collections.OrderedDict()}
            tail = open(os.path.join(bd, "eval_tail.txt"), encoding="utf-8").read().splitlines()
            d["tail"] = tail
            rel = [l.split() for l in tail if l.startswith("input relations ")][0]
            d["relations"] = kept.check(os.path.dirname(rel[3]), os.path.basename(rel[3]), rel[2])
            d["eval"] = kept.check(ev, "eval.txt")
            for line in open(os.path.join(bd, "explain.txt"), encoding="utf-8"):
                if line.startswith("branch "):
                    br = line.split()[1]
                    idx = kept.check(ex, f"{br}/index.tsv")
                    ust = kept.check(ex, f"{br}/uses.tsv")
                    recs = collections.OrderedDict()
                    if idx:
                        for r in read_tsv(idx):
                            p = kept.check(ex, f"{br}/records/{r['file']}")
                            if p:
                                with open(p, encoding="utf-8") as f:
                                    recs[r["file"]] = json.load(f)
                    d["branches"][br] = {"index": read_tsv(idx) if idx else [], "uses": read_tsv(ust) if ust else [],
                                         "records": recs}
            data[(b, n)] = d
    if kept.bad:
        print("report27: kept files differ from their lists; nothing written", file=sys.stderr)
        for l in kept.bad:
            print(l, file=sys.stderr)
        return 1
    os.makedirs(a.out)
    W = {}

    def out(name, line):
        if name not in W:
            W[name] = open(os.path.join(a.out, name), "w", encoding="utf-8")
            W[name].write(f"# report27.py, results 27.3 table {TABLES[name]}\n")
        W[name].write(line + "\n")

    uses_lists = {b: read_uses_list(os.path.join(a.records, "uses", f"uses_{b}.tsv")) for b, _ in builds}
    branches = list(data[(builds[0][0], 1)]["branches"].keys())
    main_br = branches[0]

    # (1)
    out("t01_boots.tsv", "# columns build boot gate losses chain same_build same_build_object(lines)")
    for b, s in builds:
        for n in boots:
            bd = data[(b, n)]["bd"]
            g = open(os.path.join(bd, "gate.txt"), encoding="utf-8").read().splitlines()
            ch = open(os.path.join(bd, "chain.txt"), encoding="utf-8").read().splitlines()
            gate = [l for l in g if l.startswith("gate ")]
            loss = [l for l in g if l.startswith("losses")]
            chain = [l for l in ch if l.startswith("chain\t")]
            sb = [l for l in ch if l.startswith("same_build\t")]
            sbo = [l.split("\t") for l in ch if l.startswith("same_build_object\t")]
            sbo_t = "; ".join(f"{x[2].split()[0] if x[2].split() else '-'}={x[1]}" for x in sbo)
            out("t01_boots.tsv", "\t".join([b, str(n), " ".join(gate), " ".join(loss), " ".join(chain).replace("\t", " "),
                                           " ".join(sb).replace("\t", " "), sbo_t]))

    # helpers over records of the path A rows of the uses lists
    def row_records(b, n, br, ul):
        recs = data[(b, n)]["branches"][br]["records"]
        got = []
        for fn, rec in recs.items():
            rid = rec.get("row_id") or {}
            if rid.get("row_counter") == ul["row_label"] and rid.get("use_site_key") == ul["row_key"] \
                    and root_of(rec) == ul["consumer_root"]:
                got.append((fn, rec))
        return got

    rows_once = {}
    for b, _ in builds:
        seen = collections.OrderedDict()
        for ul in uses_lists[b]:
            seen[(ul["row_label"], ul["row_key"], ul["consumer_root"])] = ul
        rows_once[b] = list(seen.values())

    # (2)
    out("t02_uses.tsv", "# columns branch build row key consumer_root context plan_state boot reached events "
        "outcomes(status) contexts name_states link_results record_violation violation_kind reason_prefixes")
    out("t02_uses.tsv", "# a use is reached in a boot when its record holds a use event whose context is the use's "
        "context (any context for '-'); the distribution line counts per boot the record's violation, or 'not reached'")
    for br in branches:
        for b, _ in builds:
            for ul in uses_lists[b]:
                per = []
                for n in boots:
                    rr = row_records(b, n, br, ul)
                    evs, vio, kind, rp = [], [], [], []
                    for fn, rec in rr:
                        for u in use_events(rec):
                            if ul["context_id"] == "-" or str(u.get("context")) == ul["context_id"]:
                                evs.append(u)
                        e = elements(rec)
                        vio.append(e["violation"])
                        kind.append(e["violation.kind"] or "-")
                        rp.append(reason_prefixes(rec))
                    reached = "yes" if evs else "no"
                    per.append(vio[0] if (evs and vio) else "not reached")
                    out("t02_uses.tsv", "\t".join([
                        br, b, ul["row_label"], ul["row_key"], ul["consumer_root"], ul["context_id"], ul["plan_state"],
                        str(n), reached, str(len(evs)),
                        ",".join(f"{u.get('outcome')}({u.get('status_name') or u.get('status_value')})" for u in evs) or "-",
                        ",".join(str(u.get("context")) for u in evs) or "-",
                        "|".join(sorted({str(u.get("name_state")) for u in evs})) or "-",
                        "|".join(sorted({str(u.get("link_result")) for u in evs})) or "-",
                        ",".join(vio) or "no record", ",".join(kind) or "-", ";".join(rp) or "-"]))
                out("t02_uses.tsv", "\t".join([br, b, ul["row_label"], ul["row_key"], ul["consumer_root"],
                                              ul["context_id"], ul["plan_state"], "distribution",
                                              f"{len(boots)} boots: {dist(per)}"]))

    # (3)
    out("t03_elements.tsv", "# columns branch build row key consumer_root field values_over_boots sources(boot 1)")
    for br in branches:
        for b, _ in builds:
            for ul in rows_once[b]:
                vals = collections.defaultdict(list)
                src = "-"
                for n in boots:
                    rr = row_records(b, n, br, ul)
                    for fn, rec in rr:
                        for k, v in elements(rec).items():
                            vals[k].append(str(v))
                        if n == 1:
                            src = element_sources(rec)
                for k, v in vals.items():
                    out("t03_elements.tsv", "\t".join([br, b, ul["row_label"], ul["row_key"], ul["consumer_root"], k,
                                                      dist(v), src if k == "(c1) callee_check" else ""]))

    # (4)
    out("t04_pairs.tsv", "# columns build row key consumer_root use create_pos create_key_text pair_kind item "
        "state(reason) boots result")
    for b, _ in builds:
        for ul in rows_once[b]:
            agg = collections.OrderedDict()
            for n in boots:
                for fn, rec in row_records(b, n, main_br, ul):
                    texts = {}
                    for pe in (rec.get("order") or {}).get("publish_events") or []:
                        texts[(str(pe.get("actor")), (pe.get("at_call") or {}).get("aseq"))] = \
                            (pe.get("key") or {}).get("text")
                    for p in pair_rows(rec):
                        kt = texts.get((str(p.get("create_actor")), p.get("create_aseq")))
                        base = (str(p.get("use")), p.get("create_pos"), str(kt), p.get("pair_kind"))
                        for item, st in (p.get("items") or {}).items():
                            key = base + (item, f"{st.get('status')}({st.get('reason') or ''})")
                            agg.setdefault(key, collections.Counter())[p.get("result")] += 1
            for key, c in agg.items():
                out("t04_pairs.tsv", "\t".join([b, ul["row_label"], ul["row_key"], ul["consumer_root"]] + list(key) +
                                               [str(sum(c.values())), dist(list(c.elements()))]))

    # (5)
    out("t05_checks.tsv", "# columns build boot branch check class result units (from eval.txt, the lines of "
        "the columns line: check class branch build row_label row_key use result ground note)")
    out("t05b_units.tsv", f"# columns build check class result row_label row(from the record name) note boots "
        f"units_per_boot (branch {main_br})")
    for b, _ in builds:
        units = collections.OrderedDict()
        for n in boots:
            c = collections.Counter()
            started = False
            with open(data[(b, n)]["eval"], encoding="utf-8") as f:
                for line in f:
                    line = line.rstrip("\n")
                    if line.startswith("columns check"):
                        started = True
                        continue
                    if not started:
                        continue
                    x = line.split("\t")
                    if len(x) < 10:
                        continue
                    chk, cls, brn, bld, rl, rk, use, res, ground, note = x[:10]
                    c[(brn, chk, cls, res if chk not in ("undetermined_split",) else "count")] += 1
                    if brn == main_br and res in ("fail", "not_evaluable"):
                        rowname = ground.split(".")[1] if ground.count(".") >= 2 else rl
                        k = (chk, cls, res, rl, rowname, note)
                        units.setdefault(k, collections.Counter())[n] += 1
            for (brn, chk, cls, res), v in sorted(c.items(), key=lambda kv: (branches.index(kv[0][0])
                                                                              if kv[0][0] in branches else 99, kv[0])):
                out("t05_checks.tsv", "\t".join([b, str(n), brn, chk, cls, res, str(v)]))
        for k, c in units.items():
            out("t05b_units.tsv", "\t".join([b] + list(k) + [str(len(c)), ",".join(f"{n}:{c[n]}" for n in boots)]))

    # (6)
    out("t06_discriminate.tsv", "# columns build branch row result class boots note (eval_tail.txt lines "
        "'expect_discriminate')")
    for b, _ in builds:
        agg = collections.OrderedDict()
        for n in boots:
            for line in data[(b, n)]["tail"]:
                if line.startswith("expect_discriminate\t"):
                    x = line.split("\t")
                    k = (x[2], x[4], x[7], x[1], x[9] if len(x) > 9 else "")
                    agg.setdefault(k, set()).add(n)
        for (brn, row, res, cls, note), ns in agg.items():
            out("t06_discriminate.tsv", "\t".join([b, brn, row, res, cls, str(len(ns)), note]))

    # (7)
    out("t07_expect.tsv", "# columns build boot kind ... (expect_compare.txt of the boot's own build; lines of the "
        "other build are no_record there and are counted apart)")
    for b, _ in builds:
        mism = collections.OrderedDict()
        for n in boots:
            c = collections.Counter()
            with open(os.path.join(data[(b, n)]["bd"], "expect_compare.txt"), encoding="utf-8") as f:
                for line in f:
                    x = line.rstrip("\n").split("\t")
                    if len(x) != 9:
                        continue
                    own = "own build" if x[1] == b else "other build"
                    c[(own, x[6], x[7])] += 1
                    if x[7] == "mismatch":
                        e, g = set(x[5].split(",")), set(x[8].split(","))
                        k = (x[0], x[2], x[3], x[4], x[5], x[8], ",".join(sorted(g - e)) or "-",
                             ",".join(sorted(e - g)) or "-")
                        mism.setdefault(k, set()).add(n)
            for k, v in sorted(c.items()):
                out("t07_expect.tsv", "\t".join([b, str(n), "count"] + list(k) + [str(v)]))
        for k, ns in mism.items():
            out("t07_expect.tsv", "\t".join([b, "boots " + str(len(ns)), "mismatch"] + list(k)))

    # (8)
    out("t08_pairkinds.tsv", f"# branch {main_br}; columns build boot kind value count")
    for b, _ in builds:
        for n in boots:
            recs = data[(b, n)]["branches"][main_br]["records"]
            pk, fs, aff = collections.Counter(), collections.Counter(), collections.Counter()
            for fn, rec in recs.items():
                for v in (rec.get("pair_kind") or {}).values():
                    pk[v] += 1
                if elements(rec)["odf"] == "failure_sign":
                    fs[",".join(sorted(set((rec.get("pair_kind") or {}).values()))) or "-"] += 1
                    aff[((rec.get("link") or {}).get("app_of_actor") or {}).get("class") or "unknown"] += 1
            for k, v in sorted(pk.items()):
                out("t08_pairkinds.tsv", "\t".join([b, str(n), "pair_kind (consumer x publisher)", k, str(v)]))
            for k, v in sorted(fs.items()):
                out("t08_pairkinds.tsv", "\t".join([b, str(n), "failure_sign records by pair_kind set", k, str(v)]))
            for k, v in sorted(aff.items()):
                out("t08_pairkinds.tsv", "\t".join([b, str(n), "failure_sign records by consumer class", k, str(v)]))
            for ul in rows_once[b]:
                for fn, rec in row_records(b, n, main_br, ul):
                    og = j(rec.get("order_ground"))
                    oo = j(rec.get("other_order_seen_in_boots"))
                    for p in pair_rows(rec):
                        b6 = (p.get("items") or {}).get("basis6_create_after_use") or {}
                        out("t08_pairkinds.tsv", "\t".join([b, str(n), "pair order line", ul["row_label"],
                                                           f"use {p.get('use')} create {p.get('create_pos')} "
                                                           f"aseq {p.get('create_aseq')} {p.get('pair_kind')}: "
                                                           f"basis6 {b6.get('status')}({b6.get('reason') or ''}) "
                                                           f"grounds {j(b6.get('grounds'))}; order_ground {og}; "
                                                           f"other_order_seen_in_boots {oo}"]))

    # (9)
    out("t09_aux.tsv", "# columns branch build kind field main_value aux_value records_x_boots (fields of every record "
        "of the branch against the record of the same file name in the main branch)")

    def flat(rec):
        f = collections.OrderedDict(elements(rec))
        for p in pair_rows(rec):
            for item, st in (p.get("items") or {}).items():
                f[f"pair[{p.get('use')},{p.get('create_pos')},{p.get('create_aseq')}].{item}"] = \
                    f"{st.get('status')}({st.get('reason') or ''})"
            f[f"pair[{p.get('use')},{p.get('create_pos')},{p.get('create_aseq')}].result"] = p.get("result")
        for i, u in enumerate(use_events(rec)):
            for k in ("outcome", "context", "name_state", "link_result"):
                f[f"use_event[{i}].{k}"] = str(u.get(k))
        f["undetermined_reasons"] = j(((rec.get("tool_verdict") or {}).get("violation") or {})
                                      .get("undetermined_reasons"))
        return f

    for br in branches[1:]:
        for b, _ in builds:
            r1 = data[(b, 1)]["branches"][br]["records"]
            any_rec = next(iter(r1.values()))
            out("t09_aux.tsv", "\t".join([br, b, "condition", "eval_branch", "", j(any_rec.get("eval_branch")), ""]))
            out("t09_aux.tsv", "\t".join([br, b, "condition", "data_branch", "", j(any_rec.get("data_branch")), ""]))
            diff = collections.OrderedDict()
            for n in boots:
                mr = data[(b, n)]["branches"][main_br]["records"]
                ar = data[(b, n)]["branches"][br]["records"]
                for fn in sorted(set(mr) | set(ar)):
                    if fn not in mr or fn not in ar:
                        diff.setdefault(("record", fn, str(fn in mr), str(fn in ar)), 0)
                        diff[("record", fn, str(fn in mr), str(fn in ar))] += 1
                        continue
                    fm, fa = flat(mr[fn]), flat(ar[fn])
                    for k in list(fm) + [k for k in fa if k not in fm]:
                        if fm.get(k) != fa.get(k):
                            row = fn.split(".")[1]
                            if k == "undetermined_reasons":
                                sm = set(json.loads(fm.get(k) or "null") or [])
                                sa = set(json.loads(fa.get(k) or "null") or [])
                                key = ("field", f"{row} {k}", "only main: " + j(sorted(sm - sa)),
                                       "only auxiliary: " + j(sorted(sa - sm)))
                            else:
                                key = ("field", f"{row} {k}", str(fm.get(k)), str(fa.get(k)))
                            diff[key] = diff.get(key, 0) + 1
            if not diff:
                out("t09_aux.tsv", "\t".join([br, b, "field", "(none)", "", "", "0"]))
            for (kind, field, mv, av), v in diff.items():
                out("t09_aux.tsv", "\t".join([br, b, kind, field, mv, av, str(v)]))

    # (10)
    out("t10_compare11.tsv", f"# branch {main_br}; columns build boot record (a) (b) other_actor_same_text(actor:aseq) "
        "(c) link.status pair results use_cond violation")
    for b, _ in builds:
        tot = collections.Counter()
        for n in boots:
            for fn, rec in data[(b, n)]["branches"][main_br]["records"].items():
                a_ = elements(rec)["odf"] == "failure_sign"
                cons = str((rec.get("consumer_actor") or {}).get("run_actor"))
                ut = {(u.get("key") or {}).get("text") for u in use_events(rec)} - {None}
                oth = [f"{pe.get('actor')}:{(pe.get('at_call') or {}).get('aseq')}"
                       for pe in (rec.get("order") or {}).get("publish_events") or []
                       if str(pe.get("actor")) != cons and (pe.get("key") or {}).get("text") in ut]
                b_ = a_ and bool(oth)
                if not a_:
                    continue
                e = elements(rec)
                res = ",".join(sorted({str(p.get("result")) for p in pair_rows(rec)})) or "-"
                tot[("a", )] += 1
                tot[("b", )] += b_
                tot[("c violation", e["violation"])] += 1
                out("t10_compare11.tsv", "\t".join([b, str(n), fn, "yes", "yes" if b_ else "no", ",".join(oth) or "-",
                                                   e["link.status"], res, e["use_cond.status"], e["violation"]]))
        for k, v in sorted(tot.items()):
            out("t10_compare11.tsv", "\t".join([b, "all boots", "total"] + list(k) + [str(v)]))

    # (11)
    out("t11_rows_c3.tsv", f"# branch {main_br}; columns build row key consumer_root plan_state(s) part ...")
    for b, _ in builds:
        rel = data[(b, 1)]["relations"]
        lines = open(rel, encoding="utf-8").read().splitlines()
        for ul in rows_once[b]:
            ps = ",".join(sorted({u["plan_state"] for u in uses_lists[b] if u["row_key"] == ul["row_key"]
                                  and u["row_label"] == ul["row_label"]}))
            head = [b, ul["row_label"], ul["row_key"], ul["consumer_root"], ps]
            rr = row_records(b, 1, main_br, ul)
            if not rr:
                out("t11_rows_c3.tsv", "\t".join(head + ["static", "no record"]))
                continue
            uc = rr[0][1].get("use_cond") or {}
            refs = [("(i)", s.get("ref")) for s in (uc.get("branch_i") or {}).get("sources") or []]
            for bi in uc.get("branch_iii") or []:
                refs += [(f"(iii) context {bi.get('context')}", s.get("ref")) for s in bi.get("sources") or []]
            for what, ref in refs:
                txt = "-"
                if ref and ref.startswith("relations.txt:"):
                    k = int(ref.split(":")[1])
                    txt = lines[k - 1].strip() if 0 < k <= len(lines) else "-"
                out("t11_rows_c3.tsv", "\t".join(head + ["static line", what, ref or "-", txt[:400]]))
            out("t11_rows_c3.tsv", "\t".join(head + ["static value", "(i)", str((uc.get("branch_i") or {})
                                                                                 .get("status")),
                                                    ((uc.get("branch_i") or {}).get("why") or "")[:400]]))
            for bi in uc.get("branch_iii") or []:
                out("t11_rows_c3.tsv", "\t".join(head + ["static value", f"(iii) context {bi.get('context')}",
                                                        str(bi.get("status")), (bi.get("why") or "")[:400]]))
            for n in boots:
                for fn, rec in row_records(b, n, main_br, ul):
                    u2 = rec.get("use_cond") or {}
                    evs = use_events(rec)
                    out("t11_rows_c3.tsv", "\t".join(head + [
                        f"boot {n}", fn, "(i) " + str((u2.get("branch_i") or {}).get("status")) + "; (iii) " +
                        ";".join(f"{x.get('context')}:{x.get('status')}" for x in u2.get("branch_iii") or []),
                        "events " + (" | ".join(
                            f"aseq {(u.get('at_call') or {}).get('aseq')} t_ms {(u.get('at_call') or {}).get('t_ms')} "
                            f"{u.get('call')} {u.get('status_name') or u.get('status_value')} {u.get('outcome')} "
                            f"context {u.get('context')} key {(u.get('key') or {}).get('text')}" for u in evs) or "none")]))

    for name in TABLES:
        if name not in W:
            out(name, "# (no lines)")
    for f in W.values():
        f.close()
    with open(os.path.join(a.out, "inputs.txt"), "w", encoding="utf-8") as f:
        f.write("# kept files read, per directory: files checked against the keep list (or the eval head), and the "
                "sha256 of the sorted lines '<sha256>  <path>' of those files\n")
        per = collections.defaultdict(list)
        for line in kept.checked:
            path = line.split("  ", 1)[1]
            top = path
            for base in kept.lists:
                if path.startswith(base + os.sep):
                    top = base
                    break
            else:
                top = os.path.dirname(path)
            per[top].append(line)
        for top in sorted(per):
            body = "\n".join(sorted(per[top])) + "\n"
            f.write(f"{top}\tfiles {len(per[top])}\tsha256 {hashlib.sha256(body.encode()).hexdigest()}\n")
    with open(os.path.join(a.out, "run.txt"), "w", encoding="utf-8") as f:
        f.write(f"report27 builds {a.builds} boots {a.boots} branches {len(branches)} kept files checked "
                f"{len(kept.checked)} differing 0\n")
        for name in sorted(W):
            f.write(f"{name} lines {sum(1 for _ in open(os.path.join(a.out, name), encoding='utf-8'))}\n")
    print(f"report27: kept files checked {len(kept.checked)}, differing 0, tables {len(W)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
