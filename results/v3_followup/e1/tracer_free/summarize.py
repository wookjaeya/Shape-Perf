import json,glob,os,sys
d=sys.argv[1]
for f in sorted(glob.glob(os.path.join(d,'*.json'))):
    if f.endswith('.spec.json') or f.endswith('index.json'): continue
    r=json.load(open(f))
    print(f"== {os.path.basename(f)[:-5]}  TracerPid={r['tracer_pid']} LD={r['env_LD']}")
    for s in r['log']:
        if s['step']=='entry-check':
            print("   entry(dlsym handle): "+", ".join(f"{n}->{e[k]['handle_classified']}" for n,e in s['entries'].items() for k in e if k.startswith('run_main_graph')))
            continue
        ok = f" exact={s['output_exact']}" if 'output_exact' in s else ''
        parts=[]
        for n,g in s['got'].items():
            parts.append(n+"{"+"; ".join(f"{k.split('_main_graph')[0] if 'ciface' in k else ('entry' if k.startswith('run_') else 'compute')}->{v['classified'].replace(' (UNRESOLVED lazy stub)','[lazy]')}" for k,v in g.items())+"}")
        print(f"   {s['step']:<16}{ok:<12} "+"  ".join(parts))
