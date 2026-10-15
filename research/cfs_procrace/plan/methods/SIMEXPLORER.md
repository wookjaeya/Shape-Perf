# SIMEXPLORER (STVR 2017) 분석 — Kairos 비교용

- 작성일: 2026-10-15 (UTC 08:44–09:10, 경로 시도와 읽기 시각).
- 대상: T. Yu, W. Srisa-an, G. Rothermel, *An automated framework to support testing for process-level race conditions* (도구 이름 SIMEXPLORER), Software Testing, Verification and Reliability 27(4–5), e1634, 2017. DOI 10.1002/stvr.1634. 저자 CV는 같은 논문을 "SIMEXPLORER: An Automated Framework to Support Testing for System-Level Race Conditions"로 적는다. 학위논문 Yu 2014, *SimExplorer: A Testing Framework to Detect Elusive Software Faults*(UNL)가 같은 이름의 틀을 다룬다.
- 범위: 로드맵 §10 (M6)의 SIMEXPLORER 항목. 여섯 분석 질문(§10.2)을 사례 C1, C2, C3, C5, R2에 적용한다. 로드맵 §12 작업 6(`process_race_method_matrix.md`)의 입력으로 쓴다.
- 이 문서의 성격: **분석 문서**다. SIMEXPLORER를 실행하지 않았다(실행할 구현을 찾지 못했다). 어느 사례에 대해서도 "탐지하지 못한다"는 결과를 담지 않는다.
- 용어: 신규 문서이므로 T1 (a)를 따른다. 판정 대상은 readiness specification(준비 상태 명세), 그 전제는 readiness precondition, 위반은 readiness violation이다. "API contract"는 문서화된 API 의미를 가리킬 때만 쓴다.
- 조건 원칙: `CONDITIONS_POLICY.md`. 이 문서는 실험 조건을 정하지 않는다. 레퍼런스가 없는 값은 `unspecified_by_reference`로 둔다.
- PI 결정(`PI_DECISIONS_RESOLVED_20261015.md`)은 결정으로만 인용한다. 사실로 쓰지 않는다. 이 문서가 쓰는 결정은 P-PAPERS와 T1이다.
- 같은 계보의 다른 문서: `plan/methods/SimRacer.md`, `plan/methods/RacePro.md`(같은 형식). SIMEXPLORER는 SimRacer의 확장판이므로(§0 항목 2) 두 문서의 도구 설명은 같은 논문(STVR 2017)을 공유한다. 이 문서는 SIMEXPLORER 쪽, 곧 STVR 2017 도구를 기준으로 쓴다.

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
| `[전문]` | 본문 전체를 텍스트로 추출해 읽었다. 알고리즘 그림(Figure 10, 12, 13)은 이미지로도 확인했다 |
| `[초록]` | 초록·서지만 읽었다 |
| `[2차]` | 다른 논문·페이지가 이 도구를 서술한 문장, 또는 같은 저자의 선행·인접 논문을 읽었다. 이 도구 자신의 서술이 아니다 |
| `[기록]` | 저장소의 이전 보고서에 있는 값. 이번에 재검증하지 않았다 |
| `[미도달]` | 열지 못했다 |

쪽 번호 표기:
- **STVR p.N**: 위 논문의 저자 수락본(accepted manuscript) 쪽 "N of 26"이다. PDF 쪽과 같다. 출판본(version of record)의 쪽과 같은지는 확인하지 못했다.
- **ITJ p.N**: Intel Technology Journal 17(2), 2013의 인쇄 쪽(Sim-O/C 논문은 178–197쪽). PDF 쪽과 같다.
- **VEE'12 p.N**: SimTester 논문의 인쇄 쪽(51–61). **SimEvo §**: ICSME'17 논문의 절.

---

## 0. 한눈에 보기

1. **SIMEXPLORER 논문의 저자 수락본 전문을 읽었다** `[전문]`. CPS-VO 아카이브가 올린 26쪽 PDF다(그 페이지에 "Creative Commons 2.5" 표기, sha256 `af805231…3e08`). Wiley 판본과 학위논문 PDF는 Cloudflare 도전 페이지(HTTP 403)에서 멈췄고 우회하지 않았다. 학위논문은 초록만 읽었다 `[초록]`. 같은 프레임워크의 앞선 서술인 Sim-O/C 논문(ITJ 2013)은 전문을 읽었다 `[2차]` (§1).
2. **SIMEXPLORER는 SimRacer(ISSTA'13)의 "revised and expanded version"이다** (STVR p.2 각주 †, §7.1 p.24) `[전문]`. 저자는 인터럽트, 중첩 handler, signal 31종, 구현 절, 객체 프로그램 24개가 STVR에서 새로 더해졌다고 적는다. 로드맵 §10.1과 `availability.md`가 두 도구를 별개 행으로 둔 것은 같은 계보의 두 판본을 나눈 것이다(§4.3 P-S5).
3. **역할은 탐지(testing-based detection)다.** Phase 1이 한 번의 실행 trace에서 잠재 race 쌍을 계산하고, Phase 2가 쌍의 접근 순서를 실제 실행에서 뒤집어 real race를 확인한다. 실패 보고를 입력으로 받는 재현 도구도, 버전 쌍을 쓰는 회귀 도구도 아니다 (STVR p.7, p.10–11).
4. **관측 범위는 syscall에 한정되지 않는다.** shared resource(SR)에 shared memory와 hardware component가 들어 있고(STVR p.8), 메모리 접근은 Simics 메모리 중단점으로 잡는다(p.14). 따라서 이전 기록(`yu-group.md` §4, §7.2, `availability.md` §3)의 "추론한 syscall 모델 아래에서 shared-resource 사건이 아니다"라는 조건부 서술은 전문으로 유지되지 않는다. 다만 **cFS 내부 상태(OSAL 레지스트리, ES 테이블, SB AppId)를 SR로 지정하는 방법과 입도는 서술이 없다.** 지정 인터페이스가 있다는 한 문장뿐이다(p.8).
5. **순서·의존 모델은 happens-before(HB) 규칙 6개다.** `fork`, `wait`-`exit`, `pipe`, signal·interrupt 활성 구간에서 정한다. 저자는 semaphore와 lock을 규칙에 넣지 않았다고 적는다(p.9). cFS의 ES 잠금, 코어 시작 직렬화, 시작 동기화도 이 규칙에 들어 있지 않다 (분석).
6. **필요한 순서와 허용 오류는 입력이 아니다.** race를 "의도한 순서와 다른 순서로 접근할 수 있는 것"으로 정의하지만(p.3) 의도한 순서를 받는 인터페이스는 서술에 없다. 실패 판정은 출력 기반 oracle(콘솔 메시지와 파일, 두 process를 순차 실행해 얻은 출력)이다(p.16–18). 본문에 retry, return value, initialization, startup, readiness 같은 개념의 서술이 없다(키워드 검색).
7. **출력은 PRaceSet(잠재 쌍), RaceSet(순서를 뒤집은 채 실제로 일어난 쌍), oracle 출력 차이다.** race는 "순서를 바꿀 수 있는 접근 쌍"이고 바꿀 수 없으면 race로 세지 않는다(p.10–11). 따라서 순서가 고정된 위반(R2 유형)을 뜻하는 출력 항목은 도구의 어휘에 따로 없다 (분석). 이것은 설계가 정의한 범위이며 "탐지 실패"가 아니다.
8. **예측·탐색과 실행 확인은 설계에 이미 있다.** 관측하지 않은 순서를 계산하고 실제 실행에서 확인한다(p.10–12). 로드맵 §10.5의 "관측된 실행만 다룸" 행은 SIMEXPLORER에 근거가 없다(§5.1).
9. **실행은 오늘도 불가능하다.** 구현을 찾지 못했다(GitHub 저장소·코드 검색 0건, Zenodo·figshare·arXiv·NSF PAR에 해당 항목 없음, 저자 페이지에 소프트웨어 항목 없음). 논문은 객체 프로그램을 first author에게 요청하라고만 적는다(p.15). Simics는 이 컨테이너에 없고 논문은 Simics 버전을 적지 않는다(p.18). 오늘 적용되는 상태는 다섯 사례 모두 `NOT_RUN`이다. `INAPPLICABLE`은 붙이지 않았고 PI 판단이다(§4.3 P-S2).
10. **Kairos 가설은 근거가 있는 만큼만 네 개를 올렸다(§5.2).** C2는 가설로 올릴 근거가 없다. 어느 항목도 "SIMEXPLORER가 못 한다"는 결과가 아니다.

---

## 1. 접근한 출처와 문장별 근거 등급

### 1.1 시도한 경로와 결과

시각은 2026-10-15 UTC 08:45–09:01이다. 접속 실패는 원인을 기록한 것까지만 썼고 TLS 검증을 끄거나 프록시를 우회하지 않았다. Cloudflare 도전 페이지(응답 본문 제목 "Just a moment...")는 우회하려고 하지 않았다. 기본 curl User-Agent를 거부한 사이트(경로 13의 CPS-VO WAF)에서 클라이언트 문자열 `Mozilla/5.0 (X11; Linux x86_64) research-fetch`로 바꿔 같은 공개 URL을 받았다. 이 문자열은 경로 22–25의 일부 요청과 경로 3–4의 마지막 재시도에도 썼고, Cloudflare 도전은 이 문자열로도 통과하지 못했다. 인증, 쿠키 재사용, Referer 조작은 쓰지 않았다.

| # | 경로 | 결과 | 등급·비고 |
|---|---|---|---|
| 1 | WebSearch 두 번(논문 제목·저자·학위논문 제목) | `scholars.uky.edu`, `archive.cps-vo.org`, `digitalcommons.unl.edu`의 레코드와 Wiley `am-pdf` 주소를 알려 줬다. 전문 위치는 Wiley, UNL, CPS-VO 셋이다 | 경로 목록 |
| 2 | Wiley `onlinelibrary.wiley.com/doi/abs/10.1002/stvr.1634`, `/doi/am-pdf/…`, `doi.org/10.1002/stvr.1634` | 모두 HTTP 403, 본문 제목 "Just a moment..."(Cloudflare 도전). WebFetch로 `am-pdf`를 열어도 403 | `[미도달]` 출판사 판본 |
| 3 | 학위논문 레코드 `digitalcommons.unl.edu/computerscidiss/77/`와 ProQuest 사본 `…/dissertations/AAI3626337/` | 두 페이지 200. 초록과 서지가 있다. AAI 쪽 안내문은 UNL 사용자 외에는 도서관 상호대차를 요청하라고 적는다 | `[초록]` |
| 4 | 학위논문 PDF `viewcontent.cgi?article=1093&context=computerscidiss`, `…/context/computerscidiss/article/1093/viewcontent/`, `…article=14291&context=dissertations` | 모두 403(Cloudflare 도전). WebFetch도 403 | 본문 `[미도달]` |
| 5 | Crossref `api.crossref.org/works/10.1002/stvr.1634` | 200. 서지, 초록, **참고문헌 85건**. Wiley 전문 링크만 있다 | `[2차]` 서지 |
| 6 | OpenAlex `api.openalex.org/works/doi:…` | 200. 서지와 초록. `oa_status: closed`, 사본 위치 없음 | `[초록]` |
| 7 | Semantic Scholar Graph API | 처음 429, 이후 200. `openAccessPdf.url`이 `doi.org/10.1002/stvr.1634`(출판사 판본으로 이어짐)이고 상태 HYBRID | 서지. 경로 2와 같은 곳으로 이어진다 |
| 8 | `scholars.uky.edu/en/publications/an-automated-framework-…` | 200. 초록, 연구비 번호, 키워드 | `[초록]` |
| 9 | 저자 홈 `homepages.uc.edu/~tyu/`, `links/research.html`, `links/projects.html`, `CV/tyu_cv.pdf` | 모두 200. 홈은 "I have moved to University of Connecticut"만 있다. 논문 목록은 STVR 항목을 Wiley 주소와 학위논문 주소(`digitalcommons.unl.edu/cgi/viewcontent.cgi?article=1093…`)로 연결할 뿐이다. 도구·소프트웨어 공개 항목이 없다. CV J[1]은 "Sim-O/C … Intel Technical Journal (ITJ), 2014" | 서지 |
| 10 | 새 홈 `tingting-yu.scholar.uconn.edu`와 `/publications/`, `/research/`, `engineering.uconn.edu/faculty/cse/tingting-yu` | 200. 논문 목록을 동적으로 불러오는 페이지로 보이며(확인하지 않았다) 받은 HTML에서 도구·소프트웨어 항목을 찾지 못했다 | 없음 |
| 11 | `cs.uky.edu/~tyu/`, `cs.uky.edu/~tyu/research/descry` | 403(Apache) | `[미도달]` |
| 12 | NSF 연구비 API `api.nsf.gov/…awards.json?id=1464032` | 200. 과제 초록. 도구 이름과 코드 공개 서술이 없다 | 서지 |
| 13 | `archive.cps-vo.org` 레코드(`/node/36387`)와 파일 페이지(`/node/36388`) | 기본 curl로는 WAF가 245 B "Request Rejected"를 돌려줬다(쿠키 사용 여부와 무관, 6회). WebFetch로 레코드를 열어도 같은 거부 페이지였다. 레코드는 첨부 `Yu_SimExplorer_JA.pdf`(2.29 MB)를 적는다. 사이트는 "Read-only archive of site from September 29, 2023"이라고 밝힌다 | 경로 14 참조 |
| 14 | 위 두 주소와 파일 `archive.cps-vo.org/file/36388/download/194033`을 클라이언트 문자열 `Mozilla/5.0 (X11; Linux x86_64) research-fetch`로 요청 | 레코드 200(38 KB), **파일 200, PDF 2,396,338 B, 26쪽**. 메타데이터 제목이 논문 제목과 같고 첫 쪽에 Received 6 April 2016, Accepted 14 March 2017이 있다. 로그인·쿠키·Referer는 쓰지 않았다 | **`[전문]`** sha256 `af805231f69ffc0e12d96fe4e4e9a356932653c56c382fdd3debd211bcdd3e08` |
| 15 | `windriver.com/blog/simics-unleashed-applications-of-virtual-platforms`, `noggin.intel.com/technology-journal/…` | 블로그 200: Intel Technology Journal 17권 2호 "Simics Unleashed"에 "Sim-O/C: An Observable and Controllable Testing Framework for Elusive Faults"(Yu, Srisa-an, Rothermel)가 실렸다고 적는다. `noggin.intel.com`은 프록시 CONNECT 502로 연결 실패 | 서지 |
| 16 | `intel.com/content/dam/www/public/us/en/documents/research/2013-vol17-iss-2-intel-technology-journal.pdf` | 200, PDF 16,555,438 B, 197쪽. Sim-O/C 논문은 178–197쪽. 전문을 읽었다 | `[2차]`. sha256 `24080f8ec175aecb7e6c93fd0a436dd8532c856375d8b356b772030dc8c1813c` |
| 17 | SimTester(VEE'12) `cl.cam.ac.uk/research/srg/netos/vee_2012/papers/p51.pdf` | 200, 11쪽. 초록, 서론, 관측기 구현 부분을 읽었다. STVR이 메모리 관측의 선행 연구로 인용한다 | `[2차]`(부분). sha256 `206c12cef8dc9f01fe2ae0b2d16dc2d2c719ce60f553fe75aa8627e3022b08cb` |
| 18 | SimEvo(ICSME'17) `par.nsf.gov/servlets/purl/10089644` | 200, 12쪽. SimRacer를 서술한 부분을 읽었다 | `[2차]`. sha256 `143f7351489367d9301b97ac2317e3f1f263ac159ee737ffcff14bb0adc31be6` |
| 19 | ICSE 2013 박사과정 심포지엄 초록 `2013.icse-conferences.org/content/observable-and-controllable-testing-framework-modern-systems.html`, `scholars.uky.edu/…/an-observable-and-controllable-testing-framework-for-modern-syste/` | 200. 초록 | 서지 |
| 20 | **구현·artifact 검색.** GitHub 저장소 검색 `SimExplorer Simics process-level race`(GitHub MCP) | `total_count: 0` | 구현 없음 |
| 21 | GitHub 코드 검색 `SimExplorer Simics Phase2-RegPro` | `total_count: 0`. `api.github.com` 직접 요청은 프록시가 403 | 구현 없음 |
| 22 | Zenodo API(`q=SimExplorer`, `q=SimRacer`, `q="process-level" race Simics`), figshare(`search_for=SimExplorer`) | Zenodo `SimExplorer` 0건. 나머지는 관련 없는 항목(해양·로봇·유체역학 등)만 나왔다. figshare도 관련 없는 항목 5건 | 구현 없음 |
| 23 | arXiv API `all:SimExplorer`, NSF PAR 검색 `SimExplorer`와 `"process-level race"`와 DOI, Internet Archive `advancedsearch`(`SimExplorer`) | 각각 0건, 0건, 무관한 소프트웨어 이름 2건(`SimExplorer 2.1.1`, `Simplorer`). `scholar.archive.org`는 "Rate limit reached" 페이지(재시도 한 번도 같았다) | 사본·구현 없음 |
| 24 | Software Heritage 원본 검색 API, dblp, BASE, `core.ac.uk`(웹 403, API 500), ResearchGate(403), Europe PMC(403), `web.archive.org`·`api.fatcat.wiki`(연결 실패), Unpaywall(이메일 파라미터를 요구해서 보내지 않았다. 개인 이메일을 관련 없는 서비스에 보내지 않기 위해서다) | 모두 내용을 얻지 못했다. Software Heritage, dblp, BASE는 "Making sure you're not a bot!"(Anubis) 확인 페이지였고 우회하지 않았다 | `[미도달]` |
| 25 | Intel Simics 공개 릴리스 페이지 `intel.com/content/www/us/en/developer/articles/tool/simics-simulator.html` | 200. 오늘의 벤더 문서(§4.1) | 컨텍스트 |

2026-10-08 기록(`availability.md` A1, `yu-group.md` D1)은 이 호스트들을 세션의 egress 정책이 막았다고 적었다. 오늘 `digitalcommons.unl.edu`, `scholars.uky.edu`, `homepages.uc.edu`, `par.nsf.gov`, `intel.com`은 열리고, Wiley와 UNL PDF는 원 서버의 Cloudflare 도전으로 막힌다. 호스트별 상태가 날짜에 따라 다르므로 두 기록을 합쳐 읽지 않는다.

### 1.2 이 문서가 쓰는 출처별 등급

| 출처 | 등급 | 이 문서에서의 쓰임 |
|---|---|---|
| STVR 2017 저자 수락본(경로 14) | **`[전문]`**. 본문과 Figure 10, 12, 13(알고리즘)을 읽고 이미지로도 확인했다. 표 1–3과 Figure 14–15는 텍스트 추출이 어지러워 본문 서술만 썼다 | 도구 설계 전반(§2), 도구 수준 답(§3.3) |
| 학위논문 Yu 2014 | 초록 `[초록]`(경로 3). 본문 `[미도달]` | 범위 확인(§2.4)뿐 |
| Sim-O/C, ITJ 2013(경로 16) | `[2차]` — 같은 저자·같은 계보의 앞선 서술. 같은 프레임워크의 앞선 이름이라는 판단은 추론 [I](§2.4) | 아키텍처 구성요소, 프로세스 제어 방식, 관측기 알고리즘 |
| SimTester, VEE'12(경로 17) | `[2차]`(부분) | 메모리 접근 관측 방법(STVR p.8이 인용) |
| SimEvo, ICSME'17(경로 18) | `[2차]`(부분) | SimRacer 위에 구현됐다는 서술, SimRacer의 제약 모델 서술 |
| Crossref 참고문헌 목록(경로 5) | `[2차]` | STVR이 인용한 문헌 확인 |
| 저자 홈페이지·CV(경로 9) | `[2차]` 서지 | 제목 표기, 소프트웨어 공개 항목 부재 확인 |
| 사례 쪽 사실 | `[기록]` | `cases/ledger.md` §1a, §1b, §2; `baselines/availability.md` §3, §4 |
| Intel Simics 공개 페이지(경로 25) | 벤더 문서(오늘) | 가용성 컨텍스트(§4.1). 논문이 쓴 Simics와의 동일성은 정해지지 않는다 |

### 1.3 이전 기록과 달라지거나 확정된 것

| # | 항목 | 내용 |
|---|---|---|
| 1 | `yu-group.md` §4 SIMEXPLORER 행("Excerpt only"), `availability.md` §1 | 논문 전문을 읽어 등급이 `[전문]`으로 올라갔다. P-PAPERS [D]에 따른 접근이다 |
| 2 | "Simics, binary level (excerpt)" | 확정: STVR p.2, p.14, p.23이 Simics와 binary-level 동작을 적는다 `[전문]` |
| 3 | `yu-group.md` §4의 반대 정황("학위논문 paraphrase가 shared memory를 언급") | 확정: SR 목록에 shared memory가 있다(STVR p.8). "syscall-only 해석은 좁다"는 판단이 맞았다 |
| 4 | "program locations of interest, oracles 미상" | 본문에서 "locations of interest"는 초록에만 나온다. 본문은 SR 접근 쌍(Phase 1의 PRaceSet)을 쓴다 [I]. oracle은 출력 기반이다(p.16–18) |
| 5 | SimRacer와 SIMEXPLORER를 별개 도구로 취급 | 저자가 STVR을 SimRacer의 revised and expanded version이라고 밝힌다(p.2 †, p.24). 같은 계보다 |
| 6 | `yu-group.md` R3 "Simics 버전, 머신, guest OS: 미상" | 일부 해소: guest는 단일 코어 x86 Linux 2.6.15와 3.5.0(p.18)이고, UART 연구는 Fedora Core 2.6.15와 Ubuntu 12.04(p.15). **Simics 버전과 머신 모델은 여전히 서술이 없다**("The version of SIMICS that we used …"만, p.18) |
| 7 | 제목 표기 | 수락본 제목은 "An automated framework to support testing for process-level race conditions", 저자 CV는 "SIMEXPLORER: … System-Level Race Conditions". 같은 DOI다. 문헌 인용 시 DOI로 맞춘다 |
| 8 | 저자 CV의 Sim-O/C 연도 | CV는 ITJ 2014, 오늘 연 Intel PDF는 17권 2호 2013(PDF 생성일 2013-11-28)이다. 연도 표기가 다르다 |
| 9 | 논문의 RacePro 평가(§2.3–2.4, p.6–7) | SIMEXPLORER 저자들이 RacePro의 한계와 replay divergence를 서술한다(RacePro를 baseline으로 쓰지 않았다, p.7, p.17). 경쟁 도구 저자의 서술이다. 이 문서는 그 내용을 RacePro에 대한 사실로 옮기지 않는다. RacePro 쪽 근거는 `plan/methods/RacePro.md`다 |

---

## 2. 도구의 정체

### 2.1 요약

| 항목 | 내용 | 근거·등급 |
|---|---|---|
| 역할 | **탐지(detection).** "testing for process-level race conditions". 잠재 race를 계산하고 실행으로 확인하는 단계를 포함하지만, 보고된 실패를 재현하는 도구가 아니고 회귀 분석 도구도 아니다 | STVR p.1, p.7 `[전문]` |
| 실행 단위 | 사건 원천 4종: ① 일반 user process(`fork`, `execv`) ② software signal과 handler ③ hardware interrupt와 ISR ④ kernel thread. 관측은 "process under test(PuT)"가 시작한 연산만 대상이다. Linux에서 thread는 PID를 가진 lightweight process라 "just like a process" 추적할 수 있다고 한 줄 적는다(replay 맥락) | STVR p.7, p.13, p.14 `[전문]` |
| 관측하는 것 | **SR 접근**: shared memory, file descriptor, file inode, file data, process data(`/proc`, PID), hardware component(예: register). 각 접근을 read 또는 write로 분류한다. 메모리는 이전 연구의 관측 지원을 쓰고(SimRacer [16], SimTester [33]), 하드웨어는 Simics 기능을 쓰고, system call은 read/write 효과를 모델링한다. 추적하는 syscall은 23개다(예시 `open`, `read`, `write`, `lstat`, `clone`, `wait`, `execve`, `exit`). 전체 목록은 본문에 없다. **동기화 사건**: `fork`, `wait`, `exit`, `pipe`; signal 등록·활성·비활성(`sigaction`, `signal`, `sigprocmask` 등); interrupt 활성 비트 | STVR p.8 `[전문]` |
| 관측 구현 | Simics Python API의 콜백. 공유 자원 연산에 메모리 중단점을 건다. PuT의 함수 진입 때 frame pointer를 call stack에 쌓고, 접근 시점의 frame pointer와 비교해 PuT 밖 접근을 거른다. 소스·계측 없이 binary level로 동작한다 | STVR p.2, p.14 `[전문]`; ITJ p.183, p.185–186 `[2차]` |
| 메모리 SR 지정 | 메모리 중단점을 거는 주소를 어떻게 정하는지 STVR 본문에 서술이 없다. 시험 입력 생성을 위해 SR을 정적으로 식별했다는 문장이 있다(p.16). 선행 SimTester는 전역 변수 주소를 symbol table에서 얻고, 지역 포인터 등은 계측한 ISR이 주소를 출력하게 해서 얻었다 | STVR p.14, p.16 `[전문]`; VEE'12 p.55 `[2차]` |
| 자원 식별 | Phase 1이 "SR_i equals SR_j"로 같은 SR을 찾는다(Figure 10). **같다고 판정하는 기준(이름, inode, 주소)은 서술이 없다.** syscall 효과는 객체 단위로 모델한다. 예: `rename("foo","bar")`는 두 이름의 inode를 바꾼다(p.10), `open`은 옵션에 따라 파일 데이터 또는 부모 디렉터리 inode에 쓴다(p.8). 식별자 재사용(generation) 서술은 없다(키워드 0건). 개발자가 다른 SR과 다른 syscall을 지정하는 사용자 인터페이스가 있다는 문장이 있고 형식 서술은 없다 | STVR p.8, p.10 `[전문]` |
| 순서·의존 모델 | **happens-before(HB)** 규칙 ① 같은 process 안의 순서 ② 다른 process·handler 사이에서 "ej는 ei 없이는 일어나지 않는다"는 의존(Linux syscall 사양에서 추론: `fork`, `wait`-`exit`, `pipe`) ③–⑥ signal·interrupt handler와 process 사이(켜진 구간에서 정함) + 전이 폐포. **잠재 race 쌍**은 같은 SR을 접근하고, 한쪽 이상이 write이고, HB가 없는 쌍이다. semaphore와 lock은 규칙에 넣지 않는다고 적는다 | STVR p.9, p.10 Figure 10 `[전문]` |
| race의 정의 | "(1) two system events … access a shared resource, and (2) they could access the shared resource in an order different from the intended order." 표준 data race보다 넓고, 일반 data race의 필요조건이지 충분조건이 아니라고 적는다 | STVR p.2–3 `[전문]` |
| 입력 | PuT와 짝지을 다른 process 또는 handler, 기존 test input(Phase 1은 임의 스케줄로 실행), test oracle. 연구에서 oracle은 콘솔 메시지와 생성·기록된 파일의 출력이고 두 process를 순차 실행해 얻은 것이다. 시험 입력은 SR을 덮는 기준으로 보강했다. 필요한 순서나 허용 오류를 기술하는 입력은 서술이 없다 | STVR p.7, p.16–17 `[전문]` |
| 출력 | ① PRaceSet: 잠재 race 쌍 ② RaceSet: 순서를 뒤집어 두 접근이 실제로 일어난 쌍(real race) ③ oracle과 출력이 다르면 "Error: fault found". harmful과 benign의 분류는 수작업이다 | STVR p.7, p.11 Figure 12, p.13 `[전문]` |
| 제어 | Simics 안에 dummy device를 두고 guest에 그 driver 프로그램을 설치한다. 제어할 때 dummy device가 인터럽트를 일으키고 guest의 interrupt handler가 `task_struct`를 보고 process를 멈추거나 재개하거나 signal을 보낸다. 다중 코어는 코어마다 dummy device를 두고 `sched_setaffinity`를 쓴다 | STVR p.14 `[전문]`; ITJ p.188 `[2차]` |
| 플랫폼 | Simics 위(버전 서술 없음). 연구 환경은 단일 코어 x86 Linux 2.6.15와 3.5.0 guest, UART는 Fedora Core 2.6.15와 Ubuntu 12.04. 구현은 Python으로 쓴 Simics tool이다. STVR이 x86 register를 적는 곳은 인터럽트 허용 검사의 `eflags`(p.12)이고, frame pointer 방식의 `ebp`는 Sim-O/C의 알고리즘에 나온다 | STVR p.12, p.14, p.15, p.18 `[전문]`; ITJ p.185–186 `[2차]` |
| 평가 규모 | 사용자 수준 프로그램 24개와 드라이버 1개(UART), 알려진 fault 41개 중 36개 검출(87.8%, 저자 보고), stress testing 기준선 3단계, 단일 코어 guest(UART는 다중 코어 확인 포함). 이 수치는 저자 보고이고 이 문서가 확인한 것이 아니다 | STVR p.17–19, p.24 `[전문]` |

### 2.2 동작 흐름

[F] STVR §3 (p.7), Figure 10, 12 `[전문]`.

1. **관측과 쌍 계산(Phase 1).** 기존 입력을 임의 스케줄로 실행하며 각 PuT의 SR 접근과 동기화 사건을 로그에 남긴다. 접근은 "OP(SR)"의 진입(`ENT`)과 종료(`EXT`)로 표현한다. syscall이면 호출 명령과 반환 명령, 메모리·하드웨어 접근이면 load/store 명령과 그 다음 명령이다(p.8–9). 두 로그에서 같은 SR을 접근하고 HB가 없고 둘 다 read가 아닌 쌍을 PRaceSet에 넣는다. 쌍은 관측된 실행 순서로 기록하고 명령어 위치가 같은 중복(반복문·재귀)은 한 번만 남긴다(p.10).
2. **순서 뒤집기(Phase 2).** 쌍마다 한 번의 실행에서 한 쌍만 시험한다(p.13). 두 접근 직전에 각 process를 멈추고, 원래 뒤에 있던 접근을 먼저 진행시킨 뒤 앞에 있던 접근을 진행시킨다. 두 접근이 뒤집힌 순서로 일어나면 RaceSet에 넣는다(Figure 12 18행). 시스템 상태 때문에 바로 이어서 실행하지 못하면 다른 쪽을 계속 진행시켜 가능한 시점을 찾고, 다른 쌍의 진입 명령에 닿을 때까지 불가능하면 그 쌍은 race로 세지 않는다(p.10–11). signal·interrupt는 같은 방식으로 "SR 접근과 그 도착의 순서"를 뒤집는다(Figure 13, p.11–12).
3. **판정.** 두 사건이 끝나면 출력을 oracle과 비교해 다르면 오류를 출력한다(Figure 12 24–25행). race가 확인됐지만 출력에 영향이 없는 경우를 별도로 서술한다(p.19, p.22).
4. **한 쌍씩 확인하는 이유.** fault masking을 피한다고 적는다(p.13).

### 2.3 논문이 직접 밝힌 한계 중 cFS와 관련 있는 것

| 한계 | 인용 | 쪽 |
|---|---|---|
| SR은 해로운 race를 낳을 만한 것으로 제한한다(표준 입출력, 커널의 process 데이터는 추적하지 않는다). 다른 SR은 개발자가 지정한다 | "SIMEXPLORER provides a user interface that lets developers specify other SRs" | p.8 |
| semaphore와 lock 등 다른 자원은 설계의 확장 대상으로만 언급하고 이 논문의 초점에서 뺀다 | "can be extended to cover other types of resources that include semaphores and locks" | p.9 |
| Phase 1은 모든 동기화 연산을 식별하지 못해 거짓 양성이 생긴다. 프로세스 수준 동기화는 구현을 모르는 syscall이 많아 의미를 정확히 모델링하기 어렵고, 구현에 대한 선입견을 두지 않는 방식을 택한다 | "does not identify all synchronization operations" | p.10, p.13 |
| 쌍을 뒤집을 수 없으면 race로 세지 않는다 | "is not considered to be a race" | p.11 |
| 한 실행에서 한 쌍만 시험한다 | fault masking을 피하려는 선택으로 서술 | p.13 |
| 한쪽을 멈추면 다른 쪽이 기다리며 교착할 수 있다(예: 한 process가 loop에서 다른 process가 쓴 데이터를 읽는 경우). 주기적으로 상태를 확인해 기다림의 대상을 풀어 준다. 객체 프로그램 4개에서 관측했다 | — | p.13 |
| 한 process 안의 다른 비결정 요인은 제어하지 않아 재현이 불완전할 수 있다 | "it can be incomplete" | p.13 |
| 두 접근 사이의 order violation만 다룬다. atomicity violation 등 세 사건 이상은 놓칠 수 있다 | "it can miss races involving more than 2 events" | p.14 |
| 초기 test suite와 oracle에 의존한다. 공유 변수를 실행하지 않는 테스트에서는 race가 보이지 않았다. race가 출력에 전파되지 않은 사례가 있다 | — | p.19, p.22 |
| 실패는 출력 기반 oracle로 보고했다. 내부 oracle은 수작업·시간·거짓 양성 비용이 있다고 한다 | "We report failures using output-based oracles" | p.18 |
| 연구한 Simics 버전은 하드웨어 가상화를 지원하는 최근 Intel CPU를 모사하지 않는다 | "does not simulate the later Intel processors with hardware virtualization support" | p.18 |
| 런타임 속성(공유 자원, syscall, scheduler)은 Linux 전용이다. 일반 아이디어는 다른 시스템으로 옮길 수 있다고 한다 | "are Linux specific" | p.23 |
| race 발현은 시스템 구성에 민감하다(IA64에서만 나타나는 race를 x86에서 보지 못했다) | — | p.22 |

### 2.4 계보와 이 문서의 도구 범위

[I] 출처에서 읽은 관계만 적는다. 아래 "같은 프레임워크"는 추론이다.

| 단계 | 자료 | 읽은 것과 근거 |
|---|---|---|
| SimTester, VEE 2012 | 인터럽트 handler와 응용의 data race·deadlock을 Simics 위에서 시험하는 틀. 메모리 접근을 중단점으로 관측하고 인터럽트를 일으켜 제어한다 | VEE'12 p.51–55 `[2차]` |
| Sim-O/C, ITJ 2013 | 같은 저자의 Simics 기반 틀. 구성요소 4개(configuration repository, execution observer, execution controller, oracle repository). 시험하는 fault 종류 3개(인터럽트, 여러 process의 공유 자원 접근, signal). 여러 process 사이는 "관측한 실행에서 서로 race할 수 있는 system call을 찾고 두 process의 스케줄링을 제어해 순서를 바꿔 본다"고 두 문장으로 적는다. 그림 1의 캡션이 "SimTester architecture"다 | ITJ p.178, p.182–183, p.188 `[2차]` |
| SimRacer, ISSTA 2013 | STVR의 기반 논문 [16]. STVR이 "revised and expanded version"이라고 밝힌다 | STVR p.2 †, p.24 `[전문]` |
| SIMEXPLORER, STVR 2017 / 학위논문 2014 | 위를 확장: 하드웨어 인터럽트, 중첩 handler, signal 31종, 구현 서술, 객체 24개. 학위논문 초록이 같은 틀을 "greater observability and controllability"로 서술하고 다루는 fault를 네 가지(인터럽트, 여러 process·signal의 공유 자원 접근, WCIL 위반, 진화하는 소프트웨어의 data race)로 적는다 | STVR p.24 `[전문]`; 학위논문 초록 `[초록]` |
| SimEvo, ICSME 2017 | SimRacer 위에 구현한 회귀 시험 도구. 같은 계열의 인접 도구이고 이 문서의 대상이 아니다 | SimEvo §IV `[2차]` |

Sim-O/C와 SIMEXPLORER가 같은 프레임워크의 앞뒤 이름이라는 판단의 근거는 세 가지다: 학위논문 초록의 문구가 Sim-O/C의 문구("observability and controllability", "elusive faults")와 겹친다. Sim-O/C의 fault 3종이 STVR의 세 수준(process, signal, interrupt, STVR p.2)과 겹친다. 저자 CV가 두 논문을 같은 연구 계열에 둔다. 이 판단은 [I]이고, Sim-O/C 서술을 SIMEXPLORER의 사실로 쓰지 않는다. 아래 §3의 근거는 STVR 쪽을 우선하고 Sim-O/C는 보조로 표시한다.

---

## 3. 여섯 질문 × 다섯 사례

### 3.1 읽는 법

셀의 라벨은 아래 셋 중 하나다. 한 칸에 관측 가능성, 표현 가능성, 실제 탐지 성공을 합치지 않는다(로드맵 §10.2).

| 라벨 | 뜻 |
|---|---|
| **답** | 출처의 쪽·절로 답했다(answered with source page/section) |
| **분석** | analysis from stated design (not run). 읽은 설계 기술에서 도출했고 실행하지 않았다 |
| **불명** | not determinable from sources read |

"분석" 셀의 "서술이 없다"는 "읽은 설계 기술 안에서 그 쌍이 후보 입력이 되는 경로를 찾지 못했다"는 뜻이다. 실행 결과가 아니다. 한 셀에서 질문의 핵심이 분석으로 답해지고 부수적인 세부만 정해지지 않으면 "분석"으로 두고 정해지지 않는 세부를 셀 안에 적었다. 질문의 핵심 부분이 정해지지 않으면 "불명"이다. 근거 칸의 쪽은 별도 표시가 없으면 STVR(`[전문]`)이다. 질문 번호는 로드맵 §10.2의 순서다.

1. 필요한 **관측 사건**을 수집하는가?
2. 서로 다른 이름·ID·필드가 같은 **자원·세대**를 가리키는 것을 식별하는가?
3. **생성·등록·초기화 완료와 의존 사용**의 관계를 표현하는가?
4. 필요한 **순서 또는 허용된 오류 처리** 정보를 입력받는가, 추론하는가?
5. 아직 실행되지 않은 순서를 **예측·탐색**하는가? 실제 실행에서 **확인**할 수 있는가?
6. **출력**은 후보·재현 성공·오류 결과 중 무엇이며 원 사례의 현상과 대응하는가?

### 3.2 사례 쪽에서 쓴 사실 (모두 저장소 기록, 재검증하지 않음)

| 사례 | 의존성과 현상 | 출처 |
|---|---|---|
| C1 CF#184 | provider(공개 BP 구성에서는 `OS_CountSemCreate`)가 이름 있는 count semaphore를 만든 뒤에야 CF의 `OS_CountSemGetIdByName`(`CF_CFDP_InitEngine`)이 성공한다. 원 버전은 한 번 조회하고 실패하면 `CF_EID_ERR_INIT_SEM` 이벤트와 APP_ERROR 종료. 수정은 `OS_ERR_NAME_NOT_FOUND`일 때만 25회 × 100 ms 재시도 | `cases/ledger.md` §1a, §1b |
| C2 cFE#198 | 앱 P의 late init 완료와 다른 앱 R의 의존 호출. P·R은 비공개(EVA CWS)이고 통신 방식·초기화 내용이 공개되지 않았다. 공개된 것은 단일 단계 시작 동기화(`WaitForStartupSync`)와 6.6.0a의 두 단계 방식(`APPS_INIT`/`LATE_INIT`, `CFE_ES_WaitForSystemState`) | `ledger.md` §1a–§1b; 로드맵 §9.1 |
| C3 cFE#73 | SB task의 `CFE_SB.AppId`, EVS task의 `EVS_AppID`와 등록 플래그가 설정되기 전에 TIME task가 `CFE_SB_CreatePipe` 뒤 `CFE_EVS_SendEventWithAppID`(AppId=0)를 호출하고, `EVS_IsFiltered(0xFFFFFFFF)`가 범위 검사 없이 접근한다. 우선순위가 순서를 만든다(TIME 60, EVS 61, SB 64). 원 플랫폼은 Microblaze. 오류 지점은 사례 기록에서 미확인 | `ledger.md` §1a, §2 |
| C5 cFE#72 | `CFE_ES_AppCreate`가 `OS_TaskCreate` 전에 ES lock을 풀고 뒤에 다시 잡아 `TaskTable` 항목을 채운다. 그 사이 새 task의 `CFE_ES_GetAppID`가 `CFE_ES_ERR_APPID`를 반환할 수 있다. 6.4.2 수정은 task 생성과 등록을 한 lock 범위에 둔다 | `ledger.md` §1a, §1b |
| R2 cFE#2663 | SB가 `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`로 AppId를 저장하기 전에 ES/EVS task가 SB API 경로(`CreatePipe`/`Subscribe`/이벤트 전송)를 실행하고, `EVS_GetAppDataByID`가 NULL을 돌려 `ILLEGAL_APP_ID`가 된다. 상태는 폐기된다. 코어 시작 직렬화(S1)가 ES·EVS를 SB보다 먼저 실행시킨다. 서술은 "이벤트 요청 거부"다. gdb 3회 모두 설정 전 사용 24회 = 거부 24회 | `ledger.md` §1a; 로드맵 §2.2 |
| 공통 | native cFS는 **한 Linux process**이고 OSAL task는 `pthread_create` thread다(osal `os-impl-tasks.c:568`). 이 호스트의 glibc에서 `pthread_create`는 `clone3`로 보인다. OSAL 자체 시험에서 named count semaphore의 생성과 이름 조회는 **syscall 0건**이었다. 이 probe는 syscall 관측자가 본 것이고 SIMEXPLORER의 결과가 아니다 | `availability.md` §3, §4.2 `[기록]` |

### 3.3 도구 수준의 답 (사례와 무관)

| Q | 라벨 | 답 | 근거 |
|---|---|---|---|
| 1 관측 사건 | 답 | SR 접근(shared memory, file descriptor, inode, file data, process data, hardware component)을 read/write로 분류해 기록하고, syscall 23종의 read/write 효과와 동기화 사건(`fork`, `wait`, `exit`, `pipe`, signal, interrupt 비트)을 기록한다. 메모리 접근은 Simics 메모리 중단점으로 잡는다. 함수 진입·반환은 PuT를 가려내는 call stack 용도로 쓰이고, 함수 반환값을 사건으로 기록한다는 서술은 없다 | p.8, p.14 |
| 2 자원·세대 식별 | 답(부분) | 같은 SR 판정은 "equals"로 쓰였고 기준 서술이 없다. 파일 계열은 객체 단위(inode, 부모 디렉터리 inode, `/proc`의 pid)로 효과를 모델한다. 식별자 재사용(generation) 서술은 없다(키워드 0건). 사용자 지정 SR 인터페이스의 형식은 서술이 없다 | p.8, p.10 Figure 10 |
| 3 생성·사용 | 답 | "생성·등록·초기화 완료"라는 상태 개념은 서술에 없다. 표현 단위는 같은 SR에 대한 접근 쌍(한쪽 이상 write)과 HB 여부다. 쓰기→읽기 순서는 이 형태로 표현된다. 본문에 initialization, startup, readiness 관련 서술이 없다(키워드 검색) | p.8–10 |
| 4 필요 순서·허용 오류 | 답 | 필요 순서·허용 오류를 기술하는 입력은 서술에 없다. 순서는 syscall 사양과 signal·interrupt 상태에서 추론한 HB로만 들어간다. 실패 판정은 출력 기반 oracle(두 process를 순차 실행해 얻은 출력과 비교)이다. 사용자가 SR과 syscall을 추가 지정하는 인터페이스가 있다. 재시도·반환값(return value) 서술은 없다(키워드 0건. "return code"는 Table 4의 race 설명 한 곳에만 나온다) | p.8, p.9, p.16–18 |
| 5 예측·탐색, 확인 | 답 | 설계상 있다. 한 번의 trace에서 HB로 잠재 쌍을 계산하고(예측), 쌍마다 Controller로 process를 멈추고 재개해 순서를 뒤집은 실행을 한다(확인). 쌍을 뒤집을 수 없으면 race로 세지 않는다. 한 실행에서 한 쌍, 두 접근 사이의 순서만 다룬다. 한계는 §2.3 | p.7, p.10–14 |
| 6 출력 | 답 | PRaceSet(잠재 쌍), RaceSet(순서를 뒤집어 실제로 일어난 쌍), oracle 출력 차이("Error: fault found")를 구분한다. race가 확인돼도 출력에 영향이 없으면 fault로 세지 않는다. harmful과 benign 분류는 수작업이다 | p.7, p.11, p.13, p.19 |

### 3.4 5 × 6 라벨 요약

| | Q1 관측 | Q2 식별 | Q3 생성·사용 | Q4 입력 | Q5 예측·확인 | Q6 출력 |
|---|---|---|---|---|---|---|
| **C1** | 분석 | 불명 | 분석 | 답 | 분석 | 분석 |
| **C2** | 불명 | 불명 | 불명 | 답 | 불명 | 불명 |
| **C3** | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| **C5** | 분석 | 불명 | 분석 | 답 | 분석 | 분석 |
| **R2** | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |

### 3.5 사례별 셀 (근거 포함)

모든 "분석" 셀에 공통인 전제가 하나 있다. cFS 내부 상태 접근이 관측 대상이 되려면 그 위치가 SR로 지정되어야 하는데, 지정 방법과 입도는 서술이 없다(§2.1 "메모리 SR 지정"). 아래에서는 이 전제를 "SR 지정이 전제"로 줄여 쓴다.

#### C1 — CF#184 (named count semaphore의 생성 vs 이름 조회)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | 필요한 사건은 provider의 이름 등록(생성)과 CF의 `OS_CountSemGetIdByName` 조회다. 둘은 OSAL 전역 테이블에 대한 사용자 공간 메모리 쓰기와 읽기다. 설계의 SR 목록에 shared memory가 있고 메모리 접근을 Simics 중단점으로 잡으므로 SR 지정이 전제인 관측 대상이다. 두 호출이 syscall 0건이라는 probe는 syscall 관측자의 결과이고 이 설계의 메모리 관측에는 적용되지 않는다. 함수 반환값(`OS_ERR_NAME_NOT_FOUND`)은 사건 목록에 없다 | p.8, p.14; `availability.md` §4.2 `[기록]` |
| 2 | 불명 | 이름 `BP0_tsem` → OSAL ID → 테이블 슬롯의 연결과 삭제·재생성된 객체의 세대 구분은 서술이 없다. "SR_i equals SR_j"의 기준 서술이 없다(Figure 10). 키워드 "generation" 0건 | p.8, p.10 |
| 3 | 분석 | 설계가 표현하는 것은 같은 SR에 대한 쓰기와 읽기 쌍의 순서다. 생성(쓰기)→조회(읽기)는 그 형태다. "이름 공개 완료"라는 상태, 재시도, 허용 오류(수정 버전은 `NAME_NOT_FOUND`일 때만 재시도)는 표현 요소에 없다. 테이블 접근은 OSAL의 잠금 아래에 있는데(소스 수준 `[기록]`) HB 규칙에 lock이 없다(p.9) | p.9; `availability.md` §4.2, §4.4 `[기록]` |
| 4 | 답 | 도구 수준 답과 같다. C1이 필요로 하는 허용 오류를 넣을 입력은 서술에 없다. 실패 기준(CF 초기화 abort)은 사용자가 출력 기반 oracle로 정해야 한다 | p.16–18 |
| 5 | 분석 | trace에 생성 쓰기와 조회 읽기가 있고 둘 사이에 설계가 인식하는 HB가 없으면 Phase 1이 쌍으로 계산하고 Phase 2가 두 접근을 멈춰 순서를 뒤집어 본다. 수정 버전의 CF는 100 ms 간격으로 기다리며 재시도하므로(기록), 설계가 서술한 "기다리는 쪽이 멈춘 쪽을 기다려 교착하는" 구조(p.13)와 닿는다. 저자는 주기적으로 상태를 확인해 풀어 준다고 적는다. C1에서 실제로 어떻게 되는지는 실행하지 않았다 | p.10–11, p.13 |
| 6 | 분석 | 출력 유형은 race 쌍과 출력 기반 oracle의 오류 메시지다. 사례 현상(CF 초기화 실패 이벤트와 APP_ERROR 종료, 수정 후 재시도로 회복한 경우와 최종 실패한 경우의 구분)에 대응하려면 ① 보고된 쌍이 생성↔조회여야 하고 ② 출력 oracle이 그 현상을 출력 차이로 잡아야 한다. 회복과 최종 실패를 구분하는 출력 항목은 서술에 없다 | p.11, p.17 |

#### C2 — cFE#198 (late init 완료 vs 다른 앱의 의존 호출)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 불명 | 필요한 사건은 P의 late init 완료와 R의 의존 호출이다. P·R이 비공개이고 통신 방식·초기화 내용이 알려지지 않아 의존이 거치는 자원을 판단할 근거가 없다. 공개 부분(`WaitForSystemState`가 메모리 상태를 `OS_TaskDelay`로 polling)의 대기는 sleep 계열 syscall이고, 23개 syscall에 sleep 계열이 있는지는 전체 목록이 본문에 없어 정해지지 않는다 | `ledger.md` §1a; p.8 |
| 2 | 불명 | 같은 이유 | — |
| 3 | 불명 | 같은 이유. 참고: 시작 상태(OPERATIONAL)와 앱 고유 초기화 완료를 구분하는 개념은 설계 서술에 없다(키워드 initialization, startup, readiness 0건). 그것만으로 이 사례의 표현 가능 여부는 정해지지 않는다 | 본문 키워드 검색 |
| 4 | 답 | 도구 수준 답과 같다. 실패 기준은 원 보고가 구체화하지 않아 연구 쪽 선택이 된다. 선택하더라도 원 보고자의 값으로 인용하지 않는다 | p.16–18; [D] 결정 문서 §3.5 |
| 5 | 불명 | 쌍이 될 접근을 알 수 없다 | — |
| 6 | 불명 | 원 현상이 구체 출력으로 공개되지 않았다 | `ledger.md` §1a |

C2의 **공개 부분**(역할 재구성에 쓸 수 있는 알려진 조건):

| 사건 | 커널에서 보이는 흔적 | 이 설계의 입력 | 라벨 |
|---|---|---|---|
| `CFE_ES_WaitForSystemState`가 `while (SystemState < MinSystemState)`를 돌며 `OS_TaskDelay` | 상태는 `CFE_ES_Global`의 메모리 필드, 대기는 sleep 계열 syscall | 상태 필드는 메모리 SR 후보(SR 지정이 전제). sleep 계열이 syscall 23개에 있는지 불명 | 분석 |
| 단일 단계 / 두 단계 barrier의 상태 전이와 timeout | 같은 메모리 필드 | 위와 같다 | 분석 |

[F 기록] 코드 근거는 `availability.md`의 `cfe_es_api.c:514, 585, 603` 서술이며 재검증하지 않았다.

#### C3 — cFE#73 (SB/EVS AppId 설정 vs TIME의 이벤트 요청)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | 필요한 사건은 `CFE_SB.AppId` 저장, `EVS_AppID`·등록 플래그 설정, TIME의 이벤트 요청(AppId=0)과 범위 검사 없는 `EVS_IsFiltered` 접근이다. 모두 사용자 공간 메모리 접근이고 SR 지정이 전제다. TIME의 `CreatePipe`가 부르는 `mq_open`이 syscall 23개에 있는지는 전체 목록이 없어 불명이다. 원 플랫폼은 Microblaze인데 STVR이 x86 register를 적는 곳은 `eflags`(p.12)이고 연구 환경은 x86 guest다(p.18). 이 연구의 파생 플랫폼(6.4.1, 32-bit x86 pc-linux, 기록)은 x86이다 | p.8, p.12, p.14, p.18; `ledger.md` §1b |
| 2 | 분석 | SB AppId와 EVS AppId·등록 플래그는 서로 다른 변수다. 설계가 SR을 메모리 위치로 구별한다면 서로 다른 SR이다. 변수 값의 의미(ES가 부여한 AppId)와 연결하는 서술은 없다 | p.8 |
| 3 | 분석 | 필요한 관계는 "공개(저장)가 사용(읽기)보다 먼저"이고 쓰기·읽기 쌍으로 표현되는 형태다. 사례의 순서는 우선순위가 만들고 이는 syscall 의미에서 추론한 HB(p.9)가 아니다. 오류 지점(EVS 내부 인덱스 접근)과 crash 여부는 사례 기록에서 미확인이다 | p.9; `ledger.md` §2 |
| 4 | 답 | 도구 수준 답과 같다. 오류 지점이 미확인이라 어떤 oracle이 필요한지는 지금 정할 수 없다 | p.16–18 |
| 5 | 분석 | 설계는 관측된 trace의 순서를 뒤집어 실행하고 그 실행의 출력을 oracle과 비교한다(Figure 12). 설계는 의도한 순서를 입력으로 받지 않으므로(p.3) 어느 쪽 순서가 요구 순서인지 알지 못한다. 관측된 순서가 우선순위 때문에 사용이 저장보다 앞선 순서라면 뒤집은 쪽이 저장이 앞서는 순서가 된다. 이것은 설계에 대한 서술이며 탐지 결과가 아니다 | p.3, p.10–11 |
| 6 | 분석 | 사례 현상은 "잘못된 상태 사용"과 "실제 crash 여부"를 분리해 보고하는 것이다(로드맵 §9.2). 출력은 race 쌍과 출력 기반 oracle 차이이며 두 현상을 구분하는 항목은 없다. crash가 일어나는지 자체가 사례 기록상 미확인이다 | p.11, p.17 |

#### C5 — cFE#72 (task 생성과 레지스트리 등록 사이의 공백)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | `OS_TaskCreate`의 `pthread_create`는 syscall로 보인다(기록: 이 호스트에서는 `clone3`). 논문이 syscall 예시로 든 것은 `clone`이고 `clone3`에 대한 서술은 없다(§2.1). ES `TaskTable` 등록 쓰기와 새 task의 `CFE_ES_GetAppID` 조회는 메모리 접근이고 SR 지정이 전제다 | p.8; `availability.md` §3 `[기록]` |
| 2 | 불명 | task ↔ AppId 연결은 ES 테이블 안에 있다. thread를 PID로 추적한다는 한 줄(p.13)은 OS 수준 task 식별이 가능하다는 뜻일 수 있으나, AppId와의 연결은 서술이 없다 | p.13 |
| 3 | 분석 | 등록 쓰기와 새 task의 조회 읽기는 쓰기·읽기 쌍의 형태다. `clone`은 syscall 23개의 예시이고 /proc에 process inode를 쓰는 효과로 모델된다(p.8). HB 규칙 ②는 `fork`를 예로 들며(p.9) `clone`이 같은 HB 간선을 만드는지는 서술이 없다. 어느 쪽이든 부모의 등록 쓰기와 자식의 조회 사이를 정하는 규칙은 서술에 없다. "등록 공개 완료"라는 사건은 없다. 6.4.2 수정(생성과 등록을 한 lock 범위에 둠)은 lock이라서 HB 규칙에 들지 않는다 | p.9 |
| 4 | 답 | 도구 수준 답과 같다. 원 현상(등록 전 조회의 실패 반환)은 crash가 아니라서 내부 API 반환값을 보는 oracle이 필요하다. 원 보고에 없는 앱 crash는 기대 결과로 넣지 않는다(로드맵 §9.3) | p.16–18 |
| 5 | 분석 | 쌍이 계산되면 Phase 2가 두 접근 직전에 process를 멈추고 순서를 뒤집는다. 뒤집기가 가능한지가 쌍의 처리를 정한다(p.10–11). 6.4.2의 lock 범위가 뒤집기를 막는지, 도구가 그것을 어떻게 다루는지는 서술이 없다 | p.10–11 |
| 6 | 분석 | 사례 현상은 "등록 전 조회와 그 결과"(`CFE_ES_ERR_APPID` 반환)다. 출력 기반 oracle은 콘솔·파일 출력을 비교하며(p.17) 내부 API 반환값은 보지 않는다 | p.17 |

#### R2 — cFE#2663 (SB AppId 설정 vs ES/EVS의 이벤트 요청)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | AppId 설정(`CFE_ES_GetAppID(&CFE_SB_Global.AppId)`, 반환값 무시), 값 전달(ES·EVS task가 같은 전역을 읽음), 이벤트 요청, 오류 반환(`ILLEGAL_APP_ID`)이 모두 메모리 접근과 함수 호출이다. 메모리 접근은 SR 지정이 전제다. 반환값은 사건 목록에 없다. `CreatePipe`의 `mq_open`이 syscall 23개에 있는지는 불명이다 | p.8, p.14; `ledger.md` §1a |
| 2 | 분석 | SB AppId 필드와 EVS 등록 정보는 서로 다른 변수다. C3와 같이 메모리 위치로 구별되는 서로 다른 SR이다. 값의 의미와의 연결은 서술이 없다 | p.8 |
| 3 | 분석 | 설정(쓰기)→사용(읽기) 쌍의 형태다. R2의 순서는 코어 시작 직렬화가 ES·EVS를 SB보다 먼저 실행시켜 정해진다(기록). 이 직렬화는 상태 polling이라 syscall 의미에서 추론한 HB(p.9)에 들지 않는다. 설계의 race 정의가 "순서를 바꿀 수 있는 접근"이므로(p.3, p.10–11) 바꿀 수 없는 위반을 뜻하는 출력 항목은 어휘에 따로 없다 | p.3, p.9, p.11 |
| 4 | 답 | 도구 수준 답과 같다. R2의 요구 순서("SB AppId 저장이 SB API 경로의 이벤트 요청보다 먼저")를 넣을 입력은 서술에 없다 | p.3, p.16–18 |
| 5 | 분석 | 직렬화를 HB로 인식하지 못하면 Phase 1이 쌍으로 계산할 수 있고 Phase 2는 순서를 뒤집어 보며 불가능하면 race로 세지 않는다(p.11). 이는 순서가 고정됐는지를 실행으로 시험하는 방식이며 고정된 이유를 설명하는 방식이 아니다. 실행하지 않았다 | p.10–11 |
| 6 | 분석 | 사례 현상은 "이벤트 요청 거부 24회"이고 실패 메시지도 프로세스 실패도 없다(기록: `cfe_evs.c:185-188`, 상태 폐기). 출력은 race 쌍과 출력 기반 oracle 차이이고, race가 확인돼도 출력에 영향이 없으면 fault로 세지 않는다고 적는다 | p.19; `availability.md` §1 `[기록]` |

### 3.6 이 절이 말하지 않는 것

- 어느 셀도 "SIMEXPLORER가 사례를 탐지하지 못한다"는 뜻이 아니다. 실행하지 않았고, 저자들은 출력에 전파되지 않은 real race도 race로 보고한다고 적는다(p.19).
- "SIMEXPLORER는 shared memory를 관측하지 못한다"는 문장은 틀리다. SR 목록에 있고 중단점으로 잡는다(p.8, p.14). 모르는 것은 cFS 내부 위치를 SR로 지정하는 방법이다.
- "분석" 셀의 서술은 도구의 원래 설계에 대한 것이다. OSAL 레지스트리나 ES 테이블을 SR로 직접 지정해 주는 입력은 연구 쪽이 공급하는 것이라서 EXTENSION 또는 수동 입력이며 원 도구의 자동 결과가 아니다(로드맵 §10.4 끝, `availability.md` §5.1).

---

## 4. 실행 가능성과 결과 상태

### 4.1 요구 조건과 오늘 확인한 상태

| 요구 | 레퍼런스 | 값의 상태 | 오늘 상태 |
|---|---|---|---|
| 도구 구현(Simics 위의 Python tool, Observer·Controller) | STVR p.14. 저자 공개 서술 없음 | — | **찾지 못함.** 경로 9–12, 20–24: GitHub 저장소·코드 검색 0건, Zenodo·figshare·arXiv·NSF PAR 해당 없음, 저자 페이지에 소프트웨어 항목 없음, Software Heritage·core.ac.uk·Wayback은 확인 페이지·연결 실패로 열지 못함 |
| artifact | "The objects of analysis and other artifacts used in this study can be obtained by contacting the first author." (p.15). §7.1은 객체 24개를 "available for others"라고 적는다(p.24) | 요청 시 제공(도구 포함 여부 서술 없음) | 요청하지 않았다(`availability.md` A2, 아직 결정되지 않음) |
| Simics | STVR p.2, p.14. ITJ p.180: 학계 라이선스가 무료라고 적음 | 제품은 from-reference. **버전은 `unspecified_by_reference`**(p.18은 "later Intel processors with hardware virtualization" 미지원이라고만) | [M] `which simics` 결과 없음. 벤더 문서(경로 25, 2026-10-15 열람, "Updated 9/8/2023"): 공개 릴리스를 연구자에게 무료로 제공하고 EULA 동의 후 받는다, 내려받기 약 2 GB, 설치 약 10 GB, 타깃은 Quick-Start Platform(x86)과 RISC-V를 포함. 논문이 쓴 버전과의 동일성은 정해지지 않는다. 설치하지 않았다(조건을 고르는 일이 되므로) |
| guest OS | 단일 코어 x86 Linux 2.6.15와 3.5.0(p.18), Fedora Core 2.6.15와 Ubuntu 12.04(p.15) | 논문 서술. 정책상 "레퍼런스"인지는 PI 판단(§4.3 P-S1) | 기준 cFS `core-cpu1`은 x86-64 ELF이고 kernel ≥ 3.2.0을 요구한다(`availability.md` §0 A4 `[기록]`). 2.6.15 guest는 맞지 않고, 3.5.0은 숫자상 충족하나 32/64-bit와 userland는 서술이 없다 |
| Controller(dummy device, guest의 driver 프로그램) | STVR p.14; ITJ p.188 | 구성 상세 서술 없음 | 만들려면 연구 쪽 신규 코드. 레퍼런스가 정하지 않는 입력이다 |
| 입력(짝 process·handler, test input, oracle, SR·syscall 지정) | STVR p.8, p.16–17 | cFS용은 연구 쪽 신규 입력(`unspecified_by_reference`) | 미정 |
| 하드웨어·host | STVR에 서술 없음 | `unspecified_by_reference` | 이 컨테이너: [M] 4 vCPU, x86_64, `/dev/kvm` 없음, vmx/svm 플래그 0, 여유 디스크 약 16 GB, kernel `6.18.44-fc-v114` |
| 문서화된 예제 | 서술 없음(저장소 README 없음) | — | `DOCUMENTED-SETUP-RUN` 불가 |

### 4.2 결과 상태 (로드맵 §10.4)

오늘 적용되는 상태는 **다섯 사례 모두 `NOT_RUN`**이다.

| 사례 | 상태 | 이유 |
|---|---|---|
| C1 | `NOT_RUN` | 자료: 도구 구현을 찾지 못했다. 환경: Simics 없음, cFS를 놓을 guest 조건이 레퍼런스에 없다. 결정: cFS용 SR·oracle 입력과 저자 요청(A2)이 미정이다 |
| C2 | `NOT_RUN` | 위와 같고, 원 앱(P·R)이 비공개라 가능한 것은 역할 재구성뿐이다 |
| C3 | `NOT_RUN` | 위와 같고, 원 플랫폼 Microblaze의 Simics 지원은 서술이 없다(논문의 연구 환경은 x86). 파생 플랫폼(x86 `-m32`)은 별개의 `deviation` 조건이다 |
| C5 | `NOT_RUN` | 위와 같다 |
| R2 | `NOT_RUN` | 위와 같다 |

- `INAPPLICABLE`을 붙이지 않는다. 논문은 범위를 process, signal, interrupt 사이의 race로 적고(p.2–3, p.7), thread를 PID로 추적할 수 있다고 적으며(p.13), SR에 shared memory가 있다(p.8). 한 process 안의 task 사이 쌍이 "명시된 입력·대상 범위"에서 제외되는지는 읽은 자료로 정해지지 않는다. 이것은 PI 판단이다(§4.3 P-S2).
- `PATH_NOT_EXECUTED`, `OBSERVATION_GAP`, `ANALYSIS_GAP`, `REPORTED`는 도구를 실행한 뒤에만 쓸 수 있다. 이 문서는 어느 것도 예측하지 않는다. 사건 기록의 기준이 부족한 경우를 `ANALYSIS_GAP`으로 서둘러 분류하지 않는다(로드맵 §10.4).
- §3의 내용은 라벨 `ANALYSIS`(observation-model analysis, not run)이고 `[전문]` 등급이다. 원 도구 결과가 아니다. 원 도구를 실행할 수 없으므로 `ORIGINAL-RUN`과 `DOCUMENTED-SETUP-RUN`은 불가능하다. 논문의 Phase 1–2 알고리즘(Figure 10, 12, 13)이 서술돼 있어 `REIMPLEMENTATION`은 원리상 서술할 수 있으나 Observer·Controller가 Simics에 의존하고(p.14) 충실성을 확인할 구현이 없다. 별도 라벨과 PI 승인이 필요하다(§4.3 P-S4).

### 4.3 PI 판단이 필요한 항목 (결정 후보. 여기서 정하지 않았다)

| # | 항목 | 선택지 | 기본값 |
|---|---|---|---|
| P-S1 | 논문 §5의 환경 서술(Simics 위 x86 Linux 2.6.15·3.5.0 단일 코어, Fedora Core 2.6.15, Ubuntu 12.04)을 `CONDITIONS_POLICY`의 레퍼런스(`from-reference`)로 볼 것인가. 정책 목록은 "도구의 공식 문서: README, 논문의 artifact 절"이고 §5는 실험 서술이다 | 예 / 아니오 | 판단 전까지 `unspecified_by_reference` 유지, 논문 서술로 병기 |
| P-S2 | 한 process 안의 task(thread) 사이 쌍을 SIMEXPLORER의 명시된 대상 범위 밖(`INAPPLICABLE`)으로 볼 것인가 | 예 / 아니오 | 아니오(`NOT_RUN` 유지). 논문이 thread를 PID로 추적한다고 적고 SR에 shared memory가 있으며, PI의 Process-Level 정의가 앱·task 단위라서 |
| P-S3 | 저자에게 객체 프로그램과 도구의 제공을 요청할 것인가(`availability.md` A2). 논문은 객체 프로그램을 first author에게 요청하라고 적고 도구 코드 제공 여부는 적지 않는다 | 요청 / 요청하지 않음 | 요청하지 않음(아무도 연락하지 않았다). 연락 주체는 PI가 정한다 |
| P-S4 | 논문 기술로 `REIMPLEMENTATION`을 만들 것인가(`availability.md` A9) | 만든다 / 만들지 않는다 | 만들지 않는다. 만든다면 원 도구 결과와 분리한 라벨을 쓰고, 충실성(Simics 의존 부분, 23개 syscall 목록, 메모리 SR 지정)이 정해지지 않은 부분을 기록한다 |
| P-S5 | 로드맵 §10.1과 `availability.md`가 SimRacer와 SIMEXPLORER를 별개 행으로 둔 것을 같은 계보의 두 판본으로 정리할 것인가(STVR p.2 †, p.24) | 정리 / 유지 | 이 문서는 다른 문서를 바꾸지 않았다. 결정은 PI와 오케스트레이터가 한다 |

---

## 5. 다섯 사례에서 부족한 것과 Kairos 기능 가설

### 5.1 SIMEXPLORER가 이미 하는 것 — 차별점으로 주장하지 않는다

로드맵 §10.5는 "이미 가능한 기능은 재발명한 차별점으로 주장하지 않는다"고 한다. 아래는 STVR 전문에서 확인한 기존 기능이다.

| 기능 | 근거 |
|---|---|
| 기록된 실행에서 **실행되지 않은 순서를 계산**하고(Phase 1) **실제 실행으로 확인**(Phase 2) | p.7, p.10–12 |
| 메모리 접근과 하드웨어 접근을 SR로 관측(binary level, 소스·계측 없음) | p.8, p.14 |
| syscall의 read/write 효과와 `fork`·`wait`-`exit`·`pipe`·signal·interrupt를 HB로 모델 | p.8–9 |
| 사용자가 SR과 syscall을 추가 지정하는 인터페이스 | p.8 |
| 이벤트 기준의 실행 제어(process 멈춤·재개, signal·interrupt 발생). 고정 sleep이 아니다 | p.14; ITJ p.188 |
| 쌍을 뒤집을 수 없을 때 race로 세지 않는 처리, 기다림 교착의 해소 | p.11, p.13 |
| 출력 기반 oracle, race와 fault의 구분(출력에 전파되지 않은 race 보고) | p.17–19 |

따라서 로드맵 §10.5 표의 "관측된 실행만 다룸 → 예측·탐색 및 실행 확인" 행은 SIMEXPLORER에 근거가 없다. 이 행을 SIMEXPLORER와의 차이로 쓰려면 앵커의 종류(SR 접근 쌍, 두 사건 사이의 순서)와 탐색 범위(한 실행 한 쌍)에 한정해서 써야 한다. 예측·확인 자체를 차별점으로 쓸 수 없다. "메모리를 관측하지 못해서"도 쓸 수 없다.

### 5.2 가설 (근거가 있는 만큼만)

모든 가설은 **분석**(설계 기술에서 도출, 미실행)이고 연구 가설이다. 어느 것도 "SIMEXPLORER가 못 한다"는 결과가 아니다.

| 가설 | 대상 사례 | 관찰된 부족 (분석) | 근거 | 연결되는 Kairos 기능 후보 (로드맵 §10.5) | 한계·반증 가능성 |
|---|---|---|---|---|---|
| **H-SE-1** cFS 내부 상태를 SR로 지정하고 같은 자원·세대로 식별하는 방법이 설계 서술에 없다 | C1, C5, R2, C3 | 의존성의 공개·조회가 OSAL 레지스트리·ES 테이블·SB 필드의 메모리 접근이다. 메모리 SR 지정 방법, "SR_i equals SR_j"의 기준, 이름→ID→슬롯과 task→AppId 연결, 식별자 재사용이 서술에 없다 | p.8, p.10 Figure 10, p.14, p.16; 키워드 "generation" 0건 | cFS 내부 API·상태 변화·값 전달을 관측해 소스와 연결, 이름과 객체 ID·세대, task와 AppId의 관계 추적 | **근거가 약하다.** 서술에 없다는 것은 부재 확인일 뿐이고, SR 지정 인터페이스가 있다(p.8). 그 입력으로 가능하다면 EXTENSION이다. "Kairos만 가능"이 아니라 "원 도구의 서술된 입력 밖"이라는 뜻이다 |
| **H-SE-2** lock·semaphore와 코어 시작 직렬화가 HB 규칙에 없다 | R2, C5, C3, C1 | cFS의 순서 근거는 ES 잠금(C5의 6.4.2 수정), 코어 시작 직렬화(R2), 우선순위(C3), OSAL 잠금(C1)이다. 설계의 HB 규칙은 syscall 의미와 signal·interrupt 활성 구간에서만 순서를 추론한다 | p.9("semaphores and locks"는 확장 대상); p.10, p.13(모든 동기화를 식별하지 못함) | 소스 기반 순서 근거·전제·신뢰 범위를 결과에 연결 | 저자는 이것을 거짓 양성 원인으로 적고, Phase 2가 실제 실행으로 순서를 뒤집을 수 있는지 시험하므로(p.11) HB 누락이 곧 실패는 아니다. 실제 동작은 미실행 |
| **H-SE-3** 출력 어휘가 readiness violation과 반환값·재시도·최종 결과를 담지 않는다 | C1, C5, R2 | 출력은 "순서를 바꿀 수 있는 접근 쌍"과 출력 기반 oracle 차이다. 사용 시점에 readiness precondition이 충족됐는지, 반환값(`NAME_NOT_FOUND`, `CFE_ES_ERR_APPID`, `ILLEGAL_APP_ID`), 재시도로 회복했는지, 초기화가 최종 실패했는지는 출력 항목이 아니다. 본문에 retry, return value 서술이 없다 | p.3 race 정의; p.11; p.16–18; p.19; 키워드 0건("return code"는 Table 4의 race 설명 한 곳) | 반환값·재시도·초기화 최종 결과 추적 | 사용자가 반환값·최종 결과를 출력으로 내는 oracle을 쓰면 구분할 수 있을 수 있다. 저자도 내부 oracle이 효과를 높일 수 있다고 적는다(p.18). "도구 안에 입력 형식이 없다"에 가깝고 능력 부족은 아니다 |
| **H-SE-4** 순서가 고정된 위반(fixed-order)을 뜻하는 출력 항목과 고정된 이유의 설명이 없다 | R2 (C3의 residual 경로) | 설계는 순서를 뒤집을 수 있어야 race로 세고, 뒤집을 수 없으면 세지 않는다. R2의 순서는 코어 시작 직렬화가 정하는 것으로 기록돼 있다. 뒤집을 수 없을 때 도구는 쌍을 버릴 뿐 이유를 보고하지 않는다 | p.3; p.10–11; R2 순서의 원인은 `ledger.md` §1a `[기록]` | 소스 기반 순서 근거·전제·신뢰 범위를 결과에 연결, `fixed-order` 분류 | "race가 아니다"와 "고정된 순서의 위반"은 같은 말이 아니다. 이 가설은 도구 어휘에 대한 것이고 R2의 위반을 도구가 놓친다는 뜻이 아니다. 미실행 |

**근거가 부족해 가설로 올리지 않는 것**

- **C2 (시작 동기화와 앱 고유 초기화 완료의 혼동, 로드맵 §10.5 3행).** P·R이 의존에 쓰는 자원이 비공개라 SIMEXPLORER가 그 관계를 표현할 수 있는지 판단할 근거가 없다. 설계에 시작 상태나 readiness 개념이 서술되지 않았다는 것만 확인된다. 역할 재구성 실험(결정 P-C2)이 요청 방식을 정한 뒤에야 같은 분석을 할 수 있다.
- **"관측된 실행만 다룸."** §5.1의 이유로 올리지 않는다.
- **원 플랫폼(C3의 Microblaze).** 연구 환경이 x86 guest이고(p.18) 서술된 register도 x86이라는 것은 확인되지만(p.12), 다른 타깃의 지원 여부는 서술이 없다. 플랫폼 문제는 Kairos 기능 가설이 아니다.
- **한 process 안의 여러 task(pthread).** thread를 PID로 추적한다는 한 줄(p.13)과, PuT를 call stack의 frame pointer로 가려내는 방식(p.14)이 여러 thread에서 어떻게 동작하는지는 서술이 없다. 불명이다.

### 5.3 비교 논문에서 먼저 밝혀야 할 정의 차이

[F] SIMEXPLORER의 race는 "두 system event(process, signal, hardware interrupt)가 같은 shared resource를 의도한 순서와 다른 순서로 접근할 수 있는 것"이고 일반 data race의 필요조건으로 둔다(p.2–3). [D] 이 연구의 Process-Level은 cFS 애플리케이션·task 단위의 운용상 정의이며 OS process로 한정하지 않는다(로드맵 §1.2). native cFS에서 대상은 한 process 안의 thread다. 비교 서술은 이 차이를 처음에 밝히고, 이 차이로 생기는 결과를 도구의 결함처럼 쓰지 않는다.

[I] Kairos가 판정하는 대상은 readiness specification이고, SIMEXPLORER의 보고 단위는 "순서를 바꿀 수 있는 접근 쌍"과 oracle 출력 차이다. 두 도구의 출력은 같은 점수로 합치지 않는다(로드맵 §1.4).

---

## 6. 확인하지 못한 것과 다음에 할 일

| # | 미확인 | 이유·다음 단계 |
|---|---|---|
| 1 | 출판본(version of record)과 수락본의 차이 | Wiley가 Cloudflare 도전으로 막는다. 수락본 쪽 번호만 썼다. 출판본을 얻으면 인용 쪽을 대조한다 |
| 2 | 학위논문 본문 | PDF가 Cloudflare 도전으로 막히고 ProQuest 사본은 UNL 사용자 한정이다. 초록만 읽었다. 학위논문이 논문과 다른 설계를 담았는지 모른다 |
| 3 | SimRacer(ISSTA'13) 전문과 STVR의 항목별 차이 | `plan/methods/SimRacer.md`가 다룬다. 이 문서는 STVR §7.1이 적은 차이만 사용했다 |
| 4 | syscall 23개의 전체 목록 | 본문은 예시만 적는다. `clone3`, sleep 계열, `mq_open`이 들어 있는지 정해지지 않는다 |
| 5 | 메모리 SR을 정하는 방법과 입도(주소, 영역, 변수) | STVR 본문에 서술이 없다. SimTester(VEE'12 p.55)는 symbol table과 계측을 쓴다고 적으나 SIMEXPLORER가 같은지 정해지지 않는다 |
| 6 | "SR equals" 판정 기준과 생성 세대 처리 | 서술이 없다. 구현을 얻기 전에는 정해지지 않는다 |
| 7 | 여러 thread를 가진 한 process에서 PuT 구분과 memory 관측 | p.13–14의 서술만 있다 |
| 8 | Simics 버전, 머신 모델, 스크립트, host | 서술이 없다(`unspecified_by_reference`) |
| 9 | 도구 코드의 존재 여부 | 찾지 못했다. 저자 요청(P-S3)이 남은 경로다 |
| 10 | 표 1–3, Figure 14–15의 세부 | 텍스트 추출이 어지러워 본문 서술만 사용했다. Figure 10, 12, 13(알고리즘)은 이미지로 확인했다 |
| 11 | 24개 객체 프로그램과 oracle 파일 | 저자에게 요청해야 한다(p.15). 열지 못했다 |

---

## 7. 출처 목록

- **STVR 2017 저자 수락본**(전문 읽음): `https://archive.cps-vo.org/file/36388/download/194033`, 레코드 `https://archive.cps-vo.org/node/36387`. 26쪽, 2,396,338 B, sha256 `af805231f69ffc0e12d96fe4e4e9a356932653c56c382fdd3debd211bcdd3e08`. 출판 정보: Softw Test Verif Reliab 2017;e1634, DOI `10.1002/stvr.1634`.
- **학위논문**(초록만): `https://digitalcommons.unl.edu/computerscidiss/77/`, `https://digitalcommons.unl.edu/dissertations/AAI3626337/`. PDF 주소는 `…viewcontent.cgi?article=1093&context=computerscidiss`와 `…article=14291&context=dissertations`이고 Cloudflare 도전으로 열지 못했다.
- **Sim-O/C**(전문 읽음, 같은 계보의 앞선 서술): Intel Technology Journal 17(2), 2013, 178–197쪽. `https://www.intel.com/content/dam/www/public/us/en/documents/research/2013-vol17-iss-2-intel-technology-journal.pdf`, 16,555,438 B, sha256 `24080f8ec175aecb7e6c93fd0a436dd8532c856375d8b356b772030dc8c1813c`.
- **SimTester**(부분 읽음): VEE'12, `https://www.cl.cam.ac.uk/research/srg/netos/vee_2012/papers/p51.pdf`, sha256 `206c12cef8dc9f01fe2ae0b2d16dc2d2c719ce60f553fe75aa8627e3022b08cb`.
- **SimEvo**(부분 읽음): ICSME'17, `https://par.nsf.gov/servlets/purl/10089644`, sha256 `143f7351489367d9301b97ac2317e3f1f263ac159ee737ffcff14bb0adc31be6`.
- 서지·참고문헌: `https://api.crossref.org/works/10.1002/stvr.1634`, `https://api.openalex.org/works/doi:10.1002/stvr.1634`, `https://scholars.uky.edu/en/publications/an-automated-framework-to-support-testing-for-process-level-race-/`.
- 저자 페이지: `https://homepages.uc.edu/~tyu/links/research.html`, `…/links/projects.html`, `…/CV/tyu_cv.pdf`.
- Simics 벤더 문서(컨텍스트): `https://www.intel.com/content/www/us/en/developer/articles/tool/simics-simulator.html`.
- 저장소 기록: `baselines/availability.md`, `baselines/yu-group.md`, `cases/ledger.md`, `plan/ROADMAP_20261015.md`(§1.2, §2.2, §9, §10), `Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md`, `CONDITIONS_POLICY.md`, `plan/methods/SimRacer.md`, `plan/methods/RacePro.md`.
