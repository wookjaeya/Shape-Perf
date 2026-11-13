# C5 기대 결과표 — 6.4.1·6.4.2 자연 기동 전 동결 (2026-10-29)

- 성격: **출처에서 도출한 기대값만** 적는다. 실행·관측 결과는 없다. 이 표를 쓰면서 cFS를 빌드·기동하지 않았고, C5 기록을 재생하지 않았다. C5 재생 결과는 아직 없다.
- 입력: `cases/C5.md`(F1, F4–F9, F13, §4–§7, §10, §11 A4–A8), `plan/c5_execution_basis_20261028.md`(이하 CB; §3.4, §3.5 메모, §4–§7, §8.3–§8.4, §9, §10.1–§10.5), `plan/c5_queue_inventory_20261029.md`(이하 QI), `Architecture/spec/PI_DECISIONS_RESOLVED_20261028.md`(PD28) §2 항목 6, `Architecture/spec/PI_DECISIONS_RESOLVED_20261029.md`(PD29) D1–D5·A3·A7·N-C5-1, SPEC rev 1.3 §1.6·§1.10. 두 tarball 트리의 원문은 이번에 다시 읽었다(줄 번호는 그 트리의 것).
- 동결: 이 파일은 C5 재생 단계(PD29 §4 단계 5의 기록 재생)보다 먼저, 따로 커밋한다. 근거: 마스터플랜(2026-10-24) §12.1·§12.4, 결정 P-F1GATE(`Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md` §3.7) [D].
- 바꾸는 규칙: 동결 뒤에 행을 바꾸면 §5에 날짜, 이유, 이전 값을 적는다. 결과를 본 뒤 고친 행이 있으면 그 실행은 개발 자료로 옮기고 새 평가를 한다(마스터플랜 §12.4).
- 표기: [F] 해시를 적은 tarball 트리의 원문에서 읽음, [M] 이번 작업에서 도구로 얻음, [I] 추론, [D] 결정, [U] 미확인, [기록] 2026-10-28 기록(원 기록은 컨테이너 교체로 없어짐, CB 머리말). 값을 정하는 레퍼런스가 없으면 `unspecified_by_reference`로 두고 고르지 않는다(`CONDITIONS_POLICY.md`).
- 결과 단위: **고정 구성 × 자원(키) × 호출 문맥 × 세대 × 발생 위치**(마스터플랜 §12.1). 6.4.1과 6.4.2의 발생을 같은 단위로 합치지 않는다. 제어로 얻은 순서는 자연 수에 합치지 않는다(D4). 이번 동결에는 제어 실행이 없다.
- 판정 어휘: SPEC rev 1.3의 발생 상태(§1.6)와 요약 뷰(§1.10). `INCONCLUSIVE`의 이유는 §4.8 "Reasons for withholding"의 이름만 쓴다. 판정 단위는 **순서위반**이다(PD28 §2 항목 6, CB §4). Kairos의 상태 라벨과 판정 단위는 따로 적는다(아래 §3 열 설명).
- 기대값을 출처에서 도출할 수 없으면 "미정"으로 적고 이유를 붙인다. "도달하지 않음"도 기대값으로 적는다.

## 1. 고정 구성

| ID | 구성 | 출처 | 아직 고정되지 않은 것 (`unspecified_by_reference`, 실행마다 기록) |
|---|---|---|---|
| K-C5-641 | cFE 6.4.1 tarball(sha256 `ec26e33b…`) + OSAL 4.1.1(`8f0f2b23…`, `osal/`로 옮김, DV-1) + PSP pc-linux 1.2.0.0, cpu1, `-m32`. 계측 파생 빌드(CB §8.4): `core-linux.bin` `ad217232…`, `sample_app.so` `4d62fc3c…`, manifest `sites-641.draft.kzs`(runtime 기록 `6bd88dcb…`), 32비트 런타임 `kzrt 0.4.0`과 같은 빌드의 32비트 `kz-rt-dump`(`74b25512…`). 현대 호스트 파생 구성(D2). root·실시간 우선순위(N-C5-1). 별도 IPC namespace에서 `fs.mqueue.msg_max` 64, `msgsize_max`·`queues_max`는 namespace 기본값, 전역 10은 그대로(D1, QI §7). `RLIMIT_MSGQUEUE` 바꾸지 않음(A3). 시작 `--reset PO` 또는 인자 없음(PO; CB §3.4). 시작 스크립트는 tarball 그대로(`cfe_es_startup.scr` L1–L5). 자연 실행, 보류 없음(D4). 부팅 1회(P8/A7). 회수는 `KZ_RT_STORE` 저장소와 32비트 `kz-rt-dump`(CB §9 항목 2–3, D5) | `cases/C5.md` §3, §10 행 1·4·17–19·21; CB §1–§3, §8.4, §9; PD29 | CPU 수·affinity(이 컨테이너 4 CPU, `f`; 고르지 않고 기록만), 실행 길이와 SIGINT를 보내는 시점, 하네스 시간 한도, 저장소 경로(`/dev/shm` 아래, ES §12.4), 커널(`6.18.44-fc-v185`, A8) |
| K-C5-642 | cFE 6.4.2 tarball(`c64ed2aa…`), 나머지는 K-C5-641과 같음. `core-linux.bin` `d17661a8…`, `sample_app.so` `ff7b4d4a…`, manifest `sites-642.draft.kzs`(`1d17d92d…`) | 같음; `cases/C5.md` §8 | 같음 |

- **Φ 프로필 인스턴스** [M]: `kairos/profiles/c5-cfe-6.4.x` + `cfe-6.4.1/`(profile hash `38a6db1bcdb849c9677628fa61af8d6047a979e0363c85dc827c285c04126c2e`), + `cfe-6.4.2/`(`d92989a03cb21effe4f5224adf27ea906b0f70a72894bb8dbbc9ea8ccbb95303`). 재생은 이 해시의 프로필을 쓴다. 프로필을 바꾸면 §5에 적는다.
- **보고 문구**: "원 소스 기반 현대 호스트 파생 구성, root·실시간 우선순위"(N-C5-1). 원 플랫폼의 스케줄이라고 쓰지 않는다. 계측 경로의 결과이며 F0 경로와 같다고 주장하지 않는다(D3). 계측하지 않은 빌드의 창 빈도와도 같다고 주장하지 않는다(hook 비용, CB §6 항목 3).
- **세지 않는 실행**: 실시간 정책이 task에 실제로 적용되지 않은 실행(`/proc/<pid>/task/*/sched`·`stat`), 사례 경로(`CFE_ES_AppCreate`)에 이르지 못한 실행은 기록만 하고 이 구성의 결과로 세지 않는다(N-C5-1; E-01).

## 2. 결과 단위

정체성은 네 범주를 따로 적는다: **실행 task / 생성 실행자 / 소유(생성) 앱 / 돌려받은 AppID**. 서로 같다고 가정하지 않는다. 근거 수준(`confirmed`·`inferred`·`undetermined`)은 관측 뒤에 정해지므로 여기서는 쓸 근거만 적는다.

| 단위 | 구성 | 자원(키)·세대 | 실행 task | 생성 실행자 | 소유(생성) 앱 | 돌려받은 AppID | 발생 위치 [F] | 요구·허용 처리와 출처 |
|---|---|---|---|---|---|---|---|---|
| U-C5-1 | K-C5-641 | `es.task(T의 TaskId)`, 부팅 1회의 첫 세대(PO 시작에서 `CFE_ES_Main` L213–L216이 모든 칸을 비움) | 시작 스크립트 앱의 주 task T: SAMPLE_APP, CI_LAB_APP, TO_LAB_APP, SCH_LAB_APP(스크립트 L2–L5) | 초기 스레드에서 도는 `CFE_ES_Main`(F8). 근거: OSAL `OS_TaskCreate`의 spawn 기록(site 13·16·17)과 T의 `OS_TaskRegister` 알림(site 20)이 같은 id | T를 만든 `CFE_ES_AppCreate`의 AppTable 칸 `i`(E-ID1: site 33 L657, 대조 site 8 L651, 요청 반환 site 2) | 성공이면 `TaskTable[TaskId].AppId`(site 25 L863) | T의 첫 잠금 보호 조회: `CFE_ES_GetAppID`(L846–L871; site 23–26). SAMPLE_APP·CI·TO는 `CFE_EVS_Register` → `EVS_GetAppID` L127, SCH는 `CFE_SB_CreatePipe` L224 | 요구: 조회의 LP(L856 뒤)에서 T의 칸이 등록됨(`cases/C5.md` §7; 프로필 `K-SELF.es-task`). `appreq`·caller 수준 Allowed: `unspecified_by_reference`(아래 §4) |
| U-C5-2 | K-C5-641 | 같음 | 같은 T | 같음 | 같음 | 같음 | T의 둘째 이후 조회: `CFE_SB_CreatePipe`(역할 문맥 `sb-create-pipe-dependent-lookup`), `CFE_SB_Subscribe`, `CFE_EVS_SendEvent`, `CFE_ES_RunLoop`의 `CFE_ES_GetAppIDInternal`(L551, L1873–L1893) | 같음(`K-SELF.es-task`, `K-SELF.es-task-internal`) |
| U-C5-3 | K-C5-641 | `es.task`(SAMPLE_LIB task의 TaskId) | SAMPLE_LIB task(스크립트 L1, 진입 `SAMPLE_LibInit`, 우선순위 0) | `CFE_ES_Main` | `CFE_ES_AppCreate`의 칸 | — | 없음: `SAMPLE_LibInit`(`sample_lib.c` L32–L43)은 `OS_printf` 뒤 돌아온다. `OS_TaskRegister`도 조회도 없다 | — |
| U-C5-4 | K-C5-641 | `es.task`(코어 task·자식 task의 TaskId) | 코어 앱 task(ES·EVS·SB·TIME·TBL)와 TIME의 자식 task 둘(`cfe_time_task.c` L280, L294) | 코어: `CFE_ES_Main`(`CFE_ES_CreateObjects`). 자식: TIME task(`CFE_ES_CreateChildTask`) | 코어·자식 모두 이 manifest에 요청 reader가 없음(`CreateObjects`·`CreateChildTask`에 site 없음) | 성공이면 그 앱의 칸 | 각 task의 조회. 코어 task는 모두 `CFE_ES_RegisterApp`을 먼저 부른다(예: `cfe_evs_task.c` L377, L385; `cfe_sb_task.c` L190, L198) | 같은 요구 |
| U-C5-5 | K-C5-641 | `es.task(0)` | 초기 스레드(등록하지 않은 스레드, `OS_TaskGetId` = 0) | — | — | — | 시작 경로에서 기대하지 않음 [I: `cfe_es_start.c`·`cfe_es_apps.c`에 조회 호출 없음] | 사례 밖 |
| U-C5-6…10 | K-C5-642 | U-C5-1…5와 같은 대응 | 같음 | 같음 | 같음 | 같음 | 6.4.2 줄: `CFE_ES_GetAppID` L785–L810, 내부 L1812–L1832 | 같음 |

## 3. 기대 결과

열 설명. "Kairos 판정"은 프로필 `c5-cfe-6.4.x`의 발생 상태 → 요약 값이다(SPEC 규칙을 이 프로필에 적용한 것). "판정 단위(순서위반)"는 PD28 §2 항목 6의 단위이고, Kairos 상태와 따로 센다. "최종 결과"는 O1(`CFE_ES_ERR_APPID`), O2(잘못된 AppID의 `CFE_SUCCESS`), O3(소비: EVS 등록 건너뜀, AppId `0xFFFFFFFF`의 SB 파이프, `RunLoop` FALSE)이고 판정과 따로 기록한다(CB §4). 한 열의 값이 다른 열로 옮겨 가지 않는다.

| 행 | 단위 | 구간·발생(키) | Kairos 판정(상태 → 요약) | 판정 단위(순서위반) | 최종 결과 | 관측 부족일 때 | 근거 / 미정 이유 | 반박 조건 |
|---|---|---|---|---|---|---|---|---|
| E-01 | K-C5-641, K-C5-642 | 경로 도달: 시작 스크립트의 `CFE_APP` 다섯 줄이 `CFE_ES_AppCreate`(site 1)에 이름 | 판정 아님 | — | — | 저장소를 회수하지 못하면 모든 행이 "관측 없음". 그 실행은 세지 않는다 | **도달 기대** [I]: 큐 10개의 최대 깊이 64 ≤ namespace `msg_max` 64, 바이트 24,400 B ≤ 819,200 B(QI §0, §5, §8). 10-28 smoke의 멈춤(EVS 파이프 32 > 10, CB §3.2)은 이 값으로 생기지 않는다 | site 1이 다섯 번 미만; `CreatePipeErr`·`Application Init Failed` syslog; `PROCESSOR RESET`으로 1 s 안에 끝남(CB §3.2와 같은 끝) |
| E-02 | K-C5-641, K-C5-642 | N-C5-1 적용: 앱 task의 스케줄 정책 | 판정 아님 | — | — | 정책을 읽지 못하면 그 실행은 세지 않는다 | **적용 기대** [F][I]: root면 OSAL이 task를 `PTHREAD_EXPLICIT_SCHED`·SCHED_FIFO로 만든다(`osapi.c` L587–L640), 생성자 스레드는 PSP가 SCHED_FIFO 최대로 올린다(`cfe_psp_start.c` L356–L357). CAP_SYS_NICE 있음, `sched_rt_runtime_us` 950000 [M, CB §3.5 메모] | `/proc/<pid>/task/*/sched`에서 앱 task가 SCHED_OTHER |
| E-03 | U-C5-1 | 각 T의 첫 잠금 보호 조회, `(T, 1)`, 키 `es.task(TaskId_T)` | **미정**: 둘 중 하나. (가) 코드 `CFE_SUCCESS` → READY → `NO_VIOLATION_IN_CHECKED_PREFIX`, 근거 `outcome-narrowed`. (나) 코드 `CFE_ES_ERR_APPID` → NOT_READY_UNJUDGED → `INCONCLUSIVE(caller_requirement_absent)`, 근거 `outcome-narrowed` | (가) 아니오. (나) **예**: 단, 같은 기록에서 T의 site 24 값 = T의 site 20 값 = 생성자의 site 16 값이고, 생성자의 site 9(L652) 저장 값이 같은 TaskId일 때. 그 칸을 FREE→LIVE로 바꾸는 것은 시작 경로에서 그 저장뿐이다 [F: 쓰기 목록, 프로필 20_ops] | (가) 없음. (나) O1. O3은 E-06 | site 24·26을 읽지 못하면 발생이 결정되지 않음: OPEN 또는 `INCONCLUSIVE(effect_point_ambiguous)` | 6.4.1은 L610에서 잠금을 풀고 L614에서 task를 만든 뒤 L637에서 다시 잡는다(F4). T는 `pthread_create`(`osapi.c` L645) 뒤 바로 돌 수 있고, 첫 ES 호출 `CFE_ES_RegisterApp`(L771)과 첫 조회(L851)가 생성자의 L637보다 먼저인지는 스케줄이 정한다(`cases/C5.md` §6 추론, CB §7.2 항목 2). CPU 4개에서 SCHED_FIFO 최대인 생성자와 새 task가 동시에 돌 수 있다 [I]. 어느 쪽인지, 몇 task인지는 출처가 정하지 않는다(`cases/C5.md` §10 행 9–11, 13). 위반의 존재를 예측값으로 넣지 않는다(CB §10.1). 실패 결과가 O1이고 O2가 아닌 것: OSAL 4.1.1과 glibc 원 소스의 순서(CB §7.1, `cases/C5.md` A8 갱신) | (가)인데 site 25 값 ≠ 그 T의 site 33 값 `i`(O2: E-08). (나)인데 site 24 값 ≠ site 20 값, 또는 site 9가 그 TaskId를 쓴 적이 없음. 목록에 없는 반환 코드(`unmapped`) |
| E-04 | U-C5-2 | T의 둘째 이후 조회 `(T, k ≥ 2)`, `GetAppID`·`GetAppIDInternal` | 생성자의 등록(site 9) 뒤의 조회: READY → `NO_VIOLATION_IN_CHECKED_PREFIX`(`outcome-narrowed`). E-03이 (나)인 task에서 등록 전의 조회가 더 있으면 그 조회도 E-03 (나)와 같은 라벨 | E-03 (나)의 조건을 그 조회마다 적용 | 등록 전의 조회만 O1 | E-03과 같음 | 칸은 한 번 LIVE가 되면 시작 경로에서 FREE로 돌아가지 않는다(해제는 `CFE_ES_DeleteChildTask`·`ExitChildTask`·`CleanupTaskResources`뿐, 시작 경로에 없음) [F][I]. 수는 실행 길이(`unspecified_by_reference`)와 이벤트 수가 정한다. 세지 않는다 | site 9 뒤의 조회가 `CFE_ES_ERR_APPID` |
| E-05 | U-C5-2 | `CFE_SB_CreatePipe` 안의 조회(역할 문맥) | E-03·E-04의 라벨에 문맥 `sb-create-pipe-dependent-lookup`이 붙고 가정 `assumes-role`. 라벨은 바뀌지 않는다(`dependent_use`는 발생을 빼지 않음) | 같음 | 같음 | site 30을 못 보면 문맥 없음(발생은 그대로) | 프로필 10_kinds의 역할; SPEC 1.6 "Call contexts" | 역할 사이트 앞 사건이 `GetAppID` 반환이 아님(발견 사항) |
| E-06 | U-C5-1·2 | O3: 소비 결과(E-03 (나)가 있을 때만) | 판정 아님(따로 보고) | — | (나)의 조회가 `EVS_GetAppID` 안이면 EVS 등록 건너뜀(site 31 status ≠ 0; `cfe_evs.c` L105–L107). `CFE_SB_CreatePipe` 안이면 status ≠ 0인 채 AppId `0xFFFFFFFF` 저장(site 30 슬롯 0 ≠ 0, 슬롯 1 = `0xFFFFFFFF`; `cfe_sb_api.c` L173, L224, L237). `RunLoop`의 조회면 FALSE와 `CFE_ES_ExitApp`(L551, L653–L661) | site 30·31을 못 보면 "미관측" | E-03 (나)가 없으면 O3 없음 [F]. 있으면 어느 호출 안이었는지에 따른다. `RunLoop`까지 창이 이어지는지는 미정 | (나) 없이 O3가 보임 |
| E-07 | U-C5-3 | SAMPLE_LIB task | 발생 없음 → 상태·요약 없음 | — | — | — | `SAMPLE_LibInit`에 조회가 없다 [F] | SAMPLE_LIB task의 조회 발생 |
| E-08 | U-C5-1·2, U-C5-6·7 | O2: 잘못된 AppID의 `CFE_SUCCESS` | Kairos는 READY로 적는다(조건은 TaskId 칸의 상태만 본다). O2는 따로 보고 | O2만으로는 순서위반이 아니다 | **도달하지 않음 기대** [I]: (a) 낡은 칸: PO 시작에서 모든 칸이 비고(L213–L216) 시작 경로에 해제가 없다. (b) 앞 칸의 같은 `pthread_t`: 삭제가 없다. (c) 스레드 시작 뒤 출력 인자를 쓰는 C 라이브러리: glibc 원 소스는 아니다(CB §7.1) | site 25를 못 보면 판단 안 함 | CB §7.1–§7.2. 이 컨테이너의 32비트 `libc.so.6` 바이너리가 원 소스 순서인지는 미확인 [U] | site 21 status ≠ 0(`OS_ERR_INVALID_ID`), site 24 값 0인 앱 task, site 25 값 ≠ site 33 값 |
| E-09 | U-C5-4 | 코어 task·TIME 자식 task의 모든 조회 | READY → `NO_VIOLATION_IN_CHECKED_PREFIX`(`outcome-narrowed`) | 아니오(구조적으로 등록 뒤) | 없음 | E-03과 같음 | 생성자가 잠금을 생성과 등록에 걸쳐 쥔다: `CFE_ES_CreateObjects` 6.4.1 L838–L933(6.4.2 L816–L913), `CFE_ES_CreateChildTask` L1076–L1145(L1015–L1084)(F7). 새 task의 첫 잠금(`RegisterApp`·`RegisterChildTask`)이 해제까지 기다린다 [F][I] | 이 task들의 `CFE_ES_ERR_APPID` |
| E-10 | U-C5-5 | 초기 스레드의 조회 | 발생 기대 없음 | — | — | — | E-07과 같은 방식의 읽기 [I] | 있으면 사례 밖 발생으로 따로 적는다(키 `es.task(0)`) |
| E-11 | U-C5-1, U-C5-6 | 정체성: T의 생성 실행자와 소유 앱 | 발생의 `identity` 줄: creator = 초기 스레드 actor, 소유 앱 `confirmed`(값 `i`) | — | — | site 20이나 16이 없으면 creator·owner `undetermined`. site 33·2가 없으면 owner `undetermined` | 결합: thread-entry(site 20) = spawn-id(site 16)(`identity_rule`), owner-publish(site 33, L657 / 6.4.2 L660) + request-return status 0(site 2) + owner-check(site 8) 일치(CB §10.4). `i`는 L449–L460의 첫 빈 칸이다. 코어 앱 다섯이 0–4를 쓰면 스크립트 순서로 SAMPLE_LIB 5, SAMPLE_APP 6, CI 7, TO 8, SCH 9 [I; 10-28 smoke의 코어 App ID 0–4는 [기록]] | site 8 값 ≠ site 33 값(owner 철회), 같은 T에 owner-publish 둘 |
| E-12 | 모든 단위 | 실행 관계 | `relation ownership-undetermined`("the record does not fix the key of the occurrence") | — | — | — | 출처 기준 관계는 U-C5-1에서 "생성 프레임워크–새 앱"이다. Kairos는 이 프로필에서 그것을 세우지 못한다: rspec의 키는 호출 사건의 인자로만 정해지는데, T의 TaskId는 호출 안(L856 뒤 점)에서야 기록된다(프로필 `r_task_id_param`, `lib/core/eval_trace.cpp` 발생 생성부) [F: 엔진 코드]. 소유 앱 종류도 선언하지 않았다(6.4.x AppId에 undefined·reserved 값이 없음, 프로필 10_kinds) | 다른 관계 값이 나옴(엔진·프로필이 바뀐 것: §5에 적는다) |
| E-13 | 모든 단위 | 반환 코드 없는 읽기(P1 대안, `--outcome-narrowing off`) | 모든 조회가 결정되지 않음 → `INCONCLUSIVE`(이유에 `start_state_unknown` 포함) | 판정 단위는 반환 코드 없이 서지 않음 | — | — | `es.task`의 시작 상태 효과(`CFE_ES_Main.clear_task_table`)와 다른 writer(`CreateObjects`·`CreateChildTask`)의 점이 manifest에 없어 종류가 열려 있다. 생성자 등록 → 조회의 HB 규칙이 없다(잠금 템플릿 없음, reads_from 규칙 없음) [F: 프로필 30_hb] | P1을 끄고도 결정된 발생 |
| E-14 | U-C5-6 | 6.4.2 각 T의 첫 조회와 이후 조회 | READY → `NO_VIOLATION_IN_CHECKED_PREFIX`(`outcome-narrowed`) | **아니오**(0건 기대) | O1·O2·O3 없음 | E-03과 같음 | 6.4.2는 L571에서 잠근 채 `OS_TaskCreate`(L619)와 등록(L652–L660)을 지나 L669에서 푼다(F9). T의 첫 ES 호출 `CFE_ES_RegisterApp`이 L710에서 같은 잠금을 기다리므로, T의 잠금 보호 조회는 모두 L669 뒤다 [F][I]. VDD Table 1.2-2 item 2(Trac #41) | 6.4.2 앱 task의 `CFE_ES_ERR_APPID`(TaskId = 자기 id) |
| E-15 | U-C5-7…10 | 6.4.2의 나머지 단위 | E-07(SAMPLE_LIB), E-09(코어·자식), E-10(초기 스레드)과 같은 라벨 | 같음 | 같음 | 같음 | 6.4.2 줄로 같은 읽기 | 같음 |
| E-16 | K-C5-641 대 K-C5-642 | 판 비교 | 비교 결과에 "여러 수정이 섞였다"(Trac #40·#41·#42) 표시 | 6.4.1의 순서위반 수(미정) 대 6.4.2의 0 | — | — | CB §4 "6.4.2 대조", CS §4.3. 한 번의 자연 기동은 가능성 증거이고 빈도 증거가 아니다(A7) | — |
| E-17 | 범위 밖 | 보류 실행(A6), F0 경로, 명령에 의한 앱 시작·재시작(F8 런타임 경로), 낡은 칸 시나리오 | 이번 동결의 행이 아님(실행하지 않음) | — | — | — | D4, D3, A5. 하려면 질문·근거를 정한 뒤 이 표를 개정한다 | — |

## 4. 이 표가 말하지 않는 것

- **`appreq`와 caller 수준 Allowed**: 두 판의 원문과 이슈에서 이 조회가 호출 코드에 무엇을 요구하는지, 실패를 받아들인다고 말하는 caller 쪽 글을 찾지 못했다. 이슈 본문과 VDD는 프레임워크의 결함을 서술하고, `cfe_es.h`는 결과 코드(공급자 쪽)를 적고, 시작 스크립트 앱은 반환값을 주석 없이 버린다. 그래서 프로필은 `appreq unspecified_by_reference`이고, 미준비 발생의 Kairos 라벨은 NOT_READY_UNJUDGED다(E-03 (나)). 연구자가 만든 `appreq`는 넣지 않는다(QT-9). C5의 `appreq`를 둘지는 PI 결정 항목이다(C1의 D8과 같은 종류). 결정되면 E-03 (나)의 라벨이 VIOLATING → `OBSERVED_VIOLATION`으로 바뀔 수 있고, 그 개정은 C5 재생 전이어야 한다.
- **판정 단위와 Kairos 라벨의 관계**: 순서위반은 (나) 라벨의 조회 가운데 E-03의 조건을 채우는 것이다. Kairos 출력은 "그 조회 때 칸이 FREE"(반환 코드 좁힘)까지 세우고, "생성자가 나중에 같은 칸을 등록함"은 기록의 site 값으로 따로 확인한다(E-12의 키 문제 때문에 Kairos의 관계 줄은 이것을 세우지 못한다).
- 실행 횟수(부팅 1회, A7), 빈도, 창에 들어가는 확률, 우선순위·CPU의 영향: 세지도 예측하지도 않는다. 6.4.1에서 순서위반이 0건이어도 이 표와 모순되지 않는다.
- hook 비용이 창을 바꾸는지(G-I0와 G-I 비교, CB §6 항목 3)는 이 표의 범위 밖이다.
- C5는 C3과 수정 이력(6.4.1 → 6.4.2 diff의 Trac #40–#42)을 공유하고, R2·C1과 모양(K-SELF.es, reserve-then-finalize; 가설)을 공유한다. 그래서 독립 사례로 세지 않는다(SPEC_FINAL "Dependence between cases").

## 5. 개정 기록

동결 시점의 행은 없다.

| 날짜 | 행 | 바뀐 것 | 이유 | 이전 값 |
|---|---|---|---|---|
