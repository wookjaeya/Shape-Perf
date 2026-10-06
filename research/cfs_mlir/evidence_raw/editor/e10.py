from edit_lib import *
import re, json
n='10_claims.md'; t=load(n)
t=rep_between(t,'**표기.** 근거 표기는','---\n\n### 10.1','')
# claim list table
V={c['cid']:c['verdict'] for c in json.load(open('verified.json'))}
subj={'C01':'fprime-topo-analysis의 입력·검사 범위','C02':'ROSDiscover(ICSA 2022)와 ROSInfer의 범위','C03':'Ganesan·Lindvall 그룹의 cFS 검증 선례(ISSRE 2016, SPLC 2009)','C04':'TaxDC의 정의와 범위','C05':'Burrows–Leino의 stale-value 정의','C06':'Ogma cFS monitor template의 구독·평가 구조','C07':'IKOS의 검사 범위와 가정','C08':'Goblint v1.1.0의 ARINC 653·OSEK 분석','C09':'SB 전달 실패가 발신자에게 보이지 않음','C10':'SB 전달 순서, sequence counter, lock 위치','C11':'ES startup의 timeout과 준비 비보장','C12':'core startup 직렬화와 #73 경로','C13':'`NEVER_LOADED`의 포인터와 lock','C14':'TBL handle 단위 잠금과 buffering','C15':'재시작의 비동기 요청과 teardown','C16':'data·설정·명령이 정하는 구독 관계','C17':'bundle build와 154개 파일 import 조건','C18':'IR에서 MID 이름 소실과 상수성','C19':'로컬 ClangIR 사용 불가','C20':'upstream MLIR의 alias·modref·visibility 동작','C21':'cFE #73 기록과 수정 이력','C22':'Swift/BAT circular와 cFS의 관계'}
rows='\n'.join(f'| 검증 {k} | {subj[k]} | {V[k]} |' for k in sorted(subj))
t=rep(t,'**규칙.** refuted 주장은 원래 문장으로 쓰지 않는다.',
'''**검증 claim 목록.** 노트 본문의 '검증 Cnn'은 아래 claim을 가리킨다. refuted·contested와 결론에 영향을 준 보정은 10.3에 정리했다.

| ID | 대상 | 판정 |
| --- | --- | --- |
'''+rows+'''

**규칙.** refuted 주장은 원래 문장으로 쓰지 않는다.''')
# 10.3 extra rows (old IDs, renamed below)
t=rep(t,'| C02 | ROSInfer의 state 변수는 subscriber callback에서 대입되는 변수다 |',
'''| C01 | fprime-topo-analysis는 약 15개 검사를 하고 메시지 순서 분석이 없다 | confirmed, 보정 | `checks.py` 17개와 별도 analyzer 3개로 약 20개다. queue 우선순위·지연 분석은 있다. 없는 것은 state로의 순서와 freshness 분석이다. commit 날짜는 도구의 기원을 말하지 않는다 | P4 |
| C07 | IKOS는 "external functions do not update global variables"를 가정한다 | confirmed, 보정 | README 원문은 "Extern functions (without implementation)"이다. stub을 붙이면 그 함수는 분석된다. 남는 한계는 task 간 순서다 | P6 |
| C02 | ROSInfer의 state 변수는 subscriber callback에서 대입되는 변수다 |''')
# O29
i=t.find('| O28 |'); j=t.find('\n',i)
t=t[:j+1]+'| O29 | §54 | 관련 연구는 7개 무리다 | 보정 | §4의 다섯 무리(flight software 검증, pub/sub 구조 복원, 순서·race 이론과 도구, cause-effect chain, MLIR·대안 분석 기반)로 다시 묶는다. \'framework topology + handler 흐름 결합 도구\'와 \'cause-effect chain\'을 더한다. message race는 §4.3.3의 한 행이다 | §4.1–§4.5 |\n'+t[j+1:]
# ---- rename claim IDs
pref={'M':'CM','S':'CS','E':'CE','T':'CT','G':'CG','X':'CX','L':'CL','P':'CP','D':'CD','R':'CR','V':'RV'}
lines=t.split('\n')
out=[]
for l in lines:
    # row ids
    m=re.match(r'^\| ([MSETGXLPDRV])(\d{1,2}) \|',l)
    if m:
        l='| '+pref[m.group(1)]+m.group(2)+' |'+l[m.end():]
    out.append(l)
t='\n'.join(out)
protect=['`E1 \\|\\| E2`','R1–R4']
for k,p in enumerate(protect): t=t.replace(p,f'@@P{k}@@')
# S single digit (1-9), not S0x; S10/S11 handled manually
t=re.sub(r'(?<![A-Za-z0-9_./\-@])S([1-9])(?![0-9])', r'CS\1', t)
t=re.sub(r'(?<![A-Za-z0-9_./\-@])([MEGXPDT])(\d{1,2})(?![0-9\-])', lambda m: (('C'+m.group(1)) if m.group(1)!='X' or True else '')+m.group(2) if True else '', t)
t=re.sub(r'(?<![A-Za-z0-9_./\-@\[])L([1-8])(?![0-9\-])', r'CL\1', t)
for k,p in enumerate(protect): t=t.replace(f'@@P{k}@@',p)
# manual S11 claim refs
t=rep(t,'버려졌다(S11)','버려졌다(CS11)')
t=rep(t,'| S11 해석,','| CS11 해석,')
save(n,t); print('ok')
