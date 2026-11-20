# ReDPro (ACMSE 2022) 분석 — Kairos 비교용

- 작성일: 2026-10-15 (UTC 09:27–09:37, 경로 시도와 읽기 시각).
- 보완: 2026-10-16 (UTC 00:30–00:45). (1) 1차 출처 경로를 다시 시도했다(§1.1 둘째 표, 경로 28–41). 전문은 이번에도 읽지 못했다. 새로 확인한 것은 하나다. Internet Archive의 availability API가 ACM PDF 주소의 2022-05-07 스냅숏(기록 상태 200)이 있다고 답했다. 그러나 `web.archive.org`는 이 세션의 egress 정책이 막는다(HTTP는 프록시의 "Blocked by egress policy", HTTPS는 연결 재설정). 우회하지 않았고 스냅숏의 내용이 PDF인지도 확인하지 못했다(§4.3 D-RD-1 (e)). (2) 제1저자 ORCID의 작품 목록을 읽었다. ReDPro 항목은 ACM DOI 링크뿐이다. (3) 피인용 논문의 인용 문맥 목록을 다시 받아 SysPro의 ReDPro 문맥 두 곳(p.22, p.34–35)을 더했다(§2.3) `[2차]`. 바뀐 라벨은 없다. (4) Q6의 도구 수준 라벨("분석")과 사례별 라벨("불명")이 다른 이유를 §3.4에 적었다(`process_race_method_matrix.md` §2.5 항목 4가 지적한 차이). (5) R2의 수치 근거를 gdb 원시 기록(`env/logs/r2_g{1,2,3}_trace.jsonl`, `_summary.txt`)으로 바꾸고 다시 셌다. 값은 같다(§3.2). (6) 환경을 다시 측정했다(§4.1). 이번에도 ReDPro를 실행하지 않았다(구현이 없다).
- 대상: T. S. Zaman, T. Islam, *ReDPro: an automated technique to detect and regenerate process-level concurrency failures*, ACM SE '22 (2022 ACM Southeast Conference, Virtual Event), pp. 106–112. DOI 10.1145/3476883.3520207. 철자: ACM 서지와 ACMSE 프로그램은 "ReDPro", 초록 본문과 이 논문을 인용하는 논문들은 "RedPro" 또는 "Redpro"로 적는다. 같은 DOI다.
- 범위: 로드맵 §10 (M6)의 ReDPro 항목. 여섯 분석 질문(§10.2)을 사례 C1, C2, C3, C5, R2에 적용한다. 로드맵 §12 작업 6(`process_race_method_matrix.md`)의 입력으로 쓴다.
- 이 문서의 성격: **분석 문서**다. ReDPro를 실행하지 않았다(실행할 구현이 없다). 어느 사례에 대해서도 "탐지하지 못한다" 또는 "재생성하지 못한다"는 결과를 담지 않는다.
- **가장 중요한 제한: 논문 전문을 읽지 못했다.** 읽은 것은 초록(서지 집계 서비스가 돌려준 사본), 참고문헌 목록, 학회 프로그램, 저자 CV, 이 논문을 인용한 세 편의 해당 문장이다. 세부 방법·입력·관측 사건·평가 대상은 전부 `[미도달]`이거나 2차 설명이다(§1).
- 용어: 신규 문서이므로 T1 (a)를 따른다. 판정 대상은 readiness specification(준비 상태 명세), 그 전제는 readiness precondition, 위반은 readiness violation이다. "API contract"는 문서화된 API 의미를 가리킬 때만 쓴다.
- 조건 원칙: `CONDITIONS_POLICY.md`. 이 문서는 실험 조건을 정하지 않는다. 레퍼런스가 없는 값은 `unspecified_by_reference`로 둔다.
- PI 결정(`PI_DECISIONS_RESOLVED_20261015.md`)은 결정으로만 인용한다. 사실로 쓰지 않는다. 이 문서가 쓰는 결정은 P-PAPERS와 T1이다.
- CORD 결과는 근거로 쓰지 않는다. R2의 실행 근거는 gdb 원시 기록(`env/logs/r2_g1..3_*`, `results/R2_gdb_v701.md`)뿐이다.

## 증거 태그

| 태그 | 뜻 | 이 문서에서의 쓰임 |
|---|---|---|
| [F] | 인용이 있는 사실 | 원출처(초록, 서지, 인용 논문의 쪽) 또는 저장소 기록(문서·절). 저장소 기록은 "기록"이라고 표시하며 이번에 재검증하지 않았다 |
| [M] | 측정 | 오늘(2026-10-15) 이 컨테이너에서 읽기 전용으로 확인한 값, 또는 "기록"이 붙은 이전 측정 |
| [I] | 추론·분석 | 읽은 문장에서 도출한 해석. 도구를 실행하지 않았다 |
| [D] | 결정 | PI 결정 |
| [U] | 미확인 | 자료가 없거나 읽지 못함 |

문장마다 출처 등급을 아래 여섯 가지 중 하나로 붙인다.

| 등급 | 뜻 |
|---|---|
| `[전문]` | 본문 전체를 읽었다. **ReDPro에는 해당 없음** |
| `[초록]` | 초록만 읽었다. ACM 페이지 자체는 열지 못했고, 서지 집계 서비스(Semantic Scholar, OpenAlex)가 돌려준 초록 사본이다 |
| `[2차]` | 다른 논문·페이지가 이 도구를 서술한 문장을 읽었다. 도구 자신의 서술이 아니다 |
| `[서지]` | 제목, 저자, 쪽, 참고문헌 목록, 프로그램 같은 서지 정보만 읽었다. 논문이 무엇을 말하는지는 알려주지 않는다 |
| `[기록]` | 저장소의 이전 보고서에 있는 값. 이번에 재검증하지 않았다 |
| `[미도달]` | 열지 못했다 |

쪽 번호 표기: **SysPro p.N**은 arXiv 2601.09616v1 PDF의 쪽(47쪽, 인쇄 쪽과 같다), **Gender p.N**은 arXiv 2502.20289 PDF의 쪽(7쪽)이다.

---

## 0. 한눈에 보기

1. **논문을 찾았고 서지는 확정되었다. 전문은 오늘도 열리지 않았다.** ACM DL은 Cloudflare 도전 페이지(`cf-mitigated: challenge`, HTTP 403)로 막혔다. 도전 페이지를 우회하려고 하지 않았다. Unpaywall, OpenAlex, Semantic Scholar는 모두 사본이 없다(closed)고 답했다. 저자 홈페이지, NSF PAR, arXiv, Internet Archive 검색에도 사본이 없다. 경로 27개와 결과는 §1.1에 있다. `[미도달]` 2026-10-16에 경로 14개를 더 시도했다(§1.1 둘째 표). 결과는 같고, 남은 단서는 Internet Archive 스냅숏 하나다. 이 세션에서는 `web.archive.org`가 egress 정책으로 막혀 있다.
2. **초록이 말하는 것은 네 가지다.** (a) process-level 실패는 여러 process가 공유 데이터를 동기화 없이 읽고 쓸 때 생기며 system call 실행 순서에 민감하다. (b) RedPro는 process-level race를 탐지하고 그 실패를 다시 만들어낸다(regenerate). (c) binary instrumentation 도구 PIN으로 run-time 감시와 "potential race pairs"의 실행 순서 제어를 한다. (d) 실제 버그 9건으로 평가했고 effective and efficient하다고 결론짓는다. 이 이상의 방법은 읽지 못했다. `[초록]`
3. **역할은 초록 수준에서 "탐지 + 실패 재생성"이다.** 그러나 로드맵 §4.6과 §10.1은 원문 확인 전 탐지·재현 표의 확정 행으로 채우지 않는다고 정한다. 이 문서도 어느 역할 표에도 ReDPro를 넣지 않는다. 같은 저자(Zaman)가 쓴 2025년 논문이 ReDPro를 process-level 버그 재현 연구 표에 넣고 "Dynamic Binary Analysis"로 분류하지만(Gender p.3), 이것도 2차 설명이다. `[2차]`
4. **다섯 사례 × 여섯 질문 30칸 중 26칸이 "불명", 4칸이 "분석"(Q5), "답"은 0칸이다.** 읽은 자료가 관측 사건의 종류, 자원 식별 방식, 입력, 출력 형식을 하나도 밝히지 않는다. "분석" 4칸도 초록의 "order 제어와 failure 재생성 단계가 있다"는 설계 진술에 근거한 조건부 서술이다(§3.4). 어느 칸도 탐지 실패를 뜻하지 않는다.
5. **초록의 문제 정의는 system call 순서이고 PIN이 무엇을 계측했는지는 초록에 없다.** cFS 사례 C1·C3·C5·R2의 공개와 사용은 사용자 공간 메모리 쓰기와 읽기이고, OSAL 자체 시험에서 C1의 생성과 이름 조회는 syscall 0건이었다 `[기록]`. 두 사실을 연결하는 것은 **조건부 가설**일 뿐이다(§5.2 H-RD-1). PIN은 instruction 단위 메모리 계측도 지원하므로(Pin 3.21 user guide), "PIN을 쓴다"는 문장만으로 syscall만 본다고 단정할 수 없다. `[I]`
6. **실행은 불가능하고 오늘 적용되는 상태는 다섯 사례 모두 `NOT_RUN`이다.** 주된 원인은 환경이 아니라 자료다(환경 쪽 요인은 PIN 미설치와 동작 미확인뿐이다). 구현을 찾지 못했고(GitHub 저장소·코드·사용자 검색), 전문을 읽지 못했으며, PIN은 이 컨테이너에 설치되어 있지 않고 쓰인 버전도 알 수 없다. `INAPPLICABLE`은 원문의 범위 서술 위치를 보고해야 하므로 지금은 붙일 수 없다(§4.2).
7. **Kairos 가설은 근거가 있는 만큼만 올렸다(§5).** ReDPro에 부족한 것으로 확인된 사항은 없다. 확인된 것은 "확인하지 못했다"뿐이다. 조건부 가설 H-RD-1(내부 API·상태 사건)과 정의 차이 H-RD-2(실행 단위)만 올렸고, 나머지 로드맵 §10.5 행은 근거가 없어 올리지 않았다. order 제어와 failure 재생성 단계는 초록에 이미 있으므로 차별점으로 쓰지 않는다.

---

## 1. 접근한 출처와 문장별 근거 등급

### 1.1 시도한 경로와 결과

첫째 표의 시각은 2026-10-15 UTC 09:27–09:37이다. 접속 실패는 원인을 기록한 것까지만 썼고 TLS 검증을 끄거나 프록시를 우회하지 않았으며 도전 페이지(Cloudflare, Anubis)를 풀려고 하지 않았다. 같은 호스트의 반복 재시도는 속도 제한(429) 때만 한 번 했다.

| # | 경로 | 결과 | 등급·비고 |
|---|---|---|---|
| 1 | `https://dl.acm.org/doi/10.1145/3476883.3520207` (curl) | HTTP 403, `cf-mitigated: challenge` | `[미도달]` 전문 |
| 2 | `https://dl.acm.org/doi/pdf/10.1145/3476883.3520207` (curl) | HTTP 403, 같은 도전 | `[미도달]` |
| 3 | `https://doi.org/10.1145/3476883.3520207` | 302로 ACM 이동 후 같은 403 | `[미도달]` |
| 4 | 같은 ACM 페이지를 WebFetch로 | HTTP 403 | `[미도달]` |
| 5 | ACM 학회 목차 `dl.acm.org/doi/proceedings/10.1145/3476883` | HTTP 403, 같은 도전 | `[미도달]` |
| 6 | Crossref `api.crossref.org/works/10.1145/3476883.3520207` | 200. 제목 "ReDPro", 부제 "an automated technique to detect and regenerate process-level concurrency failures", 저자 Zaman(University of Kentucky)과 Islam(Syracuse University), 쪽 106–112, 2022-04-18, 참고문헌 24건. 초록 없음. 전문 링크는 `dl.acm.org`뿐 | `[서지]` |
| 7 | OpenAlex `api.openalex.org/works/doi:10.1145/3476883.3520207` | 200. `is_oa: false`, `oa_status: closed`, 사본 위치는 ACM 하나. 초록은 앞 다섯 문장만 있다(뒤 네 문장 없음: 방법·평가를 서술하는 부분) | `[서지]` + `[초록]` 일부 |
| 8 | Semantic Scholar Graph API (같은 DOI) | 200. **초록 전문 사본**, `openAccessPdf.status: CLOSED`, DBLP 키 `conf/ACMse/ZamanI22`, 제목 표기 "ReDPro: an automated technique to detect and regenerate process-level concurrency failures" | `[초록]` 이 문서의 초록 인용 기준. ACM 원 페이지와는 대조하지 못했다 |
| 9 | Unpaywall `api.unpaywall.org/v2/…` | 200. `is_oa: false`, `oa_locations: []`, `has_repository_copy: false` | 사본 없음 |
| 10 | Semantic Scholar `…/citations` | 첫 호출 429(속도 제한), 20초 뒤 200. 인용 논문 4편: SysPro(2026), LLPut(2025), Gender Dynamics(2025), 2026년 NLP 디버깅 체계적 문헌 연구(문맥 문장 없음) | `[2차]` 후보 목록 |
| 11 | ACMSE 2022 프로그램 `acmse.net/2022/schedule-papers/` | 200. Session 5 (Software Engineering), 2022-04-19 10:00–11:50 CT, Room 2. ReDPro는 10:00–10:25, Full Paper, 제목 "ReDPro: An Automated Technique to Detect and Regenerate Process-level Concurrency Failures", 저자 Tarannum Zaman, Tariqul Islam | `[서지]` 초록·PDF 링크 없음 |
| 12 | 제1저자 홈 `tarannumzaman.github.io/` 와 `/publications/`, `/cv/`, `/portfolio/`, `/riselab/`, `/sitemap/` | 모두 200. 논문 목록(`/publications/`)에는 2014–2019년 5편만 있고 **ReDPro가 없다**. 소개문에 "Ph.D. 2022, Dr. Tingting Yu 지도"가 있다 | `[서지]` |
| 13 | 제1저자 CV `tarannumzaman.github.io/files/CV_zaman.pdf` (LaTeX, 2025-03-24 생성) | 200, sha256 `bf2ad801c0e16d79ebaa2ca0e6816886dc718aed3e6d951edaa9212ef0767182`. 문헌 [14]로 ReDPro를 싣고 "7 pages (acceptance rate: 35%)"라고 적는다. 2022-04 ACMSE에서 발표했다고 적는다. PDF 링크나 코드 링크는 없다. 학위논문 제목은 "An Automated Framework to Debug System-Level Concurrency Failures" | `[서지]` (저자의 자기 서술, 수치는 재검증하지 않았다) |
| 14 | 제1저자 학위논문 UKnowledge `uknowledge.uky.edu/cs_etds/119` (curl, WebFetch) | 레코드 페이지 200. 초록은 세 연구(로그 기반 재현, system call trace 기반 localization, bug report 기반 localization)를 말하며 **ReDPro나 process-level race 탐지를 언급하지 않는다**. PDF `viewcontent.cgi?article=1125&context=cs_etds`는 403(Cloudflare 도전) | 학위논문에 ReDPro 장이 있는지는 알 수 없다. 본문 `[미도달]` |
| 15 | 제2저자 쪽: GitHub 사용자 검색 "Tariqul Islam Syracuse" → `mtipavel`, 그 홈 `mtipavel.github.io`("Md. Tariqul Islam 'Pavel', Assistant Professor, Syracuse University") | 홈·publications·cv는 200이나 academicpages 템플릿 문구뿐이고 ReDPro·race·concurrency 키워드가 없다. 동일인 확인은 이름과 소속의 일치뿐이다. 이 사용자의 저장소 2개는 템플릿 사이트와 빈 `test`다 | 구현·사본 없음 |
| 16 | GitHub 저장소 검색 `ReDPro` | 271건. 이름만 비슷한 무관한 프로젝트(RedProtect, redproxy 등)이고 race 도구는 없다. 목록의 앞 25건만 확인했다 | 구현 없음 |
| 17 | GitHub 저장소 검색 `user:tarannumzaman` | 1건: `tarannumzaman/Scminer`(SCMiner). ReDPro 저장소 없음. GitHub 사용자 검색 "Tarannum Zaman"은 `tarannumzaman` 1명 | 구현 없음 |
| 18 | GitHub 코드 검색 `"ReDPro" pintool race`, 저장소 검색 `process-level race pintool system call order tarannum OR tariqul` | 코드 검색 195건(무관한 파일: 에디터, 프록시 목록 등), 저장소 검색 0건 | 구현 없음 |
| 19 | NSF PAR `par.nsf.gov/search/term:ReDPro` | 200, 총 0건. 저자 검색 "Tarannum Zaman"은 6건(SysPro, SeCite, LLPut, Gender Dynamics, SCMiner, DESCRY)이며 ReDPro 없음 | 사본 없음 |
| 20 | arXiv API `all:ReDPro` | 0건 | 사본 없음 |
| 21 | Internet Archive `advancedsearch.php?q=ReDPro`, Wayback `web.archive.org/web/…`, Wayback availability API, `scholar.archive.org` | 검색 `numFound: 0`; Wayback는 `Connection reset by peer`; availability API는 429; `scholar.archive.org`는 속도 제한 페이지(재시도하지 않았다) | `[미도달]` |
| 22 | DBLP `dblp.org/search/publ/api`, 미러 `dblp.uni-trier.de/rec/conf/ACMse/ZamanI22.html` | Anubis 봇 확인 페이지. 통과하지 않았다 | 서지는 Semantic Scholar가 DBLP 키를 알려줬다 |
| 23 | Google Scholar 검색, ResearchGate 검색·제1저자 프로필, CORE HTML 검색, Syracuse SURFACE 검색, UKnowledge 검색 | 각각 403(차단 또는 도전). CORE API(`api.core.ac.uk/v3`)는 200이나 `ReDPro` 제목 3건이 모두 무관(독일어 프로젝트 보고서, NMR 단백질 논문) | `[미도달]` / 사본 없음 |
| 24 | Scholar Gateway `semanticSearch` | `ACCESS_DENIED: Access denied`(2026-10-08 기록과 같다) | `[미도달]` |
| 25 | WebSearch (6회: extended 2회, 나머지 standard 계열) | 새 1차 출처 없음. 결과에 ACM DL 주소와 인용 논문들이 나온다. 한 요약이 ReDPro의 근거로 든 ResearchGate 그림 항목(`…fig2_318868662`)은 **DESCRY의 ResearchGate 항목**(ID 318868662)이다. 이 문서는 그 요약의 `mv`/`tail` 서술을 ReDPro 근거로 쓰지 않는다 | — |
| 26 | 이 논문을 인용한 세 편의 전문 PDF: SysPro `arxiv.org/pdf/2601.09616`(47쪽, sha256 `8fe5daa4c7c3794715632e4379d9160a9852b723dc4a649317ef6bae0a75e726`), Gender Dynamics `arxiv.org/pdf/2502.20289`(7쪽, sha256 `5acf7a35cc4a85669b9cc68f92a6a997c429fb13053f3a99ec0f2f2fd85d7543`), LLPut `arxiv.org/pdf/2503.20578v3`(8쪽, sha256 `1eeebf3fcb3ae959d6c0af1472756b6770949e091429a2064ad91a4355735877`) | 모두 200. ReDPro를 언급한 문장만 읽었다(§2.3) | `[2차]` |
| 27 | Pin 3.21 User Guide `software.intel.com/sites/landingpage/pintool/docs/98484/Pin/html/` (ReDPro 참고문헌 항목이 적은 바로 그 주소), Intel Pin 소개 페이지 | 둘 다 200. User guide sha256 `b01a9d692fed96323ed1775b5c5313b3639a854bdd4c27839262b3e9f4d6316b`. **ReDPro의 문장이 아니라 PIN의 일반 문서다.** 맥락용으로만 읽었다 | 맥락(§2.2, §4.1) |

**둘째 표: 2026-10-16 재시도** (UTC 00:30–00:45). 요청 방식은 첫날과 같다. 도전 페이지와 egress 차단을 우회하지 않았고, 막힌 주소를 다른 도구(WebFetch 등)로 다시 열지도 않았다. 일부 curl 호출에는 일반 브라우저형 `User-Agent` 문자열을 처음부터 붙였다. 거부된 뒤에 문자열을 바꾼 일은 없다.

| # | 경로 | 결과 | 등급·비고 |
|---|---|---|---|
| 28 | ACM `doi/…`, `doi/pdf/…`, `doi/epdf/…`, `doi/abs/…` (curl) | 모두 HTTP 403, 응답 헤더 `cf-mitigated: challenge`, `server: cloudflare` | `[미도달]`. 2026-10-15와 같다 |
| 29 | 학위논문 PDF `uknowledge.uky.edu/cgi/viewcontent.cgi?article=1125&context=cs_etds` | 403, Cloudflare 도전("Just a moment...")이다. 레코드 페이지 `cs_etds/119/`는 200이고 다운로드 링크는 이 주소 하나다 | `[미도달]` |
| 30 | OpenAlex(같은 DOI) 다시 받음 | 200. `oa_status: closed`, `any_repository_has_fulltext: false`, 위치는 `doi.org` 하나다. 초록 역색인을 펼치면 앞 다섯 문장뿐이다(첫날 기록과 같다) | `[서지]` |
| 31 | Unpaywall 다시 받음 | 200. `is_oa: false`, `oa_locations: []` | 사본 없음 |
| 32 | Semantic Scholar Graph API 다시 받음 | 200. 초록 아홉 문장이 첫날 사본과 글자 단위로 같다. `openAccessPdf.status: CLOSED`, `citationCount: 4` | `[초록]` |
| 33 | Semantic Scholar `…/citations?fields=contexts` | 첫 호출 429, 다음 호출 200. 인용 논문 4편과 각 문맥 문장. SysPro 문맥 7개(첫날 읽은 5곳에 p.22와 p.34의 2곳이 더해짐). SLR(DBLP 키 `journals/scp/LotaZAFCAIK27`, *Science of Computer Programming* 2026, DOI `10.1016/j.scico.2026.103552`)은 문맥 0개 | `[2차]` 후보. SysPro 문장은 PDF에서 다시 찾았다(행 34) |
| 34 | SysPro `arxiv.org/pdf/2601.09616v1` 다시 받음 | 200, sha256 `8fe5daa4…`로 첫날과 같다. `arxiv.org/abs/2601.09616`의 제출 이력은 v1뿐이다 | `[2차]` §2.3 |
| 35 | 제1저자 CV 다시 받음 | 200, sha256이 첫날과 같다(`bf2ad801…`). 현 소속은 University of Maryland, Baltimore County(2025-01–), 그 전은 SUNY Polytechnic Institute(2022-08 – 2024-12)라고 적는다 | `[서지]`. 새 기관 저장소 후보를 찾는 데만 썼다 |
| 36 | ORCID 공개 API: 이름 검색 2건 → `0000-0002-8634-524X`(이름 "Tarannum Shaila Zaman")의 작품 목록 | 200. 22건 중 ReDPro 항목의 링크는 `dx.doi.org/10.1145/3476883.3520207` 하나다. 같은 목록에 SysPro의 JSS판(`10.1016/j.jss.2026.112785`)과 위 SLR이 있다 | `[서지]`. 사본 없음 |
| 37 | UMBC 기관 저장소 MDSOAR(`mdsoar.org`)의 검색 API와 검색 페이지, UMBC 학과 홈 | 검색 API 경로는 404(경로가 맞지 않음), 검색 페이지는 400. 학과 홈은 200이나 저자 이름이 나오지 않는다 | 사본 찾지 못함. 경로 오류 가능성이 있어 "없다"로 쓰지 않는다 |
| 38 | University of Kentucky 연구자 포털 `scholars.uky.edu` 검색 | 403, Cloudflare 도전 | `[미도달]` |
| 39 | CPS-VO `archive.cps-vo.org` 저자 목록·검색 | 저자 목록은 빈 응답(curl 52)이 두 번, 검색은 "Request Rejected". 다른 도구로 다시 열지 않았다(`process_race_method_matrix.md` §5.4 항목 16이 이 호스트의 취득 방식을 PI 사안으로 둔다) | `[미도달]` |
| 40 | Internet Archive: availability API `archive.org/wayback/available?url=dl.acm.org/doi/pdf/10.1145/3476883.3520207` → 스냅숏 `20220507074916`, 기록 상태 200. 그 스냅숏을 `web.archive.org`에서 받기(`id_` 형식 포함) | availability API는 200. 스냅숏 주소는 HTTPS에서 연결 재설정, HTTP에서 프록시의 403 "Blocked by egress policy". 프록시 상태 페이지도 `web.archive.org:443` 거부를 기록한다 | **단서.** 스냅숏이 실제 PDF인지 도전 페이지인지 알 수 없다(기록 상태 200만 안다). `[미도달]` |
| 41 | `scholar.archive.org` 검색, BASE(`base-search.net`) 검색, arXiv API(`export.arxiv.org`), WebSearch 3회(standard), SLR의 DOI | 각각 "Session Verification" 페이지, 봇 확인 페이지, 시간 초과(HTTPS 25초, HTTP는 301). WebSearch는 새 1차 출처 없음. SLR DOI는 Elsevier `linkinghub`로 이동하고 본문은 열지 않았다. WebSearch로 SLR의 arXiv판을 찾지 못했다 | `[미도달]` |

2026-10-08 기록(`baselines/availability.md` A1, `baselines/yu-group.md` D1)은 `dl.acm.org`, `acmse.net`, `ischool.syracuse.edu` 등을 세션의 egress 정책이 막았다고 적었다. 오늘 ACM의 거부는 원 서버의 Cloudflare 도전이고, `acmse.net`과 `ischool.syracuse.edu` 홈은 열린다(후자의 ReDPro 항목은 찾지 못했다). 날짜에 따라 원인이 다르므로 두 기록을 합쳐 읽지 않는다.

### 1.2 이 문서가 쓰는 출처별 등급

| 출처 | 등급 | 이 문서에서의 쓰임 |
|---|---|---|
| ReDPro 논문 본문 (ACMSE'22, 7쪽) | **`[미도달]`** | 없음. 방법, 입력, 관측 사건, 평가 대상은 읽지 못했다 |
| ReDPro 초록 (Semantic Scholar 사본, OpenAlex 앞 다섯 문장과 일치) | **`[초록]`** | §2.1의 역할·단위·감시·제어·평가 진술 |
| Crossref의 참고문헌 24건, 서지 | `[서지]` | §2.4. 인용 목록일 뿐 논문이 무엇을 말하는지는 아니다 |
| ACMSE 2022 프로그램, 제1저자 CV | `[서지]` | 서지 확인, 발표 사실 |
| SysPro arXiv 2601.09616v1 (제1저자가 ReDPro 제1저자와 같다) | `[2차]` | ReDPro를 서술한 문장 4곳 (§2.3) |
| Gender Dynamics arXiv 2502.20289 (제1저자가 ReDPro 제1저자와 같다) | `[2차]` | Table III의 분류 한 줄 |
| LLPut arXiv 2503.20578v3 (저자에 Zaman 포함) | `[2차]` | ReDPro 인용 문맥 한 곳. 내용을 서술하지 않는다 |
| Pin 3.21 User Guide | 벤더 문서(오늘). ReDPro의 문장 아님 | PIN의 일반 기능 맥락 |
| 사례 쪽 사실 | `[기록]` | `cases/ledger.md` §1a, §1b, §2; `baselines/availability.md` §4.2; 로드맵 §2.2 |
| OSAL probe 값 | `[기록]` | `baselines/availability.md` §4.2. 이번에 재측정하지 않았고, OSAL 자체 시험의 probe이지 ReDPro의 결과가 아니다 |

세 인용 논문의 저자에 ReDPro 제1저자가 들어 있다. 그래도 이 문장들은 ReDPro 논문 자신의 서술이 아니므로 `[2차]`로 둔다.

### 1.3 이전 기록과 달라지거나 확정된 것

| # | 항목 | 내용 |
|---|---|---|
| 1 | `yu-group.md` §1, `availability.md` §1 "ReDPro: 구현 없음" | 오늘도 구현을 찾지 못했다(경로 12–21). 결론은 같다. "접근 가능한 출처에서 못 찾았다"이지 "없다"가 아니다 |
| 2 | `yu-group.md` §3.2의 "ACM DL excerpt" | 같은 문장들이 초록 사본에서 확인된다(경로 8). 초록 안에서는 "PIN"이 방법의 유일한 기술 요소다. 방법의 세부는 초록에 없다 |
| 3 | `yu-group.md` R11 "Intel Pin (version unknown)" | 초록에서 PIN 사용이 확인된다. 버전은 여전히 불명이다. ReDPro의 참고문헌 항목에 "Pin 3.21 User Guide"가 있으나 이것은 인용 목록이지 사용 버전의 서술이 아니다(§2.4) |
| 4 | 서지 | 저자, 소속, 쪽, 정식 제목, 발표 일정이 확정되었다. 이전 기록에는 "ACMSE schedule"과 "ischool.syracuse.edu"가 열리지 않아 없었다 |
| 5 | 로드맵 §4.6·[P8], §10.1 "ReDPro: 역할·세부 모델 미확인" | 유지된다. 오늘의 새 근거는 초록과 2차 설명뿐이고 전문이 없으므로 확정 행으로 채우지 않는다 |
| 6 | 역할 | 초록은 "detect process-level races and also regenerate the failures"라고 한다. 이 문장은 역할 표에 넣는 근거가 아니라 §2.1의 기술 근거다 |
| 7 | 경로 21의 Wayback availability API(2026-10-15에 429) | 2026-10-16에 200으로 답했고 ACM PDF 주소의 스냅숏이 있다고 알렸다(경로 40). 스냅숏 자체는 egress 정책 때문에 열지 못했다. 첫날의 "Internet Archive에 사본이 없다"는 서술은 전문 검색(`advancedsearch`)의 결과이고, 웹 보관본이 없다는 뜻으로 읽지 않는다 |

---

## 2. 도구의 정체

### 2.1 요약

모든 칸은 **초록 수준**이거나 2차 설명이다. 전문을 읽지 않았으므로 "무엇을 하지 않는다"는 서술은 하나도 없다.

| 항목 | 읽은 것 | 태그 | 등급·근거 |
|---|---|---|---|
| 역할 | 초록: "automated technique, RedPro that can detect process-level races and also regenerate the failures." SysPro p.35: "detects system-level race conditions through runtime monitoring and then regenerates the race condition by instrumenting the code". Gender p.3 Table III는 ReDPro를 process-level 버그 재현 연구 표에 두고 type "System-level races", technique "Dynamic Binary Analysis"로 적는다. SysPro pp.34–35는 ReDPro를 "automatically reproduce such bugs" 하는 접근들의 인용 묶음에 넣는다(2026-10-16 추가) | [F] | `[초록]`, `[2차]`. 로드맵 역할 분류(탐지/재현/회귀)로의 확정은 전문 확인 뒤(§4.2, §4.3) |
| 실행 단위 | 초록: "multiple processes read and write a shared data item without proper synchronization". 제목: process-level. 인용 논문 둘은 "system-level"이라 적는다 | [F] | `[초록]`, `[2차]`. 논문이 process를 OS process로 한정하는지, 같은 process의 thread 사이도 포함하는지는 [U] |
| 무엇을 관측하는가 | 초록: PIN으로 "run-time monitoring". 감시하는 사건의 종류(syscall, 메모리 접근, 함수 호출)는 쓰여 있지 않다. 문제 정의 문장은 "sensitive to the execution order of system calls"이고 "specific system-call interleaving"을 언급한다 | [F] / [U] | `[초록]`. 사건 종류는 [U] |
| 자원 식별 | 초록은 "a shared data item"이라고만 한다. 이름·ID·경로·세대로 식별하는지는 쓰여 있지 않다 | [U] | `[미도달]` |
| 모델링하는 순서·의존 | 초록: "potential race pairs"의 실행 순서를 제어한다. "potential race pair"의 정의, happens-before 규칙, 생성·초기화 완료 같은 readiness precondition의 표현 여부는 쓰여 있지 않다 | [F] / [U] | `[초록]`. 정의는 [U] |
| 입력 | 초록은 입력을 말하지 않는다. SysPro p.32는 ReDPro를 인용 [11, 13, 42, 29]의 한 항목으로 묶어 이런 작업들이 "assume that inputs are known to developers"라고 적는다. 이것은 테스트 입력(실행 입력)에 관한 문장이지 필요한 순서나 허용 오류 입력에 관한 문장이 아니다 | [F] / [U] | `[2차]`. 필요 순서·허용 오류·oracle 입력은 [U] |
| 출력 | 초록: race 탐지와 failure 재생성. 출력의 형식(후보 목록, 보고서, 재생성 성공 표시)은 쓰여 있지 않다 | [F] / [U] | `[초록]` |
| 제어·확인 | 초록: PIN으로 "controlling the order of execution for potential race pairs", failure를 "regenerate". 즉 순서를 바꾸는 단계와 실제 실행에서 failure를 다시 만드는 단계가 설계에 있다고 서술한다. 순서를 어떻게 바꾸는지(예: 지연 삽입, 동기화)는 [U]. SysPro p.35는 "instrumenting the code"라고 적는다 | [F] / [U] | `[초록]`, `[2차]` |
| 구현·플랫폼 | PIN(초록). 버전은 쓰여 있지 않다. 참고문헌 목록에 Pin 3.21 User Guide와 Pin 논문(2005, ACM SIGPLAN Notices)이 있다 | [F] / [U] | `[초록]`, `[서지]`. 사용 버전은 `unspecified_by_reference`. 공개 구현 위치는 못 찾았다 |
| 평가 | 초록: "nine real-world bugs", "both effective and efficient". 어떤 버그인지, 무엇으로 effective·efficient를 쟀는지는 [U]. 참고문헌에 Red Hat Bugzilla bug 438076("mv creates race condition by calling unlink() before rename()")이 있으나 평가 대상 9건에 속하는지는 [U] | [F] / [U] | `[초록]`, `[서지]` |
| 분량·채택 | 7쪽, 채택률 35%(저자 CV의 서술) | [F] | `[서지]`. 재검증하지 않았다 |

### 2.2 읽은 문장이 말하지 않는 것, 그리고 PIN의 일반 기능

- 초록은 감시 대상을 syscall로 한정하지 않는다. "execution order of system calls"는 failure가 **무엇에 민감한가**를 말하는 문제 정의 문장이고, PIN이 **무엇을 계측했는가**의 문장이 아니다. `[F]` `[초록]`
- Pin 3.21 User Guide는 PIN을 "a tool for the instrumentation of programs"라고 소개하고, 도구가 임의 위치에 임의 코드를 넣을 수 있으며 실행 중에 추가된다고 설명한다. 안내서의 예제는 instruction 단위 계측과 메모리 피연산자 계측(`INS_IsMemoryRead`, `IARG_MEMORYREAD_EA`), routine 단위 계측(`RTN_`)을 싣는다. 이것은 **PIN의 일반 기능**이며 ReDPro가 무엇을 계측했는지에 대한 증거가 아니다. 이 사실이 뒷받침하는 것은 "PIN을 쓴다는 문장으로는 syscall만 보는 설계와 메모리 접근까지 보는 설계를 구분할 수 없다"는 것뿐이다. `[F]` 벤더 문서, `[I]` 해석
- 이 문서는 안내서에서 syscall 계측 API를 찾아 확인하지 않았다(본문 페이지에서 해당 이름이 검색되지 않았을 뿐이고 API 참조 문서는 열지 않았다). ReDPro가 syscall 진입을 계측했는지에 대해 어떤 주장도 하지 않는다. `[U]`

### 2.3 인용하는 논문들이 ReDPro를 서술한 문장 (모두 `[2차]`)

| 출처 | 쪽 | 문장(짧게) | 읽을 때 주의 |
|---|---|---|---|
| SysPro (Zaman 외) | p.35 §7 | ReDPro "detects system-level race conditions through runtime monitoring and then regenerates the race condition by instrumenting the code" | ReDPro 논문의 용어는 process-level이고 이 문장은 system-level이라 적는다. "instrumenting the code"가 소스 수정인지 binary 계측인지는 이 문장으로 알 수 없다 |
| SysPro | p.8 §2.3 | PIN 계측에 관해 "Previous research works [26, 29] provide a detailed explanation of how PINtool facilitates instrumentation" ([26]은 SCMiner, [29]는 ReDPro) | ReDPro 본문에 PINtool 계측 설명이 있다는 뜻으로 읽힌다. 내용은 알 수 없다 |
| SysPro | pp.21–22 §4.1 | 벤치마크는 선행 연구가 "examined and confirmed"한 것만 쓴다 ([26, 12, 11, 29]) | ReDPro의 9건이 SysPro의 24건과 겹치는지는 알 수 없다 |
| SysPro | p.32 §5 | "Many related works [11, 13, 42, 29] assume that inputs are known to developers" | ReDPro가 실행 입력을 개발자가 안다고 가정한다는 뜻으로 읽힌다. 필요 순서나 허용 오류를 입력으로 받는다는 뜻이 아니다 |
| SysPro | p.34 §6 | 버그 보고서에서 키워드 검색과 수작업 검토로 대상 버그를 고르는 방법이 "successfully applied in prior studies [26, 12, 9, 29]"라고 적는다 | ReDPro가 평가 대상 버그를 버그 트래커에서 같은 방식으로 골랐다는 뜻으로 읽힐 수 있다. 확정할 수 없다. ReDPro의 참고문헌에 Bugzilla, Debian, GNU 버그 트래커가 있다(§2.4)는 점과 겹쳐 보일 뿐이다 |
| SysPro (2026-10-16 추가) | p.22 §4.1 | SysPro의 정답(ground truth)을 정할 때 버그 보고 댓글, patch, 개발자 논의와 함께 "earlier research works [26, 12, 11, 29]"를 수작업으로 살폈다고 적는다. 같은 쪽과 p.23은 DESCRY 21건, SCMiner, RRF 7건의 벤치마크 수를 들지만 ReDPro의 수는 들지 않는다 | SysPro 벤치마크와 ReDPro 평가 대상이 겹칠 수 있다는 정도로만 읽힌다. ReDPro 9건의 목록은 여전히 [U] |
| SysPro (2026-10-16 추가) | pp.34–35 §7 | "Various approaches[11, 12, 29, 56] have been proposed to automatically reproduce such bugs." 이어서 이들 중 어느 것도 버그 보고서를 "the primary input"으로 삼는 데 집중하지 않는다고 적는다 | SysPro 저자들이 ReDPro를 자동 **재현** 계열로 묶는다는 2차 서술이다. 로드맵 §10.1의 역할 확정 근거로 쓰지 않는다. 입력이 버그 보고서가 아니라는 부정 서술에서 ReDPro의 실제 입력이 무엇인지는 알 수 없다 |
| Gender Dynamics (Zaman 외) | p.3 Table III | RedPro, 2022, type "System-level races", technique "Dynamic Binary Analysis" | 연구 분류표의 한 줄. 사건·자원 모델을 서술하지 않는다 |
| LLPut (Zaman 포함) | p.1 | "To reproduce a bug, developers need the failure-inducing input [60, 61]" ([61]이 ReDPro) | 일반 배경 문장에 붙은 인용이다. ReDPro의 내용을 서술하지 않는다 |

### 2.4 참고문헌 목록에서 읽을 수 있는 것 (`[서지]`)

Crossref가 등록한 24건(키 `e_1_3_2_1_14_1`이 빠져 있다)을 읽었다. 인용 목록이므로 논문이 이들에 대해 무엇을 말하는지는 알 수 없다.

- **Process-level race 관련 선행 연구:** RacePro(SOSP 2011), SimRacer(ISSTA 2013), DESCRY(FSE 2017), SCMiner(ASE 2019), OS-level 모델 검사 HotOS 2011 논문(Laadan 외), ODR(SOSP 2009). RRF, SIMEXPLORER, SimEvo는 Crossref에 등록된 항목에 없다.
- **PIN:** "Pin 3.21 User Guide"(Intel) 웹 문서와 Pin 논문(2005, ACM SIGPLAN Notices, `10.1145/1064978.1065034`). 3.21은 인용한 안내서의 버전이다.
- **버그 출처:** Red Hat Bugzilla 첫 화면, Debian Bug Tracking System, GNU Bug Tracker, Red Hat Bugzilla bug 438076(`mv`의 unlink-before-rename). 평가 9건이 이 출처들에서 왔을 가능성은 있으나 인용 목록만으로 알 수 없다. `[U]`
- **그 밖:** stress testing 웹 문서 두 건, Iterative context bounding, Isolating failure-inducing thread schedules, Locating causes of program failures, What are race conditions, Falcon, ConSeq, DRACULA와 signal handler race 레이블링 논문, 제목을 확인하지 못한 DOI 하나(`10.5555/1364813.1364826`, Crossref가 404를 돌려줬다).

---

## 3. 여섯 질문 × 다섯 사례

### 3.1 읽는 법

셀의 라벨은 아래 셋 중 하나다. 한 칸에 관측 가능성, 표현 가능성, 실제 탐지 성공을 합치지 않는다(로드맵 §10.2).

| 라벨 | 뜻 |
|---|---|
| **답** | 출처의 쪽·절로 답했다 |
| **분석** | analysis from stated design (not run). 읽은 설계 기술에서 도출했고 실행하지 않았다 |
| **불명** | not determinable from sources read |

이 문서는 전문을 읽지 못했다. 그래서 "답"은 0칸이고, "분석"은 초록의 설계 진술에서만 도출한 조건부 서술이다. 라벨이 "불명"인 칸의 내용에 쓴 조건부 해석은 [I]이며 결과가 아니다.

질문 번호는 로드맵 §10.2의 순서를 따른다.

1. 필요한 **관측 사건**을 수집하는가?
2. 서로 다른 이름·ID·필드가 같은 **자원·세대**를 가리키는 것을 식별하는가?
3. **생성·등록·초기화 완료와 의존 사용**의 관계를 표현하는가?
4. 필요한 **순서 또는 허용된 오류 처리** 정보를 입력받는가, 추론하는가?
5. 아직 실행되지 않은 순서를 **예측·탐색**하는가? 실제 실행에서 **확인**할 수 있는가?
6. **출력**은 후보·재현 성공·오류 결과 중 무엇이며 원 사례의 현상과 대응하는가?

### 3.2 사례 쪽에서 쓴 사실 (모두 저장소 기록, 재검증하지 않음)

| 사례 | 의존성과 현상 | 출처 |
|---|---|---|
| C1 CF#184 | provider(공개 BP 구성에서는 `OS_CountSemCreate`)가 이름 있는 count semaphore를 만든 뒤에야 CF의 `OS_CountSemGetIdByName`(`CF_CFDP_InitEngine`)이 성공한다. 버그 리비전에서는 조회 1회가 실패하면 `CF_EID_ERR_INIT_SEM` 이벤트와 `APP_ERROR` 종료. 수정은 `OS_ERR_NAME_NOT_FOUND`일 때만 25회 × `OS_TaskDelay(100)` 재시도 | `cases/ledger.md` §1a, §1b, §2 |
| C2 cFE#198 | 앱 P의 late init 완료와 다른 앱 R의 의존 호출. P·R은 비공개(EVA CWS)이고 통신 방식과 초기화 내용이 공개되지 않았다. 공개된 것은 단일 상태 시작 동기화(`WaitForStartupSync`)와 6.6.0a의 두 단계 방식(`APPS_INIT`/`LATE_INIT`, `CFE_ES_WaitForSystemState`) | `ledger.md` §1a, §1b, §2; 로드맵 §9.1 |
| C3 cFE#73 | SB task의 `CFE_SB.AppId`와 EVS task의 `EVS_AppID`·등록 플래그가 설정되기 전에, TIME task가 `CFE_SB_CreatePipe` 뒤 `CFE_EVS_SendEventWithAppID`(AppId=0)를 호출하고 `EVS_IsFiltered(0xFFFFFFFF)`가 범위 검사 없이 접근한다. 원 플랫폼은 Microblaze. segfault 지점은 사례 기록에서 미확인 | `ledger.md` §1a, §2 |
| C5 cFE#72 | `CFE_ES_AppCreate`가 `OS_TaskCreate` 전에 ES lock을 풀고 뒤에 다시 잡아 `TaskTable` 항목을 채운다. 그 사이 새 task의 `CFE_ES_GetAppID`가 `CFE_ES_ERR_APPID`를 반환할 수 있다 | `ledger.md` §1a, §2 |
| R2 cFE#2663 | SB가 `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`로 AppId를 저장하기 전에 ES/EVS task가 SB API 경로를 실행하고, `EVS_GetAppDataByID`가 NULL을 돌려 `ILLEGAL_APP_ID`가 된다. 서술은 "이벤트 요청 거부"이고, gdb 3회 모두 설정 전 사용 24회 = 거부 24회(`CreatePipe` 2 + `SubscribeFull` 4 + `ReportSingleEvent` 18; ES 20, EVS 4). 프로세스 실패는 없다(세 실행 모두 종료 상태 0). race인지 고정 순서인지는 실행으로 가려야 한다 | `ledger.md` §1a, §2; gdb 원시 기록 `env/logs/r2_g{1,2,3}_trace.jsonl`·`_summary.txt`와 그 정리 `results/R2_gdb_v701.md` §2. 2026-10-16에 원시 trace에서 `OUT_ILLEGAL_APP_ID`(`cfe_evs.c:188`) 적중을 다시 셌다: 세 실행 모두 24 [M]. 호출자별 내역은 `_summary.txt`의 `violation_appid` 줄과 같다 |
| 공통 | native cFS는 **한 Linux process**이고 OSAL task는 `pthread_create` thread다(osal `os-impl-tasks.c:568`). OSAL 자체 시험에서 named count semaphore의 생성과 이름 조회는 **syscall 0건**이었다. 이름 조회는 사용자 공간 테이블 검색이며 semaphore는 `sem_init`한 이름 없는 process-private 객체다. `CFE_SB_CreatePipe`가 부르는 `mq_open("<pid>.<name>")`은 syscall로 보인다 | `baselines/yu-group.md` §7.1; `baselines/availability.md` §3, §4.2. 이것은 OSAL 시험의 probe이지 ReDPro의 결과가 아니다 |

### 3.3 도구 수준의 답 (사례와 무관)

| Q | 라벨 | 답 | 근거 |
|---|---|---|---|
| 1 관측 사건 | 불명 | PIN으로 "run-time monitoring"을 한다. 감시하는 사건의 종류는 쓰여 있지 않다. 문제 정의가 system call 순서이므로 syscall이 감시 대상에 포함될 수 있다[I]. 그것만인지, 메모리 접근 등도 포함하는지는 알 수 없다. PIN은 일반적으로 메모리 접근과 routine도 계측할 수 있다(Pin 3.21 user guide, 맥락) | 초록; §2.2 |
| 2 자원·세대 식별 | 불명 | "a shared data item"이라고만 쓰여 있다. 식별 방식도, 세대(식별자 재사용)에 대한 서술도 읽지 못했다 | 초록 |
| 3 생성·등록 vs 의존 사용 | 불명 | 초록의 실패 정의는 "read and write a shared data item without proper synchronization"이다. readiness precondition(생성·초기화 완료가 의존 사용보다 먼저여야 한다는 전제)을 표현하는지는 쓰여 있지 않다. 쓰여 있지 않다는 것이 표현하지 못한다는 뜻은 아니다 | 초록 |
| 4 필요 순서·허용 오류 입력 | 불명 | 초록에 입력 서술이 없다. SysPro p.32는 ReDPro를 실행 입력을 개발자가 안다고 가정하는 작업으로 묶는다(2차). 필요 순서, 허용 오류, 실패 판정 기준을 입력으로 받는지는 알 수 없다. SysPro pp.34–35는 ReDPro를 포함한 자동 재현 연구들이 버그 보고서를 주 입력으로 삼지 않는다고 적는다(2차, 2026-10-16 추가). 이것도 입력이 무엇인지는 말하지 않는다 | 초록; SysPro p.32, pp.34–35 |
| 5 예측·탐색, 확인 | 분석 | 초록은 PIN이 "potential race pairs"의 실행 순서를 제어하고 failure를 "regenerate"한다고 서술한다. 즉 순서를 바꾸는 단계와 실제 실행에서 failure를 다시 만드는 단계가 설계에 있다. 순서를 바꿀 pair가 어떻게 만들어지는지(관측한 실행에서의 예측인지 탐색인지)는 알 수 없다 | 초록; SysPro p.35 |
| 6 출력 | 분석 | 초록의 출력은 "detect process-level races"와 "regenerate the failures"다. 보고 형식과 failure의 정의(crash, 출력 메시지, 사용자 oracle)는 알 수 없다 | 초록 |

### 3.4 5 × 6 라벨 요약

| | Q1 관측 | Q2 식별 | Q3 생성·사용 | Q4 입력 | Q5 예측·확인 | Q6 출력 |
|---|---|---|---|---|---|---|
| **C1** | 불명 | 불명 | 불명 | 불명 | 분석 | 불명 |
| **C2** | 불명 | 불명 | 불명 | 불명 | 불명 | 불명 |
| **C3** | 불명 | 불명 | 불명 | 불명 | 분석 | 불명 |
| **C5** | 불명 | 불명 | 불명 | 불명 | 분석 | 불명 |
| **R2** | 불명 | 불명 | 불명 | 불명 | 분석 | 불명 |

Q5의 "분석" 4칸은 **조건부**다. 설계에 순서 제어와 failure 재생성 단계가 있다는 진술은 초록에 있지만, 사례의 의존 pair가 ReDPro의 "potential race pair"가 되는지는 Q1–Q3가 불명이라 알 수 없다. 그 4칸은 "ReDPro가 이 사례를 다룬다"가 아니라 "설계에 이 단계가 있다"를 뜻한다.

**Q6의 두 라벨 (2026-10-16 추가).** §3.3의 도구 수준 Q6은 "분석"이고 위 표의 사례별 Q6은 모두 "불명"이다. 두 질문이 다르기 때문이다. 도구 수준 Q6은 "출력의 종류가 무엇인가"이고, 초록이 race 탐지와 failure 재생성이라는 두 출력을 서술하므로 설계 진술에서 도출한 "분석"이다. 사례별 Q6은 로드맵 §10.2의 뒷부분인 "원 사례의 현상과 대응하는가"까지 묻는다. 그러려면 ReDPro의 failure 정의와 보고 형식이 필요한데 읽은 자료에 없다. 그래서 사례별 칸은 "불명"이다. 집계(`process_race_method_matrix.md`)는 사례별 라벨을 쓴다. Q5에서 도구 수준과 사례별 라벨이 같은 것은 Q5의 사례별 칸이 "설계에 단계가 있다"까지만 말하고 사례와의 대응을 주장하지 않기 때문이다(C2는 앵커를 알 수 없어 "불명").

### 3.5 사례별 셀 (근거 포함)

근거 칸의 "초록"은 §1.2의 ReDPro 초록이다. 사례 쪽 사실은 §3.2다.

#### C1 — CF#184 (named count semaphore의 생성 vs 이름 조회)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 불명 | 필요한 사건은 provider의 생성(이름 등록 완료)과 CF의 이름 조회다. OSAL 자체 시험에서 둘 다 syscall 0건이었다 `[기록]`. 감시 대상이 syscall뿐이라면 두 사건은 입력에 나타나지 않고, 메모리 접근까지 감시한다면 나타날 수 있다[I]. 어느 쪽인지 읽은 자료로 정해지지 않는다 | 초록; `availability.md` §4.2 |
| 2 | 불명 | 이름 `BP0_tsem` → OSAL ID → 사용자 공간 테이블 항목의 연결을 식별하는 장치가 있는지 알 수 없다. 세대에 대한 서술도 없다 | 초록 |
| 3 | 불명 | readiness precondition("이름이 등록 완료되기 전에는 조회가 성공하지 못한다")과 의존 사용(조회)의 관계를 표현하는지 알 수 없다 | 초록 |
| 4 | 불명 | C1이 필요로 하는 허용 오류(재시도는 `OS_ERR_NAME_NOT_FOUND`일 때만, 25회 × 100 ms)와 필요 순서를 입력하는 인터페이스가 있는지 알 수 없다 | 초록; `ledger.md` §1b |
| 5 | 분석 | 설계에 순서 제어(potential race pair)와 failure 재생성 단계가 있다(초록). 생성↔조회가 "potential race pair"가 되는지는 Q1–Q3가 불명이어서 알 수 없다 | 초록 |
| 6 | 불명 | 사례 현상은 "조회 실패 → `CF_EID_ERR_INIT_SEM` + `APP_ERROR` 종료"이고, 수정 후에는 재시도 회복과 최종 실패의 구분이다. ReDPro의 failure 정의와 출력 형식을 알 수 없어 대응 여부를 판단할 수 없다 | 초록; `ledger.md` §2 |

#### C2 — cFE#198 (late init 완료 vs 다른 앱의 의존 호출)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 불명 | 필요한 사건은 P의 late init 완료와 R의 의존 호출이다. P·R이 비공개이고 통신 방식과 초기화 내용이 알려지지 않아 어떤 OS 자원을 거치는지 판단할 근거가 없다 | `ledger.md` §1a; 로드맵 §9.1 |
| 2 | 불명 | 같은 이유 | — |
| 3 | 불명 | 같은 이유. 시작 동기화 통과와 앱 고유 초기화 완료를 구분하는 표현이 ReDPro에 있는지도 알 수 없다 | 초록 |
| 4 | 불명 | 실패 기준은 원 보고가 구체화하지 않아 역할 재구성 실험의 연구 쪽 선택이 된다(P-C2 결정 [D]). 선택하더라도 원 보고자의 값으로 인용하지 않는다. ReDPro가 이런 입력을 받는지는 알 수 없다 | 초록; `ledger.md` §2 |
| 5 | 불명 | 앵커가 될 pair를 알 수 없고 대상 코드가 비공개다 | `ledger.md` §2 |
| 6 | 불명 | 원 현상이 구체 출력으로 공개되지 않았다 | `ledger.md` §1a |

#### C3 — cFE#73 (SB/EVS AppId 설정 vs TIME의 이벤트 요청)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 불명 | 필요한 사건은 `CFE_SB.AppId` 저장, `EVS_AppID`·등록 플래그 설정, TIME의 이벤트 요청(AppId=0)과 `EVS_IsFiltered`의 범위 검사 없는 접근이다. 모두 함수 호출과 전역 메모리 읽기·쓰기이고 syscall이 아니다(소스 수준 `[기록]`). TIME의 `CreatePipe`가 부르는 `mq_open`만 syscall로 보이나 의존 상태 자체가 아니다. 감시 대상 종류가 불명이라 수집 여부를 판단할 수 없다 | 초록; `availability.md` §3 |
| 2 | 불명 | 의존 상태는 AppId 정수(ES가 부여)와 EVS 배열 항목이다. 이를 식별하는 장치가 있는지 알 수 없다 | 초록 |
| 3 | 불명 | 공개(저장)와 사용(읽기)이 모두 메모리다. 이 관계를 표현하는지 알 수 없다 | 초록 |
| 4 | 불명 | 원 보고의 실패는 EVS 내부의 잘못된 인덱스 접근이고 segfault 지점이 사례 기록에서 미확인이다. 어떤 failure 기준이 필요한지 지금 정할 수 없고, ReDPro가 입력으로 받는지도 알 수 없다 | 초록; `ledger.md` §2 |
| 5 | 분석 | C1과 같다. 설계에 순서 제어와 재생성 단계가 있다(초록). AppId 저장↔TIME의 이벤트 요청이 pair가 되는지는 알 수 없다 | 초록 |
| 6 | 불명 | 사례 현상은 "잘못된 상태 사용"과 "실제 crash 여부"를 분리해 보고하는 것이다(로드맵 §9.2). crash가 일어나는지 자체가 사례 기록상 미확인이다. ReDPro의 failure 정의와 출력 형식을 알 수 없다 | 초록; `ledger.md` §2 |

#### C5 — cFE#72 (task 생성과 레지스트리 등록 사이의 공백)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 불명 | `OS_TaskCreate`의 `pthread_create`는 `clone3`로 보인다(에이전트가 쓴 작은 C 프로그램에서 관측 `[기록]`, cFS에서 측정한 것은 아니다). `TaskTable` 등록 쓰기와 새 task의 `CFE_ES_GetAppID` 조회는 메모리 접근이다. 감시 대상 종류가 불명이라 수집 여부를 판단할 수 없다 | 초록; `availability.md` §4 끝 |
| 2 | 불명 | task ↔ AppId 연결은 ES 테이블 안에 있다. 이를 식별하는 장치가 있는지 알 수 없다 | 초록 |
| 3 | 불명 | "생성한 task가 실행 가능해지는 시점"과 "등록 공개 완료"의 관계를 표현하는지 알 수 없다 | 초록 |
| 4 | 불명 | 원 현상(등록 전 조회의 실패 반환)은 crash가 아니다. 반환값을 보는 failure 기준을 입력으로 받는지 알 수 없다. 원 보고에 없는 앱 crash는 기대 결과로 넣지 않는다(로드맵 §9.3) | 초록; 로드맵 §9.3 |
| 5 | 분석 | C1과 같다. 설계에 순서 제어와 재생성 단계가 있다(초록). `OS_TaskCreate` ↔ 레지스트리 쓰기 pair가 대상이 되는지는 알 수 없다 | 초록 |
| 6 | 불명 | 사례 현상은 "등록 전 조회와 그 결과"다(로드맵 §9.3). ReDPro의 failure 정의와 출력 형식을 알 수 없다 | 초록; 로드맵 §9.3 |

#### R2 — cFE#2663 (SB AppId 설정 vs ES/EVS의 이벤트 요청)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 불명 | 필요한 사건은 AppId 설정(`CFE_ES_GetAppID(&CFE_SB_Global.AppId)`), 값 전달, 이벤트 요청, 오류 반환이다. 모두 함수 호출과 메모리이고 `CreatePipe`의 `mq_open`만 syscall로 보인다(`[기록]`). 감시 대상 종류가 불명이라 수집 여부를 판단할 수 없다 | 초록; `availability.md` §3 |
| 2 | 불명 | SB AppId의 값(0 대 유효값)은 메모리의 정수다. 식별 장치가 있는지 알 수 없다 | 초록 |
| 3 | 불명 | 설정(저장)과 사용(읽기)이 메모리다. 이 관계를 표현하는지 알 수 없다 | 초록 |
| 4 | 불명 | R2의 필요 순서("SB AppId 저장이 SB API 경로의 이벤트 요청보다 먼저")를 입력하는 인터페이스가 있는지 알 수 없다 | 초록 |
| 5 | 분석 | C1과 같다. 설계에 순서 제어와 재생성 단계가 있다(초록). R2의 순서가 고정되는 이유는 core 시작 직렬화이고(사례 기록), 이 때문에 "potential race pair"를 뒤집을 여지가 있는지는 R2가 race인지 고정 순서인지를 실행으로 가려야 알 수 있다(`ledger.md` §2). ReDPro 안에서 이 질문을 다루는지는 알 수 없다 | 초록; `ledger.md` §2 |
| 6 | 불명 | 사례 현상은 "이벤트 요청 거부 24회"이고 프로세스 실패가 없다(상태 폐기). R2는 failure 메시지도 출력하지 않는다(`cfe_evs.c:185-188`, `availability.md` §1). ReDPro의 "failure"가 프로세스 실패인지 사용자가 정한 oracle인지에 따라 대응 여부가 달라진다[I]. 어느 쪽인지 알 수 없다 | 초록; 로드맵 §2.2 |

### 3.6 이 절이 말하지 않는 것

- 어느 셀도 "ReDPro가 사례를 탐지하지 못한다"는 뜻이 아니다. 실행하지 않았고 전문을 읽지 않았다. "불명"은 판단 근거가 없다는 뜻이다.
- "ReDPro는 syscall만 본다"는 문장은 쓰지 않는다. 읽은 자료에 근거가 없다(§2.2).
- "ReDPro는 readiness precondition을 표현하지 못한다"는 문장도 쓰지 않는다. 쓰여 있지 않다는 것은 표현하지 못한다는 뜻이 아니다.
- 이 절의 "분석"은 초록의 설계 진술에 대한 것이다. OSAL 레지스트리를 자원으로 추가하는 등의 확장은 EXTENSION이며 원 도구의 결과가 아니다(로드맵 §10.4 끝, `availability.md` §5.1).
- 이 절은 ReDPro를 로드맵 §10.1의 탐지 행이나 재현 행으로 분류하지 않는다.

---

## 4. 실행 가능성과 결과 상태

### 4.1 요구 조건과 오늘 확인한 상태

| 요구 | 근거 | 오늘 상태 |
|---|---|---|
| 도구 구현(소스, 바이너리, VM 이미지, 스크립트) | 어떤 읽은 자료도 위치를 말하지 않는다. 초록과 서지에 artifact 링크가 없다 | **없음.** GitHub 저장소·코드·사용자 검색, 제1저자 홈과 CV, NSF PAR, arXiv, Internet Archive 검색에서 찾지 못했다(경로 12–21). 제2저자 쪽은 동일인 확인 없이 이름과 소속이 같은 홈만 확인했다(경로 15). "접근 가능한 출처에서 못 찾았다"이지 "없다"가 아니다. 저자에게 문의한 적은 없다 |
| 논문 전문(방법, 입력, 관측 사건, 평가 대상, 구현 환경 서술) | 이 문서의 모든 "불명"의 원인 | **못 읽음.** ACM이 Cloudflare 도전으로 막고, 합법적 공개 사본이 없다(경로 1–10, 14, 19–24) |
| Intel PIN | 초록: "binary instrumentation tool named PIN" | **이 컨테이너에 없다**(`pin` 실행 파일이 PATH에 없고 `/opt/*pin*`도 없음 `[M]`, 2026-10-15 확인, 2026-10-16T00:40Z 재확인). Intel의 PIN 소개 페이지와 Pin 3.21 User Guide는 열린다(HTTP 200). 설치하지 않았다 |
| PIN의 버전 | 쓰여 있지 않다. 참고문헌 목록에 "Pin 3.21 User Guide"가 있으나 사용 버전의 서술이 아니다 | `unspecified_by_reference`. 버전을 고르면 레퍼런스에 없는 선택이다 |
| PIN이 이 컨테이너 커널에서 동작하는지 | 확인하지 않았다 | [U]. 커널은 2026-10-15에 `6.18.44-fc-v114`, 2026-10-16T00:30Z에 `6.18.44-fc-v139`였다 [M]. `/` 여유는 2026-10-16에 1.3 GB(사용률 97%) [M]로, 설치를 정하더라도 공간이 먼저 문제가 된다. 2026-10-08 기록(`availability.md` §1.1)의 `ptrace` 동작 확인은 `fc-v80`에서의 것이고 2026-10-15·16에 재확인하지 않았다. PIN이 ptrace 주입에 의존하는지도 확인하지 않았다 |
| 사용자가 줘야 하는 입력(실행 입력, oracle, 계측 위치 등) | 쓰여 있지 않다. SysPro p.32가 ReDPro를 "실행 입력을 개발자가 안다고 가정하는" 작업으로 묶는다(2차) | [U] |
| cFS 쪽 구성 | cFS 기준 구성은 별도 작업의 대상이다(`env/ENV.md`) | 이 문서는 정하지 않는다. 도구가 요구하는 대상 형식(process 단위인지)을 모르므로 적용 구성을 정할 수 없다 |

**막는 요인(블로커).**

| # | 블로커 | 종류 |
|---|---|---|
| B1 | 구현을 찾지 못했다 | 자료 |
| B2 | 전문을 읽지 못해 방법·입력·oracle·평가 대상을 모른다. 기법을 읽고 재구현(REIMPLEMENTATION)하는 길도 막힌다 | 자료 |
| B3 | PIN 버전과 도구 구성이 레퍼런스에 없다 | 자료(`unspecified_by_reference`) |
| B4 | PIN이 설치되어 있지 않고, 이 컨테이너에서의 동작은 미확인이다 | 환경 |

환경의 차단 요인(B4)이 원인의 전부가 아니고, 자료의 차단 요인(B1–B3)이 먼저다. 이 점이 커널을 못 바꾸는 RacePro와 다르다(`plan/methods/RacePro.md`).

### 4.2 결과 상태 (로드맵 §10.4)

| 상태 | 의미 | 오늘 ReDPro에 적용되는가 |
|---|---|---|
| `NOT_RUN` | 환경·자료·결정 등으로 실행하지 못함 | **적용된다. C1, C2, C3, C5, R2 모두.** 원인은 자료(B1–B3)다. 미탐지 수에 넣지 않는다 |
| `INAPPLICABLE` | 원 기법의 명시된 입력·대상 범위에 해당하지 않음 | 적용하지 않는다. 이 상태는 이유와 원문 위치를 보고해야 하는데(로드맵 §10.4) 전문을 읽지 못해 대상 범위의 서술을 찾지 못했다. "multiple processes"(초록)만으로 한 process 안의 task 쌍을 범위 밖이라고 결론짓지 않는다. 판단은 PI의 몫이고 전문 확인 뒤에 할 수 있다 |
| `PATH_NOT_EXECUTED` | 도구는 실행했으나 목표 현상이 실행되지 않음 | 해당 없음(실행하지 않았다) |
| `OBSERVATION_GAP` | 목표 사건이 실행됐지만 필요한 정보가 기록되지 않음 | 해당 없음 |
| `ANALYSIS_GAP` | 필요한 사건이 기록됐지만 의존성·후보를 만들지 못함 | 해당 없음 |
| `REPORTED` | 관련 후보·재현·오류를 출력함 | 해당 없음 |

`baselines/availability.md`의 verdict 용어로는 `material_unavailable`이다. 로드맵 §4.6과 `availability.md` §5.3에 따라 §4.7의 탐지·재현 표에 ReDPro 행을 넣지 않는다. 이 문서도 넣지 않는다.

### 4.3 PI 판단이 필요한 항목 (결정 후보. 여기서 정하지 않았다)

| # | 항목 | 선택지 | 이 문서의 기본 |
|---|---|---|---|
| D-RD-1 | 전문을 얻는 방법 | (a) ACM DL에 접근되는 환경에서 다시 시도한다. (b) PI가 PDF를 `paper/`에 둔다(P-PAPERS [D]는 이것을 연구 착수의 필수 조건으로 만들지 않는다고 했으므로 선택 사항이다). (c) 저자에게 PDF와 구현 문의(`availability.md` A2의 결정 사항과 같다, 문의한 적 없음). (d) 전문 없이 간다. (e) (2026-10-16 추가) Internet Archive 스냅숏(`web.archive.org/web/20220507074916/https://dl.acm.org/doi/pdf/10.1145/3476883.3520207`, availability API의 기록 상태 200)을 `web.archive.org`가 열리는 환경에서 받는다. 이 세션에서는 egress 정책이 이 호스트를 막는다. 스냅숏이 PDF인지 도전 페이지인지는 모른다. 공개 보관본을 출처로 인정할지, 받은 뒤 ACM 판과 같은 판본인지 어떻게 확인할지도 함께 정해야 한다. | (d). 모든 칸이 "불명"으로 남는다 |
| D-RD-2 | 관련 연구에서 ReDPro를 어떻게 서술할지 | 전문 확인 전에는 초록 수준의 서술만 허용한다(§4.4의 문구). 역할 표에서는 제외한다 | 제외 |
| D-RD-3 | 전문을 얻은 뒤 로드맵 §10.2의 여섯 질문을 다시 채울지 | §6의 확인 목록으로 채운다 | 전문을 얻으면 채운다 |
| D-RD-4 | 구현을 얻은 뒤 PIN 버전과 실행 구성을 정하는 방법 | 도구 문서나 artifact 절에 있으면 그 값(`from-reference`), 없으면 `unspecified_by_reference`로 두고 선택은 PI 결정 | 선택하지 않는다 |

### 4.4 다른 문서가 ReDPro를 언급할 때

**쓸 수 있는 문구:**
- "ReDPro(ACMSE 2022)는 초록에서 process-level race의 탐지와 실패 재생성을 다룬다고 서술되고, PIN으로 run-time 감시와 race pair의 실행 순서 제어를 한다고 한다. 전문과 구현을 확인하지 못해 방법, 입력, 관측 사건, cFS 적용 범위는 미확인이다."
- "ReDPro는 다섯 사례에서 `NOT_RUN`이다(자료 부족)."

**쓸 수 없는 문구:**
- "ReDPro는 C1, C2, C3, C5 또는 R2를 탐지하지(재생성하지) 못한다."
- ReDPro를 탐지 표나 재현 표의 확정 행으로 넣는 것.
- "ReDPro는 syscall만 관측한다", "ReDPro는 readiness precondition을 표현하지 못한다", "ReDPro는 같은 process 안의 thread를 다루지 못한다" 같은, 읽은 자료에 근거가 없는 서술.
- ReDPro의 어떤 수치(효과, 효율, 버그 9건의 결과)를 인용하는 것. 초록은 "effective and efficient"라고만 한다.

---

## 5. 다섯 사례에서 부족한 것과 Kairos 기능 가설

### 5.1 ReDPro가 이미 하는 것으로 읽히는 것 — 차별점으로 주장하지 않는다

초록 수준의 읽기이므로 세부가 다를 수 있다. 로드맵 §10.5는 "이미 가능한 기능은 재발명한 차별점으로 주장하지 않는다"고 한다.

- 순서를 바꾸는 단계: PIN으로 "potential race pairs"의 실행 순서를 제어한다(초록). Kairos의 "예측·탐색 및 실행 확인"(§10.5 마지막 두 번째 행)을 ReDPro에 없는 기능으로 쓰지 않는다.
- 실제 실행에서 failure를 다시 만드는 단계(초록의 "regenerate the failures"). 실행 확인 자체는 차별점이 아니다.
- binary instrumentation 기반의 run-time 감시.

### 5.2 가설 (근거가 있는 만큼만)

ReDPro에 부족한 것으로 확인된 사항은 **없다**. 아래는 연구 가설과 개발 후보이며, 전문 확인 뒤에만 비교 결과와 연결한다.

| ID | 가설 | 지지하는 근거 | 근거의 한계 | 확인할 사례 |
|---|---|---|---|---|
| H-RD-1 (조건부) | ReDPro의 관측 사건이 syscall로 한정된다면, cFS 내부 API·상태 변화·값 전달 사건(이름 등록·조회, AppId 저장·사용, ES 레코드 쓰기)이 후보 생성 입력에 없을 수 있다. 그 경우 Kairos 기능 후보는 "내부 API·상태 변화·값 전달 관측 + 소스 연결"(로드맵 §10.5 첫 행)이다 | 초록의 문제 정의가 system call 실행 순서다. 사례 C1·C3·C5·R2의 공개와 사용 지점은 syscall이 아니다(OSAL 자체 시험의 syscall 0건 `[기록]`, C3·C5·R2는 소스 수준) | **약하다.** 조건("syscall로 한정된다면")이 읽은 자료로 정해지지 않고, PIN은 메모리 접근도 계측할 수 있다(Pin 3.21 user guide, 맥락). 이 가설은 §6 확인 항목 1이 "syscall만"으로 답해질 때만 성립한다 | R2·C3·C5·C1 |
| H-RD-2 (정의 차이) | 비교 논문은 실행 단위의 정의를 먼저 밝혀야 한다. 초록은 "multiple processes"를 말하고, 이 연구의 Process-Level은 cFS app·task를 작업 단위로 하는 운용상 정의이며 native cFS는 한 OS process 안의 pthread task다 | 초록의 "multiple processes", 로드맵 §1.2, `yu-group.md` §7.1 `[기록]` | 이것은 Kairos 기능의 부족이 아니라 정의 차이 후보다. ReDPro가 process를 OS process로 한정하는지, 같은 process의 thread 사이도 포함하는지는 알 수 없다. 범위 밖 판정(`INAPPLICABLE`)의 근거로 쓰지 않는다 | 다섯 사례 전체 |

**로드맵 §10.5의 나머지 행은 근거가 없어 가설을 올리지 않는다.** 전문을 읽지 못했기 때문이다.

| §10.5의 행 | 이유 |
|---|---|
| 이름·ID·등록 정보를 같은 자원으로 연결하지 못함 (C1·C5) | 초록이 자원 식별 방식을 말하지 않는다 |
| 기존 시작 동기화와 실제 초기화 완료를 혼동 (C2) | 초록이 readiness precondition을 다루는지 말하지 않는다. C2는 대상 코드도 비공개다 |
| 오류 반환과 회복 처리를 구분하지 못함 (C1) | 초록이 failure의 정의와 회복 처리를 말하지 않는다 |
| 관측된 실행만 다룸 (C1·C3·C5) | 근거가 반대다. 초록이 순서 제어와 failure 재생성 단계를 서술한다 |
| 잘못된 순서가 고정된 이유를 설명하지 못함 (R2) | 초록이 순서의 근거를 어떻게 다루는지 말하지 않는다 |

### 5.3 비교 논문에서 먼저 밝혀야 할 정의 차이

- **실행 단위:** 이 연구는 Process-Level을 cFS app·task로 정의한다(PI 결정, 로드맵 §1.2). ReDPro의 실행 단위 서술은 전문을 읽은 뒤에 비교한다.
- **역할:** 이 연구의 로드맵 §10.1 표에서 ReDPro는 "역할·세부 모델 미확인"이다. 초록은 탐지와 재생성을 모두 서술하므로 전문 확인 뒤에 탐지 행, 재현 행, 또는 두 역할의 결합 중 무엇으로 둘지 정한다.
- **race라는 단어:** P-CLASSGT [D]에 따라 원출처의 "race"를 도구 class로 자동 변환하지 않는다. ReDPro의 "process-level race"와 이 연구의 "readiness violation" 사이의 대응은 전문 확인 뒤에 기록한다.

---

## 6. 확인하지 못한 것과 다음에 할 일

전문을 얻으면 아래를 확인해 §3의 라벨을 다시 채운다. 질문 번호는 로드맵 §10.2다.

| # | 확인할 것 | 관련 질문 |
|---|---|---|
| 1 | PIN이 계측하는 사건의 종류: syscall 진입·반환, 메모리 읽기·쓰기, 함수 호출, 기타. 사례의 공개·사용 지점이 그 사건이 될 수 있는지 | Q1 |
| 2 | "shared data item"의 식별 방식(경로, inode, 이름, 주소 중 무엇인지). 식별자 재사용(세대)의 처리 | Q2 |
| 3 | "potential race pair"의 정의와 생성 규칙. 생성·초기화 완료 같은 readiness precondition을 표현하는지 | Q3 |
| 4 | 입력: 테스트 입력, 계측 위치, 필요 순서, 허용 오류, failure 판정 기준(oracle)을 사용자가 주는지 도구가 추론하는지 | Q4 |
| 5 | pair 생성 방식(관측한 실행에서 계산하는지), 순서 제어 방식(지연 삽입, 동기화 등), 재생성의 확인 기준 | Q5 |
| 6 | 출력 형식과 failure의 정의. 평가 9건의 목록과 각 건의 결과, bug 438076(`mv`)이 포함되는지, effective·efficient의 측정 방식 | Q6 |
| 7 | 대상 범위: process의 정의, 같은 process의 thread 사이 race를 다루는지(§5.3). `INAPPLICABLE` 판정이 필요하면 원문 쪽·절 | `INAPPLICABLE` |
| 8 | 구현 위치, artifact 절, PIN 버전, 실행 환경(OS, 커널, 아키텍처) | §4.1 |
| 9 | 학위논문(`uknowledge.uky.edu/cs_etds/119`)에 ReDPro 내용이 있는지. 초록은 언급하지 않는다. 본문은 Cloudflare 도전으로 못 열었다(2026-10-16에도 같다) | 보조 |
| 10 | (2026-10-16 추가) Internet Archive 스냅숏 `20220507074916`의 내용이 ACM PDF인지. 이 세션에서는 egress 정책으로 열 수 없다(D-RD-1 (e)) | 전문 확보 |
| 11 | (2026-10-16 추가) SLR(*SCP* 2026, `10.1016/j.scico.2026.103552`)이 ReDPro를 어떻게 서술하는지. Semantic Scholar에 문맥 문장이 없고 본문은 열지 않았다 | 보조 `[2차]` |

**이 문서가 확인하지 못한 것의 목록:**
- ACM 원 페이지의 초록이 Semantic Scholar 사본과 같은지(OpenAlex의 앞 다섯 문장과는 일치한다).
- Crossref의 24건 참고문헌 중 DOI가 없는 항목의 원 서지, 키 14번의 누락 사유.
- 저자 CV의 "7 pages", "acceptance rate: 35%"(저자의 자기 서술).
- 제2저자 홈페이지의 동일인 여부.
- Pin 3.21 User Guide 중 syscall 계측 API(본문에서 이름이 검색되지 않았고 API 참조 문서는 열지 않았다).

**이 작업 범위 밖이라 하지 않은 것:** `baselines/availability.md`, `yu-group.md`, 로드맵의 ReDPro 항목 갱신. 이 문서의 §1.3이 달라진 점을 기록한다.

---

## 7. 출처 목록

**ReDPro 자신(서지와 초록만):**
- T. S. Zaman, T. Islam, *ReDPro: an automated technique to detect and regenerate process-level concurrency failures*, Proceedings of the ACM Southeast Conference (ACM SE '22), pp. 106–112, 2022. DOI [10.1145/3476883.3520207](https://doi.org/10.1145/3476883.3520207). 전문 `[미도달]`.
- 초록 사본: Semantic Scholar Graph API, `DOI:10.1145/3476883.3520207`. 앞 다섯 문장은 OpenAlex(`W4229055229`)도 같다.
- 서지와 참고문헌: Crossref `api.crossref.org/works/10.1145/3476883.3520207`.
- 프로그램: [ACMSE 2022 Schedule of Paper Presentations](https://acmse.net/2022/schedule-papers/), Session 5.
- 저자 CV: [tarannumzaman.github.io/files/CV_zaman.pdf](https://tarannumzaman.github.io/files/CV_zaman.pdf), sha256 `bf2ad801c0e16d79ebaa2ca0e6816886dc718aed3e6d951edaa9212ef0767182`.
- 학위논문 레코드: [UKnowledge, 119](https://uknowledge.uky.edu/cs_etds/119). PDF `[미도달]`.
- 제1저자 ORCID [0000-0002-8634-524X](https://orcid.org/0000-0002-8634-524X) 작품 목록(공개 API, 2026-10-16). ReDPro 항목은 DOI 링크뿐이다.
- Internet Archive availability API 응답(2026-10-16): 스냅숏 `20220507074916`. 스냅숏 자체는 `[미도달]`.

**ReDPro를 인용한 논문(2차):**
- T. S. Zaman 외, *SysPro: Reproducing System-level Concurrency Bugs from Bug Reports*, [arXiv:2601.09616v1](https://arxiv.org/abs/2601.09616). pp.8, 21–23, 32, 34–35. 같은 논문의 JSS판(`10.1016/j.jss.2026.112785`)은 읽지 않았다.
- T. S. Zaman, M. H. Kishan, L. N. Lota, *Gender Dynamics in Software Engineering: Insights from Research on Concurrency Bug Reproduction*, [arXiv:2502.20289](https://arxiv.org/abs/2502.20289). p.3 Table III.
- A. Al Hasan 외, *LLPut*, [arXiv:2503.20578v3](https://arxiv.org/abs/2503.20578). p.1.
- Lota 외(DBLP 키 `journals/scp/LotaZAFCAIK27`, 저자 전체는 확인하지 않았다), *NLP Techniques for software debugging tasks: A systematic literature review…*, Science of Computer Programming 2026, DOI `10.1016/j.scico.2026.103552`. 피인용 목록에서만 확인했고 본문은 열지 않았다.

**맥락(ReDPro의 문장이 아님):**
- Intel, [Pin 3.21 User Guide](https://software.intel.com/sites/landingpage/pintool/docs/98484/Pin/html/). ReDPro의 참고문헌 항목이 적은 주소와 같다.

**저장소 기록(재검증하지 않음):**
- `research/cfs_procrace/baselines/availability.md` §1, §3, §4.2, §5
- `research/cfs_procrace/baselines/yu-group.md` §1, §3.2, §4, §5(R11), §7
- `research/cfs_procrace/cases/ledger.md` §1a, §1b, §2
- `research/cfs_procrace/results/R2_gdb_v701.md` §1–§2, 그리고 원시 기록 `research/cfs_procrace/env/logs/r2_g{1,2,3}_trace.jsonl`, `r2_g{1,2,3}_summary.txt` (2026-10-16 재계수)
- `research/cfs_procrace/plan/ROADMAP_20261015.md` §1.2, §2.2, §4.6, §10, §12
- `research/cfs_procrace/Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md` (P-PAPERS, P-CLASSGT, P-C2, T1 [D])
- `research/cfs_procrace/CONDITIONS_POLICY.md`
- `research/cfs_procrace/plan/methods/RacePro.md` (같은 형식의 자매 문서)
- `research/cfs_procrace/plan/process_race_method_matrix.md` §2.5 항목 4, §5.4 항목 16
