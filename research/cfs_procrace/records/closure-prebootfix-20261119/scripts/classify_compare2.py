#!/usr/bin/env python3
"""classify_compare2.py -- classify_compare.py of results 26.4 (records/closure-preboot-20261119/scripts, kept unchanged)
with its T4 test fixed (results 26.5; review finding pre-builds-03 (c), records/closure-review-preboot-20261119): a
missing pair at a create position is put down to the record's uses only when the record has no absent use (pairs are
made for absent uses only), and to a missing create only when the record's order.publish_events hold no create at that
position; otherwise the line is unclassified.  The kind expectation_error is still never returned: like its first
version, this script tests the synthetic input, not the expectations (evaluation protocol 14.13 item 7).

Every disagreeing line of an expectation comparison against a SYNTHETIC record (results 26.4;
evaluation protocol 14.12 item 3) with its evidence from the record and its kind:
  judge_defect        the record's value is not what the rule gives on the record's own events (stop: fix the code)
  expectation_error   the expectation is not what the rule gives on the predicted boot (fix it under a new name)
  synthetic_input     the record is the rule's value on the synthetic input, which differs from the predicted boot by
                      a rule of the generator (records/closure-explain-fix-20261119/scripts/synth_record.py, header),
                      shown here by the record's own fields
  unclassified        none of the tests below fits (read by hand; never counted as synthetic_input)
Reads the comparison output (explain_expect.py), the explanation output directory and the order name of the synthetic
record; no row id, key or expected value is an input or a constant.  Run with python3 -I.

    classify_compare.py --compare FILE --records EXPLAIN_DIR --order script|reverse --uses USES.tsv --out FILE

Tests, in this order, on the record(s) of the line (branch, build, row key, root) and the expectation:
  T1 use[i].*        the record has i or fewer use events: the generator writes one call per use-call site pair of the
                     plan (a loop's later iterations are not written)
  T2 use[i].outcome  expected absent, found found: the record's use key text and the generator's key rule (a use's key
                     text is the first key text of the row's creator lines; a use is found when a create of the same
                     partition and text ended before it); the judge's state for a found use is checked: never unmet,
                     instance link unlinked (8.14 item 6 (b))
  T3 use[i].context  the row has two or more context lines in the use list and the record's context is another of them: the generator puts
                     the use in the first frame (script) or the last (reverse)
  T4 pair[...].*     found '-': no pair at that position because the record has no absent use (its uses are found; pairs
                     are made for absent uses only, JR 8.2), or because the record's order.publish_events hold no create
                     at that position; a record with an absent use and a create there is unclassified
  T5 pair[...].missing_items   every item in one set and not the other is explained by the record's own item grounds:
                     ma held with 'no lifetime end of the name in the interval' and the record has no lifetime end event
                     (the generator writes none); basis6 held with the create after the use (the order); basis5_key_texts
                     or na_bound missing with a create key text not recorded (the generator's creator key rule)
  T6 pair[...].result          no complete key text of a create at the position (the record's order.publish_events) differs
                     from an absent use's key text, so differs_from_use_key cannot come out (the generator gives the use the
                     first creator line's text and each create the first quoted string of its creator line)
"""
import argparse
import json
import os
import re
import sys


def recs(d, br, build, key, root):
    p = os.path.join(d, br, "records")
    out = []
    if not os.path.isdir(p):
        return out
    for fn in sorted(os.listdir(p)):
        if fn.startswith(build + ".") and (".%s." % key) in fn:
            r = json.load(open(os.path.join(p, fn)))
            rr = (r["consumer_actor"].get("root_tie") or {}).get("root") or r["consumer_actor"].get("static_root")
            if root in ("*", rr):
                out.append(r)
    return out


def texts(p):
    g = " ".join(str(x) for x in p["items"]["basis5_key_texts"].get("grounds", []))
    m = re.search(r"use text (None|'[^']*') cut \w+; create text (None|'[^']*')", g)
    return (m.group(1), m.group(2)) if m else (None, None)


def classify(field, exp, found, rs, order, ctxs):
    ev = []
    if not rs:
        return "unclassified", "no record"
    r = rs[0]
    ue = r["order"]["use_events"]
    m = re.match(r"use\[(\d+)\]\.(\w+)$", field)
    if m:
        i, f = int(m.group(1)), m.group(2)
        if len(ue) <= i:
            return "synthetic_input", "T1: the record has %d use event(s); the generator writes one call per use-call site pair" % len(ue)
        u = ue[i]
        if f == "outcome" and exp == "absent" and found == "found":
            ok = u.get("name_state", "").startswith("undecided") and u.get("instance_link", {}).get("status") == "unlinked"
            if not ok:
                return "judge_defect", "T2: a found use with state %s, instance link %s" % (u.get("name_state"), u.get("instance_link"))
            return "synthetic_input", ("T2: the use's key text %r is the generator's (the first key text of the row's creator lines); "
                                       "the use is found because a create of that text ended before it (order %s); name state %s, "
                                       "instance link unlinked" % ((u.get("key") or {}).get("text"), order, u.get("name_state")))
        if f == "context":
            if len(ctxs) < 2 or u.get("context") not in ctxs:
                return "unclassified", "T3: context %s, the row's context ids %s" % (u.get("context"), sorted(ctxs))
            return "synthetic_input", ("T3: the record's context is %s, one of the row's context frames %s; the generator puts the use "
                                       "in the %s frame (order %s)" % (u.get("context"), ",".join(sorted(ctxs)),
                                                                        "first" if order == "script" else "last", order))
        return "unclassified", "use field %s" % f
    m = re.match(r"pair\[(.+)\]\.(missing_items|result)$", field)
    if m:
        pos, f = m.group(1), m.group(2)
        ps = [p for p in r["link"].get("name_binding", {}).get("pairs", []) if p.get("create_pos") == pos]
        if found == "-":
            absent = [u for u in ue if u.get("outcome") == "absent"]
            fl = re.match(r"(?:.*/)?([^/]+):(\d+):\d+$", pos)
            at_pos = [e for e in r["order"].get("publish_events", []) if e.get("other_kind") != "writer"
                      and fl and ("%s:%s" % (fl.group(1), fl.group(2))) in ((e.get("at_call") or {}).get("where") or "")]
            if not absent and ue and all(u.get("outcome") == "found" for u in ue):
                return "synthetic_input", "T4: no pair at %s: the record has no absent use (its %d use(s) found; pairs are made for absent uses only); see T2" % (pos, len(ue))
            if not at_pos:
                return "synthetic_input", "T4: no create event at %s in the record's order.publish_events" % pos
            return "unclassified", "T4: no pair at %s although the record has %d absent use(s) and %d create event(s) there" % (pos, len(absent), len(at_pos))
        if f == "missing_items":
            E, F = set(exp.split(",")), set(found.split(","))
            notes = []
            for it in sorted(E ^ F):
                vs = [p["items"][it] for p in ps]
                gr = " ".join(str(g) for v in vs for g in v.get("grounds", []))
                st = {v.get("status") for v in vs}
                if it == "ma_no_lifetime_end" and it in E and st == {"held"} and "no lifetime end of the name in the interval" in gr \
                        and not r["events"]["lifetime_ends"]:
                    notes.append("ma held: the record has no lifetime end event (the generator writes none)")
                elif it == "basis6_create_after_use" and it in E and st == {"held"} and "after the use" in gr:
                    notes.append("basis6 held: every create at the position after the use (order %s)" % order)
                elif it in ("basis5_key_texts", "na_bound") and it in F and any("create text None" in " ".join(str(g) for g in p["items"]["basis5_key_texts"].get("grounds", [])) for p in ps):
                    notes.append("%s missing: a create key text not recorded (the generator's creator key rule)" % it)
                else:
                    return "unclassified", "item %s: states %s, grounds %s" % (it, sorted(st), gr[:200])
            return "synthetic_input", "T5: " + "; ".join(notes)
        if f == "result":
            # the key texts the record holds: the absent uses' and the creates' at this position (order.publish_events)
            fl = re.match(r"(?:.*/)?([^/]+):(\d+):\d+$", pos)
            ut = sorted({(u.get("key") or {}).get("text") or "None" for u in ue if u.get("outcome") == "absent"})
            ct = sorted({(e.get("key") or {}).get("text") or "None" for e in r["order"].get("publish_events", [])
                         if fl and ("%s:%s" % (fl.group(1), fl.group(2))) in ((e.get("at_call") or {}).get("where") or "")})
            diff = [(x, y) for x in ut for y in ct if x != "None" and y != "None" and x != y]
            if "differs_from_use_key" in exp.split(",") and not diff:
                return "synthetic_input", ("T6: no complete create key text at %s differs from the use's: use texts %s, create texts %s "
                                           "(the generator's key rules)" % (pos, ut, ct))
            return "unclassified", "result: use texts %s, create texts %s" % (ut, ct)
    return "unclassified", "field %s" % field


def main():
    ap = argparse.ArgumentParser()
    for x in ("compare", "records", "order", "uses", "out"):
        ap.add_argument("--" + x, required=True)
    a = ap.parse_args()
    ctx_of = {}
    for l in open(a.uses, encoding="utf-8"):
        r = l.rstrip("\n").split("\t")
        if len(r) >= 14 and r[0] not in ("columns",):
            ctx_of.setdefault(r[2], set()).add(r[11])
    if os.path.exists(a.out):
        print("classify_compare2.py: %s exists" % a.out, file=sys.stderr)
        return 64
    out, cnt = [], {}
    for l in open(a.compare, encoding="utf-8"):
        if l.startswith("#"):
            continue
        br, build, key, root, field, val, cls, res, found = l.rstrip("\n").split("\t")
        if res != "mismatch":
            continue
        kind, why = classify(field, val, found, recs(a.records, br, build, key, root), a.order,
                             {c for c in ctx_of.get(key, set()) if c != "-"})
        cnt[(cls, kind)] = cnt.get((cls, kind), 0) + 1
        out.append("\t".join([br, build, key, root, field, val, found, cls, kind, why]))
    with open(a.out, "w", encoding="utf-8") as f:
        f.write("# classify_compare2.py (results 26.5; protocol 14.12 item 3, 14.13 item 7): %s, records %s, order %s\n" % (a.compare.rsplit("/", 1)[-1], a.records, a.order))
        f.write("branch\tbuild\trow_key\troot\tfield\texpected\tfound\tclass\tkind\tevidence\n")
        f.write("\n".join(out) + ("\n" if out else ""))
        f.write("# counts: " + "; ".join("%s/%s %d" % (c, k, v) for (c, k), v in sorted(cnt.items())) + "\n")
    print("disagreeing lines %d: %s" % (len(out), "; ".join("%s/%s %d" % (c, k, v) for (c, k), v in sorted(cnt.items()))))
    return 0


if __name__ == "__main__":
    sys.exit(main())
