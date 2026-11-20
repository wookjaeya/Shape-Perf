import sys, re, collections
# lintgroups.py DIR: the case-ids hits of the copies (lint_probe.txt, lint_instrument.txt, lint_runtime.txt in DIR;
# each tree was copied as lib/) sorted into the three groups of RV3A 6.3 O9 by path rules
def group(top, path):
    p = path[len('lib/'):]
    if top == 'instrument' and re.match(r'[^/]+/sites/', p): return '1 stage-3 input: instrument site manifests'
    if top == 'runtime' and p in ('src/kz_rt_osal.c', 'include/kz_rt_osal.h'): return '2 framework knowledge: runtime OSAL adapter'
    if '/tests/' in '/' + p: return '3a harmless: tests'
    if top == 'instrument' and re.match(r'clang/out/', p): return '3b harmless: committed transform outputs'
    if re.match(r'([^/]+/)?scripts/', p): return '3c harmless: drivers (scripts)'
    if re.search(r'\.(c|cc|cpp|cxx|h|hh|hpp)$', p): return '3d code files outside tests (comments, names, strings)'
    return '3e other: build files and tool sources'
out = collections.Counter(); files = collections.defaultdict(set)
for top in ('probe', 'instrument', 'runtime'):
    for line in open('%s/lint_%s.txt' % (sys.argv[1], top)):
        m = re.match(r'(\S+?):(\d+): \[case-ids\]', line)
        if not m: continue
        g = group(top, m.group(1)); out[(top, g)] += 1; files[(top, g)].add(m.group(1))
tot = 0
for k in sorted(out, key=lambda k: (k[1], k[0])):
    print('%-10s %-55s %5d hits %3d files  %s' % (k[0], k[1], out[k], len(files[k]), ' '.join(sorted(x[4:] for x in files[k]))[:300] if out[k] < 60 else ''))
    tot += out[k]
print('total', tot)
g = collections.Counter()
for (t, gg), n in out.items(): g[gg] += n
for gg in sorted(g): print('group %-55s %5d' % (gg, g[gg]))
