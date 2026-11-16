#!/usr/bin/env python3
"""explain.py -- explanation records of one natural boot of a build made from a generated observation plan.

  python3 -I explain.py --sidecar plan.side --manifest plan.kzs --bindings binds_instr.kzs \
      --relations relations.txt --keys keys.txt --vocabulary reason_vocabulary.txt --facts-view facts_view.txt \
      --bprime-basis basis.kz --build-label NAME --out NEW_DIR \
      [--dump-text tx.dump.txt --run-meta run.meta --chain chain.txt --stop-eval stop_eval.tsv --startup-script scr]
  python3 -I explain.py --check RECORD.json --vocabulary reason_vocabulary.txt

One record per (build, planned relation row, consumer actor) in the field names and status values of the judgement
rules (plan/ovm_rev1_20261116/JUDGEMENT_RULES_REV1.md, "JR" below: 3.1 identity fields, 3.2 the five axes, 3.3 the tool
verdict, 3.5 the reason vocabulary).  Every record passes the format checks of JR 3.6 before it is written; a record
that breaks one is refused (written to refused.txt only, rc 4).  Without --dump-text the records are those of no run
(order.status static_only).

Inputs: only the files named on the command line are opened; each is hashed into the record (inputs.txt and every
record's `inputs`).  The script reads no case value: rows, sites, values, keys and constants come from the plan
(sidecar, manifest), the relations text, the bindings and the dump.  Rules used, each named in the record:
  - creation-argument tie: the creator's at-call value of the creation argument (manifest value role creation_arg of
    an actor-create site) equals the child's start-routine param0 (role param0 of the actor-start site); a value seen
    more than once on either side makes every tie with it undetermined;
  - root tie: the actor-root entry recorded in the actor; the initial thread named by the RUNX record is the process;
  - call stacks: in each actor, events in aseq order; an at-call opens a call instance, the after-call of the same
    function, callee and line closes the innermost open one; a callee-side event (a site whose function is the callee)
    belongs to the innermost closed instance that brackets it, otherwise it is unattributed;
  - out values are read only when the status equals the success constant: the one constant among the row's return
    constants of the use that is not of failure class;
  - failure class (the method's S1 rule as ported in kairos/lib/llvm/plan_gen.cpp:58; vocabulary of the method
    document, plan/ORDER_VIOLATION_METHOD_20261111.md:381 (a) cFE severity bits, (b) OSAL negative values): v < 0, or
    0xC0000000 <= v <= 0xFFFFFFFF.  It grades run returns for odf only; never for use_cond (JR 3.6 item 3);
  - the axes by JR 3.2 and tool_verdict by JR 3.3, computed only from the axis statuses.
Research script outside the tool core (Python is not allowed there); a port to kz is a later step.
"""
import argparse
import hashlib
import json
import os
import re
import sys

SCRIPT_REV = "explain.py revision 1 (2026-11-16)"
INPUTS = {}  # name -> (path, sha256)


def read_input(name, path):
    """The only reader of input files: every file it opens is declared by its argument name and hashed."""
    with open(path, "rb") as f:
        data = f.read()
    INPUTS[name] = (path, hashlib.sha256(data).hexdigest())
    return data.decode("utf-8", errors="replace")


def sha_of(name):
    return INPUTS[name][1] if name in INPUTS else None


def same_status(const, v):
    """A row constant (printed unsigned for 32-bit statuses) against a recorded status (s64, sign-extended from the
    callee's 32-bit status: the plugin widens from the operand's own type): equal, or equal modulo 2**32 when the
    recorded value lies in the 32-bit range."""
    if const is None or v is None:
        return False
    if const == v:
        return True
    return -(1 << 31) <= v < (1 << 32) and (const & 0xFFFFFFFF) == (v & 0xFFFFFFFF)


def fail_class(v):
    return v is not None and (v < 0 or 0xC0000000 <= v <= 0xFFFFFFFF)


FAIL_CLASS_RULE = ("failure class: v < 0 or 0xC0000000 <= v <= 0xFFFFFFFF (the method's S1 rule as ported in "
                   "kairos/lib/llvm/plan_gen.cpp:58; vocabulary plan/ORDER_VIOLATION_METHOD_20261111.md:381 (a), (b))")

KV = re.compile(r'(\w+)=("(?:[^"\\]|\\.)*"|\S+)')


def kv(text):
    out = {}
    for k, v in KV.findall(text):
        if v.startswith('"'):
            v = v[1:-1].replace('\\"', '"').replace("\\\\", "\\")
        out[k] = v
    return out


# ----------------------------------------------------------------------------------------------- the plan
class Site:
    def __init__(self, n, a):
        self.n = n
        self.label = a.get("label", "")
        self.rules = self.label.split(".", 1)[1].split("+") if "." in self.label else []
        self.op = a.get("op", "")
        self.phase = a.get("phase", "")
        self.tu = a.get("tu", "")
        self.func = a.get("func", "")
        self.callee = a.get("callee")
        self.line = a.get("line")
        self.place = a.get("place") or ("entry" if self.op.startswith("entry.") else None)
        if self.place == "entry":
            self.entry_of = self.op[len("entry."):]
        else:
            self.entry_of = None
        self.ref = a.get("ref", "")
        self.values = {}  # index -> dict(role, from, type, string, bound, ref)

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


def parse_manifest(text):
    sites = {}
    for ln in text.splitlines():
        if ln.startswith("site "):
            p = ln.split(" ", 2)
            sites[int(p[1])] = Site(int(p[1]), kv(p[2] if len(p) > 2 else ""))
        elif ln.startswith("value "):
            p = ln.split(" ", 3)
            s = sites.get(int(p[1]))
            if s is not None:
                s.values[int(p[2])] = kv(p[3] if len(p) > 3 else "")
    return sites


def parse_sidecar(text):
    sc = {"rows": {}, "maps": {}, "needs": {}, "gaps": {}, "inputs": [], "budget": None, "bounds": [], "head": {},
          "stop": [], "creation": None}
    for ln in text.splitlines():
        if ln.startswith("row "):
            p = ln.split(" ", 3)
            a = kv(p[3]) if len(p) > 3 else {}
            a["key"], a["id"] = p[1], p[2]
            sc["rows"][p[1]] = a
        elif ln.startswith("map "):
            p = ln.split()
            sc["maps"][p[1]] = [int(x) for x in p[2:]]
        elif ln.startswith("need row "):
            p = ln.split(" ", 5)
            a = kv(p[5]) if len(p) > 5 else {}
            sc["needs"].setdefault(p[2], []).append({"what": p[3], "state": p[4], "reason": a.get("reason"),
                                                      "note": a.get("note")})
        elif ln.startswith("gap row "):
            m = re.match(r'gap row (\S+) "((?:[^"\\]|\\.)*)" (.*)$', ln)
            if m:
                a = kv(m.group(3))
                sc["gaps"].setdefault(m.group(1), []).append({"text": m.group(2), "reason": a.get("reason"),
                                                              "rule": a.get("rule")})
        elif ln.startswith("input "):
            sc["inputs"].append(ln[len("input "):])
        elif ln.startswith("budget "):
            sc["budget"] = kv(ln[len("budget "):])
            m = re.search(r"truncation=(.*?) unresolved=", ln)
            if m:
                sc["budget"]["truncation"] = m.group(1)
        elif ln.startswith("bound "):
            sc["bounds"].append(ln[len("bound "):])
        elif ln.startswith("stop-event "):
            sc["stop"].append(ln[len("stop-event "):])
        elif ln.startswith("creation "):
            sc["creation"] = ln[len("creation "):]
        else:
            w = ln.split(" ", 1)
            if w[0] in ("generator", "rules", "selection", "priority", "classes", "facts-artifact", "basis-artifact"):
                sc["head"][w[0]] = w[1] if len(w) > 1 else ""
    return sc


def parse_bindings(text):
    """site -> mask of bound values, or None if no line binds it (status bound, duplicated or inlined)."""
    bound = {}
    other = {}
    for ln in text.splitlines():
        if not ln.startswith("bind "):
            continue
        a = kv(ln)
        try:
            s = int(a.get("site", "-1"))
        except ValueError:
            continue
        st = a.get("status", "")
        if st in ("bound", "duplicated", "inlined"):
            bound[s] = bound.get(s, 0) | int(a.get("vals", "0x0"), 16)
        else:
            other.setdefault(s, []).append(st + (" " + a["reason"] if "reason" in a else ""))
    return bound, other


# ----------------------------------------------------------------------------------------------- the relations
ROW_HEAD = re.compile(r"^  ([ABC]\d+) ")


def parse_relations(text):
    rows = {}
    lines = text.splitlines()
    head = {}
    for ln in lines[:6]:
        w = ln.split(" ", 1)
        if w[0] in ("facts", "rules"):
            head[w[0]] = w[1] if len(w) > 1 else ""
    cur = None
    for i, ln in enumerate(lines, 1):
        m = ROW_HEAD.match(ln)
        if m:
            cur = {"id": m.group(1), "line": i, "head": ln.strip(), "body": []}
            rows[m.group(1)] = cur
        elif cur is not None and ln.startswith("      "):
            cur["body"].append((i, ln.strip()))
        else:
            cur = None
    return head, rows


def parse_keys(text):
    keys = {}
    head = {}
    for ln in text.splitlines():
        p = ln.split(" ", 2)
        if len(p) == 3 and re.fullmatch(r"[ABC]\d+", p[0]) and re.fullmatch(r"[0-9a-f]{16}", p[1]):
            keys[p[0]] = (p[1], p[2])
        elif p[0] in ("facts", "rules"):
            head[p[0]] = ln.split(" ", 1)[1]
    return head, keys


CONST = re.compile(r"returns (-?\d+) \[([^\]]*)\]")


def row_static(row, rel_name):
    """What the row text gives, each with its relations line."""
    path = row["id"][0]
    body = row["body"]

    def first(prefix):
        for n, t in body:
            if t.startswith(prefix):
                return n, t
        return None, None

    st = {"path": path, "head": row["head"], "head_line": row["line"], "ref": lambda n: "%s:%d" % (rel_name, n)}
    m = re.search(r"-> (\S+)#(\d+)", row["head"])
    st["callee"] = m.group(1) if m else None
    st["param"] = int(m.group(2)) if m else None
    st["key_unresolved"] = "key unresolved" in row["head"]
    m = re.search(r"namespace (\S+) partition \{([^}]*)\}", row["head"])
    st["namespace"] = m.group(1) if m else None
    st["partition"] = m.group(2) if m else None
    m = re.match(r"[ABC]\d+ (\S+) -> ", row["head"]) if path == "B" else None
    st["field"] = m.group(1) if m else None
    n, t = first("actors:")
    st["actors_line"] = (n, t)
    roots = []
    if t:
        if path == "B":
            mm = re.search(r"readers \{([^}]*)\}", t)
        else:
            mm = re.search(r"actors: \{([^}]*)\}", t)
        if mm and mm.group(1).strip():
            for part in mm.group(1).split(","):
                part = part.strip()
                r = re.match(r"(\S+) \(([^)]*)\)", part)
                if r:
                    roots.append((r.group(1), r.group(2)))
    st["static_consumers"] = roots
    n, t = first("flip:")
    st["flip"] = (n, t)
    consts = []
    if path == "A" and t:
        consts = [(int(v), name, n) for v, name in CONST.findall(t)]
    elif path in ("B", "C"):
        for n2, t2 in body:
            if t2.startswith("callee-check: %s returns" % st["callee"]) or (path == "C" and t2.startswith("flip:")):
                for v, name in CONST.findall(t2):
                    consts.append((int(v), name, n2))
    st["consts"] = consts
    succ = sorted({(v, name) for v, name, _ in consts if not fail_class(v)})
    st["success"] = succ[0] if len(succ) == 1 else None
    st["success_note"] = ("one non-failure-class constant" if len(succ) == 1 else
                          "%d non-failure-class constants among the row's return constants of the use" % len(succ))
    st["creators"] = [(n, t) for n, t in body if t.startswith("creator:")]
    st["writers"] = [(n, t) for n, t in body if re.match(r"(out-pointer|assign|memset|call|store|copy) at ", t)]
    n, t = first("field:")
    st["field_line"] = (n, t)
    m = re.match(r"field: INIT (-?\d+)", t) if t else None
    st["init"] = int(m.group(1)) if m else None
    st["handling"] = [(n, t) for n, t in body if t.startswith("handling:")]
    st["feasibility"] = first("feasibility:")
    st["dependency"] = first("dependency:")
    n, t = first("incomplete:")
    st["incomplete"] = [x.strip() for x in t[len("incomplete:"):].split(";")] if t else []
    st["incomplete_line"] = n
    n, t = first("site ")
    st["site_line"] = (n, t)
    return st


# ----------------------------------------------------------------------------------------------- the dump
EV = re.compile(r"^ev actor=(\S+) aseq=(\d+) site=(\d+)\[[^\]]*\] bind=\d+ link=\d+ t=(\d+) pc=\S+ key=(\S+) "
                r"v=\[([^\]]*)\](?: str=(\"(?:[^\"\\]|\\.)*\"))? flags=(\S*)$")


class Event:
    __slots__ = ("actor", "aseq", "site", "t", "key", "v", "s", "flags")


def parse_dump(text):
    d = {"run": {}, "runx": {}, "actors": {}, "events": [], "loss": [], "format": {}, "check_ok": None,
         "module_manifests": set(), "run_manifest": None}
    for ln in text.splitlines():
        if ln.startswith("ev "):
            m = EV.match(ln)
            if not m:
                d.setdefault("unparsed", 0)
                d["unparsed"] += 1
                continue
            e = Event()
            e.actor, e.aseq, e.site, e.t, e.key = m.group(1), int(m.group(2)), int(m.group(3)), int(m.group(4)), m.group(5)
            e.v = [None if x == "-" else int(x, 16) for x in m.group(6).split(",")] if m.group(6) else []
            e.s = None
            if m.group(7) is not None:
                e.s = m.group(7)[1:-1]
            e.flags = set(m.group(8).split(",")) if m.group(8) else set()
            d["events"].append(e)
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
            a = kv(ln)
            if "manifest_sha256" in a:
                d["module_manifests"].add(a["manifest_sha256"])
        elif ln.startswith("format "):
            d["format"] = kv(ln)
        elif ln.startswith("check ok") or ln.startswith("check FAIL"):
            d["check_ok"] = ln
    d["run_manifest"] = d["run"].get("manifest_sha256")
    return d


def signed(v, typ):
    if v is None:
        return None
    if typ == "s64" and v >= 1 << 63:
        return v - (1 << 64)
    return v


def hx(v):
    return None if v is None else ("-0x%x" % -v if v < 0 else "0x%x" % v)


# ----------------------------------------------------------------------------------------------- run model
class Inst:
    def __init__(self, actor, at, after, site_at, site_after, func, callee, line):
        self.actor, self.at, self.after = actor, at, after
        self.site_at, self.site_after = site_at, site_after
        self.func, self.callee, self.line = func, callee, line

    def bracketed(self, e):
        return self.at is not None and self.after is not None and self.at.aseq < e.aseq < self.after.aseq


class Run:
    def __init__(self, dump, sites):
        self.d = dump
        self.sites = sites
        self.by_actor = {}
        for e in dump["events"]:
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
                    for g in stack[hit + 1:]:
                        out.append(g)  # opened inside, never closed
                    del stack[hit:]
        out.extend(stack)
        out.sort(key=lambda i: (i.at or i.after).aseq)
        return out

    def owner(self, e):
        """The call instance a callee-side event belongs to: the innermost closed instance of its function."""
        s = self.sites[e.site]
        func = s.entry_of if s.place == "entry" else s.func
        best = None
        for i in self.insts.get(e.actor, []):
            if i.callee == func and i.bracketed(e) and (best is None or i.at.aseq > best.at.aseq):
                best = i
        return best

    def events_at(self, site_ids, actor=None):
        return [e for e in self.d["events"] if e.site in site_ids and (actor is None or e.actor == actor)]

    def val(self, e, role):
        s = self.sites[e.site]
        i = s.role_index(role)
        if i is None or i >= len(e.v):
            return None
        return signed(e.v[i], s.values[i].get("type"))

    def ms(self, t):
        return round((t - self.t_init) / 1e6, 3)

    def _ties(self):
        creates = []  # (inst, value)
        starts = []   # (event, value)
        for a, insts in self.insts.items():
            for i in insts:
                if i.site_at is not None and "actor-create" in i.site_at.rules and i.at is not None:
                    creates.append((i, self.val(i.at, "creation_arg")))
        for e in self.d["events"]:
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
                    ci = [i for i, w in creates if w == v][0]
                    t["status"], t["creator"] = "determined", ci
                    t["why"] = "creation argument %s seen once among creation calls and once among start entries" % hx(v)
            self.creation[a] = t
        self.unmatched_creates = [(i, v) for i, v in creates if cs.get(v, 0) == 0]
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
        """actor-owner stores of the creator after a creation call, before its next creation call."""
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


# ----------------------------------------------------------------------------------------------- record pieces
def evref(run, e, extra=None):
    s = run.sites[e.site]
    r = {"actor": e.actor, "aseq": e.aseq, "site": e.site, "rule": "+".join(s.rules), "where": s.where(),
         "t_ms": run.ms(e.t), "values": {}}
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


def actor_view(run, a, apps):
    ct = run.creation.get(a, {})
    rt = run.root.get(a, {})
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
        return {"status": "determined", "app": "(process)", "class": "creator", "why": rt["why"]}
    return {"status": "unknown", "why": "root kind %s" % kind}


def pair_kind(c, p):
    a, b = c.get("app", {}), p.get("app", {})
    if a.get("status") != "determined" or b.get("status") != "determined":
        return "unknown"
    ca, cb = a.get("class"), b.get("class")
    if c["run_actor"] == p["run_actor"]:
        return "unknown"
    if ca in ("app", "child") and cb in ("app", "child"):
        if a.get("app") == b.get("app"):
            if "child" in (ca, cb) and (a.get("parent_actor") == p["run_actor"] or b.get("parent_actor") == c["run_actor"]):
                return "same-app-main-child"
            return "unknown"
        return "app-app"
    if ca == "core" and cb == "core":
        return "core-core"
    if "creator" in (ca, cb) and ({ca, cb} & {"app", "child"}):
        return "creator-app"
    return "unknown"


# ----------------------------------------------------------------------------------------------- one record
NEED_REASON = {"present": None, "value-missing": "record_insufficient(value)", "text-partial": "record_insufficient(key text)",
               "not-reached": "target_not_reached", "unknown-loss": "record_insufficient(loss marker)",
               "unbound": "record_insufficient(site not bound in the build)", "not-planned": "plan_missing(need not planned)",
               "build-mismatch": "build_mismatch"}


def site_need(run, sites, bound, site_id, loss_any, actor=None):
    """The need state of one planned site (JR 3.5 table of need states); scoped to an actor when given."""
    s = sites[site_id]
    st = {"need": "site %d %s %s" % (site_id, "+".join(s.rules), s.where())}
    if actor is not None:
        st["actor"] = actor
    if site_id not in bound:
        st["state"] = "unbound"
    else:
        missing_vals = [s.values[i].get("role") for i in s.values if not (bound[site_id] >> i) & 1]
        evs = run.events_at({site_id}, actor) if run else []
        if run is None:
            st["state"] = "not-reached"
            st["note"] = "no run"
        elif not evs:
            st["state"] = "unknown-loss" if loss_any else "not-reached"
        else:
            st["events"] = len(evs)
            vm = [s.values[i].get("role") for e in evs for i in s.values if (i >= len(e.v) or e.v[i] is None)]
            tp = [e for e in evs if any(v.get("string") == "validated" for v in s.values.values()) and
                  (e.s is None or "str-trunc" in e.flags)]
            if missing_vals:
                st["state"] = "unbound"
                st["note"] = "values not bound: " + ", ".join(sorted(set(missing_vals)))
            elif vm:
                st["state"] = "value-missing"
                st["note"] = "values missing: " + ", ".join(sorted(set(vm)))
            elif tp:
                st["state"] = "text-partial"
            else:
                st["state"] = "present"
    st["reason"] = NEED_REASON[st["state"]]
    return st


def key_of_instance(run, inst, keytext_sites):
    """Key pointer at the call, key text from a callee-side key-text event the call brackets."""
    out = {"pointer": None, "text": None, "truncated": None, "text_event": None, "pointer_equal": None}
    if inst.at is not None:
        out["pointer"] = run.val(inst.at, "key")
    for e in run.by_actor.get(inst.actor, []):
        if e.site in keytext_sites and run.owner(e) is inst:
            out["text"] = e.s
            out["truncated"] = "str-trunc" in e.flags
            out["text_event"] = e
            kt = run.val(e, "key_text")
            out["pointer_equal"] = (kt == out["pointer"]) if (kt is not None and out["pointer"] is not None) else None
            break
    return out


def complete_key(k):
    return k["text"] is not None and not k["truncated"]


def explain_row(ctx, rowkey, consumer):
    """One record: rowkey (use-site key), consumer = run actor id or ('static', root, kind) without a run actor."""
    sc, sites, run, st = ctx["sc"], ctx["sites"], ctx["run"], ctx["static"][rowkey]
    srow = sc["rows"][rowkey]
    smap = set(sc["maps"].get(rowkey, []))
    by_rule = {}
    for n in smap:
        for r in sites[n].rules:
            by_rule.setdefault(r, set()).add(n)
    use_sites = by_rule.get("use-call", set())
    kt_sites = by_rule.get("key-text", set())
    pub_sites = by_rule.get("creator-call", set()) | by_rule.get("writer", set())
    hc_sites = by_rule.get("handling-chain", set()) - use_sites
    rec = {"record": "explanation", "format": "JR 3.1-3.2 fields, 3.3 verdict, 3.5 reasons", "script": SCRIPT_REV}
    mismatch = ctx["mismatch"]
    # ---- identity fields (JR 3.1)
    rec["build_id"] = {"build": ctx["label"], "bprime_basis_sha256": sha_of("bprime_basis"),
                       "executables_run_meta": ctx["exes"], "plan_facts_sha256": ctx["facts_sha"],
                       "compat": ctx["compat"]}
    rec["extractor_rev"] = ctx["extractor"]
    rec["semantics_rev"] = ctx["semantics"]
    rec["plan_id"] = {"sidecar_sha256": sha_of("sidecar"), "manifest_sha256": sha_of("manifest"),
                      "run_record_manifest_sha256": run.d["run_manifest"] if run else None}
    keytext = ctx["keys"].get(srow["id"], (None, None))[1]
    rec["row_id"] = {"use_site_key": rowkey, "key_text": keytext, "row_counter": srow["id"],
                     "row_counter_note": "the row number is an output counter of the relations text, not an identity",
                     "relations": "%s:%d" % (ctx["rel_name"], st["head_line"])}
    rec["path"] = st["path"]
    rec["handling_label"] = srow.get("label")
    rec["handling_unknown_reason"] = srow.get("why") if srow.get("label") in ("T?", "J?") else None
    rec["priority"] = {k: srow.get(k) for k in ("tier", "position", "label", "class", "order", "reach", "disposition", "why")}
    rec["priority"]["rule"] = sc["head"].get("priority")
    b = sc["budget"] or {}
    rec["budget"] = {"plan": {k: b.get(k) for k in ("value", "reason", "disclosure", "planned", "truncation", "unresolved", "sensitivity")},
                     "bounds": sc["bounds"], "run": ctx["run_budget"]}
    rec["review"] = None
    rec["eval_result"] = None
    rec["premises"] = list(ctx["premises"])
    needs = []
    loss_any = ctx["loss_any"]
    # ---- actors
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
    # ---- use instances of the consumer
    uses = []
    if ca is not None:
        for i in run.insts.get(ca, []):
            if (i.site_at is not None and i.site_at.n in use_sites) or (i.site_after is not None and i.site_after.n in use_sites):
                uses.append(i)
    succ = st["success"]
    use_events = []
    for i in uses:
        u = {"call": "%s -> %s line %s" % (i.func, i.callee, i.line)}
        if i.at is not None:
            u["at_call"] = evref(run, i.at)
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
        if st["path"] == "A":
            k = key_of_instance(run, i, kt_sites)
            u["key"] = {"pointer": hx(k["pointer"]), "text": k["text"], "truncated": k["truncated"],
                        "text_event": evref(run, k["text_event"]) if k["text_event"] else None,
                        "pointer_equal_to_key_text_argument": k["pointer_equal"]}
            u["_k"] = k
        else:
            fv = run.val(i.at, "field_value") if i.at is not None else None
            u["field_value"] = hx(fv)
            u["_fv"] = fv
        u["_i"] = i
        use_events.append(u)
    # ---- publishers
    pubs = []
    if run is not None and not mismatch:
        for a, insts in run.insts.items():
            if a == ca:
                continue
            for i in insts:
                if (i.site_at is not None and i.site_at.n in pub_sites) or (i.site_after is not None and i.site_after.n in pub_sites):
                    pubs.append(i)
            for e in run.by_actor[a]:
                s = sites.get(e.site)
                if s is not None and e.site in pub_sites and s.place == "store":
                    pubs.append(Inst(a, e, e, s, s, s.func, None, s.line))
    pub_actors = sorted({i.actor for i in pubs}, key=lambda x: int(x) if x.isdigit() else 0)
    pviews = {a: actor_view(run, a, apps) for a in pub_actors}
    publish_events = []
    for i in pubs:
        p = {"actor": i.actor, "call": "%s -> %s line %s" % (i.func, i.callee, i.line) if i.callee else "store in %s line %s" % (i.func, i.line)}
        if i.at is not None and i.at is not i.after:
            p["at_call"] = evref(run, i.at)
        if i.after is not None:
            p["after_call"] = evref(run, i.after) if i.at is not i.after else None
            if i.at is i.after:
                p["store"] = evref(run, i.at)
            else:
                p["status_value"] = hx(run.val(i.after, "status"))
                p["out"] = {"value": hx(run.val(i.after, "out0")), "read": "recorded; not graded (the creator's return constants are not in the row)"}
        if st["path"] == "A":
            k = key_of_instance(run, i, kt_sites)
            p["key"] = {"pointer": hx(k["pointer"]), "text": k["text"], "truncated": k["truncated"],
                        "text_event": evref(run, k["text_event"]) if k["text_event"] else None,
                        "pointer_equal_to_key_text_argument": k["pointer_equal"]}
            p["_k"] = k
        p["_i"] = i
        publish_events.append(p)
    rec["publisher_actors"] = [pviews[a] for a in pub_actors]
    rec["actor_pair_kind"] = {a: pair_kind(cview, pviews[a]) for a in pub_actors} if ca is not None else {}
    # ---- needs (incompleteness)
    if mismatch:
        for n in sorted(smap):
            needs.append({"need": "site %d" % n, "state": "build-mismatch", "reason": NEED_REASON["build-mismatch"]})
    else:
        for n in sorted(smap):
            scope = ca if (n in use_sites or n in hc_sites) and ca is not None else None
            needs.append(site_need(run, sites, ctx["bound"], n, loss_any, scope))
    for nd in sc["needs"].get(rowkey, []):
        needs.append({"need": "%s (sidecar)" % nd["what"], "state": nd["state"], "reason": nd["reason"], "note": nd["note"]})
    rec["incompleteness"] = {
        "needs": needs,
        "static_gaps": sc["gaps"].get(rowkey, []),
        "static_incomplete": [x for x in st["incomplete"]],
        "static_incomplete_source": st["ref"](st["incomplete_line"]) if st["incomplete_line"] else None,
        "budget_truncation": b.get("truncation"),
        "open_premises": [p["id"] for p in ctx["premises"]],
    }
    # ---- axis 1 link
    link = {"resource_kind": {"A": "named object", "B": "shared memory field", "C": "registration record"}[st["path"]],
            "actor_token": cview.get("creation_tie"), "app_of_actor": cview.get("app"),
            "static_basis": {"row": "%s:%d" % (ctx["rel_name"], st["head_line"]), "use_site_key": rowkey,
                             "relations_sha256": sha_of("relations"), "keys_sha256": sha_of("keys")},
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
    status = "unlinked"
    grounds = []
    if st["path"] == "A":
        link["resource_key"] = {"namespace": st["namespace"], "partition": st["partition"],
                                "namespace_source": "%s:%d" % (ctx["rel_name"], st["head_line"]),
                                "name": None, "generation": None}
        ukeys = [u["_k"] for u in use_events]
        if ca is not None and not use_events:
            lm.append("target_not_reached")
        elif ca is not None:
            gap_key = [g for g in sc["gaps"].get(rowkey, []) if g.get("rule") == "key-text"]
            uk = ukeys[0]
            if uk["text"] is None:
                lm.append("record_insufficient(key text)" + ("" if not gap_key else ""))
            elif uk["truncated"]:
                lm.append("record_insufficient(key text)")
            else:
                link["resource_key"]["name"] = uk["text"]
            texts = sorted({k["text"] for k in ukeys if k["text"] is not None})
            if len(texts) > 1:
                lm.append("multi_state_unlinked(the consumer's uses name %d keys)" % len(texts))
        if not publish_events and ca is not None:
            lm.append("link_unlinked(no publisher event of the row in another actor)")
        matched, differing, unknown = [], [], []
        if ca is not None and use_events and complete_key(use_events[0]["_k"]):
            ukey = use_events[0]["_k"]["text"]
            for p in publish_events:
                pv = pviews[p["actor"]]
                tied = pv["creation_tie"]["status"] == "determined" and pv["root_tie"]["status"] == "determined"
                if not complete_key(p["_k"]):
                    unknown.append((p, "publisher key text not in the record"))
                elif p["_k"]["text"] == ukey:
                    (matched if tied else unknown).append((p, "publisher not tied") if not tied else p)
                else:
                    differing.append(p)
            for p in matched:
                link["instance_evidence"].append({"publisher_actor": p["actor"], "key_text": p["_k"]["text"],
                                                  "create": p.get("at_call", {}).get("aseq"), "out": p.get("out", {}).get("value")})
            for p in differing:
                link["instance_evidence"].append({"publisher_actor": p["actor"], "key_text": p["_k"]["text"],
                                                  "differs_from_use_key": ukey})
            u0 = use_events[0]
            if matched:
                if u0.get("out", {}).get("read"):
                    same = [p for p in matched if p.get("out", {}).get("value") == u0["out"]["value"]]
                    if same:
                        status = "established"
                        grounds.append("the use's out id %s (read under the success constant) is the out id of the "
                                       "publisher's create with the same key" % u0["out"]["value"])
                    else:
                        lm.append("link_unlinked(the use's out id is not the out id of a publisher's create with its key)")
                else:
                    status = "established"
                    grounds.append("absence outcome: the use and the publisher's create name the same key in the "
                                   "namespace partition of the row in one process of one run; the use reached no "
                                   "instance (premise L-ABS)")
                link["resource_key"]["generation"] = [p.get("out", {}).get("value") for p in matched]
            elif differing and not unknown and not lm:
                status = "refuted"
                grounds.append("every publisher event of the row in the record names another key: " +
                               ", ".join("actor %s %r" % (p["actor"], p["_k"]["text"]) for p in differing))
            else:
                for p, why in unknown:
                    lm.append("link_unlinked(publisher actor %s: %s)" % (p["actor"], why))
                if not matched and not unknown and differing:
                    lm.append("link_unlinked(no publisher event names the use's key)")
        if status == "established" and any(x.startswith("link_unlinked(consumer") for x in lm):
            status = "unlinked"
    else:
        link["resource_key"] = {"field": st["field"], "use_site": st["site_line"][1],
                                "source": st["ref"](st["site_line"][0]) if st["site_line"][0] else None}
        if ca is not None and not use_events:
            lm.append("target_not_reached")
        has_addr = any(sites[n].role_index("address") is not None and sites[n].place == "at-call" for n in use_sites)
        if not has_addr:
            lm.append("record_insufficient(the address read at the use is not in the plan)")
        if not publish_events:
            lm.append("link_unlinked(no writer event of the row in another actor)")
    if status == "established" and lm:
        status = "unlinked"
    link["status"] = status
    link["grounds"] = grounds
    rec["link"] = link
    # ---- axis 2 use_cond (static only; run key values only, JR 4.2)
    uc = {"rule": {"A": "JR 4.2 name or key lookup (path A)", "B": "JR 4.2 placeholder field reaching a callee check (path B)",
                   "C": "JR 4.2 registration record read by the created task (path C)"}[st["path"]],
          "premises": ["static row text of %s (relations rules: %s)" % (ctx["rel_name"], ctx["rel_head"].get("rules"))],
          "missing": []}
    um = uc["missing"]
    fcs = [(v, nm, n) for v, nm, n in st["consts"] if fail_class(v)]
    if fcs:
        uc["callee_check"] = {"status": "present", "text": ", ".join("%d [%s]" % (v, nm) for v, nm, _ in st["consts"]),
                              "sources": [{"kind": "static-row", "ref": st["ref"](n)} for n in sorted({n for _, _, n in st["consts"]})]}
    else:
        uc["callee_check"] = {"status": "missing", "sources": []}
        um.append("absent_result_unevaluated(the row gives no failure-class return constant of the use)")
    fn, ft = st["flip"]
    if ft and ft.startswith("flip: static-possible") and "with the earlier value" in ft:
        uc["reach"] = {"status": "present", "text": ft, "sources": [{"kind": "static-row", "ref": st["ref"](fn)}]}
    else:
        uc["reach"] = {"status": "missing", "text": ft, "sources": [{"kind": "static-row", "ref": st["ref"](fn)}] if fn else []}
        um.append("absent_result_unevaluated(%s)" % (ft[len("flip: "):].split(":")[0] if ft else "no flip line"))
        if ft and "not resolved statically" in ft:
            um.append("key_unresolved(the key of the use or of a creator is not resolved statically: %s)" % st["ref"](fn))
    term = [(n, t) for n, t in st["handling"] if t.startswith("handling: terminal")]
    if term and not any(re.search(r"\bmay\)|: may\b", t) for _, t in term):
        uc["caller_dependence"] = {"status": "present", "branch": "(iii) a terminal call on the failure path",
                                   "sources": [{"kind": "static-row", "ref": st["ref"](n)} for n, _ in term]}
    else:
        why = ("the terminal chain's guard is linked 'may'" if term else
               ("the result is %s" % st["handling"][0][1][len("handling: "):] if st["handling"] else "no handling line"))
        uc["caller_dependence"] = {"status": "missing", "text": why,
                                   "sources": [{"kind": "static-row", "ref": st["ref"](n)} for n, _ in (term or st["handling"][:1])]}
        um.append("caller_dependence_unknown(%s)" % why)
    wl = st["creators"] if st["path"] == "A" else st["writers"]
    fz = st["feasibility"][1] or ""
    winc = [x for x in st["incomplete"] if x.startswith("writer_set_incomplete")]
    kinc = [x for x in st["incomplete"] if x.startswith("key_unresolved(creator")] + \
           (["key_unresolved(creator key unresolved: %s)" % st["ref"](n) for n, t in wl if "key unresolved" in t][:1])
    decided = fz.startswith("feasibility: possible") or fz.startswith("feasibility: proven-impossible")
    wsrc = [{"kind": "static-row", "ref": st["ref"](n)} for n, _ in wl]
    if wl and not winc and not kinc and decided:
        uc["writers"] = {"status": "present", "count": len(wl), "sources": wsrc + [{"kind": "static-row", "ref": st["ref"](st["feasibility"][0])}]}
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
    if st["path"] == "A" and use_events and use_events[0]["_k"]["text"] is not None:
        uc["run_key_value"] = {"text": use_events[0]["_k"]["text"], "sources": [{"kind": "record-key",
                               "ref": "dump actor %s aseq %d" % (ca, use_events[0]["_k"]["text_event"].aseq)}]}
    elems = ["callee_check", "reach", "caller_dependence", "writers"]
    np_ = sum(1 for e in elems if uc[e]["status"] == "present")
    uc["status"] = "derived" if np_ == 4 else ("partial" if np_ > 0 else "not_derived")
    if uc["status"] == "derived":
        uc["predicate"] = ("an object with the use's key exists in the namespace partition at the use" if st["path"] == "A"
                           else "the field holds a value other than its absence value %s at the use" % st["init"])
    else:
        uc["predicate"] = None
    rec["use_cond"] = uc
    # ---- axis 3 order
    order = {"static": "unknown", "static_reason": "order edges not computed in this cycle",
             "static_note": {"text": fz, "source": st["ref"](st["feasibility"][0]) if st["feasibility"][0] else None},
             "run_id": ({"run_id": run.d["run"].get("run_id"), "dump_text_sha256": sha_of("dump_text"),
                         "clock": run.d["run"].get("clock")} if run else None),
             "use_events": [{k: v for k, v in u.items() if not k.startswith("_")} for u in use_events],
             "publish_events": [{k: v for k, v in p.items() if not k.startswith("_")} for p in publish_events],
             "sync_results": {"creation": [], "wait": "not-planned: plan_missing(need not planned)",
                              "lock": "not-planned: plan_missing(need not planned)"},
             "missing": []}
    om = order["missing"]
    for v in [cview] + [pviews[a] for a in pub_actors]:
        cr = v.get("creation_tie", {}).get("creation_result")
        if cr:
            order["sync_results"]["creation"].append({"actor": v["run_actor"], "creation_result": cr})
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
            order["status"], why = evaluate_p(st, use_events, publish_events)
            if why:
                om.append(why)
        else:
            order["status"] = "events_only"
            om.append("absent_result_unevaluated(the predicate is not derived: use_cond %s)" % uc["status"])
    order["per_consumer"] = per_consumer(st, use_events, publish_events, ca)
    rec["order"] = order
    # ---- axis 4 intervention
    rec["intervention"] = {"plan_origin": "generated", "plan_inputs": sc["inputs"], "validity": "no hold point in the plan",
                           "hold": "none", "matched_use": None, "side_effects": None, "other_causes": None,
                           "repeats": ctx["run_budget"].get("boots") if run else 0, "control_level": "none (natural boot)",
                           "status": "unconfirmed",
                           "missing": ["intervention_out_of_scope(no hold, gate or directed run in this stage)"]}
    # ---- axis 5 effect
    rec["effect"] = effect_axis(ctx, st, cview, use_events, hc_sites)
    # ---- tool verdict (JR 3.3) from the axis statuses only
    rec["tool_verdict"] = verdict(rec)
    return rec


def evaluate_p(st, use_events, publish_events):
    """P evaluated from read values only (keys, out ids, field values, times), never from return grades."""
    if st["path"] == "A":
        u = use_events[0]
        if "at_call" not in u:
            return "record_insufficient", "record_insufficient(value)"
        ut = u["at_call"]["t_ms"]
        same = [p for p in publish_events if p.get("_k", {}).get("text") == u["_k"]["text"] and p.get("after_call")]
        if any(p["at_call"]["t_ms"] > ut for p in same if "at_call" in p) and not any(p["after_call"]["t_ms"] < ut for p in same):
            return "unmet_state_observed", None
        if u.get("out", {}).get("read") and any(p["after_call"]["t_ms"] < ut and p.get("out", {}).get("value") == u["out"]["value"] for p in same):
            return "met_state_only", None
        return "record_insufficient", "record_insufficient(the state at the use is not decided by the values read)"
    vals = [u.get("_fv") for u in use_events]
    if any(v is None for v in vals) or st["init"] is None:
        return "record_insufficient", "record_insufficient(value)"
    if any(v == st["init"] for v in vals):
        return "unmet_state_observed", None
    return "met_state_only", None


def per_consumer(st, use_events, publish_events, ca):
    """JR R10 (a): items 1-4 for this consumer only (JR 3.6 item 8)."""
    cells = {"item1_mhb_use_before_w": {"value": "incomplete", "why": "order edges not computed in this cycle (fixed_proof_open)"}}
    linked = [p for p in publish_events if st["path"] == "A" and use_events and p.get("_k", {}).get("text") is not None
              and p["_k"]["text"] == use_events[0]["_k"].get("text") and "at_call" in p]
    if not use_events:
        for k in ("item2_uses_before_w_all_fail", "item3_after_w_no_such_status", "item4_a_failing_use_stands"):
            cells[k] = {"value": "no_data", "why": "no use of this consumer in the record", "events": []}
        return cells
    if not linked:
        for k in ("item2_uses_before_w_all_fail", "item3_after_w_no_such_status"):
            cells[k] = {"value": "unchecked", "why": "no publish event linked to the use by its key", "events": []}
        cells["item4_a_failing_use_stands"] = {"value": "unchecked", "why": "no initialization-window end or instance key in this cycle", "events": []}
        return cells
    w = min(p["at_call"]["t_ms"] for p in linked)
    before = [u for u in use_events if "at_call" in u and u["at_call"]["t_ms"] < w]
    after = [u for u in use_events if "at_call" in u and u["at_call"]["t_ms"] > w]

    def ref(u):
        return {"actor": u["at_call"]["actor"], "aseq": u["at_call"]["aseq"]}

    def val(u):
        try:
            return int(u.get("status_value"), 16) if u.get("status_value") and not u["status_value"].startswith("-") else -int(u["status_value"][3:], 16)
        except (TypeError, ValueError):
            return None
    if before:
        allf = all(fail_class(val(u)) for u in before)
        cells["item2_uses_before_w_all_fail"] = {"value": "holds" if allf else "fails", "events": [ref(u) for u in before],
                                                 "grade_rule": FAIL_CLASS_RULE}
    else:
        cells["item2_uses_before_w_all_fail"] = {"value": "no_data", "events": [], "why": "no use of this consumer before w"}
    if after:
        bad = {val(u) for u in before}
        cells["item3_after_w_no_such_status"] = {"value": "fails" if any(val(u) in bad for u in after) else "holds",
                                                 "events": [ref(u) for u in after]}
    else:
        cells["item3_after_w_no_such_status"] = {"value": "no_data", "events": [], "why": "no_matching_use_after_w"}
    cells["item4_a_failing_use_stands"] = {"value": "unchecked", "events": [], "why": "no initialization-window end or instance key in this cycle"}
    cells["w"] = {"t_ms": w, "events": [{"actor": p["at_call"]["actor"], "aseq": p["at_call"]["aseq"]} for p in linked]}
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
                     "stop_events": ctx["stop_lines"], "consumer_app_stop": ctx["stop_eval"].get(cview.get("app", {}).get("app"))}
    eff["end_reason"] = {"process": ctx["process_end"]}
    ca = cview.get("run_actor")
    evs = []
    after_t = None
    if use_events:
        last = use_events[-1]
        after_t = (last.get("after_call") or last.get("at_call"))["aseq"]
    exit_fns = {r.split()[1] for r in ctx["stop_lines"] if r.startswith("app-exit ") and len(r.split()) > 1}
    loops = []
    if ca is not None and after_t is not None:
        for e in run.by_actor.get(ca, []):
            if e.aseq <= after_t:
                continue
            s = run.sites[e.site]
            if e.site in hc_sites or ("stop-event" in s.rules and s.entry_of in exit_fns):
                evs.append(evref(run, e, {"ref": s.ref[:200]}))
            elif "stop-event" in s.rules:
                loops.append(e)
    # a run-loop entry after the use is a window fact (the actor reached its stop event), not an effect of the result
    eff["window"]["consumer_run_loop_after_use"] = (evref(run, loops[0]) if loops else None)
    eff["window"]["consumer_run_loop_entries_after_use"] = len(loops)
    eff["events"] = evs
    eff["events_scope"] = ("events of the consumer actor after its last use of the row at the row's handling-chain sites and "
                           "the app-exit sites; their order in the actor is recorded, no causal link to the use is claimed")
    exit_ev = [x for x in evs if "stop-event" in x["rule"]]
    later_use = [u for u in use_events[1:]] if len(use_events) > 1 else []
    if later_use:
        okv = [u for u in later_use if u.get("out", {}).get("read")]
        eff["recovery"] = "completed" if okv else "attempted_not_completed"
    elif exit_ev:
        eff["recovery"] = "not_attempted"
        eff["recovery_scope"] = "in the window, at the planned sites: no later use of the row by this actor; its app-exit entry is recorded"
    else:
        eff["recovery"] = "not_observed"
    eff["end_reason"]["actor"] = ("app exit (app-exit entry recorded at aseq %d)" % exit_ev[-1]["aseq"]) if exit_ev else \
        "the process end (run budget)"
    eff["unobserved"] = ["after the process end (RUN t_stop)", "operations at no planned site",
                         "the effects of a result that the plan does not record (sidecar need lines not-planned)"]
    if any("SIGKILL" in x for x in ctx["process_end"]):
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
    v = {"inputs": ["link.status", "use_cond.status", "order.status", "order.static", "intervention.status",
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
                fails.append({"actor": u["after_call"]["actor"], "aseq": u["after_call"]["aseq"], "status": sv,
                              "name": u.get("status_name")})
    if I == "differs" and L == "established":
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
    fe = []
    roles = {}
    for r in ((E.get("window") or {}).get("stop_events") or []):
        p = r.split()
        if len(p) > 1:
            roles[p[1]] = p[0]
    for x in E.get("events", []):
        if "stop-event" in x["rule"]:
            fn = x.get("entry_of")
            role = roles.get(fn, "stop")
            if role == "app-exit":
                fe.append("app_exit_called(%s param0=%s)" % (fn, x["values"].get("param0")))
            elif role == "run-loop":
                fe.append("run_loop_entered(%s)" % fn)
            else:
                fe.append("stop_event(%s)" % fn)
    if E.get("recovery") == "completed":
        fe.append("recovered_in_window")
    elif E.get("recovery") in ("not_attempted", "attempted_not_completed"):
        fe.append("no_recovery_in_window")
    if E.get("status") == "outside_window_unknown":
        fe.append("outside_window_unknown")
    if not fe:
        fe.append("not_observed")
    w = E.get("window")
    v["final_effect"] = {"facts": fe, "window": {"start": w["start"], "end": w["end"]} if w else None,
                         "end_reason": E.get("end_reason")}
    if val == "observed":
        head = "사용 조건 위반(관측)"
    elif odf["value"] == "shown":
        head = "순서 의존 실패"
    elif odf["value"] == "failure_sign":
        head = "실패 징후"
    elif val == "candidate":
        head = "정적 후보"
    elif O == "static_only":
        head = "순서 의존 후보"
    else:
        head = "미확정"
    v["headline"] = head
    return v


# ----------------------------------------------------------------------------------------------- JR 3.6 checks
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
    return out


def check_record(rec, vocab):
    bad = []
    tv = rec.get("tool_verdict", {})
    vio = tv.get("violation", {}).get("value")
    uc = rec.get("use_cond", {})
    if vio == "observed":
        if uc.get("status") != "derived" or not all(uc.get(k, {}).get("sources") for k in ("callee_check", "reach", "caller_dependence")):
            bad.append("JR 3.6 item 1: violation observed without a derived use condition whose callee_check, reach and caller_dependence carry source lines")
        ue = rec.get("order", {}).get("use_events", [])
        has_val = any((u.get("key") or {}).get("text") is not None or u.get("field_value") is not None or
                      (u.get("out") or {}).get("read") for u in ue)
        if not has_val:
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
    for r in all_reasons(rec):
        base = r.split("(", 1)[0]
        if base not in vocab:
            bad.append("JR 3.5: reason %r is not in the vocabulary" % r)
    return bad


# ----------------------------------------------------------------------------------------------- main
PREMISES = [
    {"id": "P-ARG", "text": "the start routine receives the creation argument unchanged (POSIX pthread_create); collated only "
                            "against the local glibc 2.39 copy; the build links the distribution's C library (results "
                            "document 1.7, c_library.txt), whose patches to that function are not established",
     "facts": None},
    {"id": "P-ACTOR", "text": "the runtime's actor ids are per thread (actor key tid:<n>; RUNX evidence line)"},
    {"id": "P-STACK", "text": "within one actor events are in program order (aseq) and calls are synchronous: an event between the "
                              "at-call and the after-call of one call in the same actor belongs to that call"},
    {"id": "P-CLOCK", "text": "event times of all actors are read from one clock (RUN clock id; the POSIX adapter reads "
                              "CLOCK_MONOTONIC, kairos/runtime/src/kz_rt_posix.c:43-44)"},
    {"id": "P-STR", "text": "a key text is NUL-terminated within its bound or the cut is flagged (manifest value refs)"},
    {"id": "P-CONST", "text": "the return constants the row gives for the use (its flip or callee-check line) are the status "
                              "constants of the use's callee"},
    {"id": "L-ABS", "text": "path A, absence outcome: a use that found no object and a create of the same key in the row's "
                            "namespace partition, in one process of one run, are taken as the same named resource; the "
                            "generation the use saw is none (JR 3.2 axis 1 resource key: object kind, namespace, name, "
                            "generation; JR 4.2: the run value of the key and the same-named object)"},
]


def main(argv):
    ap = argparse.ArgumentParser()
    for a in ("sidecar", "manifest", "bindings", "relations", "keys", "vocabulary", "facts-view", "bprime-basis",
              "dump-text", "run-meta", "chain", "stop-eval", "startup-script", "build-label", "out", "check", "rows"):
        ap.add_argument("--" + a)
    o = ap.parse_args(argv)
    if o.check:
        vocab = set(x.strip() for x in read_input("vocabulary", o.vocabulary).splitlines() if x.strip() and not x.startswith("#"))
        rec = json.loads(read_input("record", o.check))
        bad = check_record(rec, vocab)
        for b in bad:
            print("refused: " + b)
        print("check %s (%d failures)" % ("ok" if not bad else "FAILED", len(bad)))
        return 4 if bad else 0
    need = ("sidecar", "manifest", "bindings", "relations", "keys", "vocabulary", "facts_view", "bprime_basis", "build_label", "out")
    for n in need:
        if getattr(o, n) is None:
            print("explain.py: --%s is required" % n.replace("_", "-"), file=sys.stderr)
            return 64
    if os.path.exists(o.out):
        print("explain.py: %s exists (new directories only)" % o.out, file=sys.stderr)
        return 64
    vocab = set(x.strip() for x in read_input("vocabulary", o.vocabulary).splitlines() if x.strip() and not x.startswith("#"))
    sc = parse_sidecar(read_input("sidecar", o.sidecar))
    mtext = read_input("manifest", o.manifest)
    sites = parse_manifest(mtext)
    bound, unbound_other = parse_bindings(read_input("bindings", o.bindings))
    rel_head, rels = parse_relations(read_input("relations", o.relations))
    key_head, keys = parse_keys(read_input("keys", o.keys))
    fv = read_input("facts_view", o.facts_view)
    read_input("bprime_basis", o.bprime_basis)
    rel_name = os.path.basename(o.relations)
    # combination checks (refused combinations become build_mismatch records)
    mism = []
    m = re.search(r"sidecar sha256=([0-9a-f]{64})", mtext)
    if not m or m.group(1) != sha_of("sidecar"):
        mism.append("the manifest names another sidecar")
    facts_in = [x for x in sc["inputs"] if x.startswith("facts ")]
    facts_sha = re.search(r"sha256=([0-9a-f]{64})", facts_in[0]).group(1) if facts_in else None
    if rel_head.get("facts") != facts_sha:
        mism.append("the relations come from facts %s, the plan from %s" % (rel_head.get("facts"), facts_sha))
    if key_head.get("facts") != facts_sha:
        mism.append("the keys come from other facts")
    if re.search(r"sha256=([0-9a-f]{64})", fv) is None or facts_sha not in fv:
        mism.append("the facts view does not name the plan's facts")
    run = None
    dump = None
    exes = {}
    process_end = []
    compat = "no run"
    stop_eval = {}
    apps = []
    if o.dump_text:
        for n in ("run_meta", "chain", "startup_script"):
            if getattr(o, n) is None:
                print("explain.py: --%s is required with --dump-text" % n.replace("_", "-"), file=sys.stderr)
                return 64
        dump = parse_dump(read_input("dump_text", o.dump_text))
        meta = read_input("run_meta", o.run_meta)
        chain = read_input("chain", o.chain)
        scr = read_input("startup_script", o.startup_script)
        if o.stop_eval:
            for ln in read_input("stop_eval", o.stop_eval).splitlines():
                p = ln.split("\t")
                if len(p) >= 12 and p[0] not in ("app", "summary") and not p[0].startswith("#"):
                    stop_eval[p[0]] = {"actor": p[4], "stop_event": p[6], "stop_ms": p[9], "inside_window": p[10], "status": p[11]}
        for ln in scr.splitlines():
            if ln.startswith("!"):
                break
            f = [x.strip() for x in ln.split(",")]
            if len(f) >= 4 and f[0] == "CFE_APP":
                apps.append((f[2], f[3]))
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
        mm = re.search(r"^dump \S+ bytes=\d+ sha256=([0-9a-f]{64})", meta, re.M)
        cv = re.search(r"^chain\t(\S+)", chain, re.M)
        refused = [ln.split("\t")[0] for ln in chain.splitlines() if "\trefused" in ln]
        if refused:
            mism.append("chain links refused: " + ", ".join(refused))
        sb = re.search(r"^same_build\t([^\t\n]+)", chain, re.M)
        bp = re.search(r"^bprime\t\S+\tbasis=([0-9a-f]+)", chain, re.M)
        if bp and not sha_of("bprime_basis").startswith(bp.group(1)):
            mism.append("the B' basis given is not the chain's")
        compat = "chain %s; same_build %s" % (cv.group(1) if cv else "absent", sb.group(1) if sb else "absent")
        run = Run(dump, sites)
    m_tool = re.search(r"tool_sha256=([0-9a-f]{64})", fv)
    m_bid = re.search(r'name="extract\.build_identity" value="([^"]*)"', fv)
    m_p0 = re.search(r'name="extract\.p0_table" value="([^"]*)"', fv)
    m_pc = re.search(r'name="p0\.pthread_create" value="([^"]*)"', fv)
    extractor = {"facts_tool_sha256": m_tool.group(1) if m_tool else None, "build_identity": m_bid.group(1) if m_bid else None,
                 "source": "facts view %s" % os.path.basename(o.facts_view)}
    fw = [x for x in sc["inputs"] if x.startswith("framework-layer-data ")]
    semantics = {"value": "p0-only", "p0_table": m_p0.group(1) if m_p0 else None,
                 "framework_layer_data": re.search(r"sha256=([0-9a-f]{64})", fw[0]).group(1) if fw else None,
                 "scope": "the P0 table of the facts; framework-layer data for the run-loop and app-exit roles only",
                 "missing": ["absent_result_unevaluated(no common-semantics table of the framework APIs in this cycle)"]}
    premises = [dict(p) for p in PREMISES]
    premises[0]["facts"] = m_pc.group(1) if m_pc else None
    loss_any = False
    if dump is not None:
        rx = dump["runx"]
        loss_any = bool(dump["loss"]) or any(int(rx.get(k, "0") or 0) for k in ("pool_dropped", "boot_bytes_dropped")) or \
            any(int(a.get("overflow", "0") or 0) or int(a.get("partial", "0") or 0) or int(a.get("dropped_bytes", "0") or 0)
                for a in dump["actors"].values()) or int(dump["format"].get("partial", "0") or 0) > 0
    ctx = {"sc": sc, "sites": sites, "run": run, "bound": bound, "keys": keys, "rel_name": rel_name, "rel_head": rel_head,
           "label": o.build_label, "exes": exes, "facts_sha": facts_sha, "compat": compat, "extractor": extractor,
           "semantics": semantics, "premises": premises, "loss_any": loss_any, "apps": apps, "stop_eval": stop_eval,
           "stop_lines": sc["stop"], "process_end": process_end, "mismatch": bool(mism),
           "run_budget": {"boots": 1 if run else 0, "reason": "one record suffices for the explanation test; repetition is a later cost study",
                          "sensitivity": "not measured"}, "static": {}}
    if mism:
        ctx["compat"] = "refused: " + "; ".join(mism)
    planned = [k for k, r in sc["rows"].items() if r.get("state") == "planned"]
    if o.rows:
        planned = [k for k in planned if k in o.rows.split(",")]
    os.makedirs(o.out)
    os.makedirs(os.path.join(o.out, "records"))
    index, refused_all = [], []
    for k in sorted(planned, key=lambda x: int(sc["rows"][x].get("position", "0"))):
        rid = sc["rows"][k]["id"]
        if rid not in rels or keys.get(rid, (None,))[0] != k:
            refused_all.append("row %s %s: not in the relations or keys under this use-site key" % (k, rid))
            continue
        ctx["static"][k] = row_static(rels[rid], rel_name)
        st = ctx["static"][k]
        consumers = []
        if run is not None and not mism:
            use_sites = {n for n in sc["maps"].get(k, []) if "use-call" in sites[n].rules}
            consumers = sorted({e.actor for e in run.events_at(use_sites)}, key=lambda x: int(x) if x.isdigit() else 0)
            tied_roots = {run.root[a]["root"] for a in consumers if run.root.get(a, {}).get("status") == "determined"}
            for root, kind in st["static_consumers"]:
                if root in tied_roots:
                    continue
                cand = [a for a in run.by_actor if run.root.get(a, {}).get("root") == root]
                consumers.extend(cand if cand else [("static", root, kind)])
        else:
            consumers = [("static", r, kd) for r, kd in st["static_consumers"]] or [("static", None, None)]
        if not consumers:
            consumers = [("static", None, None)]
        for c in consumers:
            rec = explain_row(ctx, k, c)
            rec["inputs"] = {n: {"path": p, "sha256": h} for n, (p, h) in sorted(INPUTS.items())}
            bad = check_record(rec, vocab)
            cname = c if not isinstance(c, tuple) else "static-%s" % (c[1] or "none")
            if bad:
                refused_all.extend("%s %s consumer %s: %s" % (k, rid, cname, b) for b in bad)
                continue
            fn = "%s.%s.%s.actor-%s.json" % (o.build_label, rid, k, cname)
            with open(os.path.join(o.out, "records", fn), "w") as f:
                json.dump(rec, f, indent=1, ensure_ascii=False)
                f.write("\n")
            tv = rec["tool_verdict"]
            index.append("\t".join([o.build_label, rid, k, str(cname), rec["link"]["status"], rec["use_cond"]["status"],
                                    rec["order"]["status"], rec["intervention"]["status"], rec["effect"]["status"],
                                    tv["violation"]["value"], tv["odf"]["value"], tv["headline"],
                                    ";".join(tv["violation"]["undetermined_reasons"]), fn]))
    with open(os.path.join(o.out, "index.tsv"), "w") as f:
        f.write("build\trow\tuse_site_key\tconsumer\tlink\tuse_cond\torder\tintervention\teffect\tviolation\todf\theadline\tundetermined_reasons\tfile\n")
        for ln in index:
            f.write(ln + "\n")
    with open(os.path.join(o.out, "inputs.txt"), "w") as f:
        f.write("# %s\n" % SCRIPT_REV)
        for n, (p, h) in sorted(INPUTS.items()):
            f.write("%s\t%s\t%s\n" % (n, h, p))
        f.write("combination\t%s\n" % ctx["compat"])
    with open(os.path.join(o.out, "refused.txt"), "w") as f:
        for ln in refused_all:
            f.write(ln + "\n")
    print("records %d refused %d compat %s" % (len(index), len(refused_all), ctx["compat"]))
    if refused_all:
        return 4
    return 3 if mism else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
