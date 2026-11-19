#!/usr/bin/env python3
"""explain_read.py -- the reader part of the explanation script, revision 4 (plan/c1_closure_explain_rev4_spec_20261118.md
sections 2, 4 and 10).

    python3 -I explain_read.py --sidecar plan.side --manifest plan.kzs --bindings binds.kzs --relations relations.txt \
        --keys keys.txt --facts-view facts_view.txt --bprime-basis basis.kz --build-label NAME --branches branches.txt \
        --normal-out normal.json [--pairs pairs.tsv] [--namespaces FILE]... [--candidates FILE] [--label-record TEXT] \
        [--dump-text tx.dump.txt --run-meta run.meta --chain chain.txt --startup-script scr [--stop-eval tsv]
         [--instance-list list] [--boot-budget tsv]]

It opens only the files named on the command line, hashes each, checks the form of each (spec section 4), and writes the
normal form `kz-explain-normal 1` (one JSON file): every value carries the file it came from and its line, every dump
event its actor and sequence number.  It computes no item, element or verdict.  A failed check refuses that input only
(input_checks); a failed check of the branch inputs (branch file, candidate list, admission files) stops with rc 2 and
writes nothing.  rc 0 written; 2 branch inputs refused; 64 usage.  Standard library only; imports no other part.
"""
import argparse
import hashlib
import json
import os
import re
import sys

READER_REV = "explain_read.py revision 4 (2026-11-19)"
INPUTS = {}
CHECKS = []
UNPARSED = {}
KV = re.compile(r'(\w[\w-]*)=("(?:[^"\\]|\\.)*"|\S+)')


def read_input(name, path):
    with open(path, "rb") as f:
        data = f.read()
    INPUTS[name] = {"path": path, "sha256": hashlib.sha256(data).hexdigest()}
    return data.decode("utf-8", errors="replace")


def sha_of(name):
    return INPUTS[name]["sha256"] if name in INPUTS else None


def check(name, ok, why):
    CHECKS.append({"check": name, "status": "ok" if ok else "refused", "why": why})
    return ok


def unparsed(kind, where):
    UNPARSED.setdefault(kind, []).append(where)


def kv(text):
    out = {}
    for k, v in KV.findall(text):
        if v.startswith('"'):
            v = v[1:-1].replace('\\"', '"').replace("\\\\", "\\")
        out[k] = v
    return out


def none_dash(r):
    return None if r in (None, "-", "") else r


def reason_of(text):
    """The reason= value of a sidecar need or gap line: a quoted value, or name or name(sub-reason) with the sub-reason
    up to its balanced closing parenthesis (the sub-reason may hold spaces: JR 3.5 'reason(sub-reason)').  An
    unbalanced value runs to the end of the line, and the vocabulary check refuses it."""
    m = re.search(r"(?:^|\s)reason=", text)
    if not m:
        return None
    s = text[m.end():]
    if s.startswith('"'):
        q = re.match(r'"((?:[^"\\]|\\.)*)"', s)
        return q.group(1).replace('\\"', '"') if q else s
    n = re.match(r"[A-Za-z_]\w*", s)
    if not n:
        return s.split()[0] if s.split() else ""
    rest = s[n.end():]
    if not rest.startswith("("):
        return n.group(0)
    depth = 0
    for i, ch in enumerate(rest):
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0:
                return n.group(0) + rest[:i + 1]
    return n.group(0) + rest


# ------------------------------------------------------------------------------------------------ plan
def parse_manifest(text, fname):
    sites = {}
    head = {}
    for n, ln in enumerate(text.splitlines(), 1):
        if ln.startswith("site "):
            p = ln.split(" ", 2)
            a = kv(p[2] if len(p) > 2 else "")
            a["ref"] = a.get("ref", "")[:300]
            a["src"] = [fname, n]
            a["values"] = {}
            sites[p[1]] = a
        elif ln.startswith("value "):
            p = ln.split(" ", 3)
            if p[1] in sites:
                v = kv(p[3] if len(p) > 3 else "")
                v.pop("ref", None)
                sites[p[1]]["values"][p[2]] = v
        elif ln.startswith("# sidecar sha256="):
            head["sidecar_sha256"] = ln.split("=", 1)[1].strip()
    m = re.search(r"sidecar sha256=([0-9a-f]{64})", text)
    head["sidecar_named"] = m.group(1) if m else None
    return {"sites": sites, "head": head}


SC_NEW = ("namespace-entry ", "namespace-layer ", "lifetime ", "lifetime-complete ", "context ")


def parse_sidecar(text, fname):
    sc = {"rows": {}, "maps": {}, "needs": {}, "gaps": {}, "inputs": [], "budget": None, "bounds": [], "head": {},
          "stop": [], "creation": None, "namespace_entries": [], "namespace_layers": [], "lifetimes": [],
          "lifetime_complete": {}, "contexts": [], "first_line": None, "manifest_revision": None}
    lines = text.splitlines()
    sc["first_line"] = lines[0] if lines else None
    for n, ln in enumerate(lines, 1):
        src = [fname, n]
        if ln.startswith("row "):
            p = ln.split(" ", 3)
            a = kv(p[3]) if len(p) > 3 else {}
            a["key"], a["id"], a["src"] = p[1], p[2], src
            sc["rows"][p[1]] = a
        elif ln.startswith("map "):
            p = ln.split()
            sc["maps"][p[1]] = p[2:]
        elif ln.startswith("need row "):
            p = ln.split(" ", 5)
            a = kv(p[5]) if len(p) > 5 else {}
            sc["needs"].setdefault(p[2], []).append({"what": p[3], "state": p[4], "reason": none_dash(reason_of(ln)),
                                                      "note": a.get("note"), "sites": a.get("sites"), "src": src})
        elif ln.startswith("gap row "):
            m = re.match(r'gap row (\S+) "((?:[^"\\]|\\.)*)" (.*)$', ln)
            if m:
                a = kv(m.group(3))
                sc["gaps"].setdefault(m.group(1), []).append({"text": m.group(2), "reason": none_dash(reason_of(m.group(3))),
                                                              "rule": a.get("rule"), "src": src})
        elif ln.startswith("input "):
            sc["inputs"].append({"text": ln[len("input "):], "src": src})
        elif ln.startswith("budget "):
            b = kv(ln[len("budget "):])
            m = re.search(r"truncation=(.*?) unresolved=", ln)
            if m:
                b["truncation"] = m.group(1)
            sc["budget"] = b
        elif ln.startswith("bound "):
            sc["bounds"].append(ln[len("bound "):])
        elif ln.startswith("stop-event "):
            sc["stop"].append(ln[len("stop-event "):])
        elif ln.startswith("creation "):
            sc["creation"] = ln[len("creation "):]
        elif ln.startswith("manifest-revision "):
            sc["manifest_revision"] = ln[len("manifest-revision "):]
        elif ln.startswith("namespace-entry "):
            a = kv(ln)
            a["src"] = src
            sc["namespace_entries"].append(a)
            if not all(k in a for k in ("row", "data", "sha256", "kind", "value", "lookup", "create", "compare")):
                unparsed("sidecar namespace-entry", src)
        elif ln.startswith("namespace-layer "):
            a = kv(ln)
            a["src"] = src
            sc["namespace_layers"].append(a)
        elif ln.startswith("lifetime-complete "):
            m = re.match(r"lifetime-complete kind=(\S+) (yes|no\(.*\))$", ln)
            if m:
                sc["lifetime_complete"][m.group(1)] = {"value": m.group(2), "src": src}
            else:
                unparsed("sidecar lifetime-complete", src)
        elif ln.startswith("lifetime "):
            m = re.match(r"lifetime (\S+) (delete|rollback|clear-table|clear-slot) (\S+) (\S+) data (\S+) line (\d+): "
                         r"(applies|not-applicable|not-a-lifetime-end\S*)(.*)$", ln)
            if not m:
                unparsed("sidecar lifetime", src)
                continue
            rest = m.group(8)
            ms = re.search(r"\bsites=([\d,]+)\s*$", rest)
            mr = re.search(r"\breentry=(none|possible)\b", rest)
            sc["lifetimes"].append({"kind": m.group(1), "form": m.group(2), "function": m.group(3), "def": m.group(4),
                                    "data": m.group(5), "data_line": int(m.group(6)), "state": m.group(7),
                                    "sites": ms.group(1).split(",") if ms else [], "reentry": mr.group(1) if mr else None,
                                    "text": ln[:600], "src": src})
        elif ln.startswith("context "):
            m = re.match(r'context row=(\S+) id=(\d+) frame=(\S+) chain="((?:[^"\\]|\\.)*)"', ln)
            if not m:
                unparsed("sidecar context", src)
                continue
            fr = None if m.group(3) == "-" else m.group(3).split(",")
            sc["contexts"].append({"row": m.group(1), "id": int(m.group(2)), "frame": fr, "chain": m.group(4), "src": src})
        else:
            w = ln.split(" ", 1)
            if w[0] in ("generator", "rules", "selection", "priority", "classes", "facts-artifact", "basis-artifact",
                        "control", "control-budget", "plan-id"):
                sc["head"][w[0]] = w[1] if len(w) > 1 else ""
    g = sc["head"].get("generator", "")
    m = re.search(r"tool_sha256=([0-9a-f]{64})", g)
    sc["generator_tool_sha256"] = m.group(1) if m else None
    fi = [x["text"] for x in sc["inputs"] if x["text"].startswith("facts ")]
    m = re.search(r"sha256=([0-9a-f]{64})", fi[0]) if fi else None
    sc["facts_sha256"] = m.group(1) if m else None
    sc["namespace_data"] = []
    for x in sc["inputs"]:
        if x["text"].startswith("namespace-data "):
            a = kv(x["text"])
            sc["namespace_data"].append({"sha256": a.get("sha256"), "path": a.get("path"), "src": x["src"]})
    return sc


def parse_bindings(text):
    bound, other = {}, {}
    for ln in text.splitlines():
        if not ln.startswith("bind "):
            continue
        a = kv(ln)
        s = a.get("site")
        if s is None or not s.isdigit():
            continue
        st = a.get("status", "")
        if st in ("bound", "duplicated", "inlined"):
            bound[s] = bound.get(s, 0) | int(a.get("vals", "0x0"), 16)
        else:
            other.setdefault(s, []).append(st + (" " + a["reason"] if "reason" in a else ""))
    return {"bound": bound, "other": other}


# ------------------------------------------------------------------------------------------------ relations
ROW_HEAD = re.compile(r"^  ([ABC]\d+) ")
NS_HEAD = re.compile(r"^  namespace (\S+) ")
LINE_KINDS = [  # (kind, prefix, grammar) of the row lines revision 4 reads (static_facts.md 5x)
    ("success-path", "callee-check (success path): ",
     re.compile(r"^callee-check \(success path\): (\S+): (held|missing\(.*?\)): success value (-?\d+) ")),
    ("absence-origins", "absence origins: ", re.compile(r"^absence origins: (-?\d+) \[[^\]]*\]: (\d+) origin\(s\): ")),
    ("holder", "holder: ", re.compile(r"^holder: (field \S+|local \S+ in \S+|\[)")),
    ("alternatives", "alternatives [context ",
     re.compile(r"^alternatives \[context (\d+|-)(?::[^\]]*)?\]: (none found \(exploration complete\)|found: |not established)")),
]
NS_KINDS = [
    ("ns-writers", "writers [", re.compile(r"^writers \[([^\]]*)\] of (\S+): (\d+): ")),
    ("ns-address-uses", "stored-name address uses ", re.compile(r"^stored-name address uses \(.*?\): (\d+): ")),
    ("ns-buffer", "stored-name buffer [", re.compile(r"^stored-name buffer \[([^\]]*)\]: ")),
    ("ns-lifetime-end", "lifetime end [", re.compile(r"^lifetime end \[([^\]]*)\]: ")),
]


def parse_relations(text, fname, wanted_ids):
    lines = text.splitlines()
    head = {"first": lines[0] if lines else None, "rules_lines": [], "facts": None, "rules": None, "tool": None,
            "object_access": None, "p0_compare": []}
    for n, ln in enumerate(lines[:60], 1):
        w = ln.split(" ", 1)
        if w[0] == "facts":
            head["facts"] = w[1] if len(w) > 1 else ""
        elif ln.startswith("rules with object access: "):
            head["object_access"] = {"text": ln[:400], "src": [fname, n],
                                     "rev14_lines": "with RESULT_FLOW and RESULT_EFFECT" in ln}
            head["rules_lines"].append([fname, n])
        elif w[0] == "rules" and not ln.startswith("rules with "):
            head["rules"] = w[1] if len(w) > 1 else ""
            head["rules_lines"].append([fname, n])
        elif ln.startswith("rules with "):
            head["rules_lines"].append([fname, n])
        elif w[0] == "tool":
            m = re.search(r"tool_sha256=([0-9a-f]{64})", ln)
            head["tool"] = m.group(1) if m else None
            head["tool_line"] = [fname, n]
        elif ln.startswith("p0-compare "):
            head["p0_compare"].append({"text": ln[:3000], "src": [fname, n]})
    rows, blocks = {}, {}
    cur = None
    curb = None
    for n, ln in enumerate(lines, 1):
        m = ROW_HEAD.match(ln)
        if m:
            curb = None
            cur = {"id": m.group(1), "line": n, "head": ln.strip(), "body": []} if m.group(1) in wanted_ids else None
            if cur is not None:
                rows[m.group(1)] = cur
            continue
        mb = NS_HEAD.match(ln)
        if mb:
            cur = None
            curb = {"field": mb.group(1), "line": n, "head": ln.strip(), "body": []}
            blocks[mb.group(1)] = curb
            continue
        if ln.startswith("      "):
            if cur is not None:
                t = ln.strip()
                kind = None
                for k, pre, gram in LINE_KINDS:
                    if t.startswith(pre):
                        kind = k
                        if not gram.match(t):
                            unparsed("relations " + k, [fname, n])
                            kind = k + " (unparsed)"
                cur["body"].append([n, t, kind])
            elif curb is not None:
                t = ln.strip()
                kind = None
                if not ln.startswith("        "):
                    for k, pre, gram in NS_KINDS:
                        if t.startswith(pre):
                            kind = k
                            if not gram.match(t):
                                unparsed("relations " + k, [fname, n])
                                kind = k + " (unparsed)"
                curb["body"].append([n, t, kind])
        else:
            cur, curb = None, None
    return head, rows, blocks


def parse_keys(text):
    keys, head = {}, {}
    for ln in text.splitlines():
        p = ln.split(" ", 2)
        if len(p) == 3 and re.fullmatch(r"[ABC]\d+", p[0]) and re.fullmatch(r"[0-9a-f]{16}", p[1]):
            keys[p[0]] = [p[1], p[2]]
        elif p[0] in ("facts", "rules"):
            head[p[0]] = ln.split(" ", 1)[1] if " " in ln else ""
    return head, keys


# ------------------------------------------------------------------------------------------------ pair file
PAIR_COLS = ["row_key", "path", "use_pos", "use_function", "use_root", "other_kind", "other_pos", "other_function",
             "other_root", "value", "edges", "premises", "row_line"]


def parse_pairs(text, fname, wanted_keys):
    lines = text.splitlines()
    out = {"head": {}, "tail": None, "lines": {}, "ok": True, "why": []}

    def bad(w):
        out["ok"] = False
        out["why"].append(w)
    if not lines or lines[0] != "kz-order-pairs 1":
        bad("first line is not kz-order-pairs 1")
        return out
    i = 1
    for key in ("facts", "tool", "relations", "order", "domain", "columns"):
        if i >= len(lines) or not lines[i].startswith(key + " "):
            bad("header line %d is not %s" % (i + 1, key))
            return out
        out["head"][key] = lines[i][len(key) + 1:]
        i += 1
    m = re.search(r"tool_sha256=([0-9a-f]{64})", out["head"]["tool"])
    out["head"]["tool_sha256"] = m.group(1) if m else None
    if out["head"]["columns"].split("\t") != PAIR_COLS:
        bad("the columns line is not the 13 columns of order rules revision 8")
    keys, n_pairs, n_edges, n_lines = set(), 0, 0, 0
    vals = {}
    for n in range(i, len(lines)):
        ln = lines[n]
        if ln.startswith("rows "):
            out["tail"] = {"text": ln, "src": [fname, n + 1]}
            if n + 1 >= len(lines) or lines[n + 1] != "end":
                bad("no end line after the tail")
            break
        if ln.startswith("edge\t"):
            n_edges += 1
            continue
        f = ln.split("\t")
        if len(f) != 13:
            bad("line %d has %d columns" % (n + 1, len(f)))
            unparsed("pairs line", [fname, n + 1])
            continue
        n_lines += 1
        keys.add(f[0])
        v = f[9]
        if not v.startswith("no_pair"):
            n_pairs += 1
            base = v.split("(")[0]
            vals[base] = vals.get(base, 0) + 1
        if f[0] in wanted_keys:
            d = dict(zip(PAIR_COLS, f))
            d["src"] = [fname, n + 1]
            out["lines"].setdefault(f[0], []).append(d)
    if out["tail"] is None:
        bad("no tail line")
        return out
    t = out["tail"]["text"]
    m = re.match(r"rows (\d+) pairs (\d+) (.*?) no_pair (\d+)(?: edges_not_in_tsv (\d+))?$", t)
    if not m:
        bad("the tail line is not of the form of order rules revision 8")
        return out
    if int(m.group(1)) != len(keys):
        bad("tail rows %s, row keys in the file %d" % (m.group(1), len(keys)))
    if int(m.group(2)) != n_pairs:
        bad("tail pairs %s, pair lines %d" % (m.group(2), n_pairs))
    per = dict((a, int(b)) for a, b in re.findall(r"(\w+) (\d+)", m.group(3)))
    if sum(per.values()) != n_pairs:
        bad("the per-value counts of the tail do not add up to its pairs")
    for k, c in vals.items():
        if per.get(k, 0) != c:
            bad("tail %s %d, lines %d" % (k, per.get(k, 0), c))
    if int(m.group(5) or 0) != n_edges:
        bad("tail edges_not_in_tsv %s, edge lines %d" % (m.group(5) or "0", n_edges))
    out["counts"] = {"rows": len(keys), "pair_lines": n_pairs, "lines": n_lines, "edge_lines": n_edges, "per_value": per}
    return out


# ------------------------------------------------------------------------------------------------ candidates, branches
CAND_COLS = ["layer", "function", "column", "data_file", "data_sha256", "data_line", "named_by", "kind"]


def parse_candidates(text, fname):
    lines = text.splitlines()
    out = {"plans": [], "lines": [], "ok": True, "why": [], "kinds": {}}
    if not lines or lines[0] != "kz-candidates 1":
        out["ok"] = False
        out["why"].append("first line is not kz-candidates 1")
        return out
    tail = None
    for n, ln in enumerate(lines[1:], 2):
        if ln.startswith("rule ") or ln.startswith("columns "):
            if ln.startswith("columns ") and ln[len("columns "):].split("\t") != CAND_COLS:
                out["ok"] = False
                out["why"].append("columns line is not the eight columns of the spec 4.1")
            continue
        if ln.startswith("plan "):
            p = ln.split(" ", 2)
            out["plans"].append(p[1] if len(p) > 1 else "")
            continue
        if ln.startswith("lines "):
            tail = (n, ln)
            continue
        if ln == "end":
            break
        f = ln.split("\t")
        if len(f) != 8:
            out["ok"] = False
            out["why"].append("line %d has %d columns" % (n, len(f)))
            continue
        d = dict(zip(CAND_COLS, f))
        d["line"] = ln
        d["src"] = [fname, n]
        out["lines"].append(d)
    if tail is None:
        out["ok"] = False
        out["why"].append("no tail line")
        return out
    m = re.match(r"lines (\d+) kinds (\d+): (.*)$", tail[1])
    if not m or int(m.group(1)) != len(out["lines"]):
        out["ok"] = False
        out["why"].append("the tail count is not the number of lines")
    kinds = {}
    for d in out["lines"]:
        kinds[d["kind"]] = kinds.get(d["kind"], 0) + 1
    out["kinds"] = kinds
    if m and int(m.group(2)) != len(kinds):
        out["ok"] = False
        out["why"].append("the tail kind count is not the number of kinds")
    seen = set()
    for d in out["lines"]:
        k = (d["layer"], d["function"], d["column"])
        if k in seen:
            out["ok"] = False
            out["why"].append("(layer, function, column) %s twice" % (k,))
        seen.add(k)
    return out


def data_kinds(data):
    if data == "D1":
        return None
    if data == "D2":
        return "all"
    m = re.fullmatch(r"D2\[([^\]]+)\]", data)
    return sorted(m.group(1).split(",")) if m else False


def parse_admitted(text, fname, cand, cand_sha):
    lines = text.splitlines()
    why = []
    if not lines or lines[0] != "kz-admitted 1":
        return None, ["first line is not kz-admitted 1"]
    out = {"candidates": None, "branch": None, "grounds": {}, "lines": [], "src": fname}
    tail = None
    for n, ln in enumerate(lines[1:], 2):
        if ln.startswith("candidates "):
            out["candidates"] = ln.split(" ", 1)[1].strip()
        elif ln.startswith("branch "):
            out["branch"] = ln.split(" ", 1)[1].strip()
        elif ln.startswith("ground "):
            out.setdefault("ground_lines", []).append(ln)
        elif ln.startswith("columns "):
            continue
        elif ln.startswith("lines "):
            tail = ln
        elif ln == "end":
            break
        else:
            out["lines"].append(ln)
    if out["candidates"] != cand_sha:
        why.append("candidates sha256 %s is not the candidate list's %s" % (out["candidates"], cand_sha))
    cl = {d["line"]: d for d in cand["lines"]} if cand else {}
    for ln in out["lines"]:
        if ln not in cl:
            why.append("a line not in the candidate list: %s" % ln[:120])
    if tail is None or tail != "lines %d" % len(out["lines"]):
        why.append("the tail count is not the number of lines")
    dk = data_kinds(out["branch"] or "")
    lk = sorted({cl[ln]["kind"] for ln in out["lines"] if ln in cl})
    if dk is False or dk is None:
        why.append("branch %r is not D2 or D2[kinds]" % out["branch"])
    else:
        want = sorted(cand["kinds"]) if dk == "all" else dk
        if lk != want:
            why.append("the kinds of the lines %s are not the kinds of the branch %s" % (lk, want))
        for k in want:
            allk = sorted(d["line"] for d in cand["lines"] if d["kind"] == k)
            mine = sorted(ln for ln in out["lines"] if ln in cl and cl[ln]["kind"] == k)
            if allk != mine:
                why.append("kind %s: %d of its %d lines admitted (a kind is admitted whole)" % (k, len(mine), len(allk)))
        for k in want:
            g = [ln for ln in out.get("ground_lines", []) if ln.startswith("ground %s: " % k)]
            if len(g) != 1:
                why.append("%d ground lines of kind %s" % (len(g), k))
            else:
                out["grounds"][k] = g[0][len("ground %s: " % k):]
    out["kinds"] = lk
    return out, why


READINGS = ("Ra", "Rb", "Rc", "Rd", "Re", "Rf")


def parse_branches(text, fname, cand, cand_sha, load):
    lines = text.splitlines()
    why = []
    if not lines or lines[0] != "kz-eval-branches 1":
        return None, ["first line is not kz-eval-branches 1"]
    out = []
    tail = None
    named_cand = None
    for n, ln in enumerate(lines[1:], 2):
        if ln.startswith("rule "):
            continue
        if ln.startswith("candidates "):
            named_cand = ln.split(" ", 1)[1].strip()
            continue
        if ln.startswith("lines "):
            tail = ln
            continue
        if ln == "end":
            break
        m = re.match(r"branch (\S+) data=(.+?) readings=(\S+) admitted=(\S+) admitted_sha256=(\S+)$", ln)
        if not m:
            why.append("line %d is not a branch line" % n)
            continue
        b = {"id": m.group(1), "data": m.group(2), "readings": m.group(3).split(","), "admitted_path": m.group(4),
             "admitted_sha256": m.group(5), "src": [fname, n]}
        if not re.fullmatch(r"[A-Za-z0-9_.-]+", b["id"]):
            why.append("branch id %r is no file name" % b["id"])
        if b["readings"] != ["R0"] and not all(r in READINGS for r in b["readings"]):
            why.append("branch %s: readings %s" % (b["id"], b["readings"]))
        dk = data_kinds(b["data"])
        if dk is False:
            why.append("branch %s: data %s" % (b["id"], b["data"]))
        elif dk is None:
            if b["admitted_path"] != "none":
                why.append("branch %s: D1 with an admission file" % b["id"])
            b["admitted"] = None
        else:
            if cand is None:
                why.append("branch %s: %s without a candidate list" % (b["id"], b["data"]))
                continue
            try:
                txt, sha = load(b["admitted_path"])
            except OSError as e:
                why.append("branch %s: admission file not read: %s" % (b["id"], e))
                continue
            if sha != b["admitted_sha256"]:
                why.append("branch %s: admission file sha256 %s, the branch line names %s" % (b["id"], sha, b["admitted_sha256"]))
            adm, w = parse_admitted(txt, b["admitted_path"], cand, cand_sha)
            why.extend("branch %s: %s" % (b["id"], x) for x in w)
            if adm is not None and adm["branch"] != b["data"]:
                why.append("branch %s: the admission file's branch %s is not %s" % (b["id"], adm["branch"], b["data"]))
            b["admitted"] = adm
        out.append(b)
    if named_cand != (cand_sha or "none"):
        why.append("the branch file names candidates %s, the candidate list given is %s" % (named_cand, cand_sha or "none"))
    if tail != "lines %d" % len(out):
        why.append("the tail count is not the number of branch lines")
    if len({b["id"] for b in out}) != len(out):
        why.append("a branch id twice")
    return out, why


# ------------------------------------------------------------------------------------------------ dump and run files
EV = re.compile(r"^ev actor=(\S+) aseq=(\d+) site=(\d+)\[[^\]]*\] bind=(\d+) link=\d+ t=(\d+) pc=\S+ key=(\S+) "
                r"v=\[([^\]]*)\](?: str=(\"(?:[^\"\\]|\\.)*\"))? flags=(\S*)$")


def parse_dump(text, fname):
    d = {"run": {}, "runx": {}, "actors": {}, "events": [], "loss": [], "format": {}, "format_version": None,
         "check_ok": None, "modules": {}, "buildids": [], "tok_counts": {"tok-create": 0, "tok-return": 0, "tok-start": 0},
         "unparsed_events": 0}
    for n, ln in enumerate(text.splitlines(), 1):
        if ln.startswith("ev "):
            m = EV.match(ln)
            if not m:
                d["unparsed_events"] += 1
                unparsed("dump ev", [fname, n])
                continue
            v = [None if x == "-" else int(x, 16) for x in m.group(7).split(",")] if m.group(7) else []
            s = m.group(8)[1:-1] if m.group(8) is not None else None
            d["events"].append({"actor": m.group(1), "aseq": int(m.group(2)), "site": m.group(3), "bind": int(m.group(4)),
                                "module": int(m.group(4)) >> 16, "t": int(m.group(5)), "key": m.group(6), "v": v, "s": s,
                                "flags": m.group(9).split(",") if m.group(9) else [], "src": [fname, n]})
        elif ln.startswith("run "):
            d["run"].update(kv(ln))
        elif ln.startswith("runx "):
            d["runx"].update(kv(ln))
            m = re.search(r"initial=(\S+)", ln)
            if m:
                d["runx"]["initial_key"] = m.group(1)
        elif ln.startswith("actor "):
            p = ln.split(" ", 2)
            d["actors"][p[1]] = kv(p[2])
        elif ln.startswith("loss "):
            d["loss"].append(ln)
        elif ln.startswith("module "):
            p = ln.split()
            d["modules"][p[1]] = kv(ln)
        elif ln.startswith("buildid "):
            m = re.match(r'buildid object=(main|module:\d+) path="((?:[^"\\]|\\.)*)" build_id=(\S+) '
                         r'note=(present|absent|unreadable) source=(start|registration) actor=(\S+) t=(\d+)( path_cut=yes)?$', ln)
            if not m:
                unparsed("dump buildid", [fname, n])
                continue
            d["buildids"].append({"object": m.group(1), "path": m.group(2), "build_id": m.group(3), "note": m.group(4),
                                  "source": m.group(5), "actor": m.group(6), "t": int(m.group(7)),
                                  "path_cut": bool(m.group(8)), "src": [fname, n]})
        elif ln.startswith("tok-"):
            w = ln.split(" ", 1)[0]
            if w in d["tok_counts"]:
                d["tok_counts"][w] += 1
            else:
                unparsed("dump tok", [fname, n])
        elif ln.startswith("format "):
            d["format"] = kv(ln)
            m = re.match(r"format kzrt-dump (\d+)\.(\d+)", ln)
            d["format_version"] = [int(m.group(1)), int(m.group(2))] if m else None
        elif ln.startswith("check ok") or ln.startswith("check FAIL"):
            d["check_ok"] = ln
    d["run_manifest"] = d["run"].get("manifest_sha256")
    d["module_manifests"] = sorted({m.get("manifest_sha256") for m in d["modules"].values() if m.get("manifest_sha256")})
    return d


def parse_chain(text, fname):
    c = {"lines": [], "same_build_objects": [], "same_build": None, "chain": None}
    for n, ln in enumerate(text.splitlines(), 1):
        f = ln.split("\t")
        if len(f) < 2:
            continue
        c["lines"].append({"name": f[0], "status": f[1], "detail": f[2] if len(f) > 2 else "", "src": [fname, n]})
        if f[0] == "same_build_object":
            obj = (f[2].split() or ["?"])[0] if len(f) > 2 else "?"
            c["same_build_objects"].append({"object": obj, "status": f[1], "detail": f[2] if len(f) > 2 else "",
                                            "src": [fname, n]})
        elif f[0] == "same_build":
            c["same_build"] = {"status": f[1], "detail": f[2] if len(f) > 2 else "", "src": [fname, n]}
        elif f[0] == "chain":
            c["chain"] = f[1]
    return c


# ------------------------------------------------------------------------------------------------ main
def main(argv):
    ap = argparse.ArgumentParser()
    for a in ("sidecar", "manifest", "bindings", "relations", "keys", "facts-view", "bprime-basis", "dump-text",
              "run-meta", "chain", "stop-eval", "startup-script", "instance-list", "boot-budget", "build-label",
              "normal-out", "pairs", "candidates", "branches", "label-record"):
        ap.add_argument("--" + a)
    ap.add_argument("--namespaces", action="append", default=[])
    o = ap.parse_args(argv)
    for n in ("sidecar", "manifest", "bindings", "relations", "keys", "facts_view", "bprime_basis", "build_label",
              "normal_out", "branches"):
        if getattr(o, n) is None:
            print("explain_read.py: --%s is required" % n.replace("_", "-"), file=sys.stderr)
            return 64
    if os.path.exists(o.normal_out):
        print("explain_read.py: %s exists" % o.normal_out, file=sys.stderr)
        return 64
    if o.dump_text:
        for n in ("run_meta", "chain", "startup_script"):
            if getattr(o, n) is None:
                print("explain_read.py: --%s is required with --dump-text" % n.replace("_", "-"), file=sys.stderr)
                return 64
    sc = parse_sidecar(read_input("sidecar", o.sidecar), os.path.basename(o.sidecar))
    mtext = read_input("manifest", o.manifest)
    man = parse_manifest(mtext, os.path.basename(o.manifest))
    binds = parse_bindings(read_input("bindings", o.bindings))
    khead, keys = parse_keys(read_input("keys", o.keys))
    planned = {k: r for k, r in sc["rows"].items() if r.get("state") == "planned" or r.get("control-state") == "planned"}
    wanted_ids = {r["id"] for r in planned.values()}
    rel_name = os.path.basename(o.relations)
    rhead, rows, blocks = parse_relations(read_input("relations", o.relations), rel_name, wanted_ids)
    fv = read_input("facts_view", o.facts_view)
    read_input("bprime_basis", o.bprime_basis)
    mism = []
    if man["head"]["sidecar_named"] != sha_of("sidecar"):
        mism.append("the manifest names another sidecar")
    facts_sha = sc["facts_sha256"]
    if rhead.get("facts") != facts_sha:
        mism.append("the relations come from facts %s, the plan from %s" % (rhead.get("facts"), facts_sha))
    if khead.get("facts") != facts_sha:
        mism.append("the keys come from other facts")
    if re.search(r"sha256=([0-9a-f]{64})", fv) is None or (facts_sha or "-") not in fv:
        mism.append("the facts view does not name the plan's facts")
    # relations tool line (static_facts.md 5x): comparable only when present
    if rhead.get("tool") is None:
        check("relations tool line", True, "no tool line in the relations (revision 13 or earlier): not comparable")
    else:
        check("relations tool line", rhead["tool"] == sc["generator_tool_sha256"],
              "relations tool_sha256 %s, sidecar generator %s" % (rhead["tool"][:16], (sc["generator_tool_sha256"] or "-")[:16]))
    check("relations revision 14 lines", bool(rhead.get("object_access") and rhead["object_access"]["rev14_lines"]),
          "rules with object access line %s" % ("with the revision 14 lines" if rhead.get("object_access") and
                                                rhead["object_access"]["rev14_lines"] else "absent or without RESULT_FLOW"))
    check("sidecar form", sc["first_line"] == "kz-plan-sidecar 2" and (sc["manifest_revision"] or "").startswith("4"),
          "first line %r, manifest-revision %r" % (sc["first_line"], sc["manifest_revision"]))
    # pair file
    pairs = None
    if o.pairs:
        pairs = parse_pairs(read_input("pairs", o.pairs), os.path.basename(o.pairs), set(planned))
        ok = pairs["ok"]
        if not check("pairs form", ok, "; ".join(pairs["why"]) or "form, columns, tail counts and edge lines agree"):
            pairs["refused"] = "the pair file's form: " + "; ".join(pairs["why"])
        else:
            if pairs["head"]["facts"] != facts_sha:
                check("pairs facts", False, "pair file facts %s, plan facts %s" % (pairs["head"]["facts"][:16], (facts_sha or "-")[:16]))
                pairs["refused"] = "the pair file comes from other facts"
            else:
                check("pairs facts", True, "pair file facts = sidecar input facts")
            ts = {pairs["head"]["tool_sha256"], sc["generator_tool_sha256"], rhead.get("tool")}
            if len(ts) != 1 or None in ts:
                check("pairs tool", False, "tool_sha256 pair file %s, sidecar %s, relations %s" % (
                    (pairs["head"]["tool_sha256"] or "-")[:16], (sc["generator_tool_sha256"] or "-")[:16], (rhead.get("tool") or "-")[:16]))
                pairs.setdefault("refused", "the pair file, the relations and the sidecar name different tools")
            else:
                check("pairs tool", True, "tool_sha256 equal in pair file, relations and sidecar")
    else:
        check("pairs given", False, "no pair file given")
    # name-space data files
    nsfiles = []
    for i, p in enumerate(o.namespaces):
        t = read_input("namespaces[%d]" % i, p)
        s = sha_of("namespaces[%d]" % i)
        first = next((ln for ln in t.splitlines() if ln.strip() and not ln.startswith("#")), "")
        rule = next((ln for ln in t.splitlines()[:5] if ln.startswith("rule ")), None)
        pinned = [x for x in sc["namespace_data"] if x["sha256"] == s]
        ok = first.startswith("semantics-data 2") and bool(pinned)
        check("namespace data %s" % os.path.basename(p), ok,
              "first line %r; %s" % (first[:40], "sha256 = the sidecar's input namespace-data line" if pinned else
                                     "sha256 %s on no input namespace-data line of the sidecar" % s[:16]))
        nsfiles.append({"path": p, "sha256": s, "first": first[:80], "rule": rule, "ok": ok})
    # candidates and branches
    cand = None
    if o.candidates:
        cand = parse_candidates(read_input("candidates", o.candidates), os.path.basename(o.candidates))
        if not cand["ok"]:
            print("explain_read.py: candidate list refused: " + "; ".join(cand["why"]), file=sys.stderr)
            return 2
        if sha_of("sidecar") not in cand["plans"]:
            print("explain_read.py: the candidate list names no plan line of this sidecar", file=sys.stderr)
            return 2

    def load(p):
        with open(p, "rb") as f:
            data = f.read()
        INPUTS["admitted:" + os.path.basename(p)] = {"path": p, "sha256": hashlib.sha256(data).hexdigest()}
        return data.decode("utf-8", errors="replace"), hashlib.sha256(data).hexdigest()
    branches, bwhy = parse_branches(read_input("branches", o.branches), os.path.basename(o.branches), cand,
                                    sha_of("candidates"), load)
    if bwhy or not branches:
        print("explain_read.py: branch inputs refused: " + "; ".join(bwhy or ["no branch line"]), file=sys.stderr)
        return 2
    m_tool = re.search(r"tool_sha256=([0-9a-f]{64})", fv)
    m_bid = re.search(r'name="extract\.build_identity" value="([^"]*)"', fv)
    m_p0 = re.search(r'name="extract\.p0_table" value="([^"]*)"', fv)
    m_pc = re.search(r'name="p0\.pthread_create" value="([^"]*)"', fv)
    facts_view = {"tool_sha256": m_tool.group(1) if m_tool else None, "build_identity": m_bid.group(1) if m_bid else None,
                  "p0_table": m_p0.group(1) if m_p0 else None, "p0_pthread_create": m_pc.group(1) if m_pc else None,
                  "name": os.path.basename(o.facts_view)}
    run = None
    if o.dump_text:
        dump = parse_dump(read_input("dump_text", o.dump_text), os.path.basename(o.dump_text))
        meta = read_input("run_meta", o.run_meta)
        chain = parse_chain(read_input("chain", o.chain), os.path.basename(o.chain))
        scr = read_input("startup_script", o.startup_script)
        stop_eval = {}
        if o.stop_eval:
            for ln in read_input("stop_eval", o.stop_eval).splitlines():
                p = ln.split("\t")
                if len(p) >= 12 and p[0] not in ("app", "summary") and not p[0].startswith("#"):
                    stop_eval[p[0]] = {"actor": p[4], "stop_event": p[6], "stop_ms": p[9], "inside_window": p[10], "status": p[11]}
        apps = []
        for ln in scr.splitlines():
            if ln.startswith("!"):
                break
            f = [x.strip() for x in ln.split(",")]
            if len(f) >= 4 and f[0] == "CFE_APP":
                apps.append([f[2], f[3]])
        exes, process_end = {}, []
        for ln in meta.splitlines():
            mm = re.match(r"file (\S+) sha256=([0-9a-f]{64})", ln)
            if mm:
                exes[mm.group(1)] = mm.group(2)
            if re.match(r"(stop=|tx_exit=|rx_exit=|SIGKILL|killed)", ln):
                process_end.append(ln.strip())
        mm = re.search(r"^manifest \S+ sha256=([0-9a-f]{64})", meta, re.M)
        if not mm or mm.group(1) != sha_of("manifest"):
            mism.append("run.meta names another manifest")
        if dump["run_manifest"] != sha_of("manifest") or any(x != sha_of("manifest") for x in dump["module_manifests"]):
            mism.append("the dump's RUN or module records carry another manifest sha256")
        refused = [x["name"] for x in chain["lines"] if x["status"] == "refused" and x["name"] not in ("same_build", "same_build_object")]
        if refused:
            mism.append("chain links refused: " + ", ".join(refused))
        bp = [x for x in chain["lines"] if x["name"] == "bprime"]
        mb = re.search(r"basis=([0-9a-f]+)", bp[0]["detail"]) if bp else None
        if mb and not sha_of("bprime_basis").startswith(mb.group(1)):
            mism.append("the B' basis given is not the chain's")
        sb = chain["same_build"]
        compat = "chain %s; same_build %s" % (chain["chain"] or "absent", sb["status"] if sb else "absent")
        exes_inst = {}
        ml = re.search(r"^instance_files .*\blist_sha256=([0-9a-f]{64})", meta, re.M)
        if o.instance_list:
            lst = read_input("instance_list", o.instance_list)
            if not ml or ml.group(1) != sha_of("instance_list"):
                mism.append("the instance list is not the one run.meta names by list_sha256")
            inst = {}
            for ln in lst.splitlines():
                mm = re.match(r"([0-9a-f]{64})  \./(\S+)$", ln)
                if mm:
                    inst[mm.group(2)] = mm.group(1)
            ex = {}
            for x in chain["lines"]:
                if not x["name"].startswith("exe exe/"):
                    continue
                pth = x["name"][len("exe exe/"):]
                il = re.search(r"inst_list\[[^\]]*\]=([0-9a-f]+)", x["detail"])
                if pth not in inst:
                    mism.append("the chain's executable %s is not in the instance list" % pth)
                    continue
                if il is None or not inst[pth].startswith(il.group(1)):
                    mism.append("the chain's instance-list value of %s differs from the instance list" % pth)
                ex[pth] = {"sha256": inst[pth], "chain": x["status"]}
            for pth, h in exes.items():
                if inst.get(pth) not in (None, h):
                    mism.append("run.meta's file line of %s differs from the instance list" % pth)
            exes_inst = {"executables_plan": ex,
                         "executables_plan_source": "the plan's executables as the chain's exe links name them; sha256 from "
                                                    "the instance list run.meta names (list_sha256 %s, %d entries)"
                                                    % (sha_of("instance_list")[:16], len(inst))}
            if not ex:
                exes_inst["executables_missing"] = "record_insufficient(no exe link in the chain record)"
        else:
            exes_inst = {"executables_plan": None,
                         "executables_missing": "record_insufficient(the instance list is not given: run.meta's file lines only)"}
        bb = None
        if o.boot_budget:
            bb = {}
            for ln in read_input("boot_budget", o.boot_budget).splitlines():
                if ln and not ln.startswith("#") and "\t" in ln:
                    k, v = ln.split("\t", 1)
                    bb[k] = v
        run = {"dump": dump, "chain": chain, "apps": apps, "stop_eval": stop_eval, "exes": exes, "exes_inst": exes_inst,
               "process_end": process_end, "compat": compat, "boot_budget": bb}
        check("dump format", dump["format_version"] is not None, "format line %s" % (dump["format"] or "absent"))
        check("dump build-id lines", bool(dump["buildids"]),
              "%d buildid lines (format %s)" % (len(dump["buildids"]), ".".join(map(str, dump["format_version"] or []))))
        check("chain same_build_object lines", bool(chain["same_build_objects"]),
              "%d same_build_object lines" % len(chain["same_build_objects"]))
    normal = {"format": "kz-explain-normal 1", "reader": {"script": READER_REV, "sha256": None},
              "build_label": o.build_label, "label_record": o.label_record, "inputs": INPUTS, "input_checks": CHECKS,
              "unparsed": UNPARSED, "mismatch": mism, "rel_name": rel_name,
              "plan": {"sidecar": sc, "sites": man["sites"], "bindings": binds}, "keys": keys, "keys_head": khead,
              "relations": {"head": rhead, "rows": rows, "namespace_blocks": blocks}, "pairs": pairs,
              "namespaces": nsfiles, "candidates": cand, "branches": branches, "facts_view": facts_view, "run": run}
    try:
        with open(os.path.abspath(__file__), "rb") as f:
            normal["reader"]["sha256"] = hashlib.sha256(f.read()).hexdigest()
    except OSError:
        pass
    with open(o.normal_out, "w") as f:
        json.dump(normal, f, ensure_ascii=False, sort_keys=True)
        f.write("\n")
    print("normal %s inputs %d checks refused %d unparsed %d mismatch %d branches %d" % (
        o.normal_out, len(INPUTS), sum(1 for c in CHECKS if c["status"] == "refused"),
        sum(len(v) for v in UNPARSED.values()), len(mism), len(branches)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
