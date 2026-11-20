#!/usr/bin/env python3
"""expect_write2.py -- the expectation file of the C1 cycle rewritten from the rules before any boot (kz-expect 1 with
the class column of spec 11 item 12; results 26.4; judgement rules 8.13-8.14; evaluation protocol 5-7, 14.6, 14.11,
14.12).  Run with python3 -I.  It never reads an output of the explanation script revised by spec 11.

    expect_write2.py --branches BRANCHES --static STATIC_ELEMENTS.tsv --candidates CANDIDATES.tsv
                     --predictions PREDICTIONS.tsv --uses BUILD:USES.tsv:SIDECAR:RELATIONS ... --elements-out FILE -o OUT

No row id, key or value is a constant here.  Every line comes from these files:
  static       the committed basis of 25.11 (the element values the rules before 8.14 give on the static inputs, per
               build, branch and row: (c1), (c2), (c4), (c3)(i), (c3)(iii) per context)
  relations    the row's block (found by the use list's row label): the layers line, the element source lines (path A:
               flip for (c1) and (c2), absence for (c2) under reading Rc when it is 'absence: -N ...: reached', creator
               lines for (c4), holder lines for (c3)(i)), the alternatives lines and the success path lines
  candidates   the candidate list; the branch file names each branch's admission file (D1: none admitted)
  predictions  the hand-written boot predictions [I] per build and row label, with their grounds (what a boot adds)
  uses         the pre-defined use list of the build (row key, consumer root, context ids; '-' = no context line)
Rules applied (protocol 14.12 item 2):
  depends_on   (8.14 item 6 (c); spec 11 item 5) the functions of the row's layers list that an element's source lines
               name as whole words, looked up in the candidate list; an element present (c1, c2, c4) or held (c3 (i))
               whose lines are not all admitted in the branch becomes missing / not_established
  context      (8.14 item 6 (a)) a row without context lines: every use's context is the one context id its alternatives
               lines name (else unknown); a row with context lines: the contexts of its frames, unknown otherwise
  use_cond     (8.14 item 2) derived iff (c1), (c2), (c4) present and (c3): (i) held and (iii) held in every context
               (Ra: (iii) only); partial if some element stands; not_derived otherwise; a derived line stops the script
               (violation could then depend on a boot and would need a prediction)
  violation    undetermined wherever use_cond is not derived (JR 3.3)
  text         use_cond.caller_dependence.text = '(i) <state>; (iii) <states, comma-joined, sorted>' over the row's
               contexts (the judgement part's form)
Class (protocol 14.6 and 14.12 item 1): static for the six element fields; static for caller_dependence.text when the
row has no context lines and one alternatives context, or has context lines and every alternatives context gives
not_established (else the script needs an iii_boot prediction and the line is boot); static for link.status when the
row's block has no 'callee-check (success path): ...: held' line (else boot); boot for every other prediction line.
One expectation line per (branch of the branch file, build, row, field); the root is the use list's consumer root, '*'
for a row whose consumer is none.
"""
import argparse
import hashlib
import re
import sys

STATIC_FIELDS = [("use_cond.status", "use_cond"), ("tool_verdict.violation.value", "violation"),
                 ("use_cond.callee_check.status", "c1"), ("use_cond.reach.status", "c2"),
                 ("use_cond.writers.status", "c4"), ("use_cond.branch_i.status", "c3i")]
RULE = ("rule class (evaluation protocol 14.6, 14.12 item 1): static = the six element fields; caller_dependence.text of a "
        "row whose every possible context set gives one text; link.status of a row whose block has no success path line "
        "held; boot = every other line (a use event's presence depends on the boot)")


def tsv(path):
    return [l.rstrip("\n").split("\t") for l in open(path, encoding="utf-8") if l.strip() and not l.startswith("#")]


def sha(path):
    with open(path, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()


def blocks(rel_path):
    """label -> [(line number, stripped text)] of the row blocks of a relations text."""
    out, cur = {}, None
    for n, line in enumerate(open(rel_path, encoding="utf-8"), 1):
        m = re.match(r"  ([ABC]\d+) ", line)
        if m:
            cur = m.group(1)
            out[cur] = []
            continue
        if cur is not None and line.startswith("      "):
            out[cur].append((n, line.strip()))
        elif cur is not None and not line.startswith("    "):
            cur = None
    return out


def listed(layers_text):
    """(layer, function, column or None) of a layers line's unverified lists."""
    res = []
    m = re.search(r"P0-unverified\(([^)]*)\)", layers_text)
    if m:
        res += [("P0", f.strip(), None) for f in m.group(1).split(",") if f.strip()]
    m = re.search(r"sem-unverified\(([^)]*)\)", layers_text)
    if m:
        for x in m.group(1).split(","):
            x = x.strip()
            if ":" in x:
                f, c = x.split(":", 1)
                res.append(("framework", f, c))
    return res


def names(text, fn):
    return re.search(r"(?<![\w@])%s(?![\w])" % re.escape(fn), text) is not None


def main():
    ap = argparse.ArgumentParser()
    for a in ("branches", "static", "candidates", "predictions", "elements-out"):
        ap.add_argument("--" + a, required=True)
    ap.add_argument("--uses", action="append", required=True)
    ap.add_argument("-o", required=True)
    a = ap.parse_args()
    # branches and admission files
    br = []
    for l in open(a.branches, encoding="utf-8"):
        if l.startswith("branch "):
            f = dict(x.split("=", 1) for x in l.split()[2:])
            br.append((l.split()[1], f["data"], f["readings"].split(","), f["admitted"]))
    cand = [r for r in tsv(a.candidates) if r[0] in ("P0", "framework")]
    cand_key = {"\t".join(r) for r in cand}
    admitted = {}
    for _, _, _, adm in br:
        if adm != "none" and adm not in admitted:
            admitted[adm] = {"\t".join(r) for r in tsv(adm) if r[0] in ("P0", "framework")}
            if not admitted[adm] <= cand_key:
                sys.stderr.write("expect_write2.py: %s holds a line outside the candidate list\n" % adm)
                return 1
    st = tsv(a.static)
    head, st = st[0], st[1:]
    ix = {h: i for i, h in enumerate(head)}
    static = {(r[ix["build"]], r[ix["branch"]], r[ix["row"]]): r for r in st}
    pred = {}
    for r in tsv(a.predictions):
        pred.setdefault((r[0], r[1]), []).append(r)
    plans, rows = [], []
    for spec in a.uses:
        build, upath, side, rel = spec.split(":")
        plans.append("plan %s" % sha(side))
        bl = blocks(rel)
        seen = {}
        for r in tsv(upath):
            if len(r) < 14 or r[0] != build:
                continue
            seen.setdefault(r[1], {"key": r[2], "root": r[4].split(" (")[0], "ctx": set()})["ctx"].add(r[11])
        for label, u in seen.items():
            rows.append((build, label, u["key"], "*" if u["root"] == "none" else u["root"], u["ctx"], bl[label]))
    out, elem_out = [], []
    counts = {"static": 0, "boot": 0, "format": 0}
    for brid, data, readings, adm in br:
        adm_set = admitted.get(adm, set())
        dcol = 4 if data == "D1" else 5
        for build, label, key, root, ctxs, blk in rows:
            s = static.get((build, brid, label))
            if s is None:
                sys.stderr.write("expect_write2.py: no static line for %s %s %s\n" % (build, brid, label))
                return 1
            text = {n: t for n, t in blk}
            lay = [t for n, t in blk if t.startswith("layers:")]
            lst = listed(lay[0]) if lay else []
            flip = [n for n, t in blk if t.startswith("flip:")]
            absn = [n for n, t in blk if re.match(r"absence: -?\d+ .*: reached", t)]
            src = {"c1": flip,
                   "c2": absn if ("Rc" in readings and absn and s[ix["c2_reach"]] == "present") else flip,
                   "c4": [n for n, t in blk if t.startswith("creator:")],
                   "c3i": [n for n, t in blk if t.startswith("holder:")]}
            base = {"c1": s[ix["c1_callee_check"]], "c2": s[ix["c2_reach"]], "c4": s[ix["c4_writers"]], "c3i": s[ix["c3i_branch_i"]]}
            val, deps = {}, {}
            for e in ("c1", "c2", "c4", "c3i"):
                d = sorted({"\t".join(c) for n in src[e] for (ly, fn, col) in lst if names(text[n], fn)
                            for c in cand if c[0] == ly and c[1] == fn and (col is None or c[2] == col)})
                un = [x for x in d if x not in adm_set]
                deps[e] = d
                v = base[e]
                if un and v == "present":
                    v = "missing"
                if un and v == "held":
                    v = "not_established"
                val[e] = v
            # (c3)(iii) per context (the committed basis; alternatives lines' 'rests on' unchanged) and the context rule
            per = dict(x.split(":", 1) for x in s[ix["c3iii_by_context"]].split(";"))
            alt_ids = sorted({re.match(r"alternatives \[context (\d+|-)", t).group(1) for n, t in blk if t.startswith("alternatives [context ")})
            has_ctx = ctxs != {"-"}
            if not has_ctx:
                use_ctx = [alt_ids[0]] if len(alt_ids) == 1 else [None]
            else:
                use_ctx = sorted(c for c in ctxs if c != "-")
            iii = [per.get(c, "not_established") if c is not None else "not_established" for c in use_ctx]
            i_ok = val["c3i"] == "held"
            iii_ok = bool(iii) and all(x == "held" for x in iii)
            c3 = iii_ok if "Ra" in readings else (i_ok and iii_ok)
            stands = [val["c1"] == "present", val["c2"] == "present", val["c4"] == "present", c3]
            if all(stands):
                sys.stderr.write("expect_write2.py: %s %s %s would be derived: a boot prediction is needed\n" % (build, brid, label))
                return 1
            uc = "partial" if any(stands) else "not_derived"
            vals = {"use_cond": uc, "violation": "undetermined", "c1": val["c1"], "c2": val["c2"], "c4": val["c4"], "c3i": val["c3i"]}
            for field, col in STATIC_FIELDS:
                out.append(["expect", brid, build, key, root, field, vals[col], "static"])
            text_static = ((not has_ctx and len(alt_ids) == 1) or
                           (has_ctx and all(per.get(c) == "not_established" for c in alt_ids)))
            ps = pred.get((build, label), [])
            if text_static:
                out.append(["expect", brid, build, key, root, "use_cond.caller_dependence.text",
                            "(i) %s; (iii) %s" % (val["c3i"], ", ".join(sorted(set(iii)))), "static"])
            else:
                ib = [p for p in ps if p[3] == "iii_boot"]
                if len(ib) != 1:
                    sys.stderr.write("expect_write2.py: %s %s: the text is not static and %d iii_boot lines\n" % (build, label, len(ib)))
                    return 1
                out.append(["expect", brid, build, key, root, "use_cond.caller_dependence.text",
                            "(i) %s; (iii) %s" % (val["c3i"], ib[0][dcol]), "boot"])
            no_held_sp = not any(re.match(r"callee-check \(success path\): \S+: held", t) for n, t in blk)
            for p in ps:
                if p[3] == "iii_boot":
                    continue
                cls = "static" if (p[3] == "link.status" and no_held_sp) else "boot"
                out.append(["expect", brid, build, key, p[2] if p[2] != "*" else root, p[3], p[dcol], cls])
            elem_out.append([build, brid, label, root, val["c1"], val["c2"], val["c4"], val["c3i"],
                             ";".join("%s:%s" % (c, per.get(c, "-")) for c in alt_ids),
                             ",".join(str(c) for c in use_ctx), uc,
                             " | ".join("%s=%s" % (e, ",".join("%s:%s:%s" % tuple(x.split("\t")[:3]) for x in deps[e]) or "none")
                                        for e in ("c1", "c2", "c4", "c3i")),
                             "text=%s link=%s" % ("static" if text_static else "boot", "static" if no_held_sp else "boot")])
    for r in out:
        counts[r[7]] += 1
    with open(a.o, "w", encoding="utf-8") as f:
        f.write("kz-expect 1\n")
        for p in plans:
            f.write(p + "\n")
        f.write(RULE + "\n")
        for r in out:
            f.write("\t".join(r) + "\n")
        f.write("lines %d\nend\n" % len(out))
    with open(a.elements_out, "w", encoding="utf-8") as f:
        f.write("# expect_write2.py: the element values by rule (results 26.4); depends_on as layer:function:column of the "
                "candidate list\n")
        f.write("\t".join(["build", "branch", "row", "root", "c1", "c2", "c4", "c3i", "c3iii_by_context", "use_contexts",
                           "use_cond", "depends_on", "class"]) + "\n")
        for r in elem_out:
            f.write("\t".join(r) + "\n")
    print("expectations: %d lines over %d branches, %d rows; class static %d, boot %d, format %d"
          % (len(out), len(br), len(rows), counts["static"], counts["boot"], counts["format"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
