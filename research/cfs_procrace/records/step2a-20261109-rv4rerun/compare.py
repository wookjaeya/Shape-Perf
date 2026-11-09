# compare.py T NEW OUTFILE: every file of T/sha256.txt against the same path in NEW (sha256), the files only in NEW,
# and for the comparison texts the first two lines per build (the earlier driver's lines) byte for byte.
import hashlib, os, re, sys
T, N, OF = sys.argv[1:4]
def sha(p):
    h = hashlib.sha256()
    with open(p, 'rb') as f:
        for b in iter(lambda: f.read(1 << 20), b''):
            h.update(b)
    return h.hexdigest()
ref = {}
for line in open(os.path.join(T, 'sha256.txt'), encoding='utf-8'):
    h, p = line.rstrip('\n').split('  ', 1)
    ref[p[2:] if p.startswith('./') else p] = h
out = []
cnt = {'same': 0, 'DIFF': 0, 'MISSING': 0}
for p in sorted(ref):
    q = os.path.join(N, p)
    if not os.path.exists(q):
        st, nh = 'MISSING', '-'
    else:
        nh = sha(q)
        st = 'same' if nh == ref[p] else 'DIFF'
    cnt[st] += 1
    sz = os.path.getsize(q) if os.path.exists(q) else -1
    out.append('%s\t%s\t%s\t%d\t%s' % (st, ref[p][:16], nh[:16], sz, p))
new = []
for d, _, fs in os.walk(N):
    for f in fs:
        p = os.path.relpath(os.path.join(d, f), N)
        if p not in ref:
            new.append(p)
for p in sorted(new):
    q = os.path.join(N, p)
    out.append('NEW\t-\t%s\t%d\t%s' % (sha(q)[:16], os.path.getsize(q), p))
# first two lines of each build in the comparison texts
two = []
for st in ('npsame', 's5same', 'fixsame', 'fix2same', 'fix3same'):
    a = os.path.join(T, st, st + '.txt'); b = os.path.join(N, st, st + '.txt')
    def firsts(path):
        seen = {}
        for line in open(path, encoding='utf-8'):
            m = re.match(r'(\w+): ', line)
            if not m: continue
            seen.setdefault(m.group(1), [])
            if len(seen[m.group(1)]) < 2: seen[m.group(1)].append(line)
        return seen
    fa, fb = firsts(a), firsts(b)
    for bld in sorted(set(fa) | set(fb)):
        ok = fa.get(bld) == fb.get(bld)
        two.append('%s\t%s\t%s' % ('same' if ok else 'DIFF', st, bld))
with open(OF, 'w', encoding='utf-8') as f:
    f.write('# T/sha256.txt files: %s; files only in the new run: %d\n' % (cnt, len(new)))
    f.write('\n'.join(out) + '\n')
    f.write('# first two lines per build of the comparison texts (earlier driver lines)\n')
    f.write('\n'.join(two) + '\n')
print('T files', cnt, 'new-only', len(new), 'first-two-lines', sum(1 for x in two if x.startswith('same')), 'of', len(two))
