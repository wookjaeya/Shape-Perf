# C1·R2 기대 판정 근거표 — 원 출처에서 세운 기대 판정과 Kairos 기록의 대조 (2026-10-29)

- 위치와 역할: IP §6.1의 "원 이슈·수정·API/코드 의미 → 요구 의존성과 허용 동작 → 실제 대상 작업·상태·순서 → 기대 판정 → Kairos 출력과의 대조"를 C1(nasa/CF #184)과 R2(nasa/cFE #2663)에 대해 적는다. PD29(`Architecture/spec/PI_DECISIONS_RESOLVED_20261029.md`) §4 단계 2의 산출물이고, `REVIEW_ANALYSIS_20261029.md`(RA) §2.1 항목 9와 §3 "새 문서" 행이 요구한 것이다.
- 성격: **평가 근거를 추적하는 문서**다. 새 탐지 공식도, 실험 결과도 아니다. 이 문서를 쓰면서 cFS를 빌드·기동하지 않았다. 실험 조건(구성, 반복 수, 시간 한도, 보류 지점, 스케줄, 사용자)을 하나도 정하지 않았다(`CONDITIONS_POLICY.md`).
- **정답의 출처로 쓰지 않은 것**: Φ 프로필(`kairos/profiles/**`), Kairos 출력, 동결 기대표 `plan/expected_results_C1_R2.md`(E-01–E-15)의 라벨. 기대 판정은 원 이슈·수정·고정 커밋의 소스와 문서에서 다시 세웠다. 프로필은 §4에서, 동결 표는 §5에서 대조 대상으로만 읽었다.
- 동결 표는 고치지 않았다. 결과를 본 뒤의 개정은 그 실행을 개발 자료로 옮긴다(마스터플랜 §12.4, P-F1GATE). 개정 여부는 PI 몫이다(§6).
- 표기: [F] 고정 커밋의 소스·문서, 또는 원 이슈·PR에서 읽음(줄 번호는 그 커밋의 파일 기준). [M] 이번에 도구로 얻음. [I] 연구자 추론. [D] 결정. [U] 미확인. [기록] 앞서 보고했고 원 기록이 2026-10-29 컨테이너 교체로 없어진 결과(RA §0.1). [기록]의 수는 다시 확인한 값이 아니다.
- **근거 등급**(표의 "등급" 열): `직접` = 출처의 글이 그 내용을 말한다. `코드` = 고정 커밋 코드의 동작이다(출처가 그것을 요구로 말하지는 않는다). `추론` = 연구자가 글·코드에서 이은 것이다. `모형` = SPEC rev 1.3의 판정 규칙이 정한 것이다(출처가 아니다). `미정` = 출처가 정하지 않는다. 고르지 않고 그대로 둔다.
- **기대 판정의 두 층.** (가) 출처 기준 부류: 그 발생에서 요구 의존이 충족됐는가, 허용 처리의 출처가 있는가. 이것이 정답 쪽이다. (나) 기대 요약 라벨: (가)를 SPEC 규칙(READY / TOLERATED / VIOLATING / NOT_READY_UNJUDGED, 요약 `OBSERVED_VIOLATION` / `NO_VIOLATION_IN_CHECKED_PREFIX` / `INCONCLUSIVE(…)`)으로 옮긴 것이다. (나)가 `모형`에 기대면 표시한다. 도구가 관측으로 무엇을 세울 수 있는지는 따로 적는다(§2.4·§3.4의 대조 열).

## 0. 요약

1. **C1.** 요구 의존("CF의 조회 전에 그 이름의 세마포어가 있다")은 출처가 직접 말한다(#184 본문, `cf_events.h` L155–L156, `cfs_cf.dox` L212–L214). 수정 후 앞선 실패의 허용(재시도)도 직접이다(수정 주석 L993–L999, PR #342). 그러나 두 라벨은 출처로 정해지지 않는다.
   - 수정 전 단일 실패의 허용 여부: `미정`. P3/Q-12가 열려 있다. 지금 라벨 `INCONCLUSIVE(caller_requirement_absent)`는 그 미정을 적은 라벨이다.
   - 수정 후 25회 소진의 `VIOLATING`: `모형`이다. 소진 뒤 종료를 받아들이는 출처도, 거부한다고 직접 말하는 출처도 없다.
   - 기록된 Kairos 출력[기록]은 모든 부류에서 동결 표와 라벨이 같다. 다만 이 두 곳의 일치는 같은 모형을 두 번 적용한 결과다(§4 V-C1-1, V-C1-2).
2. **R2.** 요구(`AppID`는 등록된 앱이나 코어 서비스에 대응해야 한다)는 API 헤더가 caller 쪽에 직접 말한다(`cfe_evs.h` L169–L172 @`c5fb2b4d`). 허용 처리는 없다. 이슈의 분류(bug), CCB의 "Development team will work on this ticket", 분석 댓글이 이 쪽이다.
   - ES·EVS task의 AppID 0 사용(SB 저장 L138 전)은 `VIOLATING → OBSERVED_VIOLATION`이다. 기록 24개(ES 20, EVS 4)[기록]와 일치하고, 반환 코드 없이 선다.
   - EVS 4와 SB 자기 전송 5는 코드에서 정확히 셀 수 있다[I]. 기록과 같다. ES 20은 판·빌드 정보 이벤트 수에 달려 출처에서 정해지지 않는다.
   - 기본 구성에서 사용이 저장보다 앞서는 순서는 ES의 코어 직렬화가 강제한다[I, 코드]. 스케줄 race가 아니다. Kairos는 "schedule dependence undetermined"로 적는다. 더 약하게 적을 뿐 모순은 아니다.
   - 다른 task의 사용(SB AppID·TBL AppID)은 출처 기준으로 READY다(코어 직렬화 → `CORE_READY` → 앱 시작). Kairos의 READY는 반환 코드 좁힘에만 기대고, 그것을 끄면 INCONCLUSIVE다[기록]. 출처 기준 부류는 정해지지만 도구는 독립으로 세우지 못한다.
3. **오경보 대조로 쓸 수 있는 정상·허용 경로**: C1 수정 후의 "첫 실패 → 재시도 성공"(자연 기록 전부), BP1 첫 조회 성공, BP의 존재 확인. R2 SB 자기 전송 5, 다른 task의 사용. [기록]에서 어느 것도 위반으로 나오지 않았다. 다만 BP 존재 확인과 R2 다른 task 사용은 대조의 힘이 약하다. 앞의 것은 프로필의 역할 선언에, 뒤의 것은 반환 코드에 기댄다(§2.5, §3.5).
4. **자기 검증 위험**(§4): C1 소진의 REJECT, C1 수정 전의 Allowed 미정 표기, BP 역할 선언, R2 SB 자기 전송이 기대는 A-OWNER, R2 코어 직렬화 규칙(R19)이 실제 기록에서 서지 않는 것, 두 사례의 반환 코드 좁힘.
5. **이슈 접근**: 공개 REST API는 다섯 URL 모두 HTTP 403(세션 proxy의 저장소 단위 차단)이었다. 공개 이슈 페이지는 WebFetch로 읽었다. CF#184의 댓글 앵커 둘과 인용 하나가 새로 나왔다(§1.2).

## 1. 소스 사본과 이슈 접근 [M]

### 1.1 소스 사본

`/home/user/work/expect_src/` 아래에 고정 SHA마다 `git fetch --depth=1 origin <SHA>`로 받았다(2026-10-29T02:26Z, 기록 `clone_log.txt`). 저장소 밖의 읽기용 사본이다. 보존 대상이 아니고, SHA만 있으면 다시 만들 수 있다.

| 사본 | 저장소 @ SHA | 쓰임 |
|---|---|---|
| `cFS_088b2fa` | nasa/cFS `088b2fa828db9ff7e00733f1908e0eeb59f66ce3`(v7.0.1) | R2 기준 번들. gitlink: cfe `c5fb2b4d`, osal `d2d877a6`, psp `c4b3b0b6`, apps/cf `15a871e6` [M: `git ls-tree`] |
| `cFE_c5fb2b4` | nasa/cFE `c5fb2b4d540bd55eb6c3707da7dd13eee679d4dd` | R2 줄 인용 |
| `osal_d2d877a` | nasa/osal `d2d877a69cff47452bcca274b309147d48e6c16f` | R2 번들 OSAL |
| `CF_281a941`, `CF_833fdbb` | nasa/CF `281a94188cd7…`, `833fdbb27a26…` | C1 수정 전·후 |
| `CF_15a871e` | nasa/CF `15a871e66cad…`(v7.0.1) | G1 주석 해시 대조, K-C1-neg |
| `bp_b051eb0` | nasa/bp `b051eb0d12bf…` | C1 provider(D22) |
| `cFS_1df262a`, `cFS_1b0b338` | nasa/cFS 후보 A·B | gitlink: A cfe `6cccb3f5`·osal `f34759d6`, B cfe `fef4a0da`·osal `b44f4ce7`, psp 둘 다 `a260e9cc` [M]. `plan/C1_build_manifest.md` §1과 같다 |
| `cFE_6cccb3f`, `cFE_fef4a0d`, `osal_f34759d`, `osal_b44f4ce` | 위 gitlink | C1 ES 시작·OSAL 조회. 이 문서가 인용하는 파일(`cfe_es_start.c`, `cfe_es_apps.c`, `cfe_es_api.c`의 RunLoop 구간, `osapi-idmap.c`, `osapi-countsem.c`, `osapi-countsem.h`)은 두 후보에서 바이트 단위로 같다 [M: `diff -q`]. 그래서 C1의 줄 인용은 후보 A·B에 함께 맞는다 |
| `bplib_d4c6db9` | nasa/bplib `d4c6db9f…` | E-10의 근거 확인용(읽기만) |

같은 SHA의 다른 사본(`/home/user/work/procrace/refs/…`)은 2026-10-29에 없어졌다. 이 문서의 줄 번호는 위 사본에서 다시 읽은 값이다.

### 1.2 원 이슈 접근 (2026-10-29)

| 경로 | 결과 | 쓰임 |
|---|---|---|
| `curl https://api.github.com/repos/nasa/CF/issues/184`, `…/184/comments`, `…/nasa/cFE/issues/2663`, `…/2663/comments`, `…/cFE/issues/comments/5588955819` (02:30Z) | 다섯 개 모두 HTTP 403, 378 B. 본문은 "GitHub access to this repository is not enabled for this session. Use add_repo to request access. …" | 자료 아님. 저장소 연결이나 자격 증명 연결은 시도하지 않았다(사용자 동의가 필요하고, 같은 시도가 앞서 거부됐다: `cases/R2.md` §1.5) |
| WebFetch `https://github.com/nasa/CF/issues/184` | 본문과 댓글 다섯 개가 렌더됐다 | 인용은 모형을 거친 추출기가 돌려준 125자 이하의 문구다. 전문은 저장하지 않았다 |
| WebFetch `https://github.com/nasa/cFE/issues/2663` | 본문과 댓글 둘이 렌더됐다 | 같은 성격. 같은 날 `cases/R2.md` §1.5의 읽기와 문구·앵커·날짜가 같다 |

CF#184에서 `cases/C1.md` §0a에 없던 것 [F, 모형 경유, 2026-10-29]:
- 댓글 1 앵커 `issuecomment-1012599851`(jphickey, "Jan 13, 2022"). 돌려준 문구는 "simply starting CF after the I/O app seems to avoid hitting it most of the time"이다. §0a는 같은 댓글에서 "calling this a bug because its a race condition"을 인용했다. 두 문구가 한 댓글에 함께 있는지는 확인하지 않았다 [U].
- 댓글 4 앵커 `issuecomment-1319058630`(skliper, "Nov 17, 2022").
- 이 문서는 둘 다 질적 보고로만 쓴다. 조건은 하나도 거기서 가져오지 않았다. 사례 파일 갱신은 그 파일 담당 몫이다.

## 2. C1 — nasa/CF #184

기준 구성은 동결 표 §1의 K-C1-pre(CF `281a941` + BP `b051eb0`)와 K-C1-post(CF `833fdbb` + 같은 BP)이고, cFS 후보 A·B를 함께 다룬다. #184 본문이 링크한 `2a024d8`은 표 구조가 달라 이 구성이 아니다(`plan/C1_basis_plan.md` §5.2).

### 2.1 원 이슈·수정·API와 코드의 의미

| # | 출처 · 위치 | 말하는 것 | 등급 |
|---|---|---|---|
| S-C1-1 | #184 본문(2022-01-13, `2a024d8` 기준). `cases/C1.md` F1(2026-10-08 원문), 2026-10-29 페이지 | "If configured to use a throttle sem, the CF app expects that semaphore to be created before it starts."; "if that fails, CF aborts"; Expected behavior "Should be guaranteed via sync mechanisms, or shouldn't be a hard error (e.g. maybe retry to bind later?)"; 재현 "Start CF before the app that creates the sem (still not guaranteed, but increases the chance the race will be lost)" | 직접 |
| S-C1-2 | #184 댓글 2 `issuecomment-1202543669`, 3 `…1319057031`, 5 `…1319061815`, 1 `…1012599851` | BP를 먼저 시작해도 "it still hasn't been created by the time CF checks for it so it fails out"; "it hits this race quite often and CF bails out"; 로컬 sleep 패치; CF를 I/O 앱 뒤에 시작하면 대개 피한다 | 직접(보고). 판·구성 미상 |
| S-C1-3 | `fsw/src/cf_events.h` L148–L158 @`281a941` = @`833fdbb` | 이벤트 30, Type ERROR, Cause "Failure from get semaphore by name call during engine channel initialization, semaphore needs to exist before engine is initialized." | 직접(CF 자신, caller 쪽) |
| S-C1-4 | `docs/dox_src/cfs_cf.dox` L208–L216, 두 판 같음 [M: diff] | provider는 "TO … or an equivalent application"; 이름은 "must match the name of the semaphore created by TO"; "If a semaphore is expected and not found, the application will terminate during initialization." | 직접(서술) |
| S-C1-5 | `281a941` `fsw/src/cf_cfdp.c` L991–L1001, L1029–L1034; `fsw/src/cf_app.c` L252, L330–L338, L366(두 판 같음 [M]) | 이름이 비어 있지 않으면 조회 1회(L993). 실패하면 이벤트 30(L997–L999)과 `break`(L1000). 성공일 때만 엔진 사용(L1029–L1031). `CF_Init` 오류 → `APP_ERROR`(L333) → `CFE_ES_RunLoop`(L338) → `CFE_ES_ExitApp`(L366) | 코드 |
| S-C1-6 | `833fdbb` `cf_cfdp.c` L993–L999(인정 주석), L1000–L1012(루프), L1014–L1019(최종 실패); `fsw/platform_inc/cf_platform_cfg.h` L329–L338(`CF_STARTUP_SEM_MAX_RETRIES 25`), L340–L349(`CF_STARTUP_SEM_TASK_DELAY 100`, ms) | 주석: "There is a start up race condition because CFE starts all apps at the same time, and if this sem is instantiated by another app, it may not be created yet. Therefore if OSAL returns OS_ERR_NAME_NOT_FOUND, assume this is what is going on, delay a bit and try again." 루프는 `OS_ERR_NAME_NOT_FOUND`일 때만 지연(L1011) 뒤 다시 조회한다. 다른 값이면 즉시 끝낸다(L1006–L1008). 주석 L993–L999의 sha256 `1c487620…9842f`는 `15a871e` L1272–L1278과 같다 [M] | 직접(주석) + 코드 |
| S-C1-7 | PR #342 본문(`cases/C1.md` F6, 2026-10-08 원문) | "there may be some delay until the other app gets to the point where it creates the sem. This works around the race condition."; "A retry limit is also imposed so CF will not spin here forever."; "Should start up more reliably"; 시험 "check file transfers with BP" | 직접 |
| S-C1-8 | `833fdbb` `unit-test/cf_cfdp_tests.c` L866–L889 | 이름 외 사유의 실패(`OS_ERROR`)는 즉시 실패. "Max retries … sem was never created at all"은 `OS_ERR_NAME_NOT_FOUND`, 엔진 비활성, 이벤트 30. "Retry … when sem was created late"는 엔진 사용 | 코드(작성자가 기대한 동작의 시험) |
| S-C1-9 | OSAL `src/os/inc/osapi-countsem.h` L150–L162 @`f34759d` = @`b44f4ce` | `OS_ERR_NAME_NOT_FOUND` "if the name was not found in the table". 문서화된 반환값이고, OSAL 문서는 호출 순서를 요구하지 않는다 | 직접(supplier 쪽) |
| S-C1-10 | OSAL `osapi-countsem.c` L94(AllocateNew), L100(`OS_OBJECT_INIT`), L103(Impl), L106(FinalizeNew); `osapi-idmap.c` L828–L849(`OS_ObjectIdTransactionFinish(token, &final_id)` L849) | 이름은 L100에서 기록에 들어가고, id는 L849에서 확정된다. 그 뒤 반환한다 | 코드 |
| S-C1-11 | BP `b051eb0`: `fsw/src/bp_flow.c` L121–L144, L519–L524; `fsw/src/bp_app.c` L804–L818; `fsw/custom/bp_semcfg.c` L33–L36, L44, L48; `.github/buildconfig/cfe_es_startup.scr` L6–L7; `tx/tables/cf_def_config.c` L40, L54 | BP는 이름마다 조회(L126)하고, 없으면 같은 이름으로 생성(L132–L133)하고 이벤트 15 "Created semaphore"(L135)를 낸다. 이 일은 첫 `CFE_ES_RunLoop`(L818) 전, `AppInit` 안에서 일어난다. 주석 "The CF task uses semaphores to throttle how many CFDP PDUs it posts". 스크립트 순서는 CF(L6, 65) 다음 BP(L7, 55). CF 표 이름은 `BP0_tsem`·`BP1_tsem` | 코드·설정 |
| S-C1-12 | cFE `6cccb3f` = `fef4a0d`: `cfe_es_start.c` L199, L208–L231; `cfe_es_apps.c` L258, L654, L768, L588; `cfe_es_api.c` L497–L499, L513–L515 | ES는 스크립트 줄마다 `ParseFileEntry`(L258) → `AppCreate`(L654) → `StartAppTask`(L768) → `OS_TaskCreate`(L588)를 부르고, 앱 사이에서 기다리지 않는다. 스크립트 뒤의 `LATE_INIT`·`RUNNING` 대기(L208, L223)는 실패하면 "Startup Sync failed"만 적고 OPERATIONAL로 간다(L231). `RunLoop`은 `APP_RUN`이 아니면 바로 false다(L497–L499). CF·BP의 `fsw/`에는 `CFE_ES_WaitForStartupSync`·`WaitForSystemState` 호출이 없다 [M: grep] | 코드 |
| S-C1-13 | `cf_cmd.c` L1119(두 판) | 엔진 사용 명령이 `CF_CFDP_InitEngine()`을 다시 부른다. 같은 조회 지점을 실행 중에 다시 지난다. #184 본문은 이 경로를 "may be recoverable … (but haven't tested that)"라고 적는다 | 코드 + 직접(미시험 진술) |

### 2.2 요구 의존과 허용 동작 — 출처가 직접 말하는 것과 연구자 추론

| # | 내용 | 등급 · 근거 |
|---|---|---|
| R-C1-a | **요구 의존.** 채널 *i*의 이름이 비어 있지 않으면, CF의 `OS_CountSemGetIdByName(sem_name)` 전에 그 이름의 counting semaphore가 있어야 한다 | **직접**: S-C1-1("expects … created before it starts"), S-C1-3("needs to exist before engine is initialized"), S-C1-4. caller(CF) 쪽 글이다. OSAL 문서(S-C1-9)는 not-found를 정상 반환으로 다루므로 요구의 출처가 아니다 |
| R-C1-b | 정밀화: "있다"는 조회가 표를 훑는 시점에 id가 확정됐다(L849)는 뜻이다. 생성 호출의 반환보다 약하다 | **추론**(`plan/C1_basis_plan.md` §1.2). 출처의 글은 "created before"뿐이다. 판정 라벨은 바꾸지 않는다. RESERVED 구간(L100–L849 사이의 조회)은 A5로 범위 밖이다 |
| R-C1-c | provider는 다른 앱이다. 공개 BP 기준에서는 BP | 직접(S-C1-1 "created by another app", S-C1-4 "TO or an equivalent", S-C1-2 댓글 2 "BP creates the throttle sem"). BP를 provider로 고정한 것은 D22 [D]와 공개 설정(S-C1-11)이다. 원 보고자의 provider와 같다는 근거는 없다 |
| R-C1-d | 순서를 정하는 장치가 없다. 그래서 두 앱의 상대 순서는 스케줄에 달린다 | **직접**: race라는 말(S-C1-1, S-C1-6 주석). **코드**: CF·BP 모두 시작 동기화가 없고, ES는 앱 사이를 기다리지 않는다(S-C1-12) |
| R-C1-e | **수정 전 처리의 허용 여부**(단일 실패 → `APP_ERROR` → 종료) | **미정.** 받아들이는 쪽으로 읽을 수 있는 글은 S-C1-4 "will terminate"(서술)다. 거부하는 쪽의 글은 S-C1-1 "shouldn't be a hard error … rather than having CF abort/exit"(`2a024d8`에 대해 썼다. `281a941`과는 수정의 부모라는 점으로만 이어진다 [I])와 S-C1-3의 ERROR 분류다. 어느 쪽을 이 판에 묶을지는 P3/Q-12(미결)다. 둘 다 묶으면 QT-5 충돌이다(`C1_basis_plan.md` §9.7 판단할 수 없는 것 1) |
| R-C1-f | **수정 후 앞선 실패의 허용.** `OS_ERR_NAME_NOT_FOUND`이면 지연 뒤 다시 조회한다. 같은 루프의 뒤 시도가 성공하면 앞선 실패는 허용된 처리다 | **직접**: S-C1-6 주석("assume this is what is going on, delay a bit and try again"), S-C1-7("works around the race condition"). 상한 25와 지연 100 ms는 그 판의 값이다(코드) |
| R-C1-g | **수정 후 소진 뒤 종료**(25회 모두 not-found → 이벤트 30 → 종료) | 받아들인다는 글도, 거부한다고 직접 말하는 글도 없다. S-C1-7 "so CF will not spin here forever"는 한도를 둔 목적이다. S-C1-8의 소진 시험은 provider가 없는 경우만 다룬다. S-C1-3은 ERROR로 분류한다. 그래서 **요구 미충족은 직접+코드로 정해지고, `VIOLATING`(허용 REJECT)은 "받아들이는 출처가 없다"에서 SPEC 규칙이 낸 `모형` 판정**이다(`C1_basis_plan.md` §9.8 결론 3·5(ii)) |
| R-C1-h | BP의 조회에는 요구가 없다. not-found는 생성 분기의 조건이다 | **코드**: S-C1-11 L126–L133. BP가 사전 존재를 요구한다는 글은 없다(`C1_basis_plan.md` §9.7 (b) [M]). "요구가 없다"는 **추론**이다 |
| R-C1-i | 이벤트 30, `APP_ERROR`, `CFE_ES_ExitApp`은 결과(consequence)다. 발생 판정과 따로 센다 | 코드(S-C1-5, S-C1-6). 마스터플랜 §12.1의 열 분리 [D] |

### 2.3 실행에서의 대상: task · 상태 · 순서 · 그것을 보이는 사건

- **실행 task**: CF 주 task(스크립트 L6의 생성 요청). **provider**: BP 주 task(L7). 같은 프로세스(tx 또는 rx)의 OSAL 객체 표에 있다. tx·rx의 같은 이름 객체는 서로 다른 객체다(P7).
- **상태**: 이름별 `BP0_tsem`, `BP1_tsem`. 한 부팅에서 생성은 한 번이다. 기준 구성의 어느 앱도 `OS_CountSemDelete`를 부르지 않는다(`C1_build_manifest.md` §8.5) [기록].
- **판정할 순서**: BP의 id 확정(L849. 직접 보지 못하면 생성 반환 P-3의 id)과 CF 조회(C-1 진입, C-2 반환)의 선형화 시점.
- **그것을 보이는 사건**(`C1_basis_plan.md` §9.1의 어휘): CF 쪽은 C-1(이름, 시도 번호), C-2(반환값, out id), C-3(지연, `833fdbb`만), C-5(이벤트 30과 `error=` 값), C-6(InitEngine 반환), C-7(`CFE_ES_ExitApp`). BP 쪽은 P-1(존재 확인의 반환), P-2(생성 진입), P-3(생성 반환 상태·id), 이벤트 15 문구. 같은 세대라는 근거는 C-2의 성공 id와 P-3의 id가 같다는 것이다.
- **키 사이의 관계**: 채널 0이 최종 실패하면 루프 전체가 끝난다(`281a941` L1000, `833fdbb` L1019). 그러면 채널 1 조회는 없다. 그 키는 "도달하지 않음"이다.

### 2.4 발생 부류별 기대 판정과 기록된 Kairos 출력의 대조

Kairos 열은 모두 [기록]이다. 출처는 `plan/results_pilot_20261025.md` §1(행 C1-N1–C1-C3), `plan/results_w4w5_20261028.md` §3·§4.2·§4.4다. "cur"은 기본 읽기, "nocode"는 반환 코드와 결과 유도 RF를 끈 읽기다.

| 부류 | 판 | (가) 출처 기준 부류 | (나) 기대 상태 → 요약 | 등급 | 동결 행 | Kairos [기록] | 대조 |
|---|---|---|---|---|---|---|---|
| **K1 첫 조회 성공** | pre | 충족(같은 id) | (CF,1) READY → `NO_VIOLATION_IN_CHECKED_PREFIX` | 직접+코드 | E-01 | 자연 실행에서는 나오지 않았다. 자연 기록 32개(보존·새 × F0·G-I × 8) 가운데 pre 16개 모두 `BP0: -17x1`. 연구자 개입 C1-C1·C1-C2는 READY(SO+RD). 제어 W2·W10·W11·W12의 BP0도 READY(SO+CO) | 판정 일치. 자연 실현은 없다 |
| K1 | post | 충족 | READY → `NO_VIOLATION…` | 직접+코드 | E-04 | 자연 post 기록 16개의 (BP1,1) READY. 근거는 F0 SO+RD, 계측 SO+ST | 일치 |
| **K2 첫 조회 실패 = 최종 실패** | pre | 미충족(not-found = 표에 그 이름 없음, S-C1-9). 허용 출처 **미정**(R-C1-e) | (CF,1) NOT_READY_UNJUDGED → `INCONCLUSIVE(caller_requirement_absent)`. 미정을 그대로 적은 라벨이다. 위반으로 세지 않는다 | 미충족: 직접+코드. 라벨: **미정(P3)** | E-03 | 자연 pre 기록 16개 모두 (BP0,1) NOT_READY_UNJUDGED → `INCONCLUSIVE(caller_requirement_absent)`, 근거 ON. nocode에서는 상태 미정. W2의 (BP1,1)도 NOT_READY_UNJUDGED(SO+CO) | 라벨 일치. 단 (가)의 "미충족"을 자연 실행에서는 반환 코드로만 세운다. 반환 코드 없이는 세우지 못한다(§4 V-C1-6) |
| **K3 첫 실패 → 재시도 성공** (2 ≤ m ≤ 25) | post | 앞 m−1회 미충족, 허용 **직접**(R-C1-f). m번째 충족 | (CF,1…m−1) TOLERATED, (CF,m) READY → `NO_VIOLATION…`. 좋은 실행 | 직접+코드 | E-05 | 자연 post 기록 16개 모두 `BP0: -17x1 0x1`. (BP0,1) TOLERATED(ON), (BP0,2) READY. 제어 W5·W7·W8·W11·W12의 TOLERATED(SO+CO). nocode: F0는 INCONCLUSIVE, 계측은 TOLERATED(성공 조회의 ST에 간접으로 기댐, `results_w4w5` §4.3) | 일치. **수정 후에도 첫 실패가 자연 실행에서 매번 났다**(IP §6.1 "수정 후에도 초기 실패가 가능"). 위반으로 나오지 않았다 |
| K3 | pre | 해당 없음(단일 조회, S-C1-5) | — | 코드 | E-02 | — | 일치 |
| **K4 재시도 소진** | post | 25회 모두 미충족. 소진 뒤 종료를 받아들이는 출처 없음, 거부를 직접 말하는 출처도 없음(R-C1-g) | (CF,1…25) VIOLATING → `OBSERVED_VIOLATION`(현행 모형). P3 대안이면 NOT_READY_UNJUDGED → `INCONCLUSIVE(caller_requirement_absent)` | 미충족: 직접+코드. VIOLATING: **모형** | E-06 | W1(BP0,1–25), W6·W10(BP1,1–25) VIOLATING → `OBSERVED_VIOLATION`, SO+CO. 파일럿 C1-B1은 ON이었다(P1 끔 → `INCONCLUSIVE(horizon-open, …)`). 모두 제어 실행이고 `timeout-exercised`다. 자연 실행에서는 나오지 않았다 | 라벨 일치. 그러나 기대 라벨 자체가 모형 판정이므로 이 일치는 독립 확인이 아니다(§4 V-C1-1). 자연 빈도의 근거도 없다 |
| **K5 최종 CF 종료** | pre(K2 뒤), post(K4 뒤) | 결과 | 발생 판정 아님. 이벤트 30, `APP_ERROR`, `CFE_ES_ExitApp`을 결과로 남긴다 | 코드 | E-03·E-06의 결과 열 | 반환 -17, 이벤트 30 "failed to get sem id for name BP0_tsem", `CFE_ES_ExitApp` 1회(C1-N1, C1-B1) | 일치. 판정과 섞지 않았다 |
| K6 앞 채널 실패로 조회 없음 | 둘 | 발생 없음 | 상태·요약 없음 | 코드 | E-09 | 채널 1 조회 없음(C1-N1, C1-B1) | 일치 |
| **K7 BP 존재 확인의 not-found** | 둘 | 요구 없음(R-C1-h) | 발생 아님 | 코드+추론 | E-11 | 역할 사실 2, 발생 0. 반환 -17 뒤 생성, 이벤트 15(C1-N5, C1-B2, C1-C3) | 일치. 단 일치의 원인이 프로필의 역할 선언이다(§4 V-C1-3) |
| K8 provider 초기화가 생성 전에 실패 | 둘 | 순서 출처가 아니다 | 따로 표시한다. 순서 근거로 쓰지 않는다 | 코드 | E-10 | 나오지 않았다 | 해당 없음 |
| K9 provider가 없는 이름 | 둘 | 설정 문제(#178) | 실현하지 않음 | 직접(#178) | E-07 | — | 해당 없음 |
| K10 이름이 빈 기본 표 | CF `15a871e` 기본 표 | 경로 비활성 | `NOT_APPLICABLE` | 코드 | E-13 | 실행하지 않았다 | 판단 불가(기록 없음) |
| K11 not-found가 아닌 실패(예: RESERVED 구간의 `OS_ERR_INCORRECT_OBJ_STATE` 가설, A5) | post에서는 즉시 끝남(L1006–L1008) | 미정 | 범위 밖 | 추론 | E-08 | — | 판단 불가 |
| K12 엔진 재사용 명령으로 다시 조회 | 둘(S-C1-13) | 미정. 출처는 "haven't tested that"뿐이다 | 미정 | 직접(미시험) | 없음 | 실행하지 않았다 | 판단 불가 |

**순서 의존의 주장**(동결 표 §4, SPEC App. A.4 step 6 [D]): 같은 빌드·구성·의미에서 같은 키·같은 순번에 좋은 증거와 나쁜 증거가 함께 있어야 한다.
- post (BP0,1): 자연 TOLERATED(좋음)와 W1 VIOLATING(나쁨)이 함께 있다[기록].
- pre (BP0,1): 자연 NOT_READY_UNJUDGED(나쁨)와 연구자 개입 READY(좋음)가 함께 있다[기록].
- 둘 다 한쪽이 제어 실행이다. 이 짝을 순서 의존의 근거로 셀지는 이 문서가 정하지 않는다. 제어로 얻은 순서는 자연 빈도에 합치지 않는다(PD29 D4).

### 2.5 오경보 대조로 쓸 수 있는 정상·허용 경로

| ID | 경로 | 출처 근거 | [기록]의 결과 | 대조의 힘 |
|---|---|---|---|---|
| N-C1-1 | post 자연의 첫 실패 → 재시도 성공(K3) | R-C1-f(직접) | post 자연 기록 16개 모두 위반 없음. cur은 TOLERATED, nocode는 F0 INCONCLUSIVE·계측 TOLERATED | **강함.** 허용 출처가 직접이고, 자연 실행에서 매번 났다. 오경보가 있었다면 VIOLATING으로 나왔을 것이다 |
| N-C1-2 | post BP1 첫 조회 성공(K1) | R-C1-a | READY | 보통. nocode F0에서는 INCONCLUSIVE(오경보는 아님) |
| N-C1-3 | BP 존재 확인(K7) | R-C1-h(코드+추론) | 발생 아님 | **약함.** 프로필의 역할 선언으로 빠진다. 엔진이 스스로 가린 것이 아니다. 역할 선언 전의 출력은 이 행과 맞지 않았다(동결 E-11 근거 열) |
| N-C1-4 | pre의 첫 조회 성공(K1, 연구자 개입) | R-C1-a | READY | 보통. 제어 실행이고 자연 경로가 아니다 |
| N-C1-5 | 이름이 빈 기본 표(K10) | 코드 | 실행 없음 | 아직 없음. 쓰려면 조건 설정이 필요하므로 이 문서는 정하지 않는다 |

### 2.6 출처로 정할 수 없는 것 (미정으로 둔다)

1. 수정 전 단일 실패의 허용 여부(R-C1-e, P3/Q-12). 라벨 `INCONCLUSIVE(caller_requirement_absent)`는 이 미정을 적은 것이다. 위반 수에 넣지 않는다.
2. 수정 후 소진의 위반 여부(R-C1-g). PR #342의 "will not spin here forever"를 소진 종료의 받아들임으로 읽을지는 출처가 말하지 않는다. 현행 `VIOLATING`은 모형 판정이고, 동결 E-06의 해석 주의가 이를 적는다.
3. 원 보고(2022, BP를 먼저 시작)의 판·구성. D22 구성은 그 보고의 재현이 아니다.
4. A5(RESERVED 구간), 엔진 재사용 명령 경로(K12), provider 재시작 뒤 id가 바뀌는 "secondary problem"(S-C1-1). 셋 다 기록이 없고 범위 밖이다.
5. 각 부류의 자연 빈도. 출처는 "not readily reproducible", "quite often"뿐이다. `unspecified_by_reference`다.

## 3. R2 — nasa/cFE #2663

기준 구성은 동결 표 §1의 K-R2(cFS `088b2fa8`, cFE `c5fb2b4d`, 기본 기동)다. 이슈가 보고한 판은 `cases/R2.md`가 추론한 B(`fc8c48ad`)이고, R2 기준과 다르다.
- B에서는 코어 task를 EVS, SB, ES 순서로 만든다. 그래서 코드상 EVS만 이 경로에 닿는다(`cases/R2.md` §5, Q1).
- `c5fb2b4d`에서는 ES와 EVS가 SB보다 앞선다(S-R2-11). 이슈의 "the ES and the EVS tasks"와 맞는 쪽은 R2 기준의 코드다.
- 이것은 보고자 환경의 재현이 아니다.

### 3.1 원 이슈·수정·API와 코드의 의미

| # | 출처 · 위치 | 말하는 것 | 등급 |
|---|---|---|---|
| S-R2-1 | #2663 본문(2025-08-06, 2026-10-29 페이지) | "When the ES and the EVS tasks initializes, they make various CFE_SB API calls (i.e. CFE_SB_CreatePipe() and CFE_SB_Subscribe()) which in turn call CFE_EVS_SendEventWithAppID() passing in the AppId for the CFE_SB."; "returns a CFE_EVS_APP_ILLEGAL_APP_ID presumably because CFE_SB has not been registered with event services."; "no event outputted nor is the return code set to any variable"; gdb 중단점 `cfe_evs.c#L185`(main); "cFE v6.7", Ubuntu 22.04.5; 라벨 bug | 직접(보고와 보고자의 추정) |
| S-R2-2 | CCB 댓글 `issuecomment-3255030799`(dzbaker, "Sep 4, 2025") | "CCB 4 September 2025: Tracked internally. Development team will work on this ticket." | 직접(관리자의 수용) |
| S-R2-3 | 분석 댓글 `issuecomment-5588955819`(philphauler, "Sep 8, 2026") | "The ILLEGAL_APP_ID at startup comes from SB, not from the caller EVS is checking."; "SB attributes pipe-creation events to itself using CFE_SB_Global.AppId, but that field is only filled in CFE_SB_AppInit (cfe_sb_task.c:138) when the SB task runs."; "Whichever core app's init runs before that sees SB call CFE_EVS_SendEventWithAppID with an undefined AppId, which is exactly the error reported here."; 수정 제안 둘(제안일 뿐) | 직접(기여자의 코드 읽기. 관리자의 판단도 실행 결과도 아니다) |
| S-R2-4 | PR #2820(닫힘, 병합 안 됨) | 사용 지점 fallback 시도. 작성자가 "vacuous"로 닫았다(`cases/R2.md` §1.4) | 직접. 병합된 수정은 없다 |
| S-R2-5 | `modules/core_api/fsw/inc/cfe_evs.h` L165–L166, L168–L172, L196–L200 @`c5fb2b4d` | "this function should really only be used from within an API in order to preserve the context of an Application's event."; "The Application ID must correspond to a registered application or core service. For messages outside the context of a registered application (for example early in app initialization or if registration fails) #CFE_ES_WriteToSysLog can be used for reporting."; 반환 목록에 `CFE_EVS_APP_ILLEGAL_APP_ID`(L199) | **직접**(caller 쪽 요구) |
| S-R2-6 | `modules/evs/fsw/src/cfe_evs.c` L169–L214; `cfe_evs_utils.c` L48–L53 | `EVS_GetAppDataByID(AppID)`가 NULL이면 `ILLEGAL_APP_ID`(L185–L189). 슬롯이 그 id가 아니면 `EVS_NotRegistered`(L190–L194). 필터는 그 뒤(L195). id를 index로 바꾸지 못하면 NULL이다(utils L53) | 코드 |
| S-R2-7 | `cfe_sb_init.c` L50; `cfe_es_api_typedefs.h` L220; `cfe_resourceid_api_typedefs.h` L65 | EarlyInit의 `memset(&CFE_SB_Global, 0, …)`. 0은 `CFE_ES_APPID_UNDEFINED` | 코드 |
| S-R2-8 | `modules/sb/fsw/src/cfe_sb_task.c` L77, L93, L138, L155, L183, L190, L198, L206, L242 | `CFE_SB_AppInit` 안에서 `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`(L138, 반환 무시) → `CFE_EVS_Register`(L155) → `CreatePipe`(L183) → `Subscribe` 셋(L190, L198, L206) → 시작 이벤트(L242). 그 뒤 `WaitForSystemState(CORE_READY)`(L93) | 코드 |
| S-R2-9 | `cfe_sb_api.c` L275–L277(`CFE_SB_PIPE_ADDED_EID` 5), L1183(`SUBSCRIPTION_RCVD` 10); `cfe_sb_priv.c` L786, L1086(`SEND_NO_SUBS` 14) | SB API의 이벤트는 호출한 task의 문맥에서 `CFE_SB_Global.AppId`로 보낸다. 구독자가 없는 송신은 NO_SUBS 이벤트를 같은 AppId로 낸다 | 코드 |
| S-R2-10 | `cfe_sb_task.c` L732–L737 | 다른 task의 문맥에서 실행되므로 "Events should use CFE_EVS_SendEventWithAppID() rather than CFE_EVS_SendEvent() to get proper association with TBL task."(원문 그대로) | 직접(설계 의도: 다른 task 문맥의 이벤트를 코어 서비스에 귀속) |
| S-R2-11 | `cmake/mission_defaults.cmake` L25–L40; `cmake/sample_defs/targets.cmake` L79–L82 | "For the CFE core, the order of this list is the order in which they will be initialized". 순서는 config, es, evs, fs, sb, tbl, time(L28–L34). sample_defs는 주석 처리된 예만 있고 바꾸지 않는다 [M] | 직접(순서의 의미) + 설정 |
| S-R2-12 | `modules/es/fsw/src/cfe_es_start.c` L188, L194, L201, L751–L770, L776–L846 | `CreateObjects`(L188) → `CORE_READY`(L194) → `StartApplications`(L201). Phase 1에서 모든 EarlyInit(L751–L770). Phase 2에서 코어 task마다 예약(L803) → `StartAppTask`(L810) → 확정(L818) → `MainTaskSyncDelay(RUNNING)`(L835)을 하고, 시간 초과면 panic(L845) | 코드 |
| S-R2-13 | `cfe_es_task.c` L336, L367, L377, L387, L442, L449, L466–L467; `cfe_evs_task.c` L281, L289, L297, L304, L319 | ES: 등록 → 파이프·구독 둘 → 시작 이벤트 둘 → 판·빌드 정보 이벤트. EVS: 등록 → 파이프·구독 둘 → 시작 이벤트 | 코드 |
| S-R2-14 | `cfe_es_api.c` L738–L765; `cfe_es_apps.c` L559–L596 | `GetAppID`는 문맥의 앱 기록이 없으면 `UNDEFINED`를 저장한다(L759). 새 task는 `STARTUP_SYNC_POLL_MSEC`만큼 지연한 뒤 task 기록만 확인한다(L575–L586) | 코드 |
| S-R2-15 | `modules/cfe_testcase/src/evs_send_test.c` L89–L90, L94–L95, L99–L100, L106–L108 | 기능 시험이 `CFE_ES_APPID_UNDEFINED`에 대해 `CFE_EVS_APP_ILLEGAL_APP_ID`를 기대한다 | 직접(supplier 쪽: 거부는 설계된 API 동작) |
| S-R2-16 | `cfe_tbl_task.c` L124, L196; `cfe_tbl_load.c` L938–L940 | TBL은 등록(L124) 뒤 자기 id를 저장한다(L196). 적재 이벤트를 다른 task 문맥에서 `TableTaskAppId`로 보낸다 | 코드 |

### 3.2 요구 의존과 허용 동작 — 출처가 직접 말하는 것과 연구자 추론

| # | 내용 | 등급 · 근거 |
|---|---|---|
| R-R2-a | **요구.** `CFE_EVS_SendEventWithAppID`의 `AppID`는 등록된 앱이나 코어 서비스에 대응해야 한다. 값이 유효해야 하고, 등록돼 있어야 한다 | **직접**: S-R2-5 L169(caller 쪽 글) |
| R-R2-b | SB가 귀속하는 이벤트의 `AppID`는 `CFE_SB_Global.AppId`다. 그 값은 L138 전에는 0(UNDEFINED)이다 | 직접(S-R2-1, S-R2-3) + 코드(S-R2-7, S-R2-8, S-R2-9) |
| R-R2-c | **요구 의존(순서).** L138의 저장과 L155의 등록이, 어느 task에서든 SB가 귀속하는 이벤트를 보내기 전에 있어야 한다 | 직접(S-R2-3, 기여자의 코드 읽기) + 추론(R-R2-a·b에서). 관리자가 이 순서를 요구로 적은 글은 없다. 이슈 본문의 원인 추정("not been registered")은 코드와 다르다. 0은 값 조건에서 걸려 `ILLEGAL_APP_ID`가 된다. 등록만 빠지면 `EVS_NotRegistered`(L190–L194)다 [I, 코드] |
| R-R2-d | **허용 처리: 없다.** SB가 귀속하는 이벤트를 정의되지 않은 id로 보내거나, 그 거부 반환을 버리는 것을 받아들이는 글은 없다. 헤더는 문맥 밖의 보고에 다른 API(`CFE_ES_WriteToSysLog`)를 가리킨다 | 직접(받아들이는 글 없음 + S-R2-1 라벨 bug + S-R2-2 관리자 수용 + S-R2-3 "exactly the error"). supplier인 EVS의 거부는 설계된 동작이다(S-R2-15). 그래서 결함은 caller(SB 이벤트 경로)의 이른 사용에 있다 [I] |
| R-R2-e | **순서 기전.** 기본 구성에서 ES·EVS task는 SB task가 만들어지기 전에 초기화를 모두 마친다. 그래서 이 사용은 부팅마다 저장 앞에 온다 | **추론(코드)**: S-R2-11(순서), S-R2-12(L835의 직렬화, L845의 panic). 스케줄이 바꿀 수 없는 순서다. `cases/R2.md` §6.1의 "결정적 초기화 의존 결함"과 같다. 출처(이슈·댓글)는 race인지 결정적인지 분류하지 않는다 |
| R-R2-f | 다른 인터리빙의 가능성 | 코드상 셋뿐이다 [I]. (i) 미션이 코어 순서를 바꾼 구성(S-R2-11 "Missions may add/remove/replace"). 연구자가 만들면 구성 변형이다(`cases/R2.md` A10). (ii) S7 창: SB의 L138이 ES의 L818 확정 전에 돌면 `GetAppID`가 실패해 UNDEFINED가 실행 내내 남는다(S-R2-14). 보고된 적이 없다. (iii) 코어 시작 시간 초과 → panic. 결과가 다르다 |
| R-R2-g | 저장 뒤의 사용 | SB 자신: 프로그램 순서 L138 → L155 → L183…(코드). SB 뒤에 만드는 코어 task(tbl, time)와 앱(`CORE_READY` L194 → L201 뒤): S-R2-12에 따라 저장·등록 뒤 [I, 코드]. TBL의 id도 같은 구조다(S-R2-16) |

### 3.3 실행에서의 대상: task · 상태 · 순서 · 그것을 보이는 사건

- **실행 task**: 사용은 ES 주 task(`CFE_ES`)와 EVS 주 task(`CFE_EVS`)의 초기화 문맥에서 일어난다. 저장(provider)은 SB 주 task(`CFE_SB`)의 L138이다. 생성 실행자는 ES 시작 스레드의 `CFE_ES_CreateObjects`다(S-R2-12).
- **정체성**(동결 표 §2의 네 범주): 실행 task와 소유 앱은 ES·EVS다. 전달 AppID는 0이다. 그 값이 가리켜야 할 앱은 SB인데, 아직 저장되지 않았다. 코어 앱끼리의 관계다(S-R2-3 "core app's init").
- **상태**: 셀 `CFE_SB_Global.AppId`(0 → L138에서 SB의 id), EVS 등록 슬롯 `evs.reg(AppID)`(SB는 L155).
- **그것을 보이는 사건**:
  - 사용 지점 `CFE_EVS_SendEventWithAppID`의 진입(EventID, 인자 AppID, 반환 주소로 SB 호출 지점)과 반환(상태).
  - SB의 L138 저장 뒤 지점과 L155 등록.
  - 코어 task 생성 요청(어느 앱인지)과 `MainTaskSyncDelay` 반환.
  - 사용 지점에서 직접 포획한 AppID 0이 "저장 전"을 값으로 보인다. 저장 사건이 뒤에 오면 "늦은 provider"다.

### 3.4 발생 부류별 기대 판정과 기록된 Kairos 출력의 대조

Kairos 열은 [기록]이다. 출처는 `plan/results_pilot_20261025.md` §1(R2-1–R2-4), `plan/results_w4w5_20261028.md` §2.2–§2.4다. 네 기록은 보존 F0 `v153_pilot3`, 보존 G-I d4, 새 F0 `f0b`, 새 G-I `gi`다.

| 부류 | (가) 출처 기준 부류 | (나) 기대 상태 → 요약 | 등급 | 동결 행 | Kairos [기록] | 대조 |
|---|---|---|---|---|---|---|
| **R1 ES task의 AppID 0 사용**(SB 귀속 이벤트, L138 전) | 미충족: 값 0은 유효하지 않다(R-R2-a·b). 허용 없음(R-R2-d) | VIOLATING → `OBSERVED_VIOLATION` | 직접+코드 | E-14 (U-R2-1) | ES 20. cur·p1off·nocode 모두 `OBSERVED_VIOLATION`. direct-capture만, 결과 `0xc2000003`. 네 기록 같음 | **일치.** 반환 코드 없이 선다 |
| **R2 EVS task의 AppID 0 사용** | 같음 | 같음 | 직접+코드 | E-14 (U-R2-2) | EVS 4, 같은 판정·근거 | **일치** |
| 사용 수 | 코드에서 세면 EVS = 파이프 1 + 구독 2 + 시작 이벤트의 NO_SUBS 1 = **4**. ES = 3 + 시작 이벤트 둘의 NO_SUBS + 판·빌드 정보 이벤트마다 NO_SUBS. 성공 경로와 구독자 없음(`CORE_READY` 전)을 전제한다 [I]. EventID는 5, 10, 14 | ES 수는 빌드의 모듈 구성에 달려 출처에서 정해지지 않는다(`cases/R2.md` §5 하한) | 추론(코드) | E-14 근거 열("사용 수는 출처가 정하지 않는다") | EVS 4, ES 20, EventID 5·10·14 | EVS 일치. ES는 하한(≥5)과 모순되지 않는다. 24는 결함 구조 하나에 속한 호출 수이고 독립 버그 수가 아니다(IP §2.2) |
| **R3 코어–앱 관계와 귀속** | 실행 task ES/EVS(코어), 전달 AppID는 SB(코어)의 저장 전 값, provider는 SB L138 | 앱 사이 결과는 귀속이 섰을 때만 센다 | 직접(S-R2-1 "ES and the EVS tasks", S-R2-3) + 코드 | U-R2-1·2(소유 앱 기대: `inferred`) | 소유 앱 confirmed(생성 측 근거). 관계 `core-application`, provider CFE_SB(결정 뒤의 늦은 provider). 네 기록 같음 | 일치. 귀속 근거가 기대보다 강했을 뿐 라벨과 무관하다 |
| **R4 고정 초기화 순서 대 다른 인터리빙** | 기본 구성에서 사용-저장 순서는 강제된다(R-R2-e). 같은 구성에서 같은 순번의 좋은 증거는 나올 수 없다 [I]. 다른 인터리빙은 R-R2-f의 셋뿐이다 | 스케줄 race로 주장하지 않는다. "초기화 순서가 강제한 미충족"으로 적는다 | 추론(코드) | E-14("schedule dependence undetermined") | 순서 의존을 주장하지 않았다. 같은 순번의 좋은 증거 없음. 코어 직렬화 규칙 R19는 실제 기록에서 서지 않았다(`results_pilot` §2 E-15) | 모순 없음. 출처 기준 기대("강제된 순서")가 Kairos("undetermined")보다 강하다. 실행 기록만으로 "강제됨"을 세우지 못하는 것은 도구의 한계로 남는다(§4 V-R2-4) |
| **R5 SB 자기 전송**(SB task, L138·L155 뒤) | 충족(프로그램 순서, R-R2-g) | READY → `NO_VIOLATION…` | 코드 | E-15 (U-R2-3) | 5. 세 읽기 모두 `NO_VIOLATION…`. DC + SO, 가정 A-OWNER, 간선 `R2-P-TASK`, 같은 앱 | 일치. 코드에서 세면 파이프 1 + 구독 3 + 시작 이벤트의 NO_SUBS 1 = **5**(S-R2-8) [I]. 기록 5와 같다. 근거는 trusted 가정 A-OWNER에 기댄다(§4 V-R2-2) |
| **R6 다른 task의 SB AppID(0x110003) 사용** | 충족: TBL·TIME은 SB 뒤에 만들어지고, 앱은 `CORE_READY` 뒤에 시작한다(R-R2-g) | READY → `NO_VIOLATION…` | 추론(코드) | E-15: 등록 대상·수명·순서 근거가 없으면 `INCONCLUSIVE(start_state_unknown)` 또는 `(effect_unsupported)` | 보존 F0 212 / 새 F0 211 / 보존 G-I 215 / 새 G-I 150. cur `NO_VIOLATION…`(direct-capture + 반환 코드 좁힘, 독립 아님). p1off = nocode `INCONCLUSIVE(explanation_ambiguous)` | 출처 기준 부류와 cur 라벨은 같다. 그러나 근거가 반환 코드(오라클 자신의 결과)뿐이라 **독립 판정 불가**다. 동결 E-15와는 이유 이름이 다르다. 이미 보고됐다(`results_pilot` §2 E-15) |
| **R7 다른 task의 TBL AppID(0x110004) 사용** | 충족: TBL 등록 L124 → 저장 L196이 TBL 초기화 안, `RUNNING` 전이다. 사용은 그 뒤다 | READY | 추론(코드) | E-15 | 28개(네 기록 같음). R6과 같은 판정·근거 | R6과 같음 |
| **R8 OPERATIONAL 뒤·종료 중의 사용**(예: 새 F0의 TO_LAB EID 22·25가 명령 재시작 뒤에 기록됨) | R2 출처의 범위 밖이다. 이슈는 시작 구간만 다룬다 | **미정.** 구간을 나누기 전에는 정하지 않는다(IP §7 실행 지침) | 미정 | 없음(E-15가 함께 묶음) | "다른 task 사용"에 섞여 있다(EID 14·22·25가 실행마다 다름) | 판단 불가. 이 호출들을 정상 대조로 세기 전에 구간을 분리해야 한다 |
| R9 S7 창(SB의 `GetAppID` 실패 → 실행 내내 0) | 코드상 가능, 보고 없음(R-R2-f (ii)) | 일어나면 뒤의 SB 귀속 사용이 모두 R-R2-a를 어긴다. 다만 원인은 다른 순서(ES 확정 L818 대 SB L138)다 | 추론 | 없음 | 일어나지 않았다(다른 task의 AppID가 모두 `0x110003`) | 판단 불가. 실현되지 않았고, 이 창을 만드는 조건을 정하지 않는다(`cases/R2.md` A9, D5) |
| R10 SB를 먼저 두는 구성 변형 | 구성 변형(R-R2-f (i)) | 만들면 ES·EVS 사용이 READY일 것이다. 원 구성에 race가 있다는 근거는 아니다 | 추론 | 없음 | 실행하지 않았다 | 해당 없음. 조건을 정하지 않는다(`cases/R2.md` A10, 로드맵 §6.4) |

### 3.5 오경보 대조로 쓸 수 있는 정상·허용 경로

| ID | 경로 | 출처 근거 | [기록]의 결과 | 대조의 힘 |
|---|---|---|---|---|
| N-R2-1 | SB 자기 전송 5(R5) | 코드(프로그램 순서) | 위반 없음. DC + SO | 보통. 같은 API·같은 SB 호출 지점에서 위반(R1)과 정상(R5)이 값 하나로 갈린다. 근거가 A-OWNER에 기댄다 |
| N-R2-2 | 다른 task의 사용(R6·R7) | 추론(코드) | 위반 없음. cur READY, p1off·nocode INCONCLUSIVE | **약함.** 오경보가 나지 않은 것은 맞다. 그러나 READY가 반환 코드 0에서 나오므로, 판정 규칙의 오경보 저항을 독립으로 보이지는 않는다 |
| N-R2-3 | OPERATIONAL 뒤의 사용(R8) | 미정 | 위반 없음 | 쓰지 않는다. 구간 분리 전이다 |

### 3.6 출처로 정할 수 없는 것 (미정으로 둔다)

1. ES 사용 수의 정확한 값(판·빌드 정보 이벤트 수). 하한만 정해진다.
2. OPERATIONAL 뒤·종료 중의 사용(R8). R2 출처가 다루지 않는다.
3. S7 창이 이 환경에서 일어날 수 있는지(R9).
4. 이슈가 보고한 판 B에서 ES가 이 경로에 닿는지(Q1). R2 기준 밖이다.
5. 의도된 수정의 의미(SB의 이른 이벤트를 SB에 귀속할지, 버릴지, syslog로 보낼지; `cases/R2.md` Q5). R1·R2의 기대 판정은 R-R2-a(직접)로 서므로 이 미정에 기대지 않는다.

## 4. 자기 검증 위험 — 프로필이 출처에 없는 것을 담은 곳

프로필은 정답의 출처가 아니다. 아래는 기대 판정과 Kairos 출력이 같은 이유가 출처가 아니라 프로필(또는 SPEC 규칙)일 수 있는 곳이다. "출처 상태"는 §2.2·§3.2의 등급이다.

| ID | 프로필 위치 | 담은 것 | 출처 상태 | 판정에 미치는 영향 | 위험 |
|---|---|---|---|---|---|
| V-C1-1 | `c1-bp-b051eb0/rspec-833fdbb/40_rspec.kzp` L4–L8(주석), L20–L22(`allowed`에 받아들이는 종료 항목 없음) | 25회 소진은 REJECT, 그래서 VIOLATING | 요구 미충족은 직접+코드. REJECT는 **모형**(R-C1-g) | E-06과 Kairos의 일치는 같은 모형을 두 번 쓴 결과다 | 중 |
| V-C1-2 | `rspec-281a941/40_rspec.kzp` L19 `allowed unspecified_by_reference` | 수정 전 처리의 허용 출처 없음 | **미정**(P3). 거부 쪽의 글(#184 "shouldn't be a hard error")이 있지만 이 판에 묶지 않았다 | E-03과 Kairos의 일치는 구성상 그렇다. 라벨이 출처로 고정된 것이 아니다 | 중(라벨을 결과로 쓸 때) |
| V-C1-3 | `10_kinds.kzp` L97–L101 `role bp-existence-probe` | BP의 조회를 요구의 사례에서 뺀다 | 코드+추론(R-C1-h). "요구 없음"이라는 글은 없다 | K7·N-C1-3의 "오경보 없음"은 이 선언 때문이다. 엔진이 구분한 것이 아니다 | 낮음–중 |
| V-C1-4 | `02_researcher.kzp` L17–L22 `r_countsem_by_name`; `40_rspec.kzp` L17 `requires … = LIVE` | 이름 키, 두 상태(ABSENT/LIVE), RESERVED 구간 모형 없음 | 추론(R-C1-b) | A5가 범위 밖이므로 현행 부류에는 영향 없음 | 낮음 |
| V-C1-5 | `02_researcher.kzp` L31–L36 `r_reads_from_lookup` | 성공 조회의 id → 생성 확정에서 읽음(RF) | 코드(OSAL) 기반의 손 입력 | K1·K3 READY의 SO+RD. nocode F0에서는 INCONCLUSIVE | 낮음 |
| V-C1-6 | `c1-bp-b051eb0/20_ops.kzp` L36(`outcome OS_ERR_NAME_NOT_FOUND: not_ready(exists)`, 상수는 `05_consts.kzp` L13)의 결과 좁힘(P1) | 실패 시도의 미충족을 반환 코드로 세운다 | 반환값의 뜻은 직접(S-C1-9). 다만 오라클 자신의 결과다 | 자연 실행의 K2·K3 미충족은 반환 코드 없이 서지 않는다(nocode INCONCLUSIVE) | 근거 독립성의 공백 |
| V-R2-1 | `cfs-v7.0.1/40_rspec.kzp` L21, L23–L24 | `valid && REG`, `appreq evs_h_appid_note`, `allowed none` | **직접**(S-R2-5 L169). `none`은 받아들이는 글이 없음 + 이슈·CCB(R-R2-d) | R1·R2 판정의 출처가 프로필 밖에 있다 | 낮음 |
| V-R2-2 | `50_trusted.kzp` L16–L21 `A-OWNER`; `02_researcher.kzp` L66–L71 | 등록 슬롯의 대상 = 쓴 task의 생성 id | 코드와 맞다 [I](`CFE_EVS_Register`는 호출 문맥의 앱을 등록한다). 도구가 확인하지 않는 trusted 가정이다 | R5(SB 자기 전송)의 READY. A-OWNER를 끄면 ON(P1 켬) 또는 INCONCLUSIVE(P1 끔)(`results_pilot` §1 R2-3) | 낮음–중 |
| V-R2-3 | `02_researcher.kzp` L38–L43 `r_evs_reg_key_by_id` | 등록을 정확한 AppId 값으로 키잡음. 세대(재시작)는 모형 없음 | 추론 | 시작 구간 R1–R7에는 영향 없음. 재시작 뒤의 사용에는 맞지 않을 수 있다 | 낮음(범위 밖) |
| V-R2-4 | `30_hb.kzp` L51–L64 `R19`(guard `cfe >= 6.4.2`, 손 입력 `r_guard_core_serial` L17–L22 "lower bound was NOT re-verified") | 코어 직렬화 HB | 코드(S-R2-12). guard 하한은 확인되지 않았다 | R19가 실제 기록에서 서지 않아, R4의 "강제된 순서"와 R6·R7의 "저장 뒤"를 도구가 세우지 못한다. 그래서 R6·R7은 반환 코드에만 기댄다 | 중(근거 공백. 거짓 요구는 아님) |
| V-R2-5 | `20_ops.kzp` L25–L33 `outcome CFE_SUCCESS: ready narrow` | 반환 0이면 검사 시점에 유효·등록 | 코드(S-R2-6). 오라클 자신의 결과다 | R6·R7의 cur READY 전부 | 근거 독립성의 공백 |
| V-R2-6 | `05_consts.kzp` `CFE_ES_APPID_BASE = 0x110000` | SIMPLE id 방식 | 빌드 성질. 값 0의 무효는 두 방식 모두에서 같다(`cases/R2.md` §4) | 영향 없음 | 낮음 |

출처가 직접 뒷받침해 위험으로 세지 않은 프로필 항목:
- C1 `role cf-dependent-lookup`(`10_kinds.kzp` L86–L90): S-C1-1, S-C1-3, S-C1-4.
- C1 `retry_until_ready`가 앞선 실패를 받아들이는 부분(`rspec-833fdbb` L21): S-C1-6 주석, S-C1-7. 상한 25: `cf_platform_cfg.h` L338.
- R2 `requires`(V-R2-1).

## 5. 동결 기대표(E-01–E-15)와의 대조 요약

| 행 | 이 문서의 출처 기준 기대와의 관계 | 비고 |
|---|---|---|
| E-01, E-04 | 일치(K1) | E-01은 자연 실행에서 실현되지 않았다[기록] |
| E-02, E-07, E-08, E-09, E-12 | 일치 | — |
| E-03 | 라벨 일치. 출처 기준으로는 "미충족, 허용 **미정**" | P3가 정해지면 바뀔 수 있다 |
| E-05 | 일치(허용 출처 직접) | 자연 실행에서 매번 났다[기록]. 오경보 대조로 가장 강하다 |
| E-06 | 라벨 일치. VIOLATING이 **모형**임을 E-06의 해석 주의(2026-10-25)가 이미 적는다 | — |
| E-10, E-13 | 기록 없음 | — |
| E-11 | 일치. 원인이 역할 선언이다(V-C1-3) | — |
| E-14 | 일치. 출처 기준 기대가 순서를 "강제됨"으로 더 강하게 적는다(R4) | 표의 "schedule dependence undetermined"와 모순은 없다 |
| E-15 | **층이 다르다.** E-15는 관측 조건부 도구 출력을 적는다. 이 문서의 R5–R7은 출처 기준 부류(READY)를 적는다. R8(종료 구간)은 E-15에 행이 없다 | 개정하려면 결과를 본 뒤의 개정이므로 PI가 정한다(§6) |

동결 표의 U-행 정의(실행 task, 생성 실행자, 소유 앱, 전달 AppID, 발생 위치)와 이 문서의 §2.3·§3.3 대상은 같다. 줄 번호는 이번 사본에서 다시 읽은 값과 같다. 단 하나, U-R2-1의 `cfe_evs.c:185–189`는 NULL 분기만 가리키고, 이 문서는 등록 불일치 분기까지 넣어 L185–L194로 적었다.

## 6. 이 문서가 하지 않은 것, 이탈, 알릴 것

- **하지 않은 것**: cFS 빌드·기동, 실험 조건 설정, 동결 표·사례 파일·프로필·결과 문서의 수정, 판정 라벨의 선택. 출처가 정하지 않는 것은 `미정`으로 두었다.
- **레퍼런스 이탈**: 조건 이탈은 없다. 방법상의 제한 둘이 있다. (1) 이슈 글은 모형을 거친 WebFetch 추출의 짧은 문구다. 논문에 인용하기 전에 원문 대조가 필요하다. (2) REST API는 이 세션에서 403이다.
- **PI·책임 연구자가 알거나 정할 것**:
  1. P3/Q-12(수정 전 단일 실패의 허용)는 출처로 정해지지 않는다. #184 본문에 거부 쪽의 글이 있고, 수정의 부모라는 점으로 `281a941`에 이어진다 [I]. 묶을지는 PI 결정이다. 그때까지 C1 수정 전 결과를 위반으로 쓰지 않는다.
  2. C1 E-06의 VIOLATING과 Kairos의 일치는 독립 확인이 아니다(V-C1-1). 논문 표현은 "채택한 요구·허용 모형의 판정"으로 한정해야 한다(E-06 해석 주의와 같다).
  3. R2 E-15는 관측 조건부 출력을 적고, 출처 기준 부류(READY)를 따로 적지 않는다. 또 OPERATIONAL 뒤·종료 구간 행이 없다. 두 층을 나눈 행을 둘지, 종료 구간을 따로 둘지(IP §7)는 PI 몫이다. 결과를 본 뒤의 개정이면 그 실행은 개발 자료로 옮긴다.
  4. R2 다른 task 사용과 C1 BP 존재 확인은 오경보 대조로 약하다(반환 코드, 역할 선언). 오경보 저항을 보이려면 반환 코드 없이 서는 근거(R2: 코어 직렬화 HB R19의 실제 기록 성립)나 역할 선언에 기대지 않는 대조가 필요하다. 이 문서는 그 조건을 정하지 않는다.
  5. R2 사용 수는 EVS 4, SB 자기 전송 5까지 코드로 정확히 세어진다. ES 20은 출처로 정해지지 않는다. 재검증에서 수를 기대값으로 쓰려면 EVS와 SB만 쓸 수 있다(PD28 Q4: 사건 수의 완전 일치는 요구하지 않는다).
  6. CF#184 댓글 앵커 둘과 댓글 1의 다른 문구(§1.2)는 `cases/C1.md` §0a에 아직 없다. 그 파일 담당이 반영할 일이다.
