#!/usr/bin/env python3
# report_ns2.py -- the counts the name-space data step keeps (coverage rule revision 2, kairos/docs/semantics_coverage.md 8.5),
# read from the generated files and from the scan's own functions (env/tools/semantics_scan.py, imported, not changed).
#
#   python3 -I report_ns2.py <trees_r2.tsv> <p0.cpp> <data dir> <out dir>
#
# Writes into <out dir> (new files only; an existing file stops the run with rc 2):
#   layer_functions.txt   per component the layer-fn lines (role, function, definition line, index of the kind parameter)
#   kinds_per_component.txt per component: kinds in the data, kinds with at least one operation, kinds the revision-1 file
#                         had (by constant), kinds new against revision 1, kind-unresolved and not-covered lines, counts of
#                         each new line kind
#   ops_per_kind.tsv      per component and kind: lookup / create / delete / rollback counts (public in brackets),
#                         stored-name, other-store, not-covered, and the component's clear lines (layer-op)
#   unresolved.txt        every kind-unresolved line with the reason class (the parameter of the enclosing function, or
#                         another text)
#   writers.txt           per component the name-storing writers of the name field (the name-initialising macro and direct
#                         stores in function bodies) and the entry each lies in, or none; for the table generation, every
#                         line that writes the stored-name path (assignment, or first argument of a P0 data function that
#                         writes through its first argument) and whether a
#                         copy or clear line of the entry names it
#   entry_diff.txt        per component the entry lines of the revision-1 file against the revision-2 file, per kind
#                         (generation N) or per table (generation T)
#   get_by_name.txt       calls of the layer's get functions that take a name (not lookup lines of revision 2), per kind
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'env', 'tools'))
import semantics_scan as S  # noqa: E402


def wopen(out, name):
    p = os.path.join(out, name)
    if os.path.exists(p):
        sys.stderr.write('refusing: %s exists\n' % p)
        sys.exit(2)
    return open(p, 'w')


def read(p):
    return open(p).read().split('\n')


def entry_lines_n(lines):
    """kind constant -> sorted operation lines (revision-1 or revision-2 file of generation N)."""
    by, cur = {}, None
    for l in lines:
        m = re.match(r'^kind (\S+) ', l)
        if m:
            cur = m.group(1)
            by.setdefault(cur, set())
            continue
        if l.startswith('  ') and cur:
            by[cur].add(l.strip())
        elif not l.startswith('  '):
            cur = None
    return by


def entry_lines_t(lines):
    by, cur = {}, None
    for l in lines:
        m = re.match(r'^table (\S+)$', l)
        if m:
            cur = m.group(1)
            by.setdefault(cur, set())
            continue
        if l.startswith('  ') and cur:
            by[cur].add(l.strip())
        elif not l.startswith('  '):
            cur = None
    return by


def main(argv):
    if len(argv) != 5:
        sys.stderr.write('usage: report_ns2.py <trees_r2.tsv> <p0.cpp> <data dir> <out dir>\n')
        return 2
    trees, p0cpp, data, out = argv[1:]
    os.makedirs(out, exist_ok=True)
    comps = S.read_trees(trees)
    names = S.p0_names(p0cpp)
    text = open(p0cpp).read()
    p0cmp = [n for n, t in names if t == 'compare' and re.search(r'\{"%s", "byte-string comparison"' % re.escape(n), text)]
    fills = S.fill_names(p0cpp)
    # the P0 data functions that write through their first argument (data entries with destination 0), from the table text
    wfn = '|'.join(re.escape(m) for m in re.findall(r'\bdata\("([A-Za-z_]\w*)", 0,', text))
    LF, KC, OPS, UN, WR, DF, GB = (wopen(out, n) for n in ('layer_functions.txt', 'kinds_per_component.txt', 'ops_per_kind.tsv', 'unresolved.txt',
                                                         'writers.txt', 'entry_diff.txt', 'get_by_name.txt'))
    OPS.write('component\tkind\tlookup(public)\tcreate(public)\tdelete(public)\trollback\tstored-name\tother-store\tnot-covered\n')
    for c in comps:
        lab = c['label']
        f1 = os.path.join(data, 'framework_namespaces_%s.txt' % lab)
        f2 = os.path.join(data, 'framework_namespaces2_%s.txt' % lab)
        if not os.path.isfile(f1):
            continue
        L1, L2 = read(f1), read(f2)
        if c.get('objid'):
            ns2 = S.namespaces2(c, p0cmp, fills)
            for l in L2:
                if l.startswith('layer-fn '):
                    LF.write('%s\t%s\n' % (lab, l))
            k2 = entry_lines_n(L2)
            k1 = entry_lines_n(L1)
            with_ops = sorted(k for k, v in k2.items() if any(not x.startswith('not-covered') for x in v))
            new = [k for k in with_ops if k not in k1]
            nc = [l.strip() for l in L2 if l.startswith('  not-covered')]
            unres = [l for l in L2 if l.startswith('kind-unresolved ')]
            KC.write('== %s (generation N)\n' % lab)
            KC.write('kinds in the data (constants of the kind range): %d\n' % len(k2))
            KC.write('kinds with at least one operation: %d (%s)\n' % (len(with_ops), ' '.join(with_ops)))
            KC.write('kinds of the revision-1 file (by constant): %d (%s)\n' % (len(k1), ' '.join(sorted(k1))))
            KC.write('kinds new against revision 1: %d (%s)\n' % (len(new), ' '.join(new)))
            KC.write('kind-unresolved lines: %d\n' % len(unres))
            KC.write('not-covered lines: %d\n' % len(nc))
            for w in ('layer-fn', 'layer-op', '  rollback', 'kind-unresolved', '  not-covered'):
                KC.write('line kind %s: %d\n' % (w.strip(), sum(1 for l in L2 if l.startswith(w))))
            for l in L2:
                if l.startswith('layer-op '):
                    KC.write('  %s\n' % l)
            for k, v, kr, kl in ns2['kinds']:
                e = ns2['ent'][k]
                cnt = lambda role: '%d(%d)' % (len(e[role]), sum(1 for x in e[role] if x[4]))
                ncl = [x for x in k2.get(k, ()) if x.startswith('not-covered')]
                OPS.write('%s\t%s\t%s\t%s\t%s\t%d\t%d\t%d\t%s\n' % (lab, k, cnt('lookup'), cnt('create'), cnt('delete'), len(e['rollback']), len(e['stored-name']),
                                                                 len({(r, ln) for _, r, ln in e['other-store']}), ncl[0][len('not-covered '):] if ncl else '-'))
            # unresolved: reason class
            files = S.Files(c['root'])
            sources = S.files_of(c['root'], c.get('sources', ''))
            defs = S.definitions(files, sources)
            UN.write('== %s\n' % lab)
            for fn, r, ln, arg in ns2['unresolved']:
                d = [x for x in defs.get(fn, []) if x[0] == r]
                pn = set()
                for x in d:
                    pn |= {S.param_name(q) for q in x[4]['params']}
                cls = ('the kind argument is the parameter %s of %s' % (S.unparen(arg), fn)) if S.unparen(arg) in pn else 'the kind argument is another text'
                UN.write('%s at=%s:%d: %s | argument: %s\n' % (fn, r, ln, cls, arg))
            # writers of the name field
            ws = ns2['writers']
            cov = [w for w in ws if w[4]]
            WR.write('== %s (generation N): name field %s\n' % (lab, ns2['nfield']))
            WR.write('name-storing writers in function bodies: %d; in an entry: %d; in none: %d\n' % (len(ws), len(cov), len(ws) - len(cov)))
            r1at = {m.group(1) for l in L1 for m in [re.match(r'^  (?:stored-name \S+|other-store) at=(\S+)', l)] if m}
            nonclear = [w for w in ws if w[4] != 'layer-op clear-slot']
            WR.write('the same writers without the layer clear: %d; named by a stored-name or other-store line of the revision-1 file: %d\n'
                     % (len(nonclear), sum(1 for w in nonclear if '%s:%d' % (w[1], w[2]) in r1at)))
            for fn, r, ln, form, by in ws:
                WR.write('  %s %s at=%s:%d -> %s; revision 1: %s\n' % (form, fn, r, ln, by or 'NONE', 'named' if '%s:%d' % (r, ln) in r1at else 'not named'))
            # get calls that take a name (the layer's get functions with a parameter named like the lookup's name parameter)
            GB.write('== %s\n' % lab)
            lk = [x for x in ns2['layer'] if x[0] == 'lookup'][0]
            name_types = {' '.join(S.param_type_words(q)) for q in lk[5][4]['params'] if 'char' in q}
            for role, fn, r0, line0, kp, d0 in ns2['layer']:
                if role != 'get':
                    continue
                takes_name = any(' '.join(S.param_type_words(q)) in name_types for q in d0[4]['params'])
                if not takes_name:
                    continue
                for name, dl in sorted(defs.items()):
                    for d in dl:
                        s = files.get(d[0])
                        for cln, args, pos in S.calls_in(s, d[2], d[3], fn):
                            k = S.resolve_kind(files, sources, d[0], args[kp], {x[0] for x in ns2['kinds']}) if kp < len(args) else None
                            GB.write('%s %s in %s at=%s:%d kind=%s\n' % (role, fn, name, d[0], cln, k or 'unresolved'))
            # entry diff per kind
            DF.write('== %s (generation N; revision-1 lines without the kind line, against revision-2 lines)\n' % lab)
            for k in sorted(set(k1) | set(k2)):
                a, b = k1.get(k, set()), k2.get(k, set())
                for x in sorted(a - b):
                    DF.write('%s - %s\n' % (k, x))
                for x in sorted(b - a):
                    DF.write('%s + %s\n' % (k, x))
                DF.write('%s = %d lines in both\n' % (k, len(a & b)))
        else:
            t1, t2 = entry_lines_t(L1), entry_lines_t(L2)
            KC.write('== %s (generation T, unchanged by revision 2)\n' % lab)
            KC.write('tables: revision 1 %d, revision 2 %d; entry lines equal: %s\n' % (len(t1), len(t2), 'yes' if t1 == t2 else 'NO'))
            DF.write('== %s (generation T)\n' % lab)
            for t in sorted(set(t1) | set(t2)):
                a, b = t1.get(t, set()), t2.get(t, set())
                DF.write('%s: - %d + %d = %d\n' % (t, len(a - b), len(b - a), len(a & b)))
            # writers of each stored-name path: every line that assigns into the path's last field or passes the path first
            files = S.Files(c['root'])
            sources = S.files_of(c['root'], c.get('sources', ''))
            WR.write('== %s (generation T)\n' % lab)
            for t, lines in sorted(t2.items()):
                named = set()
                for x in lines:
                    m = re.match(r'^(copy|clear) \S+ def=\S+ at=(\S+)$', x)
                    if m:
                        named.add(m.group(2))
                pre = re.escape(t).replace(r'\[\]', r'\[[^\]]*\]')
                found = []
                for r in sources:
                    s = files.get(r)
                    for k, l in enumerate(s.code_np.split('\n'), 1):
                        if re.search(pre + r'\s*(?:\[[^\]]*\]\s*)*=[^=]', l) or re.search(r'\b(?:%s)\s*\(\s*(?:\(\s*\w+\s*\**\s*\)\s*)?&?\s*' % wfn + pre + r'\s*[,)\[]', l):
                            found.append('%s:%d' % (r, k))
                inn = [x for x in found if x in named]
                WR.write('table %s: writing lines %d; named by a copy or clear line of the entry %d; not named %d\n' % (t, len(found), len(inn), len(found) - len(inn)))
                for x in found:
                    WR.write('  %s -> %s\n' % (x, 'entry' if x in named else 'NONE'))
    for f in (LF, KC, OPS, UN, WR, DF, GB):
        f.close()
    print('report written to %s' % out)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
