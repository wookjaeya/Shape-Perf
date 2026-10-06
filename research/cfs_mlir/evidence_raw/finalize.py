import re, glob, sys, collections
N = sys.argv[1]; P = N + '/parts'; apply = len(sys.argv) > 2 and sys.argv[2] == 'apply'
parts = {p.split('/')[-1]: open(p, encoding='utf-8').read() for p in sorted(glob.glob(P + '/*.md'))}

def strip_fences(t):
    out, inf = [], False
    for l in t.split('\n'):
        if l.startswith('```'): inf = not inf; continue
        out.append('' if inf else l)
    return out

# 1) reference usage
refs = parts['11_sec.md']
defined = re.findall(r'^\| ([RS]\d{2,3}) \|', refs, re.M)
where = collections.defaultdict(set)
for n, t in parts.items():
    if n in ('11_sec.md', '00_front.md'): continue
    sec = int(n[:2])
    for x in re.findall(r'(?<![A-Za-z0-9\-])([RS]\d{2,3})(?![0-9])', t):
        where[x].add(sec)
undef = sorted(set(where) - set(defined), key=lambda x: (x[0], int(x[1:])))
unused = [d for d in defined if d not in where]
print('defined', len(defined), 'used', len(where), 'undefined:', undef, 'unused:', len(unused))
new_lines, changed = [], 0
for l in refs.split('\n'):
    m = re.match(r'^\| ([RS]\d{2,3}) \|', l)
    if m and l.count('|') == 6:
        cells = l.split('|')
        rid = m.group(1)
        usage = ', '.join(f'§{s}' for s in sorted(where[rid])) if where.get(rid) else '본문 인용 없음'
        new = cells[:5] + [f' {usage} ', '']
        nl = '|'.join(new)
        if nl != l: changed += 1
        l = nl
    new_lines.append(l)
print('usage cells changed:', changed)

# 2) TOC
toc = []
for n in sorted(parts):
    if not re.match(r'^\d\d_sec\.md$', n): continue
    for l in strip_fences(parts[n]):
        m2 = re.match(r'^## (\d+)\. (.*)$', l)
        m3 = re.match(r'^### (\d+\.\d+) (.*)$', l)
        if m2: toc.append(f'- §{m2.group(1)} {m2.group(2).strip()}')
        elif m3: toc.append(f'  - {m3.group(1)} {m3.group(2).strip()}')
front = parts['00_front.md']
fm = re.search(r'(## 목차\n\n)(.*?)(\n\n---\n)', front, re.S)
old_toc = fm.group(2).split('\n')
print('toc lines old/new:', len(old_toc), len(toc))
for x in sorted(set(toc) - set(old_toc)): print('  + ', x)
for x in sorted(set(old_toc) - set(toc)): print('  - ', x)
if apply:
    open(P + '/11_sec.md', 'w', encoding='utf-8').write('\n'.join(new_lines))
    front = front[:fm.start(2)] + '\n'.join(toc) + front[fm.end(2):]
    open(P + '/00_front.md', 'w', encoding='utf-8').write(front)
    print('applied')
