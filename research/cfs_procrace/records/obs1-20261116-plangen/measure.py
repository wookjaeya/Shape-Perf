#!/usr/bin/env python3
# measure.py -- counts of a generated plan (sidecar and manifest), for the results document; reads only.
#   python3 -I measure.py <plan dir>...
import collections, re, sys
for d in sys.argv[1:]:
    side = open(d + '/plan.side').read().split('\n')
    man = open(d + '/plan.kzs').read().split('\n')
    rows = [l for l in side if l.startswith('row ')]
    tiers = collections.Counter(re.search(r' tier=(\d)', l).group(1) for l in rows)
    planned = [l for l in rows if 'state=planned' in l]
    paths = collections.Counter(l.split()[2][0] for l in planned)
    rules = collections.Counter()
    for l in man:
        m = re.match(r'site \d+ label=s\d+\.(\S+)', l)
        if m:
            for r in m.group(1).split('+'):
                rules[r] += 1
    nsites = sum(1 for l in man if l.startswith('site '))
    vals = sum(1 for l in man if l.startswith('value '))
    needs = collections.Counter()
    for l in side:
        if l.startswith('need '):
            f = l.split()
            w = 'observation' if f[1] == 'row' and 'observation not-planned' in l else (f[3] if f[1] == 'row' else ' '.join(f[2:4]) if f[1] != 'plan' else ' '.join(f[2:4]))
            needs[(w, re.search(r'reason=(\S+\([^)]*\))', l).group(1))] += 1
    gaps = collections.Counter(re.search(r'reason=(\S+\([^)]*\))', l).group(1) for l in side if l.startswith('gap '))
    print('== %s' % d.rstrip('/').split('/')[-1])
    print('rows %d tiers %s planned %d %s cut %d' % (len(rows), dict(sorted(tiers.items())), len(planned), dict(sorted(paths.items())), len(rows) - len(planned)))
    print('sites %d values %d per rule %s' % (nsites, vals, dict(sorted(rules.items()))))
    print('needs %s' % {'%s | %s' % k: v for k, v in sorted(needs.items())})
    print('gaps %s' % dict(sorted(gaps.items())))
    for l in side:
        if l.startswith(('creation ', 'stop-event ', 'default-module ')):
            print(l[:260])
    print('modules %d exes %d' % (sum(1 for l in side if l.startswith('module ')), sum(1 for l in side if l.startswith('exe '))))
