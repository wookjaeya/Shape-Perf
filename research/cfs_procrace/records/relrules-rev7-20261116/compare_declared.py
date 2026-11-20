#!/usr/bin/env python3
"""compare_declared.py -- relations rules revision 6 -> 7 on one build: the declared classes of difference, matched by
use-site key.  Run with python3 -I.

  compare_declared.py OLD_REL OLD_KEYS NEW_REL NEW_KEYS TABLE_OUT
  compare_declared.py --golden OLD_GOLDEN NEW_GOLDEN

The golden mode: the part before "# relations" must be byte-identical, and the relations part (the golden leaves out the
facts line) is compared by the same classes with the row id standing for the key (the fixtures lose no row).

OLD_*: the outputs of the tool before the build (rules revision 6), NEW_*: after it.  Writes the before/after table of the
path A rows (one line per key of the old or the new text) to TABLE_OUT and a report to stdout; rc 0 when every difference
is of a declared class, 1 otherwise (each undeclared difference is printed with "UNDECLARED").

Declared classes (the declaration of the build, records/relrules-rev7-20261116 and plan/full_stage_results_20261116.md 2.0):
  header: the rules line 6 -> 7; the "summary A" line and the "summary incomplete" line may differ; one new line
          "summary A membership and creators (relations rules revision 7): ..." right after "summary A"; nothing else.
  [A] heading: only the row count may differ.  [B], [C] headings: identical.
  namespace blocks: identical except the "uses:" line (its counts, and the new tail "; left out, another callback ...")
          and new "left out:" lines.
  rows: every B and C key is in both texts; an A key may be missing from the new text only when the new text has a
          "left out:" line for the same call, caller and callee#parameter in the block of the same namespace; no new key.
  B and C rows: identical except one new "writers:" line, and in the incomplete line the new reason
          writer_set_incomplete(zero writers under the rules); the layers column may gain basis-deviation items (files the
          new lines name) and then the incomplete line reading_options_differ(basis deviation of a unit of the row).
  A rows (kept): the head may differ in the row number and the partition only; new lines membership:, aliases:,
          id holder:, callee-check:, callee-check (returns not followed):, writers:; the partition: line may change;
          key:, actors:, handling:, run-order:, violation: identical; creator:/not a pair: lines may drop out (a creator
          left out or of another partition) or come in (a creator now of the same partition), and a creator line may end in
          "; namespace membership unresolved: ..."; when the set of creators is the same, dependency: is the old text or the
          old text followed by ": <sub-reasons>", and feasibility:, flip: are identical; when it differs, those three
          follow the new set (named "creator set changed"); layers: as for B and C, and when the creator set changed its
          P0-unverified(...) items may follow the actors of the new set; incomplete: may gain
          writer_set_incomplete(zero writers under the rules), writer_set_incomplete(key writes reachable through
          unresolved indirect calls), link_unlinked(namespace membership unresolved), reading_options_differ(...), and,
          when the creator set changed, gain or lose key_unresolved(creator key), truncated(static image),
          fixed_proof_open(no static order between the actors).
"""
import os
import re
import sys

ROW = re.compile(r"^  ([ABC])(\d+) (.*)$")
NEW_A_LINES = ("membership:", "aliases:", "id holder:", "callee-check:", "callee-check (returns not followed):", "writers:")
SAME_A_LINES = ("key:", "actors:", "handling:", "run-order:", "violation:")
ADD_INC = {"writer_set_incomplete(zero writers under the rules)", "writer_set_incomplete(key writes reachable through unresolved indirect calls)",
           "link_unlinked(namespace membership unresolved)", "reading_options_differ(basis deviation of a unit of the row)"}
PAIR_INC = {"key_unresolved(creator key)", "truncated(static image)", "fixed_proof_open(no static order between the actors)"}


def parse(path):
    lines = open(path, encoding="utf-8", errors="replace").read().split("\n")
    if lines and lines[-1] == "":
        lines.pop()
    header, groups, rows, cur, ns = [], [], {}, None, None
    blocks = {}
    i = 0
    while i < len(lines) and not lines[i].startswith("[A] "):
        header.append(lines[i])
        i += 1
    for l in lines[i:]:
        m = ROW.match(l)
        if l.startswith("[") and not l.startswith("  "):
            groups.append(l)
            cur = None
            ns = None
            continue
        if l.startswith("  namespace "):
            ns = l.split()[1]
            blocks[ns] = [l]
            cur = None
            continue
        if m:
            cur = {"path": m.group(1), "num": int(m.group(2)), "head": m.group(3), "lines": [], "ns": ns}
            rows[m.group(1) + m.group(2)] = cur
            continue
        if cur is not None:
            cur["lines"].append(l)
        elif ns is not None:
            blocks[ns].append(l)
        else:
            groups.append(l)
    return header, groups, blocks, rows


def keys(path):
    out = {}
    for l in open(path, encoding="utf-8"):
        m = re.match(r"^([ABC]\d+) ([0-9a-f]{16}) (.*)$", l.rstrip("\n"))
        if m:
            out[m.group(1)] = (m.group(2), m.group(3))
    return out


def label(l):
    s = l.strip()
    for p in ("callee-check (returns not followed):", "callee-check (called with the parameter):", "write after the spawn by the spawn operation",
              "write after the spawn in a callee", "calls after the spawn whose targets are not resolved"):
        if s.startswith(p):
            return p
    m = re.match(r"^([a-z][a-z -]*?):", s)
    if m:
        return m.group(1) + ":"
    return s.split(" ")[0]


def col(r, name):
    for l in r["lines"]:
        if l.startswith("      " + name + ": "):
            return l[len("      " + name + ": "):]
    return None


def lines_of(r, name):
    return [l for l in r["lines"] if l.startswith("      " + name)]


def creator_set(r):
    out = set()
    for l in r["lines"]:
        s = l.strip()
        if s.startswith("creator: "):
            m = re.search(r": (\S+ in \S+ -> \S+)", s)
            out.add(("creator", m.group(1) if m else s))
        elif s.startswith("not a pair: "):
            m = re.search(r": (\S+ in \S+ -> \S+) key ", s)
            out.add(("not a pair", m.group(1) if m else s))
    return out


def norm_creator(l):
    return re.sub(r"; namespace membership unresolved: .*$", "", l)


def main():
    if sys.argv[1] == "--golden":
        return main_golden(sys.argv[2], sys.argv[3])
    old_rel, old_keys, new_rel, new_keys, table_out = sys.argv[1:6]
    return compare(old_rel, keys(old_keys), new_rel, keys(new_keys), table_out)


def main_golden(old_g, new_g):
    import tempfile
    o = open(old_g, encoding="utf-8").read()
    n = open(new_g, encoding="utf-8").read()
    sep = "\n# relations\n"
    if sep not in o or sep not in n:
        print("UNDECLARED: no relations part")
        return 1
    if o.split(sep)[0] != n.split(sep)[0]:
        print("UNDECLARED: the part before the relations differs")
        return 1
    with tempfile.TemporaryDirectory() as d:
        paths = []
        for name, text in (("old", o), ("new", n)):
            rel = text.split(sep, 1)[1]
            open(os.path.join(d, name + ".rel"), "w", encoding="utf-8").write(rel)
            paths.append(os.path.join(d, name + ".rel"))
        ids_o = {m.group(1) + m.group(2): (m.group(1) + m.group(2), "") for m in (ROW.match(l) for l in open(paths[0], encoding="utf-8")) if m}
        ids_n = {m.group(1) + m.group(2): (m.group(1) + m.group(2), "") for m in (ROW.match(l) for l in open(paths[1], encoding="utf-8")) if m}
        return compare(paths[0], ids_o, paths[1], ids_n, os.path.join(d, "table.tsv"))


def compare(old_rel, ok, new_rel, nk, table_out):
    oh, og, ob, orows = parse(old_rel)
    nh, ng, nb, nrows = parse(new_rel)
    bad = []
    note = []
    # header
    oh2 = [l.replace("relations rules revision 6 (2026-11-16)", "relations rules revision 7 (2026-11-16)") for l in oh]
    nh2 = [l for l in nh if not l.startswith("summary A membership and creators (relations rules revision 7): ")]
    if len([l for l in nh if l.startswith("summary A membership and creators (relations rules revision 7): ")]) != 1:
        bad.append("header: the new summary line is not there once")
    if len(oh2) != len(nh2):
        bad.append("header: %d lines before, %d after (without the new line)" % (len(oh2), len(nh2)))
    else:
        for a, b in zip(oh2, nh2):
            if a != b and not (a.startswith("summary A ") and b.startswith("summary A ")) and not (a.startswith("summary incomplete (") and b.startswith("summary incomplete (")):
                bad.append("header line: %r -> %r" % (a[:120], b[:120]))
    # headings
    def heads(g):
        return [l for l in g if l.startswith("[")]
    for a, b in zip(heads(og), heads(ng)):
        if a.startswith("[A] ") and b.startswith("[A] "):
            if re.sub(r"^\[A\] \d+ ", "", a) != re.sub(r"^\[A\] \d+ ", "", b):
                bad.append("[A] heading: %r -> %r" % (a, b))
        elif a != b:
            bad.append("heading: %r -> %r" % (a[:120], b[:120]))
    # namespace blocks
    left = {}
    for ns in sorted(set(ob) | set(nb)):
        if ns not in ob or ns not in nb:
            bad.append("namespace block only on one side: " + ns)
            continue
        a = [l for l in ob[ns] if not l.startswith("      uses: ")]
        b = [l for l in nb[ns] if not l.startswith("      uses: ") and not l.startswith("      left out: ")]
        if a != b:
            bad.append("namespace block %s: lines other than uses:/left out: differ" % ns)
        for l in nb[ns]:
            m = re.match(r"^      left out: (\S+) in (\S+) -> (\S+) \((lookup|creator)\): (.*)$", l)
            if m:
                left.setdefault(ns, []).append(m.groups())
    # rows by key
    okey = {v[0]: i for i, v in ok.items()}
    nkey = {v[0]: i for i, v in nk.items()}
    table = ["# key\tpath\told id\tnew id\tnamespace\told partition\tnew partition\told creators\tnew creators\told dependency (class)\tnew dependency (class)\told flip\tnew flip\tchange"]
    counts = {"removed": 0, "kept": 0, "creator set changed": 0, "partition changed": 0, "B/C kept": 0}
    for k in sorted(set(okey) | set(nkey)):
        oi, ni = okey.get(k), nkey.get(k)
        if oi is None:
            bad.append("new key %s (%s) not in the old text" % (k, ni))
            continue
        orow = orows.get(oi)
        if ni is None:
            if orow["path"] != "A":
                bad.append("%s %s removed (path %s)" % (k, oi, orow["path"]))
                continue
            m = re.match(r"^lookup (\S+) in (\S+) -> (\S+) namespace (\S+) ", orow["head"])
            hit = m and any(x[0] == m.group(1) and x[1] == m.group(2) and x[2] == m.group(3) for x in left.get(m.group(4), []))
            if not hit:
                bad.append("%s %s removed without a left out: line in its namespace" % (k, oi))
            counts["removed"] += 1
            table.append("\t".join([k, "A", oi, "-", orow["ns"] or "?", re.search(r"partition \{([^}]*)\}", orow["head"]).group(1), "-",
                                    str(len([l for l in orow["lines"] if l.strip().startswith("creator: ")])), "-", (col(orow, "dependency") or "")[:60], "-",
                                    (col(orow, "flip") or "").split(":")[0], "-", "removed (left out: another callback)"]))
            continue
        nrow = nrows.get(ni)
        if ok[oi][1] != nk[ni][1]:
            bad.append("%s key text differs" % k)
        if orow["path"] in "BC":
            counts["B/C kept"] += 1
            if orow["head"] != nrow["head"]:
                bad.append("%s %s head differs" % (k, oi))
            a = [l for l in orow["lines"] if not l.startswith("      layers: ") and not l.startswith("      incomplete: ")]
            b = [l for l in nrow["lines"] if not l.startswith("      writers: ") and not l.startswith("      layers: ") and not l.startswith("      incomplete: ")]
            if a != b:
                bad.append("%s %s lines other than writers:/layers:/incomplete: differ" % (k, oi))
            if len(lines_of(nrow, "writers: ")) != 1:
                bad.append("%s %s has %d writers: lines" % (k, oi, len(lines_of(nrow, "writers: "))))
            check_layers_inc(k, oi, orow, nrow, set(), bad, note, False)
            continue
        counts["kept"] += 1
        oh_ = re.sub(r"partition \{[^}]*\}", "partition {}", orow["head"])
        nh_ = re.sub(r"partition \{[^}]*\}", "partition {}", nrow["head"])
        if oh_ != nh_:
            bad.append("%s %s head differs beyond the partition" % (k, oi))
        op = re.search(r"partition \{([^}]*)\}", orow["head"]).group(1)
        np_ = re.search(r"partition \{([^}]*)\}", nrow["head"]).group(1)
        if op != np_:
            counts["partition changed"] += 1
        for name in SAME_A_LINES:
            if lines_of(orow, name) != lines_of(nrow, name):
                bad.append("%s %s %s lines differ" % (k, oi, name))
        oc, nc = creator_set(orow), creator_set(nrow)
        changed = oc != nc
        if changed:
            counts["creator set changed"] += 1
        # the creator lines: the same set gives the same lines, except the unresolved-membership marker
        if not changed:
            a = [l for l in orow["lines"] if l.strip().startswith(("creator: ", "not a pair: "))]
            b = [norm_creator(l) for l in nrow["lines"] if l.strip().startswith(("creator: ", "not a pair: "))]
            if a != b:
                bad.append("%s %s creator/not a pair lines differ with the same set" % (k, oi))
            od, nd = col(orow, "dependency"), col(nrow, "dependency")
            if not (od == nd or nd.startswith(od + ": ")):
                bad.append("%s %s dependency: %r -> %r" % (k, oi, od[:80], nd[:80]))
            for name in ("feasibility", "flip"):
                if col(orow, name) != col(nrow, name):
                    bad.append("%s %s %s differs with the same creator set" % (k, oi, name))
        # the other lines: those of the old row are all kept, the new ones are of the new classes
        olabels = [label(l) for l in orow["lines"]]
        for l in nrow["lines"]:
            lb = label(l)
            if lb in NEW_A_LINES or lb in ("creator:", "not a pair:", "partition:"):
                continue
            if lb not in olabels:
                bad.append("%s %s new line of an undeclared class: %r" % (k, oi, l.strip()[:100]))
        check_layers_inc(k, oi, orow, nrow, PAIR_INC if changed else set(), bad, note, changed)
        table.append("\t".join([k, "A", oi, ni, nrow["ns"] or "?", op, np_, str(len([x for x in oc if x[0] == "creator"])), str(len([x for x in nc if x[0] == "creator"])),
                                (col(orow, "dependency") or "")[8:70], (col(nrow, "dependency") or "")[8:70], (col(orow, "flip") or "").split(":")[0],
                                (col(nrow, "flip") or "").split(":")[0], "kept" + ("; creator set changed" if changed else "") + ("; partition changed" if op != np_ else "")]))
    with open(table_out, "w", encoding="utf-8") as f:
        f.write("\n".join(table) + "\n")
    print("rows: A old %d new %d (kept %d, removed %d; partition changed %d, creator set changed %d); B and C kept %d" % (
        len([r for r in orows.values() if r["path"] == "A"]), len([r for r in nrows.values() if r["path"] == "A"]), counts["kept"], counts["removed"],
        counts["partition changed"], counts["creator set changed"], counts["B/C kept"]))
    print("left out lines: %d" % sum(len(v) for v in left.values()))
    for n in note:
        print("note: " + n)
    for b in bad:
        print("UNDECLARED: " + b)
    print("RESULT: %s" % ("all differences of declared classes" if not bad else "%d undeclared differences" % len(bad)))
    return 0 if not bad else 1


def check_layers_inc(k, oi, orow, nrow, extra, bad, note, pairs_changed):
    ol, nl = col(orow, "layers") or "", col(nrow, "layers") or ""
    layers_changed = ol != nl
    if layers_changed:
        # only basis-deviation(...) items may be added; P0-unverified(...) may change when the creator set changed
        od = re.search(r" basis-deviation\(([^)]*)\)", ol)
        nd = re.search(r" basis-deviation\(([^)]*)\)", nl)
        oset = set(od.group(1).split("; ")) if od else set()
        nset = set(nd.group(1).split("; ")) if nd else set()
        strip = lambda t: re.sub(r" P0-unverified\([^)]*\)", "", re.sub(r" basis-deviation\([^)]*\)", "", t)) if pairs_changed else re.sub(r" basis-deviation\([^)]*\)", "", t)
        if strip(ol) != strip(nl) or not oset <= nset:
            bad.append("%s %s layers differ beyond the declared items" % (k, oi))
        else:
            if nset - oset:
                note.append("%s %s layers gained basis-deviation items: %s" % (k, oi, "; ".join(sorted(nset - oset))))
            op = re.search(r" P0-unverified\(([^)]*)\)", ol)
            np_ = re.search(r" P0-unverified\(([^)]*)\)", nl)
            if (op.group(1) if op else "") != (np_.group(1) if np_ else ""):
                note.append("%s %s layers P0-unverified %s -> %s (creator set changed)" % (k, oi, op.group(1) if op else "none", np_.group(1) if np_ else "none"))
    oi_ = set((col(orow, "incomplete") or "").split("; "))
    ni_ = set((col(nrow, "incomplete") or "").split("; "))
    for x in ni_ - oi_:
        if x not in ADD_INC and x not in extra:
            bad.append("%s %s incomplete gained %s" % (k, oi, x))
        if x.startswith("reading_options_differ") and not layers_changed:
            bad.append("%s %s incomplete gained %s without a layers change" % (k, oi, x))
    for x in oi_ - ni_:
        if x not in extra:
            bad.append("%s %s incomplete lost %s" % (k, oi, x))


if __name__ == "__main__":
    sys.exit(main())
