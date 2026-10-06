import re, sys, collections
txt = open(sys.argv[1]).read()
defs = dict(re.findall(r'^(#loc\d*) = loc\((.*)\)$', txt, re.M))
FLC = re.compile(r'"[^"]+":\d+:\d+')
def resolve(s, depth=0):
    # expand #locN aliases recursively
    if depth > 20: return s
    return re.sub(r'#loc\d+', lambda m: resolve(defs.get(m.group(0), 'unknown'), depth+1) if m.group(0) in defs else m.group(0), s)
res = collections.defaultdict(lambda: [0, 0, 0])  # total, has_file_line_col, no_loc_on_line
examples = collections.defaultdict(list)
for line in txt.splitlines():
    m = re.match(r'^\s+(?:%[^=]+= )?"?(llvm\.(?:call|load|store|invoke))\b', line)
    if not m: continue
    op = m.group(1); res[op][0] += 1
    lm = re.search(r'loc\((.*)\)\s*$', line)
    if not lm: res[op][2] += 1; continue
    r = resolve(lm.group(1))
    if FLC.search(r): res[op][1] += 1
    elif len(examples[op]) < 3: examples[op].append(r[:200])
for op, (t, k, n) in sorted(res.items()):
    print(f'{op:12s} total={t} with_file:line:col={k} no_loc_suffix={n}')
for op, ex in examples.items(): print(op, ex)
