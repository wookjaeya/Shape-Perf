# oldlines.py T NEW: for the driver texts whose format changed, whether every line of the T/ file appears in the new
# file in the same order (old lines as a subsequence), after the normalizations named per file; prints one line each.
import re, sys
T, N = sys.argv[1:3]
def lines(p): return open(p, encoding='utf-8').read().split('\n')
def subseq(old, new):
    i = 0
    for l in new:
        if i < len(old) and l == old[i]: i += 1
    return i, len(old)
for f in ('npsame/npsame.txt', 's5same/s5same.txt', 'fixsame/fixsame.txt', 'fix2same/fix2same.txt', 'fix3same/fix3same.txt'):
    old, new = lines(T + '/' + f), lines(N + '/' + f)
    first = [l for l in new if re.match(r'\w+: (sections with the same records|sections only in the second): ', l)]
    same = (first == [l for l in old if l])
    i, n = subseq(old, new)
    print('%s: old lines %d, in order in new %d; new lines %d; the two earlier lines per build = old file: %s' % (f, n, i, len(new), same))
old, new = lines(T + '/identity/identity.txt'), lines(N + '/identity/identity.txt')
o2, n2 = old[2:], new[2:]   # line 1: date, line 2: repo commit
i, n = subseq(o2, n2)
print('identity/identity.txt: old lines 3..%d: %d, in order in new %d; new lines %d; head lines 1-2 differ by date and repo commit only: %s' % (
    len(old), n, i, len(new), re.sub(r'identity \S+:', '', old[0]) == re.sub(r'identity \S+:', '', new[0])))
def runs(p, pre):
    out = {}
    for l in lines(p):
        if not l: continue
        d = l.split(' ', 1)[0].replace(pre, '')
        f = dict(re.findall(r'(\w+)=(\S+)', l))
        out[d] = {k: f[k] for k in ('basis', 'extract', 'show', 'relations')}
    return out
a = runs(T + '/run.log', T + '/'); b = runs(N + '/run.log', N + '/')
print('run.log: T lines %d, new lines %d; rc of the T lines equal in new: %s; new-only: %s' % (
    len(a), len(b), all(b.get(k) == v for k, v in a.items()), ' '.join(sorted(set(b) - set(a)))))
