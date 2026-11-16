# compare_build.py KZDUMP OUTFILE NEW REF [--allow FILE] -- every file of one output directory (NEW, for example the
# single-build runner's OUT/<build>/NP) against a reference directory (REF, for example T8CF/<build>/NP), file by file,
# with the method of records/step2a-20261110-8cfix/compare_8cfix.py (its comparison functions are copied unchanged):
#   - same bytes: "same".
#   - containers (.kz): kz-dump of both; changed header keys, section lines and records per section are named.
#   - text: every differing hunk with its line ranges and its first old and new line.
#   - a file in only one of the two: ONLY.
# Unlike compare_8cfix.py the result is a verdict: rc 0 when every file is "same" or is named in the allowance file
# (lines "<relative path><TAB><same|DIFF|ONLY>"; # comments), rc 1 otherwise; rc 2 on a usage or tool error.
# Nothing is masked: an allowed difference is still printed with its hunks, marked "allowed".
import collections, difflib, hashlib, os, re, subprocess, sys

args = sys.argv[1:]
ALLOW = None
if '--allow' in args:
    k = args.index('--allow')
    ALLOW = args[k + 1]
    del args[k:k + 2]
if len(args) != 4:
    sys.stderr.write('usage: compare_build.py KZDUMP OUTFILE NEW REF [--allow FILE]\n')
    sys.exit(2)
KZD, OF, NEW, REF = args
out = []


def sha(p):
    h = hashlib.sha256()
    with open(p, 'rb') as f:
        for blk in iter(lambda: f.read(1 << 20), b''):
            h.update(blk)
    return h.hexdigest()


def files(root):
    res = set()
    for d, _, fs in os.walk(root):
        for f in fs:
            res.add(os.path.relpath(os.path.join(d, f), root))
    return res


def dump(path):
    r = subprocess.run([KZD, path], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if r.returncode != 0:
        sys.stderr.write('kz-dump %s: rc %d\n' % (path, r.returncode))
        sys.exit(2)
    head, secline, recs, cur = collections.OrderedDict(), collections.OrderedDict(), collections.defaultdict(list), None
    for line in r.stdout.decode('utf-8', 'replace').split('\n'):
        if not line:
            continue
        m = re.match(r'section (\S+) (.*)', line)
        if m:
            cur = m.group(1)
            secline[cur] = dict(kv.split('=', 1) for kv in m.group(2).split() if '=' in kv)
            continue
        if cur is None:
            m = re.match(r'\s*(parent\[\d+\])\s+(.*)', line)
            if m:
                head[m.group(1)] = m.group(2)
            elif line.startswith('container '):
                for kv in line.split()[1:]:
                    k, v = kv.split('=', 1)
                    head['container.' + k] = v
            elif '=' in line:
                k, v = line.split('=', 1)
                head[k.strip()] = v
            else:
                head[line.strip()] = ''
        elif line.startswith('  '):
            recs[cur].append(re.sub(r'^\s*\S+\[\d+\] ', '', line))
        else:
            # a line of the dump after the sections that is no record (artifact_sha256=...): a header key
            k, _, v = line.partition('=')
            head[k.strip()] = v
    return head, secline, recs


def cmp_kz(a_path, b_path):
    ha, sa, ra = dump(a_path)
    hb, sb, rb = dump(b_path)
    parts = []
    hk = [k for k in sorted(set(ha) | set(hb)) if ha.get(k) != hb.get(k)]
    if hk:
        parts.append('HEADER ' + ','.join(hk))
    sk = []
    for s in list(sa) + [x for x in sb if x not in sa]:
        if s not in sa or s not in sb:
            sk.append('%s(only in %s)' % (s, 'new' if s not in sa else 'ref'))
            continue
        attrs = [k for k in sorted(set(sa[s]) | set(sb[s])) if sa[s].get(k) != sb[s].get(k)]
        if attrs:
            sk.append('%s(%s)' % (s, '/'.join(attrs)))
    if sk:
        parts.append('section lines %d: %s' % (len(sk), ' '.join(sk)))
    for s in sorted(set(ra) | set(rb)):
        x, y = ra.get(s, []), rb.get(s, [])
        if x == y:
            continue
        cx, cy = collections.Counter(x), collections.Counter(y)
        gone, came = sorted((cx - cy).elements()), sorted((cy - cx).elements())
        if s == 'BASIS':
            name = lambda r: (re.match(r'name="([^"]*)"', r) or re.match(r'(.*)', r)).group(1)
            ng, nc = collections.Counter(map(name, gone)), collections.Counter(map(name, came))
            parts.append('BASIS records changed %s; removed %s; added %s' % (','.join(sorted(set(ng) & set(nc))) or '-',
                                                                           ','.join(sorted(set(ng) - set(nc))) or '-',
                                                                           ','.join(sorted(set(nc) - set(ng))) or '-'))
        elif not gone and not came:
            parts.append('%s records reordered' % s)
        else:
            parts.append('%s records removed %d added %d (e.g. %s)' % (s, len(gone), len(came), ' | '.join(r[:160] for r in (gone + came)[:2])))
    return parts


def cmp_text(a_path, b_path):
    la = open(a_path, encoding='utf-8', errors='replace').read().split('\n')
    lb = open(b_path, encoding='utf-8', errors='replace').read().split('\n')
    sm = difflib.SequenceMatcher(None, la, lb, autojunk=False)
    ops = [o for o in sm.get_opcodes() if o[0] != 'equal']
    hunks = []
    for op, i1, i2, j1, j2 in ops:
        old = la[i1][:110] if i1 < i2 else ''
        new = lb[j1][:110] if j1 < j2 else ''
        hunks.append('%s ref %d-%d new %d-%d [%s -> %s]' % (op, i1 + 1, i2, j1 + 1, j2, old, new))
    return ['lines %d vs %d; %d hunks: %s' % (len(la), len(lb), len(ops), ' || '.join(hunks))]


allowed = {}
if ALLOW:
    for line in open(ALLOW, encoding='utf-8'):
        line = line.rstrip('\n')
        if not line or line.startswith('#'):
            continue
        p, st = line.split('\t')
        allowed[p] = st
fa, fb = files(REF), files(NEW)
bad = 0
for rel in sorted(fa | fb):
    a, b = os.path.join(REF, rel), os.path.join(NEW, rel)
    if rel not in fb or rel not in fa:
        st, detail = 'ONLY', 'only in %s' % ('ref' if rel not in fb else 'new')
    elif sha(a) == sha(b):
        st, detail = 'same', 'bytes'
    else:
        d = cmp_kz(a, b) if rel.endswith('.kz') else cmp_text(a, b)
        st, detail = 'DIFF', '; '.join(d) or 'bytes differ, dump equal'
    mark = ''
    if st != 'same':
        if allowed.get(rel) == st:
            mark = 'allowed'
        else:
            mark = 'NOT-ALLOWED'
            bad += 1
    out.append('%s\t%s\t%s\t%s' % (st, rel, detail, mark))
cnt = collections.Counter(x.split('\t', 1)[0] for x in out)
with open(OF, 'w', encoding='utf-8') as fh:
    fh.write('# compare_build.py: %s against %s; allowance %s\n' % (NEW, REF, ALLOW or 'none'))
    for k, n in sorted(cnt.items()):
        fh.write('# %s %d\n' % (k, n))
    fh.write('# not allowed %d\n' % bad)
    fh.write('\n'.join(out) + '\n')
for k, n in sorted(cnt.items()):
    print(k, n)
print('not allowed', bad)
sys.exit(1 if bad else 0)
