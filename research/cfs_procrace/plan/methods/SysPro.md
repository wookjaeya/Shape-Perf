# SysPro (arXiv 2601.09616v1 / JSS 2026) 분석 — Kairos 비교용

- 작성일: 2026-10-15 (UTC 09:30–09:50, 경로 시도와 읽기 시각).
- 대상: T. S. Zaman 외, *SysPro: Reproducing System-level Concurrency Bugs from Bug Reports*. arXiv:2601.09616v1 (2026-01-14, 코멘트 "Accepted in JSS"). 학술지판은 Journal of Systems and Software vol. 236, 논문 번호 112785, 2026-06, DOI 10.1016/j.jss.2026.112785.
- 범위: 로드맵(`plan/ROADMAP_20261015.md`) §10 (M6)의 SysPro 항목. 여섯 분석 질문(§10.2)을 사례 C1, C2, C3, C5, R2에 적용한다.
- 이 문서의 성격: **분석 문서**다. SysPro를 실행하지 않았다(구현을 구하지 못했다). 어느 사례에 대해서도 "재현하지 못한다" 또는 "탐지하지 못한다"는 결과를 담지 않는다. 아래 "분석" 셀은 읽은 설계 서술에서 도출한 것이며 실행 결과가 아니다.
- 읽은 판본: **arXiv v1 PDF 47쪽 전체(본문 1–38쪽과 참고문헌)**. 학술지판(JSS)의 본문은 열지 못했다. 두 판본의 저자 목록이 다르므로(§1.3) 내용이 같다고 보지 않는다. 이 문서의 모든 "SysPro p.N"은 arXiv v1의 PDF 쪽(인쇄 쪽과 같다)이다.
- 용어: 신규 문서이므로 T1 (a)를 따른다. 판정 대상은 readiness specification(준비 상태 명세), 그 전제는 readiness precondition, 위반은 readiness violation이다. "API contract"는 문서화된 API 의미를 가리킬 때만 쓴다. SysPro 논문의 "bug", "failure", "buggy interleaving", "system-level concurrency bug"는 논문의 용어 그대로 쓰고 도구 class로 바꾸지 않는다(P-CLASSGT [D]).
- 조건 원칙: `CONDITIONS_POLICY.md`. 이 문서는 실험 조건을 정하지 않는다. 논문이 자기 평가에서 쓴 값(n=10, N=20, 100회, 120분 등)은 논문의 값이며 cFS 조건으로 가져오지 않는다. 레퍼런스가 없는 값은 `unspecified_by_reference`로 둔다.
- PI 결정(`PI_DECISIONS_RESOLVED_20261015.md`)은 결정으로만 인용한다. 이 문서가 쓰는 결정은 P-PAPERS(원문 조사 승인), T1, P-CLASSGT, QT-4다. 사실로 쓰지 않는다.
- 같은 계보의 다른 문서: `plan/methods/DESCRY.md`, `RRF.md`, `SimRacer.md`, `RacePro.md`. SysPro는 DESCRY와 RRF를 비교 대상으로 서술하고(p.2, p.31), 저자 일부(T. S. Zaman, T. Yu)가 DESCRY의 저자와 겹친다(참고문헌 [12], p.40). 형식을 DESCRY.md와 맞췄다.

## 증거 태그와 근거 등급

| 태그 | 뜻 | 이 문서에서의 쓰임 |
|---|---|---|
| [F] | 인용이 있는 사실 | 원출처(논문 쪽·절) 또는 저장소 기록(문서·절). 저장소 기록은 "기록"이라고 표시하며 이번에 재검증하지 않았다 |
| [M] | 측정 | 오늘(2026-10-15) 이 컨테이너에서 읽기 전용으로 확인한 값 |
| [I] | 추론·분석 | 설계 서술에서 도출한 해석. 도구를 실행하지 않았다 |
| [D] | 결정 | PI 결정 |
| [U] | 미확인 | 자료가 없거나 읽지 못함 |

문장마다 출처 등급을 아래 다섯 가지 중 하나로 붙인다.

| 등급 | 뜻 |
|---|---|
| `[전문]` | 본문 전체를 텍스트로 추출해 읽었다. 그림(Figure 1, 5)은 이미지로도 확인했다 |
| `[초록]` | 초록·서지만 읽었다 |
| `[2차]` | 다른 문서가 이 도구를 서술한 문장을 읽었다. 도구 자신의 서술이 아니다 |
| `[기록]` | 저장소의 이전 보고서에 있는 값. 이번에 재검증하지 않았다 |
| `[미도달]` | 열지 못했다 |

---

## 0. 한눈에 보기

1. **SysPro 논문(arXiv v1) 전문을 읽었다** `[전문]`. 2026-10-08 기록(`baselines/syspro.md`)은 arXiv가 막혀 논문을 못 열었다고 적었으나, 오늘은 PDF, HTML, TeX 원본이 모두 열린다(§1.1 경로 1–3). 그 기록의 "secondary-source derived" 항목 일부가 이제 논문 쪽으로 확정되거나 정정된다(§1.3).
2. **구현(ReproSys)은 오늘도 열지 못했다.** 논문의 Data availability(p.38)가 가리키는 `github.com/tarannumzaman/ReproSys/tree/main`은 익명 `git ls-remote`가 인증을 요구하고, raw 파일은 404이며, GitHub 검색과 Software Heritage에도 없다(§1.1 경로 6–12). 학술지판의 data-availability 문장은 읽지 못했다.
3. **역할은 재현(reproduction)이다.** 입력은 bug report(subject, body), 대상 응용의 C 소스, system call manual이고, 출력은 buggy 후보 system call 위치의 순위 목록과 재현에 성공했을 때의 interleaving schedule이다(p.3, p.14). 탐지 도구가 아니다. 탐지 표에 점수로 합치지 않는다.
4. **실행 단위는 OS process이고 제어 단위는 system call 호출 지점이다.** 논문은 inter-process(system-level) bug를 대상으로 하고 intra-process(thread-level) fault는 구분한다(p.2–3). 문제 정의는 "bug report, 버그를 일으키는 process 집합, 보고서에 나온 응용 process의 소스"다(p.4–5). 한 process 안의 thread를 어떻게 다루는지는 서술이 없다.
5. **SysPro는 실행 중 사건을 수집하지 않는다고 읽힌다.** 논문이 서술하는 실행 단계는 PIN으로 선택한 system call 호출 지점에 sleep을 넣고 실행해 보고된 실패가 재현되는지 보는 것이다(p.20–21, p.24). trace, 메모리 접근, lock, happens-before 관계를 기록한다는 서술이 없다(§2.1, 키워드 검색). `baselines/syspro.md` §5 항목 4의 "system call entry와 exit 사건을 관측한다"는 서술은 논문에서 확인되지 않는다(§1.3).
6. **여섯 질문 가운데 도구 수준에서 논문이 직접 답하는 것은 입력(Q4)과 출력(Q6), 탐색 방식(Q5)이다.** 관측 사건(Q1)은 "수집하지 않는다"가, 자원·세대 식별(Q2)과 생성·등록 대 사용의 표현(Q3)은 "서술이 없다"가 답이다. 사례별 셀은 대부분 "분석"이다(§3.4).
7. **오늘 적용되는 상태는 다섯 사례 모두 `NOT_RUN`이다**(§4.2). 주 장애는 구현 미확보다. `INAPPLICABLE`은 논문의 범위 서술(p.2–3)에 근거한 PI 판단 후보이며 이 문서가 붙이지 않았다(§4.3).
8. **Kairos 가설은 근거가 있는 만큼만 올렸다**(§5). 근거가 지지하는 것은 "SysPro의 제어 단위는 syscall 호출 지점이고 사건 관측이 없다"(H-SYSPRO-1), "순서 모델이 syscall 쌍이고 readiness 상태 개념이 서술에 없다"(H-SYSPRO-2)까지다. report→위치 매핑, 지연 삽입, 순위화는 SysPro에 이미 있으므로 차별점으로 쓰지 않는다(§5.1).

---

## 1. 접근한 출처와 문장별 근거 등급

### 1.1 시도한 경로와 결과

시각은 2026-10-15 UTC 09:30–09:50다. P-PAPERS [D]에 따라 저자 공개본, arXiv(PDF, HTML, TeX), 서지 서비스, 기관 저장소, 검색을 모두 시도했다. 접속 실패는 원인을 기록한 것까지만 썼고, Cloudflare 도전 페이지나 프록시를 우회하려고 하지 않았다. TLS 검증도 끄지 않았다. 내려받은 파일은 각각 새 빈 디렉터리에 두고 텍스트만 추출했다.

| # | 경로 | 결과 | 등급·비고 |
|---|---|---|---|
| 1 | arXiv PDF `https://arxiv.org/pdf/2601.09616v1` (`/pdf/2601.09616`도 같은 파일) | `curl` 200, 1,345,590 B, 47쪽. sha256 `8fe5daa4c7c3794715632e4379d9160a9852b723dc4a649317ef6bae0a75e726`. `pdfinfo` 제목 "SysPro: Reproducing System-level Concurrency Bugs from Bug Reports", 저자 Zaman, Yan, Wang, Islam, Shi, Yu. 쪽별로 `pdftotext -layout`으로 추출해 본문 1–38쪽과 참고문헌을 읽었다. Figure 1, Figure 5는 이미지로 확인했다. Figure 2–4, 6–10은 본문 서술과 캡션 위주로 읽었고 그림의 세부 값은 쓰지 않았다 | **`[전문]`** |
| 2 | arXiv 서지 페이지 `arxiv.org/abs/2601.09616`, `…/abs/2601.09616v1`, `export.arxiv.org/abs/…` | 모두 200. 제출 이력은 v1(2026-01-14 16:40 UTC, 1,076 KB) 하나이고 `…/abs/2601.09616v2`는 404다. Comments "Accepted in JSS". 코드 링크 없음 | `[초록]`. v1이 유일한 판본이다 |
| 3 | arXiv HTML `arxiv.org/html/2601.09616v1`, TeX 원본 `arxiv.org/e-print/2601.09616v1` | 200(HTML 437,818 B), 200(원본 gzip 1,101,458 B). 원본은 `.tex` 10개(템플릿 포함)와 그림, `.bib`를 담는다. 본문 `.tex`의 도구 서술 문장은 PDF와 같다. 원본에도 PIN, srcML, NLTK, Gensim, SPMF의 판본 서술이 없다. 저자 주석(`%`로 시작하는 줄)은 있으나 출판 내용이 아니므로 인용하지 않았다 | `[전문]` 확인용. 새 정보는 없다 |
| 4 | 학술지판: `doi.org/10.1016/j.jss.2026.112785` | 200, 그러나 Elsevier의 "Redirecting" 중계 페이지(2,682 B)이고 최종 목적지는 ScienceDirect다 | 서지만 |
| 5 | ScienceDirect `sciencedirect.com/science/article/pii/S0164121226000191` | `curl` 403, 응답 본문은 도전 페이지(`<title>ScienceDirect</title>`, 832,805 B). WebFetch도 403 | **`[미도달]`.** 학술지판 본문과 data-availability 문장은 읽지 못했다 |
| 5a | Crossref `api.crossref.org/works/10.1016/j.jss.2026.112785` | 200. 제목 "SysPro: Reproducing system-level concurrency bugs from bug reports", JSS vol. 236, 논문 번호 112785, 발행 2026-06, 저자 Zaman, Islam, Shi, **Shi, Xian**, Yu(arXiv v1의 Yan, Wang 대신), 참고문헌 68건, 인용 3건, 초록 필드 비어 있음, 본문 링크는 `api.elsevier.com`(TDM, 자격 필요) | 서지 `[M]`. 초록도 없다 |
| 5b | `api.elsevier.com/content/article/PII:S0164121226000191?httpAccept=text/plain`(Crossref가 적은 TDM 링크) | 자격 없이 한 번 요청해 400. API 키가 필요하다. 키를 구하려 하지 않았다 | `[미도달]` |
| 6 | 구현: `github.com/tarannumzaman/ReproSys`(p.38의 링크) | `git ls-remote`가 인증을 요구했다("could not read Username", 프롬프트 비활성). 같은 소유자의 공개 저장소 `tarannumzaman/Scminer`는 읽힌다(HEAD `255933c6…`). 이름 변형 `reprosys`, `Tarannum-Zaman/ReproSys`, `tarannumzaman/SysPro`도 같은 인증 요구. `curl`로 `github.com/tarannumzaman`과 `codeload` zip은 프록시가 403, `api.github.com`은 세션 범위 403, `raw.githubusercontent.com/…/main/README.md`는 404다 | **`[미도달]`.** 없는 저장소와 비공개 저장소는 이 응답으로 구분할 수 없다 |
| 7 | GitHub 검색(MCP) | 저장소 검색 `ReproSys`는 무관한 1건(`filipeive/reprosys`)이다. `user:tarannumzaman`은 `Scminer` 1건뿐이다. 코드 검색 `"ReproSys" "SysPro" bug report system call PIN`은 무관한 1건이다 | 구현 없음. 이름이 다른 저장소는 이 검색으로 찾지 못한다 |
| 8 | Software Heritage `…/api/1/origin/https://github.com/tarannumzaman/ReproSys/get/` 및 이름 검색 `ReproSys` | 정확한 origin은 404("Origin … not found"). 이름 검색은 무관한 3건이다. 같은 소유자의 `Scminer` origin은 등록돼 있다 | 보존본 없음 |
| 9 | Zenodo 레코드 검색 `SysPro ReproSys` | 200, 4건 모두 무관한 레코드(인사관리 논문, 농화학 보고, "SYSPRO"라는 이름의 상용 소프트웨어와 연동한 CRM 개발 논문 2건)이다 | 구현 없음 |
| 10 | 같은 저자의 공개 저장소 `tarannumzaman/Scminer`(blob 없는 얕은 clone으로 파일 목록만) | `Auditlog.zip`, `LocroCon_Filter.zip`, `candidategeneration.zip`, `count.zip`, `creatematrixrow.zip`, `createPCA`, `.Rhistory`, `Scminer_preprint.pdf`. 마지막 커밋 2022-09-27 "Audit log". SysPro나 PIN 도구의 파일은 없다 | SCMiner의 자료이며 SysPro의 구현이 아니다. 인접 자료로만 기록한다 |
| 11 | SSRN `papers.ssrn.com/sol3/papers.cfm?abstract_id=5035178`(2026-10-08 기록이 적은 SSRN 판) | 403, 본문은 Cloudflare "Just a moment..." 도전 | `[미도달]` |
| 12 | Semantic Scholar API, OpenAlex API | 둘 다 429(요청 한도). 재시도하지 않았다 | `[미도달]` |
| 13 | Internet Archive `web.archive.org/web/2026/…ReproSys` | 연결이 상대편에서 재설정됨(curl 35) | `[미도달]` |
| 14 | WebSearch 1회(standard) | 결과는 arXiv HTML, arXiv 서지, emergentmind, web3.arxiv, UKy 레코드, DESCRY의 PAR 사본, 저자의 NSF 과제 보도 등이다. 요약은 "저장소 링크가 결과에 없다"는 취지다. 제3자 요약 사이트(emergentmind)는 열지 않았다 | 새 1차 출처 없음 |
| 15 | WebFetch Intel PIN 소개 페이지 `intel.com/…/pin-a-dynamic-binary-instrumentation-tool.html` | 200. 요약: "Pin is supported for Linux and Windows Platforms", 대상 ISA는 IA-32와 x86-64, 릴리스 목록과 최소 커널·glibc, ptrace·Yama 요구는 페이지에 없다. WebFetch는 작은 모델이 만든 요약을 돌려주므로 문장의 정확한 형태는 확인하지 못했다 | `[2차]`. PIN 일반 정보이며 SysPro가 쓴 판본의 서술이 아니다 |
| 16 | 저자 개인·소속 페이지, 사례 이슈 댓글 | 저자 페이지(`tingting-yu.scholar.uconn.edu`)는 200이나 SysPro 항목은 확인하지 못했다(동적 목록 여부 미확인). 사례 이슈 댓글은 `nasa/CF`, `nasa/cFE`를 세션이 막아 읽지 못했다(`add_repo`는 자격을 붙이는 조치라 호출하지 않았다) | `[미도달]` |
| 17 | 호스트 도달성(HEAD만, 내려받지 않음): `intel.com`(PIN 페이지), `srcml.org`, `philippe-fournier-viger.com/spmf/`, `pypi.org/project/gensim/3.8.3/`, `pypi.org/project/nltk/`, `manpages.ubuntu.com/…/focal/…/syscalls.2.html`, `filippo.io/linux-syscall-table/` | 모두 200 `[M]`. 2026-10-08 기록(`syspro.md` §1, S2·S4·S6)은 PIN·srcML·SPMF 호스트가 막혔다고 적었다. 오늘은 열린다 | 도달성만 확인했다. 설치하지 않았다(§4.1) |

### 1.2 이 문서가 쓰는 출처별 등급

| 출처 | 등급 | 이 문서에서의 쓰임 |
|---|---|---|
| SysPro arXiv v1 PDF·HTML·TeX | **`[전문]`** | 도구 설명 전체(§2), 도구 수준의 답(§3.3), 실행 요구(§4.1) |
| SysPro 학술지판(JSS 236, 112785) | 서지 `[M]`. 본문 **`[미도달]`** | 존재와 서지 확인, 저자 목록 차이(§1.3) |
| ReproSys 저장소 | **`[미도달]`** | 위치와 접근 불가 사실만 |
| Intel PIN 소개 페이지 | `[2차]`(WebFetch 요약) | PIN의 대상 ISA 확인(§4.1) |
| 사례 쪽 사실(`cases/ledger.md` §1a·§1b·§2, `cases/C*.md` §1, `baselines/syspro.md` §1·§6) | `[기록]`. 이슈 본문은 오늘 보존본을 다시 읽었다 | §3.2 |
| OSAL visibility probe(`baselines/availability.md` §4.2) | `[기록]` | 사건이 syscall로 보이는지의 근거. **SysPro의 결과가 아니고 어느 도구의 결과도 아니다.** OSAL 자체 시험을 syscall 관측자로 본 probe다 |
| 컨테이너 사실(`baselines/availability.md` §1.1) | `[기록]`, 일부 오늘 재확인 `[M]` | §4.1 |

### 1.3 이전 기록과 달라지거나 확정된 것

| # | 항목 | 내용 |
|---|---|---|
| 1 | `syspro.md` §0·§1·§8, `availability.md` §0 A1의 "논문(arXiv, SSRN, JSS)을 열 수 없다" | **arXiv는 오늘 열린다**(경로 1–3). SSRN과 JSS 본문은 여전히 막혀 있다(경로 5, 11). 그 기록의 `material_unavailable` 중 "논문 미확보"는 해소됐고 "구현 미확보"는 그대로다 |
| 2 | `syspro.md` S3 "system-call description 출처: 내용 미상, 제3자 요약은 man 페이지 NAME 절 TF-IDF라고 한다(미확인)" | **논문이 직접 서술한다.** 보고서와 각 system call의 man 페이지 "NAME" 절의 설명 사이 TF-IDF 유사도로 순위를 매겨 상위 n개(기본 n=10)를 KeySystemCalls로 둔다(p.15–16). system call 이름 목록의 출처는 참고문헌 [35](Ubuntu focal `syscalls(2)`)와 [36](filippo.io 표)이다(p.15, p.42). 어떤 man 페이지 묶음·판본을 설명 출처로 썼는지는 서술이 없다(`unspecified_by_reference`) |
| 3 | `syspro.md` S2, S4–S6 "판본 미상" | 논문 **본문**은 PIN, srcML, NLTK, Gensim, SPMF의 판본을 쓰지 않는다. 다만 **참고문헌 목록**이 "Nltk 3.5 documentation"([18], p.40)과 "gensim 3.8.3"([19], p.41, `pypi.org/project/gensim/`)을 적는다. srcML은 2013년 도구 시연 논문([17]), SPMF는 JMLR 2014 논문([20]), PIN은 2005년 PLDI 논문([16])만 인용하므로 판본이 없다. 참고문헌 항목의 판본을 "사용한 판본"으로 읽을지는 정하지 않았다(`from-reference`로 올리지 않는다. §4.3 항목 5) |
| 4 | `syspro.md` S7 "LLM 사용 여부: 알 수 없음(부재 미확인)" | 논문이 서술한 방법에는 LLM이 없다. 본문의 LLM 언급은 향후 입력 추출 개선 계획(p.33)과 다른 도구의 관련 연구(p.37)뿐이다 `[전문]` |
| 5 | `syspro.md` S10 "실행 예산: 100회 또는 120분(제3자 요약)" | 둘은 서로 다른 값이다. **100회**는 "known failure가 100 attempts 안에 재현되는가"라는 SysPro의 효과 판정 기준이다(p.24). **120분**은 비교용 무작위 실행(SysPro man)과 RQ4 단어 제거 실험의 시간 제한이다(p.29, p.31). 둘 다 논문의 평가 값이며 도구의 설정이 아니다 |
| 5a | `syspro.md` S11 "대상 OS·커널: 어느 발췌에도 없음" | 논문이 평가 호스트를 적는다: Intel Core i5-2400 3.10 GHz, 16 GB, Ubuntu 20.04(p.21). 커널 판본은 없다 |
| 6 | `syspro.md` S9 "실패 판정(oracle) 형식: 미상" | 논문에서도 **서술이 없다.** 효과는 "알려진 실패가 재현되는가"로 측정한다고만 쓴다(p.24). 실패를 무엇으로 판정하는지(출력, 종료 상태, crash 등)는 쓰지 않는다. 키워드 `oracle` 0건 |
| 7 | `syspro.md` S8 "입력: 보고서 본문" | 구조화 IR의 질의는 **subject, body, system call 이름** 세 가지다(p.16). 비교 절은 "bug report's subject, summary, and description"만 쓴다고 적는다(p.31). **댓글(comments)을 입력으로 쓴다는 서술은 읽은 어디에도 없다.** 정책은 댓글을 원 보고의 일부로 본다(`CONDITIONS_POLICY.md` 7행). 둘의 관계는 §4.3 항목 3 |
| 8 | `syspro.md` §5 항목 4 "관측하는 것: 최대로 instrumented thread의 system call entry·exit 사건과 failure output" | **논문에서 확인되지 않는다.** 서술된 런타임 동작은 선택한 호출 지점에 sleep을 넣고 실행하는 것뿐이다(p.20–21). entry·exit 사건의 수집, trace 기록, 로그는 서술이 없다(키워드 `trace`는 예시 표와 다른 도구 인용에만 나온다). 그 항목은 2차 자료 위의 추측이었고 논문 쪽으로 정정한다 |
| 9 | `syspro.md` §5 항목 5 "여러 thread를 한 process에서 다루는지 서술 없음" | 확인된다. 논문의 thread 언급은 p.3의 한 문장("intra-process (thread-level) concurrency faults typically affect volatile memory within a single process")뿐이고 thread 처리 방식은 서술이 없다. 키워드 `thread` 1건, `pthread`, `clone`, `fork`, `futex`, `mutex`, `lock` 0건 |
| 10 | `syspro.md` §3 끝 "벤치마크 수가 19건(17개 앱)과 24건으로 다르다" | 논문 안에서 두 표현이 다 나온다: p.4 "19 bug reports from 17 applications"와 p.4 기여 목록 및 p.22 "24 bug reports from 19 program versions across 17 unique applications". p.22의 문장이 둘을 조정한다. 이 문서는 숫자를 비교에 쓰지 않는다 |
| 11 | 저자 목록 | arXiv v1은 Zaman, Yan, Wang, Islam, Shi, Yu이고, Crossref의 JSS 서지는 Zaman, Islam, Shi, Shi, Xian, Yu다(경로 1, 5a). 학술지판이 v1과 같은 내용인지 모른다 |
| 12 | 논문 내부의 불일치(해소하지 않고 기록) | (a) 직접 언급되지 않는 비율이 p.10 §2.5.1은 38%, p.15는 61%이고 Table 2(p.9)의 직접 언급 합계는 38.5%다. (b) 효과가 p.24는 24건 중 23건 재현인데, p.29는 "SysPro succeeded in 18 benchmarks, whereas SysPro man failed in all 19 cases"라고 쓴다. 이 문서는 어느 평가 수치도 쓰지 않고 설계 서술만 쓴다 |
| 13 | `availability.md` §0 A1(b), `syspro.md` D4 "PIN·srcML·SPMF 호스트가 egress에서 막혀 있다" | 오늘은 HEAD 요청이 200이다(경로 17). 도달 가능해졌지만 PIN, srcML, SPMF의 사용 판본은 여전히 논문에 없다. 이 작업 규칙에 따라 설치하지 않았다 |

---

## 2. 도구의 정체

### 2.1 요약

| 항목 | 내용 | 근거·등급 |
|---|---|---|
| **역할** | **재현(reproduction).** "bug reports만으로" system-level concurrency bug를 재현하는 도구. 논문은 "the first automated tool to reproduce system-level concurrency bugs from bug reports"라고 쓴다. 두 단계다: (1) 보고서에서 관련 system call 이름을 뽑고 소스 코드의 위치를 찾는다(localization). (2) 입력을 만들고 그 위치를 PIN으로 계측해 재현한다. 탐지 도구도 회귀 분석 도구도 아니다. 자신을 "semi-automated"라고 부른다(p.4): 보고서에 test case가 없으면 test case 생성에 사람 수작업이 든다 | p.3, p.4, p.14, p.37 `[전문]` |
| **실행 단위** | OS process와 signal(·interrupt). 정의는 "multiple processes, signals, or interrupts access a system-wide resource (e.g., a file or device) without proper synchronization"(p.5)이고 대상은 inter-process bug다. intra-process(thread-level) fault는 구분해 "typically affect volatile memory within a single process"라고 쓴다(p.3). 문제 정의: "given a bug report, a set of processes that trigger the bug, and the source code of the buggy application process mentioned in the report"(p.4–5). 평가는 Linux 유틸리티(mv, rm, mkdir, mknod, mkfifo, ln, chmod, pxz, gzip, bzip2, bash, findutils, locate, logrotate, coreutils, cp, tail)의 C 소스다(p.22 Table 4). 한 process 안의 thread 처리는 서술이 없다 | p.2–5, p.22 `[전문]` |
| **입력** | (1) bug report: subject, body(p.16), 비교 절은 subject, summary, description(p.31). (2) 대상 응용의 **소스 코드**(C/C++, srcML 변환, p.21, p.36). (3) **system call manual**(NAME 절의 설명, p.15). (4) **응용 이름과 process 집합은 사용자가 안다고 가정한다**: 보고서의 98%가 응용 이름을 포함하므로 "our system assumes prior knowledge of it"(p.10), "we assume that users know the set of processes contributing to the concurrency bugs"(p.32). (5) **test case**: command, options, input data 세 요소를 보고서에서 정규식으로 뽑고, 빠진 요소는 category-partition(TSL)로 시스템 매개변수와 기능 지식에서 만든다(p.19–20). 문서는 이를 반수동으로 인정한다(p.4, p.14, p.32). 두 명이 수작업으로 주석을 달아 일치도 P=0.958(p.20) | p.4, p.10, p.14–16, p.19–21, p.31–32 `[전문]` |
| **관측하는 것** | **정적:** 보고서 텍스트(subject, body)와 소스의 구조(srcML XML에서 XPath로 뽑은 파일 이름, 함수 이름, 변수 이름, 주석)(p.16). **실행 중:** 서술이 없다. 논문이 쓰는 실행 단계는 PIN으로 sleep을 넣고 돌린 뒤 알려진 실패가 재현되는지 보는 것이다(p.20–21, p.24). 실패를 무엇으로 판정하는지는 쓰지 않는다. trace, 메모리 접근, lock, happens-before를 기록한다는 서술이 없다(키워드 `mutex`, `lock`, `semaphore`, `futex`, `happens-before`, `memory`(thread 문장의 "volatile memory" 1건 제외) 0건) | p.16, p.20–21, p.24 `[전문]` |
| **자원 식별** | **서술이 없다.** 목표는 "a sequence of system calls on the same shared resource from the same or different processes"(p.3)지만, 알고리즘 출력은 ⟨system call 이름, 줄 번호, 함수, 파일⟩이다(p.14). 같은 자원임은 보고서의 서술과 사용자가 준 입력(예: 같은 파일 이름 `foo`)이 전제한다. 이름·ID·slot의 대응, 객체 재생성의 세대에 관한 서술이 없다(키워드 `regist` 0건, `generation`은 모두 input/test generation의 뜻) | p.3, p.14 `[전문]` |
| **모델링하는 순서·의존** | **buggy interleaving**을 "the list of system calls that are causing the bug"로 본다. 예: ⟨P1: unlink, P2: open, P1: rename⟩(p.6, p.11). 보고서에서 같은 문장에 함께 나온 system call 쌍을 apriori(level 2, min support 100%)로 모으고 빈도순으로 순위를 매긴다(p.17–18). 쌍의 앞뒤는 소스에서 한 함수 안의 호출 순서로, 다른 함수에 있으면 call graph로 정한다(p.19). 이 쌍의 두 호출이 계측 대상 process에 있고, 다른 process의 호출이 그 사이에 끼어드는 모양이다(p.6). 초기화, 등록, readiness, 시작 동기화, 재시도에 관한 서술이 없다(키워드 `initializ`, `regist`, `startup`, `readiness`, `retry` 0건) | p.6, p.11, p.17–19 `[전문]` |
| **실행 제어(재현)** | **PIN으로 sleep 삽입.** "by inserting a sleep function between each relevant system call pair of the buggy application"(p.20). 쌍을 못 찾으면 "before and after each relevant system call"(p.20). 순위가 높은 호출부터 한 번에 하나씩 계측하고, 재현되지 않으면 다음 호출로 간다(p.20–21). 한 호출만 알 때는 먼저 호출 앞에, 안 되면 호출 뒤에 계측 지점을 둔다(p.26–27, pxz, chmod, logrotate). 첫 계측으로 재현되면 더 계측하지 않고, 아니면 다음 파일로 가며 상위 n=10개 파일까지 본다(p.19). sleep의 길이, 다른 process를 함께 실행하는 방법, process 사이 스케줄링 제어의 실체는 서술이 없다. p.14는 "permutes process interleavings by controlling scheduling [13]"이라고만 쓴다 | p.14, p.19–21, p.26–27 `[전문]` |
| **탐색의 성격** | 후보를 순위대로 한 번에 하나씩 시험하는 **탐색**이다. 관측한 실행에서 아직 실행되지 않은 순서를 만들어 내는 **예측**이 아니다(trace가 없다). 확인은 실제 재실행에서 한다. 효과 기준은 100회 안의 재현이다(p.24) | p.3, p.19–21, p.24 `[전문]` |
| **출력** | (1) 후보 위치의 순위 목록: system call 이름, 줄 번호, 함수 이름, 파일 이름(p.14). 평가는 순위, MAP, recall@K다(p.23–25). (2) 재현에 성공하면 "an interleaving schedule consisting of system calls and their associated function names and locations in the program that cause the failure"(p.3). (3) 평가표에 재현 성공 여부, 시도 횟수(NoR), 소요 시간이 있다(p.25 Table 5) | p.3, p.14, p.23–25 `[전문]` |
| **구현** | Python(NLTK, Gensim), srcML, Java(SPMF), PIN. LLM은 방법에 없다 | p.3–4, p.21 `[전문]` |
| **평가 환경** | Intel Core i5-2400 3.10 GHz, 16 GB RAM, Ubuntu 20.04 | p.21 `[전문]` |
| **평가 결과(논문의 값)** | 24건 중 23건 재현(95.8%), 평균 시도 2회(1–14), 평균 8.9분(0.64–59.09분). 실패한 건은 findutils이고 보고서에 system call 정보도 올바른 응용 이름도 없다는 이유다(p.24). 보고서의 완결도(complete, semi-complete, incomplete)는 저자가 수작업으로 분류했고 정립된 분류가 아니라고 적는다(p.29). **이 값들은 cFS의 조건이나 성능으로 가져오지 않는다.** | p.24–25, p.29–30 `[전문]` |
| **artifact 위치** | p.38 "Data availability": `https://github.com/tarannumzaman/ReproSys/tree/main`(코드와 데이터를 포함한다고 적음). 열리지 않는다(§1.1 경로 6–10) | p.38 `[전문]`, 저장소 `[미도달]` |

### 2.2 동작 흐름 (Figure 5 p.13, Figure 6 p.14)

1. **KeySystemCalls 추출.** 보고서를 system call 이름 목록과 키워드로 맞춘다. 이름이 직접 나오지 않으면 목록이 비고(p.15), 이를 메우려고 각 system call의 이름과 man 페이지 NAME 절의 설명으로 문서를 만들어 보고서를 질의로 TF-IDF 유사도를 매긴 상위 n개(기본 10)를 쓴다(p.15–16). 설명 기반 경로가 이름이 직접 나올 때도 함께 도는지는 서술이 명확하지 않다.
2. **의심 파일 순위.** 질의 세 개(subject, body, KeySystemCalls)와 문서 필드 네 개(파일 이름, 함수 이름, 변수 이름, 주석 포함 소스)의 12가지 조합마다 Gensim TF-IDF 유사도를 구해 평균 점수로 파일 순위를 만든다(p.16–17). 보고서의 함수, 파일, 변수 이름은 이 단계에 쓰인다(p.12–13).
3. **계측 지점 찾기.** KeySystemCalls에서 같은 문장에 나온 쌍을 apriori로 모아 순위를 만들고, 상위 파일에서 키워드 검색으로 그 쌍의 호출을 찾아 계측 지점으로 한다(p.17–19). KeySystemCalls가 비면 상위 파일의 system call을 하나씩 계측하는 것이 최악의 경우이고, 논문은 이 경우를 실험에서 만나지 않았다고 쓴다(p.18).
4. **test case 생성.** 정규식으로 command, options, input data를 뽑고 부족하면 TSL로 만든다(p.19–20).
5. **재현.** PIN으로 위치에 sleep을 넣고 실행해 알려진 실패가 재현되는지 본다. 안 되면 다음 위치로 간다(p.20–21, p.24).

### 2.3 논문이 밝힌 범위·한계 중 cFS와 관련 있는 것

| # | 서술 | 근거 | cFS와의 관계 (분석, 미실행) |
|---|---|---|---|
| 1 | 대상은 inter-process(system-level) bug이고 intra-process(thread-level) fault와 구분한다 | p.2–3 | native cFS는 한 Linux process이고 OSAL task는 thread다(`availability.md` §3 `[기록]`). 사례의 의존성은 이 task 사이에 있다. 로드맵 §1.2는 Process-Level을 "애플리케이션·task 단위"라는 운용 정의로 쓰고 OS process로 한정하지 않는다. SysPro의 단위(OS process)와의 차이를 비교 논문 처음에 밝혀야 한다 |
| 2 | 입력에 "버그를 일으키는 process 집합"과 응용 이름을 사용자가 안다고 가정한다 | p.4–5, p.10, p.32 | 한 process 안의 task 쌍을 이 입력으로 어떻게 주는지는 서술이 없다. native cFS에서 "process 집합"은 하나다 `[기록]` |
| 3 | test case는 보고서에서 뽑거나 사람이 만든다(반수동) | p.4, p.14, p.19–20, p.32 | cFS의 "command"는 `core-cpu1` 실행과 시작 스크립트다. C3·C5 보고서에는 실행 방법이 없고, C1·R2 보고서에는 재현 방법이 일부 있다(§3.2) |
| 4 | 보고서의 품질에 크게 의존한다. 응용 이름과 system call 정보가 없으면 재현하지 못한 사례가 있다 | p.24, p.32, p.34 | 다섯 사례의 보고서 본문에는 Linux system call 이름이 없다 `[기록]`(§3.2). 이것이 SysPro의 1단계 결과를 어떻게 바꾸는지는 실행하지 않아 불명이다 |
| 5 | 대상은 C/C++ 응용이다. srcML은 C 코드를 XML로 바꾼다 | p.21, p.36 | cFS는 C다. C2의 앱 소스는 비공개다 `[기록]` |
| 6 | 계측 지점은 system call의 소스 호출 지점이다. 쌍은 보고서의 문장 동시 출현에서 온다 | p.8, p.17–19 | cFE/OSAL API(`OS_CountSemCreate`, `CFE_ES_GetAppID` 등)와 메모리 갱신은 syscall이 아니다 `[기록]`(§3.2) |
| 7 | 성공의 정의는 "알려진 실패의 재현"이고 판정 방법은 쓰지 않는다 | p.24 | 사례의 현상은 오류 반환, 상태 코드, 초기화 실패 종료, crash다 `[기록]`. 판정 방법을 모르므로 대응도 불명이다 |
| 8 | 평가는 Linux 유틸리티와 Ubuntu 20.04에서만 했다. 일반화하지 못한다고 쓴다 | p.21–22, p.33 | 사례의 원 플랫폼은 C1 Ubuntu 21.10, C3 Microblaze 등이다 `[기록]`. 해당 여부는 §4.3 |
| 9 | 향후: LLM으로 입력 추출 개선, "concurrent bugs and web-based sequential bugs"로의 확장 의도 | p.33 | 향후 계획이다. 현재 기능이 아니다 |

---

## 3. 여섯 질문 × 다섯 사례

### 3.1 읽는 법

셀의 라벨은 아래 셋 중 하나다. 한 칸에 관측 가능성, 표현 가능성, 실제 재현 성공을 합치지 않는다(로드맵 §10.2).

| 라벨 | 뜻 |
|---|---|
| **답** | 출처의 쪽·절로 답했다(answered with source page/section) |
| **분석** | analysis from stated design (not run). 읽은 설계 서술에서 도출했고 실행하지 않았다 |
| **불명** | not determinable from sources read |

질문 번호는 로드맵 §10.2의 순서다.

1. 필요한 **관측 사건**을 수집하는가?
2. 서로 다른 이름·ID·필드가 같은 **자원·세대**를 가리키는 것을 식별하는가?
3. **생성·등록·초기화 완료와 의존 사용**의 관계를 표현하는가?
4. 필요한 **순서 또는 허용된 오류 처리** 정보를 입력받는가, 추론하는가?
5. 아직 실행되지 않은 순서를 **예측·탐색**하는가? 실제 실행에서 **확인**할 수 있는가?
6. **출력**은 후보·재현 성공·오류 결과 중 무엇이며 원 사례의 현상과 대응하는가?

"분석" 셀의 "서술에 경로가 없다"는 "읽은 설계 서술 안에서 그 사건이 입력이 되는 경로를 찾지 못했다"는 뜻이다. 실행 결과가 아니다. 셀의 근거는 별도 표시가 없으면 SysPro 논문(`[전문]`)의 쪽이고, 사례 쪽 사실은 `[기록]`이다.

**모든 "분석" 셀에 공통으로 걸린 전제 세 가지.**
- **전제 1(제어·관측 단위).** SysPro가 다루는 위치는 system call의 소스 호출 지점이다(p.17–20). 사례의 의존 사건이 이 단위에 놓여야 계측 지점이 될 수 있다. 구현(ReproSys)을 읽지 못해 실제 계측 지점이 syscall 이름 목록에 한정되는지는 확인하지 못했다. 이 문서의 "분석" 셀은 **논문에 서술된 단위**에 대한 것이다.
- **전제 2(실행 단위).** SysPro는 "process 집합"과 "보고서의 응용 process"를 입력으로 가정한다(p.4–5, p.32). 한 process 안의 task(thread) 쌍을 줄 수 있는지는 서술이 없어 **불명**이다.
- **전제 3(입력).** bug report(subject, body), 대상 소스, system call manual, 응용 이름·process 집합에 대한 사전 지식, test case가 있어야 한다(p.10, p.14–16, p.19–20). 사례별 충족 상태는 §3.2와 §4.1에 적는다.

### 3.2 사례 쪽에서 쓴 사실 (저장소 기록. 이슈 본문은 오늘 보존본을 다시 읽었고 나머지는 재검증하지 않음)

SysPro의 입력은 보고서다. 아래 표는 각 보고서 **본문**에 무엇이 있는지와 사례의 현상을 적는다. 댓글은 읽지 못했다(C1 5건, C3 14건, C5 9건, R2 2건. C2는 본문 8문장이 전부이고 로그아웃 HTML 기준 댓글 수는 확정하지 않았다 `cases/C2.md` §1.1). 정책은 댓글을 원 보고의 일부로 보지만(`CONDITIONS_POLICY.md` 7행), SysPro 논문은 댓글을 입력으로 서술하지 않는다(§1.3 항목 7).

| 사례 | 의존성과 현상 | 보고서 본문이 이름 붙인 것 | 본문의 재현 서술 | 출처 |
|---|---|---|---|---|
| C1 CF#184 | provider가 이름 있는 OSAL count semaphore를 만든 뒤에야 CF의 `OS_CountSemGetIdByName`이 성공한다. provider는 원 보고에서 미식별("CI/TO or some other dedicated I/O app")이고 공개 BP 구성에서는 `OS_CountSemCreate`다. 수정 전 CF는 한 번만 조회하고 실패하면 초기화 실패 이벤트와 `APP_ERROR` 종료다. 수정은 `OS_ERR_NAME_NOT_FOUND`일 때만 25회 × 100 ms 재시도다. 생성이 조회보다 먼저임을 정하는 장치가 없다 | 함수 `CF_CFDP_InitEngine()`, `CFE_ES_WaitForStartupSync()`. 파일은 코드 링크의 `cf_cfdp.c`. Linux system call 이름은 없다 | "Start CF before the app that creates the sem", "Add an artificial delay during startup for the app that creates the sem" | `cases/ledger.md` §1a·§1b·§2, `cases/C1.md` §1 `[기록]` |
| C2 cFE#198 | 앱 P의 late init 완료와 다른 앱 R의 의존 호출. P·R은 비공개(EVA CWS)이고 통신 방식·초기화 내용이 공개되지 않았다. 공개된 것은 단일 상태 시작 동기화와 6.6.0a의 두 단계 방식이다 | 함수 `CFE_ES_WaitForStartupSync()`, 상태 `OPERATIONAL`. 앱 이름, 증상, 로그는 없다 | 없음. 본문은 발생을 "observed in the EVA CWS project"라고만 쓴다 | `cases/C2.md` §1.1, `cases/ledger.md` §1a·§2 `[기록]` |
| C3 cFE#73 | SB task의 `CFE_SB.AppId`와 EVS task의 `EVS_AppID`·등록 플래그가 설정되기 전에 TIME task가 `CFE_SB_CreatePipe` 뒤 `CFE_EVS_SendEventWithAppID`(AppId=0)를 부르고 `EVS_IsFiltered(0xFFFFFFFF)`가 범위 검사 없이 접근해 segfault로 core가 죽는다. 우선순위(TIME 60, EVS 61, SB 64)가 순서를 만든다. 원 플랫폼은 Microblaze다 | 파일 `cfe_es_objtab.c`. 함수 `CFE_SB_CreatePipe()`, `CFE_EVS_SendEventWithAppID()`, `EVS_NotRegistered()`, `EVS_SendEvent()`, `EVS_IsFiltered()`. 변수 `CFE_SB.AppId`, `CFE_EVS_GlobalData.EVS_AppId`. 우선순위 숫자 | 없음. 발생 과정을 순서대로 설명하는 서술은 있다 | `cases/ledger.md` §1a·§1b, `cases/C3.md` §1.1 `[기록]` |
| C5 cFE#72 | `CFE_ES_AppCreate`가 ES lock을 `OS_TaskCreate` 앞에서 풀고 뒤에서 다시 잡아 `TaskTable`을 채운다. 그 사이 새 task의 `CFE_ES_GetAppID`가 `CFE_ES_ERR_APPID`를 반환할 수 있다. readiness violation은 "등록 전 조회"이고 일찍 실행된 것만으로는 위반이 아니다. 수정(6.4.2)은 lock 범위 확장이다 | 함수 `CFE_ES_UnlockSharedData()`, `OS_TaskCreate()`, `CFE_ES_LockSharedData()`, `CFE_ES_GetAppID()`, `CFE_ES_CreateObjects`, `CFE_ES_CreateChildTask`. `clone`, `fork`, `pthread_create`는 없다 | 없음 | `cases/ledger.md` §1a·§1b, `cases/C5.md` §1 `[기록]` |
| R2 cFE#2663 | SB가 `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`로 AppId를 저장하기 전에 ES/EVS task가 SB API 경로(`CreatePipe`, `Subscribe`, 이벤트 송신)를 실행해 `EVS_GetAppDataByID`가 NULL을 돌려주고 `ILLEGAL_APP_ID`가 된다. 서술은 "이벤트 요청 거부"다. gdb 3회 모두 설정 전 사용 24회다(로드맵 §2.2 `[재검산]`). core 시작 직렬화(S1)가 EVS·ES를 SB보다 앞세운다. 고정 순서인지 스케줄 의존인지는 미결이다 | 함수 `CFE_SB_CreatePipe()`, `CFE_SB_Subscribe()`, `CFE_EVS_SendEventWithAppID()`. 상수 `CFE_EVS_APP_ILLEGAL_APP_ID`. 코드 링크 `cfe_evs.c#L185` | 단계 목록: `make SIMULATION=native prep`, `make`, `make install`, `cfe_evs.c`의 185번째 줄에 breakpoint, gdb로 실행. 본문은 "there is no event outputted nor is the return code set to any variable"이라고 쓴다 | `cases/ledger.md` §1a·§2, `cases/R2.md` §1.2 `[기록]`. 실행 증거는 gdb 원시 기록(`env/logs/r2_g1..3_*`, `results/R2_gdb_v701.md`) |
| 공통 | native cFS는 한 Linux process이고 OSAL task는 `pthread_create` thread다. OSAL 자체 시험에서 named count semaphore의 생성과 이름 조회는 syscall 0건이었다. SB pipe는 `mq_open("<pid>.<name>")`을 부른다. `OS_TaskCreate`는 `clone3`로 보인다 | — | — | `availability.md` §3, §4.2 `[기록]`. OSAL 시험의 probe이며 SysPro의 결과가 아니다 |

보고서 본문의 literal syscall 이름 대리 probe(`syspro.md` §6, `[기록]`): C1 `bind`, `exit`, `sync`, `time`, C2 `sync`, C3 `time`, C5 `read`, R2 없음. 모두 영어 단어이거나 cFE API 이름의 일부이고 syscall 호출이 아니다. **이 probe는 SysPro의 1단계가 아니다.** 목록과 토큰화 방식은 에이전트의 선택이었고(`unspecified_by_reference`), 논문이 서술한 1단계(이름 직접 일치, 이어서 man 페이지 NAME 절 TF-IDF)와 다르다. 오늘 다시 돌리지 않았다. SysPro의 1단계 출력은 **불명**이다.

SysPro의 분류로 읽는 보고서의 정보 종류(p.9–10 Table 2: system call 이름, 설명, 함수 이름, 파일 이름, 변수 이름): 다섯 본문 모두 함수 이름을 포함하고, C1·C3·R2는 파일 이름을 링크나 문장으로 포함하며, C3은 변수 이름을 포함한다. 논문의 보고서 완결도 분류(complete, semi-complete, incomplete, p.29)는 저자의 수작업 판단이라 이 문서는 사례에 붙이지 않았다.

### 3.3 도구 수준의 답 (사례와 무관)

| Q | 라벨 | SysPro 논문(`[전문]`)의 답 | 근거 |
|---|---|---|---|
| 1 관측 사건 | 답 | **실행 중 사건을 수집한다는 서술이 없다.** 입력으로 쓰는 것은 보고서 텍스트와 소스의 정적 구조(파일, 함수, 변수 이름, 주석)다. 런타임 단계는 선택한 system call 호출 지점에 sleep을 넣는 PIN 계측이고, 확인은 알려진 실패가 재현되는지 보는 것이다. trace 기록, 메모리 접근, lock, 사용자 수준 registry, 시작 동기화, happens-before를 사건으로 쓴다는 서술이 없다 | p.16, p.20–21, p.24 |
| 2 자원·세대 식별 | 답 | 알고리즘은 자원을 식별하지 않는다. 출력은 ⟨system call 이름, 줄, 함수, 파일⟩이고(p.14), 같은 자원임은 보고서와 사용자 입력(예: 같은 파일 이름)이 전제한다. 이름·ID·slot 대응, 객체 재생성의 세대에 관한 서술이 없다(키워드 `regist` 0건) | p.3, p.14 |
| 3 생성·등록 vs 의존 사용 | 답 | 표현하는 것은 **system call 쌍의 순서**와 그 사이에 끼어드는 다른 process의 호출이다(예: unlink와 rename 사이의 open, p.6, p.11). 벤치마크의 쌍은 모두 파일 시스템 관련 syscall이다(p.22 Table 4, p.25 Table 5: unlink, rename, fstatat, openat, mkdir, chmod, mknod, stat, fchmodat, write, open, read). 생성·등록·초기화 완료, readiness, 시작 동기화에 해당하는 개념은 서술이 없다(키워드 `initializ`, `regist`, `startup`, `readiness` 0건) | p.6, p.11, p.22, p.25 |
| 4 필요 순서·허용 오류 입력 | 답 | 필요한 순서나 허용된 오류 처리를 받는 입력이 없다. 입력은 보고서, 소스, system call manual, 응용 이름·process 집합, test case다. 순서 후보는 보고서에서 같은 문장에 함께 나온 system call 쌍의 빈도와 소스의 호출 순서로 **추론한다**(p.17–19). "무엇이 잘못인가"는 "the failure described in the report"(p.5)다. 재시도·허용 오류의 서술이 없다(키워드 `retry`, `error`, `return value` 0건) | p.5, p.10, p.14–19 |
| 5 예측·탐색, 확인 | 답 | **탐색**이다. 순위가 높은 후보 위치부터 한 번에 하나씩 sleep을 넣고 실행하며, 호출 앞, 뒤, 쌍 사이를 바꿔 본다. 첫 계측으로 재현되면 멈추고 아니면 다음 위치와 다음 파일(상위 n=10개)로 간다. trace에서 아직 실행되지 않은 순서를 만들어 내는 예측은 서술이 없다. 확인은 실제 재실행에서 알려진 실패의 재현이다. 효과 기준은 100회 안의 재현이다(논문의 평가 값) | p.19–21, p.24, p.26–27 |
| 6 출력 | 답 | (1) 후보 system call 위치의 순위 목록. (2) 재현에 성공하면 interleaving schedule(system call, 함수 이름, 위치). (3) 평가표의 재현 여부, 시도 횟수, 시간. 후보 race 목록도 readiness violation 판정도 아니다. 실패를 무엇으로 판정하는지는 서술이 없다 | p.3, p.14, p.24–25 |

### 3.4 5 × 6 라벨 요약

| | Q1 관측 | Q2 식별 | Q3 생성·사용 | Q4 입력 | Q5 예측·확인 | Q6 출력 |
|---|---|---|---|---|---|---|
| **C1** | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| **C2** | 불명 | 불명 | 분석 | 답 | 불명 | 불명 |
| **C3** | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| **C5** | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| **R2** | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |

Q4가 다섯 사례 모두 "답"인 것은 도구 수준의 답이 사례와 무관하기 때문이다(입력 필드가 없다).

### 3.5 사례별 셀 (근거 포함)

#### C1 — CF#184 (named count semaphore의 생성 vs 이름 조회)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | SysPro는 사건을 수집하지 않는다(§3.3 Q1). 사례에서 필요한 사건은 provider의 이름 등록(`OS_CountSemCreate`가 반환한 시점)과 CF의 `OS_CountSemGetIdByName` 조회와 그 반환값이다. OSAL 자체 시험에서 생성 3회와 조회 4회가 syscall 0건이었다. SysPro가 제어하는 위치는 system call의 소스 호출 지점이므로(p.17–20) 이 두 사건은 직접 계측 지점이 되지 않는다. 보고서가 주는 사람용 방법("provider의 시작을 지연")과 닮은 지연을 provider 초기화 중 어느 syscall 앞뒤에 둘 수 있는지, SysPro의 1단계가 그 위치를 고르는지는 불명이다 | p.17–20; `availability.md` §4.2, `syspro.md` §6 `[기록]` |
| 2 | 분석 | 이름 `BP0_tsem` → OSAL object ID → 이름 테이블 slot → 객체를 같은 자원으로 잇는 장치가 서술에 없다. 객체가 삭제·재생성될 때 같은 세대로 보지 않는 장치도 서술이 없다 | p.3, p.14 |
| 3 | 분석 | "생성 → 사용"은 system call 쌍의 순서로 표현될 수 있는 모양이다. 그러나 이 사례의 두 사건은 syscall이 아니다. OSAL 레코드가 예약 중인지, 공개 완료인지, CF가 재시도 중인지 최종 실패인지 구분하는 개념은 서술에 없다(`retry` 0건) | p.6, p.11 |
| 4 | 답 | 도구 수준 답과 같다. 필요한 순서("생성이 조회보다 먼저")와 허용 오류("재시도는 `OS_ERR_NAME_NOT_FOUND`일 때만")를 넣을 입력이 없다. (참고, 분석: 보고서 본문의 함수 이름 `CF_CFDP_InitEngine()` 같은 단어는 질의 텍스트에 들어가므로 파일 순위에 영향을 줄 수 있다. p.16) | p.5, p.14–19 |
| 5 | 분석 | SysPro의 제어는 "후보 위치에 sleep을 넣는다"다. C1의 나쁜 순서(provider의 생성보다 CF의 조회가 먼저)는 provider 쪽 지연으로 만들 수 있는 종류의 순서다(보고서 본문도 같은 방법을 적는다). 이것은 SysPro가 그 위치를 계측 대상으로 고를 수 있는가와는 별개이고, 그 점은 불명이다. 확인은 실제 재실행에서 한다(p.24). PI 결정 P-C1HOLD(생성 직전 보류, 사건 기반 해제)에 해당하는 서술은 없다. SysPro의 sleep은 길이가 서술되지 않고, "sleep"이라는 서술이라 시간 기반으로 읽힌다. 사건 기반 해제의 서술은 없다 | p.19–21, p.24; 보고서 본문 `[기록]`; P-C1HOLD [D] |
| 6 | 분석 | 출력은 후보 위치 순위와 재현 성공 시 schedule이다(p.3, p.14). C1의 현상은 CF의 초기화 실패 이벤트와 `APP_ERROR` 종료다 `[기록]`. 재현의 판정 방법을 논문이 쓰지 않으므로(p.24) 이 현상과의 대응은 불명이다. 수정 후 재시도로 회복한 경로가 재현으로 세어지는지도 서술이 없다 | p.3, p.14, p.24; `ledger.md` §1b `[기록]` |

#### C2 — cFE#198 (late init 완료 vs 다른 앱의 의존 호출)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 불명 | 입력의 필수 요소인 대상 응용의 소스가 비공개다(EVA CWS). 호출 지점도 사건도 정해지지 않는다. 보고서 본문은 `CFE_ES_WaitForStartupSync()` 외에 함수를 이름 붙이지 않는다 | p.4–5; `cases/C2.md` §1.1 `[기록]` |
| 2 | 불명 | P의 상태와 R의 호출 대상이 무엇인지 정해지지 않는다 | `ledger.md` §2 `[기록]` |
| 3 | 분석 | 도구 쪽 서술만으로 말할 수 있다. 관계가 system call 쌍의 순서이고 "초기화 완료"나 시작 동기화 통과에 해당하는 개념이 없다(키워드 `initializ`, `startup` 0건). P의 late init 끝에 syscall 호출 지점이 있는지는 비공개라 사례 쪽은 불명이다 | p.6, p.11 |
| 4 | 답 | 도구 수준 답과 같다. barrier 통과와 앱 고유 초기화 완료를 구분하는 입력이 없다 | p.5, p.14–19 |
| 5 | 불명 | 후보 위치가 정해지지 않았다 | — |
| 6 | 불명 | 원 보고에 실패 현상의 서술과 로그가 없다. "R이 P를 호출했다"도 추론이다(`ledger.md` §1a). 이 사례는 역할 재구성(P-C2 [D])으로만 다룬다 | `ledger.md` §1a·§2 `[기록]` |

#### C3 — cFE#73 (SB/EVS AppId 설정 vs TIME의 이벤트 요청)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | SysPro는 사건을 수집하지 않는다. 사례의 사건은 SB·EVS의 AppId 저장과 EVS 등록 플래그 설정(메모리 쓰기), TIME의 `CFE_SB_CreatePipe` 뒤 `CFE_EVS_SendEventWithAppID(AppId=0)`와 `EVS_IsFiltered`의 범위 검사 없는 접근(메모리 읽기)이다. pipe 생성의 `mq_open`은 syscall로 보이지만 의존 상태가 아니다 `[기록]`. 보고서의 함수·변수 이름(`EVS_IsFiltered`, `CFE_SB.AppId` 등)은 파일 순위의 질의에 쓰일 수 있다(p.12–13, p.16). 그 순위가 어느 파일을 위로 올리는지, 그 파일에서 syscall 이름이 계측 지점이 되는지는 불명이다. AppId를 저장(publish)하기 전에 SB thread가 거치는 시작 syscall(`prctl`, `sched_setaffinity`, `clock_nanosleep`, `rt_sigaction`)이 있다는 소스 분석이 있으나 이것이 SysPro의 선택과 같은지는 불명이다 | p.12–13, p.16–20; `availability.md` §3, `syspro.md` §6 `[기록]` |
| 2 | 분석 | SB AppId, EVS 자신의 AppId, EVS 등록 플래그가 서로 다른 필드다(`ledger.md` §1a). SysPro의 서술에 필드 단위의 식별이 없다 | p.3, p.14 |
| 3 | 분석 | SB·EVS 초기화 완료(publish) 뒤에야 TIME의 이벤트 요청이 유효하다는 관계를 표현할 장치가 서술에 없다. 순서는 task 우선순위가 만든다(`[기록]`). SysPro의 순서 모델은 system call 쌍이다 | p.6, p.17–19; `ledger.md` §1a |
| 4 | 답 | 도구 수준 답과 같다 | p.5, p.14–19 |
| 5 | 분석 | 순서가 우선순위로 정해진다는 점과 SysPro의 sleep 삽입이 어떻게 맞물리는지 논문에 서술이 없다(키워드 `priority`, `scheduler`, `kernel` 0건). 지연이 SB thread 쪽(AppId 저장 전)에 놓이면 나쁜 순서 쪽으로 작용한다는 것이 소스 분석이다 `[기록]`. 원 플랫폼 Microblaze와 PIN의 대상 ISA(IA-32, x86-64, Intel 페이지 요약 `[2차]`)의 관계는 불명이다. 이 사례의 실행 가능한 형태는 사례 기록의 파생 구성(32-bit x86, `deviation`)이다 `[기록]` | p.20–21; `syspro.md` §6, `ledger.md` §1b `[기록]` |
| 6 | 분석 | 사례의 현상은 `EVS_IsFiltered`의 잘못된 인덱스 접근에 따른 segfault, 즉 cFE core의 crash다 `[기록]`. SysPro의 출력은 후보 위치 순위와 재현 성공 시 schedule이다. 재현의 판정 방법이 서술에 없어(p.24) crash를 재현으로 세는지 알 수 없다. 대응은 불명이다 | p.3, p.14, p.24; `cases/C3.md` §1.1 `[기록]` |

#### C5 — cFE#72 (task 생성과 레지스트리 등록 사이의 공백)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | SysPro는 사건을 수집하지 않는다. 사례의 사건은 `OS_TaskCreate`(`clone3` syscall로 보임), ES lock 해제와 재획득 사이의 `TaskTable` 등록 쓰기(메모리), 새 task의 `CFE_ES_GetAppID` 읽기(메모리, lock 아래)다. 이 중 syscall로 보이는 것은 `clone3`뿐이다 `[기록]`. 보고서 본문은 `clone`, `fork`, `pthread_create`를 이름 붙이지 않는다. SysPro의 keyword 검색은 Linux system call 이름의 소스 호출 지점을 찾는다(p.18–19). thread 생성의 취급은 서술이 없다(전제 2) | p.18–19; `availability.md` §3, §4.2, `syspro.md` §6 `[기록]` |
| 2 | 분석 | OS task ID ↔ ES AppId ↔ `TaskTable` slot의 대응과 "등록 전" 상태를 식별하는 장치가 서술에 없다 | p.3, p.14 |
| 3 | 분석 | 생성(`clone3`) → 등록(메모리 쓰기) → 조회(메모리 읽기)에서 생성 한 조각만 syscall이다. readiness violation은 "등록 전 조회"이고 일찍 실행된 것만으로는 위반이 아니다 `[기록]`. 이를 표현하려면 등록 사건과 조회의 반환 결과가 필요한데 SysPro의 순서 모델은 system call 쌍이다 | p.6, p.17–19; `ledger.md` §1a |
| 4 | 답 | 도구 수준 답과 같다 | p.5, p.14–19 |
| 5 | 분석 | 참조 OSAL에서는 `clone3` 직후의 지연이 child를 막고, 약 300 ms를 넘으면 child가 `OS_ERR_OBJECT_IN_USE`로 시작하지 못한다는 소스 분석이 있다 `[기록]`(`syspro.md` §6, 분석, 미실행). 사례가 필요로 하는 과거 OSAL·cFE 판본에서의 사정은 검토하지 않았다. 따라서 SysPro식 호출 앞·뒤 지연이 이 창에 닿는지는 불명이다. 탐색과 실제 재실행 확인은 §3.3 Q5와 같다 | p.19–21, p.26–27; `syspro.md` §6 `[기록]` |
| 6 | 분석 | 사례의 현상은 새 task의 첫 조회 실패(`CFE_ES_ERR_APPID`)이고 이후 영향은 앱에 달려 있다 `[기록]`. 재현의 판정 방법이 서술에 없다(p.24). 대응은 불명이다 | p.3, p.14, p.24; `ledger.md` §1a `[기록]` |

#### R2 — cFE#2663 (SB AppId 설정 vs ES/EVS의 이벤트 요청)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | SysPro는 사건을 수집하지 않는다. 사례의 사건은 `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`의 저장(메모리 쓰기), ES/EVS task의 SB API 경로 호출, `EVS_GetAppDataByID`의 NULL과 `ILLEGAL_APP_ID` 반환이다. pipe 생성의 `mq_open`은 보이지만 AppId 저장과 반환값은 보이지 않는다 `[기록]`. 보고서 본문은 test case 비슷한 단계(빌드 명령, breakpoint, gdb 실행)를 적는다. SysPro의 정규식이 이를 command, options, input data로 읽는지는 불명이다. 읽지 못하면 사람이 TSL로 만드는 경로가 남는다(p.19–20) | p.19–20; `availability.md` §3, `ledger.md` §2 `[기록]` |
| 2 | 분석 | `CFE_SB_Global.AppId`(SB 자신의 이벤트 원천 ID)와 EVS 등록 정보를 서로 다른 상태로 구분해야 한다. SysPro의 서술에 식별이 없다 | p.3, p.14 |
| 3 | 분석 | SB가 AppId를 저장한 뒤에야 SB API 경로의 이벤트 요청이 유효하다는 관계를 표현할 장치가 서술에 없다. 저장은 syscall이 아니다 | p.6, p.17–19 |
| 4 | 답 | 도구 수준 답과 같다 | p.5, p.14–19 |
| 5 | 분석 | R2의 순서가 core 시작 직렬화(S1)가 만든 고정 순서인지 스케줄 의존인지는 미결이다 `[기록]`. 고정 순서라면 기본 실행에서 나쁜 순서가 이미 나오므로 지연 삽입이 새 순서를 만드는 역할이 없다. SysPro의 서술은 "나쁜 순서가 기본 실행에서 이미 나오는 경우"를 따로 다루지 않는다. 스케줄 의존이라면 SB 쪽(AppId 저장 전) 지연이 나쁜 순서를 늘리는 방향이다 `[기록]`. 어느 쪽인지 정해지기 전에는 불명이다 | p.19–21; `ledger.md` §2, 로드맵 §2.2 `[기록]` |
| 6 | 분석 | 사례의 현상은 이벤트 요청이 `ILLEGAL_APP_ID`로 거부되고 아무것도 출력하지 않으며 반환값이 쓰이지 않는 것이다(보고서 본문). SysPro의 출력은 후보 위치 순위와 재현 성공 시 schedule이다. 재현의 판정 방법이 서술에 없고(p.24) 이 현상에는 crash, 출력, 종료 상태가 없다. 대응은 불명이다 | p.3, p.14, p.24; R2 보고서 본문, `cfe_evs.c:185-188` `[기록]` |

#### 보충: C1·C3·C5·R2의 의존 사건과 SysPro의 제어 단위 (분석, 미실행)

| 사례 | 의존 사건(공개·사용) | syscall로 보이는가 (`[기록]`, OSAL 시험과 소스 읽기) | SysPro의 서술된 제어 단위(syscall 호출 지점)와의 대응 |
|---|---|---|---|
| C1 | 이름 등록(`OS_CountSemCreate`), 이름 조회(`OS_CountSemGetIdByName`) | 0건(OSAL 자체 시험) | 직접 대응하는 호출 지점이 없다. 인접한 syscall 지점의 선택은 불명 |
| C3 | SB·EVS AppId 저장, 등록 플래그, TIME의 이벤트 요청과 범위 검사 없는 접근 | 의존 상태는 보이지 않음. pipe `mq_open`만 보임 | 직접 대응하는 호출 지점이 없다. `mq_open`은 의존 상태가 아니다 |
| C5 | `clone3`, `TaskTable` 등록 쓰기, `GetAppID` 조회 | `clone3`만 보임 | `clone3`는 호출 지점이 될 수 있으나 두 접근(등록, 조회)이 아니다. 판본 의존 |
| R2 | AppId 저장, SB API 경로의 이벤트 요청과 반환 | 의존 상태는 보이지 않음. pipe `mq_open`만 보임 | 직접 대응하는 호출 지점이 없다 |

### 3.6 이 절이 말하지 않는 것

- **SysPro가 어느 사례를 재현하지 못한다는 결과가 아니다.** 실행하지 않았고 구현이 없다. "경로를 찾지 못했다", "직접 대응하는 호출 지점이 없다"는 읽은 설계 서술과 저장소 기록 안의 일이다.
- **SysPro의 구현이 non-syscall 위치를 계측하지 못한다는 주장이 아니다.** 논문은 system call 쌍만 서술한다. 구현이 다른 지점도 받는지는 ReproSys를 읽지 못해 불명이다.
- **SysPro가 thread에 적용되지 않는다는 주장이 아니다.** 범위 서술(p.2–3)이 있지만 thread를 어떻게 다루는지는 서술이 없다.
- **SysPro의 1단계(보고서에서 syscall을 고르는 단계)가 어느 사례에서 무엇을 내놓는다는 주장이 아니다.** 실행하지 않았다. 보고서 본문의 literal 토큰 probe는 대리 확인일 뿐이다(§3.2).
- 사건이 syscall로 보이는지의 근거(OSAL 시험)는 SysPro나 어느 도구의 결과가 아니다.
- 사례의 현상과 SysPro의 성공 기준의 대응이 "불명"인 것은 논문이 판정 방법을 쓰지 않았기 때문이다. 대응이 없다는 뜻이 아니다.
- SysPro는 재현 도구다. "탐지 성공률"이나 "미탐지"의 분모를 만들지 않는다(로드맵 §10.4, P-CLASSGT).

---

## 4. 실행 가능성과 결과 상태

### 4.1 요구 조건과 오늘 확인한 상태

상태는 정책의 용어다. "충족?"은 이 컨테이너 기준이다. `[M]`은 오늘 읽기 전용으로 확인한 값이다. 이 작업에서는 설치와 빌드를 하지 않았다.

| # | 요구 | 레퍼런스 | 상태 | 충족? |
|---|---|---|---|---|
| Y1 | SysPro 구현: `github.com/tarannumzaman/ReproSys/tree/main` | p.38 | 위치는 from-reference. 학술지판의 data-availability 문장은 읽지 못했다 | **아니오.** 열리지 않는다(§1.1 경로 6–10). 주 장애 |
| Y2 | Intel PIN | p.3–4, p.20–21, 참고문헌 [16] | 도구는 from-reference. **판본은 `unspecified_by_reference`**(본문과 TeX 원본에 판본이 없다) | **아니오.** 설치하지 않았다(`pin` 없음 `[M]`). 호스트 도달성은 있다(HEAD 200 `[M]`). Intel 페이지 요약은 대상 ISA를 IA-32와 x86-64라고 한다 `[2차]`. 이 호스트에서 PIN의 어느 판본이 어느 커널에서 도는지는 확인하지 않았다 |
| Y3 | srcML(C 소스 → XML, XPath 질의) | p.16, p.21, [17] | 도구는 from-reference. 판본 `unspecified_by_reference` | **아니오.** `srcml` 없음 `[M]`. 호스트 200 `[M]` |
| Y4 | NLTK(불용어, stemming, tokenization), Gensim(TF-IDF, 유사도 행렬) | p.16, p.21, [18], [19] | 도구는 from-reference. 참고문헌 항목이 "Nltk 3.5 documentation"(p.40)과 "gensim 3.8.3"(p.41)을 적는다. 본문이 사용 판본이라고 쓰지는 않는다. Python 판본은 서술이 없다 | **아니오.** `nltk`, `gensim` 없음(`ModuleNotFoundError` `[M]`). PyPI 도달성 있다. 판본을 어떻게 읽을지는 §4.3 항목 5 |
| Y5 | SPMF(apriori), Java | p.17, p.21, [20] | 도구는 from-reference. 판본 `unspecified_by_reference` | **아니오.** SPMF 없음. Java는 있다(`java` 있음 `[M]`) |
| Y6 | system call 이름 목록과 man 페이지 NAME 절 | p.15, [35], [36] | 이름 목록의 출처는 from-reference(Ubuntu focal `syscalls(2)`, filippo.io 표). **설명에 쓴 man 페이지 묶음·판본은 `unspecified_by_reference`** | 부분. 출처 페이지는 도달 가능(HEAD 200 `[M]`). 컨테이너에 `man2`는 없다 `[기록]` |
| Y7 | bug report: subject, body(p.16), 비교 절은 subject, summary, description(p.31) | p.16, p.31 | 댓글을 입력으로 쓴다는 서술이 없다(`unspecified_by_reference`) | 부분. 다섯 사례의 본문은 있다(C2는 8문장 전체). 댓글은 읽지 못했다(§3.2) |
| Y8 | 대상 응용의 소스(C/C++) | p.4–5, p.21, p.36 | from-reference | C1, C3, C5, R2는 cFE/OSAL/CF 소스가 공개이나 **역사 판본**의 확정은 별도 작업이다(`ledger.md`, PI 결정 D13·Q16 [D]). **C2는 앱 소스가 비공개다** `[기록]` |
| Y9 | 응용 이름과 버그를 일으키는 process 집합(사전 지식) | p.10, p.32 | from-reference(가정) | native cFS는 한 process다 `[기록]`. 사례마다 어떻게 줄지는 정해지지 않았다(전제 2) |
| Y10 | test case: command, options, input data(정규식, 필요하면 TSL과 수작업) | p.19–20 | from-reference. TSL의 구체 내용은 서술이 없다 | 미확인. 사례별 command는 `core-cpu1` 실행과 시작 스크립트다 `[기록]`. 보고서에서 뽑히는지는 불명 |
| Y11 | 매개변수: KeySystemCalls n=10(p.15–16), 파일 n=10(p.19), apriori level 2와 min support 100%(p.18), 평가의 top-N=20(p.23), 효과 100회(p.24), 무작위 비교 120분(p.29) | 해당 쪽 | 논문의 값. **도구 설정으로 요구한다고 서술된 것은 n=10, apriori 매개변수까지다.** 나머지는 평가 기준이다 | cFS 조건으로 가져오지 않는다 |
| Y12 | sleep의 길이 | p.20–21 | **서술이 없다**(`unspecified_by_reference`) | 값을 고르지 않는다 |
| Y13 | 재현 성공의 판정 방법(oracle) | p.24 | **서술이 없다**(`unspecified_by_reference`) | 값을 고르지 않는다 |
| Y14 | 평가 호스트: Core i5-2400 3.10 GHz, 16 GB, Ubuntu 20.04 | p.21 | from-reference(실험 서술이며 설치 요구로 서술하지 않음) | 다르다: Ubuntu 24.04.4 LTS, 4 vCPU, 커널 `6.18.44-fc-v114`, 여유 디스크 16 GB `[M]`. 이 호스트를 쓰면 `deviation`이다 |
| Y15 | LLM·외부 API | — | 서술된 방법에 없다 | 해당 없음 |

기준 환경 사실(`availability.md` §1.1 `[기록]`, 오늘 재확인 `[M]`): ptrace가 되는 `strace`와 `gdb`가 있다. `/dev/kvm` 없음, vmx 플래그 0. 이 호스트가 PIN을 실행할 수 있는지는 PIN의 판본이 정해지지 않아 확인할 수 없다. 2026-10-08 기록이 적은 "PIN은 kernel module이나 reboot이 필요 없다"는 가정은 레퍼런스가 아니다(`syspro.md` S12).

### 4.2 결과 상태 (로드맵 §10.4)

오늘 적용되는 상태는 다섯 사례 모두 **`NOT_RUN`**이다(환경·자료·결정 등으로 실행하지 못함. 미탐지 수에 넣지 않음).

| 사례 | 상태 | 이유(사실) | 별도 메모(분석 또는 미확인) |
|---|---|---|---|
| C1 | `NOT_RUN` | 구현 미확보(Y1). PIN 등 도구 미설치와 판본 미정(Y2–Y5). sleep 길이와 판정 방법 미서술(Y12, Y13) | 공개 BP 구성이 있어 소스는 마련될 수 있다(PI D22 [D], 역사 판본은 별도) |
| C2 | `NOT_RUN` | 위와 같고, 대상 앱 소스가 비공개(Y8) | 역할 재구성(P-C2 [D])으로만 다룰 수 있는 사례. 재구성 구성은 원 시스템이 아니다 |
| C3 | `NOT_RUN` | 구현 미확보. 원 플랫폼 Microblaze의 OS 미확인 `[기록]`. PIN의 대상 ISA가 IA-32와 x86-64라는 Intel 페이지 요약 `[2차]` | 파생 구성(32-bit x86)은 `deviation`이다 `[기록]` |
| C5 | `NOT_RUN` | 구현 미확보. 역사 판본(6.4.0·6.4.1 tarball)의 소스·OSAL 확정은 별도 작업이다 `[기록]` | 판본 대응 실패와 도구 실패는 구분한다(PI D13·Q16 [D]) |
| R2 | `NOT_RUN` | 구현 미확보 | 순서가 고정인지 스케줄 의존인지 미결이다(`ledger.md` §2) |

- `PATH_NOT_EXECUTED`, `OBSERVATION_GAP`, `ANALYSIS_GAP`, `REPORTED`는 도구를 실행한 뒤에만 쓰는 상태라 오늘은 적용되지 않는다.
- `INAPPLICABLE`은 "원 기법의 명시된 입력·대상 범위에 해당하지 않음"이다. 근거 후보(p.2–3의 inter-process 범위, p.4–5의 process 집합 입력)가 있으나 이 문서가 붙이지 않는다(§4.3 항목 1).
- 원 도구를 실행할 수 없어 논문으로 기법을 재구현하면 원 도구 결과와 분리한다(로드맵 §10.4, `availability.md` §5.1의 `REIMPLEMENTATION`/`EXTENSION`). 재구현을 하지 않았다.

### 4.3 PI 판단이 필요한 항목 (결정 후보. 여기서 정하지 않았다)

| # | 항목 | 선택지와 근거 |
|---|---|---|
| 1 | `NOT_RUN`과 `INAPPLICABLE`의 구분 | (a) 다섯 사례를 `NOT_RUN`으로 둔다(구현 미확보가 실제 장애이므로 현 상태). (b) C1·C3·C5·R2를 `INAPPLICABLE`로 올린다. 근거 후보는 SysPro의 범위 서술(p.2–3: inter-process, thread-level은 구분)과 문제 정의(p.4–5: process 집합)다. 반대 근거는 thread 처리가 서술되지 않았다는 점과 로드맵 §1.2의 Process-Level 운용 정의다. (c) C2만 입력 미충족(소스 비공개, Y8)으로 별도 처리한다. 이 문서는 (a)를 적용했다 |
| 2 | 구현(ReproSys) 접근 | (a) 저자에게 연락한다. 논문 p.1의 교신저자는 T. Yu이고 첫 저자는 T. S. Zaman이다. 학술지판 저자 목록은 다르다(§1.3 항목 11). 연락하지 않았다. 누구의 이름으로 연락할지가 결정 사항이다(`availability.md` A2와 같다). (b) 이미 접근 권한이 있으면 사용자 계정으로 세션에 붙인다. (c) 현 상태를 유지하고 SysPro를 문헌·설계 분석 행으로 둔다 |
| 3 | 사례 이슈 댓글의 취급 | 정책은 댓글을 원 보고의 일부로 본다(`CONDITIONS_POLICY.md` 7행). SysPro 논문은 subject, body(summary, description)만 입력으로 서술한다(p.16, p.31). 댓글을 SysPro의 입력에 넣을지는 결정 사항이다. 넣는다면 `nasa/CF`, `nasa/cFE` 접근(`add_repo`는 자격을 붙이는 조치라 동의가 필요하다)이나 사용자가 댓글을 직접 주는 방법이 필요하다. 이 조치는 다른 baseline 문서의 입력 조건과 공통이다 |
| 4 | 학술지판(JSS) 본문 | 출판사 페이지가 403이고 Crossref 초록이 비어 있다. arXiv v1과 내용이 같은지, data-availability 문장이 같은지 모른다. 기관 접근이나 사용자가 PDF를 주는 방법이 있다. P-PAPERS [D]는 사용자 PDF 제공을 착수의 필수 조건으로 만들지 않는다고 정했다. 이 문서는 arXiv v1로 작성했다 |
| 5 | 도구 판본의 확정 | PIN, srcML, SPMF는 판본이 없다. NLTK 3.5와 gensim 3.8.3은 참고문헌 항목에만 있다. 참고문헌 항목을 사용 판본으로 읽어 `from-reference`로 둘지, `unspecified_by_reference`로 둘지는 결정 사항이다. 설치와 판본 선택은 이 작업의 범위가 아니다 |
| 6 | 재구현(`REIMPLEMENTATION`)과 확장(`EXTENSION`) | 논문에 서술된 부분: man 페이지 NAME 절 기반 syscall 선택(p.15), 12조합 구조화 IR 파일 순위(p.16–17), apriori 쌍 순위(p.17–18), 쌍 위치 탐색(p.18–19), 정규식·TSL test case(p.19–20), PIN sleep 삽입(p.20–21). 서술되지 않은 부분: sleep 길이, 실패 판정 방법, 다른 process를 함께 실행하는 방법, man 페이지 묶음, PIN 판본. cFS용 앞단(Linux syscall 이름 대신 cFE/OSAL API)을 붙이면 원 도구가 아니라 `EXTENSION`이다. 결정과 구현은 하지 않았다 |

---

## 5. 다섯 사례에서 부족한 것과 Kairos 기능 가설

이 절의 가설은 **연구 가설과 개발 후보**다(로드맵 §10.5). 근거가 있는 만큼만 올렸고 실제 비교 결과와 연결하지 않았다. 이미 가능한 기능은 차별점으로 주장하지 않는다.

### 5.1 SysPro가 이미 하는 것 — 차별점으로 주장하지 않는다

| 기능 | 근거 |
|---|---|
| 자연어 보고서에서 관련 위치 후보를 자동으로 뽑아 순위를 매긴다(구조화 IR, apriori 쌍) | p.13–19 |
| 순위대로 후보에 지연(sleep)을 넣고 실제 실행해 재현 여부를 확인한다. 호출 앞, 뒤, 쌍 사이를 바꿔 본다 | p.19–21, p.26–27 |
| 보고서에서 test case(command, options, input data)를 정규식으로 뽑는다 | p.19–20 |
| 보고서에 syscall 이름이 없을 때 설명(man 페이지 NAME 절)으로 후보를 메운다 | p.15–16 |
| 찾은 위치와 schedule을 출력한다 | p.3, p.14 |

Kairos의 "실제 실행에서 확인", "관련 작업으로 범위를 좁힌 탐색"은 방식은 다르지만 SysPro에도 재실행 확인과 순위 탐색이 있다. 새롭다고 쓰지 않는다. 차이는 어떤 사건을 입력으로 받는가, 무엇을 순서의 근거로 쓰는가, 무엇을 결과로 내는가에서 찾아야 한다.

### 5.2 가설 (근거가 있는 만큼만)

| ID | 가설 | 지지하는 근거 | 지지하지 않는 것 / 주의 | 확인할 사례 |
|---|---|---|---|---|
| H-SYSPRO-1 | **관측 사건과 제어 단위.** 다섯 사례의 공개와 사용(AppId 저장, 등록 쓰기, 이름 테이블 쓰기, API 반환값)을 사건으로 관측해 소스와 연결하는 기능이, syscall 호출 지점만 다루는 서술에 없는 사건을 입력으로 올릴 수 있다 | SysPro의 런타임 서술은 호출 지점에 sleep을 넣는 것이고 사건 수집이 없다(p.14, p.20–21 `[전문]`). 의존 사건이 syscall로 보이지 않는다는 측정은 OSAL 자체 시험이다(`availability.md` §4.2 `[기록]`, SysPro의 결과가 아님) | 논문에 서술이 없다는 것과 구현이 못 한다는 것은 다르다. ReproSys를 읽지 못했다. SysPro는 탐지가 아니라 재현 도구이고 사건 관측은 설계 목표가 아니다. "SysPro가 지연을 못 넣는다"로 쓰지 않는다. 가설은 "서술된 제어·입력 단위에 없다"까지다 | R2, C3, C5, C1 |
| H-SYSPRO-2 | **readiness 상태의 표현.** 생성·등록·초기화 완료(publish)와 의존 사용(use)을 잇고, 사용이 readiness precondition보다 먼저 일어났는지 판정하는 표현이 필요하다 | SysPro의 순서 모델은 system call 쌍과 끼어드는 다른 process의 호출이다(p.6, p.11, p.17–19). 벤치마크의 쌍은 파일 시스템 syscall이다(p.22, p.25). 초기화·등록·시작 동기화 개념이 서술에 없다(키워드 0건) `[전문]`. 다섯 사례의 위반 조건은 "생성 전 조회", "AppId 설정 전 사용", "등록 전 조회", "late init 완료 전 호출"이다 `[기록]` | publish와 use가 둘 다 syscall이면 syscall 쌍으로 표현될 수 있다. 가설은 syscall이 아닌 publish·use에 한정된다. C2는 앱 소스가 비공개라 역할 재구성(P-C2 [D])의 메커니즘 검증에만 쓸 수 있다. 강도는 중간이다 | C1, C3, C5, R2, C2(역할 재구성) |
| H-SYSPRO-3 | **이름·ID·세대 연결.** 이름(`BP0_tsem`) → OSAL object ID → slot, 그리고 OS task ID ↔ ES AppId ↔ `TaskTable` slot을 같은 자원으로 잇는 기능이 필요하다 | SysPro의 출력은 ⟨syscall 이름, 줄, 함수, 파일⟩이고 자원 식별 단계가 없다(p.3, p.14). 키워드 `regist` 0건, `generation`은 모두 input·test generation의 뜻이다 `[전문]`. 사례 쪽 필요는 `[기록]` | "서술이 없다"가 "기능이 없다"의 확인은 아니다. 자원의 동일성은 사용자가 보고서와 입력으로 준다고 읽을 수도 있다(p.10, p.32). 강도는 약하다 | C1, C5 |
| H-SYSPRO-4 | **결과의 종류와 성공 기준.** 오류 반환(`ILLEGAL_APP_ID`), 상태 코드(`CFE_ES_ERR_APPID`), 초기화 실패 종료, crash를 근거와 함께 readiness violation으로 판정해 내놓는 결과가 필요하다 | SysPro의 출력은 후보 위치 순위, 재현 성공 여부, schedule이다(p.3, p.14, p.25). 재현의 판정 방법은 서술이 없다(p.24). R2 본문은 "no event outputted nor … return code set"이라고 쓴다 `[기록]` | 재현과 탐지는 역할이 다르다. 결핍이 아니라 목적의 차이일 수 있다. SysPro가 원 벤치마크에서 실패를 어떻게 판정했는지 모르므로 R2의 현상을 못 다룬다고 쓸 수 없다. 강도는 약하다 | R2, C3, C5, C1 |
| H-SYSPRO-5 | **API 수준의 앵커.** 사례의 보고서가 쓰는 어휘(cFE/OSAL API와 전역 변수)에서 곧바로 계측·검증 지점을 정하는 기능이 필요하다 | 다섯 보고서는 cFE/OSAL API와 변수를 이름 붙이고 Linux syscall 이름은 없다 `[기록]`(§3.2). SysPro의 최종 위치 단위는 소스에서 키워드로 찾는 syscall 호출이다(p.17–19). 함수·파일·변수 이름은 파일 순위까지만 쓰인다(p.12–13, p.16) | 파일 순위 단계는 cFE API 이름을 쓸 수 있다. 그 순위가 어디로 이어지는지는 1단계를 실행하지 않아 모른다. 강도는 약하다. H-SYSPRO-1과 겹치는 부분이 있다 | C1, C3, C5, R2 |

### 5.3 근거가 부족해 가설로 올리지 않는 것

- **"SysPro는 cFS 사례를 재현하지 못한다/탐지하지 못한다."** 실행하지 않았다. 쓰지 않는다.
- **"SysPro는 보고서에 syscall 이름이 있어야 한다."** 설명 기반 경로(p.15–16)와 파일 순위의 함수 이름 질의(p.16)가 있다. 최악의 경우(목록이 빔)도 서술돼 있고 평가에서 만나지 않았다(p.18).
- **"SysPro는 thread를 다루지 못한다."** thread 서술이 한 문장뿐이고 처리 방식은 서술이 없다. 단위의 차이는 비교 논문에서 정의로 밝힌다(§2.3 행 1).
- **"지연 삽입은 고정 순서를 만들 수 없다."** R2의 순서가 고정인지 스케줄 의존인지가 미결이다. 그리고 SysPro의 sleep 길이가 서술에 없다.
- **"보고서만으로 시작하는 것이 Kairos의 장점이다."** 입력 조건의 차이일 뿐 결핍이 아니다. 그 영역이 SysPro다.
- **C2에 대한 도구 부족 가설.** 앱 소스가 비공개라 도구 쪽 서술의 부재만으로 가설을 세울 수 없다(H-SYSPRO-2에 한정).
- **원 플랫폼(Microblaze)과 PIN의 관계.** PIN의 대상 ISA는 Intel 페이지 요약 `[2차]`뿐이고 SysPro 서술이 아니다. 불명이다.

---

## 6. 확인하지 못한 것과 다음에 할 일

| # | 항목 | 이유와 다음 조치 |
|---|---|---|
| 1 | ReproSys 구현 | 익명 접근 불가, 보존본 없음. 다음 조치는 저자 문의(§4.3 항목 2) 또는 사용자 접근 |
| 2 | 학술지판(JSS) 본문과 data-availability 문장 | 출판사 403. arXiv v1과 같은지, 저자 목록이 다른 이유가 내용 변경인지 모른다(§4.3 항목 4) |
| 3 | sleep 길이, 실패 판정 방법, 다른 process를 함께 실행하는 방법 | 논문에 서술이 없다. 구현에 있을 수 있다. 값을 고르지 않는다(Y12, Y13) |
| 4 | PIN, srcML, SPMF의 사용 판본, man 페이지 묶음 | 논문에 없다. 참고문헌의 NLTK 3.5, gensim 3.8.3의 해석은 PI 판단(§4.3 항목 5) |
| 5 | SysPro 1단계(보고서 → KeySystemCalls → 파일 순위 → 쌍)가 다섯 보고서에서 내놓는 것 | 실행하지 않았다. 구현이 없으면 알 수 없다. 논문에 서술된 방법의 재구현은 가능하나 `REIMPLEMENTATION`이며 PI 결정이 먼저다(§4.3 항목 6) |
| 6 | 사례 이슈 댓글 | 프록시가 `nasa/CF`, `nasa/cFE`를 막는다. SysPro의 입력이 댓글을 포함하는지도 정해지지 않았다(§4.3 항목 3) |
| 7 | 논문 내부 불일치의 해소 | 벤치마크 수, 비직접 언급 비율, 성공 건수(§1.3 항목 10, 12). 학술지판이 바로잡았는지 모른다 |
| 8 | Figure 2–4, 6–10의 세부 | 본문 서술과 캡션 위주로 읽었다. 이 문서는 그림의 값을 쓰지 않았다 |
| 9 | 사례 쪽 사실(`ledger.md`, `availability.md`, `syspro.md` §6) | 저장소 기록을 그대로 썼다. 소스를 다시 열어 재확인하지 않았다. 이슈 본문만 오늘 보존본을 다시 읽었다 |
| 10 | SSRN, Semantic Scholar, OpenAlex, Internet Archive | 도전 페이지 또는 요청 한도 또는 연결 재설정. 새 정보를 기대하기 어렵다 |

---

## 7. 출처 목록

**1차**
- SysPro arXiv v1 PDF: https://arxiv.org/pdf/2601.09616v1 (sha256 `8fe5daa4c7c3794715632e4379d9160a9852b723dc4a649317ef6bae0a75e726`, 47쪽)
- SysPro arXiv 서지: https://arxiv.org/abs/2601.09616 (v1 하나. Comments "Accepted in JSS")
- SysPro arXiv HTML: https://arxiv.org/html/2601.09616v1
- SysPro arXiv TeX 원본: https://arxiv.org/e-print/2601.09616v1
- SysPro 학술지판(열지 못함): https://doi.org/10.1016/j.jss.2026.112785 (JSS vol. 236, 112785)
- SysPro 학술지판 Crossref 서지: https://api.crossref.org/works/10.1016/j.jss.2026.112785
- SysPro 구현(열지 못함): https://github.com/tarannumzaman/ReproSys/tree/main

**인접 자료**
- 같은 저자의 공개 저장소(SCMiner, SysPro 자료 아님): https://github.com/tarannumzaman/Scminer
- Intel PIN 소개 페이지(WebFetch 요약만): https://www.intel.com/content/www/us/en/developer/articles/tool/pin-a-dynamic-binary-instrumentation-tool.html
- SysPro 참고문헌이 가리키는 system call 이름 출처(HEAD 도달성만 확인): https://manpages.ubuntu.com/manpages/focal/en/man2/syscalls.2.html, https://filippo.io/linux-syscall-table/
- SysPro 참고문헌이 가리키는 패키지 페이지(HEAD 도달성만 확인): https://pypi.org/project/gensim/3.8.3/, https://www.nltk.org/

**저장소 기록(이번에 재검증하지 않음, 이슈 본문 제외)**
- `baselines/syspro.md`(2026-10-08. §1.3의 정정 항목 참조), `baselines/availability.md`, `baselines/logs/syspro_*_20261008.log`
- `cases/ledger.md`, `cases/C1.md`, `cases/C2.md`, `cases/C3.md`, `cases/C5.md`, `cases/R2.md`
- `results/R2_gdb_v701.md`와 `env/logs/r2_g1..3_*`(R2 실행 증거)
- `plan/ROADMAP_20261015.md` §10, `Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md`, `CONDITIONS_POLICY.md`

**같은 계보의 분석 문서**
- `plan/methods/DESCRY.md`, `plan/methods/RRF.md`, `plan/methods/SimRacer.md`, `plan/methods/RacePro.md`
