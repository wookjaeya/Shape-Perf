import re,glob,json
exec(open('refs_data.py').read())
# assign final IDs to symbolic keys
order_groups=['rec','code','doc','tool']
fixed=set(e[0] for e in R if re.fullmatch(r'[RS]\d\d',e[0]))
nxt=26; keymap={}
for g in order_groups:
    for e in R:
        if e[1]==g and e[0] not in fixed:
            keymap[e[0]]=f'S{nxt}'; nxt+=1
def fid(k): return keymap.get(k,k)
url2id={}
for e in R:
    for u in e[3]:
        if '#L' not in u: url2id[u]=fid(e[0])
json.dump({'keymap':keymap,'url2id':url2id},open('refmap.json','w'),ensure_ascii=False,indent=0)
linkre=re.compile(r'\[((?:[^\[\]]|\[[^\]]*\])*)\]\((https?://[^)\s]+)\)')
for f in sorted(glob.glob('work/*.md')):
    t=open(f).read()
    def rl(m):
        txt,u=m.group(1),m.group(2)
        if u in url2id:
            i=url2id[u]
            if txt.strip()=='' or txt.strip()==u: return f'[{i}]'
            return f'{txt} [{i}]'
        return m.group(0)
    t=linkre.sub(rl,t)
    # dedupe
    for _ in range(2):
        t=re.sub(r'\[([RS]\d\d+)\] \[\1\]',r'[\1]',t)
        t=re.sub(r'\[([RS]\d\d+)\]; \1\)',r'[\1])',t)
        t=re.sub(r'\[([RS]\d\d+)\] \(\1\)',r'[\1]',t)
        t=re.sub(r'\[([RS]\d\d+)\]\(\1\)',r'[\1]',t)
        t=re.sub(r'\[([RS]\d\d+)\], \1(?=[);,])',r'[\1]',t)
        t=re.sub(r'\(\[([RS]\d\d+)\]\)',r'[\1]',t)
        t=re.sub(r'\(([RS]\d\d)\) \[\1\]',r'[\1]',t)
    # normalize bare (Sxx)/(Rxx) -> [Sxx]
    t=re.sub(r'\((R\d\d|S\d\d)\)',r'[\1]',t)
    open(f,'w').write(t)
print(nxt-1)
