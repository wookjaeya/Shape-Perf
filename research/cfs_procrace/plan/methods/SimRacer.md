# SimRacer (ISSTA 2013) 분석 — Kairos 비교용

- 작성일: 2026-10-15 (UTC 08:28–08:44, 경로 시도와 읽기 시각).
- 보완: 2026-10-16 (UTC 00:09–00:30). (1) 1차 출처 경로를 다시 시도했다(§1.1 경로 26–38). ISSTA'13 전문은 여전히 열리지 않았고, STVR의 둘째 취득 경로도 찾지 못했다. (2) 같은 저자의 2013년 동시대 서술 Sim-O/C(Intel Technology Journal 17(2))와 그 앞선 SimTester(VEE'12)를 `[2차]` 출처로 추가했다. Sim-O/C는 여러 user process 사이의 시험을 "system call" 쌍의 순서 바꾸기로 서술한다(ITJ p.188). (3) 그 결과 이 문서 스스로 [U]로 둔 "SimRacer 자체의 메모리 SR 관측"과 Q1·Q2 라벨이 어긋나 있던 것을 고쳤다. C1·C3·C5·R2의 Q1·Q2를 "분석"에서 "불명"으로 바꿨다(§3.4, 변경 이유는 §1.3 항목 8–10). Q3·Q5·Q6은 조건부 "분석"으로 남겼다. (4) R2 사건 대응표에 core task 생성 행(⓪)을 넣었다. (5) 가설 H-SM-1과 §5.1 표를 SimRacer 자체에 맞게 고쳤다. (6) 환경을 다시 측정했다(§4.1). 이번에도 SimRacer를 실행하지 않았다.
- 대상: T. Yu, W. Srisa-an, G. Rothermel, *SimRacer: an automated framework to support testing for process-level races*, ISSTA 2013, pp. 167–177. DOI 10.1145/2483760.2483771. ISSTA 프로그램 쪽은 제목을 "SimRacer: A Scalable and Automated Framework to Support Testing for Process-Level Races"로 적는다(§1.1 경로 7).
- 범위: 로드맵 §10 (M6)의 SimRacer 항목. 여섯 분석 질문(§10.2)을 사례 C1, C2, C3, C5, R2에 적용한다. 로드맵 §12 작업 6(`process_race_method_matrix.md`)의 입력으로 쓴다.
- 이 문서의 성격: **분석 문서**다. SimRacer를 실행하지 않았다(실행할 구현이 없다). 어느 사례에 대해서도 "탐지하지 못한다"는 결과를 담지 않는다.
- 용어: 신규 문서이므로 T1 (a)를 따른다. 판정 대상은 readiness specification(준비 상태 명세), 그 전제는 readiness precondition, 위반은 readiness violation이다. "API contract"는 문서화된 API 의미를 가리킬 때만 쓴다.
- 조건 원칙: `CONDITIONS_POLICY.md`. 이 문서는 실험 조건을 정하지 않는다. 레퍼런스가 없는 값은 `unspecified_by_reference`로 둔다.
- PI 결정(`PI_DECISIONS_RESOLVED_20261015.md`)은 결정으로만 인용한다. 사실로 쓰지 않는다. 이 문서가 쓰는 결정은 P-PAPERS와 T1이다.
- 같은 계보의 다른 문서: `plan/methods/RacePro.md`(같은 형식). SIMEXPLORER는 이 문서의 §0 항목 2와 §2.3에서 보듯 SimRacer의 확장판이다.

## 증거 태그와 근거 등급

| 태그 | 뜻 | 이 문서에서의 쓰임 |
|---|---|---|
| [F] | 인용이 있는 사실 | 원출처(논문 쪽·절) 또는 저장소 기록(문서·절). 저장소 기록은 "기록"이라고 표시하며 이번에 재검증하지 않았다 |
| [M] | 측정 | 오늘(2026-10-15) 이 컨테이너에서 읽기 전용으로 확인한 값, 또는 "기록"이 붙은 이전 측정 |
| [I] | 추론·분석 | 설계 기술에서 도출한 해석. 도구를 실행하지 않았다 |
| [D] | 결정 | PI 결정 |
| [U] | 미확인 | 자료가 없거나 읽지 못함 |

문장마다 출처 등급을 아래 다섯 가지 중 하나로 붙인다.

| 등급 | 뜻 |
|---|---|
| `[전문]` | 본문 전체를 텍스트로 추출해 읽었다. 그림은 이미지로도 확인했다 |
| `[초록]` | 초록·서지만 읽었다 |
| `[2차]` | 다른 논문·페이지가 이 도구를 서술한 문장을 읽었다. 도구 자신의 서술이 아니다 |
| `[기록]` | 저장소의 이전 보고서에 있는 값. 이번에 재검증하지 않았다 |
| `[미도달]` | 열지 못했다 |

쪽 번호 표기:
- **STVR p.N**: Yu, Srisa-an, Rothermel, STVR 27(4–5), e1634(2017)의 논문 쪽 "N of 26". PDF 쪽과 같다.
- **SimEvo p.N, DESCRY p.N, RRF p.N**: 각 PDF 파일의 쪽(SimEvo 12쪽, DESCRY 11쪽, RRF 11쪽). 인쇄 쪽과 다를 수 있다.
- **ITJ p.N** (2026-10-16 추가): Intel Technology Journal 17(2), 2013의 인쇄 쪽. Sim-O/C 논문은 178–197쪽이다. 쪽 번호는 각 쪽 꼬리말에서 읽었다. `plan/methods/SIMEXPLORER.md`의 표기와 같다.
- **VEE'12 p.N** (2026-10-16 추가): SimTester 논문의 인쇄 쪽(51–61). PDF 쪽 + 50이다. `SIMEXPLORER.md`의 표기와 같다.

---

## 0. 한눈에 보기

1. **SimRacer 논문(ISSTA'13) 전문은 오늘도 열리지 않았다.** 읽은 것은 초록과 서지뿐이다 `[초록]`. 저자 공개본, 기관 저장소, arXiv, NSF PAR, Internet Archive에는 사본이 없었고, ACM·ResearchGate·Wiley는 Cloudflare가 403으로 막았다. 학위논문(Yu 2014, SimExplorer) PDF도 Cloudflare 도전 페이지에서 멈췄다(§1.1). 도전 페이지를 우회하려고 하지 않았다. `[미도달]` 2026-10-16에 14개 경로를 더 시도했으나 결과는 같다(§1.1 경로 26–39).
2. **같은 저자들이 SimRacer의 "revised and expanded version"이라고 밝힌 STVR 2017 논문(SIMEXPLORER, 26쪽)의 전문을 읽었다** `[전문]`. CPS-VO 아카이브가 올린 파일이다(페이지에 Creative Commons 2.5 표기). 이 문서의 도구 설명(관측 사건, happens-before 규칙, Phase 1·2 알고리즘, 구현, 한계)은 대부분 이 논문에 근거한다. 이 서술이 ISSTA'13의 SimRacer에 그대로 해당하는지는 §2.3에서 항목별로 구분했고, 자료로 정해지지 않는 항목은 "불명"으로 두었다.
3. **이전 기록의 조건부 서술 하나가 STVR 전문으로 더는 유지되지 않는다.** `baselines/yu-group.md` §4·§7.2와 `baselines/availability.md` §1·§3은 "추론한 system call 모델 아래에서" C1·C3·R2의 의존성이 "shared-resource 사건이 아니다"라고 적었다(SimRacer 자체 모델은 미열람이라는 단서가 붙어 있었다). STVR 전문은 shared memory와 hardware components를 shared resource(SR)로 열거하고(p.8) 전역 변수의 load/store 사건을 예로 든다(p.12). 그 모델은 system call만 보는 모델보다 넓다. 그래서 이 문서는 "syscall 모델이라 사건이 아니다"라는 서술을 쓰지 않는다. 다만 SimRacer 자체가 메모리를 관측했는지는 자료로 정해지지 않는다(§2.3).
4. **역할은 탐지(testing-based detection)다.** Phase 1이 한 번의 실행 trace에서 잠재 race 쌍을 계산하고, Phase 2가 쌍의 순서를 실제 실행에서 뒤집어 real race를 확인한다. 재현 도구(RRF, DESCRY, SysPro)도 회귀 도구(SimEvo)도 아니다.
5. **필요한 순서와 허용 오류는 입력이 아니다.** 논문은 race를 "의도한 순서와 다른 순서로 접근할 수 있는 것"으로 정의하지만(p.3), 의도한 순서를 받는 인터페이스는 서술에 없다. 순서는 syscall 의미와 signal·interrupt 활성 상태에서 추론한 happens-before로만 들어간다(p.9). 실패 판정은 출력 기반 oracle이다(p.17–18).
6. **다섯 사례의 의존성 쌍은, SR로 관측된다면 이 설계의 후보 쌍 모양(같은 SR, 한쪽 이상 쓰기, happens-before 없음)과 맞는다 (분석, 미실행).** C1·C3·C5·R2의 공개와 사용은 사용자 공간 메모리의 쓰기와 읽기다. STVR의 SIMEXPLORER는 메모리 접근을 Simics 중단점으로 잡고 SR 지정 인터페이스가 있다고 하지만, cFS 내부 상태가 SR이 되는 경로와 입도는 서술이 없다. 이것은 "탐지 실패"가 아니라 입력 정의가 정해지지 않은 상태다. 한 process 안의 task 쌍을 도구의 입력 단위로 줄 수 있는지도 서술이 없다.
   **2026-10-16 보정.** 위 메모리 관측은 SIMEXPLORER의 서술이다. SimRacer 자체가 process 사이의 메모리 접근을 SR로 관측했는지는 읽은 자료로 정해지지 않는다. 같은 저자의 2013년 서술(Sim-O/C, ITJ p.188 `[2차]`)은 여러 user process 사이의 시험을 "identifies system calls that can potentially race with each other"로 적고, 메모리 중단점은 응용과 ISR 사이의 예(ITJ p.185–187, VEE'12 p.54 `[2차]`)에서만 보인다. 그래서 SimRacer 행의 C1·C3·C5·R2 Q1(관측 사건)과 Q2(자원 식별)를 "불명"으로 바꿨다. "SimRacer는 메모리를 관측하지 못한다"고도 쓰지 않는다. Sim-O/C는 SimRacer 논문이 아니고, 메모리 관측을 배제한다는 문장도 없다.
7. **실행은 불가능하고 오늘 적용되는 상태는 다섯 사례 모두 `NOT_RUN`이다.** 구현을 찾지 못했고, Simics는 이 컨테이너에 없으며, 원 구성(Simics 버전, 시뮬레이션 머신, guest 이미지, 제어용 driver)이 읽은 자료에 없다. `INAPPLICABLE`은 PI 판단이다(§4.3).
8. **Kairos 가설은 근거가 있는 만큼만 올렸다(§5).** 예측·탐색과 실행 확인은 SimRacer 계열에 이미 있으므로 차별점으로 쓰지 않는다. 메모리 관측 부재도 가설로 쓰지 않는다. 확장판(SIMEXPLORER)에는 근거가 반대이고, SimRacer 자체에는 있었는지 없었는지 정해지지 않기 때문이다(항목 6 보정).

---

## 1. 접근한 출처와 문장별 근거 등급

### 1.1 시도한 경로와 결과

시각은 2026-10-15 UTC 08:28–08:44다. 접속 실패는 원인을 기록한 것까지만 썼고 TLS 검증을 끄거나 프록시를 우회하지 않았다.

| # | 경로 | 결과 | 등급·비고 |
|---|---|---|---|
| 1 | `https://dl.acm.org/doi/10.1145/2483760.2483771` | HTTP 403. 응답 헤더 `cf-mitigated: challenge`(Cloudflare 도전) | `[미도달]` ISSTA'13 전문 |
| 2 | `https://doi.org/10.1145/2483760.2483771` | ACM으로 이동 후 같은 403 | `[미도달]` |
| 3 | ResearchGate 레코드 `publication/262165762` | HTTP 403(Cloudflare). WebSearch 요약은 "no full-text"라고 적었다(열지 못한 페이지의 요약) | `[미도달]` |
| 4 | OpenAlex `api.openalex.org/works/doi:10.1145/2483760.2483771` | 200. 서지와 초록. `is_oa: false`, `oa_status: closed`, 사본 위치 없음 | `[초록]` ISSTA'13 |
| 5 | Semantic Scholar Graph API (같은 DOI) | 200. 서지만. `openAccessPdf.status: CLOSED`, 초록은 출판사 요청으로 생략됨 | 서지 |
| 6 | Crossref `api.crossref.org/works/…` | 200. 쪽 167–177, 참고문헌 47건, 초록 없음. 전문 링크는 `dl.acm.org`뿐 | 서지 |
| 7 | ISSTA 2013 프로그램 `issta2013.inf.usi.ch/program/` | 200. 제목 "SimRacer: A Scalable and Automated Framework to Support Testing for Process-Level Races"가 있다. 초록·PDF 링크는 없다. 프로그램 PDF는 열지 않았다 | 서지 |
| 8 | 저자 홈 `homepages.uc.edu/~tyu/`, `links/research.html`, `links/projects.html`, `CV/tyu_cv.pdf` | 200. 논문 목록에서 ISSTA 2013은 `dl.acm.org/citation.cfm?id=2483771` 링크뿐이다. 소프트웨어·도구 공개 항목이 없다. 홈에 "I have moved to University of Connecticut"가 있다 | 서지 |
| 9 | 새 홈 `tingting-yu.scholar.uconn.edu`와 `/publications/`, `/research/` | 200. 받은 HTML에 키워드 SimRacer·SimExplorer·process-level이 없다. `/publications/`의 HTML은 쿠키 안내 등 약 4.8 KB이고 논문 목록이 들어 있지 않다(목록을 동적으로 불러오는 것으로 보이나 확인하지 않았다) | 없음 |
| 10 | 학위논문(Yu 2014, *SimExplorer: A Testing Framework to Detect Elusive Software Faults*) `digitalcommons.unl.edu/computerscidiss/77`와 ProQuest 사본 `…/dissertations/AAI3626337/` | 두 페이지 모두 200. 초록만 있다. PDF(`viewcontent.cgi?article=1093&context=computerscidiss`, `…article=14291&context=dissertations`)는 403, `cf-mitigated: challenge`. WebFetch로도 403 | 초록 `[초록]`. 본문 `[미도달]` |
| 11 | `cse.unl.edu`(Rothermel, Srisa-an 홈) | `https://cse.unl.edu/~grother/`, `~witty/`는 `SSL certificate problem: unable to get local issuer certificate`로 실패. TLS 검증을 끄지 않았다. `http://cse.unl.edu/~grother`는 404, 추측한 경로 `…/papers/issta13.pdf`도 404(내용을 가져오지 못했다) | `[미도달]` |
| 12 | `web.archive.org`(Wayback) | 연결이 중간에 끊김(`Connection reset by peer`, 프록시 상태에 `ws_closed_mid_exchange` 기록) | `[미도달]` |
| 13 | arXiv API `all:SimRacer`, NSF PAR 검색 `SimRacer`, Internet Archive 전문 검색 | 각각 0건, 0건, `numFound: 0`. `scholar.archive.org`는 "Rate limit reached" 페이지(재시도하지 않았다) | 사본 없음 |
| 14 | GitHub: 저장소 검색 `SimRacer`(저장소 이름·설명·README·topic 대상) | 34건을 모두 확인했다. 전부 sim racing 관련 프로젝트(레이싱 게임 설정, 오버레이, 장치 펌웨어 등)이고 race 테스트 도구는 없다. 이름이 다른 저장소는 이 검색으로 찾지 못한다 | 구현 없음 |
| 15 | GitHub: 코드 검색 `SimRacer Simics process-level race` | 1건. BibTeX 색인 HTML이 논문 제목을 나열한 것뿐이다 | 구현 없음 |
| 16 | Scholar Gateway `semanticSearch` | `ACCESS_DENIED: Access denied`(2026-10-08 기록과 같다) | `[미도달]` |
| 17 | STVR 2017(SIMEXPLORER). Wiley `onlinelibrary.wiley.com/doi/abs/…/stvr.1634`, `…/am-pdf/…` | 403(Cloudflare 도전) | `[미도달]` |
| 18 | 같은 논문의 CPS-VO 사본: `archive.cps-vo.org/node/36387`(레코드), `/node/36388`(파일 `Yu_SimExplorer_JA.pdf`, 페이지에 "License: Creative Commons 2.5") | 두 페이지 200. 파일 URL을 `curl`로 받으면 호스트 WAF가 245 B "Request Rejected"를 돌려줬다(쿠키·Referer를 붙여도, 리다이렉트 대상 `bitcache/…` 직접 요청도 같다). 같은 URL을 WebFetch 도구로 열었더니 PDF 2.29 MB가 내려왔다. 26쪽 | **`[전문]`** sha256 `af805231f69ffc0e12d96fe4e4e9a356932653c56c382fdd3debd211bcdd3e08` |
| 19 | SimEvo(ICSME'17) `par.nsf.gov/servlets/purl/10089644` | 200, PDF 12쪽 | `[2차]`. sha256 `143f7351489367d9301b97ac2317e3f1f263ac159ee737ffcff14bb0adc31be6` |
| 20 | RRF(ISSRE'16) CPS-VO `node/30361`의 파일(`/file/30362/download/194493`) | `curl`은 경로 18과 같은 WAF 거부, WebFetch는 PDF 563 KB를 받았다. 11쪽 | `[2차]`. sha256 `e8c26e2fbca15e87c9eb837c1bddc0ade0493fb261f3ebd2fbbc200c06f00dce` |
| 21 | DESCRY(FSE'17) `chaowang-vt.github.io/pubDOC/YuZW17.pdf` | 200, 11쪽. sha256 `f9d7b57b…dfbf60`으로 2026-10-08 기록과 같다. 이번에 SimRacer 언급 부분을 다시 읽었다 | `[2차]` |
| 22 | SysPro `arxiv.org/abs/2601.09616`, `/pdf/2601.09616v1` | 200, 47쪽. sha256 `8fe5daa4c7c3794715632e4379d9160a9852b723dc4a649317ef6bae0a75e726` | `[2차]` |
| 23 | SCMiner preprint: `git clone --depth 1 github.com/tarannumzaman/Scminer` @`255933c6` | 성공. preprint PDF sha256 `854f86c1…172099`로 2026-10-08 기록과 같다. 저장소의 zip 파일은 열지 않았다 | `[2차]` |
| 24 | Intel Simics 공개 릴리스 페이지 `intel.com/content/www/us/en/developer/articles/tool/simics-simulator.html` | 200. 오늘의 벤더 문서다(§4.1) | 컨텍스트 |
| 25 | WebSearch (extended 1회, standard 4회) | 위 호스트들의 목록. 새 1차 출처는 CPS-VO 두 파일뿐이다 | — |

**2026-10-16 재시도 (UTC 00:09–00:25).** 같은 방식으로 기록한다. 우회는 하지 않았다.

| # | 경로 | 결과 | 등급·비고 |
|---|---|---|---|
| 26 | `dl.acm.org/doi/pdf/10.1145/2483760.2483771` | HTTP 403, Cloudflare 도전 | `[미도달]` ISSTA'13 전문 |
| 27 | Unpaywall API (ISSTA'13 DOI, STVR DOI) | 200. 두 DOI 모두 `is_oa: false`, `oa_status: closed`, 사본 위치 없음 | 사본 없음 |
| 28 | OpenAlex (STVR DOI) | 200. `is_oa: false`, `any_repository_has_fulltext: false`, 위치는 `doi.org`뿐 | STVR 둘째 경로 없음 |
| 29 | Semantic Scholar Graph API (STVR DOI) | 200. `openAccessPdf`가 `doi.org/10.1002/stvr.1634`를 가리키고 "author manuscript" 라이선스를 적는다. 그 주소는 Wiley로 간다. 같은 API의 ISSTA'13 조회는 429(요청 한도) | 서지 |
| 30 | Crossref (STVR DOI) | 200. 전문 링크는 Wiley(`pdf`, `full-xml`, `am-pdf`, TDM API)뿐이다. 참고문헌 85건 중 SimRacer 항목은 DOI 10.1145/2483760.2483771로 연결된다 | 서지 |
| 31 | Wiley `onlinelibrary.wiley.com/doi/am-pdf/10.1002/stvr.1634`, Wiley TDM API | am-pdf는 403(`cf-mitigated: challenge`). TDM API는 400(토큰 없음). 토큰을 만들지 않았다 | `[미도달]` STVR 둘째 경로 |
| 32 | 학위논문 PDF `digitalcommons.unl.edu/cgi/viewcontent.cgi?article=1093&context=computerscidiss` (curl, WebFetch), digitalcommons 검색 `?q=SimRacer` | 모두 403(Cloudflare) | `[미도달]` |
| 33 | `cse.unl.edu`: `https://…/~witty/CV/witty_CV.pdf`(WebSearch 결과) | https는 인증서 체인 검증 실패(어제와 같다). TLS 검증을 끄지 않았다. `http://…/~witty/CV/witty_CV.pdf`는 404, `http://…/~witty/`는 `witawassrisaan.wordpress.com`으로 넘긴다. 그 블로그 첫 페이지(200)에 SimRacer·ISSTA 2013 문자열이 없다 | `[미도달]` |
| 34 | Wayback 가용성 API(`archive.org/wayback/available`) | `cse.unl.edu/~tyu/papers/issta13.pdf`, `~grother/papers/issta13.pdf`는 스냅샷 없음. `cse.unl.edu/~tyu/`는 2016-02-04 스냅샷이 있다. 그 스냅샷은 curl로 연결이 끊기거나 403, WebFetch는 "unable to fetch from web.archive.org"였다 | `[미도달]`. 스냅샷 안에 PDF 링크가 있는지 모른다 |
| 35 | dblp 검색 API, CORE API, `scholar.archive.org`, NSF PAR 검색 `process-level race` | dblp는 봇 확인 페이지, CORE는 429와 500, scholar.archive.org는 302(따라가지 않음). PAR 결과 HTML에 SimRacer·Yu 항목 없음 | 사본 없음 |
| 36 | NCSU(Rothermel 현 소속) `people.engr.ncsu.edu/gerother/`, 도서관 저장소 저자 검색 | 앞은 프록시 CONNECT 502, 뒤는 200이나 SimRacer·ISSTA 2013 문자열 없음 | 사본 없음 |
| 37 | GitHub 저장소 검색 `simics race`(32건), 코드 검색 `"RaceController" simics`(0건) | 저장소 32건은 목록·무관 프로젝트다. Sim-O/C의 제어 모듈 이름으로 찾은 코드는 없다 | 구현 없음 |
| 38 | Intel ITJ 17(2) PDF `intel.com/content/dam/www/public/us/en/documents/research/2013-vol17-iss-2-intel-technology-journal.pdf`, SimTester `cl.cam.ac.uk/research/srg/netos/vee_2012/papers/p51.pdf` | 둘 다 200. ITJ는 16,555,438 B, sha256 `24080f8ec175aecb7e6c93fd0a436dd8532c856375d8b356b772030dc8c1813c`. SimTester는 1,145,000 B, sha256 `206c12cef8dc9f01fe2ae0b2d16dc2d2c719ce60f553fe75aa8627e3022b08cb`. 두 해시는 `SIMEXPLORER.md` 경로 16·17의 값과 같다. Sim-O/C 본문(178–197쪽)과 SimTester의 관측기 부분을 읽었다 | `[2차]` (§1.2) |
| 39 | WebSearch (extended 1회) | 새로 나온 것은 위 Srisa-an CV 주소뿐이다. 요약문은 ResearchGate가 "No full-text available"이라고 적는다고 했다(열지 못한 페이지의 요약) | — |

STVR 사본(CPS-VO)은 이 작업 공간에서 두 번 받은 파일의 sha256이 같다(어제 WebFetch로 받은 것과 `SIMEXPLORER.md` 경로 14의 클라이언트 문자열 변경 요청). 그러나 같은 호스트의 같은 파일이므로 **독립된 둘째 취득 경로가 아니다.** 쪽 번호 인용의 "단일 취득 경로, 독립 재확인 안 됨" 표시는 그대로다(`process_race_method_matrix.md` A10).

2026-10-08 기록(`baselines/availability.md` A1, `baselines/yu-group.md` D1)은 이 호스트들을 세션의 egress 정책이 막았다고 적었다. 오늘 ACM·Wiley·digitalcommons의 거부는 원 서버의 Cloudflare 도전(`cf-mitigated: challenge`)이다. 같은 호스트의 거부 원인이 날짜에 따라 다르므로 두 기록을 합쳐 읽지 않는다.

### 1.2 이 문서가 쓰는 출처별 등급

| 출처 | 등급 | 이 문서에서의 쓰임 |
|---|---|---|
| SimRacer(ISSTA'13) | **`[초록]`** (OpenAlex가 옮긴 초록, ISSTA 프로그램 제목, Crossref·Semantic Scholar 서지). 전문 `[미도달]` | 역할, "무엇을 계산하고 제어하는가" 한 단락, 플랫폼이 "commercial virtual platform"이라는 서술 |
| STVR 2017 "An automated framework to support testing for process-level race conditions"(SIMEXPLORER) | **`[전문]`**. 본문과 Figure 10·12·13을 읽었다. 표 1–3은 텍스트 추출이 어지러워 본문 서술만 썼다 | 도구 설계 전반. 저자가 SimRacer의 revised and expanded version이라고 밝힌다(p.2 각주 †, §7.1 p.24) |
| SimEvo(ICSME'17, Yu 단독) | `[2차]` | SimRacer의 제약 모델 서술(p.3), "SimRacer 위에 구현"(p.7), SimRacer 비교 서술(p.8–9) |
| DESCRY(FSE'17) | `[2차]` | SimRacer의 입력 요구와 입력 생성 능력(p.8), SimRacer와 RacePro 비교 인용 |
| RRF(ISSRE'16) | `[2차]` | SimRacer를 active testing 계열로 서술한 두 곳 |
| SysPro(arXiv 2601.09616v1) | `[2차]` | SimRacer 인용 두 곳. 설명 문장 한 줄 |
| SCMiner preprint p.11 | `[2차]` | "interleavings of system calls" 한 문장 |
| Sim-O/C, Intel Technology Journal 17(2), 2013 (같은 세 저자, 2026-10-16 추가) | `[2차]`. 본문 178–197쪽을 읽었다. SimRacer라는 이름은 본문에 없다(검색 0건). SimRacer와 같은 해, 같은 Simics 기반 틀의 서술이다. 같은 도구의 서술이라는 판단은 [I]다 | 여러 user process 사이의 시험 방식(p.188), 관측기가 process와 thread를 구별하는 방법(p.185), 다루는 fault 3종(p.183) |
| SimTester, VEE'12 (같은 세 저자, 2026-10-16 추가) | `[2차]`(부분). 관측기 부분만 읽었다 | STVR p.8이 메모리 관측의 선행 연구로 [16](SimRacer)과 함께 인용하는 [33]이다. 메모리 중단점은 응용과 ISR 사이의 공유 변수에 건다(p.54) |
| Yu 2014 학위논문 | 초록 `[초록]`. 본문 `[미도달]` | 범위 확인뿐 |
| 사례 쪽 사실 | `[기록]` | `cases/ledger.md` §1a, §1b, §2; `baselines/availability.md` §4.2 |
| Intel Simics 공개 페이지 | 벤더 문서(오늘) | 가용성 컨텍스트(§4.1). SimRacer가 쓴 제품·버전과의 동일성은 정해지지 않는다 |

### 1.3 이전 기록과 달라지거나 확정된 것

| # | 항목 | 내용 |
|---|---|---|
| 1 | `availability.md` A1, `yu-group.md` D1 | 1차 출처 접근이 부분적으로 해소되었다. SIMEXPLORER 전문, SimEvo, RRF 전문, 학위논문 초록이 열린다(P-PAPERS [D]). SimRacer 전문과 학위논문 PDF는 여전히 열리지 않는다 |
| 2 | `yu-group.md` §4 "SimRacer 모델: inferred" | 이벤트 모델에 대한 근거가 `[전문]`(SIMEXPLORER 설계, 그리고 SimRacer와의 차이를 적은 STVR §7.1)과 `[2차]`(SimEvo)로 올라갔다. SimRacer 자체의 전문 근거는 여전히 없다 |
| 3 | `yu-group.md` §7.2, `availability.md` §3의 "추론한 syscall 모델 아래에서 shared-resource 사건이 아니다" | 조건부 서술이었고, STVR 전문의 SR 목록은 그 조건(syscall만)보다 넓다(§0 항목 3). 이 문서에서는 쓰지 않는다. 두 문서의 해당 칸 갱신은 이 작업의 범위 밖이다(§6) |
| 4 | `yu-group.md` R2의 SimRacer 플랫폼 "inference from a co-citation" | SimEvo p.7이 "SimRacer, a process-level testing tool built on top of the Simics Virtual Platform"이라고 적는다. 같은 저자(Yu)의 서술이므로 `[2차]` 등급이다. ISSTA 초록의 "commercial virtual platform"(`[초록]`)과 일치한다 |
| 5 | `yu-group.md` R3 "Simics version, simulated machine, guest OS/kernel: not stated" | 일부 해소되었다. STVR p.18은 단일 코어 x86 Linux 2.6.15와 3.5.0 guest를 적는다. Simics 버전, 머신 모델, 스크립트는 여전히 없다 |
| 6 | SimRacer와 SIMEXPLORER를 별개 도구로 다룬 표 | 저자가 같은 계보로 밝힌다(STVR p.2 각주 †). 로드맵 §10.1과 `availability.md` 표에서 두 행은 같은 계보의 두 판본이다 |
| 7 | 제목 표기 | 출판본 PDF 제목은 "An automated framework to support testing for process-level race conditions"이고 저자 CV는 "SIMEXPLORER: An Automated Framework to Support Testing for System-Level Race Conditions"로 적는다. 같은 논문(DOI 10.1002/stvr.1634)에 대한 표기 차이다 |
| 8 | (2026-10-16) 이 문서 2026-10-15판의 Q1·Q2 라벨 | 2026-10-15판은 §2.3에서 "메모리 관측이 SimRacer에 있었는지 [U]"라고 적으면서, C1·C3·C5·R2의 Q1(관측 사건)을 "분석"(SR로 지정되면 메모리 중단점으로 사건이 된다)으로 두었다. 그 "분석"의 근거는 SIMEXPLORER의 설계였다. SimRacer 행의 Q1에는 SimRacer 자체의 서술이 필요하므로 "불명"으로 바꿨다 |
| 9 | (2026-10-16) Q2 라벨과 `process_race_method_matrix.md` §2.5 항목 1 | 그 문서가 지적한 대로 C1·C5의 Q2가 SIMEXPLORER.md(불명)와 달랐다. 2026-10-15판은 "같은 SR 판정 기준 = 주소"라는 가정 [I]을 붙여 "분석"으로 두었다. 가정은 서술된 설계가 아니다. 또 SimRacer의 SR에 메모리가 있었는지부터 정해지지 않는다. 그래서 C1·C3·C5·R2의 Q2를 모두 "불명"으로 바꿨다. C3·R2는 SIMEXPLORER.md에서 "분석"이지만, 그쪽은 STVR이 서술하는 도구 자체를 다루므로 근거가 다르다 |
| 10 | (2026-10-16) Sim-O/C(ITJ 2013)의 process 사이 서술 | 같은 해 같은 저자의 서술은 여러 user process 사이의 시험을 "system calls that can potentially race with each other"의 순서 바꾸기로 적는다(p.188). SCMiner p.11 `[2차]`의 "interleavings of system calls"와 같은 층위다. SimRacer의 process 사이 사건이 system call에 한정됐다는 결론은 아니다. Sim-O/C 본문은 메모리 관측을 배제하지 않고, 관측기가 BasicConfig에 적힌 공유 메모리 주소를 다른 프로그램도 접근할 수 있다고 적는다(p.185). 그러나 메모리 중단점의 예는 응용과 ISR 사이다 |

---

## 2. 도구의 정체

### 2.1 요약

"대상" 칸의 SimRacer는 ISSTA'13 도구, SIMEXPLORER는 STVR 2017 도구를 뜻한다.

| 항목 | 내용 | 근거·등급 | 대상 |
|---|---|---|---|
| 역할 | **탐지(detection).** "testing-based framework"로 process-level race를 테스트한다. 잠재 race를 계산하고 실행으로 확인하므로 확인 단계를 포함하지만, 보고된 실패를 재현하는 도구가 아니다. RRF는 스스로를 "not a race detection system"이라 하고 SimRacer를 active testing 계열의 탐지 접근으로 둔다 | ISSTA 초록 `[초록]`; STVR p.7 `[전문]`; RRF p.3 `[2차]` | 둘 다 |
| 실행 단위 | STVR이 다루는 사건 원천 4종: ① 일반 user process(fork, execv로 생성) ② software signal과 handler ③ hardware interrupt와 ISR ④ kernel thread. 관측은 "process under test(PuT)"가 시작한 연산만 대상이다. Linux에서 thread는 PID를 가진 lightweight process라 "just like a process" 추적할 수 있다고 한 줄 적는다(replay 맥락) | STVR p.7, p.13, p.14 `[전문]` | SIMEXPLORER. SimRacer는 ①과 한 종류의 signal handler만 다룬 것으로 읽힌다 [I](§2.3) |
| 관측하는 것 | SR 접근: shared memory, file descriptor, file inode, file data, process data(`/proc`, PID), hardware component. 각 접근은 read 또는 write로 분류한다. 동기화 사건: `fork`, `wait`, `exit`, `pipe`, signal 등록·활성·비활성(`sigaction`, `signal`, `sigprocmask`), interrupt 활성 비트. system call 23개의 read/write 효과를 모델링한다(예시: `open`, `read`, `write`, `lstat`, `clone`, `wait`, `execve`, `exit`). 전체 목록은 본문에 없다 | STVR p.8 `[전문]` | SIMEXPLORER |
| 관측 구현 | Simics의 콜백으로 구현한다. 공유 자원 연산에 메모리 중단점을 건다. PuT 이외의 연산은 PuT 함수의 frame pointer를 쌓은 call stack과 비교해 걸러 낸다. 알고리즘은 Python으로 쓴 Simics tool이다 | STVR p.14 `[전문]` | SIMEXPLORER |
| 자원 식별 | SR 종류별로 "같은 SR"에 대한 접근 쌍을 찾는다. 같은 SR로 판정하는 기준(이름, inode, 주소 등)은 서술이 없다. 이름·ID·슬롯 사이의 대응, 식별자 재사용(generation)에 관한 서술은 없다(본문 키워드 "generation" 0건, 참고문헌 제목 외). 개발자가 다른 SR과 다른 syscall을 지정하는 사용자 인터페이스가 있다는 한 문장이 있다. 입도는 서술이 없다 | STVR p.8, p.10 `[전문]` | SIMEXPLORER |
| 순서·의존 모델 | **happens-before(HB)** 규칙 6개와 전이 폐포. ① 같은 process 안의 순서 ② 다른 process·handler 사이에서 "ej는 ei 없이는 일어나지 않는다"는 의존(Linux syscall 사양에서 추론: `fork`, `wait`-`exit`, `pipe`) ③–⑥ signal·interrupt handler와 process 사이(signal 또는 interrupt 비트가 켜진 구간에서 정함). **잠재 race 쌍**은 같은 SR을 접근하고, 하나 이상이 write이며, HB가 없는 쌍이다(Figure 10). Phase 1은 모든 동기화 연산을 식별하지는 못해서 거짓 양성이 생긴다고 적는다(p.10). 사용자 정의 동기화를 인식하지 못할 수 있다는 서술은 happens-before 기반 동적 기법 일반에 대한 것이다(p.2). 대상은 두 접근 사이의 order violation이며 세 사건 이상이 얽힌 atomicity violation은 놓칠 수 있다고 적는다 | STVR p.2, p.9, p.10, p.14 `[전문]` | SIMEXPLORER. SimRacer의 규칙은 signal·interrupt 부분이 다르다(§2.3) |
| race의 정의 | "(1) two system events … access a shared resource, and (2) they could access the shared resource in an order different from the intended order." 의도한 순서를 입력으로 받는다는 서술은 없다 | STVR p.3 `[전문]` | SIMEXPLORER |
| 입력 | PuT와 짝지을 다른 process 또는 handler(basic program마다 최소 하나), 기존 test input(Phase 1은 임의 스케줄로 실행), test oracle. 연구에서 oracle은 콘솔 메시지와 생성·기록된 파일의 출력이며, 두 process를 순차 실행해 얻은 출력이다. SimRacer는 기존 테스트로 trace를 만든다는 서술만 있고, 새 입력을 만들지 못한다 | STVR p.7, p.16–17 `[전문]`; ISSTA 초록 `[초록]`; DESCRY p.8, SimEvo p.8 `[2차]` | 둘 다 |
| 출력 | ① PRaceSet: 잠재 race 쌍(명령어 위치 쌍) ② RaceSet: 순서를 뒤집은 채 두 접근이 실제로 일어난 쌍(real race) ③ oracle과 출력이 다르면 "Error: fault found". 이 셋을 구분해 보고한다. harmful과 benign의 분류는 수작업이다 | STVR p.7, p.11, p.13 Figure 12 `[전문]` | SIMEXPLORER |
| 플랫폼 | Simics 위에서 동작한다. 제어는 dummy device와 guest 안의 driver 프로그램이 한다. 연구 환경은 단일 코어 x86 Linux 2.6.15, 3.5.0 guest다. Simics 버전, 머신 모델, 스크립트, host는 서술이 없다. SimRacer는 "commercial virtual platform"(초록)이고 SimEvo가 "built on top of the Simics Virtual Platform"이라고 적는다 | STVR p.2, p.14, p.18 `[전문]`; ISSTA 초록 `[초록]`; SimEvo p.7 `[2차]` | 둘 다 |

### 2.2 동작 흐름

[F] STVR §3 (p.7)와 Figure 10, 12, 13 `[전문]`.

1. **관측(Phase 1).** 기존 입력을 임의 스케줄로 실행하며 각 PuT의 SR 접근과 동기화 사건을 로그로 남긴다. 접근 쌍은 "OP(SR)"의 진입과 종료(`ENT`, `EXT`)로 표현한다. syscall이면 호출 명령과 반환 명령, 메모리·하드웨어 접근이면 load/store 명령과 그 다음 명령이다(p.8–9).
2. **쌍 계산.** 두 로그에서 같은 SR을 접근하고 HB가 없고 둘 다 read가 아닌 쌍을 PRaceSet에 넣는다. 쌍은 관측된 실행 순서대로 기록하며, 명령어 위치가 같은 중복(반복문·재귀)은 한 번만 남긴다(p.10).
3. **순서 뒤집기(Phase 2).** 쌍마다 한 번의 실행에서 한 쌍만 시험한다(p.13). 두 접근 직전에 각 process를 멈추고, 뒤에 있던 접근을 먼저 진행시킨 뒤 앞에 있던 접근을 진행시킨다. 두 접근이 뒤집힌 순서로 일어나면 RaceSet에 넣는다. 시스템 상태 때문에 바로 이어서 실행하지 못하면 다른 쪽을 계속 진행시켜 가능한 시점을 찾고, 다른 쌍의 진입 명령에 닿을 때까지 불가능하면 그 쌍은 race로 세지 않는다(p.10–11).
4. **판정.** 두 사건이 끝나면 출력을 oracle과 비교한다. 다르면 오류를 출력한다(Figure 12 25행). race가 확인됐지만 출력에 영향이 없는 경우를 따로 서술한다(p.19, p.22).
5. 제어는 Simics 안의 dummy device가 인터럽트를 일으키고, guest의 interrupt handler가 `task_struct`를 보고 process를 멈추거나 재개하거나 signal을 보내는 방식이다(p.14).

### 2.3 SimRacer 자체와 SIMEXPLORER의 차이 (자료로 확인되는 만큼)

STVR p.24 §7.1이 "SimRacer 알고리즘이 하지 못했던 것"을 직접 나열한다 `[전문]`. SimRacer 전문을 읽지 못했으므로 아래 "같다"는 모두 추론이다.

| 항목 | 근거 | 내용 |
|---|---|---|
| hardware interrupt | STVR p.24 [F] | "The algorithm presented in [16] did not handle hardware interrupts." SimRacer에는 없다 |
| 중첩 handler | STVR p.24 [F] | 중첩된 interrupt·software signal handler를 SimRacer 알고리즘은 다루지 않았다 |
| signal 종류 | STVR p.24 [F] | SimRacer 알고리즘은 한 종류의 signal handler만 다뤘고, STVR은 Linux의 31종을 다룬다 |
| 평가 대상 | STVR p.24 [F]; ISSTA 초록 | SimRacer 연구는 process-level race만, 16개 프로그램이었다. STVR은 24개다. STVR은 그 이전 연구가 "included in this article"이라고 적는다 |
| 알고리즘 개선 목록 | STVR p.24 [F] | STVR이 꼽는 알고리즘 개선은 세 가지(hardware interrupt, 중첩 handler, signal 31종)다. SR 종류(메모리 포함)를 넓혔다는 항목은 이 목록에 없다. 이것은 SimRacer에 메모리 SR이 있었다는 근거가 아니다. 목록에 없다는 것만 확인된다 |
| process 사이 사건의 층위 (2026-10-16) | ITJ p.188 `[2차]` | 같은 해의 Sim-O/C는 여러 user process 사이를 "first observes the correct execution of two processes and identifies system calls that can potentially race with each other"로 적고, 그 순서를 바꿔 실패가 나면 결함으로 본다 |
| 실행 단위 구별 (2026-10-16) | ITJ p.185 `[2차]` | Simics 콜백만으로는 process와 thread 같은 실행 단위를 구별하지 못하고, Simics의 process tracker가 user-space process를 구별한다고 적는다. thread를 따로 구별한다는 서술은 이 쪽에 없다 |
| 구현 서술 | STVR p.24 [F] | 구현을 설명한 §4는 STVR에서 새로 넣은 절이다. ISSTA'13이 같은 구현 내용을 담았는지는 정해지지 않는다 |
| 제약(HB) 모델 | SimEvo p.3 `[2차]` | SimEvo는 "previous work [57](SimRacer)"의 partial order로 program order, fork-return, wait-exit, pipe-read, signal order 다섯 가지를 든다. STVR p.9의 규칙 ①②(fork, wait-exit, pipe)와 signal 규칙에 대응한다 [I] |
| 메모리 관측 | STVR p.8 [F]; VEE'12 p.54 `[2차]` | "SimExplorer identifies reads and writes using the observability support implemented in our prior work [16,33]." [16]은 SimRacer, [33]은 SimTester(VEE 2012)다. SimTester는 응용과 ISR이 함께 쓰는 공유 변수에 메모리 read/write 중단점을 건다(2026-10-16 확인). 이 관측 지원이 SimRacer의 process 사이 SR에도 쓰였는지는 정해지지 않는다 **[U]** |
| 시스템콜 서술 | SCMiner p.11 `[2차]` | "SimRacer [18] and RacePro [11] aim to detect process-level concurrency faults by testing for different interleavings of system calls." 이 문장은 SIMEXPLORER가 shared memory와 hardware를 SR로 둔다는 STVR 서술과 층위가 다르다. 둘 중 어느 쪽이 SimRacer를 더 정확히 말하는지 정해지지 않는다 |
| 탐색 방식 | SimEvo p.8 `[2차]` vs STVR p.10–13 | SimEvo는 SimRacer가 "first identifies anomalous event pairs and then randomly explores interleaving schedules"라고 적는다. STVR의 Phase 2는 쌍마다 순서를 뒤집는 결정적 제어다. "randomly"가 쌍의 선택을 뜻하는지 스케줄 제어를 뜻하는지 정해지지 않는다 **[U]** |
| 입력 | DESCRY p.8, SimEvo p.8 `[2차]` | SimRacer는 구체적 data input을 요구하고 새 입력을 만들지 못한다. 기존 입력만 쓴다 |

**정리.** process·signal 부분의 Phase 1–2 구조(관측 → HB로 쌍 계산 → 순서 뒤집기 확인)는 ISSTA 초록의 서술(computes potential races … controls process scheduling relative to the potential races)과 일치하므로 SimRacer에도 해당한다고 본다 [I]. §3의 도구 수준 답은 이 부분에 한정해 SimRacer에 연결하고, 메모리 관측처럼 정해지지 않는 항목은 "SimRacer 근거" 칸에 따로 적는다. 사례별 칸(§3.5)에서는 SR의 종류(메모리가 SR인지)에 기대는 Q1·Q2를 SimRacer에 대해 "불명"으로 두고, 쌍의 모양·순서 뒤집기·출력 종류에 기대는 Q3·Q5·Q6만 "분석"으로 둔다(2026-10-16).

### 2.4 논문이 직접 밝힌 한계 중 cFS와 관련 있는 것

| 한계 | 인용 | 쪽 |
|---|---|---|
| SR은 해로운 race를 낳을 만한 것으로 제한하고 표준 입출력, 커널의 process 데이터는 추적하지 않는다. 다른 SR은 개발자가 지정한다 | "SIMEXPLORER provides a user interface that lets developers specify other SRs" | p.8 |
| semaphore와 lock 등 다른 자원은 설계의 확장 대상으로만 언급하고, 이 논문의 초점에서 뺀다 | "can be extended to cover other types of resources that include semaphores and locks" | p.9 |
| Phase 1은 모든 동기화 연산을 식별하지 못해 잠재 race에 거짓 양성이 생긴다. 프로세스 수준 동기화는 구현을 모르는 syscall이 많아 의미를 정확히 모델링하기 어렵다고 하고, 구현에 대한 선입견을 두지 않는 방식을 택한다 | "does not identify all synchronization operations" | p.10, p.13 |
| 사용자 정의 동기화를 인식하지 못해 거짓 양성이 생길 수 있다는 서술은 happens-before 기반 동적 기법 일반에 대한 것이다(SIMEXPLORER의 Phase 1도 HB 기반이다) | "may also fail to recognize user-defined synchronization primitives" | p.2 |
| 쌍을 뒤집을 수 없으면 race로 세지 않는다 | "is not considered to be a race" | p.11 |
| 한 실행에서 한 쌍만 시험한다 | fault masking을 피하려는 선택으로 서술 | p.13 |
| 한 process 안의 다른 비결정 요인은 제어하지 않아 재현이 불완전할 수 있다 | "it can be incomplete" | p.13 |
| 한쪽을 멈추면 다른 쪽이 기다리다 교착할 수 있다. 주기적으로 상태를 확인해 풀어 준다. 객체 프로그램 4개에서 관측했다 | — | p.13 |
| 두 접근 사이의 order violation만 다룬다. atomicity violation 등 세 사건 이상은 놓칠 수 있다 | "it can miss races involving more than 2 events" | p.14 |
| 초기 test suite와 oracle에 의존한다. 공유 변수를 실행하지 않는 테스트에서는 race가 보이지 않았다. race가 출력에 전파되지 않은 사례가 있다 | — | p.19, p.22 |
| 실패는 출력 기반 oracle로 보고했다. 내부 oracle은 수작업·시간·거짓 양성 비용이 있다고 한다 | "We report failures using output-based oracles" | p.18 |
| 연구한 Simics 버전은 하드웨어 가상화를 지원하는 최근 Intel CPU를 모사하지 않는다 | "does not simulate the later Intel processors with hardware virtualization support" | p.18 |
| race 발현은 시스템 구성에 민감하다(IA64에서만 나타나는 race를 x86에서 보지 못했다) | — | p.22 |

---

## 3. 여섯 질문 × 다섯 사례

### 3.1 읽는 법

셀의 라벨은 아래 셋 중 하나다. 한 칸에 관측 가능성, 표현 가능성, 실제 탐지 성공을 합치지 않는다(로드맵 §10.2).

| 라벨 | 뜻 |
|---|---|
| **답** | 출처의 쪽·절로 답했다(answered with source page/section) |
| **분석** | analysis from stated design (not run). 읽은 설계 기술에서 도출했고 실행하지 않았다 |
| **불명** | not determinable from sources read |

"분석" 셀의 "경로가 서술에 없다"는 "읽은 설계 기술 안에서 그 쌍이 후보 입력이 되는 경로를 찾지 못했다"는 뜻이다. 실행 결과가 아니다. 셀의 근거는 별도 표시가 없으면 STVR(`[전문]`)의 쪽이다. 질문 번호는 로드맵 §10.2의 순서다.

1. 필요한 **관측 사건**을 수집하는가?
2. 서로 다른 이름·ID·필드가 같은 **자원·세대**를 가리키는 것을 식별하는가?
3. **생성·등록·초기화 완료와 의존 사용**의 관계를 표현하는가?
4. 필요한 **순서 또는 허용된 오류 처리** 정보를 입력받는가, 추론하는가?
5. 아직 실행되지 않은 순서를 **예측·탐색**하는가? 실제 실행에서 **확인**할 수 있는가?
6. **출력**은 후보·재현 성공·오류 결과 중 무엇이며 원 사례의 현상과 대응하는가?

### 3.2 사례 쪽에서 쓴 사실 (모두 저장소 기록, 재검증하지 않음)

| 사례 | 의존성과 현상 | 출처 |
|---|---|---|
| C1 CF#184 | provider가 이름 있는 OSAL count semaphore를 만든 뒤에야 CF의 `OS_CountSemGetIdByName`이 성공한다. provider는 원 보고에서 미식별이고 공개 BP 구성에서는 `OS_CountSemCreate`다. CF는 한 번만 조회하고 실패하면 `CF_EID_ERR_INIT_SEM`과 `APP_ERROR` 종료다. 생성이 조회보다 먼저임을 정하는 장치가 없다. 수정은 `OS_ERR_NAME_NOT_FOUND`일 때만 25회 × 100 ms 재시도다. 실패 출력과 빈도는 `unspecified_by_reference` | `cases/ledger.md` §1a, §1b, §2 `[기록]` |
| C2 cFE#198 | 앱 P의 late init 완료와 다른 앱 R의 의존 호출. P·R은 비공개(EVA CWS)이고 통신 방식·초기화 내용이 공개되지 않았다. 공개된 것은 단일 상태 시작 동기화(`WaitForStartupSync`)와 6.6.0a의 두 단계 방식(`APPS_INIT`, `LATE_INIT`, `CFE_ES_WaitForSystemState`)이다 | `ledger.md` §1a, §1b, §2; 로드맵 §9.1 `[기록]` |
| C3 cFE#73 | SB task의 `CFE_SB.AppId`와 EVS task의 `EVS_AppID`·등록 플래그가 설정되기 전에 TIME task가 `CFE_SB_CreatePipe` 뒤 `CFE_EVS_SendEventWithAppID`(AppId=0)를 부르고 `EVS_IsFiltered(0xFFFFFFFF)`가 범위 검사 없이 접근한다. 우선순위(TIME 60, EVS 61, SB 64)가 순서를 만든다. 원 플랫폼은 Microblaze이고 오류 지점은 미확인 | `ledger.md` §1a, §2 `[기록]` |
| C5 cFE#72 | `CFE_ES_AppCreate`가 ES lock을 `OS_TaskCreate` 앞에서 풀고 뒤에서 다시 잡아 `TaskTable`을 채운다. 그 사이 새 task의 `CFE_ES_GetAppID`가 `CFE_ES_ERR_APPID`를 반환할 수 있다. 위반은 "등록 전 조회"이고 일찍 실행된 것만으로는 위반이 아니다. 수정(6.4.2)은 lock 범위 확장이다 | `ledger.md` §1a, §1b `[기록]` |
| R2 cFE#2663 | SB가 `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`로 AppId를 저장하기 전에 ES/EVS task가 SB API 경로(`CreatePipe`, `Subscribe`, 이벤트 전송)를 실행해 `EVS_GetAppDataByID`가 NULL을 돌려주고 `ILLEGAL_APP_ID`가 된다. 상태는 폐기된다. 서술은 "이벤트 요청 거부"이고 gdb 3회 모두 설정 전 사용 24회다. core 시작 직렬화(S1)가 EVS·ES 초기화를 SB보다 앞세운다. race인지 결정적 순서인지는 실행 확인이 필요한 미결 항목이다 | `ledger.md` §1a, §2; 로드맵 §2.2 `[기록]` |
| 공통 | native cFS는 한 Linux process이고 OSAL task는 `pthread_create` thread다. OSAL 자체 시험에서 named count semaphore의 생성과 이름 조회는 **syscall 0건**이었다. SB pipe는 `mq_open("<pid>.<name>")`을 부른다 | `availability.md` §4.2 `[기록]`. OSAL 시험의 probe이며 SimRacer의 결과가 아니다 |

### 3.3 도구 수준의 답 (사례와 무관)

| Q | 라벨 | SIMEXPLORER(STVR `[전문]`)의 답 | 근거 | SimRacer 자체에 대한 근거 |
|---|---|---|---|---|
| 1 관측 사건 | 답 | SR 접근(shared memory, fd, inode, file data, `/proc` process 데이터, hardware)과 동기화 사건(fork, wait, exit, pipe, signal, interrupt 비트)을 모은다. 메모리 접근은 Simics 중단점으로 잡는다. 응용 함수의 반환값과 메모리에 쓰인 값은 수집 대상으로 열거되지 않는다(PuT 함수의 frame pointer를 call stack으로 따라가지만 PuT가 아닌 연산을 거르는 용도다). 개발자가 SR과 syscall을 더 지정할 수 있다 | p.8, p.14 | 초록: "runtime traces obtained by running existing tests" `[초록]`. SimEvo p.3 `[2차]`: shared resource와 event 모델링의 세부는 선행 연구 [26](RacePro)과 [57](SimRacer)에 있다고 하며, trace의 사건은 shared resource access(R/W)와 동기화 연산이다. 메모리 load/store가 SimRacer에 있었는지는 **불명**(STVR p.8이 [16,33]을 함께 인용). 2013년 Sim-O/C 서술(ITJ p.188 `[2차]`)은 process 사이 사건을 system call로 적는다(§2.3) |
| 2 자원·세대 식별 | 답 | SR 종류별로 같은 SR 접근을 짝짓는다. "같은 SR"의 판정 기준, 이름·ID·슬롯 대응, 세대에 관한 서술은 없다(본문 "generation" 0건) | p.8, p.10; 키워드 검색 | 서술 없음. SimRacer 전문 **불명**. Sim-O/C의 예는 "두 process가 모두 `write`로 같은 파일에 쓰는" 경우다(ITJ p.188 `[2차]`). 같은 파일의 판정 기준은 적혀 있지 않다 |
| 3 생성·등록 vs 의존 사용 | 답 | "같은 SR, 하나 이상 write, HB 없음"인 접근 쌍으로 표현한다(Figure 10). 생성은 write, 사용은 read다. 등록 완료나 초기화 완료 같은 상태 개념은 없다. HB는 process 순서, fork 이전 사건, wait-exit, pipe, signal·interrupt 활성에서만 나온다. lock과 semaphore는 모델 밖이다 | p.9, p.10 | SimEvo p.3 `[2차]`: 같은 종류의 partial order |
| 4 필요 순서·허용 오류 입력 | 답 | 입력이 아니다. race 정의에 "intended order"가 있지만 입력 필드는 없고, 순서는 syscall 의미에서 추론한 HB로만 들어온다. 허용 오류와 재시도에 관한 서술은 없다(본문 "retry" 0건). 입력은 PuT 쌍, 기존 test input, oracle(출력 기반)이다 | p.3, p.9, p.16–18 | DESCRY p.8, SimEvo p.8 `[2차]`: 구체적 data input을 요구하고 입력을 만들지 못한다 |
| 5 예측·탐색, 확인 | 답 | 설계상 있다. Phase 1이 한 실행의 trace에서 실행되지 않은 반대 순서의 잠재 쌍을 계산하고, Phase 2가 쌍마다 실제 실행에서 순서를 뒤집어 확인한다. 한 실행에 한 쌍이고 두 접근 사이의 order violation에 한정한다 | p.7, p.10–11, p.13–14 | 초록 `[초록]`: "computes potential races … controls process scheduling relative to the potential races so that real races can be created". RRF p.3 `[2차]`: "pause and resume binary execution" |
| 6 출력 | 답 | 세 종류를 구분한다. 잠재 쌍(PRaceSet), real race(RaceSet), oracle 불일치(fault). real race이지만 출력에 전파되지 않은 경우를 따로 보고한다. 같은 명령어 위치의 반복 접근은 한 쌍으로 합친다. harmful과 benign 분류는 수작업이다 | p.10, p.13, p.19, p.22 | 초록 `[초록]`: race와 그 race가 일으킨 fault를 구분하는 서술 |

### 3.4 5 × 6 라벨 요약

| | Q1 관측 | Q2 식별 | Q3 생성·사용 | Q4 입력 | Q5 예측·확인 | Q6 출력 |
|---|---|---|---|---|---|---|
| **C1** | 불명 | 불명 | 분석 | 답 | 분석 | 분석 |
| **C2** | 불명 | 불명 | 불명 | 답 | 불명 | 불명 |
| **C3** | 불명 | 불명 | 분석 | 답 | 분석 | 분석 |
| **C5** | 불명 | 불명 | 분석 | 답 | 분석 | 분석 |
| **R2** | 불명 | 불명 | 분석 | 답 | 분석 | 분석 |

2026-10-16 변경: C1·C3·C5·R2의 Q1·Q2가 "분석"에서 "불명"으로 바뀌었다(이유는 §1.3 항목 8–10). Q4를 뺀 사례별 25칸은 이제 분석 12, 불명 13이다. 2026-10-15판은 분석 20, 불명 5였다. 이 숫자는 점수가 아니고 근거의 종류를 센 것이다.

### 3.5 사례별 셀 (근거 포함)

모든 "분석" 셀에는 한 가지 전제가 공통으로 걸려 있다. **사례의 접근이 SR로 관측되어야 한다.** 이 전제와, 한 process 안의 task 쌍을 PuT 쌍으로 줄 수 있는지는 §6 항목 3, 4에 미확인으로 적었다.

Q1·Q2 칸(2026-10-16 "불명"으로 변경)은 두 층으로 적는다. 라벨은 **SimRacer(ISSTA'13) 자체**에 대한 것이다. 그 뒤의 "SIMEXPLORER 설계 기준" 문장은 확장판 STVR의 설계에서 도출한 분석으로, 라벨을 정하지 않는다. 같은 분석은 `plan/methods/SIMEXPLORER.md`가 그 도구의 칸으로 다룬다.

#### C1 — CF#184 (named count semaphore의 생성 vs 이름 조회)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 불명 | **SimRacer 자체:** 필요한 사건 중 메모리 쓰기·읽기가 SimRacer의 SR 사건이었는지 정해지지 않는다(§2.3, §1.3 항목 8·10). **SIMEXPLORER 설계 기준(분석, 라벨 아님):** 필요한 사건은 provider의 이름 등록(OSAL 이름 테이블 쓰기)과 CF의 이름 조회(읽기)다. 둘 다 syscall이 아니다(OSAL 자체 시험, probe). 설계는 SR 접근을 메모리 중단점으로 잡으므로(p.14) OSAL 테이블의 해당 주소가 SR로 지정되면 사건이 될 수 있다는 것이 설계에서 도출된다. 어떤 주소가 SR이 되는지(자동 선택인지 사용자 지정인지)와 지정 인터페이스의 입도는 서술이 없다. 두 사건은 한 process 안의 서로 다른 thread다 | p.8, p.14; `availability.md` §4.2 `[기록]`; §6 항목 3 |
| 2 | 불명 | **SimRacer 자체:** "같은 SR"의 판정 기준도, 메모리가 SR이었는지도 서술되지 않았다(§1.3 항목 9). **SIMEXPLORER 설계 기준(분석, 라벨 아님):** 세마포어 이름(공개 BP 구성에서는 `BP0_tsem`. 원 보고자 환경의 이름은 미확인, `cases/C1.md`) → OSAL ID → 이름 테이블 슬롯 → `sem_init`된 객체를 같은 자원으로 잇는 장치는 서술이 없다. SR 종류에도 OSAL 객체가 없고 세대 서술도 없다. 메모리 SR이 같은 슬롯의 쓰기와 읽기를 짝지을 수 있다는 해석은 같은 SR의 판정 기준이 주소라는 가정이 필요하다 [I] | p.8, p.10; 키워드 검색 |
| 3 | 분석 | 생성(쓰기)과 조회(읽기)는 "같은 SR, 하나 이상 쓰기, HB 없음" 쌍(Figure 10)으로 표현될 수 있다. OSAL 레코드가 예약된 상태인지 공개 완료인지, 재시도 중인지 최종 실패인지를 구분하는 개념은 모델에 없다. OSAL 이름 테이블 lock(PI mutex)은 HB 규칙에 없다 | p.9, p.10 |
| 4 | 답 | 도구 수준 답과 같다. C1이 필요로 하는 허용 오류("재시도는 `OS_ERR_NAME_NOT_FOUND`일 때만")를 넣을 인터페이스는 없다. 실패 기준은 사용자가 출력 기반 oracle로 써야 한다(예: CF 초기화 실패 이벤트나 종료) | p.17–18 |
| 5 | 분석 | 쌍이 SR로 관측되면 Phase 1 → Phase 2 구조가 적용될 수 있는 모양이다. 관측된 trace의 순서가 생성 → 조회이면 Phase 2는 조회를 생성 앞으로 뒤집는다. 이 방향이 실패 순서다. 어느 쪽이 정상 순서인지는 도구가 알지 못한다. 확인은 Simics 실행이다. 25회 재시도 수정 버전에서도 같은 쌍이 real race가 될 수 있고, 최종 실패인지는 oracle이 정한다 [I] | p.10–11, p.13 |
| 6 | 분석 | 출력 유형은 잠재 쌍, real race, oracle fault다. C1의 현상은 "CF 초기화 실패(이벤트와 `APP_ERROR` 종료)"와 "재시도 뒤 회복 vs 최종 실패"의 구분이다. real race와 fault를 나눠 보고하는 구조는 있으나 회복과 최종 실패를 구분하는 출력 항목은 서술에 없다. 사례의 실패 출력이 `unspecified_by_reference`라 oracle이 볼 출력이 정해지지 않았다 | p.19, p.22; `ledger.md` §1b `[기록]` |

#### C2 — cFE#198 (late init 완료 vs 다른 앱의 의존 호출)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 불명 | 필요한 사건은 P의 late init 완료와 R의 의존 호출이다. P·R이 비공개이고 통신 방식·초기화 내용이 알려지지 않아 어떤 자원을 거치는지 판단할 근거가 없다. 공개된 시작 동기화 부분은 아래 별도 표를 본다 | `ledger.md` §1a `[기록]`; 로드맵 §9.1 |
| 2 | 불명 | 같은 이유 | — |
| 3 | 불명 | 같은 이유. "barrier 통과"와 "앱 고유 초기화 완료"를 구분하는 상태 개념은 STVR 본문에 없다(키워드 "startup" 0건, "initializ" 0건). 그러나 P의 late init이 거치는 자원이 비공개라 이 칸은 불명으로 둔다 | 키워드 검색 |
| 4 | 답 | 도구 수준 답과 같다. 실패 기준은 원 보고가 구체화하지 않아 연구 쪽 선택이 된다. 선택하더라도 원 보고자의 값으로 인용하지 않는다 | p.17–18; [D] 결정 문서 §3.5 |
| 5 | 불명 | 쌍이 될 접근을 알 수 없다 | — |
| 6 | 불명 | 원 현상이 구체 출력으로 공개되지 않았다 | `ledger.md` §1a `[기록]` |

C2의 **공개 부분** (역할 재구성에 쓸 수 있는 알려진 조건). 아래 라벨은 SIMEXPLORER 설계 기준이다. SimRacer 자체로는 상태 필드가 메모리라서 불명이다(§1.3 항목 8):

| 사건 | 흔적 | SIMEXPLORER 입력(설계) | 라벨 |
|---|---|---|---|
| `CFE_ES_WaitForSystemState`가 상태 변수가 목표에 이를 때까지 `OS_TaskDelay`로 기다림 | 상태는 `CFE_ES_Global`의 메모리 필드. 대기는 sleep 계열 syscall | 상태 필드를 SR로 지정하면 메모리 read와 write 사건이 된다. sleep이 모델링된 23개 syscall에 드는지는 목록이 없어 불명이다 | 분석 |
| 단일 상태 / 두 단계 barrier의 전이와 timeout | 같은 메모리 필드 | 위와 같다. 상태 전이를 "시작 단계"나 "준비 상태"로 읽는 개념은 모델에 없다 | 분석 |

[F 기록] 코드 근거는 `baselines/racepro.md` §6와 `availability.md`의 `cfe_es_api.c:514, 585, 603` 서술이며 재검증하지 않았다.

#### C3 — cFE#73 (SB/EVS AppId 설정 vs TIME의 이벤트 요청)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 불명 | **SimRacer 자체:** 필요한 사건 중 메모리 쓰기·읽기가 SimRacer의 SR 사건이었는지 정해지지 않는다(§2.3, §1.3 항목 8·10). **SIMEXPLORER 설계 기준(분석, 라벨 아님):** 필요한 사건은 `CFE_SB.AppId` 저장, `EVS_AppID`와 등록 플래그 설정, TIME의 이벤트 요청(AppId=0)과 `EVS_IsFiltered`의 범위 검사 없는 접근이다. 모두 사용자 공간의 전역 메모리 읽기와 쓰기다. 설계상 SR로 지정되면 사건이 된다. TIME의 `CreatePipe`가 부르는 `mq_open`(현재 OSAL에서 측정, 6.4.1 시대는 추정)이 모델링된 syscall에 드는지는 서술이 없다. 원 플랫폼(Microblaze)은 STVR이 시험한 플랫폼(x86 Linux)이 아니다. "Simics에서 돌 수 있는 시스템이면 쓸 수 있다"는 서술만 있다 | p.2, p.8, p.18; `ledger.md` §1a `[기록]` |
| 2 | 불명 | **SimRacer 자체:** "같은 SR"의 판정 기준도, 메모리가 SR이었는지도 서술되지 않았다(§1.3 항목 9). **SIMEXPLORER 설계 기준(분석, 라벨 아님):** 의존 상태는 서로 다른 세 필드(SB AppId, EVS AppId, EVS 등록 플래그)다. 메모리 위치로 구별할 수 있다는 해석은 주소 기반 동일성 가정이 필요하다 [I]. AppId 값 0이 EVS 배열의 인덱스로 쓰이는 관계(값 → 슬롯)를 추적하는 서술은 없다 | p.8, p.10 |
| 3 | 분석 | 공개(쓰기)와 사용(읽기)이 메모리라 같은 SR의 쌍으로 표현될 수 있다. core 시작 순서를 정하는 ES의 task 대기(F3 수정)와 우선순위는 HB 규칙에 없다 | p.9, p.10 |
| 4 | 답 | 도구 수준 답과 같다. 원 보고의 실패는 EVS 내부의 잘못된 인덱스 접근이고 오류 지점과 주 결과는 사례 기록상 미결이라, oracle이 무엇을 봐야 하는지 지금 정할 수 없다 | p.17–18; `ledger.md` §2 `[기록]` |
| 5 | 분석 | C1과 같다. 사례의 순서(TIME 사용 → SB/EVS 공개)는 우선순위가 만든 것이라 관측된 trace에 이미 있을 수 있다. 그러면 Phase 2는 그 반대(공개 → 사용)를 시도한다. 도구는 어느 쪽이 정상 순서인지 알지 못한다 [I] | p.10–11 |
| 6 | 분석 | 사례 현상은 "잘못된 상태 사용"과 "실제 crash 여부"를 분리해 보고하는 것이다(로드맵 §9.2). real race와 oracle fault를 분리하는 구조는 있다. 그러나 crash를 별도로 검출한다는 서술은 없다(본문 "crash"는 예시 설명 한 곳, "segfault" 0건). 사례 기록상 crash 자체가 미확인이다 | p.4, p.19; `ledger.md` §2 `[기록]` |

#### C5 — cFE#72 (task 생성과 레지스트리 등록 사이의 공백)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 불명 | **SimRacer 자체:** 필요한 사건 중 메모리 쓰기·읽기가 SimRacer의 SR 사건이었는지 정해지지 않는다(§2.3, §1.3 항목 8·10). **SIMEXPLORER 설계 기준(분석, 라벨 아님):** `OS_TaskCreate`의 `pthread_create`는 `clone` 계열이다. `clone`은 모델링된 syscall의 예시에 들고(p.8) `/proc`에 process inode를 만드는 쓰기로 모델링된다. `TaskTable` 등록 쓰기와 새 task의 `GetAppID` 조회는 사용자 공간 메모리 접근이라 SR로 지정되어야 사건이 된다 | p.8; `ledger.md` §1a `[기록]` |
| 2 | 불명 | **SimRacer 자체:** "같은 SR"의 판정 기준도, 메모리가 SR이었는지도 서술되지 않았다(§1.3 항목 9). **SIMEXPLORER 설계 기준(분석, 라벨 아님):** 커널 쪽 task 식별은 PID다(thread도 PID를 가진 lightweight process, p.13). OSAL task ID, ES `TaskTable` 인덱스, AppId 사이의 연결을 식별하는 서술은 없다 | p.13 |
| 3 | 분석 | `fork` 규칙은 "fork 이전의 부모 사건은 자식의 모든 사건보다 앞선다"(p.9)다. thread 생성(`clone`)이 이 규칙의 대상인지는 서술이 없다. 어느 쪽이든 등록 쓰기는 task 생성 뒤에 있으므로 자식의 조회와 HB가 없고 잠재 쌍이 된다. "등록 공개 완료"라는 상태는 모델에 없다. 6.4.2의 lock 범위 확장은 lock이 모델 밖이라(p.9) Phase 1이 같은 쌍을 다시 내고, Phase 2에서 자식이 lock에 막혀 뒤집기가 불가능해지는지로 가려질 것이다 [I] | p.9, p.11, p.13 |
| 4 | 답 | 도구 수준 답과 같다. 원 현상(등록 전 조회의 실패 반환)은 crash가 아니라 내부 API 반환값이다. 출력 기반 oracle이 이를 보려면 앱의 출력에 나타나야 한다. 원 보고에 없는 앱 crash는 기대 결과로 넣지 않는다(로드맵 §9.3) | p.17–18 |
| 5 | 분석 | 이 쌍(task 생성 뒤 부모의 등록 쓰기 vs 자식의 조회)은 설계가 드는 예(부모가 fork한 자식과의 race, Figure 1)와 모양이 가장 가깝다 [I]. 관측만 되면 Phase 1 → Phase 2가 적용될 수 있다. 실행 확인은 Simics 실행이다 | p.3, p.10–11 |
| 6 | 분석 | 사례 현상은 "등록 전 조회와 그 결과"다(로드맵 §9.3). 출력은 쌍과 real race, oracle fault이고, 반환값 `CFE_ES_ERR_APPID`는 출력 항목이 아니다. real race 목록은 쌍의 명령어 위치를 담지만 조회의 반환 결과는 담지 않는다 | p.10, p.18 |

#### R2 — cFE#2663 (SB AppId 설정 vs ES/EVS의 이벤트 요청)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 불명 | **SimRacer 자체:** 필요한 사건 중 메모리 쓰기·읽기가 SimRacer의 SR 사건이었는지 정해지지 않는다(§2.3, §1.3 항목 8·10). **SIMEXPLORER 설계 기준(분석, 라벨 아님):** 아래 사건 대응표의 네 사건(AppId 설정, 값 읽기, 이벤트 요청, 오류 반환) 중 앞의 둘은 전역 메모리 쓰기와 읽기다. `CreatePipe`의 `mq_open`만 syscall이다. 오류 반환은 반환값이다 | 아래 표 |
| 2 | 불명 | **SimRacer 자체:** "같은 SR"의 판정 기준도, 메모리가 SR이었는지도 서술되지 않았다(§1.3 항목 9). **SIMEXPLORER 설계 기준(분석, 라벨 아님):** `CFE_SB_Global.AppId` 하나의 메모리 위치다. 쓰기와 읽기의 짝은 같은 위치로 맺을 수 있다는 해석이 가능하다(주소 기반 동일성 가정). EVS가 이 값을 이벤트 원천 ID로 받아 자기 테이블을 찾는 값 전달은 추적하지 않는다 | p.8, p.10 |
| 3 | 분석 | 설정(쓰기)과 사용(읽기)이 같은 SR의 쌍이다. 이 순서를 정하는 core 시작 직렬화(S1)는 HB 규칙에 없다 | p.9 |
| 4 | 답 | 도구 수준 답과 같다. R2의 요구 순서("SB AppId 저장이 SB API 경로의 이벤트 요청보다 먼저")를 넣을 입력은 없다 | p.3, p.9 |
| 5 | 분석 | 쌍이 관측되면 Phase 2는 기록 순서의 반대를 시도한다. 시작 직렬화 때문에 반대 순서를 스케줄로 만들 수 없다면 그 쌍은 race로 세지 않는다(p.11). R2의 순서가 스케줄로 뒤집힐 수 있는지가 사례 기록상 미결이므로(race인지 결정적 순서인지, `ledger.md` §2 R2 Open D2·Q3) 이 칸이 답하는 범위는 "설계상 시도하고, 불가능하면 세지 않는다"까지다. 실제 결과는 자료로 정해지지 않는다. 순서가 고정된 이유를 담는 출력 항목은 서술에 없다. **(2026-10-16 추가, [I] 미실행)** 사례 기록의 코드 읽기에 따르면 S1은 ES 시작 thread가 core task를 하나씩 만들고 그 task가 init 뒤 `CFE_ES_WaitForSystemState`에 이를 때까지 폴링으로 기다리는 구조이고, 그래서 EVS(B) 또는 ES·EVS(M, D)의 init이 SB task 생성 전에 끝난다(`cases/R2.md` §6 S1·S2, §6.1 추론; `case_basis_register.md` R2.E5 "분"). 이 구조를 STVR의 Phase 2 서술에 대입하면 다음이 도출된다. 뒤집기는 사용 task(예: EVS)를 사용 직전에 멈추고 SB의 ① 쓰기를 먼저 진행시켜야 한다. 그런데 SB task는 멈춘 task가 RUNNING에 이른 뒤에야 만들어진다. 이것은 STVR p.13이 적는 "한쪽을 멈추면 다른 쪽이 기다리는" 교착 모양이다. 교착을 풀면(p.13) 사용이 먼저 일어나 뒤집기가 이뤄지지 않고, 그 쌍은 race로 세지 않는다(p.11). 교착 해소 전에 S1의 시간 상한이 지나면 ES panic이라는 다른 결과가 된다(`cases/R2.md` A7). 이 도출은 S1에 대한 코드 읽기와 설계 서술만 쓴 것이고, 둘 중 어느 결과가 나오는지, 쌍이 애초에 관측되는지는 정해지지 않는다 | p.10–11, p.13; `ledger.md` §2 `[기록]`; `cases/R2.md` §6 `[기록]` |
| 6 | 분석 | 사례 현상은 "이벤트 요청 24회 거부"이고 프로세스 실패가 없다(상태 폐기). 반환값은 출력 항목이 아니고 실패 메시지도 출력하지 않는다. 출력 기반 oracle에는 차이가 나타나지 않을 수 있다. 같은 명령어 위치의 반복 접근은 한 쌍으로 합쳐지므로(p.10, p.13) "24"는 이 도구의 출력 수량이 아니다 | p.10, p.13, p.18; 로드맵 §2.2 `[기록]` |

**R2 사건 대응표** (로드맵 §10.3 방식의 SimRacer판). 참조 revision은 사례 기록의 B/M/D(`fc8c48ad`/`12cb84fc`/`f22e538c`)이며 v7.0.1 코드와 다를 수 있다.

2026-10-16부터 라벨 열을 둘로 나눈다. "SIMEXPLORER 설계" 열은 STVR 설계에서 도출한 분석이고, "SimRacer 자체" 열이 이 문서의 대상 도구에 대한 라벨이다.

| R2 사건 | 커널에서 보이는 흔적 | SIMEXPLORER 기록 사건(설계) | HB 간선 | 후보 규칙 | 라벨: SIMEXPLORER 설계 | 라벨: SimRacer 자체 |
|---|---|---|---|---|---|---|
| ⓪ core task 생성 (2026-10-16 추가): ES 시작 thread가 `CFE_ES_CreateObjects`에서 core task를 하나씩 `OS_TaskCreate`로 만들고, 각 task가 init 뒤 RUNNING이 될 때까지 `CFE_ES_MainTaskSyncDelay`로 폴링한다(`cases/R2.md` §6 S1·S2 `[기록]`) | `pthread_create` → `clone` 계열 syscall(C5 보충표와 같은 근거, cFS에서 측정하지 않음). 폴링은 메모리 읽기와 지연 | `clone`은 모델링된 syscall의 예시다(p.8) | fork 규칙(p.9)이 `clone`에 적용된다면 생성자의 생성 이전 사건 → 새 task의 모든 사건. 적용되는지는 서술이 없다. RUNNING 폴링은 HB 규칙(fork, wait-exit, pipe, signal·interrupt)에 없어 간선이 되지 않는다 | 후보 쌍이 아니라 HB 간선의 원천 | 분석 | 불명: SimRacer의 partial order에 fork-return이 있다(SimEvo p.3 `[2차]`). thread 생성이 그 대상인지는 서술이 없다 |
| ① SB AppId 설정: `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`, 반환값 무시 (B `cfe_sb_task.c` L126, M L138, D L128) | syscall 없음(소스 수준) | 대상 주소가 SR이면 memory write | SB task 생성 이전 사건과만 HB. 생성 뒤의 이 쓰기는 다른 task와 HB 없음 | Figure 10: 같은 SR, 쓰기, HB 없음이면 쌍 | 분석 | 불명(메모리 SR 여부, §1.3 항목 8) |
| ② 값 읽기: EVS(B) 또는 ES·EVS(M, D) task가 같은 전역을 읽음 | syscall 없음 | memory read | ①과 HB 없음. ⓪의 폴링은 간선이 아니다 | ①과의 쌍 | 분석 | 불명(같은 이유) |
| ③ 이벤트 요청: `CreatePipe`/`Subscribe`/전송 → `CFE_EVS_SendEventWithAppID` | `CreatePipe`는 `mq_open("<pid>.<name>")`(OSAL 시험 69회, probe). `Subscribe`와 이벤트 전송 자체는 syscall이 아니다(소스 수준) | `mq_open`이 23개 syscall에 드는지는 서술이 없다 | — | — | **불명** | **불명**(SimRacer의 syscall 목록도 읽지 못했다) |
| ④ 오류 반환: `EVS_GetAppDataByID` NULL → `ILLEGAL_APP_ID`, 상태 폐기 (B `cfe_evs.c` L182-L186) | 반환값 | SR이 아니다 | — | — | 분석 | 분석: 출력 기반 oracle(초록·STVR p.17–18)의 대상이 아닌 내부 반환값이다. 사용자 oracle이 이를 볼 수 있는지는 사용자 입력의 문제다 |

[I] ⓪이 들어가도 ①과 ②는 HB가 없는 쌍으로 남는다. ⓪이 주는 간선은 생성자 쪽 사건에서 새 task 쪽으로만 가고, ②를 실행하는 EVS(ES) task와 ①을 실행하는 SB task 사이에는 간선이 없다. 이것은 사용 task가 생성자인 ES 시작 thread 자신이 아니라는 전제에 선다(사례 기록의 호출 task는 ES·EVS task다, `case_basis_register.md` R2.E3). 사용이 생성자 thread에서 SB 생성 전에 일어난다면 fork 규칙이 적용되는 경우 사용 → 설정의 HB가 생겨 그 쌍은 후보가 아니다. 이 전제는 이 문서에서 코드로 다시 확인하지 않았다. 반대 순서를 실제로 만들 수 있는지는 위 Q5 칸의 도출(교착 또는 시간 상한)과 함께 읽는다.

#### C1·C3·C5 보충 사건 대응표

이 표의 라벨은 SIMEXPLORER 설계 기준이다. SimRacer 자체로는 메모리 접근 행(C1 생성·조회, C3 AppId 저장, C5 레코드 쓰기·조회)이 모두 불명이다(§1.3 항목 8). C5 `OS_TaskCreate` 행도 thread 생성이 SimRacer의 fork-return 순서 대상인지 서술이 없어 불명이다.

| 사례·사건 | 커널에서 보이는 흔적 | SIMEXPLORER 입력(설계) | 라벨 |
|---|---|---|---|
| C1 생성: `OS_CountSemCreate`(`sem_init` + 이름 테이블 등록) | 0 syscalls (OSAL 자체 시험, probe) | 테이블 주소를 SR로 지정하면 memory write. 아니면 입력 없음 | 분석 |
| C1 조회: `OS_CountSemGetIdByName` | 0 syscalls (probe) | 위와 같다. memory read | 분석 |
| C1 수정 후 재시도 대기: `OS_TaskDelay(100)` × 25 | sleep 계열 syscall(추정) | 모델링된 syscall에 드는지 목록이 없다 | 불명 |
| C3 SB `AppId` 저장, EVS `EVS_AppID`·등록 | syscall 없음 | SR이면 memory write | 분석 |
| C3 TIME의 `CreatePipe` | `mq_open`(probe에서 부분 가시) | 모델링 여부 서술 없음 | 불명 |
| C5 `OS_TaskCreate` | `pthread_create`. 에이전트가 쓴 작은 C 프로그램에서 `clone3`로 관측(`availability.md` §4 끝, 기록). OSAL·cFS 측정은 아니다 | `clone`은 HB 노드이자 `/proc` 쓰기. 부모의 `clone` 이전 사건만 자식과 HB | 분석 |
| C5 ES 레코드 쓰기·`GetAppID` 조회 | 0 syscalls(OSAL 시험의 task 레지스트리 대기는 futex 대기만 보임) | SR이면 memory write와 read | 분석 |

### 3.6 이 절이 말하지 않는 것

- 어느 셀도 "SimRacer가 사례를 탐지하지 못한다"는 뜻이 아니다. 실행하지 않았고, 사례의 접근이 SR로 지정될 수 있는지는 자료로 정해지지 않는다.
- "SimRacer는 메모리를 관측하지 못한다"는 문장은 근거가 없다. SIMEXPLORER는 shared memory를 SR로 두고 메모리 접근을 중단점으로 잡는다(p.8, p.14). SimRacer 자체의 메모리 관측은 정해지지 않았다. 2026-10-16에 Q1·Q2를 "불명"으로 바꾼 것은 이 미정 상태를 라벨에 반영한 것이며, "관측하지 못한다"는 뜻이 아니다. 반대로 "SimRacer는 메모리를 SR로 관측한다"도 쓰지 않는다.
- 이 절의 "분석"은 도구의 원래 설계에 대한 것이다. cFS 내부 상태를 SR로 지정하는 입력을 연구 쪽에서 추가하면 EXTENSION이며 원 도구의 결과가 아니다(로드맵 §10.4 끝, `availability.md` §5.1).
- 이 절의 도구 서술은 STVR의 SIMEXPLORER이다. ISSTA'13의 SimRacer와 같다고 확인된 것은 §2.3에 적은 범위뿐이다.

---

## 4. 실행 가능성과 결과 상태

### 4.1 요구 조건과 오늘 확인한 상태

| 요구 | 레퍼런스(등급) | 값의 상태 | 오늘 상태 |
|---|---|---|---|
| **도구 구현**(소스, 바이너리, 스크립트) | 없음. STVR §4는 구현을 설명하며 다른 연구자가 재현할 수 있게 하려는 것이라고 적는다(p.24) | 정책상 도구 레퍼런스(README, artifact 절)가 없다 | **없음** [M]. GitHub 저장소 검색 34건 중 해당 없음, 코드 검색 1건(BibTeX 색인). 저자 홈, CV, UConn 홈에 소프트웨어 항목이 없다(§1.1 경로 8, 9, 14, 15). 2026-10-16에 다시 찾았으나 없다(경로 33, 36, 37) |
| 연구 자료(objects of analysis 등) | STVR p.15 각주 §: "The objects of analysis and other artifacts used in this study can be obtained by contacting the first author." `[전문]` | 저자 연락 필요 | 연락하지 않았다(`availability.md` A2). "other artifacts"가 도구 코드를 포함하는지는 서술이 없다 |
| **Simics** | ISSTA 초록 "commercial virtual platform" `[초록]`; STVR p.2, p.14 `[전문]`; SimEvo p.7 `[2차]` | **버전, 시뮬레이션 머신, 스크립트: `unspecified_by_reference`** | 이 컨테이너에 없다 [M 2026-10-15]: `simics`·`simics-gui`가 PATH에 없고 `/opt/*simics*`도 없다. 설치하지 않았다. 벤더 공개 페이지(오늘 200, 페이지에 "Updated 9/8/2023")는 Intel Simics 공개 릴리스를 커뮤니티에 무료로 제공하고 사용 전 EULA 동의를 요구한다고 적는다. 다운로드 약 2 GB, 설치 약 10 GB, host는 64-bit Linux(권장 최소 RHEL 8 동등, glibc ≥ 2.22) 또는 64-bit Windows다. 이 컨테이너의 `/` 여유는 16 GB다 [M]. 이 제품·버전이 SimRacer(2013)가 쓴 Simics와 같은지는 서술이 없다. EULA 동의는 PI 결정 항목이다. **2026-10-16 재측정** [M]: `simics`·`simics-gui`가 PATH에 없고 `/opt/*simics*`가 없으며 `/dev/kvm`도 없다. 커널 `6.18.44-fc-v139`. `/`의 여유는 2.0 GB로 줄었다(크기 252 GB, 사용 95%). 벤더 페이지가 적는 설치 크기(약 10 GB)보다 작다. 설치는 여전히 하지 않았다 |
| Guest 환경 | STVR p.18: Simics로 단일 코어 x86 Linux 2.6.15와 3.5.0 가상화. p.15: UART 연구는 Fedora Core 2.6.15, Ubuntu 12.04 | 논문 기술(정책상 레퍼런스인지는 PI 판단, §4.3 P-1) | cFS(6.4.x 또는 7.0.1)를 그 guest에 올리는 조건은 `unspecified_by_reference`다. 현재 기준 바이너리 `core-cpu1`은 x86-64 ELF이고 커널 ≥ 3.2.0을 요구한다(`baselines/availability.md` A4 기록) |
| 제어용 장치 | dummy device와 guest 안의 driver 프로그램(p.14) | 논문 기술. 세부(kernel module 여부 등) 서술 없음 | 구현 없음 |
| Python | Simics tool을 Python으로 작성(p.14) | 버전 서술 없음 | — |
| 실패 판정 | 출력 기반 oracle. 두 process를 순차 실행해 얻은 출력과 비교(p.17–18) | 사용자가 작성 | cFS용은 연구 쪽 신규 입력 |
| host 하드웨어 | 서술 없음 | `unspecified_by_reference` | — |

### 4.2 결과 상태 (로드맵 §10.4)

오늘 적용되는 상태는 **다섯 사례 모두 `NOT_RUN`**이다.

| 사례 | 상태 | 이유 |
|---|---|---|
| C1 | `NOT_RUN` | 자료·환경: 도구 구현이 없고(R1), Simics와 원 구성이 갖춰져 있지 않다 |
| C2 | `NOT_RUN` | 위와 같고, 원 앱이 비공개다. 가능한 것은 역할 재구성뿐이다 |
| C3 | `NOT_RUN` | 위와 같다 |
| C5 | `NOT_RUN` | 위와 같다 |
| R2 | `NOT_RUN` | 위와 같다 |

- `INAPPLICABLE`을 붙이지 않는다. STVR의 대상은 process, signal, interrupt, kernel thread이고(p.7) 같은 process 안의 thread는 "lightweight process"라 추적할 수 있다고 한 줄만 있다(p.13). 입력은 PuT 쌍(basic program과 짝 program 또는 handler, p.16)이다. cFS task가 이 입력 단위에 해당하는지는 서술이 없고, 이를 "명시된 입력·대상 범위"의 제외로 볼지는 PI 판단이다. 2026-10-16에 읽은 Sim-O/C(ITJ p.185 `[2차]`)는 Simics 콜백만으로는 process와 thread를 구별하지 못하고 process tracker가 user-space process를 구별한다고 적는다. thread 단위 구별 방법은 그 쪽에 없다. 이 서술은 판단 자료를 더할 뿐 `INAPPLICABLE`의 근거로 쓰지 않았다.
- `PATH_NOT_EXECUTED`, `OBSERVATION_GAP`, `ANALYSIS_GAP`, `REPORTED`는 도구를 실행한 뒤에만 쓸 수 있다. 이 문서는 어느 것도 예측하지 않는다. 로드맵이 말하듯 사건 기록의 기준이 부족한 경우를 `ANALYSIS_GAP`으로 서둘러 분류하지 않는다.
- 이 문서의 §3 내용은 라벨 `ANALYSIS`(observation-model analysis, not run)이고 등급은 SIMEXPLORER `[전문]` + SimRacer `[초록]` + Sim-O/C·SimTester `[2차]`이다. 원 도구 결과가 아니다.

### 4.3 PI 판단이 필요한 항목 (결정 후보. 여기서 정하지 않았다)

| # | 항목 | 선택지 | 기본값 |
|---|---|---|---|
| P-1 | STVR p.18의 guest 값(Linux 2.6.15, 3.5.0, 단일 코어 x86)과 Simics를 `CONDITIONS_POLICY`의 레퍼런스(`from-reference`)로 볼 것인가 | 정책의 레퍼런스 목록은 "도구의 공식 문서: README, 논문의 artifact 절"이다. p.18은 실험 서술이다 | 판단 전까지 `unspecified_by_reference` 유지, 논문 서술로 병기 |
| P-2 | 한 process 안의 task 쌍을 PuT 쌍으로 줄 수 없다고 볼 근거가 서술에 없다. 이를 명시된 대상 범위 제외(`INAPPLICABLE`)로 볼 것인가 | 예 / 아니오 | 아니오(`NOT_RUN` 유지). 논문이 thread를 추적 가능하다고 적고, PI의 Process-Level 정의가 앱·task 단위라서 |
| P-3 | 저자에게 도구 코드와 연구 자료를 요청할 것인가(`availability.md` A2) | 연락 / 연락 안 함, 그리고 누구 명의로 | 연락 안 함. 도구는 `material_unavailable`로 둔다 |
| P-4 | Intel Simics 공개 릴리스를 설치할 것인가(EULA 동의, 약 10 GB). 설치하더라도 도구 코드가 없어 그것만으로 SimRacer 실행은 되지 않는다 | 설치 / 안 함 | 설치 안 함 |
| P-5 | STVR §3–§4의 알고리즘(Figure 10, 12, 13, HB 규칙, 관측·제어 구현)을 근거로 **REIMPLEMENTATION**을 만들 것인가(`availability.md` §5.1, A9). 이것은 SimRacer 원 도구의 결과가 아니고 SIMEXPLORER 서술을 기준으로 하므로 ISSTA'13과도 다를 수 있다. Simics 대신 다른 관측·제어 수단을 쓰면 `deviation`이다 | 만든다 / 안 만든다 | 만들지 않는다 |
| P-6 | SimRacer와 SIMEXPLORER를 비교표에서 한 계보의 두 판본으로 묶을 것인가 | 묶는다 / 따로 둔다 | 묶고 판본을 표시한다. 같은 결과를 두 번 세지 않는다 |

---

## 5. 다섯 사례에서 부족한 것과 Kairos 기능 가설

### 5.1 SimRacer 계열이 이미 하는 것 — 차별점으로 주장하지 않는다

로드맵 §10.5는 "이미 가능한 기능은 재발명한 차별점으로 주장하지 않는다"고 한다. 아래는 STVR 전문에서 확인한 기존 기능이다.

| 기능 | 근거 |
|---|---|
| 기록된 실행에서 **실행되지 않은 반대 순서를 예측**하고(Phase 1) 순서를 뒤집어 **실제 실행으로 확인**(Phase 2) | p.7, p.10–11. SimRacer: 초록 `[초록]` |
| 공유 메모리 접근을 SR로 두고 load/store 사건을 기록 | p.8, p.12, p.14. **SIMEXPLORER에만 확인된다.** SimRacer 자체에 있었는지는 [U]라서(§2.3) 이 행은 SimRacer의 기존 기능 목록에서 뺀다(2026-10-16, `process_race_method_matrix.md` §4.3과 같은 처리) |
| 개발자가 SR과 syscall을 추가 지정하는 인터페이스 | p.8 (입도는 서술 없음). SIMEXPLORER의 서술이며 SimRacer에 있었는지는 서술이 없다 |
| system call 쌍의 순서를 바꿔 실행으로 확인하는 process 사이 시험 | ITJ p.188 `[2차]`(같은 해 같은 저자의 Sim-O/C 서술). 초록의 "controls process scheduling"과 같은 방향이다 |
| 거짓 양성을 Phase 2의 실제 실행으로 걸러 냄 | p.10–11, p.18 |
| race와 oracle이 보는 fault를 나눠 보고 | p.19, p.22 |
| 같은 명령어 위치의 반복 접근을 한 쌍으로 합침 | p.10, p.13 |
| 순서위반 계열(두 접근 사이의 순서)을 겨냥한 설계 | p.3, p.14 |

따라서 로드맵 §10.5 표의 "관측된 실행만 다룸 → 예측·탐색 및 실행 확인" 행은 SimRacer 계열에 근거가 없다. 이 행을 SimRacer와의 차이로 쓰려면 앵커의 종류(두 접근 사이의 쌍, syscall 의미로 추론한 HB)에 한정해서 써야 하며, 예측·확인 자체를 차별점으로 쓸 수 없다.

### 5.2 가설 (근거가 있는 만큼만)

모든 가설은 **분석**(설계 기술에서 도출, 미실행)이고 연구 가설이다. 어느 것도 "SimRacer 계열이 못 한다"는 결과가 아니다. 근거 칸의 p.는 STVR의 쪽이다. 이 문서의 대상 도구는 SIMEXPLORER로 확인된 서술이며 SimRacer 자체와의 일치는 §2.3의 범위 안에서만 말할 수 있다.

| 가설 | 대상 사례 | 관찰된 부족 (분석) | 근거 | 연결되는 Kairos 기능 후보 (로드맵 §10.5) | 한계·반증 가능성 |
|---|---|---|---|---|---|
| **H-SM-1** cFS 내부 상태가 기본 SR 목록에 없고, SR이 되는 경로가 서술에 없다 (2026-10-16 보정: SimRacer 자체에는 SR에 메모리가 있었는지부터 [U]) | R2, C3, C5, C1 | SIMEXPLORER의 기본 SR은 shared memory, fd, inode, file data, `/proc`, hardware다. AppId, 등록 플래그, `TaskTable`, OSAL 이름 테이블이 어떤 경로로 SR이 되는지는 서술이 없고, 지정 인터페이스의 입도도 알 수 없다. 공개·조회가 syscall이 아니라는 측정(OSAL 자체 시험의 probe)은 syscall 중심 관측에만 관계된다 | p.8, p.14; `availability.md` §4.2 `[기록]` | cFS 내부 API·상태 변화·값 전달을 관측하고 소스와 연결 | **SIMEXPLORER에서는 메모리 접근이 설계상 관측되므로 "관측 불가능"이 아니다.** 지정 인터페이스로 메울 수 있다면 부족은 입력 작성 비용이다. SimRacer 자체에 대해서는 같은 해 서술(ITJ p.188 `[2차]`)이 process 사이 사건을 system call로 적지만 메모리 관측을 배제하지 않으므로, 이 가설을 "SimRacer가 메모리를 보지 못한다"로 강하게 쓰지 않는다. 이 가설은 "Kairos만 가능하다"가 아니라 "원 도구의 기본 입력 정의 밖에 있다"는 뜻이다. 실행 미확인 |
| **H-SM-2** 등록 상태와 lock 범위가 후보 모델에 없다 | C1, C5 (R2는 값 0 vs 유효값) | 잠재 쌍은 "같은 SR, 하나 이상 쓰기, HB 없음"뿐이다. 쓰기가 예약인지 공개 완료인지, lock 안인지를 구분하는 개념이 없다. lock과 semaphore는 모델 밖이다. 이름 → ID → 슬롯의 대응과 세대도 서술이 없다 | p.9, p.10; 키워드 검색(generation 0건) | 이름과 객체 ID·세대, task와 AppId의 관계 추적 | **근거가 약하다.** 근거는 "서술 없음"이고, 이 논문의 목적이 아닌 영역이다. Phase 2의 실제 실행이 불가능한 쌍을 걸러 낸다(p.11, p.13). C1의 RESERVED 구간은 코드 읽기 가설이며 미확인이다(`ledger.md` §2) |
| **H-SM-3** 오류 반환, 재시도, 최종 결과가 입력과 출력의 항목이 아니다 | C1, C5, R2 | oracle은 출력 기반이고 내부 반환값을 보는 oracle은 비용이 든다고만 적는다. 재시도 서술은 없다. 출력에 "회복 vs 최종 실패"가 없다 | p.18; 키워드 "retry" 0건 | 반환값·재시도·초기화 최종 결과 추적 | 사용자가 내부 oracle을 쓰면 구분할 수 있다(p.18이 가능성을 인정). "도구 안에 입력 형식이 없다"에 가깝다 |
| **H-SM-4** 순서가 고정된 이유(사용자 수준 직렬화)가 출력에 없다 | R2 (C3의 core 시작 순서도 같은 성격) | 뒤집을 수 없는 쌍은 race로 세지 않고 이유를 출력한다는 서술이 없다. HB는 syscall·signal·interrupt 활성에서만 나온다(p.9). Phase 1은 모든 동기화 연산을 식별하지 못한다고 적고(p.10), 사용자 정의 동기화를 인식하지 못할 수 있다는 서술은 HB 기반 기법 일반에 대한 것이다(p.2) | p.2, p.9, p.10, p.11 | 소스 기반 순서 근거·전제·신뢰 범위를 결과에 연결 | 실행하지 않았다. R2의 순서가 스케줄로 뒤집힐 수 있는지(race vs 결정적)가 사례 기록상 미결이다. 이유를 출력하지 않는다는 것은 서술의 부재 확인이다. 2026-10-16 보충: §3.5 R2 Q5 칸의 도출([I])대로 뒤집기가 교착 해소 뒤 실패하면 그 쌍은 race로 세지 않으므로, 이 경우 도구 출력에는 그 쌍이 나타나지 않을 수 있다. 이것도 실행 결과가 아니다 |

### 5.3 근거가 부족해 가설로 올리지 않는 것

- **C2 (시작 동기화와 앱 고유 초기화 완료의 혼동, 로드맵 §10.5 3행).** P·R이 의존에 쓰는 자원이 비공개라 SimRacer 계열이 그 관계를 표현할 수 있는지 판단할 근거가 없다. 논문에 startup·readiness 상태 개념이 없다는 키워드 확인(0건)만 있다. 역할 재구성 실험(결정 P-C2)에서 선택한 요청 방식이 정해진 뒤에야 같은 분석을 할 수 있다.
- **"SimRacer 계열은 메모리를 관측하지 못한다."** SIMEXPLORER에는 STVR p.8, p.12, p.14가 반대다. SimRacer 자체에는 정해지지 않는다(2026-10-16의 Sim-O/C 서술도 배제 문장이 아니다). 어느 쪽으로도 쓰지 않는다.
- **R2와 C3의 통신 경로(`mq_open`).** 모델링된 23개 syscall의 전체 목록이 없어 SB pipe 통신이 도구에 보이는지 판단할 수 없다. "SB pipe 통신이 도구에 보이지 않는다"고 쓰지 않는다.
- **"한 process 안의 task라서 대상이 아니다."** 이를 대상 범위 제외로 읽을지는 PI 판단(§4.3 P-2)이고 논문은 thread를 추적 가능하다고만 적는다.

### 5.4 비교 논문에서 먼저 밝혀야 할 정의 차이

[F] SimRacer·SIMEXPLORER의 process-level race는 "two system events (eg, processes, software signals, and hardware interrupts) access a shared resource" 그리고 "in an order different from the intended order"로 정의된다(p.3). 이것은 data race보다 넓고 data race의 필요조건이라고 적는다. [D] 이 연구의 Process-Level은 cFS 애플리케이션·task 단위의 운용상 정의이며 OS process로 한정하지 않는다(로드맵 §1.2, T1과 같은 결정 묶음). 두 정의가 달라서 native cFS에서는 대상이 한 process 안의 thread다. 비교 서술은 이 차이를 처음에 밝히고, 이 차이 때문에 생기는 결과를 도구의 결함처럼 쓰지 않는다.

[I] Kairos가 판정하는 대상은 readiness specification이다. SimRacer 계열의 출력은 잠재 쌍, real race, oracle이 보는 fault다. 두 출력은 같은 점수로 합치지 않는다(로드맵 §1.4). 평가 규모(STVR 24개, SimRacer 16개 프로그램)와 저자가 보고한 탐지 수치는 이 비교에 쓰지 않는다. 사례도 다르고 도구를 실행하지 않았기 때문이다.

---

## 6. 확인하지 못한 것과 다음에 할 일

| # | 미확인 | 이유·다음 단계 |
|---|---|---|
| 1 | ISSTA'13 SimRacer 전문. 그 알고리즘이 STVR의 Phase 1·2와 같은지, SR 목록에 메모리가 있었는지, 평가 규모 | 원문 호스트 접근 실패(§1.1 경로 1–3, 11, 12). 방법: 저자 연락(P-3), 기관 도서관 경유, 다른 사본. P-PAPERS는 사용자 PDF 제공을 착수 조건으로 만들지 않는다 |
| 2 | 학위논문(Yu 2014) 본문. SimRacer 장이 있을 가능성이 있으나 초록은 "concurrency faults caused by improper shared resource accesses among multiple processes, and software signals"까지만 보여 준다 | PDF가 Cloudflare 도전에서 막혔다. 우회하지 않았다 |
| 3 | SR 지정 인터페이스의 입도(메모리 주소를 SR로 줄 수 있는지)와, 메모리 SR이 자동으로 정해지는지 | STVR p.8은 한 문장이다. 메모리 SR을 정하는 방식은 서술이 없다(p.16의 "statically identified SRs"는 test 증강 맥락이다) |
| 4 | 한 process 안의 task 쌍을 PuT 쌍으로 줄 수 있는지, Phase 1의 HB 규칙 ①이 같은 process의 thread 사이에도 적용되는지 | p.13의 한 줄(thread는 lightweight process)뿐이다 |
| 5 | 모델링된 23개 syscall의 전체 목록(sleep 계열, `mq_*`, `futex` 포함 여부) | 본문은 예시만 든다. 본문 키워드 "futex", "mq_", "message queue", "mutex", "pthread" 0건 |
| 6 | SimRacer의 탐색 방식: SimEvo p.8의 "randomly explores"와 STVR의 결정적 뒤집기의 관계 | §2.3. 두 서술이 어긋나는지 같은 것의 다른 표현인지 정해지지 않는다 |
| 7 | SCMiner p.11의 "interleavings of system calls"와 STVR의 SR 목록(shared memory, hardware 포함)의 관계 | §2.3. 층위가 다른 서술이다 |
| 8 | Simics 버전, 머신 모델, 스크립트, guest 이미지, driver, host | 읽은 자료에 없다. `unspecified_by_reference` |
| 9 | STVR 표 1–3(객체 프로그램, 효과, 효율)의 수치 | 텍스트 추출이 어지러워 읽지 않았다. 이 문서의 질문과 직접 관련이 없다 |
| 10 | SimEvo, RRF, SysPro의 나머지 본문 | 이 작업은 SimRacer의 서술을 찾는 데 필요한 부분만 읽었다. 각 도구의 문서에서 따로 다룬다 |
| 11 | `baselines/yu-group.md`와 `baselines/availability.md`의 갱신 필요 칸(§1.3 항목 2–6) | 이 작업의 소유 파일이 아니라 수정하지 않았다. 담당 작업에서 반영해야 한다. 특히 "shared-resource 사건이 아니다"(§1.3 항목 3)와 SIMEXPLORER 행의 "Unknown" |
| 12 | 이 문서의 §3을 `process_race_method_matrix.md`(로드맵 §12 작업 6)에 옮길 때 SimRacer와 SIMEXPLORER를 한 계보로 표시할지 | P-6 |
| 13 | (2026-10-16) `process_race_method_matrix.md`의 SimRacer 행 갱신: §2.2 라벨 표(C1·C3·C5·R2의 Q1·Q2가 불명으로 바뀜), §2.3 합계(SimRacer 사례별 25칸이 분석 12·불명 13), §2.4.2 칸, §2.5 항목 1(C1·C5 Q2 불일치는 SimRacer 쪽이 불명이 되어 해소), §6 R2 사건표의 SimRacer 행(⓪ 행 추가, ①② SimRacer 자체 불명) | 그 문서는 이 작업의 소유가 아니다. 담당 작업에서 반영해야 한다 |
| 14 | (2026-10-16) R2 사건표 ⓪의 전제: R2의 사용 task가 생성자 thread(ES 시작 thread)가 아니라는 것 | 사례 기록 R2.E3에 기댄다. 코드로 다시 확인하지 않았다. 이 전제가 틀리면 fork 규칙 아래에서 그 쌍은 후보가 아니다 |
| 15 | (2026-10-16) Sim-O/C(ITJ 2013)와 SimRacer(ISSTA'13)가 같은 도구의 서술인지 | 같은 저자, 같은 해, 같은 Simics 기반 제어(dummy device, `task_struct`)이지만 Sim-O/C 본문에 SimRacer라는 이름이 없다. [I]로만 쓴다 |
| 16 | (2026-10-16) Yu의 2016년 UNL 홈 스냅샷(Wayback) 안에 ISSTA'13 PDF 링크가 있는지 | 이 작업 공간에서 web.archive.org를 열 수 없었다(경로 34). 다른 환경에서 확인할 수 있다 |

---

## 7. 출처 목록

- STVR 2017(`[전문]`): T. Yu, W. Srisa-an, G. Rothermel, *An automated framework to support testing for process-level race conditions*, Softw. Test. Verif. Reliab. 27(4–5), e1634, 2017. DOI 10.1002/stvr.1634. 사본: `https://archive.cps-vo.org/node/36387`(레코드), `https://archive.cps-vo.org/node/36388`(파일 `Yu_SimExplorer_JA.pdf`). sha256 `af805231f69ffc0e12d96fe4e4e9a356932653c56c382fdd3debd211bcdd3e08`, 26쪽.
- Sim-O/C(`[2차]`, 2026-10-16 추가): T. Yu, W. Srisa-an, G. Rothermel, *Sim-O/C: An Observable and Controllable Testing Framework for Elusive Faults*, Intel Technology Journal 17(2), 2013, pp. 178–197. `https://www.intel.com/content/dam/www/public/us/en/documents/research/2013-vol17-iss-2-intel-technology-journal.pdf`(호 전체 PDF). sha256 `24080f8ec175aecb7e6c93fd0a436dd8532c856375d8b356b772030dc8c1813c`.
- SimTester(`[2차]` 부분, 2026-10-16 추가): T. Yu, W. Srisa-an, G. Rothermel, *SimTester: a controllable and observable testing framework for embedded systems*, VEE 2012, pp. 51–61. `https://www.cl.cam.ac.uk/research/srg/netos/vee_2012/papers/p51.pdf`. sha256 `206c12cef8dc9f01fe2ae0b2d16dc2d2c719ce60f553fe75aa8627e3022b08cb`.
- SimRacer(`[초록]`): DOI 10.1145/2483760.2483771. 서지·초록: `https://api.openalex.org/works/doi:10.1145/2483760.2483771`, `https://api.crossref.org/works/10.1145/2483760.2483771`, Semantic Scholar Graph API(corpus ID 18130539). ISSTA 2013 프로그램: `https://issta2013.inf.usi.ch/program/`.
- SimEvo(`[2차]`): T. Yu, *SimEvo: Testing Evolving Multi-process Software Systems*, ICSME 2017, pp. 204–215. `https://par.nsf.gov/servlets/purl/10089644`. sha256 `143f7351489367d9301b97ac2317e3f1f263ac159ee737ffcff14bb0adc31be6`.
- DESCRY(`[2차]`): T. Yu, T. S. Zaman, C. Wang, ESEC/FSE 2017. `https://chaowang-vt.github.io/pubDOC/YuZW17.pdf`. sha256 `f9d7b57bbb0c1269f9143b4ec69286e90348a58fadd071f76f11022e06dfbf60`.
- RRF(`[2차]`): S. Rattanasuksun, T. Yu, W. Srisa-an, G. Rothermel, ISSRE 2016, pp. 162–172. `https://archive.cps-vo.org/node/30361`. sha256 `e8c26e2fbca15e87c9eb837c1bddc0ade0493fb261f3ebd2fbbc200c06f00dce`.
- SysPro(`[2차]`): *SysPro: Reproducing System-level Concurrency Bugs from Bug Reports*, arXiv 2601.09616v1. sha256 `8fe5daa4c7c3794715632e4379d9160a9852b723dc4a649317ef6bae0a75e726`.
- SCMiner preprint(`[2차]`): `github.com/tarannumzaman/Scminer` @`255933c62394e0d4fc0a9063975d36aca1cb15a9`, `Scminer_preprint.pdf`. sha256 `854f86c14bcffe8c446d9e8b84f347ace3697145dbc31a094e8dec4b37172099`.
- 학위논문 landing(`[초록]`): `https://digitalcommons.unl.edu/computerscidiss/77`, `https://digitalcommons.unl.edu/dissertations/AAI3626337/`.
- 저자 페이지: `https://homepages.uc.edu/~tyu/`(`links/research.html`, `links/projects.html`, `CV/tyu_cv.pdf`), `https://tingting-yu.scholar.uconn.edu`.
- Intel Simics 공개 릴리스(오늘의 벤더 문서, 컨텍스트): `https://www.intel.com/content/www/us/en/developer/articles/tool/simics-simulator.html`.
- 저장소 기록: `baselines/availability.md`, `baselines/yu-group.md`, `cases/ledger.md`, `plan/ROADMAP_20261015.md`(§1.2, §1.4, §2.2, §9, §10), `plan/methods/RacePro.md`(같은 형식), `Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md`, `CONDITIONS_POLICY.md`. 2026-10-16 추가: `cases/R2.md`(§6 S1·S2, §6.1, A7), `cases/C1.md`, `plan/case_basis_register.md`(R2.E3, R2.E5), `plan/process_race_method_matrix.md`(§2.5 항목 1, §4.3, A10), `plan/methods/SIMEXPLORER.md`(경로 14, 16, 17의 쪽 표기와 해시).
