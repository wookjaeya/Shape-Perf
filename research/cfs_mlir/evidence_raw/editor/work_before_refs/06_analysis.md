## 6. 분석 설계

이 절은 분석기의 설계를 정한다. 순서는 다음과 같다.

- 처리 단계 (6.0)
- 사건 모델 (6.1)
- happens-before(HB)의 출처와 조건 (6.2)
- 추상 도메인 (6.3)
- `cfs` op별 전이 함수 (6.4)
- 함수 사이 분석 (6.5)
- 필요 순서 `Req`를 얻는 방법 (6.6)
- 후보에서 witness까지의 판정 (6.7)
- 오경고 통제 (6.8)
- 진단 형식 (6.9)
- 구현 위험 (6.10)

**[설계 제안]** 이 절의 규칙, 도메인, 의사 코드, op 정의는 모두 제안이다. 구현·실행·평가한 결과는 없다. 이 절은 검출률, precision·recall, soundness, MLIR의 우월성을 주장하지 않는다.

**표기와 참조.**

- 프레임워크 사실은 §3의 ID로 인용한다. SB-*, OS-*, ES-*, TBL-*, EVT-*, 모델 요소 M1–M14, 가정 A1–A11이 그 ID다.
- 이벤트 어휘, 결함 범주 F1–F8, 결함 조건 1–4, 실행 가정 Σ는 §1을 따른다.
- op 이름은 §5의 `cfs` dialect 초안을 점 표기로 쓴다(예: `cfs.sb.subscribe`). 이 절이 쓰는 op는 모두 §5.5에 정의되어 있다.
- GitHub 줄 번호는 별도 표시가 없으면 cFE `546a002515be5a1e3b66f9ae2c14f948d9cec76f`(S25) 기준이다.
- 로컬 MLIR 경로는 `/home/user/work/llvm-project`의 commit `1053047a`(2026-02-25) 기준이다.

**[설계 제안] 설계 원칙.** 아래 여섯 원칙이 이 절의 모든 규칙을 제약한다.

| ID | 원칙 | 이유 | 적용 |
| --- | --- | --- | --- |
| AP1 | 보장 관계 `G0`에는 Σ와 무관하고 가정이 없는 규칙만 넣는다. 이름 붙은 가정이 필요한 규칙은 `G_A`에만 넣는다. | **[실측]** 같은 코드에서 relay 역전이 1 CPU·SCHED_RR에서는 500/500(run1·run5), 4 CPU에서는 0/500(run2·run3)이었다 (§3 SB-4; `$N/sem-sb/probe_runs/SBPROBE_lines_all_runs.txt` L15, L102, L38, L61). priority·CPU 수는 순서를 정하지 않는다. | priority·CPU는 6.7의 실행 가능성 확인에서만 쓴다. |
| AP2 | `Req`는 `G`를 보지 않고 만든다. | **[원노트 구상]** 수정본 §10.1: 필요한 순서를 보장된 순서에 미리 넣으면 검사 전에 정상이라고 가정하는 셈이다. | 6.6의 생성 절차는 `G`를 입력으로 받지 않는다. |
| AP3 | 상태를 바꾸는 사건은 API의 성공 경로에만 둔다. | **[확인된 사실]** 구독·수신·대기는 실패 경로가 있다 (§3 SB-2, SB-9, ES-5). `WaitForStartupSync`는 상태를 버린다 (ES-6). | 반환 상태를 검사하지 않은 호출은 성공을 가정하지 않는다. `unchecked`로 표시한다. |
| AP4 | 정보 손실은 경고가 아니라 `unknown`이다. | **[확인된 사실]** §3.9 서두. | 상관 손실, unknown callee, 해석하지 못한 MID는 따로 센다. |
| AP5 | 결과를 세 수준으로 나눈다. | **[설계 제안]** §1.1의 증거 수준. | 후보, 모델상 가능, 재현으로 나눈다. |
| AP6 | 의미 모델은 cFE 버전을 인자로 받는다. | **[확인된 사실]** §3.6. put과 SB mutex의 관계, NEVER_LOADED 포인터, TBL 재등록이 버전마다 다르다. | 버전 의존 규칙에는 버전 조건을 붙인다. |

---

### 6.0 입력과 처리 단계

**[설계 제안]** 입력은 §1.1의 `System` 튜플이다. 구성 요소는 cFE·OSAL·PSP revision, 앱 소스와 `compile_commands.json`, build 설정과 MID mapping header, table 이미지, startup script, Σ다. 처리 단계는 아래 표와 같다.

| 단계 | 하는 일 **[설계 제안]** | 산출 | 현재 근거와 위험 |
| --- | --- | --- | --- |
| S1 frontend | 1) `compile_commands.json`의 C 파일마다 다음 옵션으로 IR을 만든다: `clang -O1 -Xclang -disable-llvm-passes -g -Wno-unknown-warning-option -Wno-error -S -emit-llvm`. 2) `mlir-translate --import-llvm --mlir-print-debuginfo`로 import한다. 3) `mlir-opt --inline --sroa --mem2reg --canonicalize --cse`를 적용한다. | LLVM dialect 모듈 | **[실측]** 이 경로에서 코드에서 정의한 MID가 call site의 상수 operand가 되었다. 앱별 회수 수와 to_lab 정정은 §7.7.3에 있다. gcc compile DB를 그대로 쓰면 gcc 전용 `-Wno-stringop-*`와 `-Werror` 때문에 실패하고, `-Wno-unknown-warning-option`만 더해도 2개 파일이 clang 경고로 실패한다 (§7.5.2). 그래서 `-Wno-error`가 필요하다. |
| S2 보조 입력 | Clang AST pass로 MID literal의 spelling 위치를 얻는다. table 이미지(C 초기화자)와 build가 고른 MID mapping header를 읽는다. | `MID 값 → macro 이름·정의 위치`, `cfs.config_send`/`cfs.config_subscribe` 선언 | **[실측]** IR에는 MID macro 이름이 남지 않는다. AST에는 literal의 spelling 위치가 남는다 (`$N/probe/cir/ast_SAMPLE_APP_Init.txt` L124–L136). **[확인된 사실]** HK·TO_LAB·SCH_LAB 구독·발행의 일부는 table에서 MID를 얻는다 (§3.7). EDS build는 1차 범위 밖이다 (§1.4). |
| S3 closed world | entry 함수를 뺀 모든 `llvm.func`에 `sym_visibility = "private"`를 붙인다. entry 함수는 startup script의 main entry, child task 함수, 등록된 callback이다. | 호출자를 모두 아는 모듈 | **[실측]** import된 C 함수는 `static`이어도 sym_visibility가 없다. 그래서 DeadCodeAnalysis가 predecessor를 모른다고 처리한다 (`$N/rw-mlir/mlir_probe/dca_vis.out`). private을 손으로 붙이면 `op_preds: (all) predecessors`가 된다 (`$N/verify-C20/counter/s44_O0_dca_priv.out`). **[확인된 사실]** 판정 코드: [DeadCodeAnalysis.cpp L192 @ccac700c](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/lib/Analysis/DataFlow/DeadCodeAnalysis.cpp#L192). |
| S4 lifting | cFE API 호출을 `cfs.*` op로 바꾼다. 앱 전역 field의 store·load는 `cfs.state.update`·`cfs.state.read`로 바꾼다. field 이름은 GEP index와 DI member 순서를 합쳐 얻는다. | `cfs` op가 들어간 모듈 | **[실측]** debug info를 켠 import에서 전역 field store 83곳을 찾았고, 모두 file:line을 가졌다. 세 패턴은 놓쳤다: offset 0 field, API out-parameter 쓰기, 지역 포인터를 통한 쓰기. sch_lab에서는 0곳이었다 (`$N/probe/mlir/O0_*.json`). |
| S5 구조 복원 | 앱·task·pipe·dispatch case를 복원한다 (6.5.1). | `cfs.app`, `cfs.task`, `cfs.dispatch_case` | 6.5 참조 |
| S6 task 내부 분석 | MLIR `DataFlowSolver` 위에서 dense forward 분석을 돌린다 (6.3–6.5). | program point마다의 추상 상태 | **[확인된 사실]** solver는 fixpoint 반복과 분석 의존성을 관리한다. thread·interleaving 개념은 없다 ([DataFlowFramework.h @ccac700c](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/include/mlir/Analysis/DataFlowFramework.h#L300); 검증 C20). |
| S7 사건과 G | task마다 사건을 뽑고 HB 규칙으로 `G0`·`G_A`를 만든다 (6.1–6.2). | 사건 그래프 | MLIR solver 밖에서 계산한다 (6.5.6). |
| S8 Req | 계약, API 규칙, 데이터 의존에서 `Req`를 만든다 (6.6). | `(e1, e2, 근거)` 목록 | — |
| S9 판정 | 후보 → 실행 가능성 → 재현 (6.7). 오경고 통제 (6.8). | 진단 (6.9) | — |

---

### 6.1 사건 모델

#### 6.1.1 구체 의미

**[설계 제안]** 실행 trace `τ`는 사건의 열이다.

- 사건 `e`는 `(kind, task, site, attrs)`다. `task`는 OS task 하나다. `app(task)`는 그 task의 AppId다 (§3 M8).
- `cfs.sb.transmit` 한 번은 새 메시지 instance `ι`를 만든다. `ι`는 MID, 호출 시점의 payload 복사본, sequence 번호를 가진다. **[확인된 사실]** `TransmitMsg`는 호출 시점에 메시지 전체를 SB buffer로 `memcpy`하고, 같은 buffer descriptor를 모든 destination에 넣는다 (§3 SB-3; [cfe_sb_api.c L1538–L1597](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_api.c#L1538-L1597)).
- pipe `p`마다 FIFO queue `Q_p`가 있다. reader는 하나다 (A1. OSAL POSIX·RTEMS 조건. VxWorks는 **[미확인]**).
- 같은 instance의 send–receive만 HB를 만든다. **[확인된 사실]** Lamport의 happened-before는 같은 실행 주체 안의 순서, 같은 메시지의 send–receive, 추이성으로 정의된다 ([R01](https://www.microsoft.com/en-us/research/wp-content/uploads/2016/12/Time-Clocks-and-the-Ordering-of-Events-in-a-Distributed-System.pdf), pp. 558–560).

#### 6.1.2 사건 어휘

아래 표는 §1.1의 어휘를 op 수준으로 구체화한다.

| 사건 | 생기는 지점 **[설계 제안]** | 속성 | 생기지 않는 경우 | 근거 |
| --- | --- | --- | --- | --- |
| `sub(a,m,p)` 구독 완료 | `cfs.sb.subscribe`의 status가 `CFE_SUCCESS`인 분기에 들어갈 때 | app, mid, pipe, MsgLim, site | status ≠ SUCCESS. status를 검사하지 않으면 `sub?`(미확정)로 둔다 | **[확인된 사실]** §3 SB-2 ([cfe_sb_api.c L936–L1110](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_api.c#L936-L1110)) |
| `unsub(a,m,p)` | `cfs.sb.unsubscribe` 성공 | 같음 | — | **[확인된 사실]** §3 SB-6 |
| `tx.b(a,ι)`, `tx.e(a,ι)` transmit 시작·반환 | `cfs.sb.transmit`의 진입과 반환 | mid(ι), IsOrigination, payload 출처 스냅샷 | 반환이 호출자 쪽 오류(SB-8)이면 instance가 없다 | **[확인된 사실]** §3 SB-3, SB-8. **[확인된 사실]** MID는 transmit 인자가 아니고 앞선 `CFE_MSG_Init`이 buffer에 묶는다 ([sample_app.c L134](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app.c#L134), [sample_app_cmds.c L61](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app_cmds.c#L61)) |
| `enq(ι,p)` / `drop(ι,p,why)` | tx 안에서 destination마다 하나씩, sender task에서 일어난다. 정적 분석은 이 사건을 MID–구독 결합 단계(S7)에서 만든다 | why ∈ {NO_ROUTE, ZERO_DEST, MSGLIM, PIPE_FULL, INACTIVE} | — | **[확인된 사실]** §3 SB-3, SB-5–SB-7, SB-11, M1 |
| `rcv(b,p)` 수신 성공 | `cfs.sb.receive`의 status == SUCCESS 분기 | pipe. 꺼낸 instance는 정적으로 모른다 | NO_MESSAGE, TIME_OUT, PIPE_RD_ERR 경로 | **[확인된 사실]** §3 SB-9, M6 |
| `hdl(b,m,cc)` handler 진입 | `cfs.dispatch_case`가 가리키는 handler의 진입. dispatch 분기 조건 `MsgId_Equal(msgid(buf), m)`과 function code `cc`가 참인 경로 | mid, cc, rcv site | MID를 상수로 풀지 못한 분기는 `hdl(b,⊤)` | **[확인된 사실]** sample_app은 `CFE_MSG_GetMsgId` out-parameter를 함수 static cache(`CMD_MID`, `SEND_HK_MID`)와 `CFE_SB_MsgId_Equal`로 비교한다. cache는 처음 호출 때 채운다 ([sample_app_dispatch.c L132–L167](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app_dispatch.c#L132-L167)) |
| `w(b,f,o)` state 쓰기 | `cfs.state.update` | field `f`, 출처 `o` (6.3.1) | — | §1.1 |
| `r(b,f,u)` state 읽기·소비 | `cfs.state.read`. 읽은 값이 분기 조건, `cfs.sb.transmit` payload, 외부 호출 인자, 제어 출력으로 흘러가면 소비로 표시한다 | use site `u` | 같은 field를 갱신하기 위한 read-modify-write의 읽기(예: `CmdCounter++`)는 소비로 세지 않는다 | §1.1 |
| `ready(b,s)` AppState 상승 | `cfs.es.wait_system_state(S)`, `cfs.es.wait_startup_sync`, `cfs.es.run_loop`의 진입. 대기·반환보다 먼저 일어난다 | `s = raise(S)`: OPERATIONAL→RUNNING, APPS_INIT→LATE_INIT, SHUTDOWN→STOPPED. `run_loop`은 RUNNING | — | **[확인된 사실]** §3 ES-5, ES-7 ([cfe_es_api.c L514–L608](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_api.c#L514-L608), [L469–L472](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_api.c#L469-L472)) |
| `waitok(b,S)` 대기 성공 | `cfs.es.wait_system_state`의 status == SUCCESS 분기 | S | `wait_startup_sync`(상태를 버린다), `CFE_ES_OPERATION_TIMED_OUT` 경로 | **[확인된 사실]** §3 ES-5, ES-6 ([cfe_es_api.c L616–L619](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_api.c#L616-L619)) |
| `sys(S)` 시스템 단계 | ES main의 `SystemState := S`. 분석기는 cFE 소스를 분석하지 않고 손으로 쓴 ES 모델에서 이 사건을 얻는다 | S | — | **[확인된 사실]** §3 ES-1 ([cfe_es_start.c L74–L236](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_start.c#L74-L236)) |
| `tget(h,st,v)` table 주소 획득 | `cfs.tbl.get_address`의 반환. status별로 분기한다 | handle, status, buffer 버전 변수 `v` | — | **[확인된 사실]** §3 TBL-1, TBL-2 |
| `trel(h)` | `cfs.tbl.release_address` | handle | — | **[확인된 사실]** §3 TBL-2 |
| `tact(t,v)` table 활성화 | owner의 `cfs.tbl.load`(double-buffered, 또는 single의 첫 load), `cfs.tbl.update`, `cfs.tbl.manage`가 활성화를 일으키는 경로 | table, 새 버전 | `INFO_TABLE_LOCKED`, `NO_BUFFER_AVAIL` 경로 | **[확인된 사실]** §3 TBL-4, TBL-5 |
| `treg(a,t,st)`, `tunreg(a,t)` | `cfs.tbl.register`, `cfs.tbl.share`, `cfs.tbl.unregister` | status | — | **[확인된 사실]** §3 TBL-7 |
| `rst.req(b)`, `rst.clean(b)`, `rst.create(b′)` | `CFE_ES_RestartApp` 호출 / ES background의 `CFE_ES_CleanUpApp` / `CFE_ES_AppCreate`. 뒤의 두 사건은 ES 모델에서 얻는다 | 새 AppId | — | **[확인된 사실]** §3 ES-9, ES-10, M9 |

#### 6.1.3 정적 사건과 출현 한정자

**[설계 제안]** 정적 분석은 사건의 site를 다룬다. 같은 site가 loop 안에서 여러 번 실행된다. 그래서 정적 사건에 출현 한정자를 붙인다.

| 한정자 | 뜻 | 판정 방법 | 대표 예 |
| --- | --- | --- | --- |
| `once(e)` | task instance 하나에서 많아야 한 번 | site가 task supergraph에서 어떤 loop에도 속하지 않고, 그 함수에 이르는 모든 호출 경로도 loop 밖에 있다 | Init 안의 `cfs.sb.subscribe` |
| `first(e)` | 반복되는 site의 첫 출현 | 같은 site의 첫 실행 | run loop 안 handler의 첫 `hdl` |
| `each(e)` | 모든 출현 | — | 주기 handler의 `w`, `r` |

**[설계 제안]** task 사이의 순서(`G0`, `G_A`)는 `once`·`first` 사건 사이에만 만든다. 반복 사건 사이의 관계는 task 안의 dataflow(6.3)와 FIFO 규칙(HB-FIFO)으로만 다룬다. **[해석]** startup·lifecycle 사건은 대부분 `once`다. 이 제한 덕분에 메시지 instance 번호를 추적하지 않아도 된다. 대가로 "k번째 발행과 k번째 수신" 같은 관계는 표현하지 않는다.

**[설계 제안] 정적 instance class.** `inst(s,m)`은 발행 site `s`가 낸 MID `m`의 instance 전체다. 수신된 instance가 어느 발행에서 왔는지는 정적으로 모른다. 그래서 HB-MSG(6.2)는 "발행자 집합의 공통 선행자"라는 형태로 쓴다.

---

### 6.2 Happens-before의 출처와 정확한 조건

**[설계 제안]** `G0`와 `G_A`는 아래 규칙으로 만든 간선의 추이 폐포다. 규칙의 조건을 모두 만족할 때만 간선을 넣는다. `G0`는 가정 열이 비어 있는 규칙만 쓴다. `G_A`는 가정 열의 이름 붙은 가정을 함께 기록한다. Σ의 priority·CPU 수는 어느 쪽에도 넣지 않는다 (AP1).

| ID | 간선 | 조건 (모두 만족) | 간선을 넣지 않는 경우 | 가정 | 근거 |
| --- | --- | --- | --- | --- | --- |
| HB-PO | `x → y` (같은 task) | `x`가 task supergraph에서 `y`를 지배한다. `x`가 status를 가진 사건이면 성공 분기에 있다. `once`/`first` 사건 사이에만 쓴다 | 같은 앱의 다른 task 사이. 지배하지 않는 loop iteration 사이 | — | **[확인된 사실]** R01의 같은 실행 주체 안의 순서. **[확인된 사실]** 실행 단위는 task다 (§3 M8, ES-8). **[해석]** 지배 관계이므로 "y가 일어나면 그 전에 x가 일어났다" |
| HB-MSG | `⋁{first tx.b(s) : s ∈ Pub(m)} → first hdl(U,m)` (pipe `p`) | ① `U`의 pipe `p`에 `sub(U,m,p)`가 있다. ② `Pub(m)`이 완전하다: m의 발행 site가 모두 code-literal 또는 해석한 table에서 왔다. 명령 기반·unknown 발행자가 없다. ③ non-EDS build다. ④ 같은 cFE instance다. 의미: `Pub(m)`의 모든 원소에 HB로 앞서는 사건은 `first hdl(U,m)`에도 앞선다. `|Pub(m)|=1`이면 `first tx.b(s) → first hdl(U,m)` | `drop`된 instance. `tx.e → hdl`(반환과 수신 처리 사이). EDS. 명령 기반 발행자(예: TO_LAB AddPacket처럼 실행 중에 바뀌는 집합) | — | **[확인된 사실]** 수신자는 transmit 반환 전에 실행될 수 있다 (§3 SB-3, SB-4; [cfe_sb.h L417–L422](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb.h#L417-L422)). **[해석]** 첫 hdl이 꺼낸 instance는 어떤 발행 site의 어떤 출현에서 왔다. 그 출현은 그 site의 첫 출현보다 늦거나 같다 |
| HB-FIFO | 수신 task `U`에서 `first hdl(U,m1) → first hdl(U,m2)` | ① 같은 sender task `T`에서 `tx(m1)`이 `tx(m2)`를 지배한다(HB-PO). ② 두 MID가 `U`의 같은 pipe `p`로 간다. ③ `Pub(m2)={tx(m2) site}`이다. ④ OSAL POSIX 또는 RTEMS다. ⑤ `sub(U,m1,p)`가 `G`에서 첫 `tx(m1)`보다 앞선다(NO_ROUTE·ZERO_DEST 배제) | 서로 다른 sender. 서로 다른 pipe. VxWorks (**[미확인]**) | `NoDrop(p,m1)`: 그 instance가 MSGLIM·PIPE_FULL·INACTIVE로 버려지지 않는다 | **[확인된 사실]** put은 sender task 안에서 순서대로 일어난다 (§3 SB-3, M1). pipe 하나는 FIFO다 (§3 OS-1, A1). **[실측]** MsgLim=2에서 5번 발행하면 3개가 버려졌고 rc는 모두 0이었다 (§3 SB-7, T3) |
| HB-SYS1 | `sys(S) → waitok(A,S)` | `cfs.es.wait_system_state(S)`의 status를 검사했고 그 분기가 SUCCESS다 | `cfs.es.wait_startup_sync`. TIMED_OUT 경로 | — (`G0`). 사슬로 lock 없는 일반 메모리 쓰기의 가시성까지 주장할 때만 `MemOrder(SystemState)`를 붙여 `G_A`에 둔다 | **[확인된 사실]** §3 ES-1, ES-5, A6, R-sync-1: 성공 반환은 ES가 `SystemState`를 S 이상으로 쓴 뒤다. **[미확인]** `SystemState`는 lock 없이 읽힌다. 다중 코어에서 앞선 일반 메모리 쓰기가 보이는 순서는 확인하지 않았다 (§3.3). **[해석]** 아래 사슬 예가 덮는 쓰기(SB 구독, ES AppState, TBL descriptor)는 각각 SB mutex·ES lock·TBL registry lock 아래에서 일어난다 (§3 SB-2, ES-5, TBL-2). 그래서 이 사슬은 `MemOrder`에 기대지 않는다 |
| HB-SYS2 | `ready(B,RUNNING) → sys(OPERATIONAL)`. 마찬가지로 `ready(B,LATE_INIT) → sys(APPS_INIT)` | `B`가 startup script 앱이다 | 가정을 끈 경우(기본) | `NoStartupTimeout(B)`: B가 `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC`(기본 1000 ms, soft) 안에 ready에 도달한다 | **[확인된 사실]** ES는 앱 record의 AppState를 lock 아래에서 센다. 제한 시간을 넘기면 syslog만 쓰고 진행한다 (§3 ES-4; [cfe_es_start.c L204–L229](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_start.c#L204-L229), [L886–L944](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_start.c#L886-L944)). **[실측]** probe 앱 하나를 2.5 s 늦게 시작시켰다. 24/24 실행에서 'Startup Sync failed'가 찍힌 뒤 ES가 OPERATIONAL에 들어갔다. 그때 늦은 앱은 아직 ready에 도달하지 않았다. 이 판단은 로그 순서와 코드로 추론한 것이고, AppState를 직접 읽지는 않았다 (§3 ES-4) |
| HB-CORE | core 앱 `c`의 `TaskInit` 반환 → 다음 core task 생성과 모든 startup script 앱 task의 첫 사건 | 기본 `MISSION_CORE_MODULES` | core child task(ES_BG_TASK, TIME tone·1Hz) | — | **[확인된 사실]** §3 ES-2, A4 ([cfe_es_start.c L767–L876](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_start.c#L767-L876)). **[확인된 사실]** core 앱의 RUNNING은 `TaskInit` 성공이 아니라 반환을 뜻한다 (ES-2) |
| HB-LIB | `CFE_LIB` init 반환 → script에서 그 뒤에 오는 항목의 생성 | startup script 경로 | — | — | **[확인된 사실]** §3 ES-3, A5 |
| HB-RST | `rst.req(b) → rst.clean(b) → rst.create(b′) → b′의 첫 사건` | cFE ≥ v7.0.0 계열의 ES 모델 | — | 옛 instance의 마지막 사건 `→ rst.clean(b)`는 `CoopExit(b)` 가정에서만 넣는다: `RunLoop`이 false를 돌려주고 `ExitApp`이 STOPPED를 쓴 경우 | **[확인된 사실]** §3 ES-9, ES-10, M9. **[확인된 사실]** kill-timer 경로에서는 cleanup 동안 task가 아직 살아 있다 (ES-10) |
| HB-TACT | `tact(t,v) → tget(h, SUCCESS∨INFO_UPDATED, v)` | 활성화가 `Update`·`Manage` 경로에서 일어났다(registry lock 보유) | owner `cfs.tbl.load` 경로 | owner `Load` 경로는 `TblLoadAtomic` 가정에서만 넣는다 (**[미확인]** TBL-6) | **[확인된 사실]** `GetAddress`는 lock 아래에서 `desc.buf := active`를 쓴다 (§3 TBL-2). `CFE_TBL_Update`는 `UpdateInternal` 동안 lock을 쥔다. owner `Load`는 lock을 일찍 놓는다 (TBL-6) |
| HB-TREG | 남은 sharer descriptor의 해제(`Unregister` 또는 sharer cleanup) → `DUPLICATE_NOT_OWNED`를 거친 뒤의 owner `treg(...,SUCCESS)` | cFE ≥ v7.0.0 (commit `c1ab1b7` 이후) | 그 이전 버전 | — | **[확인된 사실]** §3 TBL-7 |
| HB-OSAL | 이름 있는 OSAL 자원 생성 성공 → 같은 이름의 `OS_*GetIdByName` 성공 | 분석기가 이름 문자열을 상수로 푼다 | — | `OsalNameLookup` (**[미확인]** OSAL 이름 조회 의미를 이 연구에서 확인하지 않았다) | **[확인된 사실]** CF #184 수정은 `OS_CountSemGetIdByName`을 100 ms 간격으로 25번까지 재시도한다 ([CF 833fdbb](https://github.com/nasa/CF/commit/833fdbb27a26f15f2429e79392e7eb73d61abdea)) |

**[설계 제안] 버전 조건부 규칙.** cFE에서 commit [`550e7f7d`](https://github.com/nasa/cFE/commit/550e7f7dd349c24e234ab0881f5c13e667f05741) 이전 버전은 `OS_QueuePut`을 SB mutex 안에서 부른다 (§3.6, **[확인된 사실]**). 이 버전에서만 규칙 HB-ATX를 켠다. relay가 `ι1`을 받은 뒤 보낸 `ι2`에 대해 `enq(ι1,p) → enq(ι2,p)`를 넣는다. 근거는 다음과 같다. relay의 tx는 mutex를 얻어야 시작하고, sender는 모든 put을 마친 뒤에 mutex를 놓는다. **[해석·미측정]** 이 규칙은 옛 소스를 읽어 얻었고 실행으로 확인하지 않았다.

**G에 넣지 않는 관계.**

| 관계 | 넣지 않는 이유 |
| --- | --- |
| `tx.e → hdl` | **[확인된 사실]** 반환과 수신 처리 사이에는 정해진 순서가 없다 (§3 SB-3, SB-4) |
| 한 메시지의 서로 다른 pipe 도착 순서, 메시지와 그 결과(relay 재발행)의 다른 pipe 도착 순서 (cFE ≥ v7.0.0) | **[실측]** relay 역전이 1 CPU에서 500/500, 4 CPU에서 0/500, priority 없는 실행에서 15/500·30/500이었다 (§3 SB-4) |
| startup script의 load 순서 | **[실측]** 기본 bundle 6회 실행에서 init 완료 순서가 6가지로 모두 달랐다 (§3 ES-3) |
| task priority | **[확인된 사실]** 권한이 없으면 priority가 조용히 사라진다 (§3 OS-5). Σ1 실행 가능성 확인에만 쓴다 |
| syslog 시각, EVS event의 유무 | **[확인된 사실]** syslog 시각은 TIME init 때 뛴다 (§3 EVT-4). event는 filter와 발행 앱의 type mask에 걸린다 (§3 M12) |

**[설계 제안] G 구성 의사 코드.**

```text
# 입력: Ev[T] = task T의 once/first 사건 (6.1.3), Pub(m) = MID m의 발행 site 집합,
#       Sub(m) = (U, p, MsgLim) 구독 집합, ver = cFE 버전, A = 켤 가정의 이름 집합
# 출력: G0, G_A (사건 위의 DAG; 간선마다 규칙 ID, 가정, 근거 site를 기록)
procedure BUILD_HB(Ev, Pub, Sub, ver, A):
  G0 := ∅; GA := ∅
  for T in tasks:                                         # HB-PO
    for x, y in Ev[T]: if dominates_T(x, y) and on_success_edge(x): add(G0, x→y, "HB-PO")
  for c in core_apps(default_MISSION_CORE_MODULES):       # HB-CORE, HB-LIB
    for y in first_events_of_script_tasks(): add(G0, taskinit_ret(c)→y, "HB-CORE")
  for (U, p, m) in Sub where complete(Pub(m)) and not eds:              # HB-MSG
    h := first_hdl(U, m, p)
    if |Pub(m)| == 1: add(G0, first_tx(the s)→h, "HB-MSG")
    else: add_join_node(G0, {first_tx(s) | s ∈ Pub(m)} ⇒ h, "HB-MSG")   # 공통 선행자만 h로 전파
  for T, (s1, s2) in tx_pairs(T) where dominates_T(s1, s2):              # HB-FIFO
    for (U, p) in common_pipe(mid(s1), mid(s2)) where Pub(mid(s2)) == {s2} and osal ∈ {POSIX, RTEMS}
                                                    and G0 ⊢ sub(U, mid(s1), p) ≺ first_tx(s1):
      addA(GA, first_hdl(U,mid(s1))→first_hdl(U,mid(s2)), "HB-FIFO", assume=NoDrop(p, mid(s1)))
  for A_app in apps: for w in waitok_events(A_app, S): add(G0, sys(S)→w, "HB-SYS1")   # Σ와 무관 (AP1)
  for B in script_apps: addA(GA, ready(B,RUNNING)→sys(OPERATIONAL), "HB-SYS2", assume=NoStartupTimeout(B))
  add_restart_edges(G0, GA, ver)                          # HB-RST
  add_table_edges(G0, GA, ver)                            # HB-TACT, HB-TREG
  if ver < commit_550e7f7d: add_atomic_tx_edges(GA, "HB-ATX")   # [해석·미측정]
  return closure(G0), closure(G0 ∪ GA restricted to A)
```

**[해석] 사슬 예.** F1의 `Req(sub(b,m,p) ≺ first tx(a,m))`는 다음 사슬로 해소될 수 있다.

```text
sub(b,m,p) →HB-PO ready(b,RUNNING) →HB-SYS2[NoStartupTimeout(b)] sys(OPERATIONAL)
           →HB-SYS1 waitok(a,OPERATIONAL) →HB-PO first tx(a,m)
```

**[해석]** 이 사슬은 `G_A`에만 있다. 결과는 "가정 `NoStartupTimeout(b)` 아래에서 보장됨"으로 보고한다. `a`가 `WaitForStartupSync`를 쓰면 `waitok`이 없으므로 사슬이 끊긴다 (§3 ES-6). `b`의 `ready`가 `sub`보다 먼저 오면 사슬이 `sub`를 덮지 않는다 (§3 ES-5). 이것이 [cFE #198](https://github.com/nasa/cFE/issues/198)의 구조다. **[확인된 사실]** #198에서는 `WaitForStartupSync`를 쓰는 앱이 다른 앱의 늦은 초기화가 끝나기 전에 요청을 받았다. 6.6.0a에서 APPS_INIT·LATE_INIT·`CFE_ES_WaitForSystemState`가 추가되었다 (§1 F5).

---

### 6.3 추상 도메인

**[설계 제안]** task 안 분석의 추상 상태는 네 성분의 곱이다.

```text
σ♯ = ( Prov ,  Mem ,  Aux ,  Facts )
  Prov  : 상관을 보존하는 출처·초기화·freshness 정보 (6.3.1–6.3.3)
  Mem   : field마다 메모리 초기화 상태 (6.3.2)
  Aux   : 수신 buffer, table 포인터, 구독 집합, ready 표시 (6.3.4)
  Facts : 사건 기록 (6.1). 사건 추출 단계(S7)가 읽는다
```

**메모리 모델을 직접 만드는 이유.** upstream MLIR의 기본 alias 분석은 이 목적에 맞지 않는다. **[실측]** `LocalAliasAnalysis`는 서로 다른 전역을 MayAlias로, 한 struct의 서로 다른 field를 MustAlias로 판정했다. `llvm.call`은 readnone `memory_effects` 속성이 있어도 모든 위치에 대해 ModRef였다 (§4.5.3; `$N/verify-C20/counter/modref_memattr.out`). **[확인된 사실]** field가 구분되지 않는 것은 GEP가 `ViewLikeOpInterface`라서 base로 풀리기 때문이다 ([LocalAliasAnalysis.cpp L100–L106 @ccac700c](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/lib/Analysis/AliasAnalysis/LocalAliasAnalysis.cpp#L100-L106)). **[해석]** `llvm.call`이 이 경로에서 구체적인 메모리 효과를 내놓지 않기 때문으로 본다.

**[설계 제안]** 그래서 field를 `(전역 symbol, 상수 byte 범위)`로 식별한다. byte 범위는 GEP 상수 index에서 얻고, 이름은 DI member에서 얻는다. 포인터는 mem2reg 이후 SSA로 추적하고, 포인터 인자는 summary(6.5.3)로 처리한다. 대상을 모르는 store는 escape된 전역의 모든 field에 `Unknown` 출처를 쓴다.

#### 6.3.1 출처: 상관을 보존하는 집합

**[원노트 구상]** 원노트 §45는 field마다 출처 집합을 두고 합류에서 합집합을 취한다 (`origin: A + B → {A,B}`). **[원노트 구상]** 수정본 §6.4는 이 방식이 가짜 조합을 만든다고 지적한다. 한 분기가 `(A,B)=(k,k)`만, 다른 분기가 `(j,j)`만 만들면, field별 집합의 곱은 소스에 없는 `(k,j)`를 만든다.

**[설계 제안]** 그래서 출처를 "field별 집합"이 아니라 "경로별 공동 배정의 집합"으로 둔다.

```text
Field   := (global symbol, byte range)          # 예: SAMPLE_APP_Data[0,1) = CommandCounter
Origin  := Msg(mid, payloadPath, rcvSite)       # 수신 payload의 field에서 온 값
         | Tbl(table, path)                     # table 포인터를 통해 읽은 값
         | Init(site) | Const(c)                # 정적 초기값, 상수 대입
         | OutParam(api, site)                  # API의 out-parameter (예: CreatePipe의 pipe id)
         | Time(cfe)                            # CFE_TIME_GetTime 결과 (A9)
         | Param(i)                             # summary 안에서만 쓰는 기호 출처
         | Unknown(reason)
Joint   := ( asg : Field ⇀ Origin,              # 이 경로에서 각 field의 마지막 쓰기의 출처
             cls : Field ⇀ ClassId,             # 같은 '활성화'에서 쓰인 field끼리 같은 id
             val : CondField ⇀ {c, ⊤},          # 분기 조건에 쓰이는 field의 값 (validity flag 등)
             upd : β ↦ 2^Field )                # 경계 β 이후 갱신된 field (6.3.3)
Prov    := ⊥ | Disj(S ⊆ Joint, |S| ≤ K) | NonRel(Field → 2^Origin, lostAt : Loc)
```

**[설계 제안] 각 요소의 뜻.**

- **활성화(activation).** 한 번의 수신 성공부터 그 handler 반환까지, 또는 한 번의 `tget`부터 `trel`까지다. 같은 활성화에서 쓰인 field는 같은 `cls`를 갖는다. `cls`는 Joint 안에서 처음 나온 field 순서로 정규화한다. 그래서 개수가 유한하다.
- **CondField.** 앱 전역 field 중 분기 조건의 operand로 쓰이는 것이다. 사전 pass로 정한다. validity flag(예: 수정본 §6.1의 `attitude_valid`)가 여기에 들어간다.
- **K.** Disj의 크기 상한이다. 값은 실험에서 정한다. 넘으면 `NonRel`로 사영하고 `lostAt`에 위치를 기록한다.
- **순서와 합류.**
  - 순서: `Disj(S1) ⊑ Disj(S2)` ⟺ `S1 ⊆ S2`이다. `Disj(S) ⊑ NonRel(π(S))`이고, `π`는 field별 합집합 사영이다.
  - 합류: `Disj(S1) ⊔ Disj(S2) = Disj(S1 ∪ S2)`이다. `|S1 ∪ S2| > K`이면 `NonRel(π(S1 ∪ S2), here)`가 된다.
- **종료.** Origin은 유한하다(site × MID × payload 경로). cls는 정규화되고 Disj는 K로 막힌다. 그래서 widening 없이 높이가 유한하다. **[미확인]** 구체 의미에 대한 건전성(갈루아 연결)은 증명하지 않았다.

**[해석] 수정본 §6.4 예의 처리.** 두 분기를 합류하면 `Disj{ {A↦k,B↦k, cls A=B}, {A↦j,B↦j, cls A=B} }`가 된다. `(k,j)`는 생기지 않는다. 서로 다른 handler가 A와 B를 쓰면 `cls(A) ≠ cls(B)`인 Joint가 생긴다. 이 조합은 실제로 가능한 최신값 조합이다. 경고 여부는 계약(6.6)이 정한다.

**[설계 제안] payload를 통한 앱 사이 출처 연결.** `cfs.sb.transmit` 시점에 보내는 buffer의 field 출처를 `Out(m, site)`로 기록한다. 이 기록은 호출 시점 복사(§3 SB-3)와 맞는다. 수신 앱의 `Msg(m, g, …)`는 이 기록으로 이어진다. 그러면 relay·HK처럼 받은 값을 다시 내보내는 앱을 거쳐도 출처를 이을 수 있다. 이어 붙일 때는 MID 집합 `Pub(m)`의 모든 site 기록을 합집합으로 쓴다.

#### 6.3.2 초기화 격자: 메모리 초기화와 입력 수신을 구별

**[원노트 구상]** 수정본 §6.2는 "메모리 값이 정의되어 있다"와 "앱이 필요한 정상 입력을 받아 상태를 설정했다"를 구별하라고 쓴다. **[설계 제안]** 두 축을 따로 둔다.

| 축 | 원소 | 뜻 | 계산 |
| --- | --- | --- | --- |
| `Mem(f)` | `ZERO_STATIC` | C 정적 저장 기간 객체의 0 초기화 | 앱 전역 field의 entry 값 |
| | `WRITTEN` | 실행 중 한 번 이상 쓰였다 | `cfs.state.update` 이후 |
| | `UNDEF` | 정의되지 않은 값(지역·heap) | 지역 객체의 entry 값 |
| | `⊤` | 모름 | 합류 |
| `Inp(f)` | `⊥` | 도달 불가 | — |
| | `NONE` | 모든 Joint에서 `f`의 출처가 `Init`·`Const`·`OutParam`이다 | Prov에서 유도 |
| | `RCVD` | 모든 Joint에서 `f`의 출처가 `Msg`·`Tbl`이다 | Prov에서 유도 |
| | `MAYBE` (=⊤) | Joint마다 다르다 | Prov에서 유도 |
| `Val(f)` | `VALIDATED` / `UNCHECKED` | `Msg` 출처 쓰기가 같은 handler 안에서 검증 술어의 참 분기에 지배되는가 | 검증 술어는 계약에서 받는다. 없으면 `UNCHECKED` |

**[설계 제안]** `Inp`는 Prov에서 계산하므로 상관을 잃지 않는다. 예를 들어 flag `v`가 `CondField`이면 `if (v)`의 참 분기에서 `val(v)=0`인 Joint가 걸러진다. 남은 Joint가 모두 `asg(f)=Msg(...)`이면 그 분기에서 `Inp(f)=RCVD`다. 이것이 guard 인식이다 (6.8). `Mem=UNDEF`인 읽기는 C 미정의 동작이며 순서 결함과 따로 보고한다 (§1.4).

#### 6.3.3 Freshness: 이벤트 경계와 timestamp 층

**[설계 제안] 이벤트 경계 층 (항상 사용).** 경계 β는 "MID `m_β`의 handler 진입"으로 정의한다. 예: SCH_LAB이 보내는 wakeup MID다. Joint의 `upd[β]`는 마지막 β 이후 갱신된 field 집합이다.

- `hdl(b, m_β)` 진입에서 `upd[β] := ∅`.
- `w(b,f,·)`에서 `upd[β] ∪= {f}`.
- **[해석]** 이 층은 Burrows–Leino의 ghost boolean `stale_t`·`from_critical_t`와 구조가 비슷하다. 다만 경계가 critical section이 아니라 메시지 handler다. **[확인된 사실]** Burrows–Leino는 지역 변수마다 ghost boolean을 두고, 뒤의 critical section 진입에서 stale로 표시한다 ([krml107](https://www.microsoft.com/en-us/research/wp-content/uploads/2016/12/krml107.pdf), 전문; §1.5).

**[설계 제안] timestamp 층 (계약이 있을 때만).** 계약이 payload field의 시각 의미를 선언할 때만 쓴다. 선언 항목은 `measurement | publication`, clock, 단위다.

- 분기 조건 `|a.g − b.g| < D`에서 두 operand의 출처가 모두 measurement로 선언된 field이면, 참 분기의 Joint에 `tsCoherent(a,b,D)`를 붙인다. `D`는 기호 상수로 둔다.
- header 시각(`CFE_MSG_GetMsgTime` 결과)은 기본으로 `publication`이다. **[확인된 사실]** 기본 `CFE_MSG_OriginationAction`은 IsOrigination=true인 telemetry의 header 시각을 transmit 시점 시각으로 덮는다 (§3 EVT-6; [cfe_msg_integrity.c L30–L53](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/msg/fsw/src/cfe_msg_integrity.c#L30-L53)). 그래서 header 시각 비교는 measurement guard로 인정하지 않는다. mission의 MSG override는 **[미확인]**이다.
- 계약이 없으면 이 층은 `⊤`이고 시간 기반 경고를 만들지 않는다. **[원노트 구상]** 원노트의 `20 ms`, `100 ms`는 기준값으로 쓰지 않는다 (수정본 §6.5).
- **[확인된 사실]** 시작 직후 TIME은 `ClockSetState=NOT_SET`이다 (§3 EVT-4). 계약의 clock이 cFE TIME이면, guard는 clock 상태 검사가 함께 있을 때만 인정한다.

#### 6.3.4 보조 상태

| 성분 | 내용 **[설계 제안]** | 근거 |
| --- | --- | --- |
| `bufOf : Value ⇀ (pipe, rcvSite, cls, mid?, live)` | 수신 buffer 포인터와 그 출처. 같은 pipe의 다음 `cfs.sb.receive`에서 `live := false` | **[확인된 사실]** buffer는 같은 pipe의 다음 receive 호출까지만 유효하다. 인자가 유효하면 결과와 무관하게 이전 buffer를 놓는다 (§3 SB-9) |
| `tblPtr : Value ⇀ (h, table, state)` | `state ∈ {PROTECTED, UNPROTECTED, RELEASED, NULL}` | **[확인된 사실]** 같은 handle로 `GetAddress`를 다시 부르면 보호가 새 buffer로 옮겨 간다. NEVER_LOADED에서 포인터는 NULL이다 (§3 TBL-1, TBL-2) |
| `subs : 2^(mid × pipe × msglim)` | 성공한 구독 | §3 SB-2 |
| `readyMark : {NO, YES}` | 이 task가 AppState를 올렸는가 | §3 ES-5, ES-7 |
| `status : Value ⇀ 2^Code` | API 반환값의 가능한 코드. INFO 코드는 양수 | **[확인된 사실]** `status >= CFE_SUCCESS`는 INFO 코드를 성공으로 본다 (§3 TBL-9) |

---

### 6.4 op별 전이 함수

#### 6.4.1 효과 선언

**[설계 제안]** 모델링한 cFE 호출이 C 메모리 전체를 덮어쓰지 않게 하려고, op의 효과를 non-addressable 자원에 둔다. **[확인된 사실]** upstream main(ccac700c)의 문서는 자원 계층과 addressable 여부를 설명한다. 같은 commit의 `LocalAliasAnalysis`는 non-addressable 자원에 대한 효과를 포인터 메모리와 NoAlias로 본다 ([SideEffectsAndSpeculation.md L78–L121](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/docs/Rationale/SideEffectsAndSpeculation.md#L78-L121); §4.5.1, 검증 C20). **[확인된 사실]** 로컬 1053047a의 `Resource` 클래스(`mlir/include/mlir/Interfaces/SideEffectInterfaces.h` L79)에는 `isAddressable`이 없다 (`grep -rn isAddressable mlir/include/mlir/Interfaces` → 결과 없음). **[해석]** 따라서 구현은 LLVM을 main 쪽 commit으로 올리거나, 자원을 직접 보는 ModRef 판정을 따로 만들어야 한다.

**[설계 제안]** resource 목록(`CFS_SBRoutes`, `CFS_SBQueues`, `CFS_ESState`, `CFS_TBLRegistry`, `CFS_TBLBuffers` 등)과 op별 effect·operand·결과는 §5.4–§5.5의 정의를 그대로 쓴다. 이 절은 별도의 ODS 초안을 두지 않는다. **[확인된 사실]** 이 interface는 ODS에서는 `MemoryEffectsOpInterface`, C++에서는 `MemoryEffectOpInterface`라는 이름이다 (로컬 `SideEffectInterfaces.td` L26-L28).

**[확인된 사실]** IRDL만으로는 효과를 선언할 수 없다. IRDL로 정의한 op는 `MemoryEffectOpInterface`를 구현하지 않는다. `--test-side-effects`는 `cfs` op에 대해 아무것도 보고하지 않았고, 남은 remark 한 줄은 `arith.constant`의 것이었다 (`$N/rw-mlir/mlir_probe/cfs_irdl.out`, `cfs_use.mlir` L2–L4; §4.5.3). **[해석]** 효과가 필요한 op는 ODS/C++로 정의해야 한다.

#### 6.4.2 전이 표

**[설계 제안]** 아래 표는 op마다의 추상 전이다. `σ♯ = (Prov, Mem, Aux, Facts)`이다. "성공 분기"는 status가 SUCCESS(또는 표에 적은 코드)인 CFG 간선을 뜻한다. status를 분기하지 않으면 모든 경우의 합류로 처리하고 `unchecked`를 붙인다.

| op | 사건 | Prov·Mem 전이 | Aux 전이 | 근거 |
| --- | --- | --- | --- | --- |
| `cfs.sb.create_pipe` | — | 성공 분기에서 pipe id를 `OutParam(CreatePipe, site)`로 쓴다 | — | §3 SB-1 |
| `cfs.sb.subscribe` | 성공 분기에 `sub(a,m,p)` | — | 성공 분기에서 `subs ∪= {(m,p,msglim)}` | §3 SB-2 |
| `cfs.sb.unsubscribe` | 성공 분기에 `unsub` | — | `subs −= {(m,p,·)}` | §3 SB-6 |
| `cfs.msg.init(buf, mid, size)` | — | — | 지역 메시지 buffer `buf`에 `mid`를 묶는다 | §1.1 `pub` 행 |
| `cfs.sb.transmit(msg)` | `tx.b`, `tx.e` (enq/drop은 S7에서) | `Out(m, site) := {출처(msg.g) | g}`. 상태 field는 바꾸지 않는다 | — | §3 SB-3, SB-8 |
| `cfs.sb.receive(p, t)` | 성공 분기에 `rcv(b,p)` | 성공 분기에서 새 활성화 class `c`를 연다 | 모든 경로(인자가 유효할 때): `bufOf[*]`의 pipe `p` 항목을 `live:=false`. 성공 분기: `bufOf[buf] := (p, site, c, mid=?, live)` | §3 SB-9, M6 |
| `cfs.msg.get_msgid(buf)` + `cfs.mid_equal` 분기 | 참 분기에 `hdl(b,m,cc)` | 참 분기에서 `bufOf[buf].mid := m` | — | §3 SB-13 |
| `cfs.state.update(f, v)` | `w(b,f,o)` | 각 Joint `J`: `J.asg[f] := origin(v)`. `J.cls[f] := class(origin(v))`. `J.upd[β] ∪= {f}`. `f ∈ CondField`이면 `J.val[f] := const(v)` 또는 `⊤`. `Mem(f) := WRITTEN` | — | 6.3.1–6.3.3 |
| `cfs.state.read(f)` | `r(b,f,u)` | 변경 없음. 사용 기록 `(u, f, Prov|f)`를 남긴다 | — | §1.1 |
| `cfs.es.wait_system_state(S, t)` | op 위치에 `ready(b, raise(S))`, 성공 분기에 `waitok(b,S)` | — | `readyMark := YES` | §3 ES-5 |
| `cfs.es.wait_startup_sync(t)` | `ready(b, RUNNING)`만 | — | `readyMark := YES`. 이후 경로는 성공·timeout을 구별하지 않는다 | §3 ES-6 |
| `cfs.es.run_loop(rs)` | 처음 도달에서 `ready(b, RUNNING)` | false 분기: 종료 경로 | `readyMark := YES` | §3 ES-7 |
| `cfs.es.restart_app(name)` | `rst.req(target)` | — | — | §3 ES-9 |
| `cfs.tbl.register` / `share` | `treg(a,t,st)` | 성공 분기에서 handle을 `OutParam` 출처로 쓴다 | `DUPLICATE_NOT_OWNED` 분기를 따로 둔다 | §3 TBL-7 |
| `cfs.tbl.get_address(h)` | `tget(h,st,v)` | — | SUCCESS·INFO_UPDATED 분기: `tblPtr[ptr] := (h,t,PROTECTED)`, 새 활성화 class를 연다. 같은 `h`로 얻은 이전 포인터는 `UNPROTECTED`. NEVER_LOADED·UNREGISTERED 분기: `tblPtr[ptr] := NULL` | §3 TBL-1, TBL-2 |
| `cfs.tbl.release_address(h)` | `trel(h)` | 활성화를 닫는다 | `h`로 얻은 포인터 모두 `RELEASED` | §3 TBL-2 |
| table 포인터를 통한 load | `r` 또는 F6 후보 | 출처 `Tbl(t, path)`, 활성화 class는 해당 `tget`의 class | `NULL`·`RELEASED`·`UNPROTECTED`이면 F6 후보를 기록한다 | §3 TBL-1–TBL-5 |
| `cfs.tbl.load` / `update` / `manage` | 활성화 경로에 `tact(t,v)` | — | single + `INFO_TABLE_LOCKED`: pending 표시. double + `NO_BUFFER_AVAIL`: pending 없음 | §3 TBL-4, TBL-5 |
| `cfs.msg.timestamp(buf)` | — | header 시각의 출처를 `publication`으로 둔다 | — | §3 EVT-6 |
| `CFE_TIME_GetTime` | — | 결과 출처 `Time(cfe)`. 여러 field가 같은 version에서 온다 | — | §3 EVT-5, A9 |
| 모델 없는 외부 호출 | — | 인자로 넘긴 포인터가 가리키는 field와 escape된 전역의 field에 `Unknown(call)`을 쓴다 | 정밀도 표시 `unknownCall` | 6.5.4 |

#### 6.4.3 핵심 전이의 의사 코드

```text
# [설계 제안] DenseForwardDataFlowAnalysis<CfsLattice>::visitOperation(op, before, after)
# (로컬 DenseAnalysis.h L217: visitOperation(Operation*, const LatticeT &before, LatticeT *after))
def visit(op, before, after):
  s = copy(before)
  match op:
    case SbReceive(pipe=p, timeout=t) -> (st, buf):
      for b in s.aux.bufOf: if b.pipe == p: b.live = False          # SB-9: 결과와 무관하게 이전 buffer 해제
      s.pending_edge[st == SUCCESS] = lambda x: (x.aux.bufOf.put(buf, (p, op.site, fresh_cls(x), None, True)),
                                                 x.facts.add(Rcv(app, p, op.site)))
    case StateUpdate(field=f, value=v):
      o = origin_of(v, s)                 # SSA 역추적: load(bufOf[ptr]) → Msg(mid, path, rcvSite); const → Const;
                                          # tblPtr → Tbl; call result → OutParam/Unknown
      for J in s.prov.joints():
        J.asg[f] = o; J.cls[f] = class_of(o, s)
        for beta in J.upd: J.upd[beta].add(f)
        if f in CondField: J.val[f] = const_or_top(v)
      s.mem[f] = WRITTEN; s.facts.add(W(app, f, o, op.site))
    case CondBranch(cond) where cond reads f ∈ CondField:
      # 경로 필터: 각 후속 블록에서 cond를 만족할 수 없는 Joint를 버린다 (guard 인식)
      s.edge_filter[true]  = lambda J: sat(cond, J.val)
      s.edge_filter[false] = lambda J: sat(not cond, J.val)
    case TblGetAddress(tbl=h) -> (st, ptr):
      for q, e in s.aux.tblPtr.items(): if e.h == h and e.state == PROTECTED: e.state = UNPROTECTED  # TBL-2
      s.pending_edge[st in {SUCCESS, INFO_UPDATED}] = lambda x: x.aux.tblPtr.put(ptr, (h, tbl_of(h), PROTECTED))
      s.pending_edge[st in {NEVER_LOADED, UNREGISTERED}] = lambda x: x.aux.tblPtr.put(ptr, (h, tbl_of(h), NULL))
    case EsWaitSystemState(min_state=S) -> st:
      s.facts.add(Ready(app, raise(S), op.site)); s.aux.readyMark = YES
      s.pending_edge[st == SUCCESS] = lambda x: x.facts.add(WaitOk(app, S, op.site))
    ...
  if s.prov.size() > K: s.prov = NonRel(project(s.prov), lostAt=op.loc)
  return propagateIfChanged(after, after.join(s))
```

**[설계 제안]** status에 따른 간선별 갱신(`pending_edge`)은 결과 status SSA 값을 쓰는 첫 `llvm.cond_br`·`llvm.switch`에서 적용한다. status가 분기에 쓰이지 않으면 모든 경우를 합류한다.

**[확인된 사실]** MLIR dense forward 분석은 간선 단위 전이 hook을 제공한다. `visitBlockTransfer(Block *block, ProgramPoint *point, Block *predecessor, const LatticeT &before, LatticeT *after)`는 predecessor 끝의 상태 `before`를 block 시작의 상태 `after`로 옮긴다. 기본 구현은 `join(after, before)`이고 하위 분석이 override할 수 있다 (로컬 `mlir/include/mlir/Analysis/DataFlow/DenseAnalysis.h` L278–L288, 추상 선언 L130–L139). **[설계 제안]** `edge_filter`와 `pending_edge`는 이 hook의 override로 구현한다. override는 `predecessor`의 terminator(`llvm.cond_br`·`llvm.switch`)와 `block`이 그 terminator의 몇 번째 후속인지로 간선의 조건을 정한다. 그 조건을 만족할 수 없는 Joint를 버리고, 해당 status 경우의 `pending_edge`를 적용한 뒤 `after`에 합류한다. `visit()`은 분기 op에서 조건만 기록하고 상태를 바꾸지 않는다.

---

### 6.5 함수 사이 분석

#### 6.5.1 앱·task·dispatch 구조 복원

**[설계 제안]**

1. startup script의 `CFE_APP` 항목에서 앱과 main entry를 얻는다.
2. `CFE_ES_CreateChildTask`의 함수 포인터 인자를 상수로 풀어 child task entry를 얻는다. 풀지 못하면 `unknown task`를 기록한다.
3. main entry에서 `cfs.es.run_loop`이 지배하는 loop를 run loop로 표시한다. 그 안의 `cfs.sb.receive` 성공 분기 아래 MID 비교를 dispatch로 복원한다.
4. dispatch 비교가 함수 static cache를 읽으면, 그 static에 대한 상수 store를 흐름과 무관하게 모은다. **[실측]** sample_app·hk·ci_lab의 dispatch는 `CFE_MSG_GetMsgId` out-parameter를 처음 호출 때 채워지는 static cache와 비교한다. opt pipeline 이후 cache 쓰기는 `...CMD_MID.Value`에 대한 상수 store가 된다 (§7.7.3; `$N/probe/mlir_opt/sample_app.json`).

#### 6.5.2 solver 설정

**[확인된 사실] upstream 기본 동작.**

- solver는 기본으로 interprocedural이다 ([DataFlowFramework.h L300 @ccac700c](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/include/mlir/Analysis/DataFlowFramework.h#L300); 로컬 L281–L298의 `DataFlowConfig::setInterprocedural`).
- 사용자 분석이 돌려면 `DeadCodeAnalysis`와 `SparseConstantPropagation`을 먼저 load해야 한다. 헤더는 이를 'interim fix'라고 부른다 ([Utils.h L23–L32 @ccac700c](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/include/mlir/Analysis/DataFlow/Utils.h#L23-L32)).
- callee entry에서는 알려진 모든 call site의 상태를 합류한다. 즉 context-insensitive다 (§4.5.1).
- 몸체가 없는 callee, 또는 non-interprocedural 설정의 모든 call은 `visitCallControlFlowTransfer(..., ExternalCallee, ...)`로 간다. 기본 구현은 `setToEntryState`다 (로컬 `mlir/lib/Analysis/DataFlow/DenseAnalysis.cpp` L92–L110, 판정 L107; [DenseAnalysis.h L163–L173 @ccac700c](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/include/mlir/Analysis/DataFlow/DenseAnalysis.h#L163-L173)).
- return site를 모두 알지 못하면 entry 상태가 된다 (로컬 같은 파일 L117).

**[해석] 그대로 쓸 때의 문제.** context-insensitive 합류는 6.3.1의 상관을 깬다. `SetState(x)`를 두 handler가 부르면 callee entry에서 두 caller의 Joint가 합쳐진다. 그 결과가 두 caller 모두에게 돌아간다. **[실측]** upstream의 테스트 분석 `-test-last-modified`는 원노트 §44 예(`HandleAttitude → SetState → global.att → Guidance`)에서 `<unknown>`을 돌려준다. private visibility와 driver를 붙여도 같다 (`$N/rw-mlir/mlir_probe/lastmod_llvm.out`, `$N/verify-C20/counter/s44_O0_priv_driver_lastmod.out`). 이 분석은 SSA 값을 기준으로 메모리를 식별하는 테스트 분석이다. 이 결과는 upstream이 제공하지 않는 부분을 보여 줄 뿐, MLIR로 표현할 수 없다는 뜻은 아니다.

**[설계 제안] 선택한 설정.**

1. frontend의 `--inline`으로 작은 helper를 먼저 펼친다 (S1).
2. solver를 `setInterprocedural(false)`로 둔다.
3. 남은 호출은 모두 `ExternalCallee` hook으로 오게 한다. 이 hook을 override해 summary(6.5.3) 또는 API 모델(6.5.4)을 적용한다.
4. `loadBaselineAnalyses`는 그대로 먼저 load한다.

#### 6.5.3 summary

**[설계 제안]** 함수 summary는 호출 그래프의 SCC를 아래에서 위로 계산한다.

```text
Summary(fn) = { writes : Field ⇀ Expr(Param(i), Global(f′), Const, Unknown),   # 반환 시점의 마지막 쓰기
                events : 정규 사건열 (예: 'w(f) ; tx(m)?'),                       # 사건 추출용
                ptrOut : 반환·out-param 포인터의 출처,
                pathCond : 쓰기마다의 경로 조건 (CondField 위의 술어) }
apply(Summary, call, σ♯):
  각 Joint J에 대해 writes를 call site의 실인자 출처로 치환해 적용한다.
  pathCond가 J.val로 결정되면 그 경로만 적용하고, 결정되지 않으면 두 경우를 모두 J로 나눈다.
재귀 SCC: writes가 고정점에 이르지 않으면 해당 field를 Unknown(recursion)으로 둔다.
```

**[해석]** caller마다 따로 적용하므로 caller 사이의 Joint가 섞이지 않는다. 원노트 §44의 `global.att ← ATTITUDE_MID`는 `writes(SetState) = {att ↦ Param(0)}`와 `HandleAttitude`의 실인자 출처 `Msg(ATTITUDE_MID, att, site)`의 결합으로 얻는다.

#### 6.5.4 외부 호출

| 대상 | 처리 **[설계 제안]** | 근거 |
| --- | --- | --- |
| lifted cFE API | 6.4의 전이 | §3 |
| OSAL·PSP·libc 목록 | 기본 가정 `A_ext`: 포인터 인자가 가리키는 객체만 쓴다. 앱 전역은 쓰지 않는다. 보고서마다 이 가정을 적는다 | **[확인된 사실]** IKOS도 "Extern functions (without implementation) do not update global variables"를 가정한다 ([analyzer/README.md L528–L546 @ac7f7c17](https://github.com/NASA-SW-VnV/ikos/blob/ac7f7c1738976cabc58c6a53413df6e458995c38/analyzer/README.md#L528-L546)). **[해석]** 여기서는 숨은 가정이 아니라 보고되는 가정으로 둔다 |
| 그 밖의 unknown | 인자 포인터 대상과 escape된 전역 field에 `Unknown(call)`. 관련 후보는 `unknown` 수준 | AP4 |

#### 6.5.5 함수 포인터와 dispatch table

**[실측]**

- import한 mission 모듈(cFE 5개 module과 17개 앱 디렉터리, 154개 파일)에 indirect `llvm.call`이 73곳 있다 (`$N/rw-mlir/import_exp/import_summary.txt` L5). 73곳 중 55곳이 세 곳에 몰려 있다. SBN 계열 30곳(`sbn_app.c` 17, `sbn_udp_if.c` 10, 그 밖 3), CF 16곳(7개 파일), ES generic pool 9곳(`cfe_es_generic_pool.c`)이다 (`$N/rw-mlir/import_exp/import_results.json`의 파일별 `indirect_calls`; §7.5.2).
- `static const` 함수 포인터 table을 통한 호출은 MLIR `CallGraph`에서 `<Unknown-Callee-Node>` 간선 하나만 갖는다. -O0과 -O2 모두 같다 (`$N/rw-mlir/cir_test/t2.callgraph.out`, `$N/verify-C20/counter/t2_O2.callgraph.out`; 로컬 `mlir/lib/Analysis/CallGraph.cpp` L154).

**[설계 제안] 해결 순서.**

1. 상수 table: `llvm.mlir.global constant` 초기화자의 `llvm.mlir.addressof @fn` 목록과 GEP index에서 대상 집합을 얻는다. index가 상수가 아니면 table 전체를 대상 집합으로 둔다.
2. task entry: `CFE_ES_CreateChildTask`의 함수 포인터 인자를 같은 방법으로 푼다.
3. 나머지: 외부 points-to 결과를 대상 집합으로 받는다. 예: SVF. **[확인된 사실]** SVF README는 field·flow-sensitive points-to를 제공한다고 쓴다 (§4.5.2, README만 확인; 실행하지 않음). **[확인된 사실]** MLIR 기반 points-to인 PoTATo는 로컬 LLVM 1053047a에 대해 build가 실패했다 (`$N/rw-mlir/potato_build.log`).
4. 그래도 풀리지 않으면 `ExternalCallee`로 처리하고 정밀도 표시 `unknownCallee`를 붙인다.

#### 6.5.6 task 사이 분석은 solver 밖에서

**[확인된 사실]** MLIR DataFlow 헤더에는 thread·interleaving 개념이 없다 (`grep -i 'thread|interleav|concurren|parallel'` 결과 없음, §4.5.1, 검증 C20). **[설계 제안]** 그래서 task 사이 순서는 별도 그래프 엔진에서 계산한다. 입력은 task마다 solver가 남긴 사건 기록(Facts)과 summary의 정규 사건열이다. 사건 그래프는 MLIR module 안의 `cfs.system` 영역에 기록하여 source location을 유지할 수 있다. 계산은 6.2의 `BUILD_HB`다.

---

### 6.6 RequiredHB를 얻는 방법

**[원노트 구상]** 원노트 §48은 필요 순서의 출처로 framework semantics, data dependency, explicit contract를 든다. 원노트 §49는 annotation·외부 계약을 허용한다. **[확인된 사실]** TaxDC §8.4는 메시지 타이밍 결함 검출이 order·atomicity 명세의 획득과 그 위반의 정적·동적 검출에 집중해야 한다고 제안한다. 명시적 오류 검사에서 거꾸로 명세를 추론하는 방법도 제안한다. 다만 이것은 구현·평가된 도구가 아니라 연구 방향의 제안이다 ([TaxDC](https://raw.githubusercontent.com/ucare-uchicago/ucare-html/87a6a05aa681a3f4d8993e0b84f786c9b994babc/pdf/asplos16-TaxDC.pdf) §8.4; §1.1). **[해석]** 따라서 아래 규칙의 기여는 '명세 후 검사'라는 틀이 아니라 cFS에서 `Req`를 만드는 구체 규칙에 있다.

**[설계 제안] 비순환 규칙.** `Req`의 생성은 `G0`·`G_A`를 입력으로 받지 않는다 (AP2). 코드가 "이미 순서를 지키는 것처럼 보인다"는 이유로 `Req`를 지우지 않는다. `Req`는 6.7에서 `G` 증명으로만 해소된다. 모든 `Req`는 근거 ID를 가진다.

| 규칙 | 범주 | 생성되는 `Req` | 생성 조건 | 계약 없을 때의 기본값 | 근거 |
| --- | --- | --- | --- | --- | --- |
| R-F1c | F1 | `sub(b,m,p) ≺ first tx(a,m)` | 계약 `must_receive_first(b, m)` | 생성하지 않음 | **[확인된 사실]** SB에는 durability·latch가 없다 (§3 M3) |
| R-F1i | F1 (추론, 신뢰 낮음) | 같음 | ① `tx(a,m)`이 `once`다. ② `b`의 소비 `r(b,f,u)`가 `Msg(m)` 출처를 요구한다(R-F2 참조). ③ `m`의 다른 발행 site가 없다. ④ `b`가 `m`을 다시 요청하는 경로(request–reply, 재전송)가 소스에 없다 | — | **[해석]** 주기 발행이면 다음 메시지로 회복되므로 생성하지 않는다 (수정본 §4.4) |
| R-F2 | F2 | `w(b,f,Msg(m)) ≺ r(b,f,u)` | `u`에서 어떤 Joint의 `asg(f)`가 `Init`·`Const`이고, 다른 Joint에서는 `Msg`·`Tbl`이다. 즉 코드가 메시지 값을 쓰는 경로와 기본값을 쓰는 경로가 같은 사용에 섞인다 | 생성한다. 단 `allows_default(f)` 계약이 있으면 생성하지 않는다 | **[해석]** 데이터 의존에서 나온 필요 순서다. 원노트 §48의 2번 |
| R-F3 | F3 | `w(b,f) ≺ hdl(b,m_β)` (각 β 주기 안에서) | 계약 `requires_updated_since(f, β)` | 생성하지 않음. 사용 위치·출처는 정보로만 출력 | §1.2 F3 |
| R-F4 | F4 | `u`가 읽는 `F′`의 `cls`가 모두 같다 | 계약 `requires_same_instance(F′)` 또는 `requires_same_cycle(F′, β)` | 생성하지 않음. latest-value 조합 목록만 출력 | **[해석]** 수정본 §6.3: 다른 시점의 값이 섞인 것만으로 오류가 아니다 |
| R-F5 | F5 | `provide(R) ≺ use(R)` | ① API 전제: `CFE_TBL_Share(name)`에는 owner의 `Register` 성공, `OS_*GetIdByName(name)`에는 같은 이름의 생성이 앞서야 한다. ② 늦은 초기화: `b`의 handler가 읽는 field가 `b`의 init 경로(`ready(b)` 이후 포함)에서만 설정되면, 그 init 쓰기 ≺ 그 handler의 MID를 보내는 다른 앱 `a`의 첫 발행 | 생성한다 (API 규칙) | **[확인된 사실]** CF #184·cFE #198 (§1 F5) |
| R-F6 | F6 | typestate: `tget(h,SUCCESS∨INFO) ≺ 사용 ≺ trel(h)`, 재`tget` 이후 옛 포인터 사용 금지, NULL 포인터 역참조 금지 | API 규칙 | 생성한다. 순서 결함과 API 오용을 따로 센다 (§1.2) | **[확인된 사실]** §3 TBL-1, TBL-2, M10, M11 |
| R-F7c | F7 | `rcv_c(m1) ≺ rcv_c(m2)` | 계약 `order(c, m1, m2)` | — | §1.2 F7 |
| R-F7i | F7 (추론) | 같음 | ① `m2`의 발행자가 `m1`을 받은 handler 안에서 `m2`를 보낸다(인과 사슬). ② `c`의 `m2` handler가 `m1` handler가 쓴 field를 읽는다 | — | **[해석]** 데이터 의존과 인과에서 나온 필요 순서다. **[실측]** v7.0.x의 relay 역전 (§3 SB-4) |
| R-F8 | F8 | sharer의 `UNREGISTERED` 분기에 `tunreg`가 있다 ≺ owner 재등록 | API 규칙 (cFE ≥ v7.0.0) | 생성한다 | **[확인된 사실]** §3 TBL-7 |

**[설계 제안] 계약 문법.** 원노트 §49의 형식을 P 언어의 deferred·ignored 개념으로 넓힌다. **[확인된 사실]** P는 상태마다 deferred·ignored event 집합을 두고, 둘 다 아닌 event가 처리되지 않으면 'unhandled event'로 보고한다 ([MSR-TR-2012-116](https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/tr-8.pdf), §2).

```text
# [설계 제안] 앱별 외부 계약 파일 (YAML 또는 MLIR attribute로 같은 내용)
contract GUIDANCE {
  must_receive_first   ATTITUDE_INIT_MID            # F1: 첫 메시지를 놓치면 안 됨
  may_drop             CFE_EVS_LONG_EVENT_MSG_MID   # F1 계열 경고 억제 (TO_LAB 같은 의도된 지연)
  allows_default       GUIDANCE_Data.gain           # F2: 기본값 사용 허용
  validate(ATTITUDE_MID) = GUIDANCE_ValidateAtt     # Val(f) 판정용 검증 술어
  requires_same_instance { att.q, att.w }           # F4: 같은 packet
  requires_same_cycle   { att.q, pos.r } boundary SCH_WAKEUP_MID   # F4: 같은 β 주기
  requires_updated_since pos.r boundary SCH_WAKEUP_MID             # F3
  timestamp att.t kind=measurement clock=CFE_TIME unit=subseconds  # 6.3.3 timestamp 층
  order(CONTROL, ARM_MID, FIRE_MID)                 # F7
}
```

**[설계 제안] 오류 처리에서 추론하는 `Req` (선택).** TaxDC의 방법을 따른다. 사용 직전의 명시적 오류 검사를 단서로 쓴다. 예: `if (ptr == NULL) { CFE_EVS_SendEvent(..., CFE_EVS_EventType_ERROR, ...); return; }`. 이런 검사가 있으면 개발자가 그 반대 순서를 오류로 본다는 단서로 쓴다. 이렇게 얻은 `Req`는 `inferred-from-error`로 표시하고 신뢰 수준을 낮게 둔다.

---

### 6.7 판정: 후보 → 실행 가능성 → 재현

#### 6.7.1 판정 수준

| 판정 | 조건 **[설계 제안]** | 보고 |
| --- | --- | --- |
| `GUARANTEED` | `G0 ⊢ e1 ≺ e2` | 보고하지 않음. 증명 경로만 기록 |
| `GUARANTEED_UNDER(A′)` | `G0 ⊬`이고 `G_A ⊢ e1 ≺ e2`. `A′`는 쓰인 가정 | 정보로 보고. 가정 목록 포함 |
| `VIOLATED_ALWAYS` | `G0 ⊢ e2 ≺ e1` | 후보로 보고. 순서와 무관한 결정적 결함이면 순서 결함 집계에서 뺀다 (§1.4) |
| `CANDIDATE` | 위 셋이 아님 | 실행 가능성 확인으로 넘긴다 |
| `UNKNOWN` | 관련 사건·출처에 정밀도 표시(`corrLost`, `unknownCall`, `unknownCallee`, `unresolvedMid`, `eds`)가 있다 | 따로 센다 (AP4) |

**[설계 제안] 조건 4(관측 가능한 차이) 확인.** `CANDIDATE`가 되기 전에 다음을 확인한다. 순서가 뒤집힌 경우 사용 `u`에 도달하는 출처가 바뀌는가? 즉 `u`의 Joint 집합에 `Init`·`Const`·옛 class의 출처가 남는가? 그 차이가 분기·출력·`cfs.sb.transmit` payload로 흘러가는가? 흘러가지 않으면 `NO_EFFECT`로 기록하고 보고하지 않는다.

#### 6.7.2 실행 가능성 확인 (모델상 가능)

**[설계 제안]** 후보마다 관련 앱만 잘라 유한 상태 모델을 만든다.

| 모델 요소 | 내용 | Σ 매개변수 |
| --- | --- | --- |
| task | 후보 사건에 이르는 task의 CFG를 사건 alphabet으로 사영한 자동자. CondField 값과 사용 field의 출처 class만 상태로 둔다 | — |
| pipe | 길이 `depth_eff(p)`의 FIFO와 (MID, pipe)별 `MsgLim` 계수기 | `depth_eff = min(요청, 배치 상한)`; **[실측]** non-root native에서 상한 10 (§3 SB-1, OS-2) |
| SB transmit | §3 M1의 put 열. sender 선점 지점은 put 사이 | cFE 버전 (AP6) |
| ES startup | `sys(S)` 진행과 soft timeout을 비결정 선택으로 둔다 | `STARTUP_SCRIPT_TIMEOUT_MSEC` |
| scheduler | Σ1: 1 CPU, 고정 priority 선점. 같은 priority는 비결정. Σ2: 다중 CPU, 임의 interleaving | **[확인된 사실]** OSAL→Linux priority 대응과 permissive fallback (§3 OS-5) |
| 한계 | MID별 발행 수, loop 반복 수, 탐색 깊이 | 보고서에 기록 (수정본 §12.5) |

**[설계 제안]** 탐색기는 `e2`가 `e1`보다 먼저 일어나고 6.7.1의 차이 조건이 성립하는 trace를 찾는다. 찾으면 그 trace가 witness다. witness는 사건 열과 각 사건의 source location(file:line:col), Σ, 한계를 가진다. 찾지 못하면 "한계 안에서 발견 안 됨"으로 남긴다. 이 결과를 안전 증명으로 바꾸지 않는다 (수정본 §12.5). 탐색 도구(TLA+, Promela, 자체 탐색기)는 아직 정하지 않았다. **[확인된 사실]** 비교 사례로, ROSInfer는 추론한 상태기계를 PlusCal/TLA+로 내보내 LTL 성질을 검사한다 ([ROSInfer PDF](https://raw.githubusercontent.com/clegoues/clegoues.github.io/master/assets/papers/Duerschmid2024ROSInfer.pdf), 전문; 검증 C02).

**[확인된 사실] source location의 가용성.** `--mlir-print-debuginfo`로 import한 파일별 결과에서 `llvm.call` 8830개와 `llvm.load` 26378개가 모두 file:line:col을 가졌다 (`$N/rw-mlir/import_exp/loc_by_op.txt`). 상수 operand에는 위치가 없다. 그래서 MID 값의 위치는 그 값을 쓰는 op의 위치와 S2의 AST 위치로 보고한다 (§7.9).

#### 6.7.3 선택: native cFS build에서의 재현

**[설계 제안]** witness를 native cFS build에서 재생한다. 성공하면 수준을 `재현`으로 올린다.

| 항목 | 방법 | 근거 |
| --- | --- | --- |
| build | bundle README 절차: `make native_std.prep`, `make native_std.install` | **[실측]** install 48.8 s, compile warning 0, gcc 13.3.0 (`$N/probe/install.log`, `$N/probe/prep.log`) |
| 실행 권한 | Σ1: root + private IPC namespace(`unshare --ipc`, 내부 `msg_max` 상향). Σ2: `setpriv` uid 65534 | **[실측]** `CAP_SYS_RESOURCE` 없는 root는 `mq_open` EINVAL로 abort(exit 134)했다. uid 65534에서는 queue 깊이가 10으로 잘리고 RT priority가 꺼졌다 (`$N/probe/run_root.log`, `run_nobody.log`, `run_root_ipcns.log`) |
| CPU | Σ1에서 `taskset -c 0` | **[실측]** RR+cpu0에서만 priority 순서로 시작했다 (5/5; `$N/sem-es-tbl/runs/order_repeats.txt`) |
| 순서 강제 | startup script의 priority와 순서. 시험 전용 패치로 witness 지점에 `OS_TaskDelay`를 넣는다 | **[실측]** probe 앱의 2.5 s 사전 지연으로 'Startup Sync failed' → OPERATIONAL 경로를 24/24 실행에서 재현했다 (§3 ES-4) |
| 관측 | `CLOCK_MONOTONIC` 기록. syslog 시각은 쓰지 않는다 | **[확인된 사실]** syslog 시각은 TIME init 때 뛴다 (§3 EVT-4) |
| 기록 | uid, `msg_max`, permissive mode, CPU 집합, cFE·OSAL·PSP commit | §1.3 |

**[해석]** 재현 결과는 Linux native Σ에 대한 것이다. RTOS 비행 build의 증거로 쓰지 않는다 (§3.0).

#### 6.7.4 전체 판정 절차

```text
# [설계 제안] 입력: System, Σ, Contracts, K, bounds
procedure ANALYZE(System, Σ, Contracts):
  M      := FRONTEND(System)                                  # S1–S4
  Tasks  := RECOVER_STRUCTURE(M, System.startup_script)        # S5
  for T in Tasks: (Prov[T], Facts[T]) := SOLVE_DENSE(M, T)     # S6: 6.3–6.5
  Ev     := EXTRACT_EVENTS(Facts)                              # once / first / each
  (G0,GA):= BUILD_HB(Ev, Pub, Sub, System.cfe, A_all)          # S7: 6.2
  Req    := DERIVE_REQ(Ev, Prov, Contracts, API_RULES)         # S8: 6.6, G를 보지 않음
  for (e1, e2, src) in Req:
    if precision_flags(e1, e2):            emit(UNKNOWN, flags);            continue
    if G0 ⊢ e1 ≺ e2:                        record(GUARANTEED, path);        continue
    if GA ⊢ e1 ≺ e2:                        emit(GUARANTEED_UNDER, assumptions(path)); continue
    eff := EFFECT(e1, e2, Prov)                                # 조건 4
    if not eff:                             record(NO_EFFECT);               continue
    sup := FP_CONTROL(e1, e2, eff, Contracts)                  # 6.8
    if sup:                                 record(SUPPRESSED, sup.reason);  continue
    lvl := VIOLATED_ALWAYS if G0 ⊢ e2 ≺ e1 else CANDIDATE
    w   := FEASIBLE(slice(e1, e2), Σ, bounds)                  # 6.7.2
    if w: lvl := MODEL_POSSIBLE
    if w and replay_enabled:
       r := REPLAY(w, native_build, Σ)                         # 6.7.3
       if r.observed: lvl := REPRODUCED
    emit(lvl, root_cause_key(src, e1.site, e2.site), witness=w, Σ, bounds)
```

---

### 6.8 오경고 통제

**[설계 제안]** 아래 통제는 후보를 지우지 않는다. `SUPPRESSED`, `NO_EFFECT`, `GUARANTEED_UNDER`로 이유와 함께 기록한다. 평가에서 정상 대조 사례의 결과를 이 기록으로 확인한다 (§1 H2′).

| 통제 | 분석 안의 기제 | 정상 대조 사례 | 남기는 출력 |
| --- | --- | --- | --- |
| FP1 guard·validity flag | CondField의 값을 Joint에 두고 분기마다 Joint를 거른다 (6.3.2, 6.4.3). 참 분기에서 모든 Joint가 `Msg` 출처이면 F2 `Req`가 생기지 않는다 | **[원노트 구상]** 수정본 §6.1의 `attitude_valid` 패턴. **[확인된 사실]** ROSInfer 구현([states_analyzer.py L30–L61 @78301b86](https://github.com/cmu-rss-lab/rosdiscover/blob/78301b86fca119cda9ad55500dcf800e56bb9504/src/rosdiscover/recover/states_analyzer.py#L30-L61))은 publish의 경로 조건에 쓰이고 다른 함수에서 대입되는 변수를 state 변수로 본다. 논문의 휴리스틱은 이보다 넓다 (검증 C02) | guard 위치와 flag 이름 |
| FP2 status 경로 | 성공 분기에만 사건을 둔다 (AP3). INFO 코드는 양수로 처리한다 | **[확인된 사실]** sample_app #101: `CFE_TBL_INFO_UPDATED`를 실패로 다루어 release를 빠뜨렸다 ([수정 61f657d](https://github.com/nasa/sample_app/commit/61f657d940670cd59311cf1506a5fa9e7b60f8d6); §1 F6) | 코드 집합 |
| FP3 설정된 정책 | 앱 설정 macro와 그 분기를 정책으로 인식한다 | **[확인된 사실]** HK는 입력마다 `DataPresent`를 두고, 누락이 있으면 `MissingDataCtr`를 올린다. 기본 `HK_DISCARD_INCOMPLETE_COMBO`=0이면 불완전한 combined packet도 보낸다 ([hk_utils.c L483–L504](https://github.com/nasa/HK/blob/0dc16b7a7bab71747b9c063cf405ad59a65a40a5/fsw/src/hk_utils.c#L483-L504), [hk_internal_cfg.h L57–L69](https://github.com/nasa/HK/blob/0dc16b7a7bab71747b9c063cf405ad59a65a40a5/fsw/inc/hk_internal_cfg.h#L57-L69)) | 정책 이름과 설정값 |
| FP4 주기 회복 | 발행 site가 run loop 안에 있거나 SCH table 항목이면 R-F1i를 만들지 않는다. F2는 "첫 수신 전 창"으로 정보 보고한다 | **[확인된 사실]** SCH_LAB은 table의 `MessageID`를 rate counter에 따라 보낸다 ([sch_lab_app.c L125–L248](https://github.com/nasa/sch_lab/blob/607e2f90c5f828f3f3ae655a4debacf8b2cc4bbb/fsw/src/sch_lab_app.c#L125-L248)) | 주기 근거(site 또는 table 행) |
| FP5 허용된 기본값 | `allows_default(f)` 계약 | 수정본 §6.2 | 계약 위치 |
| FP6 의도된 늦은 구독 | `must_receive_first`가 없으면 R-F1c를 만들지 않는다. `may_drop`을 계약으로 받는다 | **[확인된 사실]** TO_LAB은 table 기반 telemetry 구독을 `WaitForStartupSync` 뒤로 미룬다. 주석은 시작 시 event 폭주로 생기는 MsgLimit 오류를 피하기 위해서라고 쓴다 ([to_lab_app.c L66–L73](https://github.com/nasa/to_lab/blob/d27c6014cb5d2979140500f635876ee87b81995b/fsw/src/to_lab_app.c#L66-L73)) | — |
| FP7 설계된 latest-value 조합 | `requires_same_*` 계약이 없으면 F4 `Req`를 만들지 않는다. 조합 목록만 출력 | **[확인된 사실]** Ogma 생성 템플릿은 메시지 값을 전역에 복사하고, active 입력일 때만 `copilot_step()`을 부른다 ([copilot_cfs.c L218–L239](https://github.com/nasa/ogma/blob/08b384b4335cce324fac74577adc7f08e61b7cce/ogma-core/templates/cfs/copilot/fsw/src/copilot_cfs.c#L218-L239)). **[해석, 이견 있음]** 입력을 둘 이상 고르면 monitor는 따로 갱신되는 전역의 마지막 값을 함께 읽는다. 이 일반화에는 검증자 사이에 이견이 있었다 (검증 C06; §4 Ogma 템플릿 상세). 그래서 Ogma 생성 앱은 입력 선택과 active/passive 설정을 고정한 경우에만 대조 사례로 쓴다 | 조합 목록 |
| FP8 프레임워크 보장 | §3.9의 A1–A11을 `G0`의 규칙으로 쓴다. 예: core API 사용은 HB-CORE로 해소된다. TIME 기준값의 여러 field는 같은 version에서 온다(A9) | **[확인된 사실]** core 앱 시작은 직렬화되어 있다 (§3 ES-2). TIME은 versioned read를 한다 (§3 EVT-5) | 사용한 규칙 ID |
| FP9 정밀도 손실 | `corrLost`, `unknownCall`, `unknownCallee`, `unresolvedMid`, `eds`가 있으면 `UNKNOWN` | AP4 | 손실 위치 |
| FP10 다중 writer | 한 field를 같은 앱의 둘 이상의 task가 쓰면, run-to-completion 전제(§1 F4)가 깨진다. F4는 `UNKNOWN(atomicity)`로 보고한다 | **[해석]** §1 F4의 전제 | writer task 목록 |
| FP11 중복 제거 | 근본 원인 키 `(Req 규칙, e1 site, e2 site)`로 묶는다 | 수정본 §12.1 | 묶인 경고 수 |

**[설계 제안] validity flag 판정의 설명 규칙.** 6.3의 경로 필터가 실제 계산을 한다. 아래 조건은 진단 설명용이다. `v`가 field 집합 `F`의 validity flag이려면 다음 세 조건이 필요하다.

1. `v := 참` 쓰기마다 같은 활성화에서 `F`의 모든 field가 `Msg`·`Tbl` 출처로 쓰인다.
2. `v := 거짓` 쓰기(초기값 포함)는 `F`의 무효 상태와 함께 있다.
3. `F`를 소비하는 사용이 `v`의 참 분기에 지배된다.

---

### 6.9 진단 형식

**[설계 제안]** 진단은 원노트 §67의 형식에 근거와 증거 수준을 더한다. 아래는 합성 예제를 가정한 출력 예다. 실제 실행 결과가 아니다.

```text
[F2-CANDIDATE → MODEL_POSSIBLE]  root-cause key: (R-F2, guid.c:88, guid.c:142)
Consumer      GUIDANCE::GUID_Step() guid.c:142:17   r(GUIDANCE_Data.att.q)  consume → Control()
Origins at u  Disj{ {att.q ↦ Msg(NAV_ATT_MID, payload.q, guid.c:61)}, {att.q ↦ Init(GUIDANCE_Data)} }
Req           w(att.q, Msg(NAV_ATT_MID)) ≺ r(att.q)   source=R-F2 (data dependence)
G0 proof      none. Missing link: hdl(GUIDANCE, NAV_ATT_MID) vs hdl(GUIDANCE, SCH_WAKEUP_MID)
G_A proof     none (HB-FIFO not applicable: different senders NAV, SCH_LAB)
Guards        none found on path guid.c:120→142 (CondField: GUIDANCE_Data.att_valid not tested)
Witness (Σ1: 1 CPU, RR, cFE 546a0025, bounds: 2 msgs/MID, 3 loop iters)
  1. SCH_LAB tx(SCH_WAKEUP_MID)        sch_lab_app.c:131
  2. GUIDANCE rcv + hdl(SCH_WAKEUP_MID) guid.c:118 → GUID_Step() guid.c:142 reads Init value
  3. NAV tx(NAV_ATT_MID)               nav.c:77
Assumptions   A_ext (OSAL/libc do not write app globals)
Unknowns      none
Not claimed   reproduction on native build (replay not run)
```

---

### 6.10 구현 위험과 남은 미확인 항목

| 위험 | 근거 | 완화 **[설계 제안]** |
| --- | --- | --- |
| upstream이 field·전역 구분 메모리 모델을 주지 않는다 | **[실측]** 6.3 서두 | 6.3의 field 식별과 출처 도메인을 직접 구현한다 |
| import된 함수의 visibility | **[실측]** S3 | closed-world pass |
| context-insensitive 합류 | **[확인된 사실]** 6.5.2 | `setInterprocedural(false)`와 summary |
| indirect call | **[실측]** 73곳, Unknown-Callee | 상수 table 해석, 외부 points-to |
| MLIR 분석 API 변화 | **[실측]** PoTATo가 로컬 LLVM에 대해 `RegionSuccessor`·DataFlow API 오류로 build 실패 (`$N/rw-mlir/potato_build.log`, `potato_build2.log`) | LLVM commit을 고정한다. 자원 효과는 main 쪽 기능에 의존하므로 고정 commit을 정할 때 함께 확인한다 (6.4.1) |
| lifting이 놓치는 쓰기 | **[실측]** offset 0 field, out-parameter, 지역 포인터 (S4) | `cfs.field`의 byte 범위로 offset 0을 식별한다. API 모델에 out-parameter 쓰기를 넣는다. 포인터 대상은 SSA·summary로 추적한다 |
| MID가 table·명령에서 온다 | **[실측]** `CFE_SB_Subscribe*` 47곳 중 8곳 (§1 RQ1) | table 이미지 입력. 명령 기반 간선은 `Pub(m)` 불완전으로 처리(HB-MSG 조건 ②) |
| ClangIR 경로 | **[실측]** 로컬 clang은 CIR을 쓸 수 없다. `-fclangir -S -emit-llvm`은 일반 codegen과 같은 출력을 낸다 (`$N/probe/cir/emit_cir.log`) | LLVM dialect 경로로 시작한다. CIR은 CIR을 켠 build를 고정한 뒤 비교 실험으로 둔다 |
| 같은 모델을 쓴 대안이 같은 결과를 낼 수 있다 | **[확인된 사실]** CodeQL global data flow는 전역을 통해 함수 사이를 순서와 무관하게 잇는다 ([DataFlowPrivate.qll L93–L120](https://github.com/github/codeql/blob/f1d3f1defc5ca73bcf1ed48031fb31c62fd91952/cpp/ql/lib/semmle/code/cpp/ir/dataflow/internal/DataFlowPrivate.qll#L93-L120)). **[실측]** Clang Static Analyzer의 taint는 double payload에서, 그리고 불투명 호출 뒤에서 사라졌다 (`$N/rw-mlir/csa_exp/csa_all.log`) | §1 RQ5·RQ6의 기준선을 같은 SB·ES·TBL 모델로 만들어 비교한다 |

**[미확인]** 다음 항목은 이 설계의 결론에 쓰지 않았다.

- 6.3 도메인의 구체 의미에 대한 건전성 증명과 K 값.
- HB-SYS1을 lock 없는 일반 메모리 쓰기의 가시성으로 넓힐 때 필요한 다중 코어 메모리 순서(`MemOrder`). HB-TACT의 owner `Load` 경로(§3 TBL-6). HB-OSAL의 OSAL 이름 조회 의미. HB-ATX(옛 버전의 atomic tx)의 실행 확인.
- VxWorks queue 순서(HB-FIFO 조건 ④).
- child task가 부모 AppState와 TBL 잠금에 주는 영향의 실행 확인 (§3 ES-8).
- 실행 가능성 탐색기의 선택과 그 비용. 재현 실행에서 순서를 강제하는 패치의 부작용.
- SBN·원격 구독, EDS build, mission의 MSG·MID mapping override.
- 이 설계가 정상 대조 사례에서 실제로 경고 0을 내는지, 주입 사례에서 후보를 내는지. 이 판단은 구현과 평가 뒤에만 할 수 있다.
