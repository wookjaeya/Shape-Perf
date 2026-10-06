from edit_lib import *
import re, json
V={c['cid']:c['verdict'] for c in json.load(open('verified.json'))}
n='10_claims.md'; t=load(n)
def norm(m):
    cid=m.group(1); rest=(m.group(2) or '').strip()
    extra=''
    mm=re.match(r'^(confirmed|refuted|contested)?\s*(?:\((.*)\))?$',rest)
    if not mm: return m.group(0)
    v=mm.group(1) or V[cid]
    if mm.group(2): extra=', '+mm.group(2)
    return f'| 검증 {cid} ({v}{extra}) |'
t=re.sub(r'\| (C\d\d)((?: (?:confirmed|refuted|contested))?(?:\([^|]*\))?) \|(?= §| 10\.| –| [0-9])', norm, t)
save(n,t)
print(len(re.findall(r'\| 검증 C\d\d \(',t)))
