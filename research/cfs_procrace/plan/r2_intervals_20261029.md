# R2 구간 분리 — 초기화 / OPERATIONAL 뒤 / 명령 재시작 뒤 (2026-10-29)

- 결정: 통합본(IP) §7, 2026-10-29 채택. "R2의 초기화 질문과 OPERATIONAL 이후 구독·종료 질문은 구간을 분리하여 설계한다". `Architecture/spec/PI_DECISIONS_RESOLVED_20261029.md`(PD29) §4 단계 3의 "R2 구간 분리".
- 계기: 새 F0 `f0b`에서 명령 재시작이 TO_LAB의 구독보다 먼저 왔고, 새 G-I `gi`에서는 TO_LAB의 구독 보고(EID 22)가 없었다. EID 14·22·25가 실행마다 다르다(`results_w4w5_20261028.md` §2.1, §2.4). 기대 판정 근거표의 R8 행은 "구간을 나누기 전에는 정하지 않는다"고 적었다(`expected_judgement_basis_C1_R2.md` §3.4 R8, §3.6 항목 2). 검토의 남은 위치 의존(CR §8.2, `REVIEW_ANALYSIS_20261029.md` §2.2)은 SCOPE가 병합 위치에 기댄다는 지적이다.
- 성격: **설계와 도구 문서**다. 실행 결과가 아니다. cFS를 기동하지 않았다. 실험 조건을 정하지 않았다(`CONDITIONS_POLICY.md`). 기대 판정을 나누거나 바꾸지 않았다(§7은 필요한 행의 목록일 뿐이다). 분석기(`kairos/lib`, `include`, `tools`)와 시험(`kairos/tests`)은 고치지 않았다.
- 소스 기준: ENV 번들 `/home/user/work/procrace/cfs_ref/cFS`. cFS `088b2fa8`(v7.0.1), cFE `c5fb2b4d`, 앱·PSP·OSAL은 번들 고정 판(`env/build_cfs.sh` PINNED_SUBMODULES). 줄 번호는 그 트리의 파일 기준이다.
- 표기: [F] 소스·문서에서 읽음, [M] 이번에 도구로 얻음, [I] 추론, [D] 결정, [U] 미확인, [기록] 앞서 보고했고 원 기록이 없어진 결과.

## 0. 요약

| 항목 | 내용 |
|---|---|
| 구간 | **I0 초기화**: ES 주 task가 OPERATIONAL에 들기 전. **I1 운용**: 그 뒤, 명령 재시작 전. **I2 종료**: 명령 재시작 뒤, 기록 끝(프로세스 종료)까지 (§1) |
| 새 관측 site | 셋. `es.main.operational`(cfe_es_start.c L233–L234), `es.restart_cmd.reset_cfe`(cfe_es_task.c L664), `sb.sub_reporting.set`(cfe_sb_task.c L1224–L1227). F0 계획 site 35–37, 계측 manifest site 42–44. 같은 label, 같은 위치(§2). 그리고 후속 지시로 프로필 SCOPE의 LP 둘(`CFE_PSP_Restart.request_shutdown` PSP L87, `OS_Application_Run.teardown` L507): F0 site 38–39, 계측 site 45–46(§6 항목 1) |
| 더하지 않은 것 | 프로세스 종료(기록 끝이 경계다), TO_LAB 구독 루프의 시작·끝(시작은 이미 관측된다. 끝은 `to_lab.so`에 있어 두 경로에 새 binary/module이 필요하다). §6에 lead·PI 몫으로 적었다 |
| 확인 [M] | F0: `kz anchor` 38 targets, 인용 43개 hash 일치. `kz observe --check` 38 sites, 셋 모두 주소에 묶임. 계측: manifest parser 수용. 파생 R2 빌드 binding **42개 모두 bound**, hook 42, hookcheck ok 42(§5). 후속 판(SCOPE LP 둘 더함): F0 40 targets·인용 45개, 계측 binding **44개 모두 bound**, hook 44, hookcheck ok 44(§5.1; 빌드 도중 컨테이너 재시작이 있었고 다시 돌렸다) |
| 배정 규칙 | 확립된 순서만 쓴다. 프로그램 순서(po), 시작 동기화(sync), OPERATIONAL 대기의 성공 반환(rf). 그 밖은 **기록 시각만(time)**으로 표시한다. time은 happens-before가 아니다(§3) |
| 스크립트 | `kairos/instrument/gcc/scripts/r2_intervals_tag.sh`(발생마다 구간·근거, 구간별 부류 수), `r2_intervals_selftest.sh`(두 경로 도구 시험, 통과) (§4) |
| 지금 기록으로 말할 수 있는 것 | 없다. 보존 기록은 없어졌고, 남은 F0 기록 `s1r2_p1`은 새 site 전의 계획이라 271개 발생 모두 "미정"이다(§4.3) |

## 1. 경계와 그 소스 [F]

### 1.1 초기화의 끝: OPERATIONAL

ES 주 task(`CFE_ES_Main`, 프로세스의 초기 스레드)의 프로그램 순서:

| 줄(cfe_es_start.c) | 내용 |
|---|---|
| L183 | `SystemState = CORE_STARTUP` |
| L188 | `CFE_ES_CreateObjects()`: 코어 앱마다 예약(L803) → `StartAppTask`(L810) → 확정(L818) → `MainTaskSyncDelay(RUNNING)`(L835), 실패면 panic(L845) |
| L194 | `SystemState = CORE_READY` |
| L201 | `CFE_ES_StartApplications`: 시작 스크립트의 앱 생성 |
| L210 | `MainTaskSyncDelay(LATE_INIT, …)` |
| L216 | `SystemState = APPS_INIT` |
| L225 | `MainTaskSyncDelay(RUNNING, …)`. 실패해도 계속한다(L226–L228, syslog만) |
| L233 | `CFE_ES_WriteToSysLog("%s: CFE_ES_Main entering OPERATIONAL state\n")`: 콘솔 줄 |
| L234 | `CFE_ES_Global.SystemState = CFE_ES_SystemState_OPERATIONAL` |

- `SystemState`에 OPERATIONAL(5)을 쓰는 곳은 L234뿐이다. 그 밖의 저장은 L87·L183·L194·L216이다. SHUTDOWN(6)은 시험 코드 밖에서 저장되지 않는다(grep, `default_cfe_es_extern_typedefs.h` L184–L214) [F].
- 콘솔 줄: `CFE_ES_WriteToSysLog`(cfe_es_api.c L1505–L1532)는 ES 잠금 안에서 syslog에 붙이고(L1524–L1526), `OS_printf`(L1529)로 넘긴다. OSAL 콘솔은 비동기다(`osal/default_config.cmake` L220 `OSAL_CONFIG_CONSOLE_ASYNC TRUE`). 그래서 줄이 실제로 찍히는 때는 L233 호출 안의 넘김보다 뒤다 [F].
- `MainTaskSyncDelay`(L860–L918): ES 잠금 안에서(L878–L888) 쓰이는 앱 레코드 가운데 `AppState < AppStateId`인 것을 센다(L882). 0이면 `CFE_SUCCESS`(L890–L894). 아니면 poll 주기만큼 쉬고 다시 센다.

### 1.2 종료의 시작: 명령 재시작

| 단계 | 소스 | 내용 |
|---|---|---|
| 하네스 | `env/run_cfs.sh` L27–L31, L98, L303–L307 | `operational` 모드에서 콘솔에 L233의 줄(README L114)이 보이면 `cmd_send --pktid=0x1806 --cmdcode=2 --half=0x0002`(ES Restart, POWERON)을 보낸다 |
| ES 명령 경로 | `cfe_es_dispatch.c` L102–L104 → `cfe_es_task.c` L647–L668 | `CFE_ES_RestartCmd`: RestartType이 PROCESSOR·POWERON이면(L651) `CFE_ES_ResetCFE(cmd->RestartType)`(L664). 돌아오지 않는다(L661–L663 주석) |
| ResetCFE | `cfe_es_api.c` L65, POWERON 갈래 L126–L146 | L128 syslog "POWERON RESET called from CFE_ES_ResetCFE (Commanded).", L134 ER 기록, L142 `CFE_PSP_Restart` |
| PSP | `psp/fsw/pc-linux/src/cfe_psp_support.c` L60–L108 | L64 "Exiting cFE with POWERON Reset status.", L67 예약 메모리 삭제, L87 `ShutdownReq = true`, L88 `pthread_kill`(주 스레드), L99 지연, 그 뒤 abort(L99–L107) |
| 주 스레드 | `cfe_psp_start.c` L452–L510 | `sigwait` 루프(L488–L499)가 `ShutdownReq`로 끝나고, L504 "Shutdown initiated", L507 `OS_DeleteAllObjects()` |
| 프로세스 끝 | `osal/src/bsp/generic-linux/src/bsp_start.c` L244–L247 | `OS_Application_Run()`이 돌아오면 `main`이 돌아온다 |

- `CFE_ES_ResetCFE`의 다른 호출: `cfe_es_api.c` L352(`CFE_ES_ExitApp` L292 안, 코어 앱 초기화 오류), HS `hs_monitors.c` L138·L290. 명령 경로가 아니다. 새 site는 이것들을 표시하지 않는다(§6 항목 3).
- 프로필의 SCOPE: `profiles/cfs-v7.0.1/10_kinds.kzp` L22–L29는 "기본 범위는 종료 요청에서 끝난다(SPEC 1.6)"라고 선언한다. 요청 LP는 `CFE_PSP_Restart.request_shutdown`(PSP L87 저장, `20_ops.kzp` L238–L243), teardown LP는 `OS_Application_Run.teardown`(L507 호출, `20_ops.kzp` L245–L249)이다. **2026-10-29 전의 R2 관측 계획은 이 두 LP를 관측하지 않았다.** 그래서 그 계획의 R2 기록을 재생하면 `shutdown_request no`이고, 명령 재시작 뒤의 사용을 포함해 모든 발생이 `scope default`다 [M: `s1r2_p1` 재생]. 계획과 프로필 사이의 빈틈이었다. 두 LP를 더한 것은 §6 항목 1이다.

### 1.3 프로세스 종료

경계 site가 필요 없다. 기록에 있는 사건은 모두 종료 전이다. F0는 대상이 끝날 때까지 기록한다(BASIS `target.exit`). 계측 경로는 atexit 처리기가 관측 창을 닫고 dump를 쓴다(`kairos/runtime/src/kz_rt_posix_auto.c` 머리말 항목 2). 그 뒤의 사건은 기록되지 않는다 [F].

### 1.4 SB 구독 보고 켜기

| 소스 | 내용 |
|---|---|
| `cfe_sb_init.c` L60 | 시작 스레드의 EarlyInit에서 `SubscriptionReporting = CFE_SB_DISABLE`(L50 memset 뒤) |
| `cfe_sb_task.c` L1224–L1227 | `CFE_SB_SetSubscriptionReporting(state)`: 플래그의 유일한 저장 |
| `cfe_sb_task.c` L294–L298, L306–L310 | 켜기·끄기 명령이 setter를 부른다. 분배는 `cfe_sb_dispatch.c` L93–L96 |
| `cfe_sb_api.c` L1192–L1195 → `cfe_sb_task.c` L659, L670–L677 | 구독이 성공하고 범위가 GLOBAL이면 보고 함수를 부른다. 그 함수는 구독한 task의 문맥에서 플래그를 읽고(L659), ENABLE이면 보고를 보낸 뒤 `CFE_SB_Global.AppId`로 EID 22를 낸다 |
| `cfe_sb_priv.h` L71–L72 | `CFE_SB_DISABLE 0`, `CFE_SB_ENABLE 1` |
| `apps/sbn/fsw/src/sbn_app.c` L1536–L1540, `sbn_subs.c` L44–L46 | SBN이 OPERATIONAL 대기를 성공할 때까지 되풀이한 뒤, 켜기 명령을 보낸다. 번들에서 켜기 명령을 보내는 곳은 여기뿐이다(grep) |

### 1.5 TO_LAB 구독 루프

| 소스 | 내용 |
|---|---|
| `apps/to_lab/fsw/src/to_lab_app.c` L62–L68 | 초기화 성공 뒤 `CFE_ES_WaitForStartupSync`(반환값을 버림) → `TO_LAB_UpdateSubscriptionsFromTable()` |
| L458–L527 | 루프 본체. 이전 구독 지우기(L467, 처음에는 없음) → 표 항목마다 `CFE_SB_SubscribeEx`(L476–L477) → 끝에 "TO Lab subscribed to %u messages from the table" 이벤트(L520–L524, `CFE_EVS_SendEvent`) |
| L552 | 표가 바뀌면 같은 함수를 다시 부른다 |
| `cfe_es_api.c` L616–L619 | `WaitForStartupSync` = `WaitForSystemState(OPERATIONAL, …)` |

- 루프의 시작은 이미 관측된다. TO_LAB actor의 `es.wait.ret`(WaitForSystemState의 반환) 바로 뒤가 루프이고, 처음 부를 때는 그 사이에 관측 사건이 없다. 루프의 각 반복은 SB 사건(EID 10 `cfe_sb_api.c` L1183, 보고가 켜졌으면 EID 22, 넘치면 EID 25)으로 관측된다.
- 루프의 끝(L520의 이벤트)은 관측되지 않는다. 그 이벤트는 `CFE_EVS_SendEventWithAppID`가 아니다. 그리고 그 코드는 `to_lab.so`에 있다(§6 항목 2).

## 2. 더한 관측 site [F][M]

| label | 소스 | F0 `observe/sites.kzp` | 계측 `instr/sites.kzs` | 같은 의미의 근거 |
|---|---|---|---|---|
| `es.main.operational` | cfe_es_start.c L233–L234 | site 35, L208. `point at=cfe_es_start.c:233 after-call=CFE_ES_WriteToSysLog`, 값 없음 | site 42, L212. `place=after-call callee=CFE_ES_WriteToSysLog line=233`, 값 없음 | 같은 호출 바로 뒤. F0 주소 `0x24e22`는 L234의 저장 명령(`movl $0x5, CFE_ES_Global+0x18`)이고 probe는 그 실행 전에 걸린다 [M: objdump]. 둘 다 "L233 줄을 넘긴 뒤, L234 저장 전"이다 |
| `es.restart_cmd.reset_cfe` | cfe_es_task.c L651–L664 | site 36, L210. `point at=cfe_es_task.c:664 at-call=CFE_ES_ResetCFE s0=rdi:u32:reset_type` | site 43, L214. `place=at-call callee=CFE_ES_ResetCFE line=664`, `value 0 reset_type from=arg0 u32` | 같은 호출. F0 주소 `0x272f5`는 `call CFE_ES_ResetCFE`이고 바로 앞이 `mov %eax,%edi`다 [M]. 이 호출은 `CFE_ES_RestartCmd` 안의 하나뿐이다 |
| `sb.sub_reporting.set` | cfe_sb_task.c L1224–L1227 | site 37, L212. `entry func=CFE_SB_SetSubscriptionReporting s0=rdi:u32:state` | site 44, L217. `phase=before func=CFE_SB_SetSubscriptionReporting line=1224`, `value 0 state from=arg0 u32` | 같은 함수 진입. 진입과 L1226 저장 사이에 다른 문장이 없다 |
| `CFE_PSP_Restart.request_shutdown` (후속, §6 항목 1) | cfe_psp_support.c L87; 프로필 `20_ops.kzp` L240 | site 38, L225. `point at=cfe_psp_support.c:87 line-start`, 값 없음 | site 45, L228. `phase=effect func=CFE_PSP_Restart line=87 place=store`, 값 없음 | 프로필 LP `store(CFE_PSP_IdleTaskState_t.ShutdownReq)`. F0 주소 `0x5c21b`는 L87의 `movb $0x1, CFE_PSP_IdleTaskState+0x8`이고 probe는 그 실행 전이다(줄에 호출이 없어 `CFE_ES_WaitForSystemState.raise_state`와 같은 근사). GCC는 저장 바로 뒤다. 그 사이에 관측 사건은 없다(다음 문장 L88 `pthread_kill`) |
| `OS_Application_Run.teardown` (후속) | cfe_psp_start.c L488–L507; 프로필 `20_ops.kzp` L247 | site 39, L226. `point at=cfe_psp_start.c:507 after-call=OS_DeleteAllObjects` | site 46, L231. `phase=effect func=OS_Application_Run callee=OS_DeleteAllObjects line=507 place=after-call` | 프로필 LP `call("OS_DeleteAllObjects")`, 효과는 그 함수가 한다(after-call, 계측 manifest 머리말의 규칙). 같은 호출 바로 뒤. F0 주소 `0x5be67`은 L508의 `nop`이다 [M: objdump] |

- 각 site에 인용 줄과 sha256을 붙였다(F0 `prov es_main_operational` `69e78ab4…`, `es_restartcmd_reset` `782826d7…`, `sb_set_subreporting` `09e5ade5…`; `sed -n 'A,Bp' | sha256sum`, 기존 줄 L82의 값 `a99fd434…`로 방법을 확인함) [M].
- 계측 쪽 `equiv … status=unconfirmed`. 이름이 같다고 같은 의미가 확인된 것은 아니다(`kairos/docs/event_semantics.md` §3). 위치 관계는 `ref`에 적었다.
- 두 파일 모두 **끝에만 더했다**(후속 두 site도 끝에 더했다. 계측 manifest에는 `source psp rev=c4b3b0b6…`(번들 고정 판)도 끝에 더했다). 기존 줄 번호가 그대로라서, 계측 manifest의 `equiv` ref가 인용하는 F0 계획 줄(L74–L190)이 바뀌지 않는다. 계측 manifest 머리 주석의 "(35)"만 "(35; 38 with the interval sites 42-44 at the end)"로 고쳤다(줄 수 같음).
- 처음 셋(35–37 / 42–44)은 프로필의 LP가 아니다(label이 OP.LABEL이 아니다). 후속 둘(38–39 / 45–46)은 LP이고 재생이 묶는다(§6 항목 1). 재생은 이 사건들을 "events of unmapped sites"로 세고 버린다. 점 site의 out 값 경로(`lib/trace/real_trace.cpp` L390–L415)에도 걸리지 않는다. 둘째 site의 값 역할 `reset_type`은 어느 연산의 out 매개변수도 아니고, 그 지점에서 열린 프로필 연산 호출도 없다 [F]. 도구 시험 [M]: 같은 합성 일정에서 세 사건만 뺀 기록과 넣은 기록의 재생을 `kz occ diff`하면 발생 10 대 10, 키 짝 10, 판정 차이 0, `result: exit 0`이다.
- 관측 부담: F0 probe 셋(후속 둘을 더하면 다섯; 요청과 teardown은 한 실행에 한 번씩). 한 실행에서 각각 한 번(OPERATIONAL, 재시작), setter는 켜기·끄기 명령 수만큼 걸린다. 다른 site의 횟수와 비교하면 작다. 측정하지 않았다 [U].

확인 [M]:

| 확인 | 명령 | 결과 |
|---|---|---|
| F0 계획 묶기 | `kz anchor <ENV core-cpu1> observe/sites.kzp -o basis.kz --source-root <ENV 트리>` | 종료 0. 38 targets, 함수 기호 1788. 인용 43개가 선언한 hash와 같다. hash 없는 1개는 `spec_s1r2`(원래 그렇다). 후속 두 site를 더한 판: 40 targets, 인용 45개 일치(PSP 인용 둘은 `01_prov.kzp`의 `psp_restart_req` `94bc76fe…`·`psp_teardown` `5005392f…`와 같은 줄·hash) |
| F0 계획 점검 | `kz observe basis.kz observe/sites.kzp -o … --check -- /bin/true` | 종료 0. "hash, size, build-id and the bytes at 38 targets match". site 35 `CFE_ES_Main` `0x24e22`, site 36 `CFE_ES_RestartCmd` `0x272f5`, site 37 `CFE_SB_SetSubscriptionReporting` `0x35b36`. 후속 판: 40 targets 일치, site 38 `CFE_PSP_Restart` `0x5c21b`, site 39 `OS_Application_Run` `0x5be67` |
| 계측 manifest 읽기 | `kz-rt-dump --manifest instr/sites.kzs /dev/null` | manifest 오류 없음(dump magic 오류만). 대조: phase가 틀린 줄을 더한 사본은 `sites.kzs:220: phase is …`로 거부 |
| 계측 binding | §5의 파생 빌드 | site 42·43·44가 bind `66208`(after-call), `66224`(at-call), `66240`(entry), 모두 `status=bound`. 후속: site 45 → `66256`(store), 46 → `66272`(after-call), bound |

파일 sha256 [M]: `observe/sites.kzp` `426d3e01…`, `instr/sites.kzs` `f55a8079…`(후속 두 site까지 넣은 판. 셋 site만의 판은 `2adf247e…` / `7a2bfe0e…`, 그 전 `897c7510…`).

## 3. 구간 배정 규칙 — 순서를 만들어 내지 않는다 [D][F][I]

발생(R2의 `CFE_EVS_SendEventWithAppID` 호출, 기록 사건 `evs.send.entry`)마다 두 관계를 따로 정한다. B1(`es.main.operational`)에 대해 앞·뒤, B2(`es.restart_cmd.reset_cfe`)에 대해 앞·뒤다. 관계마다 근거를 붙인다.

| 근거 | 조건(기록에서) | 소스 · 등급 |
|---|---|---|
| **po** 프로그램 순서 | 발생이 경계를 기록한 actor에 있다. aseq로 앞·뒤 | 한 actor 안의 순서 |
| **sync** 시작 동기화 (B1 앞) | ① 발생이 그 actor의 RUNNING 올림(`CFE_ES_WaitForSystemState.raise_state`)보다 aseq가 앞이다. 그 올림은 `min_system_state ≥ OPERATIONAL`인 대기 안의 것이거나, 코어 앱(요청 함수 `CFE_ES_CreateObjects`)의 `≥ CORE_READY` 대기 안의 것이다. ② 그 actor는 주 task가 만들었고, 그 생성 사건이 주 task의 성공한 `MainTaskSyncDelay(app_state ≥ RUNNING)` 진입보다 앞이다. ③ 그 동기화가 B1보다 주 task에서 앞이다 | 올림 수준은 `cfe_es_api.c` L536–L569, 올림 저장 L571–L574(ES 잠금 안, L528–L576). 동기화는 잠금 안에서 쓰이는 레코드의 `AppState`를 읽고(cfe_es_start.c L878–L894), 성공하면 모든 그런 레코드가 RUNNING 이상이다. 올림 전에는 그 레코드가 RUNNING 미만이었으므로(L571) 올림이 동기화보다 앞이다. 레코드는 task를 만들기 전에 예약된다(core: cfe_es_start.c L803 → L810, 앱: cfe_es_apps.c L821 → L849). 그래서 생성 뒤의 동기화는 그 레코드를 센다. 동기화 → L233–L234는 주 task의 프로그램 순서다. 등급: 코드(추론) |
| **rf** 대기 반환 (B1 뒤) | 발생이 그 actor의 `es.wait.ret`(status `CFE_SUCCESS`, 짝 `es.wait.entry`의 `min_system_state ≥ OPERATIONAL`)보다 aseq가 뒤다 | 대기 루프는 잠금 없이 `SystemState`를 읽고(cfe_es_api.c L581–L585 "only dependent on the main (startup) task updating the global variable") 시간이 다하면 `OPERATION_TIMED_OUT`이다(L599). 성공은 OPERATIONAL 이상을 읽었다는 뜻이고, 그 값을 쓰는 저장은 L234뿐이다. 값에 의한 reads-from이다. 잠금에 의한 동기화가 아니다. 등급: 코드(추론) |
| **time** 기록 시각만 | 위 셋이 서지 않을 때, 두 사건의 기록 시각(F0·계측 모두 CLOCK_MONOTONIC)으로 앞·뒤 | **순서 근거가 아니다.** happens-before로 쓰지 않는다 |
| **none** | 시각이 없거나 같거나, 경계가 기록에 없음 | 배정하지 않음 |

- 구간: I0 = B1 앞. I2 = B2 뒤. I1 = B1 뒤이면서 B2 앞. 배정 근거는 그 구간에 필요한 관계의 근거다(I1은 둘, 예 `rf/time`). 필요한 관계가 모두 po·sync·rf일 때만 "established"로 센다. B1 뒤인데 B2가 기록에 없으면 `after-operational`, 둘이 어긋나면(B1 앞이면서 B2 뒤) `inconsistent`로 따로 둔다.
- B2와의 관계는 ES task 안에서만 po로 선다. 다른 task와 ES task의 재시작 사이에는 기록된 동기화가 없다. 그래서 **TO_LAB의 사건이 명령 재시작 앞인지 뒤인지는 새 site로도 기록 시각으로만 정해진다.** `results_w4w5_20261028.md` §2.1의 "EID 25 31개 모두 재시작 뒤"([I], 콘솔 줄 기준)는 새 site가 있어도 time 근거다.
- B1과 B2 사이: 하네스는 L233의 줄을 읽은 뒤 명령을 보낸다(§1.2). 그 줄은 L233 호출 안에서 넘겨지고, B1 점은 그 호출 뒤다(§1.1). 그래서 B1 점과 B2 사이에도 기록된 순서는 없다. 스크립트는 둘의 순서를 기록 시각으로 적는다. 하네스의 규칙(기록 밖)을 배정에 쓰지 않는다.
- sync가 덮지 않는 것 [F][I]:
  - RUNNING 올림을 `CFE_ES_RunLoop`의 첫 호출(cfe_es_api.c L469–L472)에서만 하는 앱. 계획이 그 저장(프로필 `raise_running`)을 관측하지 않는다. 시작 대기를 부르는 앱은 CS·HS·SBN·TO_LAB뿐이다(grep: `cs_app.c` L77, `hs_app.c` L75, `sbn_app.c` L1536·L1647, `to_lab_app.c` L67).
  - child task. 동기화는 앱 레코드만 센다.
  - 시작 스크립트 뒤에 명령으로 만든 앱. 동기화 때 레코드가 없었다.
  - 이런 발생은 time이다.
- 구독 보고: EID 22 발생은 구독한 task가 플래그를 ENABLE로 읽은 뒤다(§1.4). ENABLE을 쓰는 것은 `state=1`인 setter뿐이다. 그래서 setter(켜기) → EID 22는 값에 의한 reads-from이다. 스크립트는 이것을 배정에 쓰지 않는다. setter 자체(SB task)의 B1과의 관계는 time이기 때문이다. 대신 각 EID 22에 앞선 켜기가 기록 시각으로 있는지 센다. 없으면 기록 시각을 그대로 믿을 수 없다는 신호다.
- SCOPE와의 관계(CR §8.2): 재생의 SCOPE는 발생의 결정과 종료 요청의 **병합 순서**로 정한다(`kairos/docs/replay_output.md` §7.4, `lib/core/eval_trace.cpp` L1890). 다른 actor 사이에서 그것은 기록 시각과 표현의 순서다. 이 문서의 time 근거와 같은 약함이다. 스크립트는 발생마다 재생의 `scope default|teardown`을 함께 적어, 구간과 SCOPE를 나란히 보게 한다. 2026-10-29 전의 계획에서는 요청 LP가 관측되지 않아 SCOPE가 아예 걸리지 않았다(§1.2). 두 LP를 더한 뒤 재생의 SCOPE가 무엇에 기대는지는 §6 항목 1이다.

## 4. 스크립트

### 4.1 `kairos/instrument/gcc/scripts/r2_intervals_tag.sh <run.kzt> <replay.txt> <out>`

- 입력: F0 run 컨테이너(스크립트가 `kz convert`로 중립 층을 만든다) 또는 중립 층이 있는 run 컨테이너(계측: `kz convert --kzrt … --out`). 그리고 같은 컨테이너의 `kz replay` 텍스트 출력(어느 읽기든).
- 발생과 기록 사건 잇기: 재생 텍스트의 `occurrence … actor A … call #c`에서, #c가 사건표에서 A의 몇 번째 `CFE_EVS_SendEventWithAppID call`인지 센다. 그것을 중립 층에서 A의 같은 번째 `evs.send.entry`와 잇는다. 한 actor 안의 순서는 선형화가 지킨다(`replay_output.md` §7.3). actor 이름은 `lib/trace/neutral.cpp` `actor_names`와 같은 규칙으로 만든다. 잇고 나서 발생 키 값과 기록된 `app_id`가 같은지, actor별 호출 수가 같은지 확인한다. 하나라도 어긋나면 종료 2다.
- 코어 앱 여부(sync 규칙의 CORE_READY 대기): 재생 텍스트의 identity 줄(`core service` / `not a core service` / `requested from F`)을 먼저 쓰고, 없으면 계측 binding이 적은 요청 함수(NACTORS `request_func`)를 쓴다. F0 기록은 요청 함수를 반환 주소로만 가지므로, 재생을 `--basis`와 `--core-request-caller CFE_ES_CreateObjects`로 돌린 텍스트(`w4w5_r2.sh`와 같음)에서만 코어 여부가 선다. 없으면 코어 task의 초기화 사용은 time으로 남는다(약해질 뿐 틀리지는 않는다).
- 부류: `appid0`(전달 AppID 0), `self`(실행 task의 소유 앱 id = 전달 AppID; SB 자기 전송은 `self`이고 AppID `0x110003`), `other`(다른 task의 사용), `owner-unknown`.
- 출력:
  - `occ.tsv`: 발생마다 actor, 소유, AppID, EventID, 부류, 기록 사건, aseq, 시각, B1·B2 관계와 근거, 구간, 배정 근거, established, 재생의 상태·요약·scope, 그리고 프로필의 종료 요청(`CFE_PSP_Restart.request_shutdown`)에 대한 관계와 근거(요청 actor 안이면 po, 아니면 time).
  - `boundaries.tsv`
  - `intervals.txt`: 구간 × 부류(established와 time/none), × 배정 근거, × 요약 뷰, × AppID·EventID, actor별 구간 분포(TO_LAB 루프의 분포가 여기 나온다), 재생 scope 대 구간, EID 22 점검.
  - `sha256.txt`

### 4.2 `r2_intervals_selftest.sh <work>` — 도구 시험 [M]

**도구 시험 값이다.** 연구 조건도 결과도 아니다. cFS는 돌리지 않았다.

- 계측 경로:
  - §5의 런타임과 binding 목록을 쓰는 작은 C 프로그램이 합성 일정을 기록한다. 일정은 코어 시작(ES, SB), 시작 대기가 있는 앱 하나와 없는 앱 하나, 주 task의 RUNNING 동기화 셋, OPERATIONAL, 보고 켜기, 명령 재시작, 그리고 경계 앞뒤의 호출 10개다. 단계는 조건 변수로 하나씩 돈다.
  - `kz-rt-dump` 점검 `check ok (0 failures)`. 변환에서 소유 confirmed 4, undetermined 1(초기 스레드).
  - 재생 뒤 10개 발생의 구간·근거가 기대표와 모두 같다: init sync 4, init time 1, operational rf/time 2, operational time/time 1, shutdown po 1, shutdown time 1. EID 22 점검 1/1.
  - 후속(§6 항목 1): ES task가 재시작 뒤 `CFE_PSP_Restart.request_shutdown`을, 주 task가 끝에 `OS_Application_Run.teardown`을 기록한다. 일정 둘을 돌린다. `base`(ES의 재시작·요청 단계 16, TO_LAB의 마지막 호출 단계 17)와 `swap`(두 단계를 맞바꿈)이다. 발생마다 재생의 scope와 요청에 대한 관계·근거도 기대표와 대조한다. 둘 다 10/10 일치.
- F0 경로: 남은 F0 기록 `env/logs/s1r2_p1.kzt`(2026-10-15, 계획 site 0–7)와 그 basis를 쓴다. 발생 271개가 모두 기록 사건과 이어진다(문제 0). 경계 site가 없는 계획이라 하나도 배정되지 않는다.
- 결과: `selftest passed`(2026-10-29, scratch; 후속 판도 통과).

### 4.3 지금 있는 기록에 대해

- 보존 기록 넷(`v153_pilot3`, d4 run1, `f0b`, `gi`)은 2026-10-29 컨테이너 교체로 없어졌다(RA §0.1).
- `s1r2_p1`은 새 site 전의 계획이다. 그래서 [기록]의 수(269/268/272/207)를 구간으로 나눈 값은 없다. 나누려면 새 계획으로 실행해야 한다(PD29 §4 단계 3의 캠페인).

## 5. 파생 R2 빌드 다시 만들기 [M]

- manifest가 바뀌었으므로(`897c7510…` → `7a2bfe0e…`) `env/REBUILD_20261029.md` §6.2의 절차로 다시 만들었다. 런타임은 현행 `kzrt 0.4.0`이다(같은 문서 §6 "새 실행을 위한 새 빌드").
- 명령(root, 2026-10-29 05:12–05:21 UTC). `/home/user/work/rebuild/scripts/derive_all.sh`의 R2 부분과 같다:
  ```
  build_runtime.sh /home/user/work/instr/gcc/d4/rt-r2 kairos/profiles/cfs-v7.0.1/instr/sites.kzs /home/user/work/instr/gcc/plugin-build/kzgcc.so
  CFS_NORMAL_USER=ubuntu derive_r2.sh prepare|build ref|build instr <kzgcc.so> <rt-r2>|check /home/user/work/instr/gcc/d4/r2
  ```
- 이전 빌드(binding 39, `core-cpu1` `dfebff9f…`)는 지우지 않았다. `/home/user/work/instr/gcc/d4/superseded_20261029_sites39/`로 옮겼다(`r2/`, `rt-r2/`, 로그 둘). 옮긴 트리는 경로가 바뀌어 같은 경로 비교에는 쓸 수 없다.

| 항목 | 결과 | 이전(§6.2) |
|---|---|---|
| 런타임 | `rt-r2` 종료 0. `libkzrt.a` `718acdfd…`, `libkzrt-posix.a` `d0fbd6a4…`, `kz-rt-dump` `21550a70…`(같음). build identity 객체 `806ea24c…`(manifest hash가 들어 있어 다름) | `fd10359c…` |
| prepare | 종료 0. 차이는 hook 파일 둘 | 같음 |
| ref · instr 빌드 | prep·install 종료 0, 경고 0, 275 s · 268 s. 객체(cpu1) 1335 | 같음 |
| binding | **42개, 42 bound** | 39, 39 bound |
| 링크 뒤 hook(`core-cpu1`) | 42 | 39 |
| hookcheck | `rc=0 summary: ok=42` | ok=39 |
| strip-debug 뒤 다른 객체 | 280 = 계측 단위 12(`cfe_es_task.c` 더함) + D4 1 + coverage 267. "UNEXPECTED" 0 | 279 = 11 + 1 + 267 |
| coverage 단위(plugin이 건드리지 않음) | 11(`cfe_es_task.c`의 시험용 사본 더함) | 10 |
| 새 site bind | 42 → `66208` after-call, 43 → `66224` at-call, 44 → `66240` entry, module 1 | — |
| 산출물 | 계측 `core-cpu1` `2d12febde06cbd71dcef187c763362f77c6165c5dbd04249e977a7979fffcdb3`. 같은 경로 기준 빌드 `2644bb49…`(날짜가 다름). `binds_instr.kzs` `6791ec09…` | `dfebff9f…` / `64a7158f…` |

### 5.1 후속 재빌드: 종료 범위 LP 두 site (2026-10-29 05:43–05:57 UTC) [M]

- manifest `7a2bfe0e…` → `f55a8079…`(site 45·46, `source psp`). 같은 절차를 다시 밟았다. 각 단계 앞에서 `/home/user/work/.campaign_running`이 없는지 확인하고, 있으면 기다리게 했다. 위 42-site 빌드는 `/home/user/work/instr/gcc/d4/superseded_20261029_sites42/`로 옮겼다(지우지 않음).
- `build_runtime.sh`(05:43:49) 종료 0. build identity 객체 `265a78f1…`, 라이브러리·`kz-rt-dump`는 위와 같다. `prepare` 종료 0. `build ref`(05:43:53, 237 s) 종료 0.
- **컨테이너 재시작**: 05:49Z쯤 `build instr` 도중 컨테이너가 다시 시작되었고, 그 빌드와 그것을 돌리던 작업이 끊겼다.
  - 끊긴 instr 변형은 지우지 않고 `/home/user/work/instr/gcc/d4/superseded_20261029_restart_partial/`로 옮겼다. 거기에 README가 있다. 옮긴 사람은 lead다.
  - 그 디렉터리에는 끊긴 `build-native_std`, `binds_instr/`, `build_instr.*`가 있다. 재시작 직후의 첫 재개 시도 기록도 있다. 그 시도의 `build instr`는 "refusing: build-native_std exists"로 거부되었다. 그 상태에서 돈 `check`는 binding 4개, hookcheck `missing=4`로 실패했다.
  - ref 변형(같은 경로 객체 hash `objects_ref.sha256`)은 그대로 두었다.
- 재개(lead, scratch `rebuild_r2c.sh`): `derive_r2.sh build … instr`(05:52:29, 284 s) 종료 0, 경고 0, 객체(cpu1) 1335. 이어서 `check`(05:57:25) 종료 0. 기록: `d4/r2.instr.resume.log`, `d4/r2.check.resume.log`.

| 항목 | 결과 | 42-site 빌드 |
|---|---|---|
| binding | **44개, 44 bound** | 42 |
| 링크 뒤 hook(`core-cpu1`) | 44 | 42 |
| hookcheck | `rc=0 summary: ok=44` | ok=42 |
| strip-debug 뒤 다른 객체 | 282 = 계측 단위 14(`cfe_psp_support.c`, `cfe_psp_start.c` 더함) + D4 1 + coverage 267. "UNEXPECTED" 0 | 280 |
| coverage 단위 | 11 | 11 |
| 새 site bind | 45 → `66256`(store, `cfe_psp_support.c:87`), 46 → `66272`(after-call, `cfe_psp_start.c:507`), module 1 | — |
| 산출물 | 계측 `core-cpu1` `e1921179e54835836bd2f2a9ff958a8e21f5acc8e319143645fde097bccbe737`. `binds_instr.kzs` `46ecdd0b…`. plugin `56a96b79…`(같음) | `2d12febd…` |

## 6. 레퍼런스로 정하지 못한 것 — lead·PI 몫 [U]

1. **SCOPE LP의 관측 — 2026-10-29 후속 지시로 더함.**
   - 지시(lead): 분석 의미의 레퍼런스는 `profiles/cfs-v7.0.1/10_kinds.kzp` L22–L29다. 이 블록은 모든 기본 범위를 종료 요청에서 끝낸다(prov `psp_restart_req`, `psp_teardown`). R2 계획이 그 지점을 관측하지 않은 것은 계획과 프로필 사이의 빈틈이다. 새 규칙이 아니다. 기록에 그 지점이 실리려면 실행 때 관측해야 한다.
   - 한 것: 계획의 이름 규칙대로 두 site를 두 경로 끝에 더했다(§2 표: F0 site 38·39, 계측 site 45·46). 확인은 `kz anchor` 40 targets·인용 45개, `kz observe --check` 40 sites(§2)이다. 파생 빌드는 binding 44, hook 44, hookcheck ok 44(§5.1)이다. 엔진 옵션은 더하지 않았다. `lib`·`include`·`tools`·`tests`도 고치지 않았다.
   - 재생이 이제 하는 일. 합성 도구 시험(§4.2 `base`·`swap`)에서 [M], 엔진 소스로 [F]:
     - 요청: 종류 `es.shutdown`이 SHUTDOWN_REQ에 이르는 첫 효과의 trace 위치다(`lib/core/eval_trace.cpp` L2377–L2381). 출력은 `shutdown_request yes at #43 a2 CFE_PSP_Restart.request_shutdown`이다.
     - 범위: 발생마다 결정할 때 `in_scope = !(shutdown_requested && shutdown_event < decision)`(같은 파일 L1886–L1891)다. **trace 위치의 비교이고 순서 근거를 보지 않는다.** trace 위치는 선형화한 병합 순서다(`replay_output.md` §7.3). 한 actor 안에서는 프로그램 순서를 지킨다. actor 사이에서는 변환의 병합(시계, 시각, decode 순서, actor)을 따르고, 알려진 순서 간선이 요구할 때만 옮긴다.
     - 요청한 actor(ES task `a2`) 안의 발생: 요청 뒤의 호출은 `decided at … #44 scope teardown`이다. 근거는 **프로그램 순서**다. 시각과 무관하게 선다.
     - 다른 actor(TO_LAB 꼴 `a4`)의 발생: `base`에서는 요청보다 기록 시각이 뒤라 `scope teardown`이다. `swap`에서는 같은 호출이 두 단계만 맞바뀌어 요청보다 앞에 기록되었고, `scope default`다. 근거는 **병합 위치, 곧 기록 시각뿐**이다. 그 actor와 요청 사이에 기록된 순서 간선이 없다. happens-before가 아니다(CR §8.2).
     - 판정은 바뀌지 않았다. 같은 `base` 일정에서 두 지점만 뺀 기록과 비교했다. `kz occ diff` 종료 0, 판정 차이 0, 근거 차이 0, 요약 같음(INCONCLUSIVE(not_evaluated) 8, OBSERVED_VIOLATION 2).
     - 이 엔진에서 범위가 하는 일은 다음뿐이다(`in_scope`의 쓰임: L1890, L3309, L3688, L4263) [F]. teardown 범위 발생 자신의 상태는 바꾸지 않는다.
       - 발생의 `scope` 표시. 텍스트 출력에만 있다. `--out` 컨테이너(SUMMARIES, OCC_BASIS …)에는 범위가 없으므로 `kz occ diff`는 범위 차이를 보지 못한다 [M: kz-dump].
       - 기본 범위 발생에 대해 TEARDOWN 지점과 그 뒤의 쓰기를 읽기 순서에서 뺀다(L1066–L1090, 위치와 HB).
       - 같은 쓰기를 실행 관계에서 뺀다(L3307–L3312: 그 지점, 그 actor의 프로그램 순서 뒤, 또는 HB 뒤).
       - 종료 monitor: TEARDOWN 뒤의 어떤 사건도 기본 범위 발생을 결정하지 않는다(L3688).
     - 실제 R2에서는 `CFE_PSP_Restart`가 ES task에서 돌아오지 않는다(`cfe_psp_support.c` L99–L107). 그래서 teardown 범위가 될 R2 발생은 거의 다른 actor(TO_LAB 등)의 것이다. 그 범위는 기록 시각으로만 정해진다. `r2_intervals_tag.sh`는 발생마다 재생의 scope와, 요청에 대한 관계·근거(po 또는 time)를 나란히 적는다(§4.1).
     - 두 경로의 위치: F0는 L87 저장 전, GCC는 저장 뒤다. 그 사이에 그 actor의 관측 사건은 없다. teardown은 두 경로 모두 `OS_DeleteAllObjects` 호출 바로 뒤다.
   - 민감도 읽기(범위를 적용하지 않는 읽기):
     - **기존 옵션이 없다.** `kz replay --help`의 선택지(`--outcome-narrowing`, `--compare-narrowing`, `--clearing-writes`, `--owner-identity-targets`, `--reads-from-edges`, `--reads-from-outcome`, `--control-order`) 가운데 범위를 끄는 것은 없다.
     - `--clearing-writes counted|excluded`는 이전의 관계 읽기(`relate_legacy`)를 고른다. 그 읽기의 관계에는 TEARDOWN 제외가 없다. 그러나 범위 표시와 읽기 순서의 제외는 그대로다 [M: `base`를 `--clearing-writes counted`로 읽어도 `shutdown_request yes`, `scope teardown` 2].
     - 범위 없는 읽기는 두 지점이 없는 기록(이전 계획의 기록)이나 `scope` 블록을 뺀 프로필로만 된다. 후자는 프로필 변경이라 하지 않았다.

2. **TO_LAB 루프의 끝.** L520의 이벤트, 또는 `TO_LAB_UpdateSubscriptionsFromTable`(L458)의 진입·반환이 끝을 표시한다. 그 코드는 `to_lab.so`에 있다. F0 계획에 둘째 binary(`binary to_lab.so`, `kz anchor --bin`, 실행 때 `--basis` 하나 더)를, 계측 manifest에 module 하나를 더해야 한다. 캠페인 스크립트와 basis 만들기에 미치는 변경이라 하지 않았다. 지금은 TO_LAB 사건의 actor별 분포로 루프의 위치를 읽는다(§4.1).
3. **명령 경로가 아닌 재시작.** HS(`hs_monitors.c` L138·L290)나 코어 앱 오류(`cfe_es_api.c` L352)의 `CFE_ES_ResetCFE`, 그리고 하네스의 SIGINT 대체(`env/run_cfs.sh` L314)는 B2를 남기지 않는다. 그때는 `after-operational`로 남는다. 이 경로들도 경계로 볼지는 정하지 않았다.
4. **time 근거의 쓰임.** time으로 배정한 발생을 구간별 기대 판정에 넣을지, 따로 보고할지는 정하지 않았다(§7).
5. **콘솔 줄의 시각.** 콘솔이 비동기라 L233의 줄이 찍힌 때와 B1 점의 순서는 기록이 정하지 않는다(§1.1). 콘솔 기반의 시각(`results_w4w5` §2.1의 ms 값)과 B1 사건의 시각은 같은 것이 아니다.

## 7. 구간별로 필요한 기대 판정 행 (목록만; 문구는 lead·PI)

동결 표(`expected_results_C1_R2.md` E-01–E-15)와 근거표(`expected_judgement_basis_C1_R2.md`)는 고치지 않았다. 아래는 구간을 나누면 필요해지는 행과, 그 행의 근거가 될 출처다.

| 필요한 행 | 지금 | 출처 후보 | 비고 |
|---|---|---|---|
| R1·R2(AppID 0, ES·EVS)의 구간 | 근거표 R1·R2. 행은 있지만 구간을 적지 않음 | ES·EVS 초기화의 프로그램 순서(`cfe_es_task.c` L336–L467, `cfe_evs_task.c` L281–L319, 그 뒤 CORE_READY 대기 L134·L229), §3 sync | 출처상 모두 I0다. 기록에서 sync로 서는지는 실행 뒤에 본다 |
| R5(SB 자기 전송)의 구간 | 근거표 R5 | `cfe_sb_task.c` L138–L242, L93 | 같음 |
| R6·R7(다른 task, SB·TBL AppID) **I0만** | 근거표 R6·R7은 I0 근거만 있고, [기록]의 수는 나뉘지 않았다 | R-R2-g(근거표 §3.2), §3 sync·time | 수를 I0의 established와 time으로 나눠 적을 열이 필요하다 |
| **R8-I1** 다른 task 사용, OPERATIONAL 뒤·재시작 전(TO_LAB EID 10·22·25, 앱의 NO_SUBS EID 14, SBN EID 25 등) | 없음(R8 미정) | 요구: `cfe_evs.h` L168–L172(시점을 가리지 않음). SB id 셀의 writer: L50 memset·L138뿐(정적 닫힘 `kairos/tests/static/data/core_cpu1.golden` L161–L162). 등록 해제: `CFE_EVS_CleanUpApp`(`cfe_evs_task.c` L178–L198)뿐이고, `CFE_ES_CleanUpApp`(`cfe_es_apps.c` L1407, 요청 L1249)에서 module 표(`cfe_evs_objtab.c` L37)를 거쳐 닿는다. 기록에서는 `evs.cleanup.*` site로 그 id의 해제가 있었는지 본다. 보고 켜기: §1.4 | R2 이슈(S-R2-1)는 시작만 다룬다. 이 행의 기대는 코드(추론) 등급이 된다 |
| **R8-I2** 명령 재시작 뒤의 사용 | 없음 | PSP 종료 경로(§1.2 표), `OS_DeleteAllObjects`(`osal/src/os/shared/src/osapi-common.c` L357). 프로필 SCOPE(10_kinds.kzp L22–L29, SPEC 1.6, 모형 등급) | 두 LP를 관측하면 재생은 요청 뒤에 결정된 발생을 `scope teardown`으로 적는다(§6 항목 1). 다른 actor의 발생은 그 배정이 병합 순서(기록 시각)에만 기댄다. 출처(이슈)는 이 구간을 다루지 않는다 |
| 배정 근거 열 | 없음 | §3 | time으로 배정된 발생을 행에 넣을지(§6 항목 4) |
| N-R2-3(OPERATIONAL 뒤 사용을 정상 대조로) | "쓰지 않는다, 구간 분리 전" | R8-I1의 행 | 행이 생긴 뒤에만 |
| E-15(동결) | I0·I1·I2를 함께 묶는다 | — | 동결 표의 개정 절차(그 문서 §5, P-F1GATE)를 따른다. [기록] 실행들은 이미 결과를 본 것이다 |
