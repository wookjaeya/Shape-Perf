# DESCRY (ESEC/FSE 2017) 분석 — Kairos 비교용

- 작성일: 2026-10-15 (UTC 09:06–09:30, 경로 시도와 읽기 시각).
- 보완: 2026-10-16 (UTC 00:23–00:33). `plan/process_race_method_matrix.md`가 남긴 후속 항목 중 DESCRY에 걸린 것을 처리했다. (1) 원문 경로를 다시 시도했다(그 문서 §5.1 A5. 이 문서 §1.1 경로 20–29). 논문 두 사본의 해시는 같았고 주요 인용을 쪽별로 다시 대조했다. Zaman 학위논문은 기관 레코드에서 초록 전체를 읽었고 본문은 여전히 도전 페이지에 막혔다. 구현은 오늘도 찾지 못했다. (2) PuD 소스 요구의 쪽 인용을 p.1, p.3으로 고쳤다(그 문서 §9.1 지적 9(c)). (3) "syscall 없음" 문장마다 `[M-probe 기록]`과 `[I 소스]`를 나눠 붙였다(그 문서 §0.1, §9.1 지적 1). (4) R2 수치의 근거를 gdb 원시 기록으로 바꾸고 오늘 다시 셌다. (5) R2 사건 대응표(⓪–④)를 §3.5에 넣었다(그 문서 §5.1 A4). (6) STVR 기반 쪽 인용에 단일 취득 경로 표시를 붙였다(A10). (7) logging point 식별에 관한 p.5 서술과 v7.0.1 번들 EVS 출력 경로의 소스 읽기를 C1·R2 칸에 더했다. 바뀐 것은 §1.3 행 9–16에 적었다. 여섯 질문의 라벨은 바뀌지 않았다. 이번에도 DESCRY를 실행하지 않았다. 2026-10-15판 문장의 "오늘"은 2026-10-15를 뜻하고, 2026-10-16에 더한 문장과 행에는 날짜를 붙였다.
- 대상: T. Yu, T. S. Zaman, C. Wang, *DESCRY: Reproducing System-Level Concurrency Failures*, ESEC/FSE 2017, pp. 694–704. DOI 10.1145/3106237.3106266.
- 범위: 로드맵 §10 (M6)의 DESCRY 항목. 여섯 분석 질문(§10.2)을 사례 C1, C2, C3, C5, R2에 적용한다. 로드맵 §12 작업 6(`process_race_method_matrix.md`)의 입력으로 쓴다.
- 이 문서의 성격: **분석 문서**다. DESCRY를 실행하지 않았다(실행할 구현이 없다). 어느 사례에 대해서도 "재현하지 못한다" 또는 "탐지하지 못한다"는 결과를 담지 않는다. 아래 "분석" 셀은 읽은 설계 서술에서 도출한 것이며 실행 결과가 아니다.
- 용어: 신규 문서이므로 T1 (a)를 따른다. 판정 대상은 readiness specification(준비 상태 명세), 그 전제는 readiness precondition, 위반은 readiness violation이다. "API contract"는 문서화된 API 의미를 가리킬 때만 쓴다. 원 논문의 "failure", "failure point", "concurrency fault", "race"는 논문의 용어 그대로 쓰고 도구 class로 바꾸지 않는다(P-CLASSGT [D]).
- 조건 원칙: `CONDITIONS_POLICY.md`. 이 문서는 실험 조건을 정하지 않는다. 논문이 자기 평가에서 쓴 값(loop step N=10, 2시간 제한, 5회 반복)은 논문의 값이며 cFS 조건으로 가져오지 않는다. 레퍼런스가 없는 값은 `unspecified_by_reference`로 둔다.
- PI 결정(`PI_DECISIONS_RESOLVED_20261015.md`)은 결정으로만 인용한다. 사실로 쓰지 않는다. 이 문서가 쓰는 결정은 P-PAPERS(원문 조사 승인), T1, P-CLASSGT이다.
- 같은 계보의 다른 문서: `plan/methods/RacePro.md`, `plan/methods/SimRacer.md`, `plan/methods/RRF.md`(같은 형식). DESCRY는 replay에 Simics Virtual Platform을 쓰고 그 출처로 SimRacer [38]를 함께 인용하며(p.2), 자원·사건 모델링 상세를 RacePro [21]와 SimRacer [38] 논문에 위임한다(p.3). 그래서 두 문서와 겹치는 부분은 참조만 한다.

## 증거 태그와 근거 등급

| 태그 | 뜻 | 이 문서에서의 쓰임 |
|---|---|---|
| [F] | 인용이 있는 사실 | 원출처(논문 쪽·절) 또는 저장소 기록(문서·절). 저장소 기록은 "기록"이라고 표시하며 이번에 재검증하지 않았다 |
| [M] | 측정 | 2026-10-15 또는 2026-10-16(날짜를 적은 곳)에 이 컨테이너에서 읽기 전용으로 확인한 값 |
| [M-probe 기록] | probe 측정의 저장소 기록 | OSAL 자체 시험을 `strace`로 본 기록(`baselines/availability.md` §4.2, `baselines/osal-visibility.md` probe 표). 조건: OSAL `d2d877a`(v7.0.1), 경합 없는 실행, cFS 실행 아님, 도구 실행 아님, 사례 revision 아님. 이번에 다시 측정하지 않았다 |
| [I 소스] | 소스 수준 추론 | 코드를 읽고 도출했다. 실행하지 않았다 |
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

**syscall 흔적 문장의 표기 규칙 (2026-10-16 추가. `process_race_method_matrix.md` §0.1과 같다).** "syscall로 보이지 않는다" 류의 문장에는 `[M-probe 기록]` 또는 `[I 소스]`를 붙인다. `[M-probe 기록]`은 세 가지에만 쓴다: (1) 이름 있는 count semaphore의 생성·조회(C1에 해당하는 `OS_CountSemCreate`, `OS_CountSemGetIdByName`)가 `count-sem-test`에서 syscall 0건, (2) `osal-core-test`에서 `OS_QueueCreate`가 `mq_open("<pid>.<name>")`을 냄, (3) `OS_TaskCreate`가 이름 없는 `clone3`로 보임. probe는 사례 revision(C1의 OSAL `42af0f73`, C3·C5의 OSAL 4.1.1 시대, R2의 B `fc8c48ad`·M `12cb84fc`·D `f22e538c`)에서 실행되지 않았고, 경합이 있으면 lock 주소의 futex가 보일 수 있다(`availability.md` §4.5). AppId 저장, 등록 쓰기, 조회, 반환값의 "syscall 없음"은 `[I 소스]`다. R2의 gdb 기록(`env/logs/r2_g1..3_*`)은 breakpoint 기록이므로 syscall 근거로 쓰지 않는다. 어느 표기도 "DESCRY가 사례를 놓친다"를 뒷받침하지 않는다.

쪽 번호 표기: **DESCRY p.N**은 저자 공개본 PDF(11쪽)의 PDF 쪽이다. ACM 인쇄 쪽(694–704)과 쪽 수가 같고 p.1이 694쪽에 해당할 것으로 보나(추론 [I]), ACM 출판본은 열지 못했으므로 확인하지 못했다. **SysPro p.N**, **STVR p.N**, **SCMiner p.N**은 각 PDF의 쪽이다.

---

## 0. 한눈에 보기

1. **DESCRY 논문 전문을 다시 읽었다** `[전문]`. 저자 공개본(`chaowang-vt.github.io/pubDOC/YuZW17.pdf`)이 오늘은 직접 열리고 sha256이 2026-10-08 기록의 값과 같다. NSF PAR 사본도 열리며 본문 텍스트가 같다(§1.1 경로 1–3). 이전 기록(`baselines/descry.md`)의 "전문을 읽었다"는 서술은 같은 판본을 읽은 것으로 확인된다. 2026-10-16에 다시 받은 두 사본도 해시가 같았다(경로 20).
2. **구현과 아티팩트는 오늘도 찾지 못했다.** 논문에 코드 위치가 없다(본문의 URL은 DOI, auditd man page, 예시 URL 세 가지뿐이다). 저자 페이지, 학회 목차, 제3자 아티팩트 조사, GitHub·Software Heritage·Zenodo 검색 어디에도 구현 링크가 없다(§1.1). 프로젝트 페이지 `cs.uky.edu/~tyu/research/descry`는 호스트가 열리지만 원 서버가 403을 돌려주어 내용을 모른다. Zaman 학위논문(UKy 2022)은 초록만 읽었고 PDF는 Cloudflare 도전 페이지에 막혔다. 2026-10-16 재시도(경로 21–29)에서도 같다. 프로젝트 페이지 두 주소는 Wayback 가용성 API에 스냅샷이 없었고, 학위논문은 기관 레코드에서 초록 전체를 읽었다 `[초록]`.
3. **역할은 재현(reproduction)이다.** 입력은 실패한 실행의 default log와 PuD(processes under debugging)의 소스이고, 출력은 실패를 재현하는 data input과 interleaving schedule의 쌍 ⟨T, S⟩이다(p.3–4). 논문은 자신을 fault detection과 구분한다(p.10). 탐지 도구와 같은 표에 점수로 합치지 않는다.
4. **실행 단위는 OS process(와 software signal)이고 사건은 system call이다.** 논문은 thread 수준 bug를 초점 밖으로 두고(p.1), process당 thread 하나를 "for ease of presentation" 가정한다(p.3). 사건과 자원의 모델링 상세는 RacePro와 SimRacer 논문 [21, 38]에 있다고 쓴다(p.3). 이 위임 때문에 DESCRY 논문만으로는 syscall 밖의 사건(예: 메모리 접근)을 보는지 정해지지 않는다. 같은 계보의 STVR 2017 논문(SIMEXPLORER, SimRacer의 revised and expanded version이라고 밝힘)은 shared memory를 shared resource로 열거한다(STVR p.2 각주, p.8 `[2차]`, 단일 취득 경로, 독립 재확인 안 됨). 이것은 DESCRY의 서술이 아니다.
5. **여섯 질문의 도구 수준 답은 대부분 논문에 서술이 있다**(§3.3). 필요한 순서와 허용 오류를 받는 입력 필드는 없다. 무엇이 잘못인가는 로그의 실패 메시지가 정한다(p.3). 순서는 입력도 추론도 아닌 탐색(syscall 사건 쌍 뒤집기와 Simics replay로 확인)으로 만든다(p.7).
6. **가장 직접적인 불일치는 입력과 성공 기준이다.** DESCRY는 실패 실행의 default log와 "failure point"(실패 메시지를 출력하는 문장, p.3)를 요구한다. 다섯 사례 모두 원 보고 본문에 실패 로그가 없다(`[기록]`). 댓글은 이 세션에서 열 수 없었다. R2 댓글 2건은 저장소 기록에 검색 snippet 조각으로만 남아 있고 그 조각에 로그가 없다. R2 원 보고는 "there is no event outputted"라고 적는다(`cases/R2.md` §1.2–1.3 `[기록]`). R2와 C3의 현상은 메시지를 출력하지 않는다(`[기록]`). 이것은 입력 조건과 성공 기준의 불일치이며 탐지·재현 실패의 결과가 아니다.
7. **실행은 불가능하고 오늘 적용되는 상태는 다섯 사례 모두 `NOT_RUN`이다**(§4). `INAPPLICABLE`은 논문의 범위 서술(p.1, p.4)에 근거한 PI 판단이다(§4.3).
8. **Kairos 가설은 근거가 있는 만큼만 올렸다**(§5). 근거가 지지하는 것은 사건 집합의 차이(H-DESCRY-1), 이름·ID·세대 연결의 서술 부재(H-DESCRY-2), 서술된 성공 기준과 사례 현상의 불일치(H-DESCRY-3)이다. 예측과 실행 확인, 로그의 소스 위치 대응, 목표 완화는 DESCRY에 이미 있으므로 차별점으로 쓰지 않는다(§5.1).
9. **2026-10-16 보완의 요점.** 라벨은 바뀌지 않았다(§3.4). 근거 표기와 인용이 바뀌었다. PuD 소스 요구는 p.1·p.3이고, syscall 흔적은 `[M-probe 기록]`과 `[I 소스]`로 나눴고, R2 수치는 gdb 원시 기록을 오늘 다시 센 값이다 [M]. 새 R2 사건 대응표(§3.5)에서 DESCRY 서술에 닿을 수 있는 행은 ⓪ task 생성뿐이다. DESCRY는 `clone`을 `/proc` 아래 process inode 생성으로 모델링하고(p.3) fork-return 순서를 child process에 정의한다(p.7). thread 생성에 적용되는지는 서술이 없다. R2에는 failure point가 없어 DESCRY가 pair의 기준으로 쓰는 "실패 지점에 가장 가까운 사건"(p.7)을 정할 수 없다는 점도 사건표 ④에 적었다(분석, 미실행).

---

## 1. 접근한 출처와 문장별 근거 등급

### 1.1 시도한 경로와 결과

시각은 경로 1–19가 2026-10-15 UTC 09:06–09:30, 경로 20–29가 2026-10-16 UTC 00:23–00:27다. P-PAPERS [D]에 따라 저자 공개본, 기관 저장소, 서지 서비스, 검색을 모두 시도했다. 접속 실패는 원인을 기록한 것까지만 썼고, Cloudflare 도전 페이지나 프록시를 우회하려고 하지 않았다. TLS 검증도 끄지 않았다.

| # | 경로 | 결과 | 등급·비고 |
|---|---|---|---|
| 1 | 저자 공개본 `https://chaowang-vt.github.io/pubDOC/YuZW17.pdf` | `curl` 200, 1,353,307 B, 11쪽. sha256 `f9d7b57bbb0c1269f9143b4ec69286e90348a58fadd071f76f11022e06dfbf60`. 2026-10-08 기록(`baselines/descry.md` §1)의 해시와 같다. 그때는 이 HTTPS 주소가 막혀 같은 저장소의 git 사본으로 읽었다고 기록돼 있고, 오늘은 직접 열린다. `pdftotext`로 쪽별 추출해 11쪽 전부 읽었다. Figure 1(p.2)은 이미지로 확인했다. Table 2는 레이아웃 추출이 어지러워 본문 서술과 Table 1·3의 값만 썼다 | **`[전문]`** |
| 2 | 같은 파일의 `raw.githubusercontent.com` 경로(저장소 `chaowang-vt/chaowang-vt.github.io`) | 200, 같은 sha256 | 사본 일치 |
| 3 | NSF PAR `https://par.nsf.gov/servlets/purl/10075449`, 레코드 `/biblio/10075449` | PDF 200, 1,427,736 B, 11쪽, sha256 `8f966ebe9d435455dd6195d9c36bb74058e4eae3ffd643a168208b7c3948e2fb`. 저자 공개본과 쪽별 텍스트를 비교하니 3·4쪽의 코드 조각 줄바꿈과 "PuDs" 분리 차이뿐이고 나머지 9쪽은 차이가 없었다. 메타데이터의 수정일이 2018-09-18이다. 레코드는 Award ID 1405697, 소프트웨어·아티팩트 링크 없음 | `[전문]` 확인용. 2026-10-08에는 접근하지 못했다고 기록돼 있었다 |
| 4 | ACM DL `dl.acm.org/doi/10.1145/3106237.3106266`, `…/doi/pdf/…`, `doi.org/…` | 403, 응답 헤더 `cf-mitigated: challenge`(Cloudflare 도전) | `[미도달]` 출판본. 판본 동일성은 텍스트 비교로 PAR 사본까지만 확인 |
| 5 | 서지 API: Semantic Scholar, OpenAlex, Crossref | 모두 200. Semantic Scholar `openAccessPdf.status: CLOSED`(초록은 출판사 요청으로 생략), 인용 25건. OpenAlex `is_oa: false`, 사본 위치 없음, 인용 21건. Crossref는 서지 | 서지. 인용 목록은 §2.4의 2차 문헌 탐색에 썼다 |
| 6 | UKy 연구자 포털 `scholars.uky.edu/en/publications/descry-…`와 과제 레코드 `…/projects/crii-shf-simdb-…` | 둘 다 200. 논문 레코드는 초록·서지(Scopus 인용 23건)이고 코드 링크가 없다. SimDB 과제(기간 2015-03-01~2018-02-28)는 목표를 서술한다. 그중 (iii)은 "implementing an automated debugging framework that provides rich user interfaces"라는 계획이며 산출물 링크가 없다 | `[초록]` |
| 7 | Zaman 학위논문(UKy 2022, *An Automated Framework to Debug System-Level Concurrency Failures*) 레코드 `uknowledge.uky.edu/cs_etds/119/` | 레코드 200. 초록의 도입부를 HTML 메타에서 읽었고, WebFetch 요약은 세 구성(실패 재현, 시스템 호출 trace에서의 위치 추정, bug report에서의 위치 추정)을 적는다(요약 모델의 문장이며 원문 인용이 아니다). 첫 구성은 "default field logs", static·dynamic analysis, symbolic execution, 기존 virtual platform에서의 replay다. PDF `viewcontent.cgi?article=1125&context=cs_etds`(레코드는 2.9 MB로 표기)는 `curl` 403 `cf-mitigated: challenge`, WebFetch 403 | `[초록]`. 본문 `[미도달]`. 구현 상세(Simics 설정, KLEE 수정)가 있을 수 있으나 읽지 못했다 |
| 8 | 프로젝트 페이지 `cs.uky.edu/~tyu/research/descry`(SCMiner가 [45]로 인용, §2.4) | `http://`는 400(SSL 포트에 평문 요청). `https://cs.uky.edu/~tyu/research/descry`, `/`가 붙은 형태, `www.cs.uky.edu/~tyu/research/descry/index.html`, `…/~tyu/research/`, `…/~tyu/`, `cs.uky.edu/~tyu/` 모두 403(Apache, 본문 "You don't have permission to access this resource."). 같은 호스트의 `https://www.cs.uky.edu/` 루트는 200 | `[미도달]`. **2026-10-08 기록은 세션의 egress allowlist 차단(`host_not_allowed`)이라고 적었고, 오늘 응답은 원 서버의 403이다.** 원인(경로 삭제, 접근 제한 등)은 알 수 없고 내용도 모른다 |
| 9 | 저자 페이지: `homepages.uc.edu/~tyu/`(`links/research.html`, `links/projects.html`), `tingting-yu.scholar.uconn.edu`, Chao Wang `sites.usc.edu/chaowang/publications/` | 모두 200. UC 연구 페이지는 DESCRY를 ACM 링크로만 걸고 코드 링크가 없다. UConn 홈은 받은 HTML에 DESCRY 언급이 없다(목록이 동적으로 불러와지는지는 확인하지 않았다). USC 출판 목록의 DESCRY 항목은 논문 PDF 링크뿐이고, 같은 페이지의 다른 논문 항목(예: ASE 2017 `intAbs`)에는 "(artifact)" 링크가 붙어 있다 | 서지. 구현 없음 |
| 10 | ESEC/FSE 2017 목차 `sigsoft.org/resources/opentoc/ESECFSE2017-TOC.html` | 200. DESCRY 항목은 ACM DL 링크와 초록뿐이다 | 서지. 구현 없음 |
| 11 | 제3자 아티팩트 조사 `morgen52/SE-artifact` `data/papers.csv` | 200. 1504행 `FSE-2017-63,DESCRY: reproducing system-level concurrency failures.,,,,`로 URL 칸이 비어 있다(2026-10-08 기록과 같다) | 제3자 주석. 논문 본문 검색 결과와 일치 |
| 12 | GitHub 검색(MCP): 저장소 검색(`descry system-level concurrency failures reproduce logs symbolic execution`), 코드 검색(`"DESCRY" "log-guided" OR "logging points" simics klee`) | 둘 다 0건. `git ls-remote`: `tarannumzaman/Scminer`는 읽히고(branch 6개) 파일 목록에 DESCRY 관련 항목이 없다(zip 묶음과 preprint). 추측한 이름 `tarannumzaman/DESCRY`, `tarannumzaman/Descry`, `tingtingyu/descry`, `tyu/descry`, `chaowang-vt/descry`는 인증을 요구했다. 존재하지 않거나 비공개다. 둘은 구분할 수 없다 | 구현 없음. 이름이 다른 저장소는 이 검색으로 찾지 못한다 |
| 13 | SysPro 보충 자료 `github.com/tarannumzaman/ReproSys/tree/main`(SysPro의 Data availability 문장이 가리킨다, SysPro p.38) | `git ls-remote`가 인증을 요구했다. `gh api`는 이 세션에 해당 저장소 접근이 없다는 403 | `[미도달]`. SysPro의 자료이며 DESCRY 구현이 아니다 |
| 14 | Software Heritage 원본 검색(URL 부분 일치 `descry`), Zenodo 레코드 검색, Internet Archive 검색 | Software Heritage는 한도 50건을 받았고 모두 무관한 원본이다(암호 관련 `descrypt` 프로젝트, 같은 이름의 다른 도구 등). Zenodo는 `"system-level concurrency"` 검색에 2026년 레코드 3건(DataSCon, SysConDB, SysConAgent)이 나왔다. 설명은 Intel Pin 도구와 bug report 기반 재현이며 DESCRY를 서술하지 않는다. 9.7 MB의 SysConAgent 묶음은 내려받아 항목 3,213개의 이름과 5 MB 미만 텍스트에서 `descry`를 찾았고 0건이었다. 나머지 둘(약 58 MB)은 시간 초과로 끝까지 받지 못했다. Internet Archive는 `numFound: 0` | 구현 없음. URL 부분 일치라서 이름이 다른 저장소는 찾지 못한다 |
| 15 | WebSearch 2회(standard 1, extended 1) | 새 1차 출처 없음. 나온 곳은 저자 공개본, PAR, UKy 레코드, ACM 목차, SysPro, SCMiner, 서지 모음이다. 두 검색 요약 모두 "코드 저장소·다운로드·아티팩트를 찾지 못했다"는 취지다(검색 요약) | — |
| 16 | Scholar Gateway `semanticSearch` | `ACCESS_DENIED: Access denied`(2026-10-08 이전 기록과 같다) | `[미도달]` |
| 17 | 2차 문헌 PDF: SysPro `arxiv.org/pdf/2601.09616v1`(47쪽, sha256 `8fe5daa4c7c3794715632e4379d9160a9852b723dc4a649317ef6bae0a75e726`), SCMiner `par.nsf.gov/servlets/purl/10166153`(12쪽, sha256 `c361caaf7c1476b0…`), SimEvo `par.nsf.gov/servlets/purl/10089644`(12쪽) | 모두 200. 텍스트를 추출해 "Descry"를 검색했다. SysPro는 p.2, p.22, p.31, p.40에, SCMiner는 p.11, p.12에 있다. SimEvo에는 없다 | `[2차]`(§2.4). 2026-10-08에는 SysPro가 열리지 않아 검색 발췌만 단서로 적었다 |
| 18 | 계보 논문 STVR 2017(SIMEXPLORER, SimRacer의 확장판) CPS-VO 사본 `archive.cps-vo.org/file/36388/download/194033` | WebFetch로 PDF 2.29 MB를 받았다(`curl`은 같은 호스트의 WAF가 거부하는 것으로 `SimRacer.md` §1.1 경로 18에 기록돼 있어 시도하지 않았다). 26쪽, sha256 `af805231f69ffc0e12d96fe4e4e9a356932653c56c382fdd3debd211bcdd3e08`(`SimRacer.md`의 기록과 같다). **p.2, p.8, p.18, p.24만 읽었다** | `[2차]`. DESCRY가 사건·자원 모델링을 위임한 계보의 서술을 확인하는 데만 썼다. 이 논문의 나머지 쪽은 `SimRacer.md`의 `[전문]` 분석을 따른다. **단일 취득 경로, 독립 재확인 안 됨**(CPS-VO 한 경로에서만 얻음. `process_race_method_matrix.md` §1.1 취득 경로, §5.4 항목 16). 2026-10-16에 이 문서는 둘째 경로를 찾지 않았다 |
| 19 | 사례 이슈 댓글(nasa/CF#184, nasa/cFE#198, #73, #72, #2663) | GitHub MCP `issue_read`가 nasa/CF 접근을 거부했다(허용 저장소는 `wookjaeya/shape-perf`뿐). `github.com/nasa/…/issues/N`을 `curl`하면 프록시가 403과 "GitHub access to this repository is not enabled for this session"을 돌려준다. `add_repo`는 자격을 붙이는 조치라 사용자 동의가 필요하며 이 문서의 작업 범위가 아니어서 호출하지 않았다 | `[미도달]`. DESCRY 입력 조건(실패 로그)의 확인이 이 경로에 걸려 있다(§4.3) |
| 20 | (2026-10-16) 저자 공개본과 NSF PAR 사본 재취득 | 둘 다 200. sha256 `f9d7b57b…`, `8f966ebe…`로 2026-10-15와 같다 [M]. 쪽별로 다시 추출해 주요 인용을 대조했다: p.1 PuD 소스 가정, p.3 §2.2 Problem Statement와 "systems calls are modeled as concurrency events", p.3의 [21, 38] 위임, p.5 §4.1, p.7 §6.1의 순서 관계 5종과 replay의 infeasible 처리 | `[전문]` 유지 |
| 21 | (2026-10-16) 프로젝트 페이지 `https://cs.uky.edu/~tyu/research/descry/`, `https://www.cs.uky.edu/~tyu/research/descry/` | 둘 다 403(Apache "Forbidden", 199 B) | `[미도달]`. 2026-10-15와 같다 |
| 22 | (2026-10-16) Wayback 가용성 API `archive.org/wayback/available`(위 두 주소), CDX 검색 `web.archive.org/cdx/search/cdx` | 가용성 API는 200, `archived_snapshots: {}`(스냅샷 없음). CDX는 "Blocked by egress policy"(403) | 보관 사본 없음. CDX는 이 세션의 egress 정책이 막았다 |
| 23 | (2026-10-16) 학위논문 PDF `uknowledge.uky.edu/cgi/viewcontent.cgi?article=1125&context=cs_etds`(curl, WebFetch) | curl 403 `cf-mitigated: challenge`, WebFetch 403 | 본문 `[미도달]`. 우회하지 않았다 |
| 24 | (2026-10-16) 학위논문 레코드 `uknowledge.uky.edu/cs_etds/119/` | 200. 초록 전체를 HTML 본문에서 읽었다. 세 연구 중 첫째를 "automatically reproduce system-level concurrency failures using only default logs collected from the field"로 적는다. 키워드는 System Calls, Bug Localization, Bug Reproduction, System-level Trace, Console Logs, Bug Reports다. 페이지가 적은 DOI `10.10323/etd.2022.168`은 doi.org에서 404였고, `10.13023/etd.2022.168`은 이 레코드로 이어졌다 [M] | `[초록]`(전체). 구현 위치, 사건 모델, thread 처리는 초록에 없다 |
| 25 | (2026-10-16) ACM DL 논문 쪽과 PDF | 403 `cf-mitigated: challenge` | `[미도달]` |
| 26 | (2026-10-16) 서지 API로 학위논문 사본 찾기: OpenAlex 검색, CORE API, Semantic Scholar 검색 | 셋 다 429(공용 호출 한도 초과) | 결과 없음 |
| 27 | (2026-10-16) Scholar Gateway `semanticSearch` | `ACCESS_DENIED: Access denied` | `[미도달]` |
| 28 | (2026-10-16) WebSearch(standard 1회) | 새 1차 출처 없음. 나온 곳은 UKy 포털, PAR 사본, SysPro(arXiv), UKy 학위논문 레코드 등이다. 검색 요약도 GitHub 저장소·아티팩트를 찾지 못했다는 취지다 | — |
| 29 | (2026-10-16) GitHub: `gh api users/tarannumzaman/repos`와 저장소 검색 API, `github.com/tarannumzaman?tab=repositories`, `git ls-remote …/tarannumzaman/ReproSys` | API는 "sessions are bound to their configured repositories"(403). 프로필 페이지는 프록시 403. `ls-remote`는 인증 요구 | 오늘은 확인할 수 없었다. 구현이 없다는 새 확인이 아니다 |

**읽은 출처의 요약.** 본문 전체를 읽은 것은 DESCRY 논문(경로 1) 한 편이다. SysPro, SCMiner는 DESCRY를 서술한 문장만 읽었다. STVR은 4쪽만 읽었다. Zaman 학위논문은 PDF가 도전 페이지에 막혔고, 이 문서는 학위논문 본문의 어떤 내용도 쓰지 않는다. 2026-10-16에 새로 읽은 것은 학위논문 초록 전체뿐이다(경로 24).

### 1.2 이 문서가 쓰는 출처별 등급

| 출처 | 등급 | 이 문서에서의 쓰임 |
|---|---|---|
| DESCRY (FSE'17) 저자 공개본·PAR 사본 | **`[전문]`** | 도구 설명 전체(§2), 여섯 질문의 도구 수준 답(§3.3), 실행 요구(§4.1) |
| DESCRY 서지·초록(UKy 포털, 서지 API, 목차) | `[초록]` | 출판 정보 확인 |
| DESCRY 출판본(ACM) | `[미도달]` | 판본 동일성 미확인 |
| Zaman 학위논문(UKy 2022) | 초록 전체 `[초록]`(2026-10-16). 본문 `[미도달]` | 존재와 범위 확인뿐 |
| SysPro(arXiv 2601.09616v1) p.2, p.22, p.31 | `[2차]` | DESCRY의 입력 요구를 서술한 문장(§2.4) |
| SCMiner p.11–12 | `[2차]` | DESCRY를 한 문장으로 서술, 프로젝트 페이지 인용(§2.4) |
| STVR 2017 p.2, p.8, p.18, p.24 | `[2차]`, 단일 취득 경로, 독립 재확인 안 됨 | DESCRY가 위임한 사건·자원 모델 계보의 서술. DESCRY 자신의 서술이 아니다 |
| 사례 쪽 사실(`cases/ledger.md` §1a, §1b, §2; `baselines/descry.md` §6) | `[기록]` | §3.2 |
| OSAL visibility probe(`baselines/availability.md` §4.2, `baselines/osal-visibility.md` probe 표) | `[M-probe 기록]` | 사건이 syscall로 보이는지의 근거. **DESCRY의 결과가 아니고 어느 도구의 결과도 아니다.** OSAL 자체 시험을 syscall 관측자로 본 probe다 |
| 컨테이너 사실(`baselines/availability.md` §1.1) | `[기록]`, 일부 재확인 `[M]`(2026-10-15, 2026-10-16) | §4.1 |
| R2 gdb 원시 기록 `env/logs/r2_g1..3_trace.jsonl`와 `results/R2_gdb_v701.md` | `[기록]`, 2026-10-16 재계수 `[M]` | §3.2 R2 행, §3.5 R2 칸과 사건표. syscall 근거로는 쓰지 않는다 |
| `cases/R2.md` §1.2–1.3, §6, §9; `plan/case_basis_register.md` R2.E2–E5 | `[기록]` | R2 원 보고 서술, 댓글 기록 상태, 시작 직렬화 S1, 줄 번호 |
| cFE v7.0.1 번들(`c5fb2b4d`)의 EVS 소스 | `[I 소스]` | C1·R2 Q6 칸의 이벤트 출력 경로. C1·C3·C5의 사례 revision에서는 읽지 않았다 |

### 1.3 이전 기록과 달라지거나 확정된 것

| # | 항목 | 내용 |
|---|---|---|
| 1 | `availability.md` §1·§2.3, `descry.md` §1의 "paper: opened and read in full" | 같은 판본(sha256 일치)을 오늘 다시 읽어 확인했다. 11쪽 전체를 쪽 번호와 함께 인용할 수 있게 됐다 |
| 2 | `descry.md` §0 D1, §1, §8의 "프로젝트 페이지는 세션 allowlist가 막는다(`host_not_allowed`)" | 오늘은 호스트가 열리고 원 서버가 `~tyu` 경로에 403을 돌려준다(경로 8). "내용을 모른다"는 유지되지만 원인 서술이 바뀐다. 두 기록을 합쳐 읽지 않는다. `descry.md`의 권고 D1(b) "allowlist에 호스트 추가"는 이 호스트에 대해서는 더 이상 해법이 아니다 |
| 3 | `descry.md` §1, `availability.md` A1의 "SysPro arXiv NOT OPENED, Zaman 학위논문 NOT OPENED, NSF PAR 사본 NOT OPENED" | SysPro와 PAR 사본은 열린다. 학위논문은 레코드·초록이 열리고 PDF는 Cloudflare 도전에 막힌다(경로 3, 7, 17). SysPro의 DESCRY 서술이 본문으로 확인돼 "검색 발췌, 단서만"이던 등급이 `[2차]`로 올라간다 |
| 4 | `availability.md` §1·§3의 DESCRY 칸 "None of the five. The publish and use sites make no syscall" | 그 칸의 근거는 "DESCRY의 사건 집합이 syscall이다"(p.3, p.7)이고 이 근거는 유지된다. 다만 DESCRY가 사건·자원 모델링 상세를 [21, 38]에 위임하고(p.3), 그 계보의 STVR이 shared memory를 shared resource로 열거하므로(STVR p.8), "DESCRY 구현이 메모리 사건을 보지 않는다"까지는 정해지지 않는다. 이 문서는 "서술된 사건 집합에 사례의 사건이 들어오는 경로를 찾지 못했다"로 쓴다 |
| 5 | `descry.md` §3 Q8 "단일 스레드 가정은 presentation 가정이지 한계 서술이 아니다" | 전문으로 확인된다(p.3 "for ease of presentation"). 한편 범위 서술은 p.1에 별도로 있다(thread-level bug를 초점 밖으로 둠). 이 문서는 두 서술을 구분해 쓴다 |
| 6 | SysPro p.22의 "Descry includes 21 benchmarks" | DESCRY 본문은 Table 1에 24행을 싣고 "22 of 24 failures"를 재현했다고 적는다(p.2, p.8). 숫자가 다르다. 이 문서는 DESCRY 본문의 수치만 쓴다 |
| 7 | 사례 C2의 댓글 수 | `baselines/descry.md` §1은 11(검색 메타데이터)이고 `cases/C2.md`·`cases/ledger.md`는 "none"(로그아웃 HTML)이다. 같은 저장소의 `cases/C3.md` §1.2가 로그아웃 HTML은 댓글이 있는 이슈도 비어 보인다는 대조를 제시하므로 "none"은 신뢰하기 어렵다. 오늘 재확인하지 못했다(경로 19) `[U]` |
| 8 | `descry.md`의 인용 표기 `pN rN.txt:L` | 쪽 번호만 남겼다. 예: `p1 r1.txt:81-91`은 p.1이다 |
| 9 | (2026-10-16) PuD 소스 요구의 쪽 | 2026-10-15판은 §2.1 입력 행, §3.1 전제 3, §3.5 C2 Q1, §4.1 Q5, §4.3 항목 1 (c)에서 "p.1–2" 또는 "p.2"로 적었다. 요구 문장은 p.1("only assumes the existence of the source code of processes under debugging")과 p.3(§2.2 Problem Statement)에 있고, p.2에는 같은 문장이 없다(`process_race_method_matrix.md` §9.1 지적 9(c), 오늘 재대조). 모두 p.1, p.3으로 고쳤다. p.2는 C/C++ 멀티프로세스 응용을 다룬다는 서술(§4.1 Q8)에만 남는다 |
| 10 | (2026-10-16) syscall 흔적의 표기 | 2026-10-15판은 "syscall 0건", "`clone3`만 보임", "`mq_open`만 보임"을 구분 없이 `[기록]`으로 묶었다. probe는 OSAL `d2d877a`의 경합 없는 자체 시험이고 사례 revision에서 실행되지 않았다. 위 표기 규칙대로 `[M-probe 기록]`과 `[I 소스]`로 나눴다(§3.2 공통 행, §3.5 Q1·Q3 칸, 사건표) |
| 11 | (2026-10-16) C5 조회와 futex | C5의 `CFE_ES_GetAppID`는 ES lock을 잡는다(`cases/C5.md` F5 `[기록]`). 생성 task와 겹치면 lock 주소의 futex가 보일 수 있다(`availability.md` §4.5). "조회는 메모리"를 "lock 아래의 메모리 읽기이며 경합 시 futex가 보일 수 있다"로 고쳤다. DESCRY 논문에는 futex·mutex 사건의 서술이 없다(키워드 0건) |
| 12 | (2026-10-16) R2 수치의 근거 | 2026-10-15판은 `ledger.md` §2를 인용했다. R2 실행 증거는 gdb 원시 기록이므로 근거를 `env/logs/r2_g1..3_trace.jsonl`과 `results/R2_gdb_v701.md`로 바꿨다. 오늘 세 trace를 다시 셌다 [M]: SB 호출자 사용 223/229/229, SB AppId 설정 전 사용 24/24/24(AppId "0"), 함수별 `CreatePipe` 2·`SubscribeFull` 4·`ReportSingleEvent` 18, task별 ES 20·EVS 4, `OUT_ILLEGAL_APP_ID` 24/24/24. 로드맵 §2.2의 재검산과 같다. 이 24는 한 원인에서 나온 동적 발생이며 독립 결함 24개가 아니다 |
| 13 | (2026-10-16) R2의 "syscall 없음" | 그 문장은 소스 읽기(`cases/mapping_v701.md` §1.2)에서 온 것이다. gdb 기록은 syscall을 관측하지 않았다(세 run의 `r2_g*` 파일 24개에 `syscall`·`strace` 문자열 0건, 오늘 `grep` [M]). `results/R2_gdb_v701.md` §2의 "no syscall was made"는 이 문서가 근거로 쓰지 않는다(`process_race_method_matrix.md` §5.5) |
| 14 | (2026-10-16) R2 댓글 | 2026-10-15판은 "댓글 2건 미독"이라고 적었다. 저장소 기록은 두 댓글을 검색 snippet 조각으로 남겼다(`cases/R2.md` §1.3). 조각에 로그 출력은 없다. 전문은 여전히 읽지 못했다 |
| 15 | (2026-10-16) STVR 쪽 인용 | STVR 사본은 CPS-VO 한 경로에서만 얻었다. 이 문서의 STVR 인용(p.2, p.8, p.18, p.24)에 "단일 취득 경로, 독립 재확인 안 됨"을 붙였다. `SIMEXPLORER.md`와 `SimRacer.md`가 2026-10-16에 둘째 경로를 찾았으나 얻지 못했다고 적는다 |
| 16 | (2026-10-16) logging point 식별 | 2026-10-15판은 C1 Q6에서 "printing API 목록이 `CFE_EVS_SendEvent` 계열을 포함하는지 서술이 없다"고만 적었다. p.5는 실제 응용이 "customized logging facilities"를 쓰는 점을 어려움으로 들고, printing API가 라이브러리 안에 있으면 inter-procedural CFG의 data·control flow로 PuD 소스 위치까지 되짚는다고 쓴다. 이 서술과 v7.0.1 번들의 EVS 출력 경로(`[I 소스]`)를 칸에 더했다. cFE 이벤트 경로가 그 되짚기의 대상인지는 여전히 서술이 없다 |

---

## 2. 도구의 정체

### 2.1 요약

| 항목 | 내용 | 근거·등급 |
|---|---|---|
| **역할** | **재현(reproduction).** 현장에서 발생한 실패의 default log만으로 failure-inducing data input과 interleaving schedule을 만들어 결정적으로 재현한다. 논문은 "the first fully automated tool for reproducing system-level concurrency failures"라고 쓴다. fault detection은 관련 연구로 구분한다("The focus of our work is reproducing failures"). 회귀 분석 도구가 아니다 | p.1, p.3, p.10 `[전문]` |
| **실행 단위** | OS process와 software signal. 형식 모델은 process 집합 {P1…Pm}과 signal 집합 {S1…Sn}이다. 각 process가 thread를 여럿 만들 수 있으나 "for ease of presentation" 한 개로 가정한다. 초점은 inter-process(system-level) bug이고 intra-process(thread-level) bug는 "the focus of many prior work"라며 구분한다. 한 번에 PuD 둘을 분석한다. 평가 대상은 Linux 멀티프로세스 응용이다. 논문은 응용 22개(p.2)와 "22 program versions from 20 unique applications"(p.8)를 함께 적고, 알려진 실패는 24건(Table 1의 24행)이다 | p.1, p.2, p.3, p.7, p.8 `[전문]` |
| **입력** | (필드) 실패한 실행의 default log message. (사내) PuD의 소스 코드. 실행 중이던 process 집합 Pall은 Linux Auditd 같은 시스템 도구와 `/var/log`로 얻는다고 가정한다. 사람이 개입하는 곳은 하나다: PuD 규칙 (1)에서 process가 받는 입력의 종류(파일, 문자열, 숫자)를 지정하는 일. 설정 옵션은 static chopping으로 관련 옵션을 고르고 concrete 값으로 쓴다. 매개변수 N=10(loop 반복 증가 단위)이 있고 loop 상한 L_max는 값이 없다 | p.1, p.3, p.4–5, p.6 `[전문]` |
| **관측하는 것** | (필드) 로그 메시지(uninterpreted plaintext). 로그 → 소스 대응은 정적 분석으로 한다: printing API(`printf`, `sprintf` 등) 호출의 format string(따옴표 안 문자열에서 서식 지정자와 escape를 뺀 것)이 로그의 부분 문자열과 같으면 그 호출을 logging point로 본다. 논문은 실제 응용이 단순 `printf`가 아닌 "customized logging facilities"를 쓰는 점을 어려움으로 들고, printing API가 라이브러리 안에 있으면 inter-procedural CFG의 data·control flow를 따라 PuD 소스 위치로 되짚는다고 쓴다. 동적으로 조립된 문자열은 식별되지 않아 이후 탐색이 안내 없는 탐색이 된다. (사내) PuD를 구체 입력으로 실행해 얻은 trace E의 system call 사건 | p.3, p.4 Fig. 5, p.5 §4.1, p.7 `[전문]` |
| **자원 식별** | 시스템 전역 자원(file, device 등)을 syscall로 접근하는 것으로 본다. syscall의 read/write 효과를 모델링한다(예: `lstat`은 파일 metadata를 읽고, `clone`은 `/proc` 아래 process inode를 만든다(write)). 사건 표기는 `R(A.txt)`, `W(A.txt.backup)`처럼 자원 이름이다(Fig. 4). PuD별 입력이 같은 자원을 가리켜야 할 때는 각 PuD의 입력 접근 syscall 목록이 모두 비어 있지 않으면 공유 자원으로 보고 파일 이름을 통일한다. 자원·사건 모델링의 상세는 [21, 38]에 있다고 쓴다. 객체 ID, 이름과 slot의 대응, 객체 재생성의 세대(generation)에 관한 서술은 없다(키워드 `reuse`, `recreate` 0건, `generation`은 모두 input/schedule generation의 뜻) | p.3, p.4 `[전문]` |
| **모델링하는 순서·의존** | 5가지 must-happen-before 관계: program order(같은 process/thread), fork→child return, wait→child exit, pipe write→read, signal 활성화→handler. 이 관계로 partial order graph(POG)를 만든다. 관계가 없는 두 사건은 뒤집을 수 있다. mutex, semaphore, message queue, 공유 메모리, 사용자 수준 registry, 시작 동기화에 대한 관계는 서술이 없다(키워드 `mutex`, `semaphore`, `futex`, `startup`, `regist`, `initializ` 0건) | p.7 §6.1 `[전문]` |
| **탐색** | 실패 지점에 가장 가까운 PuD P_F의 사건 e_f를 고르고, 이것과 순서 관계가 없고 가까운 사건 e_i를 골라 "suspicious event pair"를 만든다. 쌍마다 순서를 뒤집은 새 schedule을 만들되 POG를 지킨다. 우선순위는 둘 중 하나 이상이 write syscall이고 실패 지점에 가까운 쌍이다. 후보 사건을 옮길 때 그에 의존하는 사건도 함께 옮긴다. 분석 단위는 PuD 둘이다 | p.7 §6.2 `[전문]` |
| **재현과 확인** | Simics Virtual Platform으로 PuD를 새 입력과 새 schedule로 실행하며 syscall 사건을 제어한다. 실행이 뒤집은 사건과 맞지 않으면(다른 분기를 탔으면) 그 schedule은 infeasible로 보고 건너뛰어 다른 쌍으로 간다. 모든 쌍이 소진되면 symbolic execution으로 돌아가 새 입력을 만든다 | p.4, p.7 `[전문]` |
| **판정(성공 기준)** | **failure point**(실패 메시지를 출력하는 프로그램 문장)에 도달하면 재현 성공이다. 목표가 infeasible이면 failure point를 failure predicate로 바꿔 목표를 완화한다. 논문의 예는 `tail`의 failure point(메시지 출력 줄)를 그 앞의 `fd == -1` 검사 줄(Line 43)로 바꾸는 것이다 | p.3, p.4, p.8 `[전문]` |
| **출력** | 튜플 ⟨T, S⟩: failure-inducing data input T와 interleaving schedule S. 재현 성공 여부. 평가표에 시간, 사건·자원 수, 뒤집은 쌍의 결과가 있다 | p.4 Fig. 5, p.8–9 `[전문]` |
| **구현** | log → 소스 대응 정적 분석은 LLVM, log-guided symbolic execution은 KLEE, schedule generator와 replay는 Simics Virtual Platform. KLEE는 goal 거리 기반 상태 선택, 가지치기, concrete seeding으로 확장했다. 평가는 "the most recent version of KLEE built from LLVM 3.4"를 썼다. Simics의 판본·모사 기계·guest·스크립트는 서술이 없다 | p.2, p.6, p.7, p.8 `[전문]` |
| **평가 환경** | Intel Core i5-2400 3.10 GHz, 8 GB, Ubuntu 14.10. 시간 제한 2시간, 설정마다 5회 | p.8 `[전문]` |
| **평가 결과** | 24건 중 22건 재현. 실패 2건은 KLEE의 평면 파일시스템 모델(계층 디렉터리 입력 불가)이 원인이라고 서술한다. SimRacer와 비교해 22건 중 8건만 재현한다(SimRacer는 data input을 만들지 못해 무작위 입력을 줬다고 적는다). 22건 모두 40분 미만 | p.2, p.8–9 `[전문]` |
| **artifact 위치** | 논문 본문에 코드·데이터 위치가 없다. 본문의 URL은 DOI, auditd man page, 예시 URL(`http://cgi-bin/hello.cgi`) 세 가지다 | 전문 검색 `[전문]` |

### 2.2 동작 흐름 (Figure 1 p.2, Figure 5 p.4)

1. **PuD 식별.** 실행 중이던 process 집합 Pall과 로그에서 실패한 process P_F와 상호작용할 만한 process를 고른다. PuD가 되는 경우는 네 가지다: (1) P_F와 같은 종류의 입력을 받는다, (2) 같은 프로그램의 다른 인스턴스다, (3) 같은 응용이 만든 다른 process다, (4) P_F 안의 software signal이다. (2)–(4)는 자동이고 (1)은 사람이 입력 종류를 줄 수 있다(p.4–5).
2. **로그 분석.** 반복 메시지를 지우고 나머지 메시지를 logging point에 대응시킨다. logging point를 로그 순서를 따르는 log hierarchy graph(LHG)로 잇고, 실행 불가능한 경로를 가지치기해 goal list를 만든다. 반복문이 낳은 같은 format string의 메시지는 한 ID로 합친다(p.5).
3. **입력 생성.** PuD별로 goal list를 지나도록 KLEE 기반 guided symbolic execution으로 path condition을 풀어 data input T[i]를 얻는다. goal에 닿지 못하면 같은 goal list 안에서 앞 goal로 되돌아가고, 마지막에는 다른 goal list를 시도한다. 반복문 안의 goal은 반복 상한을 N=10씩 늘려 다시 시도한다(p.6).
4. **schedule 생성.** T로 PuD를 임의 schedule로 구체 실행해 trace E를 얻는다. E가 실패를 일으키면 끝난다. 아니면 §6의 방법으로 suspicious pair를 뒤집어 schedule을 만든다(p.4, p.7).
5. **replay.** Simics에서 PuD와 syscall 사건을 제어해 새 schedule을 실행한다. 실패 지점에 도달하면 ⟨T, S⟩를 반환한다. 모든 pair를 소진하면 symbolic execution으로 돌아가 새 입력을 만든다. 전체 반복은 시간 제한(TIME_max)까지다(p.4, p.7).

### 2.3 논문이 밝힌 범위·한계 중 cFS와 관련 있는 것

| # | 서술 | 근거 | cFS와의 관계 (분석, 미실행) |
|---|---|---|---|
| 1 | 초점은 inter-process bug이고 intra-process(thread-level) bug는 구분한다. thread-level bug는 보통 한 process의 휘발성 메모리만 망가뜨린다고 쓴다 | p.1 | native cFS는 한 Linux process이고 OSAL task는 thread다(`availability.md` §3 `[기록]`). 사례의 의존성은 이 thread 사이에 있다. 로드맵 §1.2는 Process-Level을 "애플리케이션·task 단위"라는 운용 정의로 쓰고 OS process로 한정하지 않는다. DESCRY의 단위(OS process)와의 차이를 논문 처음에 밝혀야 한다 |
| 2 | process당 thread 하나를 가정한다("for ease of presentation") | p.3 | 도구의 한계로 쓴 문장이 아니라 설명 편의의 가정이다. thread를 어떻게 다루는지는 서술이 없다 |
| 3 | 사건은 syscall이다. 순서 관계는 fork, wait, pipe, signal, program order뿐이다 | p.3, p.7 | 사례의 의존성은 메모리 상태와 시작 순서다. 사건과 관계의 목록이 닫혀 있다("we employ the following order relations") |
| 4 | 한 번에 PuD 둘을 분석한다. 이것이 실험의 모든 bug에 충분했다고 적는다 | p.7 | 사례는 두 task 사이의 쌍이라 이 근사와 충돌하지 않는다 [I]. 단위 문제는 행 1과 같다 |
| 5 | 로그 품질에 의존한다. 동적으로 조립된 메시지는 식별되지 않는다. 너무 일반적인 메시지는 도움이 안 된다 | p.5, p.10 | 사례의 실패 로그가 없다(`[기록]`). cFS의 이벤트 메시지가 `CFE_EVS_SendEvent`의 format string과 인자로 만들어지는 점이 이 식별에 맞는지는 서술이 없다 |
| 6 | 반복문이 낳은 메시지는 한 ID로 합쳐 goal list 비용을 줄인다. 반복 횟수가 정확히 정해진 실패에는 안 맞을 수 있다고 인정한다 | p.5 | C1의 재시도(25회)처럼 횟수가 의미 있는 경로의 취급이 서술에 없다 [I] |
| 7 | symbolic execution은 중간 규모 응용까지 확장된다. KLEE가 계층 디렉터리를 모델링하지 못해 2건을 놓쳤다 | p.8, p.10 | cFS 규모에서의 KLEE 적용 가능성은 서술이 없다. 사례는 입력 조합이 아니라 순서의 문제로 서술돼 있다(`[기록]`) |
| 8 | 성공 기준은 failure point(메시지를 출력하는 문장) 도달이다 | p.3, p.8 | R2와 C3의 현상은 메시지를 출력하지 않는다(`baselines/descry.md` §6 `[기록]`) |
| 9 | 평가한 응용은 모두 Linux다. Simics의 판본·기계·guest는 서술이 없다 | p.1, p.8 | C3의 원 플랫폼은 Microblaze이고 OS는 미확인이다(`cases/ledger.md` §1b `[기록]`) |
| 10 | RacePro는 failure-inducing data input이 이미 있다고 가정하는 탐지 도구라고 쓴다. RacePro와 SimRacer는 구체적 data input을 요구하고 그 schedule 탐색이 로그에 안내받지 않는다고 비교한다 | p.8, p.10 | DESCRY의 비교 서술이다. 사례에 대한 입력 요구와 연결하는 근거로 쓰지 않는다 |

### 2.4 2차 문헌의 DESCRY 서술

| 출처 | 내용 | 논문과의 대조 |
|---|---|---|
| SysPro p.2 | "Descry [12] utilizes default console logs for bug reproduction"이고 이 로그는 bug report만 있는 현실의 경우에 없을 수 있다는 취지 | DESCRY p.1, p.3의 입력 서술과 일치 |
| SysPro p.22 | DESCRY의 벤치마크 21개 중 3개는 bug report가 없어 SysPro 벤치마크에서 뺐다. DESCRY도 평가를 비슷한 방식으로 했다고 적는다 | 벤치마크 수가 DESCRY 본문(24행, 22건 재현)과 다르다(§1.3 행 6) |
| SysPro p.31 | "We cannot directly compare SysPro with … Descry". DESCRY가 console log를 쓰는데 SysPro 실험의 bug report에는 별도 console log가 없다는 이유다 | 입력 요구 서술과 일치. 도구 실행 결과에 대한 서술이 아니다 |
| SCMiner p.11 | "Descry [17] can reproduce system-level concurrency failures by combining static and dynamic analysis techniques to generate test inputs" 취지. 목표가 다른 기법으로 인용한다 | 일치 |
| SCMiner p.12 | 참고문헌 [45]가 `http://cs.uky.edu/~tyu/research/descry`(2016)다. 본문은 SCMiner의 벤치마크가 [18], [45]에서 쓰였다고 적는다 | 이 URL이 프로젝트 페이지다. 오늘 열리지 않는다(경로 8) |
| SimEvo | DESCRY 언급 없음(키워드 검색) | — |
| RRF 논문(`RRF.md`) | DESCRY 본문이 RRF를 [29]로 "SimRacer [29, 38]" 묶음 안에서만 인용한다(DESCRY p.8). RRF를 서술하지 않는다 | — |

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

"분석" 셀의 "서술에 경로가 없다"는 "읽은 설계 서술 안에서 그 사건이 입력이 되는 경로를 찾지 못했다"는 뜻이다. 실행 결과가 아니다. 셀의 근거는 별도 표시가 없으면 DESCRY 논문(`[전문]`)의 쪽이고, 사례 쪽 사실은 `[기록]`이다.

**모든 "분석" 셀에 공통으로 걸린 전제 세 가지.**
- **전제 1(사건).** 사례의 사건이 DESCRY의 사건 집합(syscall 사건과 signal. p.3, p.7)에 들어와야 후보 쌍이 된다. 사건·자원 모델링 상세가 [21, 38]에 위임돼 있어(p.3), DESCRY 구현이 syscall 밖의 사건도 보는지는 DESCRY 논문으로 정해지지 않는다. 이 문서의 "분석" 셀은 **서술된 사건 집합**에 대한 것이다.
- **전제 2(단위).** 한 process 안의 task(thread) 쌍을 PuD로 줄 수 있어야 한다. PuD 규칙(p.4)은 process와 signal만 이름 붙이고, process당 thread 하나를 가정하며(p.3), thread 생성(`clone` 계열)의 취급은 서술이 없다. 줄 수 있는지는 **불명**이다.
- **전제 3(입력).** 실패한 실행의 default log와 PuD 소스(p.1, p.3)가 있어야 한다. 사례별 충족 상태는 §3.2에 `[기록]`으로 적는다.

### 3.2 사례 쪽에서 쓴 사실 (모두 저장소 기록, 재검증하지 않음)

| 사례 | 의존성과 현상 | 실패 로그 / failure point | 출처 |
|---|---|---|---|
| C1 CF#184 | provider가 이름 있는 OSAL count semaphore를 만든 뒤에야 CF의 `OS_CountSemGetIdByName`이 성공한다. provider는 원 보고에서 미식별이고 공개 BP 구성에서는 `OS_CountSemCreate`다. 수정 전 CF는 한 번만 조회하고 실패하면 초기화 실패 이벤트와 `APP_ERROR` 종료다. 수정은 `OS_ERR_NAME_NOT_FOUND`일 때만 25회 × 100 ms 재시도다. 생성이 조회보다 먼저임을 정하는 장치가 없다 | 원 보고 본문에 로그 없음. 댓글 5건 미독. 수정 전 소스에 실패 문장이 있다(`cf_cfdp.c@2a024d8e:1018-1020`의 `CFE_EVS_SendEvent`, "CF: failed to get sem id for name …") | `cases/ledger.md` §1a, §1b, §2; `baselines/descry.md` §6 `[기록]` |
| C2 cFE#198 | 앱 P의 late init 완료와 다른 앱 R의 의존 호출. P·R은 비공개(EVA CWS)이고 통신 방식·초기화 내용이 공개되지 않았다. 공개된 것은 단일 상태 시작 동기화(`WaitForStartupSync`)와 6.6.0a의 두 단계 방식(`APPS_INIT`, `LATE_INIT`, `CFE_ES_WaitForSystemState`)이다 | 본문에 로그·앱·함수 이름 없음. 댓글 수는 기록이 충돌한다(§1.3 행 7) | `ledger.md` §1a, §1b, §2 `[기록]` |
| C3 cFE#73 | SB task의 `CFE_SB.AppId`와 EVS task의 `EVS_AppID`·등록 플래그가 설정되기 전에 TIME task가 `CFE_SB_CreatePipe` 뒤 `CFE_EVS_SendEventWithAppID`(AppId=0)를 부르고 `EVS_IsFiltered(0xFFFFFFFF)`가 범위 검사 없이 접근한다. 우선순위(TIME 60, EVS 61, SB 64)가 순서를 만든다. 원 플랫폼은 Microblaze이고 OS는 미확인이다 | 본문에 로그 없음. 댓글 14건 중 일부만 검색 snippet으로 읽었고 로그로 보이는 내용은 snippet에 없었다(전문 미독). 현상은 segfault로 메시지가 없다 | `ledger.md` §1a, §1b `[기록]`; `descry.md` §6 |
| C5 cFE#72 | `CFE_ES_AppCreate`가 ES lock을 `OS_TaskCreate` 앞에서 풀고 뒤에서 다시 잡아 `TaskTable`을 채운다. 그 사이 새 task의 `CFE_ES_GetAppID`(ES lock을 잡는다, `cases/C5.md` F5)가 `CFE_ES_ERR_APPID`를 반환할 수 있다. readiness violation은 "등록 전 조회"이고 일찍 실행된 것만으로는 위반이 아니다. 수정(6.4.2)은 lock 범위 확장이다 | 본문에 로그 없음. 댓글 9건 미독. failure point는 앱이 `GetAppID` 실패 때 출력하는지에 달려 있고 앱은 원 보고에 없다 | `ledger.md` §1a, §1b `[기록]`; `descry.md` §6 |
| R2 cFE#2663 | SB가 `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`로 AppId를 저장하기 전에 ES/EVS task가 SB API 경로(`CreatePipe`, `Subscribe`, 이벤트 송신)를 실행해 `EVS_GetAppDataByID`가 NULL을 돌려주고 `ILLEGAL_APP_ID`가 된다. 서술은 "이벤트 요청 거부"이다. gdb 원시 기록 3회 모두 SB AppId 설정 전 사용이 24회(AppId 0)이고 24회 모두 `ILLEGAL_APP_ID`로 끝났다(`env/logs/r2_g1..3_trace.jsonl`, 2026-10-16 재계수 [M], 로드맵 §2.2와 같음). 한 원인의 동적 발생이며 독립 결함 24개가 아니다(`results/R2_gdb_v701.md` §6). core 시작 직렬화(S1)가 EVS·ES 초기화를 SB보다 앞세운다. 고정 순서인지 스케줄 의존인지는 미결이다 | 본문에 로그 없음("there is no event outputted"). 재현 절차는 gdb breakpoint다. 댓글 2건은 검색 snippet 조각만 기록돼 있고 조각에 로그가 없다. 현상은 오류 반환이고 출력이 없다(v7.0.1 `c5fb2b4d`의 `cfe_evs.c` L185–188) | `cases/R2.md` §1.2–1.3, §6; `ledger.md` §1a, §2; `descry.md` §6 `[기록]`. 실행 증거는 gdb 원시 기록(`env/logs/r2_g1..3_*`, `results/R2_gdb_v701.md`) |
| 공통 | native cFS는 한 Linux process이고 OSAL task는 `pthread_create` thread다 `[I 소스]`. OSAL 자체 시험에서 named count semaphore의 생성과 이름 조회는 syscall 0건, `OS_QueueCreate`는 `mq_open("<pid>.<name>")`, `OS_TaskCreate`는 이름 없는 `clone3`로 보였다 `[M-probe 기록]`(OSAL `d2d877a`, 경합 없음, cFS 아님, 사례 revision 아님). SB pipe 생성이 `OS_QueueCreate`를 거친다는 연결은 `[I 소스]`다 | — | `availability.md` §3, §4.2, §4.5; `osal-visibility.md` probe 표. probe이며 DESCRY의 결과가 아니다 |

### 3.3 도구 수준의 답 (사례와 무관)

| Q | 라벨 | DESCRY 논문(`[전문]`)의 답 | 근거 |
|---|---|---|---|
| 1 관측 사건 | 답 | (필드) 실패 실행의 default log message. (사내) PuD 실행의 system call 사건. "the systems calls are modeled as concurrency events"이고 schedule의 각 사건은 공유 자원 R/W 또는 동기화 연산이다. schedule은 "inter-process events (system calls)"의 순서로 정의된다. 사용자 공간 메모리 접근, 함수 반환값, futex·mutex·semaphore, 등록·초기화 상태를 사건으로 쓴다는 서술은 없다. 모델링 상세는 [21, 38]에 있다고 쓴다 | p.3, p.7 |
| 2 자원·세대 식별 | 답 | 시스템 전역 자원을 syscall로 접근하는 것으로 본다. 사건은 자원 이름(`R(A.txt)`)으로 표기한다. PuD별 입력이 같은 자원을 가리키게 하는 장치는 "입력 접근 syscall 목록이 둘 다 비어 있지 않으면 공유 자원으로 보고 파일 이름을 통일"이다. 이름·ID·slot 대응, 객체 재생성의 세대에 관한 서술은 없다 | p.3, p.4 |
| 3 생성·등록 vs 의존 사용 | 답 | 두 사건의 순서가 뒤집히면 실패하는 경우를 "자원에 대한 write 후 read"(예: `unlink` 뒤 `open`, `rename`과의 순서)로 다룬다. 생성·갱신은 syscall의 write 효과로, 의존 사용은 read로 표현한다. 등록 완료, 초기화 완료, readiness 같은 상태 개념은 서술에 없다. 관계의 목록은 5종으로 닫혀 있다 | p.2–3, p.7 |
| 4 필요 순서·허용 오류 입력 | 답 | 입력 목록(PuD 소스, default log, 실행 중 process 집합, 필요하면 입력 종류)에 필요한 순서나 허용 오류를 받는 필드가 없다. 순서는 입력도 추론도 아니고 탐색으로 만든다. "무엇이 잘못인가"는 로그의 실패 메시지(failure point)가 정한다. 재시도·허용 오류 서술은 없다(키워드 `retry` 0건) | p.1–5, p.7 |
| 5 예측·탐색, 확인 | 답 | 첫 구체 실행의 trace에서 POG를 만들고, 순서 관계가 없는 pair를 뒤집어 아직 실행되지 않은 순서를 만든다(predictive). 우선순위는 write와 실패 지점 근접도다. Simics replay로 새 schedule의 실행 가능성과 실패 도달을 확인한다. infeasible이면 건너뛴다. 분석은 PuD 둘씩이다. 순서를 뒤집을 수 없는 이유를 결과로 보고한다는 서술은 없다 | p.4, p.7 |
| 6 출력 | 답 | ⟨T, S⟩와 재현 성공 여부. 후보 목록이나 오류 결과의 설명이 아니다. 성공은 실패 메시지 문장에 도달하는 것이다. 목표가 infeasible이면 failure predicate로 완화한다 | p.3–4, p.7–8 |

### 3.4 5 × 6 라벨 요약

| | Q1 관측 | Q2 식별 | Q3 생성·사용 | Q4 입력 | Q5 예측·확인 | Q6 출력 |
|---|---|---|---|---|---|---|
| **C1** | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| **C2** | 불명 | 불명 | 분석 | 답 | 불명 | 불명 |
| **C3** | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| **C5** | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |
| **R2** | 분석 | 분석 | 분석 | 답 | 분석 | 분석 |

Q4가 다섯 사례 모두 "답"인 것은 도구 수준의 답이 사례와 무관하기 때문이다(입력 필드가 없다). 2026-10-16 보완에서 라벨은 바뀌지 않았다. 바뀐 것은 칸 안의 근거 표기(`[M-probe 기록]`, `[I 소스]`)와 인용 쪽, R2 수치의 근거다.

### 3.5 사례별 셀 (근거 포함)

#### C1 — CF#184 (named count semaphore의 생성 vs 이름 조회)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | 필요한 사건은 provider의 이름 등록(OSAL 이름 테이블 쓰기, `OS_CountSemCreate`가 반환한 시점)과 CF의 `OS_CountSemGetIdByName` 조회(읽기)와 그 반환값이다. OSAL 자체 시험에서 생성 3회와 조회 4회가 syscall 0건이었다 `[M-probe 기록]`(OSAL `d2d877a`, 경합 없음. C1의 OSAL `42af0f73`에서는 실행하지 않았다). DESCRY의 사건은 syscall이므로(p.3, p.7) 이 두 사건이 후보가 되는 경로를 서술에서 찾지 못했다. 사건 모델링이 [21, 38]에 위임돼 있어(p.3) DESCRY 구현이 syscall 밖의 사건을 보는지는 정해지지 않는다(전제 1) | p.3, p.7; `availability.md` §4.2 `[M-probe 기록]` |
| 2 | 분석 | 이름 `BP0_tsem` → OSAL object ID → 이름 테이블 slot → 객체를 같은 자원으로 잇는 장치가 서술에 없다. DESCRY의 자원 식별은 syscall의 자원 이름과 입력 파일명 통일이다(p.4). 삭제·재생성된 객체를 같은 세대로 보지 않는 장치도 서술이 없다 | p.3–4 |
| 3 | 분석 | 생성 → 사용은 "write 후 read"로 표현되는 모양이지만(p.2–3) 이 사례의 두 사건은 probe에서 syscall로 보이지 않았다 `[M-probe 기록]`. OSAL 레코드가 예약 중인지, 공개 완료인지, CF가 재시도 중인지 최종 실패인지 구분하는 개념은 모델에 없다 | p.3, p.7 |
| 4 | 답 | 도구 수준 답과 같다. 필요한 순서("생성이 조회보다 먼저")와 허용 오류("재시도는 `OS_ERR_NAME_NOT_FOUND`일 때만")를 넣을 입력이 없다. 무엇이 실패인지는 로그의 메시지가 정한다 | p.1–5 |
| 5 | 분석 | 뒤집을 pair는 첫 구체 실행의 trace에서 나온다(p.7). 두 사건이 trace에 없으면 후보가 되지 않는다. replay의 제어 대상은 syscall 사건이다(p.7). provider를 생성 직전에 보류하는 식의 제어(PI P-C1HOLD [D])에 해당하는 서술은 없다 | p.7 |
| 6 | 분석 | 출력은 ⟨T, S⟩와 failure point 도달이다. 수정 전 소스에는 실패 문장이 있다(`CFE_EVS_SendEvent`, `[기록]`). 다만 DESCRY의 logging point 식별은 printing API(`printf`, `sprintf` 등)에 넘긴 format string과 로그 부분 문자열의 일치다(p.5). p.5는 "customized logging facilities"를 어려움으로 들고, printing API가 라이브러리 안에 있으면 data·control flow로 PuD 소스까지 되짚는다고 쓴다. cFE에서는 앱의 format string이 `CFE_EVS_SendEvent`의 가변 인자로 넘어가 `vsnprintf`로 이벤트 패킷이 되고, 출력 port가 켜져 있으면 `OS_printf("EVS Port%u %s\n", …)`로 나간다(v7.0.1 번들 cFE `c5fb2b4d`의 `cfe_evs_utils.c` L437–L493, L546–L599. 기본 port 설정은 `cmake/sample_defs/example_platform_cfg.h` L979의 `0x0001` `[I 소스]`. C1 구성의 cFE revision에서는 읽지 않았다). 이 경로가 DESCRY의 되짚기 대상인지, `OS_printf`가 printing API 목록에 드는지는 서술이 없다. 수정 후 재시도로 회복하는 경로는 failure point를 지나지 않는다. 입력 로그가 없는 것은 별개의 조건이다 | p.3, p.5; `descry.md` §6 `[기록]` |

#### C2 — cFE#198 (late init 완료 vs 다른 앱의 의존 호출)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 불명 | 사건을 정하는 P·R의 통신 방식과 초기화 내용이 공개되지 않았다. DESCRY는 PuD의 소스를 요구한다(p.1, p.3). 공개된 것은 ES의 시작 동기화(상태 변수와 폴링)이며 이것이 DESCRY의 syscall 사건 집합에 어떻게 나타나는지는 사례 쪽 자료로 정해지지 않는다 | p.1, p.3; `ledger.md` §1a `[기록]` |
| 2 | 불명 | P의 상태와 R의 호출 대상이 무엇인지 정해지지 않는다 | `ledger.md` §2 `[기록]` |
| 3 | 분석 | 도구 쪽 서술만으로 말할 수 있다. 관계가 5종으로 닫혀 있고(p.7) "초기화 완료"나 시작 동기화 통과에 해당하는 개념이 없다(키워드 `initializ`, `startup` 0건). P의 late init 끝에 syscall 표지가 있는지는 비공개라 사례 쪽은 불명이다 | p.7 |
| 4 | 답 | 도구 수준 답과 같다. barrier 통과와 앱 고유 초기화 완료를 구분하는 입력이 없다 | p.1–5 |
| 5 | 불명 | 사건이 정해지지 않았다 | — |
| 6 | 불명 | 원 보고에 실패 현상의 출력과 로그가 없다. "R이 P를 호출했다"도 추론이다(`ledger.md` §1a). 이 사례는 역할 재구성(P-C2 [D])으로만 다룬다 | `ledger.md` §1a, §2 `[기록]` |

#### C3 — cFE#73 (SB/EVS AppId 설정 vs TIME의 이벤트 요청)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | 사건은 SB·EVS의 AppId 저장과 EVS 등록 플래그 설정(메모리 쓰기), TIME의 `CFE_SB_CreatePipe` 뒤 `CFE_EVS_SendEventWithAppID(AppId=0)`와 `EVS_IsFiltered`의 범위 검사 없는 접근(메모리 읽기)이다. pipe 생성은 `OS_QueueCreate`를 거쳐 `mq_open`을 부른다(`mq_open`은 `[M-probe 기록]`, OSAL `d2d877a`. `CreatePipe`와의 연결은 `[I 소스]`이며 C3의 6.4.1 시대 OSAL에서는 probe를 실행하지 않았다). 그러나 이것은 의존 상태(AppId, filter 상태)에 대한 접근이 아니다. 의존 사건인 메모리 쓰기·읽기에 syscall이 없다는 것은 `[I 소스]`다. DESCRY가 message queue를 자원 접근 사건으로 모델링하는지는 서술이 없다(키워드 `mq_` 0건). 서술된 사건 집합에 의존 사건이 들어오는 경로를 찾지 못했다 | p.3, p.7; `availability.md` §3 `[기록]` |
| 2 | 분석 | SB AppId, EVS 자신의 AppId, EVS 등록·필터 상태가 서로 다른 필드다(`ledger.md` §1a). DESCRY는 필드 단위의 식별 서술이 없고 자원은 syscall의 대상이다 | p.3–4 |
| 3 | 분석 | SB·EVS 초기화 완료(publish) 뒤에야 TIME의 이벤트 요청이 유효하다는 관계를 표현할 장치가 없다. 순서는 task 우선순위가 만든다(`[기록]`). DESCRY의 관계 5종은 이를 담지 않는다 | p.7; `ledger.md` §1a |
| 4 | 답 | 도구 수준 답과 같다 | p.1–5 |
| 5 | 분석 | 순서가 우선순위로 정해진다는 점은 "kernel scheduler 제어"가 필요한 문제(p.3)와 가깝다. 그러나 뒤집을 사건 pair가 trace에 있어야 한다는 조건이 먼저다. 원 플랫폼이 Microblaze이고 DESCRY의 Simics 설정은 서술이 없다 | p.3, p.7 |
| 6 | 분석 | 현상은 EVS 내부의 잘못된 인덱스 접근에 따른 segfault이고 메시지가 없다(`descry.md` §6 `[기록]`). DESCRY의 failure point(p.3)는 메시지를 출력하는 문장이므로 성공 기준을 세울 수 없다. 논문은 core dump를 쓰는 기법을 별도 부류로 서술한다(p.10). 이 사례의 crash 전후에 로그에 남는 메시지가 있는지는 정해지지 않는다(댓글 미독) | p.3, p.10; `descry.md` §6 `[기록]` |

#### C5 — cFE#72 (task 생성과 레지스트리 등록 사이의 공백)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | 사건은 `OS_TaskCreate`(`clone3` syscall로 보임), ES lock 해제와 재획득 사이의 `TaskTable` 등록 쓰기(메모리), 새 task의 `CFE_ES_GetAppID` 읽기(메모리, lock 아래)다. 이 중 probe에서 syscall로 보인 것은 `OS_TaskCreate`의 `clone3`다 `[M-probe 기록]`(OSAL `d2d877a`. C5의 OSAL 4.1.1 시대에서는 실행하지 않았다). 등록 쓰기와 조회는 ES lock 아래의 메모리 접근이고(`cases/C5.md` F5 `[기록]`), 경합이 없으면 syscall이 없으며 경합하면 lock 주소의 futex가 보일 수 있다 `[I 소스]`(`availability.md` §4.5). DESCRY 논문에는 futex·mutex 사건의 서술이 없다(키워드 0건). DESCRY는 `clone`을 `/proc` 아래 process inode 생성(write)으로 모델링하고(p.3) fork→child return을 child **process**에 정의한다(p.7). thread 생성(`CLONE_THREAD`)의 취급은 서술이 없다(전제 2) | p.3, p.7; `availability.md` §3, §4.2 `[기록]` |
| 2 | 분석 | OS task ID ↔ ES AppId ↔ `TaskTable` slot의 대응과 "등록 전" 상태를 식별하는 장치가 서술에 없다 | p.3–4 |
| 3 | 분석 | 생성(`clone3`) → 등록(메모리 쓰기) → 조회(lock 아래 메모리 읽기)에서 probe에서 syscall로 보인 것은 생성 한 조각이다 `[M-probe 기록]`. readiness violation은 "등록 전 조회"이고 일찍 실행된 것만으로는 위반이 아니다(`[기록]`). 이를 표현하려면 등록 사건과 조회의 반환 결과가 필요하다 | p.7; `ledger.md` §1a |
| 4 | 답 | 도구 수준 답과 같다 | p.1–5 |
| 5 | 분석 | 새 task가 먼저 실행되는 창은 lock 해제와 재획득 사이다. pair를 뒤집으려면 등록 쓰기와 조회가 trace의 사건이어야 한다. `clone3`는 사건이 되더라도 순서가 의미를 갖는 두 접근이 아니다 | p.7 |
| 6 | 분석 | 현상은 새 task의 첫 조회 실패(`CFE_ES_ERR_APPID`)이고 이후 영향은 앱에 달려 있다. 원 보고에 앱과 로그가 없어 failure point가 앱 의존이다(`[기록]`). DESCRY의 성공 기준(p.3)을 세울 수 있는지는 정해지지 않는다 | p.3; `descry.md` §6 `[기록]` |

#### R2 — cFE#2663 (SB AppId 설정 vs ES/EVS의 이벤트 요청)

| Q | 라벨 | 내용 | 근거 |
|---|---|---|---|
| 1 | 분석 | 사건은 `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`의 저장(메모리 쓰기), ES/EVS task의 SB API 경로 호출, `EVS_GetAppDataByID`의 NULL과 `ILLEGAL_APP_ID` 반환이다. pipe 생성의 `mq_open`은 probe에서 보였고(`[M-probe 기록]`, `OS_QueueCreate` 단위) `CreatePipe`와의 연결은 `[I 소스]`다. AppId 저장, 값 읽기, 반환값에 syscall이 없다는 것은 `[I 소스]`(`cases/mapping_v701.md` §1.2)이며 gdb 기록이 뒷받침하지 않는다. gdb 원시 기록 3회 모두 설정 전 사용 24회가 있었다(2026-10-16 재계수 [M]. gdb 기록이며 DESCRY의 결과가 아니다). 서술된 사건 집합에 들어오는 경로를 찾지 못했다 | p.3, p.7; `availability.md` §3, §4.2; `env/logs/r2_g1..3_trace.jsonl` |
| 2 | 분석 | `CFE_SB_Global.AppId`(SB 자신의 이벤트 원천 ID)와 EVS 등록 정보를 서로 다른 상태로 구분해야 한다. DESCRY는 서술이 없다 | p.3–4 |
| 3 | 분석 | SB가 AppId를 저장한 뒤에야 SB API 경로의 이벤트 요청이 유효하다는 관계를 표현할 장치가 없다. 저장에 syscall이 없다 `[I 소스]` | p.7 |
| 4 | 답 | 도구 수준 답과 같다 | p.1–5 |
| 5 | 분석 | R2의 순서는 core 시작 직렬화(S1)가 만든다는 것이 코드 읽기의 예측이고 실행으로 확인하지 않았다(고정 순서인지 스케줄 의존인지 미결, `ledger.md` §2). DESCRY는 replay가 뒤집은 사건과 맞지 않으면(다른 분기를 타면) 그 schedule을 infeasible로 보고 건너뛰어 다음 pair로 간다(p.7). 뒤집을 수 없는 이유를 결과로 보고하는 서술은 없다. (2026-10-16 추가, [I] 미실행) 사례 기록의 코드 읽기로는 S1에서 ES 시작 thread가 core task를 하나씩 만들고 각 task가 RUNNING이 될 때까지 기다리므로 SB task는 EVS(B) 또는 ES·EVS(M, D)의 init이 끝난 뒤에 생긴다(`cases/R2.md` §6 S1·S2, §6.1 `[기록]`). 그렇다면 ① 쓰기를 ③ 사용보다 앞세우는 schedule은 아직 생기지 않은 SB task를 기다리며 사용 task를 멈춰 두어야 한다. DESCRY의 replay 서술(p.7)은 분기가 달라진 경우만 다루고, 이렇게 진행이 막히는 경우의 처리는 서술이 없다. 그 전에 ①과 ③이 사건 pair가 되는지부터 정해지지 않는다(사건표) | p.7; `ledger.md` §2, `cases/R2.md` §6 `[기록]` |
| 6 | 분석 | 현상은 이벤트 요청이 `ILLEGAL_APP_ID`로 거부되고 아무것도 출력하지 않는 것이다(v7.0.1 `c5fb2b4d`의 `cfe_evs.c` L185–188 `[I 소스]`. gdb 기록의 `cfe_evs.c:188` breakpoint에 run마다 24회 도달 [M 재계수]). 결과 표현은 "이벤트 요청 24회가 거부됨"이다(로드맵 §2.2). 거부 분기는 패킷 생성(`EVS_GenerateEventTelemetry`, L204)보다 앞이므로 C1 Q6 칸의 port 출력 경로도 실행되지 않는다 `[I 소스]`. DESCRY의 failure point(p.3)는 메시지를 출력하는 문장이므로 이 현상으로는 성공 기준을 세울 수 없다. 목표 완화(p.4)는 논문의 예에서 메시지 출력 줄을 그 앞의 검사 줄(failure predicate)로 바꾸는 장치로 나온다. 메시지가 처음부터 없는 경우의 처리는 서술이 없다 | p.3–4; `descry.md` §6 `[기록]` |

#### R2 사건 대응표 (2026-10-16 추가. 분석, 미실행. `process_race_method_matrix.md` §6.2와 같은 사건 번호)

DESCRY의 층에 맞춰 열을 잡았다. "사건 집합"은 syscall 사건과 signal이고(p.3, p.7), "순서 관계"는 p.7의 5종이며, "pair 후보"는 실패 지점에 가장 가까운 사건 e_f와 그것과 순서 관계가 없는 가까운 사건 e_i의 쌍이다(p.7). 사건·자원 모델링 상세가 [21, 38]에 위임돼 있으므로(p.3) 이 표는 **서술된** 모델에 대한 것이다.

| # | R2 사건 | 소스 위치 (`[기록]`) | syscall 흔적 (표기) | DESCRY 사건 집합과 자원 (p.3) | 순서 관계 (p.7) | pair 후보 (p.7) | 대응 라벨 |
|---|---|---|---|---|---|---|---|
| ⓪ | **core task 생성.** ES 시작 thread가 `CFE_ES_CreateObjects`에서 core task를 하나씩 `OS_TaskCreate`로 만들고, 각 task가 RUNNING이 될 때까지 `CFE_ES_MainTaskSyncDelay`로 폴링한다(S1·S2) | B `cfe_es_start.c` L707–L871, 동기화 L804–L812(`cases/R2.md` §6) | `OS_TaskCreate` → 이름 없는 `clone3` `[M-probe 기록]`(cFS에서 측정하지 않음). gdb 위반 스택 24개 중 10개의 가장 바깥 frame이 `start_thread`·`clone3`다(나머지는 14 frame 제한으로 잘림) [M, 2026-10-16]. 이것은 backtrace frame이며 syscall 관측이 아니다. 폴링은 메모리 읽기와 지연 `[I 소스]` | `clone`을 "/proc 아래 process inode 생성"(write)으로 모델링한 예가 있다(p.3). thread 생성에 같은 모델이 쓰이는지는 서술이 없다 | fork-return은 "the fork that starts the child process"에 정의된다(p.7). thread 생성에 적용되는지 서술이 없다. RUNNING 폴링은 5종에 없다 | pair 후보가 아니라 관계의 원천일 수 있는 행이다 | **분석**(적용 여부는 서술 없음) |
| ① | **AppId 설정.** `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`, 반환값 무시 | B `cfe_sb_task.c` L126, M L138, D L128(`case_basis_register.md` R2.E2). gdb 기록은 v7.0.1 L138(설정 직전)과 L142(설정 뒤)에 breakpoint를 둔다 [M, trace 머리 행] | syscall 없음 `[I 소스]` | 서술된 사건 집합에 들어오는 경로를 찾지 못함 | 해당 없음 | 짝이 될 사건이 없음 | **대응 경로를 찾지 못함**(분석) |
| ② | **값 전달.** SB API 경로가 `CFE_SB_Global.AppId`를 읽어 `CFE_EVS_SendEventWithAppID`의 인자로 넘김 | R2.E3 | syscall 없음 `[I 소스]` | 위와 같음 | 해당 없음 | 해당 없음 | **대응 경로를 찾지 못함**(분석) |
| ③ | **이벤트 요청.** ES·EVS task 안의 `CFE_SB_CreatePipe`, `CFE_SB_SubscribeFull`, `CFE_SB_MessageTxn_ReportSingleEvent` → `CFE_EVS_SendEventWithAppID`. 설정 전 사용은 함수별 2·4·18, task별 ES 20·EVS 4 [M 재계수] | ES `cfe_es_task.c` L367·L377·L387, EVS `cfe_evs_task.c` L289·L297·L304(R2.E3) | `CreatePipe`만 `OS_QueueCreate` → `mq_open`(`[M-probe 기록]`은 `OS_QueueCreate` 단위, 연결은 `[I 소스]`). `Subscribe`와 이벤트 요청 자체에는 syscall이 없다 `[I 소스]` | DESCRY 논문에 message queue·`mq_`·shared memory·semaphore 서술이 없다(키워드 0건). `mq_open`이 자원 접근 사건으로 모델링되는지 **불명**. 되더라도 ①의 상태(AppId)에 대한 접근이 아니다 | pipe-read는 pipe의 stream write → read에 정의된다(p.7). POSIX message queue에 쓰이는지 서술이 없다 | `mq_open`이 사건이 되더라도 짝이 될 ① 쓰기가 사건 집합에 없다 | **분석**(`mq_open` 모델링은 불명) |
| ④ | **오류 반환.** `EVS_GetAppDataByID` NULL → `ILLEGAL_APP_ID`, SB 호출자가 상태를 버림 | v7.0.1 `c5fb2b4d` `cfe_evs.c` L185–L188. B L182–186, M·D L185–189(`cases/R2.md` §9) | 반환값. syscall 없음 `[I 소스]`. 패킷 생성(L204)과 port 출력 전에 끝난다 `[I 소스]` | 사건이 아니다. failure point(메시지를 출력하는 문장, p.3)도 아니다 | 해당 없음 | e_f는 "실패 지점에 가장 가까운 사건"이다(p.7). R2에는 failure point가 없어 e_f를 고를 기준이 서술상 없다. 목표 완화(p.4)는 메시지 출력 줄을 앞의 검사 줄로 바꾸는 예로만 나온다 | **대응 경로를 찾지 못함**(분석) |

R2의 "syscall 없음"(①②④)은 gdb 기록이 뒷받침하지 않는다. gdb 기록은 breakpoint 기록이고, cFS에서 strace를 돌린 기록은 없다(`osal-visibility.md` §0 D1 미결). R2의 syscall 부재는 소스 읽기(`cases/mapping_v701.md` §1.2)일 뿐이다. 이 표의 어느 칸도 "DESCRY가 R2를 재현하지 못한다"는 뜻이 아니다. 실행하지 않았고 구현이 없다.

#### 보충: C1·C3·C5·R2의 사건 대응표 (분석, 미실행)

| 사례 | 의존 사건(공개·사용) | syscall로 보이는가 (표기는 "증거 태그" 절의 규칙) | DESCRY의 서술된 사건 집합과의 대응 |
|---|---|---|---|
| C1 | 이름 등록(`OS_CountSemCreate`), 이름 조회(`OS_CountSemGetIdByName`) | 0건 `[M-probe 기록]`(OSAL `d2d877a`, C1의 `42af0f73` 아님) | 대응 경로를 찾지 못함 |
| C3 | SB·EVS AppId 저장, 등록 플래그, TIME의 이벤트 요청과 범위 검사 없는 접근 | 의존 상태에 syscall 없음 `[I 소스]`. pipe 생성의 `mq_open`은 `[M-probe 기록]`(`OS_QueueCreate` 단위), `CreatePipe` 연결은 `[I 소스]`. C3의 6.4.1 시대 OSAL에서는 probe를 실행하지 않았다 | 대응 경로를 찾지 못함. `mq_open`은 의존 상태가 아님 |
| C5 | `clone3`, `TaskTable` 등록 쓰기, `GetAppID` 조회 | `clone3` `[M-probe 기록]`. 등록·조회는 ES lock 아래 메모리 접근이며 경합 시 futex가 보일 수 있다 `[I 소스]` | `clone`의 모델(p.3)이 thread에 적용되는지 불명. 등록·조회는 경로를 찾지 못함 |
| R2 | AppId 저장, SB API 경로의 이벤트 요청과 반환 | 위 R2 사건 대응표 | 위 R2 사건 대응표 |

### 3.6 이 절이 말하지 않는 것

- **DESCRY가 어느 사례를 재현하지 못한다는 결과가 아니다.** 실행하지 않았고 구현이 없다. "경로를 찾지 못했다"는 읽은 설계 서술 안의 일이다.
- **DESCRY의 구현이 메모리 사건을 보지 않는다는 주장이 아니다.** 모델링 상세는 [21, 38]에 있다고 논문이 쓰고, 계보의 STVR은 shared memory를 shared resource로 열거한다(STVR p.8 `[2차]`, 단일 취득 경로, 독립 재확인 안 됨). DESCRY가 그 관측 기능을 쓰는지는 정해지지 않는다.
- **DESCRY가 thread에 적용되지 않는다는 주장이 아니다.** 범위 서술(p.1)과 단일 thread 가정(p.3)이 있지만 thread를 어떻게 다루는지는 서술이 없다.
- 사건이 syscall로 보이는지의 근거(`[M-probe 기록]`의 OSAL 자체 시험과 `[I 소스]`의 소스 읽기)는 DESCRY나 어느 도구의 결과가 아니다. probe는 사례 revision에서 실행되지 않았다.
- 사례의 failure point 유무는 저장소 기록(`descry.md` §6)이며 이번에 소스를 다시 열어 확인하지 않았다.

---

## 4. 실행 가능성과 결과 상태

### 4.1 요구 조건과 오늘 확인한 상태

상태는 정책의 용어다. "충족?"은 이 컨테이너 기준이다. `[M]`은 오늘 읽기 전용으로 확인한 값이다.

| # | 요구 | 레퍼런스 | 상태 | 충족? |
|---|---|---|---|---|
| Q1 | DESCRY 구현: LLVM 로그 분석기, KLEE 기반 입력 생성기, Simics 기반 schedule 생성기·replay | p.2, p.6, p.7 | from-reference | **아니오.** 구하지 못했다(§1.1). 주 장애 |
| Q2 | KLEE: "the most recent version of KLEE built from LLVM 3.4" | p.8 | 도구 from-reference. 정확한 tag·commit은 `unspecified_by_reference`(실험 날짜가 없다). DESCRY가 KLEE를 확장했다(p.6)는 점에서 확장본은 Q1에 속한다 | **아니오.** `klee`, `llvm-3.4`, `clang-3.4` 패키지가 없다(`apt-cache` 결과 없음 `[M]`). LLVM은 18.1.3뿐이다 `[M]`. 2026-10-16에도 같다(`/usr/lib`의 LLVM은 `llvm-18`뿐) `[M]` |
| Q3 | Simics Virtual Platform(schedule 생성기와 replay) | p.2, p.7, 참고 [8, 38] | 제품 from-reference. 판본·모사 기계·guest OS·스크립트는 `unspecified_by_reference`. 계보의 STVR p.18(단일 취득 경로, 독립 재확인 안 됨)은 단일 코어 x86 guest(Linux 2.6.15, 3.5.0)와 "하드웨어 가상화를 모사하지 않는 Simics 판본"을 적지만 DESCRY의 설정이라는 서술은 없다 `[2차]` | **아니오.** `which`와 깊이 3의 `find`에서 simics, klee가 보이지 않는다 `[M]`. 호스트에 `/dev/kvm`이 없고 vmx 플래그가 0이며 `CONFIG_MODULES`와 `CONFIG_KEXEC`가 꺼져 있다 `[M]`. Simics가 이 게스트에서 돌 수 있는지는 확인하지 않았다. 2026-10-16 재측정에서도 `klee`·`simics`가 PATH에 없고 `/dev/kvm`이 없으며 vmx 플래그가 0이다 `[M]` |
| Q4 | 실행 중 process 집합: auditd 같은 도구와 `/var/log` | p.5 | from-reference(판본·규칙 없음) | **가능은 하다.** `auditd` apt 후보 1:3.1.2-2.1ubuntu0.1 `[M]`. 소비할 구현이 없어 설치하지 않았다 |
| Q5 | PuD의 소스 코드 | p.1, p.3 | from-reference | cFE/OSAL/CF/BP 소스는 공개다. **C2의 앱 소스는 비공개다**(`[기록]`) |
| Q6 | 실패 실행의 default log | p.1, p.3 | from-reference | **미확인.** 다섯 사례 모두 원 보고 본문에 로그가 없다. 댓글은 이 세션에서 열 수 없다(C1 5, C2 없음 또는 11(§1.3 행 7), C3 14, C5 9, R2 2). R2 댓글은 검색 snippet 조각만 기록돼 있고 조각에 로그가 없다(`cases/R2.md` §1.3). 정책은 댓글을 원 보고의 일부로 본다(`CONDITIONS_POLICY.md` 7행) |
| Q7 | failure point(실패 메시지를 출력하는 문장) | p.3 | from-reference | C1: 수정 전 소스에 후보가 있다. R2: 없음. C3: 없음(segfault). C5: 앱 의존. C2: 불명(`[기록]`, §3.2) |
| Q8 | 대상: C/C++ 멀티프로세스 Linux 응용. 평가는 Linux 응용만이다 | p.2, p.1 | from-reference | cFS native는 C이고 한 process 안의 thread들이다(`[기록]`). 해당 여부는 §4.3의 PI 판단이다 |
| Q9 | 매개변수 N=10, L_max 값 없음. 평가 조건 2시간 제한·5회 반복 | p.6, p.8 | N은 from-reference, L_max는 `unspecified_by_reference`. 평가 조건은 설치 요구가 아니다 | 닿지 않음 |
| Q10 | 논문의 실험 호스트: Core i5-2400, 8 GB, Ubuntu 14.10 | p.8 | from-reference(실험 서술이며 요구로 서술하지 않음) | 다르다: Ubuntu 24.04.4 LTS, 4 vCPU, 커널 `6.18.44-fc-v114`(2026-10-15), `6.18.44-fc-v139`(2026-10-16) `[M]`. 2026-10-16의 `/` 여유는 1.4 GB다 `[M]`. 이 호스트를 쓰면 `deviation`이다 |

### 4.2 결과 상태 (로드맵 §10.4)

오늘 적용되는 상태는 다섯 사례 모두 **`NOT_RUN`**이다(환경·자료·결정 등으로 실행하지 못함. 미탐지 수에 넣지 않음).

| 사례 | 상태 | 이유(사실) | 별도 메모(분석 또는 미확인) |
|---|---|---|---|
| C1 | `NOT_RUN` | 구현 미확보(Q1). KLEE·Simics 환경 없음(Q2, Q3). 실패 로그 미확인(Q6) | 수정 전 소스에 failure point 후보가 있다. 이벤트 문장이 logging point로 식별되는지는 불명 |
| C2 | `NOT_RUN` | 위와 같고, 앱 소스가 비공개(Q5) | 역할 재구성만 가능한 사례 |
| C3 | `NOT_RUN` | 구현 미확보. 실패 로그 미확인. 원 플랫폼 Microblaze의 OS 미확인 | failure point가 없다(분석, segfault) |
| C5 | `NOT_RUN` | 구현 미확보. 실패 로그 미확인. 원 보고에 앱이 없다 | failure point는 앱 의존 |
| R2 | `NOT_RUN` | 구현 미확보. 원 보고 본문은 로그를 싣지 않고 이벤트 출력이 없다고 적는다(`cases/R2.md` §1.2). 댓글 전문 미확인 | failure point가 없다(분석, 오류 반환·무출력) |

- `PATH_NOT_EXECUTED`, `OBSERVATION_GAP`, `ANALYSIS_GAP`, `REPORTED`는 도구를 실행한 뒤에만 쓰는 상태라 오늘은 적용되지 않는다.
- `INAPPLICABLE`은 "원 기법의 명시된 입력·대상 범위에 해당하지 않음"이다. 논문의 범위 서술(p.1)과 PuD 규칙(p.4)이 근거 후보이지만 이 문서가 붙이지 않는다(§4.3 항목 1).
- 원 도구를 실행할 수 없어 논문으로 기법을 재구현하면 원 도구 결과와 분리한다(로드맵 §10.4, `availability.md` §5.1의 `REIMPLEMENTATION`/`EXTENSION`). 재구현을 하지 않았다.

### 4.3 PI 판단이 필요한 항목 (결정 후보. 여기서 정하지 않았다)

| # | 항목 | 선택지와 근거 |
|---|---|---|
| 1 | `NOT_RUN`과 `INAPPLICABLE`의 구분 | (a) 다섯 사례를 `NOT_RUN`으로 둔다(구현 미확보가 실제 장애이므로 현 상태). (b) C1·C3·C5·R2를 `INAPPLICABLE`로 올린다. 근거 후보는 DESCRY 범위 서술(p.1: thread-level bug는 초점 밖)과 PuD 규칙(p.4)이다. 반대 근거는 단일 thread 가정이 설명 편의라는 서술(p.3)과 로드맵 §1.2의 Process-Level 운용 정의다. (c) C2만 입력 미충족(PuD 소스 비공개, p.1, p.3)으로 별도 처리한다. 이 문서는 (a)를 적용했다. 이 항목은 `process_race_method_matrix.md` §5.4 항목 1·14에 올라가 있다 |
| 2 | 사례 이슈 댓글 확인 | DESCRY의 입력 조건(실패 로그)과 C2 댓글 수 충돌(§1.3 행 7)의 확인이 이 경로에 걸려 있다. nasa/CF와 nasa/cFE를 이 세션에 붙이려면 `add_repo`로 자격을 붙여야 하고 동의가 필요하다. 댓글 본문을 사용자가 직접 제공하는 방법도 있다. 이 조치는 다른 baseline 문서(RacePro, SimRacer, RRF, SysPro)의 입력 조건과 공통이다 |
| 3 | 저자에게 코드 문의 | 논문에 저자 이메일이 있다(p.1: T. Yu, T. S. Zaman, C. Wang). 연락하지 않았다. 누구의 이름으로 연락할지가 결정 사항이다(`availability.md` A2와 같다) |
| 4 | 학위논문 PDF | Zaman 학위논문 본문은 Cloudflare 도전에 막혀 있다. 사용자가 PDF를 제공하면 구현 상세(Simics 설정, KLEE 수정)를 확인할 수 있다. P-PAPERS는 사용자 PDF 제공을 착수의 필수 조건으로 만들지 않는다고 정했다 [D] |
| 5 | 재구현(`REIMPLEMENTATION`) 여부 | 논문에 서술된 부분은 로그 → logging point 대응(p.5), goal list(p.5), schedule 뒤집기(p.7)다. 서술되지 않은 부분은 KLEE 확장(p.6의 서술 수준), Simics 설정, 사건·자원 모델링([21, 38]에 위임)이다. 각 선택은 `unspecified_by_reference` 또는 `deviation`이 된다. 재구현은 원 도구 결과와 분리해 표시해야 한다 |
| 6 | (2026-10-16) STVR 사본의 취득 방식 | `process_race_method_matrix.md` §5.4 항목 16. 이 문서는 STVR을 `[2차]` 근거로만 쓰므로 라벨은 바뀌지 않는다. "단일 취득 경로, 독립 재확인 안 됨" 표시는 PI 확인이나 둘째 경로 확보 전까지 유지한다 |

---

## 5. 다섯 사례에서 부족한 것과 Kairos 기능 가설

이 절의 가설은 **연구 가설과 개발 후보**다(로드맵 §10.5). 근거가 있는 만큼만 올렸고 실제 비교 결과와 연결하지 않았다. 이미 가능한 기능은 차별점으로 주장하지 않는다.

### 5.1 DESCRY가 이미 하는 것 — 차별점으로 주장하지 않는다

| 기능 | 근거 |
|---|---|
| 관측된 실행의 순서만 보지 않고 아직 실행되지 않은 순서를 predictive하게 만들고 실제 replay로 확인한다 | p.7 §6 |
| 순서 관계(POG)를 지키며 뒤집는다. 뒤집을 수 없는 schedule은 건너뛴다 | p.7 §6.1–6.2 |
| 로그 메시지를 소스의 logging point에 자동으로 대응시킨다 | p.5 §4.1 |
| 관련 process를 자동으로 고른다(PuD 식별) | p.4–5 §4 |
| 목표가 infeasible일 때 failure predicate로 목표를 완화한다 | p.4 |
| 사건 수준의 우선순위(write, 실패 지점 근접)로 탐색 순서를 정한다 | p.7 |

Kairos의 "예측과 실행 확인", "관측한 실행에 없는 순서의 탐색"은 DESCRY에도 있다. 새롭다고 쓰지 않는다. 차이는 어떤 사건을 입력으로 받는가와 무엇을 근거로 순서를 정하는가에서 찾아야 한다.

### 5.2 가설 (근거가 있는 만큼만)

| ID | 가설 | 지지하는 근거 | 지지하지 않는 것 / 주의 | 확인할 사례 |
|---|---|---|---|---|
| H-DESCRY-1 | **사건 입력.** 다섯 사례의 공개와 사용(AppId 저장, 등록 쓰기, 이름 테이블 쓰기, API 반환값)을 사건으로 관측해 소스와 연결하는 기능이, 서술된 사건 집합(syscall)에 없는 사건을 후보 입력으로 올릴 수 있다 | DESCRY의 사건은 syscall이다(p.3, p.7 `[전문]`). 이 사건들이 syscall로 보이지 않는다는 근거는 C1 생성·조회에 대해서만 OSAL 자체 시험 `[M-probe 기록]`(OSAL `d2d877a`, 사례 revision 아님)이고, R2·C3·C5의 AppId 저장·등록 쓰기·조회는 `[I 소스]`다. 어느 쪽도 DESCRY의 결과가 아니다 | DESCRY가 사건·자원 모델링을 [21, 38]에 위임하고(p.3), 계보의 STVR이 shared memory를 shared resource로 열거한다(STVR p.8 `[2차]`, 단일 취득 경로, 독립 재확인 안 됨). "DESCRY가 메모리를 못 본다"로 쓰지 않는다. 가설은 "서술된 사건 집합에 없다"까지다 | R2, C3, C5, C1 |
| H-DESCRY-2 | **이름·ID·세대 연결.** 이름(`BP0_tsem`) → OSAL object ID → slot → 세대, 그리고 OS task ID ↔ ES AppId ↔ `TaskTable` slot을 같은 자원으로 잇는 기능이 필요하다 | DESCRY의 식별 수단은 syscall 자원 이름과 입력 파일명 통일이다(p.4). 객체 ID·slot·세대 서술이 없다(키워드 `reuse`, `recreate` 0건, `generation`은 모두 다른 뜻) `[전문]`. 사례 쪽 필요는 `[기록]` | "서술이 없다"가 "기능이 없다"의 확인은 아니다. 논문이 `/proc/pid` 자원을 다루지만(p.10) pid 재사용의 취급은 서술이 없다 | C1, C5 |
| H-DESCRY-3 | **성공 기준과 사례 현상의 대응.** 오류 반환(`ILLEGAL_APP_ID`), 상태(`CFE_ES_ERR_APPID`), crash를 결과로 보는 판정이 필요하다 | DESCRY의 성공은 출력된 실패 메시지 문장에 도달하는 것이다(p.3, p.8). R2와 C3의 현상은 메시지가 없다(`[기록]`. R2 원 보고는 "there is no event outputted"라고 적는다, `cases/R2.md` §1.2). R2의 거부 분기는 패킷 생성과 port 출력 전에 끝난다 `[I 소스]`(§3.5 R2 사건표 ④). pair 기준 e_f가 실패 지점에서 정해진다는 서술(p.7)도 같은 조건에 걸린다 | 목표 완화(p.4)와 core dump 계열(p.10)이 부분적으로 관련된다. 메시지가 처음부터 없는 경우를 DESCRY가 어떻게 다루는지는 서술이 없다. 가설은 "서술된 성공 기준과 현상이 맞지 않는다"까지다 | R2, C3 (C5는 앱 의존) |
| H-DESCRY-4 | **고정 순서의 설명.** 뒤집을 수 없는 순서가 왜 고정인지(시작 직렬화 등)를 소스 기반 근거로 결과에 연결하는 기능이 필요하다 | DESCRY는 infeasible schedule을 건너뛰고 이유를 결과로 내지 않는다(p.7 `[전문]`). 재현이 목적인 도구에서 설명은 요구 사항이 아니다 | R2의 순서가 고정인지 스케줄 의존인지가 미결이다(`ledger.md` §2). 결핍이 아니라 목적의 차이일 수 있다. 2026-10-16의 [I] 도출(§3.5 R2 Q5): S1 아래의 뒤집기 schedule은 진행이 막힐 수 있고, 그런 경우의 처리는 p.7에 서술이 없다. 실행 결과가 아니다. 근거가 중간 정도라 사례 쪽 확인 뒤에 쓴다 | R2 |
| H-DESCRY-5 | **readiness 상태의 표현.** 시작 동기화 통과와 앱 고유 초기화 완료를 구분하고, 등록·초기화 완료 상태와 의존 사용을 잇는 표현이 필요하다 | DESCRY의 관계는 5종으로 닫혀 있고 초기화·등록·시작 동기화에 해당하는 개념이 없다(p.7, 키워드 0건) `[전문]` | 도구 쪽 서술의 부재뿐이다. C2는 앱 소스가 비공개라 역할 재구성(P-C2 [D])의 메커니즘 검증에만 쓸 수 있다. C1·C5의 등록·초기화 상태에도 같은 표현이 걸린다. 가설의 강도는 약하다 | C2(역할 재구성), C1, C5 |

### 5.3 근거가 부족해 가설로 올리지 않는 것

- **"DESCRY는 thread-level이라 cFS에 쓸 수 없다."** 범위 서술(p.1)과 단일 thread 가정(p.3)은 있지만 thread를 어떻게 다루는지 서술이 없다. 단위 차이는 비교 논문에서 정의로 밝힌다(§2.3 행 1).
- **"DESCRY는 메모리를 관측하지 못한다."** 사건 모델링이 위임돼 있다(p.3). 정해지지 않는다.
- **"DESCRY는 cFS 사례를 재현하지 못한다."** 실행하지 않았다.
- **"symbolic execution 기반 입력 생성이 필요하다/불필요하다."** 사례는 순서의 문제로 서술돼 있고(`[기록]`), 입력 생성의 필요성은 사례 쪽 자료로 정해지지 않는다.
- **"로그 없이 시작하는 것이 Kairos의 장점이다."** 입력 조건의 차이일 뿐 DESCRY의 결핍이 아니다. 그 영역은 bug report에서 재현하는 SysPro 계열이 다룬다.
- **C2에 대한 도구 부족 가설.** 앱 소스가 비공개라 도구 쪽 서술의 부재만으로 가설을 세울 수 없다(H-DESCRY-5에 한정).

---

## 6. 확인하지 못한 것과 다음에 할 일

| # | 항목 | 이유와 다음 조치 |
|---|---|---|
| 1 | DESCRY 구현 | 논문과 모든 저자·기관 페이지에 위치가 없다. 프로젝트 페이지는 원 서버가 403이다. 다음 조치는 저자 문의(§4.3 항목 3)다 |
| 2 | Zaman 학위논문 본문 | Cloudflare 도전(2026-10-16에도 같다). 초록 전체는 읽었으나 구현 상세가 없다. 본문에 구현 상세(Simics 설정, KLEE 수정, 사건 모델)가 있을 수 있다. 사용자 제공(§4.3 항목 4) |
| 3 | 사건·자원 모델링의 범위 | DESCRY가 [21, 38]에 위임한 모델에 메모리가 포함되는지가 Q1 모든 칸의 핵심 불확실성이다. SimRacer ISSTA'13 본문(`SimRacer.md`에서도 미도달)과 학위논문이 답할 수 있다 |
| 4 | 사례 이슈 댓글 | 프록시가 nasa/CF·nasa/cFE를 막는다. C2 댓글 수 충돌(§1.3 행 7) 포함. R2는 snippet 조각만 기록돼 있다 |
| 5 | ACM 출판본 | Cloudflare 도전. 저자 공개본과 PAR 사본의 텍스트는 같지만 출판본 동일성은 확인하지 못했다 |
| 6 | 프로젝트 페이지가 존재하는지 | 403의 원인을 모른다. 경로가 사라졌는지 접근 제한인지 구분하지 못했다. Wayback 가용성 API에 스냅샷이 없고 CDX 검색은 egress 정책이 막았다(2026-10-16) |
| 7 | 이벤트 문장이 logging point로 식별되는지 | 논문은 printing API를 "such as `printf` and `sprintf`"라 적는다. `CFE_EVS_SendEvent` 계열이 포함되는지는 서술이 없다(C1 Q6) |
| 8 | Table 2의 수치 | 레이아웃 추출이 어지러워 이 문서는 Table 2의 개별 값을 쓰지 않았다. 본문 서술과 Table 1·3의 값만 썼다 |
| 9 | 사례 쪽 failure point 유무 | 저장소 기록(`descry.md` §6)을 그대로 썼다. 2026-10-16에 R2의 거부 분기만 v7.0.1 번들 소스로 다시 확인했다(`cfe_evs.c` L185–L188, 패킷 생성 L204 앞) `[I 소스]`. C1·C3·C5는 소스를 다시 열어 확인하지 않았다 |
| 10 | (2026-10-16) EVS 출력 경로의 판본 차이 | C1 Q6의 EVS 출력 경로는 v7.0.1 번들(`c5fb2b4d`)에서만 읽었다. C1 구성과 C3·C5의 6.4.x에서는 읽지 않았다 |
| 11 | (2026-10-16) R2 사건표 ⓪의 전제 | 사용 task(ES, EVS)가 생성자 thread(ES 시작 thread)가 아니라는 것은 사례 기록 R2.E3·§6 S1에 기댄다. gdb 스택의 `CFE_ES_TaskMain`·`CFE_EVS_TaskMain` frame과 일치하지만 이 문서는 생성 관계를 코드로 다시 확인하지 않았다 |
| 12 | (2026-10-16) STVR 둘째 취득 경로 | 이 문서는 찾지 않았다. 표시는 §4.3 항목 6 |

---

## 7. 출처 목록

**1차**
- DESCRY 저자 공개본: https://chaowang-vt.github.io/pubDOC/YuZW17.pdf (sha256 `f9d7b57bbb0c1269f9143b4ec69286e90348a58fadd071f76f11022e06dfbf60`)
- DESCRY NSF PAR 사본: https://par.nsf.gov/servlets/purl/10075449 (레코드 https://par.nsf.gov/biblio/10075449, sha256 `8f966ebe9d435455dd6195d9c36bb74058e4eae3ffd643a168208b7c3948e2fb`)
- DESCRY 출판본(열지 못함): https://doi.org/10.1145/3106237.3106266
- UKy 연구자 포털 논문 레코드: https://scholars.uky.edu/en/publications/descry-reproducing-system-level-concurrency-failures/
- UKy 연구자 포털 SimDB 과제 레코드: https://scholars.uky.edu/en/projects/crii-shf-simdb-an-automated-framework-to-debugsystem-level-concur-3/
- Zaman 학위논문 레코드(PDF는 열지 못함): https://uknowledge.uky.edu/cs_etds/119/
- ESEC/FSE 2017 목차: https://sigsoft.org/resources/opentoc/ESECFSE2017-TOC.html

**2차·계보**
- SysPro, arXiv 2601.09616v1: https://arxiv.org/pdf/2601.09616v1 (sha256 `8fe5daa4c7c3794715632e4379d9160a9852b723dc4a649317ef6bae0a75e726`)
- SCMiner(ASE 2019) PAR 사본: https://par.nsf.gov/servlets/purl/10166153
- SimEvo(ICSME 2017) PAR 사본: https://par.nsf.gov/servlets/purl/10089644
- STVR 2017(SIMEXPLORER) CPS-VO 사본: https://archive.cps-vo.org/node/36388 (파일 `/file/36388/download/194033`, sha256 `af805231f69ffc0e12d96fe4e4e9a356932653c56c382fdd3debd211bcdd3e08`)
- 제3자 아티팩트 조사: https://github.com/morgen52/SE-artifact (`data/papers.csv` 1504행)
- 서지: https://api.semanticscholar.org/graph/v1/paper/DOI:10.1145/3106237.3106266, https://api.openalex.org/works/doi:10.1145/3106237.3106266, https://api.crossref.org/works/10.1145/3106237.3106266

**2026-10-16 추가 출처**
- Zaman 학위논문 레코드: https://uknowledge.uky.edu/cs_etds/119/ (초록 전체. DOI `10.13023/etd.2022.168`이 이 레코드로 이어진다)
- cFE v7.0.1 EVS 소스(`[I 소스]`): https://github.com/nasa/cFE/blob/c5fb2b4d540bd55eb6c3707da7dd13eee679d4dd/modules/evs/fsw/src/cfe_evs.c#L169-L215 , https://github.com/nasa/cFE/blob/c5fb2b4d540bd55eb6c3707da7dd13eee679d4dd/modules/evs/fsw/src/cfe_evs_utils.c#L437-L599 , https://github.com/nasa/cFE/blob/c5fb2b4d540bd55eb6c3707da7dd13eee679d4dd/cmake/sample_defs/example_platform_cfg.h#L967-L979 (같은 commit의 로컬 사본을 읽었다)
- R2 gdb 원시 기록: `research/cfs_procrace/env/logs/r2_g1_trace.jsonl`, `r2_g2_trace.jsonl`, `r2_g3_trace.jsonl`(2026-10-16 재계수), `research/cfs_procrace/results/R2_gdb_v701.md` §6–§7
- `research/cfs_procrace/cases/R2.md` §1.2–1.3, §6, §9; `cases/C5.md` F5; `cases/mapping_v701.md` §1.2; `plan/case_basis_register.md` R2.E2–E5; `plan/process_race_method_matrix.md` §0.1, §5.1, §5.4, §5.5, §6.2, §9.1; `baselines/osal-visibility.md` probe 표

**저장소 기록(이번에 재검증하지 않음)**
- `research/cfs_procrace/baselines/availability.md`, `baselines/descry.md` (2026-10-08)
- `research/cfs_procrace/cases/ledger.md`, `cases/C1.md`, `cases/C2.md`, `cases/C3.md`
- `research/cfs_procrace/plan/ROADMAP_20261015.md` §10
- `research/cfs_procrace/Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md` (P-PAPERS, T1, P-CLASSGT)
- `research/cfs_procrace/CONDITIONS_POLICY.md`
- 같은 폴더의 `RacePro.md`, `SimRacer.md`, `RRF.md`
