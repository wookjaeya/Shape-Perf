import re,sys,glob,collections
APIS={'CFE_SB_Subscribe':0,'CFE_SB_SubscribeEx':0,'CFE_SB_SubscribeLocal':0,'CFE_SB_Unsubscribe':0,'CFE_SB_UnsubscribeLocal':0,'CFE_MSG_Init':1,'CFE_MSG_SetMsgId':1}
locre=re.compile(r'"([^"]*\.[ch])":(\d+):(\d+)')
for app in sys.argv[1:]:
    sites=collections.OrderedDict()
    for f in sorted(glob.glob(f'{app}/*.loc.mlir')):
        func=None; defs={}
        for line in open(f):
            m=re.match(r'\s*llvm\.func (?:internal |linkonce |weak )?@(\w+)\(',line)
            if m: func=m.group(1); defs={}; continue
            m=re.match(r'\s*(%\w+) = (.*)',line)
            if m:
                defs[m.group(1)]=m.group(2)[:200]
            m=re.search(r'llvm\.call @(\w+)\(([^)]*)\)',line)
            if m and m.group(1) in APIS:
                args=[a.strip() for a in m.group(2).split(',')]
                a=args[APIS[m.group(1)]]
                d=defs.get(a, 'ARG/param' if a.startswith('%arg') else '?')
                lm=locre.search(line)
                loc=f'{lm.group(1).split("/")[-1]}:{lm.group(2)}' if lm else '?'
                c=re.match(r'llvm\.mlir\.constant\((\d+) : i32\)',d)
                kind=('const '+c.group(1)) if c else d.split(' :')[0][:80]
                sites.setdefault((m.group(1),loc),set()).add((func,kind))
    print('==',app)
    for (api,loc),v in sites.items():
        kinds=sorted({k for _,k in v})
        print(f'  {api:24s} {loc:28s} {kinds}  in {sorted({fn for fn,_ in v})}')
