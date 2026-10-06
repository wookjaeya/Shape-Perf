## 1. 연구 문제·범위·용어

이 절은 다섯 가지를 정한다.

1. 연구가 찾으려는 결함을 cFS 이벤트로 정의한다.
2. 범위에서 빼는 대상을 명시한다.
3. 원노트의 용어를 기존 문헌의 정의에 대응시킨다.
4. 연구 질문과 가설을 반박 가능한 형태로 다시 쓴다.
5. 논문이 할 수 있는 주장과 할 수 없는 주장을 나눈다.

표기, `$N`, 고정점은 문서 머리의 메타데이터와 「근거 표기」를 따른다.

---

### 1.1 연구 문제

**[원노트 구상]** 원노트(§1–2, §64)의 관심은 shared-memory data race가 아니다. 앱·메시지·프레임워크 서비스·상태 갱신·lifecycle event 사이에 필요한 순서가 프로그램에서 보장되는지를 정적으로 판정하는 것이다.

**[설계 제안] 분석 대상 시스템.** 하나의 분석 대상을 다음 튜플로 고정한다.

```text
System = ( cFE·OSAL·PSP revision,
           앱 소스 집합 (compile_commands.json 포함),
           build 설정과 MID 매핑 header (non-EDS / EDS),
           table 이미지 (sample_defs/tables/*.c 등 초기화자),
           startup script (cfe_es_startup.scr),
           실행 가정 Σ  — 1.3절 )
```

**[설계 제안] 결함의 정의.** 이벤트 쌍 `(e1, e2)`가 다음 네 조건을 모두 만족하면 연구 대상 결함이다.

1. **필요 순서 `Req(e1 ≺ e2)`.** 근거가 하나 이상 있다. 근거는 (i) cFE API 의미, (ii) 앱 코드의 데이터 의존, (iii) 앱별 명시 계약, (iv) 공개 결함 기록 중 하나다.
2. **보장 부재.** 보장 관계 `G`로 `e1 ≺ e2`를 증명할 수 없다. `G`는 프로그램 순서, 성공한 SB 전달, ES 동기화, TBL 프로토콜에서 유도한다.
3. **실행 가능성.** 실행 가정 Σ 아래에서 `e2`가 `e1`보다 먼저 일어나는 실행이 있다.
4. **관측 가능한 결과.** 그 실행에서 앱 state나 출력이 달라진다. 그 차이를 흡수하는 guard·재시도·후속 갱신 경로가 없다.

**[해석] 증거 수준.** 조건 1–2만 만족한 결과는 **후보**다. Σ를 명시한 모델에서 조건 3까지 보이면 **모델상 가능**이다. 조건 3–4를 실제 실행에서 관측하면 **재현**이다. 논문은 세 수준을 따로 센다. 이는 수정본 §10.2·§10.5의 구분을 따른다. 원노트 §47의 `RequiredHB ∧ ¬ProvenHB ∧ PotentialConcurrent`는 조건 1–3에 해당한다. 조건 4는 빠져 있다.

**[확인된 사실]** "순서 명세를 얻은 뒤 그 위반을 정적·동적으로 찾는다"는 분해는 이미 제안되어 있다. TaxDC §8.4는 메시지 타이밍 결함 검출이 (1) order·atomicity 명세 획득과 (2) 그 위반의 정적·동적 검출에 집중해야 한다고 쓴다 (TaxDC, ASPLOS 2016 저자 PDF [R12], §8.4; 로컬 텍스트 `$N/rw-races/txt/taxdc_asplos16.txt` L1553-1611). 같은 절은 명시적 오류 검사에서 거꾸로 명세를 추론하는 방법도 제안한다. 단 §8.4는 'Lessons Learned'의 제안이다. 구현·평가된 검출기는 없다.

**[해석]** 따라서 위 정의 자체는 기여가 아니다. 기여 후보는 cFS에서 `Req`와 `G`를 어떻게 얻는지, 그리고 그 판정을 cFS C 소스에서 정적으로 수행하는지에 있다 (1.7절).

**[설계 제안] 이벤트 어휘.** 범주 정의는 아래 이벤트로 쓴다. 각 이벤트는 실제 cFE API의 지점에 묶는다.

| 이벤트 | 대응 API·코드 지점 | 의미 | 근거 |
| --- | --- | --- | --- |
| `pub(a,m,s)` | 앱 `a`의 호출 위치 `s`에서 `CFE_SB_TransmitMsg`/`CFE_SB_TransmitBuffer` 진입 | MID `m`은 인자가 아니다. 앞선 `CFE_MSG_Init`/`SetMsgId`로 buffer에 묶인다 | **[확인된 사실]** [sample_app.c L134](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app.c#L134), [sample_app_cmds.c L61](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app_cmds.c#L61) |
| `enq(p,m)` / `drop(p,m,why)` | 목적지별 `OS_QueuePut`. SB mutex를 푼 뒤 실행된다 | 한 번의 발행은 목적지마다 별개의 이벤트다. `why ∈ {NO_ROUTE, ZERO_DEST, MSGLIM, PIPE_FULL, INACTIVE}` | **[확인된 사실]** [cfe_sb_priv.c L1032-L1131, L1173-L1231](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_priv.c#L1032-L1131) [S12] |
| `sub(b,m,p)` | `CFE_SB_Subscribe`/`SubscribeEx`/`SubscribeLocal`가 `CFE_SUCCESS`를 반환 | 호출 존재가 아니라 성공 반환이 이벤트다 | **[확인된 사실]** [cfe_sb_api.c L936-L1110](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_api.c#L936-L1110) |
| `rcv(b,p,m)` | `CFE_SB_ReceiveBuffer`가 `CFE_SUCCESS`를 반환 | `NO_MESSAGE`·`TIME_OUT`·`PIPE_RD_ERR` 경로와 구별한다. buffer는 같은 pipe의 다음 수신 호출까지만 유효하다 | **[확인된 사실]** [cfe_sb.h L440-L479](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb.h#L440-L479) [S09] |
| `w(b,f,o)` | 앱 전역 state field `f`에 대한 store | 출처 `o ∈ {payload(m).g, table T, 상수, API out-param, unknown}` | **[설계 제안]** |
| `r(b,f,u)` | 사용 위치 `u`에서의 `f` 읽기 | 판단·제어·출력에 쓰이는 읽기만 센다 | **[설계 제안]** |
| `ready(b,σ)` | `CFE_ES_WaitForSystemState`·`CFE_ES_RunLoop`가 앱의 AppState를 올리는 지점 | 시스템 단계 `sys(σ)`는 ES main의 `SystemState` 대입이다 | **[확인된 사실]** [cfe_es_api.c L514-L619](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_api.c#L514-L619) [S11] |
| `tbl_get(h)` / `tbl_rel(h)` / `tbl_upd(T)` | `CFE_TBL_GetAddress`/`ReleaseAddress`/`Update`·`Manage` | 잠금은 pointer가 아니라 handle 단위다 (F6) | **[확인된 사실]** [cfe_tbl_accdesc.h L54-L64](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/tbl/fsw/src/cfe_tbl_accdesc.h#L54-L64) |
| `restart(b)` | ES가 control request를 처리: `CFE_ES_CleanUpApp` → `CFE_ES_AppCreate` | 새 AppId가 생긴다 | **[확인된 사실]** [cfe_es_appctrl.c L318-L379](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_appctrl.c#L318-L379) |

**[설계 제안] 분석 단위.** 실행 순서는 task 단위로 본다. 식별·소유는 앱 단위로 본다. **[확인된 사실]** child task에서 부른 ES·SB·TBL API는 부모 앱의 AppId로 동작한다 ([cfe_es_resource.c L308-L338](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_resource.c#L308-L338)). **[미확인]** 한 앱의 여러 task가 TBL handle을 공유할 때의 실행 동작은 소스만 읽었고 실행하지 않았다.

---

### 1.2 다루는 결함 범주

**[설계 제안]** 원노트 §38의 F1–F6을 유지하고, 두 범주를 덧붙인다. F7은 원노트 §14(R1)의 메시지 간 순서다. F8은 원노트 §24-D와 수정본 §5.4의 재시작 창이다.

| ID | 이름 | 필요한 순서 `Req` | 원노트 위치 | 가장 가까운 기존 용어 (1.5절) |
| --- | --- | --- | --- | --- |
| F1 | 첫 메시지 유실 | `sub(b,m,p)` ≺ `b`가 반드시 받아야 하는 첫 `pub(a,m)` | §15, §31, F1 | message–computation order violation, late handler registration, late joiner |
| F2 | 메시지 유래 state의 첫 갱신 전 사용 | `w(b,f,payload(m))` ≺ 유효 입력을 요구하는 `r(b,f,u)` | §32, F2, Property 1 | order violation (use before initialization) |
| F3 | 갱신 경계를 넘은 state 사용 | 마지막 `w(b,f)`가 앱이 정한 경계 β 이후에 있음 | §17, F3, Property 2 | stale-value error(부분), data age |
| F4 | 다중 stream snapshot 불일치 | 사용 `u`가 읽는 `f_A`, `f_B`의 갱신 메시지가 앱이 정한 일관성 관계 `C`를 만족 | §18, §33, F4, Property 3 | high-level data race·atomicity(적용 미결), data age |
| F5 | 시작 시 서비스·자원 의존 | 제공자의 준비 ≺ 소비자의 첫 사용 | §16, §34, F5 | order violation (startup) |
| F6 | Table 수명 프로토콜 위반 | `tbl_get(h)` ≺ 사용 ≺ `tbl_rel(h)` 및 buffer 규칙 | §23–24, F6 | API protocol(typestate). 일부만 순서 문제 |
| F7 | 메시지 간 도착 순서 | 소비자 `c`에서 `rcv(m1)` ≺ `rcv(m2)`, 또는 인과 순서의 보존 | §14 (R1) | message race, message–message order violation |
| F8 | 재시작 창 | 재시작 뒤 재구독·재등록 ≺ 다른 앱의 의존 사용 | §24-D, 수정본 §5.4 | reboot/restart timing (TaxDC) |

아래는 범주마다 cFS 의미, 실측, 경고하면 안 되는 정상 대조 사례, 공개 기록을 정리한다.

#### F1 — 첫 메시지 유실

- **[확인된 사실]** 경로가 없는 MID로 발행하면 SB는 `NoSubscribersCounter`(uint8)를 올리고 `CFE_SB_SEND_NO_SUBS_EID`를 요청한 뒤 `CFE_SUCCESS`를 반환한다 ([cfe_sb_priv.c L1091-L1097](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_priv.c#L1091-L1097), S12).
- **[확인된 사실]** 이 event의 `CFE_EVS_FIRST_4_STOP` filter 하나를 모든 MID와 발신자가 공유한다 ([cfe_sb_internal_cfg.h L221-L243](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/inc/cfe_sb_internal_cfg.h#L221-L243)). EVS는 호출 task의 AppId로 event type mask를 확인한다. 그래서 EVS에 등록하지 않은 발신 앱의 SB event는 나오지 않는다 ([cfe_evs_utils.c L190-L240](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/evs/fsw/src/cfe_evs_utils.c#L190-L240), [cfe_evs_task.c L1463-L1485](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/evs/fsw/src/cfe_evs_task.c#L1463-L1485)).
- **[확인된 사실]** SBR은 경로를 지우지 않는다 ([cfe_sbr_route_unsorted.c L72-L114](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sbr/fsw/src/cfe_sbr_route_unsorted.c#L72-L114)). 구독 해지나 pipe 삭제로 목적지가 0개가 된 경로로 발행하면 counter도 event도 없이 `CFE_SUCCESS`다.
- **[확인된 사실]** `CFE_SB_Qos_t`는 "Currently an unused parameter"로 문서화되어 있다 ([default_cfe_sb_extern_typedefs.h L114-L125](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/config/default_cfe_sb_extern_typedefs.h#L114-L125)). 확인한 SB·SBR 코드에는 늦은 구독자를 위한 보존·latch 기능이 없다. 이 판단은 코드 읽기 범위에 한정된다.
- **[실측]** 한 번도 구독되지 않은 MID로 보내면 `NoSubDelta=1`이었다. 구독 후 해지한 MID로 보내면 `NoSubDelta=0`이었다. 두 경우 모두 rc=0이었다 (5회 실행 동일; `$N/sem-sb/probe_runs/SBPROBE_lines_all_runs.txt` L3-4).
- **정상 대조.** **[확인된 사실]** TO_LAB은 table 기반 telemetry 구독을 `CFE_ES_WaitForStartupSync` 뒤로 미룬다. 명령 MID 구독은 그 전에 `TO_LAB_init`에서 한다. 이 지연은 commit d3d52da(2026-04-21)에서 들어왔고 tag 중에서는 v7.0.1에만 있다. **[실측]** 기본 bundle 8회 실행 모두에서 OPERATIONAL 이전에 `No subscribers for MsgId 0x808`(`CFE_EVS_LONG_EVENT_MSG_MID`)이 정확히 4번 기록되었다. 4는 filter 상한이므로 실제 유실 수는 알 수 없다. 4건 중 앞의 3건은 CORE_READY 이전이라 TO_LAB의 지연과 무관하다. 소스 주석, 발신자, filter 상한의 상세는 §7.4.3에 있다.
- **[해석]** 이 사례는 의도된 첫 메시지 유실이다. F1 검사가 이를 결함으로 경고하면 안 된다.
- **공개 기록.** **[확인된 사실]** 조사한 issue 범위에서 '구독 준비 전 발행'으로 실제 기능 오류가 난 공개 보고는 찾지 못했다. 검색 질의와 페이지 수가 제한된 결과다 (`$N/issues/search_log.tsv`). **[해석]** F1의 양성 사례는 합성·주입으로 만들어야 한다.

#### F2 — 메시지 유래 state의 첫 갱신 전 사용

- **[해석]** 메모리 초기값과 유효 입력 수신을 구별한다 (수정본 §6.2). 앱이 기본값 사용을 허용하면 `w` 이전의 `r`은 결함이 아니다. guard flag(수정본 §6.1의 `attitude_valid`)나 HK의 `DataPresent`가 있으면 그 guard의 의미를 먼저 확인한다.
- **[확인된 사실]** 공개 기록에는 table에서 오는 state를 load 전에 사용한 사례가 있다. HS #148 [S50]은 AppMon table이 load되지 않은 상태에서 명령을 받으면 NULL table 포인터를 역참조했다 (fix b7530d9 [S51]). MD #79 [S52](open)는 file에서 load한 dwell table을 `MD_AppData.MD_DwellTables[]`로 복사하지 않는다 ([md_app.c L423-L449 @65eb7b3](https://github.com/nasa/MD/blob/65eb7b3b0aa8acd05076128a623cd696582b6d7c/fsw/src/md_app.c#L423-L449)).
- **[해석]** 두 사례는 실행 순서와 무관한 결정적 결함이다. 분석기가 보고하더라도 '순서 결함' 집계에서는 분리한다.
- **[확인된 사실]** 메시지에서 오는 state의 공개 사례는 적다: LC #8 [S53], cFE #2739 [S54]. **[미확인]** 두 issue의 코드 경로는 확인하지 않았다.

#### F3 — 갱신 경계를 넘은 state 사용

- **[해석]** 경계 β를 정하지 않으면 F3을 판정할 수 없다. 이 연구는 β를 먼저 이벤트 경계로 정의한다. 예: "사용 `u` 직전에 받은 trigger 메시지 이후 `f`가 한 번 이상 갱신되어야 한다". 물리 시간 β는 1.4절에서 범위 밖으로 둔다.
- **[확인된 사실]** 기본 `CFE_MSG_OriginationAction`은 IsOrigination=true인 telemetry의 header 시각을 전송 시점의 `CFE_TIME_GetTime()`으로 덮어쓴다 ([cfe_msg_integrity.c L30-L53](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/msg/fsw/src/cfe_msg_integrity.c#L30-L53)). **[해석]** header 시각을 측정 시각으로 쓰면 원노트 Property 4의 timestamp guard 판정이 틀릴 수 있다. payload 안의 시각 field를 따로 추적해야 한다. mission별 MSG override는 확인하지 않았다.
- **[확인된 사실]** TIME은 시작 시 `ClockSetState=NOT_SET`으로 초기화된다 ([cfe_time_utils.c L231-L315](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/time/fsw/src/cfe_time_utils.c#L231-L315)). **[해석]** 초기 구간의 timestamp 비교는 clock 상태 검사와 함께 봐야 한다.

#### F4 — 다중 stream snapshot 불일치

- **[해석]** 앱의 main task가 pipe에서 메시지를 하나씩 꺼내 handler를 끝까지 실행한다고 하자. 그러면 `(A_k, B_j)` 조합이 생겨도 한 code block 안의 interleaving은 없다. 이 경우 TaxDC의 atomicity violation, AVIO의 unserializable interleaving, Burrows–Leino의 stale-value 정의는 바로 적용되지 않는다. child task가 같은 state를 쓰는 앱에서는 이 전제가 깨진다.
- **[미확인]** Artho·Havelund·Biere의 high-level data race(view consistency) 원문을 읽지 못했다. 실시간 cause-effect chain의 time-disparity 문헌도 검색하지 않았다. 따라서 F4를 '기존 정의가 없는 범주'라고 주장하지 않는다.
- **정상 대조 1.** **[확인된 사실]** HK는 입력마다 `DataPresent` flag를 두고, 결합 packet에 누락이 있으면 `MissingDataCtr`를 올린다. 기본값 `HK_DISCARD_INCOMPLETE_COMBO`=0이면 불완전한 결합 packet도 그대로 보낸다. 누락 event는 DEBUG type이라 기본 EVS type mask 0xE에서 꺼져 있다 (상세 §3.7). **[해석]** 이는 설정된 정책이다. F4 경고 대상이 아니다.
- **정상 대조 2.** **[확인된 사실]** Ogma 예제 `cfs-002-state-machines`에서 `state`는 active, `input`은 passive 입력이다. 생성 템플릿은 메시지 값을 전역에 복사하고, active 입력일 때만 `copilot_step()`을 부른다 (R09·S24 관련; 템플릿 상세와 검증 이견은 §4.1.1). **[해석]**(이견 있음) passive 입력은 다음 active 입력이 올 때까지 마지막 값으로 남는다. latest-value 조합을 설계로 허용한 사례다. "Ogma 생성 앱은 일반적으로 여러 stream의 latest value를 결합한다"로 넓히지 않는다. README의 예제 호출은 `position` 하나만 감시한다 (검증 C06, contested).
- **[확인된 사실]** ROS 2는 이 문제를 라이브러리로 다룬다. `message_filters`의 timestamp synchronizer와 rclc의 LET 의미가 있다 ([rclc executor.h L43-L59](https://github.com/ros2/rclc/blob/3064baadeabdaca6dabfae3f8351510bdbe53071/rclc/include/rclc/executor.h#L43-L59)). **[해석]** cFS에서는 같은 역할을 앱이 손으로 쓴 guard가 맡는다. 분석기는 라이브러리 호출이 아니라 그런 guard를 인식해야 한다.

#### F5 — 시작 시 서비스·자원 의존

- **[확인된 사실] core 앱은 직렬화되어 있다.** `CFE_ES_CreateObjects`는 core-app task를 만들기 전에 모든 core module의 EarlyInit을 호출한다. 이어서 core task를 하나씩 만들고, 각 task가 `CFE_PLATFORM_CORE_MAX_STARTUP_MSEC`(기본 30000) 안에 RUNNING이 되지 않으면 `CFE_PSP_Panic`을 호출한다 ([cfe_es_start.c L767-L876](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_start.c#L767-L876)). 상세는 §3.3 ES-2. **[해석]** cFE #73의 crash 경로는 이 commit에서 닫혀 있다. #73은 현재 코드의 양성 사례가 될 수 없다.
- **[확인된 사실] startup script 앱의 동기화는 soft limit이다.** ES는 모든 앱의 LATE_INIT과 RUNNING을 각각 `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC`(기본 1000) 동안 기다린다. 실패하면 syslog만 쓰고 다음 상태로 간다 ([cfe_es_start.c L204-L229](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_start.c#L204-L229)).
- **[확인된 사실]** `CFE_ES_WaitForSystemState`는 기다리기 전에 호출자 자신의 AppState를 올린다. 비core 앱이면 OPERATIONAL은 RUNNING으로, APPS_INIT은 LATE_INIT으로 올린다. `CFE_ES_WaitForStartupSync`는 상태를 버리는 `void` wrapper다. header의 "Lower values will be rounded up" 설명은 구현되어 있지 않다 ([cfe_es_api.c L514-L619](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_api.c#L514-L619), S11; [cfe_es.h L440-L444](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_es.h#L440-L444), S08). 상태 반환을 요청한 #1466 [S39]은 열려 있다. 이를 구현한 PR #2273 [S40]은 승인 후 병합되지 않고 2026-07-07에 닫혔다.
- **[실측]** probe 앱 하나를 2.5 s 늦게 시작시켰다. ES는 'Startup Sync failed'를 두 번 쓰고 OPERATIONAL로 갔다. 다른 probe 앱의 `WaitForSystemState(OPERATIONAL,5000)`은 약 1.95–2.01 s 뒤 `CFE_SUCCESS`를 반환했다. 그 시점에 늦은 앱은 아직 어떤 ES 호출도 하지 않았다. 네 가지 scheduling 설정에서 각 1회, 반복 20회 모두 같았다 (`$N/sem-es-tbl/runs/probe_results_summary.txt` L66-80, L240-254, L415-427, L590-604).
- **[실측]** 기본 bundle에서 앱의 초기화 완료 순서는 실행마다 달랐다. 짧은 실행 6회에서 6개의 서로 다른 순서가 나왔다. 명령: `$N/probe/runs/*.log`에서 앱별 'Initialized' event를 순서대로 뽑아 `sort | uniq -c` → 서로 다른 6줄. CI_LAB은 startup script에서 두 번째로 load되지만 완료 event는 5–10번째였다.
- **[확인된 사실] cFE #73 [S01].** Microblaze에서 우선순위 60인 TIME이 먼저 실행되어 생긴 역사적 core startup 결함이다. 범위를 벗어난 index는 SB의 AppId가 아니라 EVS 자신의 sentinel `EVS_AppID = 0xFFFFFFFF`였다. 수정 전 소스(cFE 6.4.1, 2014-12-12)와 수정 후 소스(6.4.2, 2015-07-13)가 SourceForge에 공개되어 있다. 실패 경로, 6.4.2의 수정 내용, SB AppId 초기값 불일치가 고쳐지지 않은 점은 §2.2에 정리했다. **[미확인]** 수정본 §2.2의 Trac 댓글 날짜(2015-05-06, 2015-06-16)는 댓글을 읽지 못해 다시 확인하지 못했다.
- **현재의 잔여 사례.** **[확인된 사실]**·**[실측]** open issue #2663(2025-08-06)은 ES·EVS 초기화 중 SB API가 아직 0인 SB AppId로 EVS를 불러 `CFE_EVS_APP_ILLEGAL_APP_ID`를 받고, 그 반환을 버린다고 보고한다. 검증 중 gdb로 이 반환을 실행당 24번 관측했다 (§2.2.4). **[해석]** #73과 같은 구조의 사례가 현재 코드에서 crash 없이 조용히 실패하는 형태로 남아 있다.
- **[확인된 사실] 앱 사이의 사례.** cFE #198 [S38]: EVA CWS 앱이 다른 앱의 늦은 초기화가 끝나기 전에 요청을 받았다. 수정 전 6.5.0a와 수정 후 6.6.0a(APPS_INIT, LATE_INIT, `CFE_ES_WaitForSystemState` 추가)가 모두 공개되어 있다. CF #184 [S43]: CF가 다른 앱이 만드는 throttle semaphore에 초기화 중 연결한다. 수정 833fdbb [S44]는 `OS_CountSemGetIdByName`을 100 ms 간격으로 25번까지 재시도한다. 소유 앱이 재시작해 ID가 바뀌는 경우는 다루지 않는다.

#### F6 — Table 수명 프로토콜 위반

- **[확인된 사실]** 잠금은 access descriptor(handle) 단위다. release 없이 `CFE_TBL_GetAddress`를 다시 부르면 보호 대상이 새 활성 buffer로 옮겨 간다 (§3.4 TBL-2). **[실측]** double-buffered table에서 sharer가 해제하지 않은 첫 포인터는 owner의 다음 Load 뒤 값 1 대신 3을 읽었다. 네 가지 설정 모두 같았다 (`$N/sem-es-tbl/runs/probe_results_summary.txt` L122-137).
- **[확인된 사실]** 한 번도 load되지 않은 table에서 `CFE_TBL_GetAddress`는 `CFE_TBL_ERR_NEVER_LOADED`(0xcc000005)와 `*TblPtr = NULL`을 반환하고 lock을 걸지 않는다. header [S10]는 "0 내용의 유효한 포인터를 반환하며 해제해야 한다"고 쓴다. 이 불일치는 v7.0.0에도 있다. v6.7.0의 구현은 내부 buffer를 쓰는 table에 한해 header와 일치했다 (§3.4 TBL-1, §3.6).
- **[확인된 사실]** buffering 방식에 따라 갱신 동작이 다르다. single-buffered table은 load가 `CFE_TBL_INFO_TABLE_LOCKED`로 보류되었다가 release 후 Manage나 Update에서 적용된다. double-buffered table은 load가 진행되고 보유자는 옛 내용을 계속 본다. 그다음 load는 `CFE_TBL_ERR_NO_BUFFER_AVAIL`로 거절될 수 있다 (§3.4 TBL-4, TBL-5).
- **[해석]** 공개 기록의 다수는 단일 앱 API 오용이다. 예: sample_app #101 [S58]은 `CFE_TBL_INFO_UPDATED` 경로에서 release를 빠뜨렸다 (수정 61f657d [S60]). F6는 '순서 결함'과 'API 프로토콜 오용'을 나누어 집계한다. 앱 간 공유 table의 경우는 F8과 겹친다.

#### F7 — 메시지 간 도착 순서

- **[확인된 사실]** `CFE_SB_TransmitMsg`는 SB mutex 아래에서 목적지 목록을 head(마지막 구독자)부터 고르고, mutex를 푼 뒤 목적지마다 `OS_QueuePut`을 부른다. 우선순위가 높은 구독 task는 전송 호출이 반환되기 전에 실행될 수 있다 (§3.1 SB-3·SB-4; S09, S12).
- **[확인된 사실]** mutex 밖에서 put하는 구조는 cFE commit 550e7f7d(2024-02-26, tag 중 v7.0.0에 처음 포함)에서 생겼다. 그 이전 버전은 SB mutex를 쥔 채 `OS_QueuePut`을 불렀다 (§3.6). **[해석]** F7 판정은 cFE 버전을 매개변수로 가져야 한다.
- **[확인된 사실]** Linux POSIX OSAL에서는 pipe마다 FIFO이고, queue가 가득 차면 메시지를 버린다. pipe 사이의 전역 순서는 없다 (§3.2 OS-1). **[미확인]** VxWorks queue의 순서 의미는 확인하지 않았다.
- **[실측]** 1 CPU·SCHED_RR에서는 송신자보다 우선순위가 높은 수신자 3개가 전송 호출 반환 전에 모두 실행되었다(200/200, 2회). 관찰자 pipe가 먼저, relay pipe가 나중에 구독한 설정에서 relay가 송신자보다 높으면 관찰자가 relay의 출력을 원 메시지보다 먼저 꺼냈다(500/500, 2회). relay가 낮으면 0/500, 4 CPU에서는 두 설정 모두 0/500, 우선순위가 무시된 실행에서는 15/500·30/500이었다 (§3.1 SB-3·SB-4; `$N/sem-sb/probe_runs/SBPROBE_lines_all_runs.txt`).
- **[해석]** 이 숫자는 하나의 Linux 환경에서 관측한 것이다. 발생률이 아니다. 4 CPU의 0/500은 불가능을 뜻하지 않는다.

#### F8 — 재시작 창

- **[확인된 사실]** `CFE_ES_RestartApp`은 요청만 기록한다. ES background scan이 나중에 `CFE_ES_CleanUpApp`과 `CFE_ES_AppCreate`를 실행하고 새 AppId를 준다. cleanup은 SB pipe를 모든 경로에서 빼고 queue의 메시지를 버리며, TBL descriptor를 해제하고 앱이 소유한 table을 무소유로 표시한다. SB 경로와 그 sequence counter는 남는다 (§3.3 ES-9·ES-10, §3.1 SB-12, §3.4 TBL-7).
- **[해석]** 재시작 창 동안 다른 앱의 발행은 목적지 0개 경로로 조용히 사라진다. F1과 같은 기전이다. 다른 앱이 보관한 옛 AppId는 무효가 된다.
- **[실측]·[확인된 사실]** 재시작 요청에서 새 instance 진입까지 1.57–1.63 s 걸렸다. owner의 table을 다른 앱이 share하고 있으면 새 instance의 `CFE_TBL_Register`는 sharer의 descriptor가 해제될 때까지 `CFE_TBL_ERR_DUPLICATE_NOT_OWNED`(0xcc00000d)를 받았다. cleanup 이전 약 1.5 s 동안 sharer는 옛 포인터를 그대로 받았다 (`$N/sem-es-tbl/runs/probe_results_summary.txt` L144-157, 네 가지 설정 모두). 이 동작은 v7.0.0 'cFS Draco' commit c1ab1b7 이후의 것이고, 같은 commit의 TBL FAQ(`docs/src/cfe_tbl.dox` L333-339)는 여전히 옛 동작을 설명한다 (§3.4 TBL-7, §3.6; `$N/verify_C15_counter/tbl_name_clearing_by_version.txt`).
- **[해석]** 원노트 §24-D의 'cross-application stale handle'이 소스와 실행으로 뒷받침된다. 다만 관측된 증상은 sharer의 옛 값 읽기가 아니라 owner의 재등록 실패였다.

---

### 1.3 실행 가정 Σ

**[설계 제안]** 실행 가능성(조건 3)은 아래 매개변수에 따라 달라진다. 모든 보고에 Σ를 기록한다. Σ가 다른 결과는 합치지 않는다.

| 매개변수 | 판정에 주는 영향 | 근거 |
| --- | --- | --- |
| cFE 버전 | TransmitMsg의 lock 구조(550e7f7d, v7.0.0), TBL 재등록 동작(c1ab1b7, v7.0.0), TO_LAB 지연 구독(d3d52da, v7.0.1)이 버전마다 다르다 | **[확인된 사실]** F7·F8·F1 항목 |
| 우선순위 적용 여부 | 권한이 없으면 OSAL POSIX는 permissive mode에서 우선순위 없이 SCHED_OTHER로 조용히 떨어진다. native sample config는 permissive mode를 켠다 | **[확인된 사실]** [os-impl-tasks.c L333-L446](https://github.com/nasa/osal/blob/dad0ee99c121f2ecc11f8c0798c1e836e41d8ab1/src/os/posix/src/os-impl-tasks.c#L333-L446), [native_osconfig.cmake L22-L39](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/cmake/sample_defs/native_osconfig.cmake#L22-L39) |
| CPU 수·affinity | 같은 코드에서 relay 역전이 500/500(1 CPU)과 0/500(4 CPU)으로 갈렸다 | **[실측]** F7 |
| 실효 pipe 깊이 | 비root 실행에서 OSAL은 queue 깊이를 `msg_max`(이 container에서 10)로 자른다. 깊이 20을 요청한 pipe에서 10개만 받고 10개가 overflow되었다 | **[확인된 사실]** [os-impl-queues.c L97-L129](https://github.com/nasa/osal/blob/dad0ee99c121f2ecc11f8c0798c1e836e41d8ab1/src/os/posix/src/os-impl-queues.c#L97-L129); **[실측]** `$N/sem-sb/probe_runs/run4_nonroot_cpu0_msgmax10.log` |
| MID 매핑 | EDS build에서는 `CFE_PLATFORM_CMD_TOPICID_TO_MIDV`가 `CFE_SB_LocalCmdTopicIdToMsgId` 호출이 된다. 이 함수는 `CFE_PSP_GetProcessorId()`에 의존한다 | **[확인된 사실]** [eds_cfe_core_api_msgid_mapping.h L37-L66](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/config/eds_cfe_core_api_msgid_mapping.h#L37-L66) |
| table 이미지 | HK copy table, TO_LAB subscription table, SCH_LAB schedule table이 구독·발행 MID를 정한다. sch_lab 저장소의 기본 table은 비어 있고, bundle은 `sample_defs/tables/sch_lab_table.c`를 쓴다 | **[확인된 사실]** [hk_utils.c L316-L324](https://github.com/nasa/HK/blob/0dc16b7a7bab71747b9c063cf405ad59a65a40a5/fsw/src/hk_utils.c#L316-L324), [sch_lab_table.c (bundle)](https://github.com/nasa/cFS/blob/5a9b075cd4c818ee8555f349a9f78ba5632d0384/sample_defs/tables/sch_lab_table.c#L55-L79) |
| 지상 명령 | TO_LAB AddPacket 명령이 구독을 더한다. SB route disable 명령이 목적지를 조용히 건너뛰게 한다 | **[확인된 사실]** [to_lab_cmds.c L216-L219](https://github.com/nasa/to_lab/blob/d27c6014cb5d2979140500f635876ee87b81995b/fsw/src/to_lab_cmds.c#L216-L219), [cfe_sb_task.c L440-L468](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_task.c#L440-L468) |
| startup script | load 순서는 완료 순서가 아니다 | **[실측]** F5 |

---

### 1.4 범위에서 제외하는 것

| 제외 대상 | 제외 이유 | 다루는 방식 |
| --- | --- | --- |
| 공유 메모리 data race. 예: cFE #950 [S68](exit·cleanup race), #2523(EVS counter mutex, dd596c96 [S81]), #1034 [S82](`SystemState`를 volatile로) | **[원노트 구상]** §25·§65 기준. 일반 C race detector의 대상이다 | **[설계 제안]** Goblint·ThreadSanitizer [S20]를 기준선으로 겹치는 범위만 비교한다 |
| cFE 내부 SB routing race (#1073 [S79], 수정 b17cd1e [S80]) | **[해석]** 앱 간 분석은 SB routing이 옳다고 가정한다. 이 결함은 그 가정 안쪽에 있다 | 대조 사례로만 쓴다 |
| driver·interrupt race, lock-free 알고리즘, MPI message matching | **[원노트 구상]** §25 | 다루지 않는다 |
| WCET, schedulability, priority inversion 검증 | **[원노트 구상]** §25·§59 | 우선순위는 Σ의 입력으로만 쓴다 |
| SBN과 다중 노드, 다중 cFE instance | **[원노트 구상]** §59, 수정본 §3.3. **[확인된 사실]** SBN(GSC-16917-1)은 peer 간 subscription database를 다루는 확장이다. 로컬 SB 명세가 아니다 (NASA Software Catalog 2025-26, 인쇄 p.207; `$N/factcheck/catalog_2025-26.txt` L8968-8972) | 확장 과제로 남긴다 |
| 다중코어 memory model | **[원노트 구상]** §59 | 다루지 않는다 |
| 물리 시간 기반 freshness 한계 (원노트의 20 ms·100 ms 등) | **[확인된 사실]** data age 분석은 주기·응답시간·통신 의미를 입력으로 요구한다 (R07, §II–IV). **[해석]** cFS의 `ReceiveBuffer` 시점 읽기는 implicit·LET 통신 모델과 다르다 | F3·F4는 이벤트 경계로만 정의한다 |
| 보안·인증 (예: SB 명령 무인증) | **[미확인]** 찾은 2022–2026의 cFS 분석 문헌은 주로 보안·신뢰 경계를 다룬다. 예: arXiv 2608.14532 [R64], SpaceSec 2024 [R62]. 모두 검색 snippet 수준이다 | 다루지 않는다 |
| 순서와 무관한 결정적 결함 (MD #79, HS #148) | **[해석]** 조건 3을 만족하지 않는다 | 보고는 하되 순서 결함 집계에서 뺀다 |
| 수신 buffer·zero-copy buffer의 수명 위반 | **[확인된 사실]** 다음 `ReceiveBuffer` 이후, 또는 `TransmitBuffer` 성공 이후의 사용은 수명 규칙 위반이다 ([cfe_sb.h L440-L479, L530-L634](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb.h#L530-L634)) | 별도의 수명 검사로 처리한다. 순서 결함과 섞지 않는다 |
| EDS build | MID가 runtime 함수의 결과다 (1.3절) | 1차 범위 밖으로 둔다 |
| RTEMS·VxWorks에서의 실행 | **[미확인]** queue 의미는 OSAL 코드만 읽었다. 실측은 Linux뿐이다 | Σ에 플랫폼을 기록하고 일반화하지 않는다 |

---

### 1.5 용어: 기존 문헌 정의와의 대응

| 용어 | 출처의 정의와 확인 범위 | 대응 범주 | 그대로 옮길 수 없는 점 |
| --- | --- | --- | --- |
| happened-before | **[확인된 사실]** 같은 실행 주체의 순서, 같은 메시지의 send–receive, 추이성으로 정의한다 (R01, pp. 558–560) | `G`의 이론적 근거 | **[해석]** cFS에서 send–receive 간선은 `enq`가 아니라 성공한 `rcv`에만 생긴다. `drop`된 메시지에는 간선이 없다 |
| order violation | **[확인된 사실]** TaxDC §3.1: 메시지가 다른 이벤트(메시지 또는 로컬 계산)보다 먼저(늦게)오면 결함이 나타나고, 반대 순서면 나타나지 않는다. order violation만으로 생긴 결함은 104개 중 46개(44%)다 (TaxDC [R12], 전문). PCT는 '초기화 전 접근'을 depth 1 ordering bug의 예로 든다 (PCT [R25], Fig. 1(a)). **[미확인]** Lu 등(2008)은 초록만 확인했다: 조사한 non-deadlock 결함의 약 1/3이 프로그래머의 순서 의도 위반이다 (MSR 서지 [R24]) | F1, F2, F5, F7, F8 | **[확인된 사실]** TaxDC의 대상은 shared-nothing 노드이고, 한 노드 안의 thread interleaving 결함(LC)은 따로 분류한다. 저자들은 통계를 일반화하지 말라고 쓴다. **[해석]** cFS 앱은 한 cFE instance 안의 task이므로 F 범주는 TaxDC 범주의 유사 사례이지 그 사례 자체가 아니다. 44% 같은 비율은 flight software에 옮기지 않는다 |
| atomicity violation | **[확인된 사실]** TaxDC: 메시지가 한 묶음의 이벤트 중간에 도착할 때 나타나는 결함이다 (20%). **[미확인]** Flanagan–Qadeer의 type system과 AVIO는 검색 요약만 보았다 | child task가 같은 state를 쓰는 F4 | **[해석]** run-to-completion handler에서는 중간 도착이 없다 (F4 항목) |
| high-level data race (view consistency) | **[미확인]** Artho·Havelund·Biere, STVR 13(4), 2003. 검색 요약에 따르면 원자적으로 접근해야 하는 관련 field 집합의 일관성을 다룬다. 원문은 열지 못했다 (저자 목록 [R22]) | F4의 참조 개념 | 적용 여부를 판단하지 않는다. 원문 확인 전에는 인용만 한다 |
| stale-value error | **[확인된 사실]** Burrows–Leino: 한 critical section에서 읽은 지역 변수 값을 같은 thread가 뒤의 critical section에 들어간 뒤(`wait()` 재획득 포함) 사용하는 오류다. 나간 직후의 사용은 'justifiable'로 보고 경고하지 않는다. ESC/Java 확장이 지역 변수마다 ghost boolean `stale_t`·`from_critical_t`를 두며, 주석 없이 617 kloc에서 경고 48개(오경고 43, 무해 race 1, 결함 4)를 냈다 (krml107 원고 [R13], 전문; CCPE 16, 2004, pp. 1161–1172). **[미확인]** 논문 DOI 10.1002/cpe.866은 해석하지 못했다 | F3의 일부 | **[해석]** 여기서 stale은 critical section 경계를 넘었다는 뜻이다. 물리 시간이나 생산자의 새 값 발행 여부가 아니다. cFS state는 지역 변수가 아니라 앱 전역 변수에 있다 |
| message race | **[확인된 사실]** R05는 비동기 활동에 도착하는 메시지 사이의 HB를 정적으로 추론한다. 종료성·건전성 증명은 후속 과제로 남긴다 (§3.8). R11은 send·deliver·receive를 구별하고, 잠재 race가 실제 race가 되려면 matching 조건이 필요하다고 쓴다. **[미확인]** Netzer–Miller의 feasible/apparent race 구분은 검색 요약만 보았다 | F7 | **[해석]** 이 연구의 '후보'는 apparent race 수준이다. 실행 가능성(조건 3)을 따로 확인해야 한다 |
| event race | **[확인된 사실]** EventRacer는 계측된 브라우저의 trace를 쓰는 동적 분석이다 (README @07ced1cc [S134]). P 언어는 상태마다 deferred·ignored event 집합을 두고, 둘 다 아닌 event가 오면 'unhandled event'로 보고한다 (MSR-TR-2012-116 [R26], §2). **[미확인]** InitRacer의 late-event-handler-registration, SIERRA·nAdroid의 정적 lifecycle 모델은 검색 요약만 보았다 | F1 (늦은 handler 등록 ≈ 늦은 구독), `Req`의 계약 형식 | **[해석]** cFS에는 deferred·ignored 선언이 없다. SB는 구독된 pipe에만 조용히 전달한다. 앱별 `must_handle / may_drop / deferred` 계약은 P를 본뜬 **[설계 제안]**이다 |
| late joiner / first-message loss | **[확인된 사실]** ZeroMQ guide는 PUB–SUB에서 구독자가 첫 메시지를 항상 놓치는 'slow joiner'를 설명한다 ([chapter1.txt L215-L224](https://github.com/booksbyus/zguide/blob/6752d24b215997aa161e6896941bfd6010d4d3f5/chapter1.txt#L215-L224)). ROS 2는 TRANSIENT_LOCAL durability로 늦게 붙는 구독자에게 sample을 보존한다 ([QoS 문서 L48-L61](https://github.com/ros2/ros2_documentation/blob/e2388aa72c17598a54fa228df6e9e9a44c5e7aec/source/ROS-Framework/interfaces/topics/About-Quality-of-Service-Settings.rst#L48-L61)) | F1, F8 | **[해석]** ROS·DDS에서 F1은 일부 설정 문제다. cFS에는 durability라는 설정 자체가 없으므로 순서와 앱의 복구 관용구로 판정해야 한다 |
| data age, reaction time | **[확인된 사실]** R07이 cause-effect chain의 두 시간 특성을 따로 정의한다 (preprint §II–IV). 제3자 학위논문 요약에 따르면 Becker 등의 분석은 주기 task와 implicit 통신을 가정하고, explicit 통신을 가정한 분석은 거의 없다 (E2EEvaluation README [S138], 2차 자료) | F3·F4의 시간 버전 | 1.4절에서 범위 밖. **[해석]** 이 연구가 복원한 chain 구조와 SCH 주기는 나중에 이런 분석의 입력이 될 수 있다 |
| "known race condition" | **[확인된 사실]** Swift 임무팀 Circular의 표현이다 (S03–S06). GCN 22706만 "incorrect spacecraft attitude information to be applied to a known source"라고 쓴다 (검색 snippet 수준; 원 페이지는 egress 차단) | 동기 사례 | 1.7절 참조. 내부 기전을 추정하지 않는다 |
| Flight-Software-Specific Cross-Application Race | **[원노트 구상]** 원노트 §2의 이름 | 연구 범위 전체 | **[해석]** 표준 정의가 아니다 (수정본 서두). 범위 이름으로만 쓴다 |

**[설계 제안] 용어 사용 규칙.**

- 논문 전체의 대상은 'cFS 앱 간 순서·시간 의존 결함(cross-application ordering / temporal-dependency fault)'으로 부른다.
- 'race'는 실행 순서에 따라 결과가 바뀌는 F1·F5·F7·F8 사례에만 쓴다. 순서와 무관하게 재현되는 F2·F6 결함에는 쓰지 않는다.
- 정적 분석 결과는 '후보'로 부른다. '검출'은 재현 수준에만 쓴다.
- TaxDC의 'Global missing messages'를 F1과 같은 말로 쓰지 않는다. **[확인된 사실]** TaxDC §4.1에서 이 증상은 트리거 노드가 다른 노드가 기다리는 응답을 보내지 않는 경우다 (9%). 전송 중 유실이 아니다.

---

### 1.6 연구 질문과 검증 가능한 가설

**[설계 제안]** 원노트 Q1–Q5와 H1–H3를 다시 쓴다. 각 질문에 측정 단위, 지지 증거, 반박 증거, 현재 근거를 붙인다. 아래 '기준'은 결과가 아니라 실험 전에 정하는 판정 규칙이다.

#### RQ1 — 앱 간 publish/subscribe 관계를 얼마나 복원하는가 (원 Q1·H1)

- **질문.** 고정 bundle에서 소스, build 설정, table 이미지를 입력으로 주면, 실행 시 구독 집합을 얼마나 재현하는가?
- **측정 단위.** (앱, MID, pipe) 구독 간선과 (앱, MID) 발행 간선. 간선마다 출처를 `code-literal / table / command-dynamic / EDS-runtime`으로 표시한다.
- **정답 자료.** **[설계 제안]** SB subscription reporting을 켜고 실행 중 구독 보고와 `CFE_SB_SEND_PREV_SUBS_CC` 응답을 모은다 ([cfe_sb_fcncodes.h L432-L462](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/inc/cfe_sb_fcncodes.h#L432-L462)). **[실측]** 기본 bundle에서 TO_LAB의 구독 보고 16건 이상이 SBN pipe의 MsgLim(16)이나 overflow로 버려졌다 (`$N/probe/run_nobody.log`, `run_root_ipcns.log`). 따라서 보고는 MsgLim이 큰 전용 pipe로 받아야 한다.
- **지지 기준.** `code-literal`과 `table` 간선의 누락이 0이다. `command-dynamic` 간선은 빠뜨리지 않고 '동적'으로 따로 보고한다.
- **반박 기준.** SSA 정리 후에도 IR에서 복원할 수 없는 `code-literal` 간선이 있다. 이 경우 IR 단독 경로를 포기하고 AST 보조 경로를 더한다.
- **현재 근거.** **[실측]** 코드에서 정의한 MID는 `-O1 -Xclang -disable-llvm-passes`와 MLIR SSA 정리 뒤 call site의 상수 operand가 된다. 상수 회수/전체 site는 sample_app 3/3, hk 4/7, sch_lab 1/2, ci_lab 4/4, to_lab 3/7이다 (§7.7.3; to_lab은 검증에서 3/7로 정정). `apps/*/fsw/src`의 `CFE_SB_Subscribe*` 호출 47곳 중 8곳은 table·명령·설정 구조체에서 MID를 받는다 (§3.7). **[확인된 사실]** IR에는 MID macro 이름이 남지 않는다. Clang AST에는 literal의 spelling 위치가 남는다 (§7.7.3).
- **신규성 주의.** **[확인된 사실]** 소스에서 pub/sub topology를 복원하는 작업은 선행 사례가 있다. ROSDiscover(ICSA 2022)는 ROS C++에서 port를 복원하고 구조 규칙만 검사한다 (§4.2.1). SPLC 2009(Ganesan 등)는 CFS 구현을 개발자 가이드의 아키텍처 규칙에 대조했다 (초록·snippet 수준, §4.1.1). **[해석]** RQ1의 결과는 기반 작업이며 그 자체로 기여가 아니다.

#### RQ2 — payload field에서 state field, 사용 위치까지 추적하는가 (원 Q2, 원노트 §44)

- **질문.** `rcv(b,p,m)` → handler → `w(b,f,payload(m).g)` → 다른 함수의 `r(b,f,u)`를 함수 사이에서, field 단위로 복원하는가?
- **측정 단위.** (사용 위치, field, 출처 집합). 원노트 §44의 패턴과 5개 lab 앱(sample_app, hk, sch_lab, ci_lab, to_lab)에 수작업 label을 단다.
- **지지 기준.** 실측에서 놓친 세 패턴을 포함해 label된 출처를 복원한다. 세 패턴은 offset 0 field, API out-parameter 쓰기, 지역 포인터를 통한 쓰기다. 수정본 §6.4의 가짜 조합(`(k,j)`)을 만들지 않는다.
- **반박 기준.** 대부분의 사용 위치가 앱별 수작업 주석 없이는 출처를 얻지 못한다.
- **현재 근거.** **[실측]** debug info를 켠 import에서 전역 field store 83곳을 찾았다. 모두 file:line을 가졌다. 세 패턴(offset 0 field, API out-parameter 쓰기, 지역 포인터를 통한 쓰기)은 놓쳤고 sch_lab에서는 0곳이었다 (§7.8). upstream MLIR의 `LocalAliasAnalysis`는 서로 다른 전역을 MayAlias로, 한 struct의 서로 다른 field를 MustAlias로 판정한다. 외부 `llvm.call`은 모든 위치에 대해 ModRef다. 테스트 분석 `-test-last-modified`는 §44 예제에서 `<unknown>`을 반환한다 (§4.5.3). **[해석]** field·전역을 구분하는 메모리 모델은 연구자가 만들어야 한다.

#### RQ3 — 필요 순서의 보장 여부를 Σ별로 판정하는가 (원 Q3·H2)

- **질문.** F1·F2·F5·F7·F8의 각 `Req`에 대해 `{보장됨, Σ에서 위반 가능, 알 수 없음}`을 판정하는가? '위반 가능' 판정이 재현되는가?
- **정답 자료.** **[설계 제안]**
  - native sim 재실행을 최소 두 Σ에서 한다. Σ1은 1 CPU와 SCHED_RR, Σ2는 4 CPU와 SCHED_OTHER다. uid, `msg_max`, permissive 설정을 기록한다.
  - 수정 전후 쌍을 쓴다: cFE #198(6.5.0a/6.6.0a), CF #184(833fdbb [S44]의 parent와 수정본), #73 파생 예제(6.4.1/6.4.2 소스 기준).
  - 합성 예제를 더한다.
- **지지 기준.** '위반 가능' 후보가 하나 이상의 Σ에서 재현된다. 수정 전후 버전에서 판정이 기대한 방향으로 바뀐다.
- **반박 기준.** '위반 가능' 후보의 대부분이 어떤 Σ에서도 재현되지 않고 소스 수준 witness도 없다. 또는 수정 전후에 판정이 같다.
- **현재 근거.** **[실측]** F5·F7의 실측(초기화 순서 6회 6종, relay 역전 500/500 대 0/500, 늦은 앱이 RUNNING이 아닌 채 OPERATIONAL 진입)은 Σ가 판정을 바꾼다는 점을 보여 준다.

#### RQ4 — 시간 일관성 경고에서 설계된 허용을 구별하는가 (원 Q4·H3)

- **질문.** F3·F4 후보를 (a) 명시적 guard·정책이 있음, (b) 계약상 latest-value 조합 허용, (c) guard가 없고 계약이 일관성을 요구함으로 나누는가?
- **정상 대조 집합.** HK 결합 packet(`DataPresent`, 불완전 packet 전송 설정), Ogma `cfs-002` active/passive, TO_LAB 지연 구독.
- **지지 기준.** 정상 대조에서 경고가 0이다. guard를 지운 주입 변형에서는 경고가 나온다. field 단위 출처를 끈 ablation에서 이 구별이 사라진다.
- **반박 기준.** 대부분의 사례가 앱별 계약 없이 구별되지 않는다. 이 경우 기여는 '계약 검사'로 줄어든다.
- **현재 근거.** **[미확인]** 아직 구현·평가하지 않았다.

#### RQ5 — lifecycle 의미가 같은 모델을 쓴 기준선보다 무엇을 더 찾는가 (원 Q5)

- **질문.** 같은 SB·ES·TBL API 모델을 넣은 기준선과 비교할 때, ES·TBL lifecycle 의미 때문에만 나오는 후보가 있는가?
- **기준선.** **[설계 제안]** CodeQL global data flow에 `isAdditionalFlowStep`으로 SB 간선을 넣은 것, fprime-topo-analysis 방식의 topology·handler 결합, taint 설정을 쓴 Clang Static Analyzer, CFE stub을 붙인 IKOS. 공유 메모리 부분은 Goblint로 비교한다.
- **lifecycle 규칙 예.** `CFE_ES_WaitForStartupSync` timeout 경로 무시. `NEVER_LOADED` 뒤 NULL 사용. owner 재시작 뒤 `UNREGISTERED` 경로에 `Unregister`가 없는 sharer. 재`GetAddress` 이후 옛 포인터 사용.
- **지지 기준.** 위 규칙의 양성 사례를 기준선은 놓치고 제안 분석은 보고한다. 차이가 모델 차이가 아니라 lifecycle 의미에서 온다는 것을 ablation으로 보인다.
- **반박 기준.** 같은 모델을 넣은 기준선이 같은 집합을 찾는다.
- **현재 근거.** **[실측]** Clang Static Analyzer의 taint는 int payload에서만 전파되었다. double payload와, 다음 `ReceiveBuffer` 같은 불투명 호출 뒤에서는 `static` 전역에서도 사라졌다 (§4.5.2). **[확인된 사실]** CodeQL의 C/C++ global data flow는 전역 변수를 통해 함수 사이를 순서와 무관하게 잇는다 (§4.5.2). **[해석]** 순서 판정은 별도로 붙여야 한다.

#### RQ6 — MLIR이 같은 의미의 대안보다 무엇을 개선하는가 (원노트 §42–43)

- **질문.** 의미 모델을 고정했을 때, `cfs` dialect와 MLIR solver 구현이 AST·CodeQL 구현보다 측정 가능한 이점을 주는가?
- **측정.** 같은 사례 집합에서의 후보 집합, 모델·분석 코드 크기, 새 API(예: TBL)를 추가하는 비용, 진단의 소스 위치 완전성.
- **지지 기준.** 같은 후보 집합을 더 적은 확장 비용으로 얻는다. 또는 non-addressable resource에 대한 typed effect 덕분에 순서 판정의 정밀도가 오른다.
- **반박 기준.** 대안이 같은 결과를 더 적은 코드로 낸다. 원노트 §43의 기준대로 그 경우 MLIR은 정당화되지 않는다.
- **현재 근거.** **[실측]** `C → LLVM IR → mlir-translate --import-llvm` 경로로 154개 C 파일을 진단 0개로 import했다. 범위는 cFE의 5개 module과 17개 앱 디렉터리이고, 성공한 compile flag에는 조건이 있다 (§7.5.2). 파일별 import에서 `llvm.call`과 `llvm.load`는 모두 file:line:col을 가졌다 (§7.9). 로컬 clang에서는 ClangIR을 쓸 수 없고 `-fclangir -S -emit-llvm`은 조용히 무시된다 (§7.10). **[해석]** import 경로의 비용은 import가 아니라 의미 복원에 있다.

#### 가설의 재진술

| 가설 | 원노트 | 재진술 **[설계 제안]** | 반박 조건 |
| --- | --- | --- | --- |
| H1′ | H1 | table 이미지와 build 설정을 입력으로 받는 lifting은 고정 bundle에서 `code-literal`·`table` 구독 간선을 runtime 구독 집합과 비교해 누락 없이 복원한다 | 한 간선이라도 누락되면 기각한다. 원인을 IR 정보 손실, table 해석, 모델 누락으로 나누어 보고한다 |
| H2′ | H2 | label된 F1·F2·F5·F7·F8 사례(합성, 공개 앱 변형, 역사 파생)에서 근본 원인마다 후보가 하나 이상 나온다. 정상 대조(TO_LAB 지연 구독, HK 결합 정책, Ogma passive 입력, 직렬화된 core startup)에서는 후보가 0이다 | 정상 대조에서 후보가 나오거나, 근본 원인이 누락되면 범주별로 기각한다. 단위는 경고 수가 아니라 근본 원인이다 (수정본 §12.1) |
| H3′ | H3 | field 단위 출처를 끄면 F2·F4의 누락이 늘거나 정상 대조의 오경고가 늘어난다 | ablation 전후 결과가 같으면 기각한다. 이 경우 '출처 추적이 필요하다'는 주장을 하지 않는다 |

---

### 1.7 논문이 할 수 있는 주장과 할 수 없는 주장

**할 수 있는 주장 (조건 포함)**

| 주장 | 조건 | 근거 |
| --- | --- | --- |
| cFE 546a002의 로컬 SB에서 목적지별 전달 실패(경로 없음, 목적지 0, MsgLim, pipe full)는 발신자의 반환값에 나타나지 않는다 | 버전과 OSAL을 명시한다. 경로 없음과 목적지 0의 차이를 밝힌다 | **[확인된 사실]** F1·F7 항목, [sb_sendrecv_test.c L103-L107](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/cfe_testcase/src/sb_sendrecv_test.c#L103-L107) |
| startup script 앱에 대한 ES 동기화는 timeout이 있는 soft limit이다. OPERATIONAL은 모든 앱의 RUNNING을 뜻하지 않는다 | core 앱은 hard limit과 panic이 있음을 함께 쓴다. 실측은 Linux native임을 쓴다 | **[확인된 사실]**·**[실측]** F5 항목 |
| `CFE_TBL_ERR_NEVER_LOADED`에 대한 header 설명과 구현이 546a002와 v7.0.0에서 다르다 | 구현을 기준으로 모델을 만든다 | **[확인된 사실]** F6 항목 |
| F1·F2·F5·F7은 기존 order violation·event race 범주의 cFS 유사 사례다 | 기여는 cFS에서 `Req`·`G`를 얻는 방법과 cFS C 소스에서의 정적 판정에 둔다 | **[확인된 사실]** TaxDC §3.1·§8.4. **[해석]** 1.5절 |
| cFS C 코드는 진단 없이 MLIR LLVM dialect로 import될 수 있다 | 범위(5개 cFE module, 17개 앱 디렉터리, `-O0 -g`, `-Werror` 제거)를 함께 쓴다. 의미 복원 가능성의 근거로 쓰지 않는다 | **[실측]** RQ6 |
| 확인한 문헌·도구 범위에서 cFS 앱 사이의 순서·시간 state를 정적으로 분석한 작업은 찾지 못했다 | 'egress 차단으로 Ganesan 2016, Valente 2025, ROSInfer 일부, IV&V 보고서 원문을 읽지 못했다'는 제한을 함께 쓴다 | **[미확인]** 검색 범위 한정. 차단된 host와 snippet 출처는 `$N/rw-flight/access_log.txt`, `$N/rw-flight/snippet_sources.txt`에 있다 |

**할 수 없는 주장**

| 주장 | 할 수 없는 이유 |
| --- | --- |
| "cFS 동시성의 첫 정적 분석" 또는 "cFS를 처음 모델링" | **[확인된 사실]** cFS SB의 동시성을 Spec Explorer 모델로 시험한 연구(R03, ISSRE 2016, 초록 수준)와 CFS 아키텍처 규칙을 SAVE로 검사한 연구(SPLC 2009, 초록·snippet 수준)가 있다 |
| "프레임워크 의미를 복원해 순서 결함을 찾는 방법이 새롭다" | **[확인된 사실]** ROSInfer(ICSE 2024)는 ROS C++에서 입력 trigger(구독 callback, 주기, component 시작)가 state 변수를 바꾸고 그 변수가 발행 조건을 정하는 상태기계를 추론한다. 생성한 PlusCal/TLA+ 모델로 이미 알려진 결함 3개를 다시 찾았다. 메시지 내용은 모델링하지 않는다 (§4.2.1). fprime-topo-analysis는 F′ topology와 C++ handler 흐름을 결합해 lock·data race·queue 우선순위를 분석한다 (§4.1.2; peer review 미확인). Goblint v1.1.0의 `arinc.ml`은 ARINC 653 process·동기화 동작을 graphviz·Promela 모델로 내보냈지만 sampling·queuing port 호출은 `Nop`으로 처리했다 (§4.1.2; 검증 C08 정정 문구). **[미확인]** Android의 SIERRA·nAdroid는 검색 요약만 보았다 |
| "cFS topology 복원이 기여다" | **[확인된 사실]** RQ1의 선행 사례 |
| "Swift/BAT의 원인은 서로 다른 epoch의 snapshot 혼합이다" | **[확인된 사실]** Circular는 'incorrect spacecraft attitude information'(22706)과 'known race condition'만 말한다. GCN 34691의 '7 minute problem'은 attitude를 언급하지 않고 22706과의 관계도 밝히지 않는다 (모두 검색 snippet 수준) |
| "Swift/BAT는 cFS 사례다" 또는 "Swift/BAT는 cFS와 무관하다" | **[확인된 사실]** 검색 snippet에 따르면 BAT flight software는 RAD6000·VxWorks 위의 C++이고 Triana에서 온 'Command and Data Handling Software Bus'를 쓴다. 반면 GSFC의 2008년 발표 'cFE/CFS'(NTRS 20090005965)의 'cFE Heritage' slide는 'Swift BAT (12/04)'를 나열한다 (§2.1.5). **[해석]** BAT는 cFE를 실행한 것으로 확인되지 않은 cFE 계보의 선행 시스템이다. 검증 과정에서도 이 부분은 이견이 있었다 (검증 C22, contested). benchmark로 쓰지 않고, 동기 사례로만 쓴다 |
| "cFE #73을 검출했다" | **[해석]** 현재 코드에서는 경로가 닫혀 있다 (F5). 6.4.1/6.4.2 소스로 파생 예제를 만들 수 있지만, Microblaze·GRC EVA 환경은 공개되어 있지 않다. 파생 예제의 결과는 '#73에서 파생한 예제'로만 보고한다 (수정본 §5.3) |
| 검출률, precision·recall, 분석의 soundness, MLIR의 우월성 | **[미확인]** 구현·평가 결과가 없다 |
| "발행 성공은 수신 처리 완료를 뜻한다", "한 번의 발행은 모든 구독자에게 원자적으로 전달된다" | **[확인된 사실]** F1·F7 항목 |
| "`CFE_ES_WaitForStartupSync` 호출이 준비를 보장한다" | **[확인된 사실]**·**[실측]** F5 항목 |
| "Ogma 생성 monitor는 항상 여러 메시지의 latest value를 결합한다" | **[확인된 사실]** `copilot_step()`은 active 입력에서만 호출되고, README 예제는 입력이 하나다. **[해석]** 두 개 이상 입력을 고를 때만 해당하며, 검증자 간 이견이 있었다 |
| TaxDC의 비율(order violation 44% 등)을 flight software에 적용 | **[확인된 사실]** TaxDC는 Cassandra·HBase·Hadoop MapReduce·ZooKeeper의 104개 결함 연구이고, 저자들이 일반화를 경고한다 |
| 물리 시간 freshness 한계(20 ms 등)를 판정 기준으로 사용 | **[확인된 사실]** 수정본 §6.5. 임무 근거가 없다 |
| "cFS Software Bus에는 원래 race가 있다" | **[원노트 구상]** 원노트 §50이 이미 금지했다. **[해석]** 위 실측은 Linux 일정에서의 관측일 뿐이다 |

---

### 1.8 이 절에서 바로잡은 원노트·수정본의 항목

이 절과 관련된 원노트·수정본의 보정(원노트 §4.1·§4.2·§21·§27·§41·§53, 수정본 §2.1·§2.2·§7.1)은 §10.4의 O1, O4, O17, O22, O24, O27, RV1, RV2, RV5에 모았다.
