# C1·R2 기대 결과표 — W4 실행 전 동결 (2026-10-24)

- 성격: **출처에서 도출한 기대값만** 적는다. 실행·관측 결과는 없다. 이 표를 만들면서 빌드·실행은 하지 않았다.
- 동결: 이 파일은 W4의 어떤 실행(파일럿 포함)보다 먼저 커밋한다. 근거: 마스터플랜(2026-10-24) §12.1·§12.4·W0, 결정 P-F1GATE("기대 결과표는 해당 사례의 첫 출력 전에, 평가 엔진·규칙은 본 평가 전에 동결한다", `Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md` §3.7) [D].
- 바꾸는 규칙: 동결 뒤에 행을 바꾸면 §5에 날짜, 이유, 이전 값을 적는다. 결과를 본 뒤에 고친 행이 있으면 그 실행은 개발 자료로 옮기고 새 평가를 한다(마스터플랜 §12.4).
- 표기: [F] 고정 커밋의 소스·문서에서 읽은 사실, [I] 추론, [D] 결정, [U] 미확인. 값을 정하는 레퍼런스가 없으면 `unspecified_by_reference`로 두고 고르지 않는다(`CONDITIONS_POLICY.md`).
- 결과 단위: **고정 구성 × 자원(키) × 호출 문맥 × 세대 × 발생 위치**(마스터플랜 §12.1). 다른 판의 호출을 같은 단위로 합치지 않는다.
- 판정 어휘: SPEC rev 1.3의 발생 상태(§1.6)와 요약 뷰(§1.10 "Summary view", ADR-054). `INCONCLUSIVE`의 이유는 §4.8 "Reasons for withholding"의 이름만 쓴다.
- 기대값을 출처에서 도출할 수 없으면 "미정"으로 적고 이유를 붙인다.

## 1. 고정 구성

| ID | 구성 | 출처 | 아직 고정되지 않은 것 |
|---|---|---|---|
| K-C1-pre | CF `281a94188cd7d885a5aed01ee041f3ece2b0486f` + nasa/bp `b051eb0d12bfe76c0230215c2ff851cfaa553ce4`(D22 공개 BP 기준). 이름 `BP0_tsem`·`BP1_tsem`, 시작 스크립트 CF(L6) 다음 BP(L7), 설정 우선순위 CF 65·BP 55 | `C1_basis_plan.md` §2·§3; SPEC App. A.4 step 2 [D] | cFE·OSAL·PSP·bplib 조합(Q-4, Q-5), 인스턴스 수(Q-3), 실시간 우선순위 적용, CPU 수·affinity: `unspecified_by_reference`. W3 빌드 manifest가 정한다 |
| K-C1-post | CF `833fdbb27a26f15f2429e79392e7eb73d61abdea` + 같은 BP | 같음 | 같음 |
| K-C1-neg | ENV cFS v7.0.1, CF `15a871e`, 기본 표(이름 둘 다 빈 문자열) | SPEC App. A.4 step 1 [D]: `cf_cfdp.c:1270`, `cf_def_config.c:60, 78` | — |
| K-R2 | ENV cFS v7.0.1 `088b2fa8`(cFE `c5fb2b4d`), 기본 기동, S1-R2의 8개 지점 계획(`kairos/profiles/cfs-v7.0.1/observe/sites.kzp`) | SPEC App. A.1 머리 주석 [D/I]; `proof_status_R2.md` §5 | 새 고정 실행 묶음(W2)의 관측기·커널·프로필·환경 기록. 과거 개발 기록과 섞지 않는다 |

**C1 행의 적용 조건.** C1 행은 W3에서 실제로 빌드한 구성이 위 K-C1-pre/post와 같을 때만 쓴다. W3가 다른 판으로 옮긴 구성(ENV 이식 등, `C1_basis_plan.md` §8)이면 W4 실행 전에 행마다 다시 확인하고 §5에 개정으로 남긴다. 2022년 #184 댓글의 구성(BP를 CF보다 먼저 시작, 버전 미상)은 이 표의 어느 구성도 아니다(`cases/C1.md` §0a).

## 2. 결과 단위

정체성은 네 범주를 따로 적는다: **실행 task / 생성 실행자 / 소유(생성) 앱 / 전달 AppID**(SPEC rev 1.3 보충 2026-10-24, §1.2). 서로 같다고 가정하지 않는다. 앱 귀속의 근거 수준(`confirmed`·`inferred`·`undetermined`)은 관측 뒤에 정해지므로 여기서는 쓸 근거만 적는다(SPEC §5.7, ADR-056).

| 단위 | 구성 | 자원(키)·세대 | 실행 task | 생성 실행자 | 소유(생성) 앱 | 전달 AppID | 발생 위치 [F] | 요구·허용 처리와 출처 | 자료 성격 |
|---|---|---|---|---|---|---|---|---|---|
| U-C1-1 | K-C1-pre | `osal.countsem("BP0_tsem")`, 한 부팅의 첫 생성 세대 | CF 주 task(CF 앱 세대 0) | ES의 앱 생성 경로(시작 스크립트 L6의 생성 요청) | CF(근거: 생성 요청과 생성 호출 안의 fork 기록) | 없음(이름 전달) | `cf_cfdp.c` L993 @`281a941`, 채널 0 | `Req`: 조회 LP에서 객체 존재(`C1_basis_plan.md` §1.2). 역할 출처: `cfs_cf.dox` L208–L214, #184 본문. caller 수준 Allowed: `unspecified_by_reference`(수정 전 처리의 `caller` 기록 없음; Q-12) | 자연 실행. 보류 실행은 `held-F0` |
| U-C1-2 | K-C1-pre | `osal.countsem("BP1_tsem")`, 같음 | 같음 | 같음 | 같음 | 없음 | 같은 줄, 채널 1 | 같음 | 같음 |
| U-C1-3 | K-C1-post | `osal.countsem("BP0_tsem")`, 같음 | CF 주 task | 같음 | CF | 없음 | `cf_cfdp.c` L1003 @`833fdbb`, 채널 0, 시도 k = 1 … 25 | `Req` 같음. Allowed: `retry_until_ready(OS_ERR_NAME_NOT_FOUND, 25)`, `caller` 기록 = 인정 주석 L993–L999(G-REBIND 충족 2026-10-24, `C1_basis_plan.md` §9.8), 상수 `cf_platform_cfg.h` L338(25)·L349(100 ms) | 같음 |
| U-C1-4 | K-C1-post | `osal.countsem("BP1_tsem")`, 같음 | 같음 | 같음 | 같음 | 없음 | 같은 줄, 채널 1 | 같음 | 같음 |
| U-C1-5 | K-C1-pre·post | 같은 두 키 | BP 주 task(BP 앱 세대 0) | ES의 앱 생성 경로(스크립트 L7) | BP | 없음 | `bp_flow.c` L126 @`b051eb0` | **요구 없음.** 역할: 생성 전 존재 확인. 출처: `bp_flow.c` L121–L144, `bp_semcfg.c` L33–L36(`C1_basis_plan.md` §9.7 (b)) | 자연 실행 |
| U-C1-6 | K-C1-neg | 이름 없음 | CF 주 task | ES의 앱 생성 경로 | CF | 없음 | `cf_cfdp.c:1270`의 빈 이름 검사(조회 호출 없음) | 경로 비활성 | 음성 대조 |
| U-R2-1 | K-R2 | `evs.reg(arg.AppID)`와 값 `valid(arg.AppID)`, 부팅 1회 | ES 주 task(`CFE_ES_TaskInit` 문맥) | ES 시작 스레드(`CFE_ES_CreateObjects`의 core 객체 루프, SPEC §3.3 "Creation events used for `app_owner`") | ES(SPEC App. A.1 기대: `inferred`, `match-by-time`) | `CFE_SB_Global.AppId`(SB의 값; 저장 전에는 EarlyInit memset 값, `cfe_sb_init.c:50`) | SB API의 이벤트 지점이 부르는 `CFE_EVS_SendEventWithAppID`(지점 목록은 census가 정한다, SPEC App. A.1 step 1) → 검사 `cfe_evs.c:185–189` | `K-ID.send-with-appid`: `appreq` = `cfe_evs.h:169–172`(caller 쪽), Allowed `none`, harm `event_request_rejected`(`proof_status_R2.md` §5) | 자연 실행(관측) |
| U-R2-2 | K-R2 | 같음 | EVS 주 task(`CFE_EVS_TaskInit` 문맥) | 같음 | EVS(같음) | 같음 | 같음 | 같음 | 같음 |
| U-R2-3 | K-R2 | 같음 | SB 저장 뒤 같은 지점을 지나는 task | 같음 | 실행 task의 앱 | 저장된 SB AppId(`cfe_sb_task.c:138` `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`) | 같음 | 같음 | 같음 |

## 3. 기대 결과

열 설명. "관측 판정"은 발생 상태 → 요약 값이다. 그 뒤의 네 열은 따로 센다: **확정 위반**(VIOLATING이고 모든 설명에서 성립 → `OBSERVED_VIOLATION`), **의존 미충족이나 허용 판단 불가**(NOT_READY_UNJUDGED → `INCONCLUSIVE(caller_requirement_absent)`), **프로그램 오류 반환·종료**(API 반환 코드, 이벤트, 앱 종료. 판정과 따로 기록), **관측 부족**(필요한 사실이 없을 때의 결과). 한 열의 값이 다른 열로 옮겨 가지 않는다. 오류 반환이나 종료가 있다고 위반을 확정하지 않고, 피해가 없다고 위반을 취소하지 않는다(마스터플랜 §12.1).

| 행 | 단위 | 구간 | 관측 판정(상태 → 요약) | 확정 위반 | 의존 미충족·허용 판단 불가 | 프로그램 오류 반환·종료 | 관측 부족일 때 | 예측·확인 | 필요한 관측 | 근거 / 미정 이유 |
|---|---|---|---|---|---|---|---|---|---|---|
| E-01 | U-C1-1, U-C1-2 | A1 첫 조회 성공 | (CF, 1) READY → `NO_VIOLATION_IN_CHECKED_PREFIX` | 없음 | 없음 | 없음. 이벤트 20 "CF Initialized" | 같은 id의 성공이나 생성 세대를 잇지 못하면 `INCONCLUSIVE(resource_or_lifetime_unmatched)` | 예측 W5, 확인 해당 없음 | C-1, C-2(반환·id), P-3의 id(같은 세대) | SPEC App. A.4 step 6; `C1_basis_plan.md` §9.3. 이 구간이 자연 실행에서 나오는지와 빈도는 미정(관측으로만 안다) |
| E-02 | U-C1-1, U-C1-2 | A2 재시도 성공 | 해당 없음 | — | — | — | — | — | — | 단일 조회 판(`cf_cfdp.c` L993–L1001 @`281a941`) |
| E-03 | U-C1-1, U-C1-2 | A3 provider가 늦어 실패(자연 또는 보류) | (CF, 1) NOT_READY_UNJUDGED → `INCONCLUSIVE(caller_requirement_absent)` | 없음(위반으로 세지 않음) | **예** | 이벤트 30 "failed to get sem id … error=%ld"(L997–L999), `break`(L1000), `CF_Init` 오류 → `APP_ERROR` → `CFE_ES_ExitApp`(`C1_basis_plan.md` §9.1 C-5~C-7) | C-2 반환을 못 읽으면 상태 미정, 이유 `effect_point_ambiguous` | 보류 G(name), 해제 R-a(C-4)/R-b(C-6) → `held-F0`. 확인되는 것은 미충족의 실현이며 위반 확인이 아니다 | C-1, C-2, C-5, C-6, C-7; P-2/P-3의 위치 | SPEC rev 1.3 App. A.4 step 3·6, ADR-054. Q-12가 정해지기 전에는 이 라벨을 바꾸지 않는다 |
| E-04 | U-C1-3, U-C1-4 | A1 첫 조회 성공 | (CF, 1) READY → `NO_VIOLATION_IN_CHECKED_PREFIX` | 없음 | 없음 | 없음. 이벤트 20 | E-01과 같음 | 예측 W5 | E-01과 같음 | 같음 |
| E-05 | U-C1-3, U-C1-4 | A2 먼저 실패, 재시도 성공(2 ≤ m ≤ 25) | (CF, 1 … m−1) TOLERATED, (CF, m) READY → `NO_VIOLATION_IN_CHECKED_PREFIX`(G-REBIND 충족, `C1_basis_plan.md` §9.8). P3 대안(`cfs_cf.dox` L212–L214를 종료를 받아들이는 caller 기록으로 묶음; 현행 아님): QT-5 충돌 → (CF, 1 … m−1) NOT_READY_UNJUDGED → `INCONCLUSIVE(caller_requirement_absent)`, 충돌 명시 | 없음 | P3 대안일 때만 | `OS_ERR_NAME_NOT_FOUND` m−1번(루프 안, L1006–L1011). 앱 종료 없음, 이벤트 20 | 시도 번호를 세지 못하면 `INCONCLUSIVE(explanation_ambiguous)` | 자연 실행에서만(§9.2.1, Q-9). 보류로 만들지 않는다 | C-1~C-3 모든 시도, P-3의 id | SPEC App. A.4 step 6; m의 상한 25는 `cf_platform_cfg.h` L338 @`833fdbb` |
| E-06 | U-C1-3, U-C1-4 | A3 provider가 늦어 25번 모두 실패(자연 또는 보류) | (CF, 1 … 25) VIOLATING(소진으로 REJECT; G-REBIND 충족, `C1_basis_plan.md` §9.8) → 모든 설명에서 성립하면 `OBSERVED_VIOLATION`, 아니면 `INCONCLUSIVE(explanation_ambiguous)`. P3 대안(E-05와 같음): QT-5 충돌 → NOT_READY_UNJUDGED → `INCONCLUSIVE(caller_requirement_absent)`, 충돌 명시 | **예**(P3 대안이면 아니오). 순서 의존은 같은 빌드에서 같은 순번의 좋은 증거가 있을 때만 주장(그 전에는 "schedule dependence undetermined", OUT-02) | P3 대안일 때만 | 이벤트 30(L1016–L1018), `break`(L1019), `APP_ERROR` → `CFE_ES_ExitApp` | C-2를 25번 모두 읽지 못하면 소진 사유 미정, `INCONCLUSIVE(explanation_ambiguous)` | 보류 G(name) + R-a/R-b → `held-F0`. 마지막 시도의 실패는 시간 기반 전이이므로 `timeout-exercised`(SPEC §1.8). 보류 실행은 자연 빈도의 근거가 아니다 | C-1~C-6, C-4 사유 `exhausted`, P-2/P-3 위치 | SPEC App. A.4 step 5·6; `C1_basis_plan.md` §9.2·§9.3. 감시 한도·중단 조건은 파일럿 값(PV-1~PV-3) |
| E-07 | U-C1-1~4 | A4 provider 없는 이름 | 실현하지 않음: "not realized: no referenced configuration"(litmus L-PATIENT variant 2, LP-CASE-C1) | — | — | — | — | — | — | SPEC App. A.4 step 6 [D]: 참조 구성이 없다. PI가 provider 없는 구성을 기록된 이탈로 정할 때만 실현 |
| E-08 | U-C1-1~4 | A5 RESERVED 구간 | 이 표의 범위 밖 | — | — | — | — | — | — | P-C1HOLD: 후속 OSAL 결함 후보 항목. 다섯 사례 수에 넣지 않는다 |
| E-09 | U-C1-2, U-C1-4 | 도달하지 않음(채널 0이 실패해 채널 1 조회 없음) | 발생 없음 → 상태·요약 없음 | — | — | (채널 0의 E-03/E-06 결과) | — | 이 키의 시험은 G(`BP1_tsem`)에서 보류하는 별도 실행 | C-1이 없다는 것 | `C1_basis_plan.md` §9.2 |
| E-10 | U-C1-1~4 | provider 초기화가 생성 전에 실패(P-E 관측) | CF 시도의 상태는 E-03/E-06의 규칙 그대로. 실행에 `provider-init-failed` 표시 | E-06과 같은 규칙. 단 순서 근거·짝짓기에 쓰지 않는다 | E-03과 같은 규칙 | BP: 표준 오류 또는 반환, 종료 이벤트 7과 `CFE_ES_ExitApp`(`bp_app.c` L839–L844). CF: 이벤트 30 | P-E를 못 보면 A3와 구분할 수 없음 → 그 실행은 A3로 세지 않고 `INCONCLUSIVE(explanation_ambiguous)` | — | P-E, P-0·P-2 없음 | `C1_basis_plan.md` §5.4·§9.7 (c). 일어나는지는 미정: bplib `d4c6db9`의 `bplib_init`은 항상 성공(`v7_bplib.c` L61–L71) [I] |
| E-11 | U-C1-5 | 정상 첫 기동(객체 없음) | 요구 없음 → 발생·요약 없음 | 없음 | 없음 | API 반환 `OS_ERR_NAME_NOT_FOUND`(L126), 이어서 생성(L132)과 이벤트 15 "Created semaphore"(L135). 정상 경로 | — | — | P-1, P-2, P-3, P-4 문구 | `C1_basis_plan.md` §9.7 (b); SPEC rev 1.3 보충 §1.6 "Call contexts". 역할 선언이 프로필에 들어가기 전의 도구 출력은 이 행과 맞지 않으며 결과로 쓰지 않는다 |
| E-12 | U-C1-5 | 다른 생성자가 먼저 만든 경우("Found semaphore") | 실현하지 않음 | — | — | — | — | — | — | 참조 구성의 provider는 BP 하나다. 확인·생성의 원자성은 `not supported`(§9.7 "판단할 수 없는 것" 4) |
| E-13 | U-C1-6 | 기본 표 | 조회 없음 → `NOT_APPLICABLE`(경로 근거: 빈 이름 검사) | 없음 | 없음 | 없음 | — | — | 빈 이름 표의 적재 | SPEC App. A.4 step 1 [D] |
| E-14 | U-R2-1, U-R2-2 | SB의 AppId 저장(`cfe_sb_task.c:138`) 전에 SB 이벤트 지점을 지난 사용 | 호출 시점에 VIOLATING(값 conjunct를 인자에서 캡처) → 모든 설명에서 성립하면 `OBSERVED_VIOLATION` | **예**. "저장 앞"이라는 순서는 HB 근거나 인증서가 없으면 한 시계의 증거이고, 순서 의존은 "schedule dependence undetermined" | 없음(`appreq`가 있어 Allowed `none`) | `CFE_EVS_APP_ILLEGAL_APP_ID`: `EVS_GetAppDataByID`가 NULL(`cfe_evs.c:185–189`). 이벤트 요청 거부(harm `event_request_rejected`). 앱 종료 없음 | `AppID` 인자를 못 읽으면 `INCONCLUSIVE(effect_point_ambiguous)`. 실행 task의 앱을 정하지 못하면 앱 간 결과로 세지 않음(`ownership_undetermined`) | 예측 W5. 확인 해당 없음 | 인자 `AppID`, 반환값, 실행 task, 생성 측 근거 | SPEC App. A.1 머리 주석 [I]; `proof_status_R2.md` §1·§5; `cases/R2.md` §4–§6. 사용 수는 출처가 정하지 않는다(census와 관측). 과거 gdb 기록의 24회는 개발 기록이며 기대값이 아니다 |
| E-15 | U-R2-3 | SB 저장 뒤의 사용 | 등록 conjunct의 LP와 `register` 효과가 계획에 없으면 `INCONCLUSIVE(start_state_unknown)` 또는 `INCONCLUSIVE(effect_unsupported)`. 계획에 더하면 READY → `NO_VIOLATION_IN_CHECKED_PREFIX`. VIOLATING이 되지 않으며 NOT_EVALUATED를 준비됨으로 읽지 않는다 | 없음 | 없음 | 없음 | 위와 같음 | — | X-03의 등록 관측 | SPEC App. A.1 머리 주석, ADR-055 |

## 4. 이 표가 말하지 않는 것

- 실행 횟수, 빈도, 감시 한도, 보류 길이, CPU·affinity, 우선순위 적용: 모두 `unspecified_by_reference`이다. 반복 수는 파일럿 뒤 PI가 정한다(마스터플랜 §12.3; `C1_basis_plan.md` §9.4).
- C1의 순서 의존 주장은 같은 빌드·구성·의미에서 같은 키·같은 순번의 좋은 증거와 나쁜 증거가 함께 있을 때만 한다(SPEC App. A.4 step 6). `281a941`와 `833fdbb`의 결과를 같은 순번으로 섞지 않는다.
- R2는 관측 경로의 기반 검증이며 기여의 증거가 아니다(SPEC App. A.1 머리 주석, ADR-057).
- 관측 결과, 모델 후보, 확인된 실행은 한 탐지 수로 더하지 않는다(OUT-01).

## 5. 개정 기록

| 날짜 | 행 | 바뀐 것 | 이유 | 이전 값 |
|---|---|---|---|---|
| 2026-10-24 | 전체 | 최초 동결 | W0 | — |
| 2026-10-24 | U-C1-3, E-05, E-06 | G-REBIND 충족을 반영하고, "거부" 분기를 P3 대안(QT-5 충돌)으로 바꿈. 기대 라벨은 그대로 | anchor audit(`C1_basis_plan.md` §9.8): L993–L999 @`833fdbb`의 글·해시가 `15a871e` L1272–L1278과 같음. W4 출력 전 개정 | U-C1-3 "(**G-REBIND 전제**)"; E-05/E-06 "G-REBIND 충족: … G-REBIND 전: 출력하지 않음. 거부: NOT_READY_UNJUDGED → `INCONCLUSIVE(caller_requirement_absent)`", 열 "G-REBIND 거부일 때만"/"G-REBIND 거부 시", E-06 "**예**(G-REBIND 충족 시)" |
