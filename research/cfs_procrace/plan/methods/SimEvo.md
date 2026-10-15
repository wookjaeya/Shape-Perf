# SimEvo (ICSME 2017) 분석 — Kairos 비교용

- 작성일: 2026-10-15 (UTC 09:09–09:20, 경로 시도와 읽기 시각).
- 대상: T. Yu, *SimEvo: Testing Evolving Multi-Process Software Systems*, ICSME 2017, pp. 204–215. DOI 10.1109/ICSME.2017.29. 단독 저자, 당시 University of Kentucky.
- 범위: 로드맵 §10 (M6)의 SimEvo 항목(회귀 관련 분석). 여섯 분석 질문(§10.2)을 사례 C1, C2, C3, C5, R2에 적용한다. 로드맵 §12 작업 6(`process_race_method_matrix.md`)의 입력으로 쓴다.
- 이 문서의 성격: **분석 문서**다. SimEvo를 실행하지 않았다(실행할 구현이 없다). 어느 사례에 대해서도 "탐지하지 못한다"는 결과를 담지 않는다.
- 용어: 신규 문서이므로 T1 (a)를 따른다. 판정 대상은 readiness specification(준비 상태 명세), 그 전제는 readiness precondition, 위반은 readiness violation이다. "API contract"는 문서화된 API 의미를 가리킬 때만 쓴다.
- 조건 원칙: `CONDITIONS_POLICY.md`. 이 문서는 실험 조건을 정하지 않는다. 레퍼런스가 없는 값은 `unspecified_by_reference`로 둔다.
- PI 결정(`PI_DECISIONS_RESOLVED_20261015.md`)은 결정 `[D]`로만 인용하고 사실로 쓰지 않는다. 이 문서가 쓰는 결정은 P-PAPERS, T1, D22, P-R2CTRL이다.
- 같은 계보의 다른 문서: `plan/methods/SimRacer.md`, `plan/methods/SIMEXPLORER.md`, `plan/methods/RacePro.md`. SimEvo는 SimRacer 위에 구현됐다고 논문이 밝힌다(§2.3).

## 증거 태그와 근거 등급

| 태그 | 뜻 | 이 문서에서의 쓰임 |
|---|---|---|
| [F] | 인용이 있는 사실 | 원출처(논문 쪽·절) 또는 저장소 기록(문서·절). 저장소 기록은 "기록"으로 표시하며 이번에 재검증하지 않았다 |
| [M] | 측정 | 오늘(2026-10-15) 이 컨테이너에서 읽기 전용으로 확인한 값 |
| [I] | 추론·분석 | 설계 기술에서 도출한 해석. 도구를 실행하지 않았다 |
| [D] | 결정 | PI 결정 |
| [U] | 미확인 | 자료가 없거나 읽지 못함 |

문장마다 출처 등급을 아래 다섯 가지 중 하나로 붙인다.

| 등급 | 뜻 |
|---|---|
| `[전문]` | 본문 전체를 텍스트로 추출해 읽었다. SimEvo는 그림 1, 2, 3, 5를 이미지로도 확인했다. STVR은 본문 전체를 텍스트로 추출했고 p.2–3, p.7–14, p.24를 읽었다(§2·3·4·7.1에 해당) |
| `[초록]` | 초록·서지만 읽었다 |
| `[2차]` | 다른 논문·페이지가 이 도구를 서술한 문장, 또는 이 도구가 위임한 출처의 문장을 읽었다. 도구 자신의 서술이 아니다 |
| `[기록]` | 저장소의 이전 보고서에 있는 값. 이번에 재검증하지 않았다 |
| `[미도달]` | 열지 못했다 |

쪽 번호 표기:
- **SimEvo p.N**: PDF 파일의 쪽(12쪽 중 N). 인쇄 쪽은 N+203이다(PDF 1쪽 = 인쇄 204쪽). 형제 문서(SimRacer.md 등)와 같은 방식이다.
- **STVR p.N**: Yu, Srisa-an, Rothermel, STVR 27(4–5), e1634(2017)의 "N of 26". PDF 쪽과 같다.
- **RacePro §N**: Laadan et al., SOSP 2011의 절 번호.

---

## 0. 한눈에 보기

1. **SimEvo 논문 전문을 읽었다** `[전문]`. NSF Public Access Repository(PAR)의 PDF(12쪽, sha256 `143f7351…adc31be6`)다. 이전 기록(2026-10-08)은 이 논문을 열지 못해 검색 발췌만 갖고 있었다(§1.3). 논문 안에는 코드·artifact 위치에 관한 서술이 없다(키워드 검색: URL은 CodeSurfer 제품 페이지 하나뿐).
2. **역할은 회귀 시험(regression testing)이다.** 두 버전 S, S′의 소스와 기존 테스트 T, interleaving 커버리지 기준 PT를 받아 변경 영향 분석(정적·동적), 변경 유도 test input 생성(GA), 영향받은 interleaving의 예측·재생을 한다 `[전문]` p.4. 오라클이 확인하는 결함(F)을 출력하지만, 변경 영향 범위로 좁힌 회귀 시험이라는 점에서 범용 탐지 도구(RacePro, SimRacer 계열)나 보고·로그 기반 재현 도구(RRF, DESCRY, SysPro)와 입력과 목적이 다르다. 출력은 선택된 테스트 집합, 영향받은 Def-Use 쌍의 커버리지, 결함(F)이다 p.4.
3. **사건 모델은 SimEvo 본문에 직접 있다.** system call의 read/write 효과와 fork, wait, exit, pipe, signal 동기화 연산이다 p.1, p.3. 이전 기록(`baselines/yu-group.md` §4, §7.2)은 발췌만 보고 "시스템 호출 모델이 서술되지 않았다"고 적었다. 전문으로 이 부분은 정정된다(§1.3).
4. **메모리 접근·lock·반환값·재시도는 SimEvo 본문에 사건으로 나오지 않는다.** 키워드 검색에서 load, store, lock, mutex, semaphore, return value, retry가 0건이다. "memory"는 thread-level 버그와의 대비(p.1)와 참고문헌 제목에만 있다. 자원 모델의 세부는 RacePro [26]과 SimRacer [57]에 위임한다("Details of shared resource and event modeling can be found in prior work [26], [57]", p.3). 동적 CIA와 interleaving 탐색은 SimRacer 위에 구현했다 p.7. SimRacer 원문(ISSTA'13)은 오늘도 열리지 않았다. 그래서 SimEvo 구현이 메모리 접근을 관측하는지는 **불명**이다(§2.3).
5. **필요한 순서(readiness precondition)와 허용 오류는 입력이 아니다.** 입력은 S, S′, T, PT이고 p.4, 실패 판정은 순차 실행의 출력(콘솔 메시지·파일)과의 비교다 p.8. 논문은 "출력이 오류 없이 interleaving마다 바뀔 수 있는 문제"를 미해결로 남겼다 p.8.
6. **다섯 사례의 의존성은 SimEvo가 서술한 사건 집합에 들어 있지 않다 (분석, 미실행).** C1·C3·C5·R2의 공개와 사용은 메모리 쓰기·읽기와 함수 호출이고, 사건 모델의 다섯 순서 관계(program, fork-return, wait-exit, pipe-read, signal; p.3)에 readiness 관계가 없다. 이것은 "탐지 실패"가 아니라 입력 정의가 정해지지 않은 상태다. C2는 대상 앱이 비공개라 대부분 칸이 불명이다.
7. **실행은 불가능하고 오늘 적용되는 상태는 다섯 사례 모두 `NOT_RUN`이다.** 구현을 찾지 못했고, 동적 단계의 기반인 SimRacer의 구현도 찾지 못했다. Simics는 이 컨테이너에 없고 `/dev/kvm`도 없다. 논문이 Simics 버전·시뮬레이션 머신·guest OS를 적지 않아 그 선택은 `unspecified_by_reference`다. static CIA에 쓴 CodeSurfer(상용)의 현재 가용성은 불명이다. `INAPPLICABLE`은 PI 판단이다(§4.3).
8. **Kairos 가설은 근거가 있는 만큼만 올렸다(§5).** 변경 영향 분석, 관측 trace에서 실행되지 않은 순서를 offline으로 만들고 재생해 확인하는 구조는 SimEvo에 이미 있다. 차별점으로 쓰지 않는다. "SimEvo가 메모리를 못 본다"는 가설도 근거가 반대 방향이라 올리지 않았다.

---

## 1. 접근한 출처와 문장별 근거 등급

### 1.1 시도한 경로와 결과

시각은 2026-10-15 UTC 09:09–09:20이다. 원문 경로를 먼저 시도했다(P-PAPERS `[D]`). 접속 실패는 원인을 기록한 것까지만 썼다. TLS 검증을 끄거나 프록시를 우회하지 않았고, 도전(challenge) 페이지를 우회하려고 하지 않았다. "[M]"은 오늘 측정한 응답이다.

**A. SimEvo 논문과 서지**

| # | 경로 | 결과 | 등급·비고 |
|---|---|---|---|
| 1 | `https://par.nsf.gov/servlets/purl/10089644` | HTTP 200, `application/pdf`, 323,090 B, 12쪽. sha256 `143f7351489367d9301b97ac2317e3f1f263ac159ee737ffcff14bb0adc31be6` [M]. 제목·부제(ICSME 2017) 메타데이터가 논문과 일치 | **`[전문]`** |
| 2 | `https://par.nsf.gov/biblio/10089644` | 200. 서지(Award ID 1652149, 2017-09-01, 쪽 204–215)뿐. 코드·데이터 링크 없음 | 서지 |
| 3 | `https://doi.org/10.1109/ICSME.2017.29` → IEEE Xplore `document/8094422` | 302로 이동 후 HTTP 202, 본문 0 B, 응답 헤더 `x-amzn-waf-action: challenge` | `[미도달]` |
| 4 | `https://ieeexplore.ieee.org/abstract/document/8094422/` | HTTP 418 | `[미도달]` |
| 5 | OpenAlex `api.openalex.org/works/doi:10.1109/ICSME.2017.29` | 200. 초록, 참고문헌 73건, 피인용 3건. `is_oa: false`, 사본 위치 없음 | `[초록]` |
| 6 | Semantic Scholar Graph API (같은 DOI) | 200. DBLP 키 `conf/icsm/Yu17`. `openAccessPdf.status: CLOSED`, 초록은 출판사 요청으로 생략 | 서지 |
| 7 | Crossref `api.crossref.org/works/10.1109/ICSME.2017.29` | 200. 쪽 204–215, 참고문헌 58건. 초록 없음. 전문 링크는 IEEE뿐 | 서지 |
| 8 | Unpaywall API (같은 DOI) | 200. `is_oa: false`, `oa_locations: []` | 사본 없음 |
| 9 | `https://scholars.uky.edu/en/publications/simevo-testing-evolving-multi-process-software-systems/` | 200. 초록, 학회(Shanghai, 9/19–9/22/17), 후원(NSF CCF-1464032, CCF-1652149), Scopus 링크 | 서지. artifact 링크 없음 |
| 10 | DBLP `dblp.org/rec/conf/icsm/Yu17.xml`, `.html` | HTTP 202 (약 7 KB 도전 페이지, 내용을 쓰지 않았다) | `[미도달]` |
| 11 | ResearchGate의 SimEvo 레코드 | HTTP 403 | `[미도달]` |

**B. artifact(코드·데이터·VM 이미지)를 찾는 경로**

| # | 경로 | 결과 |
|---|---|---|
| 12 | 저자 홈 `homepages.uc.edu/~tyu/`, `links/research.html`, `links/projects.html`, `CV/tyu_cv.pdf` | 모두 200. SimEvo는 논문 목록에 IEEE Xplore 링크 하나로만 나온다. 프로젝트 페이지는 SimEvo(ICSME 2017)를 "Testing Evolving Complex Software Systems" 과제(NSF CCF-1652149)에 묶는다. 소프트웨어·도구 공개 항목이 없다. CV에서도 SimEvo는 논문 항목(C[20])으로만 나오고 소프트웨어·artifact 항목을 키워드 검색으로 찾지 못했다 |
| 13 | 새 홈 `tingting-yu.scholar.uconn.edu/`, `/research/`, `/publications/` | 200. 받은 HTML에 SimEvo, SimRacer, SimExplorer, Simics 키워드가 없다 |
| 14 | `www.cs.uky.edu/~tyu/` | HTTP 403 |
| 15 | NSF Award API `api.nsf.gov/services/v1/awards/1652149.json`, `…/1464032.json` | 200. 과제 초록은 "develop, evaluate, and make available a family of techniques and tools"라고 적는다(과제 계획 서술이다. 공개 위치는 없다). 출판 목록에도 코드 링크가 없다 |
| 16 | GitHub 저장소 검색 `SimEvo` | 16건. 전부 무관(인공생명·진화 시뮬레이션 게임, 회사 저장소 등) |
| 17 | GitHub 코드 검색 `"SimEvo" Simics`, `SimRacer Simics "process-level"` | 각각 1건. 도메인 목록 파일과 BibTeX 색인 HTML뿐(무관) |
| 18 | GitHub 저장소 검색 `process-level race Simics system-level concurrency regression testing`, 사용자 검색 | 0건 |
| 19 | arXiv API `all:SimEvo` 등 | 0건 |
| 20 | Zenodo API `q=SimEvo` | 0건. `q="process-level" race Simics`는 11,001건이지만 상위 10건이 모두 무관 |
| 21 | figshare API 검색(POST) | 빈 결과 `[]` |
| 22 | Software Heritage 저장소 이름 검색 `SimEvo` | 상위 10건, 무관(simevo 회사 저장소, 진화 시뮬레이션 등) |
| 23 | WebSearch 3회(논문 제목, 도구 이름, "artifact available github") | 코드 위치를 돌려주지 않았다. 논문 사본은 par.nsf.gov와 scholars.uky.edu만 나왔다 |
| 24 | Scholar Gateway `semanticSearch` | `ACCESS_DENIED` |
| 25 | OpenAlex 피인용 목록, Semantic Scholar 인용 목록, Wayback 가용성 조회 | HTTP 429(공유 일일 한도 소진). 피인용 3건의 제목은 확인하지 못했다 |
| 26 | `noggin.intel.com`의 "Sim-OC" 페이지(저자 프로젝트 페이지가 링크) | 프록시 `CONNECT … 502`. 열지 못했다. SimEvo와의 관계는 불명 |

**C. SimEvo가 위임·의존한 출처**

| # | 경로 | 결과 | 등급·비고 |
|---|---|---|---|
| 27 | SimRacer, ISSTA'13 (`dl.acm.org/doi/10.1145/2483760.2483771`, `doi.org`) | HTTP 403, 헤더 `cf-mitigated: challenge` | `[미도달]` |
| 28 | STVR 2017, SIMEXPLORER (`onlinelibrary.wiley.com/doi/abs/10.1002/stvr.1634`) | HTTP 403, `cf-mitigated: challenge` | `[미도달]` |
| 29 | 같은 STVR 논문의 CPS-VO 사본. `archive.cps-vo.org/node/36388`을 `curl`로 요청 | 245 B "Request Rejected"(호스트 WAF). WebFetch로 같은 페이지를 열어 파일 링크 `/file/36388/download/194033`과 "Creative Commons 2.5" 표기를 얻었다. 그 파일 URL을 WebFetch로 열자 PDF 2.3 MB가 저장됐다. 26쪽, sha256 `af805231f69ffc0e12d96fe4e4e9a356932653c56c382fdd3debd211bcdd3e08` [M] | **`[전문]`**. 그러나 SimEvo에 대해서는 `[2차]`(계보 논문) |
| 30 | RacePro, SOSP 2011 (`www.cs.columbia.edu/~orenl/papers/sosp2011-racepro.pdf`) | HTTP 200, 15쪽, 1.74 MB. sha256 `645a21ba1dc3e4d7f3930294cdbee29429c3f9f6bd43decc0112fa8e133fd64e` [M] | 이 문서에는 자원 모델 쪽(Table 2, §5.1–5.2)만 읽어 썼다. SimEvo에 대해서는 `[2차]` |
| 31 | 학위논문(Yu 2014, SimExplorer) UNL `digitalcommons.unl.edu/computerscidiss/77/` | 200. 초록 페이지. PDF는 시도하지 않았다 | `[초록]` |

**D. 실행 환경 쪽 출처**

| # | 경로 | 결과 | 등급·비고 |
|---|---|---|---|
| 32 | Intel Simics Simulator 공개 배포 페이지 `intel.com/…/simics-simulator.html` | HTTP 200. WebFetch의 요약이 돌아왔다(페이지 "Updated 9/8/2023"). 원문을 직접 대조하지 않았다 | `[2차]`(요약) |
| 33 | GrammaTech `www.grammatech.com/`, 논문이 인용한 `…/products/codesurfer/overview.html` | 루트 200, 인용 URL은 404 [M]. WebSearch는 오래된 제품 소개만 돌려줬다 | 현재 상태 불명 |
| 34 | 컨테이너 읽기 전용 확인 | 커널 `6.18.44-fc-v114`, `/dev/kvm` 없음, `/proc/cpuinfo`의 vmx/svm 0, `simics`·`qemu-system-x86_64`·`codesurfer` 실행 파일 없음, `/` 여유 16 GB [M] | `[M]` |

### 1.2 이 문서가 쓰는 출처별 등급

| 출처 | 등급 | 무엇에 썼나 |
|---|---|---|
| SimEvo 본문(PAR PDF) | `[전문]` | §2의 도구 서술 전부, §3의 "도구 수준의 답" 중 SimEvo 쪽 |
| STVR 2017(SIMEXPLORER) | SimEvo에는 `[2차]` | SimEvo가 위임한 [57]의 확장판이라고 STVR이 스스로 밝히므로(STVR p.2 각주), SimEvo 본문이 정하지 않는 사건·자원 모델의 **계보 근거**로만. SimEvo 구현의 서술로 쓰지 않는다 |
| RacePro(SOSP'11) | SimEvo에는 `[2차]` | SimEvo가 [26]으로 위임한 자원 모델의 참고 |
| 저자 홈페이지, NSF 과제 초록, UKy 기록 | 서지 | artifact 소재 확인 |
| Intel Simics 페이지(WebFetch 요약) | `[2차]` | 실행 환경의 일반 요구. 요약이라 문구를 인용하지 않았다 |
| `cases/C1.md`, `C2.md`, `C3.md`, `C5.md`, `R2.md`, `baselines/availability.md`, `yu-group.md` | `[기록]` | 사례 쪽 사실과 이전 조사 상태. 재검증하지 않았다 |
| 로드맵·PI 결정 | `[D]` 또는 계획 | 질문 목록, 결과 상태, 사례 구성 후보 |

### 1.3 이전 기록과 달라지거나 확정된 것

| 이전 기록 | 오늘 |
|---|---|
| `yu-group.md` §3.2, §4: SimEvo는 "excerpt only", "No system-call model is stated in the excerpt" | 전문을 읽었다. **SimEvo 본문이 system call의 read/write 효과와 동기화 연산(fork, wait, exit, pipe, signal)을 사건 모델로 서술한다** p.1–3. "서술되지 않았다"는 문장은 발췌의 한계였다. 정정한다 |
| `availability.md` §1, `yu-group.md` §5 R9: Simics는 발췌로만 확인 | 본문이 확인한다. 논문 서문(p.1)이 Simics 사용을 말하고, p.7이 구현을 "SimRacer … built on top of the Simics Virtual Platform"이라 적는다. Simics 버전·머신·guest OS는 본문에 없다 |
| `yu-group.md` R10: "Two versions plus tests" | 확인. 입력은 S, S′, 기존 테스트 T, 커버리지 기준 PT다 p.4. 테스트는 연구진이 category-partition으로 만든 black-box suite였다 p.7–8 |
| `yu-group.md` §7.2, `availability.md` §3: SimEvo를 SimRacer와 한 행으로 묶음 | 일부 분리한다. SimEvo는 자기 사건 서술과 회귀 전용 단계(변경 영향 분석, 테스트 생성)를 가진다. 다만 **사건·자원 모델의 세부는 SimRacer·RacePro에 위임**하므로 그 부분은 SimRacer 원문 없이는 정해지지 않는다 |
| 구현 없음("None found") | 오늘 경로 12–26을 더 시도했고 결과는 같다. 코드·VM 이미지·스크립트를 찾지 못했다. "존재하지 않는다"가 아니라 "접근 가능한 출처에서 찾지 못했다"이다 |
| (새로 확정) | SimEvo의 동적 CIA와 interleaving 탐색은 **SimRacer 위에 구현**됐다 p.7. SimEvo를 쓰려면 SimRacer 구현이 필요한데 그것도 찾지 못했다. static CIA는 상용 CodeSurfer, GA는 C++로 별도다 |

### 1.4 주요 진술의 등급

| 진술 | 등급 | 근거 |
|---|---|---|
| SimEvo는 S, S′, T, PT를 받아 변경 영향 분석·테스트 생성·interleaving 예측·재생을 한다 | `[전문]` | p.4–6 |
| 사건은 system call의 R/W 효과와 fork, wait, exit, pipe, signal이다 | `[전문]` | p.1, p.3 |
| 구현은 CodeSurfer(static CIA), C++ GA, SimRacer 위의 동적 단계다 | `[전문]` | p.7 |
| 자원 모델의 세부는 [26], [57]에 위임됐다 | `[전문]` | p.3 |
| 오라클은 순차 실행 출력 또는 내장 오라클이다 | `[전문]` | p.8 |
| SimEvo 구현이 메모리 접근을 관측한다 / 관측하지 않는다 | `[U]` 불명. 어느 쪽도 쓰지 않는다 | — |
| 계보 도구 SIMEXPLORER는 SR 연산에 메모리 중단점을 건다 | `[2차]` | STVR p.8, p.14 |
| SimEvo 구현·artifact를 찾지 못했다 | 접근 가능한 출처에서 찾지 못했다는 뜻이다. 부재의 증명이 아니다 | §1.1 경로 12–26 |
| OSAL 이름 등록·조회는 syscall 0건이었다 | `[기록]` (OSAL 자체 시험의 probe. SimEvo의 결과가 아니다) | `availability.md` §4.2 |
| 사례별 의존성과 현상, 수정 내용 | `[기록]` | `cases/*.md` |
| 사례별 버전 쌍 후보(C1, R2) | `[D]` | PI 결정 D22, P-R2CTRL |
| Simics 공개판의 요구 조건 | `[2차]`(요약) | Intel 공개 배포 페이지 |

---

## 2. 도구의 정체

### 2.1 요약

| 항목 | 내용 | 근거 |
|---|---|---|
| 역할 | **회귀 시험.** 코드 변경이 만든 system-level 동시성 결함을, 변경 영향 범위로 좁혀 시험한다. 결론은 "detecting system-level concurrency faults that are induced due to code changes in multi-process systems"라고 적는다. 오라클이 확인하는 결함을 보고하지만 역할은 회귀 시험이다. 로그·버그 보고에서 출발하는 재현 도구가 아니다 | p.1, p.10 `[전문]` |
| 실행 단위 | **OS process**(processes under test, PuT)와 **signal handler**·interrupt handler. 한 subject에서 두 개를 다뤘고 "more than two processes in a pairwise fashion" 처리할 수 있다고 한다. 같은 process 안 thread-level 버그는 대비 대상으로만 나온다("Unlike an intra-process (thread-level) concurrency bug…") | p.1, p.7 |
| 관측 | Simics VM의 observability. "shared resource accesses (e.g., accessing system-level resources through system calls)"와 "process-level synchronization operations (e.g., fork, wait)"를 모은다 | p.1, p.2 |
| 제어 | Simics VM의 controllability. "causing the kernel scheduler to explore the affected interleavings" | p.2 |
| 자원 식별 | 공유 자원은 "system-wide resource (e.g., file, device, or hardware register)"이고 system call로 접근한다고 서술한다. 정적 단계는 공유 자원을 구분하지 않고("does not distinguish shared resources from other resources"), 동적 단계가 보완한다. 구체적 식별 기준은 [26] RacePro와 [57] SimRacer에 위임한다. 본문이 직접 쓰는 식별은 하나뿐이다. 생성한 test input의 파일 이름을, 두 PuT가 그 입력에 접근한 system call 목록이 모두 비어 있지 않으면 통일한다 | p.3, p.5 |
| 순서·의존 모델 | 다섯 partial order: program order, fork-return, wait-exit, pipe-read, signal. system call 의미에서 **추론**한다. 접근마다 dynamic synchronization context(DSC)를 만들어 두 버전의 trace에서 비교한다 | p.3, p.6 |
| 입력 | 두 버전의 소스(모든 process), 기존 테스트 T, 커버리지 기준 PT, GA의 시간 한계 t_iter. 오라클은 테스트 suite의 내장 오라클이거나 프로그램 출력(콘솔 메시지·생성 파일). 연구에서는 두 process를 순차 실행해 오라클을 얻었다 | p.4, p.5, p.8 |
| 출력 | 선택된 테스트 집합 T∆, 영향받은 Def-Use 쌍의 커버리지 C, 결함 F. Table II는 subject별로 결함을 Y/No로 보고한다 | p.4, p.9 |
| 구현 | static CIA는 상용 CodeSurfer [17], 유도 GA는 C++, 동적 CIA와 interleaving 탐색은 SimRacer [57] 위에 구현했다. SimRacer는 Simics 위의 process-level 시험 도구다 | p.7 |
| 평가 | 오픈소스 C/C++ 프로그램 13종(subject 쌍 15개, 최대 약 195K NLOC): mv, rm, chmod, cp, ln, mkdir, tail, sort, strace, bzip2, bash, tcsh, apache. 각 프로그램의 두 버전을 쓰고, 수정 버전은 실제 보고된 process 간 공유 자원 결함을 담는다. Table I의 root cause는 system call 순서(unlink, rename, stat, write 등), fork/wait/execve, signal/sigprocmask 순서다. 논문 보고: 같은 조건에서 SimRacer보다 결함을 더 찾고 커버리지가 높다 | p.2, p.7, p.9 |

### 2.2 동작 흐름 (Algorithm 1, Figure 1)

1. **정적 변경 영향 분석(StaticCIA).** 변경 집합 ∆diff를 diff와 AST 구조 비교로 얻고, 변경 문장마다 forward slicing해 의존하는 system call을 영향 집합 SL에 넣는다(구현은 CodeSurfer를 썼다고 p.7이 적는다). 동기화 연산이 변경이면 연산 종류별 규칙(fork-notify, wait-exit, pipe read-write, signal enable)으로 의존 system call을 더 넣는다 p.4–5. 결과는 과대추정이다.
2. **기존 테스트의 도달 확인(ExeConcrete).** T를 S′에서 돌려 SL 중 도달한 것을 빼고 남은 목표 TG를 얻는다 p.4.
3. **변경 유도 test input 생성(GuidedGA).** PuT마다 GA로 TG를 덮는 새 입력 NT를 만든다. fitness는 approach level과 branch distance다. 한 PuT에서 만든 입력을 같은 종류의 입력을 받는 다른 PuT에 시드로 재사용한다 p.5.
4. **테스트 선택(SelectTests).** 변경·영향 요소를 하나라도 지나는 테스트만 T∆로 고른다 p.4.
5. **동적 변경 영향 분석(DynamicCIA).** T∆의 각 테스트를 S와 S′에서 임의 스케줄로 실행해 trace E, E′를 얻고, 접근별 DSC를 비교해 실제로 영향받은 접근 SL∆를 가른다 p.4, p.6.
6. **영향 interleaving 예측·재생(ScheduleGeneration, Replay).** predictive trace analysis(PTA)로, 영향받은 접근을 하나 이상 포함하는 read/write 쌍의 순서를 partial order 안에서 뒤집은 스케줄을 **offline으로** 만든다. 목표 패턴은 inter-process Def-Use다(정의 = system call write, 사용 = system call read). 재생이 성공하면 C와 F를 갱신한다 p.4, p.6. 논문은 offline 생성이 "active randomized testing"과 달리 쌍을 뒤집는 feasible 스케줄을 보장한다고 주장하고 근거를 Huang et al. [20]에 둔다 p.6.

### 2.3 SimEvo 본문이 정한 것, 위임한 것, 계보 논문이 말하는 것

| 항목 | SimEvo 본문(`[전문]`) | 위임·계보 근거(`[2차]`) |
|---|---|---|
| 사건 종류 | system call의 R/W 효과 + fork, wait, exit, pipe, signal p.1, p.3. 기술 문구는 "system-level concurrent events (i.e., system calls)" | STVR p.8(SIMEXPLORER): SR로 shared memory, fd, inode, file data, process data, hardware components를 열거하고 23개 system call을 추적한다. **memory 접근의 read/write는 "observability support implemented in our prior work [16,33]"으로 식별한다.** [16]은 SimRacer(ISSTA'13), [33]은 SimTester(VEE'12)다. 어느 쪽이 어느 부분을 맡는지는 서술이 없다 |
| 메모리 관측 | 본문에 서술 없음(load/store 0건) | STVR p.8, p.12, p.14: Observer가 SR 연산에 메모리 중단점을 건다. 전역 변수 load/store 사건 예가 있다(p.12). SIMEXPLORER의 서술이며 SimRacer·SimEvo 구현에 해당하는지는 **불명** |
| 같은 자원 판정 | 서술 없음. 파일 이름 통일만(p.5). 세부는 [26], [57]에 위임 p.3 | RacePro §5.2(`[2차]`): 자원은 커널 객체(inode, file, file-table, mmap, pid …; Table 2)이고 pathname마다 별도 메모리 위치로 모델링한다. 커널 객체 모델이며 SimEvo 구현이 같은지는 불명 |
| thread 단위 | 대비 대상으로만 언급 p.1 | STVR p.13: "every thread is a lightweight process with its own Process Identification Number (PID), so SIMEXPLORER can track it just like a process". SIMEXPLORER에 대한 서술이다 |
| interleaving 탐색 방식 | offline predictive 생성 + 재생. SimRacer는 "active testing … randomly explores interleaving schedules"라고 SimEvo가 서술 p.6, p.8 | STVR p.10: 한 쌍마다 두 접근에서 두 process를 정지시킨 뒤 순서를 바꿔 재개하는 Phase 2를 서술. 서술 방식이 SimEvo의 "offline 생성 후 재생"과 다르다 |
| 오라클 | 순차 실행 출력 또는 내장 오라클 p.8 | STVR p.7, p.10: 출력을 test oracle과 비교 |

### 2.4 논문이 직접 밝힌 한계 중 cFS와 관련 있는 것

- **출력이 오류 없이 바뀌는 문제.** "the messages printed on the console and the written files might change between interleavings without being associated to any error." 연구에서는 그런 경우를 보지 못했고 future work로 남겼다 p.8. 허용된 오류 처리를 입력으로 받지 않는 구조와 이어진다.
- **적용 범위.** 변경이 프로그램 일부에 국한될 때 효과적이다. 연구의 변경은 프로그램의 약 2.5%–15.8%였고, 전체가 바뀌면 incremental 분석이 비증분 분석으로 퇴화한다 p.10.
- **interleaving 커버리지 기준.** 기본은 Def-Use이고 다른 기준은 미탐구다 p.10.
- **static CIA는 보수적이다.** 과대추정하며 공유 자원을 구분하지 않는다 p.5. 영향 집합은 동적 CIA로 줄인다.
- **쌍 단위 처리.** subject마다 process 둘을 다뤘다 p.7.
- **큰 프로그램에서 GA가 24시간 한계에 도달했다.** bash, tcsh, apache에서 "> "로 표시된다. 이 세 subject의 커버리지는 82.7%, 79.4%, 58.5%였다 p.8, p.9(Table II). 24시간은 논문의 연구 조건이지 cFS 조건이 아니다.

---

## 3. 여섯 질문 × 다섯 사례

### 3.1 읽는 법

셀의 라벨은 아래 셋 중 하나다. 한 칸에 관측 가능성, 표현 가능성, 실제 탐지 성공을 합치지 않는다(로드맵 §10.2).

| 라벨 | 뜻 |
|---|---|
| **답** | 출처의 쪽·절로 답했다(answered with source page/section) |
| **분석** | analysis from stated design (not run). 읽은 설계 기술에서 도출했고 실행하지 않았다 |
| **불명** | not determinable from sources read |

"분석" 셀의 "서술에 없다"는 "읽은 설계 기술 안에서 그 대응 경로를 찾지 못했다"는 뜻이다. 실행 결과가 아니다. 쪽 번호는 별도 표시가 없으면 SimEvo(`[전문]`)의 쪽이다. 질문 번호는 로드맵 §10.2의 순서다.

1. 필요한 **관측 사건**을 수집하는가?
2. 서로 다른 이름·ID·필드가 같은 **자원·세대**를 가리키는 것을 식별하는가?
3. **생성·등록·초기화 완료와 의존 사용**의 관계를 표현하는가?
4. 필요한 **순서(readiness precondition) 또는 허용된 오류 처리** 정보를 입력받는가, 추론하는가?
5. 아직 실행되지 않은 순서를 **예측·탐색**하는가? 실제 실행에서 **확인**할 수 있는가?
6. **출력**은 후보·재현 성공·오류 결과 중 무엇이며 원 사례의 현상과 대응하는가?

**모든 "분석" 셀에 공통으로 걸린 두 전제.**
- (a) 사례의 접근이 SimEvo의 사건 집합(system call 효과와 다섯 동기화 연산)에 들어 있어야 한다.
- (b) 사례의 task들은 한 Linux process 안의 pthread다(저장소 기록: `yu-group.md` §7.1). SimEvo의 PuT는 process다 p.1, p.7. 한 process 안의 cFS app/task 쌍을 PuT 쌍으로 줄 수 있는지는 SimEvo 본문에 서술이 없다. 이 연구의 Process-Level은 cFS app/task를 작업 단위로 쓰는 운용상 정의라는 PI 결정 `[D]`과 SimEvo의 명시 대상(multi-process, 신호·인터럽트 핸들러) 사이의 차이는 §4.3에서 PI 판단 후보로 올렸다.

### 3.2 사례 쪽에서 쓴 사실과 SimEvo가 요구하는 버전 쌍 (모두 저장소 기록, 재검증하지 않음)

| 사례 | 의존성과 현상 | 회귀 버전 쌍 (S → S′) 후보 |
|---|---|---|
| C1 CF#184 | provider가 이름 있는 OSAL count semaphore를 만든 뒤에야 CF의 `OS_CountSemGetIdByName`이 성공한다. 수정 전 CF는 한 번만 조회하고 실패하면 `CF_EID_ERR_INIT_SEM` 이벤트와 `APP_ERROR` 종료다. 수정은 `OS_ERR_NAME_NOT_FOUND`일 때만 25회×100 ms 재시도이며 준비 보장이 아니다(`cases/C1.md` §5, §7, §8). 공개 BP 구성은 `BP0_tsem`·`BP1_tsem`이다 | CF `281a941` → `833fdbb` `[D]` (D22의 첫 후보. 사실이 아니라 결정이다) |
| C2 cFE#198 | 앱 P의 late init 완료와 앱 R의 의존 호출. P·R은 비공개(EVA CWS)이고 통신·초기화 내용이 공개되지 않았다. 공개된 것은 단일 상태 시작 동기화와 6.6.0a의 두 단계 방식(`APPS_INIT`, `LATE_INIT`)이다(`cases/C2.md` §4–§8) | cFE 6.5.0a → 6.6.0a(수정과 #198의 연결은 추론, 기록). P·R 쪽은 소스 없음 |
| C3 cFE#73 | SB task의 `CFE_SB.AppId`와 EVS task의 `EVS_AppID`·등록 플래그가 설정되기 전에 TIME task가 `CFE_SB_CreatePipe` 안에서 이벤트를 보내고, `EVS_IsFiltered(0xFFFFFFFF)`가 범위 검사 없이 접근한다. 오류 지점은 미확인이다(`cases/C3.md` §4, §5, §7) | cFE 6.4.1 → 6.4.2. 수정에 해당하는 공개 git 객체가 없고 SourceForge tarball 차이로만 관측된다(`cases/C3.md` §8.1) |
| C5 cFE#72 | `CFE_ES_AppCreate`가 ES lock을 `OS_TaskCreate` 앞에서 풀고 뒤에서 다시 잡아 `TaskTable`을 채운다. 그 사이 새 task의 `CFE_ES_GetAppID`가 `CFE_ES_ERR_APPID`를 반환할 수 있다. 새 task가 먼저 실행된 것만으로는 위반이 아니다(`cases/C5.md` §4–§7) | cFE 6.4.1 → 6.4.2. 같은 이유로 tarball 차이뿐(`cases/C5.md` §8). 수정은 lock 범위 확장 |
| R2 cFE#2663 | SB가 `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`로 AppId를 저장하기 전에 ES/EVS task가 SB API 경로를 실행해 EVS 조회가 `CFE_EVS_APP_ILLEGAL_APP_ID`를 낸다. 서술은 "이벤트 요청 거부"이고 gdb 3회 모두 설정 전 사용 24회다(로드맵 §2.2). 출력 메시지는 없다(`cfe_evs.c:185-188`, `availability.md` §1). 이 이슈에는 병합된 수정이 없다(`cases/R2.md` §8) | `b9bec567^` → `b9bec567` `[D]` (P-R2CTRL (c2). core 목록 순서를 바꾼 변경이며 CMake 목록 `mission_defaults.cmake`가 바뀐다. `cases/R2.md` §9) |
| 공통 | native cFS는 한 Linux process이고 OSAL task는 `pthread_create` thread다. OSAL 자체 시험에서 named count semaphore의 생성과 이름 조회는 **syscall 0건**이었다(probe이며 SimEvo의 결과가 아니다) | `availability.md` §4.2 `[기록]` |

### 3.3 도구 수준의 답 (사례와 무관)

| Q | 라벨 | SimEvo의 답 | 근거 | 계보 논문이 보태는 것(`[2차]`, SimEvo 구현에 해당하는지는 불명) |
|---|---|---|---|---|
| 1 관측 사건 | 답 | system call의 R/W 효과와 fork, wait, exit, pipe, signal을 Simics VM에서 관측한다. 메모리 접근·lock·반환값은 사건으로 서술되지 않는다 | p.1, p.2, p.3 | STVR p.8, p.14: shared memory를 SR로 열거하고 메모리 중단점으로 관측 |
| 2 자원·세대 식별 | 답(파일 이름 통일만). 나머지는 불명 | 본문의 식별은 test input 파일 이름 통일뿐이다. 이름·ID·슬롯 대응과 세대는 서술이 없다(키워드 검색: generation은 test·schedule generation 용법뿐). 나머지는 [26], [57]에 위임 | p.3, p.5 | RacePro §5.2: 커널 객체와 pathname별 위치. 세대 서술 없음 |
| 3 생성·등록 vs 의존 사용 | 답 | 본문이 정의한 관계는 다섯 partial order(program, fork-return, wait-exit, pipe-read, signal)다. 생성·등록·초기화 완료 같은 상태 관계는 정의에 없다. 접근 쌍은 "읽기와 쓰기, 읽기가 먼저, 적어도 하나가 impacted"로 다룬다 | p.3, p.6 | STVR p.8–10: 같은 SR, 하나 이상 쓰기, HB 없음이면 잠재 race. "semaphores and locks"로 확장할 수 있다고만 적는다 |
| 4 필요 순서·허용 오류 입력 | 답 | **입력이 아니다.** 입력은 S, S′, T, PT다. HB는 system call 의미에서 추론한다. 실패는 순차 실행 출력과의 비교다. 허용 오류와 재시도 서술은 없다(retry 0건) | p.3, p.4, p.8 | STVR p.2–3: race 정의에 "intended order"가 있으나 입력 필드는 없다 |
| 5 예측·탐색, 확인 | 답 | 설계상 있다. 관측된 한 trace에서 impacted 접근을 포함하는 쌍을 뒤집는 스케줄을 offline으로 만들고(PTA), Simics 재생으로 feasibility를 확인한다. 대상은 Def-Use 패턴의 쌍이다. 두 process씩 짝지어 처리한다 | p.4(Algorithm 1 10–12행), p.6, p.7 | STVR p.10–11: 쌍마다 실제 실행에서 순서를 바꿔 확인 |
| 6 출력 | 답 | 선택된 테스트 T∆, 영향받은 Def-Use 쌍의 커버리지 C, 결함 F(오라클 불일치). subject별 Y/No와 커버리지·시간을 보고한다 | p.4, p.9 | STVR p.10, p.13: 잠재 쌍, real race, fault를 구분 |

### 3.4 5 × 6 라벨 요약

| | Q1 관측 | Q2 식별 | Q3 생성·사용 | Q4 입력 | Q5 예측·확인 | Q6 출력 |
|---|---|---|---|---|---|---|
| **C1** | 분석 | 불명 | 분석 | 답 | 분석 | 분석 |
| **C2** | 불명 | 불명 | 불명 | 답 | 불명 | 불명 |
| **C3** | 분석 | 불명 | 분석 | 답 | 분석 | 분석 |
| **C5** | 분석 | 불명 | 분석 | 답 | 분석 | 분석 |
| **R2** | 분석 | 불명 | 분석 | 답 | 분석 | 분석 |

### 3.5 사례별 셀 (근거 포함)

#### C1 — CF#184 (named count semaphore의 생성 vs 이름 조회)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | 필요한 사건은 provider의 이름 등록 완료, CF의 `OS_CountSemGetIdByName` 호출과 반환값, 재시도 지연, CF 초기화의 최종 결과다. SimEvo가 서술하는 사건 종류(system call 효과와 다섯 동기화 연산)에 이름 등록·조회는 없다. OSAL 자체 시험에서 두 호출이 syscall 0건이라는 기록이 있다(`availability.md` §4.2). 재시도 지연은 `OS_TaskDelay`가 `clock_nanosleep`을 부르므로(`yu-group.md` §7.1) syscall로 보이지만 SimEvo가 sleep을 사건 종류로 서술하지는 않는다. 기반 SimRacer가 메모리를 관측하는지는 불명이다(§2.3) | p.1, p.3; `[기록]` |
| 2 | 불명 | 이름 `BP0_tsem` ↔ OSAL ID ↔ 이름 테이블 슬롯, 삭제·재생성으로 달라지는 세대를 같은 자원으로 잇는 서술이 없다. 본문의 식별 장치는 test input 파일 이름 통일뿐이고 cFS 이름과 무관하다 | p.5 |
| 3 | 분석 | 표현 수단은 다섯 partial order뿐이고 생성→조회는 그 안에 없다. 쌍이 사건으로 관측된다면 접근 쌍(쓰기, 읽기)으로 다뤄질 수 있으나, 등록 예약 상태와 공개 완료, 재시도 중과 최종 실패를 구분하는 개념은 서술에 없다 | p.3, p.6 |
| 4 | 답 | 입력은 S, S′, T, PT이고 오라클은 출력 비교다. C1의 readiness precondition("생성이 조회보다 먼저")과 허용된 오류("`NAME_NOT_FOUND`일 때만 재시도")를 넣을 입력이 없다. 수정의 재시도는 S′의 코드 변경으로만 들어간다. 오류 없이 출력이 바뀌는 경우는 미해결로 남겼다 | p.4, p.8 |
| 5 | 분석 | 구조는 맞는다. S(수정 전)와 S′(재시도 추가)의 소스 쌍이 입력 모양이고, 변경 문장에서 forward slice로 영향 system call을 고른 뒤 관측 trace에서 쌍의 순서를 뒤집어 재생한다. 단, 이 구조가 C1에 닿으려면 생성·조회가 영향 집합과 trace의 사건이어야 한다. "재시도 뒤 회복"과 "최종 실패"를 구분하는 서술은 없다 | p.4–6 |
| 6 | 분석 | 출력은 T∆, C, F다. C1의 현상은 CF 초기화 실패 이벤트와 `APP_ERROR` 종료이고, 수정 뒤에는 재시도로 회복하거나 한도 뒤 실패한다. 오라클이 콘솔 메시지·파일을 비교하므로(p.8) 이 현상이 어떤 출력으로 나타나는지는 cFS 실행에서만 정해진다. 사례의 실패 출력과 빈도는 `unspecified_by_reference`다 | p.4, p.8, p.9; `cases/C1.md` §7 `[기록]` |

#### C2 — cFE#198 (late init 완료 vs 다른 앱의 의존 호출)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 불명 | provider의 late-init 완료와 consumer의 의존 호출이 어떤 호출·상태인지 공개 자료가 없다. 앱 소스가 비공개라 필요한 사건을 정의할 수 없다 | `cases/C2.md` §4, §5 `[기록]` |
| 2 | 불명 | 대상 자원이 정해지지 않았다 | 같음 |
| 3 | 불명 | 대상 자원이 정해지지 않았다. 참고로 공개된 cFE 쪽 장치(`WaitForStartupSync`, `APPS_INIT`, `LATE_INIT`)와 같은 종류의 상태 관계는 SimEvo의 다섯 관계에 없다. 이 사실은 대상 앱의 관계를 표현하는지에 대한 답이 아니다 | p.3; `cases/C2.md` §8 `[기록]` |
| 4 | 답 | 도구 수준 답과 같다. readiness precondition과 허용 오류를 입력으로 받지 않는다 | p.4, p.8 |
| 5 | 불명 | 실행할 대상 코드가 없다. 6.5.0a→6.6.0a의 cFE 쪽 변경은 소스 쌍으로 줄 수 있으나 P·R이 없어 impacted 접근의 trace가 정해지지 않는다 | `cases/C2.md` §8 `[기록]` |
| 6 | 불명 | 사례의 실패 현상이 서술되지 않았다 | `cases/C2.md` §7 `[기록]` |

로드맵 §9.1에 따라 C2의 결과 이름은 "C2 메커니즘의 역할 기반 검증"이다. 원 비공개 시스템의 재현으로 쓰지 않는다.

#### C3 — cFE#73 (SB/EVS AppId 설정 vs TIME의 이벤트 요청)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | 필요한 사건은 SB의 `CFE_SB.AppId` 저장, EVS의 `EVS_AppID` 저장과 `RegisterFlag` 설정, TIME task의 `CFE_SB_CreatePipe`와 그 안의 이벤트 전송 호출이다. 전부 메모리 연산과 함수 호출이다. task 생성은 `clone3` syscall이다. 서술된 사건 종류에 AppId·등록 플래그는 없다 | p.1, p.3; `cases/C3.md` §4, §5 `[기록]` |
| 2 | 불명 | SB AppId, EVS 자신의 AppId, 등록 플래그, ES `TaskTable`은 서로 다른 필드다(로드맵 §2.2의 경고). 이것들을 서로 구분하거나 같은 자원·세대로 잇는 서술이 없다 | p.5 |
| 3 | 분석 | 수정이 쓴 "publish-last"(AppId를 초기화 끝에 공개)와 core task별 시작 barrier는 다섯 partial order 밖이다. 설정(쓰기)과 사용(읽기)의 쌍이 관측된다면 접근 쌍으로 다뤄질 수 있다 | p.3; `cases/C3.md` §8.2 `[기록]` |
| 4 | 답 | 도구 수준 답과 같다 | p.4, p.8 |
| 5 | 분석 | S=6.4.1, S′=6.4.2의 소스 쌍은 공개 git 객체 없이 tarball로만 얻을 수 있다. 입력 모양(소스 두 벌)은 SimEvo가 요구하는 형태와 맞는다. 쌍 뒤집기와 재생의 적용은 사건이 영향 집합에 들어가는지에 달려 있다 | p.4–6; `cases/C3.md` §8.1 `[기록]` |
| 6 | 분석 | 사례의 현상은 TIME 경로에서 `EVS_IsFiltered(0xFFFFFFFF)`의 범위 밖 접근과 이어지는 crash다. 오류 지점은 `unspecified_by_reference`다. SimEvo의 결함 판정은 출력 비교이고 crash를 오라클이 어떻게 받는지는 서술이 없다 | p.8; `cases/C3.md` §5 `[기록]` |

#### C5 — cFE#72 (task 생성과 레지스트리 등록 사이의 공백)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | 필요한 사건은 `OS_TaskCreate`(`clone3`), 생성 뒤 ES lock을 다시 잡고 `TaskTable[TaskId]`를 채우는 기록, 새 task의 `CFE_ES_GetAppID` 조회와 반환값이다. task 생성은 `clone3`이고, SimEvo가 다루는 process 생성(fork)과 같은 사건으로 보는지는 서술이 없다. ES 레코드 기록·조회와 lock은 서술된 사건 종류에 없다 | p.3; `cases/C5.md` §4, §5 `[기록]` |
| 2 | 불명 | OSAL task 인덱스(TaskId)와 AppId를 잇는 서술이 없다 | p.5 |
| 3 | 분석 | fork-return order는 fork 사건과 자식 쪽 대응 return 사이의 순서다(p.3). 부모가 task를 만든 **뒤**에 하는 등록(6.4.1 L637–L673)을 자식의 조회보다 앞세우는 관계는 다섯 가운데 없다. 수정은 lock 범위 확장이고 lock은 사건 종류에 없다 | p.3; `cases/C5.md` §4.2, §8 `[기록]` |
| 4 | 답 | 도구 수준 답과 같다 | p.4, p.8 |
| 5 | 분석 | S=6.4.1, S′=6.4.2는 tarball 차이뿐이다. 변경은 mutex 사용 범위 변경인데, SimEvo가 연산별로 규칙을 정한 동기화 연산은 fork, wait, pipe, signal뿐이다(p.5). lock 변경이 영향 집합에 들어가는지는 서술이 없다 | p.5; `cases/C5.md` §8 `[기록]` |
| 6 | 분석 | 사례의 현상은 `CFE_ES_GetAppID`가 `CFE_ES_ERR_APPID`를 돌려주는 "등록 전 조회"다. 새 task가 먼저 실행된 것만으로는 위반이 아니다. SimEvo의 출력은 결함(오라클 불일치)이며 이 반환값과의 대응 경로는 서술에 없다 | p.4, p.8; `cases/C5.md` §7 `[기록]` |

#### R2 — cFE#2663 (SB AppId 설정 vs ES/EVS의 이벤트 요청)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | 필요한 사건은 SB의 `CFE_ES_GetAppID(&CFE_SB_Global.AppId)` 저장, ES·EVS task의 SB API 호출(`CreatePipe`, `Subscribe`, 이벤트 요청), EVS 조회의 반환 `ILLEGAL_APP_ID`다. 전부 메모리 연산과 함수 호출이다. 서술된 사건 종류에 없다 | p.1, p.3; 로드맵 §7.1 |
| 2 | 불명 | `CFE_SB_Global.AppId`와 EVS 등록 정보를 서로 다른 상태로 구분하는 서술이 없다 | p.5 |
| 3 | 분석 | 로드맵 §1.3의 분석은 R2를 lock 부재가 아니라 기존 core 시작 직렬화가 반대 순서를 강제하는 사례로 본다. 시작 직렬화와 "저장 후 사용"은 다섯 partial order 밖이다. 저장(쓰기)과 사용(읽기)의 쌍이 관측된다면 접근 쌍으로 다뤄질 수 있다 | p.3; 로드맵 §1.3 |
| 4 | 답 | 도구 수준 답과 같다 | p.4, p.8 |
| 5 | 분석 | 자연 회귀 쌍 `b9bec567^`/`b9bec567`은 core 시작 순서를 바꾼 변경이다. SimEvo의 변경 집합은 소스 diff와 AST 비교로 서술된다. 그 변경이 CMake 목록이라는 점이 변경 집합에 들어가는지는 서술이 없다. 동적 단계의 DSC는 다섯 관계의 차이만 비교하므로 시작 직렬화의 변화는 그 관계에 담기지 않는다 | p.4, p.6; `cases/R2.md` §9 `[기록]` |
| 6 | 분석 | 사례의 현상은 이벤트 요청 거부(`ILLEGAL_APP_ID` 반환)이고 출력 메시지가 없다. 오라클이 콘솔 메시지·파일 출력 비교인 구조(p.8)와 이 현상의 대응 경로는 서술에 없다. 이것은 탐지 결과가 아니다 | p.8; `availability.md` §1 `[기록]` |

### 3.6 이 절이 말하지 않는 것

- SimEvo가 어느 사례를 **탐지하지 못한다**는 결과를 말하지 않는다. 실행하지 않았다.
- "분석" 셀은 설계 서술에서 도출한 해석이며, 구현이 서술보다 넓거나 좁을 수 있다. 특히 사건 모델은 SimRacer와 RacePro에 위임됐고 SimRacer 원문은 열지 못했다.
- "불명" 셀은 읽은 자료로 정해지지 않았다는 뜻이다. 부정이 아니다.
- 한 칸에 관측 가능성, 표현 가능성, 실제 탐지 성공을 합치지 않았다.

---

## 4. 실행 가능성과 결과 상태

### 4.1 요구 조건과 오늘 확인한 상태

| 요구 | 출처 | 오늘 상태 |
|---|---|---|
| SimEvo 구현(코드·VM 이미지·스크립트) | — | 찾지 못했다(§1.1 경로 12–26). 논문에도 위치 서술이 없다. NSF 과제 초록의 "make available"은 계획 서술이고 위치가 없다 |
| 기반 도구 SimRacer 구현 | p.7 | 찾지 못했다. 오늘 GitHub 코드 검색 `SimRacer Simics "process-level"`은 1건이고 무관했다. `plan/methods/SimRacer.md` §1.1의 기록과 같은 결론이다. 원문은 열지 못했다 |
| Simics | p.1, p.7 | **이 컨테이너에 없다** [M]. 논문은 버전·시뮬레이션 머신·guest OS·kernel을 적지 않았다 → 선택하면 `unspecified_by_reference`다. Intel 공개 배포 페이지의 요약(`[2차]`): 연구자에게 무료이고 EULA 동의가 필요하며, 호스트는 64-bit Linux(RHEL 8이 권장 최소)나 Windows, RAM 16 GB 이상이다. VT-x는 선택이며, VM 안에서 돌리면 VT-x를 쓰지 못하거나 느려진다고 한다. 이 컨테이너는 `/dev/kvm`이 없고 vmx/svm이 0이다 [M]. 논문이 쓴 Simics가 이 공개판과 같은 버전인지는 불명이다 |
| CodeSurfer(상용, static CIA) | p.7 [17] | 현재 판매·라이선스는 불명. 논문이 인용한 제품 URL은 404 [M]. 이 컨테이너에 없다 |
| 두 버전의 소스 S, S′ | p.4 | 사례별로 §3.2. C1은 결정 후보 `[D]`, C3·C5는 tarball 차이뿐, C2는 P·R 소스가 없음, R2는 병합된 수정이 없어 자연 쌍 `[D]`을 쓴다 |
| 기존 테스트 T (구체 입력) | p.4, p.7 | cFS에서 "테스트 입력"이 무엇인지 레퍼런스가 정하지 않았다 → `unspecified_by_reference` |
| 오라클 | p.8 | 사례별 출력 기준이 정해지지 않았다. 순차 실행 출력 방식은 cFS 다중 task 시작에는 정의가 없다 → `unspecified_by_reference` |
| GA 시간 한계 t_iter | p.5 | 연구 조건 24시간은 논문의 값이지 cFS 조건이 아니다(p.8). 선택은 PI 결정이 필요하다 |

### 4.2 결과 상태 (로드맵 §10.4)

| 사례 | 상태 | 이유 |
|---|---|---|
| C1 | `NOT_RUN` | 구현 없음(SimEvo, SimRacer), Simics·CodeSurfer 없음, 입력(T, 오라클) 미정 |
| C2 | `NOT_RUN` | 위와 같음. 추가로 대상 앱 소스가 비공개 |
| C3 | `NOT_RUN` | 위와 같음. 수정 쪽 소스는 tarball뿐 |
| C5 | `NOT_RUN` | 위와 같음. 수정 쪽 소스는 tarball뿐 |
| R2 | `NOT_RUN` | 위와 같음 |

`INAPPLICABLE`, `PATH_NOT_EXECUTED`, `OBSERVATION_GAP`, `ANALYSIS_GAP`, `REPORTED`는 도구를 실행한 뒤에만 쓸 수 있으므로 오늘은 어느 칸에도 쓰지 않는다. `NOT_RUN`은 미탐지 수에 넣지 않는다(로드맵 §10.4).

`availability.md` §5.1의 라벨로 보면 다음과 같다.
- `ORIGINAL-RUN`, `DOCUMENTED-SETUP-RUN`, `EXTENSION`: 구현이 없어 불가능하다.
- `DEVIATION-RUN`: 실행할 코드가 없어 불가능하다.
- `REIMPLEMENTATION`: 방법은 알고리즘 수준(p.4–6)까지 읽었으나, 사건·자원 모델은 SimRacer에 위임돼 있고 Simics·CodeSurfer 쪽 값이 정해지지 않았다. 재구현은 원 도구 결과가 아니며 PI 승인(A9)이 필요하다.
- `ANALYSIS`: 이 문서가 한 것이다. 등급은 `[전문]`(SimEvo 본문), `[2차]`(위임·계보)이다.

### 4.3 PI 판단이 필요한 항목 (결정 후보. 여기서 정하지 않았다)

1. **`INAPPLICABLE`을 쓸지.** SimEvo의 명시 대상은 multi-process 소프트웨어와 signal·interrupt handler이고 같은 process 안 thread-level 결함은 대비 대상이다(p.1). 이 연구의 Process-Level은 cFS app/task를 작업 단위로 쓰는 운용상 정의다 `[D]`. native cFS는 한 Linux process이다(`yu-group.md` §7.1 `[기록]`). 계보 논문 STVR p.13은 Linux에서 thread를 PID를 가진 lightweight process로 추적한다고 서술하는데 SIMEXPLORER에 대한 서술이며 SimEvo 구현은 불명이다. 이 둘 때문에 범위 일치 여부를 문서가 정하지 못한다. 쓰려면 SimEvo p.1, p.7을 원문 위치로 보고한다.
2. **저자에게 코드를 문의할지**(`availability.md` A2). 이번에도 누구에게도 연락하지 않았다.
3. **방법 재구현을 할지**(A9). 하게 되면 SimRacer 원문 열람(ISSTA'13)과 Simics 버전·타깃의 `unspecified_by_reference` 처리가 먼저다.
4. **Simics를 설치해 볼지.** 이 컨테이너에는 `/dev/kvm`이 없고, Intel 페이지의 요약은 VM 안에서 VT-x가 안 되거나 느려진다고 적는다. 설치하면 버전·타깃 선택이 `deviation` 또는 `unspecified_by_reference` 기록 대상이다.
5. **cFS에서 "테스트 입력"과 "오라클"을 무엇으로 정의할지.** SimEvo의 두 입력(T, 오라클)에 해당하는 레퍼런스가 없다.

---

## 5. 다섯 사례에서 부족한 것과 Kairos 기능 가설

### 5.1 SimEvo가 이미 하는 것 — 차별점으로 주장하지 않는다 (로드맵 §10.5 마지막 단락)

- **버전 쌍을 쓴 변경 영향 분석.** 정적(diff, AST 비교, forward slice)과 동적(DSC 비교)이다 p.4–6. 로드맵 §7.4·§8의 "자연 회귀 쌍에서 판정이 반응하는지 본다"와 발상이 겹친다. 쌍을 쓴다는 사실 자체를 새롭다고 쓰지 않는다.
- **관측된 trace에서 실행되지 않은 순서를 만들고 재생해 확인하는 구조.** offline predictive 생성과 Simics 재생이다 p.4, p.6. 로드맵 §10.5의 "관련 작업·자원으로 범위를 좁힌 예측·탐색 및 실행 확인"과 겹친다. 좁히는 대상이 "변경으로 영향받은 접근"이라는 점이 SimEvo의 방식이다.
- **영향받은 쌍으로 탐색 범위를 좁히는 것**(커버리지 C로 보고) p.4, p.9.
- (계보, SimEvo 구현은 불명) 메모리·하드웨어 접근의 관측 STVR p.8, p.14.

### 5.2 가설 (근거가 있는 만큼만)

모두 `[I]` 가설이며 확인된 부족이 아니다. 실제 도구 부족이 확인된 행만 비교 결과와 연결한다(로드맵 §10.5).

| # | 확인된 서술의 부족 | Kairos 기능 가설 | 사례 | 근거 |
|---|---|---|---|---|
| H-SE-1 | SimEvo 본문의 사건은 system call 효과와 다섯 동기화 연산이고(p.1, p.3), 자원 식별은 test input 파일 이름 통일뿐이며(p.5), 나머지는 [26], [57]에 위임됐다. 사례의 의존성(공개와 사용)은 syscall이 아닌 상태·메모리 연산이다(저장소 기록: OSAL probe, 소스 수준 서술). SimEvo 본문에는 cFS 수준 의미(AppId, 이름 테이블 항목, 등록 상태, task↔AppId)로 자원을 식별하는 서술이 없다 | 내부 API·상태 변화·값 전달을 관측하고 소스와 연결한다. 이름과 객체 ID·세대, task와 AppId의 관계를 추적한다 | C1, C3, C5, R2 | p.1, p.3, p.5; `availability.md` §4 `[기록]`. 한계: 위임 모델과 SimRacer 구현이 어디까지 보는지 불명이다 |
| H-SE-2 | 다섯 partial order에 readiness·초기화 완료 관계가 없다(p.3). lock, semaphore, 반환값, 재시도는 본문에 0건이다. 사례의 required order는 "생성·등록·초기화 완료 후 사용"이다(`cases/*.md` §7) | readiness precondition을 1급 관계로 표현한다. 시작 상태와 앱 고유 의존 상태를 구분하는 규칙을 둔다 | C2, C5, C1 | p.3 |
| H-SE-3 | 오라클은 순차 실행 출력 비교이고(p.8), 출력이 오류 없이 바뀌는 문제는 미해결이다(p.8). 허용된 오류 처리를 받는 입력이 없다 | 반환값·재시도·초기화 최종 결과를 추적하고 허용된 오류 처리를 판정 입력으로 받는다 | C1 | p.4, p.8 |
| H-SE-4 | 변경 집합은 소스 diff와 AST 비교로 서술된다(p.4). R2의 자연 쌍은 core 시작 순서를 바꾼 변경이고 CMake 목록이 바뀐다(`cases/R2.md` §9). 이런 구성 수준 변경이 SimEvo의 변경 집합에 들어가는지는 서술이 없다 | 빌드 구성이 만드는 시작 순서의 변화를 변경 집합과 판정 근거에 포함한다 | R2 | p.4; `cases/R2.md` §9 `[기록]`. 한계: SimEvo가 다루지 못한다는 단정은 근거가 없고, 서술이 없다는 사실뿐이다 |

### 5.3 근거가 부족해 가설로 올리지 않는 것

- **"SimEvo는 메모리 접근을 관측하지 못한다."** 계보 논문이 메모리 접근 관측을 SimRacer와 SimTester에서 이어받았다고 서술한다(STVR p.8). SimEvo는 SimRacer 위에 구현됐다(p.7). 근거가 반대 방향이거나 불명이다.
- **"SimEvo는 cFS의 task 단위를 다루지 못한다."** 계보 논문은 thread를 PID로 추적한다(STVR p.13). SimEvo 구현은 불명이다.
- **"SimEvo의 예측·탐색이 부족하다."** 예측(offline PTA)과 재생 확인이 설계에 있다(p.4, p.6).
- **"SimEvo가 어느 사례를 탐지하지 못한다."** 실행하지 않았다. 쓰지 않는다.
- **판정 근거의 설명(왜 그 순서가 고정됐는가).** SimEvo의 출력은 T∆, C, F이고(p.4) 목적이 회귀 시험이라 이 항목이 없는 것을 부족으로 읽을 근거가 없다. 로드맵 §10.5 마지막 행(R2)과의 연결은 SimEvo에서는 올리지 않는다.

### 5.4 비교 논문에서 먼저 밝혀야 할 정의 차이

- **실행 단위.** SimEvo는 OS process와 signal·interrupt handler이고, 이 연구는 cFS app/task다(운용상 정의 `[D]`).
- **"회귀"의 뜻.** SimEvo는 코드 변경이 만든 결함을 변경 영향 범위로 시험한다(p.1, p.10). 이 연구의 C1 수정 쌍·R2 자연 쌍은 순서 근거가 바뀔 때 readiness violation 판정이 철회되는지를 본다(로드맵 §7.4, §8). 두 문서가 같은 말을 다른 뜻으로 쓸 수 있다.
- **판정 기준.** SimEvo는 순차 실행 출력과의 불일치이고, 이 연구는 readiness specification에 근거한 판정을 탐지·분류·근거로 나누어 보고한다(로드맵 §11.1).

---

## 6. 확인하지 못한 것과 다음에 할 일

1. **SimRacer ISSTA'13 본문.** SimEvo가 사건·자원 모델을 위임한 [57]이다. ACM과 Wiley는 Cloudflare 도전으로 403이다. STVR 전문은 CPS-VO 사본으로 읽었으나 SimRacer 자체의 서술은 아니다. SimRacer 원문을 열어야 H-SE-1, §3의 Q1·Q2 칸을 더 정할 수 있다.
2. **RacePro [26]의 해당 절을 SimEvo 구현에 대응시키는 일.** RacePro는 커널 객체를 모델로 하고(§5.2) SimEvo가 이를 그대로 쓰는지는 서술이 없다.
3. **피인용 3건의 제목.** OpenAlex·Semantic Scholar 인용 조회가 429로 막혔다. 피인용 문헌이 SimEvo의 사용 가능성을 서술하는지 확인하지 못했다.
4. **`noggin.intel.com`의 Sim-OC 페이지.** 저자 프로젝트 페이지가 링크한 페이지이며 열지 못했다. SimEvo와의 관계는 불명이다.
5. **CodeSurfer의 현재 상태.** 판매·라이선스를 확인하지 못했다.
6. **학위논문(Yu 2014) 본문.** PDF를 시도하지 않았다.
7. **이슈 댓글.** 사례별 댓글은 읽지 않았다(`availability.md` A6). 이 문서의 사례 서술은 저장소 기록이다.
8. **저자 문의.** 하지 않았다(A2).

---

## 7. 출처 목록

원출처와 서지:
- T. Yu, *SimEvo: Testing Evolving Multi-Process Software Systems*, ICSME 2017, pp. 204–215, DOI 10.1109/ICSME.2017.29. 사본: https://par.nsf.gov/servlets/purl/10089644 (서지: https://par.nsf.gov/biblio/10089644, https://scholars.uky.edu/en/publications/simevo-testing-evolving-multi-process-software-systems/)
- T. Yu, W. Srisa-an, G. Rothermel, *An automated framework to support testing for process-level race conditions* (SIMEXPLORER), STVR 27(4–5), e1634, 2017, DOI 10.1002/stvr.1634. 사본: CPS-VO 아카이브 `archive.cps-vo.org/node/36388` (페이지 표기 Creative Commons 2.5)
- O. Laadan et al., *Pervasive Detection of Process Races in Deployed Systems*, SOSP 2011. https://www.cs.columbia.edu/~orenl/papers/sosp2011-racepro.pdf
- T. Yu, W. Srisa-an, G. Rothermel, *SimRacer*, ISSTA 2013, DOI 10.1145/2483760.2483771. 본문 미도달
- Intel Simics Simulator 공개 배포 페이지: https://www.intel.com/content/www/us/en/developer/articles/tool/simics-simulator.html (요약만 읽음)
- NSF 과제 CCF-1652149, CCF-1464032: `api.nsf.gov/services/v1/awards/`
- 저자 페이지: https://homepages.uc.edu/~tyu/links/research.html, https://homepages.uc.edu/~tyu/links/projects.html

저장소 문서(기록):
- `research/cfs_procrace/baselines/availability.md`, `baselines/yu-group.md`
- `research/cfs_procrace/plan/ROADMAP_20261015.md` §7.4, §8, §9, §10, §11
- `research/cfs_procrace/Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md` (P-PAPERS, T1, D22, P-R2CTRL)
- `research/cfs_procrace/cases/C1.md`, `C2.md`, `C3.md`, `C5.md`, `R2.md`
- `research/cfs_procrace/CONDITIONS_POLICY.md`
