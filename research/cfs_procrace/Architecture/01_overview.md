# Kairos 개요: cFS의 process-level 준비·순서 위반 판정

이 문서는 NASA cFS v7.0.1에서 앱·서비스 task 사이의 준비(readiness)·순서 위반을 판정하는 설계 Kairos를, spec을 읽지 않은 연구자가 15–25분 안에 파악하도록 요약한다. 문제의 정의, 기존 race 탐지와의 차이, 핵심 아이디어, 판정 체계, 주장하는 것과 주장하지 않는 것을 다룬다. spec에 없는 설계 요소·수치·인용은 더하지 않으며, 세부는 spec의 해당 절로 넘긴다.

상태: 설계 문서 — 구현·측정 전. 규범 원문: [spec/SPEC_FINAL.md](spec/SPEC_FINAL.md)(revision 1.1: 외부 검토 EXT-20261012 반영, spec §0.7)

## 목차

- [0. 읽는 법](#0-읽는-법)
- [1. 문제](#1-문제): process-level의 뜻, 준비 계약과 발생의 상태, 세 판단, 네 가지 예(R2, C1, SB 구독 전 송신, OSAL RESERVED 구간), 다섯 설계 사례
- [2. 기존 탐지 기법의 한계](#2-기존-탐지-기법의-한계): 순서가 잘못 정해진 사용, lock으로 보호된 순서 위반, soft barrier
- [3. Kairos의 핵심 아이디어](#3-kairos의-핵심-아이디어): 한 장 그림(그림 D0), 하나의 모형과 세 해석, 증명 동반 판정과 trust ledger, 미수정 바이너리 관측
- [4. 판정 체계](#4-판정-체계): (σ, M) 칸과 경계, 안전성 기록과 class, 진행(progress) 결과, 근거 축, "경보 없음 ≠ 안전"
- [5. 기여 가설과 하지 않는 주장](#5-기여-가설과-하지-않는-주장)
- [6. 다음에 읽을 곳](#6-다음에-읽을-곳)
- [부록 A. 처음 읽을 때 필요한 용어](#부록-a-처음-읽을-때-필요한-용어)

---

## 0. 읽는 법

**근거 표기**는 spec 머리말을 따른다. 이 문서의 수치는 spec과, spec이 인용하는 기록([LED], [R2g], [BM], [R07])에서 왔다. Kairos가 직접 측정한 값은 아직 없다.

| 표기 | 뜻 |
|---|---|
| [F] | 코드·기록·보고서 인용이 있는 사실 |
| [M] | 측정값. 누가 측정했는지 출처를 붙인다(예: [R07], [BM], [R2g]) |
| [I] | 추론 또는 설계상 추론. 실행하지 않았다 |
| [D] | 설계 결정. spec §11의 ADR이 기록한다 |
| [U] | 미확인 |

**문헌**은 [R04]·[R05]·[R06]이 확인한 만큼만 인용하고 그 상태 표지를 붙인다. V-doc은 원문을 직접 읽은 것, V-abs는 서지와 초록 수준의 주장을 확인한 것, V-me는 해당 보고서 작성자가 검색으로 서지·초록 수준을 확인한 것, P는 일부만 확인한 것, UNVERIFIED는 확인하지 않아 결론에 쓰지 않는 것이다([R04 §0.2]; [R05 §0.2]; V-me는 [R06 §9]).

**인용 형식.** 이 폴더의 모든 문서가 같은 형식을 쓴다(spec 머리말).

| 표기 | 가리키는 것 |
|---|---|
| (spec §4.4), (spec App. A.1) | [spec/SPEC_FINAL.md](spec/SPEC_FINAL.md)의 절과 부록 |
| [R01]–[R07], 예: [R03 §4] | [background/](background/README.md)의 배경 조사 보고서와 그 절. "[R07] L340"처럼 L이 붙으면 그 보고서의 줄 번호다 |
| [LED], [R2g], [R2c], [BM], [POL] | 연구 기록: `cases/ledger.md`, `results/R2_gdb_v701.md`(gdb), `results/R2_cord_v701.md`(CORD), `env/BASELINE_MEASURE.md`, `CONDITIONS_POLICY.md`(모두 `research/cfs_procrace/` 아래) |
| [A], [B], [C], 예: [B §4.4] | 세 설계안 [history/proposal_A-static-model.md](history/proposal_A-static-model.md)(CAIRN), [history/proposal_B-dynamic-predictive.md](history/proposal_B-dynamic-predictive.md)(Sextant), [history/proposal_C-hybrid-layered.md](history/proposal_C-hybrid-layered.md)(Kairos)와 그 절 |
| [J1]–[J3], 예: [J1 §6], [J2-9], [J1 X8] | 세 설계안을 각각 형식(J1), cFS(J2), 방법(J3) 관점에서 심사한 문서([history/judgment_J1-formal.md](history/judgment_J1-formal.md) 등)와 그 절·항목 번호. "[J1 X8]"의 X8은 심사 J1 안의 항목 번호이고, 검토 X1–X3과 관계없다 |
| [X1]–[X3], 예: [X1-C1], [X2-14 (d)] | 세 적대적 검토(X1 형식적 건전성, X2 cFS 충실도, X3 실현 가능성·평가, [history/](history/))와 그 issue 번호. C는 critical, M은 major, m은 minor다. 처리는 [spec/ISSUE_RESOLUTION.md](spec/ISSUE_RESOLUTION.md) |
| [X3, here] | 검토 X3이 이 머신에서 직접 잰 값 [M] |
| v0.1, 예: v0.1 §7.3 | 사용자가 2026-10-08에 쓴 설계안 v0.1과 그 절. [R06]이 비판한 대상이고, 이 폴더에는 없다([background/06_critique_v01.md](background/06_critique_v01.md) 머리말에 파일 이름이 있다) |
| `path:line` | 고정된 cFS tree(cFS `088b2fa8`) 기준의 코드 위치. "`:835`"처럼 경로가 없으면 바로 앞에 적은 파일이다 |

**이름이 겹치는 기호.** 같은 기호가 여러 뜻으로 쓰인다. 예를 들어 "PI 결정 D4"(native σ_R1 실현 여부)와 "그림 D4"(연구 워크플로우 그림), "보장 G5"(spec §6.3)와 "gap G5"([R04 §5]), "단계 S0"와 "Init snapshot S0", HB stratum S/O와 cold/warm 층(stratum)이 다르다. 이 폴더의 문서는 언제나 앞에 종류를 붙인다. 전체 목록은 [10_decisions_adr.md §0.2](10_decisions_adr.md#02-이름-충돌-주의)다.

**연구 조건 정책.** 모든 경계·예산·매개변수는 세 부류 중 하나다. reference(코드 상수나 고정 문서)에서 온 값은 `from-reference`, 자원이 유한해서 사람이 고른 값은 `deviation`(이유·효과·필요성을 기록), reference가 정하지 않았고 값도 고르지 않은 것은 `unspecified_by_reference`다(spec 머리말; `CONDITIONS_POLICY.md`).

---

## 1. 문제

### 1.1 "process-level"의 뜻

POSIX native port에서 cFS의 앱, core 서비스 task, child task는 모두 한 OS process 안의 thread다 [F] (spec §1.1; `osal/src/os/posix/src/os-impl-tasks.c:568`). 그래서 여기서 *process-level*은 OS process 경계가 아니다. 서로 다른 cFS 앱·서비스 task·실행 문맥이 하는 논리적 작업 사이의 순서를 말한다(spec §1.1).

관심 대상은 raw memory word가 아니라 **framework resource**다. 이름 붙은 OSAL object, ES의 app/task/lib record, 서비스 identity, SB route와 pipe, pipe 용량, TBL registry 항목, phase와 app state, CDS block, callback, loader symbol, file이 여기에 든다(spec §1.1).

분석 중에 memory-level data race를 발견할 수도 있다(예: EVS filter counter를 lock 없이 갱신하는 것, [R02 §3.3]). 이것은 별도 class **M**에 넣고 순서 위반으로 세지 않는다(spec §1.1).

### 1.2 준비 계약 (readiness contract)

Kairos가 판정하는 단위는 **readiness contract**(준비 계약)의 instance다. 준비 계약은 "어떤 연산이 framework resource를 쓰려면 그 resource가 생애주기의 특정 상태에 있어야 한다"는 규칙이다(spec §0.1, §1.6).

생애주기는 resource 종류(kind)마다 상태 기계로 적는다. 이 방식을 **typestate**라 부른다. 객체의 현재 상태가 어떤 연산이 허용되는지를 정한다는 개념이다(Strom & Yemini 1986 [V-me]; spec §1.3).

계약 하나는 다음 요소를 가진다(spec §1.6).

| 요소 | 뜻 | cFS 예 |
|---|---|---|
| use pattern `U` | 어떤 연산과 위치가 "사용"인가 | `CFE_EVS_SendEventWithAppID` 호출 |
| `Req` | 사용 시점에 참이어야 할 조건들의 논리곱. 각 항(conjunct)을 따로 평가한다 | 넘긴 AppID 값이 유효함 ∧ 그 AppID가 EVS에 등록됨(spec §1.4) |
| `Oracle` | 각 조건에 대응하는 framework 자신의 유효성 검사. Kairos가 실행하지는 않는다 | `EVS_GetAppDataByID(a) != NULL && EVS_AppDataIsMatch(...)` |
| `Allowed` | 준비되지 않았을 때 받아들이는 결과. 정해진 어휘와 reference에서 온 관찰 범위(horizon)를 가진다. 상태를 정하는 것은 호출자 수준 Allowed다(아래 "세 판단") | CF의 횟수 제한 재시도는 `retry_until_ready`; EVS 자기 ID guard는 `documented_drop` |
| `appreq` | 이 사용이 호출하는 앱이나 미션을 위해 이뤄야 할 것을 적은 reference. 기본값은 `unspecified_by_reference`다. 공급자(supplier) 쪽 문서나 Kairos의 규칙에서 가져오지 않는다 | K-ID: `cfe_evs.h:169–172` |
| `harm` | 허용되지 않은 결과의 피해 종류 | `event_lost_silently`, `panic`, `task_never_runs` |
| `prov` | 출처 기록. 종류(API 문서, 기능 요구사항 `requirement`, 코드 주석, issue 등), 쪽(side: `supplier`, `caller`, `rule`), 본문의 해시 | `cfe_evs.h:168–172`(`caller`) |
| `scope` | 발생이 결정되기 전에 종료 요청(shutdown request)이 없었던 발생에만 적용한다. scope는 instance의 일부다 | 결정이 종료 요청 뒤에 오는 발생은 K-TEARDOWN instance로 따로 보고 |

**발생과 상태.** 사용 지점이 한 번 실행된 것을 발생(occurrence)이라 한다. 발생은 호출에서 시작하고, 늦어도 호출이 돌아올 때 결정된다. 발생마다 서수(ordinal) `(a, k)`가 붙는다. actor instance `a`가 그 key로 시작한 k번째 발생이라는 뜻이고, `a`는 실행이 달라도 같은 이름으로 부른다(spec §1.2, §1.6). 상태는 유한 실행(finite execution) 위에서 정한다(spec §1.6, §1.8).

- **NOT_EVALUATED**: 호출이 모든 conjunct의 LP를 지나기 전에 돌아왔다(예: 이름 검색 전의 인자 검사). 준비도 미준비도 아니고, 어떤 진술도 이것을 세지 않는다.
- **READY**: 모든 조건이 참이다.
- **TOLERATED**: 준비되지 않았지만 Allowed의 horizon monitor가 ACCEPT했다.
- **VIOLATING**: 준비되지 않았고, monitor가 REJECT했거나 Allowed가 `none`이다.
- **PENDING**: 준비되지 않았고 monitor가 아직 OPEN이다. PENDING은 결코 TOLERATED로 세지 않는다.
- **NOT_READY_UNJUDGED**: 준비되지 않았는데 호출자 수준 Allowed가 `unspecified_by_reference`다.

**horizon monitor**는 Allowed 항목 하나를 감시 대상 actor의 ACCEPT 사건과 REJECT 사건으로 읽은 것이다. 예를 들어 `returned_and_tested`는 상태를 검사한 뒤 정상 연속(normal continuation)에 이르면 ACCEPT이고, 그 전에 오류 종료나 오류 반환이 오면 REJECT다. 프로세스를 끝내는 단계(process-terminal step) 뒤에는 어떤 상태도 정하지 않는다(spec §1.6).

이 문서에서 "준비·순서 위반"은 VIOLATING 발생을 말한다. Allowed가 정해지지 않은 계약에서는 NOT_READY_UNJUDGED 발생을 말하고, 결과 label이 어느 뜻인지 밝힌다(spec §1.6).

**세 판단**(spec §1.6 "Three judgements"; ADR-043). 준비되지 않은 사용은 세 번 따로 판단하고, 기록도 따로 둔다 [D].

| 판단 | 묻는 것 | 근거 | 기록 |
|---|---|---|---|
| framework 수준 | 자원이 준비되지 않았을 때 공급자가 무엇을 돌려주고 무엇을 하는가 | op의 결과 대응표와 그 문서화된 결과, O4(a), `kz conform`, SB counter oracle | `fw_doc`(instance마다), `fw_conf`(칸마다). 이 판단으로 TOLERATED가 되는 발생은 없다 |
| 호출자 수준 | 사용하는 코드가 무엇을 이뤄야 하고, 어떤 미준비 결과를 받아들이는가 | `appreq`와 호출자 수준 Allowed. `caller` 기록이 있는 항목이 여기에 든다. PI 항목 QT-4의 기본값에서는 `rule` 기록만 있는 항목도 들고, 그 ACCEPT는 rule-backed로 따로 센다 | 상태, class, 결과 label, `harm`은 이 판단만으로 정한다 |
| 관측된 결과 | 실제로 무슨 일이 일어났나(실패, 손실, 종료) | trace나 모형의 연속, run oracle의 관측 횟수 | `consequence`. 앞의 두 판단과 관계없이 미준비 발생마다 적는다 |

`appreq`와 `caller` 항목이 충돌하면 두 reference를 함께 기록하고, PI가 우선순위를 정할 때까지 그 발생은 NOT_READY_UNJUDGED로 둔다. 이 설계 뒤의 PI 항목(STATUS 결정 T2 재질의, QT-3, QT-4)은 아직 열려 있고, PI가 다르게 정하면 ADR-043을 고친다. 용어 결정 T1이 나올 때까지 "contract"(계약)라는 말은 그대로 쓴다(spec §0.7 항목 4).

**계약 이름.** K-로 시작하는 이름은 계약 template family다. `K-<family>.<변형>` 꼴로 쓴다. 예를 들어 K-ID는 "넘긴 identity 값이 유효하고 등록되어 있다", K-ROUTE는 "송신 시점에 route가 있다", K-SELF.es는 "호출자 자신의 ES App·task record가 LIVE다"이다. 전체 목록과 [R03 §4]의 계약 종류 RC1–RC15와의 대응은 [05_cfs_mapping.md §3](05_cfs_mapping.md#3-계약-라이브러리와-15개-계약-종류)에 있다.

### 1.3 네 가지 구체 예

| 예 | 준비되어야 할 것 | 먼저 일어날 수 있는 것 | 결과 | 상태 |
|---|---|---|---|---|
| R2 | SB의 AppId 셀 | ES·EVS init 안 SB 코드의 이벤트 송신 | `ILLEGAL_APP_ID`, 이벤트 소실 | 관측됨 [M] (gdb) |
| C1 | 다른 앱이 만드는 OSAL counting semaphore | CF의 이름 조회 | 조회 실패 → 재시도 또는 오류 | v7.0.1 기본 구성에서는 실행 안 됨 [F] |
| SB 구독 전 송신 | `(MsgId, pipe)` route | 송신 | 성공을 반환하며 메시지 소실 | 카운터로 관측됨 [M] |
| OSAL RESERVED 구간 | 이름은 보이지만 아직 LIVE가 아닌 OSAL object | 이름 또는 ID 조회 | 즉시 오류와 lock 누수, 또는 task 소실 | 코드 읽기에서 나온 후보 [I] |

#### R2: SB의 AppId가 발행되기 전에 쓰인다 (cFE #2663)

- **자원.** `CFE_SB_Global.AppId`다. SB task가 자기 init에서 `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`로 채운다(`cfe_sb_task.c:138`) [F] (spec §1.3, App. A.1).
- **사용.** ES와 EVS task가 자기 init에서 `CFE_SB_CreatePipe`, subscribe, transmit 같은 SB API를 부른다. SB 코드는 호출자의 task 안에서 실행되면서 `CFE_EVS_SendEventWithAppID`에 SB의 AppId를 넘긴다(spec §1.2, §3.9).
- **결과.** 이때 AppId는 아직 0(= `CFE_ES_APPID_UNDEFINED`)이다. `CFE_SB_Global`은 0으로 초기화된 전역이고 EarlyInit도 0을 쓴다. EVS는 `ILLEGAL_APP_ID`를 돌려주고, SB 호출자는 이 상태를 버린다. 이벤트는 조용히 사라진다 [F] (spec App. A.1 steps 2–3, 8).
- **세 판단**(spec App. A.1 step 8). 호출자 수준: `appreq`는 `cfe_evs.h:169–172`(caller 쪽)이고 Allowed는 `none`, harm은 `event_lost_silently`다. framework 수준: `ILLEGAL_APP_ID`는 `\retval`로 문서화된 오류 결과다(`fw_doc` = `documented-result`). 관측된 결과: "status ignored and effect lost silently".
- **순서.** core 시작은 직렬화되어 있다. ES startup thread는 생성된 core 순서대로 core task를 하나씩 띄우고, 각각이 RUNNING에 이를 때까지 기다린다(`MainTaskSyncDelay`, `cfe_es_start.c:835`; spec §3.6의 규칙 R19 CORE-SERIAL). ES와 EVS가 SB보다 먼저 오므로 위반 사용은 SB의 AppId 저장보다 앞선다.
- **관측(Kairos 이전의 기록).** v7.0.1 ENV 빌드를 gdb로 세 번 실행한 기록이 있다. 세 번 모두 부팅마다 위반 사용 24건(ES 20, EVS 4)이 나왔고, 모두 SB의 발행보다 먼저였다 [M] ([R2g §2]). gdb는 breakpoint hit마다 모든 thread를 잠시 멈추므로, 이 기록은 교란 없는 스케줄에 대한 증거가 아니다([R2g] 머리말). 그 순서가 core 직렬화에서 온다는 것은 기록 자신의 추론이다 [I] ([R2g §2]). Kairos는 이것을 certificate로 증명하려 한다(§3.3).
- **주의.** 24라는 수는 Kairos의 결과가 아니다. ENV의 version table이 있는 basis(스케줄을 뺀, 실행을 정하는 모든 조건, §4.1)에서만 맞는 수이고, version table이 없으면 11건이다(spec §6.4 항목 21; App. A.1 step 12).
- **성격.** R2는 race가 아니라 순서가 고정된 의존 오류다(spec §6.4 항목 12).

#### C1: CF가 다른 앱이 만들 semaphore를 이름으로 찾는다 (CF #184)

- **자원.** OSAL counting semaphore다. 이름은 CF table 필드 `chan[i].sem_name`에서 온다. 만드는 쪽은 다른 앱이다. 공개된 예로 nasa/bp가 init에서 `OS_CountSemCreate`를 부른다 [F] ([LED §1a]).
- **사용.** CF init이 `OS_CountSemGetIdByName`으로 semaphore를 찾는다. 수정 전에는 한 번만 시도했고, 실패하면 앱이 오류로 종료했다 [F] ([LED §1a]).
- **순서.** 생성이 조회보다 먼저임을 보장하는 장치가 없다. 두 앱 모두 startup phase를 기다리지 않는다. OSAL table lock은 각 호출을 원자적으로 만들 뿐, 두 호출의 순서는 정하지 않는다 [F] ([LED §1a]).
- **수정.** `NAME_NOT_FOUND`일 때만 지연을 두고 재시도하는 bounded retry다. 순서 보장은 아니다 [F] ([LED §1b]). spec은 재시도 횟수를 빌드에서 상수 평가한 유효값 `CF_STARTUP_SEM_MAX_RETRIES`로 다룬다(spec §0.5).
- **v7.0.1.** 기본 table의 이름이 비어 있어서, 기본 구성에서는 이 조회 경로가 실행되지 않는다. 이런 경로를 latent하다고 한다 [F] ([LED §2]; spec App. A.4 step 1).

#### SB 구독 전 송신: 구독자가 생기기 전에 보낸 메시지는 조용히 사라진다

- **자원.** route `sb.route(MsgId, pipe)`다. SB에서 구독(subscribe)은 동기화 edge가 아니라 **상태**다. 송신할 때 SB는 자기 lock 아래에서 route를 찾고 목적지 목록을 snapshot한다. 늦게 구독한 pipe는 이미 보낸 메시지를 받지 못한다 [F] (spec §3.9 "Subscribe-before-send", 규칙 R8).
- **결과.** route가 없으면 SB는 `NoSubscribersCounter`를 올리고 `SEND_NO_SUBS` 이벤트를 낸 뒤, 송신자에게 `CFE_SUCCESS`를 돌려준다 [F] (spec §3.9).
- **관측.** 기본 bundle에서 부팅마다 `NoSubscribersCounter`가 15–26이었다 [M] ([BM §0]; spec §8.1 RQ5). R2의 ES 사용 중 17건은 `SEND_NO_SUBS` 이벤트의 사용이다 [M] ([R2g §2]). 이것이 아직 아무도 구독하지 않은 EVS event packet(MsgId 0x808)을 보낼 때 이 경로에서 생긴다는 귀속은 기록의 추론이다 [I] ([R2g §2]는 이 귀속을 event ID와 뒤의 콘솔 줄 "No subscribers for MsgId 0x808"에서 끌어낸 추론으로 적는다; spec §1.6, App. A.1 step 13).
- **계약과 세 판단.** K-ROUTE다(spec §3.15 RC5).
  - 호출자 수준: 대부분의 소비자·미션 문서는 전달을 요구하지 않는다. 전달을 요구하는 `appreq`가 없으면 Allowed는 `unspecified_by_reference`이고, 결과 label은 **LOSS-UNDER-ORDER**, harm은 `unspecified_by_reference`가 된다. TO_LAB처럼 소비자가 늦은 구독을 밝힌 경우는 `defer_by_design`이다. 이때 송신 시점의 발생은 PENDING이고, TO_LAB이 구독하면 TOLERATED, TO_LAB이 먼저 끝나면 VIOLATING이 된다(spec App. A.1 step 13).
  - framework 수준: SB의 기능 요구사항 cSB4305.5는 이 결과를 오류로 보지 않는다(`fw_doc` = `documented-not-error`). 이 판단은 LOSS-UNDER-ORDER 옆에 따로 보고하고 합치지 않는다. 발생을 TOLERATED로 만들지도 않는다.
  - 관측된 결과: 손실은 counter 변화량과 함께 `consequence`로 남는다(spec §1.6, §3.9).

#### OSAL RESERVED 구간: 이름은 보이는데 object는 아직 준비되지 않았다

- **자원.** `osal.obj<T>`다. POSIX 구현 수준에서 OSAL object 생성은 한 단계가 아니다(spec §1.3, §3.2) [F].
  1. type lock 아래에서 slot을 할당한다.
  2. slot을 RESERVED로 표시하고 lock을 **푼다**.
  3. 이름이 보이게 된다.
  4. `pthread_create`, `mq_open` 같은 구현 호출을 한다.
  5. 다시 lock을 잡고 최종 ID를 기록해 발행한다(`osapi-idmap.c:1092`).

  3과 5 사이(RES_CN)에서는 이름이 보이지만 object는 LIVE가 아니고, lock도 잡혀 있지 않다.
- **이름으로 찾을 때.** 이 구간에서 by-name lookup은 즉시 `OS_ERR_INCORRECT_OBJ_STATE`로 실패한다. header는 실패 시 lock을 항상 푼다고 적지만(`osapi-idmap.c:384–385`), 이 경로는 unlock 전에 return한다(`:401–404`) [F] (spec §3.2). spec은 이 문서화된 불변식에서 계약 K-LOCKBAL을 끌어낸다. lock이 남으면 생성자가 발행 단계에서 막힐 수 있다(OSAL F1, spec App. A.4 step 4) [I]. 이것은 class가 아니라 진행(progress) 결과의 `leaked-lock` 종류로 나타난다(§4.6; spec §1.8). OSAL F1·F7은 [R01]이 코드 읽기로 낸 OSAL 결함 후보 번호이고, 실행으로 확인한 적은 없다.
- **ID로 찾을 때.** by-ID lookup은 RESERVED record를 만나면 대기한다. broadcast나 timeout으로 깨어나 다시 시도하고, 정해진 횟수 뒤에는 `OS_ERR_OBJECT_IN_USE`로 실패한다(`osapi-idmap.c:475–480`) [F] (spec §3.2). 새 task의 자기 확인(handshake)이 이렇게 실패하면, 생성자는 `OS_SUCCESS`를 받았는데 task entry는 실행되지 않는다(OSAL F7, 계약 K-SPAWN.osal, 효과 `task_never_runs`). OSAL timebase callback thread도 같은 handshake를 하므로 1 Hz 체인이 시작되지 않을 수 있다(spec §3.3) [I].
- **세 판단에서.** K-LOCKBAL과 K-SPAWN.osal의 `Allowed = none`은 공급자 쪽 문서에만 기댄다. 그래서 `appreq`가 기록되거나 PI가 구현 결함 family로 옮길 때까지 두 계약의 발생은 NOT_READY_UNJUDGED이고 harm은 `unspecified_by_reference`다. 효과는 consequence로 남는다(PI 항목 QT-9; spec §1.6 harm).
- **모형 수준.** 이 구간은 OSAL을 POSIX 구현 수준까지 펼치는 모형 M_impl에서만 보인다. API 계약 수준 모형 M_api에서는 생성이 원자적이다(spec §1.4; §4.3 아래).
- **상태.** 이 경로들은 코드 읽기에서 나온 후보다. 확인 실행 전에는 버그라고 주장하지 않는다(spec §6.4 항목 23).

### 1.4 다섯 설계 사례와 그 역할

설계 과정에서 다섯 사례를 개발 사례(development cases)로 썼다([LED §1a, §1b, §2]).

| 사례 | 내용 | v7.0.1에서 |
|---|---|---|
| C1, CF #184 | CF의 semaphore 이름 조회 대 제공 앱의 생성 | 재시도 수정이 있고 기본 구성에서는 latent. 파생 구성(B′)에서만 실행된다 |
| C2, cFE #198 | 앱 P가 RUNNING을 알린 뒤에 하는 late init이 끝나기 전에 다른 앱들이 호출을 진행함(그 호출이 P를 향했다는 것은 추론) | 수정으로 들어간 2단계 barrier(APPS_INIT/LATE_INIT)가 있다. opt-in이고 timeout은 soft하다([LED §1b]; spec §3.6). 원 시스템이 비공개라 역할 파생(role-derived) 시험만 가능하다 |
| C3, cFE #73 | 6.4.1에서 우선순위(TIME 60이 EVS 61, SB 64보다 높음) 때문에 TIME이 먼저 실행되어 core 시작 중 crash | 원래 버그 코드는 없다(spec §6.4 항목 13). SB AppId를 발행 전에 읽는 잔여 경로가 R2로 남았다([LED §2]) |
| C5, cFE #72 | 6.4.0/6.4.1의 AppCreate가 lock을 푼 채 task를 만들고 나중에 등록함 | 원래 코드는 없다. App-record 구간이 C5 모양의 잔여 후보다 [I] (spec §8.1 RQ2) |
| R2, cFE #2663 | 위 §1.3 | 존재하며 issue는 열려 있다 |

이 사례들은 개발용일 뿐이다. 계약은 사례가 아니라 API와 resource kind마다 쓴다(spec §7.1). 다섯 사례로 일반성을 주장하지 않으며(spec §6.4 항목 16), C3와 C5를 v7.0.1에서 재현했다고 주장하지도 않는다(spec §6.4 항목 13).

v7.0.1에서 실행 경로의 증거가 있는 것은 R2뿐이다([R2g]). C1의 파생 basis(PI 결정 D22), C2의 역할 파생 쌍(P-C2), C3·C5의 6.4.x basis(D13)가 정해지기 전에는 사례 자신의 현상에 대한 도구 수준 증거도 R2에만 생긴다. v7.0.1의 대응물에서 나온 결과는 그 사례의 증거로 보고하지 않고, R2 하나에 그친 결과는 단일 사례 결과로 보고한다(spec §0.7 항목 5·7, §9.3, §6.4 항목 42–43).

---

## 2. 기존 탐지 기법의 한계

### 2.1 짧은 복습: data race, happens-before, vector clock

- **data race**는 두 thread가 같은 메모리 위치에 동기화 없이 접근하고, 그중 적어도 하나가 쓰기인 경우다.
- **happens-before(HB)**는 한 실행 안에서 program order와 동기화(lock 해제→다음 획득, 메시지 송신→수신 등)로 정해지는 사건 사이의 부분 순서다. "a가 반드시 b보다 먼저"라는 관계다(Lamport 1978 [V-abs]).
- **vector clock**은 HB를 계산하는 자료 구조다. thread마다 "다른 각 thread의 사건을 몇 번째까지 알고 있는가"를 숫자 벡터로 들고 다닌다(Fidge 1988; Mattern 1989 [V-abs]).
- HB 기반 detector(FastTrack PLDI 2009, ThreadSanitizer [V-abs]; Djit+ [P])는 HB로 순서가 정해지지 않은 충돌 접근 쌍을 보고한다.
- **predictive detector**는 관측한 trace의 사건을 재배열해, 같은 프로그램에서 가능한 다른 실행을 추론한다. CP, WCP, SHB, M2, SyncP [V-abs]가 그렇다. 이들도 재배열 가능한 쌍만 보고한다([R04 §3 L4]; spec §1.11).

### 2.2 cFS의 위반이 이 틀을 벗어나는 세 가지 방식 (spec §1.11)

**(1) 순서가 잘못된 방향으로 정해진 사용(wrong-way ordered uses), 예: R2.**
- spec §1.11은 R2의 위반 사용이 core 직렬화 때문에 HB상 SB의 발행보다 앞선다고 정리한다([R2g §2]; [R2c §7]). 순서가 정해져 있으니 HB detector와 predictive detector에게는 보고할 쌍이 없다.
- DefUse(OOPSLA 2010 [V-abs])는 실행에서 def-use 불변식(어느 쓰기를 어느 읽기가 읽는가)을 배운다. R2에서는 모든 실행이 EarlyInit의 0을 읽으므로, 그 잘못된 정의를 정상으로 배울 것이다 [I] (spec §1.11).
- GPredict(ICSE 2015 [V-abs])는 사용자가 쓴 일반 속성을 예측된 trace에서 검사한다. [R04 L553]의 "합법적인 오류 결과라는 개념이 없다"는 초록 수준의 추론이고, 도구와 그 속성 형식은 확인하지 않았다(P-PAPERS) [U]. spec은 유한 horizon의 Allowed 항목을 GPredict 식 속성으로 쓸 수 있다고 추론한다 [I] (spec §1.11, §8.3).
- **Kairos의 답.** reference에서 온 계약을 호출자 수준 Allowed(§1.2 "세 판단")와 함께, 순서가 정해진 쌍에도 검사한다. 그리고 σ(스케줄 의미론)와 M(OSAL 모형 수준)을 밝힌 PWB certificate로 "잘못된 방향으로 정해짐"을 `fixed-order`로 끌어올린다. 이것은 모든 유한 실행에 대한 안전성 진술이다(spec §1.10, §1.11). PWB는 "둘 다 일어나면 항상 먼저"라는 관계이고, certificate는 다시 확인할 수 있는 증명 기록이다(§3.3). σ와 M은 §4.2–4.3에서 설명한다.

**(2) lock으로 보호된 순서 위반(lock-protected order violations), 예: C1, C5, App-record 구간.**
- 세 경우 모두 같은 lock 아래에서 일어나는 연산 쌍이다. C1은 생성과 조회, C5와 두 App-record 구간은 발행과 `GetAppID`다.
- 한 trace에서 HB는 이 둘을 lock edge로 순서 짓는다. **lockset** 기법(공유 변수를 항상 같은 lock이 보호하는지 보는 기법)은 일관된 보호를 본다([R04 §2.3, §3 L5]; spec §1.11).
- 그러나 lock은 원자성을 줄 뿐 순서를 주지 않는다(spec §4.4). 반대 순서도 가능하다.
- **선행 연구.** RVPredict(PLDI 2014 [V-abs])와 GPredict 식의 제약 인코딩은 이미 key를 충돌 대상으로, lock을 원자성으로 다룬다([R04 L259, L492]). ExceptioNULL(FSE 2012 [V-abs])은 한 실행에서 "초기화 전 사용"을 예측하며, readiness에 가장 가까운 예측 선행 연구다([R04 L253–255]).
- **Kairos의 답.** solver 대신 explicit-state 탐색(가능한 상태를 하나씩 직접 열거하는 탐색)으로 lock 구간의 순서를 뒤집어 본다. 이것은 공학적 선택이지 새로움이 아니다. framework typestate에도 선행 연구가 있다(predictive typestate checking, ASE 2008; Lifestate, ECOOP 2019; 둘 다 전문 미확인, P-PAPERS). Kairos가 여기에 더하는 것은 호출자·소비자·미션의 reference가 받아들이는 미준비 결과(호출자 수준 Allowed)를 horizon과 출처와 함께 두는 것뿐이고, 이 차이도 두 논문을 확인할 때까지 보류다. 그 밖에 같은 서수의 짝으로 정하는 σ별 class, S-edge가 쌍의 순서를 정하는 곳에서만 나오는 certificate, hold 확인이 있다. Kairos의 trace는 API 수준이라 성기므로, 예측의 건전성은 RVPredict보다 약하다(spec §1.11). 그래서 예측 witness(판정을 뒷받침하는 구체적 실행)마다 가정 A-COMM을 검사한다. trace에 없는 공유 읽기가 관측되었거나, 상수이거나, 유지된 결과로 덮이는지 보는 검사이고, 실패하면 그 witness는 모형 근거(`predicted-modelled`)로 내려간다(spec §4.9).

**(3) 임시방편 soft barrier(ad hoc soft barriers).**
- 앱이 쓰는 시작 barrier(script 단계)는 opt-in이고, 시간이 초과되면 log만 남기고 통과한다. core 직렬화의 대기는 예외로, timeout되면 panic한다(spec §3.14). `CFE_ES_WaitForStartupSync`는 void를 반환하므로 호출자는 대기가 성공했는지 알 수 없다(`cfe_es_api.c:616–619`; spec §1.11, §3.6).
- 그래서 barrier가 주는 순서는 "제때 성공했을 때만" 성립하는 조건부 순서다.
- **선행 연구.** EventRacer(OOPSLA 2013 [V-abs])와 Tian et al.(ISSTA 2008 [V-abs])은 ad hoc 동기화를 인식한다. 그러나 timeout에 따라 성립 여부가 갈리는 edge는 모형화하지 않는다([R04 §5]의 gap G3). gap G1–G8은 [R04 §5]가 "조사한 기법이 모형화하지 않는데 cFS가 요구하는 것"으로 정리한 공백 목록의 번호이고, [R04]는 이 목록을 추론으로 표시한다. spec §6.3의 보장 G1–G11과는 다른 번호 체계다([10_decisions_adr.md §0.2](10_decisions_adr.md#02-이름-충돌-주의)).
- **Kairos의 답.** 조건부 O-edge, guard가 붙은 S-edge, σ_U와 σ_U∞의 분리, timeout-dependent refinement(class를 더 나눈 하위 판정, §4.5)를 쓴다(spec §1.8, §1.9, §1.11). S-edge는 두 끝이 모두 있는 모든 실행에서 성립하는 구조적 순서이고, O-edge는 관측한 실행에서만 성립하는 동기화 순서다(§3.1).
- **새로움은 미확인.** 이 형식화(gap G3)가 새로운지는 문헌 확인이 끝나지 않았다(open literature check)([R04 §8 항목 5]; spec §1.11).

### 2.3 그 밖의 cFS 특성

spec §1.11 표의 나머지 행이다. 괄호 안의 gap 번호는 §2.2 (3)에서 설명한 [R04 §5]의 공백 목록 번호다.

| cFS 현상 | 선행 연구가 하는 것 | Kairos가 더하는 것 (새 메커니즘 주장 없음) |
|---|---|---|
| 구독 전 송신: 성공을 반환하는 조용한 손실 | InitRacer(OOPSLA 2017 [V-abs])의 "late event handler registration" | 그 분류의 cFS instance. registry를 key로 삼고, 전달을 요구하는 `appreq`가 없으면 LOSS-UNDER-ORDER. framework 수준 판단(cSB4305.5: 오류가 아님)은 옆에 따로 보고하고 합치지 않는다 |
| 용량 손실: queue가 차면 조용히 버린다. 부팅당 4–49건 [M] ([BM §0]) (gap G5) | event·분산 HB 모형은 전달을 가정한다([R04 §5]) | 용량(queue depth, MsgLim)을 typestate로 두고, 가능한 순서가 결과를 바꾸는지 예측·탐색. gap G5는 용량에 대해서만 실제로 열린 문제다 |
| 실행 task ≠ 소유 앱 ≠ 서비스 ≠ 찍힌 AppId (gap G6) | thread identity. CAFA/DroidRacer는 thread와 event를 나누고, DCatch는 RPC pool을 추상화한다 | identity 필드 네 개, on-behalf frame(다른 앱을 대신해 실행되는 코드의 identity 표시), capture point(§3.1)(spec §1.2, §1.4) |
| 구성이 도달 여부와 용량을 정함 (gap G7) | 구성을 주어진 것으로 받는다 | basis B와 구성 closure, `inactive`, provider 축 |
| 우선순위 순서는 RT target에서만 성립 (gap G8) | edge도 없고 판정도 없다 | σ_R1[P_sched]를 schedule 제약으로 두고, σ마다 판정 |

### 2.4 빌려 오는 것

Kairos는 어떤 메커니즘도 새롭다고 주장하지 않는다(spec §1.11; §6.4 항목 18). 주요 출처는 다음과 같다(spec §1.11; App. B).

| 요소 | 출처 (상태) |
|---|---|
| HB 부분 순서, vector clock | Lamport 1978; Fidge 1988; Mattern 1989 [V-abs] |
| feasible 대 apparent race; order violation이 주요 버그 부류 | Netzer & Miller 1992 [V-abs]; Lu et al. ASPLOS 2008 [V-abs] |
| framework/event HB 규칙(looper 등) | DroidRacer PLDI 2014, CAFA PLDI 2014 [V-abs]; DCatch ASPLOS 2017 [P] |
| 초기화 race | InitRacer OOPSLA 2017 [V-abs] |
| typestate 계약 | Strom & Yemini 1986 [V-me] |
| predictive typestate checking; trace 위에서 검사하는 framework 생애주기 typestate | Joshi & Sen ASE 2008; Lifestate ECOOP 2019 (서지 확인, 내용은 초록 수준, 전문 미확인 P-PAPERS; spec §1.11 "Defensible contribution") |
| 예측 분석, 후보 제안 뒤 별도 검증(vindication) | maximal causal models RV 2012; RVPredict PLDI 2014; Vindicator PLDI 2018 [V-abs] |
| 체계적 탐색 | DPOR POPL 2005 [V-abs; 보장 문구 UNVERIFIED]; source-DPOR POPL 2014 [V-abs; disabling 확장(JACM)은 P]; fair stateless model checking PLDI 2008 [V-abs] |
| 확인과 triage(보고를 결과별로 분류·선별) | CTrigger, RaceFuzzer, RacePro, Portend, DataCollider [V-abs] |
| soundiness(가정 목록을 명시하고 그 아래에서만 건전성을 주장하는 태도; spec §6.1의 "soundiness manifest") | Livshits et al. CACM 2015 [V-abs] |

**DPOR**(dynamic partial-order reduction)은 의존하는(dependent) transition이 뒤바뀔 수 있는 곳에만 backtracking 지점(다른 순서를 시도하려고 되돌아갈 지점)을 두어, 독립 연산의 순서만 다른 interleaving의 탐색을 줄이는 체계적 탐색 기법이다([R05 §2.2]; DPOR POPL 2005 [V-abs; 보장 문구 UNVERIFIED]). interleaving은 여러 thread의 사건이 끼어드는 순서 하나를 말한다. Kairos의 source-DPOR은 class(독립 연산의 순서만 다른 실행들의 동치류)당 실행 하나를 주장하지 않는다(spec §6.4 항목 9).

---

## 3. Kairos의 핵심 아이디어

spec §0.2는 설계의 형식적 핵심을 여덟 요점으로 적는다. 이 절은 그중 세 축을 다룬다. 하나의 모형을 세 방식으로 해석하는 것(§3.2), 증명 동반 판정(§3.3), 수정하지 않은 바이너리의 관측(§3.4)이다. 그리고 이 셋을 묶는 상호 검사와 feedback(§3.5)을 설명한다.

### 3.0 한 장 그림 (그림 D0)

**그림 D0. 한 장으로 보는 Kairos** (렌더링: [diagrams/D0_big_picture.svg](diagrams/D0_big_picture.svg); spec §0.1–0.2, §2.1, §2.4, §4.14)

```mermaid
flowchart TD
    %% D0 - Kairos big picture (one screen). Sources: spec §0.1-0.2, §2.1, §2.4, §4.14.
    %% Dashed border = PI-gated or late-stage component. Dashed edge = feedback path (F1-F4).

    B[("L0 input: basis B<br/>build, config, Init, Env, Input")]
    PHI[("L0 input: profile Φ_cFS<br/>kinds, ops + per-conjunct LP,<br/>HB rules, contracts")]

    CLO["L1 static closure + skeletons (kz-static)<br/>basis, extract, link, skel<br/>Sk^cert certificate view / Sk^h exploration view"]
    CORE["L2 formal core: libkz-core<br/>KEM event type, one Φ evaluator,<br/>vector clocks, verdict lattice"]

    ABS["L3 abstract interpretation<br/>kz-static static: census, MustBefore,<br/>certificates → kz proofcheck"]
    OPR["L3 operational interpretation<br/>kz explore: bounded explorer<br/>(S7, gated)"]

    OBS["L4 kz observe (F0)<br/>uprobe + eBPF, driven by plan.kz"]
    BIN["cFS process core-cpu1<br/>unmodified gcc binary"]

    CON["L3 concrete interpretation<br/>kz trace / predict / conform:<br/>HB, per-conjunct LP, lock reversal"]

    HOLD["L4 kz hold (F0): ptrace + debug-register gate<br/>kz gate (F1): PI decision D3"]

    VER[("L5 verdicts.kz<br/>per (σ, M) cell: class or unknown(reason)<br/>no alarm ≠ safe")]
    REP["L5 kz report / kz loop<br/>fixed point + run set"]
    FIX["Φ revision (human review)<br/>→ new Φ hash, new loop"]

    B --> CLO
    PHI -->|"bindings, patterns"| CLO
    PHI --> CORE
    CLO --> ABS
    CORE --> ABS
    CORE --> OPR
    CORE --> CON
    ABS -->|"open candidates → probe plan"| OBS
    OBS ---|"observe"| BIN
    OBS -->|"run-*.kzt traces"| CON
    CON -->|"witness → hold plan"| HOLD
    OPR -->|"witness → hold plan"| HOLD
    HOLD ---|"hold"| BIN
    ABS -->|"certificates, inactive"| VER
    CON -->|"observed / predicted evidence"| VER
    OPR -->|"model(b) evidence"| VER
    VER --> REP

    HOLD -.->|"F3 confirmation runs"| CON
    CON -.->|"F1 keys, targets; F2 model gaps"| CLO
    CON -.->|"F4 obligation failures"| FIX

    classDef comp fill:#eef3fb,stroke:#3b5b92,color:#111
    classDef art fill:#fff8e6,stroke:#a07a00,color:#111
    classDef gated fill:#f4f4f4,stroke:#777,stroke-dasharray:5 5,color:#111
    classDef bin fill:#eaf6ea,stroke:#2e7d32,color:#111
    class CLO,CORE,ABS,CON,OBS,HOLD,REP,FIX comp
    class B,PHI,VER art
    class OPR gated
    class BIN bin
```

읽는 법은 다음과 같다(범례는 [diagrams/README.md](diagrams/README.md)).

- L0–L5는 spec §2.1의 계층이다. L0 입력, L1 정적 closure와 skeleton(§3.2), L2 formal core, L3 세 해석기, L4 실제 바이너리 관측·제어, L5 orchestration·보고.
- 원통은 산출물이고, 점선 테두리는 PI 결정이 있어야 하거나 늦은 단계에 쓰는 구성요소다. explorer가 그런 예다.
- 점선 화살표는 feedback F1–F4다(§3.5).
- 판정(L5)에는 "safe" 상태가 없고, "live"나 "deadlock-free"를 뜻하는 진행 값도 없다(§4.7).

### 3.1 하나의 typed event model과 framework profile Φ_cFS

**KEM(Kairos Event Model)**은 모든 구성요소가 공유하는 타입 있는 사건 형식 하나다(spec §0.2 항목 1, §1.5). 사건 하나에는 다음이 들어 있다.

- 실행 주체(actor)와 actor별 순번
- 연산과 인자
- 프로그램이 실제로 받은 결과 코드
- resource key와 코드 위치
- identity 필드 네 개
- LP 구간
- timestamp(증거로만 쓴다)

정적 template도 같은 타입을 쓰고, 필드만 기호적이다. 그래서 정적 층, trace 층, 모형 층이 같은 검사기를 쓴다(spec §1.5).

**Φ_cFS**는 cFS에 대한 지식을 코드가 아닌 데이터로 적은 profile이다. 작은 선언형 DSL(영역 전용 언어)로 쓴다(spec §0.2 항목 1; §2.1 원칙 2). Φ에 담기는 것은 다음과 같다.

| Φ 항목 | 뜻 | cFS 예 |
|---|---|---|
| resource kind와 lifecycle automaton | 자원 종류와 그 상태 기계 | `osal.obj<T>`: FREE → ALLOC → RES_C → RES_CN → LIVE → RES_D → FREE(spec §1.3) |
| operation, effect, LP | API가 자원 상태를 어떻게 바꾸는가, 그 효과가 어느 명령 지점에서 일어나는가 | `CFE_SB_TransmitMsg`: SB lock 아래의 route_lookup LP, 이어서 목적지마다 lock 밖의 put LP(spec §1.4, §3.9) |
| HB rule, stratum, robustness tag | 어떤 사건 쌍에 순서 edge를 주는가, 그 edge가 얼마나 강한가 | R19 CORE-SERIAL(stratum S, tag obs-fatal)(spec §3.6) |
| contract | §1.2의 준비 계약 | K-ID.send-with-appid, 출처 `cfe_evs.h:168–172`(spec App. A.1) |

몇 가지 용어를 정리한다.

- **actor**는 실행 흐름 하나다. OSAL task, OSAL record가 없는 raw thread, 외부 입력의 근원이 actor다(spec §1.2).
- **linearization point(LP)**는 연산의 효과가 한순간에 일어났다고 보는 명령 지점이다. Kairos는 요구 조건의 각 항(conjunct)마다 LP를 따로 둔다[D].
- 호출자가 값을 미리 읽어 두었다가 넘기는 경우에는 그 load 지점이 LP다. 이것을 **capture point**라 한다(spec §1.4). R2에서는 SB 코드가 `CFE_SB_Global.AppId`를 읽는 지점이 capture point다(spec App. A.1 step 2).
- **stratum**은 순서 edge의 등급이다(spec §1.9).
  - S(structural): 두 끝이 모두 있는 모든 실행에서 성립한다. 예: program order, task 생성 → 자식 시작.
  - O(observed): 관측한 실행에서만 성립하고, 예측에서는 뒤집을 수 있다. 예: lock 해제 → 다음 획득, 메시지 전달.
  - C_σ: σ_R1의 우선순위 제약이다. HB edge가 결코 아니다.
- **looper**는 메시지를 받아 handler를 부르는 loop를 도는 actor다. looper에서는 handler 하나 안에서만 program order를 HB로 인정하고, handler 사이에는 HB edge를 두지 않는다(CAFA [V-abs], DCatch [P]; spec §1.9).

engine은 framework에 중립적이다. engine 코드에 사례 식별자(예: `CFE_SB_Global`, `C1`)를 쓰지 못하도록 lint(코드 규칙 자동 검사)가 막는다(spec §2.1 원칙 2, §7.2).

### 3.2 같은 Φ를 세 방식으로 해석한다

**skeleton**은 각 actor의 제어 흐름에서 Φ가 관심 갖는 사건만 남긴 모형이다(spec §4.3). 구성을 닫은(configuration-closed) skeleton을 쓴다. 즉 시작 script, 생성된 core object table, table 값 같은 basis B의 상수를 반영해 경로를 접는다(spec §1.7, §4.3).

| 해석 | 무엇 위에서 | 무엇을 내놓나 | 구성요소 (단계) |
|---|---|---|---|
| abstract(정적) | actor별 skeleton의 두 view. **certificate view** `Sk^cert`는 상한 없는 loop를 "0회 이상 반복"으로 닫는다(star-closure). **exploration view** `Sk^h`는 loop를 h번까지 펼치고 나머지에 잘림(TRUNC)을 표시한다 | census(모든 계약의 instance 목록, §4.1), MustBefore와 PWB(§3.3), certificate, `inactive`. 모두 certificate view에서만 낸다 | `kz-static static`(S3a) |
| concrete(trace) | 수정하지 않은 gcc 바이너리의 실행 trace | vector clock으로 계산한 HB, conjunct별 LP에서의 계약 검사, lock 구간을 뒤집는 예측 | `kz trace`, `kz predict`, `kz conform`(S2, S5) |
| operational(모형 탐색) | 후보마다 닫은 유한 모형, exploration view 사용 | 경계 b 안의 안전성 기록과 class, 발생 서수가 붙은 witness(판정을 뒷받침하는 유한 실행), reach, 진행(progress) 결과(§4.5–4.6) | `kz explore`(S7; 늦은 단계이고 kill criterion이 있다) |

S0–S10은 spec §9.3의 구현 단계 이름이다.

세 해석은 모두 `libkz-core` 안의 evaluator 하나로 Φ predicate를 평가한다(spec §0.2 항목 2; §2.1 원칙 1). 그래서 세 층의 답이 다르면 "해석 차이"로 묻히지 않고, §3.5의 obligation 실패로 드러난다. 의도한 예외가 하나 있다. `kz proofcheck`는 코드에서 뽑은 대기 조건의 함축(`Pred_code ⇒ C`)을 자기 절차로 판단한다. 검사기의 다른 Φ 평가는 이 evaluator를 부르므로, evaluator는 검사기의 trusted base에 든다(§3.3; spec §2.1 원칙 1).

view가 둘인 이유는 이렇다. protected, fixed-order, no-final-status, inactive는 모든 실행에 대한 주장이다. loop를 h에서 자른 view로는 이런 주장을 할 수 없다. 그래서 이 판정들은 certificate view에서만 내고, exploration view에서 나온 결과에는 h를 경계 b에 적는다. 독립 검사기는 잘린 영역을 쓰는 certificate 단계를 거부한다(spec §0.6 항목 1, §4.3). 그림은 [diagrams/D10_skeleton_views.svg](diagrams/D10_skeleton_views.svg)에 있다.

### 3.3 증명 동반 판정 (proof-carrying verdicts)

**정적 순서는 must-analysis다**(spec §0.2 항목 3, §4.4). 모든 실행에서 반드시 성립하는 순서만 결론으로 낸다는 뜻이다.

- **MustBefore(MB)**: `a ∈ MB(n)`은 "모든 실행에서 n이 일어날 때마다 그 전에 a가 일어났다"는 뜻이다. 최대 고정점(greatest fixpoint)으로 계산한다. 즉 시작 actor의 entry 같은 기저 노드는 빈 집합에서, 나머지 노드는 가장 큰 집합에서 시작해, 규칙이 더 줄일 수 없을 때까지 줄인다.
- 규칙은 넷이다.
  - **T-PO**: actor 안의 program order.
  - **T-SPAWN**: task 생성 → 자식 시작. 생성 지점이 여럿이면 모두에 대한 교집합을 쓴다.
  - **T-OBS**: 코드에서 뽑은 조건을 기다리다 빠져나오는 관측. writer 집합이 완전할 때만 적용한다.
  - **T-TERM**: panic처럼 끝나는 분기를 잘라낸다.
- lock만으로는 순서가 생기지 않는다.

**PWB(precedes-whenever-both-occur)**는 "둘 다 일어나는 실행에서는 항상 a가 b보다 먼저"라는 관계다. 이것을 쓰려면 **once(τ)**를 분석으로 증명해야 한다. once(τ)는 actor τ가 한 실행에서 많아야 하나 존재한다는 사실이다(spec §1.2, §1.9, §4.5).

**certificate**는 안전성 진술 하나를 다시 확인할 수 있게 적은 증명 기록이다. FIXED_ORDER certificate는 `¬OK_i`(도달한 in-scope 발생 중 READY나 TOLERATED가 없다)를, PROTECTED certificate는 `¬NR_i`(도달한 in-scope 발생 중 미준비로 결정된 것이 없다)를 증명한다(spec §2.1 원칙 3, §4.5; 진술은 §4.5에서 설명한다). 다음이 들어간다.

- 사용한 규칙 instance
- writer 항목과 각 항목의 전이·key 상태
- once 사실과 `derives` 사실
- 코드에서 뽑은 predicate
- 각각의 코드 위치

certificate는 사용에 도달하는지, 모든 실행이 결정에 이르는지, 영구 대기나 deadlock이 없는지는 증명하지 않는다. 그래서 certificate로 낸 판정의 진행(progress) 값은 explorer가 따로 정하기 전까지 `unknown(not_checked)`다(spec §4.5, §6.4 항목 26).

**검사기 `kz proofcheck`**는 producer와 분석 코드를 공유하지 않고, Clang도 link하지 않는다. 다만 두 쪽 모두 `libkz-core`를 link한다. 검사기는 certificate의 추론 단계를 **이름 붙인 trusted base에 상대적으로** 다시 검증한다 [D] (spec §4.5; ADR-041).
- **믿는 것(trusted base).** 추출 사실(`facts.kz`), skeleton(`skel.kz`, `anchors.kz`; 검사기는 skeleton을 다시 만들지 않는다), closure의 spawn 목록, Φ, 공유 `libkz-core`, 그리고 reach에는 그 단계가 가리키는 run·`hb-*.kz`·prefix-probe 기록, 곧 recorder의 기록이다.
- **스스로 다시 하는 것.** view, dominance, spawn 목록, once 사실, 코드에서 뽑은 대기 조건과 그 함축, writer 집합의 재계산과 제외, `derives` 사슬, frontier, SCOPE, INIT 평가, badness 경로 검사, 상태 일관성, reach.
- **"via T5" 표시.** `libkz-core`의 evaluator나 rule engine을 부르는 단계(INIT 평가, falsifier 조건, 규칙 해석)는 "via T5"로 표시한다. 그 단계에서는 검사기의 추론이 producer와 독립이 아니다.
- **시험.** 일부러 망가뜨린 certificate(mutant)는 모두 거부해야 한다(certificate-mutation test). 이 시험은 검사기를 시험할 뿐이고, 추출 사실이나 `libkz-core`의 오류는 잡지 못한다. 분석 자체는 프로그램을 바꾼 program mutant로 시험한다(spec §2.2, §4.5, §8.5).

**trust ledger.** 검증하지 못한 것을 숨기지 않는다. certificate와 판정 칸마다 네 부분 기록이 붙는다 [D] (spec §4.5.1; §6.3 보장 G10).
1. 다시 검증한 추론 단계(own 또는 via T5)
2. 그 단계들이 읽은 추출 사실. hash로 그 추출에 묶는다
3. 신뢰 항목: 쓴 Φ 항목, `DECLARED_TRUSTED` 항목, 가정, 증명이 아직 sketch인 lemma, 도구 기반(`libkz-core`와 검사기의 tool hash)
4. 관측으로 반박될 수는 있지만 증명되지 않은 항목: 반박할 수 있는 obligation, 그 상태(`not run`, 실행한 경로에서 반박되지 않음, 반박됨), 실행한 경로의 한계

obligation을 통과한 것을 증명으로 적지 않는다. 관측이 증명하는 것은 reach 하나뿐이다. "독립 검증"이라는 말도 위의 뜻으로만 쓴다(spec §6.4 항목 31).

**R2 certificate의 골자.** 기대 동작이며, 실행한 것은 없다 [I] (spec App. A.1).

1. ES와 EVS의 init 안에 있는 각 사용(capture point `c`)은 그 task가 RUNNING을 알리기(raise) 전에 있다. 더 정확히는, 같은 task 안에서 `c`로 가는 경로가 없는 frontier 노드 `m`보다 앞에 있다.
2. startup thread는 ES가(이어서 EVS가) RUNNING에 이른 것을 관측한 뒤에야 다음 core task를 띄운다. 관측 지점은 `o`(`cfe_es_start.c:835`)이고, 관측 조건은 코드에서 뽑는다.
3. SB는 그 뒤에 생성된다. SB AppId를 유효하게 만드는 writer `x`(`cfe_sb_task.c:138`)는 SB 안에 있다.
4. 따라서 모든 실행에서 `m` → `o` → SB 생성 → `x` 순서가 성립하고, 사용은 항상 `x`보다 먼저다. 도달한 모든 사용에서 값 조건이 거짓이므로 READY인 발생이 없다(`¬OK_i`). Allowed가 `none`이라 발생은 결정 시점에 VIOLATING이 되고, 그래서 결정 경로가 있는 `fixed-order`다.
5. 실패 시의 early return은 영향이 없다. 이 증명은 사용 지점이 다른 지점을 지배(dominate)할 것을 요구하지 않는다(step 7).
6. trust ledger를 붙인다. 신뢰 항목 중 `DECLARED_TRUSTED`는 ES 쪽이 `CleanUpApp` 관계 guard 하나이고, EVS 쪽이 할당 신선도 가정 A-FRESH, 소유 가정 A-OWNER, 같은 guard다. 기본 부팅은 성공 경로만 지나므로 `:823` → panic 분기 같은 경로는 기본 부팅의 obligation으로 반박할 수 없다는 한계도 적는다(steps 9, 9a).
7. 출처 회계: "generic 계약 K-ID + framework 규칙 R19". 사례를 아는 상태에서 쓴 framework 규칙에 기댄다는 것을 판정에 밝힌다(spec §7.2; step 10).
8. 진행은 주장하지 않는다. 모든 칸의 진행 값은 `unknown(not_checked)`다(step 9).

그림은 [diagrams/D8_r2_certificate_overview.svg](diagrams/D8_r2_certificate_overview.svg)에 있다.

### 3.4 수정하지 않은 바이너리를 관측한다

- **실행 바이너리.** cFS 빌드 그대로의 gcc 바이너리(gcc 13.3.0, `-O0 -g`)를 실행한다. Clang/LLVM은 분석에만 쓰고, 분석 사실은 DWARF와 기계어 수준에서 gcc 바이너리에 묶는다(spec §2.6, §3.17).
- **충실도 분류**(fidelity class, [R07 §7]; spec §5.1). 기본은 F0이다(spec §2.1 원칙 4).
  - F0: 바이너리와 실행 조건을 바꾸지 않고, timing만 흔들린다.
  - F1: 같은 source와 compiler에서 link나 code-generation 옵션만 바꾼다.
  - F2: source를 고친다.
  - F3: 다른 compiler를 쓴다.
- **관측: `kz observe`, F0.**
  - **uprobe**는 실행 파일을 고치지 않고 사용자 프로그램의 명령 주소에 커널이 거는 탐침이다. **eBPF**는 그 탐침에서 커널 안에서 실행되는, 검증된 작은 프로그램이다.
  - 새 패키지 설치 없이 perf-PMU uprobe(kernel perf 하위 시스템의 PMU 인터페이스로 거는 uprobe; tracefs에 전역 event를 만들지 않는다)와 자체 raw `bpf()` loader로 구현한다(spec §2.1 원칙 7, §5.1).
  - entry+4 위치의 uprobe 비용은 hit당 0.57 µs다 [M] ([R07 §4.7]). BPF 본체의 비용은 아직 측정하지 않았다(spec §5.1).
- **확인: `kz hold`, F0.**
  - ptrace(다른 process를 멈추고 조사·제어하는 Linux system call)와 x86 debug register(지정한 주소를 실행하면 trap을 내는 CPU 레지스터)의 실행 breakpoint로 한 thread만 멈춘다. 코드 text는 고치지 않는다.
  - #DB trap(x86 debug exception: debug register에 건 주소를 실행할 때 CPU가 내는 예외)은 약 34 µs다 [M] ([R07 §4.7]). 전용 holder의 비용은 아직 측정하지 않았다(spec §4.13, §5.1).
- **PI 결정이 필요한 것.**
  - F1 `--wrap` gate(linker가 API 호출을 감싼 함수로 돌리게 하는 방식)와 PCT(깊이 d인 버그를 실행마다 하한이 있는 확률로 찾는 무작위 schedule 시험 기법 [V-abs; 메커니즘 P], spec §6.3 보장 G7)는 PI 결정 D3 이후에만 쓴다(spec §4.13; `spec/PI_DECISIONS.md` PI 결정 D3).
  - root 쪽 관측 hook에는 PI 결정 D1이 필요하다(spec §5.1).
- **timestamp는 HB를 만들지 않는다.** 증거로만 쓴다(spec §1.5, §5.8).
- **관측은 지지 증거일 뿐이다.** 관측만으로 보편 class(protected, fixed-order, no-final-status)를 만들지 않는다(spec §4.12 항목 2).

관측 파이프라인 그림은 [diagrams/D7_observation_pipeline.svg](diagrams/D7_observation_pipeline.svg)에 있다.

### 3.5 층끼리 서로 검사하고, 고정점까지 되먹인다

**obligation O1–O10과 certificate 검사 CC**가 층 사이의 일관성을 검사한다(spec §2.5). 예를 들면 다음과 같다.

- O1/O2: 관측된 사건열을 skeleton이 받아들이는가.
- O3: 관측된 순서가 MB 사실이나 certificate와 모순되지 않는가.
- O4: 프로그램 자신의 결과 코드가 계산한 상태와 맞는가. PROTECTED certificate가 있는 instance에 미준비 발생이 없는가.
- O7: 정적으로 찾은 writer가 Φ가 선언한 효과와 같은가.
- O10: 같은 칸과 경계에서 `exh`의 결과가 certificate의 진술(또는 그것을 옮긴 진술)과 어긋나지 않는가.

**모순이 나오면** 관련 규칙이나 모형을 중지한다. 실제 run의 증거가 certificate에서 온 진술과 어긋나면(O3, O4, O9) `unknown(model_fault)`와 model-fault 기록을 남기고, 그 Φ 버전의 loop를 끝낸다. 모순을 조용히 해결하지 않는다(spec §0.2 항목 7, §6.3 보장 G9).

**모형 증거와 어긋나면 그 경계만 다툰다.** `exh` witness가 certificate 진술과 어긋나거나(O10), 실현된 witness가 `exh`만으로 세운 진술과 어긋나면(O6 (b)), 그 탐색 경계만 `contested`가 된다. `contested`는 `unknown(model_fault)`로 보인다. O10은 이어서 directed check를 한다. reduction을 끄고 다시 탐색하고, 그래도 남으면 `kz proofcheck`를 다시 돌리고, 그래도 남으면 certificate의 b = ∞ 기록은 그대로 둔 채 contested 표시를 붙이고 witness의 확인을 계획한다. 이때 loop는 끝나지 않는다(spec §2.5, §4.12 항목 5).

예외가 하나 있다. M_api에서만 성립하는 certificate가 실제 바이너리와 모순되면, 이것은 model fault가 아니라 portability-hazard의 증거다(spec §1.4).

**feedback**은 네 경로가 있다(spec §2.4, §4.14; 그림 D0의 점선).

- F1: 관측에서 배운 key와 target을 정적 closure로 되돌린다.
- F2: model gap을 정적 closure로 되돌린다.
- F3: 확인 실행의 trace를 trace 분석에 다시 넣는다.
- F4: obligation 실패를 사람의 Φ 수정으로 넘긴다. Φ를 고치면 새 Φ hash로 새 loop를 시작한다.

이름이 겹치는 것에 주의한다. feedback F1–F4는 충실도 분류 F0–F3(§3.4), 동결 F1·F2(§5.2), OSAL 결함 번호 F1·F7(§1.3)과 이름만 같다.

**`kz loop`**는 판정 상태가 더 바뀌지 않는 고정점에서 멈추고, 그 결과를 run 집합과 함께 보고한다. 분석 예산은 wall clock이 아니라 결정적인 work unit(전개한 상태 수, 탐색 노드 수 등)으로 센다. 그래서 저장된 산출물에서 다시 돌리면 판정 저장소가 byte 단위로 같아야 한다(spec §2.1 원칙 8, §6.3 보장 G8).

---

## 4. 판정 체계

### 4.1 판정 단위: instance와 (σ, M) 칸

- **instance**는 (계약 κ, 문맥까지 구분한 template t, key, scope)다. template은 (static actor, inline된 호출 문자열, 위치)로 구분한다. 그래서 같은 SB 코드 위치라도 ES의 TaskInit에서 도달했는지 ES 명령 loop에서 도달했는지에 따라 다른 template이 된다. scope는 default 또는 teardown이다(spec §1.5, §1.6).
- **census**는 모든 계약을 그 계약이 맞는 모든 위치에 instance로 만들고, 각각에 class 또는 `unknown(reason)`을 주는 작업이다(spec §4.6).
- **판정 기록**은 instance마다 하나다. 분석한 (σ, M) 조합마다 칸(cell)이 하나씩 있다. 칸은 경계 b마다 안전성 기록, class, reach, 진행 결과를 따로 둔다. certificate, run, 그리고 그것에서 옮긴 결과는 b = ∞이고, explorer는 끝낸 h마다 결과를 낸다(spec §0.1, §1.10).
- **basis B**는 스케줄을 뺀, 실행을 정하는 모든 것이다(Build, Config, Init, Env, Input). 구성이나 입력을 바꾼 변형은 새 basis B′다. B′에서 나온 결과는 "새 버그"가 아니다(spec §1.7).

### 4.2 σ: 어떤 스케줄을 고려하는가 (spec §1.8)

| σ | 정의 (요약) | 역할 |
|---|---|---|
| σ_U | 시간을 무시한 임의 interleaving이다. `OS_TaskDelay`는 양보(yield)다. timeout이 있는 대기는 시작 후 언제든 timeout될 수 있다. 횟수 제한 retry는 reference 횟수를 다 쓴 뒤에만 소진된다 | certificate와 모형 분류의 기준 의미 |
| σ_U∞ | σ_U에서 **시간 기반 전이**(time-based transition)를 뺀 것이다. 주기적이지 않은 timed wait의 timeout, 지연을 둔 retry의 마지막 시도의 실패, ES kill timer가 끝나지 않은 앱을 강제로 죽이는 것이 여기에 든다. 그래서 지연을 둔 retry의 마지막 시도는, 수준 M에서 전이 하나이면 성공할 수 있을 때만 일어나고, 여러 전이이면 그 actor가 실패한 결과 위에 남겨진다(stranded). 주기적 대기, timer tick, 이미 끝난 앱의 정리는 남는다. Φ가 아직 시간 기반으로 선언하지 않은 포기(HS의 application monitor, TIME의 flywheel 진입)도 남고, 그것에 기대는 결과에는 `tb-undeclared`를 붙인다 | 시간 기반 전이를 거쳐서만 위반되는지, 순서 논리 자체가 위반되는지 가른다. 시간 기반 전이가 가리는 영구 대기도 드러낸다 |
| σ_R1[P_sched] | 한 CPU에서 고정 우선순위로 선점한다. 인용한 scheduling profile `P_sched`로 매개변수화한다. 따로 정의한 전이 시스템이고, 투영(projection)하면 σ_U에 들어간다 | 우선순위가 순서를 정하는 RT 플랫폼(예: 6.4.1의 C3). 기본은 모형으로만 |
| σ_E | 기록된 환경에서 실제 바이너리가 보이는 실행이다. 모형화하지 않고 관측·확인 실행으로 표본만 얻는다 | 실제 witness와 빈도 |

**안전성은 유한 실행 위에서, 진행은 도달 가능한 상태 위에서.** 상태, 나쁜 유한 실행, §4.5의 진술과 class, witness는 유한 실행(finite execution) 위에서 정의한다. dead actor와 stall 상태(§4.6)는 도달 가능한 상태 위에서 정의한다. 어떤 판정도 공정성(fairness) 가정에 기대지 않는다. σ_U∞의 강한 공정성은 극대 실행을 정의할 때만 쓴다(spec §1.8) [D].

**포함 관계**(spec §1.8 L-CONT, §6.2). 유한 실행(prefix)에 대해서만, 모형 수준 M마다 말한다.
- σ_U∞의 유한 실행은 시간 기반 전이가 없는 σ_U의 유한 실행과 정확히 같다.
- σ_R1의 유한 실행은 투영하면 같은 M의 σ_U 유한 실행이다.
- 실제 run(σ_E)은 모두 (σ_U, M_impl)의 유한 실행이다. 이것은 가정 A(spec §6.1) 아래에서만 주장한다. M_api에 대해서는 이 포함을 주장하지 않는다.
- 극대 실행(maximal execution)의 포함은 주장하지 않는다. 예를 들어 영원히 참이 되지 않는 조건을 기다리는 timed wait는 σ_U∞에서는 실행을 끝내지만 σ_U에서는 timeout으로 이어진다(litmus LP-TW; spec §6.4 항목 27).

### 4.3 M: OSAL을 어느 수준으로 모형화하는가 (spec §1.4)

- **M_api.** OSAL API 하나가 LP에서의 원자적 전이 하나다. M_api는 "API 계약을 원자적으로 따르는 가상의 OSAL backend"의 실행이다. 그래서 M_api 판정은 API 계약에 대한 이식성(portability) 주장이고, POSIX 바이너리에 대한 주장이 아니다.
- **M_impl.** OSAL의 idmap, task, binsem, queue 경로를 pthread, `sem_*`, `mq_*` 수준으로 펼친다. RESERVED 구간, timeout 대기, lock 누수 경로가 여기서 보인다.
- **M_impl-relevant 연산.** 두 수준에서 다르게 모형화되는 연산이다. idmap의 생성·조회·삭제, task 생성, binsem, queue가 해당한다.
- **두 수준의 결과가 다르면** 그대로 보고한다. M_impl에서는 보호되는데 M_api에서는 위반 가능하면 **portability hazard**다. M_impl에서만 위반 가능하면 OSAL POSIX의 **implementation-defect candidate**다.

### 4.4 σ 사이의 전이 규칙 (spec §1.8)

전이 규칙은 EXT-20261012 §4.2로 고쳐졌다 [D]. 옮기는 것은 안전성 진술(§4.5)이고, 진술 하나씩 옮긴다.

1. **안전성은 아래로**(규칙 1(a)). σ_U에서 세운 진술 `¬Bad_i`, `¬OK_i`, `¬NR_i`는 같은 M, 같은 경계 b에서 σ_U∞에도 성립한다. σ_R1로는 b = ∞이거나, 경계가 있으면 σ_R1 모형의 cone이 σ_U 모형의 cone을 포함할 때만 옮긴다. 옮긴 진술에는 `SAFETY_TRANSFERRED`를 붙이고, `¬OK_i`의 결정 경로는 받는 σ에서 다시 정한다.
2. **존재는 σ마다**(규칙 1(b)). reach, witness(따라서 `tolerated`, `schedule-dependent`, `ordinal-split`), 진행 결과는 아래로 옮기지 않고 σ마다 계산한다. 예외는 deadlock 순환과 남은 lock의 stall 상태다. 계산하지 않은 칸은 `reach = assumed`, `progress = unknown(not_checked)`다. `exh`가 경계 b 안에서 사용에 이르지 않음을 보이면 reach는 `unreached(b)`이고, 옮긴 보편 class는 "도달한다면 …; b 안에서는 도달하지 않음"으로 읽는다.
3. **σ_E로는 b = ∞의 안전성 진술만**(규칙 2). M_impl에서 세운 경우, 또는 M_api에서 세웠는데 (i) 그 instance의 관련 범위(cone)와 certificate에 M_impl-relevant 연산이 없거나 (ii) M_impl 결과가 있고 같은 경우만 옮긴다. 그 밖에는 σ_E 칸이 `unknown(model_level)`이다.
4. **witness는 위로**(규칙 3). σ_U∞의 유한 실행, 투영한 σ_R1의 유한 실행, 실제 run(M_impl에서, 또는 M_api에서는 cone에 M_impl-relevant 연산이 없거나 cone actor의 M_api 적합성 검사를 통과한 run)은 같은 M의 σ_U에서도 witness다. witness는 판정을 뒷받침하는 유한 실행 하나이고, 발생 서수를 함께 적는다. PENDING과 NOT_EVALUATED 발생은 witness가 아니고, stall witness는 옮기지 않는다.
5. **σ_E로 들어가는 빈도·존재는 없다**(규칙 4). σ_U 예측이나 모형 witness에서 σ_E 빈도·class·reach·진행 결과를 만들지 않고, 실행 횟수(나쁜 실행을 보지 못했다는 사실)에서 σ_E class를 만들지 않는다. σ_E 빈도 `n of N`은 pool key가 같은 실행끼리만 합치고 `σ_E(plan P)`로 표시한다. `schedule-dependent(σ_E)`는 같은 발생 서수에서 나쁜 발생과 좋은 발생을 보인 서로 다른 두 자연 run으로만 얻는다(spec §1.10, §4.12 항목 4, §6.4 항목 3·10·28).

### 4.5 class (spec §1.10, §1.8)

class는 **안전성**(safety)에 대한 분류다. 영구 대기와 deadlock은 class가 아니라 칸마다 따로 적는 진행(progress) 결과다(§4.6). 이 구분은 EXT-20261012 §4.1이 고친 것이다 [D] (ADR-039).

**안전성 기록**(safety record). 칸과 경계마다 다음 세 진술 중 세워진 것을 그 경로(certificate, `exh`, 전이)와 함께 적는다(spec §1.10).
- `¬Bad_i`: 도달한 in-scope 발생 중 VIOLATING(Allowed가 정해지지 않았으면 NOT_READY_UNJUDGED)인 것이 없다.
- `¬OK_i`: 도달한 in-scope 발생 중 READY나 TOLERATED인 것이 없다.
- `¬NR_i`: 도달한 in-scope 발생 중 미준비로 결정된 것이 없다. PROTECTED certificate(또는 그것을 옮긴 것)에서만 나오고, `¬Bad_i`를 함축한다.

`¬OK_i`에는 **결정 경로**(decision route)를 함께 적는다. 그 칸·경계에 나쁜 witness가 있거나, 진술이 FIXED_ORDER certificate에서 왔고 그 REJECT 사건이 발생의 결정 시점에 있으면(`Allowed = none`, `unspecified_by_reference`, 항목의 코드 밖 결과) `¬OK_i`는 결정적이다. REJECT가 결정 뒤에 있으면(retry 소진, 오류 종료나 오류 반환) certificate만으로는 어느 σ에서도 결정 경로가 없다. 그 길은 timeout 없는 대기, σ_U∞가 없앤 전이, σ_R1의 starvation에 막힐 수 있기 때문이다. 기록에는 나쁜 witness와 좋은 witness도 발생 서수와 함께 들어간다.

세 진술은 도달한 발생에 대한 것이다. 도달한 발생이 없으면 공허하게 성립하고, 도달하지 않는 실행에 대해서는 아무것도 말하지 않는다. 도달 여부는 class가 아니라 reach 축이 적는다(§4.6).

**class는 안전성 기록의 함수다**(spec §1.10).

| class | 기록 | 근거 요건 |
|---|---|---|
| `inactive(B)` | 어떤 실행도 사용에 도달하지 않는다 | certificate view에 경로가 없다. B의 상수와 메시지 출처로 접은 경로 조건이 증명이다. **관측되지 않았다는 것은 inactive가 아니다**: 그것은 `unknown(not_reached)`다 |
| `protected` | `¬Bad_i`는 서고 `¬OK_i`는 서지 않았다. 도달한 in-scope 발생은 READY, TOLERATED, PENDING 중 하나이고, VIOLATING이나 NOT_READY_UNJUDGED는 없다. `¬NR_i`가 있으면 모두 READY다 | certificate(b = ∞, `¬NR_i`를 증명), 경계 b 안의 `exh`(`Q_bad` = no), 또는 전이 |
| ↳ `tolerated` | protected이면서 TOLERATED 발생이 있다 | 경계 b 안의 `exh`에서만. PROTECTED certificate는 `¬NR_i`를 증명하므로 tolerated를 뒷받침하지 않는다 |
| `fixed-order` | `¬OK_i`가 서고 결정적이며, `¬Bad_i`는 서지 않았다 | certificate(결정 시점의 REJECT), 나쁜 witness가 있는 경계 b 안의 `exh`(`Q_ok` = no), 또는 받는 σ에서 결정 경로가 있는 전이. 반복된 실패 실행으로는 결코 내지 않는다. 모든 실행이 결정에 이른다고 주장하지도 않는다 |
| `no-final-status` | `¬Bad_i`와 `¬OK_i`가 둘 다 선다. 도달한 in-scope 발생이 모두 PENDING(또는 NOT_EVALUATED)이거나, 도달한 발생이 없다 | 두 진술, 각각 어느 경로로든 |
| `schedule-dependent` | **같은 발생 서수**에 나쁜 witness와 좋은 witness가 있다(서로 다른 두 유한 실행) | 같은 칸·경계의 두 witness. stall witness는 그중 하나가 될 수 없다 |
| ↳ `timeout-dependent` | σ_U에서 schedule-dependent이고, σ_U∞에서 `¬Bad_i`가 서고, σ_U∞에서 사용에 도달한다. 곧 σ_U의 나쁜 유한 실행은 모두 시간 기반 전이를 거친다 | σ_U와 σ_U∞ 칸에서 유도하고 자기 경계와 함께 적는다. `exh`가 필요하다 |
| `ordinal-split` | 나쁜 witness와 좋은 witness가 있지만 같은 서수의 짝은 없다 | 서수가 붙은 witness. 어느 한 발생이 스케줄에 따라 달라진다고 말하지 **않는다** |
| `unknown(reason)` | 아무것도 서지 않았거나, `¬OK_i`만 있고 결정 경로가 없다(`no_decision_route`; 진술은 그대로 보인다) | reason ∈ {`not_reached`, `model_gap`, `bound`, `budget`, `key_unresolved`, `loss`, `unmodelled`, `writer_incomplete`, `horizon`, `no_decision_route`, `conformance`, `model_fault`(`contested` 경계 포함), `model_level`} |

좋은 쪽 flag `timeout-rescued`는 σ_U에서 좋은 witness가 있고 σ_U∞에서 `¬OK_i`가 설 때, 곧 좋은 발생이 모두 어떤 대기가 포기한 덕분일 때 붙는다. protected, schedule-dependent, ordinal-split에 붙을 수 있다(spec §1.8, §1.10). **explorer 없이는 refinement가 성립하지 않는다.** certificate는 σ_U의 진술만, 예측은 witness만 주기 때문이다(spec §1.8).

**C1로 본 refinement의 뜻**(기대 동작 [I], spec App. A.4 step 3).
- 수정 전(CF `2a024d8`, 조회를 한 번만 시도)에는 조회 실패 뒤의 오류 반환이 REJECT다. 시간 기반 전이 없이 VIOLATING이 되므로 σ_U∞에서도 schedule-dependent다.
- 수정 뒤에는 지연을 둔 retry가 있다. M_api의 σ_U∞에서는 마지막 시도의 실패가 없어지므로, provider가 아직 실행될 수 있는 동안 나쁜 유한 실행이 없다(`¬Bad_i(σ_U∞)`). σ_U∞에서 사용에 도달하면 refinement는 `timeout-dependent`다. 이것을 세우려면 `exh`가 필요하다.
- 즉 C1의 수정은 순서 논리의 위반을 timing 여유에 대한 의존으로 바꾸었다.

### 4.6 class 밖의 축 (spec §1.10)

| 축 | 값 | 뜻 |
|---|---|---|
| provider | `present`, `none`, `env-absent`, `unknown` | 준비 상태를 만드는 writer가 있는가. `none`: 조건이 처음에 거짓이고 writer 집합이 비었다. `env-absent`: writer가 환경 조건에 묶였는데 B.Env가 그 조건을 거짓으로 기록했다(예: HS의 `linux_sysmon`, `/proc/schedstat` 없음, spec App. A.6) |
| reach | b = ∞에서 `proved(run id 또는 확인된 모형 경로)`, `assumed`. 경계 b에서 `model(b)`(`exh`가 b 안에서 결정된 발생을 찾음), `unreached(b)`(찾지 못함). 증거가 경계를 반박하면 `contested` | 칸(σ, M)과 경계마다 따로 적는다. `proved`에는 **그 σ, 그 M**의 유한 실행이 필요하다. 예를 들어 M_api의 칸을 run으로 증명하려면 cone actor의 OSAL 호출 결과가 모두 M_api가 허용하는 것이어야 하고, σ_U∞의 칸이면 cone actor의 prefix에 시간 기반 전이가 없어야 한다. `assumed`면 "도달한다면 …", `unreached(b)`면 "도달한다면 …; b 안에서는 도달하지 않음"으로 읽는다 |
| progress | `stall-witness(S)`, `no-stall-within(b)`, `unknown(reason)`(기본 `unknown(not_checked)`) | 칸과 경계마다 따로 적는다. 아래 "진행 결과" |
| scope | `in-scope`, `teardown` | 결정이 종료 요청 뒤에 오는 발생은 K-TEARDOWN이다 |
| realization | `certificate`, `model(b)`, `predicted`, `predicted-modelled`, `predicted-extrapolated`, `realized` | 판정을 무엇이 지지하는가 |
| naturalness(realized만) | `natural`, `held-F0`, `gated-F1`, `timeout-exercised`; flag `derived` | 실현된 실행이 얼마나 자연스러웠나. hold 중이나 hold 때문에 cone actor에 시간 기반 전이가 일어났으면 `timeout-exercised`다. 주기적 대기의 timeout은 세지 않는다 |

**진행(progress) 결과**(spec §1.8, §1.10; ADR-039). 영구 대기와 deadlock은 어떤 class에도 속하지 않는다 [D].
- **dead actor와 stall 상태.** 끝나지 않은 actor가 도달 가능한 어느 상태에서도 다시 움직일 수 없으면 그 instance에 대해 dead다. 이 정의에는 공정성이 필요 없고, 굶고 있는(starved) actor는 dead가 아니다. dead actor가 아직 사용 앞에 있으면 pre-use stall, 호출 안에서 결정 전에 있으면 in-use stall, PENDING 발생의 horizon을 감시하는 actor이면 horizon stall이다. 프로세스를 끝내는 단계 뒤의 상태와, default scope에서는 종료 요청 뒤의 상태를 뺀다.
- **값.** `stall-witness(S)`는 S ⊆ {pre-use, in-use, horizon}이 성립하는 stall 상태에 이르는 유한 실행이다. 종류(`deadlock-cycle`, `leaked-lock`, `other`), qualifier(`idle`, `terminal-pending`, `choice-dependent`), wait-for root, harm(`hang_or_deadlock`, `task_never_runs`, `terminal_pending(panic | abort)`), progress chain, 결과 label `liveness`를 함께 적는다. `no-stall-within(b)`는 `exh`가 경계 b 안에 stall 상태가 없음을 보였다는 뜻이고, 그 모형에 대한 진술일 뿐이다. 그 밖은 `unknown(reason)`이다.
- **누가 정하나.** 모형의 진행 결과는 `exh`(단계 S7)만 정한다. 실제 run에서는 영구성을 보이는 wait-for 사실(`deadlock-cycle`, 또는 holder가 끝났거나 같은 lock에 막힌 `leaked-lock`)이 있을 때만 `stall-witness(realized)`다. certificate, 예측, wait-for 사실 없는 관측, σ 사이의 전이는 진행 결과를 정하지 않는다. deadlock 순환과 남은 lock의 stall 상태만 예외다.
- **class와의 관계.** 진행 결과는 class를 바꾸지 않고, class는 어떤 진행 값도 함축하지 않는다. protected와 stall witness가 함께 있는 기록은 모순이 아니다(litmus LP-DL).
- **refinement.** `timeout-masked-liveness`는 instance의 진행 refinement다. σ_U∞에서는 hang 상태인데 σ_U에서는 아닌 상태가 있을 때 붙는다. 그 상태에서 σ_U로 빠져나가는 길은 모두 시간 기반 전이(위반, panic 등)를 거친다. progress chain이 들어오지 않는 뿌리 instance에는 flag `tml-root`가 붙고, 보고서는 뿌리를 보인다.
- **보이는 법**(rendering rule). 보편 class는 **언제나 그 칸의 reach·progress 값과 경계를 함께** 보인다. 예: "protected — every reached occurrence READY (certificate, b = ∞); reach proved; progress not checked". stall witness는 종류, qualifier, wait-for root와 함께 보인다.
- **세는 법**(counting rule). stall witness는 `realized`이거나, 확인할 수 있는(confirmable) 종류이면서 `choice-dependent`가 아닌 모형 witness일 때만 탐지로 센다. 같은 wait-for root 집합끼리 묶어 칸마다 한 번 센다. 나머지 모형 stall witness는 진행 후보로만 나열한다.

**축별로 어느 쪽이 약한가.** realization은 `model(b)` ≈ `predicted-modelled` ≈ `predicted-extrapolated` < `predicted` < `realized`다. naturalness는 `timeout-exercised` < `gated-F1` ≈ `held-F0` < `natural`이다. schedule-dependent 기록은 축마다 더 약한 쪽 근거를 남긴다(spec §1.10).

**그 밖의 필드**(spec §1.10).
- shape: 나쁜 witness와 "실패한 조건을 참으로 만들 첫 전이" p* 사이의 관계다. witness는 유한 실행이므로, 그 실행과 기록된 연장(같은 run의 나머지나 explorer 경로)에서 정한다.
  - WRONG-WAY: p*가 사용 뒤에 오고, 사용이 HB상 p*보다 앞선다.
  - UNORDERED: 둘 사이에 HB 순서가 없다.
  - ABSENT: 어떤 연장도 p*를 가질 수 없다. 정적으로(provider `none`, 또는 once 노드를 p* 없이 이미 지남) 또는 경계 b 안의 `exh`로 보인다. 그냥 끝난 run에서 추론하지 않는다.
  - NOT-OBSERVED: p*가 실행에도 기록된 연장에도 없고, ABSENT는 보이지 않았다.
  - STALE: 조건이 한때 참이었다가 사용 전에 다시 거짓이 되었다.
- 결과 label(호출자 수준): `violation`(Allowed가 정해짐), `LOSS-UNDER-ORDER`(route·용량, Allowed 미정), `not-ready-unjudged`(그 밖의 계약, Allowed 미정). class가 fixed-order, schedule-dependent, ordinal-split인 칸만 이 label을 보인다. `liveness`는 진행 결과의 label이다.
- framework 수준: `fw_doc`(instance마다: `documented-not-error`, `documented-result`, `undocumented`), `fw_conf`(칸마다: `as-documented`, `deviates`, `unknown`). 결과 label과 합치지 않는다.
- 관측된 결과 `consequence`(관측 횟수와 함께), harm(호출자 수준), 위반 사슬과 progress chain, trust ledger 참조, B·Φ·tool hash.

판정 상태 기계 그림은 [diagrams/D6_verdict_overview.svg](diagrams/D6_verdict_overview.svg)와 [diagrams/D6_verdict_axes.svg](diagrams/D6_verdict_axes.svg)에 있다.

### 4.7 경보 없음 ≠ 안전

- 판정 기록에는 "safe" 필드가 없고, "live"나 "deadlock-free"를 뜻하는 필드도 없다(spec §1.10).
- 경보가 없다는 것은 근거가 없다는 뜻이고, 그런 칸은 `unknown(reason)`이 된다. σ_E class를 나쁜 실행이 없다는 사실에서 유도하는 일은 없다(spec §0.1).
- σ_E class는 자연 실행의 witness로만 정한다. `schedule-dependent(σ_E)`에는 **같은 발생 서수**에서 나쁜 발생과 좋은 발생을 보인 서로 다른 두 자연 run이 필요하다. 서수가 다르면 `ordinal-split(σ_E)`이고, 이것은 run 하나에서도 나올 수 있다. hold나 gate가 개입한 실행이면 `σ_E(held)`다. σ_E의 protected·fixed-order·no-final-status는 실행에서 유도하지 않고, 전이 규칙 2(b = ∞)로만 얻는다(spec §1.10).
- σ_E의 진행 결과는 영구성을 보이는 wait-for 사실이 있을 때만 `stall-witness(realized)`이고, 그 밖에는 `unknown(not_checked)`다. σ_E에 `no-stall-within(b)`는 없다(spec §1.10).
- 나쁜 실행을 관측하지 못하면 기록 문구는 정확히 "no σ_E bad witness in n of N runs (pool key); σ_E class unknown"이다. pool key는 빈도를 합칠 때 같아야 하는 (B hash, plan hash, cold/warm 층)이다(spec §4.12 항목 4). "timing-protected"라고 쓰지 않는다(spec §1.10; §6.4 항목 3).
- 예측이 반대 순서를 찾지 못한 것("not predictable")은 protected가 아니다(spec §4.9 항목 9; §6.3 보장 G4). prefilter(예측 전에 후보를 고르는 SHB, WCP, SyncP 같은 빠른 검사)가 아무것도 내지 않았다는 것은 아무 뜻도 없다(spec §4.9 항목 6).
- 경계 b를 넘어선 탐색, obligation이 실패한 모형에서도 안전을 말하지 않는다(spec §6.4 항목 2).
- "stall 없음", "deadlock-free", "live"는 `exh`의 경계 b 안에서만, 그것도 closed cone 모형에 대해서만 말한다(spec §6.4 항목 29).

### 4.8 예: 기대 판정 (모두 [I], 실행 전)

| instance | 기대 판정 | 근거 |
|---|---|---|
| R2: ES·EVS init의 SB 이벤트 사용(K-ID) | `fixed-order(σ_U, M_api)`와 `fixed-order(σ_U, M_impl)`: b = ∞의 안전성 진술 `¬OK_i`이고, Allowed가 `none`이라 결정 시점에 REJECT하므로 결정 경로가 있다. σ_U∞와 σ_R1 칸은 전이 규칙 1(a)로 같은 진술을 받고, σ_E 칸은 M_impl에서 전이 규칙 2로 얻는다. reach는 칸마다 다르다: (σ_U, M_impl)은 S1이 발생을 관측하면 `proved`, M_api 칸은 cone actor의 M_api 적합성 검사를 통과할 때만 `proved`, σ_R1은 `assumed`다. progress는 모든 칸에서 `unknown(not_checked)`다. realization은 `certificate`이고, 관측 `n of N`을 더한다. shape는 SB가 나중에 발행하는 곳에서 WRONG-WAY, SB 자신의 App-record 구간에 걸린 곳에서 ABSENT다 | spec App. A.1 steps 9, 9a |
| C1, 기본 B | `inactive(B)`: 기본 table의 이름이 비어 있어 조회 경로가 접힌다 | App. A.4 step 1 |
| C1, 파생 B′, M_api | σ_U에서 `schedule-dependent`. σ_U∞에서는 provider가 아직 실행될 수 있는 동안 나쁜 유한 실행이 없으므로 refinement `timeout-dependent`(`exh` 필요). 이른 조회 한 번은 `tolerated`(`exh`에서) | App. A.4 step 3 |
| C1, 파생 B′, M_impl | RESERVED 구간의 by-name 조회가 `INCORRECT_OBJ_STATE`를 낸다. 이 결과는 `retry_until_ready`의 코드 밖이라 결정 시점에 REJECT, 곧 VIOLATING이다. 시간 기반 전이가 필요 없으므로 refinement는 없다. countsem table lock이 남아 provider와 그 lock을 잡는 뒤의 연산이 dead가 되고(`leaked-lock`), 이 진행 결과들은 CF의 VIOLATING 시도에 사슬로 붙는다. implementation-defect candidate(F1)다. M_api 결과는 σ_E로 옮기지 않는다 | App. A.4 step 4 |
| core task의 App-record 구간(K-SELF.es) | 정적 certificate가 없어 열린 후보다. lock을 뒤집는 예측으로 간다. 두 M 모두 σ_U에서 `schedule-dependent`이고, timeout이 끼지 않으므로 σ_U∞에서도 그렇다(refinement 없음). σ_U∞에서 raise가 빠지면 startup thread가 `cfe_es_start.c:835`에서 dead가 된다. 그 뒤에 있는 instance들은 `timeout-masked-liveness`를 가진 채 progress chain으로 이 instance에 붙고, 이 instance가 `tml-root`로 보고된다 [I]. σ_R1[`P_sched^posix`]에서는 한 CPU에서 이 구간이 열리지 않는다 [I]. `P_sched^posix`는 RT 스케줄링이 허용될 때의 OSAL POSIX scheduling profile이다([04 §7.2](04_formal_model.md#72-네-의미론)). σ_E는 "no σ_E bad witness in n of N runs; σ_E class unknown"으로 예상한다 | App. A.2; spec §7.3 |
| timebase 이름 조회(TIME, SCH_LAB) | 종료 구간 밖에서 `protected(σ_U, M_api)`와 `protected(σ_U, M_impl)`. 도달한 조회는 모두 READY라는 안전성 진술(`¬NR_i`)이다. 진행은 주장하지 않는다(모든 certificate 칸에서 `unknown(not_checked)`). 코드가 순서를 정한 곳에서 race를 지어내지 않는지 보는 calibration이다 | App. A.7 |
| `SBNSubPipe`(SBN 앱이 구독 보고를 받는 pipe, 실효 용량 10) 시작 폭주(K-MODE: SB 전역 mode 변경, K-CAP: queue 용량, K-CAP.msglim: 구독별 MsgLim) | 같은 put 서수 k가 한 자연 run에서는 전달되고 다른 자연 run에서는 넘치면 `schedule-dependent(σ_E)`다. 한 run에서 앞의 put은 전달되고 뒤의 put이 넘치는 것만으로는 `ordinal-split(σ_E)`다. 호출자 수준 LOSS-UNDER-ORDER, harm `unspecified_by_reference`; framework 수준 cSB4305.3 `documented-result` | App. A.5 |

---

## 5. 기여 가설과 하지 않는 주장

### 5.1 기여 가설 [I]

spec이 방어 가능하다고 보는 기여는 다음 조합이다(spec §1.11 "Defensible contribution"; [X1 §4]를 따르고 EXT-20261012가 고쳤다).

1. σ와 M을 밝힌 must-order certificate다. 관측된 "HB상 순서가 정해진 계약 위반"을 보편 class인 fixed-order로 끌어올린다. 그 결과는 안전성 진술이고, 명시한 trusted base에 상대적으로 다시 검증한 것이다(trust ledger, §3.3).
2. 호출자 수준 Allowed 결과(출처와 reference horizon이 붙는다; §1.2 "세 판단")와 conjunct별 LP.
3. 위 둘을 lock을 뒤집는 예측, hold 확인, 그리고 평가와 결합한 것. 평가는 탐지, 분류, 근거를 따로 보고하고, 놓친 것을 history 유래 mutant와 수정 전후 쌍으로 잰다.
4. 이 전체를 cFS 의미론, 곧 [R04 §5]의 gap G1–G8을 중심으로 조직한 것.

개별 메커니즘에는 모두 선행 연구가 있다. 더 가까운 선행 연구도 세 갈래 있다. 예측된 trace에서 사용자가 쓴 속성을 검사하는 GPredict(ICSE 2015 [V-abs]), 다중 thread 프로그램의 predictive typestate checking(Joshi & Sen, ASE 2008), trace 위에서 검사하는 framework 생애주기 typestate와 protocol 명세(Lifestate, ECOOP 2019)다. 세 논문 모두 전문을 읽지 않았다(P-PAPERS). 그래서 후보 기여는 cFS에 특화한 추출, 분류, 근거이고, 명세나 framework typestate를 쓰는 것 자체가 아니다. Joshi & Sen과 Lifestate에 대해 주장하는 유일한 차이(호출자·소비자·미션의 reference가 받아들이는 미준비 결과마다 horizon과 출처를 붙이고, framework의 문서화된 결과는 따로 보고하는 것)도 두 논문을 확인할 때까지 보류다. 다른 명세 기반 기법이 Allowed를 표현하지 못한다는 주장도 아니다(spec §6.4 항목 37). gap G3(timeout 조건부 barrier edge) 형식화가 새로운지는 문헌 확인 전이다(spec §6.4 항목 18).

**R2 밖에서 보여야 한다.** C1, C2, C3, C5에서 분류의 차이를 보이려면 explorer(S7)와 사례 basis(C1은 D22, C2는 P-C2, C3·C5는 D13)가 있어야 한다. R2 하나에 그친 결과는 단일 사례 결과다(spec §1.11, §8.8, §9.3).

**평가는 놓친 것을 잰다**(spec §0.2 항목 8, §8.5).
- recall 프로토콜은 history에서 유래한 program mutant와 수정 전후 쌍에 대해 층별로 detected / unknown / missed를 보고한다. class label이 있으면 class-correct를 따로 적고, mutant별·원래 수정별로 적는다. 구간에는 그 단위와 독립 가정을 밝힌다(spec §8.7). 프로토콜은 F2 동결 전에 고정한다.
- mutant의 문서화된 동작과 모순되는 certificate는 soundness 실패로 센다.
- recall 수치는 합성(synthetic) 측정이며, 실제 순서 버그 일반에 대한 recall이 아니다(spec §6.4 항목 25).
- 비교 대상은 원 도구, 같은 정보를 받은 재구현, Kairos의 한 요소 ablation으로 나누어 둔다. 탐지, 분류, 근거를 따로 보고하고, 짝지은 검정은 탐지에만 쓴다(spec §0.7 항목 6, §8.3).

**첫 목표와 최소 핵심.** 첫 목표는 thin MVC(S0+S1+S2+S3a) 전체가 아니라 검증 사다리(validation ladder)의 rung V1이다. ENV basis에서 R2의 K-ID 사슬을 끝까지 통과시키는 것이고, 단계 출구 조건 가운데 R2 사슬에 필요한 항목만 떼어 낸 하위 관문 S0-R2, S3a-R2, S1-R2, S2-R2를 쓴다. thin MVC의 나머지 출구 조건은 rung V1b다 [D] (spec §9.3; ADR-044). thin MVC까지 오면 다음을 주장할 수 있다고 spec은 계획한다 [I] (spec §9.3 Claims).
- census
- R2의 fixed-order 안전성 진술. 독립 검사기가 명시한 trusted base(추출 사실, skeleton, Φ)에 상대적으로 추론 단계를 다시 검증한 것이다
- 구성 closure와 메시지 출처에서 나온 inactive·provider 판정
- 정적 층과 trace 층의 recall

### 5.2 사례에 맞추지 않았다는 근거 (개방성)

- **계약 단위.** 계약은 사례가 아니라 API와 resource kind마다 쓴다. census가 cFE core와 번들 앱 15개의 모든 문맥 위치에 적용한다(spec §7.1).
- **후보 출처 14개의 합집합**(spec §7.1).
  - 정적 census.
  - outcome mining: 관측된 not-ready 결과 코드에서 instance를 만든다.
  - key-pair 열거.
  - retry·timeout loop에서 유도한 계약.
  - 생애주기 대칭과 문맥 조회 대칭.
  - API 문서의 `\par Assumptions` 블록(cFE core API header에 138개).
  - 코드 주석이 인정한 순서 가정과 race.
  - belief deviance: 순위만 매기고 판정은 하지 않는다.
  - 한 순서로만 관측된 쌍.
  - 앱별 profiling, watchpoint, 구성 변형, held-out 출처.
- **반순환(anti-circularity) 장치**(spec §7.2).
  - 두 oracle(Φ 효과와 framework 유효성 검사)의 일치를 검사한다.
  - Φ 항목마다 출처의 종류와 쪽(side), 작성 시점, 작성자가 어떤 사례를 봤는지 기록한다.
  - engine 코드에 사례 식별자를 금지하는 lint를 둔다.
  - 첫 평가 실행은 사례별 항목 없이(template-only) 한다.
  - 동결 F1(첫 평가 전)과 F2(held-out을 열기 전)를 둔다.
  - 사례를 아는 상태에서 쓴 framework 규칙을 판정마다 밝힌다.
- **개발 사례는 맹검이 아니다.** 저자들은 R2, C1, C2, C3, C5와 두 App-record 구간을 이미 안다. 그래서 개발 집합의 실행은 모두 맹검이 아니다. 수정·issue 텍스트는 정답(ground truth)의 출처일 뿐 맹검의 근거가 아니다. 맹검은 노출·동결 기록(exposure and freeze record)이, 그 항목을 지배하는 동결 전에 아무도 그 사례, 수정, 기대 판정을 보지 않았음을 보일 때만 주장한다. 그 대상은 held-out 출처(설계 중 열어 보지 않은 평가 출처: 다른 cFS revision, 다른 구성, 동결 전 고정한 질의로 찾은 issue, recall mutant, 설계에 쓰지 않은 코드 주석)다(spec §7.2, §7.5).
- **다섯 사례 밖의 후보**도 일반 template에서 나온다(spec §7.3). core-task App-record 구간, OSAL lock 누수(F1), 조용한 task 소실(F7), timebase handshake, SB pipe 이름 가시성, TIME callback, `SBNSubPipe` 폭주, SCH_LAB 깨우기 대 늦은 구독자 등이다. 증거가 나오기 전까지는 모두 후보다.

### 5.3 일반화 가설: 진술하되 주장하지 않는다 (spec §7.6)

| 가설 | 내용 | 시험 / 상태 |
|---|---|---|
| H-V(cFS 버전) | 버전과 backend에 따라 바뀌는 것은 Φ와 scheduling profile `P_sched`뿐이다 | 6.4.x용 Φ, 인용한 `P_sched`, 바뀌지 않은 engine으로 C3(6.4.1)가 σ_R1에서 fixed-order(`exh`로만, S7), σ_U에서 schedule-dependent로, C5(6.4.1)가 schedule-dependent로 나와야 한다. engine을 고쳐야 하면 강한 형태가 반박된다. PI 결정 D13이 필요하다 |
| H-C(구성) | 판정은 B를 통해서만 바뀐다 | §7.4의 구성·입력 변형으로 시험한다 |
| H-F(다른 비행 framework) | 다른 framework(예: F Prime)의 profile에 무엇이 필요한지만 적었다 | F Prime의 메커니즘은 R01–R07에서 검토하지 않았다. 아무것도 주장하지 않는다 |

### 5.4 하지 않는 주장 (spec §6.4, 43개 항목을 묶음)

항목 26–43은 EXT-20261012가 더한 것이다.

| 범주 | 하지 않는 주장 | §6.4 항목 |
|---|---|---|
| 건전성·완전성 | 한정어 없는 "sound", "complete", "모든 race를 탐지", "false positive 없음" | 1 |
| 안전 | 경보 없음, 경계를 넘은 탐색, 예측·prefilter의 침묵, obligation이 실패한 모형에서 "safe"나 "위반 없음" | 2 |
| σ_E | 나쁜 실행이 없다는 사실에서 "σ_E에서 timing-protected" 또는 σ_E class. σ_U 예측이나 모형 witness에서 σ_E 빈도나 class. hold·gate·timeout-exercised 실행에서 σ_E class. σ_R1과 σ_E 사이의 우선순위 판정 이동. "σ 사이에 아무것도 옮기지 않음"(너무 약함)과 σ_U→σ_E 빈도 이전(너무 강함) | 3, 10, 11 |
| fixed-order | 반복된 실패 실행에서. 정적 certificate 없이 trace 안의 강건 사슬(VC_R: 무조건·obs-fatal edge만으로 계산한 vector clock)만으로. h로 자른 view에서 h를 밝히지 않은 보편 판정. fixed-order instance의 모든 실행이 결정에 이른다는 것 | 4, 5, 6, 35 |
| 모형 수준 | cone이 M_impl-relevant인 M_api 판정의 σ_E 타당성 | 7 |
| 예측·탐색 | A-COMM 검사에 실패한 witness를 "trace-sound"라 부름. T-JACM(disabling까지 확장한 source-DPOR의 정확성 정리, 상태 P)이 미확인인 동안 `dpor`가 "b 안에서 정확". "Mazurkiewicz class(독립 연산의 순서만 다른 실행들의 동치류)당 실행 하나" | 8, 9 |
| 진행 | certificate에서 진행, 종료, deadlock 없음, 도달을 주장함. `exh`의 경계 안 closed cone 모형 밖에서 "stall 없음", "deadlock-free", "live"; livelock, starvation, divergence. K-TEARDOWN instance의 진행. `other` 종류이거나 `idle`, `terminal-pending`, `choice-dependent`인 모형 stall witness를 바이너리의 hang으로 부름. explorer 없는 refinement. 사용하는 actor가 발생 없이 끝나거나 panic으로 끝나는 실행(안전성 축도 진행 축도 덮지 않는다). closed cone에서 계산한 `terminal-pending`이 프로세스 전체에서 성립함 | 26, 29, 32, 33, 34, 35 |
| 포함·전이 | 극대 실행의 포함. reach, witness(따라서 `tolerated`, `schedule-dependent`, `ordinal-split`), stall witness(deadlock 순환과 남은 lock 제외), 진행 결과의 σ 간 전이. 전이로 얻은 σ_E 진행, 경계가 있는 진술의 σ_E 전이, cone을 포함하지 않는 σ_R1 모형으로의 경계 진술 전이 | 27, 28 |
| 서수 | `ordinal-split` instance가 스케줄에 의존한다는 것 | 30 |
| 검사기 | 이보다 강한 뜻의 "독립 검증": `kz proofcheck`는 추출 사실, skeleton, closure 목록, Φ, `libkz-core`를 믿고 그에 상대적으로 추론 단계를 다시 검증했으며, "via T5" 단계는 공유 evaluator나 rule engine도 썼다 | 31 |
| σ_U∞ | Φ가 시간 기반으로 선언하지 않은 포기가 σ_U∞에 없다는 것(그런 결과에는 `tb-undeclared`). 프로세스 종료 단계 뒤에 결정된 발생. L1 요약에 기댄 결과가 L1 없이도 성립함 | 36 |
| R2 | "R2는 race다". R2 certificate가 generic template만으로 나왔다. Kairos 자체 recorder가 사용 집합을 재관측하기 전에 R2 수를 Kairos 결과로 씀. ENV의 version table이 없는 basis에서 "24" | 12, 21 |
| 사례 | v7.0.1에서 C3·C5 재현. v7.0.1 기본 구성에서 C1, C2, C3, C5 "재현". 다섯 사례에서 나온 일반성. 한 사례의 결과를 다른 사례의 증거로 씀, 다섯 사례를 독립 표본으로 셈. 파생·역할 파생·seeded·mutant·구성 변경 시험에서 "새 버그". 기존 process-level 도구가 C1–R2에서 실패함(모두 `NOT_RUN`이고, `NOT_RUN`은 false negative가 아니다) | 13, 14, 16, 42, 43 |
| 새로움 | HB/vector clock, DPOR, PCT, lockset, delay injection, 예측 분석, SMT 인코딩, points-to, key 충돌 lock 뒤집기(RVPredict/GPredict), 늦은 등록 손실(InitRacer), 예측된 초기화 전 사용(ExceptioNULL), framework typestate(Joshi & Sen; Lifestate)의 메커니즘 새로움. "최초의 process race detector". 명세 기반·GPredict 식 기법이 Allowed, 유한 retry, 유한 horizon을 표현하지 못한다는 것 | 18, 24, 37 |
| 평가 | 탐지(경보 유무)만으로 certificate와 예측이 더하는 가치. 노출·동결 기록 없는 맹검, 수정 텍스트에서 온 맹검, rater masking만으로 "blind evaluation". 발생·instance·mutant·run 위의 구간을 독립 결함에 대한 진술로 읽음; 동적 발생 수를 표본 크기로 씀. class label 없는 class 정확도 | 38, 39, 40, 41 |
| 범위 | 측정된 이득(G-MLIR) 없이 "MLIR이 탐지나 정밀도를 높인다". H-F를 넘는 F Prime 등에 대한 주장. Linux `SCHED_OTHER` permissive mode에서 얻은 비행 대표 timing이나 overhead. profile이 없는 RTEMS/VxWorks의 RT 판정. ad hoc 동기화 위치의 memory model 정확성(가정 A4) | 15, 17, 19, 20 |
| 결과 해석 | EVS가 거부한 이벤트 요청을 "잃어버린 packet"으로 부름. 확인 실행 전에 OSAL F1, F7, timebase handshake 소실, 두 App-record 구간을 버그로 부름. 실제 순서 버그 일반에 대한 recall 수치 | 22, 23, 25 |

전체 가정 목록(soundiness manifest)은 spec §6.1, 보장 G1–G11의 문구는 spec §6.3에 있다. 보장 G11은 진행 결과가 경계가 있고 모형에 상대적이라는 것이다. 모든 보장은 B, Φ, M, σ, 경계 b, skeleton view, 가정 A에 상대적이다. "sound"라는 말을 한정어 없이 쓰지 않는다(spec §6 머리말).

---

## 6. 다음에 읽을 곳

처음 읽는다면 01 → 02 → 03 → 04 → 05 → 06 → 07 → 08 → 09 → 10 순서를 권한다. 문제와 구조(01–03)를 먼저 보고, 형식 모형과 cFS 대응(04–05), 관측·확인과 평가(06–07), 구현 단계와 선행 연구(08–09)를 본 뒤, 설계 결정·PI 결정·용어집(10)으로 끝난다. 알고 싶은 것이 분명하면 아래 표에서 바로 고른다. 폴더의 입구는 [README.md](README.md)다.

| 문서 | 내용 | spec |
|---|---|---|
| [README.md](README.md) | 폴더 입구: 읽는 순서, 문서별 한 줄 요약 | — |
| **01_overview.md (이 문서)** | 문제, 네 가지 예(R2, C1, SB 구독 전 송신, OSAL RESERVED 구간), 기존 기법의 한계, 판정 체계, 기여 가설과 하지 않는 주장(그림 D0) | §0, §1, §6.4 |
| [02_architecture.md](02_architecture.md) | 계층 L0–L5, 구성요소, 두 실행 파일, typed 산출물, 데이터 흐름, obligation O1–O10(그림 D1, D2) | §2 |
| [03_workflow.md](03_workflow.md) | instance 하나의 분석 순서, feedback loop, 판정 상태 기계, 연구 단계 개요(그림 D3, D4 개요, D6, D7) | §4, §9.3 |
| [04_formal_model.md](04_formal_model.md) | 형식 모형: actor·자원·계약, 의미론 σ와 모형 수준 M, MustBefore·PWB·certificate의 형식 유도, 보장(그림 D10) | §1, §4.2–4.5, §6 |
| [05_cfs_mapping.md](05_cfs_mapping.md) | cFS 계층과 Φ_cFS의 대응, 계약 template K-*와 RC1–RC15, R2 certificate, 종료 범위(그림 D5, D8, D9) | §3, §7.3, App. A |
| [06_observation_confirmation.md](06_observation_confirmation.md) | 수정하지 않은 바이너리의 관측(`kz observe`)과 확인 실행(`kz hold`, `kz gate`), 비용 모형(그림 D7) | §4.13, §5 |
| [07_evaluation.md](07_evaluation.md) | 평가 계획: 연구 질문, 기준선, ablation, recall protocol, triage, perturbation 실험군, 통계(그림 D4 상세) | §7.2, §7.5, §8 |
| [08_roadmap.md](08_roadmap.md) | 구현 단계 S0–S10, 출구 조건, 공수 추정, 범위를 줄이는 순서, 위험(그림 D4 개요) | §9, §10.2 |
| [09_prior_work.md](09_prior_work.md) | 선행 연구와의 관계, 빌려 온 것과 새로움, [R04 §5]의 gap G1–G8(그림 D0) | §1.11, §0.3, §0.5, App. B |
| [10_decisions_adr.md](10_decisions_adr.md) | ADR 44개, PI 결정 목록(§2), 이름 충돌 주의(§0.2), 한국어 용어집(§3) | §10, §11, §13 |
| [diagrams/README.md](diagrams/README.md) | 그림 D0–D10 목록, 범례, 다시 렌더링하는 법 | §12 |
| [spec/SPEC_FINAL.md](spec/SPEC_FINAL.md) | 규범 원문(영문). 검토 쟁점 처리는 [spec/ISSUE_RESOLUTION.md](spec/ISSUE_RESOLUTION.md), PI 결정 원문은 [spec/PI_DECISIONS.md](spec/PI_DECISIONS.md) | — |
| [background/README.md](background/README.md) | 배경 보고서 [R01]–[R07]의 목록 | — |
| [history/](history/) | 설계 이력: 설계안 [A]/[B]/[C], 심사 [J1]–[J3], 검토 [X1]–[X3], SPEC v1 | — |

용어는 [10_decisions_adr.md §3 용어집](10_decisions_adr.md#3-용어집), 이름이 겹치는 기호(그림 D4와 PI 결정 D4, 보장 G5와 gap G5 등)는 [§0.2 이름 충돌 주의](10_decisions_adr.md#02-이름-충돌-주의)에서 찾는다.

---

## 부록 A. 처음 읽을 때 필요한 용어

이 문서를 읽는 데 꼭 필요한 말만 남겼다. 전체 용어집은 [10_decisions_adr.md §3](10_decisions_adr.md#3-용어집)이고, 그 원문은 spec §13이다.

| 용어 | 이 문서에서의 뜻 |
|---|---|
| actor | 실행 흐름 하나(OSAL task, raw thread, 외부 입력 근원) |
| basis B / B′ | 스케줄을 뺀, 실행을 정하는 모든 것 / 구성이나 입력을 바꾼 새 basis. "새 버그"의 근거가 아니다 |
| certificate | 안전성 진술 하나(fixed-order의 `¬OK_i` 또는 protected의 `¬NR_i`)의 검사 가능한 증명 기록. `kz proofcheck`가 명시한 trusted base에 상대적으로 추론 단계를 다시 검증한다 |
| K-* | 계약 template family의 이름(예: K-ID, K-ROUTE). 목록은 [05_cfs_mapping.md §3](05_cfs_mapping.md#3-계약-라이브러리와-15개-계약-종류) |
| LP / capture point | linearization point. 연산의 효과가 한순간에 일어났다고 보는 지점이고, 조건(conjunct)마다 둔다 / 호출자가 넘길 값을 미리 읽은 load 지점. 값 조건의 LP다 |
| MustBefore(MB) / PWB | 모든 실행에서 반드시 먼저 일어나는 사건의 정적 집합(최대 고정점) / 둘 다 일어나면 항상 a가 먼저라는 관계 |
| S / O / C_σ (HB stratum) | 두 끝이 모두 있는 모든 실행에서 성립하는 구조적 순서 / 관측한 실행에서만 성립하는 순서 / σ_R1의 우선순위 제약(HB가 아님) |
| 발생 / 서수 | 사용 지점이 한 번 실행된 것. 호출에서 시작하고 늦어도 호출이 돌아올 때 결정된다 / (실행이 달라도 같은 이름의 actor instance, k): 그 actor가 그 key로 시작한 k번째 발생 |
| 안전성 기록 / class | 칸과 경계마다 세운 진술 `¬Bad_i`, `¬OK_i`, `¬NR_i`와 그 경로, 서수가 붙은 witness / 안전성 기록의 함수(§4.5) |
| 진행(progress) 결과 | 칸과 경계마다 `stall-witness`, `no-stall-within(b)`(`exh`만), `unknown(reason)`(기본 `not_checked`). class와 따로 적고, "live"를 뜻하지 않는다 |
| trust ledger | certificate와 판정 칸마다 붙는 네 부분 기록: 다시 검증한 단계, 추출 사실, 신뢰 항목, 관측으로 반박될 수 있지만 증명되지 않은 항목 |
| 세 판단 | framework 수준(`fw_doc`, `fw_conf`), 호출자 수준(`appreq`와 Allowed; 상태와 class를 정한다), 관측된 결과(consequence) |
| witness | 판정을 뒷받침하는 유한 실행 하나. 나쁜 witness와 좋은 witness는 발생 서수를 함께 적는다. stall witness는 진행 결과이고 class의 witness가 아니다 |
