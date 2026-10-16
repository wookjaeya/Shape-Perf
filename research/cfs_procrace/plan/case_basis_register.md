# 사례 기준 대장 — 다섯 사례의 조건표와 기대 결과표 초안 (2026-10-15, 2026-10-16 개정)

- 대상: C1 (nasa/CF #184), C2 (nasa/cFE #198), C3 (cFE #73), C5 (cFE #72), R2 (cFE #2663).
- 위치와 역할: `plan/case_basis_register.md`. `plan/ROADMAP_20261015.md` §6.1의 조건표(9개 필드)를 다섯 사례에 채우고(§3–§7), 사례별 기대 결과표 골격과 해시 규칙(§8), 사례별 확정·제안·미비 요약(§1), 1차 출처 점검 기록(§9), 남은 항목(§10)을 붙인 것이다. ROADMAP §12의 작업 3 "다섯 사례의 조건·미확인 항목을 한 형식으로 정리"의 산출물이며, §6.3의 완료 기준(확정된 항목 / 실험 설계로 제안된 항목 / 아직 없는 항목의 분리)에 대응한다.
- 읽은 입력: `Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md`, `plan/ROADMAP_20261015.md`, `Architecture/spec/SPEC_FINAL.md` rev 1.2(2026-10-16 개정 때 읽은 것은 2026-10-16의 정합성 수정이 들어간 미커밋 작업 트리, sha256 앞 8자 `861041ac`; 커밋 `6be8935`의 파일은 `741809d5`. 앵커 확인은 §9.2 V-SPEC-1과 §9.3 V-SPEC-2. 초안은 rev 1.1을 읽고 썼다), `CONDITIONS_POLICY.md`, `cases/C1.md`·`C2.md`·`C3.md`·`C5.md`·`R2.md`·`ledger.md`·`mapping_v701.md`, `env/CONDITIONS.md`, `results/R2_gdb_v701.md`, `env/logs/r2_g1..3_trace.jsonl`. 2026-10-15 검토 반영 때 추가로 읽은 것: `roadmap_input_20261007.md`, `env/ENV.md`, `env/CONDITIONS.md`의 E4·M1-3·M1-5, `env/logs/s1r2_p1*`(파일 이름·시각·머리말만).
- 성격: **초안이다.** 구현·빌드·실행·설치·삭제를 하지 않았고 커밋하지 않았다. Kairos는 SPEC rev 1.2의 설계 단계이고 SPEC S1 기록기의 R2 첫 실행이 한 번 있다(아래 '상태 주의'). 이 문서의 도구 분류(§8의 T 항목)는 SPEC이 서술한 기대를 옮긴 것이며 도구 출력이 아니다. 값을 새로 정하지 않았다. 출처가 정하지 않은 값은 `미`로 남겼고, PI 결정은 결정으로만 적었으며 사실로 옮기지 않았다.
- **상태 주의 (2026-10-15 검토 반영 개정, 2026-10-16 재확인; 먼저 읽을 것).**
  1. **R2에는 Kairos의 첫 도구 출력이 이미 있고 R2 시트는 그보다 먼저 동결되지 않았다.** 작업 트리에 `env/logs/s1r2_p1*`(파일 10개, 2026-10-15T11:17Z; ENV 바이너리 `core-cpu1`에 Kairos의 `kz observe`로 한 SPEC S1 기록 실행; driver 머리말의 `kz` 경로는 kairos 빌드 디렉터리의 `tools/kz/kz`)가 있다. 이 문서는 파일 이름·시각·머리말만 확인했고 내용과 gdb 기록과의 일치 여부를 평가하지 않았다. 이 대장 §8에는 R2 시트의 sha256이 없다. 따라서 §8.1 규칙 5(a)와 SPEC §7.2·§9.3('사례의 첫 도구 출력 전에 시트를 해시로 동결')의 순서가 R2에서는 지켜지지 않았다. R2 시트(§8.6)의 기대는 **사전 등록이 아니라** 출력 이후에 기재한 것으로 읽어야 한다(특히 E-R2-T3; §8.1 규칙 10). 시트를 지금 동결하고 실제 순서를 변경 로그에 적을지는 orchestrator·PI가 정할 일이다(§10 항목 16). 이 문서가 확인한 한 다른 사례(C1, C2, C3, C5)의 Kairos 출력 파일은 `env/logs`에 없다(`kairos/` 안은 보지 않았다). **2026-10-16 재확인:** `s1r2_p1*` 10개는 커밋 `6be8935`(2026-10-15T23:59Z)로 저장소에 들어갔다. `env/logs`의 파일 목록과 `kairos/` 밖의 `*.kzt` 검색에서 그 뒤의 Kairos 출력은 없고, R2 시트의 해시도 여전히 없다(§9.3 V-R2-R5).
  2. **SPEC은 rev 1.2다.** 2026-10-15 개정은 rev 1.2 작업 트리(sha256 앞 8자 `741809d5`; 그 뒤 커밋 `6be8935`에 같은 파일로 들어갔다)에서 앵커를 확인했다(§9.2 V-SPEC-1). 2026-10-16 현재 작업 트리는 같은 rev 1.2에 2026-10-16의 정합성 수정(D11의 커널을 실행마다 기록, recall을 확장 평가로 배치, ADR 주석)이 더해진 미커밋 파일(2026-10-16T00:07Z 수정, `861041ac`)이며 앵커를 그 파일에서 다시 확인했다(§9.3 V-SPEC-2). SPEC은 용어 변경 단계 Spec-C(T1 = (a))를 적용했으므로 SPEC 본문도 'readiness specification'을 쓴다(SPEC §0.11, 부록 C). 한국어 거울 문서의 이름 변경은 별도 단계로 진행 중이다(2026-10-16 `grep` 기준 `Architecture/01`–`05`에는 새 용어가 있고 `06`–`10`에는 없다). 해시가 바뀌면 앵커를 다시 확인한다.

## 0. 읽는 법

### 0.1 셀 태그

모든 표의 모든 행에 태그가 하나씩 붙어 있다. 태그는 그 행의 **내용이 어떤 종류의 근거인가**를 말한다.

| 태그 | 뜻 | 작업 규칙의 표시 |
|---|---|---|
| `원` | **원출처 사실.** 이슈 본문·댓글, 수정 커밋, 명시된 revision의 소스 코드, 같은 릴리스의 공식 문서·VDD가 직접 말하는 내용 | [F] |
| `저` | **저장소 기록.** 이 저장소의 사례 기록·원장·ENV 기록·gdb 원시 기록이 보고하는 내용. 기록이 있다는 사실이며 그 타당성을 독립적으로 확인했다는 뜻이 아니다. 실행에서 얻은 수치는 `저·M` | [M] 또는 기록 |
| `분` | **연구자 분석.** 원출처·코드·기록을 연결한 추론·해석. 출처가 직접 말한 결론으로 인용하지 않는다 | [I] |
| `선` | **실험 선택.** 실험·분석을 위해 고른, 또는 제안된 조건. `선(ID)`는 PI 결정 ID가 있는 것(**결정이지 사실이 아니다**). `선(제안)`은 ROADMAP·SPEC·사례 기록의 제안이며 PI 결정 전이다. `선(차이)`는 레퍼런스와 다르게 두었거나, 어느 레퍼런스에도 없는 값을 골라 쓴 조건(CONDITIONS_POLICY의 `deviation`; SPEC_FINAL 머리말: 사람이 고른 값은 `deviation`이며 `unspecified_by_reference`가 아니다) | [D] |
| `미` | **unspecified_by_reference.** 레퍼런스가 정하지 않는다. **값을 고르지 않았다.** 필요하면 가정으로 표시하고 PI 결정으로 넘긴다. 값을 골라 실제로 쓴 행은 `미`가 아니라 `선(차이)`(기록 사실은 `저`)다 | [U] |

보조 표시:

- `†` — 행의 내용 중 일부라도 이슈·댓글·PR 페이지 자체에 기대고, 그 페이지를 이 문서 작성 중에 다시 읽지 못했다. 이 세션에서 nasa/CF와 nasa/cFE의 이슈 페이지와 API는 HTTP 403이다(§9). 해당 내용은 사례 기록이 보존한 인용(GitHub MCP 검색 본문, 웹 조각, 검색 발췌)에 근거한다. 같은 행의 `검`은 직접 다시 읽은 부분에만 해당한다. **`†` 행은 §1에서 확정 항목에 더하지 않고 따로 센다.**
- `검` — 행의 주장(또는 `†`가 같이 있으면 그 중 명시한 부분)을 이 문서 작성·개정 중(2026-10-15, 2026-10-16)에 1차 출처(raw.githubusercontent.com의 고정 SHA 파일, 공식 tarball의 sha256 일치 확인 후 파일, 얕은 git fetch, 저장소의 원시 기록 재집계, 2026-10-16에는 로컬 전체 git 클론)로 직접 다시 확인했다. 확인 내용은 §9에 있다(2026-10-16 재점검은 §9.3).
- `M` — 실행에서 측정·기록된 값.

### 0.2 `from-reference`의 뜻

`CONDITIONS_POLICY.md`의 세 상태(`from-reference`, `deviation`, `unspecified_by_reference`)를 쓰되, 이 대장에서 `from-reference`는 **해당 사례의 원출처가 정한 조건**만 뜻한다. 원출처는 (a) 원 보고(이슈 본문·댓글), (b) 원 보고가 지목한 수정 커밋과 명시된 revision의 소스, (c) 그 revision의 공식 문서다.

- **다른 곳의 공개 구성에서 가져온 값은 `from-reference`가 아니다.** 예: C1의 공개 nasa/bp 구성(`BP0_tsem`, CF 65·BP 55), C3·C5의 파생 플랫폼, R2의 ENV 번들 설정. 그 값 자체는 해당 공개 소스의 사실(`원`)일 수 있으나, 사례의 조건으로 쓰는 것은 실험 선택(`선`)이고 원 보고자 환경과 같다는 주장은 별개다(`미`). 이 구분이 필요한 행은 두 행으로 나누었다.
- 조건 표의 `변경 기록`(필드 I)은 세 부류를 나눈다: 원출처의 조건(`원`) / 차이가 있는 조건(`선(차이)`) / 출처가 정하지 않은 조건(`미`).

### 0.3 용어와 근거 규칙

- **T1 = (a).** 이 문서는 판정 대상을 '준비 상태 명세'(readiness specification), 전제를 '준비 상태 전제'(readiness precondition), 위반을 '준비 상태 위반'으로 쓴다. '계약'(API contract)은 문서화된 API 자체의 의무를 말할 때만 쓴다. SPEC의 식별자(`K-ID`, `K-SELF`, `K-PHASE`, `(σ_U, M_api)` 등)와 인용은 그대로 둔다.
- **세 층(QT-3).** 판정 기준(필드 F)은 (층1) 프레임워크 문서·실제 API 동작, (층2) 호출자 요구(준비 상태 전제), (층3) 관측 결과를 따로 적는다. 프레임워크가 어떤 반환을 허용한다고 호출자의 요구 충족까지 인정하지 않는다.
- **CORD.** 현재 문서의 근거로 쓰지 않는다. R2의 실행 근거는 gdb 원시 기록(`env/logs/r2_g1..3_*`, `results/R2_gdb_v701.md`)뿐이다.
- **'race'.** 원출처의 'race'를 도구 class로 자동 변환하지 않는다(P-CLASSGT). 일치 여부는 결과로 보고하며 정확도 분모를 만들지 않는다.
- **N, 임계값, 예산, 노력 수치.** 이 문서에 새 값이 없다. 실행 횟수는 SPEC S1에서 목적별로 정하고(D14), 탐색 상한은 단계 파일럿 뒤에 동결한다(D15).

### 0.4 표의 열

각 사례의 표는 `ID | 항목 | 내용 | 태그 | 근거·비고` 순서다. ID는 `사례.필드번호`(예: `C1.C7`)이며 §8의 기대 결과표가 이 ID로 근거를 가리킨다. 필드는 ROADMAP §6.1의 순서를 따른다: A 사례 정체성, B 소스 기준, C 실행 조건, D 초기 상태, E 의존성, F 정상·오류 판정 기준, G 관측 계획, H 예상 결과, I 변경 기록.

### 0.5 표기: 행 ID와 단계 이름

이 문서는 같은 모양의 약어가 서로 다른 것을 가리키지 않도록 아래 표기를 쓴다. 초안은 §2.2의 행 ID를 `S1`–`S17`로 적어 다른 'S1'·'S7'과 겹쳤으므로 `SH-1`–`SH-17`로 바꿨다.

| 표기 | 뜻 |
|---|---|
| `C1.C7`, `R2.F8`, `C3.E6a` | §3–§7의 사례 행(`사례.필드번호`). 한 행을 사실 행과 분석 행으로 쪼갠 경우 뒤에 소문자를 붙인다 |
| `K1`–`K16` | §2.1 ENV 기준 행. SPEC의 `K-ID`·`K-SELF` 같은 식별자와 다르다 |
| `SH-1`–`SH-17` | §2.2 공유 6.4.x 기준 행(`SH-5a`처럼 쪼갠 행 포함) |
| `SPEC S0`–`SPEC S10`, `SPEC S3a`, `SPEC S1-R2` | SPEC_FINAL §9.3의 단계 이름. `SPEC S1` = 관측기(F0 observer) 단계(D14의 'S1에서 N 결정', D11·D1의 'S1 관측 영향 비교'가 이것이다), `SPEC S3a` = 정적 닫힘·census·인증서 단계, `SPEC S7` = 탐색기 단계. SPEC 단계는 항상 `SPEC` 접두어를 붙인다 |
| `ES 직렬화 S1`–`S4` | `cases/R2.md` §6 '기존 동기화'의 항목 번호(S1 = 코어 task 시작 직렬화, S4 = 우선순위). `env/CONDITIONS.md` M1-3의 'S1'도 이것이다 |
| `E-…`, `V-…` | §8의 기대 결과표 행, §9.2의 점검 행 |
| `C3.md D3`, `R2.md A6` | 파일 이름이 앞에 붙은 번호는 그 사례 기록 파일 안의 결정(D)·가정(A)·질문 번호다 |
| `X1`–`X7`, `D13`, `P-C1HOLD` … | PI 결정 후보 번호(ledger §5)와 PI 결정 ID(RESOLVED). 결정 ID가 붙은 것은 결정이지 사실이 아니다 |

## 1. 사례별 요약

### 1.1 태그별 행 수 (필드 A–G)

집계는 §3–§7의 A–G 필드 행에서 태그의 첫 글자와 `†`·`검` 표시를 센 것이다. 초안의 표를 같은 방식으로 다시 세어 초안의 숫자와 일치함을 확인한 뒤, 2026-10-15 검토 반영(행 쪼개기, 태그 수정)을 마친 문서에서 다시 센 값이다. 2026-10-16 개정에서 C2만 바뀌었다(C2.B2를 `원·검` 행과 `분` 행 C2.B2a로 나누고 `원·검` 행 C2.B9를 더해 행 56 → 58, 확정 11 → 13, `검` 7 → 9; 같은 스크립트로 다시 세었고 다른 사례의 수는 그대로다). 행 수는 항목 수이며 중요도나 진척률이 아니다. 서로 다른 단위를 합산하지 않는다. §3–§7의 태그를 고치면 다시 세어야 한다.

| 사례 | 행 수(A–G) | 확정: 원+저(`†` 없음) | 원문 미재확인: `†` 행(원/저) | 분 | 선: 결정 ID | 선: 제안 | 선: 차이 | 미 | `검` 표시 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| **C1** | 77 | 21 (17+4) | 7 (7/0) | 10 | 18 | 7 | 0 | 14 | 17 |
| **C2** | 58 | 13 (8+5) | 7 (7/0) | 7 | 10 | 6 | 1 | 14 | 9 |
| **C3** | 66 | 19 (14+5) | 11 (11/0) | 9 | 9 | 7 | 0 | 11 | 15 |
| **C5** | 64 | 18 (14+4) | 5 (5/0) | 7 | 8 | 9 | 0 | 17 | 15 |
| **R2** | 74 | 24 (11+13) | 9 (7/2) | 12 | 14 | 7 | 2 | 6 | 16 |

- **확정된 항목** = `원` + `저` 중 `†`가 없는 행. 초안은 `원†`·`저†` 행도 확정에 더했고 `†`를 하위 표시로만 셌다. `†` 행은 행의 내용 중 일부라도 다시 읽지 못한 이슈·댓글·PR 페이지에 기대므로 이제 확정 열에 더하지 않고 **'원문 미재확인' 열**로 따로 센다. 검색 발췌에 기대는 행(C3.A2, C3.A7, R2.A8 등; 발췌의 지위는 X2 미결정)은 모두 `†`이므로 확정에 들어가지 않는다. `검` 표시는 같은 행에서 직접 다시 확인한 부분이 있는 행이며 `†`와 겹칠 수 있다(C2.B1, C3.C3, C5.F2).
- **연구자 분석** = `분`. 사실로 세지 않는다. 초안에서 `원·검`이나 `저`로 적었던 행 중 분석을 품은 것은 사실 행과 분석 행으로 쪼개거나(C1.B6/B6a, C3.E6/E6a, C5.E6/E6a, SH-5/5b) `분`으로 내렸다(C1.C7).
- **실험 설계로 제안·결정된 항목** = `선`. 결정 ID가 붙은 행(PI 결정 또는 SPEC 결정이며 사실이 아니다), 제안 행(PI 결정 전), 차이 행(레퍼런스와 다르게 두었거나 어느 레퍼런스에도 없는 값을 골라 쓴 조건)을 따로 센다. A–G 필드의 차이 행은 C2.C8(제안 단계), R2.C7, R2.C9다(SPEC_FINAL 머리말의 규칙). 필드 I는 원출처와 다른 조건을 모은 요약이다.
- **아직 없는 항목** = `미`. 값을 채우지 않은 행이다. 값을 골라 실제로 쓴 행은 `미`가 아니다(초안이 `미`로 적었던 R2.C7, R2.C9, K2를 `선(차이)`로 고쳤다. 초안의 `미` 수 R2 8은 6이 되었다).
- 같은 사실이 여러 행으로 나뉘는 정도가 사례마다 달라서 행 수는 사례 간 비교용 수치가 아니다. 이 개정에서 행을 쪼갰으므로 초안의 행 수와 다르다.

### 1.2 사례별 대표 항목

`†`가 붙은 항목은 확정 열에 넣지 않고 따로 적는다(§1.1).

| 사례 | 확정된 항목(`†` 없는 `원`·`저`) | 원문 미재확인 항목(`†`) | 실험 설계로 제안·결정된 항목 | 아직 없는 항목 |
|---|---|---|---|---|
| **C1** | 보고 리비전 2a024d8의 단일 조회 코드(`검`), 수정 833fdbb의 제한 재시도(NAME_NOT_FOUND만, 25회·100 ms; `검`), 수정 전 부모 281a941(`검`), 공개 BP 소스의 이름·시작 순서·우선순위(`검`; 원 보고자 환경의 값이 아님), 기본 표의 빈 이름(`검`), BP PR CI의 CF 브랜치 이름과 PR #342 head 브랜치 이름의 일치(`검`; 그 CI가 833fdbb를 썼다는 것은 `분`) | 이슈 #184의 주장과 일자 | 공개 BP 기반 기준 구성과 CF 281a941→833fdbb 후보(D22), 생성 직전 사건 기반 보류와 사건 기준 해제(P-C1HOLD), 재시도 상수는 선택 버전에서 읽기, v7.0.1 빈 이름 구성은 경로 비활성 음성 대조, N은 SPEC S1(D14), 관측 영향 점검(D1) | 원 보고자의 provider·세마포어 이름·core 리비전·플랫폼·로그·빈도, 이슈 댓글 5개(→ 입력 로드맵 `roadmap_input_20261007.md`의 "BP를 먼저 시작해도" 서술은 미검증이며 공개 BP 스크립트는 CF 먼저이고 20261015 로드맵에는 그 서술이 없다), BP CI 구성요소 SHA(변동 참조), 공개 통합의 core 조합 빌드 가능성, D17·K8–K10의 D22 기준 적용성(C1.D5), 공개 BP 구성에서의 우선순위 효과(C1.C7은 추정), OS 실시간 우선순위 적용·CPU 수, 보류 감시 한도·hang 한계 |
| **C2** | 단일 OPERATIONAL 배리어 코드(6.4.2 tarball sha256 일치, v6.5.0a; `검`), 6.4.2와 v6.5.0a의 세 동기화 함수가 공백 무시 같음(`검`, 2026-10-16), 단일 단계 코드에 `WaitForSystemState`·`APPS_INIT`·`LATE_INIT` 없음(`검`, 2026-10-16), 수정 v6.6.0a의 APPS_INIT·LATE_INIT·2단계 ES 배리어(`검`), 6.6.0 VDD의 #167 행(`검`), 현재 dev의 같은 설계와 void 반환(`검`) | 이슈 본문의 8개 문장(조각 인용), 이슈 본문의 6.4.2 시작 동기화 서술(C2.B1) | 역할 재구성까지 진행(P-C2), 결과 명칭 "C2 메커니즘의 역할 기반 검증", 단일/두 단계 배리어 × 기존/수정 provider 2×2, Q22: 1차 대조는 late init이 정상 완료하는 구성, 사건 기반 보류, 재구성 플랫폼은 어느 레퍼런스에도 없는 값을 고르는 것이므로 차이 제안(C2.C8) | EVA CWS의 앱·함수·cFE 버전·플랫폼·증상·빈도 전부, "R의 호출이 P를 향했다"는 해석, R→P 요청 방식·초기화 내용·타임아웃 인자·우선순위·앱 수의 재구성 값, 단일 단계 셀에 쓸 cFE 코드(6.4.2 대 v6.5.0a)와 OSAL/PSP 짝, '단일 단계 × 수정 provider' 셀의 제외·이식 선택(C2.B9), 세 함수 밖의 동등성(C2.B2a), Trac #167 원 기록 |
| **C3** | 6.4.1 tarball에서 이슈의 호출 경로·생성 순서·우선순위 샘플 값·SB L198/L257·EVS L190/L385/L393·TIME L252/L308·EVS_SendEvent 무검사(`검`), 6.4.2의 F1·F2·F3(`검`), 두 tarball의 sha256·크기(`검`), 6.4.1 readme의 OSAL 요구·출처와 파생 후보 플랫폼의 값(SH-5, SH-9; `검`) | #73 = Trac #42의 헤더 증거와 수정 커밋 ID·병합일(검색 발췌 경유, X2 미결정), 이슈가 우선순위 값을 인용했다는 서술(C3.C3) | D13: 분석 전용 파싱 인정, (b)(i)부터, F1 전에 배치. Q16 버전별 대응 시험. 공유 6.4.x 기준. 실제 실행 검증은 별도 산출물. 현재 v7.0.1의 R2 기록을 C3 증거로 세지 않음 | Microblaze OS/PSP/OSAL/컴파일러, EVA가 실행한 cFE 빌드, fault 위치·로그·빈도, 댓글 14개 중 4개 미식별과 전문 전부, 실제 우선순위 적용·CPU 수·순서 제어 방식, OSAL artifact 통일(X3; readme가 말하는 것과 'git tag는 차이 조건'이라는 분류는 구분, SH-5b), 주 oracle(X7), C3와 R2를 한 패턴으로 세는지(X4) |
| **C5** | 6.4.1/6.4.0의 같은 `cfe_es_apps.c`와 락 해제 L610–재획득 L637(`검`), GetAppID 실패 조건(`검`), 6.4.2의 락 범위 확장 L571–L669(`검`), OSAL 4.1.1 SourceForge artifact와 git tag의 차이가 `osapi-version.h`뿐(`검`), 현재 코드의 가드와 잔여 창 코드(`검`) | 이슈 #72의 제목·milestone·주장(#72 = Trac #41 대응 자체는 `분`, C5.A8), 호출자 요구(C5.F2: 이슈 본문†, VDD, `0749d692` 주석) | D13·Q16, 사건 기반 보류(생성자를 OS_TaskCreate 반환 후 재획득 전에 보류), P-RGIT 보조 대조(원 C5의 대체 아님), 현재 버전 App-record 잔여는 별도 파생 결과(D16) | 관측자의 플랫폼·OSAL·앱·생성 경로·우선순위·루트/RT·CPU 수·로그, 댓글 9개와 [C5-context], `OS_TaskGetId` 순서(A8), stale TaskTable 항목 도달성, 빌드 호스트·권한 선택(X3), 보류 감시 한도 |
| **R2** | ENV 리비전 5개(`검`), 게시·거부·사용·직렬화 코드 줄(`검`), 보고 리비전 후보 B의 코드(`검`), gdb 3회의 원시 기록 재집계(각 부팅에서 거부된 이벤트 요청 24회, ES 20·EVS 4, 3 정적 사이트, 6 문맥; `검`), `b9bec567`의 전체 SHA·부모·변경 파일 수(`검`) | 이슈의 보고자 환경(Ubuntu 22.04.5, "cFE v6.7")과 재현 서술, 코멘트 2개, PR #2820과 그 head | ENV 기준 첫 전체 구현(P-LADDER), 자연 쌍 `b9bec567^`/`b9bec567` 1차·v7.0.0 보조·PR #2820 무변화 대조(P-R2CTRL), 정상 종료·정규화 부팅 기준(D17), `msg_max=10` 유지(D19), 관측 영향 점검(D1), 커널 변경 후 재측정(D11). gdb 15.1·all-stop·3회는 어느 레퍼런스에도 없는 값을 고른 것(차이; R2.C7, R2.C9) | B가 보고 시점 main이라는 확정, 보고자의 CPU·커밋·컴파일러·gdb·실행 횟수, 비-gdb 비수정 바이너리의 측정, 결정성의 증명(3회 반복으로 불가), 콘솔·카운터·시스템 호출의 관측(코드 읽기만 있다, R2.F8a), 보고의 "ES and EVS"가 B에서 성립하는지(Q1), 코멘트 원문의 누락 구간, 임무상 피해. **R2에는 시트 동결 전에 Kairos 첫 출력이 있다(상태 주의, §8.1 규칙 10)** |

## 2. 공통 기준

### 2.1 ENV 기준 (cFS v7.0.1 번들) — R2의 실행 기준이며 다른 네 사례에서는 일반 층 점검용

ENV는 `env/CONDITIONS.md`가 고정한 빌드·실행 환경이다. R2의 첫 전체 구현의 기준(P-LADDER)이다. C1(경로 비활성 음성 대조), C2·C3·C5(현재 버전의 대응 코드)에서는 해당 사례의 증거가 아니다(K16).

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| K1 | 번들·모듈 리비전 | cFS `088b2fa828db9ff7e00733f1908e0eeb59f66ce3`(tag v7.0.1), cFE `c5fb2b4d540bd55eb6c3707da7dd13eee679d4dd`, OSAL `d2d877a69cff47452bcca274b309147d48e6c16f`, PSP `c4b3b0b65b119e106481ad8e20976ae4d7f554e3`, CF `15a871e66cad7ec75e1be78989a93075ff142055` | 저·검 | `env/CONDITIONS.md` "Pinned revisions". 2026-10-15에 ENV 트리에서 읽기 전용 `git rev-parse HEAD`로 5개 일치(안전 디렉터리 검사는 명령행 옵션으로만 넘김). |
| K2 | 번들 리비전 선택(값을 고른 것) | v7.0.1 번들(K1)을 ENV가 골랐다: 2026-10-08 main HEAD가 v7.0.1이어서 쓴 것. README의 클론 명령은 브랜치를 지정하지 않는다(기본 브랜치 dev는 139 커밋 앞섬). 값이 실제로 쓰였으므로 `미`가 아니다. 어느 레퍼런스도 이 값을 정하지 않으므로 차이로 기록한다 | 선(차이) | `env/CONDITIONS.md` A2: 값은 지시문에서 왔고 레퍼런스가 아니다. "PI decision pending" 표시가 그대로 있다. 분류는 SPEC_FINAL 머리말의 규칙(사람이 고른 값은 `deviation`)에 따른다. |
| K3 | 빌드 | `make native_std.prep`, `make native_std.install`(옵션·-j 없음), CMake debug, `-O` 없음(GCC 기본 -O0), 단위 시험 포함, 경고를 오류로 취급(-Werror) | 저 | CONDITIONS C1–C5(cFS v7.0.1 README L102–L105 기반). R2 보고자의 빌드와 다르다(R2.I2). |
| K4 | 실행 | 사용자 `ubuntu`(uid 1000), 작업 디렉터리 `build-native_std/exe/cpu1`, 명령 `./core-cpu1`(옵션 없음), root 세션에서 `setsid setpriv`로 기동 | 저 | CONDITIONS E1–E3, E9. |
| K5 | 스케줄링 | `RLIMIT_RTPRIO` 0이어서 OSAL task 27개가 모두 `SCHED_OTHER` nice 0(12개 스냅샷에서 동일). 시작 스크립트의 우선순위는 스케줄링에 영향이 없다 | 저·M | CONDITIONS E4, M1-5. 측정 대상은 ENV 프로세스(시작 스크립트의 CF 우선순위 70, BP 없음; `env/ENV.md` L149)이며 C1의 공개 BP 구성(CF 65·BP 55)이 아니다(C1.C7). |
| K6 | 호스트 | Ubuntu 24.04.4 LTS, 4 vCPU, x86_64. 커널은 VM 세션마다 바뀌어 실행마다 기록한다: gdb 기준 실행 3회는 6.18.44-fc-v80(`env/logs/r2_g1_gdb_operational_esrestart.log.meta` L12), SPEC S1-R2 시험 실행 `s1r2_p1`은 -fc-v114(`env/logs/s1r2_p1_perf_operational_esrestart.log.meta` L13), 2026-10-15 이 대장 작성 때 `uname -r`는 -fc-v114, 2026-10-16에는 -fc-v139 | 저·M·검 | CONDITIONS B3; 두 meta 파일의 해당 줄과 2026-10-16 `uname -r`를 직접 읽었다(§9.3 V-R2-R6). SPEC rev 1.2는 2026-10-16 정합성 수정에서 커널을 실행마다 기록하도록 했다(SPEC §1.7 Env). |
| K7 | 커널 변경 후 재측정 | 커널이 바뀌었으므로 기준 실행을 SPEC S1의 관측 영향 비교보다 먼저 재측정한다. 예전 커널의 결과를 현재 기준 실행처럼 합산하지 않는다. 재측정은 SPEC S1 때 쓰는 커널에서 하고, 재측정과 비교 사이에 커널이 바뀌면 재측정을 되풀이한다 | 선(D11) | RESOLVED §2.2, §4.1. 'SPEC S1 때의 커널, 실행마다 기록'이라는 적용 방식은 `PI_DECISIONS.md` D11의 Resolution 줄과 SPEC §1.7 Env(2026-10-16 수정)의 것이다. |
| K8 | 메시지 큐 | `msg_max=10` 현 기준을 유지하고 큐 손실과 설정을 함께 기록한다. 손실이 목표 경로를 방해하면 민감도 구성을 별도로 설계하며 값을 조용히 바꾸지 않는다 | 선(D19) | RESOLVED §4.1. `env/CONDITIONS.md` E5의 "PI decision pending"은 이 결정으로 해소된 것으로 읽는다(그 파일은 갱신되지 않았다). |
| K8b | 큐 손실 측정 | 기동 중 큐 가득 참 손실 33, 37, 4, 49, 45, 41회/실행(M1 6회). 모두 ES 직렬화 S1(`cases/R2.md` §6; ENV M1-3의 'S1') 이전 | 저·M | CONDITIONS M1-3. |
| K9 | 정상·비정상 종료 후 정규화 | CI의 ES POWERON 재시작 명령(`cmd_send … --pktid=0x1806 --cmdcode=2 --half=0x0002`)을 기준으로 한다. 초기 상태와 종료 기록을 남긴다. POWERON이 모든 유지 파일·파일 캐시·공유 파일시스템을 초기화한다는 뜻으로 쓰지 않는다 | 선(D17) | RESOLVED §2; CONDITIONS E11(a). |
| K10 | 유지되는 상태 | `/dev/shm/osal:RAM` 아래 파일은 POWER ON 부팅 뒤에도 남는다. `EEPROM.DAT`와 keyfile도 남는다. M1 B3가 남긴 `cfe_sb_pipe.dat`(1624 B)가 이후 실행의 초기 상태를 바꾼다 | 저·M | CONDITIONS E7, M1-7, M1-8. |
| K11 | 관측 방식 | root 권한의 관측 프로세스가 cFS 프로세스를 관측하는 hook을 추가한다. cFS 실행 사용자와 기준 바이너리는 유지한다. cFS의 실제 UID·실행 명령·우선순위·스케줄링 정책·환경·초기 상태를 기록한다. 관측기와 실행 제어(보류·정지·해제)를 구분하고 후자를 쓴 실행에는 표시한다. 기록 손실·사건 대응 실패·부팅/진행 시간·외부에서 확인 가능한 기능 결과를 기준 실행과 비교한다. 기준 실행과 비교할 수 없는 내부 사건은 영향이 없다고 결론 내리지 않는다 | 선(D1) | RESOLVED §2, §2.2. '시간만 바뀐다'는 설명은 쓰지 않는다. |
| K12 | 실행 횟수 N의 절차 | SPEC S1에서 목적별(동작 확인, 관측 영향 비교, 발생 빈도 평가)로 파일럿 결과와 자원 예산을 근거로 정하고 해당 본실험 전에 동결한다 | 선(D14) | RESOLVED §2. |
| K13 | 실행 횟수 N의 값 | 지금 없다. 기존 gdb 3회, M1 6회 등은 특성 측정·기록이지 실험 N이 아니다 | 미 | 이 문서는 값을 정하지 않는다. |
| K14 | 격리 | `run_cfs.sh`는 다른 `core-cpu*`·OSAL 자원·UDP 포트·SysV 세그먼트가 보이면 시작을 거부한다(최선 노력, 청소하지 않음) | 저 | CONDITIONS E13, H-5. |
| K15 | ENV의 환경 이탈 목록 | (1) 클론을 root가 만들고 `chown -R ubuntu`(D-1). (2) glibc가 2.39-0ubuntu8.9로 올라감(gcc-multilib 설치의 부작용, B5). (3) `/proc/schedstat` 없음 → HS 시스템 모니터 초기화 실패, HS의 시작 동기화가 다름(D-3). (4) README L106 단위 시험이 IPv6 부재로 실패(68 통과, 129 미실행; C12). (5) `lcov` 없음 → README L107 미실행(D-2) | 저·M | CONDITIONS §F. 모두 레퍼런스와 다른 조건이며 ENV 기반 모든 실행에 해당한다. |
| K16 | ENV의 사례별 의미 | v7.0.1 번들의 C1·C2·C3·C5 대응 코드는 일반 층만 시험하며 해당 사례의 대상이 아니다. 다른 네 사례를 현재 버전의 대응 코드 분석으로 대체하지 않는다 | 선(RESOLVED §1-2; SPEC ADR-044) | RESOLVED §1; SPEC §9.3 "per-case paths". |

### 2.2 공유 6.4.x 기준 (C3·C5)

C3와 C5는 같은 6.4.1→6.4.2 릴리스를 쓰므로 소스 기준과 파생 플랫폼 논의를 한 곳에 둔다. 두 사례의 수정 이력은 하나(Trac #40–#42)이며 독립 표본이 아니다. 이 절의 행 ID는 `SH-n`이다(§0.5).

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| SH-1 | 버그 쪽 소스 | `cFE-6.4.1-OSS-release.tar.gz`, 39,586,940 B, sha256 `ec26e33b4ab40b9616ccdbc984c5a00a790a7d05b260f2628c3335a29c23e98f` | 원·검 | SourceForge. 2026-10-15 재다운로드로 크기·sha256 일치. |
| SH-2 | 수정 쪽 소스 | `cFE-6.4.2-OSS-release.tar.gz`, 36,411,531 B, sha256 `c64ed2aac910e79a641f49279dfdff3b7b49374e54ed56b595d47e16a3f53a4f` | 원·검 | 같은 방법으로 일치. C2의 6.4.2 행과 같은 artifact. |
| SH-3 | git에서의 위치 | 두 릴리스 모두 nasa/cFE git에 없다. 수정이 들어 있는 첫 git 리비전은 import 커밋 `792f5e35`이고 tag `v6.5.0a`(`b2765d9f`)의 `cfe/` 트리와 같다 | 저 | C3.md §3; C5.md F10. 이 문서에서는 `b2765d9f`의 파일 내용만 다시 읽었다. |
| SH-4 | 세 수정이 한 diff에 | 6.4.1→6.4.2 diff는 Trac #40(C3의 F3), #41(C5), #42(C3의 F1·F2)를 함께 담는다. 한 수정만 분리하려면 diff의 구성된 하위집합이 필요하고 그것은 차이 조건이다 | 분 | ledger §3.3; C3.md A8. |
| SH-5 | OSAL 요구와 출처(6.4.1 readme) | `cfe-OSS-readme.txt`(SH-1 tarball 안): 요구는 "Operating System Abstraction Layer 4.1.1 or higher"(L76). 얻을 곳은 "http://sourceforge.net/projects/osal or github.com/nasa/osal" 두 곳이다(L77–L78; L14–L16도 같은 두 곳). 풀기 절차는 "( obtain from sources above )"(L137) 뒤에 파일 이름을 "( example: /home/acudmore/Projects/osal-4.1.1-release.tar.gz)"(L139)로 예시로만 적고, 이어서 `tar -zxf … osal-4.1.1-release.tar.gz`·`mv osal-4.1.1-release osal`(L141–L142)을 보인다 | 원·검 | 이 문서 작성 중 tarball의 sha256 일치를 확인한 뒤 readme를 다시 읽었다(§9.2 V-C3-10). |
| SH-5a | 릴리스 구성의 OSAL 버전 | 6.4.2 VDD가 릴리스 구성의 OSAL을 4.1.1로 적는다(Table 1.4-1) | 저 | C3.md §10 #25; C5.md F13(VDD는 미재확인). |
| SH-5b | readme의 OSAL 서술에 대한 해석 | readme가 SourceForge artifact를 '지명'했고 그래서 git tag로 빌드하면 차이 조건이라는 초안의 분류는 해석이다. readme가 말하는 것은 허용된 출처 두 곳, 예시로 든 파일 이름, 'or higher'뿐이다. 어느 artifact를 쓸지와 git tag를 차이 조건으로 셀지는 X3(미결정)에 맡긴다 | 분 | SH-5, SH-6, SH-7. C5 기록(C5.md)은 이 분류를 확정으로 적었다(SH-8). |
| SH-6 | OSAL SourceForge artifact | `osal-4.1.1-release.tar.gz`, 1,973,805 B, sha256 `8f0f2b23d8226256fdb8543bf9fcb560b38434411dc12cb4f7f35b9c19e61d46`; `src/os/inc/osapi-version.h`의 `OS_REVISION`이 (1) | 원·검 | 2026-10-15 재다운로드·sha256 일치. |
| SH-7 | OSAL git tag | `osal-4.1.1`(`2f1fdb0c89523ba865dda65bbf599671a7a79fa3`)의 `osapi-version.h`는 `OS_REVISION (0)`(4.1.0으로 보고). `src/os/posix/osapi.c`는 SourceForge 파일과 동일. git tag를 쓰면 보고되는 OSAL 버전 문자열이 4.1.0이 된다(소스 파일 하나의 차이) | 원·검 | 두 파일을 직접 비교(posix `osapi.c` 동일, `osapi-version.h`만 다름). 이것을 차이 조건으로 셀지는 해석이다(SH-5b). |
| SH-8 | 사례 기록 간 OSAL 불일치 | C3 기록은 OSAL을 git tag로 쓰고 artifact 동일성을 미검증으로 적었다. C5 기록은 SourceForge artifact와의 차이를 확정으로 적었다. 두 기록이 서로 다르다 | 저 | ledger §3.2, §5 X3. 통일 방법은 미결정. |
| SH-9 | 6.4.1 릴리스 문서·PSP·make 파일이 적은 값 | (a) 빌드 환경: "built on CentOS/RHEL 6.x x86 ( 32 bit )"(readme L83); 다른 최근 리눅스에서도 빌드할 수 있을 것이고 64비트 시스템에서 32비트 응용으로 빌드·실행할 수 있어야 한다(L85–L87); "This release was compiled on CentOS 6.5-x86, CentOS-7.0-x64, Ubuntu 12.04 LTS x86, Ubuntu 14.04 LTS x64"(L88–L89). (b) 대상: "out of the box" 대상은 32 bit x86 Linux(CentOS 6.x)(L98–L100); `cpu1`은 pc-linux 대상으로 설정됨(L153; `build/cpu1/cfe/cfe-config.mak` L32 `PSP = pc-linux`); OSAL 4.1.1 artifact의 `build/osal-config.mak` L21 `OS = posix`. (c) 컴파일: PSP pc-linux `make/compiler-opts.mak` L33 `ARCH_OPTS = -m32`; `build/cpu1/cfe/debug-opts.mak` L4 `DEBUG_OPTS = -g -O0`. (d) 실행: 인수 없이 실행하면 POWER ON reset(`psp/fsw/pc-linux/src/cfe_psp_start.c` L244–L252의 기본 가지); POSIX 메시지 큐는 `msg_max`를 올리거나 root로 실행해야 한다(readme L168–L174); 실시간 스케줄러는 root/sudo가 필요하다(L176–L177) | 원·검 | 이 문서 작성 중 직접 확인(§9.2 V-C3-10). 원 보고자(Microblaze)의 관측 조건이 아니라 릴리스 문서의 값이다. 초안은 이 행을 `선(D13)`으로 적었으나 값 자체는 원출처 사실이다. C3 기록 C3.md §10 #21·#27–#29, C5 기록 C5.md §10 #6·#10·#18·#20·#21과 같은 내용이다. |
| SH-9a | D13이 정한 것과 정하지 않은 것 | 정한 것: 분석 전용 파싱 인정, 현재 컨테이너의 32-bit 헤더로 하는 첫 파싱((b)(i)), F1 전 배치((c)). 정하지 않은 것: SH-9의 값(빌드 호스트 목록 포함)을 파생 플랫폼의 조건으로 채택하는 것. 문헌상 host의 sysroot((b)(ii))는 헤더 민감성이 확인될 때의 전환 대상이지 채택된 것이 아니다 | 선(D13) | RESOLVED §3.4. SH-10, SH-12와 겹친다. |
| SH-10 | 현재 컨테이너의 첫 파싱 | 현재 컨테이너(Ubuntu 24.04, 32-bit 헤더)로 하는 첫 파싱은 **분석 전용이며 차이가 있는 조건**이다. 원 플랫폼 실행 재현으로 인정하지 않는다 | 선(D13 (a), (b)(i)) | RESOLVED §3.4. |
| SH-11 | 확인 항목 | 생성 설정, include 목록, 정수·포인터 크기, 매크로가 선택한 소스 경로, 구조체·반환값 해석 | 선(D13) | RESOLVED §3.4. |
| SH-12 | 민감성 전환 | 관련 경로·자료형 해석이 헤더·환경에 민감하면 문헌상 host의 sysroot를 확보하는 (ii)로 전환한다. 민감성이 해소되기 전에는 영향받는 결과를 보류한다 | 선(D13 (b)(ii)) | RESOLVED §3.4. |
| SH-13 | 버전별 대응 | 버전별 상태·반환값·소스 위치 대응을 명시하고 시험한다. 입력·대응 실패를 탐지 기법의 실패로 세지 않는다 | 선(Q16) | RESOLVED §3.4. |
| SH-14 | 배치와 보고 | 6.4.x 기준은 F1 전에 배치한다. 과거 코드의 실제 실행 검증은 분석 전용 작업과 별도 산출물로 보고한다 | 선(D13 (c)) | RESOLVED §3.4. |
| SH-15 | 실행 제어 | 추가 링크 변경을 이용한 실행 제어와 PCT는 후순위(현재 사건 기반 보류로 필요한 확인이 되는지 먼저 평가). 초기에는 모델 기반 단일 코어 실시간 해석이며 실제 우선순위 실행은 필요성 확인 뒤 별도 구성·조건으로 한다. 모델 결과를 실제 실시간 실행 결과로 표기하지 않는다 | 선(D3, D4) | RESOLVED §4.2. |
| SH-16 | 아직 열린 선택 | 빌드 호스트(SH-9 목록 안에서 고르는가, 이탈하는가), 바이너리 폭(-m32 유지), 권한(root 대 `msg_max` 상향), C3의 순서 제어(우선순위 구동 대 사건 기반 보류), OSAL artifact 통일, 6.x 코드에서 D17 명령의 적용성 | 미 | ledger §5 X3; C3.md D3; C5.md D3a·D3b. |
| SH-17 | CPU 수·affinity·선점 모델 | 어느 사례의 레퍼런스에도 없다 | 미 | ledger §4.1 U-CPU. |

## 3. C1 — nasa/CF #184 (throttle 세마포어 생성 전 조회)

### 3.A 사례 정체성

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C1.A1 | 원 이슈 | nasa/CF #184 "Possible race condition with creation of throttle sem (needs to exist before CF initializes)". 작성·담당 jphickey. 2022-01-13T23:06:06Z 생성, 2022-12-01T19:35:24Z 닫힘(completed). milestone v7.0.0. 댓글 5개 | 원† | C1.md F1(GitHub MCP 검색 본문). 이 세션에서 페이지·API 403. |
| C1.A2 | 원 보고의 주장 | (i) CF는 throttle 세마포어가 CF 시작 전에 생성돼 있다고 기대한다. 생성자가 다른 앱(CI/TO 또는 전용 I/O 앱)이면 `CF_CFDP_InitEngine()` 중 bind 시점 전에 생성됐다는 보장이 없고, bind가 실패하면 CF가 abort한다. (ii) 보조 문제: 세마포어를 소유한 앱이 재시작·재로드되면 세마포어 ID가 바뀔 가능성이 있다 | 원† | C1.md F1. |
| C1.A3 | 원 보고의 재현 서술 | "Its a race condition, so not readily reproducible." CF를 세마포어 생성 앱보다 먼저 시작하고, 생성 앱의 시작에 인위적 지연을 넣으면 질 가능성이 커진다(지연의 양·방식은 없음) | 원† | C1.md F1. |
| C1.A4 | 공개 코드와 연결: 보고 리비전의 조회 | CF `2a024d8` `fsw/src/cf_cfdp.c` L1012–L1023: 비어있지 않은 `sem_name`마다 `OS_CountSemGetIdByName` 한 번, 실패하면 이벤트 `CF_EID_ERR_INIT_SEM`("…error=0x%08x") 후 `goto err_out` | 원·검 | raw.githubusercontent.com nasa/CF@2a024d8. |
| C1.A5 | 공개 코드와 연결: 수정 | CF `833fdbb`(부모 `281a941`; 저자 시각 2022-11-17 16:37:36 -0500; PR #342, 병합 커밋 `2ffbecc` 2022-12-01): `OS_ERR_NAME_NOT_FOUND`일 때만 `OS_TaskDelay(CF_STARTUP_SEM_TASK_DELAY)` 후 재시도, 최대 `CF_STARTUP_SEM_MAX_RETRIES`. 코드 주석이 NAME_NOT_FOUND를 시작 경쟁으로 가정한다고 적는다 | 원·검 | 833fdbb `cf_cfdp.c` L1000–L1011 직접 확인; 부모·시각은 얕은 git fetch. 병합 커밋 `2ffbecc`의 제목·시각·부모는 이 개정에서 확인(V-C1-10). PR 본문은 저(미재확인). |
| C1.A6 | 공개 BP 소스의 값 | nasa/bp `b051eb0`: 시작 스크립트 L6 `cf … 65`, L7 `bp … 55`(CF가 BP보다 먼저, 우선순위 CF 65·BP 55); tx/rx CF 표 L40 `"BP0_tsem"`, L54 `"BP1_tsem"`; `fsw/custom/bp_semcfg.c` 기본값 `BP0_tsem`/`BP1_tsem` | 원·검 | **공개 BP 소스의 사실이다. 원 보고자 환경의 값이라는 뜻이 아니다**(C1.A8). |
| C1.A7 | PI의 정정 | "이름·순서·우선순위에 레퍼런스가 없다"는 설명은 정정한다. 공개 구성의 값에는 출처가 있고, 원 보고자 환경의 값·다른 버전으로 옮길 때의 값은 구분한다 | 선(D22) | RESOLVED §3.2, §5. |
| C1.A8 | 연결되지 않는 부분 | provider 정체("CI/TO or some other dedicated I/O app"), 원 보고자의 세마포어 이름·채널 수, cFE·OSAL·PSP 리비전, 빌드 구성, 실패 출력, 빈도. 공개 BP 구성이 원 보고자의 provider와 같다는 근거도 없다 | 미 | C1.md §10 #5·#10·#11·#15·#25·#29·#30; RESOLVED §3.2. |
| C1.A9 | 이슈 댓글 5개 | `issuecomment-1202543669`([C1-BP]) 포함 미열람. 입력 로드맵 `roadmap_input_20261007.md`(L86)가 "BP를 먼저 시작해도 객체 생성 전에 CF가 조회"한다는 서술의 근거를 이 댓글([C1], [C1-BP])로 표시한다. 그 서술은 `plan/ROADMAP_20261015.md`에는 없다(grep 확인). 공개 BP 시작 스크립트는 CF를 BP보다 먼저 시작하므로(C1.A6) 그 서술은 공개 소스로 뒷받침되지 않는다 | 미 | C1.md §0. 이 문서는 서술의 옳고 그름을 판정하지 않는다. 미검증으로 둔다. |
| C1.A10 | 'race' 서술의 처리 | 이슈 제목·본문의 "race"를 도구 class로 자동 변환하지 않는다. 일치 여부는 별도 보고하고 정확도 분모를 만들지 않는다 | 선(P-CLASSGT) | RESOLVED §4.3. |
| C1.A11 | 성과 수에서의 위치 | RESERVED 구간 조회·lock 잔류(OSAL 구현 결함 후보)는 추가 검증이며 원 C1 검증의 착수 조건이나 다섯 사례 성과 수에 넣지 않는다 | 선(P-C1HOLD, QT-9) | RESOLVED §3.3, §4.1. |

### 3.B 소스 기준

C1의 공개 구성과 버전 호환성에 대한 자세한 정리는 ROADMAP §12 작업 4의 별도 문서 `plan/C1_basis_plan.md`가 맡는다. 이 대장의 C1 행은 그 문서의 결과를 인용하지 않고 사례 기록과 1차 출처에서 직접 만들었다.

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C1.B1 | 보고된 CF 리비전 | `2a024d8efd9f44f54b7a59f7face24f255536236` | 원† | 이슈 "Code snips" 링크. 이 리비전의 코드는 C1.A4에서 확인했다. |
| C1.B2 | 첫 수정 전후 비교 후보 | CF `281a941`(수정 833fdbb의 부모, 단일 조회) → `833fdbb` | 선(D22) | RESOLVED §3.2. 부모 관계는 C1.A5에서 확인(`검`). 같은 시기 main `32a8b03`은 README만 다르다는 기록은 저(C1.md §3). |
| C1.B3 | 혼합 금지 | 원 보고의 `2a024d8`과 공개 BP 표를 그대로 섞지 않는다. 공개 BP 표(채널별 타이머)는 `281a941` 레이아웃과 같고 `2a024d8`(전역 타이머, 앱 이름 `CF_APP` 제약)과 다르다 | 선(D22) | RESOLVED §3.2. 근거: C1.md §10 #38·#39(저). |
| C1.B4 | 공개 통합의 소스 | nasa/bp `b051eb0d12bfe76c0230215c2ff851cfaa553ce4`(작성 2022-11-15, 커밋 2022-11-18, 병합 `e871ad9` 2022-11-30): 시작 스크립트, tx/rx CF 표, `bp_semcfg.c`, CI 워크플로 | 원·검 | 파일 내용 직접 확인. 날짜는 저(C1.md F9). |
| C1.B5 | 공개 provider 코드 | `BP_AppMain` → `AppInit`(`bp_app.c` L203 `BP_FlowInit`) → `BP_FlowInit`(`bp_flow.c` L519, L524 `initialize_throttling()`) → `initialize_throttling`(L121; `OS_CountSemCreate` L132). 첫 `CFE_ES_RunLoop`(`bp_app.c` L818) 이전 | 원·검 | b051eb0 파일 직접 확인. |
| C1.B6 | PR CI의 CF 소스 | `jphickey/cf@fix-184-throttle-sem-race`("For now this needs to run with a patch to CF"). 이 브랜치 이름은 PR #342의 head 브랜치 이름과 같다: nasa/CF 병합 커밋 `2ffbecc`(2022-12-01)의 제목은 "Merge pull request #342 from jphickey/fix-184-throttle-sem-race"이고, 수정 `833fdbb`의 메시지는 "Fix #184, work around throttle sem creation race"다 | 원·검 | `build-run-bp-pullreq.yml`(nasa/bp@b051eb0) L6·L13–L14. nasa/CF 이력은 blobless git fetch로 확인(§9.2 V-C1-6, V-C1-10). 워크플로 파일 안에 833fdbb를 지명하는 곳은 없다. |
| C1.B6a | PR CI가 쓴 패치가 833fdbb라는 추정 | BP PR CI에 필요했던 패치가 이 경쟁 수정(833fdbb)이라는 추정은 위 브랜치 이름 일치로 뒷받침된다(근거이지 증명이 아니다). 병합 시점의 head는 833fdbb다(병합 커밋의 두 번째 부모). CI가 돌던 시점(2022-11-18 이후)에 그 브랜치가 어느 커밋을 가리켰는지는 기록되어 있지 않다 | 분 | C1.B6의 사실에서 도출. D22의 기준 선택(CF 281a941→833fdbb)은 이 추정과 별개로 이미 결정이다. |
| C1.B7 | cFS·cFE·OSAL·PSP 전체 조합 | 확정된 조합이 없다. 후보: `2a024d8` 경로는 caelum-rc4(cFE `98f78e8`, OSAL `42af0f7`, PSP `1bd7bd9`; CF README "cFS Caelum"에서 유도), BP 경로는 BP CI 창 [2022-11-18, 2022-11-30]의 cFS `1df262a` 또는 `1b0b338` | 분 | C1.md §3, §11 A1. 후보이며 결정이 아니다. |
| C1.B8 | 공개 통합의 변동 참조 | BP CI는 cFS main, bplib main, tinycbor v0.6.0, 그리고 `jphickey/PSP@techdev-iodriver` 병합을 쓴다. 2022년의 SHA는 기록되지 않았다(PSP 브랜치는 2026-10-08에 `7c29c50`) | 미 | C1.md §3, §10 #7·#8·#32. |
| C1.B9 | 공개 통합이 해당 core 조합에서 빌드되는가 | 확인한 기록이 없다. 이 문서도 빌드하지 않았다 | 미 | RESOLVED §3.2 적용 순서: 호환성 확인이 먼저. |
| C1.B10 | 다른 버전으로 옮길 때 | 그대로 구성할 수 없으면 옮긴 구성·코드 차이를 기록한다(공개 코드 기반 파생 실험 표시) | 선(D22) | RESOLVED §3.2. |
| C1.B11 | 음성 대조 | ENV의 CF `15a871e`(v7.0.1): `sem_name[0]` 검사 L1270, 재시도 루프 L1279–L1290, 기본 표 이름 `""`(L60·L78). 따라서 경로 비활성 | 원·검 | `cf_cfdp.c`·`cf_def_config.c` @15a871e 직접 확인. |
| C1.B12 | 음성 대조의 용도 | 현재 v7.0.1 기본 구성의 빈 이름은 경로 비활성 음성 대조로만 쓴다 | 선(D22) | RESOLVED §3.2. 이것은 C1 증거가 아니다(K16). |

### 3.C 실행 조건

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C1.C1 | 원 보고자의 OS | Ubuntu 21.10(이슈 "System observed on"). 수정의 시험 환경은 Ubuntu 22.04(PR #342 본문) | 원† | C1.md F1, F6. |
| C1.C2 | 원 보고자의 빌드 구성·core 리비전·시작 순서 실제·우선순위 | 이슈는 "CF를 provider보다 먼저 시작"하라는 조언만 한다. 실제 값은 정해져 있지 않다 | 미 | C1.md §10 #5·#10·#18·#21. |
| C1.C3 | 공개 BP CI의 환경(참고 값) | 러너 `ubuntu-22.04`; `cmake -DCMAKE_BUILD_TYPE=Release -DSIMULATION=native -DCMAKE_INSTALL_PREFIX=/exe -DMISSIONCONFIG=bpxfer -DMISSION_DEFS=$PWD/bpxfer_defs -B build -S cfs-bundle/cfe` | 원·검 | `build-run-bp-common.yml` L51, L127. 공개 CI의 값이며 원 보고자의 것이 아니다. |
| C1.C4 | 공개 BP CI의 나머지 | 대상 `tx`/`rx`(PROCESSORID 1/2), 앱 `cf bp bplib ci_lab to_lab sch_lab`, PSP 모듈 `iodriver unsock_intf`, 실행 `./core-tx`, 5초 뒤 OPERATIONAL 문구 확인 | 저 | C1.md §10 #31·#33–#37(미재확인). |
| C1.C5 | C1 기준 구성으로의 채택 | 공개 BP 기반(이름·시작 순서·설정 우선순위 포함)을 C1의 기준 구성으로 둔다. 공개 값은 원 보고자 환경의 값으로 인용하지 않는다 | 선(D22) | RESOLVED §3.2. |
| C1.C6 | OS의 실시간 우선순위 적용·CPU 수·affinity | 원 보고자, 공개 CI 모두 정해진 바가 없다. 네이티브 샘플 설정은 PERMISSIVE 모드다 | 미 | C1.md §10 #22·#23. |
| C1.C7 | 공개 BP 구성에서의 우선순위 효과 | 공개 BP 시작 스크립트의 CF 65·BP 55가 일반 사용자 실행에서 스케줄링에 영향이 없을 것이라는 것은 추정이다. ENV의 측정(K5: 27 task 모두 `SCHED_OTHER`)은 ENV 프로세스(시작 스크립트의 CF 우선순위 70, BP 없음)에 대한 것이고 BP는 ENV에 없으므로, 이 구성에서 측정된 바가 없다. 기대 근거는 ENV의 OSAL이 elevated priority를 허용받지 못하면 기본 우선순위로 만든다는 동작(K5의 근거)이 2022년 시기의 OSAL에도 있다는 가정이며, 그 가정은 확인하지 않았다. 기준을 v7.0.1로 옮기는 경우(C1.I2(7)의 차이 조건)에만 K5를 그대로 적용할 수 있다 | 분 | K5; `env/ENV.md` L149. 실제 우선순위 실행은 D4에 따라 필요성 확인 뒤 별도 구성이다. |
| C1.C8 | provider 지연 | 이슈는 "artificial delay"라고만 하고 양·방식이 없다. 고정 sleep으로 성공률을 조절하지 않는다. provider를 **생성 직전**에 사건 기반으로 보류하고, CF의 재시도 종료 또는 최종 초기화 결과라는 사건으로 해제한다 | 선(P-C1HOLD) | RESOLVED §3.3; CONDITIONS_POLICY(사건 기반 제어). |
| C1.C9 | 재시도 수·지연 읽기 규칙 | 선택한 CF 버전의 생성 설정·소스에서 실제 재시도 수와 지연을 읽는다. 25 또는 100 ms를 모든 버전에 고정하지 않는다 | 선(P-C1HOLD) | RESOLVED §3.3. |
| C1.C10 | 833fdbb의 상수 | `CF_STARTUP_SEM_MAX_RETRIES 25`(L338), `CF_STARTUP_SEM_TASK_DELAY 100`(ms, L349), `fsw/platform_inc/cf_platform_cfg.h`. 281a941은 단일 조회다 | 원·검 | 직접 확인. 15a871e는 같은 루프이며 상수 파일은 저(`cf_internal_cfg.h`, 미재확인). |
| C1.C11 | 보류의 감시 한도·중단 조건 | 재시도 수×지연을 벽시계상의 안전한 대기 상한으로 보지 않는다. 관측 영향과 실제 종료 시간을 파일럿에서 확인하고 본실험 전에 감시 한도·중단 조건을 기록한다. 보류로 만든 실패는 자연 실행과 구분하며 timeout 관측만으로 영구 교착이라고 판정하지 않는다 | 선(P-C1HOLD) | RESOLVED §3.3. |
| C1.C12 | 감시 한도·중단 조건의 값 | 지금 없다 | 미 | 파일럿 뒤에 기록한다(RESOLVED §3.3). |
| C1.C13 | 제안된 `WaitForStartupSync` 대조 | 이슈가 제안한 CF의 `CFE_ES_WaitForStartupSync()` 호출을 파생 대조로 넣는다면 `TimeOutMilliseconds` 값이 필요한데 레퍼런스에 값이 없다 | 미 | C1.md §6, A10. |
| C1.C14 | 관측 방식·N | K11(D1), K12(D14) | 선(D1, D14) | §2.1. |

### 3.D 초기 상태

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C1.D1 | CF 설정 표의 기본값 | 두 채널의 `sem_name`이 `""`다. `2a024d8` L42·L51, `281a941` L59·L78, `15a871e` L60·L78 | 원·검 | 직접 확인. 기본 구성에서는 조회가 실행되지 않는다. |
| C1.D2 | 파생 구성의 이름 | 활성 경로에는 비어있지 않은 이름이 필요하다. 공개 BP 값 `BP0_tsem`/`BP1_tsem`을 쓰는 것은 기준 구성 채택(C1.C5)의 일부다 | 선(D22) | 값은 C1.A6. |
| C1.D3 | provider 세마포어의 초기 카운트 | `OS_CountSemCreate(&Sem, Name, Max, 0)`, `Max = BP_APP_READ_LIMIT`(기본 10) | 저 | C1.md §10 #13(`bp_cfg.h` L86–L87; 미재확인). |
| C1.D4 | 원 보고자의 reset 종류·큐·유지 상태 | 정해진 바가 없다 | 미 | C1.md §10. |
| C1.D5 | 파생 실행의 정상·비정상 종료 후 기준, 큐, 유지 상태 | K8(D19), K9(D17), K10은 v7.0.1 ENV에서 정해졌거나 측정된 것이다. D22 기준(공개 BP 통합, CF 281a941→833fdbb, 2022년 시기의 cFS·cFE·OSAL·PSP)에 그대로 적용되는지는 **확인되지 않았다**. D17의 명령(CI_LAB을 통한 ES 재시작, pktid 0x1806)은 v7.0.1 번들 CI에서 온 것이고, 그 기준 구성의 core 조합에서 같은 명령이 동작하는지 확인한 바가 없다. 기준을 v7.0.1로 옮기는 경우(C1.I2(7)의 차이 조건)에만 적용된다. 적용성 확인은 호환성 확인(D22 적용 순서 1) 단계에서 하는 것을 제안한다 | 미 | §2.1; RESOLVED §2, §4.1. C2·C3·C5의 같은 문제는 SH-16, C2.D2. |
| C1.D6 | provider의 시작 상태 | 공개 BP 스크립트 순서상 CF 다음에 BP가 시작한다. 보류 실험에서는 provider를 `OS_CountSemCreate` 호출 직전(OSAL 내부가 아니라 앱 쪽)에 둔다 | 선(P-C1HOLD) | RESERVED 구간(OSAL 내부) 보류는 후속 과제다. |

### 3.E 의존성

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C1.E1 | 자원 | OSAL counting semaphore. CF 표 `chan[i].sem_name`의 이름으로 찾고 핸들은 `CF_AppData.engine.channels[i].sem_id`에 저장 | 원·검 | C1.A4의 코드. |
| C1.E2 | 게시 지점(공개 provider) | `OS_CountSemCreate`(`bp_flow.c` L132) ← `initialize_throttling` ← `BP_FlowInit` ← `AppInit` ← `BP_AppMain`. 첫 `RunLoop` 전 | 원·검 | C1.B5. |
| C1.E3 | OSAL 수준에서 이름 조회가 성공하는 시점 | `OS_ObjectIdFinalizeNew`가 레코드의 `active_id`를 최종 값으로 쓴 뒤라는 코드 읽기 추론(OSAL v6.0.0-rc4 등) | 분 | C1.md §4. 실행으로 확인한 바 없음. |
| C1.E4 | 사용 지점(CF) | `CF_AppMain` → `CF_Init`(`cf_app.c` L252 `status = CF_CFDP_InitEngine()` @281a941) → `OS_CountSemGetIdByName`. 재바인드: `CF_CmdEnableEngine`. 런타임 사용: `CF_CFDP_MsgOutGet` | 원·검 | 호출 줄은 직접 확인. 재바인드·런타임 사용은 저(C1.md §5). |
| C1.E5 | 요구되는 순서(원출처) | 비어있지 않은 이름의 각 채널에 대해, provider의 세마포어 생성이 CF의 조회 이전에 완료돼 있어야 한다("the CF app expects that semaphore to be created _before_ it starts") | 원† | C1.md F1, F2, F4. |
| C1.E6 | 수정 후 요구의 완화 | 수정 후에는 최대 R회(R은 선택한 버전의 재시도 상수) 조회 중 마지막 이전에 생성이 완료되면 된다 | 분 | 833fdbb 코드(`검`)에서 도출. |
| C1.E7 | 기존 동기화 | CF와 BP 모두 `CFE_ES_WaitForStartupSync`/`WaitForSystemState`를 부르지 않는다(`git grep`) | 저 | C1.md §6(미재확인). |
| C1.E8 | 기존 동기화의 평가 | ES 시작 스크립트의 순서는 task가 **시작**하는 시점만 정하고 각 task가 조회·생성에 도달하는 시점은 정하지 않는다. 우선순위는 적용이 보장되지 않는다(permissive 모드, 비실시간). OSAL 객체 테이블 락은 각 호출을 원자적으로 만들 뿐 순서를 정하지 않는다 | 분 | C1.md §6. |
| C1.E9 | 수정의 종류 | 제한 재시도. 순서 보장·가드·동기화가 아니다. `OS_ERR_NAME_NOT_FOUND` 외의 상태는 즉시 중단하고 init 실패. provider가 재시도 창보다 늦으면 실패 | 분 | 분류는 분석, 근거 코드는 `검`(C1.A5). |
| C1.E10 | RESERVED 구간 가설 | 생성 중간 상태(`active_id = RESERVED`)에서 이름이 보이면 조회가 `OS_ERR_INCORRECT_OBJ_STATE`를 반환하고 count-sem 테이블 락이 풀리지 않을 수 있다는 코드 읽기 가설. 5개 OSAL 리비전의 같은 구조. 미실행 | 분 | C1.md §4. 구현 결함 후보이며 C1.A11에 따라 성과 수에 넣지 않는다. |
| C1.E11 | 보조 문제(provider 재시작) | 이슈 서술. 수정은 초기화 경로만 건드린다(분). 이번 범위에 넣는지, 재시작 trigger·시점은 정해져 있지 않다 | 미 | C1.md A9. |
| C1.E12 | 다른 사례와의 관계 | C1·C5·R2는 "reserve, then finalize" 형태를 공유하지만 층이 다른 가설일 뿐이다. 독립 표본이 아니다 | 분 | ledger §3.6; SPEC §8.2. |
| C1.E13 | C2와의 관계 | 이슈가 제안한 조치(CF의 `WaitForStartupSync`)는 C2가 기록한 한계(void 반환, 소프트 타임아웃, #1466/#1467 open)를 물려받는다 | 분 | ledger §3.5. |

### 3.F 정상·오류 판정 기준

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C1.F1 | 층1: API 문서·실제 동작 | `OS_CountSemGetIdByName`은 이름이 없으면 `OS_ERR_NAME_NOT_FOUND`를 반환한다(OSAL v6.0.0-rc4 `osapi-countsem.h`의 `@retval`). 문서화된 `@retval` 목록에 `OS_ERR_INCORRECT_OBJ_STATE`는 없다 | 저 | C1.md F7(미재확인). 분석 대상 OSAL 리비전과의 동일성은 기준 구성 확정 뒤 다시 확인한다. |
| C1.F2 | 층2: 호출자 요구(준비 상태 전제) | 세마포어가 CF 시작 전에 생성돼 있어야 한다. "If a semaphore is expected and not found, the application will terminate during initialization"(CF 문서 @2a024d8). 현재 문서의 "not-in-use 폴백" 서술은 코드와 모순이어서 판정 근거로 쓰지 않는다 | 원† | 이슈 본문 + CF 문서(C1.md F2, F4, F5). |
| C1.F3 | 층3: 관측 결과(조회 실패) | 이벤트 `CF_EID_ERR_INIT_SEM`("CF: failed to get sem id for name %s, error=…"; 2a024d8은 `0x%08x`, 833fdbb·281a941은 `%ld`)가 발생하고, 엔진이 활성화되지 않으며, `CF_Init`이 실패 상태를 반환하고, `CF_AppMain`이 `CFE_ES_RunStatus_APP_ERROR`로 둔 뒤 `CFE_ES_ExitApp`한다 | 원·검 | 이벤트 문구·엔진 미활성·APP_ERROR(`cf_app.c` L333) 직접 확인. |
| C1.F4 | 구조 차이(새 확인) | 2a024d8은 실패 시 `goto err_out`이고, 281a941은 `break` 후 함수 끝의 `if (ret == CFE_SUCCESS) { enabled = 1; } return ret;`로 반환한다. 사례 기록의 전파 서술(C1.md F3)은 2a024d8 구조만 기술한다. 두 경우 모두 오류가 호출자에게 반환되고 엔진은 활성화되지 않는다 | 원·검 | SPEC App. A.4가 "281a941에서 앵커 감사로 다시 읽는다 [U]"라고 남겨 둔 부분이다. |
| C1.F5 | 정상 경로: 코드 사실 | 833fdbb의 코드와 주석은 `OS_ERR_NAME_NOT_FOUND`를 다른 앱이 아직 생성하지 않은 시작 경쟁으로 보고 재시도하는 것을 의도된 대응으로 적는다 | 원·검 | `cf_cfdp.c` L992–L1011. |
| C1.F6 | 정상 경로: 판정 규칙 | 재시도 후 성공(A2)을 '허용된 회복'으로 일반화하는 판정은 근거를 명시해야 한다. 코드에서 후속 경로를 추출한 사실과 그 경로를 허용된 회복으로 해석하는 판단을 구분하고, 근거가 없으면 후자를 미판정으로 둔다 | 선(QT-4, Q15) | RESOLVED §4.1. |
| C1.F7 | 오류 경로의 구분 | A3(provider 지연으로 재시도 종료 후 최종 실패)과 재시도 중 첫 실패를 혼동하지 않는다. 단일 시도 리비전에는 A2가 없으므로 "해당 없음"으로 쓴다 | 선(제안) | ROADMAP §8.3, §8.5. |
| C1.F8 | A4(해당 이름의 provider 없음) | 원출처가 정한 cFS 구성이 없다. 단위 예제로 검증하거나 "미실현"으로 기록하며 실제 사례 결과로 세지 않는다 | 선(제안) | ROADMAP §8.3; SPEC A.4 step 6. |
| C1.F9 | A5의 판정 | `INCORRECT_OBJ_STATE`는 `OS_ERR_NAME_NOT_FOUND`가 아니므로 833fdbb의 재시도 대상이 아니다(코드 `if (ret != OS_ERR_NAME_NOT_FOUND) break;`) | 원·검 | 직접 확인. |
| C1.F10 | A5의 hang 판정 한계 | 어느 시간 뒤 hang이라고 부를지 | 미 | C1.md A7. |
| C1.F11 | 임무상 피해 | 이슈는 CF abort만 말한다. 임무상 피해는 확인된 바가 없다 | 미 | ROADMAP §6.1: 근거 없는 임무 피해는 미확인. |

### 3.G 관측 계획

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C1.G1 | 소스 위치 | 선택한 버전에서 조회와 생성 호출의 위치를 다시 읽어 고정한다. 확인한 위치: 2a024d8 L1012–L1023, 281a941 L993–L1001(함수 말미 반환 구조는 C1.F4), 833fdbb L1000–L1011, 15a871e L1270–L1290. 실패 이벤트의 이름은 버전마다 다르다: 2a024d8·281a941·833fdbb에서는 `CF_EID_ERR_INIT_SEM`(`cf_cfdp.c` L1018·L997·L1016), v7.0.1의 15a871e에서는 `CF_INIT_SEM_ERR_EID`(L1295)이며 모두 직접 확인했다(§9.2 V-C1-9; 그 사이 리비전은 보지 않았다) | 선(제안) | ROADMAP §8.2. 위치는 `검`이나 줄 고정은 기준 구성 확정 뒤 앵커 감사로 한다. |
| C1.G2 | 실제 바이너리 위치 | 파생 빌드를 만든 뒤 소스↔바이너리 대응을 확인한다. 아직 빌드가 없다 | 미 | C1.B9. |
| C1.G3 | 자원 식별값 | 이름 문자열과 OSAL 객체 ID·세대. 이름이 같아도 삭제·재생성된 객체를 같은 세대로 가정하지 않는다 | 선(제안) | ROADMAP §8.2. |
| C1.G4 | task 식별 | CF main task와 provider main task를 ES의 task/app 식별로 구분한다 | 선(제안) | ROADMAP §6.1 관측 계획. |
| C1.G5 | 연결할 사건 | provider의 생성 시작·이름 공개·객체 사용 가능 시점, CF의 조회 시점과 이름, 반환값, 시도 번호, 재시도 종료, CF 초기화의 최종 결과 | 선(제안) | ROADMAP §8.2. |
| C1.G6 | 실행 제어 | 사건 기반 보류(생성 직전)와 해제. 보류·정지·해제를 쓴 실행에는 표시한다. 추가 링크 변경 실행 제어와 PCT는 후순위 | 선(P-C1HOLD, D3, D1) | RESOLVED §3.3, §4.2. |
| C1.G7 | 관측 영향 점검 | K11의 항목들(기록 손실, 사건 대응 실패, 부팅·진행 시간, 외부 기능 결과) | 선(D1) | §2.1. |
| C1.G8 | 기본 구성의 처리 | 기본 구성에서 경로가 실행되지 않는 이유를 판정한다(설정 폐쇄) | 선(제안) | ROADMAP §8.4 단계 2. C1 증거가 아니다. |
| C1.G9 | RESERVED 구간 probe | 별도 후속 과제. 하는 경우 provider 또는 테이블 hang을 oracle에 포함해야 한다 | 선(P-C1HOLD) | RESOLVED §3.3; C1.md A7. |

### 3.H 예상 결과

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C1.H1 | 예상 결과표 | 실제 현상·도구 분류·기대 근거를 따로 적는 골격은 §8.2에 있다. 모든 값은 기대이며 미실행이다 | 선(제안) | ROADMAP §6.1. |

### 3.I 변경 기록

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C1.I1 | 원출처의 조건 | 보고 리비전 2a024d8(B1), 보고자 OS Ubuntu 21.10(C1), 재현 서술(A3), 요구 순서(E5), 수정 833fdbb·PR #342(A5) | 원 | 행: C1.A1–A5, B1, C1, E5. |
| C1.I2 | 원출처와 다른 조건 | (1) 기준 CF를 보고 리비전 2a024d8이 아니라 281a941→833fdbb로 둔다: 이유는 공개 BP 표가 281a941 레이아웃이고 혼합하지 않기 위해서(D22); 영향은 실패 전파 구조가 다르다(C1.F4). (2) provider를 원 보고자의 것이 아닌 공개 BP로 둔다: 동일성은 미확인. (3) 이름·시작 순서·우선순위가 공개 BP 값이며 원 보고자 값이 아니다. (4) 실행 호스트가 Ubuntu 24.04(원 21.10, 수정 시험 22.04). (5) ENV의 환경 이탈(K15). (6) 보류로 만든 실패는 제어된 실행이며 자연 실행이 아니다. (7) 공개 통합을 다른 core 조합·v7.0.1로 옮기면 그 구성·코드 차이 전부 | 선(차이) | RESOLVED §3.2–§3.3. |
| C1.I3 | 출처가 정하지 않은 조건 | A8, A9, B8, B9, C2, C6, C12, C13, D4, D5(적용성 미확인), E11, F10, F11, G2(확정된 core 조합이 없다는 점은 분석 행 B7에 후보로만 있다) | 미 | 위 행의 `미`. |
| C1.I4 | 이 대장의 변경 기록 | 2026-10-15 최초 작성(초안). 이 작성 중 281a941의 실패 전파 구조 차이(C1.F4)를 새로 확인했다. 2026-10-16 재점검(V-C1-R1–R5): 행 변경 없음 | 저 | §11. |

## 4. C2 — nasa/cFE #198 (시작 동기화 이후의 late initialization)

### 4.A 사례 정체성

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C2.A1 | 원 이슈 | nasa/cFE #198 "Additional CFE start up state for application sync". skliper, 2019-09-30 열림, 같은 날 닫힘(milestone 6.6.0 목록 근거), label enhancement, milestone 6.6.0, 댓글 없음, "No branches or pull requests" | 원† | C2.md §1.1. 웹 조각(≤125자) 인용. 이 세션에서 페이지 403. |
| C2.A2 | 원 보고의 주장 | 6.4.2의 시작 동기화는 단일 상태(`OPERATIONAL`)만 구현한다. 일부 앱은 지역 자료구조를 초기 상태로 두는 "early" init 뒤에, 다른 앱·라이브러리와의 통신이 필요할 수 있는 "late" init을 한다. 시작 동기화는 early init만 다루고 late init은 operational 상태에서 이뤄져, 아직 완전히 초기화되지 않은 앱에 요청이 들어올 수 있다. EVA CWS 프로젝트에서 한 앱이 다른 앱들의 완전한 로드를 기다리려고 late-phase init 전에 `CFE_ES_WaitForStartupSync()`를 썼고, 같은 호출을 쓰는 다른 앱들이 late-phase init이 끝나기 전에 다른 함수를 호출했다. 제안: late-phase init용 새 시작 상태 | 원† | C2.md §1.1 문장 1–8. |
| C2.A3 | 공개 코드와 연결: 단일 단계 배리어 | v6.5.0a(`b2765d9f`) `cfe_es_start.c` L266–L291: `CORE_READY` → `StartApplications` → `ApplicationSyncDelay(UNDEFINED, 시한)`(실패하면 syslog만) → `OPERATIONAL`. `cfe_es_api.c`의 `WaitForStartupSync`는 호출자를 RUNNING으로 표시한 뒤 core 앱은 `CORE_READY`, 그 외는 `OPERATIONAL`까지 대기. 6.4.2 tarball과 같다는 점은 코드 비교에 의한 추정(C2.B2) | 원·검 | b2765d9f 파일 직접 확인. |
| C2.A4 | 공개 코드와 연결: 수정 | v6.6.0a(`8811c920`): 시스템 상태 `APPS_INIT=4`(OPERATIONAL=5), 앱 상태 `LATE_INIT=2`, `CFE_ES_WaitForSystemState`, ES 메인의 2단계 배리어(`MainTaskSyncDelay(LATE_INIT)` → `APPS_INIT` → `MainTaskSyncDelay(RUNNING)` → `OPERATIONAL`) | 원·검 | `cfe_es_extern_typedefs.h` L189–L209·L271–L281, `cfe_es_start.c` L207–L233. |
| C2.A5 | 6.6.0 VDD의 행 | "#167 es Additional CFE start up state for application sync — `465a958`: Add additional sync state for apps; `b227d57`: Remove stale comment in CFE_ES_ExitApp". 두 커밋은 공개 git 객체가 아니다 | 원·검 | `docs/cFE_6_6_0_version_description.md` L189–L190(VDD 행). 커밋이 비공개라는 점은 저(C2.md §1.3). |
| C2.A6 | #198 = Trac #167 | GitHub 제목 동일성과 상수 오프셋 +31(제목이 같거나 거의 같은 20쌍)으로 추정. 명시적 상호 참조는 없다 | 분 | C2.md §1.3, D1. |
| C2.A7 | 현재 코드 | dev `f22e538c`도 같은 설계. `CFE_ES_WaitForStartupSync`는 `CFE_ES_WaitForSystemState(OPERATIONAL, …)`를 부르는 `void` 래퍼 | 원·검 | `cfe_es_api.c` L616–L619; `cfe_es_start.c` L211·L217·L226·L235. |
| C2.A8 | 연결되지 않는 부분 | EVA CWS(비공개)의 앱 이름·수·함수·서비스 구현·cFE 버전·플랫폼·로그·증상·빈도. 이슈가 6.4.2를 지명한 것은 동기화를 개선한 릴리스라는 맥락뿐이다 | 미 | C2.md §3, §10. |
| C2.A9 | "R의 호출이 P를 향했다" | 이슈는 R이 "other functions"를 불렀다고만 한다. 호출 대상이 late-init 앱 P라는 것은 추정이다 | 분 | C2.md §4, §7. |
| C2.A10 | 원 Trac 기록·Trac 커밋 | 접근 불가(호스트 해석 실패/프록시 차단), 커밋 비공개 | 미 | C2.md §1.3, §13. |
| C2.A11 | 결과 명칭과 한계 | 결과의 이름은 "C2 메커니즘의 역할 기반 검증"이며 비공개 원 시스템(EVA CWS)의 재현으로 쓰지 않는다 | 선(P-C2) | RESOLVED §3.5. |
| C2.A12 | 경로 | 코드 읽기·정적 분석에서 시작하되 역할 재구성 실험까지 진행한다. 원 애플리케이션의 비공개 부분을 사실처럼 채우지 않는다 | 선(P-C2) | RESOLVED §3.5. |
| C2.A13 | 'race' 서술 | 이슈 본문의 "race condition"을 도구 class로 자동 변환하지 않는다 | 선(P-CLASSGT) | RESOLVED §4.3. |

### 4.B 소스 기준

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C2.B1 | 단일 단계 쪽 후보 (a) | cFE 6.4.2 tarball(sha256 `c64ed2aa…a53a4f`, SH-2). 이슈 본문이 6.4.2의 시작 동기화를 서술한다 | 원·검† | tarball sha256·크기 일치(`검`). 이슈 본문의 6.4.2 서술은 †(이슈 미재확인, 조각 인용; 이슈가 EVA의 빌드를 말하지는 않는다 — C2.A8). 클래식 make 빌드뿐, CMake 파일 없음(저). |
| C2.B2 | 단일 단계 쪽 후보 (b): 세 함수의 비교 | v6.5.0a(`b2765d9f`)와 6.4.2 tarball에서 `CFE_ES_WaitForStartupSync`(`cfe_es_api.c` 6.4.2 L652–L695, v6.5.0a L651–L694)와 `CFE_ES_ApplicationSyncDelay`(`cfe_es_start.c` 6.4.2 L1007–L1040, v6.5.0a L1009–L1042)는 공백을 무시하면 같다. `CFE_ES_Main`(6.4.2 L166–L294, v6.5.0a L164–L292)은 인자 `StartFilePath`의 형(`uint8 *` 대 `const char *`)과 syslog 인자 하나의 형 변환만 다르고, 동기화 말미(6.4.2 L268–L294, v6.5.0a L266–L292)는 같다 | 원·검 | 2026-10-16 직접 비교(§9.3 V-C2-8). 같은 결과가 `plan/case_setup_C2_C3_C5.md` §2.2에도 기록돼 있다. |
| C2.B2a | 단일 단계 쪽 후보 (b): 전체 동등성 | 세 함수 밖의 시작 동기화 관련 코드(`cfe_es_apps.c`의 `AppStartedCount` 증가, `RunLoop`, 빌드 순서)까지 두 개정이 같은 동작을 하는지는 비교하지 않았다. 따라서 'v6.5.0a는 6.4.2와 동기화가 같다'는 전체 주장은 분석으로 남는다 | 분 | C2.md §1.2. CMake 빌드는 `_ENHANCED_BUILD_`로 정적 앱을 스크립트 앱보다 먼저 만든다는 차이가 있다(저). |
| C2.B3 | 수정 쪽 | v6.6.0a `8811c92094512975e14fd545df355705141560b2`(첫 공개 커밋 `2661d19f` "Bulk add of cFE 6.6.0a") 또는 현재 dev | 원·검 | 8811c920 내용 직접 확인. `2661d19f`는 저. |
| C2.B4 | 리비전별 OSAL/PSP 짝 | 6.4.2: OSAL 4.1.1·PSP 1.2.0.0. v6.5.0a: git의 OSAL gitlink `7139592f`(4.2.1a-1)와 PSP 1.3.0.0, 그러나 VDD는 OSAL 4.2.0. 6.6.0: OSAL 4.2.1, OSAL 4.2.0·PSP 1.3.0 미만과 비호환. 리비전별 짝을 따라야 한다 | 저 | C2.md §3, §10 #10b. VDD와 git이 어긋난다는 점도 기록에 있다. |
| C2.B5 | 단일 단계 셀에 쓸 cFE 코드의 선택 | 6.4.2와 v6.5.0a 중 어느 것인지, 어떤 빌드 시스템·OSAL/PSP 짝을 쓰는지 정해지지 않았다. 6.4.2 행은 D13의 하위 항목이다 | 미 | C2.md D2; SPEC §9.3("D13 for a 6.4.2 row"). 병행 계획 문서 `plan/case_setup_C2_C3_C5.md` §1은 6.4.2를 단일 단계 기준으로 삼는 안을 적었다. 이는 계획 문서의 제안이며 PI 결정이 아니어서 이 행의 상태를 바꾸지 않는다. |
| C2.B6 | 역할 앱의 정의 | provider P(late init 완료 후 서비스 준비)와 consumer R(의존 사용)의 역할만 재구성한다. 요청 방식·초기화 내용·우선순위·timeout은 재구성 실험의 선택으로 기록하고 원 보고자의 값으로 인용하지 않는다 | 선(P-C2) | RESOLVED §3.5. |
| C2.B7 | 2×2 교차 | 단일 단계/두 단계 시작 동기화 × 기존/수정 provider 역할. 평가에는 각 셀을 그대로 제시하고 네 셀을 독립된 실제 결함 네 개로 세지 않는다. 실행 가능한 셀과 제외한 셀의 이유를 남긴다 | 선(P-C2) | RESOLVED §3.5; ROADMAP §9.1. |
| C2.B8 | Q22 | 1차 대조는 provider의 late init이 정상 완료하는 구성. 초기화 실패까지 가능한 구성은 별도 보조 실험. "초기화는 실패하지 않는다"는 가정은 재구성 구성에만 적용하고 cFS 전체의 보장으로 일반화하지 않는다. 모델 분류는 가정의 포함 여부와 함께 보고 | 선(Q22) | RESOLVED §3.5. |
| C2.B9 | 단일 단계 코드에 없는 기호 | 6.4.2 tarball의 `cfe/fsw/cfe-core/src` 전체와 v6.5.0a(`b2765d9f`)의 `cfe/fsw/cfe-core/src`에 `WaitForSystemState`, `APPS_INIT`, `LATE_INIT`이라는 문자열이 없다. v6.6.0a(`8811c920`)에는 있다(`cfe_es_api.c`, `cfe_es_start.c`, `cfe_es_task.c` 등) | 원·검 | 2026-10-16 `grep`(§9.3 V-C2-9). 그 결과 '단일 단계 × 수정 provider' 셀은 이 코드에 이식 없이 만들 수 없다는 것은 분석이며, 이식하면 다른 프로그램 변형(U3)이 된다. 셀을 제외할지 이식할지는 P-C2의 하위 선택이다(E-C2-P4). |

### 4.C 실행 조건

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C2.C1 | EVA CWS의 플랫폼·OS·CPU·보드·SMP·빌드 | 이슈에 없다 | 미 | C2.md §3, §10 #10a. |
| C2.C2 | EVA CWS의 앱 이름·수·시작 순서·우선순위·스택 | 이슈에 없다. 재구성 실험의 값은 선택하는 시점에 선택으로 기록하며 이 문서는 정하지 않는다 | 미 | C2.md §10 #3. |
| C2.C3 | P의 late init의 내용·소요 | 일반 서술("may require some communications with other applications or libraries")뿐이다 | 미 | C2.md §10 #4. |
| C2.C4 | R→P 요청의 방식 | SB 명령, 라이브러리 호출, 공유 메모리 등 어느 것인지 정해진 바가 없다 | 미 | C2.md D4, A3. |
| C2.C5 | P와 R이 `WaitForStartupSync`에 넘긴 timeout | 이슈에 없다. 문서에 "must be at least 1000. Lower values will be rounded up"이 있으나 코드가 강제하지 않는다(문서 문구는 `검`, 코드 비강제는 저) | 미 | C2.md §10 #11, A7. |
| C2.C6 | EVA CWS의 ES 시작 관련 설정값 | `CFE_ES_STARTUP_SCRIPT_TIMEOUT_MSEC` 등 | 미 | C2.md §10 #12–#14. |
| C2.C7 | 샘플 설정의 참조 상수 | 6.4.2·v6.5.0a 샘플: `SCRIPT_TIMEOUT` 30000, `SYNC_POLL` 50, `CORE_MAX_STARTUP` 30000. v6.6.0a·현재 샘플: 1000, 50, 30000. EVA CWS의 값이 아니다 | 저 | C2.md §10 #12–#15c(미재확인). 정책이 허용하는 '레퍼런스에 있는 상수'이다. |
| C2.C8 | 재구성 플랫폼 | 네이티브 Linux(pc-linux)를 제안한다. EVA 플랫폼이 불명이어서 비교할 레퍼런스 값은 없지만, 이 값은 어느 레퍼런스에도 없는 값을 고르는 것이므로 차이로 기록한다(SPEC_FINAL 머리말의 규칙; 초안의 '편차가 아니라 가정'이라는 표현은 철회). OSAL/PSP는 리비전별 짝을 따른다 | 선(차이) | C2.md A6; C2.I2(4). PI 결정 전의 제안이다. |
| C2.C9 | 타이밍 제어 | 사건 기반: R의 요청 진입이 관찰될 때까지 P의 late-init 완료를 보류한다. 고정 지연을 쓰지 않는다 | 선(제안) | C2.md A5; CONDITIONS_POLICY. |
| C2.C10 | 시작 스크립트 앱 여부 | 시작 스크립트로 시작한 앱일 가능성(이슈가 "start up sync"를 다루고 API 문서가 스크립트 앱으로 한정) | 분 | C2.md §10 #16. |
| C2.C11 | 관측 방식·N | K11(D1), K12(D14). 두 단계 셀을 ENV(v7.0.1) 위에 둘 경우 K8–K10도 적용 | 선(D1, D14, D19, D17) | §2.1. |

### 4.D 초기 상태

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C2.D1 | EVA CWS의 reset 종류 | 정해진 바가 없다 | 미 | C2.md §10 #17. |
| C2.D2 | 재구성의 종료·정규화 기준 | v7.0.1 CI 명령(D17)이 6.x 코드의 단일 단계 셀에 적용되는지 확인되지 않았다 | 미 | SH-16과 같은 문제. |
| C2.D3 | provider P의 초기 상태 정의 | early init 완료와 RUNNING 게시 시점, late init 시작과 완료를 나타내는 관측 가능한 준비 지표를 재구성 단계에서 정의해야 한다 | 미 | C2.md A3. |

### 4.E 의존성

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C2.E1 | 자원 | P가 late init을 마친 뒤의 준비 상태("completely up and running"). 내용은 비공개 | 원† | 이슈 문장 4. 내용은 C2.C3 `미`. |
| C2.E2 | 게시 지점 | P 내부(위치 미상). cFE가 노출하는 신호는 앱 상태 RUNNING(`WaitForStartupSync` 또는 첫 `RunLoop`)과 시스템 상태 OPERATIONAL뿐이다. 이슈의 방식은 late init **이전에** RUNNING을 게시한다 | 분 | C2.md §4. cFE 코드는 `검`(C2.A3). |
| C2.E3 | 사용 지점 | R의 "other functions" 호출(함수·메커니즘 미상). R을 푸는 지점은 `ApplicationSyncDelay`가 `SystemState >= OPERATIONAL`에서 나가는 곳 | 저 | C2.md §5(미재확인). |
| C2.E4 | 요구되는 순서(원출처) | P의 late-phase init이 다른 앱들의 "other functions" 호출보다 먼저 끝나야 한다. 앱은 late init을 마쳐야 완전히 동작하는 것으로 본다 | 원† | 이슈 문장 4, 5b–5c, 7. |
| C2.E5 | 기존 동기화 | P와 R이 모두 `CFE_ES_WaitForStartupSync`를 썼다(이슈). ES 공유 데이터 mutex는 부기만 보호. 소프트 타임아웃. 생성 순서(시작 스크립트 순서)와 우선순위는 초기화 완료 순서가 아니다 | 원† | P·R 모두 같은 호출을 썼다는 서술은 이슈 문장 6b·7. 나머지는 저(C2.md §6). |
| C2.E6 | 수정의 종류 | 순서 보장(새 배리어 단계). 옵트인이며 소프트 타임아웃으로 한정된다. 재시도·가드가 아니다. P가 `WaitForSystemState(APPS_INIT)`로 코드를 바꿔야 하며 6.4.2 방식(late init 전에 `WaitForStartupSync`)을 유지하면 여전히 일찍 RUNNING이 된다 | 분 | C2.md §8; 코드는 `검`. |
| C2.E7 | 수정의 한계 | `WaitForStartupSync`는 void다. ES의 2단계 소프트 타임아웃이 만료되면 waiter가 `CFE_SUCCESS`로 풀려난다. #1466·#1467은 open, PR #2273·#2286은 병합 없이 닫힘 | 저 | 코드(void 래퍼, 타임아웃 후 계속)는 `검`. 이슈·PR 상태는 C2.md §1.4(미재확인). |
| C2.E8 | C1과의 관계 | C1의 제안 조치와 같은 호출을 다룬다. 한 사례의 결과로 다른 사례를 완료 처리하지 않는다 | 분 | ledger §3.5. |

### 4.F 정상·오류 판정 기준

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C2.F1 | 층1: `WaitForStartupSync`의 API 문서 | "This API should only be called as the last item of an Apps initialization." / 시한 인자 "must be at least 1000. Lower values will be rounded up." 같은 소스의 함수 목적 주석은 이슈의 P 방식(다른 앱의 실행 확인 후 초기화 완료)을 문서화된 사용으로 읽을 수 있게 해 문서가 서로 어긋난다. 따라서 문서화된 API 의무(API contract)를 하나의 해석으로 고정하지 않는다 | 원·검 | `cfe_es.h` L665·L671 직접 확인. 내부 주석과의 어긋남은 저(C2.md §1.2). |
| C2.F2 | 층1: 수정 후 API의 반환 | `WaitForSystemState`는 호출자 자신의 timeout에 `CFE_ES_OPERATION_TIMED_OUT`을 반환한다. ES의 단계 timeout은 로그 후 계속 진행한다. `WaitForStartupSync`는 void다 | 저 | C2.md §8, §9(void와 단계 타임아웃의 계속 진행은 `검`). |
| C2.F3 | 층2: 호출자 요구(준비 상태 전제) | R의 요청 진입 시점에 P의 late init이 완료돼 있어야 한다. 재구성에서는 P 내부의 late-init 완료 사건으로 관측해야 하며, 그 지표는 재구성의 선택이다 | 선(제안) | C2.md A3, A4. |
| C2.F4 | 층3: 원 보고의 관측 결과 | "다른 앱이 late-phase init 완료 전에 다른 함수를 호출했다"뿐이다. 충돌·오류 코드·로그는 보고되지 않았다 | 원† | 이슈 문장 7. |
| C2.F5 | 층3: 재구성에서 추가로 관측되는 증상 | 파생 결과로 표시한다 | 선(제안) | C2.md A4; ROADMAP §9.1. |
| C2.F6 | timeout 종료와 정상 완료 | timeout으로 기다림이 끝난 경우를 정상 초기화 완료와 구분한다. barrier 통과와 앱 고유 초기화 완료는 같지 않다 | 선(제안) | ROADMAP §9.1 작업 6. |
| C2.F7 | 정상·오류 라벨 | 원출처가 허용된 회복을 말하지 않는 항목은 미판정으로 두고, 코드상 상태 변화와 오류 반환은 계속 보고한다 | 선(QT-4, Q15) | RESOLVED §4.1. |
| C2.F8 | 임무상 피해 | 확인된 바가 없다 | 미 | — |

### 4.G 관측 계획

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C2.G1 | 기록할 사건 | 동기화 반환, 실제 초기화 완료, 의존 사용, 오류 반환 또는 회복 결과 | 선(제안) | ROADMAP §9.1 작업 5. |
| C2.G2 | 소스 위치(버전별 상태 전이·timeout 경로) | v6.5.0a `cfe_es_start.c` L266–L291, `cfe_es_api.c`의 `WaitForStartupSync`; v6.6.0a `cfe_es_start.c` L207–L233; dev `cfe_es_start.c` L211–L235, `cfe_es_api.c` L616–L619 | 선(제안) | ROADMAP §9.1 작업 1. 위치는 `검`. |
| C2.G3 | 바이너리 위치·자원 식별 | 역할 앱도 빌드도 아직 없다 | 미 | — |
| C2.G4 | 비교 구성 | 단일/두 단계 barrier × 기존/수정 provider의 2×2. 각 셀을 그대로 제시한다 | 선(P-C2) | RESOLVED §3.5. |
| C2.G5 | v7.0.1에서의 일반 층 점검 | barrier의 writer 집합(`ExitApp`의 STOPPED 저장 포함)과 K-PHASE census가 코드 읽기로 찾은 항목을 다시 찾는지. C2 자체의 인스턴스는 v7.0.1에 없으므로 C2 증거가 아니다 | 선(SPEC §9.3, ADR-044) | K16. |

### 4.H 예상 결과

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C2.H1 | 예상 결과표 | §8.3. 모든 값은 기대이며 미실행이다 | 선(제안) | ROADMAP §6.1. |

### 4.I 변경 기록

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C2.I1 | 원출처의 조건 | 이슈 본문이 말하는 구조: 단일 `OPERATIONAL` 배리어, late init 이후에야 완전히 동작, P와 R의 같은 호출 사용, 새 시작 상태를 제안 | 원 | 행: C2.A1, A2, E1, E4, F4. |
| C2.I2 | 원출처와 다른 조건 | (1) 비공개 EVA CWS 대신 역할 기반 합성 대조(원 시스템 재현이 아님). (2) 단일 단계 셀을 6.x 코드(6.4.2 또는 v6.5.0a)로 만들면 ENV(v7.0.1)와 다른 빌드 시스템·OSAL/PSP 짝을 쓰게 된다. (3) 역할 앱의 요청 방식·초기화 내용·우선순위·timeout은 재구성의 선택이다. (4) 네이티브 Linux(pc-linux): EVA 플랫폼이 불명이어서 비교할 값은 없지만 어느 레퍼런스에도 없는 값을 고르는 것이므로 차이로 기록한다(C2.C8) | 선(차이) | P-C2; C2.md §11. |
| C2.I3 | 출처가 정하지 않은 조건 | A8, A10, B5, C1–C6, D1–D3, F8, G3 | 미 | 위 행의 `미`. |
| C2.I4 | 이 대장의 변경 기록 | 2026-10-15 최초 작성(초안). 2026-10-16: C2.B2를 C2.B2(`원·검`)와 C2.B2a(`분`)로 나누고 C2.B9를 더했으며 E-C2-P4를 고쳤다(V-C2-8, V-C2-9); C2.B5에 병행 계획 문서의 제안을 비고로 적었다 | 저 | §11. |

## 5. C3 — nasa/cFE #73 (core 앱 시작 순서와 EVS 오류 경로)

공유 소스 기준과 파생 플랫폼은 §2.2(SH-1–SH-17)에 있다. 아래 행은 C3에 고유한 부분이다.

### 5.A 사례 정체성

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C3.A1 | 원 이슈 | nasa/cFE #73 "Race conditions / dependencies between CFE core apps". 마이그레이션 작성자 skliper, 2019-09-30 열림·같은 날 닫힘, label bug, milestone 6.4.2, 댓글 14개 | 원† | C3.md §1.1. 이 세션에서 페이지 403. |
| C3.A2 | Trac 연결 | #73 = Trac #42. 마이그레이션 댓글 헤더 "Imported from trac issue 42. Created by jphickey on 2015-05-06T17:04:36"과 6.4.2 VDD Attachment 1의 같은 제목 | 원† | 헤더는 검색 발췌로 읽은 것이다(검색 발췌의 지위는 X2 미결정, 아래 C3.I3). VDD 행은 저. |
| C3.A3 | 원 보고의 주장(목록 1) | core 앱 간 의존성이 더 적극적으로 충족돼야 한다. 최소한 core 앱을 동기화해 각 앱이 RunLoop에 도달한 뒤 다음을 시작한다. `EVS_IsFiltered`는 값으로 표 조회를 하기 전에 범위 검사를 해야 한다. SB와 EVS가 서로 다른 값을 자기 AppID 전역에 넣는다(SB는 기본 0이며 다른 앱에게는 유효한 AppID, EVS는 0xFFFFFFFF) | 원† | C3.md §1.1. |
| C3.A4 | 원 보고의 주장(목록 2: EVA팀 GRC Microblaze 디버깅) | 생성 순서 EVS, SB, ES, TIME, TBL. 우선순위 61/64/68/60/70. TIME task가 먼저 실행. `CFE_SB_CreatePipe`가 `CFE_SB.AppId`(=0)로 `CFE_EVS_SendEventWithAppID`를 호출 → `EVS_NotRegistered` → `EVS_SendEvent` → `EVS_IsFiltered(EVS_AppID=0xFFFFFFFF)`가 범위 검사 없이 사용 → segfault, CFE core 중단 | 원† | C3.md §1.1. |
| C3.A5 | 공개 코드와 연결: 버그 쪽 | 6.4.1 tarball에 호출 경로 전체가 있다: 생성 순서(`cfe_es_objtab.c`), 우선순위(`cfe_platform_cfg.h` L1115·L1138·L1161·L1188·L1217), SB `GetAppID`(L198)·EVS 등록(L257), EVS `EVS_AppID` 초기값 L190·`EVS_GetAppID` L385·`CFE_EVS_Register` L393, TIME `CFE_EVS_Register` L252·`CFE_SB_CreatePipe` L308, `CFE_SB_CreatePipe`의 `CFE_EVS_SendEventWithAppID(PIPE_ADDED, DEBUG, CFE_SB.AppId)`(`cfe_sb_api.c` L264), `EVS_SendEvent`의 범위 검사 없는 `EVS_IsFiltered` 호출(`cfe_evs_utils.c` L616) | 원·검 | 6.4.1 tarball에서 직접 확인(§9). |
| C3.A6 | 공개 코드와 연결: 수정 쪽 | 6.4.2의 세 부분. F3: ES가 core task를 하나 만든 뒤 준비를 기다리고 timeout이면 panic하며, 각 core TaskMain이 `CFE_ES_WaitForStartupSync`를 부른다. F1: `EVS_AppID < CFE_ES_MAX_APPLICATIONS` 검사(`cfe_evs_utils.c` L615–L621). F2: `EVS_AppID`를 초기화 마지막(`cfe_evs_task.c` L436)에 대입 | 원·검 | 6.4.2 tarball 직접 확인. 6.4.1에는 core task의 `WaitForStartupSync` 호출이 없음(소스 전체 grep)도 확인. |
| C3.A7 | 수정 커밋 | Trac 쪽 커밋 `2633a3d`(#40), `0c00114`·`e08eada`(#42)와 브랜치 `trac-42-rangecheck-evs-appid`, "Merged to development on Wed May 20 09:39:24 2015 -0400". 모두 공개 git 객체가 아니다 | 원† | 마이그레이션 댓글의 검색 발췌(C3.md §1.2.1, 8.1). X2 미결정. |
| C3.A8 | "found in 6.4.1" | 6.4.2 VDD §1.1 "The purpose of this build is to fix a series of startup race conditions that were found in cFE build 6.4.1" | 저 | C3.md §1.3(미재확인). 이 문장은 시리즈를 말하며 #42를 지명하지 않는다. |
| C3.A9 | #42가 그 시리즈에 속한다 | #42가 6.4.2 티켓 중 시작 경쟁이고 VDD Table 1.2-2 row 3이 그 수정을 기술한다는 것에서 나온 추정 | 분 | C3.md §3. |
| C3.A10 | 연결되지 않는 부분 | Microblaze의 OS/RTOS·BSP/PSP·OSAL·컴파일러·클록·메모리 맵, EVA가 실제로 실행한 cFE 빌드, fault 주소·신호·로그, 재현 빈도, 14개 댓글 전문과 4개 미식별 댓글 | 미 | C3.md §10 #5·#7·#20·#22. |
| C3.A11 | R2와의 관계 | 둘 다 SB task의 AppId 전역을 사용 전에 게시하지 못하는 계열을 공유한다. 6.4.2의 잔여 경로는 EVS, v7.0.1은 ES·EVS다. 같은 자원 계열이라는 연결은 추정이다 | 분 | C3.md §8.3; ledger §3.1. |
| C3.A12 | R2 기록의 사용 | 현재 버전의 R2 기록을 C3의 추가 실행 근거로 계산하지 않는다. R2의 관측을 C3의 crash로 바꿔 쓰지 않는다 | 선(제안) | ROADMAP §9.2 완료 기준. |
| C3.A13 | C3와 R2를 한 패턴으로 세는가 | "SB AppId before publication"을 한 패턴으로 세는지 두 사례로 세는지 | 미 | ledger §5 X4. |
| C3.A14 | C5와의 관계 | 같은 6.4.1→6.4.2 수정 릴리스와 Trac #40–#42를 공유하므로 수정 전후 쌍이 하나의 수정 이력을 나눈다. 독립 표본이 아니다 | 분 | ledger §3.2–§3.3; SPEC §8.2. |
| C3.A15 | 'race' 서술 | 이슈 제목과 VDD의 "startup race conditions"를 도구 class로 자동 변환하지 않는다. SPEC H-V는 이 사례가 σ_R1에서 fixed-order로 나올 것을 기대해 원출처 서술과 어긋날 수 있다. 그 불일치 자체를 결과로 보고한다 | 선(P-CLASSGT) | RESOLVED §4.3; SPEC §8.2, §7.6. |

### 5.B 소스 기준

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C3.B1 | 버그 쪽 | 6.4.1 tarball(SH-1) | 원·검 | §2.2. |
| C3.B2 | 수정 쪽 | 6.4.2 tarball(SH-2) | 원·검 | §2.2. 6.4.2를 C2의 단일 단계 후보와 한 빌드로 겸할 수 있는지는 D13에서 일관되게 정해야 한다(ledger §3.3). |
| C3.B3 | EVA가 실행한 cFE 빌드 | 이슈가 버전을 말하지 않는다 | 미 | C3.md §10 #5. |
| C3.B4 | OSAL | SH-5–SH-8. 6.4.1 readme는 OSAL을 "4.1.1 or higher"로 요구하고 얻을 곳을 SourceForge와 github.com/nasa/osal 두 곳으로 적으며 파일 이름은 예시로만 적는다(SH-5). SourceForge artifact와 git tag `osal-4.1.1`은 `osapi-version.h`만 다르다(SH-6, SH-7). C3 기록은 git tag를 썼다. 이것을 차이 조건으로 셀지는 해석이며 X3 결정 전이다(SH-5b) | 원·검 | §2.2. 통일은 미결정(SH-16). |
| C3.B5 | PSP | 6.4.1 tarball의 `psp/fsw/`에 Microblaze PSP가 없다(`grut699-vxworks6`, `mac-osx`, `mcf5235-rtems`, `mcp750-vxworks6.4`, `pc-cygwin`, `pc-linux`) | 저 | C3.md §3(미재확인). |
| C3.B6 | 기준 결정 | 분석 전용 파싱 인정, (b)(i)부터, F1 전에 배치 | 선(D13) | SH-10–SH-14. |
| C3.B7 | 대응표 | 상태·우선순위·생성 순서·호출 경로를 6.4.1/6.4.2 소스에서 대응시키는 작업. 버전별 상태·반환값·소스 위치 대응을 시험하고, 입력·대응 실패는 탐지 기법의 실패로 세지 않는다 | 선(D13, Q16) | ROADMAP §9.2 작업 1; RESOLVED §3.4. |

### 5.C 실행 조건

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C3.C1 | 원 플랫폼 | Xilinx Microblaze(EVA팀, GRC) | 원† | 이슈 목록 2 도입문, #71. |
| C3.C2 | Microblaze의 OS·BSP/PSP·OSAL·컴파일러·워드 크기 | 정해진 바가 없다 | 미 | C3.md §10 #7·#27. |
| C3.C3 | core task 우선순위 | 이슈가 "default/example platform configuration"의 EVS 61, SB 64, ES 68, TIME 60, TBL 70을 인용하며 6.4.1의 cpu1 샘플 값과 같다 | 원·검† | 이슈가 그 값을 인용했다는 부분은 †(이슈 미재확인, C3.A4). 샘플 값은 직접 확인(§9). 낮은 숫자가 높은 우선순위(`cfe_platform_cfg.h` L1185–L1186는 저). |
| C3.C4 | EVA의 빌드가 기본 우선순위를 썼는가 | 이슈 서술의 함의일 뿐 명시되지 않았다 | 미 | C3.md §10 #10. |
| C3.C5 | core task 생성 순서 | EVS, SB, ES, TIME, TBL | 원·검 | 6.4.1 `cfe_es_objtab.c`. |
| C3.C6 | TIME 자식 task 우선순위 | 샘플에서 tone·1Hz task가 25로 모든 core task보다 높다. EVA의 값은 정해진 바가 없다 | 원·검 | `cfe_platform_cfg.h` L1189–L1190(샘플 값). EVA 값은 C3.md §10 #26. |
| C3.C7 | `CFE_ES_MAX_APPLICATIONS` | 샘플 32. EVA 값 미상 | 저 | C3.md §10 #17. |
| C3.C8 | 파생 후보 플랫폼의 값 | 32-bit x86 Linux(`-m32`, `-g -O0`), `OS=posix`, `PSP=pc-linux`, 구성 `cpu1`. 값은 SH-9에 인용한 6.4.1 릴리스 문서·PSP·make 파일의 것이며 원 보고자(Microblaze)의 관측 조건이 아니다 | 원·검 | SH-9; 확인은 §9.2 V-C3-10. |
| C3.C8a | 파생 플랫폼으로 쓰는 범위(D13) | 분석 전용 파싱, 현재 컨테이너(Ubuntu 24.04)의 32-bit 헤더로 하는 첫 파싱((b)(i)), 헤더 민감성이 확인되면 (ii)로 전환(SH-9a–SH-12). Microblaze에 대한 차이 조건이다(C3.I2) | 선(D13) | RESOLVED §3.4. |
| C3.C9 | 순서 제어 | 우선순위 구동(root와 단일 CPU 필요)인가 사건 기반 보류(SB `CFE_SB_AppInit`과 EVS `CFE_EVS_TaskInit`을 TIME의 `CFE_SB_CreatePipe` 이벤트가 관측될 때까지 보류)인가는 정해지지 않았다. 추가 링크 변경·PCT는 후순위(D3), 실제 우선순위 실행은 별도 구성(D4) | 미 | C3.md D3; SH-15. 선택 전에 값을 고르지 않는다. |
| C3.C10 | OSAL 4.1.1의 우선순위 적용 | root일 때만 `SCHED_FIFO`와 재매핑 우선순위를 적용한다(`osapi.c` L587 `geteuid() == 0`). 비루트면 기본 정책이다 | 원·검 | OSAL 4.1.1 git tag 직접 확인. 재매핑된 숫자(TIME 66, EVS 65, SB 64, ES 63, TBL 62)는 분석(C3.md §6). |
| C3.C11 | CPU 수·선점 모델 | 정해진 바가 없다. 우선순위 구동 순서를 만들려면 단일 CPU 또는 같은 제한이 필요하다는 것은 추정이다 | 미 | SH-17; C3.md A10. |
| C3.C12 | 시작 스크립트·외부 앱 | core task는 스크립트 실행 전에 생성되어 이 경로와 무관하다 | 저 | C3.md §10 #23(미재확인). |
| C3.C13 | 기록기의 독립 | 기록기는 EVS·SB·TIME을 쓰지 않는다(시험 대상 자원이므로) | 선(제안) | C3.md A9. |
| C3.C14 | 실행 결과의 분리 | 분석용 환경에서는 소스 추출과 기대 결과만 보고하고, 실제 플랫폼을 구성한 뒤 실행 결과를 추가한다. 과거 플랫폼 실행이 불가능하면 분석 전용 결과를 먼저 보존하되 실행 재현으로 올려 세지 않는다 | 선(D13, 제안) | RESOLVED §3.4; ROADMAP §6.3, §9.2. |
| C3.C15 | 관측 방식·N | K11(D1), K12(D14) | 선(D1, D14) | §2.1. |

### 5.D 초기 상태

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C3.D1 | EVA의 reset 종류·빈도 | 정해진 바가 없다 | 미 | C3.md §10 #21·#22. |
| C3.D2 | 파생(pc-linux)의 reset | 인수 없이 실행하면 PO(전원 켜기). 6.4.x에서의 종료·재시작 절차와 D17 명령의 적용성은 확인되지 않았다 | 선(D13) | SH-9; SH-16. |
| C3.D3 | `EVS_AppID` 초기값 | `CFE_EVS_EarlyInit`이 `CFE_EVS_UNDEF_APPID`(0xFFFFFFFF)로 설정한다 | 원·검 | `cfe_evs_task.c` L190, `cfe_evs_task.h` L105. |
| C3.D4 | `CFE_SB.AppId` 초기값 | 파일 스코프 객체이며 `CFE_SB_EarlyInit`이 설정하지 않아 0이다. 0은 다른 앱(EVS)에게는 유효한 AppID다 | 원† | 이슈 항목 4 "actually 0". 코드 줄은 저(C3.md §4). |
| C3.D5 | AppId 0의 소유자 | `CFE_ES_Main`이 시작할 때마다 `AppTable[i].RecordUsed`를 지우고 첫 빈 슬롯을 쓰며 `CFE_EVS`가 객체 표의 첫 core task다 → AppId 0 = CFE_EVS | 분 | C3.md §4. 실행 로그("ES Startup: Core App: … App ID: …")로 확인 가능하나 하지 않았다. |

### 5.E 의존성

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C3.E1 | 자원 | 서로 다른 필드 4종: R-SB `CFE_SB.AppId`, R-EVSID `CFE_EVS_GlobalData.EVS_AppID`, R-EVSREG EVS의 앱별 등록 상태(`RegisterFlag`·필터), R-ES core task의 TaskTable/AppTable 항목. SB의 AppId와 EVS 자신의 AppId를 합치지 않는다 | 원·검 | 코드 위치는 C3.A5, 구분의 근거는 이슈 항목 4–6. |
| C3.E2 | 게시 지점 | SB task: `CFE_SB_AppInit`의 `CFE_ES_GetAppID(&CFE_SB.AppId)`(L198), EVS 등록(L257). EVS task: `CFE_EVS_TaskInit`의 `EVS_GetAppID`(L385), 등록(L393) | 원·검 | 6.4.1. |
| C3.E3 | 사용 지점 | TIME task: `CFE_TIME_TaskInit` → `CFE_SB_CreatePipe`(L308) → `CFE_EVS_SendEventWithAppID`(`CFE_SB.AppId`) → `EVS_NotRegistered` → `EVS_SendEvent` → `EVS_IsFiltered`(0xFFFFFFFF, 범위 검사 없음). 모두 호출자(TIME)의 task 안에서 실행된다 | 원·검 | 주요 줄 직접 확인; `cfe_evs.c`·`cfe_evs_utils.c`의 중간 호출은 저(C3.md §5). |
| C3.E4 | 요구되는 순서(거친 형태) | 각 core 앱이 RunLoop에 도달한 뒤에 다음 core 앱이 시작한다(충분조건) | 원† | 이슈 목록 1 항목 1; 6.4.2 VDD Table 1.2-2 row 1(저). |
| C3.E5 | 요구되는 순서(세밀한 형태) | TIME의 `CFE_SB_CreatePipe` 이벤트 이전에 SB가 L198과 L257을 마치고 EVS가 L385와 L393을 마쳐야 한다 | 분 | 이슈나 VDD가 말하는 것이 아니라 코드에서의 추론이다(C3.md §7). |
| C3.E6 | 기존 동기화(코드 사실) | 6.4.1에서 core task는 객체 표(`cfe_es_objtab.c`)의 순서대로 생성된다(생성 순서이지 실행 순서가 아님). 설정 우선순위 값은 TIME 60, EVS 61, SB 64다(낮은 숫자가 높은 우선순위인 규약은 `cfe_platform_cfg.h` L1185–L1186의 저). ES 락이 core task 생성과 TaskTable 채우기를 감싼다(L838 락, L875 `OS_TaskCreate`, L933 해제). 6.4.1의 시작 동기화는 스크립트 앱용이며 core task는 `WaitForStartupSync`를 부르지 않는다 | 원·검 | 직접 확인(V-C3-2, V-C3-4, V-C3-6). |
| C3.E6a | 기존 동기화의 평가 | 우선순위가 나쁜 순서(TIME이 EVS·SB보다 먼저 실행됨)를 일으킨다는 것은 OSAL이 우선순위를 실제로 적용할 때(C3.C10: root일 때만)에만 성립하는 분석이다. 비루트이거나 CPU 수가 다르면 달라지고 이 조건은 C3.C9–C11에서 미정이다. ES 락이 R-ES만 보호한다는 것도 같은 분석이다 | 분 | C3.md §6; C3.C9–C11. |
| C3.E7 | 수정이 보장하는 것·않는 것 | F3 + F1 + F2로 TIME 쪽 crash 경로는 닫힌다. 그러나 6.4.2에서도 EVS가 SB보다 먼저 `CFE_SB_CreatePipe` 이벤트를 `CFE_SB.AppId=0`으로 내보낸다(EVS 앱에 오귀속되고 DEBUG 이벤트는 기본 필터로 버려짐). 수정 후 crash가 사라졌다는 사실만으로 순서 의존성이 제거됐다고 하지 않는다 | 분 | C3.md §8.3; ROADMAP §9.2. |
| C3.E8 | 수정 전후 비교의 한계 | 6.4.1→6.4.2 diff가 Trac #40·#41·#42를 함께 담으므로 한 수정의 효과는 구성된 하위집합(차이 조건)으로만 분리된다 | 분 | SH-4. |

### 5.F 정상·오류 판정 기준

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C3.F1 | 층1: API 동작 | `CFE_EVS_SendEventWithAppID`: `AppID >= CFE_ES_MAX_APPLICATIONS`이면 ILLEGAL, `RegisterFlag == FALSE`이면 `EVS_NotRegistered`, 아니면 필터 경로 | 저 | C3.md §5 U3(미재확인). |
| C3.F2 | 층2: 호출자 요구(준비 상태 전제) | TIME이 SB API 이벤트를 내기 전에 SB가 AppId를 게시하고 등록해야 하고 EVS의 초기화가 끝나 있어야 한다. EVS_AppID는 다른 초기화가 끝난 뒤 유효해야 한다(VDD row 3의 문장) | 원† | 이슈 목록 1 항목 3, 목록 2; VDD Table 1.2-2 row 3(저). |
| C3.F3 | 층3: 원 보고의 관측 결과 | Microblaze에서 "segfaults and crashes CFE core" | 원† | 이슈 목록 2 항목 7. |
| C3.F4 | 파생 -m32에서의 범위 밖 색인 | `AppData[0xFFFFFFFF]`가 `AppData[-1]`로 감겨 `CFE_EVS_GlobalData` 바로 앞 메모리를 가리킨다는 추정. 실제로 fault가 나는지는 정해져 있지 않다 | 분 | C3.md §5, A4. fault 여부는 C3.F6. |
| C3.F5 | 주 oracle | 잘못된 상태 사용(`EVS_IsFiltered`가 AppID 0xFFFFFFFF로 호출됨)과 실제 crash를 분리해 보고한다 | 선(제안) | ROADMAP §9.2 완료 기준. |
| C3.F6 | 주 oracle의 확정과 fault 여부 | crash를 필수로 할지, 부차로 할지 결정되지 않았다. `AppData[-1]`이 fault를 내는지 정해져 있지 않다 | 미 | ledger §5 X7; C3.md D4, A4. |
| C3.F7 | 오귀속 가지와 정상 후속 | 오귀속(AppId 0=EVS로 보내져 이벤트가 버려지는 가지)은 K-ID로 표현되지 않는 명세의 표현 한계다. 허용된 회복으로 읽는 판정은 근거를 명시해야 한다 | 선(QT-4, Q15) | SPEC §7.6 H-V; RESOLVED §4.1. |
| C3.F8 | 임무상 피해 | 확인된 바가 없다 | 미 | — |

### 5.G 관측 계획

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C3.G1 | 소스 대응 | 6.4.1/6.4.2에서 상태·우선순위·생성 순서·호출 경로를 대응시킨다 | 선(제안) | ROADMAP §9.2 작업 1. |
| C3.G2 | 분리해서 기록할 상태 | SB의 AppId, EVS 자신의 AppId, EVS 등록·필터 상태를 각각 | 선(제안) | ROADMAP §9.2 작업 3. |
| C3.G3 | 순서 확인 | TIME 문맥의 호출이 어느 초기화보다 먼저 일어나는지. 실제 우선순위 적용과 CPU 수를 기록한다 | 선(제안) | ROADMAP §9.2 작업 4. |
| C3.G4 | 6.4.2 변경의 해석 | 시작 순서 제어(F3)·범위 검사(F1)·공개 시점 변경(F2)으로 나누어 해석한다. 배포 버전 전체 비교에는 여러 수정이 섞였다고 표시한다 | 선(제안) | ROADMAP §9.2 작업 5. |
| C3.G5 | 실제 바이너리 위치 | 컴파일 DB 재구성과 폐쇄·수집 점검(D13 (a)(b))이 먼저다. 아직 없다 | 미 | SPEC VP-D3. |
| C3.G6 | 입력 점검 | 생성 설정, include 목록, 정수·포인터 크기, 매크로가 선택한 소스 경로, 구조체·반환값 해석 | 선(D13) | SH-11. |

### 5.H 예상 결과

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C3.H1 | 예상 결과표 | §8.4. 모든 값은 기대이며 미실행이다 | 선(제안) | ROADMAP §6.1. |

### 5.I 변경 기록

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C3.I1 | 원출처의 조건 | 보고 호출 경로와 생성 순서·우선순위(이슈의 "default/example configuration"), 원 플랫폼 Microblaze, 관측 결과 crash, 6.4.2의 수정 | 원 | 행: C3.A3–A6, C1, C3, C5, F3. |
| C3.I2 | 원출처와 다른 조건 | (1) 플랫폼을 Microblaze 대신 32-bit x86 Linux(-m32 -g -O0, posix/pc-linux)로 둔다: Microblaze의 OS·PSP·OSAL·툴체인을 구할 수 없기 때문; 영향: 스케줄링·메모리 배치·fault 동작이 다르다. (2) 현재 컨테이너의 32-bit 헤더를 쓰는 첫 파싱은 분석 전용이다(D13). (3) OSAL을 git tag로 쓰면 `osapi-version.h`가 SourceForge artifact와 다르다(SH-7). 이것을 차이 조건으로 셀지는 X3 전의 해석이다(SH-5b). (4) ENV 환경 이탈(K15)은 이 사례에는 해당하지 않으나 파생 6.4.x 환경에서 같은 종류의 이탈을 기록해야 한다. (5) 순서 제어를 도입하면 그것은 제어된 실행이다 | 선(차이) | C3.md §10, A1; RESOLVED §3.4. |
| C3.I3 | 출처가 정하지 않은 조건 / 지위가 미결정인 근거 | 미: A10, A13(X4), B3, C2, C4, C9, C11, D1, F6, F8, G5. 검색 발췌(댓글 헤더·커밋 ID·병합일)를 레퍼런스로 인정할지는 X2로 미결정이어서, 인정하지 않으면 `원†`으로 둔 A2·A7의 해당 부분이 `저`로 내려간다(§1은 `†` 행을 확정에 더하지 않는다) | 미 | ledger §5 X2. |
| C3.I4 | 이 대장의 변경 기록 | 2026-10-15 최초 작성(초안). 2026-10-16 재점검(V-C3-R1–R4): 행 변경 없음 | 저 | §11. |

## 6. C5 — nasa/cFE #72 (task 생성과 앱 등록 사이의 공백)

### 6.A 사례 정체성

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C5.A1 | 원 이슈 | nasa/cFE #72 "Race condition within CFE_ES_AppCreate". skliper, 2019-09-30T17:50:20Z 생성·6초 뒤 닫힘(completed), label bug, milestone 6.4.2, 댓글 9개 | 원† | C5.md F1(GitHub MCP 검색 본문). 이 세션에서 페이지 403. |
| C5.A2 | 원 보고의 주장 | ES가 앱을 시작할 때 `OS_TaskCreate()` 전에 `CFE_ES_UnlockSharedData()`를 부르고 task ID를 얻은 뒤 다시 잠가 전역 표를 갱신한다. 이 공백에서 자식 스레드가 아직 채워지지 않은 표를 읽을 수 있고, 많은 task가 초기에 부르는 `CFE_ES_GetAppID()`가 실패해 "snowball effect"가 생긴다. `CFE_ES_CreateObjects`와 `CFE_ES_CreateChildTask`는 전 절차에 락을 유지해 안전하다 | 원† | C5.md F1. |
| C5.A3 | 공개 코드와 연결: 버그 쪽 | 6.4.1 `cfe_es_apps.c`: L571 락, L610 해제, L614 `OS_TaskCreate`, L637 재획득, L649 `RecordUsed = TRUE`, L651 `AppId = i`, L673 해제. 6.4.0과 이 파일이 같다 | 원·검 | 6.4.1 tarball 직접 확인; 파일 sha256 `0a54974b…`·git blob `8c6d0417…`. |
| C5.A4 | 공개 코드와 연결: 사용 지점 | `CFE_ES_GetAppID`(`cfe_es_api.c` L846–L871): `TaskTable[OS_TaskGetId()].RecordUsed == TRUE`가 아니면 `CFE_ES_ERR_APPID` 반환 | 원·검 | 직접 확인. |
| C5.A5 | 이슈의 '안전' 주장 | `CFE_ES_CreateObjects`는 락 L838 → `OS_TaskCreate` L875 → 등록 → 해제 L933으로 락을 유지한다 | 원·검 | 직접 확인(`cfe_es_start.c`). `CFE_ES_CreateChildTask`는 저(C5.md F7). |
| C5.A6 | 공개 코드와 연결: 수정 쪽 | 6.4.2 `cfe_es_apps.c`: L571 락, L607 `++AppStartedCount`, L619 `OS_TaskCreate`, L669 해제. 한 임계구역이 task 생성과 등록을 감싼다 | 원·검 | 직접 확인; git blob `94362464…`. |
| C5.A7 | 6.4.2 VDD의 설명 | Trac #41: "The unlock-relock code has been removed and updated to be the same as other areas where tasks are spawned." | 저 | C5.md F13(미재확인). |
| C5.A8 | #72 = Trac #41 | 이슈 제목·milestone·VDD 서술이 이슈 본문과 거의 같고 다섯 티켓이 모두 +31 오프셋으로 대응한다 | 분 | C5.md F13("strongly supported"). 명시적 마이그레이션 헤더는 없다. |
| C5.A9 | 연결되지 않는 부분 | 관측자의 플랫폼·OSAL·대상 앱·앱 생성 경로·우선순위·루트/RT·CPU 수·실패 로그·빈도. 이슈 #72는 플랫폼을 말하지 않는다(형제 이슈 #71·#73이 EVA Microblaze를 말하지만 #72 자체는 아니다) | 미 | C5.md F12, §10. |
| C5.A10 | 이슈 댓글 9개 | [C5-fix] `issuecomment-536673810`, [C5-context] `536673820` 포함 미열람. 입력 로드맵 `roadmap_input_20261007.md`(L394, L630)가 [C5-context]로 인용하는 "런타임 부하·watchdog 서비스 기한" 서술은 미검증이다. `plan/ROADMAP_20261015.md`에는 이 서술이 없다(grep 확인) | 미 | C5.md F2, §13. |
| C5.A11 | 현재 코드 | 원 패턴은 없다. `0749d692`(2020-10-16)부터 `OS_TaskCreate`는 락 밖이고 새 task의 진입 래퍼가 등록 완료까지 대기하는 가드이며, `58abd38d`(2021-02-02)부터 그 대기가 **task 레코드만** 확인한다. `0749d692`의 설계 주석은 "…before its fully registered in the global app table"이라는 같은 요구를 적는다 | 원·검 | 주석·`GetTaskFunction` 직접 확인(`0749d692`, `58abd38d`). |
| C5.A12 | 현재 코드의 잔여 창 후보 | 현재 main(`12cb84fc`)에서 task 레코드 게시(L672–L722) 후 App 레코드는 아직 `RESERVED`이고 두 번째 임계구역(L861 락, L869 `AppRecordSetUsed`)에서야 최종화된다. 그 사이에 새 task의 폴링이 성공하면 `GetAppID`가 실패할 수 있다는 추정. 미실행 | 분 | 코드 줄은 `검`(L664·L672·L722·L861·L869). 도달 가능성은 C5.md D4. |
| C5.A13 | 원 C5와 현재 잔여 후보의 분리 | 현재 버전의 App-record 잔여는 원 C5의 대체 검증이 아니고 별도 파생 결과로 보고한다. 원 C5(#72)의 과거 버전 lock 공백 검증이 먼저다 | 선(D16, P-RGIT, ROADMAP §9.3) | RESOLVED §3.6, §4.1. |
| C5.A14 | P-RGIT 보조 대조 | `58abd38d^`/`58abd38d`는 현재 설계가 예상한 근거의 생성·철회를 점검하는 별도 소스 이력 쌍이다. 추가 입력 대응 비용이 확인된 뒤 수행하며 이미 설계 문서에 알려진 쌍이므로 맹검 결과로 보고하지 않는다 | 선(P-RGIT) | RESOLVED §3.6. |
| C5.A15 | C3와의 관계 | 같은 수정 릴리스·Trac #40–#42·형제 이슈. C3의 core task는 `CreateObjects` 경로여서 C5의 창에 노출되지 않고, C5는 SB/EVS AppId와 무관하다 | 분 | ledger §3.2. |
| C5.A16 | 'race' 서술 | 이슈 제목의 "Race condition"을 도구 class로 자동 변환하지 않는다 | 선(P-CLASSGT) | RESOLVED §4.3. |

### 6.B 소스 기준

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C5.B1 | 버그 쪽 | 6.4.1 tarball(SH-1). 6.4.0과 `cfe_es_apps.c`가 같다 | 원·검 | §2.2. 6.4.0은 이 문서에서 받지 않았다(파일 동일성은 저). |
| C5.B2 | 수정 쪽 | 6.4.2 tarball(SH-2) | 원·검 | §2.2. |
| C5.B3 | OSAL | SH-5–SH-8. 6.4.1 readme의 요구·출처·예시 파일 이름은 SH-5에 인용했다. SourceForge artifact(sha256 `8f0f2b23…`)와 git tag는 `osapi-version.h`만 다르다(SH-6, SH-7). 이것을 차이 조건으로 셀지는 해석이며 X3 결정 전이다(SH-5b) | 원·검 | §2.2. |
| C5.B4 | OSAL 4.1.1 posix | `*task_id = possible_taskid`(L690)는 `pthread_create`(L645) 뒤라서 task가 `OS_TaskCreate` 반환 전에 실행될 수 있다. `geteuid() == 0`일 때만 우선순위(L587) | 원·검 | 직접 확인. |
| C5.B5 | PSP | 6.4.1 tarball에 동봉된 pc-linux PSP 1.2.0.0 | 저 | C5.md §3(미재확인). |
| C5.B6 | 관측자의 OSAL·target·host | 정해진 바가 없다 | 미 | C5.md §10 #4·#5·#20. |
| C5.B7 | "6.4.1에서 발견" | 6.4.2 VDD §1.1: 시작 경쟁 시리즈가 6.4.1에서 발견됐다. 범위는 시리즈이며 #41 단독이나 플랫폼은 말하지 않는다 | 저 | C5.md F13(미재확인). |
| C5.B8 | 기준 결정 | D13(분석 전용 파싱 인정, (b)(i)부터, F1 전 배치), Q16(버전별 대응) | 선(D13, Q16) | SH-10–SH-14. |
| C5.B9 | 'present in current code'의 기준 리비전 | ENV c5fb2b4d, main `12cb84fc`, dev `f22e538c` 중 무엇인가 | 미 | ledger §5 X6. |

### 6.C 실행 조건

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C5.C1 | 관측자의 환경 전부 | 정해진 바가 없다 | 미 | C5.md §10. |
| C5.C2 | 파생 플랫폼 | SH-9–SH-12. 현재 컨테이너(Ubuntu 24.04)는 6.4.1 readme가 '컴파일한 곳'으로 적은 목록(SH-9) 밖이다. readme는 다른 최근 리눅스에서도 빌드할 수 있을 것이라고 적는다. 목록 밖에서 만드는 것을 차이 조건으로 기록할지와 승인은 열린 선택이다(SH-16) | 선(D13) | C5.md D3a. |
| C5.C3 | 빌드 절차의 모호함 | readme의 `mv osal-4.1.1-release osal`은 tarball에 이미 `osal/`(readme.txt만)이 있어 그대로 하면 `osal/osal-4.1.1-release`가 된다는 추정이며, 레퍼런스가 어떻게 해소하는지 적지 않았다 | 미 | C5.md §10 #6. 조정은 조용히 적용하지 않고 기록한다. |
| C5.C4 | 권한·실시간 | readme가 인정하는 두 선택(`msg_max` 상향 또는 root)을 둘 다 고르지 않았다. 관측에서 RT를 썼는지도 불명 | 미 | C5.md §10 #10; SH-16. |
| C5.C5 | 창이 열리는 조건 | pc-linux 시작 경로에서 root이고 단일 CPU이면 새 task가 생성자(`SCHED_FIFO` 최대)를 선점하지 못해 창이 열리지 않을 수 있다는 추정. 다중 CPU, 비루트, 또는 런타임 Start App 경로가 필요할 수 있다 | 분 | C5.md §6. |
| C5.C6 | CPU 수 | 정해진 바가 없다 | 미 | SH-17. |
| C5.C7 | 앱·생성 경로 | 시작 스크립트, Start App 명령, 재시작·재로드 중 무엇인지, 대상 앱과 그 첫 `GetAppID` 의존 호출이 정해지지 않았다. 샘플 앱의 `RegisterApp → EVS_Register → SB_CreatePipe` 패턴을 쓸 수 있으나 보고자의 앱이 아니다 | 미 | C5.md §10 #7·#8. |
| C5.C8 | 순서 제어 | 사건 기반: `OS_TaskCreate` 반환 직후부터 재획득(L637) 전까지 생성자를 보류하고 새 task의 `GetAppID`가 관측되면 해제한다. 수정 쪽 대조의 보류 위치는 수정 코드의 락을 위반하지 않는 곳이어야 한다. 고정 sleep 금지 | 선(제안) | C5.md A6. 추가 링크 변경 실행 제어는 후순위(D3). |
| C5.C9 | 보류의 감시 한도 | Φ_6.4.x에서 읽을 수 있으면 읽고, 아니면 D13의 하위 항목이다 | 미 | SPEC VP-D3. |
| C5.C10 | `OS_TaskGetId` 순서(A8) | `pthread_create`가 `OS_task_table[i].id`를 쓰는 시점과 새 스레드의 `OS_TaskRegister` 조회 순서가 "GetAppID 실패"와 "잘못된 AppID"를 가른다 | 미 | C5.md A8. |
| C5.C11 | oracle의 범위 | 새 task의 `GetAppID`가 실패하고 그 task의 TaskTable 항목이 `RecordUsed`가 아닌 동안으로 한정할지는 선택이다(잘못된 AppID 가능성, stale 항목) | 선(제안) | C5.md A7. 승인 필요. |
| C5.C12 | 관측 방식·N | K11(D1), K12(D14) | 선(D1, D14) | §2.1. |

### 6.D 초기 상태

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C5.D1 | 관측의 reset 종류 | 정해진 바가 없다. 파생 반복 실행: 인수 없는 `./core-linux.bin`은 PO. Ctrl-C 뒤 다음 시작은 반드시 PO(6.4.2 readme) | 미 | C5.md §10 #21. 파생 부분의 읽기는 저. |
| C5.D2 | stale `TaskTable` 항목 | 이전 task의 `RecordUsed=TRUE`가 남아 있을 때(삭제·재시작) 창 안의 `GetAppID`가 이전 `AppId`와 성공을 반환할 수 있다는 가설. 도달 가능성은 확인되지 않았다 | 미 | C5.md A8. |
| C5.D3 | 샘플 시작 스크립트 | cpu1 샘플: `sample_lib`(CFE_APP로 적혀 있음, 우선순위 0·스택 0), SAMPLE_APP 50, CI_LAB 60, TO_LAB 70, SCH_LAB 80. 모두 `AppCreate`를 거친다. `CFE_LIB`로 "고치면" 차이 조건이다 | 저 | C5.md §6(미재확인). |
| C5.D4 | 큐·종료 | SH-9, SH-16(미결정) | 미 | — |

### 6.E 의존성

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C5.E1 | 자원 | `CFE_ES_Global.TaskTable[TaskId]`(`RecordUsed`, `AppId`, `TaskId`, `TaskName`). 인덱스는 OSAL task ID이고 `OS_TaskGetId()`로 찾는다 | 원·검 | C5.A3, A4. |
| C5.E2 | 게시 지점 | L649–L655의 쓰기. 다른 task가 쓸 수 있게 되는 시점은 해제(L673). 새 task는 `OS_TaskCreate`(L614) 이후부터 실행 가능하다 | 원·검 | 6.4.1; OSAL `pthread_create` L645는 C5.B4. |
| C5.E3 | 사용 지점 | `CFE_ES_GetAppID`, `GetAppIDInternal`을 쓰는 `CFE_ES_RunLoop`·`WaitForStartupSync` 등, `EVS_GetAppID` ← `CFE_EVS_Register`, `CFE_SB_CreatePipe`(상태 무시) | 원·검 | `GetAppID`는 직접 확인. 나머지는 저(C5.md §5). |
| C5.E4 | 요구되는 순서(원출처) | 새 앱 main task T의 최초 읽기 전에 생성자의 TaskTable 등록이 완료되고 해제돼야 한다. 읽기가 "fully populated" 전이면 문제다 | 원† | 이슈 본문; VDD Table 1.2-2 item 2(저). |
| C5.E5 | 위반의 정의 | 위반 = 등록 직전의 task에 대해 `GetAppID`가 실패를 반환하는 것(등록 전 조회와 그 결과). 새 task가 단지 먼저 실행된 사실은 위반이 아니다 | 선(제안) | ROADMAP §9.3 완료 기준. |
| C5.E6 | 기존 동기화(코드 사실) | 6.4.1 `cfe_es_apps.c`에서 ES mutex는 L610에서 풀리고 L637에서 다시 잡힌다. task 생성(L614)이 등록(L637–L673)보다 먼저다 | 원·검 | 락 줄은 직접 확인(V-C5-1). |
| C5.E6a | 기존 동기화의 평가 | OSAL task-table mutex는 OSAL 자신의 표만 보호하고, 시작 동기화는 새 task의 `GetAppID`와 자기 등록의 순서를 정하지 않는다 | 분 | OSAL 락 범위·mutex 프로토콜은 저(C5.md §6, 미재확인). 나머지는 C5.E6의 코드 사실에서의 해석. |
| C5.E7 | 수정의 종류 | 락 범위 확장(순서 보장). 락 없이 읽는 경로(`CFE_ES_IncrementTaskCounter`)는 보호하지 못한다. 현재 코드는 가드(등록 완료 대기 + timeout)다 | 분 | C5.md §8, §9. |
| C5.E8 | 다른 사례와의 관계 | "reserve, then finalize" 형태는 C1·R2와 가설 수준으로만 공유된다. R2의 SB AppId 미게시가 C5의 App-record 창으로도 생길 수 있다는 추정은 별도 파생 후보이다 | 분 | ledger §3.4, §3.6. |

### 6.F 정상·오류 판정 기준

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C5.F1 | 층1: API 동작 | `CFE_ES_GetAppID`는 현재 task의 TaskTable 항목이 사용 중이 아니면 `CFE_ES_ERR_APPID`를 반환한다(6.4.1). 현재 코드는 App 레코드가 일치하지 않으면 `CFE_ES_ERR_RESOURCEID_NOT_VALID`를 반환 | 원·검 | 6.4.1 직접 확인. 현재 코드의 반환 코드 이름은 저(C5.md §9). |
| C5.F2 | 층2: 호출자 요구(준비 상태 전제) | `GetAppID` 호출 시점에 호출 task의 TaskTable 등록이 완료돼 있어야 한다. 이슈 본문, VDD(저), 그리고 `0749d692`의 설계 주석("any other function that depends on having an AppID context, before its fully registered in the global app table") | 원·검† | 주석은 직접 확인; 이슈는 †. |
| C5.F3 | 층3: 관측 결과 | `GetAppID`가 `CFE_SUCCESS`가 아닌 값을 반환하거나 `RunLoop`가 false를 반환("Cannot get AppID for the caller"). 하류 영향("snowball")은 정해진 바가 없다 | 원† | 이슈 본문; 하류는 미(C5.md §10 #12). |
| C5.F4 | 하류 영향 | 정해진 바가 없다 | 미 | — |
| C5.F5 | 기대에 넣지 않는 것 | 원 보고에 없는 앱 crash를 기대 결과에 넣지 않는다 | 선(제안) | ROADMAP §9.3 작업 5. |
| C5.F6 | 정상·오류 라벨 | 원출처가 허용 회복을 말하지 않는 항목은 미판정이며 코드상 상태 변화·오류 반환은 보고한다 | 선(QT-4, Q15) | RESOLVED §4.1. |
| C5.F7 | 임무상 피해 | 확인된 바가 없다 | 미 | — |

### 6.G 관측 계획

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C5.G1 | 소스 대응 | 6.4.1/6.4.2와 대응 OSAL에서 생성·진입·task ID 반환·ES 등록·조회 경로를 확인한다 | 선(제안) | ROADMAP §9.3 작업 1. |
| C5.G2 | 관측 계획에 반영할 점 | task가 `OS_TaskCreate` 반환 전에 실행될 수 있다(OSAL 4.1.1 posix: `pthread_create` L645, ID 반환 L690) | 선(제안) | ROADMAP §9.3 작업 2; 줄은 `검`. |
| C5.G3 | 기록기의 식별 | 기록기가 아직 성공하지 않은 `GetAppID`에 자기 식별을 의존하지 않도록 OS task ID와 생성 관계를 임시 식별로 쓰고 나중에 앱 정보와 연결한다 | 선(제안) | ROADMAP §9.3 작업 3. |
| C5.G4 | 연결할 사건 | 새 task 진입, ES 등록 공개 완료, 조회 결과, 락 범위 | 선(제안) | ROADMAP §9.3 작업 4. |
| C5.G5 | 수정 전후 비교 | 첫 조회가 실패하는 경로가 달라지는지 확인하고, 수정 후에는 선택한 분석·탐색 범위에서 해당 경로가 제거됐는지 확인한다. 전체 프로그램의 안전성으로 확대하지 않는다 | 선(제안) | ROADMAP §9.3 작업 5, 완료. |
| C5.G6 | 현재 버전 잔여 후보 | 별도 결과로 관리한다 | 선(D16) | RESOLVED §4.1. |
| C5.G7 | 실제 바이너리 위치 | 컴파일 DB 재구성과 입력 점검 뒤 | 미 | SPEC VP-D3. |

### 6.H 예상 결과

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C5.H1 | 예상 결과표 | §8.5. 모든 값은 기대이며 미실행이다 | 선(제안) | ROADMAP §6.1. |

### 6.I 변경 기록

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| C5.I1 | 원출처의 조건 | 버그 쪽 6.4.1(·6.4.0)의 락 해제–재획득 구조, `GetAppID`의 실패 조건, 이슈의 안전 주장, 6.4.2의 락 범위 확장 | 원 | 행: C5.A1–A6, E4. |
| C5.I2 | 원출처와 다른 조건 | (1) 플랫폼은 파생이다(관측 플랫폼이 정해져 있지 않으므로 편차 기준이 없지만, 현재 컨테이너 Ubuntu 24.04와 32-bit 헤더 사용은 readme의 빌드 호스트 목록 밖이다). (2) OSAL을 git tag로 쓰면 `osapi-version.h`가 SourceForge artifact와 다르다(SH-7). 이것을 차이 조건으로 셀지는 X3 전의 해석이다(SH-5b). (3) 순서 제어는 제어된 실행이다. (4) 현재 버전 잔여 후보는 원 C5가 아닌 파생이다. (5) 분석 전용 파싱은 실행 재현이 아니다 | 선(차이) | C5.md §3, A2·A3; RESOLVED §3.4. |
| C5.I3 | 출처가 정하지 않은 조건 | A9, A10, B6, B9, C1, C3, C4, C6, C7, C9, C10, D1(관측), D2, D4, F4, F7, G7 | 미 | 위 행의 `미`. |
| C5.I4 | 이 대장의 변경 기록 | 2026-10-15 최초 작성(초안). 2026-10-16 재점검(V-C5-R1–R5): 행 변경 없음 | 저 | §11. |

## 7. R2 — nasa/cFE #2663 (SB AppId가 게시되기 전의 사용)

R2의 실행 근거는 gdb 원시 기록(`env/logs/r2_g1..3_*`)과 `results/R2_gdb_v701.md`뿐이다. 다른 도구의 기록은 근거로 쓰지 않는다. R2는 첫 전체 구현 대상이다(P-LADDER).

### 7.A 사례 정체성

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| R2.A1 | 원 이슈 | nasa/cFE #2663 "Unregistered AppId error with CFE_EVS_SendEventWithAppID() during cFE task startup". myc-yang, 2025-08-06 열림, open, label bug, 담당·milestone 없음, 댓글 2개 | 원† | R2.md §1.2. 웹 조각 인용. 이 세션에서 페이지 403. |
| R2.A2 | 원 보고의 주장 | ES와 EVS task가 초기화할 때 SB API(`CFE_SB_CreatePipe()`, `CFE_SB_Subscribe()`)를 부르고, 이들이 SB의 AppId로 `CFE_EVS_SendEventWithAppID()`를 호출해 `CFE_EVS_APP_ILLEGAL_APP_ID`가 반환된다("presumably because CFE_SB has not been registered with event services"). 이벤트 출력도 없고 반환 코드도 변수에 설정되지 않는다 | 원† | R2.md §1.2. |
| R2.A3 | 원 보고의 재현 서술 | `make SIMULATION=native prep` / `make` / `make install`; gdb로 `cfe_evs.c#L185`(`main` 링크)에 중단점; "Run the gdb and see it stopped at the breakpoint". OS Ubuntu 22.04.5 LTS, "cFE v6.7". backtrace·hit 수·반환값 출력은 없다 | 원† | R2.md §1.2. |
| R2.A4 | 공개 코드와 연결 | ENV(cFE `c5fb2b4d`)에서 대응하는 위치: 게시 `cfe_sb_task.c` L138, 거부 `cfe_evs.c` L186–L188, ES init의 SB 호출 `cfe_es_task.c` L367·L377·L387, EVS init `cfe_evs_task.c` L289·L297·L304, 초기값 `cfe_sb_init.c` L50의 `memset`, SB 이벤트 보고 `cfe_sb_priv.c` L786, 직렬화 `cfe_es_start.c` L810·L835 | 원·검 | ENV 고정 SHA 파일 직접 확인. 보고의 L185는 내용 이동으로 L188이 됐다. |
| R2.A5 | 보고가 가리킨 리비전 | "cFE v6.7"과 `blob/main`의 링크뿐이다. 후보 B = `fc8c48ad09460ce0277ad2bfcd81d376634aa2c6`(보고 날짜의 main) | 미 | B가 보고 시점의 main이라는 것은 git 이력 읽기에 의한 추정이다(R2.A6). |
| R2.A6 | B의 코드와 가설 | B의 `cfe_version.h`는 6.7.0 개발 버전(`CFE_BUILD_NUMBER 247`, `CFE_LAST_OFFICIAL "v6.7.0"`)이고, `cfe_evs.c` L182–L186에 `ILLEGAL_APP_ID` 할당, `cfe_sb_task.c` L126이 게시, 정적 객체 표의 core task 순서가 EVS, SB, ES, TIME, TBL이다. "보고 날짜의 main이 B였다"는 가정은 분석이다 | 분 | 코드는 `검`. 날짜 대응은 R2.md §3(무재작성 가정). |
| R2.A7 | 수정 | 없다. #2663은 open이고 nasa/cFE에 이 이슈를 참조하는 커밋이 없다. PR #2820(philphauler, base dev)은 use-site fallback을 제안했고 작성자가 병합 없이 닫으며 "the fix is vacuous"라고 했다 | 저† | R2.md §8, §1.4(PR 페이지 미재확인). |
| R2.A8 | 코멘트 2개 | CCB 코멘트("CCB 4 September 2025: Tracked internally…")와 분석 코멘트(SB가 `CFE_SB_Global.AppId`로 이벤트를 보내는데 그 필드는 `CFE_SB_AppInit`에서만 채워지므로 그보다 먼저 도는 core 앱이 정의되지 않은 AppId를 본다는 취지). 검색 결과 발췌에서 읽었고 일부 구간이 빠져 있으며 모델이 전달한 텍스트다. 인용 전에 브라우저에서 확인해야 한다 | 원† | R2.md §1.3. 발췌의 지위는 X2 미결정. |
| R2.A9 | 근거 종류의 구분(QT-2) | 구독자 없는 송신에 대한 SB 규정(`CFE_SB_SEND_NO_SUBS_EID`는 SB가 보고하는 이벤트)과 R2의 유효하지 않은 AppId 오류(`ILLEGAL_APP_ID`)를 혼동하지 않는다. 근거 종류에 `requirement`를 추가한다 | 선(QT-2) | RESOLVED §2, §5. |
| R2.A10 | 결과의 표현 | 24회는 "이벤트 요청 24회가 거부됨"이다. `CFE_EVS_SendEventWithAppID`는 유효하지 않은 AppId에서 이벤트 패킷을 만들기 전에 오류를 반환한다. 이미 만들어진 텔레메트리 패킷 24개의 손실이 아니다 | 원·검 | `cfe_evs.c` L183–L204: 유효하지 않은 AppId의 거부 가지(L188)가 먼저이고, 이벤트 패킷 생성(`EVS_GenerateEventTelemetry`, L204)은 그 뒤의 `else if` 가지에서만 일어난다. |
| R2.A11 | 'race' 구분 | R2에는 원출처의 class 서술이 없다. 고정 순서라는 읽기는 코드 읽기와 3/3 관측에 기대고 있으며 관측은 증명이 아니다 | 분 | SPEC §8.2; R2_gdb §3, §6. |
| R2.A12 | C3와의 관계 | SB AppId 준비 자원 계열을 공유한다. 한 패턴으로 셀지 두 사례로 셀지 미정이며, R2의 관측을 C3의 crash로 바꿔 쓰지 않는다 | 분 | ledger §3.1, §5 X4. |

### 7.B 소스 기준

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| R2.B1 | R2의 첫 기준 | 현재 기본 구성(ENV, cFS v7.0.1)을 R2의 첫 전체 구현 기준으로 한다. 다른 네 사례를 이것으로 대체하지 않는다 | 선(P-LADDER) | RESOLVED §2; ROADMAP §6.2. |
| R2.B2 | ENV 리비전 | K1. cFE `c5fb2b4d` = tag v7.0.1 | 저·검 | §2.1. |
| R2.B3 | ENV와 main M(`12cb84fc`)의 관계 | `cfe_sb_task.c`, `cfe_evs.c`, `cfe_es_task.c`, `cfe_evs_task.c`, `cfe_es_start.c`에서 `c5fb2b4d`와 M은 차이가 없어 M에 대해 기록한 코드 사실이 ENV에 그대로 적용된다 | 저 | mapping_v701 §0(`git diff --stat` 공백). 이 문서에서 diff를 다시 하지 않았다. |
| R2.B4 | B와 ENV의 core 순서 | B: 정적 객체 표 EVS, SB, ES, TIME, TBL. ENV: `MISSION_CORE_MODULES` config, es, evs, fs, sb, tbl, time("the order of this list is the order in which they will be initialized"). ENV에서 ES와 EVS가 모두 SB보다 먼저다 | 원·검 | 양 끝점의 코드를 직접 확인. |
| R2.B5 | 순서가 바뀐 커밋 | `b9bec5679c748d6a7c1181d74a2f52dd6ba70688`(저자 시각 2026-03-26, 커밋 2026-04-07, "Fix #197, dynamic object table creation…"), 부모 `063cb3022abd7b8193280fad9552fb11f194b47b`. 부모의 정적 객체 표 순서는 EVS, SB, ES, TIME, TBL이고 `b9bec567`에서 `MISSION_CORE_MODULES`가 config, es, evs, fs, sb, tbl, time이다. **이 커밋은 52개 파일을 바꾸며(`cfe_es_start.c`, `cfe_es_apps.c`, `cfe_evs_task.c`, 각 모듈의 objtab 등) object table 생성 방식 전체가 바뀐다** | 원·검 | 얕은 blobless git fetch로 확인. 사례 기록에는 약식 `b9bec567`만 있었고 전체 SHA는 이 문서가 처음 적는다(§9). |
| R2.B6 | 1차 대조 (c2) | `b9bec567^`와 `b9bec567`의 자연 회귀 쌍. 관련 시작 순서와 영향받는 task의 변화, 다른 동시 변경을 먼저 확인한다(R2.B5가 그 확인의 시작이다) | 선(P-R2CTRL) | RESOLVED §3.1. |
| R2.B7 | 보조 대조 (c1) | `v7.0.0`과 현재 기준의 비교. 배포 버전 전체가 달라지므로 시작 순서만 바꾼 실험으로 해석하지 않는다 | 선(P-R2CTRL) | RESOLVED §3.1. |
| R2.B8 | 선택 대조 (b) | PR #2820. 원 PR의 변경과 현재 버전으로 이식한 변경을 구분한다. '해당 변경을 적용한 무변화 대조'로 쓰고 "PR을 되돌린"이라는 표현은 쓰지 않는다 | 선(P-R2CTRL) | RESOLVED §3.1. |
| R2.B9 | 대조에 넣지 않는 것 | 직접 만든 core 순서, lazy resolution 구현, 모델 변형은 첫 검증에 필수가 아니다. 자연 쌍으로 확인할 수 없는 질문이 남을 때 추가한다 | 선(P-R2CTRL) | RESOLVED §3.1. |
| R2.B10 | 판정 기준(대조) | 순서 근거가 달라진 부분에서 기존 고정 오류 판정이 적절히 철회되는지 확인한다. 철회됐다는 사실만으로 전체 안전성을 인정하지 않는다. 구성·버전 차이는 같은 바이너리의 스케줄 의존성의 증거가 아니다 | 선(P-R2CTRL) | RESOLVED §3.1. |
| R2.B11 | PR #2820 head | `5bc6a792a0d671717e9ccff86dd67efe4ca45f3a`(base `nasa:dev`) | 저† | R2.md §1.4. |
| R2.B12 | 보고자의 OSAL·PSP·번들 | 정해진 바가 없다. 후보: 2025-07-16 번들 `9c786d25`가 cfe `18651ef6`(트리가 B와 같음)·osal `3bcb1377`·psp `9173cba3`을 고정 | 분 | R2.md §3. 후보이며 결정이 아니다. |
| R2.B13 | '현재 코드에 있다'의 기준 리비전 | ENV `c5fb2b4d`, main `12cb84fc`, dev `f22e538c` 중 무엇인가 | 미 | ledger §5 X6. |

### 7.C 실행 조건

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| R2.C1 | 보고자의 OS | Ubuntu 22.04.5 LTS | 원† | 이슈 "System observed on". |
| R2.C2 | 보고자의 cFE | "cFE v6.7" | 원† | 이슈. |
| R2.C3 | 보고자의 빌드·관측 | `make SIMULATION=native prep` / `make` / `make install`, gdb 중단점(`cfe_evs.c#L185`). 중단점 줄은 `main`의 줄이다 | 원† | 이슈. |
| R2.C4 | 보고자의 나머지 조건 | CPU 아키텍처·VM 여부, cFE/OSAL/PSP/번들 커밋, PSP(후보 pc-linux), 번들 README 설정 사용 여부, `BUILDTYPE`·`ARCH`·`O`, `OMIT_DEPRECATED`, `ENABLE_UNIT_TESTS`, 플랫폼 설정값, 컴파일러·gdb 버전, 권한, CPU 수·affinity, 시작 스크립트, gdb 기동 방식과 `core-cpu1` 옵션, gdb 모드·중단점 범위·hit 수, 실행 횟수 | 미 | R2.md §3, §10. |
| R2.C5 | ENV 기준 실행 | K3–K6: cFS v7.0.1, `make native_std.*`, 일반 사용자 `ubuntu`, Ubuntu 24.04.4 | 선(P-LADDER) | 보고자의 빌드와 다른 점은 R2.I2. |
| R2.C6 | 기존 gdb 증거의 관측 조건 | gdb 15.1 `-batch -nx`, all-stop, 각 중단점이 JSON 한 줄을 기록하고 계속한다. ASLR 기본(꺼짐), 쉘 경유 시작. 중단점 `cfe_evs.c` L188(보고의 L185가 내용으로 L188로 이동). 추가 관측점 `cfe_sb_task.c` L138·L142·L156, `CFE_EVS_SendEventWithAppID` 진입, `cfe_evs.c` L193. 실행 길이는 `operational`, 종료는 CI ES 재시작(POWERON) | 저·M | R2_gdb §1; mapping_v701 §1.3. 방법(gdb 중단점)은 보고자의 방법이다. |
| R2.C7 | gdb의 버전·모드 | 쓴 값: gdb 15.1, all-stop(R2.C6). 보고자의 버전·모드는 정해져 있지 않아(R2.C4) 이 값은 어느 레퍼런스에도 없는 값을 고른 것이므로 차이로 기록한다(초안은 이 행을 `미`로 적었다) | 선(차이) | R2.md A4, A6; `results/R2_gdb_v701.md` §1; SPEC_FINAL 머리말의 규칙. |
| R2.C8 | gdb의 교란 | 모든 중단점 적중이 전 스레드를 잠시 멈춘다. OPERATIONAL 도달이 약 1.6 s에서 2693/2244/2257 ms로 늦어졌다(gdb 없이는 1613–1636 ms) | 저·M | R2_gdb §1. |
| R2.C9 | gdb 실행 횟수 | 3(r2_g1–r2_g3). 보고자의 횟수는 정해져 있지 않고(R2.C4) 3은 어느 레퍼런스에도 없는 값을 고른 것이므로 차이로 기록한다. 실험 N이 아니다(K13) | 선(차이) | `results/R2_gdb_v701.md` §1. |
| R2.C10 | Kairos 자체 기록기 | root 권한 관측 프로세스, perf-sample 백엔드부터, 프로브: `CFE_EVS_SendEventWithAppID` 진입·반환, `cfe_sb_task.c:138` 저장, `MainTaskSyncDelay` 종료, core task spawn(SPEC S1-R2). 기록기는 EVS·SB·TIME·ES `GetAppID`를 쓰지 않는다 | 선(제안) | SPEC §9.3 S1-R2; R2.md A6. 이 행은 설계 제안이며 실행의 기록이 아니다. **초안의 '아직 구현되지 않았다'는 낡았다:** SPEC S1 기록기의 R2 첫 실행 `env/logs/s1r2_p1*`(2026-10-15T11:17Z, `kz observe`)가 있다. 이 문서는 파일 이름·시각·머리말만 확인했고 내용과 일치 여부를 평가하지 않았다. 판정기·증명 확인기의 구현 상태는 확인하지 않았다. 시트 동결 순서는 §8.1 규칙 10. |
| R2.C11 | 시작 스크립트·외부 앱 | 앱은 `CORE_READY` 이후에 시작해 ES/EVS init의 SB 호출에 도달할 수 없다는 추정. ENV 스크립트 앱은 15개 | 분 | R2.md §10; R2_gdb §2. |
| R2.C12 | 관측 방식·N | K11(D1), K12(D14). 실행 횟수 값은 K13(미) | 선(D1, D14) | §2.1. |
| R2.C13 | 이전 도구의 결과 | R2의 관측 근거는 gdb 원시 기록뿐이다. 폐기된 기록기의 결과·성능 수치·순서 분석은 근거로 쓰지 않는다 | 선(D12) | RESOLVED §4.1, §6.2. |

### 7.D 초기 상태

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| R2.D1 | reset과 종료 | gdb 3회 모두 POWER ON으로 시작하고 CI ES 재시작(POWERON)으로 정상 종료했다(exit 0). 이후 정규화 부팅의 기준은 D17 | 저·M | R2_gdb §1; K9(D17). |
| R2.D2 | 시작 시 유지 상태 | `/dev/shm/osal:RAM/cfe_sb_pipe.dat`(1624 B, M1 B3가 남김) 존재, `EEPROM.DAT`는 모두 0, SysV 세그먼트 없음. 정리하지 않았다 | 저·M | R2_gdb §1; `env/logs/r2_g*_persist_before.txt`. |
| R2.D3 | `CFE_SB_Global.AppId`의 초기값 | `CFE_SB_EarlyInit`의 `memset(&CFE_SB_Global, 0, …)`(`cfe_sb_init.c` L50)로 0 = `CFE_ES_APPID_UNDEFINED` | 원·검 | 직접 확인. |
| R2.D4 | 게시된 값 | 3회 모두 `1114115`(0x110003) | 저·M·검 | 원시 기록 재집계(`SB_PUB.sb_appid`). |
| R2.D5 | 큐 | `msg_max=10`, 기동 중 큐 가득 참 손실 | 선(D19) | K8, K8b. |
| R2.D6 | 보고자의 reset·유지 상태·큐 | 정해진 바가 없다 | 미 | R2.md §10. |

### 7.E 의존성

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| R2.E1 | 자원 | `CFE_SB_Global.AppId`: SB API가 이벤트를 낼 때 쓰는 이벤트 소스 ID. 별개의 상태로 SB의 EVS 등록(`cfe_sb_task.c` ENV L155, B L193)이 있다 | 원·검 | 둘을 서로 다른 상태로 구분한다(ROADMAP §7.1). |
| R2.E2 | 게시 지점 | `CFE_SB_AppInit`의 첫 문장 `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`(ENV L138, B L126, dev L128), 반환값은 무시된다. 이 저장을 보호하는 락·배리어는 없다 | 원·검 | 직접 확인(반환 무시는 코드 형태). |
| R2.E3 | 사용 지점 | 호출자 task 안에서 실행되는 SB API: `CFE_SB_CreatePipe`의 PIPE_ADDED, `CFE_SB_SubscribeFull`의 SUBSCRIPTION_RCVD, `CFE_SB_MessageTxn_ReportSingleEvent`(SEND_NO_SUBS; `cfe_sb_priv.c` L786)가 `CFE_EVS_SendEventWithAppID(…, CFE_SB_Global.AppId, …)`를 부른다. 호출 task는 ES(L367·L377·L387)와 EVS(L289·L297·L304) | 원·검 | 직접 확인. |
| R2.E4 | 요구되는 순서 | `CFE_SB_AppInit`이 유효한 SB AppId를 저장한 뒤에야 어떤 task든 SB 이벤트 경로를 실행해야 한다. 이벤트가 생성되려면 SB의 EVS 등록도 먼저여야 한다. 근거는 API 문서(R2.F1)와 코드다 | 분 | R2.md §7. |
| R2.E5 | 기존 동기화(ES 직렬화 S1) | `CFE_ES_CreateObjects`가 core task를 하나씩 시작하고 각 task가 RUNNING일 때까지 기다린다(`CFE_ES_MainTaskSyncDelay`, 시간 초과 시 panic). 순서 보장이지만 생성 순서상 ES·EVS init이 SB보다 먼저 완료되므로 R2에 대해서는 **틀린 순서를 결정적으로 보장한다**는 코드 읽기 | 분 | `cfe_es_start.c` L810·L835는 `검`; 결론은 R2.md §6.1. |
| R2.E6 | 우선순위 | 모듈 기본값 EVS 61, SB 64, ES 68, TIME 60, TBL 70이지만 ES 직렬화 S1에서는 한 번에 한 core task만 초기화하므로 이 순서를 결정하지 않는다는 추정. ENV에서는 어차피 모든 task가 SCHED_OTHER다 | 분 | `cases/R2.md` §6 S4(ES 직렬화); K5. |
| R2.E7 | EVS의 검증 | `EVS_GetAppDataByID`가 NULL이면 `ILLEGAL_APP_ID`로 돌아간다. 가드이지 순서가 아니며 SB 호출자가 상태를 버린다 | 원·검 | `cfe_evs.c` L186–L188. 호출자가 버린다는 부분은 저. |
| R2.E8 | 두 번째 준비 단계 | 게시 후 SB의 EVS 등록 전 구간에서 유효하지만 미등록인 AppId는 `ILLEGAL_APP_ID`가 아니라 `EVS_NotRegistered` 가지로 간다. gdb 3회에서 이 구간의 사용은 0이다 | 분 | 코드 읽기. 0은 `저·M`(R2_gdb §7). |
| R2.E9 | 별개의 파생 후보(App-record 창) | `CFE_SB_AppInit`이 `CFE_ES_GetAppID`의 반환을 무시하는데, 새 task의 task-function 폴링은 *task* 레코드만 확인하므로 생성자가 *app* 레코드를 최종화하기 전에 SB가 `GetAppID`를 부르면 SB AppId가 실행 내내 UNDEFINED일 수 있다는 추정. 보고된 적이 없고 실행하지 않았다. 원 결과와 분리해 파생으로 표시한다 | 분 | R2.md D5; 선(D16)에 따른 분리. |

### 7.F 정상·오류 판정 기준

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| R2.F1 | 층1: 문서화된 API 의무와 실제 동작 | `CFE_EVS_SendEventWithAppID` 헤더(API contract): "The Application ID must correspond to a registered application or core service. For messages outside the context of a registered application (for example early in app initialization or if registration fails) #CFE_ES_WriteToSysLog can be used for reporting."(`cfe_evs.h` L165–L172). 실제 동작: 정의되지 않은 ID → NULL → `CFE_EVS_APP_ILLEGAL_APP_ID`(`cfe_evs.c` L186–L188) | 원·검 | 직접 확인. |
| R2.F2 | 층2: 호출자 요구(준비 상태 전제) | SB API가 SB의 AppId로 이벤트를 내는 시점에 그 AppId는 등록된 유효 AppId여야 한다. 근거는 R2.F1의 헤더 문서다 | 원·검 | R2.md §7 provenance 1. 호출자 task(ES·EVS)가 SB의 이벤트를 대신 내보내는 구조이므로, 요구는 SB의 게시 시점에 걸린다는 해석은 분석이다. |
| R2.F3 | 층2의 허용 처리 | 오류 반환을 정상으로 받아들인다는 원출처 근거는 확인된 바가 없다. 엄격안(QT-4): 원출처 근거가 없는 항목은 미판정으로 두고 코드상 상태 변화·오류 반환은 계속 보고한다. 프레임워크가 어떤 반환을 허용한다는 것만으로 호출자의 요구 충족을 인정하지 않는다(QT-3) | 선(QT-4, QT-3, Q15) | RESOLVED §2, §4.1. |
| R2.F4 | 층3: 관측 결과 | gdb 아래 3회의 각 부팅에서 거부된 이벤트 요청 24회(ES 20, EVS 4). 3/3 실행에서 같은 hit 위치(위반 사용 hit 1–47, SB 게시 hit 50, 첫 정상 사용 hit 52). 24회 모두 AppID 0이고 같은 스레드에서 `OUT_ILLEGAL_APP_ID`가 뒤따른다. SB 게시와 등록 사이의 사용은 0. 이후 정상 사용 199/205/205 | 저·M·검 | R2_gdb §2, §7. 원시 기록을 이 문서에서 별도 스크립트로 재집계해 일치(§9). |
| R2.F5 | 관측 단위 | 24는 독립 결함 24개가 아니다. 원인 1, 정적 SB 호출 사이트 39 중 3(`cfe_sb_api.c` L275·L1183, `cfe_sb_priv.c` L786), task×함수 문맥 6, 호출열 12, 부팅당 동적 발생 24, 실행 3(풀링하지 않음) | 저·M | R2_gdb §6. 서로 다른 단위를 더하지 않는다(SPEC §8.2). |
| R2.F6 | 이후 정상 호출 | SB AppId가 설정된 뒤의 정상 호출에 R2의 고정된 오류 판정을 일괄 적용하지 않는다 | 선(제안) | ROADMAP §7.3. |
| R2.F7 | 원인의 구분 | SB AppId 미설정과 EVS 미등록을 서로 다른 원인으로 표현한다 | 선(제안) | ROADMAP §7.3. |
| R2.F8 | 거부된 이벤트 요청의 도달(gdb 기록) | gdb 3회의 각 부팅에서 24개의 이벤트 요청이 `ILLEGAL_APP_ID` 가지(`cfe_evs.c` L188)에 도달했고 같은 스레드에서 `OUT_ILLEGAL_APP_ID`로 반환했다(R2.F4, R2.G5와 같은 사실) | 저·M·검 | `results/R2_gdb_v701.md` §2, §7; 원시 기록 재집계(V-R2-4). |
| R2.F8a | 거부 가지의 가시성(출력·카운터·시스템 호출) | 거부 가지(`cfe_evs.c` L186–L189)에 출력과 카운터가 없고 시스템 호출이 없다는 것은 코드 읽기이며 관측이 아니다. syscall: not observed; code analysis only (SPEC §6.4 item 47). `env/logs/r2_g*`에는 시스템 호출을 관측한 기록이 없고, gdb 트레이스(필드: `seq`, `t_ns`, `kind`, `lwp`, `thread_name`, `event_id`, `app_id`, `stack`, `sb_appid_now`, `sb_appid`, `status`; 머리 줄의 `meta`·`points`)는 카운터 값을 기록하지 않았다. `results/R2_gdb_v701.md` §2·§5의 'invisible on the console, in counters and at the syscall level'·'no syscall was made'는 gdb 기록이 뒷받침하지 않으므로 이 대장은 채택하지 않는다(그 파일은 이 문서의 범위 밖이다, §10 항목 17). 프로세스 내부 관측(gdb)으로 24개 요청의 도달이 보였다는 것만 R2.F8이 말한다 | 분 | `cases/mapping_v701.md` §1.2(코드 읽기); `plan/ROADMAP_20261015.md` §3('R2 결과 표현의 과장'); SPEC §0.9 항목 4, §6.4 항목 47. |
| R2.F9 | 임무상 피해 | 거부된 이벤트 요청 외에 확인된 것이 없다 | 미 | ROADMAP §6.1. |
| R2.F10 | gdb 없는 비수정 바이너리 | 같은 집합이라는 기대는 코드 읽기일 뿐 이 바이너리에서 측정한 바가 없다 | 미 | R2_gdb §5(지원되지 않는 주장). |
| R2.F11 | 결정성 | 3/3 동일은 고정 순서의 증명이 아니다(3 부팅, all-stop 디버거 아래, 같은 유지 상태, 한 호스트). 고정 순서 주장은 정적 논증과 가정이 필요하다 | 분 | R2_gdb §6; proof_status_R2.md. |

### 7.G 관측 계획

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| R2.G1 | 소스 위치 | R2.A4의 줄들과 생성 core 목록(`cmake/mission_defaults.cmake` L26–L40), `CFE_ES_MainTaskSyncDelay` 조건을 실제 빌드에 연결한다 | 선(제안) | ROADMAP §7.1. 줄은 `검`. |
| R2.G2 | 실제 바이너리 위치 | ENV `core-cpu1`의 디버그 라인 테이블로 중단점 줄을 gdb `info line`으로 확인했다 | 저 | mapping_v701 머리말. |
| R2.G3 | 자원 식별값 | `CFE_SB_Global.AppId`와 EVS 등록 정보를 서로 다른 상태로 구분한다. 설정 직전(`CFE_ES_GetAppID` 호출 직전)과 값 저장 완료(L142)를 구분한다 | 선(제안) | ROADMAP §7.1. |
| R2.G4 | task 식별 | gdb 스택의 가장 바깥 `*TaskMain` 프레임으로 구분했다. Kairos는 자체 identity join을 쓴다 | 저·M | R2_gdb §7; SPEC S1. |
| R2.G5 | 반환값과 사건의 연결 | 같은 LWP에서 다음 사용 전에 나온 `OUT_ILLEGAL_APP_ID`를 사용에 대응시킨다(24/24 일치, AppId·EID 동일) | 저·M·검 | R2_gdb §7. |
| R2.G6 | 값 전달 추적 | SB 필드에서 이벤트 API 인자로 전달되는 경로 | 선(제안) | ROADMAP §7.1. |
| R2.G7 | 기존 기록과의 대조 | 기존 기준 구성에서 24회의 집합과 대응 결과를 대조한다. 일치하지 않으면 구성 차이·기록 손실·관측 영향·구현 오류를 조사하고 과거 숫자를 재현하도록 규칙을 조정하지 않는다 | 선(제안) | ROADMAP §7.3. |
| R2.G8 | 관측 영향 | K11. 보고: 부팅 시간, 관측 기록의 누락, 요청·반환 사건 대응 실패, 환경 차이. 커널이 바뀌었으므로 기준 실행을 재측정한다(K7) | 선(D1, D11) | §2.1. |
| R2.G9 | 단일 경로 기록의 의미 | 한 번의 정상적인 경로 기록은 그 경로의 발생 증거다. 빈도·확률·일반적 오버헤드에는 별도 표본 설계가 필요하다. 첫 사례 평가는 기술통계와 사례별 근거 중심이다 | 선(P-STATS) | ROADMAP §7.3; RESOLVED §4.3. |

### 7.H 예상 결과

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| R2.H1 | 예상 결과표 | §8.6. R2의 사용별 집합 기대는 gdb 측정을 본 뒤에 적은 것이어서 예측이 아니다(§8.1 규칙 7). 시트는 R2의 첫 Kairos 출력보다 먼저 동결되지 않았다(§8.1 규칙 10) | 선(제안) | ROADMAP §6.1. |

### 7.I 변경 기록

| ID | 항목 | 내용 | 태그 | 근거·비고 |
|---|---|---|---|---|
| R2.I1 | 원출처의 조건 | 보고자 OS(Ubuntu 22.04.5), "cFE v6.7", 빌드 명령 세 개와 gdb 중단점 방법, 관측 기준(중단점 정지), 보고자의 설명("might have gone unnoticed…") | 원 | 행: R2.A1–A3, C1–C3. |
| R2.I2 | 원출처와 다른 조건 | (1) cFE가 "v6.7"/`main`@2025-08-06(B로 추정)이 아니라 v7.0.1(`c5fb2b4d`): 이유는 R2의 첫 전체 구현을 현재 기본 구성에 둔다는 결정(P-LADDER); 영향: core 순서가 달라 ES도 SB보다 먼저가 된다. (2) 빌드가 `SIMULATION=native prep/make/make install`이 아니라 `make native_std.prep/install`. (3) 호스트 Ubuntu 24.04.4(보고 22.04.5). (4) 중단점이 `cfe_evs.c` L185가 아니라 L188(내용 이동). (5) 일반 사용자로 root 세션에서 기동. (6) ENV 환경 이탈(K15). (7) 시작 시 유지 상태가 정리되지 않았다(R2.D2). (8) gdb 3회는 gdb 아래 교란이 있다(R2.C8). (9) gdb 15.1과 all-stop 모드, 실행 횟수 3은 어느 레퍼런스에도 없는 값을 고른 것이다(R2.C7, R2.C9) | 선(차이) | R2.B1, A4; CONDITIONS §F. |
| R2.I3 | 출처가 정하지 않은 조건 | A5, C4, B13, D6, F9, F10 | 미 | 위 행의 `미`. |
| R2.I4 | 이 대장의 변경 기록 | 2026-10-15 최초 작성(초안). 이 작성 중 `b9bec567`의 전체 SHA·부모·변경 파일 수(R2.B5)를 새로 확인했다. 2026-10-16 재점검(V-R2-R1–R6): 사례 행 변경 없음, 공통 행 K6·K7 갱신 | 저 | §11. |

## 8. 기대 결과표 초안 (사례별 골격과 해시 규칙)

### 8.1 공통 규칙

1. **상태.** 이 §8은 초안이며 동결되지 않았다. **해시를 계산하지 않았다.** 확정 전에 문장이 바뀌고, 시트의 (σ, M, b) 셀·인스턴스 목록은 SPEC S3a의 census가 있어야 채울 수 있기 때문이다. SPEC 용어집('Pre-registered expectation sheet')이 정하는 시트의 한 행은 "개발 사례, 기준, (σ, M, b) 셀과 인스턴스, 기대 class, 정제(refinement), 진행성 값, 결과 라벨, 원출처의 class 서술이 있으면 그 서술과 인용"이다. 아래 골격은 그 틀의 머리이며 셀·인스턴스 열은 미정 [U]이다. R2는 이미 Kairos의 첫 출력이 있어 규칙 10의 예외다.
2. **세 목록을 따로 둔다.** P = 예상 실제 현상(실제 cFS 실행에서 일어날 일), T = 예상 도구 분류(Kairos가 낼 분류), B = 기대 근거(각 기대가 어느 출처·분석·가정에 서는가). 셋은 서로를 대체하지 않는다. 실제 현상과 분류는 다를 수 있다.
3. **미확인은 미확인으로 둔다.** 상태 `미정[U]`인 행은 값을 채우지 않았다. 근거가 부족한 정상·오류 라벨을 연구자 규칙만으로 강제하지 않는다(QT-4, Q15).
4. **도구 분류의 출처.** T 목록은 SPEC rev 1.2가 서술한 기대 [I]를 옮긴 것이다. 이 대장이 새로 판단한 것이 아니며 도구 출력도 아니다. 분류의 어휘와 셀 표기는 SPEC의 것이다. 원출처의 'race'를 class로 자동 변환하지 않는다(P-CLASSGT).
5. **해시 규칙.**
   - (a) 해당 사례의 **첫 도구 출력 전에** 그 사례 시트 확정본의 sha256을 저장한다. (R2에서는 지켜지지 못했다: 규칙 10.)
   - (b) '첫 도구 출력'은 Kairos의 어느 층(SPEC S1 기록기 포함)이든 그 사례 또는 그 사례의 대조에 대해 처음 내는 출력이다. 이미 있는 gdb 기록(R2)은 도구 출력이 아니라 저장소 기록이다.
   - (c) 저장하는 것: 확정본 파일의 바이트열에 대한 해시, 저장 시각, 시트를 쓴 사람, 열람 이력(사례·수정·기대를 본 사람과 시점. SPEC §7.5 항목 5의 노출·동결 기록).
   - (d) 확정본은 이 대장에서 해당 사례의 §8 부분을 떼어 별도 파일로 만든다 [선(제안)]. 그 뒤 이 대장을 고쳐도 확정본의 해시는 바뀌지 않는다.
   - (e) 동결 뒤 변경은 변경 로그(시각, 이유, 계기가 된 항목, 바뀌는 판정)에 기록한다. 도구 출력이 기대와 다르면 조사하고 보고하며 기대에 맞추도록 조정하지 않는다(SPEC §9.3).
   - (f) 평가 엔진·규칙의 동결은 별도이며 본 평가 전에 한다(P-F1GATE).
   - (g) **맹검이 아니다.** 작성자는 다섯 사례와 수정 내용을 알고 있다. 항목은 `non-blind(case | fix | expected)`로 표시하고 맹검·미관측 성과로 부르지 않는다(SPEC §7.2, §7.5).
   - (h) 미정 [U]: 해시 직렬화(줄바꿈·공백 정규화 규칙), 해시를 기록할 위치, 기록 담당자.
6. **P-F1GATE.** F1 전에 D13·D22·P-C2의 선택과 필요한 세부 조건을 기록하고, 사례별 자체 기준 구성·기대 결과표·필요한 구현 기능·검증 단계가 존재해야 한다. 수행하지 못한 사례는 그 이유와 연구 범위의 제한을 명시하며 다른 코드의 결과로 완료 처리하지 않는다.
7. **측정 뒤 기재.** R2는 gdb 3회 기록을 본 뒤에 쓰므로 R2의 사용별 집합(24 = ES 20 + EVS 4 등)은 예측이 아니다. `측정 후 기재[M]`로 표시하고 예측 적중으로 세지 않는다(SPEC S1-R2의 "the expectation was measured before it was written").
8. **단위.** 한 행의 수는 하나의 단위(U1 결함 원인, U2 수정 이력, U3 프로그램 변형, U4 정적 인스턴스, U5 동적 발생, U6 실행)만 센다. 서로 다른 단위를 더하거나 표본 수로 쓰지 않는다(SPEC §8.2).
9. **집합의 성격.** 다섯 사례와 그 수정은 알려진 설계 사례다. 이 시트의 결과는 개발 집합 결과이며 미관측 평가 집합 성과로 보고하지 않는다(D8/P-RECALL).
10. **R2 시트의 실제 순서 (2026-10-15 기록).** R2에 대한 Kairos의 첫 도구 출력(`env/logs/s1r2_p1*`, SPEC S1 기록 실행, 2026-10-15T11:17Z)은 이 시트의 해시가 저장되기 전에 있었다. 해시는 지금도 없다. 따라서 §8.6의 R2 기대는 사전 등록이 아니다. 특히 E-R2-T3('기록기가 gdb 기록과 같은 per-use 집합을 다시 관측한다')는 그 출력 이후에도 사전 기대로 세지 않는다. 이 문서는 그 출력의 내용이 기대와 맞는지 보지 않았다. 과거 순서는 바꿀 수 없으므로 시트 문장을 그 출력에 맞추어 고치지 않는다. 시트를 지금 동결할지, 동결 기록에 실제 순서(출력 시각 → 동결 시각)를 어떻게 적을지는 orchestrator·PI의 결정이다(§10 항목 16). 다른 사례에는 해당하지 않는다(이 문서가 확인한 한 그 사례의 출력이 없다).

상태 표기: `기대[I]` 코드 읽기·SPEC 서술에서 나온 기대(미실행) / `측정 후 기재[M]` 이미 관측된 값을 옮긴 것 / `미정[U]` 값을 채우지 않음 / `결정[D]` PI 결정 또는 설계 결정.

§0.1 태그와의 대응: `기대[I]`는 `분`(연구자 분석 또는 SPEC의 기대), `측정 후 기재[M]`는 `저·M`, `미정[U]`는 `미`, `결정[D]`는 `선(ID)`에 해당한다. 시트의 `근거` 열은 §3–§7의 행 ID를 가리킨다.

### 8.2 C1 시트

기준: 공개 BP 기반(D22), 후보 쌍 CF 281a941→833fdbb. 기본 구성(v7.0.1)은 음성 대조. (σ, M, b) 셀·인스턴스 목록은 미정[U].

**P. 예상 실제 현상**

| ID | 내용 | 상태 | 근거 |
|---|---|---|---|
| E-C1-P1 | 활성 구성(비어있지 않은 이름, provider)에서 provider가 먼저 생성하면 첫 조회가 성공한다(A1). 자연 실행에서 그렇게 되는 빈도·조건은 가정하지 않는다 | 기대[I] / 빈도 미정[U] | C1.E5, C1.B5 |
| E-C1-P2 | CF 281a941에서 provider 생성이 CF 조회보다 늦으면: 조회가 `NAME_NOT_FOUND` → 이벤트 `CF_EID_ERR_INIT_SEM`(`error=%ld`) → 엔진 미활성 → `CF_Init` 실패 → `APP_ERROR` 종료. 보류로 만든 제어 실행이며 자연 실행이 아님 | 기대[I] | C1.F3, C1.F4, C1.C8 |
| E-C1-P3 | CF 833fdbb에서 provider가 재시도 구간 안에 생성되면 먼저 실패하고 후속 재시도로 성공(A2). 몇 번째 시도에서 성공하는지는 해제 사건의 선택에 달려 있다 | 기대[I] / 해제 사건 미정[U] | C1.A5, C1.C9 |
| E-C1-P4 | CF 833fdbb에서 보류가 재시도 종료까지 이어지면 R회 시도 뒤 최종 실패(A3). R은 선택 버전의 상수(833fdbb는 25회, 100 ms) | 기대[I] | C1.C10, C1.F3 |
| E-C1-P5 | v7.0.1 기본 구성(빈 이름)에서는 조회가 실행되지 않는다(경로 비활성) | 기대[I]; 같은 내용이 ENV 기록(D7)에 이미 있다 | C1.B11, C1.D1 |
| E-C1-P6 | A4(해당 이름의 provider 없음): 참조된 cFS 구성이 없다. 단위 예제로 검증하거나 '미실현'으로 기록하며 실제 사례 결과로 세지 않는다 | 미정[U] | C1.F8 |
| E-C1-P7 | A5(RESERVED 구간): `OS_ERR_INCORRECT_OBJ_STATE`와 count-sem 락 잔류·provider hang은 코드 읽기 가설이다. 원 C1의 착수 조건·성과 수에 넣지 않는다. hang 한계는 미정 | 기대[I] / 한계 미정[U] | C1.E10, C1.A11, C1.F10 |
| E-C1-P8 | 원 보고의 실제 실패 빈도·출력·환경 | 미정[U] | C1.A8 |
| E-C1-P9 | provider 재시작 시 세마포어 ID 변경(보조 문제)의 현상과 범위 포함 여부 | 미정[U] | C1.E11 |

**T. 예상 도구 분류** (SPEC에서 옮긴 [I])

| ID | 내용 | 상태 | 근거 |
|---|---|---|---|
| E-C1-T1 | 기본 구성에서 두 조회가 `inactive(B)`(설정 폐쇄로 판정). C1 증거가 아니다 | 기대[I] | SPEC §9.3 V1b; C1.G8 |
| E-C1-T2 | 공개 BP 기반 (σ_U, M_api): `schedule-dependent`. 같은 빌드·구성·의미에서 같은 순번(ordinal)에 정상·비정상 근거가 모두 있을 때만 주장한다 | 기대[I] | SPEC App. A.4 step 3; ROADMAP §8.5 |
| E-C1-T3 | (σ_U∞, M_api): 281a941은 시간 기반 전이 없이 위반이어서 `schedule-dependent`가 유지된다. 833fdbb는 `exh`가 ¬Bad(σ_U∞)를 보이면 `timeout-dependent` 정제. 탐색기(SPEC S7) 없이는 정제가 성립하지 않는다 | 기대[I] | SPEC App. A.4 step 3 |
| E-C1-T4 | M_impl: RESERVED 구간의 `INCORRECT_OBJ_STATE`는 CF의 재시도 코드 밖이어서 그 시도가 위반이다(안전성). 구현 결함 후보로 별도 보고 | 기대[I] | SPEC App. A.4 step 4 |
| E-C1-T5 | 진행성: 안전성 결과에 진행성 미검사를 함께 표시한다. 값은 탐색기 이후 | 미정[U] | SPEC §1; RESOLVED D20 |
| E-C1-T6 | 281a941→833fdbb의 이동은 (σ_U∞, M_api) 셀과 최종 실패의 자연성(`held-F0` → `timeout-exercised`)에서만 나타난다 | 기대[I] | SPEC §8.2 표 |
| E-C1-T7 | 원출처의 'race' 서술(이슈 제목·본문, v7.0.1 `cf_cfdp.c` 주석, 833fdbb 주석)과 도구 분류의 일치는 별도 보고하고 정확도 분모를 만들지 않는다 | 결정[D] / 일치 여부 미정[U] | P-CLASSGT |
| E-C1-T8 | 위반 라벨은 '오류 반환 + 기본 정상 후속 경로 없음'이라는 Kairos 규칙에 의존한다. QT-4 엄격안에서는 NOT_READY_UNJUDGED이며 class는 같다 | 기대[I] | SPEC App. A.4 step 3 |

**B. 기대 근거**

| ID | 내용 | 상태 | 근거 |
|---|---|---|---|
| E-C1-B1 | P2–P4 ← CF 코드(281a941 L993–L1001, 833fdbb L1000–L1011, `cf_app.c` L252·L333·L366)와 833fdbb의 상수. 근거 종류: 원출처 코드 + 분석 | `검` | C1.A4, A5, F3, F4, C10 |
| E-C1-B2 | P1·P5 ← 15a871e의 기본 표와 재시도 코드, ENV 기록 | `검` + 저 | C1.B11, D1 |
| E-C1-B3 | T2–T6, T8 ← SPEC rev 1.2 App. A.4, §8.2, §9.3. 이 대장은 그 기대를 옮겼을 뿐이다 | [I] | SPEC |
| E-C1-B4 | T7 ← P-CLASSGT, 이슈 본문†, 833fdbb 커밋 주석 | 결정[D] | C1.A10 |
| E-C1-B5 | 모든 P·T 기대의 전제: 공개 BP 통합이 해당 core 조합에서 빌드·실행되는가 | 미정[U] | C1.B9 |
| E-C1-B6 | 노출 기록: 작성자는 사례·수정·기대를 알고 있다 | `non-blind(case, fix, expected)` | §8.1 (g) |

### 8.3 C2 시트

기준: 역할 재구성(P-C2). 결과 명칭은 "C2 메커니즘의 역할 기반 검증"이며 원 시스템 재현이 아니다.

**P. 예상 실제 현상**

| ID | 내용 | 상태 | 근거 |
|---|---|---|---|
| E-C2-P1 | 단일 단계 배리어 + 기존 방식 provider(sync 호출 뒤 late init): R의 요청이 P의 late-init 완료 전에 도달할 수 있다. 단일 호출이 '준비됨(RUNNING)'을 먼저 게시하고 OPERATIONAL이 P의 late init 중에 선언될 수 있다는 코드 읽기에 근거한다. 실제 발생·빈도는 역할 재구성 실행이 필요하다 | 기대[I] / 발생 미정[U] | C2.A2, C2.E2, C2.E6 |
| E-C2-P2 | 두 단계 배리어 + 수정 provider(`WaitForSystemState(APPS_INIT)` → late init → RunLoop): ES는 P가 RUNNING이 되기 전에는 OPERATIONAL을 선언하지 않아 R은 P의 late init 뒤에 풀린다(소프트 타임아웃이 만료되지 않는 한) | 기대[I] | C2.E6, C2.A4 |
| E-C2-P3 | 두 단계 배리어 + 기존 방식 provider: 호환 규칙상 여전히 일찍 RUNNING이 되어 P1과 같은 조기 해제가 가능하다 | 기대[I] | C2.E6 |
| E-C2-P4 | 단일 단계 + 수정 provider: 단일 단계 코드(6.4.2, v6.5.0a)에는 `WaitForSystemState`·`APPS_INIT`·`LATE_INIT`이 없으므로(C2.B9, `검`) 이 셀은 이식 없이는 구성되지 않는다. 이식하면 다른 프로그램 변형이 된다. 제외 셀이면 이유를 기록한다 | 기대[I] / 제외·이식 선택 미정[U] | C2.B7, C2.B9 |
| E-C2-P5 | ES 단계 소프트 타임아웃이 만료되면 waiter가 `CFE_SUCCESS`로 풀려 정상 초기화 완료와 구분되지 않는다 | 기대[I] | C2.E7 |
| E-C2-P6 | provider의 late init이 실패할 수 있는 구성의 현상(별도 보조 실험, Q22) | 미정[U] | C2.B8 |
| E-C2-P7 | 원 보고의 증상(충돌·오류 코드·로그)은 없다. 재구성에서 관측되는 추가 증상은 파생 결과로 표시한다 | 미정[U] | C2.F4, F5 |

**T. 예상 도구 분류** (SPEC에서 옮긴 [I])

| ID | 내용 | 상태 | 근거 |
|---|---|---|---|
| E-C2-T1 | 첫 실행은 템플릿만 사용하며 `unknown(unmodelled)`을 기록한다 | 기대[I] | SPEC §9.3 VP-D4 |
| E-C2-T2 | 이후 분류는 Q22의 가정을 포함한 것과 포함하지 않은 것을 함께 보고한다. 값은 정해지지 않았다 | 미정[U] | Q22; SPEC VP-D4 |
| E-C2-T3 | 2×2의 각 셀을 그대로 제시하고 독립된 실제 결함 4개로 세지 않는다 | 결정[D] | P-C2 |
| E-C2-T4 | barrier 통과(OPERATIONAL 도달)와 앱 고유 초기화 완료가 같지 않음을 도구가 다루는가(ROADMAP §9.1 완료 기준). 예상 분류는 정해지지 않았다 | 미정[U] | ROADMAP §9.1 |
| E-C2-T5 | v7.0.1 일반 층: barrier의 writer 집합과 K-PHASE census가 코드 읽기로 찾은 항목(in-tree의 SBN·CS 이탈)을 다시 찾는다. C2 증거가 아니다 | 기대[I] | SPEC §9.3 C2 행 |
| E-C2-T6 | 이슈의 'race' 서술을 class로 자동 변환하지 않는다 | 결정[D] | P-CLASSGT |

**B. 기대 근거**

| ID | 내용 | 상태 | 근거 |
|---|---|---|---|
| E-C2-B1 | P1–P3 ← 이슈 문장 4·5b–5c·7†, v6.5.0a 코드, v6.6.0a 수정, 호환 노트 | `검` + 저 | C2.A2–A4, E6 |
| E-C2-B2 | P5 ← ES 메인이 타임아웃 뒤에도 계속 진행하는 코드(v6.5.0a, dev). waiter가 `CFE_SUCCESS`로 풀린다는 반환은 기록 | `검` + 저 | C2.E7, F2 |
| E-C2-B3 | T1–T2 ← SPEC VP-D4와 §9.3 C2 행 | [I] | SPEC |
| E-C2-B4 | 모르는 것: R→P 방식, late-init 내용, timeout 인자, 앱 수·우선순위, 플랫폼, 단일 단계 셀의 cFE 코드 | 미정[U] | C2.C2–C6, B5 |
| E-C2-B5 | 결정: 역할 재구성, 2×2, Q22의 1차 대조 | 결정[D] | P-C2, Q22 |
| E-C2-B6 | 노출 기록 | `non-blind(case, fix, expected)` | §8.1 (g) |

### 8.4 C3 시트

기준: 6.4.1(버그)과 6.4.2(수정) tarball. 첫 파싱은 분석 전용이며 차이가 있는 조건이다(D13). 실제 실행 검증은 별도 산출물이다.

**P. 예상 실제 현상**

| ID | 내용 | 상태 | 근거 |
|---|---|---|---|
| E-C3-P1 | 6.4.1 파생 구성에서 TIME의 `CFE_SB_CreatePipe` 이벤트(`CFE_SB.AppId`=0)가 SB·EVS 초기화 완료 전에 발행되어 `EVS_NotRegistered` → `EVS_SendEvent` → `EVS_IsFiltered(0xFFFFFFFF)`에 도달한다(잘못된 상태 사용). 그 순서가 실제로 만들어지는지는 순서 제어·실제 우선순위 적용·CPU 수에 달려 있다. 분석 전용 환경에서는 실행하지 않고 소스 추출과 기대만 보고한다 | 기대[I] / 실현 미정[U] | C3.E3, E5, C9–C11 |
| E-C3-P2 | 실제 crash: `AppData[0xFFFFFFFF]`(-m32에서 `AppData[-1]`)이 fault를 내는지 | 미정[U] | C3.F4, F6 |
| E-C3-P3 | 6.4.2에서는 F3·F1·F2로 TIME 쪽 crash 경로가 닫힌다 | 기대[I] | C3.A6, E7 |
| E-C3-P4 | 6.4.2에서도 EVS가 SB보다 먼저 `CFE_SB_CreatePipe` 이벤트를 `CFE_SB.AppId`=0으로 내고 EVS에 오귀속되어 DEBUG 이벤트가 버려진다(crash 아님). 수정 뒤 crash가 사라진 것은 순서 의존성 제거가 아니다 | 기대[I] | C3.E7 |
| E-C3-P5 | 수정 부분(F1/F2/F3)별 효과의 분리. 구성된 하위집합이 필요하며(차이 조건) 계획이 없다 | 미정[U] | C3.E8, SH-4 |
| E-C3-P6 | Microblaze에서의 원 현상 재현 | 미정[U] | C3.A10 |

**T. 예상 도구 분류** (SPEC에서 옮긴 [I])

| ID | 내용 | 상태 | 근거 |
|---|---|---|---|
| E-C3-T1 | H-V: 6.4.1에서 두 연쇄 인스턴스(TIME의 `CreatePipe` 이벤트의 K-ID 사용, TIME 문맥의 EVS 자기 AppID 사용)가 σ_R1에서 `fixed-order`(`exh`로만, SPEC S7), σ_U에서 `schedule-dependent`. Allowed는 Φ_6.4.x가 6.4.1 레퍼런스를 인용할 때까지 `unspecified_by_reference`. 엔진 변경이 필요하면 H-V의 강한 형태는 반증이다 | 기대[I] | SPEC §7.6 |
| E-C3-T2 | 6.4.1→6.4.2: `schedule-dependent` → `protected` within b, `exh`로만. 어느 쪽에도 인증서가 없다 | 기대[I] | SPEC §9.3 C3 행 |
| E-C3-T3 | 오귀속 가지(READY)는 K-ID로 표현되지 않는다(명세의 표현 한계이며 엔진 변경이 아님) | 기대[I] | SPEC §7.6 |
| E-C3-T4 | 입력·대응 실패(basis/binding failure)는 탐지 기법의 실패나 H-V의 반증으로 세지 않는다 | 결정[D] | Q16; SPEC VP-D3 |
| E-C3-T5 | 원출처는 'race'라 하지만 H-V는 σ_R1에서 `fixed-order`를 기대한다. 불일치가 있으면 결과로 보고한다 | 결정[D] | SPEC §8.2; P-CLASSGT |
| E-C3-T6 | 진행성은 검사하지 않는다(`unknown(not_checked)` 표시) | 미정[U] | SPEC §1 |

**B. 기대 근거**

| ID | 내용 | 상태 | 근거 |
|---|---|---|---|
| E-C3-B1 | P1–P4 ← 6.4.1/6.4.2 소스, 이슈†, VDD, 분석 | `검` + 저 + 분 | C3.A5, A6, E3, E5, E7 |
| E-C3-B2 | T1–T3 ← SPEC §7.6 H-V와 §9.3 | [I] | SPEC |
| E-C3-B3 | 모르는 것: EVA 환경 전부, 순서 제어, CPU 수, 실제 fault, OSAL artifact 통일, 주 oracle | 미정[U] | C3.C2, C9, C11, F6; SH-16 |
| E-C3-B4 | 결정: D13 (a)(b)(c), Q16 | 결정[D] | SH-10–SH-14 |
| E-C3-B5 | C5와 한 수정 이력, R2와 자원 계열 공유 → 독립 표본이 아님. 비맹검 | `non-blind(case, fix, expected)` | C3.A11, A14 |

### 8.5 C5 시트

기준: 6.4.1(·6.4.0) 버그, 6.4.2 수정. 분석 전용 파싱 우선(D13).

**P. 예상 실제 현상**

| ID | 내용 | 상태 | 근거 |
|---|---|---|---|
| E-C5-P1 | 6.4.1 파생에서 생성자가 L610에서 락을 풀고 `OS_TaskCreate`(L614)한 뒤 L637에서 다시 잡기 전에 새 task가 `GetAppID`를 부르면 `CFE_ES_ERR_APPID`가 반환된다(또는 `OS_TaskGetId` 순서에 따라 잘못된 AppID). 창이 열리는 조건(CPU 수·루트·우선순위·생성 경로)이 정해져 있지 않아 보류가 필요할 수 있다 | 기대[I] / 창 열림 미정[U] | C5.E1–E6, C5.C5, C5.C10 |
| E-C5-P2 | 새 task가 먼저 실행된 사실만으로는 위반이 아니다. 위반은 등록 전 조회와 그 결과다 | 기대[I] | C5.E5 |
| E-C5-P3 | 6.4.2에서는 락 범위 확장으로 락으로 보호된 읽기가 등록 완료 뒤에 일어난다. 락 없이 읽는 경로는 보호되지 않는다 | 기대[I] | C5.A6, E7 |
| E-C5-P4 | 원 보고에 없는 앱 crash는 기대하지 않는다. 하류 영향("snowball")은 정해지지 않았다 | 미정[U] | C5.F4, F5 |
| E-C5-P5 | 현재 버전의 App-record 잔여 후보(task 레코드 게시 뒤 App 레코드 최종화 전)의 도달 가능성. 별도 결과이며 원 C5를 대체하지 않는다 | 미정[U] | C5.A12, A13 |
| E-C5-P6 | stale `TaskTable` 항목이 창 안의 `GetAppID`에 이전 AppId를 돌려줄 수 있는가 | 미정[U] | C5.D2 |

**T. 예상 도구 분류** (SPEC에서 옮긴 [I])

| ID | 내용 | 상태 | 근거 |
|---|---|---|---|
| E-C5-T1 | 6.4.1: `schedule-dependent`(O-ordered, K-SELF.es) | 기대[I] | SPEC §7.6 H-V, §9.3 |
| E-C5-T2 | 6.4.2: `exh`를 통해서만 움직이며, 탐색기(SPEC S7) 없이는 `unknown`이고 `protected`가 아니다. 통제는 diff 전체이고 C3와 공유한다 | 기대[I] | SPEC §9.3 C5 행 |
| E-C5-T3 | A.3 줄(스크립트 앱 App-record 창): PROTECTED 인증서가 `58abd38d^`에 나타나고 `58abd38d`에서 보류될 것으로 기대. #72가 아닌 별도 줄이며 비맹검이다 | 기대[I] | SPEC §9.3 VP-D5; P-RGIT |
| E-C5-T4 | 입력·대응 실패와 기법 실패를 분리한다 | 결정[D] | Q16 |
| E-C5-T5 | 이슈 제목의 'race'를 class로 자동 변환하지 않는다 | 결정[D] | P-CLASSGT |

**B. 기대 근거**

| ID | 내용 | 상태 | 근거 |
|---|---|---|---|
| E-C5-B1 | P1–P3 ← 6.4.1/6.4.2 `cfe_es_apps.c`·`cfe_es_api.c`, OSAL 4.1.1, 이슈†, VDD | `검` + 저 | C5.A3–A7, B4 |
| E-C5-B2 | P5 ← 현재 코드(`검`)와 잔여 창 분석 | `검` + 분 | C5.A11, A12 |
| E-C5-B3 | T1–T3 ← SPEC §7.6, §9.3, App. A.3 | [I] | SPEC |
| E-C5-B4 | 모르는 것: 관측자 환경, 댓글 9개, `OS_TaskGetId` 순서, 보류 감시 한도, 빌드 호스트·권한 | 미정[U] | C5.A9, A10, C1–C10 |
| E-C5-B5 | 비맹검. `58abd38d` 쌍은 설계 문서에 이미 알려진 쌍이다 | `non-blind(case, fix, expected)` | C5.A14 |

### 8.6 R2 시트

기준: ENV(cFS v7.0.1, cFE `c5fb2b4d`) 기본 구성. 대조: P-R2CTRL.

**주의:** 이 시트는 R2의 첫 Kairos 출력(2026-10-15T11:17Z)보다 먼저 동결되지 않았다(§8.1 규칙 10). 아래 기대는 사전 등록이 아니다.

**P. 예상 실제 현상**

| ID | 내용 | 상태 | 근거 |
|---|---|---|---|
| E-R2-P1 | gdb 아래 3회의 각 부팅(ENV 빌드, POWER ON 시작, all-stop)에서, SB AppId가 게시되기(`cfe_sb_task.c` L138) 전에 ES와 EVS task가 SB API의 이벤트를 AppID 0으로 요청했고 모두 `ILLEGAL_APP_ID`로 거부되었다. 한 부팅에서 24회: ES 20(`SEND_NO_SUBS` 17 + `SUBSCRIPTION_RCVD` 2 + `PIPE_ADDED` 1), EVS 4(1 + 2 + 1). 단위는 동적 발생(U5)이며 독립 결함 24개가 아니다. SB 호출자가 반환 상태를 버린다는 것은 코드 읽기다(R2.E2, E7). gdb 없는 바이너리와 반복 실행으로의 일반화는 이 행이 말하지 않는다(E-R2-P2, P3) | 측정 후 기재[M]: gdb 3회 | R2.F4, F5 |
| E-R2-P2 | 같은 24회가 gdb 없는 비수정 바이너리에서도 나오는가: 코드 읽기상 같을 것이라는 기대는 있으나 이 바이너리에서 측정한 바가 없다 | 미정[U] | R2.F10 |
| E-R2-P3 | 반복 실행에서 집합이 같은가. 3/3은 증명이 아니다. Kairos 실행의 N은 SPEC S1에서 정한다 | 미정[U] | R2.F11; K12 |
| E-R2-P4 | gdb 3회에서 SB AppId 게시 뒤의 정상 사용(199/205/205)은 위반이 아니다 | 측정 후 기재[M] | R2.F4 |
| E-R2-P5 | gdb 3회에서 게시와 SB의 EVS 등록 사이의 사용은 0이었다 | 측정 후 기재[M] | R2.F4, E8 |
| E-R2-P6 | 보고 리비전 B에서 ES도 영향을 받는가(Q1): B의 코드는 EVS만 예측(하한 4), 보고 서술은 "ES and EVS" | 미정[U] | R2.A5, A6 |
| E-R2-P7 | 거부된 이벤트 요청 외의 임무상 피해 | 미정[U] | R2.F9 |
| E-R2-P8 | 자연 쌍: `b9bec567^`(부모)에서는 ES가 SB 뒤에서 초기화되어 ES 쪽 사용은 게시 뒤이고 EVS는 여전히 SB 앞이다. `b9bec567`에서는 ES와 EVS가 모두 SB 앞이다. 이 커밋이 52개 파일을 바꾸므로 순서 이외의 동시 변경이 판정에 영향을 주는지는 먼저 확인해야 한다 | 기대[I] / 동시 변경의 영향 미정[U] | R2.B5, B6 |
| E-R2-P9 | PR #2820 head는 무변화 대조다: 거부 횟수가 달라지지 않을 것이다 | 기대[I] | R2.A7, B8 |

**T. 예상 도구 분류** (SPEC에서 옮긴 [I])

| ID | 내용 | 상태 | 근거 |
|---|---|---|---|
| E-R2-T1 | ES 반쪽과 EVS 반쪽 각각, 셀 (σ_U, M_api)와 (σ_U, M_impl)에서 b = ∞의 `fixed-order`(안전성 진술). 진행성은 `unknown(not_checked)`. 증명 상태를 함께 출력한다: V0-P 전에는 "accepted by `kz proofcheck`, conditional on the unwritten lemma obligations" | 기대[I] | SPEC App. A.1 step 9, 15; plan/proof_status_R2.md §1 (R-1~R-3, R-6) |
| E-R2-T2 | 도달: (σ_U, M_impl)은 SPEC S1이 관측한 실행 하나가 있으면 `proved`, (σ_U, M_api)는 M_api 적합성 검사를 통과해야 `proved`이고 아니면 `assumed` | 기대[I] | plan/proof_status_R2.md §1 (R-4) |
| E-R2-T3 | 사용별 집합: Kairos 자체 기록기가 per-use 집합(task, SB 함수, EID)을 gdb 기록과 같게 다시 관측한다. 24 = ES 20 + EVS 4. 불일치는 조사·보고하며 규칙을 과거 숫자에 맞추지 않는다 | 측정 후 기재[M]; 출력 후 기재(규칙 10: 시트 동결 전에 `s1r2_p1` 출력이 있었다. 이 문서는 그 출력과 이 기대의 일치를 확인하지 않았다) | SPEC S1-R2; ROADMAP §7.3 |
| E-R2-T4 | E-post/E-TBL(SB AppId 게시 뒤의 사용 및 TBL identity 사용)에는 `fixed-order` 판정이 없고 열려 있으며 App. A.2 루트에 연결(chained)된다 | 기대[I] | SPEC §9.3; App. A.2 |
| E-R2-T5 | 대조: 교정된 반쪽에서 `fixed-order`가 철회되고 `protected`가 대신 나오지 않는다. 구성·버전 민감성이며 스케줄 증거가 아니다 | 기대[I] | SPEC App. A.1 step 15; RESOLVED §3.1 |
| E-R2-T6 | R2는 'race'가 아니다: 원출처의 class 서술이 없다. 고정 순서는 정적 논증과 가정에 의존하며 반복 관측은 증명이 아니다 | 결정[D] | SPEC §8.2; P-CLASSGT |
| E-R2-T7 | "Kairos가 R2를 탐지·인증했다"는 문장은 구현·검토 전에는 쓰지 않는다 | 결정[D] | plan/proof_status_R2.md §1 |

**B. 기대 근거**

| ID | 내용 | 상태 | 근거 |
|---|---|---|---|
| E-R2-B1 | P1, P4, P5, T3 ← 원시 기록(`env/logs/r2_g1..3_trace.jsonl`)과 `results/R2_gdb_v701.md`. 근거 종류: 측정 | 저·M·검 | R2.F4, G5 |
| E-R2-B2 | P2, P3, P6, P7 ← 알 수 없음. 코드 읽기만 있거나 측정이 없다 | 미정[U] | R2.F9–F11, A5 |
| E-R2-B3 | T1, T2, T4, T5 ← SPEC App. A.1·A.2·§9.3 [I]와 `plan/proof_status_R2.md` | [I] | SPEC |
| E-R2-B4 | P8, P9 ← 양 끝점의 코드, 커밋 메타, PR 서술 | `검` + 저† | R2.B4, B5, A7 |
| E-R2-B5 | 한계 가정: gdb all-stop 아래 3 부팅, 같은 유지 상태, 한 호스트. 이벤트 단위 가시성 등 적용 가정은 결과에 표시한다(D10) | 결정[D] | R2_gdb §6; RESOLVED §4.1 |
| E-R2-B6 | 비맹검이며 일부는 측정 뒤 기재다 | `non-blind(case, fix, expected)` | §8.1 (g), 7 |

## 9. 1차 출처 점검 기록

### 9.1 방법과 접근 한계

| 방법 | 쓴 곳 | 비고 |
|---|---|---|
| raw.githubusercontent.com의 고정 SHA 파일 | C1, C2, C3, C5, R2 | nasa/CF, nasa/bp, nasa/cFE, nasa/osal. 각 파일을 받아 줄 번호로 확인했다 |
| SourceForge 공식 tarball 재다운로드 후 sha256·크기 대조, 필요한 파일만 추출 | C3, C5 (cFE 6.4.1, 6.4.2, OSAL 4.1.1) | 받은 tarball과 추출물은 별도 임시 디렉터리에서 읽기만 했고 저장소·빌드 트리에 넣지 않았다 |
| 얕은 git fetch | C1 (nasa/CF `833fdbb`, depth 2), R2 (nasa/cFE dev, 2026-03-20 이후, blob 없이) | 부모·시각·변경 파일 목록 확인 |
| 저장소 원시 기록 재집계 | R2 | `env/logs/r2_g1..3_trace.jsonl`을 읽기 전용 스크립트(`python3 -I`)로 새로 집계했다. 파일을 바꾸지 않았다 |
| ENV 트리의 읽기 전용 `git rev-parse HEAD` | R2, C1 | 5개 모듈. 안전 디렉터리 검사는 명령행 옵션으로만 넘기고 전역 설정을 바꾸지 않았다 |
| 작업 트리의 `Architecture/spec/SPEC_FINAL.md`(rev 1.2) 읽기 | 모든 사례의 SPEC 앵커 | 파일은 미커밋 상태다. 확인한 파일의 sha256 앞 8자 `741809d5`(2026-10-15T11:03Z 수정본). 파일이 계속 고쳐지므로 해시가 바뀌면 앵커를 다시 확인한다 |
| nasa/CF `main` 이력 전체의 blobless git fetch(파일 blob 없이) | C1 | 병합 커밋 `2ffbecc`, 수정 `833fdbb`, `281a941`, `2a024d8`의 부모·시각·제목 확인 |
| 작업 트리의 `env/logs/s1r2_p1*` 파일 이름·시각·머리말, `git status` | R2 | 내용·집계는 읽지 않았다. 존재와 시각만 확인했다 |
| `plan/ROADMAP_20261015.md`와 `roadmap_input_20261007.md`의 grep | C1, C5 | 인용된 서술이 어느 로드맵에 있는지 확인 |
| 6.4.1 tarball(sha256 일치 확인 뒤)과 OSAL 4.1.1 artifact(sha256 일치 확인 뒤)에서 readme·PSP·make 파일 읽기 | C3, C5 | 이 개정에서 추가. 추출은 별도 임시 디렉터리에서 읽기만 했다 |
| 로컬 전체 git 클론(`/home/user/work/procrace/refs/` 아래 nasa/CF, nasa/cFE, nasa/osal, nasa/bp)의 `git show`·`git grep`·`git log` | 2026-10-16 재점검(§9.3), 모든 사례 | 읽기만 했다. 작업 트리·빌드 트리를 바꾸지 않았다 |
| 2026-10-15에 받은 SourceForge tarball 사본의 sha256·크기 재계산과 새 빈 디렉터리 추출 | 2026-10-16 재점검(§9.3), C2, C3, C5 | 사본은 이 저장소 밖 임시 디렉터리에 있고 저장소에 넣지 않았다 |
| **접근 불가** | 이슈·댓글·PR 페이지와 API (nasa/CF, nasa/cFE) | HTTP 403(2026-10-15, 2026-10-16 모두) "GitHub access to this repository is not enabled for this session". 우회하지 않았다. 따라서 `†` 행(이슈 본문·댓글 인용)은 이 문서가 재확인하지 못했다. 자격 있는 접근은 PI의 결정 사항이다(X1) |

### 9.2 사례별 확인 결과

| # | 사례 | 사례 기록의 주장 | 1차 출처와 방법 | 결과 |
|---|---|---|---|---|
| V-C1-1 | C1 | CF `2a024d8` `cf_cfdp.c` L1012–L1023은 이름마다 `OS_CountSemGetIdByName` 한 번이며 실패하면 `CF_EID_ERR_INIT_SEM` 후 `goto err_out` | raw nasa/CF@2a024d8 | 일치 |
| V-C1-2 | C1 | 833fdbb는 `OS_ERR_NAME_NOT_FOUND`일 때만 `OS_TaskDelay` 후 재시도(최대 `CF_STARTUP_SEM_MAX_RETRIES`), 상수 25·100(ms) | raw nasa/CF@833fdbb (`cf_cfdp.c` L1000–L1011, `cf_platform_cfg.h` L338·L349) | 일치 |
| V-C1-3 | C1 | 833fdbb의 부모는 281a941, 저자 시각 2022-11-17 16:37:36 -0500 | 얕은 git fetch(depth 2) | 일치 |
| V-C1-4 | C1 | 공개 BP `b051eb0`: 시작 스크립트 L6 `cf … 65`, L7 `bp … 55`; tx 표 L40·L54 `BP0_tsem`·`BP1_tsem`; `bp_semcfg.c` 기본값 | raw nasa/bp@b051eb0 | 일치. CF가 BP보다 먼저 시작한다(입력 로드맵 `roadmap_input_20261007.md`의 "BP를 먼저 시작해도" 서술과 합치하지 않음; `plan/ROADMAP_20261015.md`에는 그 서술이 없다) |
| V-C1-5 | C1 | 기본 표의 `sem_name`이 2a024d8 L42·L51, 281a941 L59·L78, 15a871e L60·L78에서 `""`; 15a871e에 재시도 루프가 있다 | raw nasa/CF@2a024d8, 281a941, 15a871e | 일치 |
| V-C1-6 | C1 | BP PR CI는 `jphickey/cf@fix-184-throttle-sem-race`를 쓰고 "needs to run with a patch to CF"; 러너 `ubuntu-22.04`; cmake 명령 | raw nasa/bp@b051eb0 워크플로 | 일치. 브랜치 이름과 PR #342 head 브랜치 이름의 일치는 V-C1-10 |
| V-C1-7 | C1 | 사례 기록의 전파 서술(F3)은 2a024d8의 `goto err_out` 구조 | raw nasa/CF@281a941 `cf_cfdp.c`, `cf_app.c` | **보완**: 281a941은 `break` 후 말미 `if (ret == CFE_SUCCESS) { enabled = 1; } return ret;`이며 `cf_app.c`가 `APP_ERROR`를 설정(L333)하고 `ExitApp`(L366)한다. 결과(오류 반환, 엔진 미활성)는 같다 |
| V-C1-8 | C1 | 이슈 본문·댓글 5개 | github.com·API | **접근 불가(403)**. 재확인하지 못함 |
| V-C1-9 | C1 | 실패 이벤트의 이름: 2a024d8·281a941·833fdbb에서 `CF_EID_ERR_INIT_SEM`, v7.0.1의 15a871e에서 `CF_INIT_SEM_ERR_EID` | raw nasa/CF@2a024d8, 281a941, 833fdbb, 15a871e의 `fsw/src/cf_cfdp.c` | 일치: 이벤트 호출 줄 L1018, L997, L1016, L1295. 그 사이 리비전은 보지 않았다 |
| V-C1-10 | C1 | PR #342와 BP PR CI의 브랜치 | nasa/CF `main` 이력의 blobless git fetch | 병합 커밋 `2ffbeccc3e75fc69cd4a10652e7cc6abc2e4c90e`(2022-12-01 14:35:23 -0500)의 제목 "Merge pull request #342 from jphickey/fix-184-throttle-sem-race", 부모 `32a8b03`·`833fdbb`. `833fdbb27a26f15f2429e79392e7eb73d61abdea`의 메시지 "Fix #184, work around throttle sem creation race", 저자 시각 2022-11-17 16:37:36 -0500, 부모 `281a941`. BP PR CI의 `upstream-cf-ref: fix-184-throttle-sem-race`(nasa/bp@b051eb0 `build-run-bp-pullreq.yml` L13–L14)와 브랜치 이름이 일치한다. 일치는 근거이지 그 CI가 833fdbb를 썼다는 증명이 아니다(C1.B6a) |
| V-C1-11 | C1 | K5(M1-5)의 측정 대상 | `env/CONDITIONS.md` E4·M1-5, `env/ENV.md` L149 | ENV 프로세스의 측정이며 시작 스크립트의 CF 우선순위는 70이다. BP는 ENV에 없다. 공개 BP 구성(CF 65·BP 55)에서의 스케줄링 측정은 없다(C1.C7) |
| V-C2-1 | C2 | v6.5.0a `cfe_es_start.c` L266–L291: `CORE_READY` → `StartApplications` → `ApplicationSyncDelay(UNDEFINED, …)`(실패 시 syslog만) → `OPERATIONAL` | raw nasa/cFE@b2765d9f | 일치 |
| V-C2-2 | C2 | `WaitForStartupSync`는 호출자를 RUNNING으로 표시하고 `++AppReadyCount`, core는 `CORE_READY`·그 외는 `OPERATIONAL`까지 대기. `cfe_es.h`에 "last item of an Apps initialization", "at least 1000. Lower values will be rounded up" | raw nasa/cFE@b2765d9f (`cfe_es_api.c`, `cfe_es.h` L665·L671) | 일치 |
| V-C2-3 | C2 | v6.6.0a: `APPS_INIT=4`, `OPERATIONAL=5`, `LATE_INIT=2`, `RUNNING=3`; ES 메인이 `MainTaskSyncDelay(LATE_INIT)` → `APPS_INIT` → `MainTaskSyncDelay(RUNNING)` → `OPERATIONAL` | raw nasa/cFE@8811c920 | 일치 |
| V-C2-4 | C2 | 6.6.0 VDD에 "#167 es Additional CFE start up state for application sync"와 커밋 `465a958`·`b227d57` | raw nasa/cFE@8811c920 `docs/cFE_6_6_0_version_description.md` L189–L190 | 일치 |
| V-C2-5 | C2 | dev의 `CFE_ES_WaitForStartupSync`는 `WaitForSystemState(OPERATIONAL, …)`를 부르는 `void` 래퍼, ES 메인은 2단계 | raw nasa/cFE@f22e538c (`cfe_es_api.c` L616–L619, `cfe_es_start.c` L211·L217·L226·L235) | 일치 |
| V-C2-6 | C2 | 6.4.2 tarball sha256 `c64ed2aa…a53a4f`, 36,411,531 B | SourceForge 재다운로드 | 일치 |
| V-C2-7 | C2 | 이슈 #198 본문 | github.com | **접근 불가(403)** |
| V-C3-1 | C3 | 6.4.1 tarball sha256 `ec26e33b…e98f`, 39,586,940 B | SourceForge 재다운로드 | 일치 |
| V-C3-2 | C3 | 6.4.1 생성 순서 EVS, SB, ES, TIME, TBL; 우선순위 61·64·68·60·70(L1115·L1138·L1161·L1188·L1217), TIME 자식 25(L1189–L1190) | 6.4.1 tarball `cfe_es_objtab.c`, `cfe_platform_cfg.h` | 일치 |
| V-C3-3 | C3 | 6.4.1: `EVS_AppID` 초기값 `CFE_EVS_UNDEF_APPID`(L190, 헤더 L105 0xFFFFFFFF), `EVS_GetAppID` L385, `CFE_EVS_Register` L393; SB `GetAppID` L198·등록 L257; TIME 등록 L252·`CreatePipe` L308; `EVS_SendEvent`의 무검사 `EVS_IsFiltered`(L616); `CFE_SB_CreatePipe`의 PIPE_ADDED 이벤트가 `CFE_SB.AppId` 사용(`cfe_sb_api.c` L264) | 6.4.1 tarball | 일치 |
| V-C3-4 | C3 | 6.4.1의 core task 소스는 `CFE_ES_WaitForStartupSync`를 부르지 않는다(주석 제외). 6.4.2의 EVS·SB·ES·TIME·TBL TaskMain은 부른다. 6.4.2 ES는 core task 생성 뒤 `ApplicationSyncDelay`로 기다리다 실패하면 `CFE_PSP_Panic(CFE_PSP_PANIC_CORE_APP)` | 두 tarball의 `cfe-core/src` grep과 `cfe_es_start.c` L936–L949 | 일치 |
| V-C3-5 | C3 | 6.4.2의 F1(범위 검사 L615–L621)·F2(`EVS_AppID`를 L436에 대입) | 6.4.2 tarball | 일치 |
| V-C3-6 | C3 | 6.4.1 ES: 락 L838 → `OS_TaskCreate` L875 → 해제 L933 | 6.4.1 tarball `cfe_es_start.c` | 일치 |
| V-C3-7 | C3 | OSAL 4.1.1은 root일 때만 `SCHED_FIFO`·재매핑 우선순위(`osapi.c` L587 `geteuid() == 0`) | raw nasa/osal@2f1fdb0c | 일치 |
| V-C3-8 | C3 | 현재 dev의 `EVS_SendEvent`는 `EVS_AppDataIsMatch` 검사로 재작성됨; core 모듈 순서 config, es, evs, fs, sb, tbl, time(`mission_defaults.cmake`) | raw nasa/cFE@f22e538c, c5fb2b4d | 일치 |
| V-C3-9 | C3 | 이슈 #73 본문과 댓글 14개(Trac 헤더 포함) | github.com | **접근 불가(403)** |
| V-C3-10 | C3, C5 | SH-5·SH-9의 readme·PSP·make 값 | 6.4.1 tarball(sha256 `ec26e33b…` 일치 확인)의 `cfe-OSS-readme.txt`, `psp/fsw/pc-linux/make/compiler-opts.mak`, `psp/fsw/pc-linux/src/cfe_psp_start.c`, `build/cpu1/cfe/debug-opts.mak`, `build/cpu1/cfe/cfe-config.mak`; OSAL 4.1.1 artifact(sha256 `8f0f2b23…` 일치 확인)의 `build/osal-config.mak` | 일치: readme L14–L16, L76–L78, L83, L85–L89, L98–L100, L137–L142(파일 이름은 예시, L139), L153, L168–L177; `ARCH_OPTS = -m32`(compiler-opts.mak L33); `DEBUG_OPTS = -g -O0`(debug-opts.mak L4); `PSP = pc-linux`(cpu1, cfe-config.mak L32); `OS = posix`(osal-config.mak L21); PSP의 reset 기본 가지는 POWER ON(`cfe_psp_start.c` L244–L252). 6.4.1 readme에는 reset 인수에 관한 문장이 없고 6.4.2 readme L196에 "--reset PO"가 나온다 |
| V-C5-1 | C5 | 6.4.1 `cfe_es_apps.c`: 락 L571, 해제 L610, `OS_TaskCreate` L614, 재획득 L637, `RecordUsed` L649, `AppId` L651, 해제 L673. 파일 sha256 `0a54974b…`, git blob `8c6d0417…` | 6.4.1 tarball | 일치 |
| V-C5-2 | C5 | `CFE_ES_GetAppID`(`cfe_es_api.c` L846–L871)는 `TaskTable[OS_TaskGetId()].RecordUsed`가 아니면 `CFE_ES_ERR_APPID` | 6.4.1 tarball | 일치 |
| V-C5-3 | C5 | 6.4.2: 락 L571, `++AppStartedCount` L607, `OS_TaskCreate` L619, 해제 L669; git blob `94362464…` | 6.4.2 tarball | 일치 |
| V-C5-4 | C5 | OSAL 4.1.1 SourceForge artifact sha256 `8f0f2b23…e61d46`; git tag `2f1fdb0c`와는 `osapi-version.h`만 다름(tag는 `OS_REVISION (0)`, SourceForge는 (1)); posix `osapi.c`는 동일; `pthread_create` L645, `*task_id` L690, `geteuid()` L587 | SourceForge 재다운로드, raw nasa/osal@2f1fdb0c | 일치 |
| V-C5-5 | C5 | `0749d692`의 설계 주석("…before its fully registered in the global app table"); `58abd38d`의 `GetTaskFunction`은 task 레코드만 확인; main `12cb84fc`에서 `OS_TaskCreate` L664, 임계구역 L672–L722, App 레코드 최종화 L861–L869 | raw nasa/cFE@0749d692, 58abd38d, 12cb84fc | 일치 |
| V-C5-6 | C5 | 이슈 #72 본문과 댓글 9개 | github.com | **접근 불가(403)** |
| V-R2-1 | R2 | ENV 트리의 리비전: cFS `088b2fa8…`, cFE `c5fb2b4d…`, CF `15a871e6…`, OSAL `d2d877a6…`, PSP `c4b3b0b6…` | ENV 트리의 읽기 전용 `git rev-parse HEAD` | 일치 |
| V-R2-2 | R2 | ENV cFE: SB 게시 `cfe_sb_task.c` L138; SB 등록 L155; EVS 거부 `cfe_evs.c` L186–L188; ES init `cfe_es_task.c` L367·L377·L387; EVS init `cfe_evs_task.c` L289·L297·L304; ES 직렬화 S1 `cfe_es_start.c` L810·L835; `cfe_sb_init.c` L50 `memset`; `cfe_sb_priv.c` L786; core 순서(`mission_defaults.cmake`) | raw nasa/cFE@c5fb2b4d | 일치 |
| V-R2-3 | R2 | B `fc8c48ad`: `cfe_sb_task.c` L126(게시)·L193(등록), `cfe_evs.c` L182–L186, 객체 표 EVS·SB·ES·TIME·TBL, `cfe_version.h` 6.7.0 개발(빌드 247) | raw nasa/cFE@fc8c48ad | 일치 |
| V-R2-4 | R2 | gdb 3회: SB 호출자 사용 223/229/229, SB 게시 전 24 (함수별 2/4/18, task별 ES 20·EVS 4), AppId 0 24건, 일치하는 `OUT_ILLEGAL_APP_ID` 24, 게시 값 1114115, 게시 hit 50, 등록 hit 51, 트레이스 sha256 앞 8자리 `34759887`/`5dfc9a46`/`7b959389` | 원시 기록을 새 읽기 전용 스크립트로 재집계 | 일치 |
| V-R2-5 | R2 | `CFE_EVS_SendEventWithAppID` 헤더 노트(`cfe_evs.h` L165–L172)와 거부 가지 뒤에만 이벤트 패킷 생성(`EVS_GenerateEventTelemetry` L204) | raw nasa/cFE@c5fb2b4d | 일치 |
| V-R2-6 | R2 | `b9bec567`의 순서 변화 | 얕은 blobless git fetch(nasa/cFE dev, 2026-03-20 이후) | **보완**: 전체 SHA `b9bec5679c748d6a7c1181d74a2f52dd6ba70688`, 저자 시각 2026-03-26, 커밋 2026-04-07, 부모 `063cb3022abd7b8193280fad9552fb11f194b47b`. 부모의 정적 객체 표는 EVS·SB·ES·TIME·TBL, 커밋의 `MISSION_CORE_MODULES`는 config·es·evs·fs·sb·tbl·time. 52개 파일 변경(`cfe_es_start.c`, `cfe_es_apps.c`, `cfe_evs_task.c` 포함). 사례 기록에는 약식 SHA만 있었다 |
| V-R2-7 | R2 | 이슈 #2663 본문과 코멘트 2개, PR #2820 페이지 | github.com | **접근 불가(403)** |
| V-R2-8 | R2 | `results/R2_gdb_v701.md` §2·§5와 `cases/mapping_v701.md` §1.2의 'syscall'·'lost' 서술 | 두 문서를 다시 읽음. `env/logs/r2_g*` 파일 목록과 `r2_g1_trace.jsonl`의 종류·필드를 확인 | **관측이 아니라 코드 읽기:** mapping §1.2는 "No syscall is involved"를 코드 읽기로 적는다. R2_gdb §2·§5의 'no syscall was made'·'invisible … at the syscall level'을 뒷받침하는 시스템 호출 관측 기록은 없다(트레이스는 `SEND_WITH_APPID` 251·`OUT_ILLEGAL_APP_ID` 24·`SB_PUB_PRE`·`SB_PUB`·`SB_EVS_REGISTERED` 각 1이고 필드에 카운터나 시스템 호출이 없다; r2_g1). 이 대장은 후자를 채택하지 않았다(R2.F8a) |
| V-R2-9 | R2 | 첫 Kairos 출력의 존재와 R2 시트의 해시 부재 | `env/logs/s1r2_p1*`의 `ls`·시각·머리말, `git status` | 파일 10개(`.kzt`, `.kzt.record.txt`, `_basis.kz`, `_conditions.tsv`, `_driver.txt`, `_hits.txt`, `_perf_operational_esrestart.log`(+`.meta`), `_persist_before.txt`, `_persist_after.txt`), 2026-10-15T11:17:24Z–11:17:31Z, 기록 머리말은 `kz observe`, 실행 컨테이너 artifact_sha256 앞 8자 `6df021fb`. 내용은 평가하지 않았다. 이 대장 §8에 R2 시트의 해시는 없다 |
| V-SPEC-1 | SPEC | 이 대장이 인용한 SPEC 앵커 | SPEC_FINAL.md rev 1.2 작업 트리(sha256 앞 8자 `741809d5`) | 존재 확인: §0.9(항목 4: "rejected", 항목 5: D22), §6.4 항목 47, §7.2(F1 항목: 사례 시트는 첫 도구 출력 전에 해시로 동결), §7.5 항목 5, §7.6 H-V, §8.2, §9.3(단계표 S0–S10, 사다리 V0–V4·VP-D3/D4/D5, 하위 게이트 S1-R2, 사례별 경로 표, E-post/E-TBL), App. A.1(단계 9·9a·13a·15), A.2, A.3, A.4(단계 2–6), App. C. 본문은 §0.9 항목 4, §6.4 항목 47, §7.2 F1 항목, 용어집의 'Pre-registered expectation sheet' 행(§8.1 규칙 1이 인용하는 시트의 행 구성), §9.3 단계표·사다리·S1-R2·사례별 경로 표, App. A.4 단계 2–6을 읽었다. 용어집의 그 행은 시트를 'frozen by hash at F1'이라 적어 §7.2·§9.3의 '사례의 첫 도구 출력 전'과 어긋난다(SPEC 안의 불일치이며 이 대장은 후자를 따랐다) 그 밖의 앵커 본문은 이 개정에서 다시 읽지 않았다 |

### 9.3 2026-10-16 재점검

2026-10-15의 점검(§9.2)과 다른 경로로 사례마다 사실 세 개 이상을 다시 확인했다. 방법: (1) `/home/user/work/procrace/refs/` 아래의 전체 git 클론(nasa/CF, nasa/cFE, nasa/osal, nasa/bp; 얕은 클론이 아니다)에서 고정 SHA의 파일을 `git show`·`git cat-file`·`git grep`·`git log`로 읽기만 했다(작업 트리·빌드 트리를 바꾸지 않았다). (2) 2026-10-15에 SourceForge에서 받아 이 저장소 밖 임시 디렉터리에 둔 tarball 사본 3개의 sha256과 크기를 다시 계산해 기록값과 일치함을 확인한 뒤, 필요한 파일만 새 빈 디렉터리에 풀어 읽었다. (3) 저장소의 gdb 원시 기록을 새 읽기 전용 스크립트(`python3 -I`)로 다시 집계했다. 이슈·PR 페이지는 2026-10-16에도 HTTP 403이었다(`gh api repos/nasa/CF/issues/184`, `https://github.com/nasa/CF/issues/184`, `https://api.github.com/repos/nasa/cFE/issues/2663`). 따라서 `†` 행은 여전히 재확인되지 않았다.

| # | 사례 | 확인한 주장 | 1차 출처와 방법 | 결과 |
|---|---|---|---|---|
| V-C1-R1 | C1 | C1.A4: `2a024d8` `fsw/src/cf_cfdp.c` L1012–L1023에서 이름마다 `OS_CountSemGetIdByName` 한 번, 실패하면 `CF_EID_ERR_INIT_SEM`("…error=0x%08x") 후 `goto err_out`; 기본 표 `fsw/tables/cf_def_config.c` L42·L51의 `sem_name`이 `""` | 로컬 클론 `git show 2a024d8:…` | 일치 |
| V-C1-R2 | C1 | C1.A5, C1.C10: `833fdbb`는 L1000에서 `ret = OS_ERR_NAME_NOT_FOUND`, L1001 `for (j < CF_STARTUP_SEM_MAX_RETRIES)`, L1006 `NAME_NOT_FOUND`가 아니면 탈출, L1011 `OS_TaskDelay(CF_STARTUP_SEM_TASK_DELAY)`, L1016 실패 이벤트; `fsw/platform_inc/cf_platform_cfg.h` L338 `25`, L349 `100`. `281a941`은 L993 단일 조회, L997 이벤트(`error=%ld`), 이어서 `break` | 로컬 클론 `git show` | 일치 |
| V-C1-R3 | C1 | C1.B4·B6: `833fdbb27a26…`의 부모 `281a94188cd7…`, 저자 시각 2022-11-17 16:37:36 -0500, 제목 "Fix #184, work around throttle sem creation race". 병합 `2ffbeccc…`(2022-12-01 14:35:23 -0500)의 부모 `32a8b035…`·`833fdbb`, 제목 "Merge pull request #342 from jphickey/fix-184-throttle-sem-race" | 로컬 클론 `git log -1` | 일치 |
| V-C1-R4 | C1 | 공개 BP `b051eb0`: `.github/buildconfig/cfe_es_startup.scr` L6 `cf … 65`, L7 `bp … 55`(CF가 BP보다 먼저); tx·rx 표 `cf_def_config.c` L40 `BP0_tsem`, L54 `BP1_tsem`; `fsw/custom/bp_semcfg.c` L44·L48; `build-run-bp-pullreq.yml`의 `upstream-cf-repo: jphickey/cf`, `upstream-cf-ref: fix-184-throttle-sem-race` | 로컬 클론 `git show b051eb0:…`, `git grep` | 일치. 원 보고자 환경의 값이라는 뜻은 아니다(§0.2) |
| V-C1-R5 | C1 | C1.B11: v7.0.1의 CF `15a871e` 기본 표 `fsw/tables/cf_def_config.c` L60·L78의 `sem_name`이 `""`; `cf_cfdp.c` L1290 `OS_TaskDelay(CF_STARTUP_SEM_TASK_DELAY)`(재시도 루프), L1295 `CF_INIT_SEM_ERR_EID` | 로컬 클론 `git show` | 일치 |
| V-C2-R1 | C2 | C2.A3: v6.5.0a `b2765d9f` `cfe/fsw/cfe-core/src/es/cfe_es_start.c` L266–L291이 `CORE_READY` 설정 → `CFE_ES_StartApplications` → `CFE_ES_ApplicationSyncDelay(CFE_ES_SYSTEM_STATE_UNDEFINED, …)`(실패 시 syslog "Startup Sync failed - Applications may not have all started") → `OPERATIONAL` 설정 | 로컬 클론 `git show` | 일치 |
| V-C2-R2 | C2 | C2.F1: v6.5.0a `cfe_es.h` L665 "This API should only be called as the last item of an Apps initialization.", L671 "This parameter must be at least 1000. Lower values" | 로컬 클론 `git show` | 일치 |
| V-C2-R3 | C2 | C2.A4·A5·A7: v6.6.0a `8811c920`의 `cfe_es_extern_typedefs.h` L204 `APPS_INIT = 4`, L209 `OPERATIONAL = 5`; `cfe_es_start.c` L207 `MainTaskSyncDelay(LATE_INIT, …)` → L214 `APPS_INIT` → L223 `MainTaskSyncDelay(RUNNING, …)` → L233 `OPERATIONAL`; `docs/cFE_6_6_0_version_description.md` L189–L190의 #167 행(`465a958`, `b227d57`); dev `f22e538c` `modules/es/fsw/src/cfe_es_api.c` L616–L619의 `void` 래퍼 | 로컬 클론 `git show`, `git grep` | 일치 |
| V-C2-8 | C2 | C2.B2(신규 확인): 6.4.2와 v6.5.0a의 시작 동기화 함수 비교 | 6.4.2 tarball(sha256 일치 확인 뒤 추출)과 로컬 클론의 `b2765d9f` 파일을 함수 단위로 잘라 공백을 지우고 비교 | `CFE_ES_WaitForStartupSync`와 `CFE_ES_ApplicationSyncDelay`는 같다. `CFE_ES_Main`은 인자 형(`uint8 *StartFilePath` 대 `const char *StartFilePath`)과 syslog 인자의 `(unsigned int)` 형 변환 한 곳만 다르고 동기화 말미(6.4.2 L268–L294, v6.5.0a L266–L292)는 같다. 세 함수 밖의 코드는 비교하지 않았다(C2.B2a) |
| V-C2-9 | C2 | C2.B9(신규 확인): 단일 단계 코드에 두 단계 기호가 없다 | 6.4.2 tarball의 `cfe/fsw/cfe-core/src` 전체 `grep -r`, 로컬 클론 `git grep b2765d9f -- cfe/fsw/cfe-core/src`, `git grep 8811c920` | 6.4.2·v6.5.0a: `WaitForSystemState`·`APPS_INIT`·`LATE_INIT`을 담은 파일 0개. v6.6.0a: `cfe_es_api.c`, `cfe_es_start.c`, `cfe_es_task.c` 등에 있다 |
| V-C3-R1 | C3, C5 | SH-1·SH-2·SH-6: tarball sha256·크기 | 2026-10-15 SourceForge 사본의 sha256·크기 재계산 | 일치: cFE 6.4.1 `ec26e33b4ab40b96…5a29c23e98f` 39,586,940 B; cFE 6.4.2 `c64ed2aac910e79a…56b595d47e16a3f53a4f` 36,411,531 B; OSAL 4.1.1 `8f0f2b23d8226256…9fcb560b38434411dc12cb4f7f35b9c19e61d46` 1,973,805 B |
| V-C3-R2 | C3 | C3.A5·C3.C3: 6.4.1 `cfe_es_objtab.c` L120·L122·L124·L126·L129의 core task 순서 EVS, SB, ES, TIME, TBL; `cpu1/cfe_platform_cfg.h` L1115 `61`, L1138 `64`, L1161 `68`, L1188 `60`, L1189·L1190 `25`, L1217 `70` | 6.4.1 tarball에서 추출 | 일치 |
| V-C3-R3 | C3 | C3.A5·C3.E3: 6.4.1 `cfe_evs_task.c` L190 `EVS_AppID = CFE_EVS_UNDEF_APPID`, L385 `EVS_GetAppID`, L393 `CFE_EVS_Register`; `cfe_sb_task.c` L198 `CFE_ES_GetAppID(&CFE_SB.AppId)`, L257 `CFE_EVS_Register`; `cfe_time_task.c` L252 `CFE_EVS_Register`, L308 `CFE_SB_CreatePipe`; `cfe_evs_utils.c` L608 `EVS_SendEvent`, L616 범위 검사 없는 `EVS_IsFiltered(CFE_EVS_GlobalData.EVS_AppID, …)`(함수 머리 주석: "without having to verify that the caller has a valid AppID") | 6.4.1 tarball에서 추출 | 일치 |
| V-C3-R4 | C3 | C3.A6: 6.4.2 `cfe_evs_utils.c` L615–L621의 `EVS_AppID < CFE_ES_MAX_APPLICATIONS` 검사(F1), `cfe_evs_task.c` L436의 `EVS_AppID = AppID`(F2), EVS·SB·ES·TIME·TBL의 `*_task.c`가 각각 `CFE_ES_WaitForStartupSync`를 부름(F3의 일부) | 6.4.2 tarball에서 추출 | 일치 |
| V-C5-R1 | C5 | C5.A3: 6.4.1 `cfe_es_apps.c` L571 락, L610 해제, L614 `OS_TaskCreate`, L637 재획득, L649 `TaskTable[TaskId].RecordUsed = TRUE`, L651 `TaskTable[TaskId].AppId = i`, L673 해제; 파일 sha256 앞 16자 `0a54974ba77af3db`, git blob 앞 8자 `8c6d0417` | 6.4.1 tarball에서 추출, `sha256sum`, `git hash-object`(저장소 밖 파일) | 일치 |
| V-C5-R2 | C5 | C5.A4: 6.4.1 `cfe_es_api.c` L846–L871의 `CFE_ES_GetAppID`가 `Result = CFE_ES_ERR_APPID`로 시작하고 `OS_TaskGetId()`로 찾은 `TaskTable` 항목이 `RecordUsed == TRUE`일 때만 성공 | 6.4.1 tarball에서 추출 | 일치 |
| V-C5-R3 | C5 | C5.B·SH: 6.4.2 `cfe_es_apps.c` L571 락, L607 `++AppStartedCount`, L619 `OS_TaskCreate`, L669 해제(git blob `94362464`). 그 사이의 다른 해제는 L634 하나이며 `OS_TaskCreate` 실패 가지(`RecordUsed = FALSE` 후 `return CFE_ES_ERR_APP_CREATE`)다 | 6.4.2 tarball에서 추출 | 일치(실패 가지 해제는 `cases/C5.md`에 이미 기록됨) |
| V-C5-R4 | C5 | SH-7: OSAL `2f1fdb0c`의 `src/os/posix/osapi.c` L587 `geteuid() == 0`, L645 `pthread_create`, L690 `*task_id = possible_taskid`; 이 파일은 SourceForge 4.1.1 artifact의 같은 파일과 바이트 단위로 같다 | 로컬 클론 `git show`, tarball에서 추출 후 `cmp` | 일치 |
| V-C5-R5 | C5 | C5.A12·C5.F2: `0749d692` `fsw/cfe-core/src/es/cfe_es_apps.c` L500–L506의 설계 주석("…before its fully registered in the global app table"); main `12cb84fc` `modules/es/fsw/src/cfe_es_apps.c` L664 `OS_TaskCreate`, L672 락, L722 해제, L861 락, L869 `CFE_ES_AppRecordSetUsed` | 로컬 클론 `git show` | 일치 |
| V-R2-R1 | R2 | R2.B·E: ENV cFE `c5fb2b4d`의 `modules/sb/fsw/src/cfe_sb_task.c` L138 `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`, L155 `CFE_EVS_Register`; `modules/evs/fsw/src/cfe_evs.c` L186–L188(`EVS_GetAppDataByID`가 NULL이면 `CFE_EVS_APP_ILLEGAL_APP_ID`); `modules/es/fsw/src/cfe_es_start.c` L810 `CFE_ES_StartAppTask`, L835 `CFE_ES_MainTaskSyncDelay(RUNNING, …)` | 로컬 클론 `git show` | 일치 |
| V-R2-R2 | R2 | R2.A5·B: 보고 리비전 후보 B `fc8c48ad`의 `cfe_sb_task.c` L126 `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`, L193 `CFE_EVS_Register` | 로컬 클론 `git show` | 일치 |
| V-R2-R3 | R2 | R2.B5: `b9bec5679c748d6a7c1181d74a2f52dd6ba70688`의 부모 `063cb3022abd7b8193280fad9552fb11f194b47b`, 저자 날짜 2026-03-26, 커밋 날짜 2026-04-07, 부모와의 차이 52개 파일(삽입 817, 삭제 404) | 로컬 클론 `git log -1`, `git diff --shortstat` | 일치. 삽입·삭제 줄 수는 이번에 새로 적은 것이다 |
| V-R2-R4 | R2 | R2.C6·F8·F9: gdb 3회 트레이스의 sha256 앞 8자 `34759887`/`5dfc9a46`/`7b959389`; 각 실행에서 SB 호출자 사용(스택 둘째 프레임이 `cfe_sb*` 파일) 223/229/229, 그중 `SB_PUB` 이전 24(ES 20·EVS 4, 모두 `app_id` 0), `OUT_ILLEGAL_APP_ID` 24(ES 20·EVS 4). 트레이스의 `SEND_WITH_APPID` 전체는 251/257/257 | 저장소 기록 `env/logs/r2_g1..3_trace.jsonl`을 새 읽기 전용 스크립트로 집계 | 일치(저장소 기록의 재집계이며 새 실행이 아니다) |
| V-R2-R5 | R2 | 상태 주의 1, 규칙 10: R2 첫 Kairos 출력 뒤의 상황 | `git log -1 -- env/logs/s1r2_p1.kzt`, `env/logs` 파일 목록, `kairos/` 밖의 `*.kzt` 검색 | `s1r2_p1*` 10개는 커밋 `6be8935`(2026-10-15T23:59Z)에 들어갔다. 그 밖의 Kairos 출력 파일은 없다. 이 대장에 R2 시트의 해시는 여전히 없다 |
| V-R2-R6 | R2 | K6: 실행별 커널 | `env/logs/r2_g1_gdb_operational_esrestart.log.meta` L12, `env/logs/s1r2_p1_perf_operational_esrestart.log.meta` L13, `uname -r` | `6.18.44-fc-v80`, `-fc-v114`, 2026-10-16 `-fc-v139` |
| V-SPEC-2 | SPEC | 이 대장이 인용한 SPEC 앵커(2026-10-16 작업 트리) | `SPEC_FINAL.md` rev 1.2 + 2026-10-16 정합성 수정(sha256 앞 8자 `861041ac`); 커밋 `6be8935`의 파일(`741809d5`)과의 차이를 `git diff`로 읽음 | 존재 확인: §0.9 항목 4('rejected')·5(D22), §6.4 항목 47, §7.2 F1 항목('A case's expectation sheet is frozen by hash before the first tool output on that case (§9.3), not at F1'), §7.5 항목 5, §7.6 H-V, §8.2, §9.3(단계표, 사다리, VP-D3/D4/D5, S1-R2 행, 'the expectation was measured before it was written' 1회), App. A.1 단계 9·9a·13a·15, A.2, A.3, A.4 단계 2–6, App. C. 용어집의 'Pre-registered expectation sheet' 행은 여전히 'frozen by hash at F1 (§7.2)'이어서 §7.2 F1 항목과 어긋난다(V-SPEC-1의 불일치가 남아 있다). 2026-10-16 수정은 D11의 커널 기록 방식, recall의 확장 평가 배치, ADR 주석에 관한 것이며 이 대장이 옮긴 §8의 T 항목 문장을 바꾸는 줄은 차이에서 찾지 못했다 |

## 10. 남은 항목

| # | 항목 | 영향 | 필요한 것 |
|---|---|---|---|
| 1 | 이슈·댓글·PR 페이지에 접근하지 못했다(2026-10-16에도 403, §9.3). C1 댓글 5개(→ 아래 2), C3 댓글 14개(4개 미식별), C5 댓글 9개, R2 코멘트 2개와 PR #2820, C2 #198의 타임라인. `†` 행 전부가 재확인되지 않았다 | C1, C2, C3, C5, R2 | 자격 있는 접근(X1) 또는 PI가 원문 제공 |
| 2 | 입력 로드맵 `roadmap_input_20261007.md`(L86)의 "BP를 먼저 시작해도 객체 생성 전에 CF가 조회" 서술의 근거 댓글([C1-BP])을 읽지 못했고, 공개 BP 시작 스크립트는 CF를 BP보다 먼저 시작한다. `plan/ROADMAP_20261015.md`에는 그 서술이 없다 | C1 | 댓글 열람(X1). 서술 정정을 요청할 대상 문서는 20261015 로드맵이 아니라 입력 로드맵이다(정정 여부는 PI·orchestrator 몫이며 이 문서의 범위 밖) |
| 3 | 공개 BP 통합이 해당 core 조합에서 빌드되는지, BP CI 구성요소의 당시 SHA, core 조합 | C1 | 호환성 확인(D22 적용 순서 1단계); 같은 단계에서 D17 명령(K9)과 K8–K10의 적용성도 확인(C1.D5). 안 되면 이식 차이 기록 |
| 4 | 보류 감시 한도·중단 조건, A2에서의 해제 사건, hang 판정 한계, 보조 문제 범위 | C1 | 파일럿 후 기록(P-C1HOLD) |
| 5 | 281a941의 실패 전파 구조(`break` 말미 반환)는 2a024d8의 `goto err_out`과 다르다. 앵커 감사에서 후속 경로 전체를 다시 읽어야 한다 | C1 | SPEC S3a 앵커 감사 |
| 6 | 단일 단계 셀의 cFE 코드(6.4.2 대 v6.5.0a)와 OSAL/PSP 짝, 역할 앱 정의(R→P 방식, late-init 내용, timeout, 우선순위, 앱 수). 2026-10-16: 세 동기화 함수의 비교는 했다(C2.B2, V-C2-8; 같음). 세 함수 밖의 동등성은 비교하지 않았다(C2.B2a). 단일 단계 코드에 두 단계 기호가 없어(C2.B9, V-C2-9) '단일 단계 × 수정 provider' 셀은 이식 없이 만들 수 없으므로 그 셀을 제외할지 이식할지 정해야 한다. 병행 계획 문서(`plan/case_setup_C2_C3_C5.md` §1, §7 항목 1)의 '6.4.2를 단일 단계 기준으로, 3셀 + 1셀 제외' 안은 제안이며 결정이 아니다 | C2 | D13의 하위 항목 결정, P-C2 구성 단계(셀 제외·이식의 PI 확인) |
| 7 | 공유 6.4.x 기준: OSAL artifact 통일(SourceForge 대 git tag), 빌드 호스트, 폭, 권한, C3 순서 제어, 6.x에서 D17 명령의 적용성 | C3, C5 | X3 결정 |
| 8 | C3의 주 oracle(X7), `AppData[-1]`의 fault 여부, C3와 R2의 계수(X4), 검색 발췌의 지위(X2) | C3, R2 | PI 결정 |
| 9 | `OS_TaskGetId` 순서(A8), stale 항목 도달성, 잔여 창 도달성, 보류 감시 한도, `mv osal` 문제 | C5 | 관측·결정 |
| 10 | B가 보고 시점 main인지(D1), 보고의 "ES and EVS"가 B에서 성립하는지(Q1), 보고자 환경, '현재 코드' 기준 리비전(X6) | R2 | 실행·PI 결정 |
| 11 | (c2) 대조 쌍은 52개 파일을 바꾼다. 순서 이외의 동시 변경이 판정에 영향을 주는지 먼저 확인해야 한다 | R2 | P-R2CTRL 첫 단계 |
| 12 | SPEC S1 기록기의 R2 첫 실행(`env/logs/s1r2_p1*`, 2026-10-15T11:17Z)이 있다. 이 문서는 그 내용을 평가하지 않았고 판정기·증명 확인기의 구현 상태도 확인하지 않았다(`kairos/`는 다른 작업이 진행 중이다). §8의 모든 T 항목은 SPEC이 서술한 기대이며 도구 출력이 아니다 | 전부 | 구현 진행(별도). R2 시트의 동결 순서는 항목 16 |
| 13 | 커널 변경 후 기준 실행 재측정이 없다(D11). 커널은 2026-10-16에 다시 바뀌었다(-fc-v80 → -fc-v114 → -fc-v139, K6); D11은 'SPEC S1 때의 커널에서, 실행마다 기록'으로 적용된다(K7). N의 값이 없다(D14). `env/CONDITIONS.md`의 'PI decision pending' 표시(E5, E7, E11)는 D19·D17 결정 뒤에도 갱신되지 않았고 A2(번들 리비전)는 여전히 pending이다 | R2와 ENV 기반 전부 | D11 재측정, SPEC S1에서 N, 해당 기록의 갱신(이 문서는 그 파일을 고치지 않았다) |
| 14 | 시트 해시의 직렬화·기록 위치·담당자 | 전부 | 구현자와 PI |
| 15 | ROADMAP §6.1의 소스 기준 필드가 요구하는 "앱의 커밋 또는 배포 파일 해시" 중 C1의 provider 앱(BP)·bplib·tinycbor의 당시 SHA와 C2의 역할 앱은 아직 없다 | C1, C2 | 기준 구성 확정 뒤 기록 |
| 16 | R2 시트(§8.6)는 R2의 첫 Kairos 출력(`s1r2_p1`, 2026-10-15T11:17Z)보다 먼저 해시로 동결되지 않았고 지금도 해시가 없다. SPEC §7.2·§9.3과 §8.1 규칙 5(a)의 순서를 지키지 못했다 | R2 | orchestrator·PI의 결정: 시트를 지금 동결할지, 변경 로그에 실제 순서(출력 시각 → 동결 시각)를 어떻게 적을지, E-R2-T3를 어떻게 표시할지. SPEC 용어집의 'frozen by hash at F1'과 §7.2·§9.3의 '사례의 첫 도구 출력 전'이 어긋나므로(V-SPEC-1) 어느 시점을 시트의 동결 시점으로 볼지도 정해야 한다. 이 문서는 시트를 출력에 맞추어 고치지 않았다. 2026-10-16 상태: `research/cfs_procrace`의 Markdown 문서(`kairos/` 제외)에서 R2 시트 동결에 관한 결정 기록을 찾지 못했고 출력 파일은 커밋 `6be8935`에 들어갔으며 시트 해시는 여전히 없다(V-R2-R5). 용어집과 §7.2의 어긋남도 2026-10-16 작업 트리에 남아 있다(V-SPEC-2) |
| 17 | `results/R2_gdb_v701.md` §2·§5가 '24 SB events were lost silently'·'no syscall was made'·'invisible … at the syscall level'이라고 적는다. 이 대장은 채택하지 않았다(R2.F8a). SPEC §0.9 항목 4, §6.4 항목 47과 `plan/ROADMAP_20261015.md` §3은 'rejected' 표현과 코드 분석/관측의 구분을 요구한다 | R2 | 그 기록 파일의 문장 정정 여부(이 문서의 범위 밖; 과거 기록의 보존 여부는 PI·orchestrator). 정정 전에는 이 대장이 그 표현을 인용하지 않는다 |

## 11. 이 대장의 변경 기록

| 날짜 | 변경 |
|---|---|
| 2026-10-15 | 최초 초안. 다섯 사례의 조건표(필드 A–I), 공통 기준(ENV, 공유 6.4.x), 사례별 기대 결과표 골격과 해시 규칙, 1차 출처 점검 기록, 남은 항목. 작성 중 새로 확인·보완한 것: (1) `b9bec567`의 전체 SHA·부모·변경 파일 수(R2.B5), (2) 281a941의 실패 전파 구조(C1.F4), (3) cFE 6.4.1·6.4.2와 OSAL 4.1.1 artifact의 sha256·크기 재확인 및 OSAL SourceForge/git tag 차이의 재확인(SH-6, SH-7), (4) 공개 BP CI 워크플로의 러너·cmake 명령·패치 브랜치 지정. 사례 기록(`cases/*.md`)과 어긋난 사실은 발견하지 못했다(보완은 (1)·(2)뿐이다) |
| 2026-10-15 (검토 반영) | 외부 검토 11건을 verify하고 고쳤다. (1) R2.F8을 쪼갰다: 도달은 `저·M·검`(R2.F8), 콘솔·카운터·syscall 비가시성은 코드 읽기 `분`(R2.F8a, 'syscall: not observed; code analysis only'). 'losses'를 R2.F9·E-R2-P7에서 '거부된 이벤트 요청'으로 바꿨다. (2) C1.D5를 `미`로, C1.C7을 `분`으로 내렸다(K5·D17은 ENV의 것이고 D22 기준에 옮겨지지 않았다). (3) R2에 이미 있는 Kairos 첫 출력(`s1r2_p1`, 11:17Z)과 시트 해시 부재를 '상태 주의', §8.1 규칙 10, §10 항목 12·16에 적었고 SPEC 참조를 rev 1.2로 올렸다(V-SPEC-1). (4) E-R2-P1에서 'ENV 기본 구성의 매 부팅'을 'gdb 아래 3회의 각 부팅'으로 고쳤다. (5) 6.4.1 readme의 OSAL 서술을 직접 다시 읽어 SH-5(`원·검`, 줄 번호 인용)와 해석 SH-5b(`분`)로 나눴다. (6) SH-9를 `원·검`으로 고치고 V-C3-10을 붙였으며 D13이 정한 것은 SH-9a로 분리했다. C3.C8도 같다. (7) `미`였던 R2.C7·R2.C9·K2를 `선(차이)`로, C2.C8을 `선(차이)`로 고쳐 C2.I2(4)와 맞추고 §1.1을 다시 셌다. (8) '로드맵' 서술의 출처를 입력 로드맵 `roadmap_input_20261007.md`로 바로잡았다(C1.A9, C5.A10, V-C1-4, §10 항목 2). (9) C1.G1의 이벤트 이름과 C1.B6의 PR CI 브랜치 근거를 고치고 V-C1-9·V-C1-10을 추가했다. (10) 혼합 행을 쪼개고(C3.E6/E6a, C5.E6/E6a) `†`를 붙였으며(C2.B1, C3.C3, C5.F2) §1을 '확정'과 '원문 미재확인'으로 나눴다. (11) §2.2의 행 ID를 `S1`–`S17`에서 `SH-1`–`SH-17`로 바꾸고 SPEC·ES 직렬화 단계에 접두어를 붙였으며 §0.5에 표기표를 넣었다. 거절한 항목은 없고 처리하지 않은 부분은 §12에 있다 |
| 2026-10-16 (재점검) | (1) 사례마다 사실 3개 이상을 2026-10-15와 다른 경로(로컬 전체 git 클론, tarball 사본의 sha256 재계산 뒤 추출, gdb 원시 기록 재집계)로 다시 확인했다(§9.3, 26행). 사례 기록이나 이 대장과 어긋난 사실은 없었다. (2) C2: 6.4.2와 v6.5.0a의 세 동기화 함수를 직접 비교해 C2.B2를 `원·검`으로 올리고 비교하지 않은 부분을 C2.B2a(`분`)로 남겼다. 단일 단계 코드에 두 단계 기호가 없음을 확인해 C2.B9를 더하고 E-C2-P4를 고쳤다. §1.1의 C2 행을 다시 셌다. (3) SPEC 작업 트리가 2026-10-16 정합성 수정으로 `861041ac`가 되어 앵커를 다시 확인했다(V-SPEC-2; T 항목 문장에 영향 없음, 용어집과 §7.2의 어긋남은 남음). (4) 커널이 -fc-v139로 바뀌어 K6·K7을 실행별 기록으로 고쳤다. (5) R2 첫 Kairos 출력이 커밋 `6be8935`에 들어갔고 그 밖의 출력은 없으며 R2 시트 해시는 여전히 없음을 기록했다(상태 주의 1, §10 항목 16). (6) 이슈·PR 페이지는 여전히 403이다. 값을 새로 정하지 않았고 해시를 계산하지 않았다 |

## 12. 거절한 검토 항목과 처리하지 않은 부분

- **거절한 검토 항목: 없음.** 11건 모두 원출처·저장소 기록으로 다시 확인했고 지적이 맞았다(확인한 내용은 §9.2의 V-C1-9–11, V-C3-10, V-R2-8, V-R2-9, V-SPEC-1과 해당 행).
- **검토의 제안과 다르게 처리한 부분.** (a) 검토 3: SPEC 참조를 'rev 1.2 또는 commit/hash'로 고정하라는 제안은 rev 1.2와 파일 sha256 앞 8자로 고정했다(커밋된 파일은 rev 1.1이어서 커밋 해시로는 고정할 수 없다). (b) 검토 6: S9를 `원` 행과 `선(D13)` 행으로 나누라는 제안에서, `선(D13)` 행은 S10과 겹치지 않도록 'D13이 정한 것과 정하지 않은 것'(SH-9a)으로 썼다. (c) 검토 7: R2.C7·R2.C9를 `저`와 `선(차이)`의 둘로 나누지 않고 `선(차이)` 하나로 두었다. 쓴 값의 기록은 R2.C6(`저·M`)에 이미 있다. (d) 검토 11: 행 ID 접두어로 제안된 `SB-`는 cFE의 Software Bus(SB)와 겹쳐 `SH-`(shared)로 했다.
- **처리하지 않은 부분 1 — R2 시트의 동결 결정.** 검토는 orchestrator가 R2 시트를 지금 동결할지 정하라고 했다. 이 문서는 시트에 해시를 만들지 않았고 시트를 출력에 맞추어 고치지 않았으며 실제 순서만 기록했다(규칙 10, §10 항목 16).
- **처리하지 않은 부분 2 — 쪼개지 않은 혼합 행.** 검토의 '혼합 행을 쪼갠다'는 이름을 든 행(C3.E6, C5.E6, C1.C7)과 `†` 누락 행(C2.B1, C3.C3; 같은 사유로 C5.F2)에 적용했다. C1.A5, C1.C10, C1.E4, C3.E3, C5.A5, C5.E3, C5.F1처럼 태그가 직접 확인한 주장 부분에 붙고 곁가지만 `저`로 구분해 둔 행은 쪼개지 않았다.
- **처리하지 않은 부분 3 — 다른 문서.** `results/R2_gdb_v701.md`의 'lost silently'·'no syscall was made' 문장(§10 항목 17), 같은 유형의 과장이 `plan/` 아래 다른 문서나 `Architecture/`에 있는지는 이 문서의 범위 밖이어서 확인하지 않았다. 입력 로드맵 `roadmap_input_20261007.md`의 서술 정정 여부도 같다.
- **처리하지 않은 부분 4 — SPEC 본문 대조.** SPEC rev 1.2에서 앵커의 존재는 확인했으나 이 대장이 SPEC에서 옮긴 모든 기대 문장(§8의 T 항목)을 rev 1.2 본문과 한 줄씩 대조하지는 않았다(V-SPEC-1). 검토 보고는 A.4, §8.2, §9.3, §7.6, VP-D4/D5가 rev 1.2에서도 유지된다고 적었다.
- **2026-10-16 개정에서 처리하지 않은 부분.** (a) R2 시트의 동결 여부는 여전히 orchestrator·PI의 결정으로 남겼고 해시를 만들지 않았다(§10 항목 16). (b) `s1r2_p1*`의 내용은 이번에도 읽거나 평가하지 않았다. (c) `†` 행은 이슈·PR 페이지가 403이어서 다시 확인하지 못했다. (d) SPEC은 커밋된 rev 1.2(`741809d5`)와 작업 트리(`861041ac`)의 차이만 읽었다. 이 대장이 옮긴 §8의 T 항목을 SPEC 본문과 한 줄씩 대조하는 일(처리하지 않은 부분 4)은 이번에도 하지 않았다. (e) 병행 계획 문서 `plan/case_setup_C2_C3_C5.md`는 C2.B2·C2.B5의 비고와 §10 항목 6에서 가리키기만 했고 그 문서의 다른 주장은 이 대장의 행으로 옮기지 않았다. `plan/C1_basis_plan.md`는 §3 머리말대로 인용하지 않았다.
