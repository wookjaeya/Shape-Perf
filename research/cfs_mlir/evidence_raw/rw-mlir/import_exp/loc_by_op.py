import re, glob, collections, sys
known = collections.Counter(); unk = collections.Counter()
for ml in glob.glob('out/*.mlir'):
    txt = open(ml).read()
    locdefs = dict(re.findall(r'^(#loc\d*) = loc\((.*)\)$', txt, re.M))
    for op, l in re.findall(r'^\s+(?:%[^=]+= )?"?(llvm\.[a-z_.]+)"?.*loc\((#loc\d*|"[^"]*"[^)]*|fused[^)]*|unknown)\)\s*$', txt, re.M):
        if l.startswith('#loc'): l = locdefs.get(l, 'unknown')
        (unk if 'unknown' in l else known)[op] += 1
print('op kind: known / unknown')
for op in sorted(set(known)|set(unk), key=lambda o: -(known[o]+unk[o]))[:14]:
    print(f'{op:28s} {known[op]:7d} {unk[op]:7d}')
