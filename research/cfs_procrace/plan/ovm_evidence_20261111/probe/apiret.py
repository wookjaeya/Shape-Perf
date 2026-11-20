#!/usr/bin/env python3
# Probe 1b: return type census of public API prototypes (which returns carry a status the R-CLASS rule can grade).
# usage: python3 -I apiret.py <label> <header>...
import re, sys, collections
label = sys.argv[1]; files = sys.argv[2:]
cnt = collections.Counter(); names = collections.defaultdict(list); seen = set()
for f in files:
    t = open(f, errors='replace').read()
    t = re.sub(r'/\*.*?\*/', ' ', t, flags=re.S); t = re.sub(r'//[^\n]*', ' ', t)
    t = re.sub(r'^\s*#.*?$', ' ', t, flags=re.M)
    for m in re.finditer(r'(?:^|(?<=[;}]))\s*((?:static\s+inline\s+|extern\s+)?[A-Za-z_][A-Za-z0-9_ \t]*?[ \t\*]+)((?:CFE|OS|OSAL)_[A-Za-z0-9_]+)\s*\(([^;{}]*)\)\s*(?=[;{])', t, flags=re.S):
        rt = ' '.join(m.group(1).split()); fn = m.group(2)
        if fn in seen or 'typedef' in rt: continue
        seen.add(fn)
        rt = rt.replace('static inline ', '').replace('extern ', '').replace('const ', '')
        if rt in ('CFE_Status_t', 'int32', 'OS_Status_t'): k = 'status(' + rt + ')'
        elif rt == 'void': k = 'void'
        elif rt == 'bool': k = 'bool'
        elif '*' in rt: k = 'pointer'
        else: k = 'other(' + rt + ')'
        cnt[k] += 1; names[k].append(fn)
tot = sum(cnt.values())
print(label, 'prototypes', tot)
for k, v in cnt.most_common(): print('  %-32s %d' % (k, v))
for k in names:
    if k.startswith('bool') or k == 'pointer' or k.startswith('other'): print('  ', k, ':', ' '.join(sorted(names[k])[:60]))
