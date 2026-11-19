# compare_derived.py (python3 -I) -- the outputs of the single-build runner on a DERIVED build (B', one_build.sh
# --path-map) against the same runner's outputs on the reference build (B, same kz-static), record by record, after the
# derived work space's paths are mapped back to the reference build's paths.
#
#   python3 -I compare_derived.py KZDUMP OUT B_DIR BP_DIR --path-map MAP --declared DECL
#
#   B_DIR, BP_DIR  the NP directories (basis.kz facts.kz basis.txt extract.txt relations.txt relations.log args.txt sha256.txt)
#   MAP            the work space's path map (path_map.sh format: reference prefix TAB derived prefix), used inverted
#   DECL           the declared difference list, written before the derived build: lines
#                    <file>\t<part>\t<condition>\t<category>
#                  <file> basis.kz | facts.kz | a text file name | *; <part> a section name, "header", "section-line",
#                  "line" (text files) or * (any); '#' lines are comments.  <condition> is one or more terms joined by ";;", all of
#                  which must hold for the finding (records after the path mapping):
#                    both=RE   RE is found in the record on each side where it exists (B, B', or both)
#                    new=RE    RE is found in the B' record (an added or changed record)
#                    old=RE    RE is found in the B record (a removed or changed record)
#                    only=F,G  a changed record: the two are equal once the fields F, G (name=value; a quoted value may
#                              hold escaped quotes) are left out
#                    treetok   (2026-11-19) a changed record of facts.kz: B's record equals B''s once every source-tree
#                              token "<t>" of B (B's facts BASIS items source_tree[i] = "<dir> <t>", static_facts.md 5x,
#                              kairos/lib/llvm/serialize.cpp:1196-1212) is written as its directory <dir>; a derived work
#                              space copies the sources without their git metadata (derive_c1.sh D2), so B' has no source
#                              tree and writes those names with the absolute path
#                  A plain RE is both=RE.
#
# Steps:
#   1. Path mapping.  Every derived prefix of MAP is replaced by its reference prefix in every record of BP (a prefix
#      also matches the directory itself).  The map is then confirmed by content: every FILE_HASHES entry of B' maps to
#      a FILE_HASHES path of B; the entries whose sha256 differs are listed (they must be declared like any other
#      difference).  FILE_HASHES entries are matched by path, so a "fh=<n>" field of any record (and in facts a "tu=<n>"
#      field, which is also a FILE_HASHES index: serialize.cpp:175, :198, :314) is compared as the path of
#      FILE_HASHES[<n>] of its own file, and FILE_HASHES itself is compared as a set (whether its order is the same
#      is recorded in the summary); any other section whose records are the same but in another order is a finding.
#   2. Containers (kz-dump): header keys, section lines (count, crc32) and the records of each section in order.  A
#      section whose records are equal in order is "same"; otherwise every record removed, added or changed (difflib on
#      the record lists) is shown and checked against DECL.
#   3. Text files: lines after the path mapping, every differing hunk checked against DECL (part "line").
# Output: one line per finding with "declared:<category>" or "NOT-DECLARED"; a summary per file; rc 0 when every
# difference is declared, rc 1 otherwise, rc 2 on a usage or tool error.  Nothing is masked: declared differences are
# printed with their records.
import collections, difflib, os, re, subprocess, sys

args = sys.argv[1:]
opts = {}
for k in ('--path-map', '--declared'):
    if k in args:
        i = args.index(k)
        opts[k] = args[i + 1]
        del args[i:i + 2]
if len(args) != 4 or len(opts) != 2:
    sys.stderr.write('usage: compare_derived.py KZDUMP OUT B_DIR BP_DIR --path-map MAP --declared DECL\n')
    sys.exit(2)
KZD, OF, BD, PD = args

pairs = []
for line in open(opts['--path-map'], encoding='utf-8'):
    line = line.rstrip('\n')
    if not line or line.startswith('#'):
        continue
    f, t = line.split('\t')
    if not (f.startswith('/') and f.endswith('/') and t.startswith('/') and t.endswith('/')):
        sys.stderr.write('path map: bad line %r\n' % line)
        sys.exit(2)
    pairs.append((t.rstrip('/'), f.rstrip('/')))
pairs.sort(key=lambda p: -len(p[0]))
prx = [(re.compile(re.escape(t) + r'(?=/|"|\'|\s|:|,|;|\)|$)'), f) for t, f in pairs]


def unmap(s):
    for rx, f in prx:
        s = rx.sub(lambda m: f, s)
    return s


decl = []
for line in open(opts['--declared'], encoding='utf-8'):
    line = line.rstrip('\n')
    if not line or line.startswith('#'):
        continue
    p = line.split('\t')
    if len(p) != 4:
        sys.stderr.write('declared list: want 4 tab-separated fields: %r\n' % line)
        sys.exit(2)
    for t in p[2].split(';;'):
        re.compile(t.split('=', 1)[1] if re.match(r'(both|new|old)=', t) else ('' if t.startswith('only=') or t == 'treetok' else t))
    decl.append((p[0], p[1], p[2], p[3]))


def strip_fields(r, fields):
    for f in fields:
        r = re.sub(r'(^|\s)' + re.escape(f) + r'=("(?:[^"\\]|\\.)*"|\S*)', r'\1' + f + '=*', r)
    return r


def holds(term, old, new):
    if term == 'treetok':
        k, v = 'treetok', ''
    else:
        k, _, v = term.partition('=') if re.match(r'(both|new|old|only)=', term) else ('both', '', term)
    if k == 'both':
        rx = re.compile(v)
        return all(rx.search(r) for r in (old, new) if r is not None)
    if k == 'new':
        return new is not None and re.search(v, new) is not None
    if k == 'old':
        return old is not None and re.search(v, old) is not None
    if k == 'only':
        return old is not None and new is not None and strip_fields(old, v.split(',')) == strip_fields(new, v.split(','))
    if k == 'treetok':
        return old is not None and new is not None and bool(TREES) and expand_trees(old) == new
    return False


TREES = []  # (token, directory) of B's facts BASIS source_tree items, the longest token first (filled by cmp_container)


def expand_trees(r):
    for t, d in TREES:
        r = r.replace(t + '/', d + '/')
    return r


def declared(fname, part, old, new):
    """the category of the first declaration whose terms all hold for the finding (None if none)"""
    for f, pt, cond, cat in decl:
        if f not in (fname, '*') or pt not in (part, '*'):
            continue
        if all(holds(t, old, new) for t in cond.split(';;')):
            return cat
    return None


def dump(path, mapped):
    r = subprocess.run([KZD, path], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if r.returncode != 0:
        sys.stderr.write('kz-dump %s: rc %d\n' % (path, r.returncode))
        sys.exit(2)
    head, secline, recs, cur = collections.OrderedDict(), collections.OrderedDict(), collections.OrderedDict(), None
    for line in r.stdout.decode('utf-8', 'replace').split('\n'):
        if not line:
            continue
        if mapped:
            line = unmap(line)
        m = re.match(r'section (\S+) (.*)', line)
        if m:
            cur = m.group(1)
            secline[cur] = dict(kv.split('=', 1) for kv in m.group(2).split() if '=' in kv)
            recs[cur] = []
            continue
        if cur is None or not line.startswith('  '):
            m = re.match(r'\s*(parent\[\d+\])\s+(.*)', line)
            if m:
                head[m.group(1)] = m.group(2)
            elif line.startswith('container '):
                for kv in line.split()[1:]:
                    k, v = kv.split('=', 1)
                    head['container.' + k] = v
            else:
                k, _, v = line.partition('=')
                head[k.strip()] = v
            continue
        recs[cur].append(re.sub(r'^\s*\S+\[\d+\] ', '', line))
    fh = [re.match(r'path="([^"]*)"', x).group(1) for x in recs.get('FILE_HASHES', [])]
    # fields that hold a FILE_HASHES index: fh= in every container; in facts also tu= (the unit's main file,
    # kairos/lib/llvm/serialize.cpp:175, :198, :314 r.tu = fh_of(...)); in basis tu= is the TUS ordinal and stays.
    # (2026-11-19) In facts also decl_fh= (ARG_TYPES, serialize.cpp:1032) and effect_fh= (RESULT_EFFECT, :1356)
    idx = r'\b(fh|tu|decl_fh|effect_fh)=(\d+)' if head.get('container.kind') == 'facts' else r'\b(fh)=(\d+)'
    for s in recs:
        recs[s] = [re.sub(idx, lambda m: '%s=<%s>' % (m.group(1), fh[int(m.group(2))] if int(m.group(2)) < len(fh) else '?' + m.group(2)), x)
                   for x in recs[s]]
    return head, secline, recs


out, bad, summ = [], 0, []


def finding(fname, part, what, old, new):
    global bad
    cat = declared(fname, part, old, new)
    if cat is None:
        bad += 1
    out.append('%s\t%s\t%s\t%s' % (fname, part, what, ('declared:' + cat) if cat else 'NOT-DECLARED'))
    return cat


def cmp_container(name):
    ha, sa, ra = dump(os.path.join(BD, name), False)
    hb, sb, rb = dump(os.path.join(PD, name), True)
    if ha.get('container.kind') == 'facts':
        TREES[:] = sorted(((m.group(2), m.group(1)) for m in (re.match(r'name="source_tree\[\d+\]" value="(\S+) (<[^>"]+>)"', r)
                                                         for r in ra.get('BASIS', [])) if m), key=lambda x: -len(x[0]))
    nd = 0
    for k in sorted(set(ha) | set(hb)):
        if ha.get(k) != hb.get(k):
            nd += 1
            finding(name, 'header', '%s: %s -> %s' % (k, ha.get(k), hb.get(k)), '%s=%s' % (k, ha.get(k)) if k in ha else None, '%s=%s' % (k, hb.get(k)) if k in hb else None)
    secs = list(sa) + [s for s in sb if s not in sa]
    same = 0
    for s in secs:
        if s not in sa or s not in sb:
            nd += 1
            finding(name, 'section-line', '%s only in %s' % (s, 'B' if s in sa else "B'"), s if s in sa else None, s if s in sb else None)
            continue
        x, y = ra[s], rb[s]
        attrs = [k for k in sorted(set(sa[s]) | set(sb[s])) if sa[s].get(k) != sb[s].get(k)]
        if attrs:
            nd += 1
            finding(name, 'section-line', '%s %s' % (s, ' '.join('%s:%s->%s' % (k, sa[s].get(k), sb[s].get(k)) for k in attrs)),
                    '%s %s' % (s, ' '.join(attrs)), '%s %s' % (s, ' '.join(attrs)))
        if x == y:
            same += 1
            continue
        if s == 'FILE_HASHES':
            # matched by path (step 1): the order is compared apart and recorded, the entries as a set
            summ.append('%s FILE_HASHES: same order after the map: no; same entries: %s' % (name, 'yes' if sorted(x) == sorted(y) else 'no'))
            x, y = sorted(x), sorted(y)
            if x == y:
                same += 1
                continue
        elif sorted(x) == sorted(y):
            nd += 1
            finding(name, s, 'records reordered (%d records, same multiset)' % len(x), 'reordered', 'reordered')
            continue
        sm = difflib.SequenceMatcher(None, x, y, autojunk=False)
        for op, i1, i2, j1, j2 in sm.get_opcodes():
            if op == 'equal':
                continue
            old, new = x[i1:i2], y[j1:j2]
            # changed records pairwise when the counts agree, else each removed and added record on its own
            if op == 'replace' and len(old) == len(new):
                for k in range(len(old)):
                    nd += 1
                    finding(name, s, 'changed B[%d] B\'[%d]: %s  ->  %s' % (i1 + k, j1 + k, old[k], new[k]), old[k], new[k])
            else:
                for k, r in enumerate(old):
                    nd += 1
                    finding(name, s, 'removed B[%d]: %s' % (i1 + k, r), r, None)
                for k, r in enumerate(new):
                    nd += 1
                    finding(name, s, 'added B\'[%d]: %s' % (j1 + k, r), None, r)
    summ.append('%s: sections %d, equal in order %d, findings %d' % (name, len(secs), same, nd))
    return ra, rb


def cmp_text(name):
    pa, pb = os.path.join(BD, name), os.path.join(PD, name)
    if not os.path.exists(pa) or not os.path.exists(pb):
        finding(name, 'line', 'only in %s' % ('B' if os.path.exists(pa) else "B'"), name if os.path.exists(pa) else None, name if os.path.exists(pb) else None)
        summ.append('%s: only in one' % name)
        return
    la = open(pa, encoding='utf-8', errors='replace').read().split('\n')
    lb = [unmap(x) for x in open(pb, encoding='utf-8', errors='replace').read().split('\n')]
    sm = difflib.SequenceMatcher(None, la, lb, autojunk=False)
    nd = 0
    for op, i1, i2, j1, j2 in sm.get_opcodes():
        if op == 'equal':
            continue
        old, new = la[i1:i2], lb[j1:j2]
        if op == 'replace' and len(old) == len(new):
            for k in range(len(old)):
                nd += 1
                finding(name, 'line', 'changed %d/%d: %s  ->  %s' % (i1 + k + 1, j1 + k + 1, old[k], new[k]), old[k], new[k])
        else:
            for k, r in enumerate(old):
                nd += 1
                finding(name, 'line', 'removed %d: %s' % (i1 + k + 1, r), r, None)
            for k, r in enumerate(new):
                nd += 1
                finding(name, 'line', 'added %d: %s' % (j1 + k + 1, r), None, r)
    summ.append('%s: lines %d vs %d, findings %d' % (name, len(la), len(lb), nd))


rb_ = cmp_container('basis.kz')
# the path map confirmed by content: FILE_HASHES of B' (mapped) against B
fa = {}
for r in rb_[0].get('FILE_HASHES', []):
    m = re.match(r'path="([^"]*)" sha256=(\S+) size=(\S+)', r)
    fa[m.group(1)] = (m.group(2), m.group(3))
fb = {}
for r in rb_[1].get('FILE_HASHES', []):
    m = re.match(r'path="([^"]*)" sha256=(\S+) size=(\S+)', r)
    fb[m.group(1)] = (m.group(2), m.group(3))
both = sorted(set(fa) & set(fb))
samec = [p for p in both if fa[p] == fb[p]]
diffc = [p for p in both if fa[p] != fb[p]]
unm = [p for p in fb if p.startswith('/') and any(p == t or p.startswith(t + '/') for t, _ in pairs)]
summ.append('path map by content: FILE_HASHES B %d, B\' %d; same path after the map %d, of them same sha256 %d, other sha256 %d (%s); only in B %d; only in B\' %d; derived paths left unmapped %d'
            % (len(fa), len(fb), len(both), len(samec), len(diffc), ', '.join(diffc) or '-', len(set(fa) - set(fb)), len(set(fb) - set(fa)), len(unm)))
cmp_container('facts.kz')
for t in ('basis.txt', 'extract.txt', 'relations.txt', 'relations.log', 'args.txt', 'sha256.txt'):
    cmp_text(t)
cnt = collections.Counter(x.rsplit('\t', 1)[1] for x in out)
with open(OF, 'w', encoding='utf-8') as fh:
    fh.write('# compare_derived.py: B %s, B\' %s, path map %s (inverted), declared list %s\n' % (BD, PD, opts['--path-map'], opts['--declared']))
    for s in summ:
        fh.write('# %s\n' % s)
    for k, n in sorted(cnt.items()):
        fh.write('# %s %d\n' % (k, n))
    fh.write('# not declared %d\n' % bad)
    fh.write('\n'.join(out) + '\n')
for s in summ:
    print(s)
for k, n in sorted(cnt.items()):
    print(k, n)
print('not declared', bad)
sys.exit(1 if bad else 0)
