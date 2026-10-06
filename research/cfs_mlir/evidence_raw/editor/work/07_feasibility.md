## 7. 실현 가능성 실측

이 절은 이 컨테이너에서 직접 실행한 결과를 보고한다. 질문은 하나다. 원노트 §10의 초기 경로(C → LLVM IR → MLIR LLVM dialect → cFS API 인식)가 고정한 실제 cFS 코드에서 작동하는가?

측정한 것은 다섯 가지다. 실제 bundle의 build, 실행 log, IR 생성과 MLIR import, IR에 남는 정보(API 호출, MID, 상태 저장, 소스 위치), ClangIR 사용 가능 여부다. 분석기를 구현하지 않았다. 따라서 이 절에는 검출 결과, 검출률, 오경고율이 없다.

주 probe의 산출물은 `$N/probe`에 있다. 154개 파일 import 실험의 산출물은 `$N/rw-mlir`에 있다. 검증 재실행의 산출물은 `$N/verify-C17`, `$N/verify_C17_counter`, `$N/verify_C17_overreach`, `$N/verify_C18_counter`, `$N/verify-C19`, `$N/verify-C20`에 있다.

### 7.0 결과 요약

| 질문 | 결과 | 절 |
| --- | --- | --- |
| 고정한 bundle이 build되는가? | **[실측]** 된다. gcc 13.3.0, install 단계 48.8 s, 컴파일 경고 0개. | §7.3 |
| 실행되는가? | **[실측]** non-root와 private IPC namespace의 root에서 `OPERATIONAL`까지 간다. host IPC의 root에서는 abort한다. | §7.4.1 |
| 시작 순서가 고정되는가? | **[실측]** script의 load 순서는 고정이다. 앱 초기화 완료 순서는 짧은 실행 6회에서 6가지로 달랐다. | §7.4.2 |
| startup 중 SB event가 있는가? | **[실측]** 매 실행 `No subscribers for MsgId 0x808` 4줄, SBN pipe에서 `0x80e` 손실 16줄이 나온다. 둘 다 EVS filter 상한과 같은 수다. | §7.4.3 |
| C → LLVM IR → MLIR import가 되는가? | **[실측]** 된다. 앱 5개 17 TU와 cFE 5개 module + 앱 디렉터리 17개 154개 파일이 진단 0개로 import된다. 단 flag 조건이 있다. | §7.5 |
| MID가 IR에서 상수인가? | **[실측]** `-O0`에서는 call site의 상수가 아니다. `-O1 -Xclang -disable-llvm-passes` + MLIR pipeline 뒤에는 코드에서 정한 MID가 상수가 된다. table·명령·실행 상태에서 오는 MID는 상수가 아니다. | §7.7 |
| 상태 저장을 인식하는가? | **[실측]** 앱 전역 struct의 field 직접 store는 인식된다. offset 0 field, API out-parameter, 지역 포인터를 통한 쓰기는 놓친다. | §7.8 |
| 소스 위치가 남는가? | **[실측]** `llvm.call`과 `llvm.load`는 모두 file:line:col을 가진다. 상수와 `addressof`는 위치가 없다. | §7.9 |
| ClangIR를 쓸 수 있는가? | **[실측]** 로컬 toolchain으로는 쓸 수 없다. `-fclangir`는 일부 모드에서 조용히 무시된다. | §7.10 |

### 7.1 측정 환경

| 항목 | 값 | 근거 |
| --- | --- | --- |
| OS·자원 | **[실측]** Linux 6.18.44, 4 vCPU | `uname`, `nproc` |
| 권한 | **[실측]** root이지만 `CapEff 000001fffeffffff`로 `CAP_SYS_RESOURCE`가 없다 | `grep CapEff /proc/self/status` |
| POSIX mqueue | **[실측]** `/proc/sys/fs/mqueue/msg_max = 10` | `cat` 결과. host 값은 바꾸지 않았다. |
| build 도구 | **[실측]** gcc 13.3.0, cmake 3.28.3 | `$N/probe/prep.log`의 `The C compiler identification is GNU 13.3.0` |
| 분석 도구 | **[실측]** clang·MLIR 23.0.0git, llvm-project `1053047a4be7d1fece3adaf5e7597f838058c947`, Release+assertions, 읽기 전용 | `clang --version` |
| CIR build 설정 | **[실측]** `#define CLANG_ENABLE_CIR 0` | `/home/user/work/llvm-project/build/tools/clang/include/clang/Config/config.h:86` |
| 디스크 | **[실측]** build tree 502 MB. probe 디렉터리는 cpu2 산출물(약 249 MB)을 지운 뒤 375 MB다. 최대 사용량은 616 MB였다. | `du -sh $N/probe` → `375M` |

**[해석]** 이 환경은 x86-64 Linux 시뮬레이션이다. 비행용 RTOS build의 증거가 아니다. 아래의 횟수는 관측값이며 발생률이 아니다 (§3.0과 같은 원칙).

### 7.2 고정한 소스

| 구성 요소 | commit | 근거 |
| --- | --- | --- |
| cFS bundle | `5a9b075cd4c818ee8555f349a9f78ba5632d0384` (2026-09-30, "Merge pull request #1144 from nasa/ic-latest-dev") | **[실측]** `git -C $N/probe/cFS log -1`. **[확인된 사실]** 조사일의 nasa/cFS `dev` HEAD다 (검증 C17의 `git ls-remote`). |
| cFE | `546a002515be5a1e3b66f9ae2c14f948d9cec76f` | **[확인된 사실]** 수정본 S25와 같은 commit이다 (`$N/probe/submodules.txt`). **[실측]** 실행 log가 `CFE_ES v7.0.1+dev1 (Draco) DEV BUILD, based on v7.0.1, EDS disabled`를 출력한다 (`$N/probe/run_nobody.log` L43). |
| OSAL | `dad0ee99c121f2ecc11f8c0798c1e836e41d8ab1` | **[확인된 사실]** `$N/probe/submodules.txt` |
| PSP | `53df1d54c3d29fd23832c59df1104de07a043af2` | 같음 |
| sample_app | `199476a34827ae84d50d66f97619227854cd971a` | 같음. 수정본 S21이 기록한 `dev`의 blob `b07a4317`과 다르다. 이 절의 줄 번호는 `199476a3` 기준이다. |
| hk, to_lab, sch_lab, ci_lab | `0dc16b7a`, `d27c6014`, `607e2f90`, `edb8ddf5` | 같음 |
| sbn, cf, cs, ds | `bf058c45`, `a9c36d63`, `795e387f`, `06e420c4` | 같음. 나머지 앱·tool의 SHA도 같은 파일에 있다. |

**[실측]** 명령은 `git clone --depth 1 --recurse-submodules --shallow-submodules https://github.com/nasa/cFS`다. real 20.652 s, 크기 99 MB였다 (`$N/probe/clone.log`).

**[확인된 사실]** 이 절의 probe는 bundle이 고정한 OSAL·PSP를 썼다. §3의 ES·TBL probe는 날짜로 고른 OSAL `5befd8e9`·PSP `36c24cb9`를 썼다 (§3.0). **[해석]** prototype 평가 전에는 하나의 고정값으로 통일해야 한다.

### 7.3 build와 compile_commands.json

| 항목 | 결과 | 근거 |
| --- | --- | --- |
| 절차 | **[확인된 사실]** bundle README의 절차는 `make native_std.prep`, `make native_std.install`이다. `Makefile.sample`을 복사하는 예전 절차가 아니다. | README L110-L111 (cFS README @5a9b075 [S87]) |
| 설정 | **[확인된 사실]** `native_std`는 `ENABLE_UNIT_TESTS=TRUE`, `SIMULATION=native`, `CFE_EDS_ENABLED=OFF`, `MISSIONCONFIG=sample`, `CMAKE_BUILD_TYPE=debug`다. | `$N/probe/cFS/target-configs.mk` L52-L56 |
| prep | **[실측]** real 5.584 s | `$N/probe/prep.log` L712 |
| install | **[실측]** real 48.791 s, user 2m11.713s, `exit=0`, `native_std.install successfully completed` | `$N/probe/install.log` 끝부분 |
| 병렬도 | **[미확인]** log에 `-j` 값이 기록되지 않았다. README에도 `-j4`가 없다. **[해석]** user/real 비가 약 2.7이므로 병렬 make였을 것이다. | 검증 C17 |
| 경고 | **[실측]** install log의 컴파일 경고 0개 (`grep -ci warning` = 0). prep log에는 SBN의 CMake Deprecation Warning 6줄이 있다. | `install.log`, `prep.log` |
| 크기 | **[실측]** build tree 502 MB. unit-test runner와 cpu2를 포함한다. | probe 기록 |
| compile DB 생성 | **[확인된 사실]** `target-rules.mk:33`이 `-DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE`를 넘긴다. 따로 설정할 필요가 없다. | `$N/probe/cFS/target-rules.mk` L33 |
| compile DB 내용 | **[실측]** cpu1 DB는 1435개 항목이다. 앱 `fsw/src`의 unit-test가 아닌 항목은 98개다. 각 앱 소스는 두 번 나온다. 한 번은 앱 module build이고, 한 번은 `-D_UNIT_TEST_ -pg -ftest-coverage` coverage build다. | `$N/probe/compile_commands.cpu1.json` (build tree의 DB와 byte 동일) |

**[해석]** compile DB [S19]는 공짜로 얻는다. 그러나 이 DB는 gcc용 flag를 담는다. clang에 그대로 넣으면 실패한다 (§7.5.1). coverage 중복 항목도 걸러야 한다.

### 7.4 실행 관측

#### 7.4.1 실행 구성

| 구성 | 결과 | 원인 **[확인된 사실]** |
| --- | --- | --- |
| host IPC, root (`timeout -s INT 20 ./core-cpu1`) | **[실측]** 약 11 s 뒤 exit 134. `OS_QueueCreate Error. errno = 22`, `CFE_ES_TaskInit: Cannot Create SB Pipe, RC = 0xCA000005`, `CFE_EVS_TaskMain: Application Init Failed`, PROCESSOR RESET, `Aborted` (`$N/probe/run_root.log` L41, L49, L60) | OSAL은 non-root일 때만 queue depth를 `msg_max`로 줄인다 ([bsp_start.c L66-L75](https://github.com/nasa/osal/blob/dad0ee99c121f2ecc11f8c0798c1e836e41d8ab1/src/bsp/generic-linux/src/bsp_start.c#L66-L75), [os-impl-queues.c L59-L68, L102-L104](https://github.com/nasa/osal/blob/dad0ee99c121f2ecc11f8c0798c1e836e41d8ab1/src/os/posix/src/os-impl-queues.c#L59-L68)). root라도 `CAP_SYS_RESOURCE`가 없으면 `msg_max`=10을 넘는 queue를 만들 수 없다. |
| uid 65534 (`setpriv`) | **[실측]** `OPERATIONAL` 도달. `Maximum user msg queue depth = 10`, `Could not setschedparam in main thread: Operation not permitted` (`run_nobody.log` L1, L9) | queue depth가 10으로 잘린다. native sample 설정이 `OSAL_CONFIG_DEBUG_PERMISSIVE_MODE TRUE`를 켜므로 RT priority 실패가 오류가 되지 않는다 (`$N/probe/cFS/cfe/cmake/sample_defs/native_osconfig.cmake` L39). |
| root + `unshare --ipc`, namespace 안의 `msg_max=256` | **[실측]** `OPERATIONAL` 도달. `Selected policy 2 for RT tasks` (SCHED_RR) (`run_root_ipcns.log` L8) | host의 `msg_max`는 10 그대로다. |

**[실측]** `OPERATIONAL`에 도달한 실행은 8회다. 20 s 실행 2회(`run_nobody.log`, `run_root_ipcns.log`)와 6 s 실행 6회(`$N/probe/runs/{nobody,rootipc}_{1,2,3}.log`)다. 남은 SysV shared memory는 `ipcrm`으로 지웠다.

**[해석]** root 권한은 필요 없다. 오히려 이 컨테이너의 host IPC root에서는 실패한다. 두 성공 구성은 queue depth와 scheduling이 다르다. 그래서 같은 bundle에서도 관측되는 SB 손실 경로가 달라진다 (§7.4.3).

#### 7.4.2 앱 시작 순서

**[실측]** 실행 log의 `Loading file` 줄로 본 startup script의 load 순서는 고정이다. SCH_LAB, CI_LAB, TO_LAB, SAMPLE_APP, LC, CF, DS, FM, HK, HS, MM, SC, MD, CS, SBN 순이다 (`run_root_ipcns.log`). **[확인된 사실]** 이 script(`build-native_std/exe/cpu1/cf/cfe_es_startup.scr`)는 `sample_defs/generate_startup.cmake`가 만든다.

**[실측]** core service는 매 실행 ES → EVS → SB → TBL → TIME 순서로 초기화 event를 냈다. 각각 약 100 ms 간격이다. 이어서 `CORE_READY`, `APPS_INIT`, `OPERATIONAL`이 나온다 (`run_nobody.log` L43-L67, L69, L136-L137). 8회 실행 모두 `Startup Sync failed` 줄이 없다 (`grep -c` = 0).

**[실측]** 앱 초기화 완료 event의 순서는 짧은 실행 6회에서 6가지였다. 아래 표는 log에서 각 앱의 `Initialized`, `Initialization complete`, `initialized` event가 처음 나온 순서다. HS와 SBN은 그런 event가 없어 제외했다. SCH_LAB은 EVS가 아닌 콘솔 출력으로 셌다.

| 실행 | 초기화 완료 순서 |
| --- | --- |
| nobody_1 | SCH_LAB, TO_LAB, SAMPLE_APP, LC, CF, FM, **CI_LAB**, MM, HK, DS, MD, SC, CS |
| nobody_2 | SCH_LAB, TO_LAB, SAMPLE_APP, LC, CF, DS, FM, HK, MM, **CI_LAB**, SC, CS, MD |
| nobody_3 | SCH_LAB, TO_LAB, SAMPLE_APP, LC, CF, DS, FM, HK, **CI_LAB**, MM, SC, CS, MD |
| rootipc_1 | SCH_LAB, TO_LAB, SAMPLE_APP, LC, **CI_LAB**, MM, FM, CF, HK, MD, SC, DS, CS |
| rootipc_2 | SCH_LAB, TO_LAB, SAMPLE_APP, LC, CF, **CI_LAB**, FM, MM, HK, DS, MD, SC, CS |
| rootipc_3 | SCH_LAB, TO_LAB, SAMPLE_APP, LC, CF, DS, FM, **CI_LAB**, MM, HK, MD, SC, CS |

**[실측]** CI_LAB은 script에서 2번째로 load되지만 초기화 완료는 5번째부터 10번째 사이였다.

**[해석]** 현재 공개 bundle에서도 load 순서는 준비 완료 순서가 아니다. 수정본 §2.2의 #73 교훈을 오늘의 코드에서 다시 관측한 것이다. 이것은 결함의 증거가 아니다. 분석기는 script 순서를 happens-before로 쓰면 안 된다. event가 찍힌 시점은 앱의 실제 준비 시점과 다를 수 있다. 실행 8회, 컨테이너 1개라서 순서의 분포는 말할 수 없다.

#### 7.4.3 startup 중 SB event

| 관측 | 실행당 줄 수 | 구성 | 소스상의 원인 **[확인된 사실]** | 판단 |
| --- | --- | --- | --- | --- |
| `CFE_SB 14: No subscribers for MsgId 0x808,sender X`. X는 `CFE_SB`, `CFE_TBL`, `CFE_TIME`, `TO_LAB` | **[실측]** 4 (8/8 실행) | 모든 구성 | `0x808`은 `CFE_EVS_LONG_EVENT_MSG_MID`다 (TLM base 0x0800 \| EVS topic 8). TO_LAB은 table에 있는 이 MID(`sample_defs/tables/to_lab_sub.c` L73, BufLimit 16)를 `OPERATIONAL` 뒤에 구독한다. | **[해석]** 의도된 첫 메시지 손실. 결함 후보가 아니다. |
| `CFE_SB 25: Pipe Overflow,MsgId 0x80e,pipe SBNSubPipe,sender TO_LAB` | **[실측]** 16 | uid 65534 | SBNSubPipe의 요청 depth 32가 10으로 잘린다. | **[해석]** 후보. 기능 영향 미확인 |
| `CFE_SB 17: Msg Limit Err,MsgId 0x80e,pipe SBNSubPipe,sender TO_LAB` | **[실측]** 16, overflow 0 | root + ipcns | SBN이 `0x80e`를 MsgLim 16으로 `SubscribeLocal`한다. | **[해석]** 후보. 기능 영향 미확인 |

명령은 `grep -ac 'No subscribers\|Pipe Overflow\|Msg Limit' <log>`다. 결과는 위 표와 같다.

**[확인된 사실] event를 낸 주체.** 네 줄은 모두 SB가 낸 event ID 14(`CFE_SB_SEND_NO_SUBS_EID`)다. `CFE_TBL` 같은 이름은 보낸 쪽(sender) 이름이다. 그중 세 줄(`CFE_SB`, `CFE_TBL`, `CFE_TIME`)은 `CORE_READY` 전에 나온다 (`run_nobody.log` L63, L65, L67 vs L69). 즉 앱이 load되기 전이다. 이 세 줄은 TO_LAB의 구독 지연과 무관하다. 네 번째 줄(sender TO_LAB, L93)은 `OPERATIONAL`(L137) 전에 나온다.

**[확인된 사실] 횟수 4와 16은 EVS filter 상한이다.** SB는 `CFE_SB_SEND_NO_SUBS_EID`를 `CFE_EVS_FIRST_4_STOP`으로, `MSGID_LIM_ERR`·`Q_FULL_ERR`을 `CFE_EVS_FIRST_16_STOP`으로 등록한다 ([cfe_sb_internal_cfg.h L221-L243](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/inc/cfe_sb_internal_cfg.h#L221-L243), S25). 이 filter는 SB 앱의 event ID 단위다. 모든 MID와 sender가 같은 상한을 나눠 쓴다. **[미확인]** 따라서 실제 손실 수는 알 수 없다. SB housekeeping의 `NoSubscribersCounter`를 읽으면 알 수 있지만 수집하지 않았다.

**[확인된 사실] TO_LAB의 구독 지연.** TO_LAB은 table 기반 telemetry 구독만 `CFE_ES_WaitForStartupSync` 뒤로 미룬다. 소스 주석은 "Defer subscribing until the system is operational will avoid possibly seeing MsgLimit errors due to the apps sending many events at start up"이다 ([to_lab_app.c L69-L73](https://github.com/nasa/to_lab/blob/d27c6014cb5d2979140500f635876ee87b81995b/fsw/src/to_lab_app.c#L69-L73)). 자기 명령 MID는 그보다 먼저 `TO_LAB_init`에서 구독한다 (L208, L213). 이 지연은 commit `d3d52da`(2026-04-21, "Fix cFS/cFS#676, startup msg limit errors")가 넣었다. 이를 포함하는 tag는 `v7.0.1`뿐이다 (검증 C16). 대기 timeout 기본값은 10000 ms다 (`fsw/inc/to_lab_internal_cfg.h` L79-L80).

**[실측] SBN pipe 손실의 경로.** `OPERATIONAL` 뒤 TO_LAB은 `TO Lab subscribed to 37 messages from the table`을 낸다 (`run_nobody.log` L224). **[확인된 사실]** 구독마다 SB는 구독 보고(`CFE_SB_ONESUB_TLM_MID` = `0x80e`)를 보낸다. SBN은 이 MID를 depth 32 pipe에 MsgLim 16으로 `SubscribeLocal`한다 ([sbn_app.c L1463-L1489](https://github.com/nasa/SBN/blob/bf058c45bb378446a22e78627693c240ee56df1c/fsw/src/sbn_app.c#L1463-L1489); `sbn_platform_cfg.h` L67, L74). **[해석]** TO_LAB이 자기 pipe의 MsgLim 오류를 피하려고 구독을 미루자, 그 burst가 SBN의 pipe로 옮겨 간 셈이다. 어느 손실 경로가 나타나는지는 실행 구성이 정한다. **[미확인]** SBN이 이 손실에서 회복하는지(예: 전체 구독 보고 요청)는 확인하지 않았다. 그래서 결함으로 보고하지 않는다.

**[확인된 사실] 추가 구독자.** DS의 filter table과 HS(event 감시를 켤 때)도 자기 초기화 중에 `0x808`을 구독한다 (검증 C16). **[해석]** 그러므로 TO_LAB 구독 전의 실제 손실 수는 두 가지 이유로 알 수 없다. filter 상한과, 중간에 생기는 다른 구독자다.

**[해석] 분석기에 주는 의미.** 이 관측들은 원노트 F1(구독 전 발행)의 **정상 사례**다. 수정본 §4.4가 요구한 "첫 메시지 유실이 허용되는 앱"의 실제 예이기도 하다. 첫 메시지 규칙은 TO_LAB의 table 구독 지연을 경고하면 안 된다. 실행 중 counter와 event log는 상한이 있어 정답 자료로 쓸 수 없다.

### 7.5 LLVM IR 생성과 MLIR import

#### 7.5.1 앱 5개, 17 TU

**[실측] 첫 시도는 실패했다.** compile DB의 gcc 항목을 clang 23에 `-S -emit-llvm -O0 -g`를 붙여 그대로 넣었다. sample_app 4개 TU가 모두 실패했다. 오류는 `error: unknown warning option '-Wno-stringop-overflow' … [-Werror,-Wunknown-warning-option]`였다 (`$N/probe/ir/sample_app/commands_attempt1_fail.log`). gcc 전용 `-Wno-stringop-*` flag와 `-Werror`가 겹친 결과다.

**[실측]** `-Wno-unknown-warning-option`을 더하자 17개 TU가 모두 컴파일되었다. sample_app 4, hk 4, sch_lab 1, ci_lab 4, to_lab 4개다. TU당 0.03–0.06 s였다 (`$N/probe/ir/<app>/commands.log`, 스크립트 `$N/probe/emit_ir.py`).

**[실측]** `mlir-translate --import-llvm`은 17개 모두 rc 0, 진단 0줄이었다. TU당 8–25 ms였다 (`$N/probe/mlir/import.log`). 예를 들어 `sample_app.ll` 645줄은 MLIR 420줄이 되었다. `hk_utils.ll` 2890줄은 1905줄이 되었다.

**[실측]** `mlir-opt --inline --sroa --mem2reg --canonicalize --cse`도 17개 모두 오류 없이 끝났다 (`$N/probe/mlir_opt/`).

#### 7.5.2 154개 파일과 연결한 mission module

**[실측] 범위.** cFE 5개 module의 `fsw/src` 56개 파일과 앱 디렉터리 17개의 98개 파일이다.

| 구분 | 파일 수 **[실측]** |
| --- | --- |
| cFE | ES 18, EVS 6, SB 9, TBL 17, TIME 6 |
| 앱 | cf 14, ci_lab 4, cs 11, ds 5, fm 7, hk 4, hs 8, lc 7, md 6, mm 9, sample_app 4, sbn 4, sbn_f_remap 1, sbn_udp 1, sc 8, sch_lab 1, to_lab 4 |
| 제외 | cFE MSG 10, config 6, FS 3, SBR 3, ResourceId 2. OSAL, PSP, sample_lib 전부 |

**[확인된 사실]** 앱 디렉터리 17개는 앱 repository 15개와 SBN plugin 디렉터리 2개(`sbn_udp`, `sbn_f_remap`, symlink)다. 원래 보고의 "앱 20개", "cFE core 전체"는 틀렸다 (검증 C17 정정).

**[실측] 154/154를 얻은 조건.** 이 결과는 README의 gcc DB가 아니라 clang으로 configure한 별도 build tree(`build-ana`, `ENABLE_UNIT_TESTS=FALSE`)의 DB에서 나왔다. 스크립트는 `-Werror*`와 `-Wno-stringop*`를 지우고 `-Wno-everything -Xclang -disable-O0-optnone`을 더했다. `-O0 -g`로 컴파일했다. 결과는 clang 154/154, import 154/154, 진단이 있는 파일 0개다 (`$N/rw-mlir/import_exp/run_import.py`, `import_summary.txt`).

**[실측] 문자 그대로의 절차는 2개 파일에서 실패한다.** README의 gcc DB에 `-Wno-unknown-warning-option`만 더하고 `-Werror`를 두면 152/154다. 실패는 두 곳이다.

- `apps/cf/fsw/src/cf_codec.c:169`, `:239`: `unused function 'CF_Codec_Store_uint64'`/`'CF_Codec_Load_uint64' [-Werror,-Wunused-function]` ([cf_codec.c L169](https://github.com/nasa/CF/blob/a9c36d63522babae4ec6e14c57df7315c927c58f/fsw/src/cf_codec.c#L169))
- `apps/cs/fsw/src/cs_table_processing.c:932`: `equality comparison with extraneous parentheses [-Werror,-Wparentheses-equality]` ([cs_table_processing.c L932](https://github.com/nasa/CS/blob/795e387fce497c698c87a5a68f2a9014f13c3ce8/fsw/src/cs_table_processing.c#L932))

`-Wno-error`를 더하면 154/154가 되고 개수도 같아진다 (`$N/verify_C17_counter/per_file_results.json`, `$N/verify-C17/rerun_results.json`). clang으로 configure한 tree의 정식 build도 같은 두 파일에서 멈춘다 (`$N/rw-mlir/cfs_build_cpu1.log` L361, L364, L401). **[확인된 사실]** clang tree에서는 CMake의 `add_compile_options_if_available`이 stringop flag를 스스로 뺀다. 그래서 거기서는 `-Wno-unknown-warning-option`이 필요 없다 (`sample_defs/global_build_options.cmake` L40-L48, `cfe/cmake/global_functions.cmake` L422-L445).

**[실측] 연결 module.** 154개 bitcode를 `llvm-link`로 묶는 데 1.3 s가 걸렸다. `mlir-translate --import-llvm mission.bc`는 real 1.186 s, rc 0, 20,246,233 byte였다 (`$N/rw-mlir/import_exp/link_import2.txt`). 검증 재실행에서는 1.40–1.72 s였다.

| 항목 | 값 **[실측]** |
| --- | --- |
| 정의된 `llvm.func` | 2389 |
| 외부 선언 | 184. `OS_` 82, `CFE_PSP_` 39, 제외한 cFE module 44(`CFE_MSG_` 16, `CFE_FS_` 10, `CFE_SBR_` 9, `CFE_ResourceId_` 5, `CFE_Config_` 4), `SAMPLE_LIB_Function` 1, libc 18 |
| `llvm.call` | 8830 |
| 간접 호출 | 73. `sbn_app.c` 17, `sbn_udp_if.c` 10, `cfe_es_generic_pool.c` 9, 나머지 CF 등 |
| call graph (`mlir-opt -test-print-callgraph`) | 간선 6177개, 그중 `<Unknown-Callee-Node>`로 가는 것 562개 |

**[해석]** SB routing을 맡는 SBR과 메시지 header 접근자 MSG가 module 밖에 있다. SB 의미를 모델로 넣거나, 이 module들을 연결해야 한다. 간접 호출은 SBN·CF·ES pool에 몰려 있다. sample_app의 명령 dispatch는 switch이고 간접 호출이 아니다.

**[확인된 사실]** 공식 문서는 LLVM IR import를 "An experimental flow allows one to import a substantially limited subset of LLVM IR into MLIR"로 설명한다 ([TargetLLVMIR.md L947-L950 @ccac700c](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/docs/TargetLLVMIR.md#L947-L950), S16). **[해석]** 이 제한은 위 조건의 cFS 파일들을 막지 않았다. 수정본 §9.3이 가설로 둔 import 경로는 이 범위에서 성립한다. 비용은 import가 아니라 의미 복원에 있다.

#### 7.5.3 import 실험의 범위

**[미확인]** 다음은 시험하지 않았다. OSAL·PSP의 import, `-O2` 같은 최적화 build의 전체 import, RTEMS·VxWorks 교차 build, EDS build(`native_eds`), cpu2, 여러 node의 SBN이다.

### 7.6 cFS API 호출 수

**[실측]** 아래는 `-O0` MLIR의 정적 `llvm.call` site 수다. regex로 셌다 (`$N/probe/analyze_mlir.py`, 출력 `$N/probe/mlir/O0_<app>.json`). 실행 횟수가 아니다. "SB helper"는 `static inline`인 `CFE_SB_ValueToMsgId`, `CFE_SB_MsgId_Equal`, `CFE_SB_MsgIdToValue`다. `-O0`에서는 module마다 내부 함수로 정의되어 호출로 남는다.

| 앱 | SB API | SB helper | ES | TBL | EVS | MSG | `OS_` | 정의 함수(helper 포함) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| sample_app | 7 | 9 | 12 | 6 | 13 | 6 | 0 | 16 |
| hk | 19 | 31 | 12 | 14 | 38 | 13 | 0 | 33 |
| sch_lab | 4 | 1 | 16 | 5 | 0 | 2 | 9 | 4 |
| ci_lab | 10 | 13 | 9 | 0 | 17 | 8 | 8 | 18 |
| to_lab | 18 | 23 | 10 | 6 | 32 | 5 | 8 | 30 |

**[실측] 발행 MID의 연결.** sample_app의 `CFE_MSG_Init`와 `CFE_SB_TransmitMsg`는 둘 다 같은 전역 field 경로 `SAMPLE_APP_Data.HkTlm`을 인자로 받는다 (`sample_app.c:134`, `sample_app_cmds.c:61`; `O0_sample_app.json`의 `msgptr_args`). **[확인된 사실]** `TransmitMsg`는 MID를 인자로 받지 않는다. MID는 앞서 `CFE_MSG_Init`가 buffer에 넣는다 (§4.2.2).

**[해석]** 앱당 API site는 수십 개다. API 의미표는 감당할 수 있는 크기다. HK와 TO_LAB에서는 EVS 호출이 가장 많다.

### 7.7 MID는 IR에서 상수인가

#### 7.7.1 `-O0`

**[실측]** 코드에서 정한 MID는 call site의 상수가 아니다. 상수는 `CFE_SB_ValueToMsgId`의 인자로 들어가고, 그 결과가 `CFE_SB_MsgId_t` 임시 alloca에 저장된 뒤 다시 load된다.

```llvm
; $N/probe/ir/sample_app/sample_app.ll L143-L149 (발췌, 실측 출력)
%call16 = call i32 @CFE_SB_ValueToMsgId(i32 noundef 6275), !dbg !292
%coerce.dive17 = getelementptr inbounds nuw %struct.CFE_SB_MsgId_t, ptr %agg.tmp15, i32 0, i32 0, !dbg !292
store i32 %call16, ptr %coerce.dive17, align 4, !dbg !292
%6 = load i32, ptr getelementptr inbounds nuw (%struct.SAMPLE_APP_Data_t, ptr @SAMPLE_APP_Data, i32 0, i32 4), align 4, !dbg !294
%coerce.dive18 = getelementptr inbounds nuw %struct.CFE_SB_MsgId_t, ptr %agg.tmp15, i32 0, i32 0, !dbg !295
%7 = load i32, ptr %coerce.dive18, align 4, !dbg !295
%call19 = call i32 @CFE_SB_Subscribe(i32 %7, i32 noundef %6), !dbg !295
```

**[실측]** alloca를 한 단계 거슬러 올라가면 값을 되찾는다. sample_app의 값은 `6275`(`0x1883`, SEND_HK, `sample_app.c:158`), `6274`(`0x1882`, CMD, `:173`), `2179`(`0x0883`, HK_TLM, `CFE_MSG_Init`, `:134`)다. **[확인된 사실]** 이 값은 `SAMPLE_APP_*_PLATFORM_MIDVAL` → `CFE_PLATFORM_*_TOPICID_TO_MIDV` → base | topic 계산에서 나온다. base는 `sample_defs` cpu1 설정이 정한다. 따라서 숫자는 이 설정에 묶인다 ([default_sample_app_msgids.h L29-L31](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/config/default_sample_app_msgids.h#L29-L31), 검증 C18).

**[실측]** `-O0` IR에 같은 `mlir-opt` pipeline을 돌려도 `CFE_SB_ValueToMsgId` 호출은 남는다. `-O0` 함수가 `noinline optnone`이기 때문이다 (`sample_app.ll` L246-L247의 `CFE_SB_ValueToMsgId` 정의와 L274의 attribute group; `$N/verify_C18_counter/o0.opt.mlir` L265, L280, L295).

#### 7.7.2 `-O1 -Xclang -disable-llvm-passes` + MLIR pipeline

**[실측]** clang을 `-O1 -Xclang -disable-llvm-passes -g`로 돌리면 `optnone`·`noinline`이 붙지 않는다 (`$N/probe/emit_ir_o1np.py`). 그 IR을 import한 뒤 `mlir-opt --inline --sroa --mem2reg --canonicalize --cse`를 돌리면 코드에서 정한 MID가 call site의 `llvm.mlir.constant` operand가 된다.

```mlir
// $N/probe/mlir_opt/sample_app/sample_app.dbg.mlir (발췌, 실측 출력)
%16 = llvm.mlir.constant(6274 : i32) : i32 loc(#loc172)
%19 = llvm.mlir.constant(6275 : i32) : i32 loc(#loc172)
%26 = llvm.mlir.constant(2179 : i32) : i32 loc(#loc172)
%58 = llvm.call @CFE_SB_Subscribe(%19, %57) : (i32, i32 {llvm.noundef}) -> i32 loc(#loc191)   // L268
%65 = llvm.call @CFE_SB_Subscribe(%16, %64) : (i32, i32 {llvm.noundef}) -> i32 loc(#loc200)   // L283
```

**[실측]** 상수는 함수 입구로 올라가 자기 소스 위치를 잃는다 (`loc(#loc172)`). site의 위치는 그것을 쓰는 `llvm.call`의 위치로 정해야 한다. **[실측]** inlining은 call site를 복제한다. 그래서 pipeline 뒤의 raw call 수는 `-O0`의 수와 비교할 수 없다. 아래 수는 debug 위치로 중복을 제거한 소스 site 단위다.

#### 7.7.3 앱별 회수 결과

대상 API는 `CFE_SB_Subscribe`, `SubscribeEx`, `Unsubscribe`, `CFE_MSG_Init`의 MID 인자다.

| 앱 | 코드 정의 MID가 상수로 회수된 site / 전체 | 나머지의 출처 |
| --- | --- | --- |
| sample_app | **[실측]** 3/3 | — |
| hk | **[실측]** 4/7 | **[확인된 사실]** HK copy table (`hk_utils.c` 305, 322, 436) |
| sch_lab | **[실측]** 1/2 | **[확인된 사실]** schedule table (`sch_lab_app.c` 246의 `ConfigEntry->MessageID`) |
| ci_lab | **[실측]** 4/4 | — |
| to_lab | **[실측]** 3/7 | **[확인된 사실]** 구독 table (`to_lab_app.c` 504), 실행 상태 `ActiveSubs` (454), 명령 payload (`to_lab_cmds.c` 218, 280) |

**[확인된 사실] to_lab 정정.** probe의 원래 보고는 to_lab을 2/7로 셌다. 검증에서 3/7로 정정했다 (검증 C18). 차이는 helper `TO_LAB_CmdSubscribe`의 `CFE_SB_Subscribe(CFE_SB_ValueToMsgId(MsgIdValue), …)`([to_lab_app.c L261](https://github.com/nasa/to_lab/blob/d27c6014cb5d2979140500f635876ee87b81995b/fsw/src/to_lab_app.c#L261))다. out-of-line 사본에서는 `%arg0`이다. inline된 사본(`TO_LAB_init`, `TO_LAB_AppMain`)에서는 `6272`(`0x1880`)과 `6273`(`0x1881`) 상수다. 호출자는 L208과 L213이다. 이 site는 table이나 명령 payload가 아니다.

**[확인된 사실] 세지 않은 것.**

- 수신 쪽 dispatch의 MID 비교. sample_app·hk·ci_lab은 `CFE_MSG_GetMsgId`의 out-parameter 값을 함수 안 `static` cache(`SAMPLE_APP_TaskPipe.CMD_MID` 등)와 비교한다. cache는 첫 호출 때 채워진다 ([sample_app_dispatch.c L134-L158](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app_dispatch.c#L134-L158)). **[실측]** pipeline 뒤에도 비교 대상은 이 전역의 load다.
- CI_LAB이 uplink buffer를 그대로 내보내는 `CFE_SB_TransmitBuffer`(`ci_lab_app.c:267`). MID는 uplink 데이터에서 온다.

**[확인된 사실] macro 이름.** IR과 MLIR에는 숫자만 남는다. `SAMPLE_APP_CMD_MID` 같은 이름은 없다. `-gdwarf-5 -fdebug-macro`로 컴파일하면 `.ll`에 `!DIMacro(name: "SAMPLE_APP_CMD_MID", …)`가 생긴다. 그러나 `mlir-translate --import-llvm`은 경고 없이 이를 버린다 (`$N/verify_C18_counter/sample_app.macro.ll` L2883-L2884 vs `sample_app.macro.mlir`; 검증 C18). **[실측]** Clang AST는 MID를 만드는 literal의 spelling 위치를 가진다. `6144`는 `global_core_api_base_msgid_values.h:30:34`, `131`은 `sample_app_topicids.h:31:52`다. AST는 `MemberExpr .CommandPipe` 같은 field 이름도 가진다 (`$N/probe/cir/ast_SAMPLE_APP_Init.txt` L124-L136, L169-L181).

**[확인된 사실] EDS build.** EDS 설정에서는 `CFE_PLATFORM_{CMD,TLM}_TOPICID_TO_MIDV`가 `CFE_SB_Local{Cmd,Tlm}TopicIdToMsgId(topic)` 호출이 된다 ([eds_cfe_core_api_msgid_mapping.h L37-L66](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/config/eds_cfe_core_api_msgid_mapping.h#L37-L66)). 구현은 `CFE_PSP_GetProcessorId()`를 쓴다 ([cfe_sb_eds_msg_id_util.c L231-L235](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_eds_msg_id_util.c#L231-L235)). **[해석]** 이 mapping을 쓰는 sample_app·hk·ci_lab·to_lab의 MID는 EDS build에서 IR 상수가 되지 않을 것이다. **[미확인]** EDS build는 컴파일하지 않았다.

**[해석]** 원노트 §56 MVP의 네 함수(`CreatePipe`, `Subscribe`, `ReceiveBuffer`, `TransmitMsg`)만으로는 메시지 graph가 닫히지 않는다. HK와 TO_LAB의 실제 구독 대부분이 table과 명령에서 오기 때문이다. 발행 MID는 `CFE_MSG_Init`가 정한다. 수신 분기는 `CFE_MSG_GetMsgId`와 cache 비교가 정한다.

### 7.8 상태 저장 패턴

**[실측]** clang 23은 `-O0`에서 전역 struct field 접근을 struct 타입이 붙은 상수 GEP로 남긴다. import 뒤에는 `llvm.mlir.addressof` 결과에 대한 `llvm.getelementptr inbounds|nuw %1[%5, 3]` 형태가 된다 (`$N/probe/mlir/sample_app/sample_app.mlir` L181-L183, L241). GEP의 결과 타입에 `!llvm.struct<"struct.SAMPLE_APP_Data_t", …>`가 붙는다. field 이름은 GEP index를 DI의 `DW_TAG_member` 순서와 맞춰서 얻는다.

| 앱 | 전역 field store site | 이름 결정 | offset 0 모호 | 전역 load site |
| --- | --- | --- | --- | --- |
| sample_app | **[실측]** 11 | 7 | 4 | 17 |
| hk | **[실측]** 15 | 15 | 0 | 50 |
| sch_lab | **[실측]** 0 | 0 | 0 | 9 |
| ci_lab | **[실측]** 16 | 15 | 1 | 25 |
| to_lab | **[실측]** 41 | 40 | 0 | 41 |

store site는 모두 83개이고 모두 소스 위치를 가진다. 예: `HK_AppData.MissingDataCtr`, `SAMPLE_APP_Data.RunStatus` (`O0_hk.txt`, `O0_sample_app.json`).

**[실측] 놓친 패턴은 세 가지다.**

| 패턴 | 예 **[확인된 사실]** | IR에서 보이는 형태 |
| --- | --- | --- |
| offset 0 field | sample_app의 첫 field `CommandCounter`에 대한 쓰기 (`sample_app_cmds.c:81`, `:100`, `:122`, `:157`) | GEP 없이 base 주소로 접근. probe는 `<base:i8>`로 표시 |
| API out-parameter 쓰기 | `CFE_SB_CreatePipe(&SAMPLE_APP_Data.CommandPipe, …)`, `CFE_TBL_Register(&…TblHandles[0], …)`, `OS_CountSemCreate(&SCH_LAB_Global.TimingSem, …)` | store가 없다. 외부 함수에 주소만 넘어간다 |
| 지역 포인터를 통한 쓰기 | `LocalStateEntry->Counter = 0` ([sch_lab_app.c L130](https://github.com/nasa/sch_lab/blob/607e2f90c5f828f3f3ae655a4debacf8b2cc4bbb/fsw/src/sch_lab_app.c#L130)) | 전역과 연결되지 않는 store. sch_lab의 store가 0개인 이유다 |

**[확인된 사실]** field 이름은 DI metadata에만 있다. 예: `#llvm.di_derived_type<tag = DW_TAG_member, name = "att" …>` (`$N/rw-mlir/cir_test/t1.llvm.dbg.mlir` L14-L40). member DI는 그 struct를 정의에 쓰는 TU에만 나온다. 그래서 앱 단위 연결 뒤에 이름을 붙여야 한다 (§5.7.4).

**[해석]** 직접 전역 field 갱신은 IR에서 충분히 보인다. 원노트 §44의 상태 출처 추적은 이 패턴에서 출발할 수 있다. 그러나 실제 구현은 세 가지를 더 해야 한다. out-parameter API 쓰기를 상태 정의로 모델링하고, offset 0 field를 타입 정보로 풀고, 지역 포인터에 대한 points-to를 갖춰야 한다. 그렇지 않으면 sch_lab 같은 앱에서는 상태 갱신이 하나도 보이지 않는다.

**[미확인]** `analyze_mlir.py`는 regex 기반의 함수 내 probe다. 제어 흐름, alias, out-parameter, 분기 조건을 모델링하지 않는다. 위 수는 검출된 수이며 recall이 아니다.

### 7.9 소스 위치 정보

**[실측] 앱 5개.** 기본 출력에는 위치가 거의 없다 (`sample_app.mlir`에 `loc(` 4개). `--mlir-print-debuginfo`를 주면 `llvm.call` 477/477, `llvm.load` 996/996, 상태 store 83/83, MID·메시지 site 79/79가 file:line을 가진다. 위치가 없는 것은 함수 입구의 매개변수 spill store 95개와 입구로 올라간 상수뿐이다. inline된 op는 `loc(callsite(… at …))`를 가진다 (`$N/probe/mlir_opt/sample_app/sample_app.dbg.mlir` L165).

**[실측] 154개 파일.** 출력된 op 136,155개 중 94,824개(69.6%)가 file:line:col을 가진다 (`$N/rw-mlir/import_exp/import_summary.txt`). op 종류별로 보면 다음과 같다 (`loc_by_op.txt`).

| op | 위치 있음 | 위치 없음 |
| --- | --- | --- |
| `llvm.call` | 8830 | 0 |
| `llvm.load` | 26378 | 0 |
| `llvm.cond_br` | 5016 | 0 |
| `llvm.store` | 11274 | 3445 |
| `llvm.getelementptr` | 12492 | 2723 |
| `llvm.alloca` | 1 | 8085 |
| `llvm.mlir.constant` | 0 | 17150 |
| `llvm.mlir.addressof` | 0 | 3789 |

**[확인된 사실]** 연결한 `mission.mlir`은 `--mlir-print-debuginfo` 없이 만들었다 (`link_import2.txt`의 명령). **[실측]** 검증에서 같은 bitcode를 이 옵션으로 다시 import했다. `llvm.call` 8830/8830, `llvm.load` 26378/26378이 위치를 가졌다. `llvm.store`는 14,719개 중 11,274개였다 (검증 C17, `$N/verify_C17_counter/locchk.py`).

**[해석]** 수정본 §10.5가 요구한 설명용 소스 위치는 LLVM IR 경로에서도 남는다. MID 상수 같은 operand는 자기 위치가 없다. 진단은 그것을 쓰는 op의 위치를 인용해야 한다.

### 7.10 ClangIR 시험

**[실측] 로컬 toolchain.**

| 명령 | 결과 |
| --- | --- |
| `clang -fclangir -emit-cir sample_app.c` (compile DB flag 포함) | exit 1. `error: clang IR support not available, rebuild clang with -DCLANG_ENABLE_CIR=ON` (`$N/probe/cir/emit_cir.log`) |
| `clang -cc1 -fclangir -emit-cir t1.c` | 같은 오류 (`$N/rw-mlir/cir_test/clangir_local_test.log`) |
| `clang -fclangir -S -emit-llvm sample_app.c` | exit 0, 진단 없음. 출력이 일반 codegen과 byte 단위로 같다 (`diff $N/probe/cir/sample_app.cir.ll $N/probe/ir/sample_app/sample_app.ll` → 0줄) |
| `clang -fclangir -c`, `-O2`, `-Weverything` | exit 0, CIR 관련 진단 없음 (검증 C19, `$N/verify-C19/`) |
| `/usr/bin/clang` 18.1.3 | `unknown argument: -fclangir` |
| `cir-opt` | build되지 않았다 |

**[확인된 사실] 조용히 무시되는 이유.** `ExecuteCompilerInvocation.cpp` L68-L102는 CIR 분기를 `#if CLANG_ENABLE_CIR`로 감싼다. CIR이 없는 build에서는 assembly·bitcode·LLVM IR·object 출력이 일반 codegen action으로 떨어진다. `EmitCIR`만 `err_fe_cir_not_built`를 낸다 (`/home/user/work/llvm-project/clang/lib/FrontendTool/ExecuteCompilerInvocation.cpp`).

**[해석]** `-fclangir`가 rc 0으로 끝났다는 사실은 CIR을 썼다는 증거가 아니다. 이후 실험 기록에서 이를 CIR 결과로 보고하면 안 된다.

**[확인된 사실] upstream 상태.**

- llvm-project main `ccac700c`(2026-10-01)의 문서는 "upstreaming … is still in progress, and ClangIR is not included in a default clang build"라고 쓴다 ([clang/docs/CIR/index.md L3-L7](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/clang/docs/CIR/index.md#L3-L7), S17). 더 새 main `9d9b912d`에서도 같다.
- `CLANG_ENABLE_CIR`는 `llvm/CMakeLists.txt`와 `clang/CMakeLists.txt` 어디에도 `option()`으로 선언되지 않는다. `if()`로만 쓰인다 ([llvm/CMakeLists.txt L222-L227](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/llvm/CMakeLists.txt#L222-L227)). 설정하지 않으면 `#cmakedefine01`에 의해 0이다.
- incubator `llvm/clangir`의 HEAD는 `63412d47`("[CIR] Incubator is now closed", 2026-02-20)이다.
- incubator의 `cir-lifetime-check` pass는 upstream `ccac700c`·`9d9b912d`에 없다. upstream main에는 offset 기반의 `CIRBasicAliasAnalysis`가 2026년 8–9월에 들어왔다. 서로 다른 전역은 여전히 MayAlias로 둔다 (코드의 TODO, 검증 C20).

**[확인된 사실]** 로컬 source tree에는 `clang/lib/CIR`, `clang/tools/cir-opt`가 있다. `LLVM_ENABLE_PROJECTS`에 `mlir`가 이미 들어 있다. **[미확인]** `-DCLANG_ENABLE_CIR=ON` 재build는 시도하지 않았다. 5 GB 디스크 제한 안에 들어가는지, 시간이 얼마나 드는지, cFS 154개 파일 중 몇 개를 CIR이 컴파일하는지 모른다.

### 7.11 import된 cFS 형태의 IR에서 upstream MLIR가 주는 것

**[실측]** 아래는 로컬 `mlir-opt`(`1053047a`)의 test pass를 cFS 형태의 작은 LLVM dialect 입력과 clang에서 import한 C 입력에 돌린 결과다 (`$N/rw-mlir/mlir_probe/`, `$N/verify-C20/`). 관련 코드는 main `ccac700c`에서도 같다 (검증 C20).

시험별 명령, 결과 파일, 함의는 §4.5.3의 표에 있다. 요지는 다섯 가지다. 서로 다른 전역 두 개는 MayAlias, 한 struct의 다른 field 두 개는 MustAlias다. 모든 `llvm.call`이 모든 위치에 ModRef이고, `llvm.call`은 `MemoryEffectOpInterface`를 구현하지 않는다. import된 `static` 함수에는 `sym_visibility`가 없어 호출자를 모두 안다고 보지 않는다. `static const` 함수 포인터 table을 통한 호출은 `<Unknown-Callee-Node>` 간선 하나뿐이다(`-O0`, `-O2` 모두). `-test-last-modified`는 원노트 §44 예제에서 Guidance의 읽기를 `<unknown>`으로 돌려준다. 같은 함수 안에서도 같은 전역을 두 번째 `addressof`로 읽으면 `<unknown>`이다.

**[확인된 사실]** `LocalAliasAnalysis`는 MLIR core(`mlir/lib`)의 유일한 alias 구현이다. monorepo에는 Flang의 `fir::AliasAnalysis`와 CIR의 `CIRBasicAliasAnalysis`가 따로 있다. 둘 다 LLVM dialect 입력에는 적용되지 않는다 (검증 C20 정정).

**[해석]** solver는 upstream이 준다. field를 구별하고 전역을 아는 memory model, closed-world 가정, 간접 호출 해석, task 사이 관계는 연구자가 만들어야 한다. 이 결과는 수정본 §9.2의 보정을 실측으로 뒷받침한다. 설계는 §5.4와 §6에 있다.

### 7.12 frontend 선택에 대한 의미

| 후보 | 실측 상태 | 얻는 것 | 잃는 것 | 판단 |
| --- | --- | --- | --- | --- |
| LLVM IR `-O0` → import | **[실측]** 154/154 (flag 조건 §7.5.2) | callee 이름, 모든 call·load의 위치 | MID가 상수 아님, macro 이름, offset 0 field, out-parameter 쓰기 | **[해석]** 대비 경로. alloca 1단계 추적으로 MID를 얻을 수 있다 |
| LLVM IR `-O1 -Xclang -disable-llvm-passes` → import → `--inline --sroa --mem2reg --canonicalize --cse` | **[실측]** 앱 5개 17 TU에서 작동 | 코드 정의 MID의 call site 상수 | 상수의 자기 위치, macro 이름. inlining이 site를 복제한다 | **[설계 제안]** MVP의 주 경로 (§5.8 P0) |
| ClangIR | **[실측]** 로컬 불가 | — | — | **[설계 제안]** CIR이 켜진 build를 고정하고 cFS 파일 coverage를 잰 뒤 ablation으로만 둔다 |
| Clang AST (LibTooling, S18) | **[실측]** `-ast-dump`가 `sample_app.c`에 작동 | literal의 spelling 위치, `MemberExpr` field 이름 | SSA·dataflow | **[설계 제안]** MID 이름과 field 이름의 sidecar |

**[해석]** 원노트 §10의 "초기 prototype은 LLVM IR 경로, 장기적으로 CIR"이라는 순서는 실측과 맞는다. 다만 이유가 달라졌다. LLVM 경로가 막히는 지점은 import가 아니었다. 막히는 지점은 의미 복원이다. MID 상수화, macro 이름, field 이름, out-parameter, table 입력이 그것이다. CIR로 바꿔도 table·명령 유래 MID와 out-parameter 문제는 그대로 남는다 (§5.10). 따라서 frontend 선택이 연구의 핵심 위험은 아니다.

### 7.13 첫 prototype에 대한 의미

#### 7.13.1 입력 고정

**[설계 제안]** prototype의 입력은 다음으로 고정한다.

```text
source        : nasa/cFS 5a9b075c + $N/probe/submodules.txt의 submodule SHA 전부
config        : native_std, cpu1, MISSIONCONFIG=sample, CFE_EDS_ENABLED=OFF
compile DB    : clang으로 configure한 tree의 compile_commands.json
                - coverage 항목(-ftest-coverage) 제외
                - -Werror 제거 또는 -Wno-error 추가 (cf_codec.c, cs_table_processing.c 때문)
toolchain     : clang·mlir-translate·mlir-opt @ 1053047a (읽기 전용)
IR 생성       : clang <DB flags> -O1 -Xclang -disable-llvm-passes -g -S -emit-llvm
import        : mlir-translate --import-llvm --mlir-print-debuginfo
cleanup       : mlir-opt --inline --sroa --mem2reg --canonicalize --cse
config 입력   : sample_defs/tables/*.c, 앱 table 소스, 생성된 cfe_es_startup.scr
```

**[설계 제안]** 실행으로 후보를 확인할 때는 결과마다 실행 조건을 함께 기록한다. 기록 항목은 uid, `msg_max`, scheduling policy, CPU 고정, OSAL permissive flag, OSAL·PSP SHA다. §7.4.3의 두 구성처럼 같은 bundle에서 손실 경로가 달라졌기 때문이다.

#### 7.13.2 MVP의 API 범위

**[설계 제안]** 원노트 §56의 네 함수를 다음으로 넓힌다. 근거는 §7.7의 실측이다.

| 묶음 | API | 이유 |
| --- | --- | --- |
| 구독 | `CFE_SB_Subscribe`, `SubscribeEx`, `SubscribeLocal`, `Unsubscribe` | TO_LAB·HK의 table·명령 구독, SBN의 `SubscribeLocal` |
| 발행 | `CFE_MSG_Init`, `CFE_SB_TransmitMsg`, `CFE_SB_TransmitBuffer` | 발행 MID는 `CFE_MSG_Init`가 정한다. CI_LAB은 `TransmitBuffer`를 쓴다 |
| 수신·분기 | `CFE_SB_CreatePipe`, `CFE_SB_ReceiveBuffer`, `CFE_MSG_GetMsgId`, `CFE_MSG_GetFcnCode` | dispatch는 `GetMsgId` out-parameter와 cache 비교다 |
| helper | `CFE_SB_ValueToMsgId`, `CFE_SB_MsgId_Equal`, `CFE_SB_MsgIdToValue` | `-O0`에서는 module 내부 함수 호출로 남는다. pipeline 뒤에는 inline되어 이름이 사라진다 (§5.7.2) |
| 시작 | `CFE_ES_WaitForStartupSync`, `CFE_ES_WaitForSystemState` | TO_LAB의 구독 지연을 정상 사례로 분류하려면 필요하다 |

#### 7.13.3 MVP의 출력 형식

**[설계 제안]** MVP는 판정을 내지 않는다. site 표만 낸다.

```text
SiteRecord {
  app   : string          // compile DB의 apps/<name>/ 또는 cfe/modules/<name>/
  api   : enum { Subscribe, SubscribeEx, SubscribeLocal, Unsubscribe,
                 MsgInit, TransmitMsg, TransmitBuffer, ReceiveBuffer, CreatePipe }
  loc   : file:line:col   // 그 site를 쓰는 llvm.call의 위치 (§7.9)
  mid   : const(u32)
        | table(src_file, field)       // 예: hk_cpy_tbl.c, InputMid
        | cmd_payload(handler)         // 예: TO_LAB_AddPacketCmd
        | state(global path)           // 예: TO_LAB_Global.ActiveSubs
        | param(func, index)           // 예: TO_LAB_CmdSubscribe, 0
        | runtime_fn(name)             // EDS의 CFE_SB_Local*TopicIdToMsgId
        | unknown
  pipe  : global field path | unknown // 예: SAMPLE_APP_Data.CommandPipe
}
```

`mid`의 분류는 §5.7.3의 `#cfs.mid_src`와 같은 뜻이다.

#### 7.13.4 MVP의 정답표

**[설계 제안]** MVP는 아래 23개 site를 정확히 재현해야 통과한다. site의 위치는 고정 commit의 소스에서 확인했다 (**[확인된 사실]**). 값은 probe와 검증 재실행에서 측정했다 (**[실측]**). 같은 앱 안의 여러 구독 값은 줄과 값의 짝을 확인하지 않았으므로 집합으로 적는다.

| 앱 | site | API | 기대 `mid` |
| --- | --- | --- | --- |
| sample_app | `sample_app.c:134` | MsgInit | const 2179 (`0x0883`) |
| | `sample_app.c:158` | Subscribe | const 6275 (`0x1883`) |
| | `sample_app.c:173` | Subscribe | const 6274 (`0x1882`) |
| ci_lab | `ci_lab_app.c:122`, `:131`, `:140` | Subscribe ×3 | const, 값 집합 {6276, 6277, 6278} (`0x1884`–`0x1886`) |
| | `ci_lab_app.c:194` | MsgInit | const 2180 (`0x0884`) |
| hk | `hk_app.c:139` | MsgInit | const 2203 (`0x089B`) |
| | `hk_app.c:163`, `:175`, `:187` | Subscribe ×3 | const, 값 집합 {6298, 6299, 6300} (`0x189A`–`0x189C`) |
| | `hk_utils.c:305` | MsgInit | table (copy table에서 온 `MidOfThisPacket`) |
| | `hk_utils.c:322` | Subscribe | table (`InputMid`) |
| | `hk_utils.c:436` | Unsubscribe | table (`InputMid`) |
| sch_lab | `sch_lab_app.c:246` | MsgInit | table (`ConfigEntry->MessageID`) |
| | `sch_lab_app.c:296` | Subscribe | const 6161 (`0x1811`, `CFE_TIME_ONEHZ_CMD_MID`) |
| to_lab | `to_lab_app.c:142` | MsgInit | const 2176 (`0x0880`) |
| | `to_lab_app.c:261` | Subscribe | param(`TO_LAB_CmdSubscribe`, 0). inline 사본에서 6272, 6273 |
| | `to_lab_app.c:454` | Unsubscribe | state (`ActiveSubs`) |
| | `to_lab_app.c:504` | SubscribeEx | table (`SubEntry->Stream`) |
| | `to_lab_cmds.c:112` | MsgInit | const 2177 (`0x0881`) |
| | `to_lab_cmds.c:218` | SubscribeEx | cmd_payload (`pCmd->Stream`) |
| | `to_lab_cmds.c:280` | Unsubscribe | cmd_payload (`pCmd->Stream`) |

**[해석]** 이 표는 추출 단계의 정답이다. race 검출의 정답이 아니다. 이 표를 맞춘다고 해서 원노트 H1이 검증되는 것도 아니다. H1은 앱 사이 관계를 복원하는 것이고, 그러려면 table 소스를 읽어 `table(...)` site를 값으로 바꾸는 단계(§5.8 P7)가 더 필요하다.

#### 7.13.5 MVP와 함께 둘 정상 사례

**[설계 제안]** 다음 세 사례를 정상 사례(음성)로 둔다. 분석기가 이들을 결함으로 보고하면 오경고다.

1. TO_LAB의 table 구독 지연과 그에 따른 `0x808` 미구독 발행 (§7.4.3). 근거는 소스 주석과 commit `d3d52da`다.
2. HK의 불완전 combined packet 전송. HK는 `DataPresent`와 `MissingDataCtr`로 이를 의도적으로 처리하고, 기본값 `HK_DISCARD_INCOMPLETE_COMBO 0`에서 그대로 보낸다 ([hk_utils.c L483-L504](https://github.com/nasa/HK/blob/0dc16b7a7bab71747b9c063cf405ad59a65a40a5/fsw/src/hk_utils.c#L483-L504), [hk_internal_cfg.h L57-L69](https://github.com/nasa/HK/blob/0dc16b7a7bab71747b9c063cf405ad59a65a40a5/fsw/inc/hk_internal_cfg.h#L57-L69)).
3. core service 초기화 중 sender가 `CFE_SB`·`CFE_TBL`·`CFE_TIME`인 `0x808` 미구독 발행 (§7.4.3). 이때는 아직 앱이 load되지 않았다.

**[미확인]** SBN의 `0x80e` 손실은 정상인지 결함인지 아직 모른다. SBN의 회복 경로를 읽기 전까지는 label을 붙이지 않는다.

#### 7.13.6 주장하지 않을 것

- **[해석]** import가 된다는 사실은 분석이 된다는 뜻이 아니다. 이 절의 결과는 frontend 단계의 실현 가능성일 뿐이다.
- **[해석]** `analyze_mlir.py`의 수는 regex probe의 검출 수다. 분석기의 coverage나 recall로 보고하지 않는다.
- **[해석]** §7.4의 SB event는 이 bundle과 실행 구성에서 관측한 것이다. cFS 일반이나 비행 build의 성질로 일반화하지 않는다.

### 7.14 실패했거나 아직 모르는 것

| 항목 | 상태 | 다음에 필요한 것 |
| --- | --- | --- |
| ClangIR | **[실측]** 로컬 불가 | CIR이 켜진 clang build. 디스크·시간 비용 측정. 154개 파일 coverage 측정 |
| clang으로 gcc DB 재사용 | **[실측]** `-Werror`로 2개 파일 실패 | `-Wno-error` 또는 clang configure tree 사용을 고정 |
| EDS build | **[미확인]** 컴파일하지 않음 | `native_eds` build와 MID 형태 관찰 |
| OSAL·PSP | **[미확인]** import하지 않음 | 연결 여부와 `OS_` 82개, `CFE_PSP_` 39개 외부 함수의 모델 결정 |
| cFE MSG·SBR·FS·ResourceId·Config | **[실측]** 154개 범위 밖, 외부 선언 44개 | 연결하거나 모델링 |
| 최적화·교차 build | **[미확인]** `-O2` 전체 import, RTEMS·VxWorks 미시험 | 대상 비행 설정 고정 뒤 시험 |
| 실제 손실 수 | **[미확인]** EVS filter 상한(4, 16)만 보임 | SB housekeeping의 `NoSubscribersCounter`, `MsgLimitErrorCounter`, `PipeOverflowErrorCounter` 수집, 또는 filter 해제 |
| SBN의 `0x80e` 손실 영향 | **[미확인]** | SBN 회복 경로 확인 |
| 시작 순서의 분포 | **[미확인]** 8회, 컨테이너 1개 | 다른 scheduler·부하·CPU 설정에서 반복 |
| host에서 `msg_max`를 키운 root 구성 | **[미확인]** host `msg_max`를 바꾸지 않았다. 두 성공 구성은 이 구성도, 비행 구성도 아니다 | 별도 VM에서 확인 |
| MID macro 이름 | **[미확인]** sidecar 미구현 | LibTooling preprocessor callback (§5.7.3) |
| 상태 저장 인식 | **[실측]** offset 0, out-parameter, 지역 포인터를 놓침 | API out-parameter 모델, 타입 기반 field 해석, points-to |
| MLIR points-to 재사용 | **[실측]** PoTATo(`80d157c`, `20e7d8f`)가 로컬 LLVM `1053047a`에 대해 `RegionSuccessor`·DataFlow API 오류로 build되지 않았다 (`$N/rw-mlir/potato_build.log`, `potato_build2.log`) | 버전을 맞춘 build 또는 자체 구현 |
| 비교 도구 | **[미확인]** CodeQL, PhASAR, SVF, Joern, Frama-C, IKOS, Goblint는 실행하지 않았다 | 같은 SB 모델로 기준선 실행 (§4) |
| OSAL·PSP 고정값의 통일 | **[확인된 사실]** 이 절과 §3의 ES·TBL probe가 서로 다른 OSAL·PSP를 썼다 | 하나의 고정값으로 다시 실행 |
| 분석 결과 | **[미확인]** `cfs` dialect, lifting pass, 분석 pass를 구현하지 않았다. 검출 결과가 없다 | §5.8, §6의 구현 |

**현재 판단.** **[해석]** 원노트의 초기 frontend 경로는 고정한 실제 cFS 코드에서 막히지 않는다. 막히는 곳은 그 뒤다. MID와 상태의 의미를 IR 밖의 정보(table 소스, build 설정, 명령, API 모델)와 함께 복원해야 한다. 다음 단계의 성패는 §7.13.4의 추출 정답표를 자동으로 재현하는지로 먼저 판단한다. race 검출 여부는 그다음 단계의 질문이다.
