# count llvm.call @CFE_*/OS_* sites in the opt-linked sample_app module, raw vs dedup by innermost file:line:col
import re,sys,collections
src=open(sys.argv[1]).read().split('\n')
alias={}
for ln in src:
    m=re.match(r'^(#[\w.$-]+) = (.*)$',ln)
    if m: alias[m.group(1)]=m.group(2)
def resolve(l,d=0):
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
    return '?'
raw=collections.Counter(); sites=collections.defaultdict(set); noloc=0
for ln in src:
    m=re.search(r'llvm\.call @((?:CFE|OS)_\w+)\(',ln)
    if not m: continue
    raw[m.group(1)]+=1
    lm=re.search(r'loc\((#loc\d+)\)\s*$',ln)
    s=resolve(lm.group(1)) if lm else 'noloc'
    if s in('unknown','noloc','?'): noloc+=1
    sites[m.group(1)].add(s)
for k in sorted(raw): print('%-28s raw %2d  distinct %2d  %s'%(k,raw[k],len(sites[k]),' '.join(sorted(sites[k]))[:150]))
print('total raw',sum(raw.values()),'distinct',sum(len(v) for v in sites.values()),'no-loc',noloc)
