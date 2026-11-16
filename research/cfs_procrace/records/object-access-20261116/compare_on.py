#!/usr/bin/env python3
"""compare_on.py OFF ON [--build B] [--list N] -- the facts views (kz-static show, both written by the same kz-static) of
one build extracted without and with --object-access (docs/static_facts.md 5s), compared section by section after the
masks below.  Every difference is put in one class; a class this script does not list as declared is undeclared
(plan/full_stage_results_20261116.md 7.0).

Masks (both views): site numbers ("site N", "sites N", "site=N", "sites=N"), LOCKSET row numbers ("row N"), the count of a
section head ("[name] N"), "(and N more)", the FILE_HASHES index that begins a [files] line.  Type names: a
name of the OFF view with an absolute position (".../<tree>/<path>_<ext>:<line>") is written as the option writes it,
"<tree>/<path>" with <tree> the last component of the longest prefix of the path that holds a `.git` entry on this
machine (the rule of TuReader::name_file); a name with no such prefix is left as it is.

Declared classes:
  new-section      every line of the sections objaccess, ftypes, argtypes (only in ON)
  writer-added     a line of [writers] only in ON                 (the writers the reading rules of the option add)
  atype-added      a line of [atype] only in ON                    (the address-escape hits and the untyped writes they expose)
  outcall-added    a line of [outcalls] only in ON                 (the decayed arrays passed to a call)
  file-added       a line of [files] only in ON                    (a header that declares a field or an argument array)
  locs-changed     a [locs] line of one location in both, with other writer counts, A-TYPE counts or status
  fstatus-changed  a [fstatus] line of one field in both, with other counts, status or flags
  basis-added      a [basis] line only in ON: extract.object_access and the five count.* items of the option
  loop-note        a [loops] line whose only difference is the field the note of an undecided L1 (i) names as DISABLED
                   ("L1 (i) undecided: <field> is DISABLED"; more fields are DISABLED with the option, and the note names
                   the first of them); the rule and its certificate are compared as they are
  basis-changed    a [basis] line of one item in both: count.sites, count.untyped_write_sites_counted,
                   count.unknown_effect_pointer_passes, count.field_status, count.lockset, count.lock_paths,
                   count.indirect_calls_incomplete
Every other line must be in both views after the masks.  Prints the counts per class and section and up to N lines of
each undeclared class; rc 0 when every class is declared, 1 otherwise, 2 on a usage error."""
import collections
import os
import re
import sys

NEW_SECTIONS = {"objaccess", "ftypes", "argtypes"}
MASKS = [
    (re.compile(r"(?<=site )\d+|(?<=sites )\d+|(?<=site=)\d+|(?<=sites=)\d+"), "<site>"),
    (re.compile(r"(?<=row )\d+"), "<row>"),
    (re.compile(r"^(\[[a-z_0-9]+\]) \d+$"), r"\1 <n>"),
    (re.compile(r"\(and \d+ more\)"), "(and <n> more)"),
    (re.compile(r"^  \d+(?= \S+ [0-9a-f]{12} \d+$)"), "  <file>"),  # the FILE_HASHES index of a [files] line
]
BASIS_ADDED = re.compile(r"  (extract\.object_access|count\.(object_access|field_types|arg_types|array_decay|type_names_outside_root)) = .*")
BASIS_CHANGED = re.compile(r"  (count\.(sites|untyped_write_sites_counted|unknown_effect_pointer_passes|field_status|lockset|lock_paths|indirect_calls_incomplete)) = .*")
# a type name's position: its file part holds no dot (TuReader::name_pos writes '.' as '_'); a function or object key keeps
# the dots of its file and is not a type name
ABS_NAME = re.compile(r"@(/[^\s:@<>.]+)(?=:\d)")


def read(path):
    with open(path, encoding="utf-8", errors="surrogateescape") as f:
        return f.read().split("\n")


_tree_cache = {}


def tree_name(path):
    """the option's spelling of an absolute file path of a type name (dots already written '_'), or None"""
    if path in _tree_cache:
        return _tree_cache[path]
    parts = path.split("/")
    out = None
    for k in range(len(parts) - 1, 1, -1):
        d = "/".join(parts[:k])
        if d and os.path.lexists(os.path.join(d, ".git")):
            out = "<%s>/%s" % (parts[k - 1], "/".join(parts[k:]))
            break
    _tree_cache[path] = out
    return out


def rename(line, counter):
    def sub(m):
        t = tree_name(m.group(1))
        if t is None:
            return m.group(0)
        counter[m.group(1)] += 1
        return "@" + t
    return ABS_NAME.sub(sub, line)


def masked(line):
    for rx, repl in MASKS:
        line = rx.sub(repl, line)
    return line


def split(lines, renames=None):
    cur, out = "head", collections.defaultdict(list)
    for l in lines:
        m = re.match(r"^\[([A-Za-z_0-9]+)\]", l)
        if m:
            cur = m.group(1)
        if l == "":
            continue  # the end of the text (its last section differs between the two views)
        if renames is not None:
            l = rename(l, renames)
        out[cur].append(masked(l))
    return out


def key_of(section, line):
    if section in ("locs", "fstatus"):
        m = re.match(r"^  (\S+) ", line)
        return m.group(1) if m else None
    if section == "basis":
        m = re.match(r"^  (\S+) = ", line)
        return m.group(1) if m else None
    return None


def main():
    a = sys.argv[1:]
    opts = {"--build": "", "--list": "20"}
    pos = []
    i = 0
    while i < len(a):
        if a[i] in opts and i + 1 < len(a):
            opts[a[i]] = a[i + 1]
            i += 2
        else:
            pos.append(a[i])
            i += 1
    if len(pos) != 2:
        print(__doc__, file=sys.stderr)
        return 2
    renames = collections.Counter()
    off = split(read(pos[0]), renames)
    on = split(read(pos[1]))
    counts = collections.Counter()
    bad = collections.defaultdict(list)
    for sec in sorted(set(off) | set(on)):
        if sec in NEW_SECTIONS:
            if sec in off:
                bad["new-section-in-off"].append(sec)
            counts[("new-section", sec)] += len(on.get(sec, []))
            continue
        ca, cb = collections.Counter(off.get(sec, [])), collections.Counter(on.get(sec, []))
        only_a = list((ca - cb).elements())
        only_b = list((cb - ca).elements())
        if sec == "loops":
            note = re.compile(r"(L1 \(i\) undecided: )\S+( is DISABLED)")
            ka = collections.Counter(note.sub(r"\1<f>\2", l) for l in only_a)
            kb = collections.Counter(note.sub(r"\1<f>\2", l) for l in only_b)
            both = ka & kb
            counts[("loop-note", sec)] += sum(both.values())
            only_a = list((ka - both).elements())
            only_b = list((kb - both).elements())
        if sec in ("locs", "fstatus", "basis"):
            ka = collections.defaultdict(list)
            for l in only_a:
                ka[key_of(sec, l)].append(l)
            rest_b = []
            for l in only_b:
                k = key_of(sec, l)
                if k is not None and ka.get(k):
                    old = ka[k].pop()
                    if sec == "basis":
                        if BASIS_CHANGED.fullmatch(l) and BASIS_CHANGED.fullmatch(old):
                            counts[("basis-changed", sec)] += 1
                        else:
                            bad["basis-other"].append("%s => %s" % (old, l))
                    else:
                        counts[(sec + "-changed", sec)] += 1
                else:
                    rest_b.append(l)
            only_a = [l for v in ka.values() for l in v]
            only_b = rest_b
        for l in only_a:
            bad["removed:" + sec].append(l)
        for l in only_b:
            if sec == "writers":
                counts[("writer-added", sec)] += 1
            elif sec == "atype":
                counts[("atype-added", sec)] += 1
            elif sec == "outcalls":
                counts[("outcall-added", sec)] += 1
            elif sec == "files":
                counts[("file-added", sec)] += 1
            elif sec == "basis" and BASIS_ADDED.fullmatch(l):
                counts[("basis-added", sec)] += 1
            else:
                bad["added:" + sec].append(l)
    n = int(opts["--list"])
    print("compare_on %s: %s" % (opts["--build"], "only declared classes" if not bad else "undeclared classes: " + ", ".join("%s %d" % (k, len(v)) for k, v in sorted(bad.items()))))
    print("renamed absolute positions of type names (OFF -> the option's spelling): %d occurrences of %d paths" % (sum(renames.values()), len(renames)))
    for p, c in sorted(renames.items()):
        print("  %s -> %s (%d)" % (p, tree_name(p), c))
    for (cls, sec), c in sorted(counts.items()):
        print("%s\t%s\t%d" % (cls, sec, c))
    for k, v in sorted(bad.items()):
        print("undeclared %s: %d" % (k, len(v)))
        for l in v[:n]:
            print("    " + l[:400])
    return 0 if not bad else 1


if __name__ == "__main__":
    sys.exit(main())
