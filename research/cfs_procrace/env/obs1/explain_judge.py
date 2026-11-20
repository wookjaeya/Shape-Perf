#!/usr/bin/env python3
"""explain_judge.py -- the judgement part of the explanation script, revision 4 (plan/c1_closure_explain_rev4_spec_20261118.md
sections 3, 5, 10 and 11; judgement rules plan/ovm_rev1_20261116/JUDGEMENT_RULES_REV1.md 3.2-3.6, 8.2-8.5, 8.12, 8.13, 8.14).
Changed after the PI decision of 2026-11-19 (spec 11 items 1, 3, 4, 5 and 6): the context of a row without context lines,
a named library call with no entry, the name state of a found use, the entries the elements rest on, format check 16.

    python3 -I explain_judge.py --normal normal.json --vocabulary reason_vocabulary.txt --out DIR
    python3 -I explain_judge.py --check RECORD.json --vocabulary reason_vocabulary.txt

It opens the normal form of the reader part and the vocabulary only.  For every evaluation branch of the branch file
(given through the normal form) it writes DIR/<branch>/records/*.json, index.tsv, uses.tsv (the per-use table before any
aggregation) and refused.txt.  One record per (build, planned row, consumer actor).  Items, elements and verdicts are
computed from the normal form only; the verdict from the axis statuses only (JR 3.3).  No row id, key or expected
verdict is an input; the rules read no case name.  Standard library only; imports no other part.
rc 0 written, 3 combination refused (build_mismatch records), 4 a record refused by the format checks.
"""
import argparse
import hashlib
import json
import os
import re
import sys

JUDGE_REV = "explain_judge.py revision 4, spec 11 fixes and spec 11.5 item 1 (2026-11-19)"
# the dump 1.5 LOSS kinds that lose no event (kairos/docs/event_semantics.md 13.5 (다), (바); spec 11.5 item 1): not a loss marker
NONEVENT_LOSS = ("no-build-id", "object-table")
READING_TAG = "4.2 의 읽기 갈래(PI 결정 전)"
ITEMS = ["basis1_same_build", "basis2_ties", "basis3_absence_constant", "basis4_creator_success", "basis5_key_texts",
         "basis6_create_after_use", "ga_namespace_entry", "na_semantics", "na_bound", "da_no_name_mapping",
         "ra_same_kind", "ma_no_lifetime_end"]
NB = "link_unlinked(name binding: %s)"
R_ITEM = {
    "basis1_same_build": "record_insufficient(same build not confirmed)",
    "basis3_absence_constant": NB % "the use's status is not the absence constant of the namespace",
    "basis4_creator_success": "record_insufficient(no success constant of the creator: the callee's returns are not all followed)",
    "basis5_key_texts": "record_insufficient(key text)",
    "basis6_create_after_use": NB % "the create is not after the use",
    "ga_namespace_entry": NB % "no namespace entry in the framework-layer data",
    "na_semantics": NB % "comparison semantics not read from the pinned source",
    "na_bound": NB % "key text not validated against the namespace bound",
    "da_no_name_mapping": NB % "a name mapping into the table is not excluded",
    "ra_same_kind": NB % "the object kind of the lookup and the create is not shown the same",
    "ma_no_lifetime_end": NB % "a delete or retire of the name between the use and the create is not excluded",
}
R_STORED = "record_insufficient(no success constant of the creator: the stored value is not tied to the success path)"
R_OUTNOTREAD = "record_insufficient(no success constant of the creator: the creator's out id is not read)"
R_MA_SEEN = NB % "the name was deleted or retired between the use and the create"
R_CONTRA = NB % "absence after a live create of the same name"
R_MANY = NB % "more than one create bound to the name"
R_SOME = NB % "refuted for some creates only"
R_MIXED = NB % "the uses of the record differ in basis"


def fail_class(v):
    return v is not None and (v < 0 or 0xC0000000 <= v <= 0xFFFFFFFF)


FAIL_CLASS_RULE = ("failure class: v < 0 or 0xC0000000 <= v <= 0xFFFFFFFF (the method's S1 rule as ported in "
                   "kairos/lib/llvm/plan_gen.cpp:58; vocabulary plan/ORDER_VIOLATION_METHOD_20261111.md:381 (a), (b))")


def same_status(const, v):
    if const is None or v is None:
        return False
    if const == v:
        return True
    return -(1 << 31) <= v < (1 << 32) and (const & 0xFFFFFFFF) == (v & 0xFFFFFFFF)


def hx(v):
    return None if v is None else ("-0x%x" % -v if v < 0 else "0x%x" % v)


def signed(v, typ):
    if v is None:
        return None
    if typ == "s64" and v >= 1 << 63:
        return v - (1 << 64)
    return v


REASON_FORM = re.compile(r"^([a-z_]+)(?:\((.*)\))?$", re.S)


def reason_form_ok(r):
    m = REASON_FORM.match(r) if isinstance(r, str) else None
    if not m:
        return False
    depth = 0
    for ch in m.group(2) or "":
        depth += {"(": 1, ")": -1}.get(ch, 0)
        if depth < 0:
            return False
    return depth == 0


def tail_match(pos_a, tu, line, col=None):
    """A position 'file:line:col' of a relations or pair line against a manifest site (tu relative to the bundle)."""
    m = re.match(r"(.*):(\d+):(\d+)$", pos_a or "")
    if not m or tu is None or line is None:
        return False
    f, ln, c = m.group(1), m.group(2), m.group(3)
    if ln != str(line) or (col is not None and c != str(col)):
        return False
    return f == tu or f.endswith("/" + tu) or tu.endswith("/" + f)


# ------------------------------------------------------------------------------------------------ plan and run model
class Site:
    def __init__(self, n, a):
        self.n = n
        self.label = a.get("label", "")
        self.rules = self.label.split(".", 1)[1].split("+") if "." in self.label else []
        self.op = a.get("op", "")
        self.tu, self.func, self.callee = a.get("tu", ""), a.get("func", ""), a.get("callee")
        self.line, self.col = a.get("line"), a.get("col")
        self.place = a.get("place") or ("entry" if self.op.startswith("entry.") else None)
        self.entry_of = self.op[len("entry."):] if self.place == "entry" else None
        self.ref = a.get("ref", "")
        self.src = a.get("src")
        self.values = {int(i): v for i, v in a.get("values", {}).items()}

    def role_index(self, role):
        for i, v in self.values.items():
            if v.get("role") == role:
                return i
        return None

    def where(self):
        if self.place == "entry":
            return "entry of %s (%s)" % (self.entry_of, self.tu)
        if self.place == "store":
            return "store in %s at %s:%s" % (self.func, self.tu, self.line)
        return "%s of %s in %s at %s:%s" % (self.place, self.callee, self.func, self.tu, self.line)


class Event:
    __slots__ = ("actor", "aseq", "site", "t", "key", "v", "s", "flags", "module", "src")

    def __init__(self, d):
        self.actor, self.aseq, self.site, self.t, self.key = d["actor"], d["aseq"], int(d["site"]), d["t"], d["key"]
        self.v, self.s, self.flags, self.module, self.src = d["v"], d["s"], set(d["flags"]), d["module"], d["src"]


class Inst:
    def __init__(self, actor, at, after, site_at, site_after, func, callee, line):
        self.actor, self.at, self.after = actor, at, after
        self.site_at, self.site_after = site_at, site_after
        self.func, self.callee, self.line = func, callee, line

    def bracketed(self, e):
        return self.at is not None and self.after is not None and self.at.aseq < e.aseq < self.after.aseq


def akey(x):
    return int(x) if str(x).isdigit() else 0


class Run:
    def __init__(self, dump, sites):
        self.d = dump
        self.sites = sites
        self.events = [Event(e) for e in dump["events"]]
        self.by_actor = {}
        for e in self.events:
            self.by_actor.setdefault(e.actor, []).append(e)
        for a in self.by_actor:
            self.by_actor[a].sort(key=lambda e: e.aseq)
        self.insts = {a: self._instances(a) for a in self.by_actor}
        self.t_init = int(dump["run"].get("t_init", "0") or 0)
        self.t_stop = int(dump["run"].get("t_stop", "0") or 0)
        self._ties()

    def _instances(self, actor):
        out, stack = [], []
        for e in self.by_actor[actor]:
            s = self.sites.get(e.site)
            if s is None:
                continue
            if s.place == "at-call":
                stack.append(Inst(actor, e, None, s, None, s.func, s.callee, s.line))
            elif s.place == "after-call":
                hit = None
                for k in range(len(stack) - 1, -1, -1):
                    f = stack[k]
                    if (f.func, f.callee, f.line) == (s.func, s.callee, s.line):
                        hit = k
                        break
                if hit is None:
                    out.append(Inst(actor, None, e, None, s, s.func, s.callee, s.line))
                else:
                    f = stack[hit]
                    f.after, f.site_after = e, s
                    out.append(f)
                    out.extend(stack[hit + 1:])
                    del stack[hit:]
        out.extend(stack)
        out.sort(key=lambda i: (i.at or i.after).aseq)
        return out

    def owner(self, e, func=None):
        s = self.sites[e.site]
        func = func or (s.entry_of if s.place == "entry" else s.func)
        best = None
        for i in self.insts.get(e.actor, []):
            if i.callee == func and i.bracketed(e) and (best is None or i.at.aseq > best.at.aseq):
                best = i
        return best

    def events_at(self, site_ids, actor=None):
        return [e for e in self.events if e.site in site_ids and (actor is None or e.actor == actor)]

    def val(self, e, role):
        s = self.sites[e.site]
        i = s.role_index(role)
        if i is None or i >= len(e.v):
            return None
        return signed(e.v[i], s.values[i].get("type"))

    def ms(self, t):
        return round((t - self.t_init) / 1e6, 3)

    def _ties(self):
        creates, starts = [], []
        for a, insts in self.insts.items():
            for i in insts:
                if i.site_at is not None and "actor-create" in i.site_at.rules and i.at is not None:
                    creates.append((i, self.val(i.at, "creation_arg")))
        for e in self.events:
            s = self.sites.get(e.site)
            if s is not None and "actor-start" in s.rules:
                starts.append((e, self.val(e, "param0")))
        cc, cs = {}, {}
        for _, v in creates:
            cc[v] = cc.get(v, 0) + 1
        for _, v in starts:
            cs[v] = cs.get(v, 0) + 1
        self.creation = {}
        for a in self.by_actor:
            mine = [(e, v) for e, v in starts if e.actor == a]
            t = {"status": None, "why": None, "creator": None, "start": None}
            if not mine:
                t["status"], t["why"] = "unlinked", "no start-routine entry recorded in the actor"
            elif len({v for _, v in mine}) > 1:
                t["status"], t["why"] = "undetermined", "more than one start-routine entry value in the actor"
            else:
                e, v = mine[0]
                t["start"] = e
                if v is None:
                    t["status"], t["why"] = "unlinked", "the start routine's param0 is not in the record (value-missing)"
                elif cc.get(v, 0) == 0:
                    t["status"], t["why"] = "unlinked", "no creation call with the creation argument %s in the record" % hx(v)
                elif cc.get(v, 0) > 1 or cs.get(v, 0) > 1:
                    t["status"] = "undetermined"
                    t["why"] = ("collision: the value %s is the creation argument of %d creation calls and the param0 "
                                "of %d start-routine entries" % (hx(v), cc.get(v, 0), cs.get(v, 0)))
                else:
                    t["status"], t["creator"] = "determined", [i for i, w in creates if w == v][0]
                    t["why"] = "creation argument %s seen once among creation calls and once among start entries" % hx(v)
            self.creation[a] = t
        self.root = {}
        initial = self.d["runx"].get("initial_key")
        for a, evs in self.by_actor.items():
            rs = []
            for e in evs:
                s = self.sites.get(e.site)
                if s is not None and "actor-root" in s.rules:
                    m = re.search(r"actor (\S+) \(([^)]*)\)", s.ref)
                    rs.append((s.entry_of, m.group(2) if m else "unknown", e))
            names = sorted({r[0] for r in rs})
            if len(names) == 1:
                r = [x for x in rs if x[0] == names[0]][0]
                self.root[a] = {"status": "determined", "root": r[0], "kind": r[1], "event": r[2],
                                "why": "actor-root entry of %s recorded in the actor (site %d, aseq %d)" % (r[0], r[2].site, r[2].aseq)}
            elif len(names) > 1:
                self.root[a] = {"status": "undetermined", "root": None, "kind": None, "event": None,
                                "why": "entries of %d roots recorded in the actor: %s" % (len(names), ", ".join(names))}
            elif initial is not None and evs and evs[0].key == initial:
                self.root[a] = {"status": "determined", "root": "(process initial thread)", "kind": "process", "event": None,
                                "why": "the actor's key %s is the initial thread of the RUNX record" % initial}
            else:
                self.root[a] = {"status": "unlinked", "root": None, "kind": None, "event": None,
                                "why": "no actor-root entry recorded in the actor"}

    def owner_stores(self, ci):
        if ci is None or ci.after is None:
            return []
        out = []
        for e in self.by_actor.get(ci.actor, []):
            if e.aseq <= ci.after.aseq:
                continue
            s = self.sites.get(e.site)
            if s is None:
                continue
            if "actor-create" in s.rules:
                break
            if "actor-owner" in s.rules:
                out.append(e)
        return out


def evref(run, e, extra=None):
    s = run.sites[e.site]
    r = {"actor": e.actor, "aseq": e.aseq, "site": e.site, "rule": "+".join(s.rules), "where": s.where(),
         "t_ms": run.ms(e.t), "values": {}, "object": "module:%d" % e.module, "src": e.src}
    if s.entry_of:
        r["entry_of"] = s.entry_of
    for i, v in sorted(s.values.items()):
        raw = e.v[i] if i < len(e.v) else None
        r["values"][v.get("role", "v%d" % i)] = hx(signed(raw, v.get("type")))
    if e.s is not None:
        r["str"] = e.s
        r["str_truncated"] = "str-trunc" in e.flags
    if extra:
        r.update(extra)
    return r


def app_of(run, a, apps, seen):
    if a in seen:
        return {"status": "unknown", "why": "creation cycle"}
    seen.add(a)
    rt = run.root.get(a, {})
    if rt.get("status") != "determined":
        return {"status": "unknown", "why": "root not tied: " + str(rt.get("why"))}
    kind = rt["kind"]
    if kind == "script-entry":
        names = [n for e, n in apps if e == rt["root"]]
        if len(names) == 1:
            return {"status": "determined", "app": names[0], "class": "app",
                    "why": "root %s is the entry point of the startup-script application %s" % (rt["root"], names[0])}
        return {"status": "unknown", "class": "app", "why": "the startup script names %d applications with the entry %s" % (len(names), rt["root"])}
    if kind == "spawn-entry":
        ct = run.creation.get(a, {})
        if ct.get("status") != "determined":
            return {"status": "unknown", "class": "child", "why": "child task whose creation is not tied"}
        parent = ct["creator"].actor
        p = app_of(run, parent, apps, seen)
        if p.get("status") == "determined":
            return {"status": "determined", "app": p.get("app"), "class": "child", "parent_actor": parent,
                    "why": "child task created by run actor %s (%s)" % (parent, p.get("why"))}
        return {"status": "unknown", "class": "child", "parent_actor": parent, "why": "the creator's app is unknown"}
    if kind == "table-image-entry":
        return {"status": "determined", "app": "(framework core task %s)" % rt["root"], "class": "core",
                "why": "root %s is a table-image entry of the framework (basis)" % rt["root"]}
    if kind == "process":
        return {"status": "determined", "app": "(process)", "class": "process", "why": rt["why"]}
    return {"status": "unknown", "why": "root kind %s" % kind}


def actor_view(run, a, apps):
    ct, rt = run.creation.get(a, {}), run.root.get(a, {})
    v = {"run_actor": a, "key": run.by_actor[a][0].key if run.by_actor.get(a) else None,
         "creation_tie": {"status": ct.get("status"), "why": ct.get("why")},
         "root_tie": {"status": rt.get("status"), "root": rt.get("root"), "kind": rt.get("kind"), "why": rt.get("why")}}
    ci = ct.get("creator")
    if ci is not None:
        v["creation_tie"]["creation_call"] = evref(run, ci.at)
        if ci.after is not None:
            v["creation_tie"]["creation_result"] = evref(run, ci.after)
        v["creation_tie"]["start_entry"] = evref(run, ct["start"])
        v["creation_tie"]["owner_stores"] = [evref(run, e) for e in run.owner_stores(ci)]
    v["app"] = app_of(run, a, apps, set())
    return v


def pair_kind(c, p):
    """Neutral actor-pair kind (judgement rules 3.1, 8.13 item 18; spec 10.5)."""
    if c.get("run_actor") is not None and c.get("run_actor") == p.get("run_actor"):
        return "same_actor"
    a, b = c.get("app", {}), p.get("app", {})
    if a.get("status") != "determined" or b.get("status") != "determined":
        return "unknown"
    ca, cb = a.get("class"), b.get("class")
    if ca in ("app", "child") and cb in ("app", "child"):
        if a.get("app") == b.get("app"):
            if "child" in (ca, cb) and (a.get("parent_actor") == p["run_actor"] or b.get("parent_actor") == c["run_actor"]):
                return "same-app-main-child"
            return "same-app-other"
        return "app-app"
    if ca == "core" and cb == "core":
        return "core-core"
    if {ca, cb} & {"core"} and {ca, cb} & {"app", "child"}:
        return "core-app"
    if {ca, cb} & {"process"} and {ca, cb} & {"app", "child"}:
        return "process-app"
    return "unknown"


def root_text(view):
    rt = view.get("root_tie", {})
    if rt.get("status") != "determined":
        return None
    return rt.get("root"), rt.get("kind")


# ------------------------------------------------------------------------------------------------ row lines
CONST = re.compile(r"returns (-?\d+) \[([^\]]*)\]")
POS = r"(\S+:\d+:\d+)"


def row_static(row, rel_name):
    path = row["id"][0]
    body = [(b[0], b[1], b[2]) for b in row["body"]]

    def first(prefix):
        for n, t, _ in body:
            if t.startswith(prefix):
                return n, t
        return None, None
    st = {"path": path, "head": row["head"], "head_line": row["line"], "rel": rel_name}
    st["text_of"] = {n: t for n, t, _ in body}
    st["ref"] = lambda n: "%s:%d" % (rel_name, n)
    m = re.search(r"-> (\S+)#(\d+)", row["head"])
    st["callee"] = m.group(1) if m else None
    m = re.match(r"[ABC]\d+ \S+ " + POS + " in ", row["head"])
    st["use_pos"] = m.group(1) if m else None
    m = re.search(r"namespace (\S+) partition \{([^}]*)\}", row["head"])
    st["namespace"] = m.group(1) if m else None
    st["partition"] = m.group(2) if m else None
    m = re.match(r"[ABC]\d+ (\S+) -> ", row["head"]) if path == "B" else None
    st["field"] = m.group(1) if m else None
    n, t = first("actors:")
    roots = []
    if t:
        mm = re.search(r"readers \{([^}]*)\}", t) if path == "B" else re.search(r"actors: \{([^}]*)\}", t)
        if mm and mm.group(1).strip():
            for part in mm.group(1).split(","):
                r = re.match(r"(\S+) \(([^)]*)\)", part.strip())
                if r:
                    roots.append((r.group(1), r.group(2)))
    st["static_consumers"] = roots
    st["flip"] = first("flip:")
    fn, ft = st["flip"]
    consts = []
    if path == "A" and ft:
        consts = [(int(v), nm, fn) for v, nm in CONST.findall(ft)]
    elif path in ("B", "C"):
        for n2, t2, _ in body:
            if t2.startswith("callee-check: %s returns" % st["callee"]) or (path == "C" and t2.startswith("flip:")):
                consts.extend((int(v), nm, n2) for v, nm in CONST.findall(t2))
    st["consts"] = consts
    succ = sorted({(v, nm) for v, nm, _ in consts if not fail_class(v)})
    st["success"] = succ[0] if len(succ) == 1 else None
    st["success_note"] = ("one non-failure-class constant" if len(succ) == 1 else
                          "%d non-failure-class constants among the row's return constants of the use" % len(succ))
    st["creators"] = [(n, t) for n, t, _ in body if t.startswith("creator:")]
    st["not_a_pair"] = [(n, t) for n, t, _ in body if t.startswith("not a pair: ")]
    st["writers"] = [(n, t) for n, t, _ in body if re.match(r"(out-pointer|assign|memset|call|store|copy) at ", t)]
    n, t = first("field:")
    m = re.match(r"field: INIT (-?\d+)", t) if t else None
    st["init"] = int(m.group(1)) if m else None
    st["handling"] = [(n, t) for n, t, _ in body if t.startswith("handling:")]
    st["feasibility"] = first("feasibility:")
    n, t = first("incomplete:")
    st["incomplete"] = [x.strip() for x in t[len("incomplete:"):].split(";")] if t else []
    st["incomplete_line"] = n
    st["site_line"] = first("site ")
    st["layers"] = first("layers:")
    st["absence"] = first("absence: ")
    st["absence_origins"] = first("absence origins: ")
    st["success_path"] = {}
    for n2, t2, k in body:
        if k == "success-path":
            mm = re.match(r"callee-check \(success path\): (\S+): (held|missing\((.*?)\)): success value (-?\d+) ", t2)
            st["success_path"][mm.group(1)] = {"state": "held" if mm.group(2) == "held" else "missing",
                                               "why": mm.group(3), "value": int(mm.group(4)), "n": n2}
    st["holders"] = [(n2, t2) for n2, t2, k in body if k == "holder"]
    st["alternatives"] = [(n2, t2) for n2, t2, k in body if k == "alternatives"]
    st["key_writers"] = [(n2, t2) for n2, t2, _ in body if t2.startswith("writers: ")]
    st["unparsed_lines"] = [(n2, k) for n2, _, k in body if k and k.endswith("(unparsed)")]
    return st


def layers_unverified(text):
    """The P0-unverified(...) and sem-unverified(...) entries of a layers or rests-on text: [(layer, function, column)]."""
    out = []
    for m in re.finditer(r"P0-unverified\(([^)]*)\)", text or ""):
        out.extend(("P0", f.strip(), None) for f in m.group(1).split(",") if f.strip())
    for m in re.finditer(r"sem-unverified\(([^)]*)\)", text or ""):
        for f in m.group(1).split(","):
            f = f.strip()
            if ":" in f:
                a, c = f.split(":", 1)
                out.append(("framework", a, c))
            elif f and f != "…":
                out.append(("framework", f, None))
    return out


def names_function(text, fn):
    """Whether a line's text names the function fn as a whole word (spec 11 item 5: matched by the text's function name)."""
    return re.search(r"(?<![\w@])%s(?![\w])" % re.escape(fn), text or "") is not None


def element_depends(st, br, line_numbers):
    """Spec 11 item 5 (judgement rules 8.13 item 2, 8.14 item 6 (c)): the list entries an element rests on.  Every
    (layer, function, column) of the row's layers line that the element's source lines name by text is looked up in
    the candidate list; the lines found are depends_on (an empty list is 'none').  Returns (depends_on, unadmitted)."""
    listed = layers_unverified((st.get("layers") or (None, None))[1] or "")
    dep, bad = [], []
    for n in sorted(set(line_numbers)):
        t = st["text_of"].get(n, "")
        for layer, fn, col in listed:
            if names_function(t, fn):
                ok, d, b = br.listed_state(layer, fn, col)
                dep.extend(d)
                bad.extend(b)
    return sorted(set(dep)), sorted(set(bad))


def source_lines(sources):
    out = []
    for s in sources or []:
        m = re.search(r":(\d+)$", s.get("ref", "")) if s.get("kind") == "static-row" else None
        if m:
            out.append(int(m.group(1)))
    return out


ADDR_NOENTRY = re.compile(r"library: (\S+) parameter (\d+): no entry says the parameter is not written")
ADDR_UNVER = re.compile(r"library: (\S+) parameter (\d+): (\S+) unverified\b")


def address_uses(blk):
    """The stored-name address uses line of a namespace block (judgement rules 8.13 item 11 (b), 8.14 item 5): its line,
    the uses whose parameter has no entry, and the uses whose entry is unverified [(text, function, column)]."""
    for n, t in block_lines(blk, "ns-address-uses"):
        body = re.sub(r"^stored-name address uses \(.*?\): \d+: ", "", t)
        noent, unver = [], []
        for u in body.split("; "):
            m = ADDR_NOENTRY.search(u)
            if m:
                noent.append((u.strip()[:200], m.group(1)))
                continue
            m = ADDR_UNVER.search(u)
            if m:
                unver.append((u.strip()[:200], m.group(1), m.group(3)))
        return n, noent, unver
    return None, [], []


def group_has(group, partition):
    """Whether a writers/buffer/lifetime group text names the row's partition."""
    if group == "all partitions":
        return True
    if group.startswith("partition {"):
        return group == "partition {%s}" % partition
    if group.startswith("shared by partitions "):
        return "{%s}" % partition in re.findall(r"\{[^}]*\}", group)
    return False


# ------------------------------------------------------------------------------------------------ the data branch
class Branch:
    def __init__(self, b, cand, named=None):
        self.id, self.data, self.readings = b["id"], b["data"], set(b["readings"]) - {"R0"}
        self.admitted = b.get("admitted")
        self.cand = cand
        self.named = named
        self.kinds = set(self.admitted["kinds"]) if self.admitted else set()

    def reads(self, r):
        return r in self.readings

    def entry_state(self, layer, function, column, named_by=None):
        """(admitted_or_not_unverified, depends_on lines, why) for one library entry the line names."""
        if self.cand is None:
            return False, [], "no candidate list given: the state of the library entries is not read"
        hits = [d for d in self.cand["lines"] if d["function"] == function and (layer is None or d["layer"] == layer)
                and (column is None or d["column"] == column) and (named_by is None or named_by in d["named_by"].split(","))]
        if not hits:   # spec 11 item 3 (b)-(d): a call with no record is 'no entry' in every data branch
            nc = (self.named or {}).get(function)
            if nc is not None and nc["records"] == 0:
                return False, [], "no entry: %s" % function
            if nc is not None:
                return True, [], "the entry %s is not in the candidate list; %d records in the facts (not unverified)" % (
                    function, nc["records"])
            return False, [], "the state of the entry of %s is not read" % function
        dep = [d["line"] for d in hits]
        if all(d["kind"] in self.kinds for d in hits):
            return True, dep, "admitted in %s (%s)" % (self.data, ", ".join(sorted({d["kind"] for d in hits})))
        return False, dep, "an unverified entry not admitted in %s: %s" % (self.data, function)

    def named_ground(self, function):
        nc = (self.named or {}).get(function)
        if nc is None:
            return None
        return {"file": list(nc["src"]), "text": "named call %s: records %d, unverified %d, layers %s"
                % (function, nc["records"], nc["unverified"], nc["layers"])}

    def listed_state(self, layer, function, column):
        """Spec 11 item 5: (admitted, depends_on lines, unadmitted 'f:c' names) of a row's listed entry in the candidate
        list; an entry the candidate list does not hold adds nothing (its depends_on is empty: 'none')."""
        if self.cand is None:
            return True, [], []
        hits = [d for d in self.cand["lines"] if d["function"] == function and d["layer"] == layer
                and (column is None or d["column"] == column)]
        bad = ["%s:%s" % (d["function"], d["column"]) for d in hits if d["kind"] not in self.kinds]
        return not bad, [d["line"] for d in hits], bad

    def tag(self):
        return {"id": self.id, "data": self.data, "readings": sorted(self.readings) or ["R0"],
                "tag": READING_TAG if self.readings else None}


# ------------------------------------------------------------------------------------------------ context of a record
NEED_REASON = {"present": None, "value-missing": "record_insufficient(value)", "text-partial": "record_insufficient(key text)",
               "not-reached": "target_not_reached", "unknown-loss": "record_insufficient(loss marker)",
               "unbound": "record_insufficient(site not bound in the build)", "not-planned": "plan_missing(need not planned)",
               "build-mismatch": "build_mismatch"}


def site_need(run, sites, bound, site_id, loss_any, actor=None):
    s = sites[site_id]
    st = {"need": "site %d %s %s" % (site_id, "+".join(s.rules), s.where())}
    if actor is not None:
        st["actor"] = actor
    if str(site_id) not in bound:
        st["state"] = "unbound"
    else:
        mask = bound[str(site_id)]
        missing_vals = [s.values[i].get("role") for i in s.values if not (mask >> i) & 1]
        evs = run.events_at({site_id}, actor) if run else []
        if run is None:
            st["state"], st["note"] = "not-reached", "no run"
        elif not evs:
            st["state"] = "unknown-loss" if loss_any else "not-reached"
        else:
            st["events"] = len(evs)
            vm = [s.values[i].get("role") for e in evs for i in s.values if (i >= len(e.v) or e.v[i] is None)]
            tp = [e for e in evs if any(v.get("string") == "validated" for v in s.values.values()) and
                  (e.s is None or "str-trunc" in e.flags)]
            if missing_vals:
                st["state"], st["note"] = "unbound", "values not bound: " + ", ".join(sorted(set(missing_vals)))
            elif vm:
                st["state"], st["note"] = "value-missing", "values missing: " + ", ".join(sorted(set(vm)))
            elif tp:
                st["state"] = "text-partial"
            else:
                st["state"] = "present"
    st["reason"] = NEED_REASON[st["state"]]
    return st


def key_of_instance(run, inst, keytext_sites):
    out = {"pointer": None, "text": None, "truncated": None, "text_event": None, "pointer_equal": None, "kind": None,
           "kind_event": None}
    if inst.at is not None:
        out["pointer"] = run.val(inst.at, "key")
    for e in run.by_actor.get(inst.actor, []):
        if e.site in keytext_sites and run.owner(e, inst.callee) is inst:
            if out["text"] is None and run.sites[e.site].role_index("key_text") is not None:
                out["text"], out["truncated"], out["text_event"] = e.s, "str-trunc" in e.flags, e
                kt = run.val(e, "key_text")
                out["pointer_equal"] = (kt == out["pointer"]) if (kt is not None and out["pointer"] is not None) else None
    return out


def kind_of_instance(run, inst, kind_sites):
    for e in run.by_actor.get(inst.actor, []):
        if e.site in kind_sites and run.owner(e, inst.callee) is inst:
            k = run.val(e, "kind")
            if k is not None:
                return k, e
    return None, None


def complete_key(k):
    return k["text"] is not None and not k["truncated"]


# ------------------------------------------------------------------------------------------------ name-space facts of a row
def ns_facts(N, st, sc):
    """The name-space entry, layer, lifetime and block lines a path A row reads (spec 10.3)."""
    rid_key = st["row_key"]
    f = {"entry": None, "layer": None, "lifetimes": [], "lifetime_complete": None, "block": None}
    ents = [e for e in sc["namespace_entries"] if e.get("row") == rid_key]
    f["entry"] = ents[0] if len(ents) == 1 else None
    f["entry_count"] = len(ents)
    if f["entry"]:
        kind = f["entry"].get("kind")
        f["lifetimes"] = [x for x in sc["lifetimes"] if x["kind"] == kind]
        f["lifetime_complete"] = sc["lifetime_complete"].get(kind)
        lay = [x for x in sc["namespace_layers"] if x.get("data") == f["entry"].get("data")]
        f["layer"] = lay[0] if lay else None
    f["block"] = N["relations"]["namespace_blocks"].get(st["namespace"]) if st["namespace"] else None
    return f


def block_lines(block, kind):
    return [(b[0], b[1]) for b in (block or {}).get("body", []) if b[2] == kind]


# ------------------------------------------------------------------------------------------------ the twelve items
def it(status, reason=None, grounds=None, depends_on=None, premises=None):
    return {"status": status, "reason": reason if status != "held" else None, "grounds": grounds or [],
            "depends_on": depends_on or [], "premises": premises or []}


def lifetime_events(ctx, kind_sites_map):
    return ctx["lifetime_ends"]


def item_basis1(ctx, u, c):
    ch = ctx.get("chain")
    if ctx["mismatch"]:
        return it("missing", "build_mismatch", ["the combination of the inputs is refused: " + ctx["compat"]])
    objs = {"main"} | {"module:%d" % u["_ev"].module} | ({"module:%d" % c["_ev"].module} if c.get("_ev") else set())
    sbo = {x["object"]: x for x in (ch or {}).get("same_build_objects", [])}
    gr, bad, refused = [], [], False
    for o in sorted(objs):
        x = sbo.get(o)
        if x is None:
            bad.append("%s: no same_build_object line" % o)
        elif x["status"] == "confirmed":
            gr.append({"file": x["src"], "text": "same_build_object confirmed %s" % o})
        else:
            refused = refused or x["status"] == "refused"
            bad.append("%s: %s" % (o, x["status"]))
    if not sbo and ch and ch.get("same_build"):
        bad.append("the chain check gives one same_build line only: %s" % ch["same_build"]["status"])
    if refused:
        return it("missing", "build_mismatch", gr + bad)
    if bad:
        return it("missing", R_ITEM["basis1_same_build"], gr + bad)
    return it("held", grounds=gr)


def item_basis2(u_view, c_view):
    bad = []
    for who, v in (("consumer", u_view), ("publisher", c_view)):
        if v["creation_tie"]["status"] != "determined":
            bad.append("link_unlinked(%s creation tie %s: %s)" % (who, v["creation_tie"]["status"], v["creation_tie"]["why"]))
        if v["root_tie"]["status"] != "determined":
            bad.append("link_unlinked(%s root tie %s: %s)" % (who, v["root_tie"]["status"], v["root_tie"]["why"]))
    if bad:
        return it("missing", bad[0], bad)
    return it("held", grounds=["creation and root ties of both actors determined"])


def null_test(when):
    return re.fullmatch(r"\(not param:\S+#\d+\)|\(eq param:\S+#\d+ 0\)|\(eq 0 param:\S+#\d+\)", when.strip()) is not None


def item_basis3(st, u):
    an, at = st["absence"]
    if not at:
        return it("missing", R_ITEM["basis3_absence_constant"], ["no absence line in the row"]), None
    m = re.match(r"absence: (-?\d+) ", at)
    if not m:
        return it("missing", R_ITEM["basis3_absence_constant"], ["the absence value is not evaluated: " + st["ref"](an)]), None
    val = int(m.group(1))
    if not same_status(val, u.get("_status")):
        return it("missing", R_ITEM["basis3_absence_constant"], ["the use's status %s is not the absence value %d (%s)"
                                                                 % (u.get("status_value"), val, st["ref"](an))]), val
    on, ot = st["absence_origins"]
    if not ot:
        return it("missing", R_ITEM["basis3_absence_constant"], ["no absence origins line (relations rules revision 14)"]), val
    k = u.get("_k") or {}
    gr = [st["ref"](an), st["ref"](on)]
    for part in re.split(r"origin\(s\): ", ot, 1)[-1].split("; "):
        mm = re.match(r"(\S+) at (\S+) when (.*) \[(search no-match|other)\]$", part.strip())
        if not mm:
            return it("missing", R_ITEM["basis3_absence_constant"], gr + ["an origin not read: " + part[:120]]), val
        if mm.group(4) == "search no-match":
            continue
        if null_test(mm.group(3)) and k.get("pointer") not in (None, 0) and complete_key(k):
            gr.append("origin %s at %s excluded: the use's key pointer %s is not null and its key text is recorded"
                      % (mm.group(1), mm.group(2), hx(k["pointer"])))
            continue
        return it("missing", R_ITEM["basis3_absence_constant"], gr + ["origin %s at %s when %s is not excluded by the record"
                                                                      % (mm.group(1), mm.group(2), mm.group(3)[:120])]), val
    return it("held", grounds=gr), val


def item_basis4(st, c):
    sp = st["success_path"].get(c["callee"])
    if sp is None:
        return it("missing", R_ITEM["basis4_creator_success"], ["no callee-check (success path) line of %s in the row"
                                                                 % c["callee"]])
    gr = [st["ref"](sp["n"])]
    if sp["state"] != "held":
        why = sp["why"] or ""
        r = R_STORED if why == "the stored value is not tied to the success path" else R_ITEM["basis4_creator_success"]
        return it("missing", r, gr + ["success path missing(%s)" % why])
    if not same_status(sp["value"], c.get("_status")):
        return it("missing", R_OUTNOTREAD, gr + ["the create's status %s is not the success value %d" % (c.get("status_value"), sp["value"])])
    if c.get("_out") is None:
        return it("missing", R_OUTNOTREAD, gr + ["the create's out id is not in the record"])
    return it("held", grounds=gr + ["status %s = success value %d; out %s" % (c["status_value"], sp["value"], hx(c["_out"]))])


def item_basis5(u, c):
    if complete_key(u["_k"]) and complete_key(c["_k"]):
        return it("held", grounds=["key texts of both events recorded without a cut flag"])
    return it("missing", R_ITEM["basis5_key_texts"], ["use text %r cut %s; create text %r cut %s" % (
        u["_k"]["text"], u["_k"]["truncated"], c["_k"]["text"], c["_k"]["truncated"])])


def item_basis6(run, u, c):
    ui, ci = u["_i"], c["_i"]
    if ui.at is None or ui.after is None or ci.at is None or ci.after is None:
        return it("missing", R_ITEM["basis6_create_after_use"], ["an at-call or after-call of the pair is not recorded"]), "unknown"
    inside = all(run.t_init <= x.t <= run.t_stop for x in (ui.at, ui.after, ci.at, ci.after)) if run.t_stop else True
    if not inside:
        return it("missing", R_ITEM["basis6_create_after_use"], ["an event of the pair lies outside the window"]), "outside"
    if ci.at.t > ui.after.t:
        return it("held", grounds=["create at-call t %s ms after the use's after-call t %s ms" % (run.ms(ci.at.t), run.ms(ui.after.t))]), "after"
    if ci.after.t < ui.at.t:
        return it("missing", R_ITEM["basis6_create_after_use"], ["the create ended before the use"]), "before"
    return it("missing", R_ITEM["basis6_create_after_use"], ["the create overlaps the use"]), "overlap"


def item_ga(N, nsf, c_callee):
    e = nsf["entry"]
    if e is None:
        return it("missing", R_ITEM["ga_namespace_entry"], ["%d namespace-entry lines of the row in the sidecar" % nsf["entry_count"]])
    gr = [{"file": e["src"], "text": "namespace-entry kind=%s value=%s" % (e.get("kind"), e.get("value"))}]
    bad = []
    for fld in ("lookup", "compare"):
        if not str(e.get(fld, "")).endswith(":applies"):
            bad.append("%s=%s" % (fld, e.get(fld)))
    cr = dict(x.rsplit(":", 1) for x in str(e.get("create", "")).split(",") if ":" in x)
    if c_callee is not None and cr.get(c_callee) != "applies":
        bad.append("create=%s does not name %s as applying" % (e.get("create"), c_callee))
    pinned = [x for x in N["plan"]["sidecar"]["namespace_data"] if x["sha256"] == e.get("sha256")]
    given = [x for x in N["namespaces"] if x["sha256"] == e.get("sha256") and x["ok"]]
    if not pinned:
        bad.append("the entry's data sha256 is on no input namespace-data line")
    if not given:
        bad.append("the entry's data file is not given (--namespaces) or was refused")
    if bad:
        return it("missing", R_ITEM["ga_namespace_entry"], gr + bad)
    return it("held", grounds=gr + ["data file %s sha256 %s = sidecar input namespace-data" % (os.path.basename(given[0]["path"]), e["sha256"][:16])])


def item_na_semantics(nsf, br):
    e = nsf["entry"]
    if e is None:
        return it("missing", R_ITEM["na_semantics"], ["no namespace-entry line"])
    calls = {}
    for fld in ("compare-call", "copy-call", "length-call"):
        v = e.get(fld, "-")
        if v in ("-", None, ""):
            return it("missing", R_ITEM["na_semantics"], ["%s=- in the namespace-entry line" % fld])
        for x in v.split(","):
            calls.setdefault(x.split("@")[0], set()).add(fld)
    dep, bad, gr = [], [], [{"file": e["src"], "text": "compare-call=%s copy-call=%s length-call=%s" % (
        e.get("compare-call"), e.get("copy-call"), e.get("length-call"))}]
    noent = []
    for fn, flds in sorted(calls.items()):
        for fld in sorted(flds):
            ok, d, why = br.entry_state(None, fn, None, fld)
            dep.extend(d)
            (gr if ok else bad).append(why)
            if why.startswith("no entry: ") and fn not in noent:   # spec 11 item 3 (b): not held in any data branch
                noent.append(fn)
                ng = br.named_ground(fn)
                if ng:
                    bad.append(ng)
    if noent:
        return it("missing", NB % ("no entry: " + ", ".join(noent)), gr + bad, sorted(set(dep)))
    if bad:
        return it("missing", R_ITEM["na_semantics"], gr + bad, sorted(set(dep)))
    return it("held", grounds=gr, depends_on=sorted(set(dep)))


def item_na_bound(nsf, u, c):
    e = nsf["entry"]
    if e is None or not str(e.get("extent-value", "-")).isdigit():
        return it("missing", R_ITEM["na_bound"], ["no extent value in the namespace-entry line"])
    ext = int(e["extent-value"])
    bad = []
    for who, k in (("use", u["_k"]), ("create", c["_k"])):
        if k["text"] is None:
            bad.append("%s key text not recorded" % who)
        elif k["truncated"]:
            return it("missing", R_ITEM["basis5_key_texts"], ["%s key text cut" % who])
        elif len(k["text"].encode("utf-8", "replace")) >= ext:
            bad.append("%s key text length %d >= extent %d" % (who, len(k["text"]), ext))
    if bad:
        return it("missing", R_ITEM["na_bound"], bad)
    return it("held", grounds=[{"file": e["src"], "text": "extent-value=%d" % ext}, "both key texts shorter than the extent"])


def item_da(st, nsf, br):
    e = nsf["entry"]
    blk = nsf["block"]
    if e is None or blk is None:
        return it("missing", R_ITEM["da_no_name_mapping"], ["no namespace-entry line or no namespace block of the row"])
    gr, bad = [], []
    part = st["partition"]
    # (a) the name pointer field
    kw = [t for n, t in st["key_writers"] if t.startswith("writers: %s:" % st["namespace"])]
    mm = re.search(r"premises: FIELD_STATUS per-field (\S+), type-level (\S+)", kw[0]) if kw else None
    if not mm:
        bad.append("(a) no writers line of the key field %s with its FIELD_STATUS" % st["namespace"])
    elif not (mm.group(1).startswith("writer-set-complete") and mm.group(2).startswith("writer-set-complete")):
        bad.append("(a) FIELD_STATUS of %s per-field %s type-level %s" % (st["namespace"], mm.group(1), mm.group(2)))
    else:
        gr.append("(a) FIELD_STATUS of %s complete under both rules" % st["namespace"])
    creates = {x.rsplit(":", 1)[0] for x in str(e.get("create", "")).split(",") if x}
    wl = block_lines(blk, "ns-writers")
    if not wl:
        bad.append("(a) no writers [group] lines in the namespace block (relations rules revision 14)")
    for n, t in wl:
        mg = re.match(r"writers \[([^\]]*)\] of (\S+): \d+: (.*)$", t)
        if not mg or mg.group(2) != st["namespace"]:
            continue
        mine = group_has(mg.group(1), part)
        tied = mg.group(1).startswith("partition ") or mg.group(1).startswith("shared by partitions ")
        for w in mg.group(3).split("; "):
            mw = re.match(r"(\S+) at (\S+) in (\S+) value=(\S+)", w)
            if not mw:
                continue
            const = re.fullmatch(r"-?\d+", mw.group(4)) is not None
            fn = mw.group(3).split("@")[0]
            if (mine or not tied) and not const and fn not in creates:
                bad.append("(a) writer %s at %s in %s value=%s [%s] is neither a create's name store nor a clear" % (
                    mw.group(1), mw.group(2), fn, mw.group(4), mg.group(1)[:60]))
    # (b) the stored-name buffers
    names = [x for x in str(e.get("stored-name", "-")).split(",") if x and x != "-"]
    if not names:
        bad.append("(b) no stored-name field in the namespace-entry line")
    for nm in names:
        bl = [(n, t) for n, t in block_lines(blk, "ns-buffer") if re.match(r"stored-name buffer \[([^\]]*)\]: %s " % re.escape(nm), t)
              and group_has(re.match(r"stored-name buffer \[([^\]]*)\]", t).group(1), part)]
        if not bl:
            bad.append("(b) no stored-name buffer line of %s in a group of partition {%s}" % (nm, part))
        for n, t in bl:
            if t.endswith("; complete"):
                gr.append("(b) %s:%d complete" % (st["rel"], n))
            elif br.reads("Rf") and t.endswith("not complete (FIELD_STATUS disabled; every cause discharged)"):
                gr.append("(b) %s:%d every cause discharged (reading Rf)" % (st["rel"], n))
            else:
                mt = re.search(r"; (not complete \(.*\))$", t)
                bad.append("(b) %s:%d %s" % (st["rel"], n, mt.group(1) if mt else "not complete"))
    # (b) the library entries of the address uses (judgement rules 8.13 item 11 (b), 8.14 item 5; spec 11 items 3 and 5):
    # a use whose parameter has no entry makes (da) missing in every data branch and under Rf; a use whose entry is
    # unverified rests on that candidate line (depends_on), held only where it is admitted
    an, noent, unver = address_uses(blk)
    dep, unadm = [], []
    for txt, fn, col in unver:
        ok, d, b = br.listed_state("P0", fn, col)
        if not d:
            ok, d, b = br.listed_state("P0", fn, None)
        dep.extend(d)
        unadm.extend(b)
    dep = sorted(set(dep))
    if noent:
        calls = []
        for _, fn in noent:
            if fn not in calls:
                calls.append(fn)
        return it("missing", NB % ("no entry: " + ", ".join(calls)), gr + bad + [
            "(b) %s:%d: %d address use(s) with no entry: %s" % (st["rel"], an, len(noent), " | ".join(t for t, _ in noent)[:1500])], dep)
    if bad:
        return it("missing", R_ITEM["da_no_name_mapping"], gr + bad, dep)
    if unadm:
        return it("missing", NB % ("rests on an unverified entry: " + ", ".join(sorted(set(unadm)))), gr + [
            "(b) %s:%d: an address use rests on an unverified entry not admitted in %s" % (st["rel"], an, br.data)], dep)
    return it("held", grounds=gr, depends_on=dep)


def item_ra(st, nsf, u, c, sc):
    e = nsf["entry"]
    if e is None:
        return it("missing", R_ITEM["ra_same_kind"], ["no namespace-entry line"])
    need = [x for x in sc["needs"].get(st["row_key"], []) if x["what"] == "kind-value"]
    if not need or need[0]["state"] != "planned":
        return it("missing", R_ITEM["ra_same_kind"], ["kind-value need %s" % (need[0]["state"] if need else "absent")])
    if str(e.get("value")) != str(st["partition"]):
        return it("missing", R_ITEM["ra_same_kind"], ["the row's partition {%s} is not the entry's value %s" % (st["partition"], e.get("value"))])
    if u.get("_kind") is None or c.get("_kind") is None:
        return it("missing", R_ITEM["ra_same_kind"], ["kind value recorded: use %s, create %s" % (hx(u.get("_kind")), hx(c.get("_kind")))])
    if u["_kind"] != c["_kind"] or str(u["_kind"]) != str(e.get("value")):
        return it("missing", R_ITEM["ra_same_kind"], ["kind values differ: use %s, create %s, entry %s" % (hx(u["_kind"]), hx(c["_kind"]), e.get("value"))])
    return it("held", grounds=["partition {%s} = entry value; kind values recorded equal (%s)" % (st["partition"], hx(u["_kind"]))])


def lifetime_sites(nsf):
    s = set()
    for x in nsf["lifetimes"]:
        s.update(int(v) for v in x["sites"] if v.isdigit())
    return s


def lifetime_complete(ctx, nsf):
    """(complete, why): the kind's lifetime-end sites all planned and bound, their needs present or not reached, no loss."""
    lc = nsf["lifetime_complete"]
    if lc is None or lc["value"] != "yes":
        return False, "lifetime-complete %s" % (lc["value"] if lc else "absent")
    ls = lifetime_sites(nsf)
    unb = sorted(s for s in ls if str(s) not in ctx["bound"])
    if unb:
        return False, "lifetime-end sites not bound: %s" % ",".join(map(str, unb))
    if ctx["loss_any"]:
        return False, "loss markers in the record"
    return True, "lifetime-complete yes; %d lifetime-end sites bound" % len(ls)


def lifetime_reason(why):
    """The reason of an instance link left unlinked by incomplete lifetime ends, by its cause (spec 11.5 item 1; the
    need states of NEED_REASON): a loss marker, a lifetime-end site not bound, else the plan (lifetime-complete not yes,
    no namespace-entry line)."""
    if why.startswith("loss markers"):
        return NEED_REASON["unknown-loss"]
    if why.startswith("lifetime-end sites not bound"):
        return NEED_REASON["unbound"]
    return NEED_REASON["not-planned"]


def ends_naming(ends, name, gen_id, t0, t1, own=None):
    """Lifetime ends in [t0, t1] that name the name text, the id, the whole table or every name of the kind."""
    hit, maybe = [], []
    for x in ends:
        if not (t0 <= x["t"] <= t1):
            continue
        if own is not None and x.get("create_of") is own:
            continue
        if x["names"] == "table" or x["names"] == "kind":
            hit.append(x)
        elif x["names"] == "id" and gen_id is not None and x["id"] == gen_id and x["successful"] is not False:
            (hit if x["successful"] else maybe).append(x)
        elif x["names"] == "name" and x["name"] == name:
            hit.append(x)
        elif x["names"] == "slot":
            maybe.append(x)
    return hit, maybe


def item_ma(ctx, nsf, u, c):
    ok, why = lifetime_complete(ctx, nsf)
    if not ok:
        return it("missing", R_ITEM["ma_no_lifetime_end"], [why])
    ui, ci = u["_i"], c["_i"]
    if None in (ui.at, ui.after, ci.at, ci.after):
        return it("missing", R_ITEM["ma_no_lifetime_end"], ["the interval is not recorded"])
    t0, t1 = min(ui.at.t, ci.at.t), max(ui.after.t, ci.after.t)
    hit, maybe = ends_naming(ctx["lifetime_ends"], u["_k"]["text"], None, t0, t1, own=ci)
    if hit:
        return it("missing", R_MA_SEEN, ["%s at actor %s aseq %d" % (x["form"], x["actor"], x["aseq"]) for x in hit])
    if maybe:
        return it("missing", R_ITEM["ma_no_lifetime_end"], ["%s at actor %s aseq %d not shown to be no lifetime end of the name"
                                                            % (x["form"], x["actor"], x["aseq"]) for x in maybe])
    return it("held", grounds=[why, "no lifetime end of the name in the interval"])


def apply_branch(items, br):
    """Judgement rules 8.13 item 2: in data branch D1 nothing is held on an unverified entry or a premise."""
    for k, v in items.items():
        if v["status"] == "held" and v["premises"]:
            v["status"], v["reason"] = "missing", R_ITEM.get(k, v.get("reason"))
            v["grounds"].append("rests on a premise: " + "; ".join(v["premises"]))
    return items


# ------------------------------------------------------------------------------------------------ use condition
def holder_branch(st, br):
    """(c3)(i) from the row's holder lines (8.13 item 15; spec 10.3 items 8 and 9)."""
    if not st["holders"]:
        return {"status": "not_established", "why": ("the row has no holder line" if st.get("rev14") else
                                                      "no holder line (relations rules revision 14 lines not in this relations text)"),
                "sources": []}
    res = []
    for n, t in st["holders"]:
        src = [{"kind": "static-row", "ref": st["ref"](n)}]
        if t.startswith("holder: local "):
            res.append({"status": "not_established", "why": "the writer set of a local holder is not shown complete", "sources": src})
            continue
        if not t.startswith("holder: field "):
            res.append({"status": "not_established", "why": "the holder is no record field", "sources": src})
            continue
        mfs = re.search(r"FIELD_STATUS per-field=(\S+) type-level=([^:;\s]+)", t)
        pf, tl = (mfs.group(1), mfs.group(2)) if mfs else (None, None)
        complete = pf == "writer-set-complete" and tl == "writer-set-complete"
        if br.reads("Re") and pf == "writer-set-complete" and tl == "DISABLED":
            complete = True
        mw = re.search(r"; writers \d+: (.*?); FIELD_STATUS ", t)
        writers = mw.group(1).split("; ") if mw else []
        mr = re.search(r"; readers (\d+): (.*)$", t)
        readers = mr.group(2) if mr else ""
        all_test = mr is not None and int(mr.group(1)) > 0 and "uses the value without such a test" not in readers and "not decided" not in readers
        others = []
        for w in writers:
            if w.startswith("own out store "):
                continue
            if w.startswith("clearing ") and br.reads("Rb") and all_test:
                continue
            others.append(w[:200])
        if others:
            res.append({"status": "not_held", "why": "other definitions: " + " | ".join(others) +
                        ("; readers: " + readers[:300] if readers else ""), "sources": src})
        elif not complete:
            res.append({"status": "not_established", "why": "the writer set is not complete (FIELD_STATUS per-field %s type-level %s)" % (pf, tl),
                        "sources": src})
        elif not mr or int(mr.group(1)) == 0:
            res.append({"status": "not_established", "why": "no later reader", "sources": src})
        else:
            res.append({"status": "held", "why": "the own out store is the only writer of a complete writer set; readers %s" % mr.group(1),
                        "sources": src, "premises": ["reading Re: per-field completeness taken as fact"] if tl == "DISABLED" else []})
    order = {"not_held": 0, "not_established": 1, "held": 2}
    worst = min(res, key=lambda x: order[x["status"]])
    out = {"status": worst["status"], "why": worst["why"], "sources": [s for x in res for s in x["sources"]], "holders": res}
    prem = [p for x in res for p in x.get("premises", [])]
    if prem:
        out["premises"], out["reading"] = prem, "Re"
    return out


def alternatives_of(st):
    out = {}
    for n, t in st["alternatives"]:
        m = re.match(r"alternatives \[context (\d+|-)(?::[^\]]*)?\]: (.*)$", t)
        cid = m.group(1)
        body = m.group(2)
        rest = re.search(r"; rests on: (.*?); premises: (.*?); decided calls: (.*)$", body)
        verdict = ("none_found" if body.startswith("none found (exploration complete)") else
                   "found" if body.startswith("found: ") else "not_established")
        out[cid] = {"verdict": verdict, "text": body[:600], "full": body, "n": n, "rests_on": rest.group(1) if rest else None,
                    "premises": rest.group(2) if rest else None, "decided": rest.group(3) if rest else None}
    return out


def branch_iii(st, alts, cid, br):
    """(c3)(iii) of one context (8.13 items 15-17)."""
    if cid is None:
        return {"status": "not_established", "why": "context unknown", "context": None, "sources": [], "depends_on": []}
    a = alts.get(cid)
    if a is None:
        return {"status": "not_established", "why": "no alternatives line of context %s" % cid, "context": cid, "sources": [],
                "depends_on": []}
    src = [{"kind": "static-row", "ref": st["ref"](a["n"])}]
    out = {"context": cid, "sources": src, "depends_on": []}
    if a["decided"] and a["decided"] != "none":
        out["decided_by"] = "stored-field rule"
        out["decided_calls"] = a["decided"]
    if a["verdict"] == "found":
        out.update(status="not_held", why=a["text"][:300])
        return out
    if a["verdict"] == "not_established":
        why = a["text"][:300]
        if br.reads("Re") and "rests on a premise" in a["full"] and (a["premises"] or "none") != "none":
            why = "the alternatives line gives no call result under the premise (reading Re): " + (a["premises"] or "")[:200]
        out.update(status="not_established", why=why)
        return out
    bad = []
    for layer, fn, col in layers_unverified(a["rests_on"] or ""):
        ok, d, w = br.entry_state(layer, fn, col)
        out["depends_on"].extend(d)
        if not ok:
            bad.append(w)
    if (a["premises"] or "none") != "none":
        bad.append("premises: " + a["premises"][:200])
    if bad:
        out.update(status="not_established", why="rests on an unverified entry or a premise: " + "; ".join(bad)[:400])
        return out
    out.update(status="held", why="none found (exploration complete); rests on: %s" % (a["rests_on"] or "none"))
    return out


def use_context(run, ctx_lines, u_inst, alt_ids=()):
    """Judgement rules 8.13 item 16: the context frame that brackets the use, or unknown.  A row without context lines
    (8.14 item 6 (a); spec 11 item 1): the one context id its alternatives lines name, whatever the event's frame and
    whether its at-call is recorded; none or several -> unknown (None).  No id is made up."""
    if not ctx_lines:
        ids = sorted(set(alt_ids))
        return ids[0] if len(ids) == 1 else None
    if u_inst is None or u_inst.at is None:
        return None
    hits = []
    for c in ctx_lines:
        if not c["frame"]:
            continue
        at_s, af_s = int(c["frame"][0]), int(c["frame"][1])
        for i in run.insts.get(u_inst.actor, []):
            if i.site_at is not None and i.site_at.n == at_s and i.site_after is not None and i.site_after.n == af_s \
                    and i.at.aseq < u_inst.at.aseq and (u_inst.after or u_inst.at).aseq < i.after.aseq:
                hits.append(str(c["id"]))
                break
    return hits[0] if len(hits) == 1 else None


# ------------------------------------------------------------------------------------------------ order from the pair file
def pair_ground(N, st, c_site, u_root, c_root):
    P = N["pairs"]
    if P is None:
        return "order_basis_unknown(no pair file given)", None
    if P.get("refused"):
        return "order_basis_unknown(the pair file is refused: %s)" % P["refused"][:80], None
    if u_root is None or c_root is None:
        return "order_basis_unknown(a root of the pair is not tied)", None
    lines = [x for x in P["lines"].get(st["row_key"], []) if tail_match(x["other_pos"], c_site.tu, c_site.line, c_site.col)]

    def rmatch(text, r):
        if r[1] == "process":
            return text.endswith("(process)")
        return text == "%s (%s)" % r
    lines = [x for x in lines if rmatch(x["use_root"], u_root) and rmatch(x["other_root"], c_root)]
    if not lines:
        return "order_basis_unknown(no line of the pair in the pair file)", None
    if len({x["value"] for x in lines}) > 1:
        return "order_basis_unknown(more than one line of the pair)", None
    x = lines[0]
    return {"value": x["value"], "edges": x["edges"], "premises": x["premises"], "row_line": x["row_line"],
            "other_kind": x["other_kind"], "src": x["src"]}, x


# ------------------------------------------------------------------------------------------------ one record
def creator_kind(st, site):
    for n, t in st["creators"]:
        m = re.search(POS, t)
        if m and tail_match(m.group(1), site.tu, site.line, site.col):
            return "creator", n
    for n, t in st["not_a_pair"]:
        m = re.search(POS, t)
        if m and tail_match(m.group(1), site.tu, site.line, site.col):
            mm = re.match(r"not a pair: (key constants differ|probe-create|same actor only)", t)
            return ("creator-not-a-pair(%s)" % mm.group(1)) if mm else "not-a-pair-other", n
    return None, None


def explain_row(ctx, rowkey, consumer, br):
    N, sc, sites, run, st = ctx["N"], ctx["sc"], ctx["sites"], ctx["run"], ctx["static"][rowkey]
    srow = sc["rows"][rowkey]
    smap = {int(x) for x in sc["maps"].get(rowkey, [])}
    by_rule = {}
    for n in smap:
        for r in sites[n].rules:
            by_rule.setdefault(r, set()).add(n)
    use_sites = by_rule.get("use-call", set())
    kt_sites = by_rule.get("key-text", set())
    kind_sites = by_rule.get("kind-value", set())
    pub_sites = by_rule.get("creator-call", set()) | by_rule.get("writer", set())
    hc_sites = by_rule.get("handling-chain", set()) - use_sites
    rec = {"record": "explanation", "format": "JR 3.1-3.2 fields, 3.3 verdict, 3.5 reasons; revision 4 items of JR 8.13",
           "script": {"judge": JUDGE_REV, "judge_sha256": ctx["judge_sha"], "reader": N["reader"]["script"],
                      "reader_sha256": N["reader"]["sha256"]}}
    if N.get("label_record"):
        rec["record_label"] = N["label_record"]
    mismatch = ctx["mismatch"]
    rec["build_id"] = {"build": N["build_label"], "bprime_basis_sha256": ctx["sha"]("bprime_basis"),
                       "executables_run_meta": run.d and ctx["runN"]["exes"] if run else {}, "plan_facts_sha256": sc["facts_sha256"],
                       "compat": ctx["compat"]}
    if run:
        rec["build_id"].update(ctx["runN"]["exes_inst"])
    rec["extractor_rev"] = ctx["extractor"]
    rec["semantics_rev"] = ctx["semantics"]
    rec["plan_id"] = {"sidecar_sha256": ctx["sha"]("sidecar"), "manifest_sha256": ctx["sha"]("manifest"),
                      "run_record_manifest_sha256": run.d["run_manifest"] if run else None}
    keytext = N["keys"].get(srow["id"], [None, None])[1]
    rec["row_id"] = {"use_site_key": rowkey, "key_text": keytext, "row_counter": srow["id"],
                     "row_counter_note": "the row number is an output counter of the relations text, not an identity",
                     "relations": "%s:%d" % (N["rel_name"], st["head_line"])}
    rec["path"] = st["path"]
    rec["handling_label"] = srow.get("label")
    rec["handling_unknown_reason"] = srow.get("why") if srow.get("label") in ("T?", "J?") else None
    rec["priority"] = {k: srow.get(k) for k in ("tier", "position", "label", "fact-label", "class", "order", "reach", "disposition",
                                                  "why", "control", "control-state")}
    rec["priority"]["rule"] = sc["head"].get("priority")
    b = sc["budget"] or {}
    rec["budget"] = {"plan": {k: b.get(k) for k in ("value", "reason", "disclosure", "planned", "truncation", "unresolved", "sensitivity")},
                     "bounds": sc["bounds"], "run": ctx["run_budget"]}
    rec["review"] = None
    rec["eval_result"] = None
    rec["premises"] = list(ctx["premises"])
    rec["eval_branch"] = br.tag()
    rec["data_branch"] = {"data": br.data, "admitted_sha256": (br.admitted and N["branches_by_id"][br.id]["admitted_sha256"]) or "none",
                          "candidates_sha256": ctx["sha"]("candidates") or "none", "admitted_kinds": sorted(br.kinds),
                          "named_calls_sha256": ctx["sha"]("named_calls") or "none"}
    loss_any = ctx["loss_any"]
    apps = ctx["apps"]
    if run is not None and not mismatch and not isinstance(consumer, tuple):
        cview = actor_view(run, consumer, apps)
    else:
        cview = {"run_actor": None, "static_root": consumer[1] if isinstance(consumer, tuple) else None,
                 "static_kind": consumer[2] if isinstance(consumer, tuple) else None,
                 "creation_tie": {"status": "unlinked", "why": "no run actor" if run is not None else "no run"},
                 "root_tie": {"status": "unlinked", "why": "no run actor"}, "app": {"status": "unknown", "why": "no run actor"}}
    rec["consumer_actor"] = cview
    ca = cview["run_actor"]
    nsf = ns_facts(N, st, sc) if st["path"] == "A" else None
    # ---- events: uses
    uses = []
    if ca is not None:
        for i in run.insts.get(ca, []):
            if (i.site_at is not None and i.site_at.n in use_sites) or (i.site_after is not None and i.site_after.n in use_sites):
                uses.append(i)
    succ = st["success"]
    absence_val = None
    if st["absence"][1]:
        ma = re.match(r"absence: (-?\d+) ", st["absence"][1])
        absence_val = int(ma.group(1)) if ma else None
    ctx_lines = [c for c in sc["contexts"] if c["row"] == rowkey]
    alts = alternatives_of(st)
    use_events = []
    for i in uses:
        u = {"call": "%s -> %s line %s" % (i.func, i.callee, i.line)}
        if i.at is not None:
            u["at_call"] = evref(run, i.at)
        stv = None
        if i.after is not None:
            u["after_call"] = evref(run, i.after)
            stv = run.val(i.after, "status")
            u["status_value"] = hx(stv)
            names = [nm for v, nm, _ in st["consts"] if same_status(v, stv)]
            u["status_name"] = names[0] if names else None
            u["status_name_source"] = ("%s (constants of the row)" % st["ref"](st["consts"][0][2])) if st["consts"] else None
            if succ is None:
                u["out"] = {"read": False, "why": "no success constant: " + st["success_note"]}
            elif same_status(succ[0], stv):
                u["out"] = {"read": True, "value": hx(run.val(i.after, "out0")), "under": "%s %d" % (succ[1], succ[0])}
            else:
                u["out"] = {"read": False, "why": "status %s is not the success constant %s %d" % (hx(stv), succ[1], succ[0])}
        u["_status"] = stv
        if stv is None:
            u["outcome"] = "unclassified"
        elif u.get("out", {}).get("read"):
            u["outcome"] = "found"
        elif absence_val is not None and same_status(absence_val, stv):
            u["outcome"] = "absent"
        elif fail_class(stv):
            u["outcome"] = "other_failure"
        else:
            u["outcome"] = "unclassified"
        if st["path"] == "A":
            k = key_of_instance(run, i, kt_sites)
            u["key"] = {"pointer": hx(k["pointer"]), "text": k["text"], "truncated": k["truncated"],
                        "length": len(k["text"]) if k["text"] is not None else None,
                        "text_event": evref(run, k["text_event"]) if k["text_event"] else None,
                        "pointer_equal_to_key_text_argument": k["pointer_equal"]}
            u["_k"] = k
            kv_, ke = kind_of_instance(run, i, kind_sites)
            u["kind_value"] = hx(kv_)
            u["_kind"] = kv_
        else:
            fv = run.val(i.at, "field_value") if i.at is not None else None
            u["field_value"] = hx(fv)
            u["_fv"] = fv
        u["context"] = use_context(run, ctx_lines, i, alts.keys())
        u["_i"] = i
        u["_ev"] = i.at or i.after
        use_events.append(u)
    # ---- events: creates (the row's creator sites, every actor; the consumer's own only for a probe-create line)
    creates = []
    if run is not None and not mismatch:
        for a, insts in run.insts.items():
            for i in insts:
                s = i.site_at if (i.site_at is not None and i.site_at.n in pub_sites) else (
                    i.site_after if (i.site_after is not None and i.site_after.n in pub_sites) else None)
                if s is None:
                    continue
                if "creator-call" not in s.rules:
                    if a != ca:   # a writer call of path B or C (revision 3's publisher events)
                        creates.append({"actor": a, "call": "%s -> %s line %s" % (i.func, i.callee, i.line), "callee": i.callee,
                                        "other_kind": "writer", "_i": i, "_site": s, "_ev": i.at or i.after,
                                        "_k": {"text": None, "truncated": None, "pointer": None}, "_status": None,
                                        "at_call": evref(run, i.at) if i.at is not None else None,
                                        "after_call": evref(run, i.after) if i.after is not None else None})
                    continue
                okind, oline = creator_kind(st, s)
                if a == ca and okind != "creator-not-a-pair(probe-create)":
                    continue
                c = {"actor": a, "call": "%s -> %s line %s" % (i.func, i.callee, i.line), "callee": i.callee,
                     "other_kind": okind or "creator-call site", "row_line": st["ref"](oline) if oline else None}
                if i.at is not None:
                    c["at_call"] = evref(run, i.at)
                pst = run.val(i.after, "status") if i.after is not None else None
                c["_status"] = pst
                c["status_value"] = hx(pst)
                if i.after is not None:
                    c["after_call"] = evref(run, i.after)
                sp = st["success_path"].get(i.callee)
                c["_out"] = run.val(i.after, "out0") if (i.after is not None and sp and sp["state"] == "held"
                                                         and same_status(sp["value"], pst)) else None
                c["out"] = ({"read": True, "value": hx(c["_out"]), "under": "success value %d (%s)" % (sp["value"], st["ref"](sp["n"]))}
                            if c["_out"] is not None else
                            {"read": False, "raw": hx(run.val(i.after, "out0")) if i.after is not None else None,
                             "why": "not read: the success path line of %s is %s" % (i.callee, ("missing(%s)" % sp["why"]) if sp else "absent")
                             if not (sp and sp["state"] == "held") else "status is not the success value"})
                if st["path"] == "A":
                    k = key_of_instance(run, i, kt_sites)
                    c["key"] = {"pointer": hx(k["pointer"]), "text": k["text"], "truncated": k["truncated"],
                                "text_event": evref(run, k["text_event"]) if k["text_event"] else None}
                    c["_k"] = k
                    kv_, _ = kind_of_instance(run, i, kind_sites)
                    c["kind_value"] = hx(kv_)
                    c["_kind"] = kv_
                c["_i"] = i
                c["_site"] = s
                c["_ev"] = i.at or i.after
                creates.append(c)
            for e in run.by_actor[a]:
                s = sites.get(e.site)
                if s is not None and e.site in pub_sites and s.place == "store" and a != ca:
                    creates.append({"actor": a, "call": "store in %s line %s" % (s.func, s.line), "callee": None,
                                    "other_kind": "writer", "store": evref(run, e), "_i": Inst(a, e, e, s, s, s.func, None, s.line),
                                    "_site": s, "_ev": e, "_k": {"text": None, "truncated": None, "pointer": None}, "_status": None})
    pub_actors = sorted({c["actor"] for c in creates}, key=akey)
    pviews = {a: (cview if a == ca else actor_view(run, a, apps)) for a in pub_actors}
    rec["publisher_actors"] = [pviews[a] for a in pub_actors if a != ca]
    rec["actor_pair_kind"] = {a: pair_kind(cview, pviews[a]) for a in pub_actors} if ca is not None else {}
    rec["pair_kind"] = dict(rec["actor_pair_kind"])
    # ---- lifetime ends of the row's kind
    ends = []
    if run is not None and nsf is not None and nsf["entry"] is not None and not mismatch:
        succ_v = None
        for sp in st["success_path"].values():
            succ_v = sp["value"]
            break
        lsites = lifetime_sites(nsf)
        reent = {x["function"]: x["reentry"] for x in nsf["lifetimes"] if x["form"] == "rollback"}
        for e in run.events:
            if e.site not in lsites:
                continue
            s = sites[e.site]
            x = {"actor": e.actor, "aseq": e.aseq, "t": e.t, "t_ms": run.ms(e.t), "site": e.site, "rules": s.rules, "src": e.src}
            if "object-delete" in s.rules and s.place == "entry":
                x.update(form="delete", names="id", id=run.val(e, "param0") if s.role_index("param0") is not None else
                         (signed(e.v[0], s.values.get(0, {}).get("type")) if e.v else None))
                fin = None
                for f in run.by_actor[e.actor]:
                    sf = sites.get(f.site)
                    if f.aseq > e.aseq and sf is not None and sf.func == s.entry_of and sf.place == "after-call" and "object-delete" in sf.rules:
                        fin = f
                        break
                    if f.aseq > e.aseq and f.site == e.site:
                        break
                x["successful"] = None if (fin is None or succ_v is None) else same_status(succ_v, run.val(fin, "status"))
                ends.append(x)
            elif "object-rollback" in s.rules and s.place == "at-call":
                stv = run.val(e, "status")
                if succ_v is None or stv is None or same_status(succ_v, stv):
                    continue
                own = run.owner(e, s.func)
                k = key_of_instance(run, own, kt_sites) if own is not None else {"text": None}
                if reent.get(s.func) == "possible" or k["text"] is None:
                    x.update(form="rollback", names="kind", successful=True)
                else:
                    x.update(form="rollback", names="name", name=k["text"], successful=True)
                x["create_of"] = own
                ends.append(x)
            elif "object-clear" in s.rules and s.place == "at-call":
                x.update(form="clear-table", names="table", successful=True)
                ends.append(x)
            elif "object-clear" in s.rules and s.place == "store":
                own = None
                for c in creates:
                    if c["actor"] == e.actor and c["_i"].bracketed(e):
                        own = c["_i"]
                x.update(form="slot-clear", names="slot", successful=None, create_of=own)
                ends.append(x)
    ctx["lifetime_ends"] = ends
    # ---- name states, generations and instance links
    gens = []
    for c in creates:
        if c.get("_out") is not None and c["_i"].after is not None:
            t0 = c["_i"].after.t
            later = [x for x in ends if x["t"] > t0 and x.get("create_of") is not c["_i"] and (
                x["names"] in ("table", "kind") or (x["names"] == "name" and x["name"] == c["_k"].get("text")) or
                (x["names"] == "id" and x["id"] == c["_out"] and x["successful"]))]
            gens.append({"create": c, "kind": c.get("_kind"), "name": c["_k"].get("text"), "id": c["_out"], "start": t0,
                         "end": min((x["t"] for x in later), default=None)})
    lc_ok, lc_why = (lifetime_complete(ctx, nsf) if nsf is not None and nsf["entry"] is not None else (False, "no namespace-entry line"))
    creators_complete = not any(x.startswith(("writer_set_incomplete", "key_unresolved(creator", "actor_not_extracted"))
                                for x in st["incomplete"])
    for u in use_events:
        if st["path"] != "A":
            u["name_state"] = "undecided(path %s: no name space)" % st["path"]
            continue
        ui = u["_i"]
        if ui.at is None or not complete_key(u["_k"]):
            u["name_state"] = "undecided(the use's key text is not recorded whole)"
            continue
        if u.get("_kind") is None:
            u["name_state"] = "undecided(the use's kind value is not recorded)"
            continue
        same = [g for g in gens if g["name"] == u["_k"]["text"] and g["kind"] == u["_kind"]]
        nokind = [g for g in gens if g["name"] == u["_k"]["text"] and g["kind"] is None]
        open_ = [g for g in same if g["start"] < ui.at.t and (g["end"] is None or g["end"] > (ui.after or ui.at).t)]
        ovl = [g for g in same if not (g["start"] < ui.at.t) and g["create"]["_i"].at is not None and g["create"]["_i"].at.t < (ui.after or ui.at).t]
        if nokind:
            u["name_state"] = "undecided(a create of the name without its kind value)"
        elif not lc_ok:
            u["name_state"] = "undecided(lifetime ends not complete: %s)" % lc_why
        elif ovl:
            u["name_state"] = "undecided(a create overlaps the use)"
        elif open_:
            maybe = [x for x in ends if x["names"] == "slot" and open_[0]["start"] < x["t"] < ui.at.t and x.get("create_of") is not open_[0]["create"]["_i"]]
            u["name_state"] = "met" if not maybe else "undecided(a slot clear not shown to be no lifetime end)"
            u["_gen"] = open_[0] if not maybe else None
        elif u["outcome"] == "found":   # spec 11 item 4 (8.14 item 6 (b)): a found use is never unmet
            u["name_state"] = ("undecided(the row's creator sites are not complete)" if not creators_complete else
                               "undecided(no create of the found id in the record)")
        elif not creators_complete:
            u["name_state"] = "undecided(the row's creator sites are not complete)"
        else:
            u["name_state"] = "unmet"
        if u.get("_gen") is not None:
            same_text = [g for g in gens if g["name"] == u["_k"]["text"] and g is not u["_gen"]]
            if same_text:
                u["same_text_unbound"] = [{"actor": g["create"]["actor"]} for g in same_text]
    for u in use_events:
        ui = u["_i"]
        # instance link (8.13 item 14): a use that found the object, only on an open generation
        if u["outcome"] == "found":
            outv = run.val(ui.after, "out0")
            g = [x for x in gens if x["id"] == outv]
            if not lc_ok:
                u["instance_link"] = {"status": "unlinked", "reason": lifetime_reason(lc_why), "why": lc_why}
            elif not g:
                u["instance_link"] = {"status": "unlinked", "reason": "link_unlinked(no create of the id in the record)"}
            elif g[0]["start"] < ui.at.t and (g[0]["end"] is None or g[0]["end"] > ui.at.t):
                u["instance_link"] = {"status": "linked", "create_actor": g[0]["create"]["actor"],
                                      "create_aseq": g[0]["create"]["_i"].at.aseq if g[0]["create"]["_i"].at else None, "id": hx(outv)}
                u["_linked"] = g[0]["create"]
            else:
                u["instance_link"] = {"status": "unlinked", "reason": "link_unlinked(the generation of the id is not open at the use)"}
        else:
            u["instance_link"] = {"status": "not_applicable", "why": "the use found no object"}
    # ---- name-binding pairs
    pairs = []
    sc_mode = (st["path"] == "A")
    for ui_, u in enumerate(use_events):
        if not sc_mode or u["outcome"] != "absent":
            continue
        for c in creates:
            if c["other_kind"] == "writer":
                continue
            items = {}
            items["basis1_same_build"] = item_basis1(ctx, u, c)
            items["basis2_ties"] = item_basis2(cview, pviews[c["actor"]])
            items["basis3_absence_constant"], _ = item_basis3(st, u)
            items["basis4_creator_success"] = item_basis4(st, c)
            items["basis5_key_texts"] = item_basis5(u, c)
            items["basis6_create_after_use"], when = item_basis6(run, u, c)
            items["ga_namespace_entry"] = item_ga(N, nsf, c["callee"])
            items["na_semantics"] = item_na_semantics(nsf, br)
            items["na_bound"] = item_na_bound(nsf, u, c)
            items["da_no_name_mapping"] = item_da(st, nsf, br)
            items["ra_same_kind"] = item_ra(st, nsf, u, c, sc)
            items["ma_no_lifetime_end"] = item_ma(ctx, nsf, u, c)
            apply_branch(items, br)
            five = all(items[k]["status"] == "held" for k in ("ga_namespace_entry", "na_semantics", "na_bound",
                                                              "da_no_name_mapping", "ra_same_kind", "ma_no_lifetime_end"))
            texts_eq = complete_key(u["_k"]) and complete_key(c["_k"]) and u["_k"]["text"] == c["_k"]["text"]
            if when == "before" and texts_eq and five:
                items["basis6_create_after_use"] = it("contradicted", R_CONTRA, ["the create of the same name ended before the use"])
            missing = [k for k in ITEMS if items[k]["status"] != "held"]
            pr = {"use": ui_, "create_actor": c["actor"], "create_aseq": c["_i"].at.aseq if c["_i"].at else None,
                  "create_site": c["_site"].n, "create_pos": "%s:%s:%s" % (c["_site"].tu, c["_site"].line, c["_site"].col),
                  "other_kind": c["other_kind"], "pair_kind": pair_kind(cview, pviews[c["actor"]]), "items": items,
                  "missing_items": missing, "missing_count": len(missing)}
            refut_base = all(items[k]["status"] == "held" for k in ("basis1_same_build", "basis2_ties",
                                                                    "basis4_creator_success", "basis5_key_texts")) and five
            if not missing and texts_eq:
                pr["result"] = "bound"
            elif refut_base and complete_key(u["_k"]) and complete_key(c["_k"]) and not texts_eq:
                pr["result"] = "refuted"
            elif complete_key(u["_k"]) and complete_key(c["_k"]) and not texts_eq:
                pr["result"] = "differs_from_use_key"
            else:
                pr["result"] = "unlinked"
            pr["_c"] = c
            pairs.append(pr)
        nb = [p for p in pairs if p["use"] == ui_ and p["result"] == "bound"]
        if len(nb) > 1:
            for p in nb:
                p["result"] = "unlinked"
                p["result_reason"] = R_MANY
    # ---- per-use link results
    for ui_, u in enumerate(use_events):
        mine = [p for p in pairs if p["use"] == ui_]
        if u["outcome"] == "found":
            u["link_result"] = u["instance_link"]["status"]
            u["link_reasons"] = [u["instance_link"].get("reason")] if u["instance_link"].get("reason") else []
        elif u["outcome"] == "absent" and sc_mode:
            b1 = [p for p in mine if p["result"] == "bound"]
            u["link_result"] = "bound" if len(b1) == 1 else ("refuted" if mine and all(p["result"] == "refuted" for p in mine) else "unlinked")
            rs = []
            for p in mine:
                rs.extend(p["items"][k]["reason"] for k in p["missing_items"])
                if p.get("result_reason"):
                    rs.append(p["result_reason"])
            if not mine:
                rs.append("link_unlinked(no create of the row in the record)")
            u["link_reasons"] = sorted(set(r for r in rs if r))
        else:
            u["link_result"] = "unlinked"
            u["link_reasons"] = ["link_unlinked(the use's outcome is %s)" % u["outcome"]]
    # ---- needs (incompleteness)
    needs = []
    if mismatch:
        for n in sorted(smap):
            needs.append({"need": "site %d" % n, "state": "build-mismatch", "reason": NEED_REASON["build-mismatch"]})
    else:
        for n in sorted(smap):
            scope = ca if (n in use_sites or n in hc_sites) and ca is not None else None
            needs.append(site_need(run, sites, ctx["bound"], n, loss_any, scope))
    for nd in sc["needs"].get(rowkey, []):
        needs.append({"need": "%s (sidecar)" % nd["what"], "state": nd["state"], "reason": nd["reason"], "note": nd["note"]})
    rec["incompleteness"] = {"needs": needs, "static_gaps": [{k: v for k, v in g.items() if k != "src"} for g in sc["gaps"].get(rowkey, [])],
                             "static_incomplete": list(st["incomplete"]),
                             "static_incomplete_source": st["ref"](st["incomplete_line"]) if st["incomplete_line"] else None,
                             "budget_truncation": b.get("truncation"), "open_premises": [p["id"] for p in ctx["premises"]],
                             "unparsed_row_lines": ["%s:%d %s" % (N["rel_name"], n, k) for n, k in st["unparsed_lines"]]}
    # ---- axis 1 link (8.12 (ㄹ), 8.13 items 6-14)
    link = {"resource_kind": {"A": "named object", "B": "shared memory field", "C": "registration record"}[st["path"]],
            "actor_token": cview.get("creation_tie"), "app_of_actor": cview.get("app"),
            "static_basis": {"row": "%s:%d" % (N["rel_name"], st["head_line"]), "use_site_key": rowkey,
                             "relations_sha256": ctx["sha"]("relations"), "keys_sha256": ctx["sha"]("keys")},
            "instance_evidence": [], "missing": [], "status": None}
    lm = link["missing"]
    if mismatch:
        lm.append("build_mismatch")
    if ca is None:
        lm.append("link_unlinked(no run actor of the consumer)" if run is not None else "no_run")
    else:
        if cview["creation_tie"]["status"] != "determined":
            lm.append("link_unlinked(consumer creation tie %s: %s)" % (cview["creation_tie"]["status"], cview["creation_tie"]["why"]))
        if cview["root_tie"]["status"] != "determined":
            lm.append("link_unlinked(consumer root tie %s: %s)" % (cview["root_tie"]["status"], cview["root_tie"]["why"]))
    status, basis, grounds = "unlinked", None, []
    if st["path"] == "A":
        link["resource_key"] = {"namespace": st["namespace"], "partition": st["partition"],
                                "namespace_source": "%s:%d" % (N["rel_name"], st["head_line"]), "name": None, "generation": None}
        if ca is not None and not use_events:
            lm.append("target_not_reached")
        if ca is not None and use_events:
            texts = sorted({u["_k"]["text"] for u in use_events if complete_key(u["_k"])})
            if any(not complete_key(u["_k"]) for u in use_events):
                lm.append("record_insufficient(key text)")
            if len(texts) > 1:
                lm.append("multi_state_unlinked(the consumer's uses name %d keys)" % len(texts))
            elif texts:
                link["resource_key"]["name"] = texts[0]
            if any(u["outcome"] == "absent" for u in use_events):
                lm.append("link_unlinked(absence outcome: the use reached no object instance)")
            res = [u["link_result"] for u in use_events]
            if all(r == "bound" for r in res):
                status, basis = "established", "name_binding"
                grounds.append("every use is an absence outcome with exactly one bound create (JR 8.2, 8.12 (ㄹ))")
                p0 = [p for p in pairs if p["result"] == "bound"][0]
                link["resource_key"]["generation"] = p0["_c"]["out"].get("value")
            elif all(r == "linked" for r in res):
                status, basis = "established", "instance"
                grounds.append("every use found an object whose create is linked on an open generation (JR 8.13 item 14)")
            elif set(res) <= {"bound", "linked"}:
                lm.append(R_MIXED)
            elif all(r == "refuted" for r in res) and creates:
                inc = [x for x in st["incomplete"] if x.startswith("writer_set_incomplete") or "indirect" in x]
                cs_needs = [n for n in needs if any("site %d " % s in n["need"] for s in pub_sites)]
                ok2 = all(n["state"] in ("present", "not-reached") for n in cs_needs) and not loss_any
                if not inc and ok2:
                    status = "refuted"
                    link["refuted_scope"] = "inside the observation window of this run (RUN t_init to t_stop)"
                    grounds.append("every create of every publisher actor at the row's creator sites is a refuted pair (JR 8.3)")
                else:
                    lm.append(R_SOME)
            for u in use_events:
                for r in u["link_reasons"]:
                    if r not in lm:
                        lm.append(r)
        if ca is not None and not creates:
            lm.append("link_unlinked(no publisher event of the row in another actor)")
    else:
        link["resource_key"] = {"field": st["field"], "use_site": st["site_line"][1],
                                "source": st["ref"](st["site_line"][0]) if st["site_line"][0] else None}
        if ca is not None and not use_events:
            lm.append("target_not_reached")
        if not any(sites[n].role_index("address") is not None and sites[n].place == "at-call" for n in use_sites):
            lm.append("record_insufficient(the address read at the use is not in the plan)")
        if not creates:
            lm.append("link_unlinked(no writer event of the row in another actor)")
    if status == "established" and any(x.startswith(("link_unlinked(consumer", "build_mismatch", "multi_state")) for x in lm):
        status, basis = "unlinked", None
    link["status"] = status
    if basis:
        link["basis"] = basis
    link["grounds"] = grounds
    link["name_binding"] = {"pairs": [{k: v for k, v in p.items() if not k.startswith("_")} for p in pairs],
                            "absence_value": absence_val}
    rec["link"] = link
    # ---- axis 2 use_cond (static lines only)
    uc = {"rule": {"A": "JR 4.2 name or key lookup (path A)", "B": "JR 4.2 placeholder field reaching a callee check (path B)",
                   "C": "JR 4.2 registration record read by the created task (path C)"}[st["path"]],
          "premises": ["static row text of %s (relations rules: %s)" % (N["rel_name"], ctx["rel_head"].get("rules"))], "missing": []}
    um = uc["missing"]
    fcs = [(v, nm, n) for v, nm, n in st["consts"] if fail_class(v)]
    if fcs:
        uc["callee_check"] = {"status": "present", "text": ", ".join("%d [%s]" % (v, nm) for v, nm, _ in st["consts"]),
                              "sources": [{"kind": "static-row", "ref": st["ref"](n)} for n in sorted({n for _, _, n in st["consts"]})]}
    else:
        uc["callee_check"] = {"status": "missing", "sources": []}
        um.append("absent_result_unevaluated(the row gives no failure-class return constant of the use)")
    fn, ft = st["flip"]
    an, at = st["absence"]
    if br.reads("Rc") and at and re.match(r"absence: -?\d+ .*: reached", at):
        uc["reach"] = {"status": "present", "text": at[:300], "reading": "Rc", "sources": [{"kind": "static-row", "ref": st["ref"](an)}]}
    elif ft and ft.startswith("flip: static-possible") and "with the earlier value" in ft:
        uc["reach"] = {"status": "present", "text": ft, "sources": [{"kind": "static-row", "ref": st["ref"](fn)}]}
    else:
        uc["reach"] = {"status": "missing", "text": ft, "sources": [{"kind": "static-row", "ref": st["ref"](fn)}] if fn else []}
        um.append("absent_result_unevaluated(%s)" % (ft[len("flip: "):].split(":")[0] if ft else "no flip line"))
        if ft and "not resolved statically" in ft:
            um.append("key_unresolved(the key of the use or of a creator is not resolved statically: %s)" % st["ref"](fn))
    wl = st["creators"] if st["path"] == "A" else st["writers"]
    fz = st["feasibility"][1] or ""
    winc = [x for x in st["incomplete"] if x.startswith("writer_set_incomplete")]
    kinc = [x for x in st["incomplete"] if x.startswith("key_unresolved(creator")] + \
           (["key_unresolved(creator key unresolved: %s)" % st["ref"](n) for n, t in wl if "key unresolved" in t][:1])
    decided = fz.startswith("feasibility: possible") or fz.startswith("feasibility: proven-impossible")
    wsrc = [{"kind": "static-row", "ref": st["ref"](n)} for n, _ in wl]
    have_pairs = N["pairs"] is not None and not N["pairs"].get("refused") and bool(N["pairs"]["lines"].get(rowkey))
    if wl and not winc and not kinc and decided:
        uc["writers"] = {"status": "present", "count": len(wl), "sources": wsrc + [{"kind": "static-row", "ref": st["ref"](st["feasibility"][0])}]}
    elif br.reads("Rd") and wl and have_pairs:
        uc["writers"] = {"status": "present", "count": len(wl), "reading": "Rd", "sources": wsrc,
                         "premises": winc + kinc + ["the partition's creator set with its incomplete reasons (reading Rd)"]}
    else:
        uc["writers"] = {"status": "missing", "count": len(wl), "sources": wsrc}
        if not wl:
            um.append("writer_set_incomplete(no writer in the row)")
        um.extend(winc)
        um.extend(kinc)
        if not winc and not kinc and wl:
            um.append("writer_set_incomplete(the completeness of the row's writer set is a premise only)")
        if not decided:
            um.append("fixed_proof_open(%s)" % (fz[len("feasibility: "):].split(":")[0] if fz else "no feasibility line"))
    # the list entries (c1), (c2) and (c4) rest on, per data branch (spec 11 item 5; judgement rules 8.13 item 2, 8.14 item 6 (c))
    for k, base in (("callee_check", "absent_result_unevaluated"), ("reach", "absent_result_unevaluated"),
                    ("writers", "writer_set_incomplete")):
        e = uc[k]
        dep, unadm = element_depends(st, br, source_lines(e.get("sources")))
        e["depends_on"] = dep
        if unadm and e["status"] == "present":
            e["status"] = "missing"
            e["why"] = "rests on an unverified entry: " + ", ".join(unadm)
            um.append("%s(rests on an unverified entry: %s)" % (base, ", ".join(unadm)))
    bi = holder_branch(st, br) if st["path"] == "A" else {"status": "not_established", "why": "path %s: no holder line" % st["path"], "sources": []}
    dep, unadm = element_depends(st, br, source_lines(bi.get("sources")))
    bi["depends_on"] = dep
    if unadm and bi["status"] == "held":
        bi["status"] = "not_established"
        bi["why"] = "rests on an unverified entry: %s; %s" % (", ".join(unadm), bi["why"])
    uc["branch_i"] = bi
    cids = sorted({u["context"] for u in use_events if u["context"] is not None}) if use_events else sorted(alts) or [None]
    unk = [u for u in use_events if u["context"] is None]
    biii = [branch_iii(st, alts, c, br) for c in cids]
    if unk:
        biii.append({"status": "not_established", "why": "context unknown", "context": None, "sources": [], "uses": len(unk),
                     "depends_on": []})
    uc["branch_iii"] = biii
    i_ok = bi["status"] == "held"
    iii_ok = bool(biii) and all(x["status"] == "held" for x in biii)
    if i_ok and iii_ok:
        uc["caller_dependence"] = {"status": "present", "branch": "(i) and (iii) held for every use",
                                   "sources": bi["sources"] + [s for x in biii for s in x["sources"]]}
    else:
        why = []
        if not i_ok:
            why.append("(i) %s" % bi["status"])
        if not iii_ok:
            why.append("(iii) %s" % ", ".join(sorted({x["status"] for x in biii})) if biii else "(iii) no context")
        sub = ("context unknown" if any(x.get("why") == "context unknown" for x in biii) else
               "rests on an unverified entry or a premise" if any("rests on an unverified" in (x.get("why") or "") for x in biii + [bi]) else
               "; ".join(why).replace(";", ","))
        uc["caller_dependence"] = {"status": "missing", "text": "; ".join(why), "sources": bi["sources"] + [s for x in biii for s in x["sources"]]}
        um.append("caller_dependence_unknown(%s)" % sub)
    if st["path"] == "A" and use_events and use_events[0]["_k"]["text"] is not None:
        uc["run_key_value"] = {"text": use_events[0]["_k"]["text"], "sources": [{"kind": "record-key",
                               "ref": "dump actor %s aseq %d" % (ca, use_events[0]["_k"]["text_event"].aseq)}]}
    c3 = (iii_ok if br.reads("Ra") else (i_ok and iii_ok))
    present = [uc["callee_check"]["status"] == "present", uc["reach"]["status"] == "present",
               uc["writers"]["status"] == "present", i_ok, iii_ok]
    if all(present[:3]) and c3:
        uc["status"] = "derived"
    else:
        uc["status"] = "partial" if any(present) else "not_derived"
    if uc["status"] == "derived" and br.readings:
        uc["reading_tag"] = READING_TAG
    uc["stored_field_rule"] = [x for x in biii if x.get("decided_by")]
    uc["predicate"] = (("an object with the use's key exists in the namespace partition at the use" if st["path"] == "A"
                        else "the field holds a value other than its absence value %s at the use" % st["init"])
                       if uc["status"] == "derived" else None)
    rec["use_cond"] = uc
    # ---- axis 3 order (pair file only)
    order = {"static": "unknown", "static_reason": None, "static_note": {"text": fz, "source": st["ref"](st["feasibility"][0]) if st["feasibility"][0] else None},
             "run_id": ({"run_id": run.d["run"].get("run_id"), "dump_text_sha256": ctx["sha"]("dump_text"), "clock": run.d["run"].get("clock")} if run else None),
             "use_events": [{k: v for k, v in u.items() if not k.startswith("_")} for u in use_events],
             "publish_events": [{k: v for k, v in c.items() if not k.startswith("_")} for c in creates],
             "sync_results": {"creation": [], "wait": "not-planned: plan_missing(need not planned)", "lock": "not-planned: plan_missing(need not planned)"},
             "missing": [], "pairs": []}
    om = order["missing"]
    for v in [cview] + [pviews[a] for a in pub_actors if a != ca]:
        cr = v.get("creation_tie", {}).get("creation_result")
        if cr:
            order["sync_results"]["creation"].append({"actor": v["run_actor"], "creation_result": cr})
    linked = [(p["use"], p["_c"]) for p in pairs if p["result"] == "bound"] + \
             [(i, u["_linked"]) for i, u in enumerate(use_events) if u.get("_linked") is not None]
    vals = []
    for ui_, c in linked:
        g, line = pair_ground(N, st, c["_site"], root_text(cview), root_text(pviews[c["actor"]]))
        order["pairs"].append({"use": ui_, "create_actor": c["actor"], "order_ground": g})
        vals.append(g["value"] if isinstance(g, dict) else "order_basis_unknown")
        if isinstance(g, dict) and g["value"] == "hard_before" and use_events[ui_].get("name_state") == "unmet":
            order.setdefault("hard_before_unmet", []).append(ui_)
    if vals and len(set(vals)) == 1 and vals[0] != "order_basis_unknown":
        order["static"] = vals[0]
    else:
        order["static_reason"] = ("no create linked to a use (JR 8.13 item 14: the order is computed for bound or "
                                  "instance-linked creates only)" if not vals else "the linked pairs give %s" % ", ".join(sorted(set(vals))))
    order["order_ground"] = [x["order_ground"] for x in order["pairs"]] or ["order_basis_unknown(no linked pair)"]
    if run is None:
        order["status"] = "static_only"
        om.append("no_run")
    elif mismatch:
        order["status"] = "record_insufficient"
        om.append("build_mismatch")
    elif not use_events:
        order["status"] = "record_insufficient"
        un = [n for n in needs if n.get("state") in ("unbound", "unknown-loss") and any("site %d " % s in n["need"] for s in use_sites)]
        om.append(un[0]["reason"] if un else "target_not_reached")
    else:
        vm = [n for n in needs if n.get("state") in ("value-missing", "unbound") and any("site %d " % s in n["need"] for s in use_sites)]
        if vm:
            order["status"] = "record_insufficient"
            om.append(vm[0]["reason"])
        elif uc["status"] == "derived":
            order["status"], why = evaluate_p(st, use_events, creates, link)
            if why:
                om.append(why)
        else:
            order["status"] = "events_only"
            om.append("absent_result_unevaluated(the predicate is not derived: use_cond %s)" % uc["status"])
    order["per_consumer"] = per_consumer(st, use_events, [c for _, c in linked])
    rec["order"] = order
    rec["order_ground"] = order["order_ground"]
    rec["other_order_seen_in_boots"] = None
    # ---- neutral counts of the record (spec 3 item 11)
    calls = {(sites[n].func, sites[n].callee, sites[n].line) for n in use_sites}
    planned_uses = max(1, len(ctx_lines)) * len(calls)
    reached = len({(u["_i"].func, u["_i"].callee, u["_i"].line, u["context"]) for u in use_events})
    rec["uses"], rec["uses_reached"] = planned_uses, min(reached, planned_uses)
    rec["same_build"] = {x["object"]: x["status"] for x in (ctx.get("chain") or {}).get("same_build_objects", [])} or None
    rec["events"] = {"uses": len(use_events), "creates": len(creates), "lifetime_ends": [
        {k: v for k, v in x.items() if k not in ("create_of",)} for x in ends], "buildids": ctx["buildids"],
        "tok_counts": ctx["tok_counts"]}
    rec["intervention"] = {"plan_origin": "generated", "plan_inputs": [x["text"] for x in sc["inputs"]], "validity": "no hold point in the plan",
                           "hold": "none", "matched_use": None, "side_effects": None, "other_causes": None,
                           "repeats": ctx["run_budget"].get("boots") if run else 0, "control_level": "none (natural boot)",
                           "status": "unconfirmed", "missing": ["intervention_out_of_scope(no hold, gate or directed run in this stage)"]}
    rec["effect"] = effect_axis(ctx, st, cview, use_events, hc_sites)
    rec["tool_verdict"] = verdict(rec)
    rec["uses_decided"] = rec["uses_reached"] if rec["tool_verdict"]["violation"]["value"] in ("observed", "not_observed_in_runs", "refuted_link") else 0
    ctx["use_rows"].extend(use_row(N, srow, rowkey, consumer, i, u) for i, u in enumerate(use_events))
    return rec


def use_row(N, srow, rowkey, consumer, i, u):
    k = u.get("key", {})
    return [N["build_label"], srow["id"], rowkey, str(consumer if not isinstance(consumer, tuple) else "static-%s" % (consumer[1] or "none")),
            str(i), u["_i"].actor, str(u["_i"].at.aseq if u["_i"].at else "-"), str(u["_i"].after.aseq if u["_i"].after else "-"),
            str(u.get("status_value")), u["outcome"], str((u.get("out") or {}).get("value")), str(k.get("pointer")),
            json.dumps(k.get("text"), ensure_ascii=False), str(k.get("truncated")), str(k.get("length")), str(u.get("kind_value")),
            str(u["context"]), u.get("name_state", "-"), u.get("link_result", "-"), ";".join(u.get("link_reasons", []))]


def evaluate_p(st, use_events, creates, link):
    if st["path"] == "A":
        u = use_events[0]
        if "at_call" not in u:
            return "record_insufficient", "record_insufficient(value)"
        if link.get("status") != "established":
            return "record_insufficient", "record_insufficient(the state at the use is not decided by the values read)"
        if all(x.get("name_state") == "unmet" for x in use_events):
            return "unmet_state_observed", None
        if all(x.get("name_state") == "met" for x in use_events) and all(x["outcome"] == "found" for x in use_events):
            return "met_state_only", None
        return "record_insufficient", "record_insufficient(the state at the use is not decided by the values read)"
    vals = [u.get("_fv") for u in use_events]
    if any(v is None for v in vals) or st["init"] is None:
        return "record_insufficient", "record_insufficient(value)"
    if any(v == st["init"] for v in vals):
        return "unmet_state_observed", None
    return "met_state_only", None


def per_consumer(st, use_events, linked_creates):
    cells = {"item1_mhb_use_before_w": {"value": "incomplete", "why": "order edges of the pair file are not read as a must-happen-before here"}}
    if not use_events:
        for k in ("item2_uses_before_w_all_fail", "item3_after_w_no_such_status", "item4_a_failing_use_stands"):
            cells[k] = {"value": "no_data", "why": "no use of this consumer in the record", "events": []}
        return cells
    lk = [c for c in linked_creates if c.get("_i") is not None and c["_i"].at is not None]
    if not lk:
        for k in ("item2_uses_before_w_all_fail", "item3_after_w_no_such_status"):
            cells[k] = {"value": "unchecked", "why": "no create linked to a use (bound or instance-linked)", "events": []}
        cells["item4_a_failing_use_stands"] = {"value": "unchecked", "why": "no initialization-window end in this cycle", "events": []}
        return cells
    w = min(c["at_call"]["t_ms"] for c in lk)
    before = [u for u in use_events if "at_call" in u and u["at_call"]["t_ms"] < w]
    after = [u for u in use_events if "at_call" in u and u["at_call"]["t_ms"] > w]

    def ref(u):
        return {"actor": u["at_call"]["actor"], "aseq": u["at_call"]["aseq"]}
    if before:
        cells["item2_uses_before_w_all_fail"] = {"value": "holds" if all(fail_class(u.get("_status")) for u in before) else "fails",
                                                 "events": [ref(u) for u in before], "grade_rule": FAIL_CLASS_RULE}
    else:
        cells["item2_uses_before_w_all_fail"] = {"value": "no_data", "events": [], "why": "no use of this consumer before w"}
    if after:
        bad = {u.get("_status") for u in before}
        cells["item3_after_w_no_such_status"] = {"value": "fails" if any(u.get("_status") in bad for u in after) else "holds",
                                                 "events": [ref(u) for u in after]}
    else:
        cells["item3_after_w_no_such_status"] = {"value": "no_data", "events": [], "why": "no_matching_use_after_w"}
    cells["item4_a_failing_use_stands"] = {"value": "unchecked", "events": [], "why": "no initialization-window end in this cycle"}
    cells["w"] = {"t_ms": w, "events": [{"actor": c["at_call"]["actor"], "aseq": c["at_call"]["aseq"]} for c in lk]}
    return cells


def effect_axis(ctx, st, cview, use_events, hc_sites):
    run = ctx["run"]
    eff = {"missing": []}
    if run is None:
        eff.update({"window": None, "end_reason": None, "recovery": "not_observed", "events": [], "unobserved": ["no run"],
                    "status": "outside_window_unknown"})
        eff["missing"].append("no_run")
        return eff
    eff["window"] = {"start_ms": 0.0, "end_ms": run.ms(run.t_stop), "start": "RUN record t_init (runtime start)",
                     "end": "RUN record t_stop (%s)" % run.d["runx"].get("end", "end of the record"),
                     "stop_events": ctx["sc"]["stop"], "consumer_app_stop": ctx["runN"]["stop_eval"].get(cview.get("app", {}).get("app"))}
    eff["end_reason"] = {"process": ctx["runN"]["process_end"]}
    ca = cview.get("run_actor")
    evs, loops = [], []
    after_t = None
    if use_events:
        last = use_events[-1]
        after_t = (last.get("after_call") or last.get("at_call"))["aseq"]
    exit_fns = {r.split()[1] for r in ctx["sc"]["stop"] if r.startswith("app-exit ") and len(r.split()) > 1}
    if ca is not None and after_t is not None:
        for e in run.by_actor.get(ca, []):
            if e.aseq <= after_t:
                continue
            s = run.sites[e.site]
            if e.site in hc_sites or ("stop-event" in s.rules and s.entry_of in exit_fns):
                evs.append(evref(run, e, {"ref": s.ref[:200]}))
            elif "stop-event" in s.rules:
                loops.append(e)
    eff["window"]["consumer_run_loop_after_use"] = evref(run, loops[0]) if loops else None
    eff["window"]["consumer_run_loop_entries_after_use"] = len(loops)
    eff["events"] = evs
    eff["events_scope"] = ("events of the consumer actor after its last use of the row at the row's handling-chain sites and "
                           "the app-exit sites; their order in the actor is recorded, no causal link to the use is claimed")
    exit_ev = [x for x in evs if "stop-event" in x["rule"]]
    later_use = use_events[1:] if len(use_events) > 1 else []
    if later_use:
        eff["recovery"] = "completed" if [u for u in later_use if u.get("out", {}).get("read")] else "attempted_not_completed"
    elif exit_ev:
        eff["recovery"] = "not_attempted"
        eff["recovery_scope"] = "in the window, at the planned sites: no later use of the row by this actor; its app-exit entry is recorded"
    else:
        eff["recovery"] = "not_observed"
    eff["end_reason"]["actor"] = ("app exit (app-exit entry recorded at aseq %d)" % exit_ev[-1]["aseq"]) if exit_ev else "the process end (run budget)"
    eff["unobserved"] = ["after the process end (RUN t_stop)", "operations at no planned site",
                         "the effects of a result that the plan does not record (sidecar need lines not-planned)"]
    if any("SIGKILL" in x for x in ctx["runN"]["process_end"]):
        eff["unobserved"].append("after the SIGKILL of the process (no record)")
    if evs:
        eff["status"] = "observed"
    elif use_events:
        eff["status"] = "none_in_window"
        eff["missing"].append("record_insufficient(no event of the handling chain or of an app-exit site after the use)")
    else:
        eff["status"] = "outside_window_unknown"
        eff["missing"].append("target_not_reached")
    return eff


def verdict(rec):
    L, U, O, I, E = rec["link"]["status"], rec["use_cond"]["status"], rec["order"]["status"], rec["intervention"]["status"], rec["effect"]
    v = {"inputs": ["link.status", "link.basis", "use_cond.status", "order.status", "order.static", "intervention.status",
                    "order.use_events.status_value", "effect.status", "effect.events", "effect.recovery"]}
    if L == "established" and U == "derived" and O == "unmet_state_observed":
        val = "observed"
    elif U == "derived" and O == "static_only":
        val = "candidate"
    elif L == "established" and U == "derived" and O == "met_state_only":
        val = "not_observed_in_runs"
    elif L == "refuted":
        val = "refuted_link"
    else:
        val = "undetermined"
    kind = {"unordered": "unordered", "fixed_early_proved": "fixed_early"}.get(rec["order"]["static"], "order_basis_unknown")
    reasons = []
    if val == "undetermined":
        for ax in ("link", "use_cond", "order"):
            for r in rec[ax].get("missing", []):
                if r not in reasons:
                    reasons.append(r)
    v["violation"] = {"value": val, "kind": kind, "undetermined_reasons": reasons}
    if val == "refuted_link":
        v["violation"]["grounds"] = rec["link"]["grounds"]
    fails = []
    for u in rec["order"]["use_events"]:
        sv = u.get("status_value")
        if sv is not None:
            n = -int(sv[3:], 16) if sv.startswith("-") else int(sv, 16)
            if fail_class(n):
                fails.append({"actor": u["after_call"]["actor"], "aseq": u["after_call"]["aseq"], "status": sv, "name": u.get("status_name")})
    if I == "differs" and L == "established" and rec["link"].get("basis") == "instance":
        odf = {"value": "shown"}
    elif I == "same":
        odf = {"value": "not_shown"}
    elif fails:
        odf = {"value": "failure_sign", "grade_rule": FAIL_CLASS_RULE, "failing_uses": fails,
               "no_valid_intervention_pair": rec["intervention"]["missing"]}
    else:
        odf = {"value": "undetermined", "undetermined_reasons": list(rec["intervention"]["missing"]) +
               (["target_not_reached"] if not rec["order"]["use_events"] else [])}
    v["odf"] = odf
    fe, roles = [], {}
    for r in ((E.get("window") or {}).get("stop_events") or []):
        p = r.split()
        if len(p) > 1:
            roles[p[1]] = p[0]
    for x in E.get("events", []):
        if "stop-event" in x["rule"]:
            fn = x.get("entry_of")
            role = roles.get(fn, "stop")
            fe.append("app_exit_called(%s param0=%s)" % (fn, x["values"].get("param0")) if role == "app-exit" else
                      "run_loop_entered(%s)" % fn if role == "run-loop" else "stop_event(%s)" % fn)
    if E.get("recovery") == "completed":
        fe.append("recovered_in_window")
    elif E.get("recovery") in ("not_attempted", "attempted_not_completed"):
        fe.append("no_recovery_in_window")
    if E.get("status") == "outside_window_unknown":
        fe.append("outside_window_unknown")
    if not fe:
        fe.append("not_observed")
    w = E.get("window")
    v["final_effect"] = {"facts": fe, "window": {"start": w["start"], "end": w["end"]} if w else None, "end_reason": E.get("end_reason")}
    head = ("사용 조건 위반(관측)" if val == "observed" else "순서 의존 실패" if odf["value"] == "shown" else
            "실패 징후" if odf["value"] == "failure_sign" else "정적 후보" if val == "candidate" else
            "순서 의존 후보" if O == "static_only" else "미확정")
    if rec["link"].get("basis") == "name_binding":
        head += "(이름 결합)"
    v["headline"] = head
    return v


# ------------------------------------------------------------------------------------------------ format checks
NO_EVIDENCE = {"link": {"unlinked"}, "use_cond": {"partial", "not_derived"},
               "order": {"events_only", "record_insufficient", "static_only"}, "intervention": {"unconfirmed"},
               "effect": {"none_in_window", "outside_window_unknown"}}


def all_reasons(rec):
    out = []
    for ax in NO_EVIDENCE:
        out.extend(rec.get(ax, {}).get("missing", []))
    out.extend(rec.get("tool_verdict", {}).get("violation", {}).get("undetermined_reasons", []))
    out.extend(rec.get("tool_verdict", {}).get("odf", {}).get("undetermined_reasons", []))
    for n in rec.get("incompleteness", {}).get("needs", []):
        if n.get("reason"):
            out.append(n["reason"])
    for g in rec.get("incompleteness", {}).get("static_gaps", []):
        if g.get("reason"):
            out.append(g["reason"])
    out.extend(rec.get("incompleteness", {}).get("static_incomplete", []))
    for p in rec.get("link", {}).get("name_binding", {}).get("pairs", []):
        for k, v in p.get("items", {}).items():
            if v.get("reason"):
                out.append(v["reason"])
    for u in rec.get("order", {}).get("use_events", []):
        out.extend(u.get("link_reasons", []))
    return out


def ground_ok(g):
    if isinstance(g, dict):
        return bool(g.get("file")) or bool(g.get("text"))
    return isinstance(g, str) and bool(g) and not re.search(r"\b(hand reading|판의 읽기|사람의 읽기)\b", g)


def check_record(rec, vocab, admitted=None):
    bad = []
    tv = rec.get("tool_verdict", {})
    vio = tv.get("violation", {}).get("value")
    uc = rec.get("use_cond", {})
    lk = rec.get("link", {})
    eb = rec.get("eval_branch") or {}
    if vio == "observed":
        if uc.get("status") != "derived" or not all(uc.get(k, {}).get("sources") for k in ("callee_check", "reach", "caller_dependence")):
            bad.append("JR 3.6 item 1: violation observed without a derived use condition whose elements carry source lines")
        ue = rec.get("order", {}).get("use_events", [])
        if not any((u.get("key") or {}).get("text") is not None or u.get("field_value") is not None or (u.get("out") or {}).get("read") for u in ue):
            bad.append("JR 3.6 item 2: violation observed with no value read in order.use_events (return grade only)")
    for k, e in uc.items():
        if isinstance(e, dict):
            for s in e.get("sources", []):
                if s.get("kind") not in ("static-row", "semantics", "record-key"):
                    bad.append("JR 3.6 item 3: use_cond.%s cites %s" % (k, s.get("kind")))
    if tv.get("odf", {}).get("value") == "shown" and rec.get("intervention", {}).get("plan_origin") != "generated":
        bad.append("JR 3.6 item 4: odf shown without a generated plan")
    ins = tv.get("inputs", [])
    if any(x.startswith("handling_label") or x.startswith("priority") for x in ins):
        bad.append("JR 3.6 item 5: the handling label is an input of the verdict")
    if any(x.startswith("eval_result") or x.startswith("review") for x in ins):
        bad.append("JR 3.6 item 6: eval_result or review is an input of the verdict")
    for ax, vals in NO_EVIDENCE.items():
        a = rec.get(ax, {})
        if a.get("status") in vals and not a.get("missing"):
            bad.append("JR 3.6 item 7: %s.status %s with no missing reason" % (ax, a.get("status")))
    if vio == "undetermined" and not tv.get("violation", {}).get("undetermined_reasons"):
        bad.append("JR 3.6 item 7: violation undetermined with no reason")
    ca = (rec.get("consumer_actor") or {}).get("run_actor")
    for k, c in (rec.get("order", {}).get("per_consumer") or {}).items():
        if k.startswith("item"):
            for e in c.get("events", []):
                if e.get("actor") != ca:
                    bad.append("JR 3.6 item 8: order.per_consumer.%s holds an event of actor %s, not of the consumer %s" % (k, e.get("actor"), ca))
    pairs = lk.get("name_binding", {}).get("pairs", [])
    ues = rec.get("order", {}).get("use_events", [])
    if lk.get("status") == "established" and lk.get("basis") == "name_binding":   # 9 and 12
        if rec.get("path") != "A":
            bad.append("JR 8.5 item 9: name binding on a path %s record" % rec.get("path"))
        for i, u in enumerate(ues):
            if u.get("outcome") != "absent":
                bad.append("JR 8.12 item 12: a name-binding record with a use that is not an absence outcome")
            b = [p for p in pairs if p.get("use") == i and p.get("result") == "bound"]
            if len(b) != 1:
                bad.append("JR 8.12 item 12: use %d has %d bound pairs" % (i, len(b)))
            for p in b:
                if any(v.get("status") != "held" or not v.get("grounds") for v in p.get("items", {}).values()):
                    bad.append("JR 8.5 item 9: a bound pair with an item not held or without grounds")
    if lk.get("status") == "refuted":                                               # 10
        if not lk.get("refuted_scope"):
            bad.append("JR 8.5 item 10: refuted without refuted_scope")
        for p in pairs:
            if p.get("result") != "refuted":
                bad.append("JR 8.5 item 10: a refuted record with a pair %s" % p.get("result"))
    if lk.get("basis") and lk.get("status") != "established":                       # 11
        bad.append("JR 8.5 item 11: link.basis without an established link")
    for p in pairs:
        for k, v in p.get("items", {}).items():
            if v.get("status") == "held" and not all(ground_ok(g) for g in v.get("grounds", [])):
                bad.append("JR 8.13 item 21 (15): item %s held on a ground that names no file, line or event" % k)
    if uc.get("status") == "derived":                                               # 13
        bi, biii = uc.get("branch_i", {}), uc.get("branch_iii", [])
        if not biii or any(x.get("status") != "held" or not x.get("sources") for x in biii):
            bad.append("JR 8.13 item 21 (13): derived without (iii) held with a static source in every context")
        if "Ra" not in (eb.get("readings") or []) and (bi.get("status") != "held" or not bi.get("sources")):
            bad.append("JR 8.13 item 21 (13): derived in R0 without (i) held with a static source")
        for k in ("reach", "writers"):
            if any(s.get("kind") not in ("static-row", "semantics") for s in uc.get(k, {}).get("sources", [])):
                bad.append("JR 8.13 item 21 (13): (c2) or (c4) cites a run record")
    if ues:                                                                         # 14
        for i, u in enumerate(ues):
            for f in ("outcome", "name_state", "link_result"):
                if f not in u:
                    bad.append("JR 8.13 item 21 (14): use %d has no %s" % (i, f))
            if "context" not in u:
                bad.append("JR 8.13 item 21 (14): use %d has no context field" % i)
            if u.get("link_result") not in ("bound", "linked", "refuted") and not u.get("link_reasons"):
                bad.append("JR 8.13 item 21 (14): use %d unlinked with no reason" % i)
            if u.get("context") is not None and not any(x.get("context") == u.get("context") and x.get("sources")
                                                        for x in uc.get("branch_iii", [])):
                bad.append("JR 8.13 item 21 (14), spec 11 item 1: use %d has context %s with no alternatives line of it"
                           % (i, u.get("context")))
            if u.get("outcome") == "found" and u.get("name_state") == "unmet":
                bad.append("JR 8.13 item 21 (14), spec 11 item 4: use %d found its object and its name state is unmet" % i)
    if not (0 <= rec.get("uses_decided", 0) <= rec.get("uses_reached", 0) <= rec.get("uses", 0)):
        bad.append("JR 8.13 item 21 (14): uses_decided <= uses_reached <= uses does not hold")
    if eb.get("data") == "D1":                                                      # 16
        for p in pairs:
            for k, v in p.get("items", {}).items():
                if v.get("status") == "held" and (v.get("depends_on") or v.get("premises")):
                    bad.append("JR 8.13 item 21 (16): D1 record with item %s held on an unverified entry or a premise" % k)
        rd = set(eb.get("readings") or [])
        for x in [uc.get("branch_i", {})] + list(uc.get("branch_iii", [])):
            if x.get("status") == "held" and (x.get("depends_on") or (x.get("premises") and x.get("reading") not in rd)):
                bad.append("JR 8.13 item 21 (16): D1 record with a (c3) branch held on an unverified entry or a premise")
        for k in ("reach", "writers"):
            e = uc.get(k, {})
            if e.get("status") == "present" and e.get("premises") and e.get("reading") not in rd:
                bad.append("JR 8.13 item 21 (16): D1 record with use_cond.%s present on a premise" % k)
    # 16 extended (spec 11 item 6): (a) every element, (c3) branch and (da) item has its depends_on field; (b) a D1
    # record has nothing present or held on a list entry; (c) a D2 record's present or held things rest on admitted
    # lines only; (d) nothing whose sub-reason is 'no entry: ...' is present or held
    elems = [("use_cond.%s" % k, uc.get(k)) for k in ("callee_check", "reach", "writers", "branch_i")] + \
            [("use_cond.branch_iii[%d]" % i, x) for i, x in enumerate(uc.get("branch_iii", []))] + \
            [("pair %d (da)" % i, p.get("items", {}).get("da_no_name_mapping")) for i, p in enumerate(pairs)]
    if uc:
        for name, e in elems:
            if not isinstance(e, dict) or not isinstance(e.get("depends_on"), list):
                bad.append("JR 8.13 item 21 (16) (a): %s has no depends_on field" % name)
    db_ = rec.get("data_branch") or {}
    kinds = set(db_.get("admitted_kinds") or [])
    things = list(elems) + [("pair %d %s" % (i, k), v) for i, p in enumerate(pairs) for k, v in p.get("items", {}).items()
                            if k != "da_no_name_mapping"]
    for name, e in things:
        if not isinstance(e, dict) or e.get("status") not in ("present", "held"):
            continue
        dep = e.get("depends_on") or []
        if eb.get("data") == "D1" and dep:
            bad.append("JR 8.13 item 21 (16) (b): D1 record with %s %s on a list entry" % (name, e.get("status")))
        if eb.get("data") != "D1" and any(d.split("\t")[-1] not in kinds or (admitted is not None and d not in admitted)
                                          for d in dep):
            bad.append("JR 8.13 item 21 (16) (c): %s %s on a line not in the admission file of %s" % (name, e.get("status"), eb.get("data")))
        texts = [e.get("reason") or "", e.get("why") or ""] + [g for g in e.get("grounds", []) if isinstance(g, str)]
        if any("no entry: " in t for t in texts):
            bad.append("JR 8.13 item 21 (16) (d): %s %s with the sub-reason no entry" % (name, e.get("status")))
    for g in rec.get("order_ground", []):
        if not (isinstance(g, dict) and g.get("src")) and not (isinstance(g, str) and g.startswith("order_basis_unknown")):
            bad.append("JR 8.13 item 21 (16): order_ground is neither a pair-file line nor order_basis_unknown")
    db = rec.get("data_branch") or {}                                               # 17
    if not eb.get("id") or not db.get("admitted_sha256") or not db.get("candidates_sha256"):
        bad.append("JR 8.13 item 21 (17): eval_branch or the admission and candidate sha256 missing")
    if eb.get("data") == "D1" and db.get("admitted_sha256") != "none":
        bad.append("JR 8.13 item 21 (17): D1 record with an admission file")
    if [r for r in (eb.get("readings") or []) if r != "R0"] and eb.get("tag") != READING_TAG:
        bad.append("JR 8.13 item 21 (17): a reading branch without its tag")
    for r in all_reasons(rec):
        if not reason_form_ok(r):
            bad.append("JR 3.5: reason %r is not of the form name or name(sub-reason) with balanced parentheses" % (r,))
            continue
        if r.split("(", 1)[0] not in vocab:
            bad.append("JR 3.5: reason %r is not in the vocabulary" % r)
    return bad


# ------------------------------------------------------------------------------------------------ main
PREMISES = [
    {"id": "P-ARG", "text": "the start routine receives the creation argument unchanged (POSIX pthread_create); collated only "
                            "against the local glibc 2.39 copy; the build links the distribution's C library (results "
                            "document 1.7, c_library.txt), whose patches to that function are not established", "facts": None},
    {"id": "P-ACTOR", "text": "the runtime's actor ids are per thread (actor key tid:<n>; RUNX evidence line)"},
    {"id": "P-STACK", "text": "within one actor events are in program order (aseq) and calls are synchronous: an event between the "
                              "at-call and the after-call of one call in the same actor belongs to that call"},
    {"id": "P-CLOCK", "text": "event times of all actors are read from one clock (RUN clock id; the POSIX adapter reads "
                              "CLOCK_MONOTONIC, kairos/runtime/src/kz_rt_posix.c:43-44)"},
    {"id": "P-STR", "text": "a key text is NUL-terminated within its bound or the cut is flagged (manifest value refs)"},
    {"id": "P-CONST", "text": "the return constants the row gives for the use (its flip or callee-check line) are the status "
                              "constants of that callee; a create's success value is the one its success path line names"},
]


def run_budget(runN, run):
    rb = {"boots": 1 if run else 0}
    if run is None:
        rb["missing"] = ["no_run"]
        return rb
    bb = runN.get("boot_budget")
    if not bb:
        rb["missing"] = ["record_insufficient(no budget record of the boot given)"]
        return rb
    rb["source"] = "boot budget record"
    rb["run"] = {k: bb.get("run." + k) for k in ("value", "reason", "truncation", "unresolved", "sensitivity")}
    stop = {k: bb.get("stop." + k) for k in ("value", "source", "reason", "truncation", "sensitivity", "rule", "note")}
    pe = runN["process_end"]
    stl = [x for x in pe if x.startswith("stop=")]
    kill = [x for x in pe if "SIGKILL" in x or x.startswith("killed")]
    ex = [x for x in pe if x.startswith("tx_exit=") or x.startswith("rx_exit=")]
    if stl:
        stop["fired"] = "yes: run.meta '%s'%s; %s" % (stl[0], ("; " + "; ".join(ex)) if ex else "",
                                                       ("SIGKILL: " + "; ".join(kill)) if kill else "no SIGKILL line in run.meta")
    else:
        stop["fired"] = None
        rb.setdefault("missing", []).append("record_insufficient(no stop line in run.meta)")
    apps = {a: r for a, r in runN["stop_eval"].items() if not r.get("status", "").startswith("library")}
    notr = sorted(a for a, r in apps.items() if r.get("status") != "reached" or r.get("inside_window") != "yes")
    if not apps:
        stop["unresolved"] = None
        rb.setdefault("missing", []).append("record_insufficient(no stop-event evaluation given)")
    elif notr:
        stop["unresolved"] = "applications without their stop event inside the window: " + ", ".join(notr)
    else:
        stop["unresolved"] = "none: %d of %d applications reached their stop event inside the window (stop-event evaluation)" % (len(apps), len(apps))
    rb["stop"] = stop
    return rb


def load_vocab(path):
    with open(path, "rb") as f:
        data = f.read()
    return set(x.strip() for x in data.decode("utf-8").splitlines() if x.strip() and not x.startswith("#")), hashlib.sha256(data).hexdigest()


def main(argv):
    ap = argparse.ArgumentParser()
    for a in ("normal", "vocabulary", "out", "check"):
        ap.add_argument("--" + a)
    o = ap.parse_args(argv)
    if o.vocabulary is None:
        print("explain_judge.py: --vocabulary is required", file=sys.stderr)
        return 64
    vocab, vsha = load_vocab(o.vocabulary)
    if o.check:
        with open(o.check) as f:
            rec = json.load(f)
        bad = check_record(rec, vocab)
        for x in bad:
            print("refused: " + x)
        print("check %s (%d failures)" % ("ok" if not bad else "FAILED", len(bad)))
        return 4 if bad else 0
    if o.normal is None or o.out is None:
        print("explain_judge.py: --normal and --out are required", file=sys.stderr)
        return 64
    with open(o.normal, "rb") as f:
        nraw = f.read()
    N = json.loads(nraw.decode("utf-8"))
    if N.get("format") != "kz-explain-normal 1":
        print("explain_judge.py: not a kz-explain-normal 1 file", file=sys.stderr)
        return 64
    try:
        with open(os.path.abspath(__file__), "rb") as f:
            judge_sha = hashlib.sha256(f.read()).hexdigest()
    except OSError:
        judge_sha = None
    sc = N["plan"]["sidecar"]
    sites = {int(k): Site(int(k), a) for k, a in N["plan"]["sites"].items()}
    runN = N["run"]
    run = Run(runN["dump"], sites) if runN else None
    inputs = dict(N["inputs"])
    inputs["vocabulary"] = {"path": o.vocabulary, "sha256": vsha}
    inputs["normal"] = {"path": o.normal, "sha256": hashlib.sha256(nraw).hexdigest()}

    def sha(name):
        return inputs[name]["sha256"] if name in inputs else None
    fv = N["facts_view"]
    extractor = {"facts_tool_sha256": fv["tool_sha256"], "build_identity": fv["build_identity"], "source": "facts view %s" % fv["name"]}
    fw = [x["text"] for x in sc["inputs"] if x["text"].startswith("framework-layer-data ")]
    semantics = {"value": "p0-only", "p0_table": fv["p0_table"],
                 "framework_layer_data": re.search(r"sha256=([0-9a-f]{64})", fw[0]).group(1) if fw else None,
                 "namespace_data": [x["sha256"] for x in sc["namespace_data"]],
                 "scope": "the P0 table of the facts; framework-layer data for the run-loop and app-exit roles; the name-space data the plan names",
                 "missing": ["absent_result_unevaluated(no common-semantics table of the framework APIs in this cycle)"]}
    premises = [dict(p) for p in PREMISES]
    premises[0]["facts"] = fv["p0_pthread_create"]
    loss_any = False
    if run is not None:
        d = runN["dump"]
        rx = d["runx"]
        loss_any = any((ln.split() + ["", ""])[1] not in NONEVENT_LOSS for ln in d["loss"]) or any(int(rx.get(k, "0") or 0) for k in ("pool_dropped", "boot_bytes_dropped")) or \
            any(int(a.get("overflow", "0") or 0) or int(a.get("partial", "0") or 0) or int(a.get("dropped_bytes", "0") or 0)
                for a in d["actors"].values()) or int(d["format"].get("partial", "0") or 0) > 0
    mism = N["mismatch"]
    compat = (runN["compat"] if runN else "no run")
    if mism:
        compat = "refused: " + "; ".join(mism)
    N["branches_by_id"] = {b["id"]: b for b in N["branches"]}
    planned = [k for k, r in sc["rows"].items() if r.get("state") == "planned" or r.get("control-state") == "planned"]
    taken = [b["id"] for b in N["branches"] if os.path.exists(os.path.join(o.out, b["id"]))]
    if taken or os.path.exists(os.path.join(o.out, "judge.txt")):
        print("explain_judge.py: output exists in %s (new directories only)" % o.out, file=sys.stderr)
        return 64
    os.makedirs(o.out, exist_ok=True)
    with open(os.path.join(o.out, "inputs.txt"), "w") as f:
        f.write("# %s; reader %s %s\n" % (JUDGE_REV, N["reader"]["script"], N["reader"]["sha256"]))
        for n, v in sorted(inputs.items()):
            f.write("%s\t%s\t%s\n" % (n, v["sha256"], v["path"]))
        f.write("combination\t%s\n" % compat)
    with open(os.path.join(o.out, "input_checks.txt"), "w") as f:
        for c in N["input_checks"]:
            f.write("%s\t%s\t%s\n" % (c["check"], c["status"], c["why"]))
        for k, v in sorted(N["unparsed"].items()):
            f.write("unparsed\t%s\t%d\t%s\n" % (k, len(v), " ".join("%s:%d" % (a, b) for a, b in v[:20])))
    rc = 3 if mism else 0
    summary = []
    for bdesc in N["branches"]:
        br = Branch(bdesc, N["candidates"], N.get("named_calls"))
        bdir = os.path.join(o.out, br.id)
        os.makedirs(os.path.join(bdir, "records"))
        ctx = {"N": N, "sc": sc, "sites": sites, "run": run, "runN": runN, "bound": N["plan"]["bindings"]["bound"],
               "rel_head": N["relations"]["head"], "mismatch": bool(mism), "compat": compat, "extractor": extractor,
               "semantics": semantics, "premises": premises, "loss_any": loss_any, "apps": runN["apps"] if runN else [],
               "run_budget": run_budget(runN, run) if runN else {"boots": 0, "missing": ["no_run"]}, "static": {},
               "sha": sha, "judge_sha": judge_sha, "chain": runN["chain"] if runN else None,
               "buildids": [{k: v for k, v in x.items() if k != "src"} for x in (runN["dump"]["buildids"] if runN else [])],
               "tok_counts": runN["dump"]["tok_counts"] if runN else None, "use_rows": [], "lifetime_ends": []}
        index, refused = [], []
        adm_lines = set(br.admitted["lines"]) if br.admitted else set()
        for k in sorted(planned, key=lambda x: (int(sc["rows"][x].get("position", "0")), x)):
            rid = sc["rows"][k]["id"]
            rows = N["relations"]["rows"]
            if rid not in rows or N["keys"].get(rid, [None])[0] != k:
                refused.append("row %s %s: not in the relations or keys under this use-site key" % (k, rid))
                continue
            st = row_static(rows[rid], N["rel_name"])
            st["row_key"] = k
            st["rev14"] = bool((N["relations"]["head"].get("object_access") or {}).get("rev14_lines"))
            ctx["static"][k] = st
            if run is not None and not mism:
                us = {int(n) for n in sc["maps"].get(k, []) if "use-call" in sites[int(n)].rules}
                consumers = sorted({e.actor for e in run.events_at(us)}, key=akey)
                tied = {run.root[a]["root"] for a in consumers if run.root.get(a, {}).get("status") == "determined"}
                for root, kind in st["static_consumers"]:
                    if root in tied:
                        continue
                    cand = [a for a in run.by_actor if run.root.get(a, {}).get("root") == root]
                    consumers.extend(cand if cand else [("static", root, kind)])
            else:
                consumers = [("static", r, kd) for r, kd in st["static_consumers"]] or [("static", None, None)]
            if not consumers:
                consumers = [("static", None, None)]
            for c in consumers:
                rec = explain_row(ctx, k, c, br)
                rec["inputs"] = inputs
                bad = check_record(rec, vocab, adm_lines)
                cname = c if not isinstance(c, tuple) else "static-%s" % (c[1] or "none")
                if bad:
                    refused.extend("%s %s consumer %s: %s" % (k, rid, cname, x) for x in bad)
                    continue
                fn = "%s.%s.%s.actor-%s.json" % (N["build_label"], rid, k, cname)
                with open(os.path.join(bdir, "records", fn), "w") as f:
                    json.dump(rec, f, indent=1, ensure_ascii=False)
                    f.write("\n")
                tv = rec["tool_verdict"]
                pk = ",".join(sorted(set(rec["pair_kind"].values()))) or "-"
                index.append("\t".join([N["build_label"], br.id, rid, k, str(cname), rec["link"]["status"], rec["link"].get("basis") or "-",
                                        rec["use_cond"]["status"], rec["order"]["status"], rec["order"]["static"], rec["intervention"]["status"],
                                        rec["effect"]["status"], tv["violation"]["value"], tv["odf"]["value"], tv["headline"], pk,
                                        str(rec["uses"]), str(rec["uses_reached"]), str(rec["uses_decided"]),
                                        str(len(rec["link"]["name_binding"]["pairs"])),
                                        ";".join(tv["violation"]["undetermined_reasons"]), fn]))
        with open(os.path.join(bdir, "index.tsv"), "w") as f:
            f.write("build\tbranch\trow\tuse_site_key\tconsumer\tlink\tlink_basis\tuse_cond\torder\torder_static\tintervention\teffect\t"
                    "violation\todf\theadline\tpair_kind\tuses\tuses_reached\tuses_decided\tpairs\tundetermined_reasons\tfile\n")
            for ln in index:
                f.write(ln + "\n")
        with open(os.path.join(bdir, "uses.tsv"), "w") as f:
            f.write("build\trow\tuse_site_key\tconsumer\tuse\tactor\tat_aseq\tafter_aseq\tstatus\toutcome\tout\tkey_pointer\tkey_text\t"
                    "truncated\tlength\tkind_value\tcontext\tname_state\tlink_result\tlink_reasons\n")
            for r in ctx["use_rows"]:
                f.write("\t".join(r) + "\n")
        with open(os.path.join(bdir, "refused.txt"), "w") as f:
            for ln in refused:
                f.write(ln + "\n")
        summary.append("branch %s records %d refused %d uses %d" % (br.id, len(index), len(refused), len(ctx["use_rows"])))
        if refused:
            rc = 4
    with open(os.path.join(o.out, "judge.txt"), "w") as f:
        f.write("# %s sha256 %s\n" % (JUDGE_REV, judge_sha))
        f.write("compat\t%s\n" % compat)
        for ln in summary:
            f.write(ln + "\n")
    for ln in summary:
        print(ln)
    print("compat %s" % compat)
    return rc


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
