import json,re,sys,collections
src=open(sys.argv[1]).read().split('\n')
alias={}
for ln in src:
    m=re.match(r'^(#[\w.$-]+) = (.*)$',ln)
    if m: alias[m.group(1)]=m.group(2)
def resolve(l,d=0):
    """return innermost file:line:col for a loc expression string or alias"""
    if d>30: return '?'
    if l.startswith('#loc'): return resolve(alias.get(l,'loc(unknown)'),d+1)
    m=re.match(r'loc\("([^"]+)":(\d+):(\d+)\)',l)
    if m: return '%s:%s:%s'%(m.group(1).split('/')[-1],m.group(2),m.group(3))
    m=re.match(r'loc\(callsite\((#loc\d+|loc\([^)]*\)) at',l)
    if m: return resolve(m.group(1),d+1)
    m=re.match(r'loc\(fused<[^>]*>\[(#loc\d+)',l)
    if m: return resolve(m.group(1),d+1)
    m=re.match(r'loc\(fused\[(#loc\d+)',l)
    if m: return resolve(m.group(1),d+1)
    if 'unknown' in l: return 'unknown'
    return '?:'+l[:40]
res=json.load(open(sys.argv[2]))
sites=collections.defaultdict(set)
unk=0
for r in res:
    ln=src[r['line']-1]
    m=re.search(r'loc\((#loc\d+)\)\s*$',ln)
    s=resolve(m.group(1)) if m else 'noloc'
    if s in ('unknown','noloc') or s.startswith('?'): unk+=1
    sites[(r['path'],r['op'],r['how'])].add(s)
tot=0
for k,v in sorted(sites.items()):
    tot+=len(v); print(len(v),k,sorted(v))
print('distinct source sites:',tot,' ops without a resolvable file:line:col:',unk)
