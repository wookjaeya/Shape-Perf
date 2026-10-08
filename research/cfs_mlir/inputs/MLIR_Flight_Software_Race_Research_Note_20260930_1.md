# 연구노트  
## MLIR 기반 Flight Software Cross-Application Race 및 Temporal Dependency 분석

**작성 기준일:** 2026-09-30  
**연구 상태:** 초기 문제정의 및 feasibility 설계 단계  
**핵심 플랫폼:** NASA cFS / cFE  
**핵심 분석 기술:** MLIR 기반 정적 분석  
**주요 관심 대상:** Software Bus, application state, startup dependency, Table Services, application/service lifecycle  
**중심 문제:** 전통적인 shared-memory data race가 아닌 Flight Software 특유의 **cross-application ordering / temporal dependency violation**

---

# 1. 연구의 출발점

Flight Software에서 발생하는 concurrency fault는 일반적인 멀티스레드 프로그램의 `data race`와 반드시 동일하지 않다.

전통적인 race detector가 주로 탐지하는 문제는 다음과 같다.

```text
Thread A                Thread B
   │                       │
 write x                write x
   │                       │
   └─────────┬─────────────┘
             ▼
       Shared-memory
          Data Race
```

그러나 Flight Software에서는 서로 다른 application, subsystem, framework service가 **공유 메모리를 직접 공유하지 않더라도**, 다음과 같은 비결정적 순서 관계 때문에 잘못된 상태가 생성될 수 있다.

```text
Application A
     │
   Event
     │
     ▼
Flight SW Framework
     │
     ▼
Application B
     │
State Update
     │
     ▼
Control / Decision
```

문제가 되는 핵심은:

> **어떤 event가 다른 event보다 먼저 발생해야 하는가?**

또는:

> **특정 상태를 소비하기 전에 그 상태가 반드시 초기화되거나 최신 상태로 갱신되었음이 보장되는가?**

이다.

따라서 본 연구는 일반적인 memory-level data race가 아니라 다음 문제를 대상으로 한다.

> **Flight Software applications, messages, framework services, state updates 및 lifecycle event 사이의 필요한 ordering dependency가 프로그램에서 실제로 보장되는지를 정적으로 분석한다.**

---

# 2. 연구의 핵심 문제 정의

본 연구에서 탐지하고자 하는 fault를 잠정적으로 다음과 같이 정의한다.

## Flight-Software-Specific Cross-Application Race

서로 다른 Flight Software application 또는 framework-managed component가 비동기적으로 동작할 때,

1. 시스템의 올바른 동작을 위해 필요한 event/state ordering constraint가 존재하고,
2. 해당 ordering constraint가 synchronization, protocol, framework semantics 또는 명시적 상태 검사를 통해 보장되지 않으며,
3. 가능한 execution ordering에 따라 observable state 또는 시스템 동작이 달라질 수 있는 경우

이를 **Flight-Software-Specific Cross-Application Race**로 정의한다.

이를 간단하게 표현하면:

```text
Required ordering:

E1  →  E2

그러나 실제 프로그램:

E1  ||  E2

결과:

Execution #1
E1 → E2 → 정상

Execution #2
E2 → E1 → 오류
```

이다.

---

# 3. 연구에서 중요한 구분

## 3.1 Data Race

```text
Thread A            Thread B

read/write X        read/write X
        \             /
          shared X
```

공유 memory location에 둘 이상의 실행 주체가 synchronization 없이 접근하는 문제.

본 연구의 **주요 대상이 아니다.**

ThreadSanitizer, Helgrind, conventional static analyzer가 이미 상당 부분 다루고 있기 때문이다.

---

## 3.2 Message Race

```text
Sender A ───────┐
                ▼
             Receiver
                ▲
Sender B ───────┘
```

둘 이상의 message event 사이의 순서가 결정되지 않아 receiver 동작이 달라지는 문제.

Message Race 자체는 MPI, Actor systems, DDS 등의 분야에서 이미 연구되어 있다.

따라서 본 연구에서 Message Race를 새로운 개념처럼 주장하지 않는다.

대신 다음과 같이 위치시킨다.

```text
Flight SW Cross-Application Race
│
├── Message/Event Ordering Race
│
├── Startup Dependency Race
│
├── Temporal State-Consistency Fault
│
└── Resource Lifecycle Ordering Fault
```

즉 Message Race는 본 연구가 다루는 더 큰 문제 중 하나다.

---

# 4. 실제 Flight Software 사례에서 확인되는 문제

연구 방향의 중요한 근거는 이러한 문제가 가상적인 문제가 아니라 실제 Flight Software에서 반복적으로 발생했다는 점이다.

---

# 4.1 Swift/BAT onboard race condition

NASA Neil Gehrels Swift Observatory의 Burst Alert Telescope(BAT)에서는 실제 비행 중 알려진 race condition 때문에 천체의 위치가 잘못 식별된 사례가 여러 차례 보고되었다.

특히 2018년 NASA GCN 보고에서는:

> known race condition 때문에 **incorrect spacecraft attitude information**이 known source에 적용되어 천체가 잘못 식별되었다.

고 명시되어 있다.  
Reference: https://gcn.nasa.gov/circulars/22706

2022년에도 Swift 팀은 onboard software의 known race condition 때문에 활성화되어 있던 Cen X-3가 잘못 식별되었다고 보고했다.  
Reference: https://gcn.nasa.gov/circulars/32397

2023년에는 이를 `"7 minute problem"`이라고 명시하면서 같은 계열의 known race condition으로 잘못된 위치가 계산됐다고 보고했다.  
Reference: https://gcn.nasa.gov/circulars/34691

2026년에도 Swift GCN에서 known software race condition에 따른 source misidentification 사례가 다시 보고되었다.  
Reference: https://gcn.nasa.gov/circulars/43470

따라서 이 문제는 단발적인 historical anomaly가 아니다.

### 추상화

```text
Observation
    │
    │ expected associated state
    ▼
Attitude(t0)

하지만 실제 사용:

Observation(t1)
+
Attitude(t0)

        ↓

Wrong coordinate transformation
        ↓
Source misidentification
```

이 문제는 단순 memory data race라기보다:

- temporal consistency violation
- state version mismatch
- asynchronous state consumption problem

으로 보는 것이 연구적으로 더 적절하다.

---

# 4.2 cFE Core Application Startup Race

NASA cFE Issue #73에는 실제로 다음 제목이 사용된다.

> **Race conditions / dependencies between CFE core apps**

NASA 개발자는 startup phase에서 core application 사이의 race condition이 심각한 문제를 만들 수 있다고 설명한다.  
Reference: https://github.com/nasa/cFE/issues/73

당시 core applications은 대략:

```text
EVS
SB
ES
TIME
TBL
```

순서로 task가 생성됐다.

하지만 priority 때문에 실제 runtime execution에서는 TIME task가 먼저 실행될 수 있었다.

TIME initialization 과정에서:

```text
TIME
  ↓
CFE_SB_CreatePipe()
```

가 실행됐지만,

```text
SB initialization
```

은 아직 완료되지 않았다.

SB는 다시 EVS 기능을 호출했는데 EVS 역시 초기화되지 않은 상태였다.

결국 uninitialized AppID가 사용되며 segfault가 발생했다.

### 핵심 교훈

```text
Task creation order
        ≠
Execution order
        ≠
Initialization completion order
```

이다.

따라서:

```text
"startup.scr에서 먼저 나오는 application"
```

이라고 해서 그 application의 service가 반드시 사용할 준비가 되어 있다고 볼 수 없다.

이 사례는 본 연구에서 가장 중요한 **cross-application startup dependency fault** 사례다.

---

# 4.3 Flight SW에서 우리가 관심을 가지는 공통 구조

앞선 사례들을 하나의 abstraction으로 바꾸면 다음과 같다.

```text
Producer / Service A
        │
        │ Event E1
        ▼
Flight Framework
        │
        ▼
Consumer B
        │
      State S
        │
        ▼
Decision / Control
```

정상 동작을 위해 사실상 다음 constraint가 필요하다.

```text
E1 happens-before Use(S)
```

그러나 코드나 framework가 이를 명시적으로 보장하지 않으면:

```text
Use(S) before E1
```

가 가능한 execution이 존재할 수 있다.

본 연구에서는 이러한 **missing happens-before relation**을 핵심 분석 대상으로 본다.

---

# 5. 왜 cFS가 좋은 연구 플랫폼인가

NASA cFS/cFE는 연구 대상으로 매우 적절하다.

이유는 다음과 같다.

1. 오픈소스 Flight Software framework다.
2. 실제 spacecraft software architecture를 목표로 한다.
3. application 간 통신을 Software Bus로 추상화한다.
4. Table Services, Event Services, Executive Services 등 명시적 framework API가 존재한다.
5. application lifecycle과 startup logic이 존재한다.
6. 실제 race/startup dependency bug가 공개 issue로 남아 있다.
7. communication semantics가 일반 C call graph에 그대로 나타나지 않는다.

NASA의 Software Bus Network 설명에서도 message identifier subscription database를 관리하고 subscriber에게 message를 배포하는 publish/subscribe 구조가 명확히 나타난다.  
Reference: https://software.nasa.gov/software/GSC-16917-1

---

# 6. cFS Software Bus가 분석에서 중요한 이유

일반 C call graph를 보면 다음처럼 보인다.

```text
APP_A
 │
 └─ CFE_SB_TransmitMsg()

APP_B
 │
 ├─ CFE_SB_Subscribe()
 └─ CFE_SB_ReceiveBuffer()
```

Compiler 입장에서는:

```text
APP_A → APP_B
```

라는 관계가 존재하지 않는다.

그러나 실제 system semantic은:

```text
APP_A
  │
Publish(MID_X)
  │
  ▼
Software Bus
  │
Route MID_X
  │
  ▼
APP_B
Subscribe(MID_X)
```

이다.

즉 일반 call graph로는 cross-application dependency가 보이지 않는다.

따라서 본 연구에서 가장 먼저 해야 할 일은:

> **Framework API를 통해 숨어 있는 semantic dependency를 복원하는 것**

이다.

---

# 7. 연구의 핵심 아이디어

## Source-level API → Domain Semantic IR

예:

```c
CFE_SB_CreatePipe(&Pipe, 32, "GUIDANCE_PIPE");
CFE_SB_Subscribe(ATTITUDE_MID, Pipe);
CFE_SB_Subscribe(POSITION_MID, Pipe);
```

일반 compiler IR에서는:

```text
call CFE_SB_CreatePipe
call CFE_SB_Subscribe
call CFE_SB_Subscribe
```

정도로만 보인다.

이를 Flight Software-specific semantic IR로 변환한다.

예:

```mlir
cfs.app @GUIDANCE {

    %pipe = cfs.sb.create_pipe
        name = "GUIDANCE_PIPE"
        depth = 32

    cfs.sb.subscribe
        %pipe,
        #cfs.mid<ATTITUDE>

    cfs.sb.subscribe
        %pipe,
        #cfs.mid<POSITION>
}
```

---

# 8. 왜 MLIR을 사용하는가

MLIR은 단순히 LLVM IR보다 보기 좋은 IR이어서 사용하는 것이 아니다.

MLIR 사용의 핵심 정당성은:

> **서로 다른 abstraction layer의 Flight Software semantics를 하나의 분석 IR 안에서 명시적으로 표현할 수 있기 때문이다.**

MLIR은 custom dialect를 정의할 수 있고, high-level dataflow representation부터 lower-level representation까지 동일 infrastructure에서 분석/변환할 수 있다.  
Reference: https://mlir.llvm.org/docs/LangRef/

또한 MLIR에는 generic dataflow analysis framework가 존재하며 fixed-point iteration과 analysis dependency 관리를 위한 solver infrastructure를 제공한다.  
Reference: https://mlir.llvm.org/doxygen/DataFlowFramework_8h_source.html

---

# 9. MLIR의 구체적인 역할

연구에서 MLIR의 역할은 최소 네 가지다.

## 9.1 cFS framework semantics 표현

```text
CFE_SB_Subscribe
→ cfs.sb.subscribe

CFE_SB_TransmitMsg
→ cfs.sb.publish

CFE_SB_ReceiveBuffer
→ cfs.sb.receive
```

---

## 9.2 Application state dependency 표현

```text
MID_X
 ↓
Receive
 ↓
State.field
 ↓
Control()
```

를 dataflow 관계로 표현한다.

---

## 9.3 Lifecycle semantics 표현

예:

```text
Table Register
 ↓
Acquire
 ↓
Use
 ↓
Release
 ↓
Update
```

---

## 9.4 Cross-application graph 구축

최종적으로:

```text
APP_A
 │
 ├─ Publish X
 │
 └─ Publish Y
       │
       ▼
Software Bus
       │
       ├────→ APP_B
       │       │
       │       └─ update state_X
       │
       └────→ APP_C
               │
               └─ update state_Y
```

를 compiler-visible graph로 만든다.

---

# 10. Front-end 전략

현재 ClangIR(CIR)은 MLIR 기반 C/C++ high-level representation이며 Clang AST와 LLVM IR 사이에 위치한다. 다만 Clang 공식 문서에서도 upstreaming이 아직 진행 중이고 default clang build에는 포함되지 않는다고 명시하고 있다.  
Reference: https://clang.llvm.org/docs/CIR/index.html

따라서 단계적으로 접근한다.

## 초기 prototype

```text
C source
   ↓
LLVM IR
   ↓
MLIR LLVM Dialect
   ↓
cFS API recognition
   ↓
cFS semantic dialect
```

---

## 장기 구조

```text
C/C++
 ↓
Clang AST
 ↓
CIR / MLIR
 ↓
cFS semantic lifting
 ↓
cFS dialect
```

초기 feasibility에서는 compiler frontend 자체를 연구 주제로 만들지 않는다.

---

# 11. 제안하는 cFS MLIR Dialect

초기 dialect 이름:

```text
cfs
```

으로 가정한다.

---

# 11.1 Software Bus Operations

```text
cfs.sb.create_pipe
cfs.sb.subscribe
cfs.sb.unsubscribe
cfs.sb.publish
cfs.sb.receive
cfs.sb.dispatch
```

예:

```mlir
%pipe = cfs.sb.create_pipe
    "GUIDANCE_PIPE"
    depth(32)

cfs.sb.subscribe
    %pipe,
    #cfs.mid<ATTITUDE>

cfs.sb.subscribe
    %pipe,
    #cfs.mid<POSITION>
```

---

# 11.2 Application Operations

```text
cfs.app
cfs.app.init
cfs.app.runloop
cfs.app.ready
cfs.app.exit
```

---

# 11.3 State Operations

```text
cfs.state.define
cfs.state.update
cfs.state.read
cfs.state.consume
```

예:

```mlir
cfs.state.update
    @attitude
    source = #cfs.mid<ATTITUDE>
```

---

# 11.4 Table Operations

```text
cfs.tbl.register
cfs.tbl.share
cfs.tbl.acquire
cfs.tbl.release
cfs.tbl.update
cfs.tbl.unregister
```

---

# 11.5 Lifecycle / Synchronization Operations

```text
cfs.lifecycle.start
cfs.lifecycle.ready
cfs.lifecycle.shutdown

cfs.sync.startup
cfs.sync.system_state
cfs.sync.handshake
```

---

# 12. Dependency Graph

Dialect extraction 이후 다음 graph를 생성한다.

## Node

```text
Application
Message
State
Table
Service
Lifecycle Event
```

## Edge

```text
publish
subscribe
update
consume
initialize
acquire
release
requires
happens-before
```

예:

```text
NAV
 │
 └─ publish
      │
ATTITUDE_MID
      │
      └─ subscribe
             │
          GUIDANCE
             │
             └─ update
                  │
             attitude_state
                  │
                  └─ consume
                         │
                    guidance_step
```

---

# 13. 핵심 분석 원리 — Happens-Before

연구의 핵심 정적 분석은 결국 다음 질문이다.

> **정상 execution을 위해 필요한 happens-before relation이 프로그램으로부터 증명되는가?**

정상 constraint:

```text
E1 → E2
```

그러나 program analysis 결과:

```text
E1 || E2
```

라면 potential race이다.

---

# 14. 분석 대상 1 — Message/Event Ordering Race

예:

```text
APP_A
 ↓
Publish STATE_A

APP_B
 ↓
Publish STATE_B

APP_C
 ↓
Receive / Process
```

APP_C의 결과가 메시지 도착 순서에 따라 바뀐다면:

```text
Execution 1

A
↓
B
↓
C

Execution 2

B
↓
A
↓
C
```

잠재적 ordering race다.

---

# 15. 분석 대상 2 — First-Message / Subscription Race

예:

```text
APP_A

Publish INITIAL_STATE
```

와:

```text
APP_B

CreatePipe
Subscribe(INITIAL_STATE)
```

가 존재한다.

정상 조건:

```text
SubscribeReady(B)
     →
FirstPublish(A)
```

그러나 다음이 가능하면:

```text
Publish
   ↓
Subscribe
```

초기 message가 consumer에 도달하지 않을 수 있다.

이를:

**startup message ordering hazard**

또는

**first-message race**

정도로 분류할 수 있다.

중요:

이 fault가 반드시 모든 cFS configuration에서 발생한다고 주장하지 않는다.

분석기는 **ordering guarantee가 존재하는지 여부를 판별**한다.

---

# 16. 분석 대상 3 — Startup Dependency Race

가장 근거가 강한 분석 대상이다.

```text
APP_A
 ↓
Initialize Service X

APP_B
 ↓
Use Service X
```

정상:

```text
A.ServiceReady
     →
B.UseService
```

하지만 scheduler priority 때문에:

```text
B.UseService
     →
A.ServiceReady
```

가 가능하다면 race다.

NASA cFE #73이 실질적인 근거 사례다.  
Reference: https://github.com/nasa/cFE/issues/73

---

# 17. 분석 대상 4 — Temporal State Consistency

본 연구에서 가장 발전 가능성이 높은 영역이다.

예:

```text
ATTITUDE_MID
     ↓
attitude_state

POSITION_MID
     ↓
position_state
```

그리고:

```c
Control(
    attitude_state,
    position_state
);
```

를 수행한다.

메모리 관점에서는 아무런 race가 없을 수 있다.

그러나:

```text
attitude_state.time = 100
position_state.time = 84
```

일 수 있다.

즉:

```text
State A = current
State B = stale
```

이다.

---

# 18. State Snapshot 개념

본 연구에서는 다음 패턴을 **logical state snapshot**이라고 부른다.

```text
State A ───────┐
               │
State B ───────┼──→ Decision
               │
State C ───────┘
```

여러 asynchronous source로부터 만들어진 state를 하나의 computation에서 동시에 소비한다.

문제는:

```text
A(t1)
B(t2)
C(t3)
```

가 동일 logical epoch을 표현하는지 보장되지 않을 수 있다는 것이다.

---

# 19. State Origin Analysis

MLIR dataflow를 이용하여 state provenance를 추적한다.

예:

```text
ATTITUDE_MID
     ↓
ProcessAttitude()
     ↓
state.attitude
     ↓
Guidance()
```

분석 결과:

```text
Origin(state.attitude)
    = ATTITUDE_MID
```

마찬가지로:

```text
Origin(state.position)
    = POSITION_MID
```

---

# 20. Temporal Metadata

각 state에 abstract metadata를 전파한다.

예:

```text
StateInfo {
    source_mid
    update_event
    epoch
    freshness
    initialization
}
```

구현 초기에는 실제 timestamp를 정확하게 계산할 필요는 없다.

예를 들어 lattice를:

```text
UNINITIALIZED
CURRENT
POSSIBLY_STALE
UNKNOWN
```

정도로 시작할 수 있다.

---

# 21. 분석 가능한 기본 property

## Property 1 — Initialization

```text
state가 consume되기 전에
반드시 한 번 이상 update되어야 한다.
```

---

## Property 2 — Freshness

```text
state update 이후
허용되지 않은 event boundary를 넘어
state가 사용되지 않아야 한다.
```

---

## Property 3 — Same-cycle consistency

```text
State A와 State B가
같은 control cycle에서 update되어야 한다.
```

---

## Property 4 — Explicit timestamp check

다음과 같은 코드가 존재하면:

```c
if (abs(att.time - pos.time) < DELTA)
{
    Control(...);
}
```

분석기는 consistency guard가 존재한다고 판단할 수 있다.

---

# 22. Swift 사례와의 개념적 연결

Swift BAT race에서:

```text
Observation
+
incorrect attitude
```

가 결합되어 source misidentification이 발생했다.  
Reference: https://gcn.nasa.gov/circulars/22706

이를 본 연구 abstraction으로 표현하면:

```text
Observation state
      │
      ├───────┐
              ▼
          Coordinate
          Transform
              ▲
              │
Attitude state
```

그리고:

```text
TemporalCoherent(
    Observation,
    Attitude
)
```

가 보장되지 않는 상태다.

따라서 본 연구의 state-consistency analysis와 직접적인 문제 구조상의 연관성이 존재한다.

단:

**Swift BAT source code에 본 분석을 직접 적용했다고 주장하면 안 된다.**

Swift 사례는 문제의 실재성을 보여주는 motivating incident다.

---

# 23. 분석 대상 5 — Table Lifecycle Ordering

cFS Table Services도 framework-managed resource semantics를 가진다.

일반적으로:

```text
Register
 ↓
GetAddress
 ↓
Use
 ↓
ReleaseAddress
```

형태의 lifecycle이 존재한다.

이를 MLIR로:

```mlir
%tbl = cfs.tbl.acquire @GAIN_TABLE

%gain =
    cfs.tbl.read %tbl

cfs.tbl.release %tbl
```

로 표현한다.

---

# 24. Table 관련 검출 대상

## A. Missing Release

```text
Acquire
 ↓
Use
 ↓
return
```

Release 없음.

---

## B. Use after Release

```text
Acquire
 ↓
Release
 ↓
Use pointer
```

---

## C. Update blocked by outstanding reference

```text
Acquire
 ↓
long computation
 ↓
Update request
```

resource lifetime 문제.

---

## D. Cross-application stale handle

향후 연구 범위.

```text
App A owns Table
      │
      └──── App B shares
              │
App A reload
              │
              ▼
        old reference used
```

---

# 25. 연구에서 제외하거나 후순위로 둘 대상

범위를 명확히 제한한다.

## 제외 1 — Conventional shared-memory data race

```text
x++
```

같은 문제는 연구 중심이 아니다.

---

## 제외 2 — Linux driver race

BurstCube와 같은 kernel/driver race는 motivation에는 포함 가능하지만 분석 대상에서는 제외한다.

---

## 제외 3 — lock-free algorithm verification

별도의 concurrency verification 분야다.

---

## 제외 4 — MPI message matching

본 연구는 MPI 프로그램용 Message Race detector가 아니다.

---

## 제외 5 — 완전한 RTOS scheduling verification

priority inversion, WCET, schedulability 분석 자체를 목표로 하지 않는다.

다만 scheduling이 ordering relation을 결정하는 보조 정보는 될 수 있다.

---

# 26. 핵심 연구 질문

잠정적으로 다음 하나를 메인 연구 질문으로 둔다.

> **Can flight-software-specific cross-application race conditions be detected statically by recovering event, state, and lifecycle dependencies from cFS applications in an MLIR-based intermediate representation?**

한국어:

> **cFS Flight Software application의 event, state 및 lifecycle dependency를 MLIR 기반 중간표현에서 복원함으로써 application 간 race condition을 실행 전에 정적으로 검출할 수 있는가?**

---

# 27. 하위 질문

### Q1

Software Bus API로 암시된 application 간 communication relation을 compiler IR에서 정확하게 복원할 수 있는가?

### Q2

Application state가 어떤 Software Bus message에서 유래했는지를 정적으로 추적할 수 있는가?

### Q3

정상 동작에 필요한 happens-before relation이 보장되지 않는 execution path를 검출할 수 있는가?

### Q4

여러 asynchronous state가 하나의 decision에서 사용될 때 temporal consistency가 보장되는지 분석할 수 있는가?

### Q5

startup/lifecycle semantics를 포함시켰을 때 기존 source-level concurrency analyzer가 놓치는 Flight Software-specific fault를 추가로 검출할 수 있는가?

---

# 28. 연구 가설

## H1

cFS framework API를 semantic IR로 lift하면 일반 call graph에서 보이지 않는 cross-application dependency를 복원할 수 있다.

## H2

복원된 dependency graph와 happens-before analysis를 이용하면 injected startup/order fault의 상당수를 실행 없이 검출할 수 있다.

## H3

state provenance를 함께 추적하면 일반 message ordering 분석으로는 발견하지 못하는 stale-state / inconsistent-snapshot fault를 검출할 수 있다.

---

# 29. 연구 시스템 구조

```text
                    cFS Application Source
                              │
                              ▼
                    C / C++ Frontend
                              │
                              ▼
                   LLVM IR / CIR / MLIR
                              │
                              ▼
                    cFS API Recognition
                              │
                              ▼
                    Semantic Lifting Pass
                              │
                              ▼
                      cFS MLIR Dialect
                              │
           ┌──────────────────┼──────────────────┐
           │                  │                  │
           ▼                  ▼                  ▼
        SB Graph          State Graph      Lifecycle Graph
           │                  │                  │
           └──────────────────┼──────────────────┘
                              ▼
                     Dependency Graph
                              │
                              ▼
                    Happens-Before Analysis
                              │
               ┌──────────────┼───────────────┐
               ▼              ▼               ▼
          Startup Race    Message Race    State Consistency
               │              │               │
               └──────────────┼───────────────┘
                              ▼
                       Diagnostics
```

---

# 30. Phase 1 — 최소 Prototype

처음부터 모든 cFS API를 지원하지 않는다.

초기 지원 API:

```text
CFE_SB_CreatePipe
CFE_SB_Subscribe
CFE_SB_ReceiveBuffer
CFE_SB_TransmitMsg
```

이 네 종류만으로 시작한다.

---

# 31. Prototype Scenario A — Subscriber Readiness

APP A:

```c
Publish(INIT_STATE_MID);
```

APP B:

```c
CreatePipe();
Subscribe(INIT_STATE_MID);
```

분석:

```text
Required:

Subscribe(B)
   HB
Publish(A)
```

ordering evidence가 없으면:

```text
Potential cross-application startup race
```

---

# 32. Prototype Scenario B — Uninitialized State

APP B:

```c
while (run)
{
    Receive(...);

    Control(State.attitude);
}
```

일부 path에서는:

```text
ATTITUDE_MID
```

를 받기 전에 `Control()`이 실행 가능.

분석:

```text
state.attitude
=
possibly uninitialized
```

diagnostic.

---

# 33. Prototype Scenario C — Inconsistent Snapshot

```text
ATTITUDE_MID
      ↓
attitude

POSITION_MID
      ↓
position

attitude + position
      ↓
Guidance
```

두 state 사이에 coherence check가 없음.

분석:

```text
Potential temporal state-consistency violation
```

---

# 34. Phase 2 — Startup Semantics

다음 정보를 추가한다.

```text
application startup
priority
startup synchronization
service initialization
ready state
```

그리고 cFE #73을 모델링한다.

목표:

```text
TIME
 ↓
SB dependency
 ↓
EVS dependency
```

가 필요한데 runtime order가 이를 위반할 수 있음을 분석기가 탐지하는 것.

---

# 35. Phase 3 — Table Services

추가 API:

```text
CFE_TBL_Register
CFE_TBL_GetAddress
CFE_TBL_ReleaseAddress
CFE_TBL_Update
CFE_TBL_Share
```

검증:

```text
Acquire → Use → Release
```

protocol correctness.

---

# 36. Phase 4 — Cross-Application State Analysis

최종적으로 가장 중요한 분석.

```text
Message
 ↓
Handler
 ↓
State
 ↓
Computation
```

dataflow를 추적한다.

예:

```text
SENSOR_MID
   ↓
SensorHandler
   ↓
CurrentSensor
   ↓
Navigation
```

---

# 37. Fault Injection Dataset

연구 평가를 위해 정상 cFS application에 의도적으로 race pattern을 삽입한다.

단순 synthetic toy만으로 끝내면 논문의 설득력이 떨어질 수 있으므로:

1. minimal synthetic benchmark
2. 실제 공개 cFS application 변형
3. 실제 cFE historical bug 재현

의 세 층을 갖는 것이 좋다.

---

# 38. Fault Category

## F1 — Publish before subscriber ready

```text
publish
 ↓
subscribe
```

---

## F2 — Use before initialization

```text
state read
 ↓
message update
```

---

## F3 — Stale state

```text
new A
+
old B
```

---

## F4 — Multi-stream inconsistent snapshot

```text
A(epoch 5)
+
B(epoch 3)
```

---

## F5 — Startup service dependency

```text
consumer starts
before
provider initialized
```

---

## F6 — Table lifecycle misuse

```text
acquire
release
use
```

---

# 39. Ground Truth

각 benchmark에 다음 정보를 명시한다.

```text
Fault ID
Affected applications
Required ordering
Possible violating ordering
Expected analyzer result
Runtime manifestation
```

---

# 40. 평가 지표

단순히 "몇 개 찾았다"로 끝내면 안 된다.

최소 다음을 측정한다.

## Detection

```text
True Positive
False Positive
False Negative
Precision
Recall
```

---

## Analysis Scalability

```text
Analysis runtime
IR size
Number of applications
Number of MID
Number of dependency edges
```

---

## Semantic Coverage

```text
지원한 cFS API 수
추출된 message relation 수
추출된 state provenance 수
추출된 lifecycle relation 수
```

---

# 41. 중요한 비교 대상

직접적인 경쟁 도구가 없을 가능성이 높다.

따라서 비교는 기능적으로 해야 한다.

## Baseline A — Conventional compiler call graph

보이는 것:

```text
APP_A → CFE_SB_TransmitMsg
APP_B → CFE_SB_ReceiveBuffer
```

보이지 않는 것:

```text
APP_A → MID_X → APP_B
```

---

## Baseline B — ThreadSanitizer / race detector

shared-memory race는 탐지 가능.

그러나:

```text
message ordering
startup dependency
stale message-derived state
```

는 원칙적으로 분석 대상이 아니다.

따라서 동일 benchmark를 실행하여:

```text
Conventional detector: no warning
Proposed analyzer: semantic warning
```

을 보여줄 수 있다.

---

# 42. MLIR 사용 정당성 검증

논문 심사에서 반드시 나올 질문:

> 왜 그냥 Clang static analyzer나 Python parser를 사용하지 않았는가?

답은 단순히 "확장성이 좋다"가 되어서는 안 된다.

MLIR 사용이 실제 기여가 되려면 다음을 보여야 한다.

```text
C-level control/data flow
        +
cFS API semantic
        +
message dependency
        +
state provenance
        +
lifecycle information
```

을 하나의 IR 안에서 유지한다.

즉:

```text
Compiler IR
+
System Architecture IR
```

의 통합이다.

---

# 43. MLIR을 억지로 쓰지 않기 위한 조건

다음 결과만 나오면 MLIR은 불필요하다.

```text
grep CFE_SB_Subscribe
grep CFE_SB_Transmit
→ graph 출력
```

이 정도는 source parser로 충분하다.

MLIR이 정당화되려면 반드시 다음 중 일부가 필요하다.

- interprocedural dataflow
- control-flow sensitive analysis
- state provenance propagation
- condition/guard recognition
- application semantics + compiler semantics 통합
- reusable analysis pass
- multi-level lowering

---

# 44. 최소한 확보해야 하는 MLIR-specific contribution

본 연구에서는 최소 다음까지 가야 한다.

```text
Receive message
   ↓
function call
   ↓
struct field assignment
   ↓
another function
   ↓
decision computation
```

를 추적한다.

예:

```c
HandleAttitude(msg)
{
    SetState(msg->att);
}

SetState(x)
{
    global.att = x;
}

Guidance()
{
    Control(global.att);
}
```

여기서:

```text
global.att
← ATTITUDE_MID
```

를 interprocedurally 복원할 수 있어야 한다.

이 지점부터 MLIR dataflow 사용이 의미를 가진다.

---

# 45. 정적 분석 알고리즘 초안

각 program point에 abstract state를 둔다.

예:

```text
StateProperty =
{
    initialized,
    sources,
    lastUpdateEvent,
    temporalClass
}
```

---

## Transfer: Receive

```text
Receive(MID_X)

state.source += MID_X
```

---

## Transfer: Update

```text
state.field = msg.field

Origin(state.field)
    = Origin(msg)
```

---

## Transfer: Merge

branch가 합쳐질 경우:

```text
initialized:
    TRUE + FALSE
        → MAYBE

origin:
    A + B
        → {A,B}
```

---

## Transfer: Consume

consume 시:

```text
initialized == FALSE/MAYBE
→ warning

temporal coherence unknown
→ warning candidate
```

---

# 46. Happens-Before Relation 종류

하나의 HB relation만 쓰기보다는 source를 구분한다.

```text
HB_program
HB_message
HB_lifecycle
HB_sync
HB_protocol
```

전체 relation:

```text
HB =
HB_program
∪ HB_message
∪ HB_lifecycle
∪ HB_sync
∪ HB_protocol
```

---

# 47. Race 판정

필요한 ordering:

```text
RequiredHB(E1, E2)
```

인데:

```text
HB(E1,E2) == false
```

이고

```text
Concurrent(E1,E2) == possible
```

이면 potential race.

즉:

```text
Race(E1,E2)
=
RequiredHB(E1,E2)
∧
¬ProvenHB(E1,E2)
∧
PotentialConcurrent(E1,E2)
```

정도로 formalization 가능.

---

# 48. 중요한 문제: RequiredHB를 어떻게 아는가

이 부분이 연구의 난점이다.

모든 ordering requirement를 자동으로 추론하는 것은 어렵다.

따라서 세 가지 출처를 사용할 수 있다.

## 1. Framework semantics

예:

```text
Subscribe must be ready
before required first message
```

---

## 2. Data dependency

```text
state must be initialized
before use
```

---

## 3. Explicit contract

예:

```mlir
cfs.requires_fresh
    @attitude,
    @position,
    max_skew = 20ms
```

---

# 49. Annotation / Contract 방식

완전 자동 분석에 집착할 필요는 없다.

예:

```text
@requires_fresh(
    attitude,
    position,
    max_delta=100ms
)
```

같은 source annotation 또는 external contract를 허용할 수 있다.

그 후 MLIR attribute로:

```mlir
cfs.contract.temporal
    states = [@attitude, @position]
    max_skew = 100
```

로 표현한다.

이 방식은 실제 Flight Software engineering에도 더 현실적일 수 있다.

---

# 50. 연구에서 반드시 피해야 할 과장

## 금지 주장

> 모든 Flight Software race를 탐지한다.

불가능.

---

## 금지 주장

> cFS Software Bus에는 원래 race가 있다.

너무 포괄적이고 틀릴 수 있다.

---

## 금지 주장

> Message Race라는 새로운 개념을 제안한다.

기존 분야가 존재한다.

---

## 금지 주장

> MLIR 자체가 race condition을 자동으로 잡는다.

MLIR은 infrastructure다.

분석 semantics와 pass는 우리가 구현해야 한다.

---

# 51. 안전한 주장

> 본 연구는 cFS framework API의 의미를 MLIR 기반 system representation으로 lift하여 일반 compiler representation에서 명시적으로 드러나지 않는 inter-application dependency를 복원한다.

---

> 복원된 dependency를 이용하여 startup-order, message-order 및 state-consistency와 관련된 잠재적 cross-application concurrency fault를 정적으로 탐지한다.

---

> 본 연구는 conventional shared-memory data race detection을 대체하려는 것이 아니라 Flight Software framework 수준의 semantic dependency analysis를 보완한다.

---

# 52. 예상 contribution

### Contribution 1

**cFS-aware MLIR representation**

Software Bus, application lifecycle, state provenance 및 Table Services를 compiler-level IR에 명시적으로 표현.

---

### Contribution 2

**Cross-application dependency recovery**

C call graph에서 보이지 않는:

```text
APP
→ Message
→ APP
→ State
```

dependency 복원.

---

### Contribution 3

**Flight-SW-specific race analysis**

다음 category의 정적 검출:

```text
startup ordering
message/event ordering
uninitialized state use
temporal state inconsistency
```

---

### Contribution 4

**Reproducible evaluation**

실제 cFS/cFE 환경에서 fault injection + historical bug pattern을 이용해 분석 정확도 평가.

---

# 53. 논문의 핵심 차별점

가장 중요하게 유지해야 할 문장:

> **The novelty is not race detection itself. The novelty lies in recovering flight-software-specific semantic dependencies that conventional race detectors and compiler call graphs do not model.**

한국어:

> **본 연구의 신규성은 race detection 자체가 아니라, 기존 race detector나 일반 compiler call graph가 표현하지 못하는 Flight Software framework 수준의 semantic dependency를 복원하고 이를 이용해 application 간 ordering fault를 정적으로 분석하는 데 있다.**

---

# 54. 관련 연구 구조

Related Work에서는 다음으로 나눈다.

```text
1. Shared-memory data race detection
2. Message race / message-order analysis
3. Event-driven concurrency analysis
4. Static analysis of publish/subscribe systems
5. Flight Software verification
6. cFS architecture/static analysis
7. MLIR-based program analysis
```

Message Race는 **2번의 한 절**이다.

연구 전체를 Message Race history 중심으로 만들지 않는다.

---

# 55. 연구 제목 후보

아직 확정하지 않는다.

### 후보 1

**Static Detection of Cross-Application Race Conditions in Flight Software Using MLIR**

---

### 후보 2

**Recovering Temporal Dependencies in Flight Software for Static Race Detection**

---

### 후보 3

**MLIR-Based Static Analysis of Inter-Application Race Conditions in cFS**

---

### 후보 4

**Beyond Data Races: Static Analysis of Cross-Application Temporal Dependencies in Flight Software**

후보 4는 다소 강한 제목이므로 최종 결과 수준에 따라 결정.

---

# 56. 처음 구현할 최소 시스템

첫 prototype은 절대로 Table Services까지 동시에 하지 않는다.

## MVP

지원:

```text
CFE_SB_CreatePipe
CFE_SB_Subscribe
CFE_SB_ReceiveBuffer
CFE_SB_TransmitMsg
```

추출:

```text
Application
MID
Publisher
Subscriber
Message handler
State update
State consumer
```

검출:

```text
1. use-before-message initialization
2. subscriber/readiness ordering
3. multi-stream state inconsistency
```

---

# 57. 성공 기준 — MVP

다음이 되면 Phase 1 성공.

```text
APP_A
 │
 Publish MID_X
 │
 ▼
Software Bus
 │
 ▼
APP_B
 │
 Handler
 │
 ▼
state.x
 │
 ▼
Control()
```

가 source에서 자동 복원되고,

의도적으로 삽입한:

```text
Use-before-update
```

또는:

```text
A/B state inconsistency
```

를 경고할 수 있어야 한다.

---

# 58. 이후 단계

## P1 — API lifting

CFE_SB API → cfs dialect.

---

## P2 — Message graph

publisher/subscriber graph 자동 추출.

---

## P3 — State provenance

MID → handler → state → consumer.

---

## P4 — Basic HB

program order + message relation.

---

## P5 — Fault injection

3~5개 minimal cFS applications.

---

## P6 — Startup semantics

cFE startup race reproduction.

---

## P7 — Temporal contract

multi-stream freshness analysis.

---

## P8 — Table lifecycle

후속 확장.

---

# 59. 우선 구현하지 않을 것

초기에는 다음을 보류한다.

```text
- full RTOS scheduler model
- WCET
- priority inversion
- interrupt race
- kernel driver race
- distributed cFS/SBN node timing
- multicore memory model
- lock-free structure
- complete temporal logic model checking
```

연구가 산으로 가는 것을 막기 위함이다.

---

# 60. 가장 중요한 연구 철학

이 연구는:

```text
"cFS에 race가 많다"
```

를 증명하는 연구가 아니다.

또:

```text
"MLIR이 race를 잘 잡는다"
```

를 증명하는 연구도 아니다.

정확한 목표는:

> **Flight Software framework abstraction 뒤에 숨겨진 inter-application temporal dependency를 compiler analysis 대상으로 끌어내는 것**

이다.

즉 핵심은:

```text
Hidden System Dependency
          ↓
Explicit Compiler IR
          ↓
Static Verification
```

이다.

---

# 61. 연구의 가장 중요한 구조

최종적으로 연구를 다음 한 그림으로 생각한다.

```text
                FLIGHT SOFTWARE

     App A        App B        App C
       │            │            │
       └────────────┼────────────┘
                    │
              cFS Framework
          ┌─────────┼─────────┐
          │         │         │
          SB       TBL       ES
          │         │         │
          └─────────┼─────────┘
                    │
                    ▼

           Hidden Dependencies

       Message    State    Lifecycle
          │         │         │
          └─────────┼─────────┘
                    │
                    ▼

                MLIR IR

                    │
                    ▼

             Dependency Graph

                    │
                    ▼

        Happens-Before Reasoning

                    │
          ┌─────────┼─────────┐
          ▼         ▼         ▼

       Message    Startup    State
        Race       Race    Inconsistency
```

---

# 62. 연구의 현재 판단

현 시점에서 이 연구는 **진행 가치가 있다.**

근거:

1. 실제 Flight Software에서 race/timing-dependent fault가 존재한다.
2. cFE 자체에서도 application initialization dependency race가 확인된 바 있다.
3. Swift 같은 실제 비행 시스템에서 state/timing 관련 onboard race가 반복적으로 보고됐다.
4. cFS는 application communication과 lifecycle을 framework API로 명시하므로 semantic lifting에 유리하다.
5. 일반 compiler call graph는 SB communication topology를 직접 표현하지 못한다.
6. 일반 data-race detector는 cross-application message/state temporal dependency를 주 분석 대상으로 삼지 않는다.
7. MLIR은 custom dialect와 dataflow analysis infrastructure를 제공하므로 이 semantic layer를 구현하기 적합하다.

다만 아직 다음은 **검증되지 않았다.**

- 기존 논문 중 cFS SB의 동일 문제를 이미 완전히 해결한 분석기가 존재하지 않는지
- state temporal consistency를 어느 수준까지 자동 추론할 수 있는지
- realistic cFS application에서 false positive를 실용적인 수준으로 억제할 수 있는지
- MLIR 방식이 다른 static-analysis implementation 대비 실제로 의미 있는 이점을 제공하는지

이 네 가지는 향후 연구에서 반드시 검증해야 한다.

---

# 63. 다음 Work 채팅에서 가장 먼저 할 일

새 연구 Work에서는 바로 구현으로 들어가지 말고 다음 순서로 진행한다.

## Step 1

**관련연구 systematic scan**

목표:

```text
Flight SW race detection
cFS static analysis
publish/subscribe race
event-order analysis
message race
temporal consistency
```

중 이미 동일한 연구가 존재하는지 검증.

---

## Step 2

**cFS API semantic inventory**

다음 API 전체 목록화:

```text
Software Bus
Table Services
Executive Services
Event Services
OSAL synchronization
```

그리고 race analysis와 직접 관련 있는 것만 선별.

---

## Step 3

**Race taxonomy 확정**

현재 후보:

```text
R1 Message/Event Ordering
R2 Startup Dependency
R3 Uninitialized State
R4 Temporal State Consistency
R5 Resource Lifecycle
```

실제 evidence와 구현 가능성을 보고 축소.

---

## Step 4

**최소 cFS test applications 작성**

3개 App:

```text
SENSOR
NAVIGATION
CONTROL
```

정도로 구성.

---

## Step 5

**MLIR representation 설계**

최소 Ops부터:

```text
cfs.app
cfs.sb.publish
cfs.sb.subscribe
cfs.sb.receive
cfs.state.update
cfs.state.consume
```

---

## Step 6

**첫 분석 구현**

가장 먼저:

```text
Use-before-initialization
```

을 잡는다.

그다음:

```text
message ordering
```

그다음:

```text
state consistency
```

순으로 확장한다.

---

# 64. 한 문장 연구 정의

새 Work에서 맥락이 끊겼을 때는 다음 문장을 기준점으로 삼는다.

> **우리는 cFS Flight Software에서 일반적인 shared-memory data race가 아니라, Software Bus 메시지, application state, startup 및 framework resource 사이의 숨겨진 ordering dependency를 MLIR에서 복원하여 cross-application race와 temporal consistency violation을 실행 전에 정적으로 탐지하는 연구를 한다.**

---

# 65. 가장 중요한 경계선

연구가 진행되면서 방향이 흔들릴 경우 다음 기준으로 판단한다.

### 포함할 것

> "이 오류는 Flight Software framework의 application/event/state semantics를 알아야 탐지할 수 있는가?"

YES → 연구에 포함.

### 제외할 것

> "일반 C race detector만으로 동일하게 탐지 가능한가?"

YES → 본 연구 중심에서는 제외.

이 기준을 유지하면 연구의 novelty가 흐려지는 것을 막을 수 있다.

---

# 66. 현재 핵심 우선순위

현 단계에서는 다음 순서로 중요하다.

**1순위**

```text
SB message → state → computation
dependency recovery
```

**2순위**

```text
startup dependency / readiness
```

**3순위**

```text
multi-stream temporal state consistency
```

**4순위**

```text
Table Services lifecycle
```

Table은 연구 범위를 넓히기 전에 1~3이 실제로 작동하는 것을 확인한 이후 붙인다.

---

# 67. 최종 지향점

최종 분석기가 다음과 같은 diagnostic을 출력하는 것이 목표다.

```text
[FS-RACE-01]

Potential cross-application state-ordering violation

Consumer:
    CONTROL_APP::ControlStep()

State:
    attitude

Origin:
    ATTITUDE_MID
    NAV_APP

Required condition:
    attitude must be initialized before ControlStep()

Observed analysis:
    execution path exists where ControlStep()
    is reachable before ATTITUDE_MID updates attitude

Related applications:
    NAV_APP
    CONTROL_APP
```

또는:

```text
[FS-TEMPORAL-02]

Potential inconsistent state snapshot

Function:
    GUIDANCE_APP::ComputeGuidance()

Inputs:
    attitude <- ATTITUDE_MID
    position <- POSITION_MID

Observation:
    values originate from independent asynchronous
    message streams.

No temporal coherence guard was identified.

Potential effect:
    computation may combine states from different
    logical update epochs.
```

이 정도 수준까지 가야 연구 아이디어가 실제 tool contribution으로 연결된다.

---

# 68. 결론

이 연구의 핵심은 **race라는 익숙한 문제를 Flight Software domain semantics 수준으로 끌어올리는 것**이다.

기존 분석:

```text
Memory
Thread
Lock
```

본 연구:

```text
Application
Message
State
Lifecycle
Framework
Time / Ordering
```

따라서 가장 중요한 연구 메시지는 다음과 같다.

> **Flight Software의 중요한 concurrency fault 중 일부는 memory access conflict가 아니라 application과 framework service 사이의 잘못되거나 보장되지 않은 temporal dependency에서 발생한다. 이러한 dependency는 일반 compiler IR에서는 숨겨져 있지만 cFS API semantics를 MLIR로 lift하면 정적 분석 대상으로 만들 수 있다.**

현재 연구의 가장 유력한 핵심 조합은:

```text
cFS
+
Software Bus
+
Application State Provenance
+
Happens-Before Analysis
+
MLIR
```

이다.

그리고 연구 진행 초기에는:

```text
Message Race 연구 전체
```

를 따라가는 것이 아니라,

```text
실제 Flight Software에서 확인된
race/timing-dependent failure
```

를 문제의 출발점으로 유지한다.

**Message Race는 related work이자 일부 fault category일 뿐 연구 전체의 정체성은 아니다.**
