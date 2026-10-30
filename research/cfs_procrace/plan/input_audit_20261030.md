# 입력 감사 — 사람 입력과 사례 지식이 들어오는 곳, 재설계에서의 대체 (2026-10-30)

- 성격: **감사 문서**다(RA §4 단계 0). 탐지 결과나 구현이 아니다. 엔진·프로필·계획·드라이버 파일은 고치지 않았다. cFS를 기동하지 않았다.
- 입력: 네 갈래 조사의 결과를 합쳤다. 프로필(`kairos/profiles/**` 57 파일), 추출기 입력(kz-static), 엔진·런타임의 내장 지식, 관측·제어 계획·예측 모델·드라이버다. 여기에 이 문서의 완전성 단계(§5)에서 직접 찾은 행을 더했다.
- 근거: `plan/REDESIGN_REVIEW_ANALYSIS_20261030.md`(이하 RA) §1 E1–E3, §2, §3.1, §4; `plan/REDESIGN_PROPOSAL_20261029.md`(이하 RP, 10-30 정정 머리말 포함); `plan/extraction_design_20261030.md`(이하 ED) §1–§3; `kairos/docs/phi_reference.md`, `kairos/docs/static_facts.md`. 검토서 RV2 원문은 저장소에 없다. RV2의 절 번호는 RA·ED가 인용한 대로만 쓴다 [U: RV2 원문과 대조하지 않음].
- 표기: [M] 이번에 도구로 확인, [F] 소스·문서에서 읽음, [I] 추론, [D] 이 문서의 분류 결정, [U] 미확인. 조사 결과에서 온 행은 그 조사가 [F]/[M]으로 확인한 것이다. 이 문서가 다시 확인하지는 않았다(과제 지시: 이미 확인한 것은 다시 확인하지 않음).
- 경로 약호: `cfs/` = `kairos/profiles/cfs-v7.0.1/`, `c1/` = `kairos/profiles/c1-bp-b051eb0/`, `c5/` = `kairos/profiles/c5-cfe-6.4.x/`. `lib/`·`include/`·`tools/`·`runtime/`·`probe/`·`instrument/`·`tests/`·`docs/`는 `kairos/` 아래, `env/`·`plan/`·`Architecture/`는 R/ 아래다. `EB/`는 ED의 출력 `/home/user/work/redesign/extract_baseline/`이다.

## 0. 요약

1. **행 수** [D]
   - 네 조사의 행이 292, 완전성 단계의 행이 18이고, 합하면 310행이다.
   - 같은 입력을 두 조사가 따로 적은 43행을 합치면 267행이 남는다. 이 행들이 묶은 항목(선언·인용·site·시험)은 1415개다.
   - 사례 전용 여부: 사례 전용(Y) 110행, 섞임(M) 64행, 사례 무관(N) 93행.
   - 후보 생성이나 판정에 영향을 주는 사례 전용 행은 Y 69행, M 35행이다(§3).
2. **판정은 모두 사람이 쓴 입력에서 온다.** 판정층(rspec·appreq·allowed·harm·역할) 32행이 모두 사례 전용이다. 엔진은 이 층만 판정 근거로 받는다. issue-derived 글은 근거로 받고 code-derived 사실은 받지 않는다(E01, E02).
3. **추출 범위도 사람 입력이 연다.**
   - kz-static의 writer·INIT·A-TYPE·LOCS·OUTCALLS는 프로필의 `loc`·`bind`·store 패턴과 C1 명령줄 `--target` 7개에서만 생긴다(KS-P01–P03, KS-C01). derives는 프로필의 역할 인자에서만 생긴다(KS-P04). 빈 프로필이면 모두 0이다.
   - facts에는 판정층까지 포함한 프로필 해시가 실린다. 그래서 rspec 변형마다 다시 추출한다(KS-P09).
4. **관측·계측·제어·예측은 사례 선택이다.**
   - manifest site는 Φ의 LP 이름에 맞춰 사람이 두었다(OBS-0). F0는 R2 40, C1 33/34 site이고, kzs는 R2 44, C1 37/38, C5 33/31 site다 [M]. 계측 모듈도 사례 앱뿐이다(INS-SCOPE).
   - hold 계획 2개는 손으로 썼다. 나머지 28개는 손으로 쓴 예측 모델 3개에서 생성됐다(HLD-*, HOLD-PRED, PRD-*).
5. **엔진 안의 cFS 지식은 함수 이름이 아닌 경로로 들어온다**(E 행).
   - 닫힌 어휘와 규칙 모양: 판독기, hb 템플릿, 종료 범위, 관계 범주, harm.
   - 명령줄 값: `--core-request-caller`, `--const`.
   - 내장 ID 공식(E11).
   - 판정 근거 정책(E01).
   - 한 사례를 본뜬 구성: existence_probe(E05), retry_until_ready(E03), app-request(E12).
   - 사례 식별자 lint는 `include/`·`lib/`만 본다(C-05).
6. **지금의 계수는 사람 입력을 적게 센다.**
   - `kz-phi stats`의 hand-entered는 cfs 3, C1 1, C5 1이다(C-06 [M]). code-derived로 분류된 항목도 사람이 골라 쓴 주장인데 세지 않는다.
   - kz-static의 hand-entered는 내장 표와 명령줄만 센다(KS-P10).
7. **오늘 사람의 의미 입력 없이 나오는 것은 좁다**(§7).
   - kz-static의 V0 기초 사실(호출, 인자 종류, 결과 사용·검사 플래그, 루프 종류, table walk, 간접 호출 대상, 함수마다 필드 첫 쓰기)과 빌드 정체성이다. 오늘 자동(A)인 행은 5행뿐이다.
   - R2의 필드 → 인자 흐름(39곳)과 C1 수정 후 재시도 루프(상한 25)는 사례 프로필 없이 나온다. 다만 재시도 루프에는 사례 무관 지연 연산 이름 하나가 필요하다(ED §2.4).
8. **대체**(§6). 집계 행 267의 분류는 다음과 같다.
   - 일반 추출(G) 122, 도구 내부 API 의미(T) 33, 일반 기구 유지(K) 31, 삭제(D) 27, 평가 전용(V) 15, 사용자 입력(U) 14, 기록 관측 사실(Q) 11, 실행 설정(S) 8, 열림(O) 5.
   - 사례 전용 110행은 G 63, D 19, V 14, Q 8, U 5, O 1로 간다.
   - G로 간 행 대부분은 ED의 새 사실 F1–F7이 있어야 실제로 대체된다. 지금 사실만으로 대체되는 것은 일부다(§7.4).

## 1. 방법

### 1.1 네 조사의 범위 [F]

| 조사 | 읽은 범위 | 행 |
|---|---|---|
| 프로필 | `kairos/profiles/**` 57 파일(Φ 3 프로필과 변형 디렉터리, basis recipe 2, E-ST 머리 프로필 2, F0 observe 3, instr 5, hold 2, predict 3). 의미는 `docs/phi_reference.md`, kz-static 능력은 `docs/static_facts.md`로 판단 | 172 |
| 추출기 입력 | `lib/llvm/*`, `tools/kz-static/*`와 EB의 V0–V3 산출물(`kz-static show`로 다시 읽음) | 44 |
| 엔진·런타임 | `lib/core`, `lib/trace`, `tools/kz`, `tools/kz-dump`, `tools/kz-phi`, `include/**`, `runtime/**`, `instrument/gcc/kzgcc.cpp`. `lib/llvm`, `lib/predict`, `probe/`는 범위 밖 | 31 |
| 계획·드라이버 | observe/instr manifest, hold 계획, predict 모델, `env/`·`instrument/gcc/scripts`·`probe/scripts` 드라이버, 기대표 | 45 |

### 1.2 열 약호

- **종류**(input_kind): prof = profile, obs = observation-manifest, rcp = recipe, hold = hold-plan, pred = predict-model, cli = cli-option, drv = driver-script, eng = hardcoded-engine, tbl = hardcoded-table, exp = expected-table, test = 시험(완전성 단계에서 새로 둔 종류), oth = other.
- **영향**: C 후보 생성, J 판정, O 관측, E 순서 간선, I 정체성, X 추출 범위, H 후속 처리, R 보고.
- **사례**: Y 사례 전용, M 섞임(판 일반 내용을 사례에 맞춰 고름), N 사례 무관.
- **수**: 조사가 센 항목 수다(한 행이 묶은 선언·인용·site·시험의 수). 단위가 행마다 다르다. 같은 파일 안 범주가 겹칠 수 있다. 예를 들어 R2 F0 manifest는 40 site인데, OBS-cfs-* 네 행의 합은 49다. 그래서 수의 합은 분량의 표시일 뿐이다.
- **대체**(§6): G 일반 추출, T 도구 내부 API 의미(판 표기·원문 대조·공개), Q 기록 관측 사실, V 평가 전용, D 삭제, U 사용자 입력, K 일반 기구 유지, S 실행 설정(기록·라벨), O 열림, - 해당 없음. 첫 글자가 분류이고 뒤는 근거다.
- **오늘**(§7): A 사람의 의미 입력 없이 빌드·기록에서 나옴. P 도구가 사실을 내거나 검증하지만, 무엇을 쓸지는 사람이 정함(인용 해시 대조, 상수 평가, 예측 계획 생성). N 사람이 쓴 입력. - 의미 지식이 아닌 기구·설정·공백.
- **위치**: 디렉터리 없이 파일 이름만 적은 위치는 다음 디렉터리에 있다. `ast_*.cpp`·`basis.cpp`·`finalize.cpp`·`unroll.cpp`·`serialize.cpp`·`spec.cpp`·`linkset.cpp`·`recipe.hpp`·`facts_query.*`는 `lib/llvm/`, `cmd_extract.cpp`·`cmd_basis.cpp`는 `tools/kz-static/`, `cmd_replay.cpp`·`cmd_hold.cpp`는 `tools/kz/`, `eval_*.cpp`·`phi_*.cpp`는 `lib/core/`, `real_trace.cpp`·`identity_*.cpp`·`occ_table.cpp`·`kzrt_convert.cpp`·`decode_sites.cpp`·`static_closure.cpp`는 `lib/trace/`, `kz_schema.h`·`kz_raw.h`·`kz_neutral.h`는 `include/kz/schema/`, `phi_ast.hpp`·`eval_trace.hpp`는 `include/kz/core/`, `kz_rt.c`는 `runtime/src/`, 드라이버 스크립트는 4.9 표의 첫 언급 위치다. 줄 번호는 조사 시점(커밋 `442d309`)의 것이다.
- **집계**: ● 집계 행. (+X)는 그 행에 합친 중복 행이다. →X는 X에 합쳐 집계에서 뺀 행이다.

### 1.3 중복 처리 [D]

- 같은 입력을 두 조사가 적었으면 한 행만 센다.
  - 프로필 파일의 내용은 프로필 조사 행이 기준이다.
  - 계획 조사의 세부 행(OBS-C1-*, OBS-R2-*, OBS-ID, INS-C1/R2/C5 …, HOLD-B/C, PRED-COMMON/CF)은 그 파일의 프로필 조사 행으로 합친다.
  - 추출기 조사의 "프로필 → 추출" 효과 행(KS-P01–P08, KS-P11)은 같은 선언의 프로필 행으로 합친다. 다만 그 행의 사실(예: `out` 역할과 `poll`은 소비처가 없는 죽은 입력)은 표에 남긴다.
  - 드라이버의 명령줄 값은 엔진·추출기 조사 행이 기준이다. DRV-TARGET → KS-C01, DRV-OPS → KS-C02–C04, DRV-CONST → E22, DRV-CORECALLER → E09, DRV-READINGS → E23, DRV-CLOSURE → KS-P09, KS-N04 → E21이다.
  - 프로필의 recipe 행(RCP-*)은 추출기 조사의 KS-R 행으로 합친다.
- 합칠 때 영향은 첫 대상 행에 합집합으로 더한다.
- 조사끼리 판단이 다른 곳이 하나 있다. `front_end_diagnostic`(PI D7)을 프로필 조사는 Y, 추출기 조사는 N으로 적었다. 집계 행 KS-R04는 N으로 둔다. 이 진단은 빌드 호환을 위한 분석 전용 편차이고 사례의 의미를 담지 않기 때문이다. 합친 행 RCP-c5-fed에는 Y를 그대로 둔다.

## 2. 요약표 [D]

### 2.1 절별 행 수

| 절 | 조사 행 | 집계 행 | Y | M | N | 항목(집계) |
|---|---|---|---|---|---|---|
| 4.1 판정층 | 32 | 32 | 32 | 0 | 0 | 32 |
| 4.2 머리·출처·연구자·이슈·인용 | 52 | 52 | 18 | 14 | 20 | 329 |
| 4.3 모델층 | 56 | 56 | 17 | 21 | 18 | 215 |
| 4.4 추출기 입력 | 50 | 34 | 2 | 2 | 30 | 106 |
| 4.5 엔진·런타임 | 31 | 31 | 1 | 13 | 17 | 104 |
| 4.6 관측·계측 | 33 | 18 | 12 | 6 | 0 | 443 |
| 4.7 제어 | 10 | 7 | 6 | 1 | 0 | 34 |
| 4.8 예측 | 10 | 8 | 7 | 1 | 0 | 29 |
| 4.9 드라이버·보고·기대표 | 18 | 11 | 8 | 2 | 1 | 29 |
| 5 완전성 단계 | 18 | 18 | 7 | 4 | 7 | 94 |
| 계 | 310 | 267 | 110 | 64 | 93 | 1415 |

### 2.2 종류 × 사례 × 영향 (집계 행)

영향 열은 그 영향을 가진 행의 수다. 한 행이 여러 영향을 가질 수 있다.

| 종류 | 사례 | 행 | 항목 | C | J | O | E | I | X | H | R |
|---|---|---|---|---|---|---|---|---|---|---|---|
| prof | Y | 67 | 207 | 22 | 47 | 7 | 5 | 9 | 10 | 14 | 16 |
| prof | M | 35 | 224 | 6 | 19 | 7 | 13 | 12 | 7 | 3 | 6 |
| prof | N | 39 | 146 | 4 | 17 | 2 | 13 | 15 | 1 | 5 | 14 |
| obs | Y | 11 | 291 | 3 | 5 | 11 | 2 | 5 | 5 | 3 | 2 |
| obs | M | 6 | 149 | 0 | 1 | 4 | 1 | 3 | 1 | 0 | 2 |
| rcp | M | 2 | 21 | 1 | 0 | 1 | 0 | 0 | 1 | 0 | 1 |
| rcp | N | 5 | 21 | 0 | 0 | 1 | 0 | 2 | 5 | 0 | 0 |
| hold | Y | 5 | 32 | 1 | 0 | 4 | 5 | 0 | 0 | 5 | 2 |
| pred | Y | 6 | 24 | 5 | 2 | 0 | 6 | 1 | 0 | 2 | 1 |
| pred | M | 1 | 1 | 0 | 0 | 0 | 1 | 0 | 0 | 0 | 0 |
| cli | Y | 4 | 26 | 3 | 2 | 1 | 0 | 1 | 2 | 1 | 1 |
| cli | M | 2 | 2 | 0 | 2 | 0 | 1 | 1 | 0 | 0 | 1 |
| cli | N | 13 | 32 | 3 | 5 | 2 | 6 | 5 | 3 | 2 | 2 |
| drv | Y | 11 | 21 | 0 | 0 | 4 | 0 | 1 | 1 | 2 | 9 |
| drv | M | 3 | 5 | 0 | 0 | 2 | 2 | 0 | 0 | 1 | 2 |
| eng | Y | 1 | 1 | 1 | 1 | 0 | 0 | 0 | 0 | 0 | 1 |
| eng | M | 10 | 29 | 2 | 5 | 3 | 1 | 2 | 0 | 3 | 2 |
| eng | N | 28 | 50 | 4 | 5 | 6 | 10 | 8 | 5 | 2 | 10 |
| tbl | M | 2 | 34 | 0 | 1 | 0 | 0 | 0 | 0 | 0 | 2 |
| tbl | N | 3 | 18 | 1 | 0 | 1 | 0 | 2 | 0 | 0 | 2 |
| exp | Y | 1 | 3 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1 |
| test | Y | 3 | 51 | 0 | 0 | 1 | 0 | 0 | 0 | 0 | 3 |
| test | M | 1 | 6 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1 |
| test | N | 1 | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1 |
| oth | Y | 1 | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1 |
| oth | M | 2 | 9 | 0 | 1 | 0 | 0 | 0 | 0 | 0 | 2 |
| oth | N | 4 | 10 | 1 | 0 | 2 | 0 | 0 | 1 | 0 | 1 |
| **계** | | **267** | **1415** | **57** | **113** | **59** | **66** | **67** | **42** | **43** | **86** |

사례별 합:

| 사례 | 행 | 항목 | C | J | O | E | I | X | H | R |
|---|---|---|---|---|---|---|---|---|---|---|
| Y | 110 | 657 | 35 | 57 | 28 | 18 | 17 | 18 | 27 | 37 |
| M | 64 | 480 | 9 | 29 | 17 | 19 | 18 | 9 | 7 | 19 |
| N | 93 | 278 | 13 | 27 | 14 | 29 | 32 | 15 | 9 | 30 |

읽기 [I]:
- 판정(J)에 닿는 집계 행의 다수는 Y·M이다. 그 Y의 대부분이 프로필이고, 나머지는 관측 결합(OBS-0 등), 역할 구성(E05), 예측 모델, 드라이버의 프로필 선택이다. 판정 정책 자체(E01, E02)는 사례 무관(N)이지만 사례 입력만 근거로 받는다.
- 사례 무관(N) 행에서는 정체성(I)과 순서 간선(E)이 가장 많다. 대부분 OSAL·cFE 기본 의미, 종료 의미, 엔진 규칙 모양이다. 이것들은 RV2 §4.2가 허용하는 도구 내부 지원 후보지만, 지금은 판 표기·원문 대조 없이 프로필이나 코드에 흩어져 있다.

### 2.3 사례 × 대체

| 사례 | G | T | Q | V | D | U | K | S | O | - | 계 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| Y | 63 | 0 | 8 | 14 | 19 | 5 | 0 | 0 | 1 | 0 | 110 |
| M | 41 | 5 | 3 | 1 | 1 | 5 | 3 | 2 | 3 | 0 | 64 |
| N | 18 | 28 | 0 | 0 | 7 | 4 | 28 | 6 | 1 | 1 | 93 |
| 계 | 122 | 33 | 11 | 15 | 27 | 14 | 31 | 8 | 5 | 1 | 267 |

### 2.4 사례 × 오늘

| 사례 | A | P | N | - | 계 |
|---|---|---|---|---|---|
| Y | 0 | 9 | 101 | 0 | 110 |
| M | 0 | 8 | 51 | 5 | 64 |
| N | 5 | 16 | 43 | 29 | 93 |
| 계 | 5 | 33 | 195 | 34 | 267 |

## 3. 후보 생성·판정에 영향을 주는 사례 전용 항목

과제 1의 요구에 따라 모든 행을 적는다. 내용은 줄였다. 위치와 전체 문안은 §4·§5의 표에 있다.

### 3.1 사례 전용(Y), 69행

| ID | 영향 | 내용(요약) | 대체 |
|---|---|---|---|
| RSP-cfs | CJ | rspec K-ID.send-with-appid: use `CFE_EVS_SendEventWithAppID`, key evs.reg(arg.AppID), requires valid ∧ REG.… | D 이 API를 고른 선택은 없앤다. use·key·requires 자리는 callee 자신의 검사에서 G(F2, RA §3.1 신호) |
| RSP-c1-281 | CJ | K-REG.osal-countsem-lookup(CF 281a941): use `OS_CountSemGetIdByName`, key osal.countsem(arg.sem_name), requi… | D (RSP-cfs와 같음) |
| RSP-c1-833 | CJH | 같은 rspec의 833fdbb판. allowed와 prov 목록만 다르다 | D |
| RSP-c1-281alt | J | 열린 PI 결정 P3의 대안 읽기(appreq 미지정, allowed exit). 디렉터리로 고른다 | D |
| RSP-c1-833alt | J | P3 대안(833fdbb). conflicts_caller 항목 둘 | D |
| RSP-c5-1 | CJ | K-SELF.es-task: use `CFE_ES_GetAppID`, key es.task(arg.task_id), requires LIVE. oracle 없음. authored 10-29, c… | D requires 자리는 callee의 `RecordUsed` 검사(F1/F2)에서 G |
| RSP-c5-2 | CJ | K-SELF.es-task-internal: `CFE_ES_GetAppIDInternal`, 같은 key·requires | D |
| APQ-cfs | J | appreq = `cfe_evs.h` L168-L172 문안(contract-backed, side caller). allowed none과 함께 not-ready를 VIOLATING으로 만든다 | D 판정 입력에서 뺀다. 문안은 평가 자료(V). 보고는 RV2 §5.5 독립 항목 |
| APQ-c1-281 | J | appreq = `cf_events.h` L148-L158 이벤트 30 문안. PI D8(10-29)로 지정(그 전에는 미지정) | D |
| APQ-c1-833 | J | appreq cf_events_sem_eid(D8). caller 항목이 여전히 결정한다 | D |
| APQ-c1-281alt | J | appreq unspecified_by_reference(P3 대안) | D |
| APQ-c1-833alt | J | appreq unspecified_by_reference(P3 대안) | D |
| APQ-c5-1 | J | appreq unspecified_by_reference(호출자 문안 없음, QT-9 PI 항목) | D |
| APQ-c5-2 | J | appreq unspecified_by_reference | D |
| ALW-cfs | JH | allowed none("거절을 받아들이는 문안 없음"이라는 K1의 읽기) | G 처리 분류: 결과 버림 39/39(extraction_design §3.5) |
| ALW-c1-281 | JH | allowed none(P3가 종료를 허용 밖에 둠) → not-ready 조회가 VIOLATING | G 처리 분류: 실패 분기 → 종료 경로(F2/F4) |
| ALW-c1-833 | JH | allowed retry_until_ready {OS_ERR_NAME_NOT_FOUND} bound CF_STARTUP_SEM_MAX_RETRIES, prov cf_cfdp_startup_adm… | G 처리 분류: 재시도·상한 25는 V1 루프 사실로 이미 나온다. 주석을 수용으로 읽는 일은 없앤다 |
| ALW-c1-281alt | JH | allowed exit APP_ERROR "cfs_cf.dox L212-L214", prov cf_dox_terminate | D |
| ALW-c1-833alt-retry | JH | retry_until_ready … conflicts_caller | D |
| ALW-c1-833alt-exit | JH | exit APP_ERROR … conflicts_caller | D |
| ALW-c5-1 | J | allowed unspecified_by_reference → NOT_READY_UNJUDGED | G 호출자마다 결과 사용 사실 |
| ALW-c5-2 | J | allowed unspecified_by_reference | G |
| ROL-c1-cf | CJH | role cf-dependent-lookup(op `OS_CountSemGetIdByName`, site cf_lookup_call, dependent_use; prov CF 문서·#184·이벤… | G RA §3.1 신호: CF 실패 분기가 ExitApp에 닿고, BP의 생성이 먼저면 분기가 뒤집힌다. 이슈는 입력에서 뺀다 |
| ROL-c1-bp (+OBS-C1-BP-ROLE) | CJ | role bp-existence-probe: BP의 조회를 발생에서 지운다(role_fact "not an instance"). E3의 경우 | G RA §3.1: 결과를 바꾸는 쓰기가 같은 행위자의 생성이고 프로그램 순서로 뒤에 고정 → 후보 아님. 처리 분류 probe-create |
| ROL-c5 | CH | role sb-create-pipe-dependent-lookup(op `CFE_ES_GetAppID`; prov에 #72 본문 포함) | G 결과 사용 사실: SB가 상태를 보지 않고 out 값을 저장 |
| SRC-c1 | RJ | bp 핀 + cFS 번들 후보 2(P4 열림) + CF 수정 쌍 281a941/833fdbb를 변형 디렉터리로 | U 핀. 변형 디렉터리 분할은 D(분석 대상 빌드 하나) |
| RES-cfs-05 | IJ | evs.reg 키 = 정확한 AppId. 세대(재시작)는 모델에 없음 | G 경로 A3(비교되는 저장 필드가 키) |
| RES-c1-02 | IJX | osal.countsem을 이름 키·ABSENT/LIVE로. create 안 RESERVED 창은 모델에 없음(A5). delete는 이름 미상. 재시도 감시의 세대 경계 | G 경로 A2(`OS_ObjectNameMatch`가 `name_entry`와 인자를 비교) |
| RES-c5-02 | IJX | es.task(t) = TaskTable[t], OSAL task id 키, RecordUsed로 FREE/LIVE, LP = 등록 구간의 TaskId 저장. 저장된 AppId·세대는 모델에 없음 | G 경로 A3. LP는 구간의 잠금 해제 관측으로 |
| RES-c5-04 | J | CFE_ES_Main의 `RecordUsed = FALSE` 루프를 "lifecycle all es.task → FREE"로 | G FACT_LOOPS constant-trip + FUNC_WRITES |
| ISS-c1 | CR | issue-derived side caller: nasa/CF #184 본문("CF는 그 세마포어가 먼저 만들어지길 기대"). 역할 cf-dependent-lookup과 C1 rspec 네 개가… | V 평가 자료만 |
| ISS-c5-1 | CR | issue-derived: #72 본문(unlock–relock 창). op `CFE_ES_GetAppID`, 역할, rspec 둘이 인용 | V |
| PRV-cfs-evs | JR | EVS 송신 검사·꼬리·squelch, 등록 memset·게시, IsMatch/SetUsed/SetFree, EarlyInit, CleanUpApp 등. R2의 사용·등록표를 위해 고름 | G writer·분기 사실의 위치 |
| PRV-cfs-sb | JX | `CFE_SB_Global` 정의·AppId 필드, EarlyInit memset, AppInit의 `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`, SB 이벤트 호출.… | G writer 3·DERIVES(경로 B) |
| PRV-c1-bp | CJH | BP 제공자 코드: probe-then-create(bp_flow.c L121-L144), semcfg 이름·표, AppInit 순서·종료, 이벤트 id, CF 표 파일(tx/rx 채널 이름),… | G 사실은 추출(결과 검사·STATIC_IMAGE·startup_script). "BP = 제공자"라는 선택은 "조회 키의 생성자"로 |
| PRV-c1-cf | CHJ | CF 조회 호출, 실패 이벤트와 break, init 상태 → APP_ERROR, exit, 저장 id 사용, 재시도 루프·상수(833fdbb). 판마다 같은 이름에 다른 줄 | G 호출·결과 검사·FACT_LOOPS·TERMINALS |
| PRV-c1-cfdox | CR | contract-backed side caller: CF 문서 Flow Control(`cfs_cf.dox` L208-L214). 역할과 rspec 목록 전부가 인용 | V 문서는 평가 자료. 후보 표면화만 G |
| PRV-c5-callers | CH | 조회 호출자와 결과 사용: SB는 상태를 보지 않고 저장, EVS 범위 검사, sample_app 검사 없음, RunLoop는 ES 잠금 안 | G 호출 지점의 결과 사용·검사 사실 |
| PRV-c5-rel | JXE | TaskTable 정의·구조, CFE_ES_Main 초기화 루프, CreateObjects, AppCreate 잠금·생성·등록·TaskId 저장, GetAppID(Internal), 자식 tas… | G WRITERS(RecordUsed·TaskId) + F5 잠금 집합 |
| CON-semnames | CI | CF_CHANNEL_{0,1}_SEM_NAME = "BP0_tsem"/"BP1_tsem"(빌드된 이름). 예측 자원 키와 hold 계획이 쓴다 | G BP 쪽은 F6 IMAGES. CF 쪽은 표 파일이라 "키 미해결, 구획 일치" → 기록 값으로 확정(extraction_design §3.6) |
| CON-retry | HJ | CF_STARTUP_SEM_MAX_RETRIES 25, CF_STARTUP_SEM_TASK_DELAY 100. "다른 판에 옮기지 않음" | G FACT_LOOPS 상한 + 상수 평가(V1에서 이미 25) |
| KND-cfs-evsreg | JCX | evs.reg(app): UNREG<REG. R2의 등록표 자원 | G 경로 A3 |
| KND-c1-countsem | JCXI | osal.countsem(name): ABSENT<LIVE, create/delete. C1의 자원 | G 경로 A2 |
| KND-c5-estask | JIX | es.task(t): FREE<LIVE, register/release. OSAL task id로 TaskTable 칸 | G 경로 A3 |
| LOC-sb (+KS-P01) | CJOX | loc `CFE_SB_Global.AppId`(Id<es.app> 보유): R2의 정체성 칸, E1이 말하는 생산자 필드. kz-static 추출 대상이 된다(KS-P01) | G 경로 B(V0 FACT_ARGS에 39곳이 이미 있다) + R-T1 |
| INI-sb | J | init sb_identity_image: 프로그램 시작 때 0(초기값 없는 전역) | G FACT_INIT(대상 밖은 F6) |
| SIT-c1 | CHO | site cf_lookup_call, bp_existence_probe_call, bp_create_call, cf_retry_delay(833fdbb만) | G API의 모든 호출 지점과 그 함수 |
| SIT-c5 (+KS-P03) | CJOX | es.get_app_id(.int).task(OS_TaskGetId 뒤), es.app_create.pub_taskid, sb.create_pipe.get_app_id(역할 지점) | G 호출·저장 사실 |
| OP-cfs-send (+KS-P04) | CJXR | `CFE_EVS_SendEventWithAppID`(stamp AppID): 연언항 id_valid·slot_registered, 결과 SUCCESS / ILLEGAL_APP_ID / NOT_R… | G F2 반환 경로 + RA §3.1 |
| OP-cfs-producer | JI | `CFE_ES_GetAppID`(self_live, out 저장 when/filter, 결과 3) + `CFE_SB_EarlyInit`(clear_global := 0): E1의 생산자 사슬 | G WRITERS(SB AppId writer 3) |
| OP-cfs-evswriters | JOI | CFE_EVS_Register(키 id(owner(exec)), A-OWNER에 기댐), EarlyInit, CleanUpApp | G WRITERS(EVS AppID 4) |
| OP-c5-lookup | JCI | `CFE_ES_GetAppID`/`Internal` + 의사 매개변수 task_id: 연언항 registered, 결과 3(narrow) | G callee 분기(RecordUsed == TRUE → SUCCESS), F1/F2 |
| OP-c5-publish | JE | CFE_ES_AppCreate publish(TaskId 저장 지점). 잠금 범위는 규칙으로 두지 않음(잠금 템플릿 없음) | G WRITERS + F5 잠금 집합 |
| OP-c5-otherwriters (+KS-P03) | JO | Main.clear_task_table, CreateObjects, CreateChildTask, Delete/Exit/CleanupTaskResources. 관측 지점이 없어 재생에서 종류가… | G WRITERS(이 grep 목록을 대신한다) |
| KS-C01 (+DRV-TARGET) | COX | 명령줄 `--target` 7(C1 드라이버만): OSAL 3(bind와 중복) + 앱 필드 4 `CF_Channel_t.sem_id`, `CF_ChannelConfig_t.sem_name`,… | G R-T1/R-T2(V3에서 앱 필드 writer 수가 같음) |
| E05 | CJR | 호출 문맥 역할(닫힌 어휘 dependent_use/existence_probe): existence_probe 지점의 호출은 발생에서 지운다. narrow 문맥이 역할 이름. BP의 조회-생성… | G RA §3.1(실패 분기 → 같은 행위자의 같은 키 생성, 프로그램 순서). 구성 자체는 삭제 |
| OBS-0 | COXJ | 결합 규칙: point site 라벨을 Φ site·LP 이름(OP.LABEL, 역할 지점)으로 지어 재생이 대응시킨다. kzs 위치는 `20_ops.kzp`의 `lp at`을 따른다 → 사람이… | G 규칙 관측점(OP-*, extraction_design §4). 사건 → 후보 대응도 추출 사실로 |
| OBS-cfs-points (+OBS-R2-BASE, OBS-R2-FIELD, OBS-R2-WRITERS) | CJOX | Φ LP의 point site 18: EVS 등록·송신, ES main reset 2, core reserve/publish/clear, sb_publication, 포착 지점 `id_valid… | G writer 위치·호출 사실. 포착 3은 DERIVES 행(core_cpu1.golden L503/525/532) |
| OBS-cfs-interval (+OBS-R2-INTERVAL) | JOR | R2 구간 경계 es.main.operational, es.restart_cmd.reset_cfe, sb.sub_reporting.set + sb.appid.store(중복). 재생은 쓰지 않음… | D 분석 구간 = 기록 자체의 범위(ES 상태 저장 추출은 G로 가능) |
| OBS-c1-sites (+OBS-C1-API, OBS-C1-LIFE, OBS-ID, OBS-C1-CF, OBS-C1-BP-ROLE) | CJOEIXH | CF 판마다의 F0 계획(core-tx, cf.so, bp.so): countsem entry/ret(fetch-arg 문자열), tail-jump·인라인 OSAL 진입 도달 point, cf.… | G 추출 호출 지점 + 바이너리 역어셈 |
| INS-c5 (+INS-C5, INS-C5-ID, INS-C5-CTL) | JOIHR | DRAFT manifest: AppCreate unlock/create/relock/게시, ES 잠금, OSAL 생성·등록, GetAppID(Int), SB/EVS 소비자, sample_app… | G site. 정체성 결합은 O(단계 1) |
| HOLD-PRED | CHE | kz predict가 후보 분류마다 합성한 계획 28. 이름 조건 "BP0_tsem"/"BP1_tsem", caller-in cf.so/bp.so, occurrence n은 모델의 hit 줄에서 | G 생성기는 두고 입력을 추출 사실로 |
| PRD-cone (+PRED-COMMON) | CJEIH | 자원 s0/s1(osal.countsem 키 CF_CHANNEL_0/1_SEM_NAME)·shutdown, 행위자 core-tx·CFE_ES·CF·BP(기록 이름으로 묶음) | G cone = 기록에서 추출 자원에 닿는 행위자, 자원 = 추출 키 |
| PRD-coretx (+PRED-COMMON) | EC | core-tx 프로그램: clear, CFE_ES·CF·BP spawn(코어 → 스크립트, CF 앞 BP), 종료 대기, teardown, delete-each | G table walk 순서 + 시작 스크립트 + 프로그램 순서 |
| PRD-bp (+PRED-COMMON) | CE | 손으로 쓴 BP initialize_throttling CFG(이름 2를 펼침: probe → found 또는 create, taken, 이벤트 14/15) + "BP_CLA_Init 성공(모든… | G 함수 CFG + 호출·결과 검사 + 상수 횟수 펼침. 기록 기반 가정은 라벨 |
| PRD-cf281 (+PRED-CF) | CJEH | CF 281a941 프로그램: 조회 2, 성공이 아니면 이벤트 30 → NAME_NOT_FOUND 반환 → APP_ERROR 종료 | G InitEngine·AppMain CFG |
| PRD-cf833 (+PRED-CF) | CE | CF 833fdbb 프로그램: 카운터 j 재시도(< CF_STARTUP_SEM_MAX_RETRIES), 지연, 그다음 실패 경로 | G CFG + FACT_LOOPS |
| PRED-OPT | CJ | 예측 입력·한도: `--model`(사례), `--observe-profile`(사례 계획), `--prov-root`, `--max-states 2000000`(연구자 선택), 시간 없는 in… | G 입력을 추출 사실로(못 하면 predict 제거, RP §3.6). `--max-states`는 S |
| DRV-PROFILE | CJIXH | 모든 재생·예측·kz-static 드라이버의 `--profile` 사례 디렉터리: C1 = 공통 + cf-<rev> + bundle-<cand> + rspec-<rev>[-altP3](pre →… | D 사례 프로필 층 제거. 남는 것은 P0/L0 판(RP §1.2 #1: rspec 층을 빼면 발생 0) |

### 3.2 섞임(M), 35행

| ID | 영향 | 내용(요약) | 대체 |
|---|---|---|---|
| SRC-c5 | RJ | rev 자리에 release tarball sha256의 앞 40자리(commit 아님). verify-prov는 unverifiable | U 별도 입력 종류(묶음 해시) |
| RES-cfs-04 | OJ | GetAppID 실패 분기의 out 저장(L759)을 뒤따르는 UnlockSharedData(L763)에 묶고 guard를 filter로 | G WRITERS의 out-pointer 실체화 + 저장 지점 관측 |
| RES-cfs-06 | JI | es.self는 소유 App 레코드가 LIVE면 LIVE. GetAppRecordByContext의 task 레코드 일치 검사는 뺌 | G RA §3.1: callee 자신의 성공 검사 |
| RES-cfs-09 | IJ | A-OWNER의 근거: ParentAppId로 시작한 task의 문맥 조회 레코드 = PendingAppId가 흐른 레코드 | O 단계 1의 독립 생성 측 토큰(RA §2, RV2 §3.7). SPAWN_PARAMS는 may-flow만 준다 |
| RES-cfs-10 | J | `CFE_ES_CleanUpApp.retire`의 선언 guard(EXTERNAL ∧ AppState > RUNNING). 레코드 사이 관계 조건 | G 호출 지점 writer guard. 관계 조건은 공개 가정으로 남음 |
| RES-c5-03 | IC | task_id를 C 매개변수가 아닌 의사 매개변수로 선언. rspec 키가 매개변수만 이름 붙일 수 있어서 | G callee 안에서 계산되는 키를 추출기 개념으로(F2/F4) |
| PRV-cfs-es | JIE | ES App 레코드 도우미·구조·enum, 시작 루프, GetAppID 분기, 문맥 조회, wait/runloop/exitapp/scan/cleanup writer, StartAppTask, A… | G WRITERS·FACT_LOOPS·SPAWN_PARAMS |
| PRV-c1-osalsem | JXE | OSAL countsem·객체 표 의미: 생성·확정, 이름 검색, 구획, 잠금 모드, 토큰, 삭제, 초기화, 인자·길이 검사, BUGCHECK, 이름 길이 상한 | T 판 표기 OSAL 지원. writer·분기는 kz-static 사실로 대조 |
| KND-cfs-esapp | IJEX | es.app: FREE<RESERVED<LIVE, 표 크기, ids, tag Type, 전이 5 | G 경로 A/C 수명 분류(AppId writer의 값 집합) |
| KND-cfs-appstate | JE | es.appstate: enum 순서, 단조 상승 | G enum + AppState writer |
| KND-cfs-esself | JI | 파생 종류 es.self(a): 소유 레코드가 LIVE면 LIVE | G RA §3.1(callee 검사) |
| BND-cfs (+KS-P02) | COEX | bind field 6(App AppId/AppState/Type, SB AppId, EVS AppID, PSP ShutdownReq) → 상태·tag·값. kz-static writer·A-T… | G 경로 A/B, R-T2(callee 성공 분기가 읽는 필드) |
| BND-c1 (+KS-P02) | XOCEJ | bind `OS_common_record_t.active_id/.name_entry`, `obj_name` → osal.countsem(name), via 함수 4(공유 객체 표의 키 분리) | G FACT_ARGS 형 상수 → 구획(E28 참조) |
| SIT-cfs | OEC | Φ site 5: sb_publication(GetAppID arg0 = `&CFE_SB_Global_t.AppId`), core_phase2_begin(R18 기준), core_sync_cal… | G OUTCALLS·writer 위치·호출/루프 사실 |
| OP-bindsym (+KS-P05) | COXR | op 집합과 bind symbol: 재생이 사건으로 아는 C 함수, 역할 인자(stamp/key/out/id)·값 인자(lp.slot, pending_id, sem_name, task_id …) | G op 집합 = 추출된 후보 API, 역할 인자 = callee가 공유 상태와 비교하는 인자(F4) |
| OP-cfs-eswriters | IJE | ES App 레코드 writer 19(CFE_ES_Main, CreateObjects, AppCreate, Wait/RunLoop/ExitApp/Scan/CleanUp) | G WRITERS(guard·값), LP = writer 위치 |
| OP-cfs-retire | JI | CleanUpApp.retire LIVE→RESERVED, declared_trusted, guard 선언 | G writer. 관계 guard는 공개 가정 |
| OP-c1-lookup (+KS-P04) | JCX | `OS_CountSemGetIdByName`(out sem_id, key sem_name): 연언항 exists, 결과 SUCCESS ready / NAME_NOT_FOUND not_ready(… | G F2 + RA §3.1. "없음 = 준비 안 됨"을 가정하지 않는다 |
| OP-c1-create | JE | `OS_CountSemCreate` publish(FinalizeNew에서 OS_SUCCESS일 때) + 결과 7 | G FinalizeNew의 active_id writer와 guard |
| OP-c1-life | JO | OS_ObjectIdInit·CountSemAPI_Init는 전체 ABSENT, CountSemDelete.retire는 이름 미기록 | G 수명 분류(memset·FinalizeDelete writer) |
| OP-c1-exit (+KS-P06) | HJ | `CFE_ES_ExitApp` terminal(외부 앱). ExitStatus = 종료 감시 코드, 코어 분기는 모델에 없음 | T cFE 조건부 종료(판 지원) + 인자 상수 |
| NAR | J | narrow 블록(P1): api, source rev 목록, 손으로 쓴 branch 문자열, 문맥, 포착 인자(이름 길이 < OS_MAX_API_NAME), holds_at. 실기록에서 반환… | G 반환 경로·전제 조건(F2), rev는 분석 트리에서 |
| TRU-owner | IJ | A-OWNER declared_trusted: id(owner(exec))를 생성 요청 id로 정하고 EVS 등록 키를 받친다 | O 단계 1 독립 생성 측 토큰 |
| KS-R02 (+RCP-c5, DRV-RECIPE) | COX | `module` 목록을 사람이 적는다: C1 7(드라이버에 고정), C5 5, R2 사례 0(닫힘에 앱 없음). 없으면 앱이 넘긴 주소를 받는 OSAL 본문이 없어 필드가 DISABLED | U 시작 스크립트 줄 + 그 모듈의 link 명령(extract_baseline.sh L74-L85 규칙) |
| E03 | JH | retry_until_ready 감시: 한 행위자·문맥·키·세대의 연속 시도. bound·codes는 프로필 상수. 주석이 "지금 사례 프로필이 보는 초기화 창"에서만 맞다고 적는다 | G 처리 사실(루프 사실 + 기록의 같은 키 반복, RV2 §5.6) |
| E04 | JH | 종료 감시와 terminal 단계: 코드는 op의 non-out 매개변수 하나(ExitApp 모양), process_terminal 뒤는 재생 안 함, teardown 뒤에는 결정 안 함 | G 정적 호출 그래프의 종료 도달(P0 씨앗) + 인자 상수 |
| E06 | JO | 결과 narrowing(P1) 게이트: api, 소스 rev SHA 목록, 역할 이름, 포착 인자, requires | G callee 반환 경로(F2) + 빌드의 소스 판 |
| E09 (+DRV-CORECALLER) | JIR | `--core-request-caller FUNC`로 코어 서비스 판정. 드라이버는 CFE_ES_CreateObjects를 R2에만 준다(C1 생략, C5 "그런 함수를 정하는 레퍼런스 없음")… | G 코어 객체 표를 도는 생성 요청 함수(table walk·spawn 사실) 또는 시작 스크립트 앱의 여집합 |
| E16 | JR | caller 수준 harm 어휘 15(event_request_rejected = R2의 EVS/SB 증상, wrong_identity_stamped도 R2에서) | Q 기록의 최종 영향. 어휘는 판 표기 표로만 |
| E21 (+KS-N04) | JE | observe 규칙의 `predicate from_code`를 엔진이 추출하지 않는다. `--predicate`가 없으면 간선 없음. 드라이버는 하나도 주지 않는다(시험만) | G 재생이 FACT_LOOPS 술어를 직접 읽음 |
| E28 | OJ | bind via 키 분리: 연구자가 이름 붙인 함수 밖의 writer를 "다른 키의 쓰기"로 덮음 → writer 적합성과 판정에 들어간다 | G 경로 A 키·색인 분석 |
| E29 | CHE | 예측 모델 언어의 내장 의미: `create … taken CONST`(OS_CountSemCreate·이름 점유 모양), `exit OP CONST`(ExitApp 모양), use·delete… | G cone을 정적 사실 + 기록 접두사에서(RP §3.6). 못 하면 D |
| OBS-cfs-entryret (+OBS-R2-BASE) | JOIX | 함수 진입 11·반환 8(evs.send, es.sync, osal.task_create, evs.register, es.wait …), SysV x86-64 레지스터 slot | G 후보 API의 진입·반환 + DWARF 형식 매개변수·ABI |
| C-09 | CJ | 예측 검사 규칙: use는 연언항 하나(`state(kind(arg.P)) = S`), 프로필 역할 지점이어야 함, 인스턴스 = 역할 purpose dependent_use, 결과마다 narro… | G 예측 입력을 추출 사실로. 못 하면 D |
| C-14 | JR | 도구가 읽는 문서: 인용으로만 읽고(verify-prov의 해시·식별자, predict `--prov-root`) 뜻은 해석하지 않는다. 문안을 caller 요구로 묶은 것은 사람(APQ-*,… | V 문서는 평가 자료. @retval 후보는 G |

읽기 [I]:
- Y 가운데 대체가 D인 것은 rspec·appreq·대안 allowed, 드라이버의 사례 프로필 선택, R2 구간 경계 site다. 판정 근거 자리를 비우는 것이고, 그 자리는 RV2 §5.5의 독립 항목이 채운다. 의존성 근거, 순서, 호출 결과, 후속 처리, 확인 상태, 최종 영향, 앱 관계다.
- Y 가운데 G인 것은 세 묶음이다.
  - 같은 사실을 사례 없이 추출하는 것: 자원 종류, writer, site, 예측 모델의 CFG.
  - RA §3.1 반사실 신호로 바꾸는 것: 역할, requires.
  - 처리 분류로 바꾸는 것: allowed.
- "G로 대체 가능"은 F1–F7과 사례 무관 질의기(ED §6의 `kz-static relations`)가 만들어진 뒤의 말이다. 지금 사실만으로 사례 행 하나를 지울 수 있는 경우는 §7.4에 따로 적었다.

## 4. 모달리티별 표

### 4.1 Φ 판정층 (rspec, appreq, allowed, harm, role)

모든 행이 버그를 아는 사람이 사례를 보고 쓴 것이다. `authored` 필드에 "cases inspected: R2/C1/C5"가 남아 있다(RP §1.2 #5). harm은 파싱·검사·해시만 하고 재생 상태에는 쓰지 않는다.

| ID | 위치 | 내용 | 종류 | 영향 | 사례 | 수 | 대체 | 오늘 | 집계 |
|---|---|---|---|---|---|---|---|---|---|
| RSP-cfs | `cfs/40_rspec.kzp:18-28` | rspec K-ID.send-with-appid: use `CFE_EVS_SendEventWithAppID`, key evs.reg(arg.AppID), requires valid ∧ REG. oracle은 이름만(재생 미사용). authored K1, "cases inspected: R2" | prof | CJ | Y | 1 | D 이 API를 고른 선택은 없앤다. use·key·requires 자리는 callee 자신의 검사에서 G(F2, RA §3.1 신호) | N | ● |
| RSP-c1-281 | `c1/rspec-281a941/40_rspec.kzp:17-27` | K-REG.osal-countsem-lookup(CF 281a941): use `OS_CountSemGetIdByName`, key osal.countsem(arg.sem_name), requires LIVE. CF 판마다 디렉터리. 머리 주석 L4-L10은 D8 이전 읽기 | prof | CJ | Y | 1 | D (RSP-cfs와 같음) | N | ● |
| RSP-c1-833 | `c1/rspec-833fdbb/40_rspec.kzp:16-28` | 같은 rspec의 833fdbb판. allowed와 prov 목록만 다르다 | prof | CJH | Y | 1 | D | N | ● |
| RSP-c1-281alt | `c1/rspec-281a941-altP3/40_rspec.kzp:21-33` | 열린 PI 결정 P3의 대안 읽기(appreq 미지정, allowed exit). 디렉터리로 고른다 | prof | J | Y | 1 | D | N | ● |
| RSP-c1-833alt | `c1/rspec-833fdbb-altP3/40_rspec.kzp:21-34` | P3 대안(833fdbb). conflicts_caller 항목 둘 | prof | J | Y | 1 | D | N | ● |
| RSP-c5-1 | `c5/40_rspec.kzp:22-31` | K-SELF.es-task: use `CFE_ES_GetAppID`, key es.task(arg.task_id), requires LIVE. oracle 없음. authored 10-29, cases C5·R2 | prof | CJ | Y | 1 | D requires 자리는 callee의 `RecordUsed` 검사(F1/F2)에서 G | N | ● |
| RSP-c5-2 | `c5/40_rspec.kzp:33-42` | K-SELF.es-task-internal: `CFE_ES_GetAppIDInternal`, 같은 key·requires | prof | CJ | Y | 1 | D | N | ● |
| APQ-cfs | `cfs/40_rspec.kzp:23`, `cfs/01_prov.kzp:13` | appreq = `cfe_evs.h` L168-L172 문안(contract-backed, side caller). allowed none과 함께 not-ready를 VIOLATING으로 만든다 | prof | J | Y | 1 | D 판정 입력에서 뺀다. 문안은 평가 자료(V). 보고는 RV2 §5.5 독립 항목 | N | ● |
| APQ-c1-281 | `c1/rspec-281a941/40_rspec.kzp:22`, `c1/01_prov.kzp:185` | appreq = `cf_events.h` L148-L158 이벤트 30 문안. PI D8(10-29)로 지정(그 전에는 미지정) | prof | J | Y | 1 | D | N | ● |
| APQ-c1-833 | `c1/rspec-833fdbb/40_rspec.kzp:21` | appreq cf_events_sem_eid(D8). caller 항목이 여전히 결정한다 | prof | J | Y | 1 | D | N | ● |
| APQ-c1-281alt | `c1/rspec-281a941-altP3/40_rspec.kzp:26` | appreq unspecified_by_reference(P3 대안) | prof | J | Y | 1 | D | N | ● |
| APQ-c1-833alt | `c1/rspec-833fdbb-altP3/40_rspec.kzp:26` | appreq unspecified_by_reference(P3 대안) | prof | J | Y | 1 | D | N | ● |
| APQ-c5-1 | `c5/40_rspec.kzp:26` | appreq unspecified_by_reference(호출자 문안 없음, QT-9 PI 항목) | prof | J | Y | 1 | D | N | ● |
| APQ-c5-2 | `c5/40_rspec.kzp:37` | appreq unspecified_by_reference | prof | J | Y | 1 | D | N | ● |
| ALW-cfs | `cfs/40_rspec.kzp:24` | allowed none("거절을 받아들이는 문안 없음"이라는 K1의 읽기) | prof | JH | Y | 1 | G 처리 분류: 결과 버림 39/39(extraction_design §3.5) | N | ● |
| ALW-c1-281 | `c1/rspec-281a941/40_rspec.kzp:23` | allowed none(P3가 종료를 허용 밖에 둠) → not-ready 조회가 VIOLATING | prof | JH | Y | 1 | G 처리 분류: 실패 분기 → 종료 경로(F2/F4) | N | ● |
| ALW-c1-833 | `c1/rspec-833fdbb/40_rspec.kzp:23` | allowed retry_until_ready {OS_ERR_NAME_NOT_FOUND} bound CF_STARTUP_SEM_MAX_RETRIES, prov cf_cfdp_startup_admission(소스 주석 L993-L999를 수용으로 읽음). 재시도 감시를 움직인다 | prof | JH | Y | 1 | G 처리 분류: 재시도·상한 25는 V1 루프 사실로 이미 나온다. 주석을 수용으로 읽는 일은 없앤다 | N | ● |
| ALW-c1-281alt | `c1/rspec-281a941-altP3/40_rspec.kzp:28` | allowed exit APP_ERROR "cfs_cf.dox L212-L214", prov cf_dox_terminate | prof | JH | Y | 1 | D | N | ● |
| ALW-c1-833alt-retry | `c1/rspec-833fdbb-altP3/40_rspec.kzp:28` | retry_until_ready … conflicts_caller | prof | JH | Y | 1 | D | N | ● |
| ALW-c1-833alt-exit | `c1/rspec-833fdbb-altP3/40_rspec.kzp:29` | exit APP_ERROR … conflicts_caller | prof | JH | Y | 1 | D | N | ● |
| ALW-c5-1 | `c5/40_rspec.kzp:27` | allowed unspecified_by_reference → NOT_READY_UNJUDGED | prof | J | Y | 1 | G 호출자마다 결과 사용 사실 | N | ● |
| ALW-c5-2 | `c5/40_rspec.kzp:38` | allowed unspecified_by_reference | prof | J | Y | 1 | G | N | ● |
| HRM-cfs | `cfs/40_rspec.kzp:25` | harm event_request_rejected. 파싱·검사·해시만 하고 재생 상태에는 안 쓴다 | prof | R | Y | 1 | Q 기록의 최종 영향(RV2 §5.5) | N | ● |
| HRM-c1-281 | `c1/rspec-281a941/40_rspec.kzp:24` | harm app_error_exit | prof | R | Y | 1 | Q 관측된 종료 사건 | N | ● |
| HRM-c1-833 | `c1/rspec-833fdbb/40_rspec.kzp:25` | harm app_error_exit | prof | R | Y | 1 | Q | N | ● |
| HRM-c1-281alt | `c1/rspec-281a941-altP3/40_rspec.kzp:30` | harm app_error_exit | prof | R | Y | 1 | Q | N | ● |
| HRM-c1-833alt | `c1/rspec-833fdbb-altP3/40_rspec.kzp:31` | harm app_error_exit | prof | R | Y | 1 | Q | N | ● |
| HRM-c5-1 | `c5/40_rspec.kzp:28` | harm unspecified_by_reference | prof | R | Y | 1 | Q EVS 등록 건너뜀, AppId 0xFFFFFFFF 파이프, RunLoop FALSE를 따로 보고 | N | ● |
| HRM-c5-2 | `c5/40_rspec.kzp:39` | harm unspecified_by_reference | prof | R | Y | 1 | Q | N | ● |
| ROL-c1-cf | `c1/10_kinds.kzp:86-91` | role cf-dependent-lookup(op `OS_CountSemGetIdByName`, site cf_lookup_call, dependent_use; prov CF 문서·#184·이벤트 문안): CF의 호출을 발생으로 남기고 재시도 감시에 호출 문맥을 붙인다 | prof | CJH | Y | 1 | G RA §3.1 신호: CF 실패 분기가 ExitApp에 닿고, BP의 생성이 먼저면 분기가 뒤집힌다. 이슈는 입력에서 뺀다 | N | ● |
| ROL-c1-bp | `c1/10_kinds.kzp:97-102` | role bp-existence-probe: BP의 조회를 발생에서 지운다(role_fact "not an instance"). E3의 경우 | prof | CJ | Y | 1 | G RA §3.1: 결과를 바꾸는 쓰기가 같은 행위자의 생성이고 프로그램 순서로 뒤에 고정 → 후보 아님. 처리 분류 probe-create | N | ● (+OBS-C1-BP-ROLE) |
| ROL-c5 | `c5/10_kinds.kzp:31-36` | role sb-create-pipe-dependent-lookup(op `CFE_ES_GetAppID`; prov에 #72 본문 포함) | prof | CH | Y | 1 | G 결과 사용 사실: SB가 상태를 보지 않고 out 값을 저장 | N | ● |

### 4.2 머리·출처 핀·연구자 노트·이슈·인용

인용(PRV-*)은 kz-phi verify-prov가 줄 해시와 식별자를 다시 확인한다. 그래서 "code-derived" 등급을 받는다. 그러나 무엇을 인용할지, 그 줄이 무엇을 받치는지는 사람이 골랐다. 오늘 열은 P로 둔다.

| ID | 위치 | 내용 | 종류 | 영향 | 사례 | 수 | 대체 | 오늘 | 집계 |
|---|---|---|---|---|---|---|---|---|---|
| HDR-1 | `*/00_profile.kzp`(Φ 3), `est-64{1,2}/00_profile.kzp:11` | 프로필 이름과 자유 문안 판. 이름이 사례를 담는다(`c1-bp-…`, `c5-…`). E-ST 머리 프로필 2는 C5 basis에 필요 | prof | R | M | 5 | U 분석 트리의 commit·묶음 해시 | N | ● |
| SRC-cfs | `cfs/00_profile.kzp:26-29` | source 핀 cfs·cfe·osal·psp 4(전체 commit). narrow가 "source cfe rev" 일치를 요구한다 | prof | JIR | N | 4 | U 트리의 git 정보(verify-prov·basis가 이미 대조) | P | ● (+KS-P11) |
| SRC-c1 | `c1/00_profile.kzp:28`, `bundle-*/00_source.kzp`, `cf-*/00_source.kzp` | bp 핀 + cFS 번들 후보 2(P4 열림) + CF 수정 쌍 281a941/833fdbb를 변형 디렉터리로 | prof | RJ | Y | 9 | U 핀. 변형 디렉터리 분할은 D(분석 대상 빌드 하나) | N | ● |
| SRC-c5 | `c5/00_profile.kzp:46`, `cfe-6.4.{1,2}/00_source.kzp:10` | rev 자리에 release tarball sha256의 앞 40자리(commit 아님). verify-prov는 unverifiable | prof | RJ | M | 3 | U 별도 입력 종류(묶음 해시) | N | ● |
| RES-cfs-01 | `cfs/02_researcher.kzp:10` | 행위자·처리기 안 프로그램 순서 공리(A3, A4), looper(SPEC 1.9). hb R1의 근거 | prof | E | N | 1 | K 엔진 내부 공리로 공개 | N | ● |
| RES-cfs-02 | `cfs/02_researcher.kzp:17` | R19의 판 guard cfe ≥ 6.4.2를 SPEC에서 가져옴. 6.4.x 소스는 다시 읽지 않음 | prof | E | Y | 1 | G 대기 루프의 존재(FACT_LOOPS poll)로 대체, 판 guard 불필요 | N | ● |
| RES-cfs-03 | `cfs/02_researcher.kzp:24` | R18 기준점을 둘째 루프의 첫 `CFE_ES_LockSharedData`로 둠. 패턴 언어가 `Entry->EarlyInit` 간접 호출을 부를 수 없어서 | prof | EO | Y | 1 | G CALL_TARGETS + table walk 펼치기. EarlyInit memset 관측 + R1 + spawn | N | ● |
| RES-cfs-04 | `cfs/02_researcher.kzp:31` | GetAppID 실패 분기의 out 저장(L759)을 뒤따르는 UnlockSharedData(L763)에 묶고 guard를 filter로 | prof | OJ | M | 1 | G WRITERS의 out-pointer 실체화 + 저장 지점 관측 | N | ● |
| RES-cfs-05 | `cfs/02_researcher.kzp:38` | evs.reg 키 = 정확한 AppId. 세대(재시작)는 모델에 없음 | prof | IJ | Y | 1 | G 경로 A3(비교되는 저장 필드가 키) | N | ● |
| RES-cfs-06 | `cfs/02_researcher.kzp:45` | es.self는 소유 App 레코드가 LIVE면 LIVE. GetAppRecordByContext의 task 레코드 일치 검사는 뺌 | prof | JI | M | 1 | G RA §3.1: callee 자신의 성공 검사 | N | ● |
| RES-cfs-07 | `cfs/02_researcher.kzp:52` | cell(loc) 일반 종류: 값이 유효 Id면 VALID | prof | J | N | 1 | K 도구 내부 개념 | N | ● |
| RES-cfs-08 | `cfs/02_researcher.kzp:59` | A-FRESH의 근거: 서로 다른 예약 노드는 새 레코드를 받는다 | prof | I | M | 1 | K 공개 가정, 기록(O9)으로 반박 | N | ● |
| RES-cfs-09 | `cfs/02_researcher.kzp:66` | A-OWNER의 근거: ParentAppId로 시작한 task의 문맥 조회 레코드 = PendingAppId가 흐른 레코드 | prof | IJ | M | 1 | O 단계 1의 독립 생성 측 토큰(RA §2, RV2 §3.7). SPAWN_PARAMS는 may-flow만 준다 | N | ● |
| RES-cfs-10 | `cfs/02_researcher.kzp:73` | `CFE_ES_CleanUpApp.retire`의 선언 guard(EXTERNAL ∧ AppState > RUNNING). 레코드 사이 관계 조건 | prof | J | M | 1 | G 호출 지점 writer guard. 관계 조건은 공개 가정으로 남음 | N | ● |
| RES-cfs-11 | `cfs/02_researcher.kzp:80` | abort(3)가 프로세스를 끝낸다(process_terminal). 도구가 확인하지 않음 | prof | HR | N | 1 | T P0 종료 기본 연산(noreturn, TERMINALS) | N | ● |
| RES-c1-01 | `c1/02_researcher.kzp:10` | 프로그램 순서 공리 사본 | prof | E | N | 1 | K | N | ● |
| RES-c1-02 | `c1/02_researcher.kzp:17` | osal.countsem을 이름 키·ABSENT/LIVE로. create 안 RESERVED 창은 모델에 없음(A5). delete는 이름 미상. 재시도 감시의 세대 경계 | prof | IJX | Y | 1 | G 경로 A2(`OS_ObjectNameMatch`가 `name_entry`와 인자를 비교) | N | ● |
| RES-c1-03 | `c1/02_researcher.kzp:24` | OSAL 형별 EXCLUSIVE 토큰 호출 약 30곳을 손 grep해, countsem 레코드를 바꾸는 함수가 넷뿐임을 보임(`via`의 근거) | prof | XO | M | 1 | G FACT_ARGS 형 상수 인자 → 이름 공간 분리(extraction_design §3.1) | N | ● |
| RES-c1-04 | `c1/02_researcher.kzp:31` | 성공한 조회의 out id가 기록된 생성의 out id와 같으면 그 생성을 읽은 것(hb RF의 근거) | prof | E | M | 1 | T OSAL 의미, 원문 대조(FinalizeNew·FindByName의 writer) | N | ● |
| RES-c1-05 | `c1/02_researcher.kzp:38` | 역할 지점 사건은 같은 행위자의 호출 반환 뒤에 온다. 바이너리 결합은 관측 계획에 | prof | OC | N | 1 | D 역할 제거와 함께 | N | ● |
| RES-c5-01 | `c5/02_researcher.kzp:8` | 프로그램 순서 공리 셋째 사본(looper no) | prof | E | N | 1 | K | N | ● |
| RES-c5-02 | `c5/02_researcher.kzp:15` | es.task(t) = TaskTable[t], OSAL task id 키, RecordUsed로 FREE/LIVE, LP = 등록 구간의 TaskId 저장. 저장된 AppId·세대는 모델에 없음 | prof | IJX | Y | 1 | G 경로 A3. LP는 구간의 잠금 해제 관측으로 | N | ● |
| RES-c5-03 | `c5/02_researcher.kzp:22` | task_id를 C 매개변수가 아닌 의사 매개변수로 선언. rspec 키가 매개변수만 이름 붙일 수 있어서 | prof | IC | M | 1 | G callee 안에서 계산되는 키를 추출기 개념으로(F2/F4) | N | ● |
| RES-c5-04 | `c5/02_researcher.kzp:29` | CFE_ES_Main의 `RecordUsed = FALSE` 루프를 "lifecycle all es.task → FREE"로 | prof | J | Y | 1 | G FACT_LOOPS constant-trip + FUNC_WRITES | N | ● |
| RES-c5-05 | `c5/02_researcher.kzp:36` | 역할 지점 관례 사본 | prof | OC | N | 1 | D | N | ● |
| ISS-c1 | `c1/02_researcher.kzp:45` | issue-derived side caller: nasa/CF #184 본문("CF는 그 세마포어가 먼저 만들어지길 기대"). 역할 cf-dependent-lookup과 C1 rspec 네 개가 인용 | prof | CR | Y | 1 | V 평가 자료만 | N | ● |
| ISS-c5-1 | `c5/02_researcher.kzp:43` | issue-derived: #72 본문(unlock–relock 창). op `CFE_ES_GetAppID`, 역할, rspec 둘이 인용 | prof | CR | Y | 1 | V | N | ● |
| ISS-c5-2 | `c5/02_researcher.kzp:50` | issue-derived: cFE 6.4.2 VDD 표 1.2-2 항목 2(Trac #41). PDF는 sha256으로만 | prof | R | Y | 1 | V | N | ● |
| PRV-cfs-err | `cfs/01_prov.kzp`(err_h_*) | `cfe_error.h` 결과 코드 매크로 줄 인용 | prof | RJ | N | 9 | G 상수 평가의 위치(FACT_CONSTS 파일:줄·해시) | P | ● |
| PRV-cfs-idspace | `cfs/01_prov.kzp`(resid_*, osal_type_user …) | 자원 ID 공간(UNDEFINED/RESERVED/MAX/BASE, OSAL 형 shift, cmake 모드, ToIndex) | prof | IJ | N | 11 | G 상수 평가 | P | ● |
| PRV-cfs-evs | `cfs/01_prov.kzp`(evs_* 20) | EVS 송신 검사·꼬리·squelch, 등록 memset·게시, IsMatch/SetUsed/SetFree, EarlyInit, CleanUpApp 등. R2의 사용·등록표를 위해 고름 | prof | JR | Y | 20 | G writer·분기 사실의 위치 | P | ● |
| PRV-cfs-sb | `cfs/01_prov.kzp`(sb_* 7) | `CFE_SB_Global` 정의·AppId 필드, EarlyInit memset, AppInit의 `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`, SB 이벤트 호출. E1의 생산자·소비자 | prof | JX | Y | 7 | G writer 3·DERIVES(경로 B) | P | ● |
| PRV-cfs-es | `cfs/01_prov.kzp`(es_* 46) | ES App 레코드 도우미·구조·enum, 시작 루프, GetAppID 분기, 문맥 조회, wait/runloop/exitapp/scan/cleanup writer, StartAppTask, AppCreate writer | prof | JIE | M | 46 | G WRITERS·FACT_LOOPS·SPAWN_PARAMS | P | ● |
| PRV-cfs-osal | `cfs/01_prov.kzp`(osal_* 8) | OSAL task 생성·진입, 주 스레드와 그 반환 | prof | EIH | N | 8 | T OSAL 기본 의미(원문 대조) | P | ● |
| PRV-cfs-psp | `cfs/01_prov.kzp`(psp_* 7) | PSP 재시작 요청 저장, abort, panic, teardown 루프, ShutdownReq 필드 | prof | RH | N | 7 | G TERMINALS + ShutdownReq writer | P | ● |
| PRV-cfs-sup | `cfs/01_prov.kzp:22, :31, :620` | contract-backed supplier 기록(문서화된 결과의 근거) | prof | R | M | 3 | G @retval 후보(미구현) | P | ● |
| PRV-c1-osalerr | `c1/01_prov.kzp`(osal_err_* 9) | `osapi-error.h` 상태 매크로 줄 | prof | RJ | N | 9 | G 상수 평가 | P | ● |
| PRV-c1-osalsem | `c1/01_prov.kzp`(osal_countsem_*, osal_find_* … 35) | OSAL countsem·객체 표 의미: 생성·확정, 이름 검색, 구획, 잠금 모드, 토큰, 삭제, 초기화, 인자·길이 검사, BUGCHECK, 이름 길이 상한 | prof | JXE | M | 35 | T 판 표기 OSAL 지원. writer·분기는 kz-static 사실로 대조 | P | ● |
| PRV-c1-osaltask | `c1/01_prov.kzp`(osal_task_* 6) | OSAL task 생성·진입·등록, 주 스레드 | prof | EI | N | 6 | T | P | ● |
| PRV-c1-idspace | `c1/01_prov.kzp`(resid_* 10) | owner 종류의 ES AppId 공간 | prof | I | N | 10 | G 상수 평가 | P | ● |
| PRV-c1-cfe | `c1/01_prov.kzp`(es_* 5) | ExitApp 서명과 외부 앱 분기(끝없는 지연), App 표 크기, cleanup 콜백, RunStatus APP_ERROR | prof | HI | M | 5 | G TERMINALS·루프 사실·enum 평가 | P | ● |
| PRV-c1-psp | `c1/01_prov.kzp`(psp_* 4) | C1 번들의 PSP 종료 요청과 teardown | prof | RJ | N | 4 | G ShutdownReq writer + teardown 호출 | P | ● |
| PRV-c1-bp | `c1/01_prov.kzp:13-173`(bp_* 18) | BP 제공자 코드: probe-then-create(bp_flow.c L121-L144), semcfg 이름·표, AppInit 순서·종료, 이벤트 id, CF 표 파일(tx/rx 채널 이름), 시작 스크립트 CF(65) 앞 BP(55) | prof | CJH | Y | 18 | G 사실은 추출(결과 검사·STATIC_IMAGE·startup_script). "BP = 제공자"라는 선택은 "조회 키의 생성자"로 | P | ● |
| PRV-c1-cf | `c1/01_prov.kzp`, `cf-281a941/01_prov.kzp`(3), `cf-833fdbb/01_prov.kzp`(7) | CF 조회 호출, 실패 이벤트와 break, init 상태 → APP_ERROR, exit, 저장 id 사용, 재시도 루프·상수(833fdbb). 판마다 같은 이름에 다른 줄 | prof | CHJ | Y | 14 | G 호출·결과 검사·FACT_LOOPS·TERMINALS | P | ● |
| PRV-c1-sup | `c1/01_prov.kzp:231, :241` | contract-backed supplier(`osapi-countsem.h` retval) | prof | R | N | 2 | G @retval | P | ● |
| PRV-c1-cfdox | `c1/01_prov.kzp:175` | contract-backed side caller: CF 문서 Flow Control(`cfs_cf.dox` L208-L214). 역할과 rspec 목록 전부가 인용 | prof | CR | Y | 1 | V 문서는 평가 자료. 후보 표면화만 G | N | ● |
| PRV-c5-osal | `c5/01_prov.kzp`(osal_* 5) | OSAL 4.1.1 task 생성: `*task_id`를 쓰기 전에 스레드가 돌 수 있음, OS_TaskRegister는 잠금 없이 pthread_self 비교, OS_TaskGetId는 TSD | prof | EI | N | 5 | T | P | ● |
| PRV-c5-err | `c5/01_prov.kzp`(err_h_* 3) | `cfe_error.h` 코드(두 판 같음) | prof | RJ | N | 3 | G 상수 평가 | P | ● |
| PRV-c5-callers | `c5/01_prov.kzp`(5) | 조회 호출자와 결과 사용: SB는 상태를 보지 않고 저장, EVS 범위 검사, sample_app 검사 없음, RunLoop는 ES 잠금 안 | prof | CH | Y | 5 | G 호출 지점의 결과 사용·검사 사실 | P | ● |
| PRV-c5-script | `c5/01_prov.kzp:128` | 빌드 산출물 `cfe_es_startup.scr` L1-L5 인용(모듈 목록·우선순위) | prof | R | Y | 1 | U 시작 스크립트(사용자 입력) | P | ● |
| PRV-c5-rel | `cfe-6.4.{1,2}/01_prov.kzp`(27 × 2) | TaskTable 정의·구조, CFE_ES_Main 초기화 루프, CreateObjects, AppCreate 잠금·생성·등록·TaskId 저장, GetAppID(Internal), 자식 task. writer 목록은 RecordUsed 저장 grep에서 | prof | JXE | Y | 54 | G WRITERS(RecordUsed·TaskId) + F5 잠금 집합 | P | ● |
| PRV-c5-sup | `c5/01_prov.kzp:82` | `cfe_es.h` L600-L623 GetAppID 반환 코드 | prof | R | N | 1 | G @retval | P | ● |

### 4.3 Φ 모델층 (상수, 종류, 문맥·범위, loc·init·bind, site, op, narrow, hb, trusted)

RP는 이 층을 "판마다 쓰는 플랫폼 모델"로 남기려 했다. RV2 §4.2는 그것이 이름만 바꾼 손 목록이면 인정하지 않는다. 이 층의 행 가운데 판 일반 의미(OSAL 생성·조회, PSP 종료, ES 시작 직렬화)는 T로, 사례 자원·연산은 G로 분류했다.

| ID | 위치 | 내용 | 종류 | 영향 | 사례 | 수 | 대체 | 오늘 | 집계 |
|---|---|---|---|---|---|---|---|---|---|
| CON-codes | `{cfs,c1,c5}/05_consts.kzp` | 결과·상태 코드를 손으로 옮긴 정수(mod 2^32 비교) | prof | JH | N | 21 | G TU 안 상수 평가(손 리터럴 불필요) | N | ● |
| CON-idspace | `cfs/05_consts.kzp:20-22`, `c1/05_consts.kzp:31-33` | CFE_RESOURCEID_UNDEFINED/RESERVED/MAX(es.app의 ids 블록) | prof | IJ | N | 6 | G 상수 평가 | N | ● |
| CON-appidbase | `cfs/05_consts.kzp:29`, `c1/05_consts.kzp:34` | CFE_ES_APPID_BASE = 0x110000을 SIMPLE 모드로 손 계산(STRICT면 0x02110000). R2 id_valid 연언항을 정한다 | prof | IJ | N | 2 | G ref eval(빌드 플래그로) | N | ● |
| CON-refeval | `cfs/05_consts.kzp:32-33`, `c1/05_consts.kzp:20, :35` | ref eval 상수: CFE_PLATFORM_ES_MAX_APPLICATIONS ×2, CORE_MAX_STARTUP_MSEC, OS_MAX_API_NAME(narrow 길이) | prof | JI | N | 4 | G basis가 이미 평가한다. 재생이 `--const`로 다시 받는 결합을 끊는다(E22) | P | ● (+KS-P11) |
| CON-semnames | `c1/05_consts.kzp:24-25` | CF_CHANNEL_{0,1}_SEM_NAME = "BP0_tsem"/"BP1_tsem"(빌드된 이름). 예측 자원 키와 hold 계획이 쓴다 | prof | CI | Y | 2 | G BP 쪽은 F6 IMAGES. CF 쪽은 표 파일이라 "키 미해결, 구획 일치" → 기록 값으로 확정(extraction_design §3.6) | N | ● |
| CON-appexit | `c1/05_consts.kzp:27` | CFE_ES_RunStatus_APP_ERROR = 3 | prof | H | M | 1 | G enum 평가 | N | ● |
| CON-retry | `c1/cf-833fdbb/05_consts.kzp:6-7` | CF_STARTUP_SEM_MAX_RETRIES 25, CF_STARTUP_SEM_TASK_DELAY 100. "다른 판에 옮기지 않음" | prof | HJ | Y | 2 | G FACT_LOOPS 상한 + 상수 평가(V1에서 이미 25) | N | ● |
| KND-cfs-esapp | `cfs/10_kinds.kzp:37-55` | es.app: FREE<RESERVED<LIVE, 표 크기, ids, tag Type, 전이 5 | prof | IJEX | M | 1 | G 경로 A/C 수명 분류(AppId writer의 값 집합) | N | ● |
| KND-cfs-appstate | `cfs/10_kinds.kzp:59-65` | es.appstate: enum 순서, 단조 상승 | prof | JE | M | 1 | G enum + AppState writer | N | ● |
| KND-cfs-esself | `cfs/10_kinds.kzp:69-75` | 파생 종류 es.self(a): 소유 레코드가 LIVE면 LIVE | prof | JI | M | 1 | G RA §3.1(callee 검사) | N | ● |
| KND-cfs-evsreg | `cfs/10_kinds.kzp:80-87` | evs.reg(app): UNREG<REG. R2의 등록표 자원 | prof | JCX | Y | 1 | G 경로 A3 | N | ● |
| KND-cfs-shutdown | `cfs/10_kinds.kzp:90-97` | es.shutdown: RUN<SHUTDOWN_REQ<TEARDOWN(PSP 종료). C1의 psp.shutdown과 같은 모델에 이름만 다르다 | prof | RJ | N | 1 | T PSP 종료 의미(판 표기·원문 대조) | N | ● |
| KND-cfs-cell | `cfs/10_kinds.kzp:101-107` | 일반 종류 cell(l) | prof | J | N | 1 | K | N | ● |
| KND-c1-esapp | `c1/10_kinds.kzp:17-29` | owner 종류 es.app(ids·표만, 전이 없음) | prof | I | N | 1 | G 상수 평가 + 표 크기 | N | ● |
| KND-c1-countsem | `c1/10_kinds.kzp:35-43` | osal.countsem(name): ABSENT<LIVE, create/delete. C1의 자원 | prof | JCXI | Y | 1 | G 경로 A2 | N | ● |
| KND-c1-pspshutdown | `c1/10_kinds.kzp:64-71` | psp.shutdown(= cfs es.shutdown) | prof | RJ | N | 1 | T | N | ● |
| KND-c5-estask | `c5/10_kinds.kzp:11-18` | es.task(t): FREE<LIVE, register/release. OSAL task id로 TaskTable 칸 | prof | JIX | Y | 1 | G 경로 A3 | N | ● |
| CTX | `cfs/10_kinds.kzp:10-20`, `c1/10_kinds.kzp:6-15` | context: owner_kind es.app, initial_thread not_app(P10). C5는 없음(6.4.x AppId에 미정·예약 값이 없음) | prof | I | N | 2 | T main의 호출 트리에 등록이 없다는 정적 검사 + 기록 관측 | N | ● |
| SCP | `cfs/10_kinds.kzp:24-29`, `c1/10_kinds.kzp:57-62` | scope: 종료 요청까지가 기본 범위, TEARDOWN 뒤 쓰기는 순서 검사에서 뺀다 | prof | RJ | N | 2 | T ShutdownReq writer + teardown 호출(판 지원) | N | ● |
| LOC-sb | `cfs/10_kinds.kzp:110-115` | loc `CFE_SB_Global.AppId`(Id<es.app> 보유): R2의 정체성 칸, E1이 말하는 생산자 필드. kz-static 추출 대상이 된다(KS-P01) | prof | CJOX | Y | 1 | G 경로 B(V0 FACT_ARGS에 39곳이 이미 있다) + R-T1 | N | ● (+KS-P01) |
| INI-sb | `cfs/10_kinds.kzp:127` | init sb_identity_image: 프로그램 시작 때 0(초기값 없는 전역) | prof | J | Y | 1 | G FACT_INIT(대상 밖은 F6) | N | ● |
| BND-cfs | `cfs/10_kinds.kzp:118-123` | bind field 6(App AppId/AppState/Type, SB AppId, EVS AppID, PSP ShutdownReq) → 상태·tag·값. kz-static writer·A-TYPE 대상(KS-P02). 재생은 묶인 필드의 writer가 모두 관측될 때만 종류를 고정 | prof | COEX | M | 6 | G 경로 A/B, R-T2(callee 성공 분기가 읽는 필드) | N | ● (+KS-P02) |
| BND-c1 | `c1/10_kinds.kzp:50-52` | bind `OS_common_record_t.active_id/.name_entry`, `obj_name` → osal.countsem(name), via 함수 4(공유 객체 표의 키 분리) | prof | XOCEJ | M | 3 | G FACT_ARGS 형 상수 → 구획(E28 참조) | N | ● (+KS-P02) |
| SIT-cfs | `cfs/10_kinds.kzp:130-134` | Φ site 5: sb_publication(GetAppID arg0 = `&CFE_SB_Global_t.AppId`), core_phase2_begin(R18 기준), core_sync_call, core_clear, app_create_clear | prof | OEC | M | 5 | G OUTCALLS·writer 위치·호출/루프 사실 | N | ● |
| SIT-c1 | `c1/10_kinds.kzp:74-76`, `cf-833fdbb/10_sites.kzp:4` | site cf_lookup_call, bp_existence_probe_call, bp_create_call, cf_retry_delay(833fdbb만) | prof | CHO | Y | 4 | G API의 모든 호출 지점과 그 함수 | N | ● |
| SIT-c5 | `c5/10_kinds.kzp:21-24` | es.get_app_id(.int).task(OS_TaskGetId 뒤), es.app_create.pub_taskid, sb.create_pipe.get_app_id(역할 지점) | prof | CJOX | Y | 4 | G 호출·저장 사실 | N | ● (+KS-P03) |
| OP-bindsym | `*/20_ops.kzp` 모든 op(cfs 22, C1 13, C5 10) | op 집합과 bind symbol: 재생이 사건으로 아는 C 함수, 역할 인자(stamp/key/out/id)·값 인자(lp.slot, pending_id, sem_name, task_id …) | prof | COXR | M | 45 | G op 집합 = 추출된 후보 API, 역할 인자 = callee가 공유 상태와 비교하는 인자(F4) | N | ● (+KS-P05) |
| OP-cfs-send | `cfs/20_ops.kzp:13-66` | `CFE_EVS_SendEventWithAppID`(stamp AppID): 연언항 id_valid·slot_registered, 결과 SUCCESS / ILLEGAL_APP_ID / NOT_REGISTERED / INVALID_PARAMETER / SQUELCHED | prof | CJXR | Y | 1 | G F2 반환 경로 + RA §3.1 | N | ● (+KS-P04) |
| OP-cfs-producer | `cfs/20_ops.kzp:112-128` | `CFE_ES_GetAppID`(self_live, out 저장 when/filter, 결과 3) + `CFE_SB_EarlyInit`(clear_global := 0): E1의 생산자 사슬 | prof | JI | Y | 2 | G WRITERS(SB AppId writer 3) | N | ● |
| OP-cfs-evswriters | `cfs/20_ops.kzp:75-104` | CFE_EVS_Register(키 id(owner(exec)), A-OWNER에 기댐), EarlyInit, CleanUpApp | prof | JOI | Y | 4 | G WRITERS(EVS AppID 4) | N | ● |
| OP-cfs-eswriters | `cfs/20_ops.kzp:135-219` | ES App 레코드 writer 19(CFE_ES_Main, CreateObjects, AppCreate, Wait/RunLoop/ExitApp/Scan/CleanUp) | prof | IJE | M | 19 | G WRITERS(guard·값), LP = writer 위치 | N | ● |
| OP-cfs-retire | `cfs/20_ops.kzp:216` | CleanUpApp.retire LIVE→RESERVED, declared_trusted, guard 선언 | prof | JI | M | 1 | G writer. 관계 guard는 공개 가정 | N | ● |
| OP-cfs-sync | `cfs/20_ops.kzp:170-175` | `CFE_ES_MainTaskSyncDelay` 결과 SUCCESS ready / OPERATION_TIMED_OUT error(R19의 poll) | prof | E | M | 1 | G FACT_LOOPS poll + TERMINALS | N | ● |
| OP-spawn | `cfs/20_ops.kzp:224-232`, `c1/20_ops.kzp:132-140`, `c5/20_ops.kzp:117-120` | bind 전용 op `OS_TaskCreate`(out task_id), `OS_TaskEntryPoint` | prof | EI | N | 5 | T P0 pthread_create + F7 래퍼 유도 | N | ● |
| OP-term | `cfs/20_ops.kzp:238-268`, `c1/20_ops.kzp:119-129` | CFE_PSP_Restart(종료 요청 + abort 뒤 process_terminal), OS_Application_Run(teardown), CFE_PSP_Panic, abort, main | prof | CJHR | N | 7 | T P0 종료 기본 연산 + 돌아오지 않는 함수 고정점 | N | ● (+KS-P03, KS-P06) |
| OP-c1-lookup | `c1/20_ops.kzp:10-49` | `OS_CountSemGetIdByName`(out sem_id, key sem_name): 연언항 exists, 결과 SUCCESS ready / NAME_NOT_FOUND not_ready(narrow) / 오류 2. "이름이 이미 있어야 함" 전제(E3가 고친 것) | prof | JCX | M | 1 | G F2 + RA §3.1. "없음 = 준비 안 됨"을 가정하지 않는다 | N | ● (+KS-P04) |
| OP-c1-create | `c1/20_ops.kzp:53-64` | `OS_CountSemCreate` publish(FinalizeNew에서 OS_SUCCESS일 때) + 결과 7 | prof | JE | M | 1 | G FinalizeNew의 active_id writer와 guard | N | ● |
| OP-c1-life | `c1/20_ops.kzp:98-117` | OS_ObjectIdInit·CountSemAPI_Init는 전체 ABSENT, CountSemDelete.retire는 이름 미기록 | prof | JO | M | 3 | G 수명 분류(memset·FinalizeDelete writer) | N | ● |
| OP-c1-frame | `c1/20_ops.kzp:68-81` | 사례 함수 이름의 사건 전용 op: CF_CFDP_InitEngine, CF_Init, BP_FlowInit(C-0/C-6, P-0/P-5 틀) | prof | OR | Y | 3 | G 추출된 호출 지점의 둘러싼 함수 | N | ● |
| OP-c1-exit | `c1/20_ops.kzp:87-91` | `CFE_ES_ExitApp` terminal(외부 앱). ExitStatus = 종료 감시 코드, 코어 분기는 모델에 없음 | prof | HJ | M | 1 | T cFE 조건부 종료(판 지원) + 인자 상수 | N | ● (+KS-P06) |
| OP-c5-lookup | `c5/20_ops.kzp:13-58` | `CFE_ES_GetAppID`/`Internal` + 의사 매개변수 task_id: 연언항 registered, 결과 3(narrow) | prof | JCI | Y | 2 | G callee 분기(RecordUsed == TRUE → SUCCESS), F1/F2 | N | ● |
| OP-c5-publish | `c5/20_ops.kzp:66-70` | CFE_ES_AppCreate publish(TaskId 저장 지점). 잠금 범위는 규칙으로 두지 않음(잠금 템플릿 없음) | prof | JE | Y | 1 | G WRITERS + F5 잠금 집합 | N | ● |
| OP-c5-otherwriters | `c5/20_ops.kzp:76-111` | Main.clear_task_table, CreateObjects, CreateChildTask, Delete/Exit/CleanupTaskResources. 관측 지점이 없어 재생에서 종류가 열린 채 | prof | JO | Y | 6 | G WRITERS(이 grep 목록을 대신한다) | N | ● (+KS-P03) |
| NAR | `cfs/20_ops.kzp:25-64`, `c1/20_ops.kzp:27-45`, `c5/20_ops.kzp:16-56` | narrow 블록(P1): api, source rev 목록, 손으로 쓴 branch 문자열, 문맥, 포착 인자(이름 길이 < OS_MAX_API_NAME), holds_at. 실기록에서 반환 코드가 연언항을 정하는 유일한 길 | prof | J | M | 10 | G 반환 경로·전제 조건(F2), rev는 분석 트리에서 | N | ● |
| DOC-outcomes | `*/20_ops.kzp`(cfs 4, C1 9, C5 2) | outcome의 documented 문안(헤더 retval을 손으로 줄임) | prof | R | M | 15 | G @retval 파싱 | N | ● |
| HB-cfs-R1 | `cfs/30_hb.kzp:15-23` | program_order looper yes. handler 표시 effect가 없어 looper는 효과 없음 | prof | E | N | 1 | K | N | ● |
| HB-cfs-R2PTASK | `cfs/30_hb.kzp:26-35` | spawn {create OS_TaskCreate out task_id; entry OS_TaskEntryPoint} | prof | CEI | N | 1 | T OSAL id 흐름(원문 대조) + spawn 사실 | N | ● (+KS-P07) |
| HB-cfs-R18 | `cfs/30_hb.kzp:38-47` | phase {from core_phase2_begin}: "EarlyInit이 모든 task보다 앞" | prof | E | Y | 1 | G R1 + spawn + table walk 순서 | N | ● |
| HB-cfs-R19 | `cfs/30_hb.kzp:51-65` | observe {poll CFE_ES_MainTaskSyncDelay called_at CreateObjects; predicate from_code; else_terminal CFE_PSP_Panic; watch es.app, es.appstate}, guard cfe ≥ 6.4.2, obs_fatal. R2를 고정된 잘못된 순서로 만드는 직렬화(E1) | prof | ER | M | 1 | G FACT_LOOPS 술어(cfe_es_start.c:878-890). guard는 루프 존재로 대체 | N | ● (+KS-P08) |
| HB-c1-R1 | `c1/30_hb.kzp:6-14` | program_order looper yes(효과 없음) | prof | E | N | 1 | K | N | ● |
| HB-c1-R2PTASK | `c1/30_hb.kzp:16-25` | spawn OS_TaskCreate/OS_TaskEntryPoint(이 OSAL에서 entry_arg는 5번째 인자) | prof | EI | N | 1 | T | N | ● (+KS-P07) |
| HB-c1-RF | `c1/30_hb.kzp:39-50` | reads_from {use exists; outcome OS_SUCCESS; publish Create.publish; match sem_id; stored_out sem_id}, stratum O obs_soft | prof | E | M | 1 | T OSAL 의미(생성 쪽 active_id·outid 저장과 조회 쪽 읽기, 원문 대조) | N | ● |
| HB-c5-R1 | `c5/30_hb.kzp:8-16` | program_order looper no | prof | E | N | 1 | K | N | ● |
| HB-c5-R2PTASK | `c5/30_hb.kzp:21-29` | phase {from OS_TaskCreate}: OSAL 4.1.1에는 id를 받는 진입 래퍼가 없다. ES 잠금 순서는 반환 코드로 읽는다(간선 아님) | prof | E | N | 1 | T + F5 잠금 사실 | N | ● |
| TRU-fresh | `cfs/50_trusted.kzp:9-14` | A-FRESH declared_trusted, refutable_by O9 | prof | I | M | 1 | K 공개 가정 | N | ● |
| TRU-owner | `cfs/50_trusted.kzp:16-21` | A-OWNER declared_trusted: id(owner(exec))를 생성 요청 id로 정하고 EVS 등록 키를 받친다 | prof | IJ | M | 1 | O 단계 1 독립 생성 측 토큰 | N | ● |

### 4.4 추출기(kz-static) 입력

| ID | 위치 | 내용 | 종류 | 영향 | 사례 | 수 | 대체 | 오늘 | 집계 |
|---|---|---|---|---|---|---|---|---|---|
| KS-P01 | `lib/llvm/spec.cpp:55`, `cfs/10_kinds.kzp:110` | Φ loc 선언이 추출 대상이 된다. 대상이어야 WRITERS·INIT·A-TYPE·LOCS·OUTCALLS·unknown-effect 검사가 생긴다. R2 생산자 writer(GetAppID out 저장, EarlyInit memset)는 이 선언이 있을 때만 나온다 [M: V0 writer 0 / V2 26] | prof | COX | Y | 1 | G 대상 무관 수집 또는 R-T1/R-T2 | N | →LOC-sb |
| KS-P02 | `lib/llvm/spec.cpp:56` | bind field마다 대상(R2 6, C1 3). `via`는 kz-static이 읽지 않는다. 재생의 aliases/writers_covered와 R2 await 인정(KS-H07)을 좌우 | prof | COEX | M | 9 | G | N | →BND-cfs, BND-c1 |
| KS-P03 | `lib/llvm/spec.cpp:45-48, 57, 66-67` | site·op의 store 패턴 T.f가 대상(C1 ShutdownReq, C5 TaskId·RecordUsed). call 패턴은 무시(`cf-833fdbb/10_sites.kzp`의 `call("OS_TaskDelay")`은 효과 없음) | prof | COX | M | 3 | G | N | →SIT-c5, OP-c5-otherwriters, OP-term |
| KS-P04 | `lib/llvm/spec.cpp:38-43, 62-63`, `ast_fn.cpp:1665-1681` | op 매개변수 역할 stamp/key/id에만 DERIVES: R2 59행(58 derived, 모두 hops 0), C1 2행(not-a-copy), C5 0. `kz-static reach`는 DERIVES만 읽는다 | prof | CR | Y | 2 | G 모든 인자에 derive(F4). hops 0은 V0 FACT_ARGS와 같은 정보 | N | →OP-cfs-send, OP-c1-lookup |
| KS-P05 | `lib/llvm/spec.cpp:64`, `spec.hpp:29` | out 역할은 OpInfo.out에 담기지만 lib/llvm에 소비처가 없다(죽은 입력, 프로필 해시에만 들어감). C1 3, R2 3, C5 5 | prof | R | N | 11 | D 매개변수 저장 요약을 직렬화하면 불필요 | N | →OP-bindsym |
| KS-P06 | `lib/llvm/spec.cpp:68-72`, `ast_fn.cpp:1715-1729` | op terminal → 종료 연산 집합: C1 {ExitApp} 14행, R2 {Restart, Panic, abort, main} 37행, C5 {} 0행. `after` 절 무시. RA §3.1 신호의 "실패 분기가 ExitApp에 닿음"이 이 표에 기댄다 | prof | CH | M | 5 | T P0 종료 씨앗 + 돌아오지 않는 함수 고정점. 조건부 종료는 따로 | N | →OP-term, OP-c1-exit |
| KS-P07 | `lib/llvm/spec.cpp:76`, `*/30_hb.kzp` | hb spawn 템플릿의 create X가 spawn 연산(C1·R2 OS_TaskCreate). C5는 phase 템플릿이라 spawn 연산이 없다 | prof | EIC | N | 2 | T P0 pthread_create + F7 | N | →HB-cfs-R2PTASK, HB-c1-R2PTASK |
| KS-P08 | `lib/llvm/spec.cpp:77`, `cmd_extract.cpp:52` | observe poll과 `--poll-fn`은 poll_fns에 들어가지만 소비처가 없다(죽은 입력). 그래도 `--poll-fn`은 hand-entered로 센다 | prof | R | Y | 1 | D | N | →HB-cfs-R19 |
| KS-P09 | `cmd_extract.cpp:120`, `tools/kz/cmd_replay.cpp:340-341` | facts 머리에 판정층을 포함한 프로필 전체의 해시. 재생은 해시가 다르면 facts를 거부 → rspec 변형마다 다시 추출(rspec는 추출 선언 0) | prof | JIXR | N | 1 | D 결합 삭제: facts 정체성 = 추출 규칙 판 + P0/L0 판(extraction_design R8, PI 결정 필요) | - | ● (+DRV-CLOSURE) |
| KS-P10 | `serialize.cpp:563-567`, `cmd_extract.cpp:24-26, 38-39, 81-83` | 출처 기록 공백: 프로필 대상만 BASIS에 from-reference로, 역할·종료·spawn·poll은 BASIS 항목 없음. "hand-entered"는 내장 표 12 + 명령줄만. 명령줄 `--target`은 BASIS에 두 번 | eng | R | N | 1 | K 입력 출처 추적(RV2 §8.1)으로 넓힌다 | - | ● |
| KS-P11 | `basis.cpp:207-232, 422-494` | source 핀과 ref eval const는 basis 항목만 만들고 facts 내용을 바꾸지 않는다 | prof | RI | N | 2 | U | P | →SRC-cfs, CON-refeval |
| KS-C01 | `cmd_extract.cpp:31-41`, `env/c1/kz_run_c1.sh:32-33` | 명령줄 `--target` 7(C1 드라이버만): OSAL 3(bind와 중복) + 앱 필드 4 `CF_Channel_t.sem_id`, `CF_ChannelConfig_t.sem_name`, `BP_Throttle_t.Name`, `.Sem` | cli | COX | Y | 7 | G R-T1/R-T2(V3에서 앱 필드 writer 수가 같음) | N | ● (+DRV-TARGET) |
| KS-C02 | `cmd_extract.cpp:47` | 지연 연산 `OS_TaskDelay`(세 드라이버 같음). 이것이 있어야 poll/count-bounded 분류, 탈출식, mod set, 술어, await 판단이 생긴다 | cli | CEXH | N | 1 | T P0 시간 대기 + 래퍼 유도 | N | ● (+DRV-OPS) |
| KS-C03 | `cmd_extract.cpp:48-49` | 잠금 쌍 `CFE_ES_LockSharedData`/`UnlockSharedData`뿐. L1 (ii) one-lock-region과 잠금 안 callee를 정한다. 다른 잠금과 callee 안의 해제는 보지 않는다 | cli | EC | N | 2 | T P0 mutex + 래퍼, 같은 뮤텍스 인자로 짝(F5) | N | ● (+DRV-OPS) |
| KS-C04 | `cmd_extract.cpp:50` | 명령줄 spawn 연산 `CFE_ES_CreateChildTask`(C5에서는 이것만) | cli | EI | N | 1 | T | N | ● (+DRV-OPS) |
| KS-C05 | `cmd_extract.cpp:51`, `env/redesign/extract_baseline.sh:60-62` | 명령줄 종료 연산: 사례 드라이버는 쓰지 않고, 기준선 G0만 4개를 준다 | cli | CH | N | 0 | T | N | ● |
| KS-C06 | `cmd_extract.cpp:126-128`, `basis.cpp:405-417` | extract의 `--extra-arg`·`--resource-dir`가 facts에 기록되지 않는다(basis와 달라도 드러나지 않음). 기본 resource dir은 빌드 때 고정 | cli | XR | N | 2 | S basis의 값을 쓰고 기록 | - | ● |
| KS-C07 | `cmd_extract.cpp:129-164` | `--root`(기록 경로의 기준, static_closure의 짝 키), `--source`(컴파일러 cc 고정, 시험용) | cli | IO | N | 2 | S | - | ● |
| KS-B01 | `tools/kz-static/cmd_basis.cpp:14-34`, `basis.cpp:235-348` | basis 명령줄 `--compile-db`, `--build-dir`, `--bundle-root`, `--link`(허용 사용자 입력). 객체 경로로 짝짓고 중복이면 첫 항목. R2는 작업 지시와 다른 cpu1 DB | cli | X | N | 4 | U | A | ● |
| KS-R01 | `lib/llvm/recipe.hpp:11-12` | recipe `link`·`binary`(R2 core-cpu1, C1 core-tx/rx, C5 core-linux.bin) | rcp | IX | N | 2 | U 빌드 출력의 link 명령에서 기계적으로 | P | ● (+RCP-cfs, RCP-c5) |
| KS-R02 | `recipe.hpp:16`, `env/c1/kz_run.sh:54-60`, `c5/basis/core-linux.kzb` | `module` 목록을 사람이 적는다: C1 7(드라이버에 고정), C5 5, R2 사례 0(닫힘에 앱 없음). 없으면 앱이 넘긴 주소를 받는 OSAL 본문이 없어 필드가 DISABLED | rcp | COX | M | 12 | U 시작 스크립트 줄 + 그 모듈의 link 명령(extract_baseline.sh L74-L85 규칙) | N | ● (+RCP-c5, DRV-RECIPE) |
| KS-R03 | `recipe.hpp:17-18` | `relocatable`(ld -r) 8(C5 psp.o … tbl.o). `env/c5/est_inputs.sh`가 빌드 로그에서 쓴다 | rcp | X | N | 8 | U link 입력 객체마다 -r 검사 | P | ● (+RCP-c5) |
| KS-R04 | `recipe.hpp:19-21`, `c5/basis/core-linux.kzb:82-83` | `front_end_diagnostic` 2(C5 osapi.c:647, cfe_psp_start.c:270; PI D7). 없으면 C5 추출이 osapi.c에서 멈춘다. 두 조사가 사례 여부를 다르게 적음(§8) | rcp | X | N | 2 | S 분석 전용 편차(결정 기록). 빌드 로그 경고에서 자동 생성·대조 가능 | N | ● (+RCP-c5-fed, DRV-RECIPE) |
| KS-R05 | `recipe.hpp:22-34`, `cfs/basis/core-cpu1.kzb:40-53`, `c5/basis/core-linux.kzb:89-109` | `startup_script`: reset(POWERON), reset별 파일 순서 rule, 파일 이름 상수, mount, cite. 고른 스크립트가 module 닫힘을 정한다 | rcp | OX | N | 5 | T L0 cFE 적재기 의미(판 표기·인용 대조). reset 종류는 U(실행 입력) | N | ● (+RCP-cfs-ss, RCP-c5-ss) |
| KS-R06 | `recipe.hpp:28-32, 123-129` | 진입 단어 CFE_LIB/CFE_APP, 기호 범위 global/local, module_file 필드와 경로 상수(C1·r2_all local, C5 global) | rcp | XI | N | 4 | T L0(또는 OS_ModuleLoad_Impl의 dlopen 플래그 상수) | N | ● |
| KS-R07 | `recipe.hpp:13-15`, `cfs/basis/core-cpu1.kzb` | generated 5, table 3, declared 1(R2 gdb 기록의 미확인 claim "24/11"), cite, name. basis 항목만, facts 불변 [M: V0 대 V0r 0줄] | rcp | R | M | 9 | U 생성 파일 목록은 빌드에서. declared claim은 V | N | ● (+RES-rcp-cfs, RCP-cfs, DRV-RECIPE) |
| RES-rcp-cfs | `cfs/basis/core-cpu1.kzb:34` | declared config.module_version_table.effect_on_observed_uses "24/11"(results/R2_gdb_v701.md §6, gdb 실행) | rcp | R | Y | 1 | V | N | →KS-R07 |
| RCP-cfs | `cfs/basis/core-cpu1.kzb:15-29` | link, binary, generated 5, table 3 | rcp | X | M | 10 | U | P | →KS-R01, KS-R07 |
| RCP-cfs-ss | `cfs/basis/core-cpu1.kzb:40-53` | startup_script 블록 | rcp | XO | M | 1 | T | N | →KS-R05 |
| RCP-c5 | `c5/basis/core-linux.kzb:59-76` | link, relocatable 8, binary, module 5 | rcp | X | M | 15 | U | P | →KS-R01, KS-R02, KS-R03 |
| RCP-c5-fed | `c5/basis/core-linux.kzb:82-83` | front_end_diagnostic 2 | rcp | X | Y | 2 | S | N | →KS-R04 |
| RCP-c5-ss | `c5/basis/core-linux.kzb:89-109` | startup_script 블록(readme가 인자 없이 실행 → POWERON) | rcp | XO | M | 1 | T | N | →KS-R05 |
| KS-H01 | `lib/llvm/spec.cpp:81-112` | 내장 데이터 함수 요약 12(memset … vsnprintf): memset 계열 writer, unknown-effect 제외, escape 면제. 표에 없는 함수(strlcpy, read 등)에 대상 포인터가 가면 unknown-effect/escape | tbl | CO | N | 12 | T P0(판 표기, 헤더 속성으로 대조) | N | ● |
| KS-H02 | `ast_run.cpp:101-107, 184-192` | front end 고정 인자(-Wno-unknown-warning-option, -Wno-error, -w, -resource-dir), 파생 빌드의 -fplugin 제거. basis가 deviation으로 기록 | eng | X | N | 2 | K 이미 공개 | - | ● |
| KS-H03 | `ast_tu.cpp:316-333`, `basis.cpp:907-945` | 시스템 헤더 제외(r2_core 54 건너뜀, 가정 G1), 본문은 정렬 순서 첫 단위에서 한 번, 이름 결합은 첫 정의 | eng | XI | N | 3 | K | - | ● |
| KS-H04 | `ast_fn.cpp:1055-1125`, `finalize.cpp` | table walk는 NULL 종단 꼴만 인식 → C1·C5의 코어 생성 순서가 추출되지 않는다(C1 cfe_es_start.c L716-L718은 상수 횟수 루프 + switch) [M: walk r2 3 / c1 0 / c5 0] | eng | EC | N | 1 | K 확장(정적 const 표 위 상수 횟수 루프의 반복별 부분 평가) | - | ● |
| KS-H05 | `finalize.cpp:407-430, 594-597` | table walk 안 간접 호출 대상의 writer에 `early-init`, 그 callee에 `init-tree` 표시(cFE 단계 이름을 붙임) | eng | ER | N | 1 | D 단계 이름 대신 "어느 walk의 몇 번째 반복" 사실로 | N | ● |
| KS-H06 | `ast_fn.cpp:636-715, 752-1053, 1187-1253` | 루프 분류와 술어 꼴: (a) 직접 시험, (b) counter-zero + cursor → forall(MainTaskSyncDelay 모양에 맞춤 [I]). 조건변수·세마포·큐 대기는 없음 | eng | EH | N | 1 | K(+ RV2 §3.6 호출 결과별 확장) | - | ● |
| KS-H07 | `unroll.cpp:515-517, 561-587` | L1 (i)의 관련 위치 = 술어 읽기 ∪ 모든 추출 대상 → await 인정이 대상 목록에 비단조로 의존 [M: R2 V1 undecided / V2 await / V3 fails] | eng | EJ | N | 1 | K 규칙 수정(술어가 읽는 위치로 한정) | - | ● |
| KS-H08 | `ast_fn.cpp:646, 2474-2481, 1840-1864` | atomic-read 인정(_Atomic, volatile sig_atomic_t), inline asm "memory" clobber | eng | EO | N | 2 | K C 언어 의미 | - | ● |
| KS-H09 | `ast_fn.cpp:1702-1713` | spawn entry = 첫 함수 포인터 매개변수, task-main 매개변수는 필드 기반 may-flow, 래퍼 유도는 한 단계 | eng | EI | N | 1 | K F7 고정점 | - | ● |
| KS-H10 | `ast_tu.cpp:186-216`, `finalize.cpp:609-636` | 대상 형·필드가 프로그램에 있는지 확인하지 않는다 → 없는 대상도 writer-set-complete가 될 수 있음 [I] | eng | OR | N | 1 | K 존재 확인 추가 | - | ● |
| KS-H11 | `basis.cpp:265-268, 792-794`, `linkset.cpp:139-160` | 빌드 관례: link.txt 실행 위치, `ar qc` 줄, GNU ld archive 규칙, -l은 프로그램 밖 | eng | X | N | 3 | K(archive 표를 직접 읽으면 U만 남음) | - | ● |
| KS-H12 | `basis.cpp:622-652` | basis의 시작 스크립트 문법('!'·'#' 줄 주석)과 대상 판 파서('!' = 파일 끝, cfe_es_apps.c L237-L322)가 다르다. 세 사례에서는 결과 같음 | eng | X | N | 1 | T 대상 판 파서와 대조 | N | ● |
| KS-H13 | `ast_fn.cpp:1468-1569, 1823`, `ast_tu.cpp:406-430`, `unroll.cpp:467` | 한도: derive 깊이 8, 포인터 단계 16, 초기값 펼치기 4096, 루프 부모 1000 | eng | CX | N | 4 | K 공개 | - | ● |
| KS-H14 | `docs/static_facts.md:132, 184` | 추상 위치 = (형, 필드). 인스턴스·키 분리 없음. C1의 키 분리는 프로필 `via`와 연구자 grep에 있다 | eng | IC | N | 1 | K + 경로 A의 키 분리 근거 | - | ● |
| KS-N01 | `EB/<build>/V0`(두 문장 프로필) | 프로필 없이 나오는 사실: FUNCS, FACTS+FACT_ARGS, call flag, FACT_LOOPS(poll 없음), LOOP_ITERS·STATIC_IMAGE(R2), FACT_CONSTS, CALL_TARGETS, FUNC_WRITES [M: 호출 5948/7908/3412] | oth | CX | N | 1 | K | A | ● |
| KS-N02 | `ast_fn.cpp:2437, 2454, 456, 1723, 1350-1387`, `finalize.cpp:432-498`, `ast_tu.cpp:500-503` | 어떤 입력으로도 안 나오는 것: 필드 읽기, 보통 호출의 guard, 검사된 결과의 분기 대응, 대상 밖 out-pointer 요약, 대상 밖 INIT | eng | CH | N | 5 | G 엔진 확장 F1–F7 | - | ● |
| KS-N03 | `lib/llvm/facts_query.hpp:11-25`, `docs/static_facts.md:41` | `kz-static reach` 질의 `--root`/`--loc`/`--callee`를 사람이 고른다(문서 예가 R2 이름) | cli | R | Y | 3 | G 모든 필드 → 인자 흐름과 spawn entry를 뿌리로 자동 실행 | N | ● |
| KS-N04 | `include/kz/core/eval_trace.hpp:427-430`, `cmd_replay.cpp:81` | 추출된 await 술어는 드라이버가 `--predicate`로 손으로 넘길 때만 판정에 들어간다 | drv | EJ | Y | 1 | G | N | →E21 |

### 4.5 엔진·런타임의 내장 지식

[M] 엔진 실행 코드에는 cFE/OSAL/CF/BP/SAMPLE 함수 이름, 사례 id, 이슈 번호, CF commit 해시가 없다(엔진 조사의 문자열 검색). 예외는 다음뿐이다.
- `runtime/src/kz_rt_osal.c`·`kz_rt_osal.h`의 OSAL 호출(E26).
- `tools/kz/cmd_hold.cpp:133` 주석(E30).
- 무관한 동음 이름: perf ABI의 `PERF_SAMPLE_*`, 런타임 칸 플래그 접두사 `CF_NOSLOT`/`CF_STR_TRUNC`.

| ID | 위치 | 내용 | 종류 | 영향 | 사례 | 수 | 대체 | 오늘 | 집계 |
|---|---|---|---|---|---|---|---|---|---|
| E01 | `lib/core/phi_provenance.cpp:103-105`, `phi_allowed.cpp:18-101`, `phi_check.cpp:637-639, 1323-1325` | 판정 근거 정책: caller 쪽에서 상태를 정할 수 있는 등급은 contract-backed·requirement-derived·admitted·issue-derived뿐. code-derived·researcher는 rule로만(QT-4, QT-9). 버그 이슈 글은 판정을 정하고 추출 사실은 못 정한다 | eng | J | N | 1 | D 정책 은퇴. RV2 §5.5 독립 항목으로 보고 | - | ● |
| E02 | `eval_trace.cpp:1877-1884, 3019, 3738-3830`, `kz_schema.h:719-753` | not-ready 발생의 상태 사상: allowed None → VIOLATING, Unspecified → NOT_READY_UNJUDGED, Entries → PENDING → 감시가 TOLERATED/VIOLATING(results_w6 §3.5: 두 줄 편집으로 뒤집힘) | eng | JR | N | 1 | D (E01과 같음) | - | ● |
| E03 | `eval_trace.cpp:3436-3653` | retry_until_ready 감시: 한 행위자·문맥·키·세대의 연속 시도. bound·codes는 프로필 상수. 주석이 "지금 사례 프로필이 보는 초기화 창"에서만 맞다고 적는다 | eng | JH | M | 1 | G 처리 사실(루프 사실 + 기록의 같은 키 반복, RV2 §5.6) | - | ● |
| E04 | `eval_trace.cpp:3655-3735`(terminal 단계 :124, :854, :1640-1661, :2218-2245) | 종료 감시와 terminal 단계: 코드는 op의 non-out 매개변수 하나(ExitApp 모양), process_terminal 뒤는 재생 안 함, teardown 뒤에는 결정 안 함 | eng | JH | M | 1 | G 정적 호출 그래프의 종료 도달(P0 씨앗) + 인자 상수 | N | ● |
| E05 | `phi_ast.hpp:241-253`, `phi_check.cpp:1356-1394`, `eval_trace.cpp:2177-2211, 3740-3741, 2814-2826` | 호출 문맥 역할(닫힌 어휘 dependent_use/existence_probe): existence_probe 지점의 호출은 발생에서 지운다. narrow 문맥이 역할 이름. BP의 조회-생성을 본뜬 구성 | eng | CJR | Y | 1 | G RA §3.1(실패 분기 → 같은 행위자의 같은 키 생성, 프로그램 순서). 구성 자체는 삭제 | N | ● |
| E06 | `eval_trace.cpp:2796-2976`, `phi_ast.hpp:315-370` | 결과 narrowing(P1) 게이트: api, 소스 rev SHA 목록, 역할 이름, 포착 인자, requires | eng | JO | M | 1 | G callee 반환 경로(F2) + 빌드의 소스 판 | - | ● |
| E07 | `phi_ast.hpp:500-507`, `eval_trace.cpp:1066-1138, 1890, 3307-3330` | 종료 범위 규칙: 요청 전에 결정되면 기본 범위, TEARDOWN 뒤 쓰기는 읽기에서 뺌, 감시는 teardown에서 멈춤(PSP 재시작·teardown 순서) | eng | JER | N | 1 | T PSP 종료 의미(판 표기). 지점은 정적 호출 그래프 | N | ● |
| E08 | `kz_schema.h:733-781`, `eval_trace.cpp:3060-3435` | 실행 관계 범주(ownership-undetermined, inter-application, core-application, creation-framework-new-application, intra-application)와 P10 규칙. cFS 앱 모델(보고 전용) | tbl | RI | N | 5 | T cFS API 어휘로 판 표기. 입력(소유·코어 여부)은 G | N | ● |
| E09 | `real_trace.cpp:243-289`, `cmd_replay.cpp:23-26, 74-80, 203-225` | `--core-request-caller FUNC`로 코어 서비스 판정. 드라이버는 CFE_ES_CreateObjects를 R2에만 준다(C1 생략, C5 "그런 함수를 정하는 레퍼런스 없음"). 관계 분류가 바뀜(r2_pilot_v153: core-application 246) | cli | JIR | M | 1 | G 코어 객체 표를 도는 생성 요청 함수(table walk·spawn 사실) 또는 시작 스크립트 앱의 여집합 | N | ● (+DRV-CORECALLER) |
| E10 | `real_trace.cpp:346-369`, `phi_check.cpp:793-797`, `kzrt_convert.cpp:389-428` | 초기 스레드는 앱 task가 아니다(프로필 initial_thread not_app + code-derived 인용) → core-application | eng | IR | N | 1 | T + Q(기록에 진입·등록 사건 없음) + main 호출 트리 정적 검사 | N | ● |
| E11 | `eval_concrete.cpp:175-177`, `eval_abstract.cpp:321-326`, `eval_trace.cpp:1203-1211`, `real_trace.cpp:38-58` | 자원 ID 공식: valid ⇔ (x − base) mod 2^32 ≤ serial_max, 색인 = 그 값 mod 표 크기, 무효 값 저장 = clearing(P10). CFE_ResourceId_ToIndex 의미이고 코드에 판 표기가 없다 | eng | IJR | N | 1 | T 판 표기 지원(또는 ToIndex 본문의 식 사실에서 G) | N | ● |
| E12 | `identity_rule.cpp:11-370`, `kz_raw.h:464-500`, `kz_neutral.h:262-294` | 생성 요청 판독기 닫힌 어휘: owner-request(ParentAppId), child-request, app-request(rev 6, C5용으로 추가: 생성 뒤 게시값·request-return·owner-check). cFE ES StartAppTask·CreateChildTask·6.4.x AppCreate 모양 | eng | I | M | 10 | T cFE ES 생성 의미(판 표기) + G 경로 C | N | ● |
| E13 | `identity_rule.cpp:276-312` | 알린 id로 묶기: 같은 id를 준 spawn이 하나, 알린 행위자가 하나면 confirmed. 알린 값이 자기 생성의 것인지 검사하지 않는다 → C5에서 재사용된 pthread_t로 SAMPLE_APP을 owner 5로 확정(E2, #72 아님) | eng | IR | N | 1 | O 단계 1 독립 생성 측 토큰. 알린 값이 분석된 조회에서 왔으면 confirmed를 거부 | - | ● |
| E14 | `kz_raw.h:464-500`, `identity_join.cpp`, `decode_sites.cpp:118-157` | F0 정체성 결합의 고정 slot 위치(SPAWN_CREATE a[1] entry_fn, a[2] priority …)와 시간 포함 → inferred | eng | IO | N | 1 | G 원형(매개변수 이름·형)과 ABI에서 | N | ● |
| E15 | `real_trace.cpp:291-345`, `occ_table.cpp:84-125, 234-240` | task 순번과 occ 키(rspec · op · owner app[key]#task · 자원 · 순번). 실행 간 비교가 rspec 이름에 기댄다 | eng | R | M | 1 | G 호출 지점 + 자원 종류·키로 | - | ● |
| E16 | `kz_schema.h:682-700`, `phi_check.cpp:1349-1351` | caller 수준 harm 어휘 15(event_request_rejected = R2의 EVS/SB 증상, wrong_identity_stamped도 R2에서) | tbl | JR | M | 15 | Q 기록의 최종 영향. 어휘는 판 표기 표로만 | N | ● |
| E17 | `kz_schema.h:609-613, 671-680` | consequence·progress-harm 어휘 19(ES·EVS·SB·PSP 개념) | tbl | R | M | 19 | Q | N | ● |
| E18 | `kz_schema.h:800-818`, `identity_join.cpp:487-495` | owner 근거 flag es-records(설정하는 코드 없음 [M]), analysed-table-excluded | tbl | IR | N | 1 | D 죽은 항목 | - | ● |
| E19 | `phi_parser.cpp:814`, `phi_check.cpp:898, 984`, `eval_trace.cpp:1786-1794` | 매개변수 역할 닫힌 어휘 {out, stamp, key, id, buffer, msg}. stamp는 R2 SendEventWithAppID의 AppID를 위해 생겼다(사용 1곳 [M]) | eng | IR | M | 7 | G out은 원형 + OUTPTR, 정체성 운반 인자는 경로 B | N | ● |
| E20 | `eval_trace.cpp:1695-1700, 1814, 2045-2135, 3915-4108, 2486-2491` | 고정 hb 템플릿 5(program_order, spawn, phase, observe, reads_from), cFE ES 시작 모양. 세마포 give/take·큐·mutex 템플릿 없음, sync·env effect는 재생에서 no-op | eng | E | N | 5 | T OSAL 동기화 기본 의미(give → take 등, 판 표기) + G spawn·루프 사실 | N | ● |
| E21 | `eval_trace.cpp:2061-2104`, `cmd_replay.cpp:81, 225` | observe 규칙의 `predicate from_code`를 엔진이 추출하지 않는다. `--predicate`가 없으면 간선 없음. 드라이버는 하나도 주지 않는다(시험만) | cli | JE | M | 1 | G 재생이 FACT_LOOPS 술어를 직접 읽음 | N | ● (+KS-N04) |
| E22 | `cmd_replay.cpp:12-14, 79`, `eval_trace.cpp:3496-3502, 2838-2843` | ref eval 상수를 `--const`로: MAX_APPLICATIONS=32(18회), OS_MAX_API_NAME=20(10회). 표 크기가 없으면 거부 | cli | JI | N | 2 | G compile DB로 설정 헤더 상수 평가(basis에 이미 있음) | N | ● (+DRV-CONST) |
| E23 | `cmd_replay.cpp:3-90`, `eval_trace.hpp:430-470` | 읽기 스위치 7(--outcome-narrowing, --owner-identity-targets(A-OWNER), --reads-from-edges, --reads-from-outcome, --clearing-writes, --control-order(P11), --compare-narrowing). 모두 판정을 바꾼다. 드라이버의 민감도 묶음 cur·p1off·nocode·rfoff·idoff·ctloff | cli | JEI | N | 7 | S 현재 읽기를 고정·공개하고 실행마다 기록 | - | ● (+DRV-READINGS) |
| E24 | `eval_trace.cpp:1586`, `cmd_replay.cpp:3, 77` | 판 guard(`guard cfe >= "6.4.2"`)를 `--version`으로 평가 | cli | EJ | N | 1 | G 빌드 입력의 판 헤더·git rev(lib/llvm/gitrev) | N | ● |
| E25 | `runtime/include/kz_rt.h:118-130`, `kz_rt.c:950-990`, `kz_neutral.h:411-416`, `kz_rt_manifest.cpp:143-157` | 보류 가능 문맥 osal-internal(registration-internal), ALLOW_OSAL_INTERNAL, LOCK_PRIVATE. manifest가 결합마다 지정 | eng | OE | N | 1 | T + F5 잠금 사실 | N | ● |
| E26 | `runtime/src/kz_rt_osal.c`, `kz_rt_osal.h` | 런타임의 OSAL 호출 구현: 제어 잠금·대기 칸 잠금은 OS_MutSem*, 대기는 OS_BinSem*, 시계는 OS_GetLocalTime(OSAL f34759d 헤더 줄 인용). 골격이고 실행한 적 없음 | eng | OI | N | 1 | T 이미 판 표기·인용 | - | ● |
| E27 | `phi_check.cpp`, `tools/kz-phi/main.cpp:7-10`, `phi_provenance.cpp` | 출처 검사(모든 주장에 기록, researcher는 authored, 역할·not_app·stored_out·narrow의 인용 등급)와 verify-prov(줄 해시·식별자·rev 대조) | eng | R | N | 6 | K 입력 출처 추적과 OSAL/cFE 의미 검증의 기반 | - | ● |
| E28 | `lib/trace/static_closure.cpp`, `eval_trace.hpp:84-96`, `phi_ast.hpp:222-232` | bind via 키 분리: 연구자가 이름 붙인 함수 밖의 writer를 "다른 키의 쓰기"로 덮음 → writer 적합성과 판정에 들어간다 | eng | OJ | M | 1 | G 경로 A 키·색인 분석 | N | ● |
| E29 | `include/kz/predict/model.hpp:19-50` | 예측 모델 언어의 내장 의미: `create … taken CONST`(OS_CountSemCreate·이름 점유 모양), `exit OP CONST`(ExitApp 모양), use·delete-each, hold 조건 caller/name | eng | CHE | M | 1 | G cone을 정적 사실 + 기록 접두사에서(RP §3.6). 못 하면 D | N | ● |
| E30 | `tools/kz/cmd_hold.cpp:132-151` | 프로필 바이너리와 실행 파일을 basename으로 짝(주석의 예가 C1 core-tx/core-rx) | eng | O | M | 1 | K 사례는 주석에만 | - | ● |
| E31 | `runtime/src/kz_rt_posix_auto.c:17-38, 79-160` | 대상 환경 변수 KZ_RT_MODE/DUMP/RUN_ID/INSTANCE/ACTOR_QUOTA/STORE/PLAN | oth | O | N | 7 | S 기록(CONDITIONS_POLICY) | - | ● |

### 4.6 관측·계측 manifest

| ID | 위치 | 내용 | 종류 | 영향 | 사례 | 수 | 대체 | 오늘 | 집계 |
|---|---|---|---|---|---|---|---|---|---|
| OBS-0 | `c1/observe/sites-cf281a941.kzp` L5-L6, `cfs/observe/sites.kzp` L45-L48, `*/instr/*.kzs` 머리 | 결합 규칙: point site 라벨을 Φ site·LP 이름(OP.LABEL, 역할 지점)으로 지어 재생이 대응시킨다. kzs 위치는 `20_ops.kzp`의 `lp at`을 따른다 → 사람이 manifest와 프로필 양쪽에 둔 곳만 발생이 된다 | obs | COXJ | Y | 1 | G 규칙 관측점(OP-*, extraction_design §4). 사건 → 후보 대응도 추출 사실로 | N | ● |
| RES-obs-cfs | `cfs/observe/sites.kzp:69` | researcher 기록 spec_s1r2(SPEC_FINAL L2818, S1-R2 sub-gate)가 site 0-7의 근거. 인용만 하고 해시는 안 함 | obs | O | Y | 1 | G 관측점은 추출 후보에서. 기록은 삭제 | N | ● |
| OBS-cfs-entryret | `cfs/observe/sites.kzp:74-212` | 함수 진입 11·반환 8(evs.send, es.sync, osal.task_create, evs.register, es.wait …), SysV x86-64 레지스터 slot | obs | JOIX | M | 19 | G 후보 API의 진입·반환 + DWARF 형식 매개변수·ABI | N | ● (+OBS-R2-BASE) |
| OBS-cfs-points | `cfs/observe/sites.kzp:107-226` | Φ LP의 point site 18: EVS 등록·송신, ES main reset 2, core reserve/publish/clear, sb_publication, 포착 지점 `id_valid@sb_api_275/1183/sb_priv_786`, EarlyInit memset, CleanUpApp, PSP 요청·teardown | obs | CJOX | Y | 18 | G writer 위치·호출 사실. 포착 3은 DERIVES 행(core_cpu1.golden L503/525/532) | N | ● (+OBS-R2-BASE, OBS-R2-FIELD, OBS-R2-WRITERS) |
| OBS-cfs-interval | `cfs/observe/sites.kzp:78, 208-212` | R2 구간 경계 es.main.operational, es.restart_cmd.reset_cfe, sb.sub_reporting.set + sb.appid.store(중복). 재생은 쓰지 않음. EID 14/22/25 차이 뒤에 추가 | obs | JOR | Y | 4 | D 분석 구간 = 기록 자체의 범위(ES 상태 저장 추출은 G로 가능) | N | ● (+OBS-R2-INTERVAL) |
| OBS-cfs-identity | `cfs/observe/sites.kzp:85-87, 141-149` | 생성 측 정체성 site 8과 판독기(owner-request r8 owner_id, request-return ×2, child-request, spawn-create/return, spawn-id r9, thread-entry) | obs | OEI | M | 8 | O 판독기 역할은 정적 흐름에서 G, 독립 생성 측 토큰은 단계 1 | N | ● (+OBS-ID) |
| OBS-cfs-prov | `cfs/observe/sites.kzp`(prov 46 − 1) | manifest 안 인용(code-derived 43, contract-backed 2) | obs | R | M | 45 | G 사실 위치(파일:줄:열 + 해시) | P | ● |
| OBS-c1-sites | `c1/observe/sites-cf281a941.kzp`(33), `sites-cf833fdbb.kzp`(34) | CF 판마다의 F0 계획(core-tx, cf.so, bp.so): countsem entry/ret(fetch-arg 문자열), tail-jump·인라인 OSAL 진입 도달 point, cf.so/bp.so 역할 지점, 시작 상태·삭제·범위, 생성 측 정체성, cf_retry_delay(833fdbb). tx만(P7) | obs | CJOEIXH | Y | 67 | G 추출 호출 지점 + 바이너리 역어셈 | N | ● (+OBS-C1-API, OBS-C1-LIFE, OBS-ID, OBS-C1-CF, OBS-C1-BP-ROLE) |
| OBS-c1-layout | `c1/observe/sites-*.kzp:71, :136` | 손 배치 사실: found `s0=+16(rdi):u32`(토큰 obj_id 오프셋 16), task_create.id `s0=r8`(이 OSAL에서는 5번째 인자) | obs | OI | M | 4 | G DWARF 멤버 오프셋·매개변수 위치 | N | ● |
| OBS-c1-prov | `c1/observe/sites-*.kzp`(30, 31) | manifest 안 인용(code-derived 59, supplier 2) | obs | R | M | 61 | G | P | ● |
| OBS-C1-API | `sites-cf{rev}.kzp` L59-L78, L119-L121 | 세부: countsem API의 모든 호출자(조회·생성·삭제·found·created). C1에서 관측하는 OSAL 자원 종류는 버그의 자원 하나뿐 | obs | OXJ | M | 11 | G(OP-A4 깊이에서만 나옴. found·Delete ×3은 규칙이 못 냄) | N | →OBS-c1-sites |
| OBS-C1-LIFE | `sites-cf{rev}.kzp` L114-L126 | 세부: 시작 상태와 종료 범위(clear_table, clear_names, request_shutdown, teardown). 사례 프로필의 Φ effect 이름 | obs | OJE | M | 4 | G 수명 분류(CountSemAPI_Init·Application_Run은 규칙이 못 냄) | N | →OBS-c1-sites |
| OBS-ID | C1 `sites-cf{rev}.kzp` L129-L139, R2 `sites.kzp` L85-L87, L141-L149 | 세부: 생성 측 정체성 site와 판독기, OSAL 판마다의 레지스터 위치(C1 r8, v7.0.1 r9) | obs | IE | N | 16 | O/G | N | →OBS-cfs-identity, OBS-c1-sites |
| OBS-C1-CF | `sites-cf{rev}.kzp` L80-L92 | 세부: CF site(init, init_engine, cf_lookup_call, cf_retry_delay, ExitApp 진입을 "CF 종료"로) | obs | OJH | Y | 7 | G(CF_Init은 F2가 있어야 나옴) | N | →OBS-c1-sites |
| OBS-C1-BP-ROLE | `sites-cf{rev}.kzp` L95-L102, `c1/10_kinds.kzp:74-76, 86-97` | 세부: BP site. 라벨 bp_existence_probe_call이 BP의 조회를 역할 bp-existence-probe(발생 아님)에 대응시킨다. E3의 구분을 사람이 라벨로 한다 | obs | JCO | Y | 4 | G RA §3.1 | N | →OBS-c1-sites, ROL-c1-bp |
| OBS-C1-SCOPE | `sites-cf{rev}.kzp` L10-L14, L28-L30 | 관측 범위: tx 인스턴스의 core-tx·cf.so·bp.so만(bplib, ci_lab, to_lab, sch_lab 없음). rx는 한 인스턴스 실행으로 따로 | obs | OX | Y | 1 | U 모듈 목록은 시작 스크립트·link에서 | N | ● |
| OBS-R2-BASE | `cfs/observe/sites.kzp` L74-L122 | 세부: evs.send, sb.appid.store, es.sync, es.wait, raise_state, X-03 등록·writer 효과 | obs | OJX | M | 18 | G(evs.send는 OP-B 39곳. Register ×4는 규칙이 못 냄) | N | →OBS-cfs-entryret, OBS-cfs-points |
| OBS-R2-FIELD | `cfs/observe/sites.kzp` L155-L169, `cfs/instr/sites.kzs` L170-L186 | 세부: sb_publication + SB 포착 3. "ES·EVS가 그 저장 전에 닿는 SB 호출 지점"으로 골랐다(버그 지식). 필드 → 인자 연결 자체는 kz-static derives | obs | OJC | Y | 4 | G 경로 B | N | →OBS-cfs-points |
| OBS-R2-WRITERS | `cfs/observe/sites.kzp` L171-L190 | 세부: EVS AppID·SB AppId의 나머지 writer(EarlyInit memset 2, cleanup). writer는 kz-static, 필드 둘은 사례 선택 | obs | OJ | M | 5 | G | N | →OBS-cfs-points |
| OBS-R2-INTERVAL | `cfs/observe/sites.kzp` L208-L226, `instr/sites.kzs` L207-L231 | 세부: 구간 경계와 기본 범위의 끝 | obs | ROJ | Y | 5 | D | N | →OBS-cfs-interval |
| INS-cfs | `cfs/instr/sites.kzs` | kzs-sites 2: site 44, value 60, identity 12, equiv 44(unconfirmed). 위치는 LP. identity 1이 `--core-request-caller CFE_ES_CreateObjects`에 기댐. es.start_task를 호출자별로 grep해 나눔. callee 쪽 GetAppID.store는 뺌 | obs | OI | Y | 44 | G 추출 writer·호출 사실 | N | ● (+INS-R2) |
| INS-c1 | `c1/instr/sites-cf{281a941,833fdbb}.kzs` | CF 판마다의 manifest 37/38 site(value 53/54, identity 8, equiv 33/34). 모듈 core 2·cf.so·bp.so. GCC 전용 at-call 4 | obs | OIX | Y | 75 | G. 판마다의 파일은 분석 빌드 하나로 사라짐 | N | ● (+INS-C1) |
| INS-c1-control | `c1/instr/sites-cf{rev}.kzs` control 34, 35, 37, 44, 45 | hold 계획의 gate/release에 맞춘 control 줄(kz_rt_ctl). "잠금 없음"은 손으로 확인. 줄이 없으면 kz rtplan이 거부(W3/W13) | obs | OEH | Y | 10 | G 추출 후보 쌍에서 계획과 control 생성(RV2 §3.8) | N | ● (+INS-C1-CTL) |
| INS-bound20 | C1 `sites-*.kzs` 값 5 × 2, C5 `sites-64*.draft.kzs` 값 1 × 2 | `string=validated bound=20`: 생성된 osconfig.h의 OS_MAX_API_NAME 20을 복사 | obs | O | M | 12 | G 상수 평가 | N | ● |
| INS-c5 | `c5/instr/sites-64{1,2}.draft.kzs`(33/31) | DRAFT manifest: AppCreate unlock/create/relock/게시, ES 잠금, OSAL 생성·등록, GetAppID(Int), SB/EVS 소비자, sample_app 진입. 판독기 app-request·owner-publish·owner-check·spawn-id·thread-entry. pthread_self(19)–pthread_t(15) 결합이 재사용 pthread_t를 확정(E2). 머리의 "DRAFT: not used by any run"이 실제 사용과 어긋남 | obs | JOIHR | Y | 64 | G site. 정체성 결합은 O(단계 1) | N | ● (+INS-C5, INS-C5-ID, INS-C5-CTL) |
| INS-C1 | (plans) `c1/instr/sites-cf{rev}.kzs` | 같은 파일 | obs | OIX | Y | 2 | G | N | →INS-c1 |
| INS-C1-CTL | (plans) control 34, 35, 37, 44, 45 | 같은 control 줄 | obs | HEO | Y | 5 | G | N | →INS-c1-control |
| INS-R2 | (plans) `cfs/instr/sites.kzs` | 같은 파일 | obs | OI | Y | 1 | G | N | →INS-cfs |
| INS-C5 | (plans) `c5/instr/sites-64{1,2}.draft.kzs` | 같은 파일(DRAFT 표지 어긋남: derive_c5.sh L41, run_c5.sh L54, replay_c5.sh L94가 씀) | obs | OIJR | Y | 2 | G | N | →INS-c5 |
| INS-C5-ID | `sites-641.draft.kzs` identity 1, 2, 8, 33, 13, 16, 17, 20 | 세부: E-ID1 설계(app-request, owner-publish L657, owner-check L651, thread-entry = OS_task_table에서 pthread_self 검색). 재사용 pthread_t에서 틀린 owner를 확정 | obs | IR | Y | 8 | O 단계 1 | N | →INS-c5 |
| INS-C5-CTL | `sites-641.draft.kzs` L57-L59 | 세부: control 5 = C5 A6 보류 지점(cfe_es_apps.c:610/637), 계획 없음 | obs | H | Y | 1 | D 버그 창에 둔 지점 | N | →INS-c5 |
| INS-STAGEC | `instrument/gcc/sites/{c1_post_1df262a,r2_v701}{,.identity}.kzs`, `instrument/clang/sites/*.kzs` | 단계 C 연결기 비교 manifest(gcc 16/15 + identity, clang 28/16). R2 머리 "What R2 needs (cases/R2.md section 5 …)". 원형 사용만 | obs | OI | Y | 6 | V 연결기 비교(RQ4) 자료. 새 manifest는 G | N | ● |
| INS-SCOPE | `derive_c1.sh` L74, `build_c1.sh` L70, `derive_c5.sh` L74, `derive_r2.sh` L76, `build_r2.sh` L71 | 계측 모듈: C1은 CF·BP만, C5는 sample_app만(sample_lib·ci_lab·to_lab 없음), R2는 core만 | drv | OX | Y | 3 | U 시작 스크립트·link에서 모듈 목록 | N | ● |

### 4.7 제어(hold) 계획

| ID | 위치 | 내용 | 종류 | 영향 | 사례 | 수 | 대체 | 오늘 | 집계 |
|---|---|---|---|---|---|---|---|---|---|
| HLD-1 | `c1/hold/hold-cf281a941.kzh:27` | gate G_cf_lookup site=osal.countsem_lookup.entry when caller-in cf.so occurrence first: 소비자 CF를 첫 조회에서 잡음(controlled A2, P-C1HOLD 밖의 연구자 개입) | hold | OEHR | Y | 1 | G 추출 후보 쌍에서 계획 생성(RP §3.6, RV2 §3.8) | N | ● (+HOLD-C) |
| HLD-2 | `c1/hold/hold-cf281a941.kzh:28` | release R_bp0_done site=osal.countsem_create.entry when name sem_name "BP1_tsem" first: BP 루프 순서를 읽고 둘째 이름 생성 시작을 BP0_tsem 존재의 증거로 씀 | hold | OEH | Y | 1 | G 생성 계획은 짝의 쓰기 완료 관측으로 release | N | ● (+HOLD-C) |
| HLD-3 | `c1/hold/hold-cf833fdbb.kzh:22` | gate G_bp0_create site=osal.countsem_create.entry when name "BP0_tsem" first: 제공자를 잡아 CF가 재시도를 소진하게(P-C1HOLD, E-06) | hold | OEHR | Y | 1 | G | N | ● (+HOLD-B) |
| HLD-4 | `c1/hold/hold-cf833fdbb.kzh:23` | release R_cf_exit site=es.exit_app.entry when caller-in cf.so first(R-d: CF의 종료가 최종 init 결과를 뜻함) | hold | OEH | Y | 1 | G | N | ● (+HOLD-B) |
| HLD-timeout | `hold-cf281a941.kzh:24-25`, `hold-cf833fdbb.kzh:19-20`(주석) | 계획에 없는 harness 제한시간("pilot safeguard, researcher choice") | hold | OR | Y | 2 | S | N | →HOLD-RUN |
| HOLD-B | `hold-cf833fdbb.kzh` L22-L23 | 단계 B 계획(= HLD-3, HLD-4) | hold | HER | Y | 1 | G | N | →HLD-3, HLD-4 |
| HOLD-C | `hold-cf281a941.kzh` L27-L28 | 단계 C 계획(= HLD-1, HLD-2) | hold | HER | Y | 1 | G | N | →HLD-1, HLD-2 |
| HOLD-PRED | `/home/user/work/w6/predict/{pre,post}_{1df262a,1b0b338}/cur/W<n>.kzh` | kz predict가 후보 분류마다 합성한 계획 28. 이름 조건 "BP0_tsem"/"BP1_tsem", caller-in cf.so/bp.so, occurrence n은 모델의 hit 줄에서 | hold | CHE | Y | 28 | G 생성기는 두고 입력을 추출 사실로 | P | ● |
| HOLD-SEL | `env/c1/ctl_w6_predict.sh` L5-L9, L60, `plan/predict_w6_20261029.md` L17, §4 | 돌릴 예측 계획 선택(post W1, pre W3: 단계 B/C에 맞춤). 라벨은 어느 행위자를 잡는지로 호출자가 준다 | drv | HR | Y | 1 | G 모든 후보를 실현하고 분류로 순위(규칙) | N | ● |
| HOLD-RUN | `kairos/probe/scripts/observe_c1_hold.sh` L35, L9-L18, L175 | hold 실행 harness: 벽시계 150 s 제한(SAFEGUARD_S, "pilot safeguard, researcher choice", 걸리면 INCONCLUSIVE), 라벨 문안은 호출자, 실행당 인스턴스 하나(P7) | drv | OEHR | M | 1 | S unspecified_by_reference로 기록하거나 사건 기반 제어로(CONDITIONS_POLICY) | N | ● (+HLD-timeout) |

### 4.8 예측 모델

예측 모델 13 인용은 `c1_runs/prov_roots`에서 다시 읽어 13/13 일치한다(계획 조사). 그러나 모델의 구성(누가 행위자인지, 어떤 순서를 남기는지, 무엇을 모델에서 뺐는지)은 유도되지 않는다.

| ID | 위치 | 내용 | 종류 | 영향 | 사례 | 수 | 대체 | 오늘 | 집계 |
|---|---|---|---|---|---|---|---|---|---|
| PRD-cites | `c1/predict/00_common.kzm:25-37` | 해시 인용 13(core → 스크립트 순서, 스크립트 줄 루프, task cleanup, 생성자, BP throttle 수, BP flow → CLA, CF 채널 루프, CF init 성공, RunLoop false 등) | pred | RE | Y | 13 | G 사실 위치(모델을 사실에서 만들면) | P | ● (+PRED-COMMON) |
| PRD-cone | `c1/predict/00_common.kzm:39-46` | 자원 s0/s1(osal.countsem 키 CF_CHANNEL_0/1_SEM_NAME)·shutdown, 행위자 core-tx·CFE_ES·CF·BP(기록 이름으로 묶음) | pred | CJEIH | Y | 7 | G cone = 기록에서 추출 자원에 닿는 행위자, 자원 = 추출 키 | N | ● (+PRED-COMMON) |
| PRD-coretx | `c1/predict/00_common.kzm:48-58` | core-tx 프로그램: clear, CFE_ES·CF·BP spawn(코어 → 스크립트, CF 앞 BP), 종료 대기, teardown, delete-each | pred | EC | Y | 1 | G table walk 순서 + 시작 스크립트 + 프로그램 순서 | N | ● (+PRED-COMMON) |
| PRD-cfees | `c1/predict/00_common.kzm:61-65` | CFE_ES 프로그램: 재시작 명령(무한 외부 입력) 뒤 종료 요청 | pred | E | M | 1 | Q 기록된 입력 사건 | N | ● (+PRED-COMMON) |
| PRD-bp | `c1/predict/00_common.kzm:69-105` | 손으로 쓴 BP initialize_throttling CFG(이름 2를 펼침: probe → found 또는 create, taken, 이벤트 14/15) + "BP_CLA_Init 성공(모든 기록처럼)" 가정 | pred | CE | Y | 1 | G 함수 CFG + 호출·결과 검사 + 상수 횟수 펼침. 기록 기반 가정은 라벨 | N | ● (+PRED-COMMON) |
| PRD-cf281 | `c1/predict/cf-281a941.kzm:9-31` | CF 281a941 프로그램: 조회 2, 성공이 아니면 이벤트 30 → NAME_NOT_FOUND 반환 → APP_ERROR 종료 | pred | CJEH | Y | 1 | G InitEngine·AppMain CFG | N | ● (+PRED-CF) |
| PRD-cf833 | `c1/predict/cf-833fdbb.kzm:9-50` | CF 833fdbb 프로그램: 카운터 j 재시도(< CF_STARTUP_SEM_MAX_RETRIES), 지연, 그다음 실패 경로 | pred | CE | Y | 1 | G CFG + FACT_LOOPS | N | ● (+PRED-CF) |
| PRED-COMMON | (plans) `00_common.kzm` | 공통 모델(= PRD-cites … PRD-bp). 13 인용 재확인 13/13 일치 | pred | CEJH | Y | 1 | G | N | →PRD-cone, PRD-cites, PRD-coretx, PRD-cfees, PRD-bp |
| PRED-CF | (plans) `cf-*.kzm` | CF 판마다의 프로그램(= PRD-cf281, PRD-cf833) | pred | CHJ | Y | 2 | G | N | →PRD-cf281, PRD-cf833 |
| PRED-OPT | `env/c1/predict_w6.sh` L76-L79, kz predict 기본값 | 예측 입력·한도: `--model`(사례), `--observe-profile`(사례 계획), `--prov-root`, `--max-states 2000000`(연구자 선택), 시간 없는 interleaving. 인스턴스 행위자 = dependent_use 역할이 있는 행위자 | cli | CJ | Y | 4 | G 입력을 추출 사실로(못 하면 predict 제거, RP §3.6). `--max-states`는 S | N | ● |

### 4.9 드라이버·명령줄·보고·기대표

| ID | 위치 | 내용 | 종류 | 영향 | 사례 | 수 | 대체 | 오늘 | 집계 |
|---|---|---|---|---|---|---|---|---|---|
| DRV-PROFILE | `env/c1/replay_w6.sh` L110-L111, `predict_w6.sh` L44, `kz_run_c1.sh` L31, `basis_gi.sh` L26, `replay_gi.sh` L40, `observe_c1_*.sh`, `env/r2/replay_w6.sh` L66, `env/c5/replay_c5.sh` L45 | 모든 재생·예측·kz-static 드라이버의 `--profile` 사례 디렉터리: C1 = 공통 + cf-<rev> + bundle-<cand> + rspec-<rev>[-altP3](pre → 281a941, post → 833fdbb), R2 = cfs-v7.0.1, C5 = c5 + cfe-6.4.<V> | cli | CJIXH | Y | 12 | D 사례 프로필 층 제거. 남는 것은 P0/L0 판(RP §1.2 #1: rspec 층을 빼면 발생 0) | N | ● |
| DRV-CLOSURE | `kz_run_c1.sh` L31-L43, `basis_gi.sh` L26-L33, `replay_c5.sh` L16-L24, L48-L52 | 닫힘·B′를 사례 프로필로 뽑아 rspec 변형마다 하나(c1prof, c1prof_altP3, c1prof_rx). C5는 est-64<V> 대신 Φ 프로필로 다시 만듦 | drv | XJ | Y | 3 | D | N | →KS-P09 |
| DRV-TARGET | `env/c1/kz_run.sh` L37-L38, `kz_run_c1.sh` L32-L33, `basis_gi.sh` L28-L29 | `--target` 7(C1만) | cli | X | Y | 7 | G | N | →KS-C01 |
| DRV-OPS | `kz_run.sh` L39, `kz_run_c1.sh` L41, `basis_gi.sh` L31, L39, `env/r2/replay_w6.sh` L46-L47, `replay_c5.sh` L50-L51 | `--delay-op`·`--lock-op`·`--unlock-op`·`--spawn-op` 4. 6.4.x는 "같은 함수가 있다"만 확인 | cli | XHE | N | 4 | T | N | →KS-C02, KS-C03, KS-C04 |
| DRV-CONST | `observe_c1_smoke.sh` L158-L159 등 | `--const` 2 | cli | JI | N | 2 | G | N | →E22 |
| DRV-CORECALLER | `w4w5_r2.sh` L62, `replay_gi.sh` L53, `env/r2/replay_w6.sh` L66, `regress_eid1.sh` L117 | `--core-request-caller`를 R2에만 | cli | IRJ | M | 1 | G | N | →E09 |
| DRV-READINGS | `replay_gi.sh` L22-L30, `env/c1/replay_w6.sh` L140-L146, `ctl_w6_predict.sh` L91-L95, `replay_c5.sh` L106-L108 | 읽기 스위치 묶음 | cli | JEI | N | 7 | S | - | →E23 |
| DRV-ENV | `replay_gi.sh` L10-L12, L38, L49, `basis_gi.sh` L13-L19, `env/c1/replay_w6.sh` L125-L126, `replay_c5.sh` L96-L97 | `KZ_SITES`(변환 manifest 덮어쓰기), `KZ_PROFILES`(프로필 스냅숏), manifest sha256 = run.meta 확인 | drv | O | M | 3 | S run.meta에서(이미 대조) | P | ● |
| DRV-RECIPE | `cfs/basis/core-cpu1.kzb`, `c5/basis/core-linux.kzb`, `env/c1/kz_run.sh` recipe() | recipe의 손 선택 항목(생성 파일·표, declared claim, front_end_diagnostic, C1 module 7) | rcp | X | M | 3 | U | N | →KS-R02, KS-R04, KS-R07 |
| DRV-RUNCOND | `env/c5/run_c5.sh` L1-L54, `observe_c1_smoke.sh` L44-L58, `observe_cfs_smoke.sh --start poweron`, `env/run_cfs.sh` | 사례별 실행 조건: C5 root·SCHED_FIFO·IPC ns msg_max 64·30 s·5 s(unspecified_by_reference), C1 CI 시작·정지, R2 Power-On 시작과 ci-es-restart-poweron 정지 | drv | O | Y | 3 | U 실행 입력. 편차는 S로 기록 | N | ● |
| DRV-GDB | `env/run_r2_gdb.sh`, `tool/gdb/r2_trace.py` L5-L100 | R2 기준 관측(보고자의 breakpoint cfe_evs.c L188과 연구 지점). Kairos가 아닌 보고자의 방법 | drv | OR | Y | 1 | V | N | ● |
| RPT-CLASSIFY | `instrument/gcc/scripts/classify_gih.awk` L6-L40 | 실행 분류 표기: CF = cf_lookup_call 행위자, BP = bp_create_call 행위자, 이름별 결과 0/−17, 끝 x/f | drv | R | Y | 1 | Q 일반 사용별 결과 이력 | N | ● |
| RPT-C5TAB | `env/c5/replay_c5.sh` L124-L233 | C5 결과표: 사이트 번호 고정, ERR_APPID 0xc4000001, 자기 spawn = pthread_t가 pthread_self와 같은 최근 spawn(E2의 재사용 위험과 같음) | drv | IR | Y | 1 | O 단계 1 토큰 | N | ● |
| RPT-R2INT | `instrument/gcc/scripts/r2_intervals_tag.sh` L10-L27, L114-L118 | R2 구간 표지와 순서 근거 순위(po; sync: raise_state → MainTaskSyncDelay(RUNNING) → OPERATIONAL; rf; time; none). 엔진 밖 스크립트에 cFE ES 동기 의미 | drv | ER | M | 1 | G 상태 저장·대기 추출(+T 판 지원) | N | ● |
| RPT-R2SITES | `instrument/gcc/scripts/w4w5_r2.sh` L102-L132 | R2 SB 지점 표: 고정 라벨 `id_valid@sb_*`·evs.send.entry, 레코드 색인 = owner_id − 1114112(0x110000) | drv | R | Y | 1 | G 기준값은 상수 평가, 라벨은 추출 지점 | N | ● |
| RPT-CONSOLE | `w4w5_c1_tables.sh` L71, L84, L152, `env/tools/m1_analyze.py` L26-L28, `env/c1/replay_w6.sh` | CF/BP 콘솔 이벤트(CF 15/20, SAMPLE_APP 1) 요약표 | drv | R | Y | 3 | V | N | ● |
| EXP-TABLES | `plan/expected_results_C1_R2.md`, `expected_results_C5.md`, `expected_judgement_basis_C1_R2.md`, `replay_c5.sh` L81-L82, `c5/20_ops.kzp` L64-L65, `c5/40_rspec.kzp` L17-L18 | 기대표: 어떤 도구도 내용을 읽지 않는다(grep 0). replay_c5.sh는 commit·sha256과 기대 해시를 문자열로 출력만 한다. C5 프로필 주석은 기대표를 보며 형태를 잡았다고 적는다(간접 결합) | exp | R | Y | 3 | V | N | ● |
| ROLE-OPT | `tools/kz/cmd_replay.cpp`, `cmd_predict.cpp`, 드라이버 전부 | `--role` 옵션은 없고 어떤 스크립트도 주지 않는다(0건). 역할은 프로필 role 블록과 site 라벨로만 들어온다 | cli | J | N | 0 | - | - | ● |

## 5. 완전성 단계

### 5.1 찾은 곳 [M][F]

네 조사가 덮지 않은 곳을 직접 찾았다.

| 찾은 곳 | 방법 | 결과 |
|---|---|---|
| 시험(`tests/**`) | 사례 식별자·사례 함수 이름·commit 해시·역할 어휘로 grep. `ctest -N`(병합 트리 빌드, 시험 216)으로 이름 목록 | 사례 프로필이나 사례 기록을 읽는 시험 51(Φ cfs 20, Φ C1 28, static·cfs-free 3). 사례 모양 구성의 단위 시험 6. lint 시험 6 |
| lint 범위 | `tests/lint/kz_lint.cpp` 읽기. `kz_lint case-ids`를 `runtime/`·`tools/`·`probe/`·`instrument/`의 사본(scratchpad, `lib/` 아래 복사)에 실행 | 지금 범위(`include/`, `lib/`)는 clean. 범위 밖 히트 수는 C-05 |
| kz-phi·kz-dump 명령줄 | 두 도구의 머리 주석과 usage 읽기 | kz-dump는 지식 입력 없음. kz-phi의 `--predicate`·`--version`은 E21·E24와 같은 입력 |
| 사람 입력 계수 | `kz-phi stats`를 R2, C1 두 판, C5 두 판 조합에 실행 | C-06 |
| `lib/predict`(엔진 조사 범위 밖) | `model.cpp`에서 역할·narrow·연언항 검사 grep | C-09 |
| `probe/`(엔진 조사 범위 밖) | hold 계획 언어·감독기 머리 주석, 프레임워크 이름 grep | C-10. 지식 입력은 주석뿐 |
| 기록 변환기·앵커 | `kz convert`, `kz anchor` 머리 주석 | C-15. 사례 지식 없음(판독기 어휘는 E12) |
| 런타임 manifest·결합 목록 | `runtime/tools/manifest`, `merge_binds.sh`, `runtime/tests/data` | C-17. 시험 데이터는 일반 이름 |
| 도구가 읽는 문서 | 도구 소스에서 문서 파일을 여는 코드 grep(없음). 문서가 들어오는 길은 인용뿐 | C-14 |
| 조사가 덮지 않은 드라이버 | 사례 라벨·값(`cf_lookup_call`, `BP0_tsem`, `id_valid@`, `0xc4000001`, `1114112`, `--core-request-caller` …)로 `env/`, `instrument/gcc/scripts`, `probe/scripts` grep | C-11, C-12, C-16. 나머지 히트는 이미 행이 있음 |
| 기준 측정 도구 | `env/tools`, `env/measure_baseline.sh` 머리 | C-18(범위 밖 표시) |

찾았지만 행을 더하지 않은 곳:
- `instrument/llvm`·`instrument/clang`의 연결기 원형 코드에는 프레임워크 이름이 없다. 단계 C 비교 스크립트는 `instrument/gcc/sites`의 manifest를 쓰고, 그 manifest는 INS-STAGEC에 있다.
- 변수 이름 `c1`·`c2`가 lint에 걸린 `probe/plan/profile.cpp` L98–L101은 오탐이다.
- `tests/golden/data/records.golden`의 `#72`는 `FACT_EXPRS#72` 참조라 무관하다.

### 5.2 완전성 단계 표

| ID | 위치 | 내용 | 종류 | 영향 | 사례 | 수 | 대체 | 오늘 | 집계 |
|---|---|---|---|---|---|---|---|---|---|
| C-01 | `tests/phi/CMakeLists.txt` L7-L40, `tests/phi/traces`(12), `tests/phi/data`(17) | Φ 시험 20(golden 17 + check + provenance + cli)이 cfs-v7.0.1 프로필과 R2 모양 합성 trace 12(kid_*, startup_*, poll_timeout*, effects_*, cleanup_window, shutdown_scope)를 쓴다. golden에 판정·요약이 고정됨 | test | R | Y | 20 | V 회귀 자료로 재분류. 새 경로 시험은 두 문장 프로필 + 빌드(RV2 §10: golden 일괄 재생성 금지) | N | ● |
| C-02 | `tests/phi/CMakeLists.txt` L41-L90, `tests/phi/traces/c1_*.trace`(3) | C1 프로필 조합 시험 28: check 8, provenance 8(`KZ_C1_PROV_ROOTS`), summary golden 6, canonical golden 2, 합성 C1 trace 재생 4. trace가 CF/BP 시작 순서·BP0_tsem·역할을 적는다 | test | R | Y | 28 | V | N | ● |
| C-03 | `tests/static/CMakeLists.txt` L24, `tests/cfs-free/CMakeLists.txt` L19-L27 | static_golden_core_cpu1(R2 core를 cfs 프로필로 추출: 대상·derives), test_realrecord_derived·test_trace_neutral_real(cfs 프로필 + `env/logs/s1r2_p1.kzt`). core_cpu1.golden L503/525/532는 관측 manifest의 근거로도 인용된다 | test | RO | Y | 3 | V(두 문장 프로필 판 golden을 따로 둔다) | N | ● |
| C-04 | `tests/unit/test_core_phi_check.cpp` L210-L216, `test_core_monitor.cpp`, `test_core_state_order.cpp`, `test_core_phi_parser.cpp`, `tests/predict/test_predict_model.cpp`, `tests/cfs-free/test_rt_plan_convert.cpp` L94-L113 | 사례 모양 구성(existence_probe/dependent_use 역할, retry_until_ready 감시, narrow)의 단위 시험 6. 이름은 합성이지만 rt_plan_convert는 "BP0_tsem"·cf.so·bp.so 문자열 | test | R | M | 6 | D 구성 삭제와 함께 정리(일반 기구 시험은 K) | - | ● |
| C-05 | `tests/lint/kz_lint.cpp` L8-L27, L337, `tests/lint/data/case_identifiers.txt`(25 항목) | 사례 식별자 lint는 include/·lib/만 본다(지금 clean [M]). 사본으로 돌리면 runtime/ 156건 15파일, tools/ 1건(cmd_hold.cpp 주석), probe/ 178건 5파일, instrument/ 2128건 73파일 [M]. "no bare constants" lint는 미구현 | test | R | N | 1 | K 보조로 유지. 주 장치는 입력 출처 추적(RA §2) | - | ● |
| C-06 | `lib/core/phi_stats.cpp` L36-L40, L78-L94, `kz-phi stats` | hand-entered = 가장 강한 인용 등급이 researcher·unreferenced인 항목 → cfs 3, C1 1, C5 1 [M]. code-derived 항목 cfs 89, C1 49/52, C5 25도 사람이 고르고 쓴 주장이다. C1 caller 수준 appreq(D8로 고름)는 "hand-entered 인용 0"으로 나온다 | eng | R | N | 1 | K 재정의: 프로필에 적힌 항목은 모두 사람 입력, 인용 등급은 따로(RV2 §8.1) | - | ● |
| C-07 | `tools/kz-phi/main.cpp` L3-L16 | kz-phi 명령줄: replay `--trace --real-record --predicate --version`, verify-prov `--root`(번들·C1 source view), check/dump/hash/stats | cli | EJR | N | 4 | S(`--predicate`·`--version`은 E21·E24로 대체) | - | ● |
| C-08 | `tools/kz-dump/main.cpp` L1-L11 | kz-dump는 형식만 읽는다(`--check`, `--hash`, `--layout`, `--identity`). 지식 입력 없음 | oth | R | N | 0 | K | A | ● |
| C-09 | `lib/predict/model.cpp` L583-L625, L599-L607, L719 | 예측 검사 규칙: use는 연언항 하나(`state(kind(arg.P)) = S`), 프로필 역할 지점이어야 함, 인스턴스 = 역할 purpose dependent_use, 결과마다 narrow 필요, exit는 프로필 terminal op | eng | CJ | M | 5 | G 예측 입력을 추출 사실로. 못 하면 D | N | ● |
| C-10 | `probe/hold/hold_plan.hpp` L12-L50, `supervisor.hpp` L10-L29, `supervisor.cpp` L434 | hold 계획 언어와 감독기: 조건 `name <slot-role> "<text>"`·`caller-in <binary>`·occurrence, 하드웨어 실행 breakpoint(주소 수 한계). 주석이 cf.so/bp.so를 예로 든다 | eng | OE | N | 1 | K | - | ● |
| C-11 | `kairos/probe/scripts/observe_cfs_smoke.sh` L238-L244 | R2 smoke 드라이버의 kz hits 질의: `--site evs.send.entry --before sb.appid.store --group-slot app_id/event_id --group-return-slot status --group-caller` | drv | R | Y | 3 | V | N | ● |
| C-12 | `env/campaigns/w5_20261028.sh`, `w6_20261029.sh` L62-L70 | 캠페인 드라이버: post → hold-cf833fdbb.kzh + "P-C1HOLD", pre → hold-cf281a941.kzh + "researcher intervention". 실행 순서·디스크 하한은 unspecified_by_reference | drv | HRO | Y | 2 | G 계획 자동 생성·모든 후보 실현. 순서는 S | N | ● |
| C-13 | `tests/phi/CMakeLists.txt` L8, L49, `kz predict --prov-root`, `docs/c1_smoke_observe.md` | 출처 검증의 뿌리: `KZ_BUNDLE_ROOT`, `KZ_C1_PROV_ROOTS`(C1 빌드마다 따로 만든 source view), predict_w6.sh의 `--prov-root` | oth | R | M | 3 | U 분석 대상 트리 그대로 | P | ● |
| C-14 | `cfs/observe/sites.kzp:69`(root=spec), `c1/01_prov.kzp:175, :185`, `cfs/01_prov.kzp:13`, 헤더 @retval 인용, VDD PDF | 도구가 읽는 문서: 인용으로만 읽고(verify-prov의 해시·식별자, predict `--prov-root`) 뜻은 해석하지 않는다. 문안을 caller 요구로 묶은 것은 사람(APQ-*, PRV-c1-cfdox) | oth | JR | M | 6 | V 문서는 평가 자료. @retval 후보는 G | N | ● |
| C-15 | `tools/kz/cmd_convert.cpp` L3-L20, `cmd_anchor.cpp` L3-L9 | 변환·앵커 입력: kz convert `--process PID`(harness 기록), `--hold`, `--manifest`·`--basis`. kz anchor `--root NAME=DIR`·`--source-root`·`--bin`. 판독은 manifest의 판독기를 따른다 | cli | OI | N | 6 | K | A | ● |
| C-16 | `instrument/gcc/scripts/compare_c1.sh` L1-L84, `compare_r2.sh` L89-L95 | F0–GCC 비교 스크립트의 사이트 번호 대응표(cf_lookup_call, bp_create_call, `id_valid@sb_*`, evs.send …)를 손으로 적음 | drv | R | Y | 2 | V 관측층 비교(RQ4) 자료 | N | ● |
| C-17 | `runtime/tools/manifest/kz_rt_manifest.cpp`, `instrument/gcc/scripts/merge_binds.sh` | 계측 결합 목록(binds.kzs)과 manifest 컴파일: 플러그인이 logical manifest에서 기계적으로 만든다. 지식 입력 없음(판독기 어휘는 E12) | oth | O | N | 2 | K | A | ● |
| C-18 | `env/tools/cfs_layout.py`, `cfs_snapshot.py`, `sb_pipeinfo_parse.py`, `env/measure_baseline.sh` | 범위 밖 표시: Kairos 이전 기준 측정(M1)의 손 표 목록(SB PipeTbl, OSAL 표, ES/EVS 전역, HS). Kairos 입력이 아님 | oth | R | Y | 1 | V | N | ● |

### 5.3 읽기 [I]

- **시험이 사례 판정을 고정한다.** 216 시험 가운데 51이 사례 프로필이나 사례 기록을 읽는다. 그 golden에는 판정·요약·발생 수가 들어 있다. 판정층을 빼면 이 시험이 바뀌는 것은 정상이다. RV2 §10(RA §3.3 인용)에 따라 golden을 일괄 재생성해 통과시키지 않는다. 새 경로의 시험은 두 문장 프로필과 빌드만으로 따로 만든다.
- **lint는 출처 추적을 대신하지 못한다.**
  - 범위 밖 히트의 대부분은 스크립트·manifest(사례 자료가 원래 있는 곳), 주석, 런타임의 OSAL 호출 구현이다.
  - 엔진 판정 코드에 새 사례 이름이 들어온 곳은 찾지 못했다.
  - 그러나 이 lint는 E01–E29 같은 "이름 없는 사례 지식"을 원리상 잡지 못한다. RA §2의 결정대로 입력 출처 추적이 주 장치다. lint는 보조로 둔다.
- **kz-phi stats의 hand-entered 수를 자동화 지표로 쓰면 안 된다.** 이 수는 "인용 없음 또는 researcher 인용"만 센다. 출처 추적의 계층(ED §3.0: B/P0/L0/R/O)으로 다시 세야 한다.

## 6. 재설계에서의 대체

### 6.1 분류의 정의 [D]

| 분류 | 뜻 | 근거 |
|---|---|---|
| G 일반 추출 | 사용자 입력(빌드·기록)에 사례 무관 규칙을 적용해 만든다. 추출 경로 A(등록 표의 레코드·필드·키), B(공유 필드 → 호출 인자), C(생성·등록·소유), 새 사실 F1–F7, 대상 규칙 R-T1/R-T2, 관측점 규칙 OP-*, RA §3.1 반사실 결과 뒤집기 신호, 후속 처리 분류 | RA §0 항목 1, §3.1, §4. ED §3.1–§3.5, §4 |
| T 도구 내부 API 의미 | 스레드·C 라이브러리 기본 연산(P0)과 cFE 적재기(L0), OSAL·cFE 기본 연산의 의미. 판 표기와 원문 대조 줄을 붙이고 공개한다. 대조하지 못한 항목은 그렇게 표시한다 | RA §3.3(RV2 §4.2, §8.2). ED §3.0, §3.6 항목 6, §5 R4 |
| Q 기록 관측 사실 | 실행 기록에서 기계적으로 읽는 사실(최종 영향, 반환 코드, 같은 키 반복, 입력 사건) | RA §2(RV2 §5.5) |
| V 평가 전용 | 탐지 입력에서 빼고 정답 대조·평가에만 쓴다(이슈 글, 수정 설명, 기대표, 사례 재현 방법) | RP §3.7, RA §3.3(RV2 §8.3) |
| D 삭제 | 대체 없이 없앤다. 판정 근거 자리를 비우는 경우(rspec·appreq·대안 allowed)는 RV2 §5.5의 독립 보고 항목이 그 자리를 갖는다. 죽은 입력·중복·결합도 여기 | RA §2 |
| U 사용자 입력 | RV2가 허용하는 입력: 소스, 빌드 명령·compile DB·link 정보, 바이너리·모듈·시작 스크립트·설정, 실행 입력 | RV2 §4.1(ED §3.0 계층 B가 인용) |
| K 일반 기구 유지 | 사례 지식이 없는 엔진 기구·한도·규칙. 공개만 하거나 일반 규칙으로 고친다 | — |
| S 실행 설정 | 도구 매개변수·읽기 스위치·harness 값. 실행마다 기록하고, 레퍼런스가 정하지 않으면 unspecified_by_reference로 라벨한다 | `CONDITIONS_POLICY.md` |
| O 열림 | 지금 정할 수 없다. 대부분 단계 1의 독립 생성 측 토큰이 정해져야 한다 | RA §2 넷째 줄, §3.2 |

### 6.2 층별 대체 [D]

| 지금의 층 | 대체 | 행 |
|---|---|---|
| rspec(use·key·requires) | D. 어떤 호출이 의존 사용인지는 RA §3.1 신호가 정한다. 사용 u의 결과 분기를 정하는 상태 S(u)와, 다른 행위자의 쓰기 W(u)가 먼저였으면 분기가 바뀌는지를 본다. requires 자리는 callee 자신의 검사(F1/F2)다 | RSP-* |
| appreq | D. 문안은 평가 자료다. "AppId가 등록된 앱이어야 함"(`cfe_evs.h`) 같은 문안을 caller 요구로 묶는 일은 RA가 거부한 사람 판단이다 | APQ-*, C-14 |
| allowed | 실제 처리가 있는 것은 G(처리 분류: 재시도·종료·버림·probe-create, RV2 §5.6 보수 규칙). 대안 읽기는 D | ALW-* |
| harm | Q. 최종 영향은 기록 사실로 따로 보고 | HRM-*, E16, E17 |
| 역할 | G. CF의 의존 조회는 RA §3.1 후보다. BP의 존재 확인은 같은 행위자의 프로그램 순서로 반대 순서가 불가능해 후보가 아니다(E3). C5 SB는 결과 사용 사실로 | ROL-*, E05, OBS-C1-BP-ROLE |
| 자원 종류·bind·loc·init | G 경로 A/B/C. 키 필드·활성 필드·값 필드는 성공 반환 guard의 분해로 얻는다(ED §3.1) | KND-*, BND-*, LOC-sb, INI-sb, RES-*-02 |
| op·결과·narrow | G. 결과 코드와 그 분기는 F2(RETURNS), 역할 인자는 F4(ARGFLOW)에서 얻는다 | OP-*, NAR, DOC-outcomes |
| hb 규칙 | 판 일반 동기화 의미(spawn, OSAL reads-from)는 T. R18·R19 같은 cFE 시작 순서는 G(table walk·poll 루프 사실). 판 guard는 루프 존재로 대체 | HB-*, RES-cfs-02/03 |
| trusted | A-FRESH는 K(공개 가정, 기록으로 반박). A-OWNER는 O(단계 1) | TRU-* |
| 인용·연구자 노트 | 사실 위치(G)로 대체. 이슈 기록은 V | PRV-*, RES-*, ISS-* |
| 상수 | G. TU 안 상수 평가. 손 리터럴과 `--const`는 없앤다 | CON-*, E22, INS-bound20 |
| 관측·계측 manifest | G. 관측점 규칙(OP-E/S/L/T/A/B/C)으로 만든다. 사례 선택 이유가 필요 없었다는 주장은 단계 3의 완료 기준(새 호출자를 수작업 site 없이 관측)으로 따로 확인한다(ED §4.3). 정체성 판독은 단계 1 | OBS-*, INS-* |
| hold 계획 | G. 추출된 후보 쌍에서 계획을 만들고, RV2 §3.8의 일곱 결과로 기록한다. harness 제한시간은 S | HLD-*, HOLD-*, INS-c1-control |
| 예측 모델 | G. cone을 정적 사실과 기록 접두사에서 만든다. 못 하면 `kz predict` 경로를 뺀다(RP §3.6) | PRD-*, PRED-OPT, E29, C-09 |
| 드라이버의 사례 프로필 선택 | D. 남는 것은 P0/L0 판과 추출 규칙 판 | DRV-PROFILE, KS-P09 |
| 명령줄 연산 이름(G0) | T. P0 씨앗과 호출 그래프 전파(ED §3.0, F7) | KS-C02–C05 |
| 판정 근거 정책·상태 사상 | D. 정책 은퇴 | E01, E02 |
| 보고 스크립트 | 일반 질의(Q·G)로 바꾸거나 평가 자료(V)로 | RPT-*, C-11, C-16 |
| 시험 | 사례 golden은 V(회귀 자료). 새 경로 시험은 따로 | C-01–C-04 |

### 6.3 열린 항목 [U]

| # | 항목 | 행 | 왜 열려 있나 | 다음 |
|---|---|---|---|---|
| 1 | owner 대응(A-OWNER), 알린 id로 묶기 | RES-cfs-09, TRU-owner, E13, OBS-cfs-identity, INS-C5-ID, RPT-C5TAB | 생성 요청과 자식 진입을 잇는 독립 관측 경로가 아직 없다. 자기 ID 반환값을 owner의 유일한 근거로 쓰지 않는다(RA §2) | 단계 1 |
| 2 | C1 CF 쪽 키(표 파일) | CON-semnames | 키가 실행 중 `CFE_TBL_Load`로 읽는 표 원본에서 온다. cFE 표 서비스 의미(L-TBL)와 빌드의 표 대응이 없으면 "키 미해결, 구획 일치"로 남는다(ED §3.6 항목 1) | L-TBL을 T로 둘지 결정 |
| 3 | 레코드 사이 관계 guard | RES-cfs-10, OP-cfs-retire | 추상 영역이 배제하지 못하는 관계 조건이다. writer guard는 G로 나오지만 관계 부분은 공개 가정으로 남는다 | 공개 가정 목록에 둠 |
| 4 | hold harness 제한시간, 예측 상태 상한 | HOLD-RUN, PRED-OPT | 레퍼런스가 정하지 않는다 | unspecified_by_reference로 기록, 사건 기반 제어로 대체 검토(CONDITIONS_POLICY) |
| 5 | C5 잘못된 AppId의 성격 | INS-c5, E13 | order violation인지 `pthread_t` 재사용에 기대는 파생 구성의 산물인지 [U](RA E2) | 정체성·수명 자료로 따로 보고. #72 수에 넣지 않음 |
| 6 | 값 평가(R2의 "0은 검사 실패") | OP-cfs-send | 본문 있는 함수의 상수 접기가 필요하다. 안 되면 "평가 불가"로 두고 관측 반환값을 쓴다(ED §3.6 항목 2) | F2 구현 때 |
| 7 | 판정층과 facts의 해시 결합 | KS-P09 | facts 정체성을 추출 규칙 판 + P0/L0 판으로 바꾸려면 엔진 변경과 PI 결정이 필요하다(ED §5 R8) | PI 결정 |

## 7. 오늘 자동으로 나오는 것과 아닌 것

"자동"은 사람의 의미 입력 없이 사용자 입력(빌드·기록)만으로 나온다는 뜻이다. 사례 무관이지만 사람이 쓴 이름·규칙은 자동으로 세지 않는다.

### 7.1 자동 (사람의 의미 입력 없음) [M]

- **kz-static V0 기초 사실**(두 문장 프로필, 명령줄 어휘 없음; KS-N01, ED §2.1).
  - FUNCS.
  - 호출 지점과 인자 종류: 상수·문자열·필드 읽기 `symbolic T.f`·필드 주소·함수·top. 수는 C1 tx 7908, R2 core 5948, C5 6.4.1 3412.
  - 결과 used/tested/discarded 플래그.
  - 루프 종류(table-walk/constant-trip/other)와 상수 횟수.
  - table walk 펼치기와 static image(R2만).
  - FACT_CONSTS, 간접 호출 대상(CALL_TARGETS).
  - 함수마다 필드 첫 쓰기(FUNC_WRITES, 값·guard 없음).
  - 이 사실들은 프로필 유무와 무관하게 행 번호만 빼면 같다.
- **사례 지점에서 이미 나오는 것**(ED §2.4).
  - R2: `CFE_SB_Global_t.AppId`를 복사 없이 `CFE_EVS_SendEventWithAppID`의 인자로 넘기는 39곳(34/1/4)과, 그 39곳 모두 결과를 버린다는 사실.
  - R2: EarlyInit 순서(table walk).
  - C1: count-semaphore 호출 4곳과 그 인자 원천, OSAL 이름 표의 검색 루프와 콜백 대상, `name_entry`·`active_id`를 쓰는 함수.
  - C5: `CFE_ES_AppCreate`의 잠금·생성·저장 호출 순서와 줄 번호.
- **빌드 정체성**(KS-B01). compile DB·link·바이너리의 짝, TU 목록, 파일 해시, `ref eval` 상수의 TU 안 평가. 다만 재생은 이 평가값을 쓰지 않고 `--const`로 다시 받는다(E22).
- **입력이 주어진 뒤 기계적으로 도는 단계**(C-15, C-17).
  - manifest가 있으면 앵커 오프셋, 관측 기록, 변환, 결합 목록이 나온다.
  - 인용이 있으면 verify-prov 해시 대조를 한다.
  - 모델이 있으면 예측 탐색과 계획 생성을 한다.
  - 계획이 있으면 `kz rtplan` 변환을 한다.
  - 이 단계들은 의미를 만들지 않고, 사람이 준 입력을 옮길 뿐이다.

### 7.2 사례 무관이지만 사람이 준 이름·규칙에 기대는 것 [M]

- **G0 이름 9개**가 있으면 다음이 나온다(ED §1.2, §2.2).
  - 루프의 poll/count-bounded 분류와 탈출식·mod set.
  - terminal 사실, spawn 사실·task 순서·spawn 매개변수, 잠금 구간.
  - 이름은 spawn `OS_TaskCreate`·`CFE_ES_CreateChildTask`, delay `OS_TaskDelay`, lock/unlock `CFE_ES_LockSharedData`/`CFE_ES_UnlockSharedData`, terminal `CFE_ES_ExitApp`·`CFE_PSP_Restart`·`CFE_PSP_Panic`·`abort`다.
  - C1 수정 후 재시도 루프(`cf_cfdp.c:1001`, count-bounded, 상한 25, 탈출 `ret != -17`, 지연 있음)는 이 단계에서 나온다.
  - 사례 드라이버는 이 가운데 4개를 명령줄로 준다(KS-C02–C04). G0은 P0 씨앗과 래퍼 유도로 대체할 대상이다.
- **L0 적재기 의미**(KS-R05, KS-R06)와 **데이터 함수 표 12**(KS-H01). 사례 무관이지만 사람이 쓴 것이다. 판 표기와 원문 대조가 아직 일부만 있다.
- **도구 규칙으로 고른 대상 R-T1**(V3). writer·LOCS·A-TYPE·INIT·OUTCALLS를 사람 선택 없이 낸다(C1 tx writer 3194, R2 core 2064, C5 1342).
  - R-T1에 든 사례 필드는 writer 수가 사례 대상(V2)과 같다.
  - 분기 조건에서만 읽히는 사례 필드 6개는 R-T1에 들지 않는다. C5 `RecordUsed`·`TaskId`, R2 `AppState`·`Type`·`ShutdownReq` 등이다.
  - 이것은 연구 드라이버(`env/redesign/extract_baseline.sh`)에서만 돈다. 판정 경로에는 이어져 있지 않다(KS-P09).
- **관측점 규칙 원형 OP-***(ED §4). 수작업 site를 함수 수준에서 C1 29/37(30/38), R2 34/44, C5 26/33 덮는다. 수는 한두 자릿수 많다. 역시 연구 드라이버에서만 돈다.

### 7.3 사람의 의미 입력 (자동 아님)

§4의 오늘 열이 N인 행 195개와 P인 행 33개다. 묶으면 다음과 같다.

- **판정층** 전부: rspec 7, appreq 7, allowed 8, harm 7, 역할 3(§4.1).
- **모델층**: 자원 종류 10, 문맥 2, 범위 2, loc·init 각 1, bind 9, Φ site 13, op 45(연언항 6, effect 42, outcome 32, narrow 10), hb 9, trusted 2, declared_trusted effect 1(§4.3).
- **연구자 노트** 22와 **이슈 기록** 3. 인용 선택(인용 기록 313, 해시는 자동 대조)(§4.2).
- **관측·계측 manifest**: F0 R2 40, C1 33/34. kzs R2 44, C1 37/38, C5 33/31. 단계 C 6. 손 배치 사실(오프셋 16, 인자 위치). 계측 모듈 선택(§4.6).
- **hold 계획** 2와 그 control 줄 10. 예측 계획 28은 생성되지만 입력이 손 모델이다(§4.7). **예측 모델** 3(§4.8).
- **명령줄 값**:
  - 대상 7(앱 필드 4), 연산 4, `--const` 2, `--core-request-caller` 1(R2만).
  - 프로필 디렉터리 선택(rspec 판 포함)과 읽기 스위치(§4.4, §4.5, §4.9).
- **recipe의 module 목록**(C1 7, C5 5, R2 0), 생성 파일·표·declared claim(KS-R02, KS-R07).
- **엔진 안의 판 의미**: ID 공식(E11), 종료 범위(E07), 판독기 어휘(E12), hb 템플릿(E20), 관계·harm 어휘(E08, E16, E17), early-init 이름(KS-H05).
- **보고 스크립트**의 사례 라벨·값(RPT-*, C-11, C-16).

### 7.4 지금 사실만으로 사례 입력 하나를 대신할 수 있는 곳 [M][I]

| 사례 입력 | 지금 사실 | 남는 공백 |
|---|---|---|
| ALW-c1-833의 재시도 상한(CON-retry) | V1 루프 사실: count-bounded, 상한 25, 탈출식 `ret != -17`, 지연 호출 | 지연 연산 이름 하나(G0, P0로 대체 예정). 탈출 상수가 그 호출의 실패 코드라는 대응은 F2 |
| LOC-sb(R2 생산자 필드)의 인자 쪽 | V0 FACT_ARGS의 `symbolic CFE_SB_Global_t.AppId` 39곳 | 생산자 writer(EarlyInit memset, AppInit out-pointer)는 R-T1(V3) 대상이어야 나온다. 그 경우 상태가 DISABLED로 바뀐다(ED §2.3) |
| KS-C01의 앱 필드 4 | R-T1이 writer 수를 같게 낸다 | `CF_ChannelConfig_t.sem_name`은 DISABLED, CF 키는 표 파일 |
| BND-cfs·BND-c1 일부(writer 집합) | R-T1 writer 집합 | 분기에서만 읽히는 필드는 R-T2(F1/F2 필요) |
| SIT-c1의 호출 지점 | V0 호출 사실(`bp_flow.c:126/132`, `cf_cfdp.c:993`) | 어느 짝이 같은 구획·키인지는 경로 A(F1, F4, F6) |
| PRD-coretx의 R2 판 코어 순서 | table walk 순서(R2만) | C1·C5 cFE의 코어 생성 순서는 table walk 꼴이 아니라 추출 안 됨(KS-H04) |

이 밖의 사례 행은 F1–F7과 사례 무관 질의기(ED §6의 `kz-static relations`)가 생겨야 대체된다. 그 공백(G1–G11)은 ED §2.5에 있다. 필드 읽기, 반환값, 모든 루프의 탈출식, 역할 없는 인자 흐름, 잠금 집합, 다단계 spawn 래퍼, table walk 밖 초기 이미지, 시작 스크립트 진입 해결, 본문 없는 함수 요약, 완전성 판정의 비단조다. 모두 입력 문제가 아니라 엔진 확장 과제다(KS-N02).

## 8. 불일치·주의 사항 [F][M]

| # | 내용 | 행 |
|---|---|---|
| 1 | CFE_ES_APPID_BASE가 ref eval이 아닌 손 계산 리터럴(0x110000, SIMPLE 모드). STRICT 빌드면 틀린다 | CON-appidbase |
| 2 | OS_MAX_API_NAME은 Φ에서는 ref eval인데, 재생은 `--const`로 받고 instr manifest 12 값은 `bound=20`을 하드코딩 | CON-refeval, E22, INS-bound20 |
| 3 | C5 source rev 자리가 tarball 해시 앞부분(commit 모양)이라 verify-prov가 unverifiable | SRC-c5 |
| 4 | es.shutdown(cfs)과 psp.shutdown(C1)이 같은 PSP 모델의 두 이름이고, context 블록도 중복 | KND-cfs-shutdown, KND-c1-pspshutdown, CTX |
| 5 | `rspec-281a941/40_rspec.kzp` 머리 주석 L4–L10은 D8 이전 읽기(NOT_READY_UNJUDGED)를 적고, L11 이후가 D8을 적음 [F: 이번에 확인] | RSP-c1-281 |
| 6 | C5 instr manifest 머리 "DRAFT: not used by any run"(L4)이 10-29 빌드·부팅·재생의 실제 사용과 어긋남 [F: 이번에 확인] | INS-c5 |
| 7 | 프로필의 `out` 역할과 hb `poll`·`--poll-fn`은 kz-static에 소비처가 없는 죽은 입력. `--poll-fn`은 그래도 hand-entered로 센다 | KS-P05, KS-P08 |
| 8 | R2 await(`CFE_ES_MainTaskSyncDelay`) 인정이 대상 목록에 비단조로 의존(V1 undecided, V2 await, V3 fails) | KS-H07 |
| 9 | 대상 형·필드의 존재를 확인하지 않아 없는 대상이 writer-set-complete가 될 수 있음 [I] | KS-H10 |
| 10 | extract의 `--extra-arg`·`--resource-dir`가 facts에 기록되지 않음. 명령줄 `--target`은 BASIS에 두 번 | KS-C06, KS-P10 |
| 11 | `--core-request-caller`를 R2에만 준다. 같은 질문(코어 서비스인가)에 사례마다 다른 입력 | E09 |
| 12 | `predicate from_code` 규칙(R19)은 `--predicate` 없이는 간선을 안 만든다. 드라이버는 하나도 주지 않는다 | E21 |
| 13 | looper yes(R1)는 handler 표시 effect가 없어 효과가 없다 | HB-cfs-R1, HB-c1-R1 |
| 14 | R19의 판 guard cfe ≥ 6.4.2는 SPEC 값이고 6.4.x 소스로 다시 확인하지 않음 | RES-cfs-02, HB-cfs-R19 |
| 15 | 사례 식별자 lint의 범위가 `include/`·`lib/`뿐이고 "no bare constants" lint는 미구현 | C-05 |
| 16 | `kz-phi stats`의 hand-entered가 사람 입력을 과소 계수(C1 caller 수준 appreq를 "hand-entered 인용 0"으로 표시) | C-06 |
| 17 | C5 결과표의 자기 spawn 결합이 pthread_t 같음에 기댄다. E2의 재사용 위험이 스크립트에도 있다 | RPT-C5TAB |
| 18 | R2 F0 manifest의 조사 범주 합(49)이 site 수(40)보다 크다(범주 겹침). 항목 수의 합은 분량 표시일 뿐 | OBS-cfs-* |

## 9. 조건 기록과 재현

- 이 감사는 실험 조건을 정하지 않는다. 연구 환경·변수를 바꾸지 않았다(`CONDITIONS_POLICY.md`).
- 이번에 실행한 것은 다음뿐이다. 모두 읽기 전용이다.
  - `kz-phi stats`: 병합 트리 빌드 `/home/user/work/merge-check/kz-llvm/tools/kz-phi/kz-phi`. R2 프로필, C1 `281a941`·`833fdbb`(번들 `1df262a`, 현재 rspec), C5 `cfe-6.4.1`·`cfe-6.4.2` 조합.
  - `kz_lint case-ids`: 같은 빌드의 `tests/lint/kz_lint`. 저장소 범위와, scratchpad에 복사한 `runtime/`·`tools/`·`probe/`·`instrument/`에 대해.
  - `ctest -N`(같은 빌드): 시험 이름 목록, 216.
  - grep과 파일 읽기.
- §2·§3의 표와 §0의 수는 §4·§5 표의 행(ID, 종류, 영향, 사례, 수, 대체, 오늘, 집계 열)에서 기계적으로 센 것이다. §4·§5 표가 곧 행 목록이므로 같은 표에서 다시 셀 수 있다. 세는 데 쓴 스크립트는 저장소에 넣지 않았다.
- 이 문서의 분류(사례 여부의 조정, 대체 분류, 오늘 열)는 [D]다. 레퍼런스가 정하는 값이 아니다. 대체 분류가 RA·ED의 어느 절에 기대는지는 §6.1에 적었다.
