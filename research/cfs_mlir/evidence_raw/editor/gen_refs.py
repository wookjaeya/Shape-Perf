import re,glob,json
exec(open('refs_data.py').read())
km=json.load(open('refmap.json'))['keymap']
def fid(k): return km.get(k,k)
texts={f[5:7]:open(f).read() for f in sorted(glob.glob('work/*.md'))}
def pats(urls):
    ps=[]
    for u in urls:
        u=u.split('#')[0]
        m=re.match(r'https://(?:github\.com|raw\.githubusercontent\.com)/([^/]+)/([^/]+)/(tree|commit)/([^/]+)$',u)
        if m:
            o,r_,_,sha=m.groups()
            ps+= [f'{o}/{r_}/blob/{sha}',f'{o}/{r_}/tree/{sha}',f'{o}/{r_}/commit/{sha}']
        elif '/blob/' in u or 'raw.githubusercontent' in u:
            ps.append(u.replace('https://',''))
    return ps
def where(i,urls=()):
    secs=[]
    ps=pats(urls)
    for k,t in texts.items():
        if k in ('11','00'): continue
        if re.search(r'(?<![A-Za-z0-9])'+re.escape(i)+r'(?![0-9])',t) or any(p in t for p in ps): secs.append('머리' if k=='00' else '§'+str(int(k)))
    return ', '.join(secs)
def sub(sc): return re.sub(r'\{(\w+)\}',lambda m:fid(m.group(1)),sc)
def esc(x): return x.replace('|','\\|')
groups=[('R','11.1 학술 자료','R01–R11은 수정본 §14.1의 ID이고, R12부터는 이 노트에서 더한 자료다. 학회·연도만 적은 항목은 검색 snippet으로만 본 자료이며, 서지 세부를 확인하지 못했다는 뜻이다.'),
 ('rec','11.2 사건·이슈·변경 기록','S01–S07은 수정본 §14.2의 ID다. GitHub issue 본문은 WebFetch 요약과 검색 metadata로 읽었고, Trac에서 이식된 댓글은 열지 못했다 (§2.0, §2.4.1).'),
 ('code','11.3 고정한 소스 snapshot','S08–S12, S25는 수정본 §14.3의 ID다. 줄 번호가 의미 있는 code permalink는 본문에 그대로 두었다. 이 표는 그 permalink가 가리키는 repository와 commit, 확인한 파일을 모은다.'),
 ('doc','11.4 공식 문서와 발표 자료','S13–S20은 수정본 §14.4의 live 문서다. 구현 단계에서는 문서 snapshot이나 revision을 따로 기록해야 한다.'),
 ('tool','11.5 공개 앱과 분석 도구','S21–S24는 수정본 §14.5의 ID다. README만 읽은 항목은 그 범위를 적었다.')]
def keyorder(e):
    i=fid(e[0]); return (i[0], int(i[1:]))
out=['## 11. 참고문헌','',
 '**ID 규칙.** 수정본(2026-10-01)의 ID R01–R11, S01–S25는 같은 자료에 그대로 쓴다. 새 학술 자료는 R12부터, 새 코드·문서·사건 자료는 S26부터 번호를 붙였다. 본문의 `[ID]`는 이 절의 항목을 가리킨다. 줄 번호가 의미 있는 code permalink는 본문에 그대로 두고, 그 repository·commit을 11.3과 11.5에 모았다. "확인 범위"는 이 노트(또는 수정본 기록)가 실제로 읽은 범위다. snippet은 검색 색인의 요약만 보았다는 뜻이고 결론의 근거로 쓰지 않았다. "인용"은 그 ID가 나오는 절이다.','']
for g,title,intro in groups:
    out+=['### '+title,'',intro,'','| ID | 서지·식별자 | 위치 | 확인 범위 | 인용 |','| --- | --- | --- | --- | --- |']
    es=[e for e in R if e[1]==g]
    es.sort(key=keyorder)
    for e in es:
        i=fid(e[0])
        loc='<br>'.join(f'<{u}>' for u in e[3]) if e[3] else '(URL 없음)'
        out.append(f'| {i} | {esc(e[2])} | {loc} | {esc(sub(e[4]))} | {where(i,e[3]) or "본문 인용 없음 (수정본 ID 유지)"} |')
    out.append('')
open('work/11_refs.md','w').write('\n'.join(out).rstrip()+'\n')
print(len(out))
