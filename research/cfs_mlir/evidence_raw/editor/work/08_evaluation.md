## 8. 평가 계획

**[설계 제안]** 이 절의 모든 benchmark, 지표, 기준선, ablation은 계획이다. 분석기 구현, 검출률, 분석 시간 결과는 아직 없다.

**[설계 제안]** 평가는 세 층으로 구성한다. (a) 직접 작성한 최소 합성 앱, (b) 공개 cFS 앱에 넣은 결함, (c) 공개 기록이 있는 역사적 결함이다. 원노트 §37의 세 층 구조를 유지하고, 각 층에 무엇을 고정하고 어떻게 확인할지를 구체화한다.

이 절에서 인용하는 실측값은 이 컨테이너에서 이미 수행한 probe 실행 결과다. 모두 native Linux 빌드의 결과이며 flight RTOS의 동작을 대신하지 않는다.

### 8.1 평가 단위와 증거 수준

**[설계 제안]** 평가 단위는 경고가 아니라 결함 사례(fault instance)다. 하나의 결함은 다음 다섯 요소로 식별한다. 범주, 필요한 순서 쌍 `(E1, E2)`, 영향받는 상태 필드 또는 자원, consumer 쪽 source 위치, producer 쪽 MID 또는 서비스 API.

**[원노트 구상]** 원노트 §39의 ground truth 항목(Fault ID, 관련 앱, 필요한 순서, 위반 순서, 기대 결과, 실행 시 증상)을 기본으로 한다. 수정본 §12.1의 요구도 반영한다. source revision, 정상 동작의 근거, 주입 내용, 재현 여부를 함께 기록해야 한다.

**[설계 제안]** 결과마다 증거 수준을 붙인다. 수준이 다른 결과는 합산하지 않는다.

| 수준 | 의미 | 필요한 자료 |
| --- | --- | --- |
| L0 후보 | 분석기가 경고를 냈다 | 경고 위치와 설명 |
| L1 모델상 가능 | 선언한 플랫폼 모델(SB 의미, scheduling 가정) 안에서 위반 순서가 존재한다 | 분석기 witness: 이벤트 순서와 각 이벤트의 source 위치 |
| L2 재현 | native 빌드에서 위반 순서와 관측 가능한 잘못된 상태·동작을 확인했다 | 실행 환경 기록, 실행 명령, 관측 로그, 반복 횟수 |

**[해석]** 이 구분이 필요한 이유는 실측 결과가 scheduling 조건에 크게 의존하기 때문이다(§8.7). L1을 L2로 보고하면 수정본 §10.2의 경고, 즉 "HB를 증명하지 못한 것과 race는 다르다"를 어기게 된다.

### 8.2 결함 범주와 정답 판정의 근거

**[원노트 구상]** 원노트 §38은 F1–F6을 정의했다. **[설계 제안]** 평가 범주는 §1.2의 F1–F8을 따른다. F7은 원노트 §14의 메시지 간 도착 순서다. F8은 재시작 창(restart window)과 앱 간 table handle 문제를 분리한 범주로, 원노트 §24-D("cross-application stale handle")를 평가 범주로 옮긴 것이다.

| 범주 | 필요한 순서 | cFS에서 이 범주를 의미 있게 만드는 근거 |
| --- | --- | --- |
| F1 첫 메시지·구독 준비 | `Subscribe(B)` 완료 → 필수 `Publish(A)` | **[확인된 사실]** 구독자가 없는 MID로 보내도 `CFE_SB_TransmitMsg`는 `CFE_SUCCESS`를 반환한다. route가 한 번도 없으면 `NoSubscribersCounter`만 증가한다([cfe_sb_priv.c L1091–1097](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_priv.c#L1091-L1097), S12). **[확인된 사실]** SB에는 durability·latching이 없다. `CFE_SB_Qos_t`는 "currently unused"로 문서화되어 있다([default_cfe_sb_extern_typedefs.h L114–125](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/config/default_cfe_sb_extern_typedefs.h#L114-L125)). |
| F2 메시지 유래 상태의 초기화 전 사용 | 첫 성공 수신과 field 갱신 → 상태 사용 | **[원노트 구상]** 원노트 §32·§44. **[해석]** 수정본 §6.2가 지적했듯이, 0으로 초기화된 메모리와 "유효한 입력을 받은 상태"를 구별해야 한다. |
| F3 오래된 상태 | 마지막 갱신 후 허용 경계 안에서만 사용 | **[확인된 사실]** LC는 watchpoint 결과의 나이를 "Sample AP 명령 처리 횟수"로 센다(`CountdownToStale`, [lc_cmds.c L91–103](https://github.com/nasa/LC/blob/32c76661dc339247d1c3b9250ed27dd18c34a344/fsw/src/lc_cmds.c#L91-L103)). 시간 단위가 아니다. |
| F4 여러 stream의 snapshot 불일치 | 함께 쓰는 입력이 같은 epoch | **[확인된 사실]** HK는 입력마다 `DataPresent` flag를 두고, 기본 설정(`HK_DISCARD_INCOMPLETE_COMBO` 0)에서는 불완전한 combined packet도 보낸다([hk_utils.c L483–504](https://github.com/nasa/HK/blob/0dc16b7a7bab71747b9c063cf405ad59a65a40a5/fsw/src/hk_utils.c#L483-L504), [hk_internal_cfg.h L57–69](https://github.com/nasa/HK/blob/0dc16b7a7bab71747b9c063cf405ad59a65a40a5/fsw/inc/hk_internal_cfg.h#L57-L69)). **[해석]** 이것은 race가 아니라 설정된 정책이다. |
| F5 시작 시 서비스 의존 | provider 초기화 완료 → consumer 사용 | **[확인된 사실]** cFE #73 [S01], CF #184(833fdbb [S44]). |
| F6 Table 생명주기 오용 | GetAddress 성공 → 사용 → Release, 그리고 Manage | **[확인된 사실]** 546a002에서 `NEVER_LOADED`는 NULL 포인터를 돌려주고 lock을 걸지 않는다([cfe_tbl_registry.c L192–196](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/tbl/fsw/src/cfe_tbl_registry.c#L192-L196)). header [S10]의 "0 내용의 유효한 포인터" 설명과 다르다. |
| F7 메시지 간 도착 순서 | 소비자에서 `rcv(m1)` → `rcv(m2)`, 또는 인과 순서의 보존 | **[확인된 사실]** 목적지별 전달 순서, 송신 반환 전 수신, cFE 버전에 따른 lock 위치(550e7f7d)는 §1.2 F7과 §3.1·§3.6에 있다. |
| F8 재시작·lifecycle | 재시작한 앱의 재구독·재등록 → 다른 앱의 의존 사용 | **[확인된 사실]** `CFE_SB_CleanUpApp`은 앱 소유 pipe를 모든 route에서 제거하고 queue에 남은 메시지를 버린다. route와 sequence counter는 남는다([cfe_sb_priv.c L87–124](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_priv.c#L87-L124), [cfe_sb_api.c L352–511](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_api.c#L352-L511)). |

**[설계 제안]** 각 사례의 "필요한 순서"에는 출처를 하나 붙인다. 허용하는 출처는 네 가지다.

1. 고정 버전의 API 문서·구현(S08–S12).
2. 앱 자체의 주석·설정(예: TO_LAB의 구독 지연 주석, HK·LC의 freshness 정책).
3. 공개 이슈의 원인 설명.
4. 분석기를 실행하기 전에 별도 작성자가 동결한 contract.

**[해석]** 출처가 없는 사례에는 true/false label을 붙이지 않는다. 수정본 §12.1과 같은 원칙이다.

### 8.3 층 (a): 최소 합성 앱

#### 8.3.1 앱 구성

**[원노트 구상]** 원노트 §63 Step 4는 SENSOR, NAVIGATION, CONTROL 세 앱을 제안했다. **[설계 제안]** 이를 다음과 같이 구체화한다. 이 앱들은 cFS bundle(5a9b075 [S87])의 `apps/`에 추가하고 같은 빌드로 컴파일한다. 그래야 실제 compile DB, 테이블, startup script가 분석 입력에 그대로 들어간다.

| 앱 | 발행 | 구독 | 상태 필드 | 주기 구동 |
| --- | --- | --- | --- | --- |
| `SENSOR` | `SENSOR_ATT_MID`(attitude, `Seq`, `MeasTime`), `SENSOR_POS_MID`(position, `Seq`, `MeasTime`) | `SENSOR_WAKEUP_MID` | 없음 | SCH_LAB 테이블 항목 |
| `NAV` | `NAV_INIT_MID`(한 번만 발행하는 초기 설정), `NAV_STATE_MID`(att+pos, `Epoch`) | `SENSOR_ATT_MID`, `SENSOR_POS_MID`, `NAV_INIT_REQ_MID` | `NAV_Data.Att`, `NAV_Data.Pos`, `NAV_Data.AttEpoch`, `NAV_Data.PosEpoch` | `NAV_WAKEUP_MID` |
| `CONTROL` | `CONTROL_HK_TLM_MID`(시험용 관측 counter 포함) | `NAV_STATE_MID`, `NAV_INIT_MID`, `SENSOR_*`, `CONTROL_WAKEUP_MID` | `CONTROL_Data.Att`, `CONTROL_Data.AttValid`, `CONTROL_Data.Cfg`, `CONTROL_Data.CfgValid` | `CONTROL_WAKEUP_MID` |
| `GAINTBL` | 없음 | 명령 MID | `GAIN` table의 owner (단일·이중 buffer 두 가지) | 없음 |

**[설계 제안]** SCH_LAB은 bundle에 있는 그대로 쓰고, 세 앱의 wakeup 항목만 테이블에 추가한다. **[확인된 사실]** SCH_LAB은 1 Hz 메시지를 한 번이라도 받은 뒤에만 schedule 전송을 시작한다(`SCH_OneHzPktsRcvd > 0`, [sch_lab_app.c L116](https://github.com/nasa/sch_lab/blob/607e2f90c5f828f3f3ae655a4debacf8b2cc4bbb/fsw/src/sch_lab_app.c#L116)). 이 guard 자체도 benchmark의 정상 사례가 된다.

**[설계 제안]** 시험용 관측은 각 앱 HK telemetry의 전용 counter로 한다. 예를 들면 `UsedInvalidCtr`, `EpochMismatchCtr`, `StaleUseCtr`, `CfgMissingCtr`이다. console 로그는 쓰지 않는다. **[실측]** EVS filter 때문에 console에 찍히는 이벤트 수는 filter 한도에서 멈춘다. 실제 발생 횟수는 알 수 없다. native 실행 8회 모두 `No subscribers for MsgId 0x808`가 정확히 4번 나왔고, 이는 `CFE_EVS_FIRST_4_STOP` 한도와 같다(`$N/probe/run_nobody.log`, `$N/probe/runs/*.log`; [cfe_sb_internal_cfg.h L221–243](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/inc/cfe_sb_internal_cfg.h#L221-L243)). **[확인된 사실]** 게다가 SB 이벤트는 발행 앱이 EVS에 등록되어 있고 해당 이벤트 type이 켜져 있어야 출력된다(`EVS_IsFiltered`가 호출 task의 `CFE_EVS_GetTypeEnable`을 먼저 본다, [cfe_evs_utils.c L190–240](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/evs/fsw/src/cfe_evs_utils.c#L190-L240)).

#### 8.3.2 변형 규칙

**[설계 제안]** 모든 사례는 한 source tree에서 `-DVARIANT=<ID>` compile flag로 변형을 만든다. 변형 사이의 diff는 사례 기록에 그대로 저장한다. 모든 사례에 `faulty`와 `correct`를 두고, 해당하는 경우 `benign`과 `conditional`을 더한다.

| 변형 | 의미 | 기대 판정 |
| --- | --- | --- |
| `faulty` | 필요한 순서가 보장되지 않고, 위반 순서에서 관측 가능한 잘못된 상태가 생긴다 | 경고 |
| `correct` | 같은 기능을 순서 보장(guard, 요청–응답, 공동 갱신 등)으로 구현한다 | 경고 없음 |
| `benign` | 순서 위반은 가능하지만 앱 정책상 허용된다(contract로 명시) | 경고 없음, 또는 "허용됨"으로 표시 |
| `conditional` | 플랫폼 가정(예: ES timeout 안에 모든 앱이 RUNNING)이 성립할 때만 올바르다 | 가정을 명시한 조건부 경고 |

**[해석]** `benign`과 `conditional`이 없으면 benchmark는 "경고를 많이 낼수록 유리한" 구조가 된다. 오경고율을 측정하려면 이 두 변형이 꼭 필요하다.

#### 8.3.3 범주별 쌍

**[설계 제안]** 범주별 사례는 다음과 같다. 표의 앱·함수 이름은 모두 아직 작성하지 않은 합성 앱의 설계다.

| ID | `faulty` | `correct` | `benign` / `conditional` | 위반 순서를 일으키는 방법 |
| --- | --- | --- | --- | --- |
| SYN-F1-01 | `NAV`가 init 끝에 `NAV_INIT_MID`를 한 번만 발행하고, `CONTROL`은 `CfgValid` 없이 `Cfg`를 사용한다 | `CONTROL`이 구독 완료 후 `NAV_INIT_REQ_MID`를 보내고, `NAV`가 응답한다 | `benign`: `Cfg`가 선택 항목이고 기본값 사용이 contract에 명시됨. `conditional`: 두 앱 모두 `CFE_ES_WaitForStartupSync` 이후 발행·구독 | `CONTROL` init에서 `OS_TaskDelay` 후 구독; startup script 순서 |
| SYN-F2-01 | `ControlStep`이 첫 `NAV_STATE` 수신 전에 `Att`를 사용한다 | handler에서 검증 후 `AttValid = true`; `ControlStep`은 `!AttValid`이면 반환 | `benign`: 기본 attitude 사용이 contract에 명시됨 | `SENSOR`/`NAV` 시작 지연, `CONTROL_WAKEUP`이 먼저 도착 |
| SYN-F2-02 | 원노트 §44 형태: `HandleAttitude → SetState → global.att → Guidance`, guard 없음 | 같은 구조에서 guard를 다른 함수(`IsAttReady()`)에 둔다 | — | 위와 같음 |
| SYN-F3-01 | `SENSOR`가 멈춘 뒤에도 `CONTROL`이 마지막 값을 계속 사용한다 | LC 방식: 갱신 때 countdown을 리셋하고 tick마다 감소, 0이면 사용 중단 | `benign`: hold-last-value 허용이 contract에 명시됨 | `CFE_ES_RestartApp(SENSOR)`, SB route disable 명령 |
| SYN-F4-01 | `CONTROL`이 `SENSOR_ATT_MID`와 `SENSOR_POS_MID`를 따로 저장하고 epoch 검사 없이 함께 사용한다 | `NAV_STATE_MID` 한 packet으로 공동 갱신, 또는 `Att.Seq == Pos.Seq` 검사 | `benign`: latest-value 조합 허용이 contract에 명시됨 | 한 stream에 MsgLim 초과 burst; relay priority 배치(§8.7) |
| SYN-F4-02 | — | 수정본 §6.4의 예: 한 분기는 `(k,k)`, 다른 분기는 `(j,j)`만 만든다 | 경로를 구별하지 않는 분석의 가짜 `(k,j)` 조합을 잡는 함정 사례 | — |
| SYN-F5-01 | `NAV`가 init 안에서 counting semaphore `NAV_SEM`을 만들고, `CONTROL` init은 `OS_CountSemGetIdByName`을 한 번 호출해 실패하면 종료한다(CF #184 형태) | 자원 생성을 startup script에서 두 앱보다 앞에 있는 `CFE_LIB`의 init으로 옮긴다. **[확인된 사실]** library init은 ES main context에서 동기적으로 실행되므로, 뒤에 나오는 앱보다 먼저 끝난다(§3.3 ES-3, [cfe_es_apps.c L681–684](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_apps.c#L681-L684)) | `conditional` 1: `NAV`가 자원 생성 후 `WaitForSystemState(APPS_INIT)`, `CONTROL`도 같은 상태를 기다린 뒤 bind. **[확인된 사실]** ES는 1000 ms 기본 timeout 뒤에도 APPS_INIT으로 진행하므로 timeout 안에 `NAV`가 LATE_INIT에 도달한다는 가정이 필요하다(§8.3.4). `conditional` 2: CF 수정판처럼 제한 횟수 재시도(25회×100 ms) | provider 앞에 consumer를 둔 startup script, provider init 지연 |
| SYN-F6-01 | `GetAddress`가 `CFE_TBL_INFO_UPDATED`를 반환하면 release 없이 return(sample_app #28→#101 형태) | `Status < CFE_SUCCESS`일 때만 실패 처리하고 항상 release | — | 테이블 load 후 명령 처리, 이어서 재load |
| SYN-F6-02 | `NEVER_LOADED`에서도 포인터를 역참조(header를 믿는 코드) | `NEVER_LOADED`이면 사용하지 않음 | — | 테이블을 load하지 않고 시작 |
| SYN-F6-03 | 이중 buffer table에서 release 없이 `GetAddress`를 두 번 호출하고 첫 포인터를 계속 사용 | 사용할 때마다 획득·해제 | — | owner의 연속 load |
| SYN-F8-01 | `GAINTBL` 재시작 중 sharer `CONTROL`이 `CFE_TBL_ERR_UNREGISTERED`를 처리하지 않는다 | `UNREGISTERED`이면 `CFE_TBL_Unregister` 후 재share | — | `CFE_ES_RestartApp(GAINTBL)` |
| SYN-F8-02 | `CONTROL` 재시작 창 동안 `NAV`가 한 번만 보내는 메시지가 사라진다 | `NAV`가 주기 재발행, 또는 `CONTROL`이 재시작 후 요청 | — | `CFE_ES_RestartApp(CONTROL)` 직후 발행 |

**[설계 제안]** 이렇게 하면 범주당 2–4개 사례, 변형까지 합쳐 약 40개의 빌드 단위가 생긴다. **[해석]** 이 숫자는 통계적 일반화를 위한 것이 아니다. 각 의미 규칙이 정상·오류·허용 사례를 구별하는지 확인하는 회귀 시험의 크기다.

#### 8.3.4 cFS 의미 모델 적합성 시험

**[설계 제안]** 분석기의 SB·ES·TBL 모델이 실제 구현과 맞는지 먼저 확인한다. 아래 사례는 결함 검출 benchmark가 아니라 모델 시험이다. 기대값은 이미 측정한 실행 결과에서 가져온다.

| 시험 | 모델이 내야 할 결론 | 기준이 되는 실측·source |
| --- | --- | --- |
| MsgLim 초과 | 같은 MID를 MsgLim보다 많이 보내면 해당 pipe에서만 손실, 반환값은 성공 | **[실측]** MsgLim=2, 5회 송신: 반환값 모두 0, 수신 2, `MsgLimitErrorCounter` +3(`$N/sem-sb/probe_runs/SBPROBE_lines_all_runs.txt` T3) |
| pipe 가득 참 | depth 초과분 손실, 반환값 성공 | **[실측]** depth 3, 5회 송신: 수신 3, `PipeOverflowErrorCounter` +2(같은 파일 T4) |
| 구독 해제 후 송신 | 이벤트·counter 없이 성공, sequence counter는 증가 | **[실측]** 구독 해제한 MID: `NoSubDelta` 0, seq 1→5(같은 파일 T1, T2) |
| 수신 순서 | 같은 pipe 안에서는 FIFO, pipe 사이에는 순서 없음 | **[확인된 사실]** POSIX OSAL은 priority 1 고정의 `mq_timedsend`를 쓴다([os-impl-queues.c L285–323](https://github.com/nasa/osal/blob/dad0ee99c121f2ecc11f8c0798c1e836e41d8ab1/src/os/posix/src/os-impl-queues.c#L285-L323)) |
| 발행 중 선점 | 높은 priority 수신자가 `TransmitMsg` 반환 전에 실행될 수 있음 | **[실측]** 단일 CPU, SCHED_RR: 200/200회 반환 전 실행, 역 구독 순서(T5) |
| OPERATIONAL 의미 | OPERATIONAL은 모든 앱의 RUNNING을 뜻하지 않음 | **[실측]** 2.5 s 지연 앱이 있을 때 ES가 "Startup Sync failed"를 두 번 기록하고 OPERATIONAL에 들어감. 다른 앱의 `WaitForSystemState(OPERATIONAL,5000)`은 약 1.95–2.0 s 후 성공(`$N/sem-es-tbl/runs/probe_results_summary.txt` L66–80) |
| `NEVER_LOADED` | 포인터 NULL, lock 없음 | **[실측]** `GetAddress(T) BEFORE load st=0xcc000005 ptr=(nil)`(같은 파일 L50–51) |
| 한 handle의 lock | lock은 handle 단위; 재획득하면 보호 대상이 바뀜 | **[실측]** release하지 않은 첫 포인터가 owner의 `Load(v3)` 후 3을 읽음(같은 파일 L81–92) |
| 단일·이중 buffer | 단일: `INFO_TABLE_LOCKED` 후 보류; 이중: load 진행, 다음 load는 `NO_BUFFER_AVAIL` | **[실측]** `0x4c000018`, `0x4c000004`→`0x4c00000e`; `0xcc00000f`(같은 파일 L61–64, L83–96) |
| owner 재시작 | sharer가 남아 있으면 새 owner의 `Register`가 `DUPLICATE_NOT_OWNED` | **[실측]** 4개 설정 모두 재현, 재시작 약 1.57–1.63 s, AppId 1114121→1114123(같은 파일 L97–178) |

**[해석]** 모델이 이 시험을 통과하지 못하면 이후 benchmark 결과는 해석할 수 없다. 따라서 이 표를 benchmark보다 먼저 실행한다.

### 8.4 층 (b): 공개 앱 결함 주입

#### 8.4.1 고정 대상

**[실측]** 대상 bundle과 submodule은 다음 commit에 고정되어 있다(`$N/probe/submodules.txt`). cFS bundle `5a9b075c`, cFE `546a0025`, OSAL `dad0ee99`, PSP `53df1d54`, sample_app `199476a3`, HK `0dc16b7a`, LC `32c76661`, sch_lab `607e2f90`, to_lab `d27c6014`, CF `a9c36d63`, MD `65eb7b3b`, HS `e76150e6`.

**[확인된 사실]** 기본 bundle 설정은 HK의 combined packet 요청과 LC의 actionpoint sample 요청을 schedule하지 않는다. LC에는 watchpoint도 정의되어 있지 않다.

- bundle의 SCH_LAB 테이블에는 `LC_SAMPLE_AP_MID`도 `HK_SEND_COMBINED_PKT_MID`도 없다([sample_defs/tables/sch_lab_table.c L55–82](https://github.com/nasa/cFS/blob/5a9b075cd4c818ee8555f349a9f78ba5632d0384/sample_defs/tables/sch_lab_table.c#L55-L82); `sample_defs`에 대한 `grep -rn` 결과 없음).
- 빌드에 쓰이는 LC watchpoint 테이블은 앱 자체의 `lc_def_wdt.c`다(`build-native_std/.../tblobj_cpu1_lc.*/DependInfo.cmake`). 그 항목은 모두 `LC_DATA_WATCH_NOT_USED`다(lc_def_wdt.c [S92]).

**[해석]** 따라서 기본 실행에서는 HK의 missing-data 검사(`HK_SendCombinedHkPacket`)와 LC의 stale 판정 경로가 지상 명령 없이는 동작하지 않을 것이다. 주입 실험에는 harness 테이블이 따로 필요하다.

**[설계 제안]** harness는 (1) SCH 항목 추가, (2) LC의 WDT·ADT 정의, (3) HK copy table의 입력 MID 선택을 데이터 파일로 제공한다. 이 데이터 파일도 분석 입력이며 정답 기록에 commit과 함께 저장한다.

#### 8.4.2 주입 목록

**[설계 제안]** 주입 연산과 대조군은 다음과 같다. 주입 하나는 한 위치만 바꾸고, 원본 함수를 같은 harness에서 정상 대조군으로 함께 실행한다.

| ID | 앱·함수(고정 commit) | 주입 연산 | 범주 | 기대 증상 | 같은 위치의 정상 대조군 |
| --- | --- | --- | --- | --- | --- |
| INJ-SA-1 | sample_app `SAMPLE_APP_ProcessCmd` ([sample_app_cmds.c L114–140](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app_cmds.c#L114-L140), S21) | `Status < CFE_SUCCESS`를 `Status != CFE_SUCCESS`로 바꾸고 그 분기에서 release 없이 return | F6 | **[해석]** 테이블이 `CFE_TBL_OPT_DEFAULT`(단일 buffer)로 등록되므로([sample_app.c L188–202](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app.c#L188-L202)), 남은 lock 때문에 다음 load가 `INFO_TABLE_LOCKED`로 보류될 것으로 예상한다 | 원본 함수 |
| INJ-SA-2 | 같은 함수 | `NEVER_LOADED`도 성공으로 보고 포인터를 사용 | F6 | **[해석]** 546a002 의미에서는 `NEVER_LOADED`의 포인터가 NULL이므로 NULL 역참조가 예상된다 | 원본 함수 |
| INJ-HK-1 | HK `HK_CheckStatusOfCopyTable` ([hk_utils.c L545–640](https://github.com/nasa/HK/blob/0dc16b7a7bab71747b9c063cf405ad59a65a40a5/fsw/src/hk_utils.c#L545-L640)) | `HK_ProcessNewCopyTable`(재구독)을 `CFE_TBL_Update` 앞으로 이동 | F6/F8 | **[해석]** 새 테이블이 아니라 이전 테이블의 MID로 재구독할 것으로 예상한다 | 원본의 teardown → release → update → 재획득 → 재구독 순서 |
| INJ-HK-2 | HK `HK_SendCombinedHkPacket` ([hk_utils.c L460–504](https://github.com/nasa/HK/blob/0dc16b7a7bab71747b9c063cf405ad59a65a40a5/fsw/src/hk_utils.c#L460-L504)) | `HK_CheckForMissingData` 호출과 `MissingDataCtr` 증가를 제거 | F4 | **[해석]** 불완전 packet이 counter 표시 없이 전송될 것으로 예상한다 | 원본. **[확인된 사실]** 기본 설정은 불완전 packet을 보내되 counter로 표시한다. missing-data 이벤트는 DEBUG type이라 기본 EVS mask(0xE)에서 출력되지 않는다(검증 C16) |
| INJ-LC-1 | LC `LC_ProcessWP` ([lc_watch.c L358–460](https://github.com/nasa/LC/blob/32c76661dc339247d1c3b9250ed27dd18c34a344/fsw/src/lc_watch.c#L358-L460)) | TRUE/FALSE 결과에서 `CountdownToStale` 리셋을 제거 | F3 | **[해석]** countdown이 0으로 남아 `LC_SampleAPReq`의 감소 블록(`> 0` 조건)이 동작하지 않으므로, 오래된 결과가 STALE로 바뀌지 않을 것으로 예상한다 | 원본 함수 |
| INJ-LC-2 | LC `LC_EvaluateRPN` ([lc_action.c L297–475](https://github.com/nasa/LC/blob/32c76661dc339247d1c3b9250ed27dd18c34a344/fsw/src/lc_action.c#L297-L475)) | `LC_WATCH_STALE` 피연산자를 FALSE로 취급 | F3 | **[해석]** stale 입력으로 actionpoint가 FALSE로 판정될 것으로 예상한다 | 원본. **[확인된 사실]** LC #8(open)이 STALE→FALSE 전이의 동작을 다룬다(LC #8 [S53]) |
| INJ-SCH-1 | sch_lab `SCH_LAB_AppMain` ([sch_lab_app.c L93–137](https://github.com/nasa/sch_lab/blob/607e2f90c5f828f3f3ae655a4debacf8b2cc4bbb/fsw/src/sch_lab_app.c#L93-L137)) | `SCH_OneHzPktsRcvd > 0` 조건 제거 | F5 후보 | **[미확인]** 결과가 결함인지 사전에 알 수 없다 | 원본. **[해석]** 주입했다고 결함이 되지는 않는다. 관측으로 label을 정하는 대조 사례다 |
| INJ-TO-1 | to_lab `TO_LAB_AppMain` ([to_lab_app.c L66–73](https://github.com/nasa/to_lab/blob/d27c6014cb5d2979140500f635876ee87b81995b/fsw/src/to_lab_app.c#L66-L73)) | d3d52da [S46] 이전처럼 테이블 구독을 `TO_LAB_init` 안으로 이동 | F1(MsgLim) | **[확인된 사실]** d3d52da의 목적은 시작 시 이벤트 burst로 생기는 MsgLimit 오류를 피하는 것이었다. **[해석]** 되돌리면 그 오류가 다시 나타날 것으로 예상한다 | 현재 버전: 의도적으로 첫 메시지를 잃는 설계(`benign`). 지연은 테이블 기반 telemetry 구독에만 적용되고 명령 MID 구독은 init에서 한다(검증 C16) |
| INJ-OG-1 | Ogma 생성 cFS monitor app(R09, S24) | 서로 다른 MID의 입력 둘 이상을 active로 선택해 생성 | F4 대상 | **[해석]** monitor가 따로 갱신된 global의 최신값을 함께 평가할 것으로 예상한다 | **[확인된 사실]** 예제 `cfs-002-state-machines`의 passive 입력(`SAMPLE_MID`)은 저장만 되고 active 입력(`STATE_MID`)이 올 때 평가된다 |

**[확인된 사실]** INJ-OG-1의 template 동작에는 다음 보정이 필요하다(검증 C06: contested). 아래 code 사실은 세 검증 모두에서 일치했다. 생성 앱은 DB의 모든 MID가 아니라 선택한 변수의 MID와 `COPILOT_CFS_REEVAL_CMD_MID`만 구독한다. `copilot_step()`은 입력이 `active`일 때만 호출된다. template의 어느 파일도 `CFE_ES_WaitForStartupSync`를 호출하지 않는다. README 예제는 `position` 하나만 선택하므로 생성 앱의 데이터 stream도 하나다([copilot_cfs.c L174–239](https://github.com/nasa/ogma/blob/08b384b4335cce324fac74577adc7f08e61b7cce/ogma-core/templates/cfs/copilot/fsw/src/copilot_cfs.c#L174-L239), CFSApp.hs [S93]). **[미확인]** Ogma를 실제로 실행해 앱을 생성하지 않았다.

#### 8.4.3 수정하지 않은 bundle 전체 실행

**[설계 제안]** 주입 전에 수정하지 않은 bundle 전체를 분석한다. 이 결과에는 정답 label이 없다. 경고마다 L0/L1/L2와 함께 다음 중 하나로 분류한다: "재현됨", "모델상 가능", "설계상 허용", "불가능(오경고)", "판단 불가".

**[해석]** 이 분포는 합성·주입 사례의 precision보다 실제 사용 환경의 오경고 부담에 가깝다. 다만 정답 집합이 없으므로 recall은 계산하지 않는다.

**[실측]** 이미 관측된 두 시작 시 현상은 이 분류의 첫 시험 대상이다. 하나는 8회 실행 모두에서 OPERATIONAL 이전에 나온 `MsgId 0x808`(`CFE_EVS_LONG_EVENT_MSG_MID`)의 no-subscriber event다. 다른 하나는 TO_LAB의 37개 table 구독이 낸 구독 보고(`0x80E`)가 SBN pipe에서 16건 이상 손실된 것이다. 로그 위치, event를 낸 주체, CORE_READY 전의 세 줄, 추가 구독자(DS, HS)는 §7.4.3에 있다. **[미확인]** SBN 손실의 기능적 영향은 확인하지 않았다. 둘 다 결함으로 label하지 않는다.

### 8.5 층 (c): 역사적 결함 재현 후보

**[설계 제안]** 역사적 사례는 수정 전후 code가 공개된 경우에만 "재현 대상"으로 둔다. 그렇지 않으면 "파생 예제"로 따로 표시한다(수정본 §5.3).

| 후보 | 공개 자료 | 재현에 필요한 것 | 계획한 사용 |
| --- | --- | --- | --- |
| cFE #73·#71 (S01, S02) | **[확인된 사실]** 수정 전 6.4.1(SourceForge, sha256 `ec26e33b…`)과 수정 후 6.4.2(`c64ed2aa…`) tarball이 공개되어 있다. 6.4.2는 core app마다 `CFE_ES_ApplicationSyncDelay`와 panic을 추가하고, `EVS_SendEvent`에 `EVS_AppID` 범위 검사를 넣었다. "일관된 AppID 초기화"는 구현되지 않았다(검증 C21의 정정 문구; `verify_C21/`). GitHub history는 6.5.0a 이식부터 시작한다 | 6.4.1과 같은 시기의 OSAL·PSP를 현재 gcc로 빌드할 수 있는지(**[미확인]**), priority 61/64/68/60/70 설정, priority 순서를 강제하는 실행 환경 | F5 historical. Microblaze/GRC 환경은 없으므로 source-level 수정 전후 대조로 한정 |
| cFE #198 | **[확인된 사실]** 6.5.0a(792f5e35 [S101])에는 `APPS_INIT`/`LATE_INIT`이 없고 6.6.0a(2661d19f)에는 있다. EVA CWS 앱은 비공개다 | 두 cFE 버전 위의 2앱 파생 예제 | F5 파생 예제. "#198에서 파생"으로 표시 |
| CF #184 | **[확인된 사실]** 수정 833fdbb [S44]와 그 parent가 공개되어 있다. 수정은 25회×100 ms 제한 재시도다 | semaphore를 만드는 합성 앱, startup 순서 제어 | F5 historical. 수정판은 `conditional`(재시도 한도와 owner 재시작을 처리하지 않음) |
| sample_app #28→#101 | **[확인된 사실]** 693d75f [S59]가 `INFO_UPDATED` 경로의 release 누락을 만들었고 61f657d [S60]가 고쳤다 | 같은 시기의 cFE(v6.7/6.8)와 table load 명령 | F6 historical. 단일 앱 API 오용이므로 앱 간 race로 세지 않음 |
| HS #148 | **[확인된 사실]** 수정 b7530d9 [S51]; 재현법은 `hs_amt.tbl` 삭제 후 enable 명령 | parent commit 빌드 | F2(table 유래 상태). 순서 race가 아니라 결정적 결함 |
| MD #79 | **[확인된 사실]** HEAD 65eb7b3에서 file load 경로가 `MD_CopyUpdatedTbl`을 호출하지 않는다([md_app.c L423–449](https://github.com/nasa/MD/blob/65eb7b3b0aa8acd05076128a623cd696582b6d7c/fsw/src/md_app.c#L423-L449)) | 현재 bundle 그대로 | F2 현재 결함(open). 결정적 |
| cFE #950 | **[확인된 사실]** 수정 6932f1f [S69]; 이슈에 수정한 sample_app 재현법이 있다 | parent commit 빌드 | F8 대조 사례. lock 수준 문제라 TSan·Goblint 비교용 |
| cFE #2663 | **[확인된 사실]** open 이슈. 검증 실행 한 번에서 gdb로 재현했다: ES·EVS init 중 `CFE_EVS_APP_ILLEGAL_APP_ID` 반환 24회, 모두 AppID 0(`verify-C12/counter_lens/gdb_attach.log`) | 현재 bundle, gdb | 반환값 미검사 규칙의 현재 사례. 증상은 무해 |
| TBL owner 재시작 | **[실측]** §8.3.4의 `DUPLICATE_NOT_OWNED` 재현 | 현재 bundle | F8 현재 사례. **[확인된 사실]** 한 검증자의 source 확인에 따르면 v7.0.0 이전 버전은 정리 시 table 이름을 지웠다(검증 C15, 실행 확인 안 함). 버전 범위를 기록한다 |
| cFE #1750 | **[확인된 사실]** open: 잠긴 table에 load하면 `LoadInProgress`가 남는다. 기존 functional test가 이를 보여 준다 | 546a002에서 현존 여부 확인(**[미확인]**) | F6 후보 |
| cFE #46·#1544 | **[확인된 사실]** TIME reference의 여러 field 읽기. 6.5.0a, 6.6.0a, 546a002의 code가 모두 공개되어 있다 | 각 버전 빌드 | guard 인식의 정상 사례(versioned read). 공유 메모리 성격이라 TSan과 비교 |
| cFE #1073 | **[확인된 사실]** 장시간 실행 후 구독하지 않은 MID가 도착; 수정 b17cd1e | 수 시간 실행 | 재현 대상에서 제외. 대조 설명에만 사용 |
| Swift/BAT (S03–S06) | **[확인된 사실]** Circular는 원인 code를 공개하지 않는다 | 해당 없음 | benchmark에서 제외. **[해석]** BAT가 cFS 배치라는 근거는 없다. GSFC 자료가 BAT를 cFE heritage로 나열한다는 보고가 있다(검증 C22: contested) |

**[해석]** 이 목록에서 "앱 간 메시지 순서" 범주(F1, F4)의 공개 실패 보고는 찾지 못했다(issues 조사, `$N/issues/candidates.json`). 이 범주의 양성 사례는 합성·주입 사례로 채워야 한다. 이 공백은 검색 범위 안의 결과이며 부재의 증명이 아니다.

### 8.6 정답 기록 스키마

**[설계 제안]** 사례마다 YAML 기록 하나를 둔다. 기록은 분석기를 실행하기 전에 동결한다.

```yaml
id: SYN-F2-01-faulty
layer: synthetic              # synthetic | injected | historical | unmodified
category: F2                  # F1..F8
provenance: {kind: synthetic, derived_from: null}   # historical이면 이슈 URL·수정 commit
pins:
  cfs_bundle: 5a9b075cd4c818ee8555f349a9f78ba5632d0384
  cfe: 546a002515be5a1e3b66f9ae2c14f948d9cec76f
  osal: dad0ee99c121f2ecc11f8c0798c1e836e41d8ab1
  psp: 53df1d54c3d29fd23832c59df1104de07a043af2
  apps: {sensor: <sha>, nav: <sha>, control: <sha>}
build:
  config: native_std          # EDS 사용 여부를 반드시 기록
  eds: false
  analysis_cc: "clang 23.0.0git 1053047a"
  flag_delta: "-Wno-unknown-warning-option -Wno-error -O1 -Xclang -disable-llvm-passes"
inputs:
  tables: [sample_defs/tables/sch_lab_table.c@<sha>, control_gain_tbl.c@<sha>]
  startup_script: cfe_es_startup.scr@<sha>
  contracts: contracts/control.yaml@<sha>
variant_diff: diffs/SYN-F2-01-faulty.patch
affected_apps: [SENSOR, NAV, CONTROL]
events:
  E1: {kind: state_update, func: CONTROL_ProcessNavState, loc: "control_app.c:142",
       field: CONTROL_Data.Att, source_mid: NAV_STATE_MID}
  E2: {kind: state_use, func: CONTROL_ControlStep, loc: "control_app.c:201"}
required_order: "E1 -> E2"
required_order_source: {kind: contract, ref: "contracts/control.yaml#att_before_use"}
                              # api_impl | api_doc | app_comment | issue | contract
violating_order: "CONTROL_WAKEUP 처리(E2)가 첫 NAV_STATE 수신(E1)보다 먼저"
guards_present: []
expected:
  verdict: warn               # warn | no_warn | allowed | conditional
  rule: FS-F2
  dedup_key: "CONTROL_Data.Att|control_app.c:201|NAV_STATE_MID"   # 범주는 key에 넣지 않음
  assumption: null            # conditional이면 가정 문장
manifestation:
  observable: "CONTROL HK UsedInvalidCtr > 0"
  confirm_env: {uid: 65534, caps: "+sys_nice", sched: SCHED_RR, cpus: "0",
                msg_max: 10, ipc_ns: false, osal_permissive: true, kernel: "6.18.44"}
  witness: "startup.scr에서 CONTROL을 SENSOR보다 높은 priority로, SENSOR init에 OS_TaskDelay(500)"
  trials: 50
  observed: null              # 실행 후 기록
  evidence_level: null        # L0 | L1 | L2
label: {status: fault, labeler: A, reviewer: B, frozen_at: null, disagreement: null}
paired_with: [SYN-F2-01-correct, SYN-F2-01-benign]
```

**[해석]** `required_order_source`와 `confirm_env`가 핵심이다. 앞의 것이 없으면 label의 근거를 확인할 수 없다. 뒤의 것이 없으면 재현 결과를 다른 환경과 비교할 수 없다(§8.7).

### 8.7 결함 확인 절차

#### 8.7.1 실행 환경

**[실측]** 이 컨테이너에서는 실행 환경 선택이 결과를 바꾼다.

| 실행 방식 | 결과 | 출처 |
| --- | --- | --- |
| root, host IPC namespace | 실패. `mq_open` EINVAL → SB pipe 생성 실패 → processor reset → abort(exit 134). root에는 `CAP_SYS_RESOURCE`가 없고 `msg_max`=10이다 | `$N/probe/run_root.log` |
| uid 65534 | OPERATIONAL 도달. 모든 queue depth가 10으로 잘린다. RT priority 설정이 거부되어 SCHED_OTHER로 실행된다. permissive mode라 오류 없이 진행하고 debug 메시지 한 줄만 남는다 | `$N/probe/run_nobody.log`; [bsp_start.c L66–75](https://github.com/nasa/osal/blob/dad0ee99c121f2ecc11f8c0798c1e836e41d8ab1/src/bsp/generic-linux/src/bsp_start.c#L66-L75) |
| root, `unshare --ipc`, namespace 안의 `msg_max`=256 | OPERATIONAL 도달, SCHED_RR 사용 | `$N/probe/run_root_ipcns.log` |
| uid 65534 + `CAP_SYS_NICE`, `taskset -c 0` | 앱 시작 순서가 priority 순서(5/5) | `$N/sem-es-tbl/runs/order_repeats.txt`; `$N/sem-es-tbl/run_probe.sh` L20–24 |
| 같은 설정, 4 CPU | startup script 순서(5/5) | 같은 파일 |
| capability 없음, 4 CPU | script 순서 4/5 | 같은 파일 |
| capability 없음, `taskset -c 0` | script 순서 5/5 | 같은 파일 |

**[설계 제안]** 모든 L2 확인은 위 표의 실행 방식 중 하나를 명시한다. 기록 항목은 uid, CapEff, `msg_max`, IPC namespace, scheduling policy, CPU affinity, OSAL permissive 여부, kernel, 반복 횟수다. 서로 다른 실행 방식의 결과는 합산하지 않는다.

#### 8.7.2 순서를 강제하는 방법

| 방법 | 적용 범주 | 근거 |
| --- | --- | --- |
| startup script 순서·priority + SCHED_RR + 단일 CPU | F1, F5 | **[실측]** 위 표 |
| init 안의 `OS_TaskDelay` 주입(변형 flag로 켜고 끔) | F1, F2, F5 | **[실측]** 2.5 s 지연으로 ES startup sync timeout과 OPERATIONAL 진입을 재현함(§8.3.4) |
| relay 앱의 priority를 sender보다 높게 | F7, F4(교차 pipe 순서 역전) | **[실측]** 단일 CPU: 500/500회 역전, relay가 낮으면 0/500. 4 CPU: 0/500. non-root: 15/500, 30/500(`SBPROBE_lines_all_runs.txt` T6) |
| MsgLim보다 많은 burst | F1, F4 | **[실측]** §8.3.4 |
| `CFE_ES_RestartApp` 명령 | F3, F8 | **[실측]** 요청부터 새 instance 진입까지 약 1.6 s |
| SB route disable 지상 명령 | F3 | **[확인된 사실]** 비활성 목적지는 조용히 건너뛴다([cfe_sb_priv.c L1067–1089](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_priv.c#L1067-L1089)) |
| gdb breakpoint | 반환값이 버려지는 경로 | **[확인된 사실]** `cfe_evs.c:188` breakpoint로 #2663 경로를 관측함(§8.5) |

**[해석]** 순서 역전 사례는 relay pipe가 목적지 목록의 head에 있을 때, 즉 observer보다 늦게 구독했을 때만 측정했다. 다른 구독 순서는 시험하지 않았다(검증 C10). benchmark에서는 구독 순서도 변형 요소로 기록한다.

#### 8.7.3 L2 판정 기준

**[설계 제안]** 다음을 모두 만족해야 L2로 기록한다.

1. 위반 순서가 실행에서 관측된다. 근거는 앱 HK counter나 CLOCK_MONOTONIC 시각 기록이다. cFE syslog 시각은 쓰지 않는다. **[실측]** TIME 초기화 때 syslog 시각이 1980-001에서 1980-012로 뛰었다(`$N/sem-es-tbl/runs/R1_rr_allcpu/console.log`).
2. 그 순서 때문에 관측 가능한 잘못된 상태나 동작이 생긴다. 예: counter 증가, 잘못된 telemetry 값, assert, crash.
3. `correct` 변형을 같은 환경·같은 반복 횟수로 실행했을 때 그 증상이 없다.
4. SB counter는 SB HK telemetry로 읽는다. EVS 이벤트 수는 쓰지 않는다(§8.3.1).

**[해석]** 정해진 횟수 안에서 증상이 0회였다는 결과는 "이 환경에서 관측되지 않음"이다. 불가능하다는 뜻이 아니다. 4 CPU의 0/500이 그 예다.

### 8.8 지표

#### 8.8.1 검출 정확도

**[원노트 구상]** precision = TP/(TP+FP), recall = TP/(TP+FN)(원노트 §40, 수정본 §12.2).

**[설계 제안]** 다음 규칙으로 센다.

| 항목 | 규칙 |
| --- | --- |
| TP | 정답 `warn` 사례의 `dedup_key`(상태 필드·consumer 위치·producer MID 또는 API)와 일치하는 경고가 1개 이상 있음. 경고의 범주가 정답과 다르면 TP로 세되 "범주 불일치"를 따로 보고 |
| FP | `no_warn` 또는 `allowed` 사례에서 낸 경고. 같은 `dedup_key`의 경고는 1개로 셈 |
| FN | `warn` 사례에 일치하는 경고가 없음 |
| 중복 제거 | 같은 key의 경고는 하나로 합친다. inline된 복사본은 debug location의 원래 source 위치로 합친다(**[실측]** 최적화 후 inline으로 호출 위치가 복제된다. 검증 C18) |
| `conditional` 사례 | 가정을 명시한 경고는 TP. 가정 없이 낸 경고는 "가정 누락"으로 따로 셈. 경고가 없으면 FN |
| 분석 실패 | timeout, unsupported 구문, import 오류는 TP/FP/FN에서 빼고 사례 수와 원인을 따로 보고 |
| 분모 0 | 비율 대신 "N/A"로 적음 |

**[설계 제안]** 보고 단위는 범주 × 층이다. 합성·주입·역사 사례를 하나의 비율로 합치지 않는다. 모든 칸에 원래 개수(TP, FP, FN)를 같이 적는다. 사례 수가 작으므로 비율에는 exact(Clopper–Pearson) 구간을 붙인다.

**[설계 제안]** precision은 증거 수준별로 두 번 계산한다. `P_L1`은 L1 이상으로 확인된 경고의 비율이고, `P_L2`는 L2로 재현된 경고의 비율이다.

#### 8.8.2 분석 비용

**[설계 제안]** 단계별로 wall time과 peak RSS를 잰다(`/usr/bin/time -v`). 단계는 frontend(clang → LLVM IR → MLIR import), lifting, solver, 보고서 생성이다. 앱 하나와 mission 전체를 따로 잰다. 각각 5회 실행의 중앙값을 보고한다. 이 컨테이너는 4 vCPU다.

**[실측]** 현재 frontend 비용의 기준값은 다음과 같다.

| 측정 | 값 | 출처 |
| --- | --- | --- |
| 앱 TU 17개의 clang `-O0 -g` | 파일당 0.03–0.06 s | `$N/probe/ir/*/commands.log` |
| 같은 17개의 `mlir-translate --import-llvm` | 파일당 8–25 ms, 진단 0 | `$N/probe/mlir/import.log` |
| 선택한 154개 파일의 link 후 import | 1.186 s, 정의된 `llvm.func` 2389개, 외부 선언 184개, 간접 호출 73개 | `$N/rw-mlir/import_exp/link_import2.txt`, `import_summary.txt` |

**[확인된 사실]** 154개 파일의 범위(cFE module 5개, 앱 디렉터리 17개)와 성공 조건(clang으로 구성한 별도 build tree, `-Werror` 제거)은 §7.5.2에 있다 (검증 C17 정정 문구).

#### 8.8.3 의미 coverage

**[설계 제안]** 다음 비율을 앱별로 보고한다. 분모는 source에서 직접 센 정답 집합이다.

| 지표 | 정의 | 현재 기준값 |
| --- | --- | --- |
| API 호출 인식 | 모델링한 cFE API 호출 위치 / 전체 호출 위치(SB, ES, TBL, EVS, MSG별) | **[실측]** 앱별 `-O0` 호출 위치 수는 §7.6의 표 |
| MID 해석 | MID 인자 위치를 세 종류로 나눈 비율: code 상수로 해석, table·명령 데이터에서 해석, 미해석 | **[실측]** 앱별 상수 회수 수는 §7.7.3의 표(to_lab은 검증 C18에서 3/7로 정정) |
| dispatch 복원 | 복원한 (MID, handler) 쌍 / 수작업 정답 | 아직 없음. **[확인된 사실]** dispatch는 `CFE_MSG_GetMsgId` 출력 인자와 function-static MID cache의 비교로 이루어진다([sample_app_dispatch.c L132–167](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app_dispatch.c#L132-L167)) |
| 상태 갱신 인식 | 인식한 global field store / 수작업 정답 | **[실측]** 정규식 probe가 global field store 83곳을 찾았다. 놓친 형태는 offset 0 field, API 출력 인자 쓰기, pointer 지역변수를 통한 쓰기다. sch_lab은 0곳이다 |
| provenance 간선 | 복원한 (MID → field → 사용 위치) 경로 / 수작업 정답 | 아직 없음 |

**[해석]** MID 해석률은 code만으로는 100%가 될 수 없다. HK copy table, TO_LAB 구독 테이블과 명령, SCH_LAB schedule이 MID를 데이터로 제공하기 때문이다. 따라서 테이블 입력을 쓰는 경우와 쓰지 않는 경우를 나누어 보고한다.

#### 8.8.4 수작업 모델링 부담

**[설계 제안]** 다음을 기록한다.

- API 모델의 LOC(API 하나당, 그리고 전체).
- 테이블 이미지 추출 코드의 LOC.
- contract 수와 그 LOC(앱당).
- 앱마다 필요한 수작업 설정(예: closed-world 함수 목록, 간접 호출 대상 목록).
- 작업 시간 기록(시간 단위).

같은 항목을 기준선 구현(§8.9)에도 적용한다.

**[미확인]** IKOS가 BioSentinel 앱 분석에 약 1200 LOC의 CFE 모델을 썼다는 수치는 NASA 슬라이드의 검색 snippet으로만 확인했다(NTRS 20190032510 접근 차단). 비교 기준 숫자로 쓰지 않는다.

#### 8.8.5 진단 설명의 완결성

**[원노트 구상]** 원노트 §67의 `[FS-RACE-01]` 형식과 수정본 §10.5의 설명 항목.

**[설계 제안]** 경고마다 다음 여섯 항목이 있는지 검사하고 비율로 보고한다.

1. 발행·구독·수신·갱신·사용 위치.
2. MID와 state field의 연결.
3. 위반 순서의 witness.
4. 필요한 순서의 출처.
5. 고려한 guard와 오류 경로.
6. 분석 경계(알 수 없는 호출, alias, 플랫폼 가정).

**[실측]** `--mlir-print-debuginfo`를 쓰면 5개 앱에서 `llvm.call` 477/477, `llvm.load` 996/996, 상태 store 83/83이 file:line을 가진다. 위치 정보가 없는 것은 진입 block의 인자 저장 95곳과 hoist된 상수다.

### 8.9 기준선

**[설계 제안]** 모든 기준선은 같은 입력을 받는다. 같은 고정 commit, 같은 compile DB, 같은 테이블 이미지, 같은 contract다. SB 모델이 필요한 기준선에는 제안 분석기와 같은 SB 의미를 넣는다. **[해석]** 그래야 차이를 SB 모델의 유무가 아니라 분석 방법의 차이로 해석할 수 있다(수정본 §12.3).

| ID | 기준선 | 구현 계획 | 할 수 있는 것·없는 것 | 현재 상태 |
| --- | --- | --- | --- | --- |
| B0 | 직접 호출 그래프 | MLIR `CallGraph` | **[실측]** const 함수 포인터 테이블을 통한 간접 호출은 `<Unknown-Callee-Node>` 간선 하나만 남는다(`$N/rw-mlir/cir_test/t2.callgraph.out`). **[해석]** SB를 거치는 앱 간 간선은 나타나지 않는다 | 참고용. 이것만 비교하면 너무 약함 |
| B1 | MID·pipe를 아는 topology 그래프 | 제안 분석기의 API 추출 결과에서 publish→subscribe→handler 간선만 사용 | 연결은 보이지만 상태·guard·순서는 보지 않음. **[확인된 사실]** ROSDiscover(ICSA 2022)가 ROS에서 같은 수준의 구조 규칙만 검사한다(검증 C02) | 구현 필요 |
| B2 | CodeQL + 같은 SB 모델 | `isAdditionalFlowStep`으로 같은 상수 MID의 `TransmitMsg`→`ReceiveBuffer` 간선을 추가하고, F1·F2·F6 규칙을 query로 작성 | **[확인된 사실]** C/C++ global data flow는 함수 사이에서 global을 `jumpStep`으로 연결하며 실행 순서를 보지 않는다(DataFlowPrivate.qll [S125]). library model 확장은 "Beta"다 | 구현 필요. CLI 미설치·미실행 |
| B3 | Clang Static Analyzer custom checker | `evalCall`로 `CFE_SB_*`를 모델링하고 F2·F6 규칙 구현 | **[실측]** YAML taint 설정만으로는 int payload의 직접 경로만 보고되었다. double payload와 opaque 호출을 지난 경우는 보고되지 않았다(`$N/rw-mlir/csa_exp/csa_all.log`). 그래서 custom checker가 필요하다 | 구현 필요 |
| B4 | ROSInfer 방식 휴리스틱 재구현 | publish 조건에 쓰이고 publish 함수 밖에서 대입되는 변수를 상태 변수로 추론 | **[확인된 사실]** ROSInfer는 메시지 내용을 모델링하지 않는다. 저자는 future work에서 정적 분석으로 실행 시간을 추론할 수 없어 그 모델을 대부분의 race 분석에 쓸 수 없다고 밝힌다(검증 C02) | 구현 필요. F2·F4에서 field 수준 provenance의 효과를 분리 |
| B5 | cFS model-based testing [R03] | 원 모델·harness로 SB 의미 적합성 시험(§8.3.4)과 대조 | **[확인된 사실]** 공개된 Spec Explorer 모델·harness를 찾지 못했다(검증 C03). 대상이 SB API conformance다 | 저자에게 요청. 얻지 못하면 제외하고 그 사실을 적음 |
| B6 | Ogma/Copilot runtime monitor(R09, S24) | 합성 앱의 F2·F3·F4 성질을 monitor로 생성해 같은 실행에서 관측 | 실행한 schedule만 본다. **[확인된 사실]** template은 startup sync를 호출하지 않는다(§8.4.2). **[해석]** 입력이 둘 이상이면 따로 갱신된 global의 최신값을 함께 평가하는 구조다 | Ogma 미실행. Haskell 도구 설치 여부 미확인 |
| B7 | ThreadSanitizer [S20] | native 빌드를 `-fsanitize=thread`로 실행 | 비교 대상이 아닌 참고값. 공유 메모리 race를 보고한다 | **[미확인]** cFS native 빌드에서 동작하는지, TSan이 POSIX mq를 동기화로 취급하는지 확인하지 않았다 |
| B8 | Goblint(공유 메모리 부분) | pthread 기반 OSAL 위에서 race 분석; `ana.thread.wrappers`로 `OS_TaskCreate` 처리 | **[확인된 사실]** pthread 의미가 내장된 thread-modular 분석으로 lock 기반 공유 메모리 race를 다룬다. SB 메시지 순서는 모델링하지 않는다(README·option schema 확인, goblint/analyzer d0ee7d5b [S123]) | 미실행. #950, #46 비교용 |

**[확인된 사실]** 구조적 선행 사례로 fprime-topo-analysis가 있다. F'의 topology(fpp-to-json)와 libclang handler 흐름을 결합해 lock 순서 deadlock, lock-aware data race 등을 검사한다. 상태 갱신 순서와 freshness는 분석하지 않는다(검증 C01, README [S121]). **[해석]** cFS에서는 실행할 수 없다. 따라서 실행 기준선이 아니라 B1–B2의 설계 근거로 인용한다.

**[해석]** B2와 B3의 규칙은 연구자가 직접 작성하므로 약한 기준선이 될 위험이 있다. §8.11에서 이 위험을 다룬다.

### 8.10 Ablation: 도메인 의미와 MLIR 기반의 효과 분리

**[원노트 구상]** 원노트 §42–43은 MLIR이 단순 grep 이상의 일을 해야 정당화된다고 했다. **[해석]** 수정본 §12.4는 도메인 정보의 효과와 MLIR 구현 기반의 효과를 구별하라고 요구한다. 아래 두 축으로 나눈다.

#### 8.10.1 도메인 의미 ablation

**[설계 제안]** 제안 분석기에서 한 가지씩 끄고 범주별 TP/FP/FN 변화를 본다.

| ID | 끄는 의미 | 영향을 예상하는 범주 **[해석]** |
| --- | --- | --- |
| AB1 | 반환 status와 timeout 경로(`ReceiveBuffer`의 `NO_MESSAGE`/`TIME_OUT`, `WaitForSystemState`의 timeout) | F1, F2, F5 |
| AB2 | guard·mode 분기 인식 | F2, F3, F4의 `correct` 변형에서 FP 증가 |
| AB3 | field 수준 provenance(구조체 전체를 하나의 상태로 취급) | F2, F4 |
| AB4 | 분기별 공동 갱신 상관관계 | SYN-F4-02 함정 사례 |
| AB5 | 테이블 입력에서 온 MID(code 상수만 사용) | HK, TO_LAB, SCH_LAB 관련 간선 누락 |
| AB6 | MsgLim·pipe depth와 목적지별 전달 실패 | F1, F4 |
| AB7 | ES 시작 의미(OPERATIONAL ≠ 모든 앱 RUNNING, sync 호출이 호출자 상태를 올림) | F1, F5의 `conditional` 변형 |
| AB8 | 재시작 의미(pipe 삭제·route 유지·새 AppId) | F8 |
| AB9 | TBL의 buffering 옵션과 handle 단위 lock | F6 |
| AB10 | scheduling 가정: "priority 순서 고정" vs "임의 interleaving" | 모든 범주의 FP/FN 균형 |

#### 8.10.2 구현 기반 ablation

**[설계 제안]** 같은 규칙 집합(F1–F8 검사와 같은 SB·ES·TBL 모델)을 세 엔진에 구현한다.

| 엔진 | 표현 | 사용하는 MLIR 기능 |
| --- | --- | --- |
| E-dialect | `cfs` dialect로 lifting한 IR + `DataFlowSolver` | dialect op·verifier, 비주소 자원에 대한 memory effect, solver, location |
| E-llvm | lifting 없이 LLVM dialect 위에서 `llvm.call`마다 전이 함수를 둠 | solver, location만 |
| E-ql | CodeQL(B2와 같은 구현) | 사용 안 함 |

**[해석]** 세 엔진의 TP/FP가 같다면 검출 차이는 도메인 의미에서 나온 것이다. 그 경우 MLIR의 기여는 아래 공학 지표로만 주장할 수 있다.

- 규칙과 모델의 LOC.
- 새 API(예: TBL 계열)를 추가하는 데 바꾼 파일 수와 LOC.
- 진단 위치의 정확도(§8.8.5).
- 분석 시간.

**[설계 제안]** MLIR 내부의 기반 선택도 따로 ablation한다. 각 항목에는 이미 확인된 upstream 동작이 있다.

| ID | 비교 | 확인된 출발점 |
| --- | --- | --- |
| I1 | upstream `LocalAliasAnalysis` vs field·global을 구별하는 자체 memory model | **[실측]** 서로 다른 global은 `MayAlias`, 한 구조체의 다른 field는 `MustAlias`(`$N/rw-mlir/mlir_probe/alias_llvm.out`) |
| I2 | `llvm.call` 기본 처리 vs SB op의 비주소 자원 effect | **[실측]** `llvm.call`은 callee가 외부든 정의되었든 모든 위치에 `ModRef`다. readnone `memory_effects` 속성이 있어도 같다(`$N/rw-mlir/mlir_probe/modref_llvm.out`, 검증 C20) |
| I3 | import한 함수 그대로 vs closed-world 단계(`sym_visibility = "private"`) | **[실측]** import한 C 함수는 `static`이어도 predecessor를 모두 알 수 없음으로 표시된다. private로 바꾸면 "(all) predecessors"가 된다(`$N/rw-mlir/mlir_probe/dca_vis.out`, 검증 C20) |
| I4 | 간접 호출 미해결 vs points-to 결과 사용 | **[실측]** const 함수 포인터 테이블도 미해결(B0 행) |
| I5 | frontend: `-O0` vs `-O1 -Xclang -disable-llvm-passes` + `--inline --sroa --mem2reg --canonicalize --cse` | **[실측]** `-O0`에서는 MID가 `CFE_SB_ValueToMsgId` 호출 → alloca → load를 거친다. 최적화 경로에서는 `llvm.mlir.constant` 피연산자가 된다(검증 C18) |
| I6 | MID 이름 복원: AST side channel 사용 vs 정수만 사용 | **[실측]** LLVM IR import 후에는 정수만 남는다. `-fdebug-macro`의 `DIMacro`도 import에서 사라진다(검증 C18) |
| I7 | ClangIR frontend | **[실측]** 로컬 clang에서는 사용할 수 없다. `-fclangir -S -emit-llvm`은 오류 없이 무시되고 출력이 일반 codegen과 같다(검증 C19). CIR을 켠 별도 빌드가 있을 때만 실행 |

**[해석]** I3–I5는 작은 준비 단계로 해결될 수 있다. 반면 I1–I2는 연구자가 직접 만들어야 하는 memory model이다. 따라서 "MLIR이 제공한 것"과 "연구자가 만든 것"을 표로 나누어 보고한다.

### 8.11 타당성 위협

| 종류 | 구체적 위협 | 대응 |
| --- | --- | --- |
| 내부 | 같은 연구자가 결함을 넣고 분석기를 만든다. 쉬운 결함만 넣을 위험이 있다 | 결함 주입과 label을 다른 사람이 맡는다. 정답 기록을 분석기 실행 전에 동결한다. 주입 diff를 공개한다 |
| 내부 | contract가 분석기에 맞게 쓰일 수 있다(순환) | 앱 자체의 주석·설정(TO_LAB, HK, LC)을 우선 출처로 쓴다. contract는 분석기 구현을 보지 않은 사람이 쓴다 |
| 구성 | "결함"과 "허용된 설계"의 경계가 모호하다. 예: HK의 불완전 packet, TO_LAB의 첫 이벤트 손실, latest-value 조합 | `benign`·`allowed` label과 그 근거 출처를 둔다. 근거가 없으면 label을 붙이지 않는다 |
| 구성 | freshness를 시간으로 정의할 근거가 없다. **[확인된 사실]** `IsOrigination=true`로 보내는 telemetry의 header 시각은 기본 origination action이 송신 시각으로 덮어쓴다([cfe_msg_integrity.c L30–53](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/msg/fsw/src/cfe_msg_integrity.c#L30-L53)) | F3는 LC처럼 이벤트 수로 정의된 정책이 있거나 contract가 있는 경우에만 label한다. 원노트의 20 ms·100 ms 값은 쓰지 않는다(수정본 §6.5) |
| 외부 | cFE 버전마다 의미가 다르다 | 모든 결과에 버전을 붙인다. **[확인된 사실]** 알려진 차이: `NEVER_LOADED`의 포인터·lock 동작은 v6.7.0과 546a002가 다르다(검증 C13). `OS_QueuePut`을 SB mutex 밖에서 호출하는 구조는 550e7f7d [S85](v7.0.0)부터다(검증 C10). TO_LAB 구독 지연은 d3d52da(v7.0.1)부터다(검증 C16) |
| 외부 | native Linux 결과를 RTOS에 일반화할 수 없다 | 결과마다 실행 환경을 적는다. **[미확인]** VxWorks queue의 순서 의미는 OSAL code만 읽었다 |
| 외부 | EDS 빌드에서는 MID가 runtime 함수 호출 결과다 | **[확인된 사실]** [eds_cfe_core_api_msgid_mapping.h L37–66](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/config/eds_cfe_core_api_msgid_mapping.h#L37-L66). 첫 평가는 non-EDS 빌드로 한정하고 그렇게 표시한다 |
| 외부 | 공개 앱이 실제 임무 code와 다를 수 있다 | 공개 앱 결과를 비행 소프트웨어 일반의 결과로 쓰지 않는다(수정본 §11.3) |
| 측정 | EVS filter와 발행 앱의 EVS 등록이 이벤트 수를 바꾼다 | SB HK counter와 앱 HK counter만 쓴다(§8.7.3) |
| 측정 | 실행 반복 수가 작다. **[실측]** 시작 순서 측정은 설정당 5회였다 | 사례당 반복 수를 사전에 정하고 관측 횟수를 그대로 보고한다. 빈도를 확률로 바꾸지 않는다 |
| 결론 | 역사적 사례가 적고, F1·F4의 공개 실패 보고는 찾지 못했다 | 범주별 결과를 층별로 따로 보고한다. **[확인된 사실]** ROS의 ROBUST dataset에서도 221개 중 CONCURRENCY 코드가 붙은 bug는 19개였고, 구독 전 발행으로 인한 손실은 제목·설명 키워드 검색으로 찾지 못했다(robust @521fe75 [S133]). 다른 생태계의 비율을 cFS의 발생률로 쓰지 않는다 |
| 기준선 공정성 | B2·B3를 연구자가 구현하면 약해질 수 있다 | 같은 SB 모델을 쓴다. query와 checker를 공개한다. 기준선 구현에도 같은 작업 시간 예산을 정하고 기록한다 |
| 도구 | MLIR 분석 API가 자주 바뀐다. **[실측]** PoTATo가 로컬 MLIR(2026-02)에 대해 빌드되지 않았다(`$N/rw-mlir/potato_build.log`) | LLVM commit을 고정하고 기록한다 |
| 새로움 판단 | 선행 연구 확인이 불완전하다. R03, Ganesan et al. SPLC 2009, 2020 IV&V 보고서 [S07] 등은 이번 조사에서 검색 snippet 수준으로만 다시 확인했다 | 평가 결과로 "최초"를 주장하지 않는다. 비교는 확인한 기능 범위로 한정한다 |

### 8.12 이 절이 아직 주장하지 않는 것

- **[미확인]** 제안 분석기의 precision, recall, 분석 시간. 실행 결과가 없다.
- **[미확인]** 기준선 B2–B8의 동작. 어느 것도 cFS 위에서 실행하지 않았다.
- **[미확인]** 역사적 사례의 원 환경 재현. 특히 #73은 Microblaze/GRC 환경이 없다.
- **[해석]** §8.3.4의 실측은 native Linux 한 컨테이너의 관측이다. 다른 kernel, CPU 수, RTOS에서 같은 결과를 기대할 근거는 없다.
- **[해석]** 합성 앱에서 성공해도 비행 결함 검출 성능을 뜻하지 않는다(수정본 §11.4).
