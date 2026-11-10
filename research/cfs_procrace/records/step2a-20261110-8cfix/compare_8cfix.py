# compare_8cfix.py KZDUMP OUTFILE NEW REF [REF ...] -- every file of the 8c fix run (NEW) against earlier runs (REF:
# tests_8c, tests_8b_fix, T/, tests_rv4), file by file.  It is compare_8c.py of the 8c stage; only these two comment lines differ.  As there: a
# text file whose lines differ is reported hunk by hunk (every hunk, its line ranges and its first old and new line),
# so that every differing line can be judged against AR 16.0.  The files are all regular files under NEW and under REF (os.walk), so
# none is left out; a file in only one of them is listed.  Nothing is masked (finding 8 of the 8b verification):
#   - same bytes: "same".
#   - containers (.kz): kz-dump of both.  The header lines (container size, basis_sha256, profile_sha256, tool_sha256,
#     parents, and artifact_sha256 after the sections) are compared one by one and each changed key is named; the
#     section lines (count, crc32) are compared
#     and each changed section is named with the attributes that changed; the records are compared per section (the
#     record index left out), BASIS by item name.  STRINGS has no record lines in a dump, so its bytes show only
#     through its section line (count, crc32).
#   - text: the differing line numbers and the first one (old -> new), or the line counts when they differ.
# Prints one line per file and a summary; exit 0 always (the reader judges every DIFF line against AR 16.0).
import collections, difflib, hashlib, os, re, subprocess, sys

KZD, OF, NEW = sys.argv[1:4]
REFS = sys.argv[4:]
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
        raise SystemExit('kz-dump %s: rc %d' % (path, r.returncode))
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


for ref in REFS:
    tag = os.path.basename(ref.rstrip('/'))
    fa, fb = files(ref), files(NEW)
    for rel in sorted(fa | fb):
        a, b = os.path.join(ref, rel), os.path.join(NEW, rel)
        if rel not in fb or rel not in fa:
            out.append('ONLY\t%s\t%s\tonly in %s' % (tag, rel, 'ref' if rel not in fb else 'new'))
            continue
        if sha(a) == sha(b):
            out.append('same\t%s\t%s\tbytes' % (tag, rel))
            continue
        d = cmp_kz(a, b) if rel.endswith('.kz') else cmp_text(a, b)
        out.append('DIFF\t%s\t%s\t%s' % (tag, rel, '; '.join(d) or 'bytes differ, dump equal'))

cnt = collections.Counter((x.split('\t')[1], x.split('\t', 1)[0]) for x in out)
with open(OF, 'w', encoding='utf-8') as fh:
    fh.write('# compare_8c.py: %s against %s\n' % (NEW, ' '.join(REFS)))
    for (t, k), n in sorted(cnt.items()):
        fh.write('# %s %s %d\n' % (t, k, n))
    fh.write('\n'.join(out) + '\n')
for (t, k), n in sorted(cnt.items()):
    print(t, k, n)
