import re,glob,collections,sys
stats=collections.Counter()
ex=[]
for ml in glob.glob('out/*.mlir'):
    txt=open(ml).read()
    locdefs=dict(re.findall(r'^(#loc\d*) = loc\((.*)\)$',txt,re.M))
    def resolve(l,depth=0):
        # expand #locN references recursively
        def rep(m):
            return resolve(locdefs.get(m.group(0),'unknown'),depth+1) if depth<10 else m.group(0)
        return re.sub(r'#loc\d+\b',rep,l)
    for line in txt.splitlines():
        m=re.match(r'^\s+(?:%[^=]+= )?(llvm\.(call|load)) .*loc\((.*)\)\s*$',line)
        if not m: continue
        op=m.group(1); l=resolve(m.group(3))
        fl=re.findall(r'"([^"]+)":(\d+):(\d+)',l)
        if fl and int(fl[-1][1])>0:
            stats[(op,'file:line>0')]+=1
            if int(fl[-1][2])==0: stats[(op,'col==0')]+=1
        else:
            stats[(op,'noloc')]+=1
            if len(ex)<5: ex.append((ml,line[:120],l[:200]))
for k,v in sorted(stats.items()): print(k,v)
for e in ex: print(e)
