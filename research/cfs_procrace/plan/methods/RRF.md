# RRF (ISSRE 2016) 분석 — Kairos 비교용

- 작성일: 2026-10-15 (UTC 08:51–09:06, 경로 시도와 읽기 시각).
- 보완: 2026-10-16 (UTC 00:15–00:40). `plan/process_race_method_matrix.md` §5.1의 후속 항목 중 RRF에 걸린 것(A5 원문 경로 재시도, A6 내부 oracle 논문, A10 쪽 번호 인용의 둘째 취득 경로)을 다시 시도했다(§1.1 경로 21–29). 저장한 RRF 추출 텍스트와 인용 위치를 다시 대조했다. R2 수치의 근거를 gdb 원시 기록으로 바꿨고, R2 사건표에 task 생성 행(⓪)을 넣고 ③ 칸을 Fig. 2의 기록 형식으로 보강했다(§3.5). 바뀐 것은 §1.3 항목 7–12에 적었다. 이번에도 RRF를 실행하지 않았다.
- 대상: S. Rattanasuksun, T. Yu, W. Srisa-an, G. Rothermel, *RRF: A Race Reproduction Framework for Use in Debugging Process-Level Races*, ISSRE 2016, pp. 162–172. DOI 10.1109/ISSRE.2016.35.
- 범위: 로드맵 §10 (M6)의 RRF 항목. 여섯 분석 질문(§10.2)을 사례 C1, C2, C3, C5, R2에 적용한다. 로드맵 §12 작업 6(`process_race_method_matrix.md`)의 입력으로 쓴다.
- 이 문서의 성격: **분석 문서**다. RRF를 실행하지 않았다(실행할 구현이 없다). 어느 사례에 대해서도 "재현하지 못한다" 또는 "탐지하지 못한다"는 결과를 담지 않는다.
- 용어: 신규 문서이므로 T1 (a)를 따른다. 판정 대상은 readiness specification(준비 상태 명세), 그 전제는 readiness precondition, 위반은 readiness violation이다. "API contract"는 문서화된 API 의미를 가리킬 때만 쓴다. 원 논문의 "race", "order violation", "oracle"은 논문의 용어 그대로 쓴다.
- 조건 원칙: `CONDITIONS_POLICY.md`. 이 문서는 실험 조건을 정하지 않는다. 논문이 자기 평가에서 쓴 값(sleep 1초, 반복 10회 등)은 논문의 값이며 cFS 조건으로 가져오지 않는다. 레퍼런스가 없는 값은 `unspecified_by_reference`로 둔다.
- PI 결정(`PI_DECISIONS_RESOLVED_20261015.md`)은 결정으로만 인용한다. 사실로 쓰지 않는다. 이 문서가 쓰는 결정은 P-PAPERS, T1, P-CLASSGT(원출처의 "race"를 도구 class로 자동 변환하지 않는다)이다.
- 같은 계보의 다른 문서: `plan/methods/RacePro.md`, `plan/methods/SimRacer.md`(같은 형식). RRF는 Scribe(RacePro의 기록 도구)를 관측에 쓰고(§2) SimRacer와 같은 저자 계보라서 두 문서와 겹치는 부분은 참조만 한다.

## 증거 태그와 근거 등급

| 태그 | 뜻 | 이 문서에서의 쓰임 |
|---|---|---|
| [F] | 인용이 있는 사실 | 원출처(논문 쪽·절) 또는 저장소 기록(문서·절). 저장소 기록은 "기록"이라고 표시하며 이번에 재검증하지 않았다 |
| [M] | 측정 | 오늘(2026-10-15) 이 컨테이너에서 읽기 전용으로 확인한 값 |
| [I] | 추론·분석 | 설계 기술에서 도출한 해석. 도구를 실행하지 않았다 |
| [D] | 결정 | PI 결정 |
| [U] | 미확인 | 자료가 없거나 읽지 못함 |

문장마다 출처 등급을 아래 다섯 가지 중 하나로 붙인다.

| 등급 | 뜻 |
|---|---|
| `[전문]` | 본문 전체를 텍스트로 추출해 읽었다. Figure 1은 이미지로도 확인했다 |
| `[초록]` | 초록·서지만 읽었다 |
| `[2차]` | 다른 논문·페이지가 이 도구를 서술한 문장을 읽었다. 도구 자신의 서술이 아니다 |
| `[기록]` | 저장소의 이전 보고서에 있는 값. 이번에 재검증하지 않았다 |
| `[미도달]` | 열지 못했다 |

쪽 번호 표기: **RRF p.N**은 CPS-VO 아카이브가 올린 PDF(11쪽)의 PDF 쪽이다. ISSRE 인쇄 쪽(162–172)은 11쪽이라 쪽 수는 같지만, 이 PDF가 IEEE 출판본과 같은 판본인지는 확인하지 못했다(IEEE 사본에 접근 불가, §1.1 경로 3, 24). **RRF 쪽 번호 인용은 단일 취득 경로이며 독립 재확인이 없다.** 2026-10-16에 둘째 경로를 다시 찾았으나 얻지 못했다(§1.1 경로 21–29, §1.3 항목 7). **SysPro p.N, DESCRY p.N**은 각 PDF의 쪽이다.

---

## 0. 한눈에 보기

1. **RRF 논문 전문을 읽었다** `[전문]`. CPS-VO 아카이브 사본(11쪽)이 열린다. 이전 기록(`baselines/yu-group.md`, `baselines/availability.md`)은 RRF를 "검색 요약(paraphrase)만"으로 적었고, 그 등급이 올라갔다. 구현과 아티팩트는 찾지 못했다(§1.1, §4). IEEE 출판본(AWS WAF 도전)과 저자 학위논문(UNL 2018)의 PDF(Cloudflare 도전)는 열리지 않았다. 학위논문은 초록만 읽었다. `[초록]`
2. **역할은 "보고된 race의 재현"이다.** 논문이 스스로 "race detection system이 아니라 race reproduction system"이라고 쓴다(p.3). 로드맵 §10.1의 역할 칸("보고된 race의 재현")이 전문으로 확인된다. 탐지 도구와 같은 표에 점수로 합치지 않는다.
3. **실행 단위는 OS process 쌍이고 관측은 system-level event다.** 입력은 프로그램 둘 이상(PuD), 설정 파일, bug report, test input, 실패 출력이다(p.3, p.7). 관측은 Scribe가 만드는 system call·resource lock 기록이다(p.2–4, Fig. 2). "yield point"는 이 기록에서 도출한 소스 위치에 `sleep`을 컴파일 시점 조건으로 넣는 것이다(p.5–6). 자유로운 위치 지정이 아니다.
4. **논문의 race 정의가 order violation을 포함한다(p.2).** "기존 기법은 순서위반을 못 다룬다"는 문장은 RRF에 대해서도 쓸 수 없다(로드맵 §3.3과 같은 규칙). 평가는 file·socket 자원 7쌍에 한정된다(p.6).
5. **여섯 질문의 답은 도구 수준에서 대부분 서술이 있다.** 필요한 순서는 입력 필드가 아니다(p.3, p.7). 허용 오류·재시도·등록·초기화 상태·세대(generation)에 관한 서술은 논문에 없다(키워드 검색 결과 `retry`, `regist`, `semaphore`, `futex` 0건). 서술이 없다는 것은 기능이 없다는 확인이 아니다.
6. **다섯 사례의 의존성 쌍은, RRF 이벤트 로그에 들어온다면 이 설계의 "같은 자원, 두 process, 두 순서" 모양과 맞는다 (분석, 미실행).** C1·C3·C5·R2의 공개와 사용은 cFE/OSAL 메모리 상태의 쓰기와 읽기이고, OSAL 자체 시험에서 C1의 생성·조회는 syscall 0건이었다(`availability.md` §4.2 `[기록]`). RRF의 후보 쌍은 syscall·resource lock 기록에서 나온다(p.3–5). 이 경로가 사례의 사건을 포함하는지는 자료로 정해지지 않는다. 한 process 안의 task 쌍을 PuD 쌍으로 줄 수 있는지도 서술이 없다.
7. **실행은 불가능하고 오늘 적용되는 상태는 다섯 사례 모두 `NOT_RUN`이다.** RRF 구현을 찾지 못했고, 논문의 관측 도구 Scribe는 패치된 Linux 커널이 필요한데 이 컨테이너는 다른 커널을 띄울 수 없다(`availability.md` §1.1 `[기록]`; 오늘 설정 파일로 재확인 `[M]`). C2는 앱 소스가 비공개라 별도 사유가 더해진다. `INAPPLICABLE`은 PI 판단이다(§4.3).
8. **Kairos 가설은 근거가 있는 만큼만 올렸다(§5).** 근거가 지지하는 것은 "cFS 내부 사건이 후보 생성 입력에 들어오는가"(H-RRF-1)와 "시간 기반 sleep 대신 사건 기반 제어"(H-RRF-4, 차이일 뿐 결핍이 아님)이다. 반사실 순서가 스케줄로 만들어지지 않는 경우의 설명(H-RRF-5)은 R2의 순서 성격이 미결이라 부분 근거다. 쌍 열거, 실행 확인, 확장 가능한 oracle 저장소는 RRF에 이미 있으므로 차별점으로 쓰지 않는다(§5.1).
9. **2026-10-16 보완.** (a) 내부 oracle 논문(RRF의 [36], ISSRE 2013)은 초록만 읽었다 `[초록]`. UNL 기관 저장소의 PDF와 IEEE 사본이 도전 페이지에 막혔다(§1.1 경로 25–26). H-RRF-3의 조건은 그대로 남는다. (b) 저자 학위논문 PDF는 다시 막혔다. (c) RRF PDF의 둘째 취득 경로는 찾지 못했다. 같은 호스트의 파일 URL은 기본 `curl`에도 WAF 거부(245 B)를 돌려줬다(경로 22). (d) 논문 p.1은 process-level race를 "multiple processes access operating system resources"로 정의하고 thread 수준 탐지기는 이런 race를 다루지 못할 수 있다고 적는다. 2026-10-15판은 p.10만 썼다(§1.3 항목 8). (e) R2 수치를 gdb 원시 기록으로 바꿨고, 그 기록은 거부 경로에서 "no syscall was made"라고 적는다(§3.5 R2).

---

## 1. 접근한 출처와 문장별 근거 등급

### 1.1 시도한 경로와 결과

시각은 경로 1–20이 2026-10-15 UTC 08:51–09:06, 경로 21–29가 2026-10-16 UTC 00:15–00:25다. 접속 실패는 원인을 기록한 것까지만 썼고, TLS 검증을 끄거나 프록시·도전 페이지를 우회하려고 하지 않았다.

| # | 경로 | 결과 | 등급·비고 |
|---|---|---|---|
| 1 | CPS-VO 레코드 `archive.cps-vo.org/node/30361` | WebFetch: 200. 제목·저자·초록, 첨부 `Rattanasuksun_RRF_JA.pdf`(563.3 KB, 파일 페이지 `node/30362`, 다운로드 `/file/30362/download/194493`). 페이지에 라이선스 표기는 없고(약관 링크뿐) 코드·아티팩트 링크도 없다. `curl`로 같은 페이지를 받으면 245 B "Request Rejected"(호스트 WAF)가 온다 | `[초록]` + 파일 위치 |
| 2 | 위 첨부 파일 | 같은 호스트가 `curl`로 받은 레코드 페이지를 거부했으므로(경로 1) 파일은 WebFetch 도구로 받았다. 파일 URL을 `curl`로 직접 시도하지는 않았다. PDF 576,818 B, 11쪽, pdfTeX, 생성 2016-08-19, 수정 2016-12-08. sha256 `e8c26e2fbca15e87c9eb837c1bddc0ade0493fb261f3ebd2fbbc200c06f00dce`. `pdftotext`로 쪽별 추출, Figure 1은 이미지로 확인. 표 II–VI는 레이아웃 추출로 숫자를 대조했다 | **`[전문]`** |
| 3 | IEEE Xplore `ieeexplore.ieee.org/document/7774517` | `curl`: 301 후 HTTP 202, 헤더 `x-amzn-waf-action: challenge`, 본문 비어 있음. WebFetch: 빈 내용 | `[미도달]` 출판본 |
| 4 | `doi.org/10.1109/ISSRE.2016.35` | 302로 IEEE Xplore로 이동, 경로 3과 같은 도전 | `[미도달]` |
| 5 | OpenAlex `api.openalex.org/works/doi:…` | 200. 초록과 서지. `is_oa: false`, `any_repository_has_fulltext: false` | `[초록]`. 초록은 논문 p.1과 같다 |
| 6 | Crossref `api.crossref.org/works/…` | 200. 쪽 162–172, 참고문헌 38건, 초록 없음. 전문 링크는 IEEE 한 곳 | 서지 |
| 7 | Semantic Scholar Graph API | 200. `openAccessPdf.status: CLOSED`, 초록·참고문헌은 출판사 요청으로 생략됨. 이 논문을 인용한 6건: SECite(2026), SysPro(2026), "Update with care"(JSS 2022), "Ad hoc test generation through binary rewriting"(SCAM 2020), DESCRY(2017), Rattanasuksun 학위논문(2018) | 서지 |
| 8 | DBLP 검색 API | HTTP 202 "Making sure you're not a bot!" 도전 페이지 | `[미도달]` |
| 9 | ResearchGate 레코드 `publication/311531842` | WebFetch: 403. WebSearch가 레코드의 존재와 쪽수만 알려주고 전문은 없다고 요약했다(열지 못한 페이지의 요약) | `[미도달]` |
| 10 | 저자 홈 `homepages.uc.edu/~tyu/`, `links/research.html` | 200. 논문 목록의 RRF 항목은 IEEE Xplore 7774517 링크뿐이다. 홈과 연구 페이지의 링크를 확인했고 소프트웨어·코드·다운로드 링크가 없다(홈이 링크하는 `links/projects.html`은 이번에 열지 않았다. 2026-10-15의 `SimRacer.md` §1.1 경로 8이 열어서 소프트웨어 항목이 없다고 적었다). 새 홈 `tingting-yu.scholar.uconn.edu`는 연결이 끊김(curl 35, `Connection reset by peer`) | 서지. 구현 없음 |
| 11 | 저자 학위논문(Rattanasuksun, UNL 2018, *A Comprehensive Framework to Replicate Process-level Concurrency Faults*). 공개 레코드 `digitalcommons.unl.edu/computerscidiss/160`, ProQuest 사본 레코드 `…/dissertations/AAI13418914` | 두 레코드 모두 200. 초록, 지도교수(Srisa-an, Rothermel), 2018-11만 있다(공개 레코드에 목차·도구 설명·코드 언급 없음). PDF `viewcontent.cgi?article=1182&context=computerscidiss`(레코드가 1.4 MB로 표기)와 `…article=15671&context=dissertations`는 `curl` 403 `cf-mitigated: challenge`, WebFetch 403. 도전을 우회하지 않았다. ProQuest 사본 페이지는 UNL 구성원의 프록시 접속과 비구성원의 도서관 상호대차를 안내한다 | `[초록]`. 본문 `[미도달]` |
| 12 | 공저자 홈 `cse.unl.edu/~grother/`, `~witty/` | `SSL certificate problem: unable to get local issuer certificate`. TLS 검증을 끄지 않았다 | `[미도달]` |
| 13 | GitHub(MCP 검색) | 저장소 검색 `"race reproduction framework"` 0건, `RRF process-level race reproduction yield points scribe` 0건. 코드 검색 `"Race Reproduction Framework" Rattanasuksun` 6건, 모두 학회·저자 서지 데이터 파일을 모은 무관한 저장소다. 사용자 검색 `Rattanasuksun` 0건. 인증 없는 `api.github.com` 검색을 `curl`로 하면 프록시가 403을 돌려준다 | 구현 없음. 이름이 다른 저장소는 이 검색으로 찾지 못한다 |
| 14 | Software Heritage 원본 검색 `RRF`(URL 부분 일치) | 한 쪽 70건. 모두 URL에 `rrf` 문자열이 들어간 무관한 원본이다(URL에 race·repro·scribe·yield·procrace·process·supat·rattan·yu·unl이 들어간 것이 있는지 걸러 확인했고 없었다) | 구현 없음 |
| 15 | Zenodo 레코드 검색, Internet Archive 전문 검색, Wayback 가용성 API, CORE 검색 페이지, figshare | Zenodo 0건, Internet Archive `numFound: 0`, Wayback 429, CORE 403(Cloudflare 도전), figshare는 보낸 요청이 검색 형식이 아니어서 404(검색하지 못함) | 사본·구현 없음. figshare는 확인하지 못함 |
| 16 | Scholar Gateway `semanticSearch` | `ACCESS_DENIED: Access denied`(2026-10-08 기록과 같다) | `[미도달]` |
| 17 | WebSearch 5회(standard 4, extended 1) | 새 1차 출처 없음. 나온 곳은 CPS-VO 레코드, ResearchGate 목록, SysPro arXiv, 학위논문 레코드, UKy 학자 페이지. "RRF 구현 저장소·다운로드는 찾지 못했다"는 취지의 결과다(검색 요약) | — |
| 18 | Unpaywall API | **쓰지 않았다.** API가 이메일 주소를 요청 변수로 요구하고, 사용자의 주소를 이 용도의 외부 서비스에 보내지 않는다 | — |
| 19 | 2차 문헌: SysPro `arxiv.org/pdf/2601.09616v1`(47쪽, sha256 `8fe5daa4…e726`), DESCRY `chaowang-vt.github.io/pubDOC/YuZW17.pdf`(11쪽, sha256 `f9d7b57b…bf60`), SimEvo `par.nsf.gov/servlets/purl/10089644`(12쪽, sha256 `143f7351…31be6`) | 세 PDF 모두 200. 텍스트를 추출해 `RRF`, `Rattanasuksun`, `Race Reproduction`을 검색했다. SysPro와 DESCRY에 언급이 있다(§2.4). SimEvo에는 없다 | `[2차]` |
| 20 | RRF 논문이 인용한 Scribe 이슈 [31] `columbia/linux-2.6-racepro#3`, [32] `columbia/racepro#6` | `gh api`가 403(이 세션에 해당 저장소 접근 없음). 논문의 서술만 읽었다(p.9) | `[미도달]`. 부차적 |
| 21 | (2026-10-16 00:17Z, 기본 `curl`, 30–40초 timeout) 저자 새 홈 `tingting-yu.scholar.uconn.edu`와 `/publications/`, `/research/` | 세 쪽 모두 200(10-15에는 연결 끊김). Publications 쪽 본문은 Google Scholar 프로필 링크뿐이고 Research 쪽은 주제 목록뿐이다. PDF·코드·다운로드 링크 없음 | 구현 없음 |
| 22 | CPS-VO 레코드 `node/30361`와 파일 URL `file/30362/download/194493`(기본 `curl`) | 둘 다 HTTP 200에 본문 245 B "Request Rejected"(WAF). 10-15에는 파일 URL을 `curl`로 직접 시도하지 않았다(경로 2). 클라이언트 문자열을 바꾸거나 다른 도구로 재요청하지 않았다 | 같은 호스트이므로 둘째 경로가 아니다 |
| 23 | 저자 CV `homepages.uc.edu/~tyu/CV/tyu_cv.pdf`(UC 홈의 projects 쪽이 링크) | 200, PDF. sha256 `61225fc2…36c5`. 학회 논문 C[14]가 RRF(저자 표기 "Rattanasuksan"), C[6]이 내부 oracle 논문(ISSRE 2013, pp. 11–20)이다. 둘 다 서지뿐이고 아티팩트·코드 링크가 없다 | 서지. 구현 없음 |
| 24 | IEEE Xplore `document/7774517`(RRF), Semantic Scholar Graph API(RRF) | IEEE: HTTP 202, 본문 0 B(경로 3과 같은 도전). S2: `openAccessPdf.status: CLOSED` | 둘째 경로 없음 |
| 25 | 내부 oracle 논문 [36]: Crossref 서지 검색, Semantic Scholar, IEEE `document/6698900` | Crossref: DOI `10.1109/ISSRE.2013.6698900`, 쪽 11–20, 링크는 IEEE 한 곳. S2: `CLOSED`. IEEE: 202 도전 | 서지 |
| 26 | 내부 oracle 논문 [36]: UNL 기관 저장소 레코드 `digitalcommons.unl.edu/cseconfwork/255`와 PDF `viewcontent.cgi?article=1276&context=cseconfwork`, WebSearch 1회 | 레코드 200(초록만). PDF는 `curl` 403 `cf-mitigated: challenge`, WebFetch 403. 우회하지 않았다. WebSearch는 이 레코드와 UKy 학자 페이지(초록·서지)만 냈다 | **`[초록]`**. 본문 `[미도달]` |
| 27 | 저자 학위논문 PDF `viewcontent.cgi?article=1182&context=computerscidiss`, 레코드 `computerscidiss/160`, WebSearch 1회 | PDF 403 `cf-mitigated: challenge`(경로 11과 같다). 레코드 200. WebSearch는 같은 레코드(UNL 구성원 한정 다운로드 안내)와 CPS-VO 레코드만 냈다 | 본문 `[미도달]` 그대로 |
| 28 | OpenAlex 검색, Scholar Gateway `semanticSearch` | OpenAlex: HTTP 429(무키 일일 한도 소진 응답). Scholar Gateway: `ACCESS_DENIED: Access denied` | `[미도달]` |
| 29 | GitHub(MCP) 저장소 검색 `process-level race reproduction sleep injection scribe`, 코드 검색 `"Rattanasuksun" race` | 저장소 0건. 코드 7건이며 모두 서지 데이터 파일이거나 무관한 파일이다(예: `morningD/lang.csconf`의 학회 소속 데이터) | 구현 없음 |

**읽은 출처의 요약.** 본문 전체를 읽은 것은 RRF 논문(경로 2) 한 편이다. SysPro와 DESCRY는 RRF를 서술한 문장만 읽었다. 저자 학위논문은 PDF가 도전 페이지에 막혔고, 이 문서는 학위논문 본문의 어떤 내용도 쓰지 않는다. 2026-10-16에 내부 oracle 논문(RRF [36])의 초록을 더 읽었다. 그 본문과 RRF의 둘째 사본은 얻지 못했다.

### 1.2 이 문서가 쓰는 출처별 등급

| 출처 | 등급 | 이 문서에서의 쓰임 |
|---|---|---|
| RRF (ISSRE'16) CPS-VO 사본 | **`[전문]`**. 단일 취득 경로, 독립 재확인 안 됨 | 도구 설명 전체(§2), 여섯 질문의 도구 수준 답(§3.3), 실행 요구(§4) |
| 내부 oracle 논문(Yu, Srisa-an, Rothermel, ISSRE 2013, RRF의 [36]) | `[초록]`. 본문 `[미도달]` | 초록은 internal oracle을 "monitoring aspects of internal program and system states"로 탐지하는 oracle이라고 적는다. 어떤 상태를 어떤 입력으로 보는지는 초록에 없다. H-RRF-3의 조건을 가르지 못한다 |
| 저자 CV(UC 홈) | 서지 | RRF와 [36]의 서지 확인. 아티팩트 링크 없음 |
| RRF 서지·초록 | `[초록]` | 출판 정보 확인 |
| RRF 출판본(IEEE) | `[미도달]` | 판본 동일성 미확인 |
| Rattanasuksun 학위논문(UNL 2018) | 초록 `[초록]`. 본문 `[미도달]` | 존재 확인뿐. 구현 상세가 있을 수 있으나 읽지 못했다 |
| SysPro(arXiv 2601.09616v1) p.2, p.23, p.31, p.35 | `[2차]` | RRF의 입력·자동화 정도를 서술한 문장 |
| DESCRY(FSE'17) p.8, p.11 | `[2차]` | RRF를 SimRacer와 함께 인용한 한 문장. RRF를 서술하지 않는다 |
| 사례 쪽 사실(`cases/ledger.md` §1a, §1b, §2) | `[기록]` | §3.2 |
| OSAL visibility probe(`baselines/availability.md` §4.2) | `[기록]` | 사건이 syscall로 보이는지의 근거. **RRF의 결과가 아니고 어느 도구의 결과도 아니다.** OSAL 자체 시험을 syscall 관측자로 본 probe다 |
| 컨테이너 사실(`availability.md` §1.1) | `[기록]`, 일부 오늘 재확인 `[M]` | §4 |

### 1.3 이전 기록과 달라지거나 확정된 것

| # | 항목 | 내용 |
|---|---|---|
| 1 | `yu-group.md` §3.2, §4, §5 R7·R8, `availability.md` §1 RRF 행 "excerpt (paraphrase only)" | 전문으로 올라갔다. 이전 서술 "No tracer or kernel is named"는 달라진다. 구현에서 쓴 이벤트 보고 도구는 Scribe이고 DTrace·inotify가 다른 예로 거명된다(p.2–3). **정적 분석 도구는 여전히 이름이 없다**(p.1, p.5 "static analysis"). 커널·Scribe 판본도 적히지 않았다 |
| 2 | `availability.md` §3, `yu-group.md` §7.2의 RRF 칸 "Possibly, via yield points" (C1·C3·C5·R2) | 전문으로 조건이 좁아진다. 논문의 yield point는 syscall 이벤트 기록을 소스 위치에 대응시켜 얻은 위치에 `sleep`을 넣는 것이다(p.3, p.5). 사람이 임의의 소스 위치를 지정하는 입력은 서술되지 않는다. 그러므로 "보고서에서 가져온 위치에 yield point"는 원 도구의 기능이 아니라 확장(EXTENSION, `availability.md` §5.1)이다. 이 문서는 해당 칸을 "분석"으로 쓴다. 두 문서의 칸 갱신은 이 작업의 범위 밖이다(§6) |
| 3 | `yu-group.md` §4 "Execution unit: OS process (paraphrase)" | 전문으로 확인된다. PuD는 프로그램 둘 이상이다(p.3). 설정 파일에 "first/second process number"가 있다(p.7) |
| 4 | `availability.md` §3 "C2 has no source" | 일치한다. RRF는 PuD의 소스 접근을 가정하고 소스에 `sleep`을 넣는다(p.5) |
| 5 | `yu-group.md` §3.1 "DESCRY cites RRF only alongside SimRacer" | 일치한다. DESCRY p.8의 "SimRacer [29, 38]"에서 [29]가 RRF다. DESCRY는 RRF를 서술하지 않는다(§2.4) |
| 6 | 로드맵 §10.1 RRF "우선 확인할 자료: 입력으로 필요한 위치·사건·실패 조건과 실행 제어" | 전문으로 답할 수 있다. 입력은 §2.1, 실행 제어는 소스 수준 `sleep` 주입이다 |
| 7 | (2026-10-16) 쪽 번호 인용의 취득 경로 | `process_race_method_matrix.md` §1.1이 RRF 쪽 번호 인용을 "단일 취득 경로, 독립 재확인 안 됨"으로 표시했다. 이 문서도 같은 표시를 단다(머리말 쪽 번호 표기). 둘째 경로(IEEE, Semantic Scholar, OpenAlex, Scholar Gateway, 저자 홈 두 곳, CV, GitHub)는 얻지 못했다(§1.1 경로 21–29). 오늘 이 문서는 PDF를 다시 받지 않았고, 10-15에 저장한 같은 사본(sha256 `e8c26e2f…`)의 추출 텍스트에서 인용 위치를 다시 찾았다. 인용 문구는 모두 적은 쪽에서 찾았다. 다만 "reasonable to assume the availability of source code"는 p.5에 있고 p.3에는 없다(§2.3 행 2의 근거를 고쳤다) |
| 8 | (2026-10-16 추가) 논문이 밝힌 대상 범위 | p.1은 process-level race를 "multiple processes access operating system resources without proper synchronization"인 경우로 정의하고, 특정 언어용이거나 "operating at the thread level"인 탐지기는 이런 race를 탐지·재현하지 못할 수 있다고 적는다. p.10은 thread 수준 기법이 "multi-threaded applications within single processes"에 맞춰져 있다고 적는다. 2026-10-15판은 p.10만 썼다. 한 process 안의 thread 사이 race를 범위에서 뺀다고 명시한 문장은 찾지 못했다. §2.1, §4.3 항목 1에 더했다 |
| 9 | (2026-10-16 정정) R2 수치의 근거 | 2026-10-15판은 R2의 "24회"를 `ledger.md`와 로드맵으로 인용했다. R2의 실행 증거인 gdb 원시 기록(`env/logs/r2_g1..3_*`, `results/R2_gdb_v701.md` §2, §7)으로 바꿨다. 값은 같다. R2 Q6의 `availability.md` §1 인용(`cfe_evs.c:185-188`)도 같은 기록의 v7.0.1 L188로 바꿨다 |
| 10 | (2026-10-16 보강) R2 ③과 C3 TIME `CreatePipe` 칸 | Fig. 2(p.4)의 기록 형식을 근거로 더했다. syscall마다 경로 문자열 인자가 `data: input string`으로 남고, `files_struct` 자원 잠금은 `desc = none`이며, 예시에서 `llseek`·`close`는 관련 없는 syscall로 분류된다. `mq_open`이 Scribe에서 어떤 기록을 남기는지는 RacePro.md §2.5 K1–K3(RacePro 저장소 커널의 코드 읽기, `[기록]`)을 인용했다. RRF가 쓴 Scribe 판본이 그 커널과 같은지는 확인이 없다. 라벨은 "불명" 그대로다 |
| 11 | (2026-10-16 추가) R2 사건표의 ⓪ task 생성 행 | `process_race_method_matrix.md` §7 항목 7이 RacePro.md에 대해 지적한 공백과 같은 행이다. RRF판에도 넣었다 |
| 12 | (2026-10-16) 가설 번호 | H-RRF-2는 결번이다. 이름·ID·세대 항목(로드맵 §10.5 둘째 행)을 §5.3에서 가설로 올리지 않았기 때문이다. 번호를 다시 매기지 않는다(다른 문서가 H-RRF-1·3·4·5로 인용한다) |

---

## 2. 도구의 정체

### 2.1 요약

| 항목 | 내용 | 근거 |
|---|---|---|
| **역할** | 재현(reproduction). 사용자가 보고한 process-level race를 재현해 디버깅을 돕는다. "race detection system이 아니라 race reproduction system"이다. 부수적으로 같은 자원에 대해 다른 oracle을 적용해 다른 유형의 fault도 센다(Table V: UPDATEDB–UPDATEDB 204건, LOCATE–UPDATEDB 120건) | p.1, p.3, p.9–10 |
| **race 정의** | "(1) 두 process가 공유 자원에 접근하고 (2) 원래 순서와 다른 순서로 접근할 수 있는 것." 기존 정의보다 넓고 **order violation을 포함한다**("desired order … reversed", 공통 lock의 보호 여부와 무관). order violation은 race의 필요조건이지 충분조건이 아니라고 적는다 | p.2 |
| **실행 단위** | OS process. "two or more programs undergoing debugging (PuDs)". 평가 대상은 C 프로그램과 shell script 7쌍이다(Table I). 설정 파일에 두 process의 번호와 소스 위치를 둔다. 같은 process 안의 thread를 단위로 다루는 서술은 없다. process-level race를 "multiple processes access operating system resources"인 경우로 정의하고(p.1), thread 수준 기법은 "단일 process 안의 multi-threaded 응용"에 맞춘 것이라고 대비해 서술한다(p.1, p.10). thread 사이 race를 범위에서 뺀다는 명시 문장은 찾지 못했다 | p.1, p.3, p.6–7, p.10 |
| **관측하는 것** | system-level event(system call, signal). 구현은 Scribe다. Scribe 출력 예(Fig. 2)에 system call과 인자, `resource lock` 기록(`type = inode/files_struct/file`, `access = read/write`, `id`, `serial`, `desc`)이 있다. DTrace(Solaris), inotify가 같은 역할의 다른 도구로 거명된다. 사용자 공간 메모리 접근, 함수 반환값, futex·mutex·semaphore 사건을 쓴다는 서술은 없다 | p.2–4 |
| **자원 식별** | (1) 엔지니어가 bug report를 분석해 관측할 자원을 지정한다("must specify resources"). (2) 설정 파일의 "shared resource name"과 두 process 번호로 로그를 거른다. 먼저 대상이 아닌 process의 syscall을 빼고, 다음으로 공유 자원을 쓰는 syscall만 남긴다. (3) symbolic link가 있으면 Scribe가 실제 파일 이름과 symbolic 이름을 함께 보고하므로 같은 자원으로 걸러낸다. (4) 두 process의 syscall을 "같은 공유 자원에 접근하면" 짝짓는다. 예시 자원은 파일 경로(`/usr/local/var/locatedb`)다. 객체 ID·세대(generation)·등록 정보를 잇는 서술은 없다 | p.3–5, p.7–8 |
| **모델링하는 순서·의존** | 두 process의 syscall 쌍이 같은 자원에 접근하는 **pair-wise access**다. 두 순서 `(σi, σj)`와 `(σj, σi)`를 모두 만들고, 쌍마다 네 패턴(Pi가 먼저, Pj가 먼저, Pi는 syscall 앞·Pj는 뒤, 그 반대)으로 `sleep`을 넣는다. RRF 자신의 happens-before 그래프나 lock·동기화 모델 서술은 없다(RacePro 쪽 모델은 RacePro 문서). syscall을 자원에 대한 read/write로 보는 "system-level event modeling"은 RacePro와 같은 방식으로 서술한다 | p.1, p.2, p.5–6 |
| **입력** | 응용 설정(설정 파일), 프로그램 둘 이상(PuD, **소스 코드 필요**), test input(없으면 생성 기법 사용 가능), bug report, 실패 출력. 설정 파일 내용: 프로젝트 이름, 공유 자원 이름, 두 process의 번호와 소스 위치, 로그 파일. 최소한 bug report에 사용자가 겪은 잘못된 증상(file corruption, missing signal, socket not found)이 있어야 한다. 필요한 순서·허용 오류를 받는 필드는 없다 | p.3–4, p.5, p.7 |
| **출력** | race report: race가 일어났는지, 분류(targeted / incidental / new), 두 PuD에서 관련 이벤트 쌍의 위치. 중간 산출물로 "관련 이벤트 로그", "잠재 interleaving 이벤트 쌍 목록", `sleep`이 주입된 PuD(mPuD)가 있다. 평가에서는 쌍마다 race가 관측된 실행 수를 센다(Table II) | p.3–4, p.6, p.8 |
| **제어** | 소스 코드에 `sleep`을 주입한다(`#ifdef`로 어느 `sleep`을 켤지 고른다, Fig. 4). 늦춰야 할 쪽의 sleep을 먼저 쪽의 두 배로 둔다(ST와 2×ST). 바이너리 수준 계측(Pin)은 가능하다고만 적고 쓰지 않았다. sleep 시간은 "임계 연산에 걸리는 시간"으로 시작해 시험 실행으로 다듬고, 논문의 사례에서는 1초가 모두에서 잘 맞았다. 반복 10회로 비결정성을 다룬다 | p.3, p.5–6, p.8 |
| **판정(oracle)** | 내부 oracle(property 위반을 찾는 TOCTOU, atomicity violation, race 검출)과 출력 기반 oracle. 확장 가능한 저장소에 oracle을 더 넣을 수 있다. 평가 대상을 file·socket 자원 사례로 좁혔다. 이유는 oracle이 있는 유형만 판정할 수 있어서다 | p.2, p.3, p.6 |
| **평가** | 실제 bug report가 있는 7쌍(Red Hat, Ubuntu, Debian, GNU 저장소). RRF가 7쌍 모두 재현했고, 같은 시간 동안의 stress testing(STA)은 한 쌍에서만 재현했다(Table II). RacePro는 7쌍 중 3쌍에서만 실행됐고 재현하지 못했다(p.9–10). 사람이 쓰는 효과(디버깅 시간)는 평가하지 않았다 | p.6–10 |
| **구현 환경** | 논문은 OS·커널 버전·하드웨어·Scribe 판본·정적 분석 도구 이름·구현 언어를 적지 않는다. "Linux 지원과 낮은 overhead 때문에 Scribe"라고만 적는다. 그 overhead는 Scribe 논문의 값(서버 2.5%, 데스크톱 15%)을 인용한다 | p.3 |
| **artifact 위치** | 논문 본문에 코드·데이터 위치를 적은 곳이 없다. 참고문헌 URL은 bug report와 Scribe 이슈, Pin 문서뿐이다 | 전문 검색 |

### 2.2 동작 흐름 (Figure 1, p.3)

1. **(A) 관련 system event 식별.** 각 PuD를 따로 실행하며(기존 test suite 또는 bug report의 실행 스크립트) 이벤트 보고 도구를 켠다. 엔지니어가 bug report의 증상으로 자원을 정하고, 이 자원에 접근할 수 있는 이벤트만 남겨 PuD별 "관련 이벤트 로그"를 만든다(p.4). 로그가 곧 syscall 단위다. 예: `unlinkat`·`fstatat64`가 `locatedb`를 건드린다(Fig. 2).
2. **(B) 이벤트를 소스 위치에 대응.** 정적 분석으로 응용 이벤트(예: `rm`으로 데이터베이스 파일 삭제)의 소스 위치를 찾고, 각 응용 이벤트를 미리 만든 모델에 따라 syscall 열로 바꾼다. 결과는 syscall 블록과 소스 줄의 대응이다(Fig. 3: 한 블록이 UPDATEDB의 144번 줄로 대응)(p.4–5). "관련 소스 위치 목록 = race가 시작될 수 있는 위치"가 산출물이다.
3. **(C) PuD 수정.** 두 process의 관련 이벤트를 쌍으로 묶어 Algorithm 1로 `sleep`을 주입한다. 쌍마다 mPuD 4개를 만든다(p.5–6).
4. **(D) 재현 실행.** mPuD를 쌍으로 컴파일해 실행하고 Scribe로 이벤트를 다시 기록한다. 각 사례를 10번 반복한다(p.6, p.8).
5. **(E) 검증.** 실행 기록과 출력을 oracle로 확인한다. race가 있으면 targeted(주입한 `sleep` 쌍이 겨냥한 것), incidental(의도하지 않았지만 자연히 일어난 것), new(분석이 정한 후보 위치와 무관한 것)로 나눈다(p.6).
6. **실행 횟수의 구조.** 필요한 실행 수는 `|Si| × |Sj| × 4 × r`이다. `r`은 비결정성을 이기기 위한 반복 수다(p.6). 관련 syscall을 자원으로 거르면 이 수가 몇 자릿수 줄어든다(Table III, p.9).

### 2.3 논문이 밝힌 범위·한계 중 cFS와 관련 있는 것

| # | 서술 | 근거 | cFS와의 관계 (분석, 미실행) |
|---|---|---|---|
| 1 | 풀려는 문제를 "system call이 다른 system call을 부르는 implicit 접근을 엔지니어가 놓치기 쉬운 것"으로 둔다. 동기는 커널 서비스 안의 사건이 보이지 않는다는 점이다 | p.1–2 | 사례의 의존성은 이 문제와 반대 위치에 있다. 접근이 syscall로 나타나지 않고 사용자 공간 메모리의 쓰기·읽기다(`availability.md` §4.2 `[기록]`) |
| 2 | 단위는 process 쌍이고(p.3) 소스 접근을 가정한다("reasonable to assume the availability of source code", p.5) | p.3, p.5 | native cFS는 한 process이고 OSAL task는 thread다(`availability.md` §3 `[기록]`). cFE/OSAL/CF/BP 소스는 공개이나 C2의 앱은 비공개다 |
| 3 | 평가 대상을 oracle이 있는 file·socket 자원으로 좁힌다. "oracle이 있는 유형만 탐지한다" | p.6 | cFS 사례에 대응하는 oracle이 논문에 없다. 저장소는 확장 가능하다고 적는다(p.2, p.6) |
| 4 | `sleep` 기반 제어는 비결정성이 남는다("a system call is a large block of execution steps"). 시간은 사례별로 시험 실행으로 정했다 | p.5–6 | 시간 값은 cFS에서 `unspecified_by_reference`다. 조건 원칙은 사건 기반 제어를 우선한다(`CONDITIONS_POLICY.md`) |
| 5 | RacePro가 2012년 이후 유지되지 않고 replay divergence가 있어 baseline으로 쓸 수 없었다. Scribe의 이슈 두 건(저장소 이슈 #3, #6)을 인용한다 | p.2, p.9 | Scribe를 쓰는 RRF의 실행 가능성에 직접 관련된다(§4) |
| 6 | bug report의 품질과 test suite의 품질이 효과에 영향을 준다. 보고가 불완전하면 더 많은 이벤트를 보수적으로 남겨야 한다 | p.9 | 다섯 사례의 이슈 댓글은 아직 읽지 않았다(`availability.md` A6) |
| 7 | 엔지니어가 쓸 때의 효과(디버깅 시간)는 평가하지 않았다 | p.8 | 해당 없음 |

### 2.4 2차 문헌의 RRF 서술

| 출처 | 내용 | 논문과의 대조 |
|---|---|---|
| SysPro p.2, p.35 | RRF는 "mostly automated"이고 bug report에서 입력·interleaving 정보를 모으는 데 사람의 개입이 필요하다 | RRF p.3("mostly automatic. An engineer must specify resources")과 일치 |
| SysPro p.31 | RRF는 "application configurations, two processes undergoing debugging, test inputs, bug reports, and failed outputs"를 요구한다 | RRF p.3 입력 목록과 일치 |
| SysPro p.23 | RRF는 벤치마크 7개 | RRF p.6–8과 일치 |
| DESCRY p.8, p.11 | "SimRacer [29, 38]"에서 [29]가 RRF다. RacePro와 SimRacer가 "구체적 data input을 요구한다"는 비교 서술에서 RRF는 서술되지 않는다 | RRF를 서술하지 않는다 |
| SimEvo | RRF 언급 없음(키워드 검색) | — |

---

## 3. 여섯 질문 × 다섯 사례

### 3.1 읽는 법

셀의 라벨은 아래 셋 중 하나다. 한 칸에 관측 가능성, 표현 가능성, 실제 재현 성공을 합치지 않는다(로드맵 §10.2).

| 라벨 | 뜻 |
|---|---|
| **답** | 출처의 쪽·절로 답했다(answered with source page/section) |
| **분석** | analysis from stated design (not run). 읽은 설계 기술에서 도출했고 실행하지 않았다 |
| **불명** | not determinable from sources read |

"분석" 셀의 "경로가 서술에 없다"는 "읽은 설계 기술 안에서 그 쌍이 후보 입력이 되는 경로를 찾지 못했다"는 뜻이다. 실행 결과가 아니다. 셀의 근거는 별도 표시가 없으면 RRF 논문(`[전문]`)의 쪽이다. 질문 번호는 로드맵 §10.2의 순서다.

1. 필요한 **관측 사건**을 수집하는가?
2. 서로 다른 이름·ID·필드가 같은 **자원·세대**를 가리키는 것을 식별하는가?
3. **생성·등록·초기화 완료와 의존 사용**의 관계를 표현하는가?
4. 필요한 **순서 또는 허용된 오류 처리** 정보를 입력받는가, 추론하는가?
5. 아직 실행되지 않은 순서를 **예측·탐색**하는가? 실제 실행에서 **확인**할 수 있는가?
6. **출력**은 후보·재현 성공·오류 결과 중 무엇이며 원 사례의 현상과 대응하는가?

**모든 "분석" 셀에 공통으로 걸린 전제 두 가지.**
- **전제 1.** 사례의 접근이 RRF의 이벤트 로그(syscall·resource lock 기록)에 들어와야 후보 쌍이 된다. 이 전제는 §3.3 Q1에서 따로 다룬다.
- **전제 2.** 한 process 안의 task(thread) 쌍을 PuD 쌍으로 줄 수 있어야 한다. RRF 논문에는 thread 쌍을 단위로 다루는 서술이 없고(p.3, p.10), 줄 수 있는지는 **불명**이다. 로드맵 §1.2는 cFS의 Process-Level을 "애플리케이션·task 단위"라고 운용 정의하고 OS process로 한정하지 않는다. RRF의 단위(OS process)와의 차이를 논문 처음에 밝혀야 한다는 점이 여기서 나온다.

### 3.2 사례 쪽에서 쓴 사실 (모두 저장소 기록, 재검증하지 않음)

| 사례 | 의존성과 현상 | 출처 |
|---|---|---|
| C1 CF#184 | provider가 이름 있는 OSAL count semaphore를 만든 뒤에야 CF의 `OS_CountSemGetIdByName`이 성공한다. provider는 원 보고에서 미식별이고 공개 BP 구성에서는 `OS_CountSemCreate`다. CF는 한 번만 조회하고 실패하면 `CF_EID_ERR_INIT_SEM`과 `APP_ERROR` 종료다. 생성이 조회보다 먼저임을 정하는 장치가 없다. 수정은 `OS_ERR_NAME_NOT_FOUND`일 때만 25회 × 100 ms 재시도다. 실패 출력과 빈도는 `unspecified_by_reference` | `cases/ledger.md` §1a, §1b, §2 `[기록]` |
| C2 cFE#198 | 앱 P의 late init 완료와 다른 앱 R의 의존 호출. P·R은 비공개(EVA CWS)이고 통신 방식·초기화 내용이 공개되지 않았다. 공개된 것은 단일 상태 시작 동기화(`WaitForStartupSync`)와 6.6.0a의 두 단계 방식(`APPS_INIT`, `LATE_INIT`, `CFE_ES_WaitForSystemState`)이다 | `ledger.md` §1a, §1b, §2; 로드맵 §9.1 `[기록]` |
| C3 cFE#73 | SB task의 `CFE_SB.AppId`와 EVS task의 `EVS_AppID`·등록 플래그가 설정되기 전에 TIME task가 `CFE_SB_CreatePipe` 뒤 `CFE_EVS_SendEventWithAppID`(AppId=0)를 부르고 `EVS_IsFiltered(0xFFFFFFFF)`가 범위 검사 없이 접근한다. 우선순위(TIME 60, EVS 61, SB 64)가 순서를 만든다. 원 플랫폼은 Microblaze이고 오류 지점은 미확인 | `ledger.md` §1a, §2 `[기록]` |
| C5 cFE#72 | `CFE_ES_AppCreate`가 ES lock을 `OS_TaskCreate` 앞에서 풀고 뒤에서 다시 잡아 `TaskTable`을 채운다. 그 사이 새 task의 `CFE_ES_GetAppID`가 `CFE_ES_ERR_APPID`를 반환할 수 있다. 위반은 "등록 전 조회"이고 일찍 실행된 것만으로는 위반이 아니다. 수정(6.4.2)은 lock 범위 확장이다 | `ledger.md` §1a, §1b `[기록]` |
| R2 cFE#2663 | SB가 `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`로 AppId를 저장하기 전에 ES/EVS task가 SB API 경로(`CreatePipe`, `Subscribe`, 이벤트 전송)를 실행해 `EVS_GetAppDataByID`가 NULL을 돌려주고 `ILLEGAL_APP_ID`가 된다. 상태는 폐기된다. 서술은 "이벤트 요청 거부"다. gdb 기록 3회(`env/logs/r2_g1..3_*`) 모두 실행당 설정 전 사용 24회(ES 20, EVS 4; `CreatePipe` 2, `SubscribeFull` 4, `ReportSingleEvent` 18), 모두 AppId 0, 모두 `cfe_evs.c` L188(v7.0.1)에서 `ILLEGAL_APP_ID`이다. 기록은 호출자가 이 상태를 버려 "Nothing was printed, no counter changed, and no syscall was made"라고 적는다. 한 원인의 반복이며 독립 결함 24개가 아니다. core 시작 직렬화(S1)가 EVS·ES 초기화를 SB보다 앞세운다. race인지 결정적 순서인지는 실행 확인이 필요한 미결 항목이고, 3회 일치는 결정성의 증명이 아니다 | `ledger.md` §1a, §2 `[기록]`; `results/R2_gdb_v701.md` §2, §3, §7 `[기록]`(측정 기록) |
| 공통 | native cFS는 한 Linux process이고 OSAL task는 `pthread_create` thread다. OSAL 자체 시험에서 named count semaphore의 생성과 이름 조회는 **syscall 0건**이었다. SB pipe는 `mq_open("<pid>.<name>")`을 부른다 | `availability.md` §3, §4.2 `[기록]`. OSAL 시험의 probe이며 RRF의 결과가 아니다 |

### 3.3 도구 수준의 답 (사례와 무관)

| Q | 라벨 | RRF 논문(`[전문]`)의 답 | 근거 |
|---|---|---|---|
| 1 관측 사건 | 답 | system-level event(system call, signal)를 Scribe로 기록한다. 기록에 syscall과 인자, `resource lock`(type `inode`·`files_struct`·`file`, `access`, `id`, `serial`, `desc`)이 들어 있다. DTrace·inotify는 같은 역할의 다른 도구로 거명된다. 사용자 공간 메모리 접근, 함수 반환값, futex·mutex·semaphore 사건, 응용 내부 상태를 기록한다는 서술은 없다. 일반론으로는 "race 검출·재현에 shared memory access와 동기화 연산의 관측이 필요하다"고 적지만(p.1) 그 관측을 RRF가 한다고 쓰지 않는다 | p.1–4 (Fig. 2) |
| 2 자원·세대 식별 | 답 | 자원은 엔지니어가 bug report에서 정해 설정 파일의 "shared resource name"으로 준다. 로그는 대상 process로 거른 뒤 그 자원을 쓰는 syscall로 거른다. symbolic link는 Scribe가 실제·symbolic 이름을 함께 보고해 처리한다. 두 process의 syscall은 같은 공유 자원에 접근하면 짝이 된다. 예시 자원은 파일 경로다. 이름·ID·슬롯 대응, 세대에 관한 서술은 없다 | p.4–5, p.7–8 |
| 3 생성·등록 vs 의존 사용 | 답 | "두 process의 syscall 쌍이 같은 자원에 접근한다"는 모양으로 표현한다. 두 순서를 모두 만든다. 평가한 쌍에는 생성과 사용이 섞인 파일 사례가 있다(PXZ가 만든 중간 파일을 STEALER가 권한 설정 전에 봄, MV·RM, LOCATE·UPDATEDB). 등록 완료·초기화 완료 같은 상태 개념은 서술에 없다 | p.5–6, p.7 |
| 4 필요 순서·허용 오류 입력 | 답 | 입력 목록(설정 파일, PuD, test input, bug report, 실패 출력)에 필요한 순서나 허용 오류를 받는 필드가 없다. 순서는 입력도 추론도 아니고 열거(쌍×두 순서×네 패턴)로 시도한다. 판정 기준은 oracle에 있다. 허용 오류·재시도에 관한 서술은 없다(키워드 `retry` 0건). 엔지니어가 확장 가능한 oracle 저장소에 정확성 property를 넣을 수 있다고 적는다 | p.2, p.3, p.6, p.7 |
| 5 예측·탐색, 확인 | 답 | 관측된 한 실행의 순서만 보지 않는다. 관련 syscall 쌍 전체를 두 순서와 네 패턴으로 열거하고(`|Si|×|Sj|×4`), `sleep`으로 순서를 강제해 실제 실행에서 확인한다. 이 과정을 사례마다 `r`회(논문은 10) 반복하고 oracle로 판정한다. happens-before 분석으로 후보를 계산하지 않는다. 쌍을 반대 순서로 만들 수 없는 경우의 처리는 서술에 없다 | p.5–6, p.8 |
| 6 출력 | 답 | race report(race 발생 여부, targeted/incidental/new 분류, 두 PuD의 이벤트 쌍 위치). 쌍마다 race가 관측된 실행 수, 후보 위치 수와 실제로 race를 낸 위치 수(Table IV)도 보고한다. 같은 자원에서 다른 oracle로 발견한 다른 유형의 fault도 센다. 출력은 후보 목록이 아니라 "겨냥한 race가 재현됐는가"이다. 반환값, 순서의 근거는 출력 항목이 아니다 | p.3, p.6, p.8–10 |

### 3.4 5 × 6 라벨 요약

| | Q1 관측 | Q2 식별 | Q3 생성·사용 | Q4 입력 | Q5 예측·확인 | Q6 출력 |
|---|---|---|---|---|---|---|
| **C1** | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| **C2** | 불명 | 불명 | 불명 | 답 | 불명 | 불명 |
| **C3** | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| **C5** | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| **R2** | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |

### 3.5 사례별 셀 (근거 포함)

#### C1 — CF#184 (named count semaphore의 생성 vs 이름 조회)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | 필요한 사건은 provider의 이름 등록(OSAL 이름 테이블 쓰기)과 CF의 이름 조회(읽기)다. OSAL 자체 시험에서 두 호출은 syscall 0건이었다. RRF의 관련 이벤트는 syscall·resource lock 기록에서 정해지므로(p.3–4) 이 두 사건이 후보가 되는 경로를 서술에서 찾지 못했다. 이 사건이 Scribe의 다른 기록(RacePro 쪽 기록은 메모리 page 소유권을 포함한다, `availability.md` §1 RacePro 행 `[기록]`)에 들어오더라도 RRF가 그것을 쓴다는 서술은 없다 | p.3–4; `availability.md` §1, §4.2 `[기록]` |
| 2 | 분석 | 이름 `BP0_tsem` → OSAL ID → 이름 테이블 슬롯 → `sem_init`된 객체를 같은 자원으로 잇는 장치는 서술이 없다. 설정 파일의 "shared resource name"을 엔지니어가 정하는 구조이나(p.7), 거기서 쓰는 이름은 Scribe가 보고하는 파일 이름이다(p.4). OSAL 이름은 커널 자원 이름이 아니다. 세대에 관한 서술도 없다 | p.4, p.7; `availability.md` §4.2 `[기록]` |
| 3 | 분석 | 생성과 조회가 syscall로 보이지 않으므로 "같은 자원의 syscall 쌍"으로 표현되지 않는다. OSAL 레코드가 예약된 상태인지 공개 완료인지, 재시도 중인지 최종 실패인지 구분하는 개념은 모델에 없다 | p.5–6 |
| 4 | 답 | 도구 수준 답과 같다. C1이 필요로 하는 허용 오류("재시도는 `OS_ERR_NAME_NOT_FOUND`일 때만")를 넣을 입력은 없다. 실패 기준은 oracle(출력 기반이면 CF 초기화 실패 이벤트나 종료)에 둔다. 사례의 실패 출력이 `unspecified_by_reference`라 oracle이 볼 출력이 정해지지 않았다 | p.3, p.6–7; `ledger.md` §1b `[기록]` |
| 5 | 분석 | 쌍이 로그에서 나오지 않으면 열거 대상이 되지 않는다. 사람이 소스 위치를 직접 지정하는 입력은 서술에 없다(수동 sleep은 일반 관행으로만 언급, p.1). 위치가 주어진다면 provider 생성 앞 또는 CF 조회 앞의 `sleep`으로 두 순서를 시도하고 oracle로 확인하는 구조가 적용될 수 있다 [I]. 그것은 확장이다 | p.1, p.5–6 |
| 6 | 분석 | 출력은 race report와 분류다. C1의 현상은 "CF 초기화 실패(이벤트와 `APP_ERROR` 종료)"와 "재시도 뒤 회복 vs 최종 실패"의 구분이다. 출력 기반 oracle이 초기화 실패 이벤트를 볼 수는 있으나(사용자가 작성), 회복과 최종 실패를 구분하는 출력 항목은 서술에 없다. 수정 버전이 재시도로 회복하는 경우의 보고 형식도 서술에 없다 | p.3, p.6 |

#### C2 — cFE#198 (late init 완료 vs 다른 앱의 의존 호출)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 불명 | 필요한 사건은 P의 late init 완료와 R의 의존 호출이다. P·R이 비공개이고 통신 방식·초기화 내용이 알려지지 않아 어떤 자원을 거치는지 판단할 근거가 없다. RRF는 PuD 소스를 필요로 한다(p.5) | `ledger.md` §1a `[기록]`; 로드맵 §9.1 |
| 2 | 불명 | 같은 이유 | — |
| 3 | 불명 | 같은 이유. "barrier 통과"와 "앱 고유 초기화 완료"를 구분하는 상태 개념은 RRF 논문에 없다(키워드 `initializ` 1건은 쌍 목록의 초기화 문장이다). P의 late init이 거치는 자원이 비공개라 이 칸은 불명으로 둔다 | 키워드 검색 |
| 4 | 답 | 도구 수준 답과 같다. 실패 기준은 원 보고가 구체화하지 않아 연구 쪽 선택이 된다. 선택하더라도 원 보고자의 값으로 인용하지 않는다 | p.3, p.7; [D] 결정 문서 §3.5 |
| 5 | 불명 | 쌍이 될 접근을 알 수 없다 | — |
| 6 | 불명 | 원 현상이 구체 출력으로 공개되지 않았다 | `ledger.md` §1a `[기록]` |

C2의 **공개 부분**(역할 재구성에 쓸 수 있는 알려진 조건):

| 사건 | 흔적 | RRF 입력(설계) | 라벨 |
|---|---|---|---|
| `CFE_ES_WaitForSystemState`가 상태 변수가 목표에 이를 때까지 `OS_TaskDelay`로 기다림 | 상태는 `CFE_ES_Global`의 메모리 필드. 대기는 sleep 계열 syscall(추정) | 기다리는 task의 sleep syscall은 이벤트 로그에 나타날 수 있으나, 그 syscall이 "같은 공유 자원 접근"이 되는 경로는 서술이 없다. 상태 필드의 읽기·쓰기는 로그에 없다 | 분석 |
| 단일 상태 / 두 단계 barrier의 전이와 timeout | 같은 메모리 필드 | 위와 같다. 상태 전이를 "시작 단계"나 "준비 상태"로 읽는 개념은 모델에 없다 | 분석 |

[F 기록] 코드 근거는 `availability.md`의 `cfe_es_api.c:514, 585, 603` 서술이며 재검증하지 않았다.

#### C3 — cFE#73 (SB/EVS AppId 설정 vs TIME의 이벤트 요청)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | 필요한 사건은 `CFE_SB.AppId` 저장, `EVS_AppID`와 등록 플래그 설정, TIME의 이벤트 요청(AppId=0)과 `EVS_IsFiltered`의 범위 검사 없는 접근이다. 모두 사용자 공간 전역 메모리의 읽기와 쓰기이며 syscall이 없다(source 수준). TIME의 `CreatePipe`가 부르는 `mq_open`은 현재 OSAL에서 보였지만(`availability.md` §3 C3 열, 부분), 6.4.1 시대의 호출은 측정하지 않았다. 원 플랫폼 Microblaze의 OS는 미확인이다. RRF의 구현은 Linux의 Scribe다(p.3). 같은 도구가 원 플랫폼에서 돌 수 있는지는 서술이 없다 | p.3–4; `ledger.md` §1a `[기록]` |
| 2 | 분석 | 의존 상태는 서로 다른 세 필드(SB AppId, EVS AppId, EVS 등록 플래그)다. RRF에서 이것들을 구분해 서로 다른 자원으로 취급하는 경로는 서술이 없다. AppId 값 0이 EVS 배열의 인덱스로 쓰이는 관계(값 → 슬롯)를 추적하는 서술도 없다 | p.4–5 |
| 3 | 분석 | 공개(쓰기)와 사용(읽기)이 메모리라 syscall 쌍으로 표현되지 않는다. core 시작 순서를 정하는 ES의 task 대기와 우선순위는 RRF의 모델에 없다 | p.5–6 |
| 4 | 답 | 도구 수준 답과 같다. 원 보고의 실패는 EVS 내부의 잘못된 인덱스 접근이고 오류 지점과 주 결과는 사례 기록상 미결이라, oracle이 무엇을 봐야 하는지 지금 정할 수 없다 | p.3, p.6; `ledger.md` §2 `[기록]` |
| 5 | 분석 | C1과 같다. 사례의 순서(TIME 사용 → SB/EVS 공개)는 우선순위가 만든 것이라 관측된 실행에 이미 있을 수 있다. 쌍이 주어지면 `sleep`으로 반대 순서(공개 → 사용)를 시도하는 구조가 적용될 수 있다 [I]. 어느 쪽이 정상 순서인지는 oracle이 정한다 | p.5–6 |
| 6 | 분석 | 사례 현상은 "잘못된 상태 사용"과 "실제 crash 여부"를 분리해 보고하는 것이다(로드맵 §9.2). RRF의 출력은 race 분류이고 crash를 따로 검출한다는 서술은 없다. 출력 기반 oracle은 사용자가 작성한다 | p.3, p.6; 로드맵 §9.2 |

#### C5 — cFE#72 (task 생성과 레지스트리 등록 사이의 공백)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | `OS_TaskCreate`의 `pthread_create`는 `clone` 계열이다. 논문은 RacePro가 추적하는 syscall의 예로 `clone`을 든다(p.2). 에이전트가 쓴 작은 C 프로그램에서 `clone3`로 관측됐다(`availability.md` §4 끝, 기록. OSAL·cFS 측정이 아니다). `TaskTable` 등록 쓰기와 새 task의 `GetAppID` 조회는 사용자 공간 메모리 접근이라 syscall이 없다(OSAL 시험에서 task 레지스트리 대기는 futex 대기만 보였다) | p.2–4; `availability.md` §4 `[기록]` |
| 2 | 분석 | 커널 쪽 task 식별은 Scribe의 process 번호다(Fig. 2의 `[39]`). OSAL task ID, ES `TaskTable` 인덱스, AppId 사이의 연결을 식별하는 서술은 없다 | p.4 (Fig. 2) |
| 3 | 분석 | 등록 쓰기는 task 생성 뒤에 있고 자식의 조회와 같은 자원 쌍이 되지만, 둘 다 syscall이 아니라 RRF의 쌍 정의(p.5)에 들어오는 경로를 찾지 못했다. "등록 공개 완료"라는 상태는 모델에 없다. 6.4.2의 lock 범위 확장은 RRF의 모델에 없는 요소다 | p.5–6 |
| 4 | 답 | 도구 수준 답과 같다. 원 현상(등록 전 조회의 실패 반환)은 crash가 아니라 내부 API 반환값이다. 출력 기반 oracle이 이를 보려면 앱의 출력에 나타나야 한다. 원 보고에 없는 앱 crash는 기대 결과로 넣지 않는다(로드맵 §9.3) | p.3, p.6 |
| 5 | 분석 | 이 쌍은 부모-자식 관계의 두 사건이다. RRF의 쌍은 서로 다른 프로그램의 두 process 사이에서 정의된다(p.3, p.5). 부모와 자식이 한 process 안의 thread라는 점이 전제 2에 걸린다. 쌍이 주어지면 `sleep`으로 순서를 시도하는 구조가 적용될 수 있다 [I] | p.3, p.5–6 |
| 6 | 분석 | 사례 현상은 "등록 전 조회와 그 결과"다(로드맵 §9.3). RRF의 출력은 race 분류와 이벤트 쌍 위치이고, 반환값 `CFE_ES_ERR_APPID`는 출력 항목이 아니다 | p.3, p.6 |

#### R2 — cFE#2663 (SB AppId 설정 vs ES/EVS의 이벤트 요청)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | 아래 사건 대응표의 사건 중 AppId 설정과 값 읽기는 전역 메모리 쓰기와 읽기다. `CreatePipe`의 `mq_open`과 ⓪의 task 생성만 syscall이다. 오류 반환은 반환값이고, gdb 기록은 거부 경로에서 syscall이 없었다고 적는다. 이 중 어느 것이 RRF의 관련 이벤트 로그에 들어와 후보가 되는지는 서술에서 정해지지 않는다 | p.3–4; 아래 표; `results/R2_gdb_v701.md` §2 `[기록]` |
| 2 | 분석 | `CFE_SB_Global.AppId` 하나의 메모리 위치다. EVS가 이 값을 이벤트 원천 ID로 받아 자기 테이블을 찾는 값 전달을 RRF가 추적한다는 서술은 없다 | p.4–5 |
| 3 | 분석 | 설정(쓰기)과 사용(읽기)이 메모리라 syscall 쌍으로 표현되지 않는다. 이 순서를 정하는 core 시작 직렬화(S1)는 RRF의 모델에 없다 | p.5–6 |
| 4 | 답 | 도구 수준 답과 같다. R2의 요구 순서("SB AppId 저장이 SB API 경로의 이벤트 요청보다 먼저")를 넣을 입력은 없다 | p.3, p.7 |
| 5 | 분석 | RRF는 쌍마다 두 순서를 `sleep`으로 시도한다(p.5). R2의 순서는 시작 직렬화(S1)가 EVS·ES 초기화를 SB보다 앞세우므로, 한 task에 `sleep`을 넣어 SB 설정이 먼저인 순서를 만들 수 있는지는 자료로 정해지지 않는다 [I]. 순서가 스케줄로 뒤집히는지가 사례 기록상 미결이라(race인지 결정적 순서인지, `ledger.md` §2 R2) 이 칸이 답하는 범위는 "설계상 두 순서를 시도한다. 시도해도 한 순서만 나오는 경우의 처리는 서술에 없다"까지다. 순서가 고정된 이유를 담는 출력 항목은 서술에 없다 | p.5–6; `ledger.md` §2 `[기록]` |
| 6 | 분석 | 사례 현상은 실행당 "이벤트 요청 24회 거부"이고 프로세스 실패가 없다(상태 폐기). 반환값은 출력 항목이 아니고, 기록상 호출자가 반환 상태를 버려 출력도 카운터 변화도 없다(`cfe_evs.c` L188, v7.0.1). 출력 기반 oracle에는 차이가 나타나지 않을 수 있다. 내부 oracle은 TOCTOU·atomicity·race 유형이다(p.6). RRF는 race가 실행마다 나타나는지 여부를 쌍·순서별 실행 수로 센다(Table II, p.8). R2처럼 기록상 3회 모두 나타난 현상에서 "targeted / incidental / new" 분류가 무엇이 될지는 서술로 정해지지 않는다. "24"는 이 도구의 출력 수량이 아니다 | p.3, p.6, p.8; `results/R2_gdb_v701.md` §2, §7 `[기록]` |

**R2 사건 대응표** (로드맵 §10.3 방식의 RRF판). 참조 revision은 사례 기록의 B/M/D(`fc8c48ad`/`12cb84fc`/`f22e538c`)이며 v7.0.1 코드와 다를 수 있다.

| R2 사건 | 커널에서 보이는 흔적 | RRF의 기록·입력(설계) | 후보 규칙 | 라벨 |
|---|---|---|---|---|
| ⓪ core task 생성 (2026-10-16 추가): ES가 core task를 `OS_TaskCreate`로 차례로 만들고 각 task가 RUNNING이 될 때까지 기다린다(S1, `cfe_es_start.c` L810, L835, v7.0.1; `results/R2_gdb_v701.md` §6 `[기록]`) | `OS_TaskCreate` → `pthread_create` → `clone` 계열 syscall(C5 보충표와 같은 근거, cFS에서 측정하지 않음). RUNNING 대기는 메모리 폴링과 지연이다 | `clone`은 RacePro의 추적 syscall 예다(p.2). 경로 문자열 인자가 없으므로 Fig. 2 방식의 자원 이름 거르기(p.4)에서 관련 이벤트가 되는 경로는 서술에 없다 | 시작 직렬화 같은 순서 근거를 모델링한다는 서술은 없다. 서술된 순서 조작은 쌍의 순서를 `sleep`으로 바꾸는 것이다(p.5) | 분석 |
| ① SB AppId 설정: `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`, 반환값 무시 (B `cfe_sb_task.c` L126, M L138, D L128) | syscall 없음(소스 수준) | 서술 없음 | 같은 자원의 syscall 쌍(p.5)이 아니다 | 분석 |
| ② 값 읽기: EVS(B) 또는 ES·EVS(M, D) task가 같은 전역을 읽음 | syscall 없음 | 서술 없음 | 위와 같다 | 분석 |
| ③ 이벤트 요청: `CreatePipe`/`Subscribe`/전송 → `CFE_EVS_SendEventWithAppID` | `CreatePipe`는 `mq_open("<pid>.<name>")`(OSAL 시험 69회, probe). `Subscribe`와 이벤트 전송 자체는 syscall이 아니다(소스 수준). RacePro 저장소의 Scribe 커널 코드 읽기에 따르면 `mq_open`은 fd 할당·설치에서 `files_struct` 쓰기를 남기고 큐 이름·큐 객체에는 자원 잠금이 없다(RacePro.md §2.5 K1–K3 `[기록]`, 실행 확인 없음) | `mq_open`이 "관련 이벤트"가 되는 조건은 엔지니어가 정한 자원 이름과 일치할 때다(p.4, p.7). Fig. 2에서 syscall의 경로 문자열 인자는 `data: input string`으로 남고, `files_struct` 잠금은 `desc = none`이며, 예시는 `llseek`·`close`(`files_struct` 잠금이 붙은 syscall)를 관련 없는 것으로 분류한다(p.4). 그러므로 큐 이름을 자원 이름으로 주면 `mq_open`의 이름 인자로 거를 수 있는 형식이라는 분석이 나온다 [I]. 그렇게 걸러진 쌍은 SB pipe 이름에 대한 것이고 AppId 설정·사용이 아니다 [I]. RRF가 쓴 Scribe 판본이 RacePro 저장소의 커널과 같은지, 엔지니어가 어떤 이름을 줄지는 서술이 없다 | — | **불명** |
| ④ 오류 반환: `EVS_GetAppDataByID` NULL → `ILLEGAL_APP_ID`, 상태 폐기 (B `cfe_evs.c` L182-L186; v7.0.1 L188) | 반환값. gdb 기록은 이 경로에서 syscall이 없었다고 적는다(`results/R2_gdb_v701.md` §2 `[기록]`) | 기록 대상이 아니다 | — | 분석 |

#### C1·C3·C5 보충 사건 대응표

| 사례·사건 | 커널에서 보이는 흔적 | RRF 입력(설계) | 라벨 |
|---|---|---|---|
| C1 생성: `OS_CountSemCreate`(`sem_init` + 이름 테이블 등록) | 0 syscalls (OSAL 자체 시험, probe) | 서술 없음 | 분석 |
| C1 조회: `OS_CountSemGetIdByName` | 0 syscalls (probe) | 서술 없음 | 분석 |
| C1 수정 후 재시도 대기: `OS_TaskDelay(100)` × 25 | sleep 계열 syscall(추정) | 이벤트 로그에 syscall로는 나타날 수 있다. 그 syscall을 "같은 자원"과 연결하는 경로는 서술이 없다 | 불명 |
| C3 SB `AppId` 저장, EVS `EVS_AppID`·등록 | syscall 없음 | 서술 없음 | 분석 |
| C3 TIME의 `CreatePipe` | 현재 OSAL에서는 `mq_open`(probe에서 부분 가시). C3의 원 판본(OSAL 4.1.1 시대)의 queue 구현이 `mq_open`을 쓰는지는 이 문서에서 확인하지 않았다 | 자원 이름 지정 여부에 달렸다(p.4, p.7). `mq_open`이라면 R2 ③과 같은 분석이다 | 불명 |
| C5 `OS_TaskCreate` | `pthread_create`. 에이전트가 쓴 작은 C 프로그램에서 `clone3`로 관측(`availability.md` §4 끝, 기록. OSAL·cFS 측정은 아니다) | `clone`은 RacePro의 추적 syscall 예(p.2). RRF가 이 이벤트를 어떻게 쓰는지는 서술이 없다 | 분석 |
| C5 ES 레코드 쓰기·`GetAppID` 조회 | 0 syscalls(OSAL 시험의 task 레지스트리 대기는 futex 대기만 보임) | 서술 없음 | 분석 |

### 3.6 이 절이 말하지 않는 것

- 어느 셀도 "RRF가 사례를 재현하지 못한다"는 뜻이 아니다. 실행하지 않았고, 사례의 접근이 RRF의 이벤트 로그에 들어오는지, 사람이 위치를 지정하는 경로가 있는지는 자료로 정해지지 않는다.
- "RRF는 공유 메모리를 관측하지 못한다"는 문장은 쓰지 않는다. 논문은 RRF가 메모리를 관측한다고도 하지 않는다고도 쓰지 않는다. 쓰지 않은 것을 쓰지 않았다고만 적었다. RRF가 쓰는 Scribe 자체는 별도 문서(`plan/methods/RacePro.md`)의 대상이다.
- "RRF는 순서위반을 못 다룬다"는 문장은 틀리다. 논문의 race 정의가 order violation을 포함한다(p.2).
- 이 절의 "분석"은 도구의 원래 설계에 대한 것이다. cFS 내부 상태를 이벤트로 지정하거나 소스 위치를 직접 주는 입력을 연구 쪽에서 추가하면 EXTENSION이며 원 도구의 결과가 아니다(로드맵 §10.4 끝, `availability.md` §5.1).
- C2의 "불명"은 도구의 속성이 아니라 사례 자료(비공개 앱)의 속성이다.

---

## 4. 실행 가능성과 결과 상태

### 4.1 요구 조건과 오늘 확인한 상태

| # | 요구 | 근거 | 오늘 상태 |
|---|---|---|---|
| R1 | RRF 구현(스크립트·도구·mPuD 생성기·oracle 저장소) | 논문은 구현 위치를 적지 않는다(전문 검색). §1.1 경로 1, 10, 13–15, 17 | **없음.** GitHub 저장소·코드·사용자 검색, Software Heritage, Zenodo, Internet Archive, 저자 홈, CPS-VO 레코드 어디에서도 구현·VM 이미지·스크립트를 찾지 못했다. 찾지 못했다는 것은 "접근한 출처에서 찾지 못함"이다. 존재하지 않는다는 확인이 아니다. 저자 학위논문 PDF는 열지 못해 구현 설명 유무를 모른다. **이것이 차단 요인이다** |
| R2 | 이벤트 보고 도구 Scribe(Linux 커널 패치) | 논문 p.3 "Linux 지원과 낮은 overhead 때문에". 판본은 적지 않았다 | **없음.** RacePro 쪽 기록은 Scribe가 i386 Linux 2.6.35에 내장하는 패치이고 설치에 커널 교체가 필요하다고 한다(`availability.md` §1, §2.1 `[기록]`). 2026-10-16T00:19Z 읽기 전용 재확인 `[M]`: 커널 `6.18.44-fc-v139`(2026-10-15에는 `fc-v114`), 아래 항목은 두 시각 모두 같다. 2026-10-16의 `/` 여유 공간은 1.9 GB였다. `# CONFIG_KEXEC is not set`, `# CONFIG_KEXEC_FILE is not set`, `# CONFIG_MODULES is not set`, `/proc/modules`와 `/dev/kvm` 없음, vmx/svm 플래그 0줄. 이 컨테이너는 다른 커널을 띄울 수 없다. 오늘은 syscall을 직접 불러 보는 확인(kexec_load, finit_module)을 포함한 명령이 권한 정책에 의해 거부되어 그 확인은 다시 시도하지 않았다. 10-08의 ENOSYS 기록을 쓴다 `[기록]`. **RRF가 쓴 Scribe 판본이 RacePro 저장소의 판본과 같다는 확인은 없다** |
| R3 | 정적 분석 도구 | 논문 p.1, p.5 "existing static program analysis tools"(이름 없음) | **불명.** 도구를 고를 수 없다(`unspecified_by_reference`) |
| R4 | 내부 oracle 저장소(TOCTOU, atomicity violation, race) | p.6. 구현은 저자의 이전 논문 [36](ISSRE 2013)을 인용한다 | 없음. [36]은 초록만 읽었다(2026-10-16, §1.1 경로 25–26). 초록은 oracle이 내부 프로그램·시스템 상태를 본다고만 적고 입력 형식은 적지 않는다 |
| R5 | PuD 소스와 `#ifdef` 빌드 | p.5–6 | cFE·OSAL·CF·BP 소스는 공개다(`ledger.md` `[기록]`). C2의 앱 소스는 비공개다. cFS 앱은 `dlopen`되는 공유 라이브러리이고(`yu-group.md` §7.1 `[기록]`) 논문은 프로그램 쌍을 각각 컴파일한다고만 서술한다. 둘의 정합은 서술이 없다 |
| R6 | `sleep` 시간 `ST`와 반복 수 `r` | p.5–6, p.8: 논문의 사례에서 1초와 10회 | **cFS 조건으로는 `unspecified_by_reference`.** 논문의 값은 논문 사례의 값이다. 조건 원칙은 레퍼런스에 있는 상수나 사건 기반 제어를 쓰라고 한다 |
| R7 | bug report(증상 포함) | p.4 | 다섯 사례의 이슈 본문은 있다. 댓글은 읽지 않았다(`availability.md` A6) |
| R8 | 실행 플랫폼(OS·커널·하드웨어) | 논문에 서술 없음 | `unspecified_by_reference` |

### 4.2 결과 상태 (로드맵 §10.4)

| 사례 | 오늘 적용되는 상태 | 사유 |
|---|---|---|
| C1 | `NOT_RUN` | 구현 없음(R1), Scribe 환경 차단(R2). 미탐지·미재현 수에 넣지 않는다 |
| C2 | `NOT_RUN` | 위와 같음. 더해서 앱 소스 비공개(R5)로 PuD를 정할 수 없다 |
| C3 | `NOT_RUN` | R1, R2. 원 플랫폼(Microblaze)과 Scribe의 Linux 전제가 다르다 |
| C5 | `NOT_RUN` | R1, R2 |
| R2 | `NOT_RUN` | R1, R2 |

`PATH_NOT_EXECUTED`, `OBSERVATION_GAP`, `ANALYSIS_GAP`, `REPORTED`는 도구를 실행했을 때의 상태라 해당 없다. 이 문서의 §3은 `ANALYSIS`(`availability.md` §5.1)이며 이 상태 중 어느 것도 대체하지 않는다.

### 4.3 PI 판단이 필요한 항목 (결정 후보. 여기서 정하지 않았다)

1. **`INAPPLICABLE`로 볼지.** 논문은 대상을 "두 process가 OS 자원에 접근하는 race"로 정의하고(p.1–2) thread 수준 기법을 다른 범주로 서술한다(p.1, p.10). p.1은 "operating at the thread level"인 탐지기가 process-level race를 다루지 못할 수 있다고 적는다. 거꾸로 한 process 안의 thread 사이 race를 범위에서 뺀다고 명시한 문장은 찾지 못했다(2026-10-16 재확인). 사례의 의존성은 한 process 안의 task 사이 메모리 상태다. 이것을 "원 기법의 명시된 입력·대상 범위 밖"으로 보면 `INAPPLICABLE`이고 이유와 원문 위치(p.1–2, p.6, p.10)를 보고한다. 보지 않으면 `NOT_RUN`이다. 로드맵 §1.2의 운용 정의("task 단위, OS process로 한정하지 않음")와 맞물려서 PI가 정할 일이다.
2. **재구현(`REIMPLEMENTATION`)을 할지.** 논문이 서술한 부분은 Algorithm 1, 쌍 열거, 네 패턴, 로그 거르기이고, 서술하지 않은 부분은 정적 분석 도구, 이벤트→소스 대응 구현, oracle 구현, Scribe 판본, `ST` 결정 세부다. 서술하지 않은 부분은 각각 `deviation` 또는 `unspecified_by_reference` 선택이 된다. 학위논문 본문을 읽은 뒤 결정하는 편이 맞다(§6). 원 도구 결과와 분리해서만 보고한다(로드맵 §10.4).
3. **저자에게 요청할지.** 아무에게도 연락하지 않았다(`availability.md` A2와 같은 상태).
4. **학위논문 PDF를 `research/cfs_procrace/paper/`에 두는 것.** P-PAPERS는 사용자의 PDF 제공을 착수의 필수 조건으로 만들지 않는다. 다만 이 PDF만 도전 페이지에 막혀 있고 구현 서술이 있을 수 있다.

---

## 5. 다섯 사례에서 부족한 것과 Kairos 기능 가설

### 5.1 RRF가 이미 하는 것 — 차별점으로 주장하지 않는다

| 항목 | 근거 |
|---|---|
| 보고된 race의 재현을 목표로 하고 bug report에서 자원을 정한다 | p.1, p.3–4 |
| order violation을 race 정의에 포함한다 | p.2 |
| 관련 syscall 쌍을 두 순서·네 패턴으로 열거하고 실제 실행에서 확인한다 | p.5–6 |
| 한 실행의 관측 순서에 갇히지 않는다(관측되지 않은 순서를 `sleep`으로 강제한다) | p.5 |
| 자원으로 거르기(culling)로 후보 수를 줄인다 | p.8–9 (Table III) |
| 같은 race의 후보 원인 여러 곳을 찾는다 | p.9 (Table IV) |
| targeted / incidental / new를 구분해 보고한다 | p.6, p.10 |
| oracle을 확장할 수 있다 | p.2, p.6 |
| 소스 위치까지 짚는다 | p.4–5 (Fig. 3) |

로드맵 §10.5의 "관측된 실행만 다룸" 행은 RRF에 근거가 없다.

### 5.2 가설 (근거가 있는 만큼만)

각 가설은 연구 가설과 개발 후보다. 실제 도구 실행으로 확인한 부족이 아니다. 로드맵 §10.5가 이미 가능한 기능을 재발명한 차별점으로 주장하지 않는다고 했으므로, 아래는 RRF의 서술에 근거가 있는 만큼만 적었다.

| ID | 가설 | 근거 | 근거 수준 | 확인할 것 | §10.5 행 |
|---|---|---|---|---|---|
| **H-RRF-1** | cFS 내부 상태의 공개·사용 사건(OSAL 이름 테이블 등록·조회, AppId 저장, 레지스트리 등록)이 RRF의 후보 생성 입력에 들어오지 않을 수 있다. Kairos는 내부 API·상태 변화·값 전달을 관측해 소스와 연결해야 한다 | RRF의 관련 이벤트는 syscall·resource lock 기록을 엔지니어가 정한 자원으로 거른 것이다(p.3–4, p.7–8). OSAL 자체 시험에서 C1의 생성·조회는 syscall 0건이었고(`availability.md` §4.2 `[기록]`), C3·C5·R2는 소스 수준 서술이다. 논문은 이 문제를 "syscall 안의 implicit 접근이 안 보임"이라고 반대편에서 정의한다(p.1–2) | 설계 분석 + OSAL probe. RRF를 실행하지 않았고 구현 상세(학위논문)를 읽지 못했다. 부분 근거 | RRF의 Component A를 다섯 사례에 적용해 사건이 기록되는지 확인하는 것은 `NOT_RUN`이다. 학위논문 본문이 syscall 외의 관측을 서술하는지 확인 | 관측 입력 부족: R2·C3·C5, 그리고 C1 |
| **H-RRF-3** | 사례의 판정 대상은 readiness precondition(준비 상태 전제)의 충족 여부와 그 결과(반환값·재시도·최종 초기화 결과)다. RRF의 oracle은 TOCTOU·atomicity·race 유형이다. Kairos는 readiness specification에 대한 판정과 반환·회복의 추적을 제공한다 | p.6의 oracle 유형, 평가가 file·socket에 한정됨, 출력이 race 분류(p.3, p.6). 사례 현상은 C1의 재시도 회복 vs 최종 실패, C5의 반환값 `CFE_ES_ERR_APPID`, R2의 "이벤트 요청 거부(출력 없음)" | **조건부.** oracle 저장소가 확장 가능하다고 논문이 적는다(p.2, p.6). 사용자가 작성한 oracle이 이 판정을 할 수 있다면 이것만으로는 Kairos 고유의 부족이 아니다. 그 oracle이 필요로 하는 사건이 H-RRF-1의 사건이므로 두 가설은 묶인다 | RRF 저장소의 oracle 인터페이스가 어떤 입력을 받는지(학위논문, 내부 oracle 논문 [36] 본문). [36]은 2026-10-16에 초록만 읽었고 입력 형식은 초록에 없다. 입력이 syscall 로그뿐이면 H-RRF-1과 같은 부족이다 | 오류 반환·회복 구분: C1. 시작 상태 vs 앱 고유 상태: C2 |
| **H-RRF-4** | 순서를 정해 두고 기다리는 제어가 필요한 사례(C1 재시도 종료, C2 late init 완료)에서 시간 기반 `sleep`은 대기 길이를 사례마다 정해야 한다. Kairos의 사건 기준 보류(P-C1HOLD)는 대기 길이를 정하지 않는다 | RRF의 제어는 `sleep`이고 길이를 시험 실행으로 정한다(p.5–6). 논문이 비결정성이 남는다고 적는다(p.6). 조건 원칙은 사건 기반 제어를 우선한다 | **차이일 뿐 RRF의 결핍으로 쓰지 않는다.** RRF의 목적은 재현이고 시간 기반이 그 목적에 맞는지는 별개다. P-C1HOLD는 [D] 결정이다 | 해당 없음(설계 차이의 기록) | 관측되지 않은 순서: C1·C3·C5 (제어 방식의 차이) |
| **H-RRF-5** | R2처럼 순서가 시작 직렬화로 고정된 것으로 보이는 경우, 반대 순서가 만들어지지 않는 이유와 가정을 결과에 설명해야 한다. RRF의 출력에는 그런 항목이 없다 | RRF는 쌍마다 두 순서를 `sleep`으로 시도한다(p.5). 출력은 race 분류와 이벤트 쌍 위치다(p.3, p.6). R2의 S1은 EVS·ES 초기화를 SB보다 앞세운다(`ledger.md` §1a `[기록]`) | **부분 근거.** R2가 schedule 의존인지 결정적 순서인지가 사례 기록상 미결이다(로드맵 §7.3). 반대 순서가 실제로 불가능한지는 gdb 3회 기록만으로 확정되지 않는다(`results/R2_gdb_v701.md` §3: "not a proof of determinism"). RRF가 이 경우를 어떻게 보고하는지는 서술에 없다 | R2 원시 기록과 소스로 반대 순서가 가능한지(별도 작업). 가능하면 이 가설은 약해진다 | 순서가 고정된 이유 설명: R2 |

### 5.3 근거가 부족해 가설로 올리지 않는 것

| 항목 | 이유 |
|---|---|
| 이름·ID·세대 연결(C1의 이름 → ID → 슬롯, C5의 task ID ↔ AppId)이 RRF에 없다 | 논문에 서술이 없다는 것뿐이다. 서술이 없음이 기능이 없음의 근거는 아니다. 자원 식별은 엔지니어의 입력과 Scribe의 이름 보고에 달려 있다(p.4, p.7). 구현을 읽기 전에는 가설로 올리지 않는다 |
| "RRF는 한 process 안의 thread 쌍을 못 다룬다" | 논문이 다루지 않는다고 쓴 곳이 없다. 단위가 process 쌍이라는 서술(p.3, p.6–7)과 thread 수준 기법을 다른 범주로 서술한 곳(p.10)뿐이다. 전제 2로 남기고 §4.3의 PI 판단 항목으로 둔다 |
| C2에서 RRF에 부족한 것 | 사례 자료(비공개 앱)가 없어 도구의 부족을 말할 근거가 없다 |
| "RRF는 쌍을 열거해서 비용이 크다" | 논문이 BASH–BASH 사례에서 613분을 보고한다(p.8). cFS 사례에서의 비용은 자료가 없다. 비용 비교는 이 문서의 질문 범위가 아니다 |
| 어떤 사례에서든 "RRF가 놓친다" | 실행하지 않았다. §3.6, 로드맵 §3.5 |

---

## 6. 확인하지 못한 것과 다음에 할 일

1. **저자 학위논문 본문**(UNL 2018, 공개 레코드에 1.4 MB PDF가 있다고 표기)은 Cloudflare 도전 페이지에 막혀 있다(2026-10-16 재시도에서도 같음, §1.1 경로 27). RRF의 구현 상세(정적 분석 도구, Scribe 판본, 커널, oracle 인터페이스)가 있을 수 있다. 읽기 전에는 H-RRF-1·H-RRF-3의 근거가 부분 근거에 머문다. 해결 경로는 PDF를 `research/cfs_procrace/paper/`에 두는 것이거나 접근 가능한 다른 경로다(P-PAPERS는 이를 필수 조건으로 만들지 않는다).
2. **IEEE 출판본**과 CPS-VO 사본(저자 수정본으로 보인다)의 동일성은 확인하지 못했다. 쪽 인용은 CPS-VO 사본 기준이다. 출판본과 쪽이 다를 수 있다.
3. **구현·아티팩트 없음.** 저자에게 요청하지 않았다(`availability.md` A2).
4. **논문의 내부 불일치.** 결과 서술은 "stress testing(STA)은 7쌍 중 6쌍에서 재현하지 못했다"고 하고 Table II도 UPDATEDB–UPDATEDB 한 쌍에서만 STA가 race를 냈다(p.8). 반면 p.8의 Discussion은 "첫 두 쌍에서 둘 다 재현했다"고 하고(Table II의 MV–RM 칸은 STA 0이다), 결론은 "STA는 둘만 재현했다"고 쓴다(p.10). 이 문서는 STA 수치를 쓰지 않는다. RRF 쪽 수치(7쌍 모두 재현)는 일관된다.
5. **Scribe 이슈 [31], [32]**(`columbia/linux-2.6-racepro#3`, `columbia/racepro#6`)는 접근 권한이 없어 읽지 못했다(논문 p.9의 서술만 읽었다).
6. **내부 oracle 논문**(Yu et al., ISSRE 2013, 논문의 [36], DOI 10.1109/ISSRE.2013.6698900)은 2026-10-16에 초록만 읽었다. UNL 기관 저장소 PDF는 Cloudflare 도전(403), IEEE는 AWS WAF 도전(202), Semantic Scholar는 `CLOSED`였다(§1.1 경로 25–26). RRF의 oracle 입력 형식은 아직 확인하지 못했다(`process_race_method_matrix.md` §5.1 A6 미해결).
7. **사례 이슈 댓글**을 읽지 않았다(`availability.md` A6). 사례 입력(bug report의 증상 서술 수준)은 이슈 본문 기준이다.
8. **전제 2**(task 쌍을 PuD 쌍으로 줄 수 있는지)는 서술이 없다. 원 도구로 답할 수 없고, 답은 구현이나 저자 설명에 있을 것이다.
9. 이 문서가 달라지게 하는 기존 문서의 칸: `baselines/availability.md` §1·§3 RRF 열, `baselines/yu-group.md` §3.2·§4·§5 R7·§7.2. 갱신은 이 작업의 범위 밖이다(§1.3).
10. 이 문서를 `process_race_method_matrix.md`(로드맵 §12 작업 6)에 넣을 때는 §3.4의 라벨과 §4.2의 상태를 따로 옮긴다. 관측 가능성, 표현 가능성, 재현 성공을 한 칸으로 합치지 않는다.
11. **쪽 번호 인용의 둘째 취득 경로**(`process_race_method_matrix.md` §5.1 A10)는 2026-10-16에도 얻지 못했다. IEEE(202 도전), Semantic Scholar(`CLOSED`), OpenAlex(429), Scholar Gateway(`ACCESS_DENIED`), 저자 홈 두 곳과 CV(서지뿐)다. CPS-VO 사본을 WAF 거부 뒤 다른 도구로 받은 취득 방식을 허용 범위로 볼지는 PI 확인 사항이다(matrix §5.4 항목 16). 확인 전까지 "단일 취득 경로, 독립 재확인 안 됨" 표시를 유지한다.
12. 이 보완(2026-10-16)으로 §3.4의 5 × 6 라벨은 바뀌지 않았다. R2 ③은 "불명" 그대로이고, ⓪ 행은 새로 넣은 "분석"이다. `process_race_method_matrix.md` §6.8의 RRF 행에 ⓪ 열은 없다. 반영 여부는 그 문서 담당이 정한다.

---

## 7. 출처 목록

- RRF(`[전문]`): S. Rattanasuksun, T. Yu, W. Srisa-an, G. Rothermel, *RRF: A Race Reproduction Framework for Use in Debugging Process-Level Races*, 2016 IEEE 27th International Symposium on Software Reliability Engineering (ISSRE), pp. 162–172. DOI 10.1109/ISSRE.2016.35. 사본: `https://archive.cps-vo.org/node/30361`(레코드), `https://archive.cps-vo.org/file/30362/download/194493`(파일 `Rattanasuksun_RRF_JA.pdf`). sha256 `e8c26e2fbca15e87c9eb837c1bddc0ade0493fb261f3ebd2fbbc200c06f00dce`, 11쪽.
- RRF 서지·초록(`[초록]`): `https://api.openalex.org/works/doi:10.1109/ISSRE.2016.35`, `https://api.crossref.org/works/10.1109/ISSRE.2016.35`, Semantic Scholar Graph API(corpus ID 326596). IEEE 출판본 `https://ieeexplore.ieee.org/document/7774517`은 열리지 않았다(`[미도달]`).
- 저자 학위논문(`[초록]`): S. Rattanasuksun, *A Comprehensive Framework to Replicate Process-level Concurrency Faults*, University of Nebraska–Lincoln, 2018-11. 레코드 `https://digitalcommons.unl.edu/computerscidiss/160`, `https://digitalcommons.unl.edu/dissertations/AAI13418914`. 본문 `[미도달]`.
- SysPro(`[2차]`): *SysPro: Reproducing System-level Concurrency Bugs from Bug Reports*, arXiv 2601.09616v1. sha256 `8fe5daa4c7c3794715632e4379d9160a9852b723dc4a649317ef6bae0a75e726`. RRF 서술은 p.2, p.23, p.31, p.35.
- DESCRY(`[2차]`): T. Yu, T. S. Zaman, C. Wang, ESEC/FSE 2017. `https://chaowang-vt.github.io/pubDOC/YuZW17.pdf`. sha256 `f9d7b57bbb0c1269f9143b4ec69286e90348a58fadd071f76f11022e06dfbf60`. RRF 인용은 p.8, p.11.
- SimEvo(RRF 언급 없음 확인): `https://par.nsf.gov/servlets/purl/10089644`. sha256 `143f7351489367d9301b97ac2317e3f1f263ac159ee737ffcff14bb0adc31be6`.
- 내부 oracle 논문(`[초록]`, 2026-10-16): T. Yu, W. Srisa-an, G. Rothermel, *An Empirical Comparison of the Fault-Detection Capabilities of Internal Oracles*, ISSRE 2013, pp. 11–20. DOI 10.1109/ISSRE.2013.6698900. 레코드 `https://digitalcommons.unl.edu/cseconfwork/255/`. 본문 `[미도달]`.
- 저자 페이지: `https://homepages.uc.edu/~tyu/`, `https://homepages.uc.edu/~tyu/links/research.html`, `https://tingting-yu.scholar.uconn.edu/`(`/publications/`, `/research/`, 2026-10-16). 저자 CV `https://homepages.uc.edu/~tyu/CV/tyu_cv.pdf`(2026-10-16, sha256 `61225fc211abcb40e6ddcf8b8779fcccb66c61946424e0c139e47ad76ac836c5`).
- R2 실행 증거(gdb 원시 기록): `env/logs/r2_g1..3_*`, `results/R2_gdb_v701.md`(§2, §3, §6, §7).
- RacePro 쪽 Scribe 커널 코드 읽기(`[기록]`, R2 ③): `plan/methods/RacePro.md` §2.5 K1–K3.
- 저장소 기록: `baselines/availability.md`(§1, §1.1, §3, §4.2, §5.1, A2, A6), `baselines/yu-group.md`(§3, §4, §5, §7), `cases/ledger.md`(§1a, §1b, §2), `plan/ROADMAP_20261015.md`(§1.2, §2.2, §3, §9, §10), `plan/methods/RacePro.md`, `plan/methods/SimRacer.md`(같은 형식), `Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md`, `CONDITIONS_POLICY.md`.
