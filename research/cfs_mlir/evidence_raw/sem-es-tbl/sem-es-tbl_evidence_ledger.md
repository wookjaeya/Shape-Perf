# sem-es-tbl 근거 원장: ES startup·lifecycle, Table Services, EVS·TIME 시작 의존성 (cFE 546a002 기준)

작성: 2026-10-01. 이 파일은 연구노트 본문을 쓰기 위한 근거 원장이다. 표기 규칙은 수정본 노트와 같다.

- **[확인-소스]**: 고정 commit의 소스를 직접 읽은 내용. 줄 번호는 아래 permalink 기준.
- **[확인-실행]**: 이 컨테이너에서 고정 commit을 빌드·실행해 관측한 내용. 특정 환경·설정에 한정.
- **[해석]**: 분석 연구에 대한 편집자의 추론. 출처가 직접 말한 내용이 아님.
- **[미확인]**: 확인하지 못했거나, 코드 읽기만으로 추정한 후보.
- **[원노트]**: 원노트(20260930_1)의 구상.

## 0. 고정점과 확인 범위

| 대상 | commit | 비고 |
| --- | --- | --- |
| nasa/cFE | `546a002515be5a1e3b66f9ae2c14f948d9cec76f` (2026-09-25T14:23:07-04:00) | 수정본 S25와 동일. `cfe_es.h`/`cfe_sb.h`/`cfe_tbl.h`/`cfe_es_api.c`/`cfe_sb_priv.c`의 blob SHA가 수정본 S08–S12 기록과 일치함을 `git ls-tree`로 재확인 |
| nasa/osal | `5befd8e9f6c62b44bb7ff4c15db8629c0e00efa0` (2026-09-24) | `dev` first-parent 중 cFE 시각 이전의 마지막 merge. cFE가 이 OSAL을 지정한 것은 아님 (선택 근거: 날짜) |
| nasa/PSP | `36c24cb953b047ab5525240579aeb06adf5ac11f` (2026-09-24) | 같은 방식으로 선택 |
| nasa/elf2cfetbl | `f97ba87035688f0cf82d22b762d42c3c7e4292a6` (2026-09-24) | native 빌드에 필요(mission_build.cmake가 table tool 요구) |

Permalink 접두어: `C=https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f`, `O=https://github.com/nasa/osal/blob/5befd8e9f6c62b44bb7ff4c15db8629c0e00efa0`. 예: `C/modules/es/fsw/src/cfe_es_start.c#L74-L236`.

읽은 파일 blob SHA(일부): cfe_es_start.c `83be95da…`, cfe_es_startupscript.c `2712d38f…`, cfe_es_apps.c `28998a7b…`, cfe_es_appctrl.c `e2570598…`, cfe_es_backgroundtask.c `3fb70f2e…`, cfe_tbl_api.c `5bc854e2…`, cfe_tbl_internal.c `aa69d062…`, cfe_tbl_registry.c `bb3a5dce…`, cfe_tbl_regrec.c `23b9cc72…`, cfe_tbl_loadbuff.c `5349fca8…`, cfe_tbl_load.c `386ed1e3…`, cfe_time_task.c `20a24929…`, cfe_evs_utils.c `fbe69ebd…`, OSAL os-impl-tasks.c `a7d9cb46…`. 전체 목록은 이 원장 작성 시 `git ls-tree` 출력으로 기록.

실행 환경(확인-실행): 4 vCPU, Linux 6.18.44, gcc 13.3.0, cmake 3.28.3, `SIMULATION=native`, debug build, `OSAL_CONFIG_DEBUG_PERMISSIVE_MODE=TRUE`(cFE `cmake/sample_defs/native_osconfig.cmake` L22-39). 컨테이너 root는 CAP_SYS_NICE는 있으나 CAP_SYS_RESOURCE가 없고 `/proc/sys/fs/mqueue/msg_max=10`이어서, root로 실행하면 SB pipe queue 생성이 실패해 cFE가 processor reset으로 종료됨(`runs/run1_default.log`). 그래서 모든 비교 실행은 uid 65534로 `setpriv`를 통해 수행(비root이면 OSAL BSP가 msg_max로 queue 깊이를 자름: `O/src/bsp/generic-linux/src/bsp_start.c#L55-L78`, `O/src/os/posix/src/os-impl-queues.c#L50-L105`). 즉 실행 결과는 pipe 깊이가 10으로 잘린 상태다.

주의: 보존된 R1 로그는 같은 디렉터리로 두 번째 실행한 것이어서 이전 실행의 PSP 공유 메모리가 남아 **PROCESSOR reset**으로 시작했다(로그 "Normal exit from previous cFE instance"). R2–R4와 순서 반복 20회는 모두 POWER ON reset으로 시작했다(`grep 'Starting the cFE with'`). R1(processor reset)과 R2(power-on)의 TBL·restart 관측이 같으므로 reset 종류가 그 결과를 바꾸지는 않았다. 실행 후 내가 만든 SysV shm segment는 모두 `ipcrm`으로 제거했다.

빌드·실행 산출물: `bundle/`(cFE·OSAL·PSP는 symlink, `sample_defs`는 수정 사본), 측정용 probe 앱 `bundle/apps/probe_common/probe_order.c`, `probe_tbl.c`, 실행 스크립트 `run_probe.sh`, 원시 로그 `runs/<run>/console.log`, 스레드 스냅샷 `runs/<run>/threads.txt`, 추출본 `runs/probe_results_summary.txt`, 반복 결과 `runs/order_repeats.txt`. probe 앱은 프레임워크 의미를 관측하기 위한 합성 코드이며 임무 코드가 아니다.

---

## 1. ES 시작 순서와 System State

### 1.1 상태 전이와 전이 주체 [확인-소스]

`CFE_ES_Main` (`C/modules/es/fsw/src/cfe_es_start.c#L74-L236`)은 PSP가 부르는 시작 task(OSAL root/main thread)에서 순차 실행되며, `CFE_ES_Global.SystemState`를 **이 함수만** 직접 대입한다.

| 순서 | 동작 | 위치 |
| --- | --- | --- |
| 1 | `SystemState = EARLY_INIT` | L88 |
| 2 | reset 변수·perf·filesystem 준비 | L121-170 |
| 3 | `SystemState = CORE_STARTUP` | L184 |
| 4 | `CFE_ES_CreateObjects()` (core module EarlyInit → core task 생성) | L189, 정의 L767-876 |
| 5 | `SystemState = CORE_READY` | L195 |
| 6 | `CFE_ES_StartApplications()` (startup script) | L202 |
| 7 | 모든 app이 `LATE_INIT` 이상이 될 때까지 대기, 시간 초과 시 syslog만 남기고 진행 | L211-214 |
| 8 | `SystemState = APPS_INIT` | L217 |
| 9 | 모든 app이 `RUNNING` 이상이 될 때까지 대기, 시간 초과 시 syslog만 남기고 진행 | L226-229 |
| 10 | `SystemState = OPERATIONAL` | L235 |

- 상태 enum 값: `UNDEFINED 0, EARLY_INIT 1, CORE_STARTUP 2, CORE_READY 3, APPS_INIT 4, OPERATIONAL 5, SHUTDOWN 6` (`C/modules/es/config/default_cfe_es_extern_typedefs.h#L184-L219`); App 상태: `UNDEFINED 0, EARLY_INIT 1, LATE_INIT 2, RUNNING 3, WAITING 4, STOPPED 5` (같은 파일 L264-294).
- `SystemState`는 `volatile sig_atomic_t` (`C/modules/es/fsw/src/cfe_es_global.h#L152`). 대기 측은 잠금 없이 polling한다(§2).
- 7·9단계의 대기는 `CFE_ES_MainTaskSyncDelay` (`cfe_es_start.c#L886-L944`): ES 공유 잠금 아래 app table을 훑어 `AppState < 목표`인 app 수를 세고, 0이 아니면 `OS_TaskDelay(CFE_PLATFORM_ES_STARTUP_SYNC_POLL_MSEC)` 후 반복, 남은 시간이 0이면 `CFE_ES_OPERATION_TIMED_OUT`. 기본값: poll 50 ms(`C/modules/es/fsw/inc/cfe_es_internal_cfg.h#L860`), timeout `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC` 1000 ms(L879).
- L204-210 주석: 모든 app이 시작하지 못해도 "fatal error가 아니며 계속 진행한다".

### 1.2 Core module의 2단계 시작 [확인-소스]

`CFE_ES_CreateObjects` (`cfe_es_start.c#L767-L876`):

1. **Phase 1 EarlyInit** (L777-799): `GLOBAL_CONFIGDATA.CoreObjectTable` 순서대로 모든 module의 `EarlyInit()`을 ES main 문맥에서 호출. 실패 시 `CFE_PSP_Panic` (L796). 이 시점에는 core task가 하나도 없다.
2. **Phase 2 TaskMain** (L801-873): `TaskMain`이 있는 module마다 app record를 `CFE_ES_AppType_CORE`로 예약하고 `CFE_ES_StartAppTask`로 task 생성 후, **그 core app이 `RUNNING`이 될 때까지** `CFE_ES_MainTaskSyncDelay(RUNNING, CFE_PLATFORM_CORE_MAX_STARTUP_MSEC)`로 기다린 다음 다음 module로 넘어간다 (L859-862). 실패·시간 초과 시 panic (L864-872). `CFE_PLATFORM_CORE_MAX_STARTUP_MSEC` = 30000 (`C/modules/core_private/config/default_cfe_core_private_internal_cfg.h#L68`).
- Module 순서 = `MISSION_CORE_MODULES` 목록 순서: `config, es, evs, fs, sb, tbl, time, osal, psp, msg, sbr, resourceid` (`C/cmake/mission_defaults.cmake#L21-L40`; 주석 L26 "the order of this list is the order in which they will be initialized"). 생성 코드: `C/cmake/target/CMakeLists.txt#L55-L70`. 결과적으로 task를 가진 core app 순서는 **ES → EVS → SB → TBL → TIME** (FS는 EarlyInit만).
- descriptor 구조: `C/cmake/target/inc/target_objtab.h#L51-L59` (`EarlyInit`, `TaskMain`, `AppCleanupCb`, `Priority`, `StackSize`). 예: `C/modules/evs/fsw/src/cfe_evs_objtab.c#L34-L39`.
- 각 core app은 자기 init 후 `CFE_ES_WaitForSystemState(CFE_ES_SystemState_CORE_READY, CFE_PLATFORM_CORE_MAX_STARTUP_MSEC)`를 부른다: ES `cfe_es_task.c#L134`, EVS `cfe_evs_task.c#L229`, SB `cfe_sb_task.c#L83`, TBL `cfe_tbl_task.c#L74`, TIME `cfe_time_task.c#L88`. core app이 `MinSystemState >= CORE_READY`로 부르면 자기 AppState가 `RUNNING`으로 올라가므로(§2), 이 호출이 곧 "다음 core app을 만들어도 된다"는 신호가 된다.
- 기본 core task priority(OSAL 값, 작을수록 높음): TIME 60, EVS 61, SB 64, ES 68, TBL 70; TIME child task(tone, 1Hz) 25 (`cfe_time_internal_cfg.h#L234-L240`, `cfe_evs_internal_cfg.h#L44`, `cfe_sb_internal_cfg.h#L340`, `cfe_es_internal_cfg.h#L44`, `cfe_tbl_internal_cfg.h#L51`).
- `modules/ta`(objtab에 `CFE_TA_CleanUpApp`)가 저장소에 있으나 기본 `MISSION_CORE_MODULES`에는 없다.

[확인-실행] 4개 설정 모두에서 core init event가 ES→EVS→SB→TBL→TIME 순서로 약 100 ms 간격으로 나타났다(`runs/probe_results_summary.txt` L35-39, L209-213, L384-388). priority(TIME 60이 가장 높음)와 무관하게 순서가 고정된다.

[해석] cFE #73(수정본 S01)에서 문제된 "task 생성 순서 ≠ 실행 순서 ≠ 초기화 완료 순서"는 현재 코드에서 (a) 모든 EarlyInit을 task 생성 전에 끝내고 (b) core app을 하나씩 만들고 RUNNING을 기다리는 구조로 core app 사이에서는 직렬화되어 있다. 따라서 현재 commit의 core startup을 #73의 결함 재현 대상으로 쓸 수는 없다. 원노트 §4.2의 "priority 때문에 TIME이 먼저 실행" 서술은 역사적 코드에 대한 것이며 현재 코드에는 그 경로가 없다. 역사적 재현에는 #73 당시 revision이 필요하다(미확보, §9).

### 1.3 #73 관련 방어 코드의 흔적 [확인-소스]

- `EVS_SendEvent`(EVS 내부용)는 `CFE_EVS_Global.EVS_AppID` 유효성을 확인하며 주석이 "이 함수가 CFE_EVS_TaskInit() 전에 다른 thread에서 호출될 수 있다"고 명시한다 (`C/modules/evs/fsw/src/cfe_evs_utils.c#L607-L635`, 주석 L613-616). EVS는 init 마지막에 `EVS_AppID`를 기록한다 (`cfe_evs_task.c#L311-L312`, 주석 "now that the rest of initialization is done").
- 새 task의 실제 entry 전에 `CFE_ES_TaskEntryPoint` → `CFE_ES_GetTaskFunction`이 **먼저 `OS_TaskDelay(50ms)`** 를 하고, 생성자가 task record에 AppId·EntryFunc를 채울 때까지 polling한다 (`C/modules/es/fsw/src/cfe_es_apps.c#L227-L273`, `L281-L309`; 생성자 쪽 기록 `L340-L378`). 즉 "AppId가 아직 없는 task가 API를 부르는" 경로를 막는다.

[해석] 이 두 지점은 원노트가 말한 "uninitialized AppID" 계열 문제에 대한 현재의 방어로 읽을 수 있다. 다만 #73의 수정 commit과 이 코드를 직접 대응시키는 작업은 하지 않았다 [미확인].

---

## 2. `CFE_ES_WaitForSystemState`와 `CFE_ES_WaitForStartupSync`

### 2.1 구현 [확인-소스]

`C/modules/es/fsw/src/cfe_es_api.c#L514-L608`:

1. ES 공유 잠금 아래 호출자의 app record를 찾고, `MinSystemState`에 대응하는 **호출자 자신의 AppState를 먼저 올린다** (L536-575):
   - core app: `MinSystemState >= CORE_READY`이면 `RUNNING`.
   - 외부 app: `>= SHUTDOWN` → `STOPPED`, `>= OPERATIONAL` → `RUNNING`, `>= APPS_INIT` → `LATE_INIT`, 그 외 `EARLY_INIT`.
   - 주석 L566-570: 호출 자체가 "호출자도 그 단계에 도달했다"는 선언이며, 그렇지 않으면 자기 자신을 기다려 항상 timeout된다.
2. 그 뒤 잠금 없이 `while (CFE_ES_Global.SystemState < MinSystemState)`를 `OS_TaskDelay(≤50ms)`로 polling (L584-605). 남은 시간이 0이 되면 `CFE_ES_OPERATION_TIMED_OUT`(0xc4000027) 반환 (L597-600). 신호는 semaphore가 아니라 ES main의 변수 대입(§1.1)이다.
3. `TimeOutMilliseconds = 0`이고 상태가 미도달이면 즉시 TIMED_OUT (L589-600 구조). 무한 대기 옵션은 없다(헤더 `C/modules/core_api/fsw/inc/cfe_es.h#L405-L409`).

`CFE_ES_WaitForStartupSync` (`cfe_es_api.c#L616-L619`)는 `void`이며 `CFE_ES_WaitForSystemState(OPERATIONAL, TimeOut)`의 반환값을 버린다.

### 2.2 헤더와 구현의 차이 [확인-소스]

- `cfe_es.h#L440-L444`: WaitForStartupSync의 timeout은 "at least 1000이어야 하며 더 작은 값은 올림된다"고 적혀 있으나 구현(L616-619)에는 올림이 없다. [해석] 헤더만 보고 의미를 모델링하면 틀린다.
- `cfe_es.h#L424-L429`: "다른 app들이 존재하고 실행 중이 될 때까지 기다려 packet을 보내기 전에 쓰라"(HS, SCH 예). 그러나 §1.1의 7·9단계가 1000 ms 후 진행하므로 OPERATIONAL은 "모든 app이 RUNNING"을 보장하지 않는다.

### 2.3 실행 확인 [확인-실행]

PROBE_SLOW는 진입 후 2.5 s 동안 ES sync API를 부르지 않는다. 4개 설정 모두에서:
- ES가 `Startup Sync failed - Applications may not have all initialized` → `APPS_INIT` → `Startup Sync failed - Applications may not have all started` → `OPERATIONAL` 순으로 진행 (summary L66-69, L240-243, L415-418).
- 다른 probe의 `WaitForSystemState(OPERATIONAL,5000)`는 `0x00000000`(성공)을 약 1.95–2.0 s 대기 후 반환 (summary L70, L76, L244, L250 등). 그 시점 PROBE_SLOW는 아직 RUNNING이 아니었다(이후 `waited_us=0~1`로 즉시 반환, L80, L254).

[해석]
- "startup sync 호출이 있다"는 사실, 심지어 그 호출이 **성공을 반환했다**는 사실도 다른 app의 구독·초기화 완료를 보장하지 않는다. 정적 분석에서 `cfs.sync.startup`을 HB 간선으로 넣으려면 "모든 app이 1000 ms 안에 RUNNING이 된 실행"이라는 조건부 간선이어야 하고, 그 조건은 소스만으로 결정되지 않는다.
- 호출이 곧 자기 상태 선언이므로, init 도중에 WaitForSystemState/WaitForStartupSync를 부르면 init 미완료 상태에서 RUNNING이 된다. "RunLoop 이전 코드 = 초기화"라는 단순 구획은 맞지 않는다.
- WaitForStartupSync는 반환값이 없어 timeout 경로와 성공 경로를 앱 코드에서 구분할 수 없다(수정본 §5.2의 결론과 일치).

---

## 3. Startup script 처리와 app 생성 순서·priority

### 3.1 파싱 [확인-소스] (`C/modules/es/fsw/src/cfe_es_startupscript.c`)

- 파일 선택 (L53-149): processor reset이면 먼저 volatile 경로(`/ram/cfe_es_startup.scr`, `CFE_PLATFORM_ES_VOLATILE_STARTUP_FILE`)를 시도, 실패하면 시작 인자 경로(기본 `/cf/cfe_es_startup.scr`).
- 줄 읽기 (L157-238): 한 글자씩 `OS_read`; `!` 또는 EOF에서 종료(L191-196), `;`에서 줄 끝(L199-203), `,`는 token 구분(L208-221), `isgraph`가 아닌 문자(공백 등)는 버림(L223-228).
- 처리 루프 (L246-309): 줄이 너무 길거나 token이 너무 많으면 경고 후 그 줄만 건너뜀(L287-295). 파싱된 줄은 **파일 순서대로** `CFE_ES_ParseFileEntry` 호출(L299).
- `CFE_ES_ParseFileEntry` (L317-422): token < 8이면 거부(L335-339). `CFE_APP`: priority가 `OS_MAX_TASK_PRIORITY`보다 크면 그 값으로 자름(L370-379), stack size는 그대로, exception action은 0(restart app) 외 값이면 processor restart(L390-399), 그다음 `CFE_ES_AppCreate`(L404). `CFE_LIB`: `CFE_ES_LoadLibrary`(L413). 반환 status는 호출자(L299)가 무시한다.

### 3.2 생성 [확인-소스]

- `CFE_ES_AppCreate` (`cfe_es_apps.c#L401-L570`): 이름 중복 검사·slot 예약(RESERVED) → module load(L509) → `CFE_ES_StartAppTask`로 main task 생성(L514-523) → record를 실제 ID로 확정(L537). **app마다 기다리지 않고** 다음 줄로 넘어간다(core app과 다름).
- `CFE_ES_StartAppTask` (L317-393): `OS_TaskCreate` 후에 잠금 아래 task record를 채운다(L340-378). 새 task는 `CFE_ES_TaskEntryPoint` → 50 ms 지연 후 record 확인(§1.3).
- Library 초기화 함수는 ES main 문맥에서 **동기적으로** 실행된다 (`cfe_es_apps.c#L681-L684`). [해석] script에서 app보다 앞에 놓인 library의 init 완료는 뒤 app의 생성보다 앞선다(같은 thread의 프로그램 순서). app 사이에는 그런 관계가 없다.

### 3.3 실행 확인: priority가 순서를 정하는가 [확인-실행]

startup script 순서: LO(200), HI(20), SLOW(100), OWN(90), SHR(95). 각 probe의 첫 문장에서 `CLOCK_MONOTONIC`과 실제 정책·priority·CPU를 기록.

| 설정 | 실제 정책 | main entry 순서 (5회 반복) |
| --- | --- | --- |
| CAP_SYS_NICE, CPU 4개 | `SCHED_RR`(policy 2), rtprio LO 22, HI 90, SLOW 60, OWN 64, SHR 62 | 5/5: LO, HI, SLOW, OWN, SHR (= script 순서) |
| CAP_SYS_NICE, `taskset -c 0` | 같은 RR 값 | 5/5: HI, OWN, SHR, SLOW, LO (= priority 순서) |
| 권한 없음, CPU 4개 | `SCHED_OTHER`(TS), rtprio 0 | 4/5 script 순서, 1/5 HI가 마지막 |
| 권한 없음, `taskset -c 0` | `SCHED_OTHER` | 5/5 script 순서 (별도 긴 실행 R4에서는 SHR이 OWN보다 먼저: 1회 예외) |

근거: `runs/order_repeats.txt`, `runs/R1_rr_allcpu/threads.txt` 등, `runs/probe_results_summary.txt`. 권한 없는 실행에서 OSAL은 `Could not setschedparam in main thread: Operation not permitted`를 남기고 계속 진행했다(permissive mode).

[해석]
- priority에 의한 시작 순서는 **(RT 권한) ∧ (단일 CPU)** 에서만 재현되었다. 4 vCPU에서 RR priority를 켜도 script 순서가 나왔다. 따라서 "priority 때문에 B가 A보다 먼저 실행된다" 유형의 시나리오를 Linux에서 재현하려면 CPU를 고정해야 하고, 반대로 다중 CPU 비행 플랫폼에 대한 주장으로 일반화하면 안 된다.
- 권한 없는 실행은 순서가 대체로 script 순서지만 결정적이지 않았다. 반복 수가 작으므로(5회) 확률을 주장하지 않는다.

---

## 4. OSAL POSIX의 task priority 처리 [확인-소스, OSAL 5befd8e]

- `OS_Posix_TaskAPI_Impl_Init` (`O/src/os/posix/src/os-impl-tasks.c#L227-L457`):
  - SCHED_FIFO와 SCHED_RR의 min/max를 조회(L156-213). 둘 다 가능할 때 **FIFO는 그 범위가 `OS_MAX_TASK_PRIORITY`(255)보다 넓을 때만** 선택하고, 아니면 RR을 선택(L355-371). Linux의 1–99 범위에서는 RR이 선택됨 [확인-실행: `Selected policy 2 for RT tasks, root task = 99`].
  - 가장 높은 RT priority는 root(main) thread용으로 남긴다(L391-399).
  - `pthread_setschedparam`이 실패하면(권한 없음) `EnableTaskPriorities=false`로 남는다(L416-428).
  - `OSAL_CONFIG_DEBUG_PERMISSIVE_MODE`가 정의되지 않았으면 이 경우 `OS_ERROR`로 초기화를 실패시킨다(L436-446). OSAL 기본값은 FALSE(`O/default_config.cmake#L159-L181`), cFE native 예제 설정은 TRUE(`C/cmake/sample_defs/native_osconfig.cmake#L22-L39`).
- `OS_TaskCreate_Impl` (L603-632): priority가 꺼져 있으면 OSAL 기록상 priority를 255로 덮어쓴다(L618-622). 켜져 있으면 `PTHREAD_EXPLICIT_SCHED`, 선택된 정책, `OS_PriorityRemap` 값을 지정(L530-570).
- `OS_PriorityRemap` (L80-111): 0 → 최고, 255 → 최저, 나머지 253개 OSAL 값을 Linux의 남은 범위(RR 기준 약 96단계)에 선형 압축. [해석] 서로 다른 OSAL priority가 같은 Linux priority로 합쳐질 수 있다(실행에서 LO 200과 ES background 200이 모두 rtprio 22).
- OSAL mutex는 `PTHREAD_PRIO_INHERIT` (`O/src/os/posix/src/os-impl-mutex.c#L82`).
- task 생성 시 CPU affinity를 설정하지 않는다. affinity는 `OS_TaskSetAffinity` API를 부를 때만 적용되고 `OSAL_CONFIG_MAX_CPUS` 기본값은 1(`O/default_config.cmake#L399`) [확인-소스: osapi-task.c에 affinity 호출 없음은 grep으로 확인]. [확인-실행] 4 CPU 실행에서 thread들이 여러 CPU에 분산됨(`runs/R1_rr_allcpu/threads.txt` PSR 열).

[해석] 비행 RTOS(단일 코어, 엄격한 선점 priority)와 Linux native 실행은 시작 순서 의미가 다르다. 동적 재현 실험을 설계할 때 (a) 권한, (b) CPU 고정, (c) permissive 여부, (d) msg_max에 따른 pipe 깊이 절단을 실험 조건으로 기록해야 한다.

---

## 5. `CFE_ES_RunLoop`, app 상태, `CFE_ES_ExitApp`

[확인-소스] `cfe_es_api.c`:
- `CFE_ES_RunLoop` (L426-506): 호출 task의 실행 counter 증가(L441). `*RunStatus != APP_RUN`이면 잠금 없이 즉시 `false`(L453-456). 아니면 잠금 아래 **AppState가 RUNNING 미만이면 RUNNING으로 올림**(L469-472), `ControlReq.AppControlRequest != APP_RUN`이면 그 값을 `*RunStatus`에 써 주고 `false`(L478-492).
- `CFE_ES_ExitApp` (L292-418): pending 요청이 없을 때만 ExitStatus를 기록(L325-328). 외부 app은 `AppState = STOPPED` 후 `OS_TaskDelay(500)` 무한 루프로 ES가 지울 때까지 대기(L393-414). core app은 `CORE_APP_INIT_ERROR`이면 **processor reset**(L338-363), `CORE_APP_RUNTIME_ERROR`이면 task 종료(L365-385).

[확인-실행] root로 실행했을 때 SB pipe 생성 실패로 EVS init이 실패하자 `CFE_ES_ExitApp: CORE Application CFE_EVS Had an Init Error.` → processor reset으로 종료(`runs/run1_default.log` L47-58). ES task 자신은 init 실패 시 즉시 종료하지 않고 `AppRunStatus`만 바꾼 뒤 `WaitForSystemState(CORE_READY)`를 부르고(이 호출로 RUNNING이 됨), 그 대기가 끝난 뒤에야 `CFE_ES_ExitApp(AppRunStatus)`를 부른다(`cfe_es_task.c#L114-L134`, `L192-L200`). 그 사이 ES main은 ES를 RUNNING으로 보고 다음 core app(EVS)을 생성했다(로그 L47-57).

[해석] RunLoop의 첫 호출은 암묵적 RUNNING 선언이다. 분석기에서 "앱 준비 완료" 이벤트는 WaitForSystemState/WaitForStartupSync/RunLoop 중 **최초로 도달하는 것**이며, 그 시점이 구독 완료 뒤인지 확인해야 한다.

---

## 6. Child task [확인-소스]

- `CFE_ES_CreateChildTask` (`cfe_es_api.c#L1202-L1294`): main task에서만 호출 가능(L1268-1274), 새 task record의 AppId는 **부모 app ID**(L1277, L1290 → `CFE_ES_StartAppTask`). 시작 경로는 main task와 같다(50 ms 지연 handshake 포함).
- `CFE_ES_GetAppRecordByContext` (`C/modules/es/fsw/src/cfe_es_resource.c#L308-L338`)는 task record의 AppId로 app record를 찾으므로, child task에서 부르는 `CFE_ES_GetAppID`·`RunLoop`·`WaitForSystemState`·TBL·EVS·SB API는 모두 **부모 app 이름으로** 동작한다.
- `CFE_ES_DeleteChildTask` (L1332-1440)는 main task 삭제를 거부, `CFE_ES_ExitChildTask` (L1448-1497)는 main task에서 호출을 거부.
- TIME core app은 init 초기에 child task 2개(tone, 1Hz; priority 25)를 만든다(`C/modules/time/fsw/src/cfe_time_task.c#L175-L199`).

[해석]
- TBL의 access descriptor는 (handle, AppId) 단위이므로 같은 app의 main·child task는 하나의 `LockFlag`를 공유한다(§8.6). 한 task의 ReleaseAddress가 다른 task가 쥔 포인터의 보호를 풀 수 있다 [미확인-실행: 코드 근거만].
- child task가 `RunLoop`이나 `WaitForSystemState`를 부르면 부모 app의 AppState가 바뀐다 [코드 근거, 실행 미확인].
- "app = 하나의 순차 실행 주체"라는 모델은 틀릴 수 있다. 분석 단위는 task이고, 소유·식별 단위는 app이다.

---

## 7. Restart / Reload / Delete

### 7.1 요청과 처리 경로 [확인-소스]

- `CFE_ES_RestartApp` / `ReloadApp` / `DeleteApp` (`cfe_es_api.c#L165-L284`)는 **요청만 기록**한다: `ControlReq.AppControlRequest = SYS_RESTART/SYS_RELOAD/SYS_DELETE`. Restart·Reload는 파일 존재를 `OS_stat`으로 먼저 확인.
- 대상 검증 `CFE_ES_LockUserAppRecord` (`cfe_es_apps.c#L72-L113`): **core app은 거부**(L87-93), **AppState가 정확히 RUNNING이 아니면 거부**(L94-100).
- 실제 처리: ES background task의 `CFE_ES_RunAppTableScan` (`C/modules/es/fsw/src/cfe_es_appctrl.c#L387-L499`):
  - `RUNNING`이고 요청이 있으면 `WAITING`으로 바꾸고 `AppTimerMsec = APP_KILL_TIMEOUT × APP_SCAN_RATE`(기본 5 × 1000 ms) 설정(L463-470).
  - `AppState > RUNNING`(WAITING 또는 STOPPED)이면 timer를 줄이고 0이 되면 `CFE_ES_ProcessControlRequest` 호출(L438-461, L483-491).
  - 앱이 RunLoop → ExitApp으로 먼저 `STOPPED`가 되면 timer는 초기값 0이므로 다음 scan에서 바로 처리된다(코드 구조에서 도출).
- `CFE_ES_ProcessControlRequest` (L318-379): `CFE_ES_CleanUpApp`(L353) 후 restart/reload이면 **cleanup 결과와 무관하게** `CFE_ES_AppCreate`로 새 app 생성(L359-362). 새 app은 **새 AppId**를 받는다(주석 L341-342).
- background job 표(`C/modules/es/fsw/src/cfe_es_backgroundtask.c#L74-L116`): app scan은 active 250 ms / idle 1000 ms. 같은 표에 "for coverage purposes"라고 주석된 항상-active job 2개가 production 코드에 포함되어 있다(L95-115). [해석] 이 때문에 background loop가 idle일 때도 999 ms마다 깨어난다. 정확한 scan 주기를 계측하지는 않았다 [미확인].

### 7.2 Teardown 내용과 순서 [확인-소스] (`cfe_es_apps.c#L1047-L1119`)

1. `GetResources` (L732-844): 잠금 아래 app의 task(main을 index 0으로), memory pool을 수집하고 task·app record를 RESERVED로 표시. 이때 **OS task는 아직 살아 있다.**
2. `FreeCoreResources` (L855-904): `CoreObjectTable` 순서로 각 module의 `AppCleanupCb` 호출 → EVS, SB, TBL, TIME 순(ES는 callback 없음).
   - EVS (`cfe_evs_task.c#L178-L198`): EVS 등록 해제.
   - SB (`C/modules/sb/fsw/src/cfe_sb_priv.c#L87-L124` → `cfe_sb_api.c#L352-L460`): app 소유 pipe마다 **모든 route에서 pipe 제거(구독 해제)**(L388-390), queue에 남은 message buffer를 꺼내 참조 해제(L417-433), OSAL queue 삭제(L438). zero-copy buffer 해제.
   - TBL (`C/modules/tbl/fsw/src/cfe_tbl_internal.c#L584-L641`): 그 app 소유 table의 dump 요청 제거, 그 app의 **모든 access descriptor 해제**(§8.7).
   - TIME (`C/modules/time/fsw/src/cfe_time_utils.c#L1018-L1041`): synch callback 제거.
3. `FreeTasks` (L915-952): child task 먼저, main task 마지막으로 OSAL 객체(queue, sem, mutex, timer, stream, module 등)와 task 삭제(`CFE_ES_CleanupTaskResources` L1207 이하, 객체별 처리 L1129-1199).
4. `FreePools` (L963-995), module unload (L1084-1098), record 해제 (L1004-1039).

[해석]
- 2단계(pipe·구독·table 해제)는 3단계(task 삭제)보다 앞선다. 앱이 요청에 협조하지 않아 아직 실행 중이거나 child task가 남아 있으면, 그 task는 자기 pipe·table이 사라진 뒤에도 잠시 실행될 수 있다(teardown 중 use-after-release 후보). 실행으로 확인하지 않았다 [미확인].
- Restart 후 새 instance는 pipe 생성·구독을 다시 해야 한다. cleanup과 재구독 사이에 발행된 message는 수신자 0개 경로(수정본 §4.2)로 사라진다. 다른 app이 보낸 일회성 message(예: 시작 시 한 번 보내는 설정 명령)는 재시작된 app에 다시 오지 않는다.
- AppId가 바뀌므로 다른 app이 보관한 AppId(예: `CFE_ES_GetAppIDByName` 결과)는 stale이 된다.

### 7.3 실행 확인 [확인-실행]

PROBE_SHR가 `t+8s`에 `CFE_ES_RestartApp(PROBE_OWN)` 호출:
- R1: PROBE_OWN이 0.49 s 뒤 RunLoop를 빠져나와(`RunStatus=5`=SYS_RESTART) ExitApp, 새 instance 진입은 요청 후 약 1.58 s, 새 AppId(1114121 → 1114123) (summary L97-105). R2: 0.014 s 뒤 exit, 약 1.63 s 뒤 새 instance(L271-279).
- 요청 후 cleanup 전까지 PROBE_SHR의 `GetAddress(T)`는 계속 성공(옛 table 유효), cleanup 후 `CFE_TBL_ERR_UNREGISTERED`(0xcc000009, ptr=NULL).

[해석] 관측된 재시작 지연(약 1.6 s)이 기본 kill timeout(5 s)보다 짧은 것은 앱이 scan보다 먼저 STOPPED가 된 경로와 맞는다. 앱이 협조하지 않으면 5 s 경로가 될 것이다(미실행). 재시작 시점의 순서는 앱의 RunLoop 주기와 ES scan 주기 사이의 경쟁으로 정해진다.

---

## 8. Table Services

### 8.1 구조 요약 [확인-소스]

이 commit의 TBL은 transaction(`cfe_tbl_transaction.c`), access descriptor(`cfe_tbl_accdesc.c`), registry record(`cfe_tbl_regrec.c`), load buffer(`cfe_tbl_loadbuff.c`)로 나뉘어 있다. 모든 registry 조작은 `CFE_TBL_LockRegistry`(OSAL mutex, `cfe_tbl_internal.c#L325-L342`; POSIX에서는 priority inheritance)를 쓴다. 단, **모든 경로가 끝까지 잠금을 유지하지는 않는다**(§8.4).

### 8.2 Register / Share / Unregister [확인-소스]

- `CFE_TBL_Register` (`C/modules/tbl/fsw/src/cfe_tbl_api.c#L48-L176`): 이름 → "AppName.RawName". 잠금 아래 중복 검사(`cfe_tbl_registry.c#L302-L365`): 같은 owner면 같은 크기일 때 `CFE_TBL_WARN_DUPLICATE`, 다른 크기면 `ERR_DUPLICATE_DIFF_SIZE`; **owner가 다르면(owner 없음 포함) `CFE_TBL_ERR_DUPLICATE_NOT_OWNED`**(L347-356). 마지막 단계에서 `OwnerAppId` 기록(api.c L145-149).
- 등록 직후 active buffer는 **dump-only 내부 buffer일 때만** 설정된다. 일반 table은 첫 load 전까지 "never loaded"(`cfe_tbl_regrec.c#L372-L387`, `cfe_tbl_regrec.h#L465-L469`).
- `CFE_TBL_Share` (api.c L184-236): 이름으로 registry를 찾아 새 access descriptor 연결. 연결 시 `Updated = (dump-only 아님) ∧ (이미 load됨)`(`cfe_tbl_registry.c#L389`). [해석] 공유자는 공유 직후 첫 GetAddress에서 `CFE_TBL_INFO_UPDATED`를 받는다 [확인-실행: R1 L122-123].
- `CFE_TBL_Unregister` (api.c L244-279) → `CFE_TBL_TxnReleaseAccDesc` (`cfe_tbl_accdesc.c#L169-L193`): 호출자가 owner면 `OwnerAppId = UNDEFINED`(un-owned, 아직 free 아님). descriptor 연결을 끊고, **owner가 없고 남은 연결도 없을 때만** buffer와 registry record를 해제(`cfe_tbl_registry.c#L136-L166`).
- `LocateRegRecByName`은 owner 유무와 관계없이 "used" record를 이름으로 찾는다(`cfe_tbl_regrec.c#L133-L157`). Register의 주석(api.c L145-148)은 "unowned entry는 이름 비교 대상이 아니다"라고 하지만, 중복 검사 경로에서는 unowned record가 이름으로 잡힌다.
- 헤더 `cfe_tbl.h#L290-L294`: 운용 중 register/unregister는 피하고, 공유 table은 "competing requests로 인한 race condition"에 주의하라고 명시. L296-297: 모든 접근 연결이 끊길 때까지 메모리에서 제거되지 않음.

### 8.3 Owner 재시작과 공유자: 실행 확인 [확인-실행]

R1·R2(및 R3·R4의 같은 시나리오)에서:
1. PROBE_SHR가 T(single), D·E(double)를 Share한 상태에서 PROBE_OWN 재시작.
2. cleanup이 OWN의 access descriptor를 해제 → T·D·E는 un-owned로 남음(SHR 연결 때문에 free되지 않음).
3. 새 OWN instance의 `CFE_TBL_Register("T")` = **`0xcc00000d` (`CFE_TBL_ERR_DUPLICATE_NOT_OWNED`)**, syslog `Registering Duplicate Table 'PROBE_OWN.T' owned by App(0)`; 0.5 s마다 재시도해도 계속 실패 (summary L105-159).
4. SHR의 `GetAddress(T)` = `CFE_TBL_ERR_UNREGISTERED`, ptr NULL.
5. SHR가 `Unregister(T/D/E)` 한 직후 OWN의 다음 재시도가 성공(L167-171), table 내용은 처음부터(NEVER_LOADED → Load v1). SHR는 다시 `Share`해야 새 table을 본다(L178).

[해석] 공유 table을 가진 앱의 재시작 성공 여부가 **다른 앱의 Unregister 시점**에 달려 있다. 이것은 앱 간 lifecycle 순서 의존성의 구체적 사례이며 원노트 §24-D("Cross-application stale handle")를 소스와 실행으로 뒷받침한다. 다만 결과는 "공유자의 stale handle 사용"보다 "**소유자의 재등록 실패**"로 나타난다. 헤더의 Unregister 설명(L286-288, "owning application was removed… CS app is an example")은 공유자가 이를 처리해야 함을 시사한다. 실제 공개 앱(CS 등)이 이를 처리하는지는 확인하지 않았다 [미확인].

### 8.4 Load와 Update가 적용되는 시점 [확인-소스]

- **소유 앱의 `CFE_TBL_Load`** (api.c L287-348): owner만 가능. registry 잠금을 잡았다가 **곧바로 풀고**(L305-306, 주석 "As all ops are confined to this registry entry alone, release the lock"), 잠금 없이 `ValidateLoadRequest` → 데이터 적재 → **검증 함수를 호출자 문맥에서 즉시 실행**(`cfe_tbl_load.c#L829-L886`, 호출 L855) → `CFE_TBL_LoadFinish`(L342-345)가 `CFE_TBL_UpdateInternal`을 **즉시** 호출(`cfe_tbl_load.c#L894-L952`, L919). 즉 앱 자신의 Load는 Manage 없이 바로 활성화된다(잠긴 경우 제외).
- **`CFE_TBL_Update`** (api.c L356-431): owner만 가능, registry 잠금을 쥔 채 `UpdateInternal` 호출(L369-379). pending load가 없으면 `CFE_TBL_INFO_NO_UPDATE_PENDING`.
- **`CFE_TBL_UpdateInternal`** (`cfe_tbl_internal.c#L467-L554`):
  - load buffer가 table 전용(private)이면 그대로 active로 전환(L493-496): double-buffered table과 single-buffered의 **첫** load.
  - 공유 load buffer(single-buffered의 2번째 이후 load)이면 `CFE_TBL_GetInactiveBufferExclusive`로 table의 유일한 buffer를 확보하려 하고, 어떤 access descriptor든 `LockFlag`가 그 buffer를 가리키면 `CFE_TBL_INFO_TABLE_LOCKED`(0x4c000018, 양수=경고)로 **갱신을 미룬다**(L497-512; 검사 `cfe_tbl_regrec.c#L47-L59`, `L278-L317`). load는 in-progress로 남는다.
  - 성공 시 데이터 복사(L522-536) → active 전환(L538) → 모든 access descriptor에 `Updated=true`(L542, L562-576) → critical table이면 CDS 갱신(L545-548) → working buffer 폐기(L550).
- **지상 경로** (TBL task 명령): `LoadCmd`(`cfe_tbl_task_cmds.c#L376-L464`)가 inactive/working buffer에 적재 → `ValidateCmd`(L540-597)가 검증 요청 생성(검증 함수가 있으면 owner의 다음 Manage에서 실행) → `ActivateCmd`(L605-649)가 `IsValid`인 buffer에 `ActivateReq=true`(`cfe_tbl_loadbuff.c#L524-L558`) → owner의 `CFE_TBL_Manage`(api.c L731-781)가 `GetStatus` → `Validate`/`Update`를 수행하고 성공 시 `CFE_TBL_INFO_UPDATED` 반환.
- `GetStatus`(registry.c L406-430) 우선순위: validation pending > update pending > dump pending.
- `CFE_TBL_Validate`(api.c L603-723): 검증 함수는 registry 잠금을 푼 뒤(L644) 호출자(owner) 문맥에서 실행(L655-664).
- 헤더 자체의 모순: `cfe_tbl.h#L200-L201`은 검증 함수가 "Table Management Service 문맥"에서 실행된다고, L214-215는 "Application 문맥"에서 실행된다고 적는다. 구현은 후자.

[확인-실행] single-buffered T: OWN이 주소를 쥔 채 `Load(T,v2)` → `0x4c000018`(TABLE_LOCKED), 쥔 포인터는 여전히 v1; Release 후 `GetStatus`=`0x4c000004`(UPDATE_PENDING), `Manage`=`0x4c00000e`(INFO_UPDATED), 이후 값 v2 (summary L61-64). cFE 자체 기능 시험도 같은 동작을 기대하며 "This call shouldn't be necessary"라는 주석을 단다(`C/modules/cfe_testcase/src/tbl_content_access_test.c#L90-L99`).

[해석]
- "update는 언제 적용되는가"는 경로마다 다르다: 앱 자신의 Load는 즉시, 지상 load는 owner가 Manage/Update를 부를 때. 잠금 때문에 미뤄지면 **다시 Manage를 불러야** 한다. 알림 message는 Validate/Activate 명령 시점에 한 번만 보내지므로(§8.8) message 구동형 앱은 미뤄진 update를 놓칠 수 있다(아래 §8.8 해석).
- [미확인·후보] `CFE_TBL_Load`가 registry 잠금 없이 `UpdateInternal`까지 진행한다는 점은 공유자의 `GetAddress`(잠금 아래 LockFlag 설정)와의 TOCTOU 후보를 만든다: single-buffered table을 owner가 재-load할 때 잠금 검사(GetInactiveBufferExclusive) 이후 복사(LoadBuffCopyData) 전에 공유자가 GetAddress로 같은 buffer를 잡으면 복사 중인 데이터를 읽을 수 있다. 같은 이유로 owner의 Load와 TBL task의 LoadCmd가 `ValidateLoadRequest`/`GetWorkingBuffer`에서 겹칠 수 있다. 코드 구조에서 도출한 후보일 뿐이며 실행으로 재현하지 않았다. cFE UT에도 동시 Share/Unregister 가능성을 인정하는 주석이 있다(`C/modules/tbl/ut-coverage/tbl_UT.c#L4058-L4062`).

### 8.5 Single vs double buffered [확인-소스 + 확인-실행]

- buffer 배치: table당 최대 2개(`cfe_tbl_loadbuff.c#L42`). single-buffered는 local index가 항상 0(L173-180)이고 갱신 시 공유 load buffer(`CFE_PLATFORM_TBL_MAX_SIMULTANEOUS_LOADS`개 pool)를 거쳐 그 한 buffer로 복사한다(`PrepareNewLoadBuff` L434-470). double-buffered는 두 전용 buffer를 번갈아 쓰며 갱신은 포인터 전환이다(internal.c L518-538).
- double-buffered에서 공유자가 **옛 active buffer를 쥐고 있어도** owner의 다음 Load는 다른 buffer에 적재·활성화되어 성공한다 [확인-실행: R1 `t+2s Load(D,v2)`, `Load(E,v2)` = 0, 쥔 포인터는 v1 유지 (summary L83-87)].
- 그다음 Load는 공유자가 쥔 buffer로 돌아와야 하므로 그 buffer가 잠겨 있으면 **`CFE_TBL_ERR_NO_BUFFER_AVAIL`(0xcc00000f)로 load 자체가 거부**되고 pending으로 남지 않는다(`GetStatus(E)=0`) [확인-실행: summary L90-91]. Release 후 재시도 성공(L96).

[해석] "주소를 보유한 동안 table update가 일어날 수 없다"(헤더 L535-539, 수정본 §7.1)는 single-buffered의 **API Update 경로**에 대해서만 정확하다. double-buffered에서는 update가 일어나되 보유 포인터가 옛 내용을 계속 가리키고(일관된 stale), 그다음 load가 거부된다. 또한 single-buffered에서도 update는 "막히는" 것이 아니라 "미뤄진다(INFO_TABLE_LOCKED)". 분석기의 table 모델은 buffering 옵션별로 달라야 한다.

### 8.6 GetAddress / GetAddresses / ReleaseAddress [확인-소스 + 확인-실행]

- `CFE_TBL_GetAddress` (api.c L439-470) → `CFE_TBL_TxnGetTableAddress` (`cfe_tbl_registry.c#L174-L220`):
  - owner가 없으면 `CFE_TBL_ERR_UNREGISTERED`, `*TblPtr=NULL` (L181-191).
  - **한 번도 load되지 않았으면 `CFE_TBL_ERR_NEVER_LOADED`, `*TblPtr=NULL`, LockFlag 미설정** (L192-196).
  - 정상이면 `LockFlag=true`, `BufferIndex=현재 active`, 포인터 반환, 상태는 `TxnGetNextNotification`(NEVER_LOADED가 UPDATED보다 우선, `cfe_tbl_transaction.c#L297-L315`), 그다음 `Updated=false`로 지움(L216) → `INFO_UPDATED`는 descriptor당 한 번만 보고.
- **헤더와 구현의 불일치**: `cfe_tbl.h#L540-L544`(및 GetAddresses L620-624)는 NEVER_LOADED일 때도 "all zero content의 유효한 table 포인터를 반환하며, 이 포인터를 release해야 load할 수 있다"고 적는다. 구현과 실행은 **NULL 포인터**를 반환한다 [확인-실행: 모든 실행에서 `GetAddress(T) BEFORE load st=0xcc000005 ptr=(nil)`, summary L50, L220, L399]. Release도 `NEVER_LOADED`를 반환(L51). cFE 기능 시험은 반환 코드만 확인하고 포인터는 확인하지 않는다(`tbl_content_access_test.c#L52-L53`).
  - [해석] 수정본 노트 §7.1의 두 번째 [확인된 사실]("`CFE_TBL_ERR_NEVER_LOADED`가 반환되는 경우에도 0 내용의 유효한 table 포인터를 얻을 수 있으며…")은 헤더(S10)에 근거한 것으로, 이 commit의 구현과 맞지 않는다. "헤더는 그렇게 말하지만 구현은 NULL을 반환한다"로 고쳐야 한다. 헤더를 믿고 NEVER_LOADED 뒤 포인터를 역참조하는 앱은 NULL 역참조가 된다(정적 분석이 잡을 수 있는 구체적 API 오용 패턴).
- `CFE_TBL_ReleaseAddress` (api.c L478-509): 그 handle의 `LockFlag=false`만 하고(L489) 대기 중인 알림 상태를 반환. **어느 포인터를 놓는지 구분하지 않는다.**
- 보호 단위는 (handle) 하나의 `(LockFlag, BufferIndex)`다. 같은 handle로 Release 없이 GetAddress를 두 번 하면 보호가 마지막 buffer로 **옮겨 가고**, 첫 포인터는 보호를 잃는다 [확인-실행: double-buffered D에서 SHR이 t+1s(포인터 A)·t+3s(포인터 B) 두 번 GetAddress → OWN의 t+4s `Load(D,v3)` 성공 → 아직 release하지 않은 첫 포인터 A의 값이 1에서 **3으로 바뀜** (summary L81-92)].
  - [해석] 헤더는 "update나 blocking 호출 전에 release하라"고만 하고 중첩 GetAddress를 금지하지 않는다. 이는 "주소 획득~해제 구간은 보호된다"는 원노트 Table lifecycle 모델(Acquire → Use → Release)이 **포인터 단위로는 성립하지 않음**을 보여 준다. 분석기는 handle 단위 lock 상태를 추적하고, 재획득 이전 포인터의 사용을 별도 경고 대상으로 다뤄야 한다. 같은 app의 main/child task가 handle을 공유하는 경우(§6)도 같은 구조다.
- `CFE_TBL_GetAddresses` (api.c L517-561): handle마다 개별 transaction. 앞의 handle이 성공(lock 설정)한 뒤 뒤의 handle이 실패해도 계속 진행하며, `CFE_ES_ERR_RESOURCEID_NOT_VALID`일 때만 중단(L553-557). 반환값은 **첫 번째 non-success 상태 하나**(L548-551). [해석] 전체 반환이 NEVER_LOADED여도 일부 포인터는 잠긴 유효 포인터이고, 일부는 NULL이다. 반환값 하나로 "아무것도 잠기지 않았다"고 판단하면 release 누락이 생긴다. `CFE_TBL_INFO_UPDATED`와 오류가 섞일 때도 첫 값만 보인다.
- `CFE_TBL_ReleaseAddresses` (L569-595): 모두 release하고 첫 non-success 반환.
- 헤더 L530-534의 "blocking … 낮은 priority 앱의 priority가 자동으로 올라간다": 구현에서 GetAddress가 기다릴 수 있는 지점은 registry mutex이고, POSIX OSAL mutex는 priority inheritance다(§4). 그러나 갱신 경로 중 소유 앱의 Load는 갱신 동안 이 mutex를 쥐지 않는다(§8.4). [해석] 헤더의 blocking 설명을 "update와 GetAddress는 항상 상호 배제된다"는 보장으로 모델링하면 안 된다.

### 8.7 App 삭제 시 TBL 정리 [확인-소스]

`CFE_TBL_CleanUpApp` (`cfe_tbl_internal.c#L584-L641`): 그 app 소유 table의 dump 요청을 제거하고, `AccessDescPtr->AppId == 삭제 app`인 descriptor마다 `TxnReleaseAccDesc`(§8.2). 따라서 owner의 table은 un-owned로 남고(공유자가 있으면) 공유자의 descriptor는 그대로다. [해석] 삭제·재시작 후의 결과는 §8.3과 같다.

### 8.8 Notification by message [확인-소스]

- `CFE_TBL_NotifyByMessage` (헤더 `cfe_tbl.h#L754-L796`): owner만 등록 가능(L767). 
- 전송 `CFE_TBL_SendNotificationMsg` (`cfe_tbl_internal.c#L744-L778`): TBL task 문맥에서 `CFE_SB_TransmitMsg`. 실패하면 event만. 호출 지점은 **ValidateCmd에서 새 요청이 생겼을 때**(`cfe_tbl_task_cmds.c#L582`), **ActivateCmd 성공 시**(L630), dump 관련(`cfe_tbl_dump.c#L312`).
- [해석] 알림은 "명령이 들어왔다"는 일회성 신호다. owner가 알림을 받고 Manage를 불렀는데 공유자의 잠금 때문에 `INFO_TABLE_LOCKED`가 나오면, 재알림은 없다. 앱이 주기적으로 Manage를 부르지 않고 알림 message에만 반응하면 update가 계속 pending으로 남을 수 있다. 알림 message는 SB의 일반 전달 의미를 따르므로 구독 전·pipe 가득 참 상태면 사라진다(수정본 §4.2). 실행 확인은 하지 않았다 [미확인].

### 8.9 status 코드 (헤더 `C/modules/core_api/fsw/inc/cfe_error.h`)

`CFE_TBL_INFO_UPDATE_PENDING 0x4c000004` (L913), `CFE_TBL_ERR_NEVER_LOADED 0xcc000005` (L921), `CFE_TBL_ERR_UNREGISTERED 0xcc000009` (L958), `CFE_TBL_ERR_DUPLICATE_NOT_OWNED 0xcc00000D` (L987), `CFE_TBL_INFO_UPDATED 0x4c00000E` (L998), `CFE_TBL_ERR_NO_BUFFER_AVAIL 0xcc00000F` (L1007), `CFE_TBL_INFO_TABLE_LOCKED 0x4c000018` (L1080), `CFE_EVS_APP_NOT_REGISTERED 0xc2000002` (L258), `CFE_ES_OPERATION_TIMED_OUT 0xc4000027` (L622). [해석] `0x4c…`(INFO)는 양수라 `status >= CFE_SUCCESS` 식의 검사에서 성공으로 취급된다. 분석기에서 "성공 경로"를 `== CFE_SUCCESS`로 정의하면 INFO_UPDATED를 받은 정상 경로를 놓친다.

---

## 9. EVS 등록 요구 [확인-소스 + 확인-실행]

- `CFE_EVS_Register` (`C/modules/evs/fsw/src/cfe_evs.c#L42-L111`): 호출자 AppId로 entry를 찾아 **memset으로 초기화**한 뒤 filter 설정. BINARY 외 scheme은 `CFE_EVS_UNKNOWN_FILTER`. 재등록은 이전 filter·counter를 지운다.
- `CFE_EVS_SendEvent` (L119-161): 등록되지 않은 app이면 `EVS_NotRegistered`(`cfe_evs_utils.c#L153-L182`)가 app당 한 번 "not registered" event·syslog를 남기고 `CFE_EVS_APP_NOT_REGISTERED`를 반환. event는 보내지지 않는다. crash나 대기는 없다.
- EVS 자체의 시작 의존성: EarlyInit(`cfe_evs_task.c#L54-L170`)은 reset 영역·mutex·log 준비. TaskInit(L266-322)은 `CFE_ES_GetAppID` → `CFE_EVS_Register` → `CFE_SB_CreatePipe` → `Subscribe` → `EVS_AppID` 기록 순.
- 정리: `CFE_EVS_CleanUpApp`(L178-198)이 등록 해제. 재시작된 app은 다시 Register해야 한다.

[확인-실행] 모든 probe에서 Register 전 SendEvent = `0xc2000002`와 syslog `App PROBE_xx not registered with Event Services` (summary L42, L45 등).

[해석] EVS 미등록은 "event 유실"이지 실행 순서 결함은 아니다. 다만 restart 직후 Register 전에 보낸 event, 또는 child task가 등록 전에 보낸 event는 조용히 사라진다(반환값만 남음).

---

## 10. TIME Services 시작 의존성 [확인-소스]

- `CFE_TIME_EarlyInit` (`cfe_time_task.c#L49-L57`) → `CFE_TIME_InitData` (`cfe_time_utils.c#L231` 이하): 전역 초기화, reset 변수 질의(processor reset이면 이전 시간 복원), `ClockSetState = NOT_SET`(L262), flywheel 상태(L263, L270), `PendingState = INVALID`(L275). 즉 **TIME task가 뜨기 전(EarlyInit 단계)부터 `CFE_TIME_GetTime`은 값을 반환하지만 시계는 "설정 안 됨" 상태**다.
- `CFE_TIME_TaskInit` (L363-395) 순서: `CFE_EVS_Register`(L148) → tone/1Hz semaphore 생성 → **child task 2개(priority 25) 생성**(L175-199) → `CFE_SB_CreatePipe`와 HK·tone·data·1Hz·ground command 구독(L210-287; client/server 구성에 따라 `Subscribe`/`SubscribeLocal` L128-135, server면 time request 구독 L263-272) → init event 전송(L295-315) → tone 신호 선택(L388-390) → PSP timebase `"cFS-Master"`에 1Hz timer callback 추가(L330-355; timebase가 없으면 조용히 생략).
- 의존 대상: ES(app·child task), EVS(Register·SendEvent), SB(pipe·구독), OSAL(semaphore·timer), PSP(timebase). core 직렬화(§1.2)로 ES·EVS·SB·TBL의 task init이 TIME보다 먼저 끝난다.
- [확인-실행] native 실행에서 syslog 시간 표기가 TIME 초기화 시점에 `1980-001-…`에서 `1980-012-14:03:20…`로 바뀌었다(processor reset 실행에서 이전 instance 시간 복원). syslog의 TIME 기반 시각은 startup 순서 측정에 쓸 수 없어 probe는 `CLOCK_MONOTONIC`을 따로 기록했다.

[해석] 앱이 시작 직후 읽는 시간은 유효해 보이지만 시계 상태가 NOT_SET/flywheel일 수 있다. freshness·timestamp 분석(수정본 §6.5)에서 "시간 값이 있다"와 "시계가 동기화되었다"를 구분해야 하며, 시계 상태 확인 API 사용 여부가 guard가 된다. TIME 1Hz/tone child task는 core app보다 높은 priority라 단일 코어 RT 환경에서는 다른 task를 선점한다.

---

## 11. 분석 모델에 주는 함의 정리 [해석]

| 원노트/수정본의 표현 | 이 조사에 따른 보정 |
| --- | --- |
| `cfs.sync.startup`을 준비 완료 HB로 사용 | OPERATIONAL은 1000 ms timeout 두 번 뒤 무조건 도달. WaitForStartupSync는 반환값 없음. 호출 자체가 자기 상태 선언. → 조건부 간선 또는 "보장 없음"으로 모델링 |
| priority가 startup 순서를 정함(#73 교훈) | 현재 core app은 직렬화되어 priority 무관. 외부 app은 RT 권한 ∧ 단일 CPU에서만 priority 순서가 재현됨. 다중 CPU·권한 없음에서는 script 순서 경향(비결정적) |
| table: Acquire → Use → Release 구간 보호 | 보호는 handle 단위 LockFlag 하나. 중첩 GetAddress, 같은 app의 여러 task, double-buffer의 다음 load에서 보호 범위가 달라짐. NEVER_LOADED는 NULL 포인터(헤더와 다름) |
| "Update blocked by outstanding reference" | single: INFO_TABLE_LOCKED로 지연(재 Manage 필요). double: update는 진행, 다음 load가 NO_BUFFER_AVAIL로 거부 |
| "Cross-application stale handle"(향후 과제) | 재시작된 owner의 Register가 DUPLICATE_NOT_OWNED로 실패하고, 공유자는 UNREGISTERED를 받음. 공유자가 Unregister해야 해소. 실행으로 재현됨 |
| restart 후 상태 | 새 AppId, pipe·구독 재생성 필요, 그 사이 message 유실, table은 처음부터. 재시작 지연은 앱의 RunLoop 협조 여부와 ES scan 주기에 의해 결정(관측 ~1.6 s, 비협조 시 kill timeout 경로) |

분석기 API 의미표(MLIR op 설계 입력)에 넣어야 할 최소 항목 [해석]:
- `es.wait_system_state(min, timeout) -> status`: 효과 = 호출자 AppState 상향(즉시) + `SystemState >= min` 관측 또는 timeout. HB 근거는 "ES main의 대입"뿐이며, 그 대입은 다른 app의 준비와 조건부로만 연결됨.
- `es.run_loop() -> bool`: 효과 = AppState를 RUNNING으로(최초 1회), 외부 요청 관측.
- `es.restart/reload/delete(app)`: 효과 = 요청 기록만. 실제 teardown은 비동기(ES background) — teardown 순서: EVS→SB(구독 해제·queue 폐기)→TBL(descriptor 해제)→TIME→task 삭제.
- `tbl.get_address(h) -> (status, ptr)`: NEVER_LOADED/UNREGISTERED이면 ptr=NULL·lock 없음, 성공/INFO_UPDATED이면 lock(h)=active buffer(이전 lock 대체).
- `tbl.release(h)`: lock(h)=false(포인터 구분 없음).
- `tbl.load(h, src)`(owner): 즉시 검증·활성화, single은 lock 시 INFO_TABLE_LOCKED로 pending, double은 다음 buffer가 잠겨 있으면 NO_BUFFER_AVAIL.
- `tbl.register(name)`: 같은 이름의 un-owned record가 남아 있으면 DUPLICATE_NOT_OWNED.

## 12. 닫지 못한 것

1. #73 당시 revision의 core startup 코드와 현재 직렬화 구조의 대응(어느 commit에서 도입되었는지) — 미조사.
2. `CFE_TBL_Load`의 잠금 해제 구간 TOCTOU 후보 — 동시성 stress 실행 미수행. 단일 코어 RT 환경에서 실제 발생 가능성 미평가.
3. 같은 app의 main/child task가 TBL handle을 공유할 때의 보호 상실, child task의 RunLoop/WaitForSystemState가 부모 AppState를 바꾸는 효과 — 코드 근거만.
4. 비협조 앱(RunLoop을 부르지 않음)의 kill timeout 경로 지연 측정, ES background scan의 실제 주기 계측 — 미실행.
5. Notification message만으로 table을 관리하는 앱에서 지연된 update가 방치되는지 — 미실행.
6. 비행 RTOS(VxWorks/RTEMS) OSAL의 priority·선점 의미 — 이번 범위 밖. Linux native 결과를 일반화하지 않음.
7. 실행은 `msg_max=10`에 따라 pipe 깊이가 잘린 상태, uid 65534, `/dev/shm/osal:RAM`(다른 프로세스가 만든 root 소유 디렉터리) 접근 불가로 volatile disk script 시도가 실패한 상태다. 이 조건이 관측 결과(순서·TBL 의미)에 영향을 줬다는 증거는 없지만 배제하지도 않았다.
8. 공개 앱(CS, HK, LC 등)이 owner 재시작·NEVER_LOADED·중첩 GetAddress를 실제로 어떻게 처리하는지 — 미조사.
