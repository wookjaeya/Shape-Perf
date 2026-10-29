# C5 결과 — cFE 6.4.1·6.4.2 자연 부팅 기록의 고정 설정 재생 (2026-10-29)

- 지위: **pilot/dev 자연 부팅**의 기록이며 본 실험 결과가 아니다. 구성마다 부팅은 1회다(P8, PD29 A7). 한 실행은 그 경로의 가능성 증거이고 빈도·재현성·분포의 증거가 아니다.
- 보고 문구: **원 소스 기반 현대 호스트 파생 구성, root·실시간 우선순위**(PD29 D2, N-C5-1). 원 플랫폼의 스케줄이라고 쓰지 않는다. 계측(G-I) 경로의 결과이며 F0 경로와 같다고 주장하지 않는다(D3). 계측하지 않은 빌드의 창 빈도와 같다고도 주장하지 않는다.
- 범위: PD29 §4 단계 5의 "C5 부팅·판정 검증" 가운데 재생과 결과 정리다. 부팅은 BOOT 작업(`env/c5/run_c5.sh`, 묶음 `records/c5-20261029-boots`, 커밋 `b1ed70c`)이 했다. 이 문서를 쓰면서 cFS를 기동하지 않았다.
- 입력:
  - 기록 `/home/user/work/c5run/{641,642}/`
  - 동결 기대표 `plan/expected_results_C5.md`(`869a75a`, 이하 EX). 고치지 않았다. 개정 행은 리드가 정한다.
  - Φ 프로필 `kairos/profiles/c5-cfe-6.4.x`(`3894638`)
  - 실행 기반 `plan/c5_execution_basis_20261028.md`(이하 CB), 사례 파일 `cases/C5.md`, 큐 목록 `plan/c5_queue_inventory_20261029.md`(이하 QI)
- 원자료: `/home/user/work/c5run/replay/`(이하 `replay/`). 재생 드라이버는 `env/c5/replay_c5.sh`(`f4ae15b`, sha256 `e3c54863…`)다. 출력 디렉터리마다 `sha256.txt`가 있고, 설정은 `replay/config.txt`에 있다.
- 조건: 실험 조건·변수를 새로 정하지 않았다(`CONDITIONS_POLICY.md`). 분석기·프로필·manifest·시험·동결 기대표는 고치지 않았다. 레퍼런스가 정하지 않은 하네스 값은 §1.4에 `unspecified_by_reference`로 적는다.
- 표기: [M] 실행·재생·표로 확인, [F] 소스·문서에서 읽음, [I] 추론, [D] 결정, [U] 미확인.
- 세는 법(PD28 §3):
  - 발생 수는 실행마다의 값이고 판끼리 더하지 않는다.
  - 판정 단위는 순서위반이다(PD28 §2 항목 6, CB §4). Kairos 상태 라벨과 따로 센다.
  - 최종 결과 O1·O2·O3(EX §3 열 설명)은 판정과 따로 적는다.

## 0. 요약

| 항목 | 결과 |
|---|---|
| 실행 | 부팅 2회: 6.4.1 1회, 6.4.2 1회. 둘 다 #72 경로(`CFE_ES_AppCreate` 5회)에 이르렀다. 두 판 모두 12 task 전부가 SCHED_FIFO여서 이 구성으로 센다. 6.4.2는 하네스 한도 30 s 안에 OPERATIONAL 줄이 없어 SIGINT로 멈췄다(§2) [M] |
| 재생 | 두 기록을 한 고정 설정으로 읽었다. 도구는 병합 트리의 kz `7c6044bf…`·kz-static `000ab68b…`, 프로필은 Φ `38a6db1b…`(6.4.1)·`d92989a0…`(6.4.2), B′는 병합 kz-static과 Φ 프로필로 새로 만들었다. 변환·읽기 셋·trace·occ 모두 종료 0이다. 중립 계층은 부팅 때의 변환과 같다(차이 0줄) [M] |
| 6.4.1 판정 | 발생 68, 전부 READY → `NO_VIOLATION_IN_CHECKED_PREFIX`, 근거 `outcome-narrowed`. p1off·nocode에서는 68 전부 `INCONCLUSIVE`다. 관계는 68 전부 `ownership-undetermined`다 [M] |
| 6.4.2 판정 | 발생 67, 같은 모양(READY 67 / p1off·nocode `INCONCLUSIVE` 67) [M] |
| 판정 단위 | 순서위반은 6.4.1 **0**, 6.4.2 **0**이다. 두 판 모두 `CFE_ES_ERR_APPID`가 0회다(조회 112 · 249). 네 앱 task의 첫 조회는 모두 생성자가 자기 칸을 등록한 뒤였다(기록 시계 +0.024 – +0.152 ms) [M] |
| 최종 결과 | O1 0, O3 0. **O2가 두 판 모두 있다**. SAMPLE_APP의 스레드가 TaskId 7(SAMPLE_LIB의 칸)을 알렸고, 성공한 조회가 모두 AppId 5를 돌려줬다(6.4.1 18회, 6.4.2 64회). 원인은 앞 칸에 같은 `pthread_t`가 남은 것이다(CB §7.1 O2 조건 2; §4.3) [M][F][I] |
| E-ID1 | 결합된 owner는 `confirmed` 4개, `undetermined` 8개로 두 판이 같다. CI·TO·SCH는 기대값 `i`(7·8·9)와 같다. **SAMPLE_APP의 스레드는 owner 5(SAMPLE_LIB 요청)로 `confirmed`**다. 발생의 identity 줄은 68·67 전부 `owner undetermined`다 [M] |
| 기대 대 실제 | EX 17행을 대조했다. 일치 10행(E-01, E-02, E-05, E-06, E-07, E-10, E-13, E-15, E-16, E-17은 범위 밖으로 실행 없음). E-09는 코어는 일치하고 TIME 자식 task에는 조회가 없다. **불일치 6행**: E-03(SAMPLE_APP), E-04(`GetAppIDInternal` 발생 없음), E-08(O2 도달), E-11(SAMPLE_APP owner, 발생 identity 줄), E-12(값은 같고 근거가 다름), E-14(O2 열)(§5) |
| 남은 것 | `GetAppIDInternal` 입구 site 없음; 발생 수준 owner 미정(owner 종류 없음); E-ID1이 같은 `pthread_t` 재사용에서 `confirmed`를 냄; 행위자 사이 HB 간선 0; 6.4.2 정지 한도; EX 개정 행(리드)(§7) |
| 보존 | 묶음 `records/c5-20261029-replays`(파일 68, 7,078,577 B, xz 369,596 B, 조각 1). `verify_bundle.sh`는 돌리지 않았다(push 전). 그래서 아직 "보존됨"으로 적지 않는다(§8) |

## 1. 구성 [M][F]

### 1.1 분석기와 저장소

| 항목 | 값 |
|---|---|
| `kz` | `/home/user/work/merge-check/kz-llvm/tools/kz/kz` sha256 `7c6044bf9b787b78e2d779dc6262407584c1254c41d736a6a38bb0597964d135`. 부팅 때의 변환과 같은 바이너리다 |
| `kz-static` | 같은 트리 `tools/kz-static/kz-static` `000ab68bd9a4543c69df202bbdf4eb19c6969d520b902f018c2f94151836a4dc`(identity `tool_sha256=9c6902db…`) |
| `kz-dump` | `36a6965eb481195a54aca10f7bfefe22b84dc2bf73332d646de90695981d2d54` |
| `kz-phi` | `c8ed043c823ffdf4e7b5b142b724da1ea85dcec8860e6d25d22d591802a0da4c`(`hash`만 씀) |
| 저장소 | 재생 때 `f4ae15b`, `kairos/` 미커밋 차이 0(`config.txt` L2) |
| 상수·추가 인자 | 없음. 프로필에 `ref eval` 상수가 없다. `--core-request-caller`·`--process`는 주지 않았다(§1.4) |

### 1.2 프로필

| 판 | 인스턴스 | profile hash(`kz-phi hash`) | EX §1과 대조 |
|---|---|---|---|
| 6.4.1 | `c5-cfe-6.4.x` + `cfe-6.4.1/` | `38a6db1bcdb849c9677628fa61af8d6047a979e0363c85dc827c285c04126c2e` | 같음 |
| 6.4.2 | `c5-cfe-6.4.x` + `cfe-6.4.2/` | `d92989a03cb21effe4f5224adf27ea906b0f70a72894bb8dbbc9ea8ccbb95303` | 같음 |

- 프로필 디렉터리의 마지막 커밋은 `3894638`이고 tree는 `281fab3b`이며 미커밋은 0이다. 파일 12개의 sha256은 `config.txt` "profile files"에 있다.
- 두 rspec에 oracle 함수가 없다는 경고 2건이 모든 읽기에 나온다(`kz-phi check`와 같다).

### 1.3 B′와 입력

부팅의 변환은 E-ST의 B′(`c5tools/build/64<V>/est_instr/basis.kz`)를 썼다. 그 B′는 C5 도구 브랜치의 kz-static `3ec20abf…`와 머리만 있는 프로필 `basis/est-64<V>`로 만든 것이다.

- **같은 B′로는 재생할 수 없다** [M]: `kz replay --facts`는 facts를 추출한 프로필이 재생 프로필과 같아야 받는다. 시험에서 "the facts were extracted with profile de360fa3…, the replay uses 38a6db1b…"로 거부했다. `docs/static_facts.md` §2와 같은 규칙이다.
- 그래서 B′를 병합 kz-static으로 다시 만들었다(`replay_c5.sh bprime`).
  - E-ST 입력은 그대로 썼다(`est_run.sh` 단계 2): compile DB `est_instr/compile_commands.json`(`2ec33a39…` 6.4.1), 파생 트리의 build dir·bundle root, recipe `basis/core-linux.kzb` `7bd4d55d…`.
  - 프로필은 Φ 인스턴스다. `basis_gi.sh`가 basis·extract에 사례 프로필을 주는 것과 같다.
  - extract 연산은 `basis_gi.sh`의 넷이다: `--delay-op OS_TaskDelay`, `--lock-op CFE_ES_LockSharedData`, `--unlock-op CFE_ES_UnlockSharedData`, `--spawn-op CFE_ES_CreateChildTask`. `--target` 목록은 C1·OSAL 5 형이라 쓰지 않았다.

| 판 | basis | facts | facts_est (대조) | 단위·함수·호출 지점 | 낮춘 진단 |
|---|---|---|---|---|---|
| 6.4.1 | `5f055767…` | `1359a41f…` | `14cc7dfc…` | 58 · 771(본문 652) · 3412 | `osapi.c:647:34`, `cfe_psp_start.c:270:19` 각 1(D7) |
| 6.4.2 | `3c7c4106…` | `bf980c45…` | `bd49ce42…` | E-ST 기록과 같은 수 | 같음 |

- 두 basis 모두 ELF32 little-endian EM_386, 포인터 4 B다. 시작 스크립트 sha256은 `aa666bc3…`이다. `pthread_create`·`signal` 호출 사실이 각 1개 있다. E-ST 기록(CB §10.5)과 같다.
- 대조 읽기 `cur_estfacts`: E-ST의 extract 인자(연산 없음)로 만든 facts로 cur를 다시 읽었다. `kz occ diff`에서 판정 차이 0, 근거 차이 0이다(68·67 쌍 모두 owner 없이 짝지음, 종료 3).
- 변환 입력:

| 판 | manifest | binds | dump |
|---|---|---|---|
| 6.4.1 | `sites-641.draft.kzs` `6bd88dcb…` | `binds_instr.kzs` `04445f6c…` | `cpu1.kzrt` `04ec0fa3…` |
| 6.4.2 | `sites-642.draft.kzs` `1d17d92d…` | `binds_instr.kzs`(해시는 `config.txt`) | `6b9ec340…` |

  manifest 해시가 `run.meta`·`build_instr.record`와 맞는지 확인한 뒤 변환했다.
- `run.kzt`: 6.4.1 `9e8e0208…`, 6.4.2 `5882944f…`. kz-dump 텍스트를 부팅 때의 `convert/run.kzt.txt`와 비교했다. 해시·basis·parent 줄을 빼면 차이는 두 판 모두 0줄이다(`kzt_vs_boot.diff`). 다른 B′로 변환해도 중립 계층은 같다.

### 1.4 읽기와 `unspecified_by_reference`

- 읽기는 `w4w5_c1_replay.sh` L64–L76·`replay_w6.sh` "one"과 같다.
  - **cur**: `--compare-narrowing`
  - **p1off**: `--outcome-narrowing off`
  - **nocode**: p1off + `--reads-from-outcome off`
  - 그리고 `--emit-trace`, 읽기마다 `kz occ show`
  - 제어 사실이 없어 ctloff 계열은 없다.
- 판 비교: `kz occ diff 641/cur.kz 642/cur.kz`(`tables/occdiff_641_642_cur.txt`).
- `unspecified_by_reference` (레퍼런스가 정하지 않아 기록만 한 것):
  - 출력 배치, 대조 읽기 `cur_estfacts`, TSV 열.
  - extract 연산 넷. C5에 이 값을 정하는 레퍼런스가 없다. G-I B′의 기존 절차(`basis_gi.sh`)를 따랐고, 대조 읽기로 판정 영향이 0임을 보였다.
  - `--core-request-caller`. R2는 `CFE_ES_CreateObjects`를 준다(`results_pilot_20261025.md` §0). C5 manifest에는 `CreateObjects` 안에 요청 reader site가 없다(EX U-C5-4). 그래서 주지 않았다. 따로 6.4.1 cur에 그 값을 주어 보았다(산출물은 묶음 밖 scratch). 판정·근거 차이는 0이었고, "core status of owners: core 0, not core 0, not established 12"였다 [M].
  - 표의 "자기 spawn" 대응 규칙(`tables/tasks.tsv`). actor의 `pthread_self`(site 19)와 같은 `pthread_t`(site 15)를 낸 spawn 가운데, 그 actor의 첫 사건보다 앞선 마지막 것으로 정했다. 같은 값이 두 번 나온 경우를 가르기 위한 분석 규칙이다 [I]. SAMPLE_APP은 site 32(`SAMPLE_AppMain` 입구)의 actor로 따로 확인했다.
- 레퍼런스로 만들지 못한 것: 위 extract 연산과 `--core-request-caller`. B′의 프로필은 도구 규칙(facts와 재생 프로필의 일치)이 정했다.

## 2. 실행과 조건 [M]

BOOT 기록(`c5run/{641,642}/run.meta`, `ns.txt`, `state_running.txt`, `checks.txt`)에서 옮겼다. 드라이버는 641이 `run_c5.sh` `be675043…`, 642가 `5a6be77c…`다. 두 판 사이의 변경은 check 모드뿐이고 부팅 단계는 같다(`INDEX.txt`).

| 항목 | 6.4.1 | 6.4.2 |
|---|---|---|
| 시각(UTC) | 07:48:11.364 – 07:48:18.053 | 07:50:20.787 – 07:50:51.380 |
| 사용자·커널 | root, `6.18.44-fc-v185`, CPU 4 | 같음 |
| IPC namespace (D1) | 별도 ns(`ipc:[4026532264]`). ns 기본값 `msg_max` 10 → 64를 쓰고 다시 읽음. `msgsize_max` 8192·`queues_max` 256(ns 기본값). 끝에도 64. 바깥 `msg_max`·`msgsize_max`·`queues_max` 10·8192·256, 앞뒤 같음 | 같음 |
| `RLIMIT_MSGQUEUE` (A3) | 819,200 / 819,200 B, 바꾸지 않음 | 같음 |
| 큐 | 10개, 깊이 8·12·32·32·32·12·32·12·8·64, 오류 줄 없음(QI와 같은 집합) | 같음 |
| 시작 | "Default Reset Type = PO", "Starting the cFE with a POWER ON reset", "POWER ON RESET due to Power Cycle" | 같음 |
| 코어 앱 | EVS·SB·ES·TIME·TBL, App ID 0–4 | 같음 |
| 시작 스크립트 | 5줄 모두 "loaded and created"(SAMPLE_LIB, SAMPLE_APP, CI_LAB_APP, TO_LAB_APP, SCH_LAB_APP) | 같음 |
| 끝 줄 | "ES Startup: CFE Core Startup Complete"(`cfe_es_start.c` L315)를 1,095 ms에 읽음 | "…CFE_ES_Main entering OPERATIONAL state"(L292)가 30 s 안에 없음(`limit_reached_without_end_of_startup`) |
| 정지 | 정착 5 s → SIGINT @6,340 ms, 종료 6,357 ms | SIGINT @30,252 ms, 종료 30,270 ms |
| 종료 | "Control-C Captured", 종료 코드 0. ns의 SysV·큐 비어 있음 | 같음 |
| 실시간 정책 (N-C5-1) | 12/12 task SCHED_FIFO(아래 표) | 12/12 |
| 커널 RT 값·affinity·능력 | `sched_rt_runtime_us` 950000 / period 1000000, affinity `f`, CAP_SYS_NICE 1, cpu cgroup `/` | 같음 |
| N-C5-1 소스 줄 | `n_c5_1_sources.txt`: `osapi.c` L430–L455, L584–L640, L3948–L4030; PSP L340–L382, L415–L421 | 같음 |
| 기록 회수 | in-process dump(store=file), 32비트 `kz-rt-dump` exit 0. 회수(`--recover`)는 필요 없어 하지 않음 | 같음 |
| hook 기록 | 33/33 site, 사건 1,133, dump 101,764 B | 31 site(3·6은 6.4.2 manifest에 없음), 사건 1,716, dump 153,068 B |

**task별 정책·우선순위** (`tables/sched.tsv`, 실행 중 `/proc/<pid>/task/*/stat`; policy 1 = FIFO):

| task | 6.4.1 actor · rt_priority | 6.4.2 actor · rt_priority |
|---|---|---|
| 초기 스레드 | a1 · 1 (PSP가 `CFE_ES_Main` 뒤에 내림, L378–L379) | a1 · 99 (아직 `CFE_ES_Main` 안, L356–L357) |
| CFE_EVS / CFE_SB / CFE_ES / CFE_TIME | a2–a5 · 65 / 64 / 63 / 66 | a2–a5 · 65 / 64 / 63 / 66 |
| CFE_TBL | a6 · 62 | a8 · 62 |
| TIME_TONE_TASK / TIME_1HZ_TASK | a7 · 77 / a8 · 77 | a6 · 77 / a7 · 77 |
| SAMPLE_APP / CI_LAB_APP / TO_LAB_APP / SCH_LAB_APP | a9–a12 · 69 / 66 / 62 / 59 | a9–a12 · 69 / 66 / 62 / 59 |

- 우선순위는 OSAL `OS_PriorityRemap`(SCHED_FIFO 1/99)으로 셈한 기대값과 같다(BOOT 기록).
- SAMPLE_LIB task(기대 86)는 실행 중에 없다.

**각 부팅이 이른 곳**:
- 6.4.1: 시작을 끝냈다(끝 줄). 네 앱 task가 `RunLoop`의 `GetAppIDInternal`까지 돌았다(SAMPLE·CI·TO 각 12·12·11회, SCH 7회).
- 6.4.2: 시작 스크립트 다섯 줄과 네 앱 task의 `RunLoop`(각 58·58·58·1회)까지 갔다. OPERATIONAL 줄 전에 하네스가 멈췄다.
  - ES는 시작 스크립트 뒤 `CFE_ES_ApplicationSyncDelay(…, CFE_ES_STARTUP_SCRIPT_TIMEOUT_MSEC)`(`cfe_es_start.c` L283–L284; 30000, cpu1 `cfe_platform_cfg.h` L1744)에서 기다린다 [F].
  - 하네스의 30 s는 실행 시작부터 세므로 ES의 한도보다 약 0.25 s 먼저 끝났다(BOOT 기록) [I].
  - 사례 경로(`CFE_ES_AppCreate` 5회와 앱 task의 조회)는 그 전에 모두 지나갔다. 그래서 이 실행은 E-01 기준으로 센다.

## 3. 재생 결과 [M]

### 3.1 읽기별 수 (`tables/readings.tsv`)

| 판 | 읽기 | 발생 | 상태 | 요약 | 근거 조합 | 관계 | findings |
|---|---|---|---|---|---|---|---|
| 6.4.1 | cur | 68 | READY 68 | `NO_VIOLATION_IN_CHECKED_PREFIX` 68 | `outcome-narrowed` 68 | `ownership-undetermined` 68 | 89 |
| 6.4.1 | p1off | 68 | UNDETERMINED 68 | `INCONCLUSIVE(start_state_unknown, resource_or_lifetime_unmatched, caller_requirement_absent, effect_unsupported)` 68 | none 68 | 같음 | 89 |
| 6.4.1 | nocode | 68 | p1off와 같음 | 같음 | 같음 | 같음 | 89 |
| 6.4.2 | cur | 67 | READY 67 | `NO_VIOLATION_IN_CHECKED_PREFIX` 67 | `outcome-narrowed` 67 | `ownership-undetermined` 67 | 365 |
| 6.4.2 | p1off·nocode | 67 | UNDETERMINED 67 | 6.4.1 p1off와 같은 이유 | none 67 | 같음 | 365 |

- `--compare-narrowing`: 두 읽기 사이에 요약이 다른 발생은 68·67 전부다.
- cur의 READY는 모두 `CFE_SUCCESS` 매핑의 좁힘이다("holds at the call's conjunct point, before its return: it fixed what the record left open").
- 발생은 모두 `K-SELF.es-task`(`CFE_ES_GetAppID`)다. `K-SELF.es-task-internal`의 발생은 0이다.
  - findings는 그 대신 나온다. 6.4.1에서 "point es.get_app_id_int.task is outside of a call of CFE_ES_GetAppIDInternal" 44와 "return … without an open call" 44, 6.4.2에서 각 182다.
  - 나머지 1건은 "real record: the order of different actors is read from the happens-before edges of the record (0 between actors)"다.
  - manifest에 `GetAppIDInternal`의 입구(phase before) site가 없다. 점(27)·저장(28)·반환(29)만 있다 [F: `sites-64<V>.draft.kzs`].
- 호출 문맥: 6.4.1·6.4.2 모두 역할 사실 10개(site 30), `assumes-role`이 붙은 발생 10개다. 라벨은 바뀌지 않았다.
- 키: 모든 발생에 키가 있다(`[0]`…`[11]`). 출력은 "arguments from linearization points inside the call 68(67)"이다. 호출 사건에 없던 `task_id`를 호출 안의 점(site 24)이 채웠다.
- 종류 `es.task`:
  - 시작 상태: `open {start_state_unknown, resource_or_lifetime_unmatched, effect_unsupported}`.
  - 관측되지 않은 writer 효과 6개: `CFE_ES_Main.clear_task_table`, `CreateObjects.publish_core`, `CreateChildTask.publish_child`, 해제 셋.
  - "static closure: not given". `--facts`는 받았지만 이 종류에는 닫힘이 걸리지 않았다. 프로필의 종류에 정적 위치(`loc`)가 없어서로 본다 [I].
- 최종 상태: `es.task[7..11] = LIVE`. 관측된 쓰기는 `AppCreate`의 site 9 다섯 번뿐이다.
- HB 간선: 6.4.1 331, 6.4.2 604. 모두 R1(프로그램 순서)이다. 행위자 사이 간선은 0이다.
  - trace에는 `creator aN a1` 11줄이 있다. 그래도 R2-P-TASK(phase) 규칙은 간선을 만들지 않았다("spawn returns 0, out-parameter bound by the id 0") [M]. 원인은 확인하지 않았다 [U].

### 3.2 정체성 (E-ID1)

변환 노트는 두 판이 같다:
- identity events 67; announced 11, spawn 12, tie 11; app requests 5, owner publish 5, owner check 5.
- 차이·충돌·읽을 수 없음은 모두 0이다.

| actor | 스레드 (자기 spawn) | creator | thread 수준 owner (`identity aN`) | 발생 identity 줄 |
|---|---|---|---|---|
| a1 | 초기 스레드 | — | undetermined | 조회 없음 |
| a2–a6 (6.4.2 a2–a5, a8) | 코어 앱 5 | a1 | undetermined (evidence announced-id) | `owner undetermined` |
| a7, a8 (6.4.2 a6, a7) | TIME 자식 2 | a5 | undetermined | 조회 없음 |
| a9 | **SAMPLE_APP** (spawn task id 8; site 32의 actor) | a1 | **confirmed id 0x5** (SAMPLE_LIB 요청) | `owner undetermined` |
| a10 / a11 / a12 | CI_LAB / TO_LAB / SCH_LAB | a1 | confirmed 0x7 / 0x8 / 0x9 | `owner undetermined` |

- 발생 수준의 owner는 모두 정해지지 않았다. 출력은 "app_owner levels: confirmed 4, …; ids not mapped to a record key 4"다. 프로필에 owner 종류(`context`)가 없어 confirmed id를 기록 키로 옮기지 못한 것으로 본다(CB §10.3 메모 2 "선언하지 않은 것") [I].
- SAMPLE_LIB의 스레드에는 actor가 없다. spawn은 12개인데 actor는 초기 스레드와 알린 스레드 11개다.

### 3.3 관계

| 판 | "no publication or clearing write of es.task[k] before the decision" | "provider(s) a1: the owner of the executing task aN is undetermined" |
|---|---|---|
| 6.4.1 | 32 (코어 task의 조회: 코어 등록 writer가 관측되지 않음) | 36 (앱 task: provider는 `CFE_ES_AppCreate.publish`) |
| 6.4.2 | 31 | 36 |

- a9(SAMPLE_APP)의 provider는 `#34 CFE_ES_AppCreate.publish on es.task[7]`(6.4.1)이다. SAMPLE_LIB를 만든 `AppCreate`의 등록이다.

## 4. C5 판정 (발생·읽기별) [M]

### 4.1 앱 task의 첫 조회 (U-C5-1 / U-C5-6; `tables/tasks.tsv`)

"자기 등록 뒤"는 생성자의 site 9(자기 spawn task id) 시각에서 첫 `GetAppID` 입구(site 23)까지를 잰 것이다. 기록 시계로 쟀다. Kairos의 HB 판단이 아니다(§3.1: 행위자 사이 간선 0).

| 판 | task | 읽은 TaskId (site 24) | 자기 spawn id (16) | 돌려받은 AppId (25) / 자기 `i` (33) | 상태 (26) | 자기 등록 뒤 | cur | p1off·nocode | 순서위반 |
|---|---|---|---|---|---|---|---|---|---|
| 6.4.1 | SAMPLE_APP a9 | **7** | 8 | **5 / 6** | 0 | +0.045 ms (TaskId 7 등록 뒤 +0.472) | READY → `NO_VIOLATION_IN_CHECKED_PREFIX` {outcome-narrowed} | `INCONCLUSIVE` | 아니오 |
| 6.4.1 | CI_LAB a10 | 9 | 9 | 7 / 7 | 0 | +0.106 ms | 같음 | 같음 | 아니오 |
| 6.4.1 | TO_LAB a11 | 10 | 10 | 8 / 8 | 0 | +0.024 ms | 같음 | 같음 | 아니오 |
| 6.4.1 | SCH_LAB a12 | 11 | 11 | 9 / 9 | 0 | +0.152 ms | 같음 | 같음 | 아니오 |
| 6.4.2 | SAMPLE_APP a9 | **7** | 8 | **5 / 6** | 0 | +0.084 ms (+0.462) | 같음 | 같음 | 아니오 |
| 6.4.2 | CI_LAB a10 | 9 | 9 | 7 / 7 | 0 | +0.041 ms | 같음 | 같음 | 아니오 |
| 6.4.2 | TO_LAB a11 | 10 | 10 | 8 / 8 | 0 | +0.024 ms | 같음 | 같음 | 아니오 |
| 6.4.2 | SCH_LAB a12 | 11 | 11 | 9 / 9 | 0 | +0.118 ms | 같음 | 같음 | 아니오 |

- 네 task 모두 첫 잠금 보호 조회는 `GetAppID`다. `GetAppIDInternal`이 그보다 앞선 task는 없다.
- 둘째 이후 조회도 같다. `GetAppID` 발생은 앱 task별로 6·6·22·2개(두 판 같음)이고 모두 READY다. 각 task의 TaskId·AppId 값은 첫 조회와 같다(`tables/lookups.tsv`).

### 4.2 나머지 단위

- 코어 task(U-C5-4/9):
  - 6.4.1 a2–a6의 `GetAppID` 32회, 6.4.2 a2–a5·a8의 31회가 모두 READY → `NO_VIOLATION_IN_CHECKED_PREFIX`다.
  - 돌려받은 AppId는 콘솔의 App ID와 같다: EVS 0, SB 1, ES 2, TIME 3, TBL 4. 6.4.2의 TBL은 TaskId 6에서 AppId 4다.
- TIME 자식 task: 두 판 모두 조회가 0회다.
- SAMPLE_LIB task(U-C5-3/8): actor가 없고 조회도 없다.
- 초기 스레드(U-C5-5/10): 조회가 0회다.
- `GetAppIDInternal`(6.4.1 44회, 6.4.2 182회): 기록에는 모두 status 0(site 29)이다. Kairos 발생은 만들어지지 않았다(§3.1).

### 4.3 최종 결과 (`tables/outcomes.tsv`)

| 판 | 조회 (GetAppID + Internal) | O1 (`CFE_ES_ERR_APPID`) | 다른 status | O2 | O3: site 30 status ≠ 0 / AppId `0xFFFFFFFF` | site 31 status ≠ 0 | site 21 ≠ 0 | 앱 task의 site 24 = 0 |
|---|---|---|---|---|---|---|---|---|
| 6.4.1 | 112 (68 + 44) | 0 | 0 | 18 (a9: 5 ≠ 6) | 0 / 0 | 0 / 20 | 0 | 0 |
| 6.4.2 | 249 (67 + 182) | 0 | 0 | 64 (a9: 5 ≠ 6) | 0 / 0 | 0 / 19 | 0 | 0 |

**O2의 경로** (두 판 같음):
1. 시작 스크립트 L1은 SAMPLE_LIB를 `CFE_APP`으로 적는다. 진입은 `SAMPLE_LibInit`, 우선순위 0이다(tarball `cfe_es_startup.scr`) [F].
2. 그 task의 진입 함수는 `OS_printf` 뒤에 돌아온다(`sample_lib.c` L32–L43) [F]. OSAL 4.1.1은 진입 함수를 `pthread_create`에 그대로 준다(`osapi.c` L645–L648; E-ST 호출 사실 arg 2 `function_pointer`). 그래서 스레드는 거기서 끝난다 [I].
3. 그 스레드는 detach되어 있다(L663) [F]. OSAL 칸 7은 `OS_TaskDelete`(L761)나 `OS_TaskExit`(L793) 없이는 비지 않는다 [F]. 그 칸의 `id`에는 끝난 스레드의 `pthread_t`가 남는다.
4. 다음 `OS_TaskCreate`(SAMPLE_APP, 칸 8)의 `pthread_create`가 같은 `pthread_t`를 돌려줬다. site 15가 641 `0xf7b42b40`, 642 `0xf7b83b40`으로 두 번 같다 [M]. 끝난 detach 스레드의 기술자를 glibc가 재사용한 것으로 본다 [I].
5. SAMPLE_APP 스레드의 `OS_TaskRegister`는 칸 0부터 `id`를 비교한다(L921–L928). 칸 7에서 멈추고 7을 알린다(site 20 = 7, site 21 = `OS_SUCCESS`) [F][M]. `OS_TaskGetId`(L959–L972)는 그 값을 돌려준다.
6. `GetAppID`는 `TaskTable[7]`을 읽는다. SAMPLE_LIB의 `AppCreate`가 등록한 칸이라 `RecordUsed` TRUE, `AppId` 5다(site 25 = 5, site 26 = 0) [M].
7. 소비:
   - SB 파이프의 AppId가 5다(site 30 = `[0x0, 0x5]`) [M].
   - SAMPLE_APP의 EVS 메시지가 발신자 "SAMPLE_LIB"로 찍혔다(콘솔 "EVS Port1 66/1/SAMPLE_LIB 1: SAMPLE App Initialized." 등) [M].
   - 이것은 EX의 O3(EVS 등록 건너뜀, `0xFFFFFFFF` 파이프, `RunLoop` FALSE)가 아니다. O2의 결과로 따로 적는다.

- 이것은 CB §7.1의 O2 조건 2(앞 칸에 같은 `pthread_t` 값이 남음)다. CB는 "시작 스크립트 경로에는 삭제가 없다"고 적었다. 그러나 이 기록에서는 삭제 없이, 진입 함수가 돌아와 스레드가 끝난 것만으로 조건이 섰다.
- CB §7.3의 계측 확인 표로 보인다. "알린 id = 배정 id"가 SAMPLE_APP에서 깨졌다(site 20 = 7 ≠ site 16 = 8). "칸의 값 = 자기 스레드"(site 19 = site 15)는 두 spawn 모두에 성립한다.
- 생성 순서의 창(L610–L637, F4)과는 다른 경로다. 순서위반이 아니다. 판정 단위의 정의(새 task가 생성자의 등록 전에 자기 칸을 읽음)에 비추면, SAMPLE_APP의 자기 칸 8은 첫 조회 0.045 ms(6.4.1)·0.084 ms(6.4.2) 전에 이미 등록되어 있었다.
- 6.4.1·6.4.2 모두 같은 OSAL·같은 시작 스크립트이므로 판 차이(F9의 잠금 범위)와 무관하다 [I].

## 5. 기대 대 실제 (EX §3, 행마다 하나)

| 행 | 기대 (요지) | 실제 | 일치 | 불일치의 설명 (자료) |
|---|---|---|---|---|
| E-01 | 두 판 경로 도달: site 1이 다섯 번, 큐 오류 없음 | 두 판 site 1 ×5, 큐 10개 생성, 오류 줄·PROCESSOR RESET 없음 | 예 | — |
| E-02 | 실시간 정책 적용 | 두 판 12/12 SCHED_FIFO, 우선순위가 OSAL 대응값과 같음 | 예 | — |
| E-03 | 6.4.1 각 T의 첫 조회: 미정 (가) READY → `NO_VIOLATION_IN_CHECKED_PREFIX` 또는 (나) NOT_READY_UNJUDGED. 키 `es.task(TaskId_T)`. (가)의 반박: site 25 ≠ site 33 | 네 T 모두 (가)이고 근거 `outcome-narrowed`다. 순서위반 0. **SAMPLE_APP은 키가 `es.task(7)`**(자기 TaskId 8이 아님)이고 site 25 = 5 ≠ site 33 = 6이다 | **부분 불일치** (CI·TO·SCH 예, SAMPLE_APP 아니오) | (가)의 반박 조건이 SAMPLE_APP에서 섰다. 원인은 §4.3이다: 같은 `pthread_t` 재사용으로 `OS_TaskRegister`가 칸 7을 알렸다. Kairos 라벨 자체는 기대 (가)와 같다 |
| E-04 | 둘째 이후 조회(`GetAppID`·`GetAppIDInternal`): READY → `NO_VIOLATION_IN_CHECKED_PREFIX` | 앱 task의 둘째 이후 `GetAppID`는 두 판 모두 32개이고 모두 READY다. **`GetAppIDInternal`은 발생이 0**이고 findings 44 + 44(6.4.1)·182 + 182(6.4.2)다. 기록상 status는 모두 0이다 | **불일치** (Internal) | manifest에 `CFE_ES_GetAppIDInternal` 입구 site가 없다(27·28·29만 있음). 그래서 점·반환이 "outside of a call"로 처리되어 `K-SELF.es-task-internal`이 서지 않았다 |
| E-05 | `CreatePipe` 안 조회: 문맥 `sb-create-pipe-dependent-lookup`, `assumes-role`, 라벨 그대로 | 두 판 역할 사실 10, 문맥 붙은 발생 10, 라벨 READY 그대로 | 예 | — |
| E-06 | O3는 E-03 (나)가 있을 때만 | (나) 없음. site 30 status ≠ 0 / `0xFFFFFFFF` 0, site 31 ≠ 0 0 | 예 | — |
| E-07 | SAMPLE_LIB task 발생 없음 | SAMPLE_LIB 스레드에 actor·조회 없음 | 예 | — |
| E-08 | O2 도달하지 않음. 이유 (b) 앞 칸의 같은 `pthread_t`: "삭제가 없다" | **O2 도달**: 두 판 SAMPLE_APP이 AppId 5와 `CFE_SUCCESS`를 받았다(18·64회). 반박 조건 "site 25 ≠ site 33"이 섰다. site 21 ≠ 0 0, 앱 task site 24 = 0 0 | **불일치** | (b)의 전제가 틀렸다. 삭제가 없어도 SAMPLE_LIB의 진입 함수가 돌아와 detach 스레드가 끝났다. 칸 7은 비지 않았고(L761·L793만 비움) 같은 `pthread_t`가 재사용되었다(site 15 값 둘이 같음)(§4.3) |
| E-09 | 코어·TIME 자식 task의 모든 조회 READY | 코어 32·31 조회 모두 READY. TIME 자식 task는 조회 0 | 예 (자식은 관측 없음) | — |
| E-10 | 초기 스레드 조회 없음 | a1 조회 0 | 예 | — |
| E-11 | 발생 identity 줄: creator = 초기 스레드, 소유 앱 `confirmed`(값 `i`: SAMPLE_APP 6, CI 7, TO 8, SCH 9) | creator는 네 T 모두 a1이다. thread 수준 owner는 CI 7·TO 8·SCH 9가 `confirmed`로 기대와 같고, **SAMPLE_APP은 `confirmed` 5**다. **발생 identity 줄은 68·67 모두 `owner undetermined`**다 | **불일치** | (1) E-ID1은 알린 id로 묶는다. 그 id(7)가 SAMPLE_LIB의 spawn id라서 SAMPLE_LIB 요청(owner 5)에 묶였다(§4.3). (2) 발생 수준은 "ids not mapped to a record key 4": 프로필에 owner 종류가 없다(§3.2) |
| E-12 | 관계 `ownership-undetermined`. 근거: "the record does not fix the key of the occurrence"(키가 비어 있음) | 값은 68·67 모두 `ownership-undetermined`다. **키는 정해졌다**("arguments from linearization points inside the call"). 이유는 둘이다: 코어 조회 32·31은 등록 writer 미관측, 앱 조회 36·36은 provider a1(`AppCreate.publish`)이 있으나 실행 task의 owner 미정 | **근거 불일치** (값 일치) | 엔진은 호출 사건에 없는 인자를 호출 안 점(site 24)의 값으로 채운다(출력 줄). EX와 프로필 메모(CB §10.3 메모 2, E-KEY)가 전제한 "빈 키"는 이 기록에서 생기지 않았다 |
| E-13 | P1 끄면 모든 조회가 `INCONCLUSIVE`, 이유에 `start_state_unknown` 포함 | p1off·nocode 68·67 모두 `INCONCLUSIVE(start_state_unknown, resource_or_lifetime_unmatched, caller_requirement_absent, effect_unsupported)` | 예 | — |
| E-14 | 6.4.2 모든 조회 READY, 순서위반 0, O1·O2·O3 없음 | READY 67, 순서위반 0, O1 0, O3 0. **O2 64** (SAMPLE_APP) | **불일치** (최종 결과 열) | E-08과 같은 원인. 6.4.2의 잠금 범위 수정(F9)과 무관한 OSAL 경로다 |
| E-15 | 6.4.2 나머지 단위: E-07·E-09·E-10과 같음 | SAMPLE_LIB 발생 없음, 코어 31 READY, TIME 자식 조회 0, 초기 스레드 조회 0 | 예 | — |
| E-16 | 판 비교: 6.4.1 순서위반 수(미정) 대 6.4.2의 0, "여러 수정이 섞였다" 표시 | 6.4.1 0 대 6.4.2 0. `kz occ diff`(cur): 짝 61, 한쪽만 7·6(6.4.1 TBL 키 4 ×6과 TIME 9번째; 6.4.2 TBL 키 6 ×6), 판정 차이 0. **여러 수정이 섞였다(Trac #40·#41·#42)**. 부팅 1회씩이라 가능성 증거일 뿐이다 | 예 | — |
| E-17 | 범위 밖(보류, F0, 명령 시작, 낡은 칸) | 실행하지 않음 | 예 (해당 없음) | E-08의 O2는 OSAL 칸에 남은 `pthread_t`에서 왔다. EX가 범위 밖으로 둔 "낡은 칸 시나리오"(ES `TaskTable`의 낡은 `RecordUsed`, CB §7.1 조건 1)와는 다른 경로다. 시작 스크립트 경로 안에서 생겼다 |

- 6.4.1의 순서위반 0은 EX와 모순되지 않는다(EX §4 "6.4.1에서 순서위반이 0건이어도 이 표와 모순되지 않는다").
- 결과를 본 뒤 EX를 고치지 않았다. 위 불일치를 개정 행으로 넣을지는 리드가 정한다. 개정하면 EX의 규칙(마스터플랜 §12.4)에 따라 이 두 실행은 개발 자료로 옮기고 새 평가를 한다.

## 6. 제한

- **부팅 1회씩** (P8, PD29 A7):
  - 6.4.1에서 창이 열리지 않은 것은 이 한 번의 스케줄에서 그랬다는 뜻이다.
  - 창이 열리는 빈도, 우선순위·CPU의 영향은 말하지 않는다.
  - O2 경로가 두 판에서 다 선 것도 두 번의 관측이다.
- **파생 구성** (D2): 원 소스를 현대 툴체인(gcc 13.3, glibc 2.39 32비트)으로 빌드하고 계측을 넣었다. 실행 조건은 root·SCHED_FIFO·IPC namespace `msg_max` 64다.
  - 원 플랫폼과 같은 스케줄이라고 쓰지 않는다.
  - `pthread_t` 재사용은 이 C 라이브러리 바이너리의 동작이다. glibc 원 소스·배포판 패치로 확인하지 않았다 [U]. CB §7.2 항목 1의 디스어셈블 확인도 아직 하지 않았다.
- **C5에는 F0 경로가 없다** (D3): 계측 경로만의 결과다. 두 경로가 같다고 주장하지 않는다.
  - 검색 순간의 칸 값은 계측 경로로 보지 못한다(CB §7.3). §4.3의 3–5는 site 15·19·20 값과 소스로 이은 것이다.
- **hook 비용**: 계측한 빌드의 창이 계측하지 않은 빌드와 같다고 하지 않는다(CB §6 항목 3).
- **6.4.2 정지**: 하네스 한도(30 s, `unspecified_by_reference`)가 ES의 시작 동기 한도보다 먼저 끝났다. OPERATIONAL 상태 뒤의 사건은 6.4.2 기록에 없다.
- **순서 서술**: "자기 등록 뒤 +x ms"는 기록 시계로 쟀다. Kairos의 HB 판단이 아니다. 재생은 행위자 사이 간선이 0이고, merged order를 쓰지 않는다.
- **Kairos 라벨의 범위**:
  - cur의 READY는 모두 반환 코드 좁힘(P1 조건부)에 기댄다. 독립 검사가 아니다.
  - `GetAppIDInternal`은 판정되지 않았다.
  - 발생 수준 owner와 실행 관계는 서지 않았다.
- B′는 병합 kz-static과 Φ 프로필로 다시 만들었다. 부팅 때 변환에 쓴 E-ST B′와 다르다. 중립 계층이 같음은 확인했다(§1.3).

## 7. 남은 것

1. **[M][D-제안] `GetAppIDInternal` 입구 site**: manifest(`sites-64<V>.draft.kzs`)에 `CFE_ES_GetAppIDInternal`의 phase before site가 없다. 그래서 E-04의 내부 조회(6.4.1 44, 6.4.2 182)가 판정되지 않았다.
   - 고치려면 manifest를 바꾸고 파생 빌드를 다시 만들어야 한다. 그러면 새 부팅이 필요하다(같은 구성의 두 번째 부팅). 리드·PI 결정이 필요하다.
2. **[M][D-제안] E-ID1과 `pthread_t` 재사용**: SAMPLE_APP 스레드가 SAMPLE_LIB 요청(owner 5)에 `confirmed`로 묶였다(BOOT 보고 항목 2와 같은 것).
   - 두 spawn이 같은 `pthread_t`를 낸 경우 `confirmed`를 막을지 정해야 한다.
   - 이 기록에는 막을 근거가 있다: 알린 id(site 20 = 7)와 그 actor를 낳은 spawn의 id(site 16 = 8)가 다르다. 엔진 변경이라 이 작업에서 하지 않았다.
3. **[M] 발생 수준 owner**: 프로필에 owner 종류가 없어 confirmed id 4개가 기록 키로 옮겨지지 않았다. 관계는 모두 `ownership-undetermined`다. EX E-11의 발생 identity 줄 기대가 이 때문에 서지 않는다. 프로필 담당 확인이 필요하다.
4. **[M] E-KEY 전제**: 실제 기록에서는 키가 호출 안 점의 값으로 정해졌다(§3.1). CB §10.3 메모 2의 "키가 비어 per-key 서수와 실행 관계가 서지 않는다"와 E-KEY 제안은 이 기록에 맞지 않는다. 합성 trace에서 키가 빈 것과 무엇이 다른지 확인이 필요하다 [U].
5. **[M][U] 행위자 사이 HB 간선 0**: R2-P-TASK(phase)가 간선을 만들지 않았다. "spawn returns 0, out-parameter bound by the id 0". 6.4.2의 구조적 순서도 반환 코드 좁힘으로만 선다(CB §10.3 메모 2와 같은 결론). 원인은 확인하지 않았다.
6. **[M] `es.task`의 정적 닫힘**: `--facts`를 줬지만 이 종류에는 "static closure: not given"이다. 종류가 정적 위치와 묶이지 않은 것으로 본다 [I].
7. **[D-제안] 6.4.2 정지 한도** (BOOT 보고 항목 3): 레퍼런스에서 유도한 한도로 다시 부팅할지 정해야 한다. 예는 시작 스크립트 시간 + `CFE_ES_STARTUP_SCRIPT_TIMEOUT_MSEC`다. 같은 구성의 두 번째 부팅이 된다.
8. **`appreq`**: C5에는 `unspecified_by_reference`다(EX §4). 이번 기록에는 (나)의 실패 조회가 없다. 그래서 결정되더라도 이 두 기록의 라벨은 바뀌지 않는다 [I].
9. **EX 개정**: §5의 불일치 행(E-03, E-04, E-08, E-11, E-12, E-14)의 개정 여부와, 이 두 실행을 개발 자료로 옮길지는 리드가 정한다.
10. **`--core-request-caller`와 extract 연산**: C5 레퍼런스가 정하지 않는다(§1.4). 판정 영향 0은 확인했다.
11. **CB §7.2**: 항목 3("O2 경로가 닿는지")은 이 기록으로 "닿음(조건 2 변형)"이다. 항목 1(32비트 `libc.so.6`의 저장·생성 순서)은 여전히 [U]다. 사례 파일 A8의 갱신 여부는 사례 파일 담당이 정한다.

## 8. 보존

- 묶음 `research/cfs_procrace/records/c5-20261029-replays/`(`env/tools/preserve_bundle.sh`):
  - 원본 `/home/user/work/c5run/replay`(B′ `bprime/` 포함), 파일 68, 7,078,577 B.
  - `bundle.tar.xz.00` 369,596 B, `MANIFEST.sha256`, `PARTS.sha256`, `INFO.txt`(repo `f4ae15b`).
- `verify_bundle.sh`(원격에서 다시 받아 대조)는 push 전이라 돌리지 않았다. 그래서 아직 "보존됨"으로 적지 않는다(PD29 A2). 부팅 묶음 `c5-20261029-boots`도 같은 상태다.
- 다시 만들기: `env/c5/replay_c5.sh all`. 입력은 `/home/user/work/c5run/{641,642}`, `/home/user/work/c5tools/build/64<V>/`(`b/`, `est_instr/compile_commands.json`, `binds_instr.kzs`), 병합 트리의 도구다. 결과 해시는 결정적이다. 처음 실행과 `f4ae15b` 재실행에서 B′·`cur.kz`가 같았다 [M].
