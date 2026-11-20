# 04 형식 모형: actor, 자원, 준비 상태 명세, 의미론, 순서 관계, 보장

이 문서는 Kairos가 판정의 근거로 삼는 형식 모형을 설명한다. 실행의 주체(actor)와 대상(자원), 준비 상태 명세와 instance, 실행 집합을 정하는 기준 B·의미론 σ·모형 수준 M, 그 위에서 계산하는 순서 관계, 판정 기록, 그리고 보장과 가정을 다룬다. spec §1.2–1.10, §4.2–4.5(§4.5.1 신뢰 장부 포함)의 핵심, §6, App. A.8의 litmus 프로그램을 spec을 읽지 않은 연구자가 따라올 수 있게 풀어 쓴다. 기준은 spec 개정 1.3이다(아래 "rev 1.3 반영"). 개정 1.1은 외부 검토 EXT-20261012에 답한 것이고(spec §0.7, ADR-039–ADR-044), 개정 1.2는 2026-10-15의 PI 결정(결정문 [spec/PI_DECISIONS_RESOLVED_20261015.md](spec/PI_DECISIONS_RESOLVED_20261015.md))을 반영하고 형식 정정 둘(σ_E 진술, 신뢰 장부)과 증명 상태를 더했다(spec §0.9–§0.11, ADR-045–ADR-052). 개정 1.2에서 바뀐 곳은 §0.4에 모았다.

상태: 설계 문서 — 구현·측정 전. 규범 원문: [spec/SPEC_FINAL.md](spec/SPEC_FINAL.md)

> **rev 1.3 반영 (2026-10-22).** spec 개정 1.3은 PI 결정 D1–D5(결정문 [spec/PI_DECISIONS_RESOLVED_20261022.md](spec/PI_DECISIONS_RESOLVED_20261022.md))를 변경표 [../plan/REV13_CHANGE_TABLE.md](../plan/REV13_CHANGE_TABLE.md)(행 C-01–C-52)를 거쳐 적용했다(spec §0.12, ADR-053–ADR-058). 모두 결정 [D]이고 결과가 아니다. 이 개정에서도 빌드·실행은 없었다. §1의 정의, lemma와 그 증명 상태, 가정은 지우거나 약하게 하지 않았다. 이 문서에 옮긴 변경표 행: C-04·C-05(§1.4 실행 관계, `app_owner`), C-06(§2.5 EV-01–EV-04), C-07(§5.10 지원하는 순서 조건), C-08(§6 IN-01–IN-05), C-09(§7.6 모형 식별자), C-10(§11.6 요약 뷰), C-11(§11.7 고정 순서 근거), C-12(§11.8 OUT-01, OUT-02), C-22(§9.2, §10 머리의 선택 경로 표시), C-23(§10.7 certificate 없는 결과의 장부), C-30(§11.6 끝), C-33(§12.2 A-OBS), C-34(§12.3 L-OBS), C-35(§12.4 G2 표시, G3 다시 씀), C-36(§12.5 항목 58–62), C-49(§10 머리), C-52(이 문서). **우선순위**(spec §0.12, ADR-053): 개정 1.0–1.2를 위해 쓴 문장이 certificate, explorer, progress 결과, loop를 Kairos의 첫 산출물, 필수 산출물, 정의하는 산출물로 말하면 ADR-053으로 읽는다. 그 경로는 쓰인 그대로 정의되어 있고 그 주장이 필요할 때 쓰며, 관측 판정·예측·확인의 전제 조건이 아니다. 이 문서의 §9–§10(MustBefore, PWB, R2 certificate)과 §12.4 G2가 그런 경로를 다룬다.

> **rev 1.3 보충 (2026-10-24).** 2026-10-24 마스터플랜에서 받아들인 작은 보충 둘을 spec과 같이 옮긴다(spec 머리말 "Revision 1.3 amendment"; 반영 기록 [../plan/MASTERPLAN_ADOPTION_20261024.md](../plan/MASTERPLAN_ADOPTION_20261024.md) §2). 모두 결정 [D]이고, 정의·status·lemma·가정을 지우거나 약하게 하지 않는다. (i) 요구를 붙이는 호출 문맥의 규칙: §5.11. (ii) 정체성 네 범주의 구분: §1.4 끝.

## 목차

- [0. 읽기 전에](#0-읽기-전에): 표기, 예로 쓰는 사례, 모형의 뼈대, 개정 1.2에서 바뀐 곳
- [1. Actor, instance, once(τ), 네 가지 identity](#1-actor-instance-onceτ-네-가지-identity) (spec §1.2): 실행 사이의 actor instance 이름 포함; 실행 관계와 생성 앱 `app_owner`(spec §1.1, §1.2, rev 1.3); 정체성 네 범주(rev 1.3 보충)
- [2. 자원: kind, lifecycle automaton, generation, key](#2-자원-kind-lifecycle-automaton-generation-key) (spec §1.3): 자원 identity 규칙 EV-01–EV-04(rev 1.3)
- [3. 연산, effect, LP, capture point, 모형 수준 M](#3-연산-effect-lp-capture-point-모형-수준-m) (spec §1.4)
- [4. Event, template, trace](#4-event-template-trace) (spec §1.5)
- [5. 준비 상태 명세](#5-준비-상태-명세) (spec §1.6): 준비 상태 명세의 요소, Allowed와 horizon monitor, 발생과 status(`NOT_EVALUATED`, `PENDING` 포함), 준비 상태 위반 사슬과 progress 사슬, 세 판단, 이름과 근거 등급, 지원하는 순서 조건(rev 1.3), 요구를 붙이는 호출 문맥(rev 1.3 보충)
- [6. 분석 기준 B](#6-분석-기준-b) (spec §1.7): 관측한 실행의 basis 규칙 IN-01–IN-05(rev 1.3)
- [7. 스케줄·시간 의미 σ와 containment](#7-스케줄시간-의미-σ와-containment) (spec §1.8): 유한 실행과 maximal 실행, 시간 기반 전이, 죽은 actor와 stall, refinement, prefix containment와 전이 규칙(σ_E 진술의 정정 포함), 모형·스케줄 식별자(rev 1.3)
- [8. Skeleton의 두 view](#8-skeleton의-두-view) (spec §4.3, 그림 D10)
- [9. 순서 관계: strata, MustBefore, PWB, HB](#9-순서-관계-strata-mustbefore-pwb-hb) (spec §1.9, §4.2, §4.4, §4.5)
- [10. R2로 읽는 once, MustBefore, PWB, certificate](#10-r2로-읽는-once-mustbefore-pwb-certificate) (spec §4.5, §4.5.1, App. A.1, A.7): 검사기의 신뢰 기반, 신뢰 장부, 증명 상태
- [11. 판정 기록](#11-판정-기록) (spec §1.10): safety 기록과 class, reach·progress 축, shape, 세 판단의 필드, 증명 상태 필드, 요약 뷰·고정 순서 근거·세는 규칙(rev 1.3)
- [12. 보장과 가정](#12-보장과-가정) (spec §6, App. A.8): 가정(A-OBS 포함), lemma와 증명 상태(L-OBS 포함), 보장 G1–G11, 하지 않는 주장(항목 58–62 포함), litmus 프로그램
- [13. 관련 문서](#13-관련-문서)

---

## 0. 읽기 전에

### 0.1 표기

근거 표기는 spec 머리말을 따른다. 자세한 설명은 [01_overview.md §0](01_overview.md#0-읽는-법)에 있다.

| 표기 | 뜻 |
|---|---|
| [F] | 코드·기록·보고서 인용이 있는 사실 |
| [M] | 측정값. 이 문서의 [M]은 모두 spec이 인용한 **기존** 측정이다([BM], [R07], [R2g]) |
| [I] | 추론 또는 설계상 추론. 실행해서 확인하지 않았다 |
| [D] | 설계 결정. spec §11의 ADR이 기록한다 |
| [U] | 미확인 |

- 문헌은 [R04]·[R05]·[R06]이 확인한 범위에서만 쓰고 상태 표지를 붙인다. V-doc은 원문 확인, V-abs는 서지·초록 수준 확인, V-me는 보고서 작성자가 검색으로 서지·초록 수준을 확인한 것, P는 일부 확인이다.
- "(spec §4.4)"는 SPEC_FINAL.md의 절이고, "[R03 §4]"는 `background/`의 보고서다. 코드 위치 `path:line`은 고정된 cFS tree(cFS `088b2fa8`) 기준이다.
- **Kairos는 아직 한 번도 실행되지 않았다.** R2·C1에 대해 "판정이 나온다"고 쓴 것은 모두 spec App. A의 기대 동작이다 [I]. App. A의 제목 자체가 "expected behaviour; nothing here was run"이다. App. A.8의 litmus 프로그램(§12.6)도 계획만 있고 실행하지 않았다.

**인용 형식과 이름이 겹치는 기호.** 인용 형식(설계안 [A]/[B]/[C], 심사 [J1]–[J3], 검토 [X1]–[X3], [X3, here], v0.1 등)은 [01_overview.md §0](01_overview.md#0-읽는-법)에 모아 두었다. 같은 기호가 여러 뜻으로 쓰이므로 이 문서는 앞에 종류를 붙인다: "PI 결정 D4"와 "그림 D4", "보장 G5"와 "gap G5", "단계 S0"와 "Init snapshot S0", HB stratum S/O와 cold/warm 층(stratum). 전체 목록은 [10_decisions_adr.md §0.2](10_decisions_adr.md#02-이름-충돌-주의)다.

### 0.2 예로 쓰는 사례

| 사례 | 한 줄 요약 | 이 문서에서 보여 주는 것 |
|---|---|---|
| **R2** | cFE core가 시작하는 동안 ES와 EVS task가 SB 코드를 실행한다. 그 코드는 `CFE_EVS_SendEventWithAppID`에 SB의 AppId(`CFE_SB_Global.AppId`)를 넘긴다. 이 값은 SB task가 `cfe_sb_task.c:138`에서 쓰기 전까지 0(UNDEFINED)이다. 기존 gdb 기록에서 매 boot의 준비 상태 위반 사용 24개(원인 하나, 정적 site 3곳, ENV basis, [R2g §6])는 모두 core serialization으로 HB상 발행보다 **앞서도록 순서가 정해져** 있었다([R2g §2]; spec §1.11, §6.4 항목 21) | 네 가지 identity, capture point, once, MustBefore, PWB, certificate, 신뢰 장부 |
| **C1** | CF가 table에 적힌 이름으로 OSAL counting semaphore를 조회한다. 그것을 만드는 provider 앱과의 순서를 정하는 장치가 없다. default B에서는 이름이 비어 있어 이 경로가 실행되지 않는다([LED]; spec App. A.4). 이름이 비어 있지 않은 파생 설정 B′는 D22가 정했다: 공개 BP 기반, 후보 쌍 CF `281a941` → `833fdbb` [D]. 아직 만들지 않았고 버전 호환성도 확인하지 않았다 [U] (spec App. A.4 step 2) | `inactive`, 지연 retry, 시간 기반 전이와 σ_U∞, timeout-dependent, 좌초(stranded), `leaked-lock` stall, M_impl |
| **SB 구독 전 송신** | 구독(route)이 생기기 전에 보낸 message는 success를 돌려받으면서 조용히 사라진다(spec §1.11) | 세 판단(framework 수준 `documented-not-error`와 caller 수준 LOSS-UNDER-ORDER), Allowed가 정해지지 않은 준비 상태 명세, 준비 상태 위반 사슬 |
| **OSAL RESERVED 구간** | OSAL object 생성 도중, 이름은 이미 보이지만 최종 ID는 아직 쓰이지 않은 구간이다(spec §1.3, §3.2) | lifecycle 상태 RES_CN, M_api와 M_impl의 차이 |

### 0.3 모형의 뼈대

각 줄은 앞줄의 개념을 쓴다. 용어의 짧은 설명은 [10_decisions_adr.md §3 용어집](10_decisions_adr.md#3-용어집)에 있다(원문 spec §13).

| 개념 | 한 줄 정의 | 절 |
|---|---|---|
| actor | 실행 흐름 하나(OSAL task, raw thread, 외부 입력 근원) | §1 |
| 자원 | framework가 관리하는 대상. kind마다 lifecycle 상태 기계가 있다 | §2 |
| 연산과 effect | actor가 부르는 API. 자원 상태를 바꾸거나(lifecycle) 자원을 쓴다(use). 효과마다 일어나는 지점(LP)이 있다 | §3 |
| event, trace | LP에서 일어난 효과 하나 / actor별 program order를 지키는 event 열 | §4 |
| 준비 상태 명세 | "이 사용 지점에서 이 자원은 이 상태여야 한다"(준비 사전조건 `Req`)와 caller 수준에서 허용되는 not-ready 행동. 사용의 발생마다 status를 정한다 | §5 |
| B, σ, M | 유한 실행 집합 `Pref(B,σ,M)`과 도달 가능한 상태를 정한다 | §6, §7 |
| skeleton view | 정적 분석이 실행 집합을 근사하는 actor별 자동자 | §8 |
| 순서 관계 | MustBefore, PWB, trace의 HB | §9, §10 |
| 신뢰 장부 | certificate와 판정 칸이 무엇을 다시 확인했고 무엇을 믿었는지 적은 네 부분 기록. 믿은 lemma의 증명 상태를 함께 적는다. rev 1.3부터 certificate 없는 관측 판정·예측·확인에도 붙는다 | §10.7 |
| 판정 기록 | instance마다 (σ, M) 칸·경계별 safety 기록과 class, reach, progress, 근거 축, 증명 상태. rev 1.3부터 그 위에 요약 뷰를 더한다 | §11 |
| 보장 | 위 모두와 가정 A에 상대적인 safety 진술과 progress 진술. `no-stall-within(b)`는 b 안의 모형에 대해서만 말한다. 모든 lemma와 보장은 증명 상태(`proved`, `depends-on-stated-assumption`, `unproved`)를 지닌다 | §12 |

### 0.4 개정 1.2에서 바뀐 곳

spec 개정 1.2가 이 문서의 범위에 준 변경이다. 모두 [D]다. PI 결정은 연구 계획, 범위, 용어에 대한 결정이고 사실이나 실행 결과가 아니다. 정의와 문장의 정정은 정의를 바꾼 것이다. 이 개정에서도 빌드·설치·실행은 없었고 Kairos의 결과는 없다. 정정 전후의 문장과 손으로 계산한 최소 예제는 [../plan/formal_corrections.md](../plan/formal_corrections.md), R2 첫 결과의 증명 상태표는 [../plan/proof_status_R2.md](../plan/proof_status_R2.md)에 있다.

| 바뀐 것 | 이 문서 | spec |
|---|---|---|
| σ_E 진술을 L-FO·L-PR이 증명하는 꼴로 정정: 판정된 범위 안 발생에 대한 진술이고 `NOT_EVALUATED` 조기 반환을 허용한다 | §7.5 전이 규칙 2, §10.4, §10.6, §11.1, §11.4, §12.3 L-TRANS, §12.4 G2, §12.6 LP-EARLY | §1.8, §1.10, §6.2, §6.3, A.1 step 9, A.7, A.8, §0.9 항목 1, ADR-045 |
| 신뢰 장부에 L-CONT (c)와 L-STATUS를 넣음: reach나 witness가 실행에 기대는 모든 칸, certificate 자신의 칸 포함. 그 실행 쪽 전제는 O1, O2, O4, O5가 반박한다 | §10.7 | §4.5.1, A.1 step 9a, §0.9 항목 2, ADR-045 |
| 증명 상태 `proved` / `depends-on-stated-assumption` / `unproved`를 lemma와 보장마다 두고, 의무로만 적힌 lemma를 "증명됨"으로 출력하지 않음(PS1–PS4) | §10.5, §10.7, §11.2, §11.5, §12.3, §12.4, §12.5 | §6.2, §6.3, §2.3, §6.4 항목 45·46, §0.9 항목 3, ADR-045 |
| caller 수준 판단: 규칙만으로는 받아들이지 않음(QT-4), 정상 연속은 인용한 노드(Q15), 충돌은 판정 보류(QT-5), K-SPAWN.osal·K-LOCKBAL은 구현 결함 계열(QT-9) | §5.2, §5.3, §5.8, §7.4, §12.6 LP-ALLOW | §1.6, §6.4 항목 53, ADR-043(개정), ADR-048 |
| C1 기준 구성 D22: 공개 BP 기반, 후보 쌍 CF `281a941` → `833fdbb` | §0.2, §3.5, §7.4 | §0.9 항목 5, A.4 step 2·3·6, ADR-044 |
| R2 표현: harm `event_request_rejected`, "이벤트 요청 거부" | §5.1, §5.3, §5.7 | §0.9 항목 4, §6.4 항목 47, A.1 step 8·13a |
| 용어 T1 = (a): 준비 상태 명세, 준비 사전조건, 준비 상태 위반, 근거 등급, `allowed-not-ready` | 문서 전체, §5.9 | §0.11, §1.6, 부록 C, ADR-052 |
| CORD 인용 정리(D12): R2의 실행 근거는 gdb 기록뿐 | §0.1, §0.2, §5.7, §6, §10.7 | §0.11, 머리말 |

---

## 1. Actor, instance, once(τ), 네 가지 identity

### 1.1 Actor의 세 부류

Actor 집합은 `T = T_osal ∪ T_raw ∪ T_env`다 [F] (spec §1.2; [R01 §2.6]; [R02 §1.1]).

| 부류 | 무엇인가 | cFS 예 |
|---|---|---|
| `T_osal` | `OS_TaskCreate`로 만든 task | cFE core main task, 앱 main task, child task(`ES_BG_TASK`, TIME의 tone·local child, FM의 child) |
| `T_raw` | OSAL task record가 없는 thread | PSP main thread(`CFE_ES_Main`을 돌리는 "ES startup thread", 나중에 `OS_Application_Run`), console writer, timebase thread `cFS-Master`, signal handler 문맥 |
| `T_env` | 환경 근원 | ground 명령, SBN peer, stop 명령. 들어오는 HB edge가 없다([R04 §4 row 17]) |

**static actor** τ는 closure 항목 하나, 즉 생성 지점(spawn site)과 entry 함수의 쌍이다(spec §3.1). **actor instance**는 `(τ, generation)`이다. skeleton(§8)의 노드는 static actor에 속한다.

**실행 사이의 actor instance 이름** [D] (spec §1.2; EXT-20261012 §4.1). 발생 ordinal(§5.5)은 서로 다른 실행의 발생을 비교한다. 그래서 actor instance에는 실행의 identifier에 기대지 않는 이름이 필요하다.

- 모형에서 actor instance는 `(τ, generation, n)`으로 부른다. n은 정규 이름을 가진 생성자(spawner)가 τ의 생성 지점을 실행한 횟수다. once actor와 raw thread는 n = 1이다.
- callback은 actor instance가 아니다. callback의 발생은 그것을 실행하는 actor에 속한다. OSAL timer callback은 timebase thread에, TIME synch callback은 TIME의 task에, `ES_BG_TASK`의 job은 `ES_BG_TASK`에 속한다.
- σ_E에서는 실행 수준 identity `(type, name, creator, creation ordinal)`(§1.3)이 이 역할을 한다. closure를 거쳐 `(τ, generation, n)`으로 옮긴다. 생성자의 정규 이름과 생성 event의 호출 위치(`creator_event`)를 τ의 생성 지점과 맞추면 τ와 생성자가 나온다(script 앱이면 startup script 줄, §6). n은 그 생성자가 τ의 생성 지점을 실행한 순서 안의 creation ordinal이다. generation은 실행 안에서 앱이 재시작한 횟수다. O5가 이 대응이 쓰는 closure를 검사한다.
- 실행 witness와 모형 witness는 이 대응을 거쳐서만 같은 ordinal로 짝짓는다. 대응이 풀지 못한 identity로는 섞인 쌍을 만들지 않는다.

### 1.2 once(τ): "이 actor는 한 실행에 많아야 하나다"

[§9.4](#94-pwb-둘-다-일어나면-언제나-a가-먼저)의 순서 증명(FRONTIER, ONCE-PWB)은 "그 actor의 **유일한** 실행 흐름"을 근거로 삼는다. 같은 entry의 instance가 둘이면 한 instance의 지점이 다른 instance의 지점 뒤에 올 수 있어 논증이 깨진다. 그래서 once는 **가정하지 않고 증명한다** [D] (spec §1.2; [X1-M2] 수정).

- PSP main thread는 정의상 once다(process entry, 가정 A3).
- 생성 지점이 `s_1 … s_k`인 actor는 다음이 모두 성립할 때만 once다.
  1. 모든 `s_i`가 once actor 안에 있다.
  2. 어떤 `s_i`도 certificate view(§8)의 star-closed loop 영역, 즉 "몇 번이든 반복"으로 닫은 loop 안에 있지 않다.
  3. 생성 지점들이 서로 배타적이다. 모두 같은 once actor 안에 있고, 어느 두 지점 사이에도 어느 방향으로든 skeleton 경로가 없다. actor의 실행 하나는 경로 하나이므로 배타적인 두 지점은 함께 실행될 수 없다.
- 환경 근원과 비동기 문맥(signal handler, 예외 경로, callback 문맥)은 결코 once가 아니다(spec §1.2, §3.1).
- **노드의 once**: 노드 `n`은 actor가 once이고 `n`이 star-closed 영역 밖이면 once다. 그러면 `n`은 한 실행에 많아야 한 번 일어난다.

once 사실은 그것을 쓰는 certificate에 유도 과정과 함께 적는다. 독립 검사기 `kz proofcheck`는 자기 once routine으로 skeleton 위에서 다시 확인한다. skeleton 자체는 검사기가 믿는 입력이다(신뢰 기반 T2, §10.5). once가 아닌 actor에는 frontier·once-PWB 논증을 쓰지 않는다. 재시작 시나리오 `B_restart`(문서화된 요청 하나)에서는 skeleton을 generation `g ∈ {0, 1}`마다 복제하고, 복제본마다 자기 생성 지점을 두어 각 복제본이 once가 되게 한다.

**R2의 예** (spec App. A.1 step 7, [I]). core object table loop는 반복 횟수가 상수라 정확히 펼친다(spec §3.1). 그러면 ES main과 EVS main은 각각 생성 노드가 하나뿐이고, 그 노드는 once인 startup thread 위, star-closed 영역 밖에 있다. 따라서 `once(ES main)`, `once(EVS main)`이다.

**반례** (litmus L-ONCE, spec App. A.8). loop 안의 생성 지점이 같은 entry의 instance 둘을 만들면 once가 성립하지 않는다. frontier와 once-PWB가 금지되고, 기대 class는 schedule-dependent다.

### 1.3 Identity와 네 가지 identity 필드

instance마다 identity record `⟨kind, osal_id, serial, tid, name, creator_event, generation⟩`를 둔다. OSAL ID는 `(type << 16) | serial`이라 실행마다 바뀌므로, 실행 사이 대응은 `(type, name, creator, creation ordinal)`로 맺는다 [F] ([R01 §1.2, §6.2 항목 3]).

event마다 identity 필드 넷을 두고 **결코 합치지 않는다**(spec §1.2; [R06 F1]).

| 필드 | 정의 | 따로 두는 이유 (cFS 사실) |
|---|---|---|
| `exec(e)` | `e`를 실행하는 actor instance | SB 코드는 호출자의 task에서 돈다([R02 §2.4, §2.8]) |
| `owner(e)` | 그 시점에 `exec(e)`를 task 집합에 가진 ES 앱(generation 구분). raw thread는 `⊥` | 앱 하나에 main task와 child task가 있다([R02 §1.12]) |
| `svc(e)` | 활성 frame의 stack: API frame(SB, EVS, TBL, …)과 on-behalf frame `cb(registrant)` | TIME synch callback은 등록한 앱을 위해 TIME task에서, FS callback은 SB/TBL을 위해 `ES_BG_TASK`에서, OSAL timer callback은 timer 생성자를 위해 timebase thread에서 돈다([R02 §5.3, §6.1]; [R01 §3]) |
| `stamp(e)` | 코드가 명시적으로 넘기거나 저장하는 identity 값(`CFE_EVS_SendEventWithAppID`의 `AppID` 인자, `CFE_SB_CreatePipe`가 저장하는 pipe 소유자) | R2: ES task가 SB 코드를 실행하며 SB의 발행되지 않은 AppId를 찍는다([R2g §2]) |

R2의 사용 하나에 정의를 적용하면 네 필드가 모두 다르다. `exec`는 ES main task, `owner`는 ES 앱, `svc`에는 SB API frame이 있고, `stamp`는 SB의 AppId 값이다. thread identity 하나만 보는 검출기는 이 구분을 표현하지 못한다(spec §1.11의 gap G6 행).

### 1.4 실행 관계와 생성 앱 (rev 1.3)

**실행 관계** [D] (spec §1.1 "Execution relations"; [P0.2] §1.2; C-04). [P0.2]는 2026-10-22 사용자가 올린 제안 0.2로, 개정 1.3의 기초다(D1). process-level은 cFS 애플리케이션들이 상호작용하는 범위를 뜻하고, 서로 다른 운영체제 process를 요구하지 않는다. 순서 분석의 단위는 task나 문맥(§1.1의 actor)이고, 소유와 역할이 붙는 단위는 애플리케이션이다. 모든 결과는 관련된 actor 사이의 관계를 밝힌다.

| 실행 관계 | 처리 |
|---|---|
| 서로 다른 애플리케이션의 task | 주 대상 |
| core service의 task, 또는 애플리케이션과 core 실행 경로 | 주 대상 |
| 애플리케이션 생성 framework와 새 애플리케이션의 task(C5) | 포함한다. 실행과 소유가 형성되는 과정으로 본다 |
| 한 애플리케이션 안의 task끼리만 | 대조군이나 기법 점검으로 쓸 수 있다. 앱 간 결과로는 결코 세지 않는다 |
| 애플리케이션이 아직 정해지지 않은 task | 위반과 미확정 소유를 따로 보고한다. 앱 간 결과로 세지 않는다 |

**생성 앱 `app_owner`** [D] (spec §1.2 "Creating application"; ADR-056; C-05). 네 가지 identity(§1.3) 말고도 모든 actor instance는 `app_owner`, 곧 그것을 만든 애플리케이션을 지닌다. 그 수명, 근거, 근거 수준 `confirmed | inferred | undetermined`를 함께 적는다. 근거는 다음 순서로 쓴다(spec §5.7).
1. 생성 요청: 애플리케이션이나 child task를 시작하는 ES 호출과 그 인자.
2. 생성하는 OSAL 호출, 그리고 그 호출의 entry–return 구간 안에서 생성자가 만든 thread의 kernel fork 기록.
3. 자식이 첫 event에서 알리는 OSAL id.
4. ES task record와 App record. **다만 그 record가 순서를 분석하는 자원 자체이면 쓰지 않는다**(EV-03, §2.5). C5에서는 task–애플리케이션 table 자체가 대상이므로, 그 table의 조회로는 결코 매핑을 세울 수 없다.

생성자 쪽 out 매개변수 없이 시간 포함 관계에만 기댄 매핑은 `inferred`이고 `match-by-time`으로 표시한다. 매핑은 정규화(spec §4.8 step 2)의 필수 산출물이고, process 안에서 framework 조회를 불러 얻지 않는다(spec §5.7). 정규화는 아래 네 범주를 따로 둔다(rev 1.3 보충). actor의 `app_owner`가 미확정인 instance의 결과는 따로 보고하고 앱 간 결과로 세지 않는다(ADR-056; §12.5 항목 61). 요약 뷰의 실행 관계는 §11.6에 있다.

**정체성 네 범주** [D] (rev 1.3 보충 2026-10-24; spec §1.2 "Identity categories kept apart"; ADR-056). 다음 넷을 따로 기록하고 **어느 둘도 같다고 가정하지 않는다**.

| 범주 | 뜻 | 기록하는 곳 |
|---|---|---|
| 실행 task | event를 실행하는 actor instance | `exec(e)` |
| 생성 실행자 | 그 task의 생성 호출을 실행한 actor. 스크립트 앱이면 `CFE_ES_AppCreate`를 도는 ES 경로, child task이면 그것을 만든 task | `creator_event` |
| 소유(생성) 앱 | task가 속한 애플리케이션 | `app_owner`(근거·수준), event 시점의 `owner(e)` |
| 전달 AppID | 인자로 넘기거나 저장한 identity 값(예: `CFE_EVS_SendEventWithAppID`의 `AppID`) | `stamp(e)` |

둘이 같다는 것은 그 instance의 근거로 보여야 하는 사실이고 기본값이 아니다. 생성 실행자는 자기가 만든 task의 소유 앱이 아니다(ES가 CF와 BP의 task를 만든다). 함수가 어느 모듈 파일에 있다고 실행 task가 그 모듈의 앱에 속하지 않는다(SB 코드는 호출자의 task에서 돈다). 전달 AppID를 실행자의 정체성으로 믿지 않는다(R2: ES task가 SB의 발행되지 않은 AppId를 넘긴다). 앱 간 결과에는 각 actor의 소유 앱이 필요하고, 나머지 셋은 함께 기록한다.

---

## 2. 자원: kind, lifecycle automaton, generation, key

### 2.1 자원의 네 요소

자원은 `r = ⟨kind, scope, key, gen⟩`이다(spec §1.3).

- `scope ∈ {process, persistent}`. persistent 자원은 process가 끝나도 남는다(CDS, boot record, reset area, `/dev/shm/osal:RAM`의 파일, `EEPROM.DAT`, [R01 §5.3]).
- `key`는 kind 안에서 자원을 가리키는 값이다. 이름, 숫자 ID, `(MsgId, pipe)`, slot 번호, 경로, scope가 붙은 symbol, 또는 **할당 기록** `ρ(n)`(§2.4)이다. 정적 분석에서는 symbolic일 수 있고(예: "CF table 필드 `chan[i].sem_name`의 문자열"), 모르면 `⊤`다.
- `gen`은 OSAL serial, cFE resource-ID serial, 또는 재시작 generation이다.

### 2.2 Lifecycle automaton (typestate)

kind `k`마다 lifecycle automaton `A_k = (Q_k, q0_k, Σ_k, δ_k)`, 즉 상태 집합, 초기 상태, 행동 집합, 전이 함수가 있다. 이 방식을 **typestate**라 한다. 객체의 현재 상태가 허용되는 연산을 정한다는 개념이다(Strom & Yemini, IEEE TSE 1986 [V-me]). 주요 kind는 다음과 같다. 전체 표는 spec §1.3이다.

| kind | 상태 | 발행 지점 (LP) |
|---|---|---|
| `osal.obj<T>` | FREE → ALLOC → RES_C(이름 안 보임) → RES_CN(이름 보임, RESERVED, type lock **안** 잡음) → LIVE → RES_D → FREE | type lock 아래 최종 ID 쓰기 `osapi-idmap.c:1092`, unlock `:1105` |
| `es.app` / `es.task` / `es.lib` | FREE → RESERVED → LIVE → RESERVED(정리 중) → FREE | core 앱: 예약 `cfe_es_start.c:803`, 발행 `:818`, 실패 시 지움 `:823`. script 앱: 예약 `cfe_es_apps.c:821`, 발행 `:869`, 실패 시 해제 `:893` |
| `es.self(exec)` | ABSENT → LIVE(실행 task의 task record와 owner의 App record가 모두 조회됨) | 위의 발행. `CFE_ES_GetAppRecordByContext`가 읽는다 |
| `cell(loc)` | INVALID → VALID | `loc` 값을 유효하게 만드는 store. 예: `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`(`cfe_sb_task.c:138`) |
| `evs.reg(slot)` | UNREG → REG. `CFE_EVS_Register`는 먼저 slot을 memset한다(`cfe_evs.c:56`) | store `cfe_evs.c:106`, lock 없음 |
| `sb.route(m,p)` | NONE ↔ ROUTED | SB lock 아래 `CFE_SB_SubscribeFull` |
| `sb.queue(p)` | 깊이 `0..cap_B(p)`. `cap_B(p) = min(요청, msg_max)`는 euid ≠ 0일 때만 | [M] `msg_max` 10, pipe 26개 중 22개가 잘림([BM §3.1]) |
| `es.shutdown` | RUN → SHUTDOWN_REQ → TEARDOWN | `ShutdownReq = true`(`cfe_psp_support.c:87`); `OS_DeleteAllObjects`(`cfe_psp_start.c:507`) |

**OSAL RESERVED 구간**은 `osal.obj<T>`의 RES_CN 상태다. 이 구간에서 by-name 조회는 기다리지 않고 곧바로 `OS_ERR_INCORRECT_OBJ_STATE`를 받는다(`osapi-idmap.c:401–404`). by-ID 조회는 제한된 횟수만큼 기다린다(spec §3.2). 발행 LP는 unlock 뒤에 도는 생성 callback이 아니라 lock 아래 최종 ID 쓰기다([R01 §6.1]). 이 구간은 모형 수준 M_impl에만 나타난다(§3.5).

### 2.3 `cell(loc)`: 일반 kind

`cell(loc)`은 값이 자원 identity인 공유 위치를 다루는 **일반** kind다(spec §1.3). R2의 `CFE_SB_Global.AppId`는 `cell(CFE_SB_Global.AppId)`다. `svc.identity(SB)` 같은 이름은 표시일 뿐 Φ 항목이 아니다. 그래서 R2에는 SB 전용 kind도 준비 상태 명세도 필요 없다(spec App. A.1 step 1).

### 2.4 할당 기록 `ρ(n)`

**할당 기록** `ρ(n)`은 펼친 **예약 노드** `n`이 할당하는 추상 record다 [D] (spec §4.2 step 8). 예약 노드는 빈 record를 골라 사용 중으로 표시하는 호출이다. core loop 반복마다의 `SetUsed(RESERVED)`(`cfe_es_start.c:803`)와 script 줄마다의 `cfe_es_apps.c:821`이 예다. 두 가정이 붙는다. 둘 다 `DECLARED_TRUSTED`(분석이 검증하지 못하고 믿은 항목)이고 관측으로 반박할 수 있다(O9).

- **A-FRESH**: 서로 다른 예약 노드는 서로 다른, 전에 쓰인 적 없는 record를 할당한다.
- **A-OWNER**: `StartAppTask(…, ParentAppId)`로 만든 task가 조회하는 App record는 `PendingAppId`가 `ParentAppId`로 흘러간 예약 노드의 `ρ(n)`이다. 흐름 자체는 def-use로 계산하고, task record → App record 단계만 믿는다.

### 2.5 자원 identity 규칙 (rev 1.3)

[D] (spec §1.3 "Identity rules for resources"; [P0.2] §5.2; C-06). 자원은 개념상 kind, 이름, ID나 주소, namespace, 수명으로 식별한다. kind마다 그중 무엇이 identity를 정하는지는 profile이 적는다.
- **EV-01.** 주소, 이름, ID가 다르다는 것만으로 다른 자원이라고 하지 않는다. 값 전달, 조회 결과, 생성 결과가 그것들을 이을 수 있다.
- **EV-02.** 숫자나 이름이 같다는 것만으로 같은 자원 수명이라고 하지 않는다(generation, §2.1; ID 재사용).
- **EV-03.** 소유 table 자체가 분석하는 자원이면(C5) 그 조회로 관측자의 task–애플리케이션 매핑을 세우지 않는다(ADR-056, §1.4).
- **EV-04.** API의 entry와 return 사이에 효과가 여럿 일어나면 검사 지점과 효과 지점을 나눈다(conjunct별 LP, §3.2). 정확한 지점을 얻을 수 없으면 가능한 구간과 순서의 모호함을 그대로 둔다.

---

## 3. 연산, effect, LP, capture point, 모형 수준 M

### 3.1 연산과 effect

연산 `op ∈ Φ.ops`는 binding(symbol 또는 AST pattern, 줄 번호가 아님), 역할이 붙은 인자(name, id, out-id, AppId, stamp, buffer, msg), 결과 code → `{ready, not_ready(q), error}`의 outcome map, 그리고 effect 목록을 가진다(spec §1.4). effect는 `lifecycle(r, a) when guard`(상태를 바꿈), `use(r)`(준비 상태 명세의 대상), `sync(obj, role)`(acq, rel, give, take, put, get, create, start, join, observe, timeout 등), `env(input)`, `mark(handler_begin | handler_end)`다.

outcome map의 not-ready code마다 **문서화된 결과**를 붙인다. supplier 쪽 reference가 그 code에 대해 문서화한 부수 효과(counter, event, drop)와, 그 문서의 `prov`(side `supplier`, §5.8)다. 예: route가 없는 `CFE_SB_TransmitMsg`는 `NoSubscribersCounter++`, `SEND_NO_SUBS` event, `CFE_SUCCESS`다(`cfe_sb_priv.c:1082–1087`, cSB4305.5). 이것은 framework 수준 판단의 근거이고, 발생의 status를 정하지 않는다(§5.8).

### 3.2 LP: 효과가 "여기서 일어났다"고 보는 지점

**linearization point(LP)**는 시간이 걸리는 연산의 효과가 한순간에 원자적으로 일어났다고 볼 수 있는 한 지점이다. Kairos는 이것을 코드의 구체적 명령 위치로 정한다.

**준비 사전조건(`Req`)의 conjunct마다 LP를 따로 둔다** [D] (spec §1.4; [X1-M1] 수정). conjunct는 `P_1 ∧ … ∧ P_n`에서 ∧로 묶인 각 조건이다.

- lifecycle effect의 LP는 API 안에서 그 효과가 실제로 일어나는 명령 수준 event다.
- use의 conjunct는 프로그램이 그 상태를 **읽는** 지점을 LP로 선언한다.
  - **registry conjunct**(상태가 framework table에 있음): API 안의 읽기. 예: EVS의 slot 검사.
  - **value conjunct**(호출자가 넘기는 값에 대한 요구, role `stamp`/`key`/`id`): 아래의 **capture point**.

### 3.3 Capture point와 T-DERIVE

**capture point**는 호출자가 인자 값을 공유 위치에서 읽어 온 load 지점이다.

- `kz-static extract`는 AST 수준 def-use(정의–사용 연결) 분석으로 `derives(arg, loc, site)`를 낸다. 인자 값이 공유 위치 `loc`을 `site`에서 읽은 값의 복사 사슬(load, store, 인자 전달, 산술 없는 cast)일 때다.
- 그러면 conjunct를 `site`에서 `cell(loc)` 위로 평가한다. 이것이 번역 규칙 **T-DERIVE**다. 일반 규칙이고 SB에 대한 지식이 없다.
- 유일한 `derives` 사실이 없으면 capture point는 호출 자체이고, 그것이 필요한 certificate는 보류되어 `unknown(writer_incomplete)`가 된다.

**왜 필요한가** (R2, litmus L-CAPTURE). SB 코드는 `CFE_SB_Global.AppId`를 인자 평가 때 읽고(`cfe_sb_api.c:226–228`), EVS는 그 복사본을 나중에 검사한다(`cfe_evs.c:185–194`). 읽은 뒤 검사 전에 SB가 값을 발행해도 검사되는 것은 옛 값이다. 그래서 결과는 capture point의 순서를 따른다. spec은 검사 LP를 겨냥한 예측기라면 거짓 good witness를 받아들일 것이라고 적는다(spec App. A.8).

### 3.4 LP의 예

[F] (spec §1.4)

| API | effect / conjunct | LP |
|---|---|---|
| `OS_*Create` | 이름 보임 / 발행 | `OS_OBJECT_INIT`의 `name_entry` 쓰기(type lock 풀림) / type lock 아래 최종 ID 쓰기 `osapi-idmap.c:1092` |
| `OS_*GetIdByName` | `use(osal.obj(name))` | type lock 아래 이름 검색 |
| `CFE_ES_GetAppID` | `use(es.self(exec))` | ES lock 아래 `CFE_ES_GetAppRecordByContext()`(`cfe_es_api.c:748–763`) |
| `CFE_SB_TransmitMsg` | `use(sb.route(hdr.MsgId, ·))` | SB lock 아래 route_lookup(`cfe_sb_priv.c:1007–1104`), 그 뒤 lock 밖에서 목적지마다 put LP(`:1177–1216`) |
| `CFE_EVS_SendEventWithAppID` | `valid(arg.AppID)` (value) | `derives`가 준 capture point(예: `cfe_sb_api.c:226–228`) |
| | `evs.reg(arg.AppID) ∈ {REG}` (registry) | slot 검사 `cfe_evs.c:185–194` |

**outcome code는 검산이다.** 준비 여부는 conjunct마다 LP에서 평가한다. 사용 지점에서 프로그램이 받은 결과 code는 계산한 상태에 대한 일관성 검사(obligation O4)다. K-ID에서는 `ILLEGAL_APP_ID`가 value conjunct 실패와, `NOT_REGISTERED`가 registry conjunct 실패와 동치다(spec §1.4).

### 3.5 모형 수준 M_api와 M_impl

OSAL을 어디까지 펼쳐 모형화하는지가 두 수준으로 나뉜다 [D] (spec §1.4; [X1-C2] 수정).

| 수준 | 정의 | 판정의 뜻 |
|---|---|---|
| **M_api** | OSAL API 하나가 주 LP에서의 원자적 전이 하나다. create는 lookup에 대해 원자적이고, lookup은 found 또는 not found다. `Pref(B,σ,M_api)`는 OSAL API 계약을 원자적으로 따르는 **가상의** backend의 유한 실행 집합이다 | 그 가상 backend에 대한 주장, 즉 이식성 주장이다. POSIX 바이너리에 대한 주장이 아니다 |
| **M_impl** | OSAL idmap, task, binsem, queue 경로를 더 낮은 어휘 Φ_L-1(pthread mutex·condvar, `sem_*`, `mq_*`, `clock_nanosleep`, `pthread_create/cancel/join`)로 투영한다. API가 LP에서 쪼개지고, RESERVED 구간, 두 깨움 원인(broadcast, timeout)을 가진 RESERVED 대기, lock 누수 경로가 나타난다 | POSIX 구현까지 반영한 주장. σ_E로 옮기는 경로가 이쪽이다(§7.5) |

- OSAL 내부 lock은 M_impl에서만 interleaving을 제약하고, 어느 수준에서도 shape용 HB edge를 만들지 않는다.
- **M_impl-relevant 연산**은 두 수준이 다르게 모형화하는 연산이다: idmap 생성·조회·삭제·token 변환, task 생성·준비·삭제, binsem, queue 생성·put·get. cone(한 후보와 관련된 actor와 object를 닫은 집합, spec §4.10)이나 certificate에 이런 연산이 있으면 그것도 M_impl-relevant다.
- **두 수준이 다르면 그대로 보고한다.** M_impl에서 protected인데 M_api에서 준비 상태 위반이 가능하면 **portability hazard**다. M_impl에서만 가능하면 고정 판 OSAL POSIX의 **implementation-defect candidate**다. M_api 전용 certificate를 실제 바이너리 관측이 반박하면 **portability-hazard-evidence**이고, model fault가 아니다.

**예** ([I], spec App. A.4 step 4). C1의 파생 설정 B′에서 by-name 조회가 RESERVED 구간에 닿는 경우다. B′는 D22가 정했다: 공개 BP 기반, 후보 쌍 CF `281a941` → `833fdbb` [D]. 아직 만들지 않았고 버전 호환성도 확인하지 않았다 [U] (§7.4).

- M_impl에서 조회는 기다리지 않고 `OS_ERR_INCORRECT_OBJ_STATE`를 바로 받는다(`osapi-idmap.c:401–404`). 이 code는 CF의 `retry_until_ready` code 밖이므로 monitor는 결정에서 REJECT를 주고, 발생은 `VIOLATING`이다(safety). 이 항목이 status를 정하는 것은 `caller` record(CF의 인정 `cf_cfdp.c:1272–1278`, `code_admission` [I])가 있을 때뿐이다(§5.8, §7.4). 시간 기반 전이가 필요 없으므로 M_impl에서는 `Bad_i(σ_U∞)`이고 timeout-dependent가 아니다.
- K-LOCKBAL에 따라 countsem table lock이 잡힌 채 남는다. CF 자신의 instance에는 이 누수로 인한 stall이 없다. CF는 lock을 쥐고 있을 뿐 기다리지 않는다.
- 죽음 판정(§7.3)은 CF의 VIOLATING 시도에 사슬로 붙은 딸린 결과를 찾는다. wait-for root는 countsem type lock 하나다. `FinalizeNew`에서 죽은 provider, 같은 type lock을 잡는 뒤 연산에서 LP 전에 막힌 actor(생성, 삭제, by-name 조회, GetInfo, 모든 type의 lock을 GLOBAL mode로 잡는 객체 순회 `OS_ForEachObject*`; `OS_LOCK_MODE_NONE`을 쓰는 `OS_CountSemGive`, `Take`, `TimedWait`은 아니다. 예: SCH_LAB의 `OS_CountSemCreate`, `sch_lab_app.c:167`), CF를 정리하다 `CFE_ES_CleanupTaskResources`에서 죽은 `ES_BG_TASK`(그 뒤 어떤 앱도 정리·재시작·재적재되지 않는다)다. 모두 confirmable `leaked-lock` stall witness이고 harm은 `hang_or_deadlock`이다(litmus LP-LEAK, LP-CASE-C1).
- 결과는 **implementation-defect candidate**(F1)다. cone이 M_impl-relevant이고 두 수준이 다르므로 M_api 결과는 σ_E로 옮기지 않는다. 준비 상태 위반이 M_impl에만 있으므로 label은 `portability-hazard`가 아니다.

---

## 4. Event, template, trace

**event**는 `e = ⟨id, actor, seq, op, phase, args, outcome, keys, site, ι(e), lp_intervals, t⟩`다(spec §1.5).

- `seq`는 actor 안의 순번, 즉 program order token이다. `phase`는 `call`, `return`, `point` 중 하나다.
- `outcome`은 프로그램 **자신이** 받은 결과다([R06 F11]). `ι(e)`는 §1.3의 identity 네 필드다.
- `lp_intervals`는 conjunct마다 LP를 관측된 두 지점 사이에 끼운 구간이다(spec §4.8).
- `t`는 timestamp이고 **근거로만** 보관한다. 물리 시간은 HB를 주지 않는다(Lamport, CACM 1978 [V-abs]).

**trace** π는 각 actor의 event가 program order대로 나타나는 event 열이다.

**static template**은 같은 type을 가지지만 필드가 symbolic이고 경로 조건이 붙는다. template은 `(static actor, inline된 호출 문자열, site)`로 **문맥을 구분한다** [D]. `CFE_ES_TaskInit`에서 도달한 `CFE_SB_CreatePipe` 안의 SB event site와 ES 명령 loop에서 도달한 같은 site는 다른 template이다. 재귀가 있으면 그 요약은 `unknown`이다.

**구체화** `t ⊒ e`는 `e`의 actor와 호출 문자열이 `t`와 맞고, site가 같고, 필드가 `t`의 제약을 만족한다는 뜻이다. 정적 층, trace 층, 모형 층이 이 하나의 type 덕분에 검사기 하나를 공유한다.

---

<a id="5-준비-계약"></a>

## 5. 준비 상태 명세

<a id="51-계약의-요소"></a>

### 5.1 준비 상태 명세의 요소

**준비 상태 명세**(readiness specification)는 `κ = ⟨id, U, sel, Req, Oracle, Allowed, appreq, harm, prov, authored, guard, scope⟩`다(spec §1.6). 이름은 PI 결정 T1 = (a)로 정했다(2026-10-15 [D]; §5.9, spec 부록 C). 한국어로는 늘 준비 상태 명세를 온전히 쓰고 '명세'만 따로 쓰지 않는다.

| 요소 | 뜻 | 예 |
|---|---|---|
| `U` | 사용 pattern: 연산과 site 조건 | `CFE_EVS_SendEventWithAppID` 호출 |
| `sel` | event 필드에서 key를 고르는 selector | `evs.reg(arg.AppID)`, `osal.countsem(arg.name)` |
| `Req`(준비 사전조건, readiness precondition) | 필요한 lifecycle 또는 값 조건의 conjunct `P_1 ∧ … ∧ P_n`. 각각 core expression 언어로 쓰고 자기 LP를 가진다. 키로 고른 자원에 대한 요구이고 누가 그것을 약속했다고는 말하지 않는다 | `valid(arg.AppID) ∧ state(evs.reg(arg.AppID)) = REG` |
| `Oracle` | conjunct마다 **framework 유효성 predicate**. Kairos는 실행하지 않는다. 정적으로는 그 읽기 집합이 conjunct의 **지원 위치**(support location)를 정하고, 동적으로는 프로그램의 결과 code가 conjunct의 관측된 모습이다(O4) | `CFE_ES_AppRecordIsMatch(CFE_ES_LocateAppRecordByID(a), a)` |
| `Allowed` | caller 수준에서 받아들이는 not-ready 결과와 그 horizon(§5.2). 어떤 항목이 status를 정하는지는 §5.8에 있다 | `none`, `retry_until_ready(…)` |
| `appreq` | 이 사용이 호출하는 앱이나 mission을 위해 이뤄야 할 것을 말하는 reference, 또는 `unspecified_by_reference`(기본). supplier 쪽 글이나 규칙에서 가져오지 않는다(§5.8) | K-ID: `cfe_evs.h:169–172` |
| `harm` | 결과가 허용되지 않을 때의 효과 종류(§5.3) | `event_request_rejected` |
| `prov` | 출처 record `(kind, side, path:line 또는 요구 ID, 인용 문장의 sha256)`. kind ∈ {`api_doc`, `requirement`, `code_admission`, `code_pattern`, `issue`, `researcher`}, side ∈ {`supplier`, `caller`, `rule`}(§5.8). `requirement`는 cFS 기능 요구(`cfe/docs/cFE_FunctionalRequirements.csv`, 앱의 `*_FunctionalRequirements.csv`)다 [D, 2026-10-15 결정: QT-2 승인, spec §10.1] | `cfe_evs.h:168–172` |
| `authored` | 작성 시각, 작성자, 작성자가 그 API에 대해 미리 본 설계 사례(C1, C2, C3, C5, R2) | — |
| `guard` | profile을 판끼리 재사용하기 위한 version guard | `cfe ≥ 6.6.0` |
| `scope` | 암묵적 scope conjunct "결정 event 전에 종료 요청이 없다"(§5.4) | — |

### 5.2 Allowed와 horizon

**horizon**은 not-ready 결과가 허용되는지 판단하려고 사용 뒤를 어디까지 보는가다. 연구자가 고르지 않고 reference에서 가져온다 [D] (spec §1.6; [J1 X2]). 이 표의 Allowed는 **caller 수준** Allowed다(§5.8). 정적·trace·explorer 쪽 검사 방법은 spec §1.6 표에 있다.

| Allowed 항목 | 뜻 | horizon |
|---|---|---|
| `none` | 어떤 not-ready 결과도 허용하지 않는다 | 사용 자체 |
| `documented_drop(src)` | 사용하는 코드가 자기 요청을 설계상 버린다. 사용을 가진 모듈의 **caller 쪽** 글이 이것을 밝힌다. 예: EVS의 own-ID guard(`cfe_evs_utils.c:613–623`, 주석은 사용하는 코드인 EVS의 것, `caller`); SB의 task별 event 재귀 guard(`cfe_sb_priv.c:305–328, 773–790`; "This prevents recursive events from occurring", `cfe_sb_priv.h:550–558`; `caller`). 뒤의 것은 사용자가 SB의 event 코드다. caller 쪽 글이 밝히지 않는 drop은 `documented_drop` 항목이 아니고, 어떤 Kairos 규칙도 그것을 받아들이지 않는다. 그래서 호출자에게 ES task record가 없을 때 SB가 event를 버리는 것(`cfe_sb_priv.c:315–318`)에는 항목이 없다. 이것은 SB의 event 코드가 status를 무시하는 `CFE_ES_GetTaskID` 호출(`cfe_sb_priv.c:773`)의 K-SELF.es instance에서 일어나는 문서화되지 않은 drop이다. 그 caller 수준 Allowed는 `unspecified_by_reference`(NOT_READY_UNJUDGED, harm `unspecified_by_reference`, consequence는 event가 버려져 효과가 조용히 사라짐)이고, SB의 `appreq`가 기록되면 `none`(VIOLATING)이다(§5.8 `appreq` 단계 1; spec §3.9). supplier가 호출자의 요청을 문서대로 버리는 것(SB의 구독자 없음 drop, cSB4305.5)은 op의 outcome map에 있는 framework 수준 정보이고(§3.1), 이 항목이 아니다 | 사용 자체 |
| `returned_and_tested(codes)` | 문서화된 code를 돌려주고, 호출자가 그 code로 분기해 **정상 연속**(normal continuation)에 이른다. 정상 연속은 항목이 인용과 함께 **이름을 적은 노드**다(엄격한 대안 Q15, 2026-10-15 결정 [D]). 인용을 적지 않은 항목은 `unspecified_by_reference`다(아래 '정상 연속'). **error exit**(`CFE_ES_ExitApp`, `CFE_ES_ExitChildTask`, `OS_TaskExit`, `CFE_ES_ResetCFE`, `CFE_PSP_Restart`, terminal 연산)나 그 전의 실패 status 반환은 정상 연속이 아니다. 정상 연속에서 REJECT event가 함께 일어나면 REJECT가 이긴다 | 정상 연속까지(ACCEPT), 또는 REJECT event까지: error exit, 문서화된 전파가 없는 error 반환, 같은 actor의 같은 자원에 대한 다음 연산, actor 종료. trace가 먼저 끝나면 `unknown(horizon)` |
| `retry_until_ready(codes, bound)` | 호출자가 재시도한다 | build에서 상수 평가한 loop bound(예: CF의 실효 `CF_STARTUP_SEM_MAX_RETRIES`) |
| `fallback(op′)` | 호출자가 문서화된 대안을 쓴다 | actor의 다음 연산 |
| `defer_by_design(src)` | reference가 연기를 문서화했다(TO_LAB은 `CFE_ES_WaitForStartupSync` 뒤에 구독한다, `to_lab_app.c:64–68`) | 연기하는 actor가 실행하는 문서화된 연기 지점. 그 actor는 사용이 결정될 때 아직 없을 수 있다 |
| `exit(code, src)` | 문서화된 종료 | actor 종료 |
| `unspecified_by_reference` | reference가 허용 여부를 말하지 않는다 | — |

**정상 연속** [D] (spec §1.6; EXT-20261012 §4.1; 엄격한 대안 Q15, 2026-10-15 결정).
- 정상 연속은 reference가 문서화한 노드를 항목이 인용과 함께 적은 것이다. 인용은 `prov` record이고 kind는 `api_doc`, `requirement`, `code_admission`, `issue` 중 하나, side는 `caller`다.
- **인용을 적지 않은 항목은 `unspecified_by_reference`다.** monitor가 없고, 그 not-ready 발생은 NOT_READY_UNJUDGED이며, ACCEPT도 REJECT도 끌어내지 않는다. 코드가 보여 주는 것은 판단과 따로 사실로 추출해 보고한다.
- **후보 연속 `cont_candidate(n)`**은 판단이 아니라 사실이다. 함수 f 안의 test 노드 n(돌려받은 status에 대한 분기)을 보자. f의 certificate view CFG를 loop 반복마다 비순환으로 만든다. 정확히 펼친 loop는 펼친 대로 두고, n을 담은 그 밖의 loop는 back edge를 loop 출구로 돌린다. 이 그래프에서 n의 가장 가까운 post-dominator c 가운데, c가 f의 return도 exit도 아니고, c에서 돌려받은 status도 n과 c 사이 not-ready 갈래에서 쓴 변수도 live가 아닌 것이 `cont_candidate(n)`이다. f의 반환 값도 사용으로 센다. 그런 c가 없으면 `none`이다. 예: C1의 수정 전 판(§7.4).
- 발생은 n에서 시작하는 경로에서 code 사실 넷도 기록한다: error exit에 이르는지, non-success status를 반환하는지, 같은 자원에 대한 다음 연산이 있는지, actor가 종료하는지. 이 사실들은 발생의 `consequence`에 속한다(§11.5).
- `cont_candidate(n)`을 허용된 회복으로 읽는 것은 판단이고, 판단에는 reference가 필요하다. reference가 없으면 판단하지 않는다.
- **문서화된 전파.** reference(`api_doc`이나 `code_admission`)가 API가 not-ready status를 자기 호출자에게 돌려준다고 적으면, 항목이 그렇게 말할 수 있다(예: `CFE_EVS_Register`의 `cfe_evs.h:98`). 그러면 API 안의 test 노드와 합류는 무시한다. API가 status를 돌려주는 것은 REJECT가 아니다. 의무는 호출자 쪽에서 API가 돌아온 지점부터 같은 규칙으로 시작한다. 코드만 읽고 알아낸 전파는 이런 항목을 만들지 않는다.

**horizon monitor** [D] (spec §1.6; EXT-20261012 §4.1–4.2). `none`과 `unspecified_by_reference` 밖의 항목은 유한 실행 ρ(§7.1) 위의 monitor μ로 읽는다. 항목은 **감시 actor**(monitored actor), ACCEPT event, REJECT event를 정한다.

| 항목 | 감시 actor | ACCEPT | REJECT |
|---|---|---|---|
| `documented_drop` | 사용 actor | 결정에서 결과가 문서화된 drop이다 | 결정에서 그 밖의 결과다 |
| `returned_and_tested` | 사용 actor | `codes` 안의 not-ready 결과 뒤, test 노드를 지나 항목이 인용한 정상 연속에 이른다 | 결과가 `codes` 밖이다. 또는 ACCEPT 전에 error exit, test를 담은 함수의 실패 status 반환, 같은 자원에 대한 다음 연산, actor 종료에 이른다 |
| `retry_until_ready` | 사용 actor | 같은 loop instance의 뒤 시도가 `READY`다(`NOT_EVALUATED` 시도는 아니다) | loop가 소진 경로로 간다. 또는 시도 결과가 `codes` 밖이다 |
| `fallback` | 사용 actor | 다음 연산이 `op′`다 | 다음 연산이 다른 것이다. 또는 process 종료 단계가 아닌 방식으로 먼저 끝난다 |
| `defer_by_design` | 연기 지점을 실행하는 actor(TO_LAB은 그 main task). 결정 때 아직 없을 수 있다 | 연기 지점을 실행한다 | process 종료 단계가 아닌 방식으로 먼저 끝난다 |
| `exit` | 사용 actor | `code`로 문서화된 종료에 이른다 | process 종료 단계가 아닌 다른 종료다 |

- **인용한 정상 연속이 없는 `returned_and_tested` 항목**은 monitor가 없다. 그 발생은 NOT_READY_UNJUDGED다(위 '정상 연속').
- **경로 검사.** 감시 actor의 제어 노드에서 ACCEPT event로도 process 종료 단계(§7.3)로도 가는 `Sk^cert` 경로가 없으면 μ는 곧바로 REJECT를 준다. 아직 없는 actor면 가장 가까운 기존 spawn 사슬 조상에서 시작해 spawn edge를 따라 찾는다.
- **process 종료 단계에서와 그 뒤에는** μ가 ACCEPT도 REJECT도 내지 않는다. process가 끝날 때 PENDING인 발생은 PENDING으로 남는다. 실제 실행에는 그런 단계 뒤의 event가 없다.
- **teardown은 기본 scope 발생을 결정하지 않는다** [D] (scope가 "종료 요청 전"이기 때문이다, §5.4). 기본 scope 발생에 대해 μ는 TEARDOWN LP(`OS_DeleteAllObjects`, spec §3.5)에서와 그 뒤에 ACCEPT도 REJECT도 내지 않는다. 그 삭제는 감시 actor를 그 actor 자신의 event 없이 끝낸다. 그래서 teardown이 시작될 때 PENDING인 기본 scope 발생은 PENDING으로 남고, 그 실행은 `unknown(horizon)`을 준다. SHUTDOWN_REQ LP 뒤의 상태가 그 발생의 progress에서 빠지는 것(§7.3)과 같은 이유다. SHUTDOWN_REQ LP와 TEARDOWN LP 사이에 일어난 감시 actor의 event는 여전히 그 발생을 결정한다.
- ACCEPT와 REJECT는 ρ의 모든 연장에서 최종이다. 그 밖에는 μ가 OPEN이다. **OPEN은 결코 ACCEPT로 세지 않는다.**
- 끝나지 않는 horizon은 progress 문제다(§7.3). μ가 OPEN인 채 trace가 끝나면 `unknown(horizon)`이다.
- 정적 검사는 `Sk^cert` 경로 위에서, trace 검사는 trace 위에서, explorer는 탐색한 prefix 위에서 μ를 계산한다.

`unspecified_by_reference`이면 harm도 `unspecified_by_reference`다. must-deliver `appreq`가 없는 route·용량 준비 상태 명세에는 결과 label **LOSS-UNDER-ORDER**도 붙는다. 둘 다 관측된 효과를 지우지 않는다. 판정은 그 효과를 `consequence`로 기록한다(§11.5).

**SB 구독 전 송신**(K-ROUTE)은 세 판단으로 나눠 보고한다(§5.8; spec §1.11, §3.9, App. A.1 step 13a).
- framework 수준: cSB4305.5가 문서화한 결과이고, framework는 그것을 error로 보지 않는다(`documented-not-error`).
- caller 수준: TO_LAB처럼 소비자가 연기를 문서화한 곳에서는 `defer_by_design`이다. 소비자나 mission reference가 말하지 않는 곳에서는 `unspecified_by_reference`와 LOSS-UNDER-ORDER다(spec §3.15 RC5).
- 이 손실은 `documented_drop`이 아니다. Kairos는 이 손실을 준비 상태 위반이라고 단정하지 않는다. reference가 caller 수준 판단 근거를 주지 않는다는 사실을 label로 남긴다.

### 5.3 harm

어휘는 `event_request_rejected`(EVS가 패킷을 만들기 전에 요청을 거절하고 호출자는 status를 버림), `status_ignored`, `wrong_identity_stamped`, `app_error_exit`, `app_zombie`(record를 영원히 쓰고 정리하지 않음), `service_task_absent`(core task가 끝났는데 시스템은 계속 돎), `processor_reset_requested`, `process_abort`(pc-linux에서 reset 요청은 `CFE_PSP_RESTART_DELAY` 뒤 abort한다), `panic`(core 동기화 실패), `task_never_runs`, `hang_or_deadlock`, `terminal_pending(panic | abort)`, `message_lost`, `unspecified_by_reference`다(spec §1.6).

`task_never_runs`와 `hang_or_deadlock` 두 값은 두 가지로 쓰이고, 기록은 둘을 나눈다 [D] (EXT-20261012 §4.1).
- **VIOLATING 발생의 harm.** caller 수준 Allowed가 지정된 곳, 곧 `caller` record를 가진 항목이나 지정된 `appreq`가 발생을 정하는 곳에서 Allowed 분석이 정한다. 엄격한 대안(QT-4, Q15) 아래에서 TIME의 `CFE_ES_CreateChildTask` 호출(`cfe_time_task.c:160, 173`; spec §3.3, App. A.2 item 4)의 K-SELF.es는 그런 곳이 아니다. 그 `returned_and_tested` 항목은 연속을 인용하지 않으므로 발생은 NOT_READY_UNJUDGED이고 harm은 `unspecified_by_reference`다. 돌려받은 error가 호출자에게 돌아가고 그 뒤 자식 task가 결코 돌지 않는다는 code 사실 [I]은 그 발생의 consequence로 기록한다. 이런 harm을 가진 VIOLATING 발생에는 `caller` record나 지정된 `appreq`가 필요하다(litmus LP-ALLOW).
- **progress 결과의 harm**(§7.3). 사용으로 가는 길에서, 사용 안에서, 또는 사용의 horizon을 열어 둔 채 죽은 actor다. 기다리는 instance의 progress 결과다.
- `terminal_pending(panic | abort)`는 progress harm으로만 쓴다. 기다리는 actor는 죽었지만, stop 입력이 일으키지 않는 process 끝이 모든 상태에서 도달 가능하다. 그래서 process는 멈춰 있지 않고 panic(`panic`)이나, 다른 abort·재시작 요청·`main`의 반환(`abort`)으로 끝난다. 두 값은 spec §3.3의 `panic`과 `process_abort` 구분을 지킨다.

caller 수준 규칙(§5.8: `Allowed = none`에는 지정된 `appreq`가 필요하다) 아래에서, `none`이 supplier 쪽 글에만 기대는 K-SPAWN.osal과 K-LOCKBAL은 NOT_READY_UNJUDGED다. harm은 `unspecified_by_reference`이고, 효과는 consequence로 기록한다. PI는 2026-10-15에 이들을 구현 결함 계열(implementation-defect family)로 정했다(QT-9). 이들을 준비 상태 위반(호출자 요구 위반)으로 만들려고 연구자가 작성한 `appreq`를 더하지 않고, 순서 위반과 이어지는 부분만 자기 근거로 보고한다.

### 5.4 scope: 종료 요청 전까지

모든 준비 상태 명세에는 암묵적 scope conjunct **"발생의 결정 event보다 앞선 `es.shutdown`의 SHUTDOWN_REQ LP가 없다"**가 붙는다 [D] (spec §1.6; [X1-M6], [X2-04] 수정, EXT-20261012 §4.1이 다시 씀).
- 발생의 준비 여부가 결정될 때(§5.5) 함께 정한다. 결정 뒤의 conjunct LP는 상관없다. conjunct가 하나면 "사용의 LP보다 앞선"과 같다.
- scope는 instance의 일부다: `i = (κ, t, key, scope)`, `scope ∈ {default, teardown}`.
- 기본 scope 밖 발생은 별도 family **K-TEARDOWN**(기본 scope의 여집합)의 instance다. 같은 status로 분류하고 따로 보고한다. 그 progress는 `unknown(teardown_scope)`다(§7.3).
- SHUTDOWN_REQ LP 뒤의 상태를 빼는 규칙(§7.3)은 기본 scope instance에만 적용한다.

실험의 stop 방법이 ES POWERON 재시작 명령이므로, 관측한 모든 실행은 teardown으로 끝나고 그 구간의 사용은 K-TEARDOWN instance다(spec §3.5). teardown이 시작될 때 PENDING인 기본 scope 발생은 PENDING으로 남고, 그 실행은 `unknown(horizon)`을 준다(§5.2 horizon monitor; spec §1.6).

### 5.5 Instance와 발생별 결과

**instance**는 `i = (κ, t, key, scope)`다. `t`는 `U`에 맞는 문맥 구분 template이다. status는 **유한 실행** ρ(§7.1) 위에서 정의한다 [D] (spec §1.6; EXT-20261012 §4.1). actor instance `a`가 실행하는 `t`의 발생 `u`를 보자.

- **시작.** `u`는 **호출에서 시작한다**. `a`가 t의 호출 노드를 실행하는 때다. registry conjunct의 LP는 호출 범위 안의 읽기다. value conjunct의 LP는 capture point다. 호출 전에 `a`가 마지막으로 실행한 capture site 가운데 `derives` 사실이 그 호출의 인자와 잇는 것이고, 없으면 호출 자체다(§3.3). capture는 실행될 때 평가하고, 그것이 값을 넘기는 호출에 붙인다. 뒤따르는 호출이 없는 capture는 아무것도 시작하지 않는다.
- **결정.** `u`는 다음 중 가장 이른 때에 결정된다: 붙은 capture가 거짓이면 호출; 그 뒤 conjunct가 거짓인 첫 conjunct LP; 모든 conjunct가 성립하면 마지막 conjunct LP; 호출의 반환.
- 호출이 모든 conjunct LP를 지나지 않고 돌아오면(예: 이름 검색 전 인자 검사) `u`의 준비 여부는 평가되지 않는다. status는 `NOT_EVALUATED`다.
- **결정 event**는 호출, 결정한 conjunct LP, 또는 호출이 먼저 돌아온 경우 마지막으로 지난 conjunct LP(없으면 호출)다. 호출과 결정 사이에서 `u`는 **열려**(open) 있다. 모든 발생은 늦어도 호출의 반환에서 결정되므로, actor 하나에 `t`의 열린 발생은 많아야 하나다.
- **ordinal.** `u`의 ordinal은 `(a, k)`다. ρ에서 `a`가 시작한, key가 `key`인 `t`의 k번째 발생이라는 뜻이다. `a`는 실행 사이에서 `(τ, generation, n)`으로 부른다(§1.1).
- **소속.** ρ에서 결정 event보다 앞선 SHUTDOWN_REQ LP가 없으면 `u`는 instance `(κ, t, key, default)`에 속하고, 있으면 K-TEARDOWN instance `(κ, t, key, teardown)`에 속한다. "i에 대해 범위 안"은 "i에 속한다"는 뜻이다. 결정 뒤의 conjunct LP는 scope에 영향이 없다.
- **process 종료 뒤.** 결정 event가 process 종료 단계(§7.3) 뒤에 오는 발생은 status가 없고 어느 instance에도 속하지 않는다.
- conjunct `P_j`에 대해 `state_ρ(r, P_j, u)`는 ρ에서 `u`의 `P_j` LP보다 앞선 LP를 가진 같은 generation의 lifecycle effect를 모두 적용한 뒤의 `A_k` 상태다.

i에 속하고 ρ에서 결정된 발생의 status는 다음 여섯 중 하나다. 위에서부터 차례로 본다. 아래 셋은 Allowed의 horizon monitor μ(§5.2)로 정한다.

| status | 조건 |
|---|---|
| `NOT_EVALUATED` | 호출이 모든 conjunct LP를 지나기 전에 돌아왔다. ready도 not ready도 아니다. witness가 아니고, `retry_until_ready`의 ACCEPT도 아니며, §11의 어떤 진술도 세지 않는다 |
| `READY` | 모든 conjunct가 자기 LP에서 성립한다 |
| `TOLERATED` | ready가 아니고, μ가 ρ에서 ACCEPT다 |
| `VIOLATING` | ready가 아니고, μ가 ρ에서 REJECT이거나 `Allowed = none`이다 |
| `PENDING` | ready가 아니고, μ가 ρ에서 OPEN이다 |
| `NOT_READY_UNJUDGED` | ready가 아니고, `Allowed = unspecified_by_reference`다 |

- `READY`, `TOLERATED`, `VIOLATING`, `NOT_READY_UNJUDGED`, `NOT_EVALUATED`는 최종이다. ρ의 모든 연장에서 그대로다(L-STATUS, §12.3).
- `PENDING`은 연장에서 `TOLERATED`나 `VIOLATING`이 될 수도 있고, 모든 연장에서 `PENDING`으로 남을 수도 있다. **`PENDING` 발생은 결코 `TOLERATED`로 세지 않는다.**

유한 실행이 `i`에 대해 **bad**라는 것은 범위 안 `VIOLATING` 발생을 담거나, Allowed가 `unspecified_by_reference`일 때 범위 안 `NOT_READY_UNJUDGED` 발생을 담는다는 뜻이다. bad 유한 실행의 모든 연장은 bad다. 유한 실행이 `i`의 **good witness**라는 것은 범위 안 `READY`나 `TOLERATED` 발생을 담는다는 뜻이다. witness는 그 발생의 ordinal을 기록한다. 어느 뜻의 bad인지는 결과 label이 말한다. 사용 전, 사용 안, horizon 안의 영원한 대기는 결코 준비 상태 위반이 아니다. 그것은 **progress** 결과다(§7.3).

<a id="56-위반-사슬"></a>

### 5.6 준비 상태 위반 사슬

`i₁ ⇝ i₂`(준비 상태 위반 사슬)는 `u₂`의 발생이 `u₁`의 not-ready 결과 **때문에만** 존재할 때다(제어나 데이터 의존. trace에서는 frame 포함 관계와 Φ `emits` 주석). 사슬은 결과의 갈래를 **모두** 기록한다([X2-02] 수정). 예: R2의 identity 사용 24개 중 17개는 ES 문맥의 `SEND_NO_SUBS` event의 사용이다 [M] ([R2g §2]). 이 17건이 어떤 구독보다 먼저 송신된 ES의 EVS event packet(MsgId 0x808)에서 나왔다는 귀속은 기록의 추론이다 [I] ([R2g §2]는 이 귀속을 event ID와 뒤의 콘솔 줄 "No subscribers for MsgId 0x808"에서 끌어낸 추론으로 적는다. spec §1.6도 사용 17개는 [F], 귀속은 [I]로 적는다). 사슬의 뿌리는 route가 없는 송신(K-ROUTE instance)이고, 그 송신 안에서 나온 `SEND_NO_SUBS` 사용(R2의 K-ID instance)이 사슬로 이어진다(K-ROUTE ⇝ K-ID; spec §4.8 step 8, App. A.1 step 13).

**K-ROUTE 쪽의 발생** ([I], spec App. A.1 step 13). ES의 version-event packet을 보내는 K-ROUTE template은 boot마다 ordinal이 여럿이므로 ordinal마다 분류한다.
- 소비자 TO_LAB의 telemetry pipe는 연기가 문서화되어 있다(`defer_by_design`). 감시 actor는 TO_LAB의 main task이고, ES가 송신할 때는 아직 없다. 그래서 각 발생은 송신에서 PENDING이고, TO_LAB이 구독하면 TOLERATED다. TO_LAB이 먼저 끝나거나 더는 구독할 수 없게 되면 같은 ordinal에서 VIOLATING이다. 예: TO_LAB 자신의 App-record 구간이 `TO_LAB_init`의 `CFE_EVS_Register`를 실패하게 하는 경우(`to_lab_app.c:134`, spec App. A.3). 그래서 이 K-ROUTE instance는 schedule-dependent이고, 그 구간의 발생에 사슬로 붙는다.
- 연기 문서가 없는 소비자의 K-ROUTE instance는 Allowed가 `unspecified_by_reference`이고 label은 LOSS-UNDER-ORDER다. route가 없는 이른 발생과 route가 있는 뒤 발생은 같은 ordinal 쌍을 보이지 않는 한 `ordinal-split`만 준다(§11.1).
- panic은 이 발생들 중 어느 것도 결정하지 않는다(§5.2).

**progress 사슬** [D] (spec §1.6, §1.8; EXT-20261012 §4.1). 준비 상태 위반 사슬은 발생을 잇는다. 그런데 stall 상태(§7.3)에는 기다리는 사용의 발생이 없다. 그래서 둘째 사슬이 progress 결과를 잇는다. `i₁ ⇝_p i₂`는 `i₂`의 어떤 stall 상태에 wait-for root r이 있어 다음 중 하나가 성립할 때다.
- (P1) r의 actor가 `u₁`의 열린 발생 안에 있거나, `u₁`의 `PENDING` 발생의 감시 actor다.
- (P2) r의 대기를 아직 채울 수 있는 모든 event(그 predicate의 지원 위치에 쓰기; 그 object의 release, give, put, send; 그 actor의 spawn)를 certificate view에서 `u₁` 호출의 READY 결과가 지배하고, 그 유한 실행에 그 경로 위에서 not ready로 결정된 `u₁` 발생이 있다. 예: core App-record 구간 안에서 건너뛴 K-SELF.es raise.
- (P3) r의 object가 not ready로 결정된 `u₁` 발생에서 잡은 lock이다(K-LOCKBAL). 또는 r이 `u₁` 발생의 not-ready 갈래에서 끝난 actor를 기다린다(K-SPAWN).

- 조건은 모두 stall 상태, 그 유한 실행, certificate view에서 읽으므로 결정 가능하다.
- 뿌리 instance를 보고하고, 딸린 progress 결과는 거기에 붙인다. 준비 상태 위반 사슬과 같은 방식이다(spec App. A.2 item 4).
- wait-for root가 not-ready 결과와 무관한 stall(predicate 대기의 순환, 그냥 결코 주지 않는 provider)은 사슬 뿌리가 없다. wait-for root 집합으로만 묶는다. 사슬에 없는 instance가 timeout-masked-liveness를 가지면 스스로 `tml-root`를 진다(§7.4).
- progress 사슬은 칸마다(`kz_verdict_cell.progress_chain`) 준비 상태 위반 사슬과 따로 저장한다.

<a id="57-r2의-계약"></a>

### 5.7 R2의 준비 상태 명세

(spec App. A.1 steps 1–2, 8)

- 준비 상태 명세는 generic `K-ID.send-with-appid`(출처 `cfe_evs.h:168–172`, 근거 등급 `contract-backed`)이고, 모든 문맥별 `CFE_EVS_SendEventWithAppID` site에 instance를 만든다. SB site 39개는 `CFE_SB_Global.AppId`를 `stamp`로 넘긴다([R2g §6]).
- 준비 사전조건 `Req = valid(arg.AppID) ∧ state(evs.reg(arg.AppID)) = REG`. value conjunct `V`는 capture point `c`에서 `cell(CFE_SB_Global.AppId)` 위로 평가한다. registry conjunct는 여기서 역할이 없다. EVS가 index 유효성을 먼저 검사하고(`cfe_evs.c:185–189`) `V`가 이미 실패하기 때문이다.
- 세 판단(§5.8)으로 나누면 다음과 같다.
  - caller 수준: `appreq` = `cfe_evs.h:169–172`(caller 쪽), `Allowed = none`(header가 대안으로 든 system log는 다른 API다), harm `event_request_rejected`.
  - framework 수준: `CFE_EVS_APP_ILLEGAL_APP_ID`는 `\retval`로 문서화되어 있고(`cfe_evs.h:199`), error severity이며(`cfe_error.h:115–119, 267`), counter도 event도 없다(`cfe_evs.c:186–189`). 그래서 `fw_doc` = {`documented-result`}다. `fw_conf`는 실행이 뒷받침하는 칸에서 `as-documented`이고(준비 상태 위반 사용마다 그 결과가 뒤따른다, [R2g §7]), M_api에서는 `unknown`이다.
  - 관측된 결과: SB 호출자는 모두 `ILLEGAL_APP_ID`를 버린다. consequence는 "status ignored"와 "event rejected"다. EVS는 요청을 패킷을 만들기 전에 거절하므로(`cfe_evs.c:185–189`) 잃어버린 패킷이 없다. 표현은 "이벤트 요청 거부"이고 "이벤트 손실"이나 "패킷 손실"이 아니다(spec §6.4 항목 22, 47).

### 5.8 세 판단: framework 수준, caller 수준, 관측된 결과

not-ready 사용은 세 번 판단하고, 기록은 셋을 따로 둔다 [D] (spec §1.6 "Three judgements"; EXT-20261012 §6.4; ADR-043).

| 판단 | 무엇을 보나 | 근거와 검사 | 기록과 쓰임 |
|---|---|---|---|
| **framework 수준** (API 행동) | 자원이 준비되지 않았을 때 supplier가 무엇을 돌려주고 무엇을 하나 | op의 outcome map과 문서화된 결과(§3.1). O4(a), `kz conform`, spec §3.9의 counter oracle, trace의 event 시도로 검사한다 | `fw_doc`(instance마다), `fw_conf`(칸마다)(§11.5). 발생을 결코 TOLERATED로 만들지 않는다 |
| **caller 수준** (앱·mission 요구) | 사용하는 코드가 무엇을 이뤄야 하고, 어떤 not-ready 결과를 받아들이나 | `appreq`와 아래 규칙으로 읽는 Allowed 항목 | 이것만 §5.5의 status, §11의 class, 결과 label, `harm`을 정한다 |
| **관측된 결과** | 무엇이 일어났나(실패, 손실, 종료) | trace나 모형 연속, 관측 count | `consequence`. 다른 두 판단과 상관없이 기록한다 |

**출처 record의 side.** 모든 `prov` record에 `side ∈ {supplier, caller, rule}`를 붙인다. 작성자가 (record, κ) 쌍마다 판단해 기록한다.
- `supplier`: 자원이 준비되지 않았을 때 supplier가 돌려주거나 하는 것, 또는 framework가 그 결과를 error로 보는지를 말한다. 예: `\retval`·`@retval` 줄; EVS나 SB를 부르는 앱의 사용에 대한 cEVS3103.1과 cSB4305.3–.5; 문서화된 전파(`cfe_evs.h:98`); barrier의 문서화된 timeout; supplier 자기 코드 위의 `code_pattern` record(spec §2.3의 K-SELF.es 출처).
- `caller`: 사용하는 코드가 무엇을 이뤄야 하는지, 또는 not-ready 결과를 어떻게 다루고 그것이 받아들일 만하다는 것을 말하는 글이다. 사용하는 코드는 호출자이고, route·용량 준비 상태 명세이면 소비자, 또는 그것을 대신해 도는 앱이나 mission이다. 예: `cfe_evs.h:169–172`; CF의 인정 `cf_cfdp.c:1272–1278`; TO_LAB의 연기 `to_lab_app.c:64–68`; 사용을 가진 모듈의 guard 주석; 앱이나 mission의 요구.
- `rule`: 글이 없다. Kairos의 고정 규칙과 그것을 적용한 코드를 가리키고, 받아들인다는 말은 하지 않는다. 예: `returned_and_tested`를 위한 `cont_candidate`의 추출; spec §7.1 source 4의 loop 인식. **`rule` record는 status를 정하지 않는다**(아래 caller 수준 Allowed). 주석이 밝히지 않는 사용 코드의 drop에는 `rule` record가 없다. Kairos의 어떤 고정 규칙도 drop을 받아들인 것으로 읽지 않는다(§5.2 `documented_drop` 행).
- side는 (record, κ) 쌍에 속한다. cSB4305.5는 K-ROUTE(사용자는 송신하는 앱)에는 `supplier`이고, SB의 `SEND_NO_SUBS` event site의 K-ID instance(사용자는 SB 자신의 event 코드)에는 `caller`다.

**caller 수준 Allowed** [D] (spec §1.6; EXT-20261012 §6.4; 엄격한 대안 QT-4, 2026-10-15 결정).
- Allowed 항목은 `caller` record가 하나라도 있을 때에만 horizon monitor로 읽는다.
- record가 모두 `supplier`인 항목, 모두 `rule`인 항목, 둘이 섞인 항목은 status를 정하지 않는다. caller 수준에서 그 항목은 `unspecified_by_reference`이고 not-ready 발생은 NOT_READY_UNJUDGED다. supplier-only 항목은 framework 수준 정보로 남는다.
- **규칙만으로는 어떤 발생도 받아들여지지 않는다.** 원출처 근거가 없는 항목은 미판정으로 남는다. 이전 기본값이 만들던 label(규칙이 받아들인 TOLERATED, 규칙의 REJECT에 기댄 VIOLATING)이 없어질 뿐, 관측은 지우지 않는다. 모든 not-ready 발생의 `consequence`는 code 사실(상태 변화, 돌려받은 code, error 반환과 exit, 추출한 `cont_candidate`)과 관측 count를 계속 지니고(§11.5), framework 수준 결과는 `fw_doc`과 `fw_conf`로 남는다.
- 이 문서의 status, class, 결과 label에서 "Allowed"는 caller 수준 Allowed를 뜻한다.

**`appreq`와 caller 수준 Allowed를 정하는 순서** [D] (spec §1.6).
1. `caller`나 `rule` 항목이 없으면, `appreq`가 지정되어 있을 때 `none`이고 아니면 `unspecified_by_reference`다. `Allowed = none`에는 지정된 `appreq`가 필요하다.
2. `rule`-only 항목과 `supplier`-only 항목은 caller 수준에 들지 않는다(QT-4).
3. 지정된 `appreq`가 배제하는 결과를 받아들이는 `caller` 항목, 또는 양립할 수 없는 연속을 받아들이는 `caller` 항목 둘은 두 reference와 함께 **충돌**로 기록한다. **충돌을 밝히고 판정은 보류한다**(QT-5, 2026-10-15 결정). 두 문서의 버전, 대상, 적용 범위를 먼저 확인하고, 그것으로 충돌이 풀리지 않는 동안 우선순위를 만들지 않는다. 충돌하는 항목은 caller 수준 Allowed를 `unspecified_by_reference`로 만든다. 그 발생은 충돌을 적은 NOT_READY_UNJUDGED이고 결코 TOLERATED가 아니다. 결과 label은 `not-ready-unjudged`다. 충돌에 must-deliver `appreq`가 끼어도 LOSS-UNDER-ORDER가 아니다.
4. 그 밖에는 항목이 정한다.

항목과 `appreq`가 충돌하는지, `appreq`가 규칙이 포기하는 것을 요구하는지는 작성자의 판단이고, 두 reference와 함께 기록한다. F1 뒤에 `appreq`나 기록된 충돌을 바꾸면 `oracle refinement`다(spec §7.2).

**예** ([I], spec §1.10, App. A.1 steps 8, 13a).
- R2: §5.7. caller 수준의 `violation`이 `as-documented`로 일어난 `documented-result` 위에 선다.
- SB 구독 전 송신(ES의 event packet 17건): `fw_doc` = {`documented-not-error`}(cSB4305.5의 근거 문장). 이 구간의 칸과 실행에서 `fw_conf` = `deviates`다. cSB4305.5의 shall 문장이 요구하는 `SEND_NO_SUBS` event를 EVS가 `ILLEGAL_APP_ID`로 거절하기 때문이다([M] ES 문맥에서 17개, [R2g §2]). caller 수준은 TO_LAB의 `defer_by_design`이거나, 소비자 reference가 말하지 않는 곳에서 LOSS-UNDER-ORDER다. 관측: "message lost"와 "event rejected". 이 구간에서 손실은 구간 뒤에 남는 흔적을 남기지 않는다 [I].
- `documented-not-error` 결과도 caller 수준에서 LOSS-UNDER-ORDER나 `violation`을 지닐 수 있다.

이 설계의 바탕인 PI 항목은 2026-10-15에 결정되었다 [D] (spec §0.10, ADR-043 개정, ADR-048). QT-2와 QT-3은 승인이다: 근거 종류 `requirement`를 더하고, 구독자 없는 송신에 대한 SB 규정과 R2의 유효하지 않은 AppId 오류를 섞지 않으며, 프레임워크 문서·실제 API 동작, 호출자 요구, 관측된 결과의 세 층을 분리한다. QT-4와 Q15는 엄격한 대안, QT-5는 충돌을 밝히고 판정 보류, QT-9는 구현 결함 계열로 분리다.

### 5.9 이름과 근거 등급

(spec §1.6 "Naming and source tiers"; spec 부록 C; ADR-052; `background/08_terminology.md` §5.1–§5.5)

**이름** [D] (PI 결정 T1 = (a), 2026-10-15).
- 판정 대상 κ는 **준비 상태 명세**(readiness specification)다. 논문의 첫 사용에서는 "근거 기록 준비 상태 명세"(provenance-tracked readiness specification)로 쓴다. 한국어로는 준비 상태 명세를 온전히 쓰고 '명세'만 따로 쓰지 않는다. 영어로도 bare word "specification"을 κ에 쓰지 않는다. 이 문서 이름이기 때문이다.
- `Req`는 κ의 **준비 사전조건**(readiness precondition)이다. 키로 고른 자원에 대한 lifecycle 또는 값 요구이고, 누가 그것을 약속했다고는 말하지 않는다.
- 범위 안 발생이 준비 사전조건을 만족하지 못하고 caller 수준 Allowed가 그 결과를 받아들이지 않으면(`VIOLATING`) **준비 상태 위반**(readiness violation)이다. 상태를 만들 provider가 있는 곳에서 이것은 순서 위반(order violation)의 하위 부류다. "계약 위반"이라 부르지 않는다. caller 수준 Allowed가 `unspecified_by_reference`인 발생은 `NOT_READY_UNJUDGED`(route·용량 준비 상태 명세는 LOSS-UNDER-ORDER)이고 위반이라 부르지 않는다.
- **계약**(contract)이라는 말은 문서화된 API 의미에만 쓴다. M_api의 OSAL API 계약(§3.5)과 아래 근거 등급 `contract-backed`다. Kairos가 쓰는 어떤 대상도 계약이라 부르지 않는다.
- class refinement였던 `tolerated`는 `allowed-not-ready`(허용된 미준비 결과)다. 산문에서는 "허용된 미준비 결과"라고 쓰고 "Kairos가 tolerate한다"고 쓰지 않는다. 발생의 status 값 `TOLERATED`는 이번 개정에서 이름을 바꾸지 않았다.
- 식별자의 이름은 한 번에 정했고 목록은 spec 부록 C에 있다. 바뀐 것은 이름뿐이다. 정의, lemma, 보장, 한계값, 숫자는 바뀌지 않았다 [I] (ADR-052). 한국어 표현(준비 상태 명세, 준비 사전조건, 준비 상태 위반, 근거 등급 등)은 한국어 용어 출처로 확인하지 않았다 [U] (`background/08_terminology.md` §5.1).

**근거 등급**(source tier). 어떤 종류의 근거가 instance 뒤에 서 있는지 말한다. κ와 caller 수준 Allowed의 `prov` record에서 보고할 때 계산한다. 추가로 작성하지 않으며, §2.3의 record에 필드를 더하지 않는다. `kz report`가 판정이 가리키는 Φ 항목에서 읽는다. 섞인 항목은 아래에 적은 순서에서 가장 약한 등급을 받는다. 등급은 `background/08_terminology.md` §5.4에서 가져왔고, `code-derived`와 `unreferenced`의 정의는 QT-4 엄격한 대안에 맞춘 것이다 [I].

| 등급 | 뜻 |
|---|---|
| `contract-backed` | caller 수준 부분이 호출자에 대한 문서화된 API 의무에 기댄다(`api_doc`, side `caller`. 글이 의무를 말하는지는 작성자의 판단이고 기록한다). supplier가 error 반환도 문서화했는지는 상관없다. 예: R2의 K-ID(`cfe_evs.h:169–172`) |
| `requirement-derived` | caller 수준의 어떤 부분이 cFS 기능 요구(kind `requirement`, QT-2)에 기대고, 더 약한 것에는 기대지 않는다 |
| `admitted` | 어떤 부분이 `code_admission`에 기댄다. 예: CF의 주석 `cf_cfdp.c:1272–1278` |
| `issue-derived` | 어떤 부분이 `issue`에 기댄다 |
| `code-derived` | 명세가 프로그램 본문에서 고정 규칙으로 만들어졌고(side `rule`인 `code_pattern` record: spec §7.1 source 4의 loop 인식, `cont_candidate`의 추출) caller 수준 부분을 대변하는 `caller` record도 지정된 `appreq`도 없다. `rule` record는 status를 정하지 않으므로(QT-4) caller 수준 Allowed는 `unspecified_by_reference`이고 발생은 NOT_READY_UNJUDGED다 |
| `researcher` | caller 수준의 어떤 부분이 `researcher` record에 기댄다 |
| `unreferenced` | caller 수준 부분을 대변하는 record가 없다. 곧 caller 수준 Allowed가 `unspecified_by_reference`이고 명세를 만든 `rule` record도 없다 |

"reference에서 온 것"(reference-derived)이라는 말은 앞의 네 등급에만 쓴다. `researcher` 항목은 reference가 아니고, `code_pattern` 항목은 분석하는 프로그램에서 규칙이 만든 것이다. 등급의 순서는 `background/08_terminology.md` §5.4를 따른다. 그 순서가 옳은지는 08에서 열려 있고(08 §7 Q4) 여기서 정하지 않았다. **보고 규칙**: census 개수, 판정, 확장 평가의 recall은 근거 등급별로 보고한다. caller 수준 Allowed가 지정된 모든 bad 발생은 등급과 함께 준비 상태 위반으로 보고한다. "계약 위반"은 쓰지 않는다.

### 5.10 지원하는 순서 조건 (rev 1.3)

[D] (spec §1.6 "Supported conditions"; [P0.2] §6.4; C-07). 준비 상태 명세는 상태, 수명, 값에 기대는 순서 조건을 나타내는 한 표현이다. 연구 대상은 process-level 순서 위반이지, 이 표현이 덮는 조건의 부류가 아니다. profile 언어와 evaluator가 표현하지 못하는 순서 조건은 이유와 함께 `not supported`로 기록하고, 확장이 필요한지는 그 조건에서 확인한다. 고정된 두 event 모형을 모든 사례의 판정 규칙으로 들이지 않는다. 식별자 `rspec`, `Req`, `Allowed`는 그대로 둔다(T1). spec 부록 C의 rev 1.3 용어 대응에서 [P0.2]의 "순서 조건 φ"는 준비 상태 명세 κ(`rspec`)이거나 `not supported`이고, 새 식별자는 없다.

### 5.11 요구를 붙이는 호출 문맥 (rev 1.3 보충, 2026-10-24)

[D] (spec §1.6 "Call contexts"; 2026-10-24 마스터플랜 §5.1). `U`의 site 술어를 어떻게 고르는지를 정한다. 위의 정의는 바뀌지 않는다.

- 준비 상태 명세는 출처가 정당화한 **역할**(호출 목적과 그 호출에 대한 요구. 예: 자원이 준비돼 있어야 하는 의존적 사용)을 가진 호출 문맥에만 붙는다. 출처는 문서, 호출자 코드, 이슈, 수정 설명이며 provenance와 side를 함께 기록한다.
- 소스 위치(고정 판의 `path:line`, basis마다 anchor audit가 다시 묶음)는 그 역할을 바이너리의 호출 지점에 묶는 근거일 뿐이다. 위치만으로 어떤 호출이 준비 사전조건을 지닌 사용이 되지 않는다.
- 같은 연산이라도 출처가 보이는 역할이 다르면 κ의 instance가 아니다. 그 호출은 역할과 출처로 선언하고, 그 event는 사실로 계속 기록한다.
- 사례 결과를 맞추려고 호출 문맥을 빼는 규칙(앱 이름으로, 역할 없는 호출 지점으로, 결과를 본 뒤에)은 금지한다.
- API의 모든 호출자에게 문서화된 의무에서 온 역할(K-ID, `cfe_evs.h:169–172`)은 그 API의 모든 호출 문맥을 덮는다.
- 예(C1; [../plan/C1_basis_plan.md](../plan/C1_basis_plan.md) §9.7): CF의 throttle semaphore 조회는 의존적 사용이다(CF 문서, 이슈 #184). nasa/bp가 자기 생성 전에 하는 조회는 존재 확인이다. "없음" 결과가 같은 이름을 만드는 조건이기 때문이다(`bp_flow.c` L121–L144 @`b051eb0`). 그래서 BP의 조회에는 준비 사전조건이 붙지 않는다.

---

## 6. 분석 기준 B

`B = ⟨Build, Config, Init, Env, Input⟩`는 스케줄을 뺀, 실행을 정하는 모든 것이다. 각 항목은 값, 출처, 정책 상태와 함께 기록한다(spec §1.7).

| 항목 | 내용과 cFS 예 |
|---|---|
| **Build** | commit, gcc 13.3.0과 flag, `link.txt` archive 의미, 생성 파일 hash. `cfe_static_module_list.c`가 core 순서를 정하고, 생성 module version table도 hash로 기록한다. R2의 boot당 개수가 그 table에 기대는지는 table 없는 basis에서 Kairos 자신의 기록기가 확인할 구성 문제다 [I]. gdb로 본 24개 중 13개는 설정된 module을 훑는 version event callback에서 나온다([M], [R2g §6]). 그 훑기가 table의 내용을 따른다는 것은 코드 읽기 [I]이고, table 없는 basis를 다룬 gdb 기록은 없다 |
| **Config** | startup script 선택 규칙(PROCESSOR reset이면 휘발성 `/ram/cfe_es_startup.scr` 먼저, POWERON이면 생성 script, `cfe_es_apps.c:130–168`), `.tbl`과 소스 초기값의 byte 대조, platform 상수, PSP module 목록 |
| **Init** | 실행 전 유지 상태(boot record, CDS·reset area SysV segment, `/dev/shm/osal:RAM`, `EEPROM.DAT`, 해독한 CDS와 critical table image). 실행마다 Init snapshot S0를 뜬다(구현 단계 S0와 다른 뜻). hash는 내용만 본다(`EEPROM.DAT` mtime은 boot마다 바뀜, [BM]) |
| **Env** | kernel, 4 vCPU, 스케줄링([M] 27개 thread 모두 `SCHED_OTHER`, [BM §0]), affinity, euid(queue 잘림을 정함), CWD, 실효 queue 깊이, platform 기능(`/proc/schedstat` 없음), cold/warm 층([M] OPERATIONAL까지 2570 ms 대 1613–1633 ms, [BM]) |
| **Input** | event 기반 trigger를 단 외부 명령, reset 종류 입력(`-R` 또는 boot record의 `NextResetType`), stop 방법(PI 결정 D17) |

**INIT** [D] (spec §1.7; [X1-m1], [X2-12] 수정)은 **프로그램 진입 상태**다. process scope 위치는 정적 초기값 image, persistent scope 위치는 해독한 S0다. EarlyInit의 store(예: `memset(&CFE_SB_Global, 0, …)` `cfe_sb_init.c:50`)는 INIT이 아니라 평범한 writer 항목이다. R2에서 `CFE_SB_Global`은 0으로 초기화되는 전역이므로(`cfe_sb_task.c:40`) INIT에서 `AppId = 0 = UNDEFINED`이고, value conjunct `V`는 INIT에서 거짓이다(`INIT_FALSE(V)`).

**실행 열과 generation.** 실행 사이 준비 상태 명세(K-PERSIST)에서는 실행 *n*의 끝 상태가 `B_{n+1}.Init`이 된다. 한 실행 안의 재시작·재적재는 generation을 만든다(`B_restart`). `CFE_PSP_Restart(POWERON)`로 끝나지 않은 실행 뒤에는 관측하지 않는 normalization boot를 한 번 해서 Init을 정규 S0로 되돌린다(spec §1.7, §4.13).

**구성 변형은 새 basis B′이고, 결코 "새 버그"가 아니다**([R06 §5.4]). 모든 판정은 B의 hash를 지닌다.

**관측한 실행의 basis 규칙** [D] (rev 1.3; spec §1.7; [P0.2] §4.2; C-08).
- **IN-01.** 모든 결과는 쓴 build, 구성, profile, 관측 기록, 도구 version까지 거슬러 올라갈 수 있다.
- **IN-02.** closure가 core-cpu1뿐인 분석은 애플리케이션 코드의 분석으로 보고하지 않는다. 빠진 애플리케이션, 동적 호출, library 효과를 나열한다.
- **IN-03.** 시작 상태는 초기화 코드, 프로그램 진입(INIT), 또는 일관성을 보인 상태 capture로 정한다. **"관측된 등록 event가 없다"를 결코 "처음부터 등록되지 않았다"로 읽지 않는다.**
- **IN-04.** 재시작, reset, ID 재사용이 일어나면 분석 구간과 자원 수명을 나눈다. 추적하지 못한 수명 경계는 미확정으로 남는다.
- **IN-05.** 순서에 영향을 주는 scheduler, 우선순위, CPU, 메모리 가정을 밝힌다. 순차 일관성 모형은 실제 build에 대한 가정으로 표시한다(A4, §12.2).

---

## 7. 스케줄·시간 의미 σ와 containment

### 7.1 실행 집합

**전이 체계** [D] (spec §1.8; [X1-C1] 수정, EXT-20261012 §4.2가 다시 씀). σ ∈ {σ_U, σ_U∞}와 수준 M마다 전이 체계 `TS(B,σ,M) = (Q, q₀, T, en_σ, δ)`를 둔다.
- 상태와 전이는 explorer의 것(spec §4.10 item 2)과 같다. 전이는 LP 단위의 effect와 명시적 local choice다. horizon 없이 **certificate view**(§8) 위에서 만든다.
- `en_σ(q)`는 상태 q에서 σ가 허락하는 전이의 집합이다. σ_U와 σ_U∞는 Q, q₀, T, δ를 같이 쓰고 `en`만 다르다.
- Φ delay 연산은 그 actor의 제어 상태만 바꾸는 전이다(σ_U의 양보).
- **ES kill timer**는 추상화한다. 쓰이는 외부 앱 중 `AppState > RUNNING`인 것을 scan할 때마다(`cfe_es_apps.c:1114`) `ES_BG_TASK`는 timer가 아직 만료되지 않았다고 보거나(`not-yet-expired`) 만료시킨다(`:1126–1136`). 만료는 결과 label이 붙은 두 전이다. 같은 scan에서 읽은 `AppState`가 STOPPED면(앱의 `CFE_ES_ExitApp`이 저장, `cfe_es_api.c:399`) `expire-stopped`, WAITING이면(명령을 받았지만 끝나지 않은 앱, `:1139–1146`) `expire-waiting`이다. 명령이 바꾸지 않는 한 앱은 생성 때부터 timer 0을 갖고(`:819`), timer 0은 scan에서 늘 만료된다.
- **σ_R1[P_sched]은 별도 체계** `TS(B,σ_R1,M)`다. 상태에 이산 시계와 actor별 ready/blocked가 더해진다. delay 연산의 전이는 actor의 block 단계이고, σ_U의 delay 전이처럼 제어를 옮긴다. 시계 진행 단계와 깨움 단계는 더한 성분만 바꾼다. 시계는 ready인 actor가 없을 때만 가장 이른 깨움 시각으로 간다. timeout은 σ_U처럼 허락된다. 투영 π는 시계 진행·깨움 단계를 지우고 더한 성분을 잊는다(L-CONT).
- 분석은 status와 stall 판정을 `TS`와 **instance monitor**의 곱(product) 위에서 계산한다. monitor는 결정적이고 어떤 전이도 막지 않는다. actor마다 열린 발생, capture site별 마지막 capture 값, horizon monitor 상태를 가진 PENDING 발생, key별 발생 ordinal을 기록하고, SHUTDOWN_REQ LP가 일어났는지도 기록한다(L-MON, §12.3).
- explorer의 bounded 모형은 exploration view를 쓰고, 그 결과는 b에 h를 지닌다.

**실행의 세 종류.**

| 이름 | 정의 |
|---|---|
| **유한 실행** `Pref(B,σ,M)` | `t_k ∈ en_σ(q_{k−1})`, `q_k = δ(q_{k−1}, t_k)`인 전이 열 `t₁…t_n`. prefix에 닫혀 있다. bounded 모형의 같은 집합은 `Pref_b`다. **도달 가능 상태** `Reach_σ(q)`는 q에서 유한 실행이 닿는 상태다 |
| **maximal 실행** `Max(B,σ,M)` | `en_σ(q) = ∅`인 상태에서 끝나는 유한 실행, 그리고 σ의 공정성 조건을 만족하는 무한 실행. σ_U와 σ_R1에는 공정성 조건이 없고, σ_U∞는 강하게 공정하다. spec의 `Exec(B,σ,M)`은 `Max(B,σ,M)`이다 |
| **실제 실행** (σ_E) | B 아래 gcc 바이너리의 유한한 LP 단위 event 열. stop 방법(PI 결정 D17), `abort()`(`cfe_psp_support.c:107, :120`), teardown 뒤 `main`의 반환, harness kill, watchdog 중 하나로 끝난다. 실제 실행은 유한 실행이다. maximal일 필요가 없고, 어떤 주장도 maximal인지에 기대지 않는다 |

σ_U에서 "n이 일어나는 모든 실행" 꼴의 진술(MustBefore)은 Pref로 읽든 Max로 읽든 같다.

**각 개념이 쓰는 것.**
- safety(status, bad 유한 실행, §11의 safety 진술과 class, witness)는 유한 실행 위에서 정의한다.
- progress(죽은 actor와 stall 상태, §7.3)는 도달 가능한 곱 상태 위에서 정의한다.
- 어떤 판정도 Max나 공정성 가정에 기대지 않는다. Max는 무엇을 주장하지 않는지 말할 때만 쓴다(§7.5).

**시간 기반 전이 TB** [D] (spec §1.8; EXT-20261012 §4.2). `TB ⊆ T`는 다음 셋이다.
1. **주기적 대기**가 아닌 모든 timed wait의 timeout 전이: soft barrier, poll(L1 요약 포함), OSAL RESERVED 대기의 timed wake. 주기적 대기는 star-closed loop 안의 timed wait로서 timeout과 성공이 모두 loop를 잇는 것이다(예: `ES_BG_TASK`의 대기, `cfe_es_backgroundtask.c:185`). 그 timeout은 OSAL timer 만료처럼 주기적 일의 박자를 맞출 뿐이고 TB가 아니다.
2. 모든 **지연 retry**(delay-paced retry)의 **마지막 시도**의 실패 결과. 수준 M에서 그 시도를 결정하는 전이에서 취한다.
   - 시도가 M에서 전이 하나면(L1 요약; M이 전이 하나로 모형화하는 Φ 연산, 예: M_api의 CF 조회) 시도 자체의 실패 결과다.
   - 그 밖에는 시도의 묵은 실패 결과를 소진 경로로 보내는, 결과 label이 붙은 갈래다(M_impl의 CF: 마지막 반복에서 `NAME_NOT_FOUND`를 가지고 지나는 `cf_cfdp.c:1285`).
   - 시도 자신의 전이는 TB가 아니다. 그래서 TB를 지워도 lock을 쥔 채 남는 actor는 없다.
3. Φ가 시간 기반이라고 선언한 모든 전이. Φ는 대기를 포기하거나 다른 actor에 작용하는 모든 countdown·시계 비교를, 짝이 되는 "아직 만료 안 됨" 결과와 함께 선언해야 한다.
   - 선언된 것: **강제 kill**, 즉 ES kill timer의 `expire-waiting`(`AppTimerMsec`, spec §3.8, §3.14). main task가 STOPPED 저장(`cfe_es_api.c:399`)에 이르지 않은 앱에 대한 만료다. `expire-stopped`(자발적 `CFE_ES_ExitApp` 뒤 timer 0의 만료 포함)는 앱 정리의 박자만 맞추므로 TB가 아니고, `not-yet-expired`도 결코 TB가 아니다.
   - 아직 선언되지 않은 것: HS의 application monitor(`hs_monitors.c:107–160`; 기본 `NOACT` table에서는 inactive)와 TIME의 flywheel 진입(`cfe_time_tone.c:1172–1186`). Φ가 선언하기 전까지 σ_U∞는 이들을 남긴다. 활성 HS application monitor 항목이 있는 basis의 instance, 또는 준비 상태 명세가 TIME의 시계 상태를 읽는 instance의 σ_U∞ 결과에는 sub-label `tb-undeclared`를 붙인다.

같은 제어 노드에 공유 상태에 따라 허락되는 비-TB 전이(timed wait의 성공 결과, 전이 하나인 마지막 시도의 성공 결과)가 있으면 그 TB 전이는 **짝이 있다**(paired). 없으면 **짝이 없다**(unpaired).

### 7.2 네 의미론

| σ | 정의 | 역할 |
|---|---|---|
| **σ_U** (untimed) | 가능한 전이의 임의 interleaving이다. `OS_TaskDelay`는 양보(yield)다. timeout 있는 대기(soft barrier, poll, OSAL RESERVED 대기의 timed wake)는 시작 뒤 언제든 timeout될 수 있다. 횟수 제한 retry는 **reference 횟수를 다 쓴 뒤에만** 소진된다(정확히 펼침, §8.2). poll은 현재 값을 읽는다. 우선순위와 affinity는 아무것도 강제하지 않는다. `CFE_PSP_Panic`은 끝(terminal)이다(`abort()`, `cfe_psp_support.c:116–121`). `CFE_PSP_Restart`는 **호출자에게** 끝이다(돌아오지 않고 지연 뒤 `abort()`, `cfe_psp_support.c:60–107`). 다른 actor는 계속 돈다 | certificate와 모형 분류의 기준 |
| **σ_U∞** (TB 없음) | σ_U에서 시간 기반 전이를 뺀 것이다: `en_σU∞(q) = en_σU(q) \ TB`. 주기적이 아닌 timed wait는 timeout되지 않는다. ES kill timer는 끝나지 않은 앱을 죽이지 않는다(`expire-waiting` 제거). 끝난 앱은 여전히 정리되므로(`expire-stopped` 남음) 재시작, 재적재, 삭제, K-LIFE 구간은 그대로 일어난다. Φ가 아직 선언하지 않은 포기도 남는다(`tb-undeclared`). 주기적 대기와 timer tick도 남는다. 지연 retry(반복 사이에 Φ delay 연산이 있는 횟수 제한 loop, pattern으로 인식, §8.2)의 마지막 시도에는 실패 결과가 없다. 그 시도가 M에서 전이 하나면 성공할 수 있을 때만 허락된다. 여러 전이면 actor는 시도를 마친 뒤 묵은 실패 결과 위에 **좌초**(stranded)한다. 여러 전이인 경우는 test가 한 critical section과 한 원자적 읽기 밖에서 여러 값을 읽어 L1이 요약하지 못할 때, 또는 M이 LP에서 쪼개는 Φ 연산(예: M_impl의 OSAL 조회)을 부를 때다. 좌초한 actor와 그것이 허락할 수 있는 actor의 죽음 판정은 정해지지 않는다(`unknown(unmodelled)`). 그래서 `Pref(B,σ_U∞,M)`은 정확히 시간 기반 전이를 담지 않은 σ_U 유한 실행의 집합이다(L-CONT). 기다리는 predicate가 결코 참이 될 수 없는 actor는 **죽었다**(§7.3). 이것은 progress 결과이지 class의 결과가 아니다. L1이 요약한 poll(예: `CFE_ES_MainTaskSyncDelay`, test 읽기가 ES lock critical section 하나 안에 있다)은 M에서 전이 하나이므로 그 actor는 좌초하지 않고 죽는다. L1을 끄면 좌초한다. Max에 대해서만 스케줄은 **강한 공정성**(strong fairness: 무한히 자주 가능해지는 전이는 무한히 자주 실행된다)을 가진다(Musuvathi & Qadeer PLDI 2008 [V-abs]). 어떤 판정도 그것에 기대지 않는다 [D] (EXT-20261012 §4.2; [X1-M5]의 quiescence 규칙을 대신함) | "시간 기반 전이를 거쳐서만 위반되는가"와 "순서 논리에서 위반되는가"를 가른다. 시간 기반 전이가 가리는 영원한 대기를 드러낸다 |
| **σ_R1[P_sched]** | 한 CPU의 고정 우선순위 선점. 인용한 scheduling profile `P_sched`(정책, 우선순위 대응, root thread 우선순위, priority inheritance)로 매개변수화한다. 지연은 막고(block) 상수 평가한 지연 인자로 이산 시간에 깨우며, 막는 지점 사이 계산은 시간 0이다(가정 A-ZT). timeout은 σ_U처럼 허락된다. §7.1의 별도 전이 체계이고, 투영하면 σ_U에 들어간다(§7.5) | 우선순위가 순서를 정하는 플랫폼(예: 6.4.1의 C3). 기본은 모형으로만 |
| **σ_E** | 기록된 Env에서 실제 바이너리의 실행. 모형화하지 않고 관측·확인 실행으로 표본만 얻는다 | 실제 witness와 빈도 |

`P_sched^posix` [F]: RT 스케줄링이 허용될 때(root/RT 권한) OSAL POSIX는 Linux에서 `SCHED_RR`를 고르고, `CFE_ES_Main`을 돌리는 root thread가 가장 높은 우선순위를 받으며, OSAL 우선순위는 `OS_PriorityRemap`이 여러 개를 하나로 압축하고, mutex는 `PTHREAD_PRIO_INHERIT`다(`os-impl-tasks.c:78–106, 348–365, 391–409`; `os-impl-mutex.c:82`; spec §1.8; PI 결정 D4). 기록된 ENV는 이 조건 밖이다. 27개 thread가 모두 `SCHED_OTHER`라 우선순위가 아무 효과가 없다 [M] ([BM §0]; §6의 Env 행). 그래서 이 profile은 σ_R1 모형과, 나중의 별도 구성인 native arm에만 쓴다. PI 결정 D4(2026-10-15): 첫 해석은 모델 기반 단일 코어이고, 실제 우선순위 실행은 필요성을 확인한 뒤 별도 구성·조건으로 검증하며, 모형 결과를 실제 실시간 실행 결과로 표기하지 않는다. RTEMS·VxWorks profile은 backend 소스가 basis에 들어오기 전까지 `unspecified_by_reference`이고 σ_R1 판정을 내지 않는다(PI 결정 D13). [I] `P_sched^posix`에서는 startup thread가 실행 가능한 동안 core App-record 구간이 한 CPU에서 열릴 수 없다.

### 7.3 실행의 결과: good, bad, stall

v1은 maximal 실행 하나에 good, bad, stall 중 하나를 붙였다. 지금은 셋이 다른 곳에 있다 [D] (spec §1.6, §1.8; EXT-20261012 §4.1; ADR-039, ADR-040).
- **bad**와 **good**은 유한 실행의 성질이다(§5.5). bad 유한 실행은 범위 안 `VIOLATING`(Allowed 미지정이면 `NOT_READY_UNJUDGED`) 발생을 담는다. good witness는 범위 안 `READY`나 `TOLERATED` 발생을 담는다. 둘 다 발생의 ordinal을 기록한다. 이것이 safety 쪽이다.
- **stall**은 class의 결과가 아니다. 도달 가능한 곱 상태에서 **죽은 actor**로 정의하는 **progress** 결과다(아래). quiescence(다른 어떤 전이도 불가능한 상태)로 정의하던 v1 규칙은 버렸다.

**종료와 죽음.**
- `TS(B,σ,M)`에서 actor instance `a`가 자기 exit에 있거나 terminal 연산(T-TERM, §9.2) 뒤에 있으면 **끝났다**(terminated).
- 죽음은 instance i마다 판정한다. i에서 제외된 상태(아래)를 모두 **sink**로 만든 도달 가능 곱 그래프 `G_i`를 쓴다. 경로는 첫 제외 상태에서 멈추고, 제외 상태에서 나가는 전이는 세지 않는다.
- 제외되지 않은 상태 q에서 `a`가 i에 대해 **죽었다**(dead)는 것은 `a`가 끝나지 않았고, `Reach_{G_i}(q)`의 어떤 제외되지 않은 상태도 `a`의 전이를 허락하지 않는다는 뜻이다.
- 죽음은 q, σ, i의 성질이지 스케줄의 성질이 아니다. 공정성이 필요 없다. 허락되지만 굶는(starved) actor는 결코 죽지 않았다. 종료 요청 뒤의 teardown이나 terminal 단계 뒤 모형의 연속만이 되살릴 수 있는 actor는 죽었다.
- σ_U에서 timed wait는 결코 죽지 않는다. timeout이 늘 허락되기 때문이다. σ_U∞에서 주기적이 아닌 timed wait는 predicate가 더는 참이 될 수 없으면 죽는다.
- σ_R1에서 죽음은 σ_U의 허락 관계로 판정한다. 투영한 상태에서 σ_U가 허락하는 `a`의 전이를 가진 σ_R1 상태에 닿을 수 없으면 `a`는 죽었다. 우선순위 굶주림은 굶는 actor 자신을 죽게 하지 않는다. 다만 굶는 giver를 기다리는 actor는 무한 σ_R1에서 죽을 수 있다(A-ZT 아래 높은 우선순위의 시간 0 loop가 giver를 영원히 굶긴다).
- σ_U∞에서 σ_U가 허락하는 전이가 모두 짝 없는 시간 기반 전이(§7.1)인 actor는 **좌초했다**(stranded). 여러 전이로 된 마지막 시도의 실패 갈래가 그렇다. 좌초한 actor와, 그 effect가 허락할 수 있는 actor의 죽음 판정은 정해지지 않는다(`unknown(unmodelled)`; censored actor와 같은 처리, spec §4.10 item 8).
- actor의 죽음은 TS 상태에서와 그 위의 모든 곱 상태에서 같다(L-MON).

**앞에 있는 사용(ahead).** 곱 상태에서 `a`의 제어 노드로부터 actor 안 edge와 spawn edge를 쓰는 `Sk^cert` 경로가 다음으로 이어지면 `u`는 `a`의 **앞에 있다**. `a`에 열린 발생이 없으면 `u`의 template 호출 노드로, 있으면 그 열린 발생의 남은 conjunct LP로 이어지는 경로다. 이전 발생은 상관없다. 그 경로의 local choice가 **ahead support**다.

**process 종료 단계와 제외된 상태.**
- Φ는 terminal 단계가 process를 끝내는 terminal 선언에 `process_terminal`을 표시한다. pc-linux에서는 `abort`; `CFE_PSP_Panic`(`cfe_psp_support.c:120`의 `abort()`); `CFE_PSP_RESTART_DELAY` 뒤 재시작 요청자의 단계(`:107`의 `abort()`); teardown 뒤 PSP main thread의 `main` 반환(`bsp_start.c:247`)이다. 이 목록은 문서 전체에서 하나다.
- 도달 가능한 곱 상태가 i에 대해 **제외된다**는 것은 그 유한 실행에 process 종료 단계가 있거나, scope가 `default`인 i에 대해 SHUTDOWN_REQ LP가 있다는 뜻이다.
- 모형은 terminal 단계 뒤에도 다른 actor를 계속 돌린다(T-TERM은 호출자만 자른다). 그 연속은 죽음과 qualifier 판정에서 sink이고, 거기서는 어떤 status도 horizon 판정도 정하지 않는다(§5.2, §5.5).
- K-TEARDOWN instance의 progress는 `unknown(teardown_scope)`다.

**stall 상태.** 제외되지 않은 도달 가능 곱 상태가 i의 **stall 상태**라는 것은 다음 중 하나가 성립한다는 뜻이다. 기록은 성립하는 절의 집합을 지닌다. 절이 가리키는 죽은 actor가 stall의 **witness actor**다.

| 절 | 조건 |
|---|---|
| **pre-use** | 그 상태에서 죽은 어떤 actor instance가 `u`의 열린 발생 없이 `u`를 앞에 두고 있다 |
| **in-use** | 죽은 어떤 actor instance가 `u`의 범위 안 열린 발생을 가진다(호출 안, 결정 전) |
| **horizon** | `u`의 범위 안 `PENDING` 발생이 있고, 그 감시 actor(§5.2)가 죽었거나, 아직 없으면서 그것을 spawn할 수 있는 기존 actor가 모두 죽었다 |

**kind와 qualifier** (stall witness마다 기록한다)

| 이름 | 뜻 |
|---|---|
| kind `deadlock-cycle` | 죽은 actor들이 순환을 이룬다. 각자 다음 actor가 쥔 holder 해제형 object(mutex, OSAL type lock)를 timeout 없이 잡으려 한다 |
| kind `leaked-lock` | witness actor가 holder 해제형 비재귀 object(OSAL type lock은 `PTHREAD_MUTEX_NORMAL`, `os-impl-idmap.c:222`)를 timeout 없이 잡으려 한다. 그 holder는 object를 잡은 API에서 이미 돌아왔고, 끝났거나, 먼저 다시 잡지 않고 풀 `Sk^cert` 경로가 없다(K-LOCKBAL) |
| kind `other` | 그 밖 |
| `terminal-pending` | B.Input의 stop event를 넣는 전이를 뺀 `G_i`에서, stall 상태에서 닿는 모든 상태로부터 **process 끝**(process 종료 단계나 SHUTDOWN_REQ LP. pc-linux에서 뒤에는 늘 요청자의 `abort()`나 `main` 반환이 온다, spec §3.5)이 계속 도달 가능하다. process는 stop이 일으키지 않은 panic이나 abort로 끝난다. 닫힌 cone 위에서 계산하므로 cone에 상대적이다(보장 G11) |
| `idle` | witness actor가 모두 **idle 대기**에 있다. idle 대기는 star-closed message loop나 주기 loop 반복의 첫 blocking 대기다(예: `ReceiveBuffer`, `cfe_time_tone.c:1266`, `sch_lab_app.c:98`) |
| `choice-dependent` | **choice support**가 비어 있지 않다. choice support는 stall의 wait-for closure(witness actor, 그들이 이행적으로 기다리는 actor, 그들이 기다리는 object의 holder)가 유한 실행에서 한 과대 근사 local choice(star-closure 반복/탈출, havoc 값, certificate view가 풀지 않은 데이터 분기)와 ahead support를 합친 것이다 |

- **wait-for 그래프**는 죽은 actor마다 그 대기를 채울 수 있는 모든 actor로 가는 edge를 둔다. 잡으려는 object의 holder; 받거나 기다리는 object에 give, put, send하는 `Sk^cert` 영역을 가진 actor; 기다리는 predicate의 지원 위치에 쓰는 actor; join 대상; 아직 spawn되지 않았으면 그 spawner다. witness actor에서 닿는 부분만 남긴다.
- **wait-for root**는 그 그래프의 끝 강연결 성분(terminal strongly connected component)이다. `deadlock-cycle`이면 lock 순환이고, 아니면 predicate·object 대기의 순환이다. 나가는 edge가 없는 노드와 그것이 채우지 못한 대기도 root다. `leaked-lock`이면 누수된 object와 그 holder다. 기록은 stall의 root 집합을 지닌다.
- stall witness는 kind가 `deadlock-cycle`이나 `leaked-lock`이고 idle도 terminal-pending도 아닐 때 **confirmable**이다. idle도 terminal-pending도 아닌 stall 상태가 **hang 상태**다.

**stall의 harm** (이 우선순위로 정한다)
1. terminal-pending이면, 계속 도달 가능한 process 끝에 따라 `terminal_pending(panic)`(`CFE_PSP_Panic`)이나 `terminal_pending(abort)`(다른 abort, 재시작 요청, `main` 반환)다.
2. 아니면, witness actor가 모두 `u`의 actor(horizon stall이면 감시 actor)로 가는 spawn 사슬 위에 있고 그 actor instance가 아직 없을 때 `task_never_runs`다.
3. 그 밖에는 `hang_or_deadlock`이다.

기록은 죽은 actor마다 그것이 기다리는 object나 predicate를 적는다.

**progress 결과** [D] (spec §1.8, §1.10). 칸 (σ, M)과 경계마다 셋 중 하나다.
- `stall-witness(S)`: i의 stall 상태에 닿을 수 있다. S는 성립하는 {pre-use, in-use, horizon} 절의 공집합 아닌 집합이다. witness는 그 상태까지의 유한 실행이고, kind, qualifier, wait-for root, harm, 실현 방식을 지닌다. 실현 방식은 explorer의 `model(b)`이거나, wait-for 사실을 가진 실행의 `realized`다(spec §4.13).
- `no-stall-within(b)`: `exh`가 b 안에서 i의 stall 상태에 닿지 않음을 보였고, 정해지지 않은 죽음 판정이 없다.
- 그 밖에는 `unknown(reason)`이다. 이유는 `not_checked`(아무 경로도 돌지 않음), `bound`(censored), `budget`, `unmodelled`(좌초, 또는 좌초 actor의 closure), `teardown_scope`, `model_gap`, `model_fault`다.

- certificate, 예측, wait-for 사실 없는 관측, σ 사이 전이는 progress 결과를 정하지 않는다. 예외는 하나다. kind `deadlock-cycle`이나 `leaked-lock`의 stall 상태는 그 유한 실행이 허락되는 모든 σ에서 stall 상태다(L-DEAD (c), (d)).
- 어떤 값도 "live", "deadlock-free", "safe"를 뜻하지 않는다.
- progress 결과가 다루지 않는 실행이 있다. `u`의 actor instance나 그 spawn 사슬의 actor가 발생 없이 끝나는 실행(F7로 잃은 task, `osapi-task.c:128–137`; timebase thread의 조용한 반환)과 panic으로 끝나는 실행이다. 그 기록은 그리로 이끈 not-ready 뿌리 발생의 safety 결과와, 그 사슬에서 관측된 결과(`consequence`: 잃은 task, panic, process abort), 그리고 그 뿌리의 caller 수준 Allowed가 주는 harm(K-SPAWN.osal과 K-SELF.es raise의 기본값 아래에서는 `unspecified_by_reference`, §5.3; spec §1.6, §3.3)뿐이다. 자발적 timeout을 거친 σ_U panic에는 not-ready 뿌리가 없어 기록이 남지 않는다(spec §1.8, §6.4 item 34).
- 모형의 stall witness는 두 가지 이유로 그 모형에 상대적이다. 첫째, `Sk^cert`는 경로를 과대 근사하고(A-SKEL) star-closed loop의 탈출 같은 local choice를 더한다. 그래서 `choice-dependent` qualifier가 있다. 둘째, 죽음은 행동을 더해도 빼도 보존되지 않는다(L-DEAD). 그래서 모형 stall witness는 확인될 때까지 후보이고, confirmable kind만 확인할 수 있으며, `no-stall-within(b)`는 모형에 대한 진술일 뿐이다(보장 G11).

### 7.4 Refinement: timeout이 하는 역할

refinement는 σ마다 따로 계산한 결과에서 끌어내고, 자기 경계와 함께 저장한다 [D] (spec §1.8; [X1-M5], [X1-m3] 수정, EXT-20261012 §4.2가 다시 씀). 아래에서 `Bad_i(σ)`와 `OK_i(σ)`는 칸의 M과 b에서 bad 유한 실행과 good witness가 있다는 뜻이다(§11.1).

| refinement | 조건 | 뜻 |
|---|---|---|
| **timeout-dependent** (σ_U의 schedule-dependent를 세분) | `class(σ_U) = schedule-dependent`, `¬Bad_i(σ_U∞)`가 확립됨, σ_U∞에서 사용에 도달함(reach `model(b)`나 `proved`) | L-CONT에 따라 앞의 두 조건은 "σ_U의 모든 bad 유한 실행이 시간 기반 전이를 담는다"와 같다. σ_U∞ reach가 없으면 붙이지 않는다. 그때는 `timeout-rescued`와 σ_U∞ reach가, 사용이 시간 기반 전이를 거쳐서만 도달됨을 보인다 |
| (없음) | `Bad_i(σ_U∞)`가 성립 | 준비 상태 위반은 순서 논리 안에 있다 |
| **timeout-rescued** (good 쪽 flag) | `OK_i(σ_U)`와 `¬OK_i(σ_U∞)`가 확립됨 | σ_U의 모든 good witness가 시간 기반 전이를 담는다. 어떤 대기가 포기했기 때문에만 good이다. protected, schedule-dependent, ordinal-split(σ_U에서 good witness를 가진 모든 class)에 붙을 수 있다 |
| **timeout-masked-liveness** (σ_U 칸의 progress refinement, **instance마다**) | σ_U∞에서 도달 가능한 어떤 곱 상태가 i에 대해 σ_U∞에서는 **hang 상태**이고 σ_U에서는 아니다 | σ_U에서 그 상태를 벗어나는 길은 모두 시간 기반 전이를 지난다. 그 전이가 영원한 대기를 준비 상태 위반, panic, 다른 연속으로 바꾼다 |
| **tml-root** | `i₁`로 들어오는 progress 사슬이 없고, `i₁ ⇝_p* i₂`인 어떤 `i₂`(`i₂ = i₁` 포함)가 timeout-masked-liveness를 가진다 | TML 구성원을 가진 progress 사슬의 뿌리, 또는 사슬 없는 TML instance 자신을 표시한다. 보고서는 `tml-root` instance와 사슬 구성원 수를 보인다. 구성원은 자기 칸에 timeout-masked-liveness와 flag `CHAINED`를 지니고 하나씩 보고하지 않는다. 뿌리 자신의 σ_U∞ progress는 `no-stall-within(b)`일 수 있다 |

- **explorer 없이는 어떤 refinement도 확립되지 않는다.** `timeout-dependent`와 `timeout-rescued`는 σ_U∞의 보편 진술이 필요하다. certificate는 σ_U만, 예측은 witness만 준다. 예측은 `Bad_i(σ_U∞)`나 `OK_i(σ_U∞)`를 보여 refinement를 배제할 수만 있다. 뒤의 둘은 progress 결과다.
- **tml-root의 예** ([I], spec §1.8, App. A.2 step 3). core App-record 구간 안에서 raise를 건너뛰면 `MainTaskSyncDelay`가 만족될 수 없다(spec §3.3). σ_U∞에서 `CFE_ES_MainTaskSyncDelay`(`cfe_es_start.c:860–918`)에 L1을 적용하면 startup thread는 `cfe_es_start.c:835`에서 죽고, 이미 만든 core task는 `CORE_READY` poll에서 죽는다. σ_U에서는 startup thread가 panic한다. 그 뒤의 instance들은 timeout-masked-liveness를 지니고 그 구간의 K-SELF.es instance에 사슬로 붙는다. 그 K-SELF.es instance가 `tml-root`를 진다.
- **v1과의 대응** [I]. v1의 timeout-dependent는 첫 timeout 직전 상태에서 σ_U∞로 이어 가면 good만 나온다는 조건이었다. 새 refinement는 그 "bad 아님" 부분을 지키고, "stall 없음" 부분은 progress refinement가 된다. v1의 timeout-masked-liveness instance 중 v1 stall 상태가 σ_U∞의 hang 상태인 것은 새 refinement를 가진다. v1은 사용 actor가 idle 대기에 있는 상태도 셌지만 새 refinement는 뺀다. 반대 방향은 성립하지 않는다. 새 refinement는 그 instance의 bad σ_U 실행이 지나지 않는 hang 상태도 다루고, progress 사슬이 그것을 뿌리에 붙인다.

**C1로 본 뜻** ([I], spec App. A.4 step 3, 파생 B′, M_api. B′는 D22가 정했다: 공개 BP 기반, 후보 쌍 CF `281a941` → `833fdbb` [D]. §3.5의 예).
- **항목.** CF의 `retry_until_ready` 항목이 status를 정하는 것은 `caller` record가 있을 때뿐이다. 그 record는 CF의 인정(`cf_cfdp.c:1272–1278`, kind `code_admission`, side `caller`; spec §7.1 source 8, RC1)이다 [I]. 인식한 loop만으로는(`code_pattern`, side `rule`) 항목이 `unspecified_by_reference`이고 모든 not-ready 시도가 NOT_READY_UNJUDGED다. 아래 class는 `caller` record가 있는 경우다(QT-4 엄격한 대안, spec App. A.4 step 3).
- **σ_U.** CF의 retry를 모두 provider의 생성보다 앞에 둔 실행이 있다. 지연은 양보일 뿐이기 때문이다. 그래서 `schedule-dependent(σ_U)`다.
- **σ_U∞, M_api.** loop는 지연 retry이고, 마지막 시도의 실패 결과는 시간 기반 전이라 지워진다. M_api에서 시도는 전이 하나(조회)이므로, 마지막 시도는 조회가 성공할 수 있을 때만 허락된다. provider가 아직 돌 수 있는 동안 σ_U∞의 어떤 유한 실행도 bad가 아니다. not-ready 시도의 monitor(`retry_until_ready`)는 뒤 시도가 READY일 때 ACCEPT하거나 OPEN으로 남을 뿐이다. REJECT event인 소진이 시간 기반이기 때문이다. 그래서 `¬Bad_i(σ_U∞)`이고, `exh`가 b 안에서 정한다. σ_U∞에서 사용에 도달하므로 **timeout-dependent**다. explorer 없이는 확립되지 않는다.
- **수정 전 판**(CF `281a941`, 선택한 쌍의 수정 직전 판, 조회 한 번: `cf_cfdp.c` L993–L1001, `cases/C1.md`. 아래 구조는 보고자의 `2a024d8`에서 읽은 것이다. 공개 BP table과 섞지 않으며 anchor audit이 `281a941`에서 다시 읽는다 [U]). 처리는 연속을 인용하지 않은 status test다. test(`cf_cfdp.c:1016` @2a024d8)는 정확히 펼친 channel loop 안에 있다. not-ready 갈래는 `CF_EID_ERR_INIT_SEM`을 올리고 `err_out: return ret;`로 간다. 성공 갈래도 거기에 닿고 `ret`을 돌려준다(`:1018–1021, 1052–1053` @2a024d8). 그래서 추출된 `cont_candidate`는 none이다(§5.2). error가 반환되고 CF는 `APP_ERROR`로 끝난다([LED §1a]). 이것들은 code 사실이고 그 발생의 consequence로 기록한다. 이 처리가 받아들여진다고 말하거나 연속을 인용하는 reference는 여기에 없다 [I]. 그래서 엄격한 대안(Q15와 QT-4, 2026-10-15 결정)에서 항목은 `unspecified_by_reference`이고 발생은 시간 기반 전이 없이 NOT_READY_UNJUDGED다. 나중에 수정 전 처리에 대한 `caller` record가 나오면 label이 바뀌고 class는 바뀌지 않는다. NOT_READY_UNJUDGED 발생은 bad이기 때문이다(§5.5). σ_U∞에서도 schedule-dependent다. C1의 수정([LED §1b])은 순서 논리의 준비 상태 위반을 timing 여유에 대한 의존으로 바꾸었다.
- **provider가 결코 돌지 않으면.** 이 경우를 쓰는 cFS basis는 없다(reference가 없다). litmus L-PATIENT variant 2(spec App. A.8)가 결과를 보인다. certificate의 REJECT인 소진이 결정 뒤에 있으므로, b = ∞에서 class는 `unknown(no_decision_route)`이고 모든 σ에서 `¬OK_i`가 보인다. b 안에서 `exh`는 σ_U에서 fixed-order(소진하는 실행이 bad witness), σ_U∞에서 `no-final-status`를 준다. σ_U∞에서 retry하는 actor는 마지막 시도의 호출 노드에서 죽는다(`stall-witness({pre-use, horizon})`, hang). σ_U는 소진으로만 벗어나므로 **timeout-masked-liveness**다.
- **M_impl.** 조회는 여러 전이다(§3.5). 시간 기반 전이는 마지막 반복에서 `NAME_NOT_FOUND`를 가지고 지나는 `cf_cfdp.c:1285` 갈래다. 실패한 마지막 조회 뒤 CF는 type lock을 풀고 **좌초**한다. CF와 그것이 허락할 수 있는 actor(startup thread의 script barrier)의 progress는 `unknown(unmodelled)`다. RESERVED 구간에 닿는 경로는 따로 있다(§3.5의 예, `leaked-lock`).
- 이른 조회 한 번(`retry_until_ready`)은 `allowed-not-ready`이고, `exh`에서만 나온다.

**timeout-exercised** (spec §1.8). 확인 실행에서 시간 기반 전이(주기적이 아닌 timed wait의 timeout, 지연 retry의 실패한 마지막 시도, 강제 kill)가 instance의 닫힌 cone에 속한 actor에서 일어난 경우다. hold 동안, 또는 hold 뒤 witness의 결정 event 전에 일어났으면 naturalness `timeout-exercised`를 붙이고 자연 σ_E로 세지 않는다. 반환 status만 보지 않고 prefix probe(spec §4.7 item 6 (c))로 정한다. hold가 provider를 붙잡아 소진된 retry(C1, App. A.4 step 5)는 timeout status가 아니라 `NAME_NOT_FOUND`를 보고, OSAL 내부 timed wake는 결코 반환되지 않기 때문이다. 주기적 대기의 timeout은 세지 않는다.

### 7.5 Containment와 전이 규칙

**containment**는 한 의미론의 실행이 다른 의미론의 실행에 모두 포함된다는 관계다. 판정을 의미론 사이에서 옮기는 근거다. spec은 이것을 **모형 수준별로**, **유한 실행(prefix) 위에서** 말한다(Lemma L-CONT; [X1-C2] 수정, EXT-20261012 §4.2가 다시 씀). 가정 A7-impl, A7-api, A3, A4, A9, A-SKEL(§12.2) 아래에서:

- 각 M에 대해 `Pref(B,σ_U∞,M) = {ρ ∈ Pref(B,σ_U,M) : ρ에 시간 기반 전이가 없다}`.
- 각 M에 대해 `π(Pref(B,σ_R1,M)) ⊆ Pref(B,σ_U,M)`. π는 σ_R1의 시계 진행·깨움 단계를 지우고, 나머지 단계(delay의 block 단계는 σ_U의 delay 전이)는 남기며, 상태를 σ_U 성분으로 투영한다.
- B의 모든 유한 실제 실행을 LP 단위 event로 투영하면 `Pref(B,σ_U,M_impl)`에 든다.

**maximal containment는 주장하지 않는다.**
- 일반적으로 `Max(B,σ_U∞,M) ⊄ Max(B,σ_U,M)`. litmus LP-TW(§12.6)에서 대기자가 죽은 채 끝나는 σ_U∞ 실행은 σ_U∞에서 maximal이지만, σ_U는 그 대기자의 timeout을 여전히 허락한다.
- 실제 실행은 유한 실행이고, 끝 상태가 σ_U 전이를 여전히 허락할 수 있다.
- σ_R1의 π를 법으로 한 maximal containment는 증명되지 않은 추측이고 쓰지 않는다 [I].

`Pref(B,σ_E) ⊆ Pref(B,σ_U,M_api)`도 **주장하지 않는다.** M_api는 실제 POSIX 행동(RESERVED 구간의 `INCORRECT_OBJ_STATE`, type lock 누수, 조용한 task 손실)을 설계상 뺀다. cone에 M_impl-relevant 연산이 없으면 두 수준의 투영된 유한 실행이 같다(Lemma L-MLEVEL [I], 아직 증명할 의무).

**전이 규칙** [D] (spec §1.8; EXT-20261012 §4.2가 고침)

1. 규칙 1은 두 부분이다.
   - **(a) safety는 진술별로 아래로 옮긴다.** σ_U, 수준 M, 경계 b에서 확립한 진술 `¬Bad_i`, `¬OK_i`, `¬NR_i`(§11.1)는 같은 진술로 σ_U∞(같은 M, 같은 b)에서 성립한다. σ_R1[P_sched](같은 M)에서는 b = ∞일 때, 또는 b가 유한하면 σ_R1 모형의 cone이 같은 h에서 σ_U 모형의 cone을 담을 때(L-CONE-R1) 성립한다. 받은 진술에는 flag `SAFETY_TRANSFERRED`를 붙인다. 옮긴 `¬OK_i`가 결정적인지(decision route가 있는지)는 받는 σ에서 다시 계산한다(§11.1). 받는 칸의 class는 그 기록에서 나오므로, 옮긴 진술과 그 자리에서 확립한 진술은 순서와 상관없이 합쳐진다.
   - **(b) 존재는 σ마다 따로.** reach, witness(그래서 `allowed-not-ready`, `schedule-dependent`, `ordinal-split`), progress 결과는 아래로 옮기지 않는다. 예외는 L-DEAD (c), (d)의 stall 상태(deadlock 순환, 누수된 lock)뿐이다. 각각 자기 σ에서 계산하고, 계산하지 않았으면 칸은 `reach = assumed`, `progress = unknown(not_checked)`다. σ′의 어떤 유한 실행도 b 안에서 사용에 닿지 않는다는 것을 `exh`가 보이면 그 칸의 reach는 `unreached(b)`이고, 옮긴 보편 class는 "… 도달한다면; b 안에서는 도달하지 않음"으로 읽는다. v1의 label `inactive_σ′`는 없앴다. `inactive`는 정적 class뿐이다.
2. **σ_E로는** b = ∞인 safety 진술만 옮긴다. M_impl에서는 옮긴다. M_api에서는 (i) instance의 cone과 certificate에 M_impl-relevant 연산이 없거나(prefix 위의 L-MLEVEL, sub-label `mlevel-lemma`), (ii) M_impl 결과가 있고 같을 때만 옮긴다. 그러면 σ_E 칸은 B의 모든 유한 실제 실행의 **판정된(decided)** 범위 안 발생에 대해, 거기서 확립된 safety 진술(§11.1)을 certificate lemma가 증명하는 꼴(L-FO, L-PR, §12.3) 그대로 말한다.
   - `¬Bad_i`: `VIOLATING`이나 `NOT_READY_UNJUDGED`인 것이 없다.
   - `¬OK_i`: `READY`나 `TOLERATED`인 것이 없다.
   - `¬NR_i`: 준비 안 됨으로 판정된 것(`TOLERATED`, `PENDING`, `VIOLATING`, `NOT_READY_UNJUDGED`)이 없다. 곧 판정된 것은 모두 `READY`나 `NOT_EVALUATED`다.

   실행이 끝났을 때 아직 열려 있는 발생은 status가 없고 세 진술 밖이다. `NOT_EVALUATED` 발생(호출이 어느 conjunct LP보다 먼저 반환했고 거짓으로 판명된 conjunct가 없는 경우. 예: 인자 검사에서의 조기 반환)은 준비 상태도 준비 안 됨도 아니다. 세 진술이 모두 허용하므로, 그런 발생을 가진 실제 실행은 이 칸의 반례가 아니다. 이 칸은 실제 실행의 모든 범위 안 발생이 `READY`라고, 어떤 발생에 도달한다고, 진행성에 대해 무엇이라고도 말하지 않는다. 그 밖에는 σ_E 칸이 `unknown(model_level)`이고, M_api 결과는 이식성 주장으로 보고한다. (2026-10-15 정정: 이전 문안은 `¬NR_i`를 "`READY` 밖이 아니다"라고 써서 `NOT_EVALUATED`까지 배제했고, 그것은 L-PR이 증명하는 것보다 강했다. spec §0.9 항목 1, ADR-045. 정정 전후의 문장은 [../plan/formal_corrections.md](../plan/formal_corrections.md) §1.)
3. **witness는 위로 옮긴다.** 다음은 범위 안의 최종 status(`READY`, `TOLERATED`, `VIOLATING`, `NOT_READY_UNJUDGED`) 발생을 담으면 같은 M의 σ_U에서 good이나 bad witness이고, 발생 ordinal을 지닌다.
   - σ′(σ_U∞, 또는 π를 거친 σ_R1)의 유한 실행.
   - M_impl에서의 실제 실행. M_api에서는 L-MLEVEL 아래이거나, instance의 닫힌 cone의 모든 actor prefix가 M_api 적합성 검사를 통과한 실제 실행(L-MAPI-CONE, §12.3; spec §4.7 item 6).

   `PENDING`과 `NOT_EVALUATED` 발생은 witness가 아니다. 실행 witness와 모형 witness는 실행 identity를 canonical 이름으로 옮기는 대응(§1.1)을 거쳐서만 같은 ordinal로 짝짓는다. **stall witness는 옮기지 않는다**(LP-TW: σ_U∞ stall이 있는데 σ_U에는 없다). 예외는 L-DEAD (c), (d)뿐이다.
4. σ_E로는 빈도, 존재 class, reach 값, progress 결과 어느 것도 옮기지 않는다. σ_U 예측이나 모형 witness에서 σ_E 빈도나 class를 만들지 않고, 나쁜 실행을 보지 못했다는 사실에서 σ_E class를 만들지 않는다. σ_E 빈도 `n of N`은 pool key가 같은 실행끼리만 합치고 `σ_E(plan P)`로 표시한다. σ_E의 보편 class(protected, fixed-order, no-final-status)는 규칙 2로만 얻는다. `schedule-dependent(σ_E)`는 같은 ordinal에서 bad와 good 발생을 가진 서로 다른 두 자연 실행으로만 얻는다(§11.4; spec §1.10, §4.12 item 4, §6.4 items 3, 10).
5. "σ 사이에 아무것도 옮기지 않는다"는 너무 약하고, trace 하나의 certificate를 옮기는 것은 너무 강해서 둘 다 버렸다.

### 7.6 모든 결과의 모형·스케줄 식별자 (rev 1.3)

[D] (spec §1.8 "Model and schedule identifiers on every result"; [P0.2] §4.3; C-09). 이 절과 §3.5의 축은 바뀌지 않는다. 모든 결과는 `model_id`, `model_level`(M_api | M_impl), `schedule_profile_id`(σ, σ_R1이면 P_sched 포함), `basis_id`, `scope_id`를 기록한다. 관측된 결과는 σ_E와 그 환경을 기록하고, 그것을 해석하는 데 쓴 profile은 따로 기록한다. **모든 (σ, M) 칸을 구현할 의무는 없다.** 보고서마다 실행한 칸, 모형으로만 분석한 칸, 지원하지 않는 칸을 표시한다. 시간 의존을 분리하지 않는 실험은 분리했다고 말하지 않는다. 특히 C1을 retry 소진으로 줄이지 않는다. 오류 경로는 선택한 CF 버전마다, 그리고 M_api와 M_impl 사이에서 다르다(spec App. A.4; §7.4).

---

## 8. Skeleton의 두 view

### 8.1 Skeleton과 두 view

정적 분석은 C 코드 전체가 아니라 **skeleton** 위에서 일한다. static actor마다 문맥을 구분해 inline한 CFG(control-flow graph, 제어 흐름 그래프)를 관련 노드(conjunct별 LP의 effect와 capture point, 지원 위치의 load·store, 생성 지점, terminal 연산, handler 표시, loop head, local choice)만 남기도록 투영한 것이다 [D] (spec §4.3). 연산 결과 분기는 결과 label이 붙은 전이가 되고, B 상수 분기는 접히며(CF의 빈 `sem_name`은 조회를 도달 불가로 만든다), 나머지 data 분기는 명시적 local choice(과대 근사)가 된다.

view는 둘이다 [D] (spec §4.3; [X1-C1] 수정).

- **Certificate view `Sk^cert`.** reference bound까지 정확히 펼치지 않는 loop는 모두 **star-closed**다. star-closure는 loop를 "0번 이상 몇 번이든 반복"으로 닫는 것으로, loop head, back edge, 명시적 반복/탈출 선택으로 표현한다. message loop의 dispatch는 message provenance(§8.3)로 접는다. horizon도 잘림도 없다. `inactive`, provider `none`, MustBefore, once, PWB, frontier 규칙, K-CAP bound(용량 준비 상태 명세의 backlog 상한)는 **이 view에서만** 계산한다.
- **Exploration view `Sk^h`.** star-closed loop를 h번까지 펼치고, 그 너머는 `TRUNC`로 표시한다. explorer와 O8만 쓰고, 여기서 나온 판정은 b에 h를 지닌다. `kz proofcheck`는 `Sk^h` 노드나 `TRUNC` 영역을 가리키는 certificate 단계를 거부한다.

**왜 둘인가.** 이전 판(v1)은 loop를 h에서 잘랐다. h 너머 반복에서만 닿는 writer가 있으면 잘린 그래프의 "경로 없음"이나 "writer 없음"은 실제 실행에 대해 거짓이 된다. MustBefore 증명의 T-PO 단계가 바로 여기서 깨졌다([X1-C1]; spec §6.2 L-MB). 그래서 보편 주장은 어떤 반복 횟수도 포함하는 view에서만 하고, 잘린 view는 유한 탐색에만 쓴다.

**가정 A-SKEL.** `Sk^cert`는 B 아래 각 actor instance의 투영된 event 열을 과대 근사한다. obligation O1/O2가 모든 trace에서 이것을 시험한다. `Sk^h`는 h 너머에서 과소 근사이므로 보편 주장에 쓰지 않는다. A-SKEL은 safety 진술을 뒷받침한다. progress 결과를 B에 대해 건전하게 만들지는 않는다. 죽음은 과대 근사로도 과소 근사로도 보존되지 않기 때문이다(L-DEAD, 보장 G11).

### 8.2 Loop 처리

임의로 펼치지 않는다(spec §4.3).

| loop 종류 | 처리 |
|---|---|
| **poll loop**(pattern으로 인식 [D], [X1-M8] 수정): body가 Φ delay 연산을 부르고, 탈출이 공유 load·Φ 연산 결과·예산을 검사 | `await(Pred_code) {success} {timeout}`. `Pred_code`는 "조건 성립으로 탈출" 분기를 symbolic하게 평가해 **코드에서 뽑는다.** 형태: (a) 공유 load나 Φ 결과의 직접 검사, (b) 상수 횟수 loop 뒤 counter가 0인지 검사(`∀ i < N: ¬φ(i)`), (c) 그 Boolean 조합. 그 밖은 `unmodelled-sync`. 요약은 Lemma L1의 전제(body가 관련 위치를 바꾸지 않음; 검사가 한 critical section 안이거나 원자적 읽기)가 성립할 때만 유효하다. M_impl에서 OSAL 조회를 담은 loop는 요약하지 않는다(조회가 lock을 누수할 수 있고, unlock broadcast가 대기자의 시도를 써 버린다). `OS_LOCK_MODE_NONE`의 OSAL 호출은 이런 뜻의 조회가 아니다. type lock을 잡지 않고, RESERVED record에서 기다리지 않고 바로 실패한다(`osapi-idmap.c:329, 460`). 그래서 `CFE_ES_LockSharedData`가 그런 token 변환으로 `OS_MutSemTake`를 부르는(`osapi-mutex.c:190`) `MainTaskSyncDelay`의 loop(`cfe_es_start.c:860–918`)는 두 M 모두에서 요약한다. σ_U∞에서는 요약의 timeout 전이와, 요약하지 않을 때 같은 loop의 예산 탈출이 모두 시간 기반 전이로 지워진다. 요약한 poller는 predicate가 결코 성립할 수 없으면 죽고, 요약하지 않은 loop는 묵은 실패 test 위에 좌초한다(§7.3). 그래서 죽음 보존이 L1 의무의 일부다 |
| **횟수 제한 retry** | build에서 **상수 평가한 bound**까지 정확히 펼친다(CF: 실효 `CF_STARTUP_SEM_MAX_RETRIES`; OSAL: 5회, `osapi-idmap.c:475–480`). 소진 갈래는 모든 σ에서 마지막 시도 뒤에만 닿는다. 반복 사이에 Φ delay가 있으면 **delay-paced**로 표시하고, 마지막 시도의 실패 결과는 시간 기반 전이다(§7.1). 그 시도가 M에서 전이 하나면(L1 요약, 또는 M_api의 CF 조회처럼 결과가 곧 test인 Φ 연산 하나) σ_U∞에서 성공할 수 있을 때만 허락된다. 여러 전이면(M_impl의 CF 조회) actor는 시도를 마친 뒤 좌초하고, 그 actor와 그것이 허락할 수 있는 actor의 죽음 판정은 `unknown(unmodelled)`다. bound가 상수가 아니면 `unknown(bound)` |
| **상수 횟수 loop** | 펼친다(core object table loop, startup script, record table scan) |
| **message loop**와 그 밖의 loop | `Sk^cert`에서는 star-closed(message loop는 `handler_begin/end`를 단 handler 자동자), `Sk^h`에서는 h까지 펼침 |

호출 문자열을 모두 inline하는 것은 최악의 경우 호출 깊이에 지수적이다. actor의 skeleton 구성이 work unit 상한(deviation)을 넘으면 원인 함수를 (actor, 함수)별 요약으로 바꾼다. 잃는 정밀도는 모두 건전한 쪽이다. MB가 작아지고, 공유 body의 노드는 once가 아니게 되며, R2 같은 certificate가 보류될 수 있다(spec §4.3; [X1-m7]).

### 8.3 Message provenance

`Sk^cert`가 handler를 어떻게 접는지 정한다 [D] (spec §4.3; [X1-M7] (a) 수정). pipe `p`마다 `Prov(p) = (Routes_B(p) ∩ Tx_B) ∪ Input_B(p)`다. `Routes_B(p)`는 도달 가능한 구독 지점이 `p`에 구독한 MsgId, `Tx_B`는 도달 가능한 송신 지점의 key(`⊤`이면 어디엔가 route된 모든 MsgId), `Input_B(p)`는 B.Input 중 `p`로 가는 MsgId다. 도달 가능성과 `Prov`는 서로 의존하므로 **최소 고정점**으로 계산한다. 최종 `Prov` 밖의 handler는 두 view 모두에서 접는다. 이것은 가정 A9(B.Input이 외부 event의 전부다) 아래에서 건전하다. litmus L-PROV는 명령으로만 닿는 handler가 B.Input이 비면 inactive, 명령 하나가 있으면 active가 되는지 본다.

### 8.4 그림 D10: 두 skeleton view

아래 그림은 handler H1, H2를 가진 message loop actor 하나를 두 view로 옮긴 모습이다. `Sk^cert`는 loop를 반복/탈출 선택으로 닫고, `Sk^h`는 h = 1, 2까지 펼친 뒤 `TRUNC`로 끝낸다. 정적 분석은 `Sk^cert`만, explorer와 O8은 `Sk^h`만 쓴다. `kz proofcheck`는 `Sk^h` 노드를 가리키는 certificate 단계를 거부한다.

**그림 D10. 두 skeleton view** (렌더링: [diagrams/D10_skeleton_views.svg](diagrams/D10_skeleton_views.svg); spec §12 D10; §4.3, §4.5, App. A.8)

```mermaid
flowchart LR
    %% D10 - two skeleton views of one actor. Source: spec §12 D10; §4.3, §4.5, §4.10, App. A.8 (litmus L-H2).
    %% Sk^cert: every unbounded loop star-closed (no horizon). Sk^h: loops unrolled to h, the rest marked TRUNC.

    subgraph CFG["actor CFG (one static actor, C code)"]
        A0["init prefix"]
        A1["message loop:<br/>ReceiveBuffer + dispatch"]
        H1["handler H1"]
        H2["handler H2<br/>(writer only reachable in<br/>handler iteration 2: L-H2)"]
        AX["exit"]
        A0 --> A1
        A1 --> H1
        A1 --> H2
        H1 --> A1
        H2 --> A1
        A1 --> AX
    end

    subgraph CERT["Sk^cert: certificate view"]
        C0["init prefix"]
        CH["loop head"]
        CC{"iterate / exit choice"}
        CD["dispatch folded by Prov(p)"]
        CH1["H1"]
        CH2["H2"]
        CX["exit"]
        C0 --> CH --> CC
        CC -->|"iterate"| CD
        CC -->|"exit"| CX
        CD --> CH1
        CD --> CH2
        CH1 -->|"back edge"| CH
        CH2 -->|"back edge"| CH
    end

    subgraph EXP1["Sk^h with h = 1"]
        E10["init prefix"]
        E11["iteration 1: H1 or H2"]
        E1T(["TRUNC"])
        E10 --> E11 --> E1T
    end

    subgraph EXP2["Sk^h with h = 2"]
        E20["init prefix"]
        E21["iteration 1: H1 or H2"]
        E22["iteration 2: H1 or H2"]
        E2T(["TRUNC"])
        E20 --> E21 --> E22 --> E2T
    end

    STA["MustBefore, certificates, inactive,<br/>provider none, once, PWB, K-CAP bound<br/>(Sk^cert only)"]
    EXPL["kz explore and O8<br/>(Sk^h; h recorded in b)"]
    PCK{"kz proofcheck<br/>view check"}
    REJ(["reject"])
    LIT["litmus L-H2 (App. A.8): provider present, not inactive,<br/>no certificate either way; h = 1: writer not reached<br/>(unknown(bound) on the good side); h = 2: schedule-dependent"]

    CFG --> CERT
    CFG --> EXP1
    CFG --> EXP2
    CERT -->|"kz-static static"| STA
    EXP1 --> EXPL
    EXP2 --> EXPL
    STA -->|"certificates"| PCK
    PCK -->|"a step names an Sk^h node or a TRUNC region"| REJ
    EXPL --- LIT

    classDef comp fill:#eef3fb,stroke:#3b5b92,color:#111
    classDef trunc fill:#fdeceb,stroke:#b23b3b,color:#111
    classDef dec fill:#fffbe6,stroke:#a07a00,color:#111
    classDef use fill:#eaf6ea,stroke:#2e7d32,color:#111
    classDef note fill:#ffffff,stroke:#999,color:#555
    class A0,A1,H1,H2,AX,C0,CH,CD,CH1,CH2,CX,E10,E11,E20,E21,E22 comp
    class E1T,E2T,REJ trunc
    class CC,PCK dec
    class STA,EXPL use
    class LIT note
```

**litmus L-H2로 읽기** (spec App. A.8, 기대 결과 [I]). looper의 M1 handler가 flag를 켜고, M2 handler는 flag가 켜졌을 때만 자원 R을 만든다. 다른 actor가 M1, M2를 차례로 보내고, 소비자가 R을 조회한다. R의 writer는 handler 반복 2에서만 닿는다.

- `Sk^h`(h = 1)만 보면 writer가 보이지 않는다. 여기서 provider `none`이나 `inactive`를 낸다면 거짓 주장이다.
- `Sk^cert`에서는 loop가 닫혀 있어 writer가 보인다. provider는 `present`이고 `inactive`가 아니다. 어느 방향의 certificate도 나오지 않는다.
- explorer는 h = 1에서 writer에 닿지 못한다. `Q_bad`는 bad witness를 준다. 그러나 cut 너머 looper 영역에 writer가 있으므로 `¬OK_i`는 safety 쪽 censoring으로 보류된다. 그래서 b = 1 안의 class는 bad witness를 기록한 `unknown(bound)`다(그림의 "good 쪽은 `unknown(bound)`"). h = 2에서는 소비자의 유일한 ordinal에서 schedule-dependent다.
- `kz proofcheck`는 h 너머 노드를 가리키도록 조작한 certificate를 거부해야 한다.

---

## 9. 순서 관계: strata, MustBefore, PWB, HB

### 9.1 HB 규칙과 세 stratum

**happens-before(HB)**는 한 실행 안에서 program order와 동기화(lock 해제 → 다음 획득, message 송신 → 수신, task 생성 → 자식 시작 등)로 정해지는 "먼저 일어남"의 부분 순서다(Lamport 1978 [V-abs]). Φ의 HB 규칙은 추론 규칙 `e₁ → e₂`이고, 각각 matching 조건, stratum, robustness tag, 코드 인용을 가진다(spec §1.9). stratum은 그 순서가 어느 범위의 실행에서 성립하는가에 따른 층이다 [D].

| stratum | 뜻 | 쓰임 |
|---|---|---|
| **S** (구조적) | 두 끝점을 모두 가진 **모든** 실행에서 성립한다: actor 안 program order(looper에서는 한 handler 안), task 생성 → 자식 시작, join, script 순서, 그리고 **guard된 관측 edge**(writer 집합이 완전하고, predicate를 코드에서 뽑았고, 다른 연속이 terminal뿐일 때) | MustBefore와 certificate |
| **O** (관측된 동기화) | 관측한 실행에서만 성립한다: mutex 해제 → 다음 획득, semaphore give → take, 전달된 queue/SB message, soft한 대안을 가진 poll 탈출, timer → callback, 결과가 함의하는 reads-from 등 | trace의 HB. 예측에서는 primitive 의미를 지키는 한 뒤집을 수 있다 |
| **C_σ** (스케줄 제약) | `P_sched` 아래 고정 우선순위 선점, affinity | σ_R1 실현 가능성에만, explorer에서만 쓴다. **결코 HB edge가 아니다**([R04 §4 row 20]; [R05 §2.7]) |

cFS 예(spec §3.16): R1 program order(S), P-TASK 생성 → 자식 첫 event(S), R4 mutex(O), R7 SB put → receive(O), **R19 core serialization(S, obs-fatal)**, SR 종료 요청 → teardown(S, obs-fatal). 우선순위, sleep, timeout된 barrier, `volatile`만으로 된 flag, handler 사이 program order는 edge를 **만들지 않는다**고 명시한다.

**looper 규칙** [D] ([X1-M7] (b) 수정). looper(`ReceiveBuffer`로 돌며 dispatch하는 actor)에서 program order는 한 handler 안, 그리고 init prefix에서 모든 handler로만 성립한다. **같은 looper의 두 handler 사이에는 HB edge가 없다**(CAFA PLDI 2014 [V-abs]; DCatch ASPLOS 2017 [P]). handler가 차례로 돈다는 사실은 예측의 제약으로만 남는다. handler는 겹치지 않고, 같은 pipe의 message는 FIFO 순서로 처리된다.

**robustness tag**(trace 쪽): `unconditional`(관측이 필요 없는 S), `obs-fatal`(terminal 탈출을 가진 관측, S), `obs-soft`(log 후 계속하는 탈출, O), `schedule-chosen`(lock, semaphore, message, O), `assumed`(가정 A4 아래에서만: volatile flag, seqlock 등).

### 9.2 MustBefore: "모든 실행에서 반드시 먼저"

> **rev 1.3부터 선택 경로** [D] (spec §4.4 머리; ADR-053; C-22). MustBefore 고정점(§9.2–§9.4)과 certificate·검사기(§10)는 보편 순서 진술이 필요할 때 쓴다(§11.7 고정 순서 근거). 관측 판정, 예측, 확인의 전제 조건이 아니고, R2도 그것을 요구하지 않는다. 다시 적용할 때는 보고서가 관계의 정의, 분석 영역, 전이 규칙의 단조성, 수렴 조건, 그리고 계산한 관계와 프로그램 순서 사이의 보존 논증을 준다. 고정점이 수렴했다는 것만으로 그 관계가 프로그램의 올바른 순서라는 뜻은 아니며, certificate는 언제나 그 전제, 모형 범위, 증명되지 않은 lemma와 함께 보고한다. 아래 정의는 바뀌지 않았다.

**정의** (spec §1.9, §4.4). `a ∈ MB(n)`은 `Pref(B,σ_U,M)`의 모든 유한 실행에서 `n`이 일어날 때마다 그 전에 `a`가 일어났다는 뜻이다. σ_U에서는 모든 유한 실행이 maximal 실행으로 이어지고 MB가 prefix에 닫혀 있으므로, 이것은 `Exec(B,σ_U,M)` 위의 관계와 같다. MB는 must 분석이다. 모든 실행에서 성립하는 순서만 결론으로 낸다. certificate view 위에서 **S 규칙만** 써서 M마다 계산하며, σ_R1의 우선순위 규칙은 넣지 않는다.

**최대 고정점(greatest fixpoint)** [D] ([A §4.4]; [J1-1], [J2-2], [X1-m2], [X2-01] 수정). MB는 아래 등식의 해 가운데 가장 큰 것이다.

- **기저**: 시작 actor(PSP main), 환경 근원, 모형 안에 등록 지점이 없는 비동기 문맥, 모형 안에 생성 지점이 없는 actor의 entry는 `∅`에서 시작한다. 나머지 노드는 `⊤`(모든 노드)에서 시작한다.
- **반복**: 규칙이 허락하지 않는 원소를 지우고, 더 지울 것이 없으면 멈춘다. 집합은 줄기만 하므로 반복은 끝난다.
- **왜 기저가 ∅인가**: 빈 교집합을 ⊤로 두면 "아무것도 앞서지 않는 노드 앞에 모든 것이 앞선다"는 거짓이 나온다. T-SPAWN의 빈 교집합도 ∅다.

| 규칙 | 등식 기여 | 근거 |
|---|---|---|
| **T-PO** | `in_PO(n) = ∩_{p ∈ pred(n)} (MB(p) ∪ {p})`. `Sk^cert`의 모든 선행 노드(back edge 포함)에 대한 교집합. terminal 연산에는 후속이 없다 | program order. terminal 갈래를 뺀 지배(dominance: 시작점에서 `n`에 이르는 모든 경로가 `a`를 지남) |
| **T-SPAWN** | actor entry에서 `∩_{c ∈ SpawnSites(τ)} (MB(c) ∪ {c})`. **모든** 생성 지점(callback은 등록 지점)에 대한 교집합. 해결되지 않은 생성 지점이 있으면 `∅` | `OS_TaskCreate` → 자식 시작 |
| **T-OBS** | 관측 탈출 `o`에서 `in_PO(o) ∪ ∩_{w ∈ W_o(MB)} (MB(w) ∪ {w})`. §9.3의 조건이 모두 성립할 때만 | reads-from: `f`와 `o` 사이의 어떤 writer가 조건을 참으로 만들었다 |
| **T-TERM** | terminal 연산에는 후속 edge가 없다. Φ는 PSP마다 인용과 함께 terminal 연산을 선언한다: pc-linux에서 `CFE_PSP_Panic`, `abort`, 그리고 **호출자에게만** `CFE_PSP_Restart`. 그 terminal 단계가 process를 끝내는 선언에는 `process_terminal` 속성을 붙인다(목록은 §7.3) | panic과 재시작 요청자의 끝(spec §3.5, §4.3) |
| (없음) | **lock 획득만으로는 must-order가 생기지 않는다** | lock은 원자성을 주지, 순서를 주지 않는다([R04 L5]) |

### 9.3 T-OBS와 writer 집합

T-OBS는 "기다리다 빠져나온 관측이 순서를 준다"는 규칙이고, R2의 core serialization(R19)이 이 규칙으로 들어온다. 재료는 다음과 같다(spec §4.2, §4.4).

**세 줄 요약.** (1) 어떤 thread가 조건이 참이 되기를 기다렸다가 빠져나왔다면, 그 조건을 참으로 만든 쓰기가 먼저 일어났다. (2) 그 결론은 조건을 참으로 만들 수 있는 쓰기(writer)를 **빠짐없이** 알 때만 믿을 수 있다. 하나라도 모르면(`DISABLED`) 규칙을 끈다. (3) 이미 순서가 증명된 writer, 다른 record에 쓰는 writer 등은 아래 네 제거 규칙으로 빼고, 남은 writer들이 모두 공유하는 선행 사건만 관측 앞에 둔다.

**writer 집합.** conjunct `C`의 지원 위치에 쓰는 store를 AST 수준에서 모두 찾고(S3a, 가정 A-TYPE 아래), 전이를 고려해 거른다. `W`는 `C`를 참으로 만들 **수 있는** store, `W^must`는 **반드시** 참으로 만드는 store, `D`(disabler)는 거짓으로 만들 수 있는 store다. 항목마다 상태가 둘 붙는다.

- **transition status**: `COMPUTED`(producer의 분석 단계가 확립했고 Φ가 동의함), `DECLARED_VERIFIED`(Φ guard를 분석이 확인), `DECLARED_TRUSTED`(분석이 배제하지 못한 site를 Φ guard가 배제; certificate마다 나열, O3/O4/O9로 반박 가능), `DISABLED`(타입 없는 쓰기, 탈출 주소, `unknown_effect`, 선언되지 않은 writer). `DISABLED`가 있으면 그 conjunct에서 T-OBS를 끄고 관련 instance는 `unknown(writer_incomplete)`다.
- **key status**: `MUST_KEY`(같은 상수, 같은 symbolic 값, 같은 할당 기록), `MAY_KEY`(같을 수 있음), `NO_KEY`(다름이 증명됨). `NO_KEY` 항목은 instance의 writer 집합에서 뺀다.

**INIT_FALSE와 falsifier.** `INIT_FALSE(P_j)`는 conjunct가 INIT에서 거짓이라는 뜻이다. **falsifier** `f`는 관측 `o`의 actor 안에서 `o`를 지배하고(terminal 갈래 제외), 관측되는 conjunct를 그 key에 대해 **확실히** 거짓으로 만드는 event다. 예: `cfe_es_start.c:803`의 `SetUsed(ρ(f), RESERVED)`는 `ρ(f)`를 사용 중으로 만들지만 그 `AppState`는 아직 UNDEFINED다.

**적용 조건.** `W_C`에 `DISABLED`가 없고, 코드에서 뽑은 `Pred_code`가 `C`를 함의하며, `f` 직후(falsifier가 없으면 INIT에서) `C`가 거짓이어야 한다.

**관측의 writer 집합 `W_o(MB)`.** `W_C`의 모든 writer site의 모든 문맥 노드에서, 아래 네 규칙으로 제거되는 것만 뺀다. `kz proofcheck`는 각 제거를 그 입력에 상대적으로 다시 확인한다. OWN-NOPATH와 PWB 규칙은 `Sk^cert` 그래프 위의 자기 도달성으로 본다. KEY-SEP은 항목의 `NO_KEY` status가 writer 절과 일관한지로 본다. status 자체는 추출된 사실이고 A-FRESH에 기댈 수 있다(§10.7).

| 제거 규칙 | 조건 | 쉬운 말 |
|---|---|---|
| **OWN-NOPATH** ([X2-01] 수정) | `w`가 `o`와 같은 actor에 있고, `once(actor(o))`이고, `Sk^cert`에 `w →* o` 경로가 없다 | 그 actor의 유일한 실행에서 `w`가 `f`와 `o` 사이에 일어났다면 skeleton에 `w`에서 `o`로 가는 경로가 있어야 한다 |
| **PWB-before-falsifier** | `PWB_MB(w, f)` | `w`는 언제나 `f`보다 먼저다 |
| **PWB-after-observation** | `PWB_MB(o, w)` | `w`는 언제나 `o`보다 뒤다 |
| **KEY-SEP** | `w`의 key status가 `NO_KEY` | 다른 record에 쓴다 |

`W_o`는 **반복마다** 현재 MB로 다시 계산한다. MB가 줄면 PWB 사실이 줄고, `W_o`가 커지고, 교집합은 작아진다. 그래서 연산자는 단조이고 ⊤에서 시작한 반복은 최대 고정점으로 수렴한다(spec §4.4).

### 9.4 PWB: "둘 다 일어나면 언제나 a가 먼저"

**정의** (spec §1.9). `PWB(a, b)`는 `a`와 `b`를 모두 담은 `Pref(B,σ_U,M)`의 모든 유한 실행에서 `a`의 모든 발생이 `b`의 모든 발생보다 앞선다는 뜻이다. σ_U에서는 `Exec` 위의 관계와 같다.

**왜 MB만으로는 부족한가.** MB(n)은 "n 앞에 무엇이 있는가"만 말한다. R2에서 보여야 할 것은 반대 방향이다. 사용 `c`가 writer `x`보다 **언제나 먼저**라는 것이다. MB는 "모든 `x` 앞에 어떤 지점 `m`이 있다"를 준다. 여기서 "모든 `c`가 `m`보다 먼저"를 끌어내려면 `m`이 `c` 앞에 올 수 없다는 논증이 필요하다. PWB 규칙 둘이 그 논증이다 [D] (spec §4.5; [X1-M2] 수정). `Anc(x)`는 `x`의 actor에서 `Sk^cert` 경로로 `x`에 닿는 노드의 집합(`x` 포함)이다.

| 규칙 | 조건 | 결론 | lemma |
|---|---|---|---|
| **FRONTIER** | `once(actor(a))`이고, `a`의 actor에 `m`이 있어 `m ∉ Anc(a)`, `m ∈ MB(b)` | `PWB(a, b)` | L-PWB |
| **ONCE-PWB** | `once(a)`이고 `a ∈ MB(b)` | `PWB(a, b)` | L-ONCE |

- **FRONTIER를 쉬운 말로.** `m`은 `b`가 일어날 때마다 그 전에 일어난다(MB). `m`에서 `a`로 가는 경로는 없다. 만약 `a`가 `m` 뒤에 일어난 적이 있다면 그 actor의 **유일한** 실행에 `m … a`가 있어야 하고, A-SKEL에 따라 skeleton에도 `m →* a` 경로가 있어야 한다. 그런 경로가 없으므로 `a`는 언제나 `m`보다 먼저이고, 따라서 `b`보다 먼저다. once가 없으면 `m`과 `a`가 다른 instance에 있을 수 있어 논증이 깨진다.
- **ONCE-PWB를 쉬운 말로.** `a`는 모든 `b` 앞에 있고(MB), 많아야 한 번 일어난다. 그러면 그 한 번이 모든 `b`보다 앞선다.

### 9.5 Trace의 HB와 vector clock

trace π의 `HB_π`는 S edge와 O edge(looper 규칙 적용)의 이행적 폐포이고, **vector clock**으로 계산한다(Fidge 1988; Mattern 1989 [V-abs]). vector clock은 actor마다 "지금까지 아는 각 actor의 진행 정도"를 정수 벡터로 들고 다니며, 동기화 edge를 따라 벡터를 합쳐 HB를 계산하는 자료 구조다. event마다 시계가 둘이다. `VC`는 확정된 모든 edge, `VC_R`은 `unconditional`·`obs-fatal` edge만 쓴다. `a ≺_R b`는 정적 certificate를 요청할지 고르는 값싼 prefilter일 뿐이고, 실행 집합 수준의 주장이 결코 아니다(spec §1.9; [J1 X5]).

### 9.6 모형에 없는 규칙

Φ에 없는 규칙은 **unmodelled**로 나열한다. predicate를 뽑지 못한 poll loop나 Φ 규칙이 없는 앱 전용 protocol(예: FM의 parent–child queue, `fm_child.c:65–100, 162`)은 `unmodelled-sync`가 되고, 그것에 결정이 달린 instance는 `unknown(unmodelled)`다. ordering 장치 하나를 빠뜨린 HB 모형은 거짓 경보를 쏟아 낸다는 KCSAN의 교훈을 따른 것이다(`kcsan.rst` L366–373 [V-doc], [R04 §2.4]).

---

## 10. R2로 읽는 once, MustBefore, PWB, certificate

spec App. A.1의 R2 certificate를 위의 개념으로 따라간다. 모두 **기대 동작**이며 실행한 것은 없다 [I]. 코드 위치는 [F]다. 이 절은 형식 유도(MustBefore 사슬, FRONTIER, 검사기)를 맡는다. cFS 사실이 certificate의 어느 재료가 되는지와 그림 D8(렌더링: [diagrams/D8_r2_certificate_overview.svg](diagrams/D8_r2_certificate_overview.svg))은 [05_cfs_mapping.md §7](05_cfs_mapping.md#7-r2-certificate-예-그림-d8)에 있다.

> **rev 1.3** [D] (ADR-053, ADR-057; spec App. A.1 머리; C-22, C-49). 아래 R2 certificate는 "도달한 모든 발생이 VIOLATING"이라는 보편 진술을 위한 선택 경로이고, 논문이 그 진술을 할 때만 쓴다. 그것이 없으면 R2의 칸은 "스케줄 의존성 미확정"으로 읽는다(§11.7). R2는 관측 경로의 기반 검증(rung V1′, spec §9.3)이고 기여의 증거가 아니다(§12.5 항목 58). R2의 관측 판정 기대 결과 [I]는 [03_workflow.md](03_workflow.md) §1.10에 있다. 이 절에서 certificate를 R2의 첫 결과나 출구 조건으로 적은 문장(§10.4의 증명 상태, §10.7 끝의 S3a gate)은 이 우선순위로 읽는다(spec §0.12).

### 10.1 증명할 것

- 사용: ES와 EVS의 `*_TaskInit` 안에 inline된 SB event site. 각 site의 capture point `c`는 `CFE_SB_Global.AppId`를 읽는 load다.
- value conjunct `V`: "읽은 AppId가 유효하다". `INIT_FALSE(V)`다(§6).
- writer: `V`를 참으로 만들 수 있는 것은 SB task의 writer `x`(`cfe_sb_task.c:138`, `GetAppID`의 성공 갈래 out-pointer store, `COMPUTED`, `MUST_KEY`)뿐이다. 실패 갈래의 `UNDEFINED` store와 EarlyInit memset은 중립이다. A-TYPE 검사가 타입 없는 쓰기나 탈출 주소를 찾으면 writer 집합이 `DISABLED`이고 certificate는 보류된다(spec App. A.1 step 3).
- 목표: **모든 유한 실행에서 모든 `c`가 모든 `x`보다 먼저**, 즉 `PWB(c, x)`다. 그러면 `V`는 도달한 모든 발생의 capture point에서 거짓이다. 그래서 모든 발생은 호출에서 결정되고, `Allowed = none`이므로 `VIOLATING`이다. 이것이 결정적인 safety 진술 `¬OK_i`, 즉 class `fixed-order`다(§11.1).

data-race 검출기는 여기서 할 말이 없다. 기존 gdb 기록에서 준비 상태 위반 사용은 HB상 발행보다 **앞서도록** 순서가 정해져 있었다. 순서 없는 두 접근을 찾는 기법은 이것을 보고하지 않는다(spec §1.11). Kairos는 "순서는 있는데 방향이 틀렸다"를 모든 실행에 대해 증명하려 한다.

### 10.2 MustBefore 사슬

R19(core serialization, `cfe_es_start.c:776–847`)는 startup thread가 core 항목마다 하는 일을 적는다(spec §3.6): 예약(`:803`, falsifier `f`, 할당 기록 `ρ(f)`) → `StartAppTask`(`:810`) → 발행(`:818`) 또는 실패 시 `memset`(`:823`, 뒤에는 panic뿐) → `MainTaskSyncDelay(RUNNING, …)`(`:835`, 관측 `o`) → 실패하면 `CFE_PSP_Panic`. 관측의 predicate는 코드에서 뽑는다: `Pred_code = ∀ r ∈ AppTable: ¬(IsUsed(r) ∧ r.AppState < RUNNING)`.

ES 쪽 사슬(spec App. A.1 steps 4, 6):

| 단계 | 사실 | 규칙 |
|---|---|---|
| 1 | `m_ES`(`CFE_ES_TaskInit` 호출의 반환, `cfe_es_task.c:114`)는 ES main의 raise(`cfe_es_task.c:134`) 앞이다: `m_ES ∈ MB(raise_ES)` | T-PO |
| 2 | `o_ES`(ES 반복의 `:835` 성공 탈출)의 writer 집합에서 남는 것은 raise다. `ExitApp`의 STOPPED store는 남든 빠지든 raise와 `m_ES` 뒤다. 나머지는 아래 표처럼 제거된다. 결과: `m_ES ∈ MB(o_ES)`, `raise_ES ∈ MB(o_ES)` | T-OBS |
| 3 | `o_ES`에서 startup thread를 따라 SB 반복의 생성 지점 `spawn_SB`까지. panic 갈래는 후속이 없고, `:823`을 거치는 실패 갈래는 panic으로만 간다 | T-PO, T-TERM |
| 4 | SB main의 생성 지점은 하나이고 `once(SB main)`이다. SB entry 앞에 `spawn_SB`가 있다 | T-SPAWN |
| 5 | SB entry에서 `x`까지 | T-PO |
| 결론 | `m_ES ∈ MB(x)` | |

`W_o(o_ES)`의 처리(spec App. A.1 step 4). 네 제거 규칙이 모두 쓰인다.

| writer | 처리 | status |
|---|---|---|
| ES main의 raise | 남김. `m_ES ∈ MB` (T-PO) | `COMPUTED` |
| `ExitApp` STOPPED (`cfe_es_api.c:399`) | 남기든 빼든 raise와 `m_ES` 뒤 | — |
| scan WAITING (`cfe_es_apps.c:1144`) | 중립(같은 record guard) | `COMPUTED` |
| `CleanUpApp` record 해제 | 제외(관계 guard: `AppState > RUNNING`인 외부 앱만) | **`DECLARED_TRUSTED`** |
| 실패 시 `memset` (`:823`, ES 반복) | **OWN-NOPATH**(뒤에 panic만) | `COMPUTED` |
| EarlyInit `memset` (`cfe_es_start.c:82`) | **ONCE-PWB**(w, `f_ES`) | `COMPUTED` |
| startup thread의 나중 writer(이후 반복의 `:823`, script 예약 등) | OWN-NOPATH(core loop를 펼쳤으므로 `o_ES`로 돌아오는 경로가 없다) | `COMPUTED` |
| EVS, SB, TBL, TIME, script 앱과 그 자식의 writer | **FRONTIER**: `PWB(o_ES, w)`. 모두 `o_ES` 뒤에 생성된다 | `COMPUTED` |

**EVS 쪽**도 같은 모양이다. `m_EVS`(`cfe_evs_task.c:213`)가 `o_EVS`를 거쳐 `x` 앞에 온다. 생성된 core 순서에서 EVS 반복이 SB 반복보다 앞이기 때문이다([R02 §1.3]). 눈여겨볼 점은 셋이다(spec App. A.1 step 5).
- ES main의 raise는 **ONCE-PWB**로 빠진다. `once(raise_ES)`이고 `raise_ES ∈ MB(o_ES) ⊆ MB(f_EVS)`이므로 `PWB(raise_ES, f_EVS)`다. ES의 raise는 EVS record가 예약되기 전에 이미 끝난다.
- ES main의 `ExitApp` STOPPED store는 A-OWNER 아래에서 빠진다(owner record `ρ_ES`는 `Type = CORE`이고 store의 guard는 `Type != CORE`). 그래서 `DECLARED_TRUSTED`(A-OWNER)다.
- `f_EVS` 직후 `AppState(ρ_EVS) = 0`이라는 사실 자체가 A-FRESH와 A-OWNER를 필요로 한다. ES 쪽 `f_ES`에는 필요 없다. 그 전에는 task가 없고, 앞설 수 있는 `AppState` writer는 `:82`의 memset뿐이다.

### 10.3 Frontier와 once로 PWB(c, x)를 얻는다

(spec App. A.1 step 7; FIXED_ORDER certificate 항목 4, spec §4.5)

- `once(ES main)`(§1.2).
- `m_ES ∉ Anc(c)`. `CFE_ES_TaskInit`은 message loop 전에 한 번 불리고, `Sk^cert`에서 loop의 back edge는 `m_ES` 뒤의 loop head로 돌아간다. 그래서 `m_ES`에서 `c`로 가는 경로가 없다.
- `m_ES ∈ MB(x)`(§10.2).
- FRONTIER(L-PWB)에 따라 `PWB(c, x)`. 모든 `c`가 모든 `x`보다 먼저다.

**early return은 상관없다.** `CFE_ES_TaskInit`은 실패하면 `cfe_es_task.c:340, 371, 381, 393`에서 돌아오지만 `CFE_ES_TaskMain`은 `cfe_es_task.c:134`에서 raise한다. 이 증명은 사용이 raise를 지배할 것을 요구하지 않는다. 이전 판의 지배 기반 FIXED_ORDER가 바로 이 early return에서 실패했다([J2-1]; spec §0.4).

### 10.4 결론, 신뢰 항목, 출처 회계

(spec App. A.1 steps 9–10, 14)

- **판정(σ_U).** `fixed-order(σ_U, M_api)`와 `fixed-order(σ_U, M_impl)`. b = ∞의 safety 진술 `¬OK_i`이고, `Allowed = none`이 발생에서 결정하므로 결정적이다(§11.1). L-FO에 따라 범위 안에서 도달한 모든 발생은 capture point에서 `V`가 거짓이므로 호출에서 결정되어 `VIOLATING`이다. certificate에 task 생성(M_impl-relevant)이 있으므로 M_impl certificate는 M_impl skeleton에서 따로 계산한다. 거기서도 T-SPAWN이 성립한다.
- **template과 ordinal.** ES 문맥의 `SEND_NO_SUBS` 사용은 여러 template에서 나온다. version-event loop template은 reference version table이 있을 때 boot마다 ordinal 13개를 가진다(`CFE_Config_IterateAll` loop의 module version event마다 하나, `cfe_config_get.c:180–187`; `cfe_es_task.c:235–241, 265, 466`; [R2g §7]). INIT_INF, INITSTATS, Mission-version, BUILD_INF event(EID 1, 2, 91, 92; `cfe_es_task.c:442, 449, 259, 287`)는 ordinal 하나씩인 template 넷이다. 추출기가 loop의 간접 callback 호출(`cfe_config_get.c:184`)을 같은 호출 문자열로 푸는지는 [U]다. frontier가 모든 template의 모든 발생을 `cfe_sb_task.c:138`의 store 앞에 두므로 ordinal-split은 없다.
- **σ_U∞와 σ_R1.** 전이 규칙 1(a)로 같은 진술을 가진다(같은 M, b = ∞, flag `SAFETY_TRANSFERRED`). 결정에서의 REJECT는 시간 기반 전이가 아니므로 계속 결정적이다.
- **reach(칸마다).**
  - (σ_U, M_impl): S1이 발생을 담은 실행을 보이면 `proved`다(L-CONT (c)).
  - (σ_U, M_api): cone에 task 생성이 있어 L-MLEVEL을 쓸 수 없다. 발생까지 닫힌 cone의 모든 actor prefix가 M_api 적합성 검사(`OS_ERR_INCORRECT_OBJ_STATE` 없음, 잃은 task 없음, 누수된 type lock 없음)를 통과할 때만 `proved`다(L-MAPI-CONE; spec §4.7 item 6). 아니면 `assumed`다.
  - σ_U∞(두 M): 더하여 닫힌 cone actor들의 prefix 전이에 시간 기반 전이가 없어야 한다(L-REACH-CONE). soft barrier·poll timeout, 소진된 지연 retry, M_impl에서 OSAL 내부 timed wake가 없어야 한다. ES, EVS, SB 자식의 `OS_TaskPrepare` self 조회가 그런 wake를 탈 수 있고(spec §3.3), prefix probe만 그것을 보인다. 아니면 `assumed`다.
  - σ_R1: `assumed`.
- **σ_E.** M_impl에서 전이 규칙 2로 얻는다: 모든 유한 실제 실행에서 판정된 범위 안 발생 중 `READY`나 `TOLERATED`인 것이 없다(`¬OK_i`, 결정적). `NOT_EVALUATED` 발생은 허용된다. S1이 판정되고 평가된 발생을 다시 관측하면 reach가 `proved`다(L-CONT (c)를 거쳐서. 신뢰 장부 (iii)부, §10.7).
- **progress.** 모든 칸에서 `unknown(not_checked)`다. certificate는 progress를 주장하지 않는다. explorer를 돌리면, M_api σ_U∞에서 이 instance의 stall 상태는 `CFE_ES_MainTaskSyncDelay`(`cfe_es_start.c:860–918`)에 L1을 적용할 때 건너뛴 core raise 뒤에서만 예상되고, App. A.2 뿌리(`tml-root`)에 사슬로 붙는다. M_impl에서는 F7로 잃은 core task 뒤에서도 생긴다(`OS_TaskEntryPoint`가 실패한 `OS_TaskPrepare` 뒤 `OS_TaskExit`을 부른다, `osapi-task.c:128–137`). 그때는 그 task의 K-SPAWN.osal 발생에 사슬로 붙는다. 이것들은 R2의 발견이 아니고, 기본 B에 상대적이다 [I].
- **근거.** realization은 `certificate`이고, 여기에 pool key를 가진 `realized/natural` 관측 `n of N`이 더해진다.
- **shape.** SB가 witness나 그 기록된 실행에서 나중에 발행하는 곳에서 WRONG-WAY다. SB 자신의 구간에 걸린 곳에서는 ABSENT다. `cfe_sb_task.c:138`의 SB `GetAppID`는 once 노드이고 그 실패 갈래를 이미 지났으므로, witness의 어떤 연장에도 성공 store가 없다(§11.2). 둘 다 bad다.
- **신뢰 항목.** certificate마다 네 부분의 신뢰 장부를 지닌다(§10.7, 보장 G10). 그 (iii)부의 `DECLARED_TRUSTED` 항목은 ES 쪽이 `CleanUpApp` 관계 guard, EVS 쪽이 A-FRESH, A-OWNER, `CleanUpApp` guard다. rendered label `trusted:<n>`(sub-label이 아니다, §11.5)이 이것, 즉 (iii)부에서 종류가 `DECLARED_TRUSTED`인 항목을 센다. 모두 O9와 O3/O4로 반박할 수 있고, 그것들로 증명되지는 않는다. writer 목록과 그림 D8 상세는 [05_cfs_mapping.md §7.4](05_cfs_mapping.md#74-writer-목록과-신뢰-항목-그림-d8-상세)에 있다.
- **증명 상태.** 이 certificate 칸들이 출력하는 상태는 쓰는 lemma 중 가장 약한 것이다(§12.3, PS3). R2의 certificate는 L-MB, L-ONCE, L-PWB, L-FO, L1, L-STATUS가 `unproved`이므로 모든 칸이 "lemma status: unproved"를 출력한다(spec App. A.1 step 9a). 그래서 첫 R2 결과는 "`kz proofcheck`가 신뢰 기반과 이 lemma들에 상대적으로 받아들였다"로 쓰고, V0-P가 lemma를 올리기 전에는 "증명됨"이나 "검증됨"이라 쓰지 않는다(PS2, ADR-045). (rev 1.3: R2의 certificate는 선택 경로이므로, 여기서 "첫 R2 결과"는 certificate를 낼 때의 R2 certificate 결과를 뜻한다. ADR-053, ADR-057.) 상태표는 [../plan/proof_status_R2.md](../plan/proof_status_R2.md)에 있다.
- **출처 회계.** "generic 준비 상태 명세 K-ID + framework 규칙 R19". R19는 `:835`의 loop, 그 stratum, terminal 탈출을 묶는다. predicate는 코드에서 뽑고, `kz proofcheck`가 추출기의 직렬화된 `:835` exit tree에서 다시 유도한다(신뢰 장부 (ii)부). R19는 사례를 아는 상태에서 쓴 framework 규칙이고, 판정은 이것을 밝힌다(spec §7.2).
- **하지 않는 주장**(step 14): R2가 race라는 것; S1 전의 개수를 Kairos 결과로 내는 것; 반복 실행에서 결정성을 끌어내는 것; certificate가 framework 규칙을 쓰지 않는다는 것; 모든 실행에서 사용에 도달한다는 것(reach는 별도 축); 어떤 progress 진술; 검사기가 추출된 사실이나 skeleton을 검증했다는 것; lemma의 증명 상태가 `proved`가 아닌데 "증명됨"이나 "검증됨"이라 쓰는 것(spec §6.4 항목 45). 기대하는 사용 집합(step 12)과 사슬(step 13)은 [05_cfs_mapping.md §7.5](05_cfs_mapping.md#75-그-밖에-이-예가-보여-주는-것)에 있다. 사슬의 세 판단(step 13a)은 §5.8, 교정 순서 대조군의 기대 기록(step 15)은 spec App. A.1에 있다.

### 10.5 Certificate와 독립 검사기

**certificate**는 판정의 safety 진술 하나(§11.1)를 다른 프로그램이 한 단계씩 다시 확인할 수 있게 적은 증명 기록이다. FIXED_ORDER certificate는 INIT_FALSE 증명, (value conjunct이면) `derives` 사실, transition·key status가 붙은 `W_j`, 사용한 once 사실, frontier 노드와 no-path 증명(또는 ONCE-PWB 사실), writer마다 MB 유도(T-OBS의 writer 제외 포함), badness 증명, reach를 담는다(spec §4.5). badness 증명은 기대는 REJECT event와, 그것이 발생의 결정 event에 있는지를 기록한다(§10.6). certificate는 증명 상태도 지닌다(`KZ_CERT.proof_status`, spec §4.5). 자기 칸의 장부 (iii)부 `LEMMA` 항목 중 가장 약한 상태이고, `kz proofcheck`가 certificate를 받아들일 때 쓴다. 받아들였다고 올라가지는 않는다.

**`kz proofcheck`: 신뢰 기반에 상대적인 독립 추론** [D] (spec §4.5; EXT-20261012 §4.3; ADR-041).
- producer(`kz-static static`)와 분석 코드를 공유하지 않고 LLVM도 link하지 않는 별도 module이다.
- 독립성은 **추론**의 독립이지 **사실**의 독립이 아니다. 공유 `libkz-core`(T5)의 evaluator나 rule engine을 부르는 단계에서는 이 독립성도 성립하지 않는다. T5의 container I/O와 Φ parser로 입력을 읽는 것만으로는 단계가 자기 것(own)으로 남는다.
- evaluator나 rule engine을 부르는 단계는 신뢰 장부에 "via T5"로 표시한다.
- "`kz proofcheck`가 받아들였다"는 신뢰 기반에 상대적으로 추론 단계를 다시 확인했다는 뜻이다. 늘 certificate의 증명 상태(§12.3, PS3)와 함께 보고하고 "증명됨"이라 쓰지 않는다.

**신뢰 기반** (모든 certificate의 신뢰 장부에 기록한다)

| 기반 | 내용 | 가정 |
|---|---|---|
| T1 | `facts.kz`: AST 사실, `derives` 복사 사슬, writer 절(W, W^must, D와 transition·key status, 전이 filter 분류), ALLOCS, A-TYPE 목록, mod set·lock 영역을 가진 직렬화된 loop-exit 식, INIT image, 상수 평가 | A-EXTRACT |
| T2 | `skel.kz`와 `anchors.kz`: `skel`이 `link`의 IR call graph에서 만든 `Sk^cert` 그래프(inline·문맥 구분, 정확한 펼침, star-closure, poll loop 인식, message provenance, `TRUNC` 표시). 검사기의 지배·도달·once routine은 이 그래프 위에서 돌고, 검사기는 그래프를 다시 만들지 않는다 | A-SKEL |
| T3 | closure의 spawn site 목록 | A3 |
| T4 | Φ: 규칙, effect, binding, LP, T-TERM 선언, 준비 상태 명세와 Allowed 항목 | A1 |
| T5 | producer와 공유하는 `libkz-core`: Φ parser, core-expression evaluator, rule engine, container I/O. 모든 단계가 입력을 container I/O와 Φ parser로 읽는다 | A-CORE |
| T6 | reach 단계가 가리키는 실행, `hb-*.kz`, prefix probe 산출물 | A2, A6 |

**다시 확인하는 단계** (T1–T6에 상대적; "own"은 검사기 자신의 routine, "via T5"는 T5의 evaluator나 rule engine도 부름)

| 단계 | 확인 | 종류 |
|---|---|---|
| view | 모든 노드가 `Sk^cert` 노드다. `Sk^h`나 `TRUNC` 노드면 거부한다([X1-C1] 수정) | own |
| T-PO | terminal 연산을 법으로 한 지배. 자기 지배 routine | own |
| T-SPAWN | spawn site 목록이 closure 목록과 같다. 기저는 ∅ | own |
| T-OBS falsifier | "f 직후 C가 거짓"을 추상 사후 상태에서 공유 evaluator로 평가 | via T5 |
| T-OBS predicate | 직렬화된 loop-exit 식에서 `Pred_code`를 자기 symbolic evaluator로 다시 유도한다. 추출된 mod set·lock 영역에서 L1 전제를 확인한다. `Pred_code ⇒ C`를 자기 절차로 판정한다. C의 구문 트리만 공유 Φ parser에서 온다([X1-M8] 수정) | own |
| T-OBS `W_o` | writer 절에서 다시 계산하고 각 제외를 확인한다: OWN-NOPATH는 자기 도달성, PWB, KEY-SEP은 status 일관성 | own |
| ONCE | 생성 지점 영역과 쌍별 배타성. 자기 도달성([X1-M2] 수정) | own |
| DERIVES | 직렬화된 AST 사실에서 호출 인자부터 `loc`까지의 복사 사슬([X1-M1] 수정) | own |
| FRONTIER | `m →* ℓ_j` 경로가 없다 | own |
| SCOPE | `SHUTDOWN_REQ ∈ MB(d)`를 다시 유도한다. 그 사용은 L-SCOPE에 기대고, 장부 (iii)부에 나열한다 | own |
| T-TERM | Φ가 이 PSP에서 그 연산을 인용과 함께 terminal로 선언했다. 선언은 T4에 속하고, 참인지는 확인하지 않는다 | own |
| INIT | INIT_FALSE(FIXED_ORDER)와 INIT 참(PROTECTED (a))을 T1의 INIT image 위에서 공유 evaluator로 평가 | via T5 |
| badness | horizon 안 모든 연속의 `Sk^cert` 경로 검사(§5.2). `retry_until_ready`처럼 같은 instance의 READY 발생이 필요한 ACCEPT는 certificate의 INIT_FALSE와 frontier(또는 once-PWB) 사실로 막는다. 기대는 REJECT event와 그것이 결정에 있는지를 기록한다. 결정에 있는 REJECT만 decision route를 준다(§11.1) | own |
| status | transition·key status가 writer 절과 일관한지 본다. 일관성 검사이고 status를 다시 유도하지 않는다. `DECLARED_TRUSTED` 목록이 완전한지, provider 값이 `W_j`와 INIT_FALSE에 맞는지도 본다 | own |
| reach | T6의 실행이 있고 발생을 담으며 칸의 조건(§11.2)을 만족하는지 본다. L-MLEVEL 없는 M_api면 닫힌 cone actor prefix의 M_api 적합성, 유도된 σ_U∞ 칸이면 cone actor에 시간 기반 전이(M_impl의 OSAL 내부 timed wake 포함)가 없음 | own |

- rule engine은 단계가 가리키는 Φ 규칙 instance를 푸는 데만 쓴다(예: T-OBS가 쓰는 R19의 binding, stratum, terminal 탈출). 그 풀이는 따로 "via T5"로 표시한 장부 항목이다 [I]. 단계별 T5 호출 목록은 S0에서 정한다.
- **다시 확인하지 않는 것**(T1–T5에서 믿는 것): `W_j`의 완전성(A-WS), 전이 filter 분류, INIT image, `Sk^cert` 그래프, closure 목록, Φ, 공유 `libkz-core`. 추출 오류는 검사기가 닿지 않는다. 그것을 막는 것은 O1, O2, O4, O5, O7, O9, anchor audit, litmus suite, spec §8.5의 program mutant다.

**certificate-mutation test**는 올바른 certificate를 일부러 망가뜨려 검사기가 모두 거부하는지 본다(spec §4.5). 흔드는 방법은 다음과 같다: 생성 지점 빼기; writer 지우기; `m`을 `Anc(ℓ_j)`의 노드로 바꾸기; terminal이 아닌 연산을 terminal로 표시하기; COMPUTED를 DISABLED로 바꾸기; once 사실 빼기; `cfe_es_start.c:823`을 OWN-NOPATH 증명 없이 `W_o`에 다시 넣기; 단계를 `TRUNC` 영역으로 옮기기; 추출한 predicate를 더 강한 것으로 바꾸기(RESERVED도 사용 중임을 잊은 것); `derives` 빼기; `MAY_KEY`를 `MUST_KEY`로 바꾸기; INIT_FALSE 뒤집기; badness 단계에서 `Allowed none`을 허용 항목으로 바꾸거나 ACCEPT event에 닿는 연속을 적기; `retry_until_ready`의 뒤 READY 시도를 막는 INIT_FALSE나 frontier 사실 빼기; 결정 뒤의 REJECT event(예: retry 소진)를 decision route에서 결정적이라고 표시하기; cone actor prefix에 timeout 결과가 있는 실행으로 σ_U∞ reach를 주장하기; 사용 actor가 아닌 cone actor가 `OS_ERR_INCORRECT_OBJ_STATE`를 받았거나 type lock을 남긴 실행으로 M_api reach를 주장하기.
- 모든 mutant를 거부해야 하고, kill rate는 검사기 지표로 보고한다.
- 이 test는 검사기를 시험한다. certificate만 흔들므로 `facts.kz`, `skel.kz`, `libkz-core`의 오류는 잡지 못한다. 그것은 O1/O2/O7, anchor audit, evaluator agreement property test, litmus suite, spec §8.5 item 5의 program mutant가 맡는다.
- R2의 S3a 출구 mutation(spec App. A.1 step 11): `m_ES`를 `Anc(c)` 안 노드로 바꾸기, `once(ES main)` 지우기, `raise_ES`의 ONCE-PWB 지우기, `:823`을 OWN-NOPATH 증명 없이 `W_o(o_ES)`에 다시 넣기, predicate를 "RESERVED도 사용 중"을 잊은 것으로 바꾸기, `derives` 지우기, 단계 하나를 `TRUNC` 영역으로 옮기기.

### 10.6 PROTECTED, INACTIVE, provider, SCOPE

(spec §4.5, App. A.7)

- **PROTECTED certificate.** 모든 conjunct `P_j`에 대해 다음 중 하나가 성립해야 한다. (a) `P_j`가 INIT에서 참이고 모든 disabler가 NO_KEY·SCOPE로 제거되었거나 MB상 도달 불가다. (b) `MUST_KEY`이고 `COMPUTED`/`DECLARED_VERIFIED`인 must-writer `x*`가 `MB(ℓ_j)`에 있고, 남은 disabler `d`마다 `PWB(d, x*)` 또는 `PWB(ℓ_j, d)`가 성립하거나 NO_KEY·SCOPE로 제거된다. `DISABLED` writer가 있으면 안 된다.
- **PROTECTED가 증명하는 것**(L-PR). `TS(B,σ_U,M)`의 모든 유한 실행에서, 범위 안 발생의 모든 conjunct는 그 LP가 발생의 결정 이전에 있을 때 성립한다. 그래서 결정된 범위 안 발생은 모두 `READY`나 `NOT_EVALUATED`다. 이것이 진술 `¬NR_i`이고, `¬Bad_i`를 함의한다.
  - 증명하지 않는 것: 사용에 도달하지 않는 실행(deadlock, 영원한 대기, 잃은 provider나 spawn), reach, horizon.
  - 판정의 progress 값은 `exh`가 정하지 않으면 `unknown(not_checked)`다.
  - certificate가 READY를 증명하므로 refinement `allowed-not-ready`를 결코 뒷받침하지 않는다. `allowed-not-ready`는 `exh`에서만 나온다. certificate에 맞서 결정된 not-ready 발생(TOLERATED, PENDING 포함)이 나오면, 실행에서 나왔으면 모순이고 `exh`에서 나왔으면 O10 불일치다(§11.1).
- **FIXED_ORDER의 badness(item 5)와 decision route.** horizon 안의 모든 `Sk^cert` 연속에서 사용의 not-ready 결과가 Allowed 밖이어야 한다. 즉 항목의 monitor가 결코 ACCEPT하지 않는다.
  - 제어 event인 ACCEPT(정상 연속, fallback 연산, 연기 지점, 문서화된 종료)는 §5.2의 경로 검사로 막는다.
  - 같은 instance의 READY 발생이 필요한 ACCEPT(`retry_until_ready`)는 certificate 자신의 INIT_FALSE와 frontier(또는 once-PWB) 사실로 막는다. 그 사실은 `ℓ_j`의 모든 발생을 덮는다.
  - certificate는 기대는 REJECT event와 그것이 발생의 결정 event에 있는지를 기록한다. REJECT가 모두 결정에 있는 badness(`Allowed none`, `unspecified_by_reference`, 항목의 code 밖 결과)만 `¬OK_i`에 decision route를 준다. 그런 route는 모든 σ에서 성립한다. 결정 뒤의 REJECT(retry 소진, error exit나 error 반환)는 route를 주지 않는다. 그 길이 막힐 수 있기 때문이다.
  - 일부 연속만 허용되지 않으면 certificate는 "fixed not-ready"(shape WRONG-WAY나 ABSENT)만 확립하고 class는 열려 있다.
- **FIXED_ORDER가 증명하는 것**(L-FO). `TS(B,σ_U,M)`의 모든 유한 실행에서, LP `ℓ_j`가 결정 이전에 있는 범위 안 발생은 `ℓ_j`에서 `P_j`가 거짓이다. 그 밖의 결정된 발생은 다른 거짓 conjunct로 먼저 결정되었거나, 호출이 `ℓ_j`를 지나지 않고 돌아와 `NOT_EVALUATED`다. 그래서 도달한 범위 안 발생은 어느 것도 READY가 아니고, item 5에 따라 어떤 연속도 TOLERATED로 만들지 않는다. 이것이 `¬OK_i`다. 사용 도달, 모든 실행이 결정에 이른다는 것, 사용에 닿지 않는 실행에 대해서는 증명하지 않는다.
- **예: timebase 조회** ([I], spec App. A.7). `cFS-Master`의 발행 LP `x*`는 PSP main thread에서 `CFE_ES_Main` 전에 일어나므로, T-PO와 T-SPAWN으로 모든 cFE task entry의 MB에 든다. disabler `OS_DeleteAllObjects`는 SCOPE로 제외된다. census가 찾는 `OS_TimeBaseDelete` site는 기본 B에서 없을 것으로 보고, 찾으면 PWB로 순서가 정해져야 하며 아니면 certificate를 보류한다.
  - 그래서 TIME과 SCH_LAB의 조회(`cfe_time_task.c:311`, `sch_lab_app.c:176`)는 teardown 밖에서 safety 진술로 `protected(σ_U, M_api)`다. 조회가 돌 때 record가 이미 LIVE이므로 `protected(σ_U, M_impl)`이기도 하다(`¬NR_i`: 판정된 범위 안 조회 발생은 모두 `READY`나 `NOT_EVALUATED`이므로 도달한 조회는 모두 `READY`). σ_U∞와 σ_R1 칸은 전이 규칙 1(a)로, σ_E 칸은 M_impl에서 규칙 2로 얻는다.
  - progress는 주장하지 않는다. certificate가 낸 모든 칸에서 `unknown(not_checked)`다. explorer를 M_api에서 돌리면 σ_U∞ stall 상태는 건너뛴 core raise 뒤에서만 닿는다. 그때 startup thread는 `cfe_es_start.c:835`에서 죽고 TIME과 SCH_LAB의 spawn은 앞에 남는다(`CFE_ES_MainTaskSyncDelay`에 L1 적용; L1을 끄면 `unknown(unmodelled)`). 이 stall은 App. A.2 뿌리(`tml-root`)에 사슬로 붙는다. M_impl에서는 F7로 잃은 core task 뒤에서도 닿고, 그 task의 K-SPAWN.osal 발생에 붙는다.
  - 이 진술은 기본 B에 상대적이다. C1의 B′ M_impl에서는 SCH_LAB의 조회가 누수된 countsem type lock 뒤에도 있다(`sch_lab_app.c:167`의 `OS_CountSemCreate`; `leaked-lock`, CF의 VIOLATING 시도에 사슬, §3.5).
  - timebase handshake 손실은 이 조회에 영향이 없다. 두 조회는 TaskInit과 AppInit에서 `OS_TimerAdd` 전에 돌고 callback을 기다리지 않는다.
  - 코드가 순서를 정한 곳에서 race를 지어내지 않는지 보는 calibration이다.
- **SCOPE 제외** [D] (EXT-20261012 §4.1이 다시 씀). `SHUTDOWN_REQ ∈ MB(d)`인 writer·disabler `d`는 범위 안 발생의 certificate에서 **conjunct마다** 뺀다. 발생의 결정 이전에 있는 conjunct LP 앞에 `d`가 일어나면 그 발생은 범위 밖이다. scope는 결정 event에서 정하기 때문이다(L-SCOPE). 결정 뒤의 conjunct LP는 다른 거짓 conjunct로 이미 결정된 발생이나 `NOT_EVALUATED` 발생에 속하고, 어떤 진술도 그것을 세지 않는다. SCOPE는 T-OBS 안에서는 쓰지 않는다. MB 사실은 teardown을 포함한 모든 실행에 대해 성립해야 하기 때문이다.
- **INACTIVE.** B 상수와 message provenance 아래에서 어떤 actor entry에서도 template에 이르는 `Sk^cert` 경로가 없다. 증명은 접힌 경로 조건과 provenance 집합이다. 예: C1의 default B에서 CF의 조회는 `sem_name` 검사 뒤에 있고(`cf_cfdp.c:1270`) 기본 항목이 둘 다 비어 있으므로(`cf_def_config.c:60, 78`) `inactive(B)`다(spec App. A.4 step 1).
- **provider 축.** `none`은 `P_j`가 INIT_FALSE이고 `W_j = ∅`이며 `DISABLED`가 없을 때다. `env-absent`는 `W_j`가 모두 B.Env가 거짓으로 기록한 `env` 조건에 묶인 Φ effect(status `DECLARED_TRUSTED`)이고 관측이 일치할 때다(O4).

### 10.7 신뢰 장부(trust ledger)

모든 certificate와 모든 판정 칸은 네 부분으로 된 **신뢰 장부**를 지닌다 [D] (spec §4.5.1; EXT-20261012 §4.3; ADR-041, ADR-045). record는 `KZ_TRUST_LEDGER`, 항목은 `KZ_TRUST`다(spec §2.3).

| 부분 | 담는 것 |
|---|---|
| (i) 다시 확인한 추론 단계 | `kz proofcheck`가 받아들인 certificate 단계. 단계 번호와 한 확인(§10.5)을 적는다. status는 `ACCEPTED`(자기 routine) 또는 `ACCEPTED_VIA_T5`(공유 evaluator나 rule engine도 부름)다. "다시 확인했다"는 (ii)와 (iii)에 상대적으로 옳다는 뜻이다 |
| (ii) 추출된 사실 | (i)의 단계가 읽은 모든 `facts.kz`, `skel.kz`, `anchors.kz`, closure, 실행, prefix probe record. typed 참조로 나열하고, 산출물 hash가 장부를 정확한 추출에 묶는다 |
| (iii) 믿은 항목 | 쓴 Φ 항목(규칙, effect, binding, LP, T-TERM 선언, 준비 상태 명세와 Allowed 항목); `DECLARED_TRUSTED` 항목; §12.2의 가정(id로; 실행으로 reach를 증명하면 A2와 A6); 쓴 lemma(종류 `LEMMA`)와 각각의 **증명 상태**(§12.3 표에서 복사한다. 의무로만 적힌 lemma는 `unproved`다); 도구 기반(`libkz-core`와 검사기 tool hash) |
| (iv) 반박할 수 있지만 증명되지 않음 | (ii)나 (iii)의 항목 중 obligation이 반박할 수 있는 것마다: obligation(O1–O10; O7은 모형 build 때, O10은 `exh` 기록이 certificate를 만날 때), 상태(`not run`, `not refuted on the exercised paths of runs R`, `refuted`), 실행된 경로의 한계 |

- (iii)부의 lemma(종류 `LEMMA`)는 증명 상태를 지닌다(§12.3). 부분을 가진 lemma(L-CONT, L-TRANS, L-DEAD)는 항목과 상태가 부분마다 하나다.
  - certificate 자신의 칸에는 늘 L-MB, L-ONCE, L-PWB, L-DERIVE, L-FO나 L-PR, L1, L-STATUS(status의 최종성. badness 항목과 실행에서 읽은 모든 status가 기댄다)를 넣는다. SCOPE 제외를 쓰면 L-SCOPE를 더한다.
  - **reach나 witness가 실제 실행에 기대는 곳마다, certificate 자신의 칸도 포함해서 L-CONT (c)**(실행의 LP 단위 사영은 (σ_U, M_impl)의 유한 실행이다)를 A2, A6과 함께 넣는다. M_api에서는 L-MLEVEL(cone에 M_impl-relevant 연산이 없음)이나 L-MAPI-CONE(적합성 검사)을 더한다. L-MAPI-CONE도 실행의 사영에서 출발한다.
  - certificate 자신의 칸이 아닌 칸에는 L-MON, 칸의 σ가 요구하는 L-CONT (a), (b), (b′), L-TRANS를 넣는다. 규칙 2(i)를 쓰면 L-MLEVEL, 실행으로 σ_U∞ reach를 증명하면 L-REACH-CONE, bounded σ_R1 전이면 L-CONE-R1을 더한다.
- **L-CONT (c), L-MAPI-CONE, L-REACH-CONE의 실행 쪽 전제는 (iv)부에서 반박할 수 있다.** certificate view 자동자가 받아들이지 않는 actor 열을 가진 실행(O1, O2), 계산한 상태와 결과가 어긋나는 실행(O4), closure 밖의 actor나 key를 가진 실행(O5)은 그 모의가 주장하는 것을 반박한다. 그렇지 않으면 항목은 "실행된 경로 R에서 반박되지 않음"을 적는다.
- **obligation 통과는 결코 증명으로 기록하지 않는다.** 관측이 증명하는 유일한 항목은 reach이고, 그것도 L-CONT (c)와 그 칸이 쓰는 다른 lemma를 거쳐서만이다. 그 lemma들의 증명 상태가 장부에 reach 옆에 보인다.
- **증명 상태** [D] (PI 결정 D20; spec §4.5.1). (iii)부의 `LEMMA` 항목은 `KZ_TRUST.proof`를 지닌다. 검사기(`exh` 경로면 탐색기)를 build할 때 §12.3 표에서 복사한다. certificate와 판정 칸의 증명 상태는 그 진술이 쓰는 `LEMMA` 항목 중 가장 약한 것이고(PS3), 그런 항목이 없으면 `NA`다. `kz report`는 그것을 certificate 기반 진술 옆에 출력한다.
- **칸별 장부.** certificate의 장부는 자기 칸(σ_U, M, b = ∞)의 장부다. 다른 칸의 장부는 그것에서 유도한다.
  - 전이로 얻은 칸은 전이 lemma와, 그것이 기대는 가정(A7-impl, A7-api, A3, A4, A9, A-SKEL)과 그 obligation을 더한다.
  - reach를 실행으로 증명한 칸은, certificate 자신의 칸도 마찬가지로, 실행과 prefix probe record, A2, A6, L-CONT (c), L-STATUS, 칸별 reach 검사를 더한다. σ_U∞ 칸이면 cone actor의 TB 없음과 L-REACH-CONE, M_api 칸이면 cone actor의 M_api 적합성과 L-MAPI-CONE(같은 실행 사영에서 출발)이다.
  - certificate(자기 것이든 전이된 것이든)와 `exh` 결과(progress 후보, litmus LP-DL)를 함께 가진 칸은 `exh` 경로의 기반(보장 G5, L-DPOR, L-DEAD, L-MON, cone 규칙, 쓰는 reduction과 L1–L3)을 (iii)부에, O6과 O10을 (iv)부에 더한다.
  - 항목마다 지탱하는 경계가 적혀 있다(`KZ_TRUST.bound`: b = ∞ 또는 탐색한 h). 그래서 b = ∞ 진술을 뒷받침하는 항목과 bounded 기록·progress 값을 뒷받침하는 항목이 구분된다.
- **certificate 없는 결과** [D] (rev 1.3에서 일반화; D20 유지; spec §4.5.1 "Results without a certificate"; C-23). 모든 관측 판정, 예측, 확인, 정적 증명은 네 부분 장부를 지닌다. (i) 검사기가 실제로 다시 확인한 관계와 계산, (ii) 출처에서 믿은 코드·문서·모형 사실, (iii) 실행·관측·요약 가정(관측 판정이면 A-OBS와 L-OBS, §12.2, §12.3), (iv) 증명되지 않은 의무와 지원하지 않는 경로다. certificate가 없으면 (i)부는 비어 있을 수 있고, 나머지 부분은 늘 있다. 장부는 요약 뷰(§11.6), status, 세 판단, 근거 등급, 모형 식별자(§7.6), 관측 적합성 기록(spec §2.3), 그리고 예측이면 값·경로 보존 검사(spec §4.9)와 이어진다. 어떤 결과도 무조건적인 탐지로 보이지 않는다.
- **certificate 없는 판정.** (i)부가 비어 있다. (iii)부는 그 경로 자신의 기반을 적는다. `exh`면 보장 G5, L-DPOR, L-DEAD, L-MON, cone 규칙, 쓰는 reduction이다. 예측이면 L-PRED와 A-COMM이다. 실현된 witness면 recorder와 oracle이다. (iv)부는 O1, O2, O6, O8을 나열하고, `exh` 기록이 certificate나 전이된 진술을 만나면 O10도 나열한다.
- `kz report`는 certificate 기반 판정마다 장부를 출력한다. 네 부분의 크기를 label `ledger:<i>/<ii>/<iii>/<iv>`로, (iii)부 `LEMMA` 항목의 증명 상태별 개수를 label `lemmas:<p>/<d>/<u>`로 보인다. 두 label은 sub-label이 아니고 bitset에 넣지 않는다(§11.5).

**R2의 장부** ([I], spec App. A.1 step 9a; 실행한 것은 없다). 절반마다, M마다 certificate 장부가 하나씩 있다. 칸은 (σ_U, M_api)와 (σ_U, M_impl), b = ∞다. 다른 칸의 장부는 이것에서 유도한다.
- **(i)** view; ONCE(once(PSP main)은 정의상; once(ES main), once(EVS main), once(SB main)은 각자 펼친 core loop 안 once startup thread 위, star-closed 영역 밖에 생성 노드가 하나뿐이라 배타성이 자명함; once(`raise_ES`), once(`cfe_es_start.c:82`)); T-PO(검사기의 지배 routine으로 `m_ES ∈ MB(raise_ES)`, `m_EVS ∈ MB(raise_EVS)`, 남긴 `ExitApp` store, `o_ES`(또는 `o_EVS`)에서 `spawn_SB`까지 panic 갈래를 자른 startup thread 사슬, SB entry → `x`, FRONTIER 제외마다의 경로); T-TERM(`CFE_PSP_Panic`과 `abort`의 pc-linux 선언); T-SPAWN(SB main과 FRONTIER로 제외한 writer의 actor마다 spawn 목록이 closure 목록과 같음); T-OBS(`:803`의 falsifier 조건 via T5; form (b) `Pred_code` 재유도 own; R19의 binding·stratum·terminal 탈출 via T5 [I]; L1 전제; `Pred_code ⇒ C`; `W_o`와 그 제외); FRONTIER(모든 capture point `c`에 대해 `m_ES →* c` 경로 없음); DERIVES; INIT_FALSE(V) via T5; badness(`Allowed = none`이므로 모든 not-ready 발생이 결정에서 VIOLATING); status; reach(S1 뒤, (σ_U, M_api) 칸은 cone actor의 M_api 적합성 포함).
- **(ii)** SB `SendEventWithAppID` site 39개와 그 문맥([R2g §6]); 복사 사슬 AST 사실; INIT image(`CFE_SB_Global`은 0으로 초기화, `cfe_sb_task.c:40`); `(CFE_SB_Global_t, AppId)`와 `AppState`/`IsUsed`의 S3a writer 절과 분류; A-TYPE 목록; record tag `Type = CORE`(`cfe_es_start.c:795`)와 `PendingAppId` 흐름(`:814`); 직렬화된 `:835` exit tree와 mod set·lock 영역; 생성된 core table 순서; call graph 기반 행; `Sk^cert` 그래프; spawn 목록. 모두 hash로 기록한다.
- **(iii)** Φ(K-ID의 준비 사전조건 `Req`, capture point·slot 검사 LP, outcome map `cfe_evs.c:185–189`, `Allowed none`, harm; T-DERIVE; `CFE_ES_GetAppID`의 effect; R19; T-TERM 선언; `AppState`/`IsUsed` binding; M_impl certificate에는 Φ_L-1). `DECLARED_TRUSTED`(`CleanUpApp` 관계 guard는 두 절반 모두; EVS 절반의 A-FRESH, A-OWNER와 `ExitApp` STOPPED 제외). 가정 A1, A3, A4, A9, A-SKEL, A-WS, A-TYPE, A-EXTRACT, A-CORE, reach에 A2와 A6, M_impl certificate에 A7-impl, A7-api, A10. 도구 기반.
  - **lemma**(각각 증명 상태가 붙는다. 이 중 가장 약한 것이 certificate 칸마다 출력되는 상태다):
    - certificate 자신의 칸 (σ_U, M_api), (σ_U, M_impl): L-MB(`unproved`), L-ONCE(`unproved`), L-PWB(`unproved`), L-DERIVE(`depends-on-stated-assumption`), L-FO(`unproved`), L1(`unproved`), L-STATUS(`unproved`). L-SCOPE(`unproved`)는 `W_j`에서 SCOPE 제외를 쓸 때만이고 예상되지 않는다 [I].
    - 그 칸들의 **reach 항목**: (σ_U, M_impl)은 S1이 실행으로 reach를 `proved`로 만드므로 L-CONT (c)(`depends-on-stated-assumption`). (σ_U, M_api)는 실행이 L-MAPI-CONE(`unproved`)을 거쳐서만 reach를 증명하고, 그것이 실행의 LP 단위 사영에서 출발하므로 L-CONT (c)도 함께 적는다. L-MLEVEL은 cone이 M_impl-relevant이므로 쓰지 않는다.
    - 유도된 칸(σ_U∞, σ_R1, σ_E): L-MON(`unproved`), L-CONT (a)(`proved`)와 칸의 σ가 요구하는 (b), (b′)(`unproved`), L-TRANS(`unproved`). M_impl에서 오는 σ_E 칸에는 L-CONT (c). 실행으로 reach를 증명한 σ_U∞ 칸에는 L-REACH-CONE(`unproved`).
    - 이 상태에서 R2의 모든 certificate 칸은 "lemma status: unproved"를 출력한다.
- **(iv)** S1/S2 전까지 모두 `not run`이다. A-FRESH와 A-OWNER는 O9; `CleanUpApp` guard는 O3/O4(구간 안에서 `CleanUpApp`이 도는 실행에서만; 기본 boot에는 그런 실행이 없을 것으로 본다 [I]); A-SKEL은 O1/O2와 S5부터 O8; closure는 O5; once·MB 사실은 O3; Φ effect, LP, `derives`, `Pred_code`는 O4(a)/(b)(`x` 전에 성공하는 SB-AppId 사용이 하나라도 있으면 certificate가 반박된다); writer 완전성은 O7(model build 때). L-CONT (c), L-MAPI-CONE과 유도된 σ_U∞ 칸의 L-REACH-CONE은 실행 쪽 전제로 반박한다: O1, O2(certificate view 자동자가 받아들이지 않는 actor 열), O4(계산한 상태와 어긋나는 결과), O5(closure 밖 actor나 key), 실행 R에서. 실행된 경로의 한계 [I]: 기본 boot는 성공 경로만 지난다. 그래서 `:823` → panic 갈래, `cfe_es_task.c:340/371/381/393`의 early return, timeout된 대기 뒤의 `ExitApp` 경로는 기본 boot의 obligation으로 반박할 수 없다. reach는 관측이 확립하는 유일한 항목이고, L-CONT (c)와 그 칸이 쓰는 다른 lemma를 거칠 때뿐이다.
- **장부 밖의 시험**(obligation이 아니므로 (iv)부 항목이 아니다, spec §4.5.1): A-CORE는 evaluator agreement property test와 golden test로, writer 완전성은 program mutant(spec §8.5)로도 시험한다. 그 결과는 반박 상태가 아니라 검사기와 recall 지표와 함께 보고한다.
- S3a gate는 R2 장부를 요구한다(ADR-041, ADR-045). (rev 1.3: 첫 rung V1′에서 V1의 certificate 출구 항목, 곧 FIXED_ORDER, `kz proofcheck`, R2 certificate 장부는 선택이 되었고, 논문이 R2의 보편 진술을 할 때만 쓴다. 단계 표와 다른 곳에서는 그 사다리 문단이 앞선다. ADR-057, spec §9.3 "Ladder of revision 1.3".)

---

## 11. 판정 기록

### 11.1 Class

instance `i`, B, σ, M, 경계 b를 고정한다(spec §1.10). status(§5.5)는 `Pref_b(B,σ,M)` 위에서 평가한다. "범위 안"은 `i`에 속한다는 뜻이다. `NOT_EVALUATED` 발생은 아래 어느 것도 세지 않는다. 발생 ordinal o에 대해 다음과 같이 쓴다.

| 기호 | 뜻 |
|---|---|
| `Bad_i` (`Bad_i(o)`) | 어떤 유한 실행이 (ordinal o에서) 범위 안 `VIOLATING` 발생을 담는다. Allowed가 미지정이면 `NOT_READY_UNJUDGED` 발생 |
| `OK_i` (`OK_i(o)`) | 어떤 유한 실행이 (o에서) 범위 안 `READY`나 `TOLERATED` 발생을 담는다 |
| `NR_i` | 어떤 유한 실행이 not ready로 결정된 범위 안 발생(`TOLERATED`, `PENDING`, `VIOLATING`, `NOT_READY_UNJUDGED`)을 담는다 |
| `Reach_i` | 어떤 유한 실행이 준비 여부가 결정되고 평가된(`NOT_EVALUATED`가 아닌) 범위 안 발생을 담는다 |

**safety 기록** [D] (EXT-20261012 §4.1; ADR-039). 칸마다, 경계마다 기록은 다음을 가진다.
- 확립된 진술 `¬Bad_i`, `¬OK_i`, `¬NR_i`("범위 안 발생 중 not ready로 결정된 것이 없다". 곧 판정된 것은 모두 `READY`나 `NOT_EVALUATED`)와 각각의 경로(certificate, `exh`, 전이). `¬NR_i`는 PROTECTED certificate나 그 전이에서만 오고, `¬Bad_i`를 함의한다.
- `¬OK_i`의 **decision route**. `¬OK_i`가 **결정적**(deciding)이라는 것은 그 칸·경계에 bad witness가 있거나, 진술이 FIXED_ORDER certificate(또는 그 전이)에서 왔고 그 badness 항목이 모든 not-ready 발생을 결정 event에서 정한다는 뜻이다(`Allowed = none`; `unspecified_by_reference`; 항목의 code 밖 결과). 그런 certificate는 모든 σ에서 결정적이다. REJECT event가 결정 뒤에 있는 certificate(retry 소진, error exit·error 반환, 다음 연산, 종료)는 어느 σ에서도 혼자 결정하지 못한다. REJECT로 가는 길이 시간 없는 대기, σ_U∞가 지우는 전이, σ_R1의 굶주림으로 막힐 수 있기 때문이다.
- bad·good witness와 그 ordinal.

**class는 기록의 함수다.**

| class (safety 축) | 기록 | 증명 의무 |
|---|---|---|
| `inactive(B)` | 어떤 실행도 사용에 도달하지 않는다 | 정적: B 상수와 message provenance 아래 **certificate view**에서 template에 닿을 수 없다(경로 조건이 증명). **관측되지 않은 사용은 inactive가 아니라 `unknown(not_reached)`다** |
| `protected(B,σ,M,b)` | `¬Bad_i` 확립, `¬OK_i`는 아님. 도달한 범위 안 발생은 모두 `READY`, `TOLERATED`, `PENDING`이고, 결코 `VIOLATING`이나 `NOT_READY_UNJUDGED`가 아니다. `¬NR_i`가 있으면 모두 `READY`다(도달한 발생은 판정되었고 `NOT_EVALUATED`가 아니다. 위 `Reach_i`. `NOT_EVALUATED` 발생은 어느 진술도 세지 않으므로 `¬NR_i`는 "모든 발생이 `READY`"가 아니다) | certificate(b = ∞; L-PR이 `¬NR_i`를 증명), b 안에서 `Q_bad = no`인 `exh`, 또는 전이(규칙 1(a)) |
| ↳ refinement `allowed-not-ready` | protected이고, 어떤 유한 실행에 `TOLERATED` 발생이 있다 | b 안의 `exh`만. PROTECTED certificate는 `¬NR_i`를 증명하므로 결코 뒷받침하지 않는다 |
| `fixed-order(B,σ,M,b)` | `¬OK_i` 확립 **이고 결정적**, `¬Bad_i`는 아님. 도달한 범위 안 발생 중 `READY`나 `TOLERATED`가 없고, bad witness가 있거나 not-ready 발생이 결정에서 bad로 정해진다 | certificate(L-FO와 badness 항목), b 안에서 `Q_ok = no`이고 bad witness가 있는 `exh`, 또는 받는 σ에 decision route가 있는 전이. **k번 중 k번 실패한 실행으로는 결코 내지 않는다**([R06 S9]). 모든 실행이 결정에 이른다는 것은 주장하지 않는다 |
| `no-final-status(B,σ,M,b)` | `¬Bad_i`와 `¬OK_i`가 모두 확립. 도달한 범위 안 발생이 모두 `PENDING`(이나 `NOT_EVALUATED`)이거나, 도달한 것이 없다 | 두 진술. 각자 어느 경로로든 된다(예: 전이된 `¬OK_i`와 `exh`의 `¬Bad_i`) |
| `schedule-dependent(B,σ,M,b)` | **같은 ordinal에서** bad witness와 good witness. 서로 다른 두 유한 실행이다 | 같은 칸·경계의 두 witness. 축마다 더 약한 근거를 기록한다. stall witness는 결코 둘 중 하나가 아니다 |
| ↳ refinement | `timeout-dependent` (§7.4) | σ_U와 σ_U∞ 칸에서 끌어내고 자기 경계를 가진다. `exh`가 필요하다 |
| `ordinal-split(B,σ,M,b)` | bad와 good witness가 있지만, 같은 ordinal의 쌍은 확립되지 않았다 | witness와 그 ordinal. 어떤 발생 하나가 스케줄에 의존한다고 말하지 **않는다** |
| `unknown(reason)` | 확립된 것이 없다. 또는 decision route 없는 `¬OK_i`만 있다(이유 `no_decision_route`; 진술은 그래도 보인다). 또는 진술 없이 한쪽 witness만 있다. 곧 `¬OK_i` 없는 bad witness이거나 `¬Bad_i` 없는 good witness다(이유 `one_sided`; witness는 그대로 보인다) | `not_reached`, `model_gap`, `bound`, `budget`, `key_unresolved`, `loss`, `unmodelled`, `writer_incomplete`, `horizon`(horizon monitor가 OPEN인 채 trace가 끝남), `horizon_static`(trace 없는 census의 정적 경우: FIXED_ORDER 항목 5가 horizon 안의 연장 중 일부만 허용되지 않음을 찾아 class가 열린 채 남음; spec §4.5, §4.6), `no_decision_route`, `one_sided`, `unordered`, `O-ordered-only`, `conditional-observation`(census의 열린 경로, 어느 경로도 결정하지 않은 동안; spec §4.6), `conformance`, `model_fault`(`contested` 경계 포함), `model_level` |

good 쪽 flag `timeout-rescued`(§7.4)는 protected, schedule-dependent, ordinal-split(σ_U에서 good witness를 가진 모든 class)에 붙을 수 있다.

**보편 진술, 존재, 경계** [D] (spec §1.10; EXT-20261012 §4.1–4.2).
- **보편 진술의 범위.** `¬Bad_i`와 `¬OK_i`는 도달한 발생을 두고 말한다. 그래서 도달한 발생이 없으면 공허하게 성립하고, 도달한 발생이 모두 `PENDING`이면 둘 다 성립한다. 서로 배타적이지 않다. 둘 다 확립된 경우가 `no-final-status`다.
- 어느 진술도 사용에 닿지 않는 실행에 대해 말하지 않는다. 사용 전의 영원한 대기는 progress 결과가 다룬다(§7.3). 사용의 actor instance가 발생 없이 끝나거나 process가 panic하는 실행은 어느 축도 다루지 않는다. 그 기록은 그리로 이끈 not-ready 뿌리 발생이 있으면 그 safety 결과와, 그 사슬에서 관측된 결과(`consequence`: 잃은 task, panic, process abort), 그리고 그 뿌리의 caller 수준 Allowed가 주는 harm(K-SPAWN.osal과 K-SELF.es raise의 기본값 아래에서는 `unspecified_by_reference`, §5.3; spec §1.6, §3.3)뿐이다. 자발적 timeout 뒤의 σ_U panic에는 그런 뿌리가 없다(spec §1.10, §6.4 item 34).
- **존재**는 칸의 **reach** 축에 기록하고 class에 넣지 않는다. `reach = assumed`면 보편 class를 "… 도달한다면"으로 읽는다. `reach = unreached(b)`면 "… 도달한다면; b 안에서는 도달하지 않음"으로 읽는다.
- **경계마다.** 기록, class, reach, progress는 경계마다 둔다. b = ∞는 certificate, 그것에서 옮긴 것, 실행, 확인된 모형 경로의 경계다. `exh`는 끝낸 h마다 경계를 가진다. b = ∞의 진술은 모든 b에서 성립하고, b 안의 witness는 더 큰 모든 경계에서 성립한다. (칸, 경계)마다 class를 하나 계산한다. 보고서는 b = ∞의 class와, 정보를 더하는 bounded class를 함께 보인다. 그래서 두 경로가 다른 경계에서 다른 class를 줄 수 있다(예: certificate의 b = ∞ protected와, 아무것도 도달하지 않은 `exh`의 b 안 `no-final-status`, spec §4.12 item 2). b = ∞ 기록에는 확인되지 않은 탐색기 witness가 들어가지 않는다. 모형 witness는 실행이 그 경로를 확인할 때만(실현된 witness나 확인된 모형 경로로) 들어간다. 그래서 `exh`의 bad witness가 certificate의 `¬OK_i`를 b = ∞에서 결정적으로 만드는 일은 없다(L-PATIENT variant 2와 3, LP-EARLY, spec App. A.8).
- **모순과 불일치** (spec §4.12 item 5). 같은 경계에서 셋을 나눈다.
  1. certificate나 그 전이에서 온 진술에 맞서는 실현된 witness는 **모순**이다: `¬Bad_i`에 맞서는 bad witness, `¬OK_i`에 맞서는 good witness, `¬NR_i`에 맞서는 관측된 결정된 not-ready 발생. 규칙을 정지시킨다(spec §4.14).
  2. 그런 진술에 맞서는 `exh` witness, 또는 한 σ의 `exh` 결과가 다른 σ에서 옮긴 `exh` 진술과 맞서는 것은 obligation **O10**(모형–certificate 일치)으로 간다. 탐색한 경계가 `contested`가 된다. certificate 자신의 b = ∞ 기록은 그대로이고, 그 Φ 판에 대한 지속적인 contested 표시를 받는다. 방향을 정한 확인이 뒤따른다. (i) L1–L3를 끄고 다시 탐색한다. witness가 사라지면 그 reduction이 원인이고, 그 instance에서 끈다. (ii) 아니면 `kz proofcheck`를 다시 돌린다. 거부하면 certificate를 거둔다. (iii) 아니면 탐색한 경계는 `contested`로 남고(`unknown(model_fault)`로 보임), witness의 확인 실행을 계획한다. 다른 경계는 낮추지 않는다. witness가 실현되지 않는 한 Φ 규칙을 정지시키지 않는다.
  3. `exh`만의 진술에 맞서는 실현된 witness는 모형 경계 충돌이다(O6, 그 경계가 `contested`).

  어느 것도 lattice 단계가 아니다. 경계 밖의 근거는 경계 안의 진술과 결코 모순되지 않는다. stall witness는 어떤 class와도 모순이 아니다. class는 safety 진술이기 때문이다.
- **실행마다 발생이 여럿인 template**(§1.1). class는 발생을 두고 양화하고, witness는 ordinal(key마다, canonical actor instance 이름 위)을 기록한다. 실행마다 범위 안 발생이 많아야 하나인 template에서는 `¬Bad_i`가 v1의 "E_i의 어떤 실행도 bad가 아니다"와 같다(v1의 `E_i ≠ ∅`, `S_i = ∅` 조건은 없다). 그때 witness 쌍은 모두 같은 ordinal이다.

### 11.2 축과 shape

- **provider** ∈ {`present`, `none`, `env-absent`, `unknown`}. `none`은 conjunct가 INIT_FALSE이고 완전한 writer 집합이 비었을 때다. `env-absent`는 writer가 B.Env가 거짓으로 기록한 `env` predicate에 묶인 Φ effect뿐이고 O4 결과가 일치할 때다.
  - provider `none`인 사용도 결과로 분류한다. 도달한 발생이 `READY`나 `TOLERATED`일 수 없고 decision route가 있으면 fixed-order다.
  - 그래서 유일한 REJECT가 결정 뒤의 소진인 retry는, certificate만으로는 모든 σ에서 b = ∞의 `unknown(no_decision_route)`이다(`¬OK_i`는 보인다). `exh`로는 b 안에서 σ_U fixed-order(소진하는 실행이 bad witness)이고, 소진이 시간 기반이면 σ_U∞ `no-final-status`다. `allowed-not-ready`를 가진 protected는 `exh`로만 얻는다. 허용된 not-ready 결과를 확립하는 certificate는 없고, FIXED_ORDER item 5는 그런 instance를 열어 둔다(spec §4.5).
  - 예: HS의 `linux_sysmon`은 provider `env-absent`, class `fixed-order`, 결과 label `not-ready-unjudged`이고, 순서 문제가 아니므로 따로 보고한다(spec App. A.6).
- **reach**, 칸 (σ, M)마다, 경계마다 [D] (spec §1.10; EXT-20261012 §4.2). b = ∞에서는 `proved(run id | 확인된 모형 경로)`나 `assumed`다. 유한 b에서는 `model(b)`(`exh`가 b 안에서 결정된 범위 안 발생을 찾음, 확인 안 됨)나 `unreached(b)`(`exh`가 찾지 못함)다. `proved`는 **그 σ, 그 M의** 유한 실행으로서 B 위에서 실현되고 결정된 범위 안 발생을 담은 것이 필요하다.

  | 칸 | `proved`의 조건 |
  |---|---|
  | σ_U, M_impl | 아무 실행(L-CONT), 또는 실행이 확인한 모형 경로 |
  | σ_U, M_api | instance의 cone에 M_impl-relevant 연산이 없거나(L-MLEVEL, sub-label `mlevel-lemma`), 발생까지의 실행 prefix가 **instance의 닫힌 cone의 모든 actor**에서 M_api 적합성 검사를 통과한다. 그 actor들의 OSAL 호출이 모두 M_api가 허락하는 결과를 받았고, M_impl 전용 행동(`OS_ERR_INCORRECT_OBJ_STATE`, 잃은 task, 누수된 type lock)이 없었다는 뜻이다. cone 밖 actor의 OSAL 호출은 상관없다(L-MAPI-CONE). prefix probe가 필요하다(spec §4.7 item 6) |
  | σ_U∞ | 그 M의 σ_U 조건, 그리고 **닫힌 cone actor들의** prefix 전이에 시간 기반 전이가 없다. 주기적이 아닌 timed wait의 timeout 결과, 소진된 지연 retry, 강제 kill, M_impl에서 OSAL 내부 timed wake(`OS_WaitForStateChange`의 timeout 반환)가 없어야 한다. cone 밖의 TB 전이는 상관없다(L-REACH-CONE). prefix probe가 생기기 전까지 M_impl의 실행 기반 σ_U∞ reach는 `assumed`다 |
  | σ_R1 | native σ_R1 arm(S10)에서 확인된 모형 경로만. 아니면 `assumed`나 `model(b)` |
  | σ_E | 관측된 발생 |

  - `unreached(b)`와 `model(b)`는 b 안의 `exh`에서만 나온다. witness가 b 안에 있는 증명(적합한 모형 prefix가 b 안에 있는 실행)이 `unreached(b)`를 만나면 그 경계는 `contested`다.
  - 그 밖에는 `assumed`이고, class는 "… 도달한다면"으로 읽는다.
- **scope** ∈ {`in-scope`, `teardown`} (§5.4).
- **progress**, 칸마다, 경계마다 [D] (spec §1.10; EXT-20261012 §4.1) ∈ {`stall-witness(S)`, `no-stall-within(b)`, `unknown(reason)`}. S는 {pre-use, in-use, horizon}의 공집합 아닌 부분집합이다(§7.3).
  - stall witness는 kind(`deadlock-cycle`, `leaked-lock`, `other`), qualifier(`idle`, `terminal-pending`, choice support를 가진 `choice-dependent`), wait-for root 집합, `confirmable` flag, 실현 방식(`model(b)` | `realized`), harm(`hang_or_deadlock` | `task_never_runs` | `terminal_pending(panic | abort)`), progress 사슬, 결과 label `liveness`를 지닌다.
  - 기본값은 `unknown(not_checked)`다. K-TEARDOWN instance는 `unknown(teardown_scope)`다. 근거가 반박한 경계는 `contested`가 되고 `unknown(model_fault)`로 보인다.
  - progress 결과는 class를 바꾸지 않고, class는 progress 값을 함의하지 않는다. `protected`와 stall witness가 함께 있는 것은 일관된 기록이지 모순이 아니다(litmus LP-DL, §12.6).
  - **보이기 규칙.** 보편 class는 언제나 그 칸의 reach·progress 값과 그 경계와 함께 보인다. stall witness는 kind, qualifier, wait-for root와 함께 보인다. certificate나 그것에서 옮긴 진술에 기댄 진술은 lemma 상태도 함께 보인다(§12.3, PS3). 예: "protected — 판정된 모든 발생이 READY 또는 NOT_EVALUATED (certificate, b = ∞); reach proved; progress not checked; lemma status: unproved". `terminal-pending`은 닫힌 cone에 상대적이라고 보인다.
  - **세는 규칙.** stall witness는 `realized`이거나, confirmable이고 choice-dependent가 아닌 모형 witness일 때만 detection이나 Reported로 센다(spec §8.5). 센 witness는 wait-for root 집합으로 묶는다. 묶음마다 칸에서 한 번만 세고, 그것이 막는 instance를 붙인다. 그 밖의 모형 stall witness는 progress 후보로 나열한다.
- **shape** ([A §1.7]). bad 유한 witness ρ와 그 발생 `u`, 그리고 실패한 conjunct를 참으로 만들 첫 전이 `p*`의 관계다. looper 규칙(handler 사이 edge 없음)과 API 수준 edge만 쓴다. witness가 유한 실행이므로 shape는 ρ와 그 **기록된 연장** 위에서 정의한다. 기록된 연장은 ρ를 담은 실행의 나머지, 또는 ρ가 속한 explorer 경로다 [D] (EXT-20261012 §4.2).

  | shape | 뜻 |
  |---|---|
  | WRONG-WAY | `p*`가 ρ나 그 기록된 연장에서 `u` 뒤에 일어나고, `u →hb p*`다 |
  | UNORDERED | `u ∥hb p*` |
  | ABSENT | ρ의 어떤 연장도 `p*`를 담을 수 없다. 정적으로 보이거나(provider `none`; 또는 `p*`의 노드가 once이고 ρ에서 `p*`를 내지 않고 이미 지났다. 예: 실패 갈래를 지난 `cfe_sb_task.c:138`의 SB `GetAppID`), ρ의 끝 상태에서 `p*`를 낼 수 있는 censored·좌초 actor가 없는 b 안 `exh`로 보인다(그러면 b에 묶인 진술). 그냥 끝난 실행에서 추론하지 않는다 |
  | NOT-OBSERVED | `p*`가 ρ에도 기록된 연장에도 없고, ABSENT는 보이지 않았다 |
  | STALE | conjunct가 한때 참이었다가 `u` 전에 끄는 전이가 일어났다(generation 불일치) |

### 11.3 Evidence 두 축

([R06 S1]; spec §1.10)

| 축 | 값 (약한 쪽 → 강한 쪽) |
|---|---|
| **realization** | `model(b)` ≈ `predicted-modelled`(A-COMM 실패) ≈ `predicted-extrapolated`(요약 기반 외삽) < `predicted`(A-COMM 아래 trace-sound) < `realized`(gcc 바이너리에서 실제로 일어남). 별도로 `certificate(id)` |
| **naturalness** (realized만) | `timeout-exercised`(hold 동안 또는 hold 때문에 cone actor의 시간 기반 전이가 일어남, §7.4; 주기적 대기는 빼고) < `gated-F1` ≈ `held-F0` < `natural`. flag `derived`(스케줄을 구성함) |

### 11.4 σ_E class와 "경보 없음 ≠ 안전"

- **schedule-dependent.** `schedule-dependent(σ_E)`에는 **같은 발생 ordinal에서** 자연 bad witness와 자연 good witness가 필요하다. 그 ordinal의 발생이 최종 status를 가진 서로 다른 두 실현 실행이고, 각자 pool key를 가진다. ordinal이 다른 bad·good 발생은 `ordinal-split(σ_E)`이고, 실행 하나에서도 나올 수 있다. 실현됐지만 hold·gate가 개입했으면 `σ_E(held)`다.
- **보편 class.** σ_E의 protected, fixed-order, no-final-status는 실행에서 결코 유도하지 않는다. 전이 규칙 2(b = ∞)로만 얻고, 모든 유한 실제 실행의 판정된 범위 안 발생에 대해 certificate가 증명하는 것과 같은 safety 진술(`¬Bad_i`, `¬OK_i`, `¬NR_i`)을 말한다. `NOT_EVALUATED` 조기 반환은 세 진술이 모두 허용하고, `¬NR_i`는 "모든 발생이 `READY`"가 아니다.
- **횟수.** 관측 횟수는 정확히 "no σ_E bad witness in n of N runs (pool key); σ_E class unknown"으로 쓴다. 결코 "timing-protected"라고 쓰지 않는다.
- **progress.** σ_E progress는 영속성을 스스로 보이는 wait-for 사실(spec §4.13)이 있을 때만 `stall-witness(realized)`다. `deadlock-cycle`이거나, holder가 끝났거나 같은 lock을 잡으려다 스스로 막힌 `leaked-lock`(K-LOCKBAL)이어야 한다. 그 밖에는 `unknown(not_checked)`이고, 결코 `no-stall-within(b)`가 아니다.

### 11.5 세 판단의 필드와 판정 기록

(spec §1.10; §5.8의 세 판단)

- **결과 label**(caller 수준, safety 결과): `violation`(Allowed 지정. 준비 상태 위반) / `LOSS-UNDER-ORDER`(route·용량 준비 상태 명세 중 caller 수준 Allowed가 미지정이고 must-deliver `appreq`가 없는 것) / `not-ready-unjudged`(그 밖 준비 상태 명세 중 Allowed가 미지정인 것, 그리고 must-deliver `appreq`가 `caller` 항목과 충돌해 Allowed가 미지정인 route·용량 준비 상태 명세, §5.8 `appreq` 단계 3). 어떤 칸의 class도 not-ready 결과를 보이지 않는 동안은 `NONE`이다.
  - label은 Allowed에 따른 뜻을 말하므로 판정마다 한 필드다. 칸은 class가 fixed-order, schedule-dependent, ordinal-split일 때만 label을 보인다.
  - `fixed-order`는 decision route가 있을 때만 label을 지닌다. App. A.6은 `not-ready-unjudged`의 fixed-order다. decision route 없는 `¬OK_i`는 `unknown(no_decision_route)`이고 label을 보이지 않는다.
  - label `liveness`는 progress 결과에 속한다.
- **framework 수준 판단** [D]. 결과 label과 결코 합치지 않는 필드 둘이다. 인코딩은 S0에서 정한다.
  - `fw_doc`, **instance마다**(op의 outcome map의 성질): instance의 outcome map이 어느 수준에서든 나열하는 not-ready 결과 code의 문서화 상태 집합이다. 값은 셋이다. `documented-not-error`는 supplier 쪽 reference가 결과를 문서화하고 error가 아니라고 말한 것이다(cSB4305.5의 근거, `cFE_FunctionalRequirements.csv:352`). `documented-result`는 그런 말 없이 문서화한 것이다(`@retval`; `\retval CFE_EVS_APP_ILLEGAL_APP_ID`, `cfe_evs.h:199`, 그 code는 error severity, `cfe_error.h:115–119, 267`). `undocumented`는 문서가 없는 것이다(M_impl에서 `OS_CountSemGetIdByName`의 `OS_ERR_INCORRECT_OBJ_STATE`, `osapi-countsem.h:157–160` 밖). map에 not-ready code가 없으면 비어 있다. 어느 code가 어디서 일어났는지는 witness와 실행 기록에 있다.
  - `fw_conf`, **(σ, M) 칸마다** ∈ {`as-documented`, `deviates`, `unknown`}: 칸의 근거에서 결과 code(O4(a))와 문서화된 부수 효과(spec §3.9의 counter, trace의 event 시도와 그 결과)가 문서대로 일어났는가. M_api는 API 계약 적합을 가정하므로 M_api 칸은 `unknown`이다. `as-documented`와 `deviates`는 M_impl 칸과 실행에서 온다. 실행마다의 적합성은 conformance 기록(spec §4.11)에 있다. 어긋남은 거기에 적고, 아무것도 정지시키지 않는다.
  - `documented-not-error` 결과도 caller 수준에서 LOSS-UNDER-ORDER나 `violation`을 지닐 수 있다. `violation`이 `as-documented`로 일어난 `documented-result` 위에 설 수도 있다(R2).
- **consequence**(관측된 결과). Allowed와 상관없이 모든 not-ready 발생에 대해, trace나 모형 연속에서 기록한다. run oracle이 주는 관측 count(counter 변화, event 시도와 그 결과)를 칸의 `runs` 참조로 함께 남긴다. 어휘: crash나 정의되지 않은 행동; deadlock(VIOLATING 발생의 결과로서. instance 자신의 영원한 대기는 progress 결과다); process abort; panic; app error exit; app zombie; core service task 없음; status 무시와 조용한 효과 손실; event 뒤 계속; 늦었지만 회복; message 손실(route 없음, queue 가득, MsgLim 도달); event 거절(EVS가 거절한 event 시도, 예: `ILLEGAL_APP_ID`).
- **sub-label**: `portability-hazard`, `implementation-defect-candidate`, `portability-hazard-evidence`(§3.5); `assumed-visibility`(spec §3.14); `A-ZT`(σ_R1 지연 의미); `tb-undeclared`(§7.1). sub-label은 bitset(`sublabels`, spec §2.3)이다. 다음 세 **rendered label**은 sub-label이 아니고 bitset에 넣지 않는다(§10.7).
  - `trusted:<n>`(연속성을 위해 둔다): certificate 신뢰 장부의 (iii)부에서 종류가 `DECLARED_TRUSTED`(`KZ_TRUST.kind`, spec §2.3)인 항목 수다.
  - `ledger:<i>/<ii>/<iii>/<iv>`: 장부 네 부분의 크기다. `TRUST_LEDGERS` record의 `n_rechecked`, `n_extracted`, `n_trusted`, `n_refutable`에서 읽는다.
  - `n_trusted`는 (iii)부 전체의 크기다. 그래서 `ledger:`의 세 번째 값은 `trusted:<n>`의 n과 다를 수 있다. (iii)부에는 `DECLARED_TRUSTED` 항목 말고도 Φ 항목, 가정, lemma 의무, 도구 기반이 들어간다.
  - `lemmas:<p>/<d>/<u>`: 신뢰 장부 (iii)부의 `LEMMA` 항목 가운데 증명 상태가 `proved`, `depends-on-stated-assumption`, `unproved`인 것의 개수다(§12.3). 장부의 `KZ_TRUST` 항목에서 센다.
- **판정 기록**(`kz_verdict`와 (σ, M)별 `kz_verdict_cell`, spec §2.3)에는 다음이 들어간다.
  - instance id; provider와 scope.
  - 칸마다: class와 refinement, 그 근거인 safety 기록(진술 `¬Bad_i`, decision route를 가진 `¬OK_i`, `¬NR_i`와 각 경로, ordinal을 가진 witness); 경계 참조(`BOUNDS` 절의 항목, 경계마다 하나: 경계와 정책 상태(exploration view면 h 포함), 경계별 class(`contested` 포함), reach와 progress 값, safety 경로와 decision route, refinement, progress flag, 이유와 harm, ordinal을 가진 bad·good·stall witness, 그 경계의 progress 사슬); reach; `fw_conf`; realization과 naturalness 집합, pool key를 가진 `n of N`; progress 결과(값, stall 절 집합, kind, qualifier, wait-for root 집합, 이유, harm, 실현 방식, 경계, stall witness, progress 사슬, flag `timeout-masked-liveness`와 `tml-root`); 신뢰 장부 참조; 증명 상태(`proof_status`): 칸의 진술이 쓰는, 칸의 장부 (iii)부 `LEMMA` 항목 중 가장 약한 상태이고 그런 항목이 없으면 `NA`(§12.3).
  - shape; caller 수준의 결과 label과 harm; framework 수준의 `fw_doc`; 관측된 consequence와 관측 count 참조; 준비 상태 위반 사슬; unknown 이유.
  - B, Φ, tool hash.
  - certificate(O10이 이 Φ 판에 contested 표시를 했으면 그 표시 포함. certificate record `KZ_CERT_FIELDS`가 certificate 자신의 `proof_status`도 지닌다), witness, 실행에 대한 typed 참조.
- **"safe" 필드는 결코 없고, "live"나 "deadlock-free"를 뜻하는 필드도 없다**(spec §1.10; 그림 [D6](diagrams/D6_verdict_overview.svg)의 불변식).

### 11.6 요약 뷰 (rev 1.3)

**요약 뷰** [D] (spec §1.10 "Summary view"; D2; ADR-054; [P0.2] §8.2–§8.3; C-10). 보고서는 위 기록 위에 요약 뷰를 더한다. 요약 뷰는 아무것도 대신하지 않는다. 모든 기록은 발생 status(§5.5, 그리고 trace가 끝났을 때 결정에 이르지 못한 호출의 `OPEN`, `phi_reference.md` §7.5), 세 판단(§5.8), 근거 등급(§5.9), 근거 축(§11.3), unknown 이유를 그대로 지닌다.

| 종류 | 요약 값 |
|---|---|
| 관측 판정 (spec §4.8) | `OBSERVED_VIOLATION`, `NO_VIOLATION_IN_CHECKED_PREFIX`, `INCONCLUSIVE(reason)`, `NOT_APPLICABLE` |
| 한정 예측 (spec §4.9) | `MODEL_CANDIDATE`, `NONE_IN_SCOPE`, `INCONCLUSIVE(reason)`, `NOT_RUN` |
| 실행 확인 (spec §4.13) | `CONFIRMED`, `NOT_CONFIRMED(reason)`, `NOT_RUN` |
| 실행 관계 (§1.4) | 앱 간(inter-application), core–앱(core–application), 생성 framework–새 앱(creation framework–new application), 앱 내부 대조(intra-application control), 소유 미확정(ownership undetermined) |

발생의 status를 관측 요약으로 옮기는 대응이다. 같은 조건, instance, 검사한 구간에 대한 것이다. 각 행은 그 조건에서만 적용되고, 기계적인 치환이 결코 아니다.

| status | 요약 | 조건과 남기는 것 |
|---|---|---|
| `VIOLATING` | `OBSERVED_VIOLATION` | 기록의 모든 설명에서 위반이 성립할 때만(spec §4.8 "Explanation set", [03_workflow.md](03_workflow.md) §1.3). 아니면 `INCONCLUSIVE(explanation_ambiguous)`. 틀린 값 하나만으로는 올리지 않는다 |
| `READY` | `NO_VIOLATION_IN_CHECKED_PREFIX` | 실제로 평가한 조건과 구간에 대해서만. 다른 조건이나 미래에 대해서는 말하지 않는다 |
| `TOLERATED` | `NO_VIOLATION_IN_CHECKED_PREFIX` | `caller` record와 horizon monitor의 ACCEPT가 있을 때만(QT-4). 다른 곳에서 이미 확립된 순서 위반을 지우지 않는다 |
| `NOT_READY_UNJUDGED` | `INCONCLUSIVE(caller_requirement_absent)` | caller 수준 요구가 없거나 충돌한다(QT-4, QT-5). 상태, 반환 code, 관측된 consequence는 남긴다. 결코 위반으로 세지 않는다 |
| **`PENDING`** | **`INCONCLUSIVE(horizon-open)`** (D2) | not-ready 결과가 이미 일어났고 caller 수준 판단이 열려 있다. 결코 `TOLERATED`나 위반 없음으로 세지 않는다 |
| **`OPEN`** | **`INCONCLUSIVE(horizon-open)`** (D2) | trace 안에서 호출의 결정에 이르지 못했다 |
| `NOT_EVALUATED` | 그 조건에 대해 `INCONCLUSIVE(not_evaluated)` | 조기 반환을 기록한다. 반환 자체는 다른 조건이 판정할 수 있다 |
| 버전, 구성, 경로 근거로 보인 조건의 범위 밖 | `NOT_APPLICABLE` | 자기 근거가 필요하다. 어떤 status도 자동으로 이리로 옮기지 않는다 |

- 관측된 값 자체가 모호하면 status는 정해지지 않은 채 남는다. `NOT_READY_UNJUDGED`를 빠진 정보의 공통 값으로 쓰지 않는다.
- `INCONCLUSIVE`는 spec §4.8 "Reasons for withholding"의 이유 하나를 지닌다(목록은 [03_workflow.md](03_workflow.md) §1.3).
- 제약을 모순되게 만드는 입력(손상된 기록, 충돌하는 event)은 **분석 입력 오류**다. 따로 보고하고 결코 위반 없음으로 보고하지 않는다. 영향을 받지 않는 instance는 계속 평가하고, 보고서는 결과를 부분 결과로 표시한다.
- 요약 값은 마지막에, 칸마다 그리고 종류(관측, 예측, 확인)마다 기록에서 계산한다. 관측, 예측, 확인은 따로 보고하고 따로 센다(OUT-01, §11.8). instance의 실행 관계를 함께 보고하고, 어떤 actor의 `app_owner`가 미확정이면 소유 미확정(`ownership undetermined`)이다(spec §4.12 item 10; C-30).
- 시험 T-19(spec §9.2): 여섯 status, `OPEN`, 적용 안 됨, 관측 불확실성에서 요약이 원래 뜻과 세 판단을 지키고, 조기 반환이 결코 ready가 아니며, `PENDING`/`OPEN`이 `INCONCLUSIVE(horizon-open)`을 준다.

### 11.7 고정 순서 근거와 미확정 스케줄 의존성 (rev 1.3)

[D] (spec §1.10 "Fixed-order evidence and undetermined schedule dependence"; [P0.2] §8.4; C-11).
- 관측된 위반 하나는 그 조건이 모든 스케줄에서 위반된다는 것을 보이지 않는다. 반복된 실패 실행(spec §6.4 항목 4)도, 예측의 `NONE_IN_SCOPE`도 그것을 보이지 않는다.
- basis, 입력, 발생 대응이 같은 good 실행과 bad 실행은 그 범위에서 스케줄에 따른 차이를 보인다(`schedule-dependent`, 같은 ordinal; §11.1). 다른 build나 구성에서의 good 결과는 교정 대조이지 같은 basis의 다른 스케줄이 아니다.
- 보편 `fixed-order` 주장에는 선언한 실행 집합 전체에 걸친 순서 근거가 필요하다. certificate(spec §4.5, 선택 경로, ADR-053; §10)이거나 유한 모형의 완전한 분석(b 안의 `exh`)이다. 경계가 있는 결과는 그 경계 안에서 성립한다.
- 그런 근거가 없으면 칸은 **"스케줄 의존성 미확정"**(schedule dependence undetermined)으로 읽는다. 이것은 관측된 위반을 지우지 않는다.

### 11.8 세는 규칙 OUT-01, OUT-02 (rev 1.3)

[D] (spec §1.10 "Counting"; [P0.2] §8.2; C-12).
- **OUT-01.** 관측된 위반, 모형 후보, 확인된 실행을 하나의 탐지 수로 결코 더하지 않는다. 결함 하나가 되풀이해 일어난 것은 발생 수와 결함 수로 따로 센다(spec §8.7).
- **OUT-02.** 확인된 위반도 스케줄 의존성이 미확정일 수 있다. 고정된 틀린 순서와 순서에 의존하는 위반을 따로 둔다. R2는 다섯 사례 기록에 남고, 고정 순서가 확립된 구성은 결코 스케줄 의존 race의 탐지로 세지 않는다.

---

## 12. 보장과 가정

### 12.1 보장은 상대적이다

모든 진술은 basis B, profile Φ(hash가 붙고 모든 규칙에 인용이 있음), 모형 수준 M, 의미론 σ, 경계 b, skeleton view, 그리고 아래 가정 A에 상대적이다(spec §6). "M과 A에 상대적으로 과대 근사한다", "b까지 완전하다"처럼 쓰고, 맨 "sound"는 쓰지 않는다([R05 §0.5]). 이 설계는 Livshits et al.(CACM 2015 [V-abs])이 말하는 뜻에서 **soundy**하다. 알려진 가정과 제외를 명시적으로 나열하고, 그 밖에서 건전하도록 만든 분석이라는 뜻이다. 아래 표가 그 목록(soundiness manifest)이다. safety 진술은 유한 실행 위에서 말한다. progress 결과 `no-stall-within(b)`는 b 안의 닫힌 cone 모형에 대해서만 말하고, stall witness는 모형에 상대적이거나 wait-for 사실을 가진 실행에서 온다(보장 G11).

### 12.2 가정 A (soundiness manifest)

(spec §6.1)

| ID | 가정 | 어기면 / 어떻게 드러나나 |
|---|---|---|
| A1 | 규칙 타당성: Φ의 규칙, effect, binding이 고정 판에 대해 옳다 | 틀린 edge·effect / 인용, O3·O4·O7이 반박, certificate가 쓴 규칙 나열 |
| A2 | binding 정확성: Clang site와 AST pattern이 gcc 바이너리 명령에 대응한다 | 틀린 probe / anchor audit, DWARF compile unit binding |
| A3 | closure: actor는 B가 고른 startup script, 생성된 object table, `CreateChildTask` site, OSAL timebase/timer와 내부 thread, PSP thread, 환경 입력에서만 온다. 남은 간접 호출은 `unknown_effect` | actor 누락 / closure boot(O5); `unknown_effect` → T-SPAWN ∅, writer 집합 `DISABLED` |
| A4 | 메모리 모형: `-O0` x86-64 TSO에서 event 단위 순차 일관성. ad hoc flag 동기화는 assumed-visibility edge | weak memory 효과 누락 / `assumed` tag와, 이 가정을 쓰는 모든 결과의 sub-label `assumed-visibility`. D10(2026-10-15 결정): 그 범위 밖의 보장은 하지 않고, 가정이 성립하는지 불분명한 곳은 `unknown` |
| A5 | shape에는 API 수준 edge만. OSAL 내부 lock은 M_impl에서만 interleaving 제약 | ablation 11 |
| A6 | 계획한 site의 관측 완전성, 손실 회계 포함 | event 누락 / `tseq` 틈, `unknown(loss)` |
| A7-impl | Φ_L-1 의미가 M_impl 경로에서 kernel, libc, OSAL POSIX를 LP 단위로 과대 근사한다(`mq`의 같은 우선순위 FIFO는 UNVERIFIED) | prefix containment 실패 / 실제 OSAL litmus suite, O4 |
| A7-api | M_impl이 다루지 않는 OSAL primitive는 실제 backend에서 LP에서 linearizable하다 | 그 primitive의 prefix containment 실패 / litmus, O4 |
| A8 | 모형 밖: inline assembly, PSP 예외 경로와 `SIGUSR1` 밖의 signal, ISR, DMA, weak memory | 나열 |
| A9 | B.Input이 외부 event의 전부다(message provenance가 기댐) | handler가 잘못 접힘 / 시나리오 family를 B′로, O5 |
| A10 | M_impl은 OSAL idmap, task, binsem, queue 경로만 다룬다 | 나열; A7-api |
| A11 | table 값: POWERON은 `.tbl`, PROCESSOR reset의 critical table은 CDS image, 실행 중 적재는 B.Input으로만 | table 대조, CDS 해독 |
| A12 | process 하나. SBN peer와 cpu2는 모형 밖 | 나열 |
| A-SKEL | `Sk^cert`가 각 actor instance의 투영된 행동을 과대 근사한다. 이것은 safety 진술을 뒷받침한다. 죽음은 과대 근사로도 과소 근사로도 보존되지 않으므로 progress 결과를 B에 대해 건전하게 만들지는 않는다(L-DEAD, 보장 G11) | 거짓이거나 빠진 모형 행동 / O1, O2, O8 |
| A-WS | writer 집합 완전성: S3a(와 S3b)가 A-TYPE 아래 모든 writer를 찾고, 모든 `DECLARED_TRUSTED` guard가 옳다 | T-OBS가 건전하지 않음 / O7, 신뢰 항목 나열, O3·O4·O9, ablation 17 |
| A-TYPE | 지원 위치 접근이 모두 선언된 type을 거친다 | 타입 없는 쓰기·주소 탈출 목록, 걸리면 `DISABLED` |
| A-EXTRACT | 추출 충실성: `facts.kz`의 사실이 컴파일된 B의 source에 충실하다. AST 복사 사슬, writer site와 그 전이 분류, mod set·lock 영역을 가진 직렬화된 loop-exit 식, INIT image, 상수 평가, ALLOCS다 [D] (EXT-20261012 §4.3) | 틀린 사실 위에서 certificate가 다시 확인됨 / O4, O7, anchor audit, litmus suite, program mutant(spec §8.5); 모든 신뢰 장부 (ii)부에 나열 |
| A-CORE | 공유 `libkz-core`(Φ parser, core-expression evaluator, rule engine, container I/O)가 옳다 [D] (EXT-20261012 §4.3) | producer와 검사기가 같은 오류를 공유 / evaluator agreement property test(spec §2.3), golden test(S0); 모든 신뢰 장부 (iii)부에 나열 |
| A-FRESH, A-OWNER | 할당 신선도, 소유 관계(§2.4) | 사용마다 `DECLARED_TRUSTED`, O9 |
| A-COMM | 예측 witness마다: cone의 관측되지 않은 코드가 다른 cone actor가 쓸 수 있는 위치를 읽으면, 그 위치는 관측되었거나 상수이거나 유지된 결과로만 읽힌다 | 실패하면 `predicted-modelled` |
| A-API | 추상 trace가 SHB/WCP/SyncP의 건전성을 옮길 만큼 충실하다 | prefilter 제안은 항상 다시 검증 |
| A-CLK-PO | tracefs 선택지에서만: CPU 사이 일관 시계 | eBPF·perf-sample backend는 이 가정이 없다 |
| A-ZT | σ_R1에서만: 막는 지점 사이 계산은 시간 0 | sub-label |
| L1, L2, L3 | lemma 의무: 뽑은 predicate의 await 요약; critical section 제거(lock이 cone object가 아니고 다른 cone actor가 잡지 않으며, body가 cone 안의 어떤 것도 건드리지 않고 막는 연산이 없을 때); mover 축약 | 모형 행동이 바뀜(stall 상태 포함) / instance마다 끌 수 있고 ablation; L1 전제는 `kz proofcheck`가 추출된 mod set·lock 영역에서 확인(A-EXTRACT); 증명은 F1 전에 씀. L2와 L3가 stall 상태를 보존하는지도 의무의 일부다(litmus LP-DL이 L2가 lock 순서 순환의 모든 section을 남기는지 본다). L1의 의무에는, 요약한 poller가 요약하지 않은 poller가 결코 참이 될 수 없는 predicate의 test에서 좌초하는 바로 그 상태에서 죽는다는 것이 들어간다(§12.3) |
| T-JACM | disabling으로 확장한 source-DPOR + sleep set의 정확성 정리(JACM 2017), 상태 P | `dpor` 판정은 unverified; `exh` 모드 제공 |
| A-OBS (rev 1.3; C-33) | 관측 대응: probe에 묶인 모형 event가 profile이 말하는 값 capture, 검사, 상태 효과에 대응한다(한 지점이 호출의 원자적 효과를 대표한다는 것에는 따로 근거가 필요하다). 그리고 관측 적합성 기록(spec §2.3)이 충실하다 | 틀린 대응 위에 세운 확정 관측 판정 / anchor audit, O4, 작은 기준 모형에서의 spec §4.8 검증; 모든 관측 결과 장부의 (iii)부에 나열 |

### 12.3 Lemma

수준 M의 모형은 actor별 certificate view skeleton 자동자와 수준 M의 Φ 의미를 합친 것이고, §7.1의 전이 체계 `TS(B,σ,M)`로 본다. 아래 safety lemma는 모두 유한 실행 `Pref(B,σ_U,M)` 위에서 말한다. 증명은 유한 실행 안의 위치에 대한 귀납이라 maximal성도 공정성도 쓰지 않는다. spec §6.2는 lemma와 **증명 스케치**를 준다. 스케치가 완전한 증명인 것은 아래 표에서 `proved`로 표시한 것뿐이다.

**증명 상태** [D] (PI 결정 D20의 추가 완료 조건, 2026-10-15; spec §6.2가 한 곳에서 정의한다). spec §6.2와 §6.3의 모든 lemma와 보장은 세 값 중 하나를 지닌다. 강한 쪽부터 적는다.
- `proved`(증명 완료): spec이 완전한 증명을 담는다. §1의 정의와, 그 자신이 `proved`인 lemma만 쓴다. §6.1의 가정도, 추출된 사실도, 쓰이지 않은 단계도 없다.
- `depends-on-stated-assumption`(명시한 가정에 의존): 논증이 §6.1이 id로 적은 가정(`A-…` 항목이나 `T-JACM`) 아래에서 그 진술에 대해 완전하다. 쓰는 다른 lemma는 `proved`이거나 `depends-on-stated-assumption`이고, 표의 행이 가정을 적는다. 진술은 그 가정에 상대적으로만 성립한다. 가정은 관측으로 반박할 수 있고(신뢰 장부 (iv)부) 관측으로 증명되지 않는다.
- `unproved`(미증명): 그 밖의 모든 것. 스케치에 주장만 있는 단계가 있거나, lemma가 의무, [I], "쓸 것"으로 표시되었거나, `unproved` lemma를 쓴다.

여러 lemma를 쓰는 진술의 상태는 그중 가장 약한 것이다(**weakest link**). 구현에 대한 요구인 보장(G9, G10, G8의 재현 부분)은 정리가 아니다. 증명이 없으므로 `unproved`이고 표의 행이 말하는 시험으로 확인한다. 부분을 가진 lemma(L-CONT, L-TRANS, L-DEAD)는 부분마다 상태가 하나다.
- **(PS1)** 상태는 spec 텍스트의 속성이다. 쓴 증명을 더하고 표를 같은 변경에서 고치는 편집만 상태를 올린다. 도구 실행, 통과한 시험, 통과한 obligation(O1–O10), 장부의 항목은 결코 올리지 않고, 틈을 찾는 검토는 내린다. 표의 상태는 이 개정의 작성자 분류이고 독립 검토는 아직 기록되지 않았다 [I]. V0-P(spec §9.3)가 검토를 기록하는 곳이다.
- **(PS2)** 신뢰 장부나 spec §9.3이 의무로만 적은 lemma는 `unproved`다. 판정, 보고서, 장부 출력, README, 논문 문장은 lemma의 상태가 `proved`가 아니면 그 lemma를 "증명됨"이라, 그 lemma에 기댄 진술을 "검증됨"이나 "인증됨"이라 쓰지 않는다.
- **(PS3)** 모든 certificate, certificate 기반 판정 칸, 신뢰 장부는 자기 진술의 증명 상태를 지닌다. 쓴 (iii)부 `LEMMA` 항목 중 가장 약한 것이다(`KZ_CERT.proof_status`, `KZ_VERDICT_CELL.proof_status`, `KZ_TRUST.proof`, spec §2.3). 보고서는 그것을 safety 진술 옆에 출력하고 `proved`가 아닌 lemma를 이름으로 적는다. 예: "fixed-order — ¬OK_i, deciding (certificate, b = ∞); lemma status: unproved (L-MB, L-ONCE, L-PWB, L-FO, L1)". "`kz proofcheck`가 받아들였다"에는 늘 "신뢰 기반에 상대적으로"와 이 상태가 따른다.
- **(PS4)** `proved`는 lemma의 상태이고 판정의 상태가 아니다. lemma가 모두 `proved`인 certificate도 추출된 사실, skeleton, closure 목록, Φ, `libkz-core`, 장부의 가정에 상대적이다. 상태가 모델에 대한 진술을 바이너리에 대한 진술로 바꾸지는 않는다.

아래 표의 마지막 열이 이 개정의 분류다. R2 첫 결과의 증명 상태표는 [../plan/proof_status_R2.md](../plan/proof_status_R2.md)에 있다.

| lemma | 진술 (쉬운 말) | 증명 상태 (이 개정의 분류) |
|---|---|---|
| **L-MB** | 모든 유한 실행 ρ, 노드 `n`, ρ 안 `n`의 발생 `n_k`, `a ∈ MB(n)`에 대해, ρ에서 `n_k` 앞에 `a`의 발생이 있다. 발생에 조건을 건 진술이라 `n`이 일어나지 않는 유한 실행에 대해서는 말하지 않는다. 그래서 MB 사실로 만든 어떤 certificate도 progress를 확립할 수 없다. 귀납의 경우: 기저(∅), T-PO(`Sk^cert`에서는 실제 선행이 모두 skeleton 선행, A-SKEL), T-SPAWN(A3), T-OBS(L1, A-WS, 네 제거 규칙). lock은 기여하지 않는다 | `unproved`. 스케치만 있다. T-OBS 경우가 쓰는 PWB 사실이 같은 MB에서 도출되는데 그 공동 귀납("앞선 발생에서의 귀납 가정으로 정당화")이 주장만 되고 쓰이지 않았다. L1이 `unproved`이고 falsifier의 지배 사실은 추출된 사실이다. 공동 귀납과 L1을 쓰면 오를 수 있다 |
| **L-ONCE** | `once(a)`, `a ∈ MB(b)`이면 유한 실행 위에서 `PWB(a, b)` | `unproved`. L-MB를 쓴다 |
| **L-PWB** | FRONTIER 조건이면, `a`와 `b`의 발생을 담은 모든 유한 실행에서 `a`의 모든 발생이 `b`의 모든 발생보다 앞선다. once가 전제다 | `unproved`. L-MB를 쓰고, 경로 논증 `m →* a`는 A-SKEL에서 주장만 한다 |
| **L-SCOPE** | `SHUTDOWN_REQ ∈ MB(d)`이고 `d`의 발생이 사용 발생 `u_k`의 결정 event(특히 그 호출이나 결정 이전의 conjunct LP)보다 앞서면 `u_k`는 범위 밖이다. 결정 뒤의 conjunct LP는 scope에 영향이 없다. 첫 conjunct LP에서 정하는 scope는 두 conjunct LP 사이의 `d`에서 틀리고, 모든 conjunct LP 위에서 정하는 scope는 short-circuit하지 않는 코드에서 틀린다(litmus LP-SCOPE2). SCOPE를 쓰는 곳마다 신뢰 장부 (iii)부에 나열한다 | `unproved`. L-MB를 쓴다 (EXT-20261012 §4.1이 다시 씀) |
| **L-DERIVE** | `derives(arg, loc, c)`이면 API가 검사하는 값은 `c`에서의 `loc` 값과 같다 | `depends-on-stated-assumption`: A-EXTRACT, A-TYPE, A2. 추출된 복사 사슬에 대해 논증은 완전하다. 사슬이 gcc 바이너리에 충실하다는 것은 가정이다 |
| **L-FO** | FIXED_ORDER certificate가 있으면, 모든 유한 실행에서 결정된 범위 안 발생 `u_k`는 `ℓ_{j,k}`가 결정 이전에 있을 때 거기서 `P_j`가 거짓이다. 아니면 다른 거짓 conjunct로 먼저 결정되었거나 `NOT_EVALUATED`다. badness 조건과 함께 어떤 연장도 `u_k`를 TOLERATED로 만들지 않는다. 즉 `¬OK_i(σ_U,M,∞)`. 결정에 있는 REJECT event가 decision route다. 사용의 지배는 필요 없다. 어떤 발생이 도달한다는 것, 모든 실행이 PENDING 대신 결정에 이른다는 것은 주장하지 않는다 | `unproved`. `unproved` lemma(L-PWB, L-ONCE, L-SCOPE, L-STATUS)를 쓴다 |
| **L-PR** | PROTECTED certificate가 있으면, 모든 유한 실행에서 범위 안 발생의 conjunct `P_j`는 그 LP가 결정 이전에 있을 때 성립한다. 그래서 결정된 범위 안 발생은 모두 `READY`나 `NOT_EVALUATED`(`¬NR_i`)이고 `¬Bad_i`가 성립한다. 사용에 닿지 않는 유한 실행, stall 상태, reach, maximal 실행에 대해서는 주장하지 않는다 | `unproved`. `unproved` lemma(L-MB, L-PWB, L-ONCE, L-SCOPE)를 쓴다 |
| **L-CONT** | prefix containment(§7.5). (a) `Pref(σ_U∞,M)`은 시간 기반 전이 없는 `Pref(σ_U,M)`과 같다. (b) `π(Pref(σ_R1,M)) ⊆ Pref(σ_U,M)`. (b′) **L-CONE-R1**: bounded σ_R1 모형의 cone이 같은 h에서 σ_U 모형의 cone을 담으면, (b)의 지운 단계와 σ_U cone 밖 actor의 전이를 지운 σ_R1 실행은 σ_U 모형의 유한 실행이다. (c) B의 유한 실제 실행의 LP 단위 투영은 `Pref(σ_U,M_impl)`에 든다. 끝 상태에 대해서는 말하지 않는다. (d) 주장하지 않음: `Max(σ_U∞) ⊆ Max(σ_U)`는 거짓이다(LP-TW); 실제 실행은 (σ_U, M_impl)에서 maximal이 아니다; σ_R1 maximal containment는 증명하지 않고 쓰지 않는다. (e) σ_E 대 M_api는 주장하지 않는다. 따름정리: 시간 기반 전이가 없는 σ_U 유한 실행은 σ_U∞ 유한 실행이고, σ_U 실행의 첫 시간 기반 전이 직전 상태는 σ_U∞에서 도달 가능하다 | 부분마다 하나다. (a) `proved`: 스케치가 완전한 증명이다(같은 상태와 후속 함수, `en_σU∞`는 `en_σU`에서 TB를 뺀 것, 길이에 대한 귀납). (b) `unproved` [I]: σ_R1의 단계 종류별 경우 분석이 쓰이지 않았다. (b′) `unproved` [I] 의무. (c) `depends-on-stated-assumption`: A7-impl, A7-api, A-SKEL, A3, A4, A9. 단계 모의가 "길이에 대한 귀납"이라고만 있고 쓰이지 않았다. 무게는 가정에 있고 실행의 끝 상태에 대해서는 말하지 않는다. V0-P가 틈을 찾으면 `unproved`로 내려간다. (d), (e)는 주장하지 않는 진술이라 상태가 없다. 따름정리는 `proved`(L-CONT (a)에서) |
| **L-REACH-CONE** | 닫힌 cone C의 actor들의 전이에 시간 기반 전이가 없는 σ_U 유한 실행이 있으면, C의 actor로 투영이 같은 σ_U∞ 유한 실행이 있다. 실행 기반 σ_U∞ reach가 쓴다 | `unproved` [I] 의무 |
| **L-MAPI-CONE** | 실제 실행에서 C의 actor들의 OSAL 호출이 모두 M_api가 허락하는 결과를 받았고 C의 actor에 M_impl 전용 행동이 없었으면, C의 actor로 투영이 같은 `Pref(σ_U,M_api)` 실행이 있다. 실행 기반 M_api reach와 M_api 실행 witness가 쓴다 | `unproved` [I] 의무 |
| **L-STATUS** | 범위 안 발생의 status와 scope는 ρ의 effect event에만 달려 있다. 그래서 (a) π 아래 불변이고, σ′로 읽든 σ_U로 읽든 같다. (b) `READY`, `TOLERATED`, `VIOLATING`, `NOT_READY_UNJUDGED`와 결정된 발생의 scope는 모든 연장에서 최종이다. (c) bad 유한 실행의 모든 연장은 bad다 | `unproved`. 정의에 가깝지만, 여섯 horizon monitor가 하나씩 최종적이고 ρ의 event만 읽는다는 확인을 쓰지 않았다. 그 확인을 쓰면 `proved`가 된다 |
| **L-MON** | instance monitor는 결정적이고 전이를 막지 않는다. (a) 곱 실행은 `Pref(B,σ,M)`에 정확히 투영되고, ρ 뒤의 monitor 상태는 ρ의 함수다. (b) actor는 곱 상태에서 죽은 것과 밑 상태에서 죽은 것이 같다. (c) 곱 상태와 전이로 결정된 발생의 status와 scope는 그 둘의 함수다. 그래서 `Bad_i(o)`, `OK_i(o)`, `NR_i`는 곱 그래프 edge의 성질이고, stall 판정은 곱 상태의 함수다 | `unproved`. 열거한 monitor 상태가 모든 horizon 종류에 충분하다는 것(절 (c))이 주장만 되고 열거되지 않았다 |
| **L-TRANS** | (1a) σ_U의 `¬Bad_i`, `¬OK_i`, `¬NR_i`는 σ_U∞(같은 M, b)와 σ_R1(b = ∞, 또는 유한 b에서 L-CONE-R1 아래)에서 같은 진술로 성립한다. (2) b = ∞, M_impl이면 L-CONT (c)로 σ_E 진술이 된다. M_api는 L-MLEVEL이나 같은 M_impl 결과가 필요하다. (3) σ′의 witness, M_impl 실제 실행, L-MLEVEL이나 prefix 적합성 검사를 거친 M_api 실제 실행은 같은 M의 σ_U witness다. (부정) 존재 주장인 `Reach_i`, `OK_i`, `Bad_i`, witness, `allowed-not-ready`, stall 상태는 아래로 옮기지 않는다. 전이를 지우면 도달하는 유한 실행이 사라지고 actor가 죽을 수 있다(LP-TW). stall 상태는 위로도 옮기지 않는다. 전이를 더하면 죽은 actor가 살아날 수 있다. 예외는 L-DEAD (c), (d)뿐이다. 빈도는 σ_E 위의 측도라 σ_U가 정하지 않는다 | `unproved`. `unproved` lemma를 쓴다. (1a)의 σ_U∞는 L-CONT (a)(`proved`)와 L-STATUS(`unproved`)에 기댄다 |
| **L-MLEVEL** | cone과 certificate에 M_impl-relevant 연산이 없으면 `Pref(B,σ,M_api)`와 `Pref(B,σ,M_impl)`의 cone 투영이 같다. 전이 규칙 2(i), M_api의 규칙 3, M_api reach, 보장 G2의 M_api 갈래가 이 prefix 꼴을 쓴다 | `unproved` — **[I], 논문에서 증명할 의무.** 그 전까지 전이 규칙 2(i)에 sub-label `mlevel-lemma` |
| **L-REF** | 같은 M, b에서 timeout-dependent는 `class(σ_U) = schedule-dependent ∧ ¬Bad_i(σ_U∞) ∧ Reach_i(σ_U∞)`; timeout-rescued는 `OK_i(σ_U) ∧ ¬OK_i(σ_U∞)`; timeout-masked-liveness는 σ_U∞에서 도달 가능한 어떤 곱 상태가 σ_U∞에서 hang 상태이고 σ_U에서는 아니라는 것; `tml-root`는 §7.4. 각각 σ별 질문(spec §4.10 item 7), progress 사슬, 공유 곱 상태 공간 위의 죽음·도달성 판정의 Boolean 조합이다. 그래서 σ별 결과가 있으면 잘 정의되고 `exh`가 b 안에서 정한다. maximal 실행, 공정성, quiescence를 쓰지 않는다. 정해지지 않은 죽음 판정은 TML에 `unknown(bound)`나 `unknown(unmodelled)`를 준다. 마지막 시도가 M에서 전이 하나인 경우 v1의 틈이 닫힌다. 여러 전이인 경우는 잘못 정하지 않고 정하지 않은 채 둔다. explorer 없이는 어느 것도 확립되지 않는다 | `unproved`. progress 사슬의 결정 가능성과 v1 틈이 닫힌다는 것이 주장만 되었다 |
| **L-DPOR** | `exh`는 b 안에서 도달 가능한 모든 **곱 상태**(모형 상태와 monitor 상태)를 방문하고 거기서 허락된 모든 edge를 본다. `Q_reach`, `Q_bad`, `Q_ok`(ordinal별 포함)는 결정하는 edge의 성질이라 b 안에서 정확히 정해진다. `Q_stall`도 censored·좌초 actor와 그 closure의 판정을 빼면 정확하다. 모형 상태만의 visited set으로는 모자란다. `dpor`는 JACM 2017 정리가 이 전이 체계 class에 성립하고, 쓰는 monitor 성질이 확장된 의존(SHUTDOWN_REQ LP 대 instance의 호출·conjunct LP; 발생의 결정 event 대 사용 actor가 아닌 감시 actor와 그 spawn 사슬의 전이; 기본 scope 발생에 대해 TEARDOWN LP 대 감시 actor의 전이, spec §4.10 item 3) 아래 불변일 때, bad·good이 있는 동치류를 모두 대표한다. `dpor`는 stall 상태에 대해 주장하지 않는다 | `exh`: `unproved`. 유한 도달성은 표준이지만 censoring과 좌초 예외를 증명하지 않았다. 외부 정리는 필요 없다. `dpor`: `unproved`이고 **unverified**다(T-JACM 상태 P, PI 결정 P-PAPERS). 확장된 의존 아래 불변성은 의무 [I]. "동치류마다 실행 하나"는 주장하지 않는다 |
| **L-DEAD** | (a) 유한 전이 체계에서 i에 대한 actor의 죽음은, i에서 제외된 상태를 sink로 한 도달 가능 곱 그래프 위의 역방향 도달성 하나로 정한다. `terminal-pending`(stop 입력 없는 그래프에서 process 끝의 `AG EF`)도 하나로 정한다. 그래서 stall 상태는 결정 가능하다. bounded 모형에서 좌초하거나 잠재적으로 허락되는 actor의 판정은 정해지지 않는다. (b) 죽음은 전이 체계에 상대적이다. 전이를 더하면 죽은 actor가 살아날 수 있고, 빼면 죽을 수 있다(LP-TW). (c) **예외, deadlock 순환**: 죽은 actor들이 holder만 풀 수 있는 object(mutex, OSAL type lock)를 timeout 없이 잡으려고 다음 actor를 기다리는 순환을 이루고, witness actor가 순환에 속하거나 그런 대기의 사슬로 순환 구성원을 기다리면, 어느 σ의 어떤 전이도 그 object를 풀지 못한다. 그 유한 실행이 허락되는 모든 σ에서 stall 상태이고, qualifier만 σ마다 다시 계산한다. (d) **예외, 누수된 lock**: witness actor가 holder만 푸는 비재귀 object를 timeout 없이 기다리고, 그 holder가 object를 잡은 API에서 돌아왔으며, 끝났거나 먼저 다시 잡지 않고 풀 `Sk^cert` 경로가 없으면(K-LOCKBAL), 어느 σ에서도 풀리지 않는다 | [D] (EXT-20261012 §4.1). 부분마다 하나다. (a) `unproved`: L-MON을 쓴다. (b) `proved`: 예제 LP-TW (a)가 명시적으로 보인다. W는 σ_U에서 죽지 않고(timeout이 허락된다) σ_U∞에서 죽는다(giver가 없다). (c), (d) `depends-on-stated-assumption`: A1, A7-impl(객체가 Φ가 선언한 대로 holder 해제형이고, 그 유한 실행이 σ에서 허락된다) |
| **L1** | 전제(body가 counter·예산 밖의 관련 위치를 바꾸지 않음; test 읽기가 한 lock의 한 critical section 안이거나 원자적 읽기 하나)를 만족하는 poll loop의 조건 탈출은 test의 원자적 읽기 지점의 `await(Pred_code)` 성공에, 예산 탈출은 timeout 전이에 대응한다. σ_U∞에서는 요약의 timeout과 요약하지 않은 loop의 예산 탈출이 모두 시간 기반 전이로 지워진다. 그래서 요약은 loop의 관련 event 위의 σ_U∞ 유한 실행을 바꾸지 않는다(관련 상태를 바꾸지 않는 실패 test 읽기는 stuttering) [I]. **죽음은 바꾼다.** 요약한 poller는 `Pred_code`가 결코 참이 될 수 없는 상태에서 죽고, 요약하지 않은 것은 거기서 좌초한다. 그래서 의무에는, 요약이 stall을 지어내지 않고 정해지지 않은 판정을 정해진 것으로 바꾼다는 것이 들어간다. `CFE_ES_MainTaskSyncDelay`(`cfe_es_start.c:860–918`)는 두 M에서 전제를 만족한다(읽기가 모두 ES lock critical section 하나 안, `:878–888`). L1을 끄면 그것에 기대는 progress 결과는 `unknown(unmodelled)`다 | `unproved`. 전체 증명은 F1 전에 쓰도록 적혀 있다. 의무에는 요약한 poller가 요약하지 않은 poller가 좌초하는 바로 그 상태에서 죽는다는 것이 들어간다. `kz proofcheck`는 추출기가 낸 직렬화된 exit tree에서 predicate를 다시 유도한다. 그 tree는 추출된 사실이다(신뢰 장부 (ii)부) |
| **L2, L3** | **L2**: lock ℓ의 critical section은 (i) ℓ이 cone object가 아니고 다른 cone actor가 ℓ을 잡지 않으며, (ii) body가 cone 위치·object를 건드리지 않고 막는 연산이 없을 때 없앨 수 있다. 자기 acquire·release는 ℓ을 건드리는 것으로 센다. 그러면 σ_U에서 cone의 도달 상태와 결과도, cone의 stall 상태도 바뀌지 않는다 [I]. (ii)만으로는 모자란다. LP-DL에서 τ_B의 안쪽 `m1` section은 body가 비어 있는데, 그것을 없애면 lock 순서 순환과 deadlock이 사라진다. 조건 (i)이 그 section을 남긴다. **L3**: 조건(오른쪽 mover acquire, 왼쪽 mover release, 보호된 접근은 양쪽 mover, non-mover 많아야 하나, 모든 경로에서 release, 중첩 blocking acquire 없음)을 만족하는 critical section의 원자 블록 축약. stall 상태를 보존하는지는 L3 의무의 일부다 | `unproved`. 증명은 F1 전에 쓰고 그 전까지 **가정**이다. instance마다 끌 수 있고 ablation한다. 축약 정리 인용은 open literature check다. 둘이 stall 상태를 보존하는지도 의무에 들어간다 |
| **L-PRED** | `predicted` witness는 A-COMM을 통과한 Pred(π)의 linearization이고 A4, A7, Φ primitive 의미 아래 실현 가능하다(maximal causal models RV 2012; RVPredict PLDI 2014 [V-abs]). `predicted-extrapolated` witness는 외삽한 loop에 대해 A-SKEL에 상대적이다. `Sk^cert`에서 **horizon completion**으로 완성한 witness도 그렇다. **예측된 not-ready 근거**(예측 prefix의 `PENDING` 발생)는 witness가 아니고 어떤 class도 뒷받침하지 않는다 | `unproved`. 인용한 결과를 이 event 모형에 맞추는 것이 쓰이지 않았다 |
| **L-LOOP** | (B, Φ, tool)을 고정하면 feedback loop는 끝난다 | `unproved`. 유한 lattice와 모든 비종단 반복의 엄격한 상승은 §4.14에서 주장만 한다 |
| **L-OBS** (설명 집합의 보존; rev 1.3; ADR-055; C-34) | `Eobs`를 실제 기록에 대해 지원하는 의미론이 허용하는 구체적 설명의 집합(spec §4.8 "Explanation set": 시작 상태, 빠진 효과, 정규화와 순서 해석 포함), `Aobs`를 구현이 평가하는 추상 표현이라 하자. 그러면 `Eobs ⊆ γ(Aobs)`다. 따라서 `Aobs`가 일관하고(비어 있지 않고 모순이 없고) `γ(Aobs)`의 모든 설명이 조건을 위반하면 `Eobs`의 모든 설명도 위반한다. 위반 없음도 마찬가지다. *주장하지 않음*: 빈 `Aobs`에서 나오는 공허한 참(입력 오류, §11.6), 그리고 위반하는 설명과 위반하지 않는 설명이 모두 남는 결과(모호함이 추상화에서 왔을 수 있으므로 보류한다). 확정 관측 판정은 L-OBS와 A-OBS에 기대고, 그 장부는 증명을 쓸 때까지 L-OBS를 `unproved`로 보인다 | `unproved`(의무). 기대는 것: A-OBS, A6, A1. spec §4.8의 모든 추상 구현이 지켜야 할 포함 관계이고, 증명은 쓰이지 않았다. evaluator agreement property test(`phi_reference.md` §6)는 식 평가만 덮고 빠진 event, 정규화, 프로그램 실행은 덮지 않는다. 범위와 반례 검사: T-17–T-20(spec §9.2), 작은 모형 위의 검증(spec §4.8) |

`mover`는 다른 actor의 전이와 순서를 바꿔도 결과가 같은 전이다(오른쪽 mover는 뒤로, 왼쪽 mover는 앞으로 옮길 수 있다). **DPOR**(dynamic partial-order reduction)은 의존하는(dependent) transition이 뒤바뀔 수 있는 곳에만 backtracking 지점(다른 순서를 시도하려고 되돌아갈 지점)을 두어, 독립 연산의 순서만 다른 interleaving의 탐색을 줄이는 체계적 탐색 기법이다([R05 §2.2]; DPOR POPL 2005 [V-abs; 보장 문구 UNVERIFIED]; source/optimal DPOR POPL 2014 [V-abs]; JACM 확장 P). Kairos의 source-DPOR은 class(독립 연산의 순서만 다른 실행들의 동치류)당 실행 하나를 주장하지 않는다(spec §6.4 항목 9).

### 12.4 보장 문장

(spec §6.3)

각 보장은 증명 상태를 지닌다(§12.3). 인용한 lemma 중 가장 약한 것이다. 상태가 `proved`가 아닌 보장은 lemma가 성립하면 명시한 가정에 상대적으로 무엇이 성립하는지를 말한 것이고, 증명된 정리가 아니다.

| ID | 보장 |
|---|---|
| **보장 G1** census 과대 근사 | [증명 상태 `depends-on-stated-assumption`: A1, A3, A-SKEL, A-WS, A-EXTRACT] `Sk^cert`에서 도달 가능한 모든 준비 상태 명세 사용은 분석한 (σ, M)마다 class나 `unknown(reason)`을 가진 instance가 된다. `inactive`는 `Sk^cert`에서만 낸다. closure 밖 instance는 세어서 나열한다 |
| **보장 G2** certificate 판정(M별, safety만; rev 1.3부터 선택 경로의 보장, ADR-053) | [증명 상태 `unproved`: L-FO, L-PR, L-CONT, L-TRANS, L-MLEVEL] realization `certificate`인 수준 M의 판정은 A 아래 `TS(B,σ_U,M)`의 모든 유한 실행에 대해 safety 진술을 말한다(L-FO / L-PR). fixed-order는 `¬OK_i`를 말한다. 이 진술은 certificate의 REJECT가 발생의 결정에 있을 때만 혼자 결정적이고, 아니면 칸은 bad witness가 생길 때까지 `unknown(no_decision_route)`를 보인다. protected는 `¬NR_i`(도달한 발생 중 not ready로 결정된 것이 없음, 그래서 `¬Bad_i`)를 말한다.<br>• **다른 σ**: L-CONT와 L-TRANS로 같은 M, b의 σ_U∞·σ_R1 유한 실행에서 같은 진술이 성립한다. 옮긴 `¬OK_i`가 decision route를 지키는지는 σ마다 정한다.<br>• **σ_E**: M = M_impl이거나, M = M_api이고 cone에 M_impl-relevant 연산이 없거나(L-MLEVEL) M_impl 판정이 같을 때만 B의 모든 유한 실제 실행에 대해 성립한다. 성립하는 것은 L-FO나 L-PR이 증명하는 꼴 그대로, 그 실행의 판정된 범위 안 발생에 대한 진술이다(전이 규칙 2). `¬NR_i`는 `NOT_EVALUATED` 발생을 허용하고 모든 발생이 `READY`라고 말하지 않는다. 그 밖에는 OSAL API 계약에 대한 주장이다.<br>• **말하지 않는 것**: reach 축을 통하지 않은 reach, progress(영원한 대기, deadlock, 잃은 provider나 spawn, 닫히지 않는 horizon), maximal 실행.<br>• **검사**: 모든 certificate는 `kz proofcheck`를 통과했고, view(`Sk^cert`)와 M을 밝히며, 신뢰 장부(§10.7)를 지닌다. `kz_verdict`는 M별 결과를 저장한다 |
| **보장 G3** 관측된 준비 상태 위반 | [증명 상태 `depends-on-stated-assumption`: A1, A2, A6, O4] 기록된 trace에서 HB로 순서가 정해진 lifecycle LP로 계산한 어떤 conjunct LP의 상태가 준비 사전조건 `Req` 밖이고, 프로그램의 결과 code가 일치하며(A2, A6, O4), **그리고** 그 발생의 trace 안 status가 `VIOLATING`(horizon monitor가 trace에서 REJECT, 또는 `Allowed = none`)이거나 Allowed 미지정이면 `NOT_READY_UNJUDGED`일 때만 보고한다. `PENDING`, `TOLERATED`, `NOT_EVALUATED` 발생은 관측된 준비 상태 위반이 아니다. monitor가 OPEN인 채 trace가 끝나면 `unknown(horizon)`이다. 준비 상태 명세가 "옳은" 요구라는 주장은 하지 않고 출처와 근거 등급을 보인다.<br>• **rev 1.3** (C-35; ADR-055): conjunct LP의 상태는 기록된 event만으로 재생한 상태가 아니라 **기록의 모든 설명**에서의 상태다(spec §4.8 "Explanation set"). 그래서 요약 `OBSERVED_VIOLATION`은 모든 설명에서 위반이 성립할 때만 낸다. 이때 A-OBS와 의무 L-OBS(§12.2, §12.3)에도 기대고, 보고서는 그 증명 상태를 보인다. `NOT_READY_UNJUDGED` 발생은 관측된 사실과 요약 `INCONCLUSIVE(caller_requirement_absent)`와 함께 보고하고, 결코 준비 상태 위반으로 보고하지 않는다(spec §1.6 이름 규칙, §11.6) |
| **보장 G4** 예측 건전성 | [증명 상태 `unproved`: L-PRED] `predicted` witness는 Pred(π)에 있고 A-COMM을 통과했다(L-PRED). `predicted-modelled`와 `predicted-extrapolated` witness(horizon completion으로 완성한 것 포함)는 모형 근거다. 예측된 not-ready 근거(예측 prefix의 `PENDING`)는 witness도 class도 아니다. 예측은 maximal하지 않다. **"예측 불가"는 결코 "protected"가 아니다** |
| **보장 G5** 경계 안 모형 분류 | [증명 상태 `unproved`: L-DPOR, L-DEAD (a), L-MON. `dpor`는 T-JACM에도 기댄다] 닫힌 모형(spec §4.10 item 1의 닫힌 cone과 쓰는 reduction), σ, M, b 안에서 `exh`는 곱 상태와 그 결정 edge 위에서 safety 기록(그래서 `no-final-status`와 `ordinal-split`을 포함한 class), reach 값, refinement를 정확히 정한다. progress 결과도 정확히 정한다. 다만 censored actor나 그 closure의 죽음 판정은 `unknown(bound)`, 좌초 actor나 그 closure의 판정은 `unknown(unmodelled)`다(L-DPOR, L-DEAD, L-MON). safety 쪽 censoring이 보류한 진술은 `unknown(bound)`다. `dpor` class는 T-JACM이 성립하고 쓰는 monitor 성질이 확장된 의존(spec §4.10 item 3) 아래 불변일 때 성립하며, 그때까지 unverified로 표시한다. `dpor`는 progress 결과를 내지 않는다. 충실도 근거는 O1, O2, O6, O8이고, certificate와의 불일치는 O10으로 간다 |
| **보장 G6** 확인 | [증명 상태 `depends-on-stated-assumption`: A2, A6, 기록기의 oracle] `realized` 판정은 B의 gcc 바이너리에서 선언한 제어 아래 재현되었고, 준비 상태 oracle 평가, naturalness label, perturbation ratio를 가진다. 실현 보고에는 recorder oracle에 상대적인 거짓 경보가 없다. σ_E `schedule-dependent`에는 서로 다른 두 실행에서 **같은 발생 ordinal**의 자연 bad·good witness가 필요하다. ordinal이 다르면 `ordinal-split(σ_E)`다 |
| **보장 G7** PCT | [증명 상태 `unproved`] PI 결정 D3로 F1 gate가 승인될 때만: F1 실행 하나가 깊이 d의 bug를 gate된 스케줄 지점에 대해 확률 `1/(n·k^(d−1))` 이상으로 찾는다. PCT는 스케줄을 무작위로 골라 깊이가 정해진 bug를 확률적으로 찾는 시험 기법이다(ASPLOS 2010 [V-abs; mechanism P]) |
| **보장 G8** loop 종료와 재현성 | [증명 상태 `unproved`: L-LOOP. 재현은 시험으로 확인하는 요구다] (B, Φ, tool)이 고정되면 loop는 끝나고, 저장된 산출물에서 다시 돌리면 판정 저장소가 byte 단위로 재현된다(예산이 결정적 work unit이므로) |
| **보장 G9** 불일치 표면화 | [증명 상태 `unproved`. 구현에 대한 요구다] 층 사이 모순(O1–O10, CC)은 모두 보고하고 관련 규칙·모형을 정지·강등한다. 정지는 그 Φ 판의 loop를 끝낸다. 조용히 해소하지 않는다 |
| **보장 G10** 신뢰 사실의 투명성 | [증명 상태 `unproved`. 구현에 대한 요구다] 모든 certificate와 모든 판정 칸은 신뢰 장부(§10.7)를 지닌다. 장부는 (i) `kz proofcheck`가 다시 확인한 추론 단계, (ii) 그 단계가 읽은 추출된 사실과 `facts.kz`·`skel.kz`·`anchors.kz`의 hash, (iii) 기대는 Φ 항목, `DECLARED_TRUSTED` 항목(A-FRESH, A-OWNER, 관계 guard), 가정, lemma 의무, (iv) (ii)·(iii)의 항목을 반박할 수 있는 obligation과 그 상태, 실행된 경로의 한계, (v) (iii)의 lemma 항목마다의 증명 상태(§12.3)와 certificate 기반 진술마다의 가장 약한 상태를 나열한다. census는 T-OBS가 꺼진 conjunct 수를 보고한다 |
| **보장 G11** progress, 경계 안·모형 상대 | [증명 상태 `unproved`: L-DEAD (a), L-MON, L-DPOR] [D] (EXT-20261012 §4.1)<br>• `no-stall-within(b)`는 `exh`만 낸다. 닫힌 cone 모형, σ, M, b 안에서 i의 stall 상태(§7.3)에 닿지 않는다는 뜻이다. 닫힌 cone 규칙, 쓰는 reduction, censoring 규칙, §7.3의 제외, A에 상대적이다.<br>• `stall-witness`는 stall 상태로 끝나는 모형의 유한 실행(`model(b)`, kind·qualifier·choice support 포함)이거나, wait-for 사실을 가진 실현 실행이다(spec §4.13).<br>• `terminal-pending`은 닫힌 cone에 상대적이다. cone 밖 actor의 process 끝이 cone의 hang 상태를 process 전체에서 terminal-pending으로 만들 수 있다.<br>• **어느 값도 바이너리에 대한 진술이 아니다.** 죽음은 과대 근사로도 과소 근사로도 보존되지 않는다(L-DEAD (b)). 모형 stall witness는 확인 전까지 후보다. idle도 terminal-pending도 아닌 `deadlock-cycle`과 `leaked-lock` witness만 wait-for oracle로 확인할 수 있다. semaphore·queue·message·predicate 위의 witness, idle·terminal-pending·choice-dependent witness는 결코 hang으로 세지 않는다(spec §8.5).<br>• **모형화하지 않는 것**: VIOLATING 발생 뒤의 정의되지 않은 행동(예: C3의 검사 없는 `AppData` index)은 전이가 아니라 harm이다. 모형은 process를 계속 돌리므로, 그런 발생 뒤의 progress 결과는 그 연속에 상대적이다 [I]. K-TEARDOWN instance는 progress 결과를 받지 않는다(`unknown(teardown_scope)`). 사용 actor instance가 발생 없이 끝나거나 panic으로 끝나는 실행은 safety 축도 progress 축도 다루지 않는다(spec §6.4 item 34). Φ가 시간 기반으로 선언하지 않은 포기에 기대는 σ_U∞ 결과는 `tb-undeclared`를 지닌다. L1의 poll 요약에 기대는 결과는 L1에 상대적이다.<br>• **배제된 경로**: certificate, 예측, wait-for 사실 없는 관측, σ 사이 전이에서 progress 결과를 끌어내지 않는다. 예외는 L-DEAD (c), (d)다 |

### 12.5 하지 않는 주장

spec §6.4의 62개 항목(58–62는 rev 1.3이 더함) 중 이 문서와 직접 이어지는 것을 묶는다. 괄호 안은 §6.4의 항목 번호다.

- **안전과 완전성.** 단서 없이 "sound", "complete", "모든 race를 찾는다", "거짓 경보 없음"이라고 하지 않는다 (1). **경보가 없다는 것에서 "safe"나 "준비 상태 위반 없음"을 끌어내지 않는다.** 경계 너머, 예측·prefilter의 침묵, obligation이 실패한 모형에서도 마찬가지다 (2). 나쁜 실행이 없었다는 것에서 "timing-protected"나 어떤 σ_E class도 끌어내지 않는다 (3).
- **progress.** certificate에서 progress, 종료, deadlock 없음, 도달성을 주장하지 않는다. certificate로 얻은 protected나 fixed-order는 사용에 닿지 않는 실행, 사용 전·안·뒤의 영원한 대기·deadlock·잃은 provider나 spawn, Allowed horizon이 닫히는지에 대해 아무것도 말하지 않는다 (26). "no stall", "deadlock-free", "live"는 b 안 `exh` 밖에서 말하지 않고, 거기서도 닫힌 cone 모형에 대해서만 말한다. livelock, starvation, divergence에 대해서는 아무것도 말하지 않는다. progress 개념은 죽음뿐이다 (29). K-TEARDOWN instance의 progress 진술 (32). kind `other`이거나 idle, terminal-pending, choice-dependent인 모형 stall witness가 바이너리의 hang이라는 것, explorer 없이 refinement가 있다는 것 (33). 사용 actor나 그 spawn 사슬의 actor가 발생 없이 끝나거나 panic으로 끝나는 실행에 대한 어떤 것 (34). fixed-order instance의 모든 실행이 결정에 이른다는 것, 닫힌 cone에서 계산한 terminal-pending이 process 전체에서 성립한다는 것 (35). σ_U∞에 Φ가 선언하지 않은 포기(HS의 application monitor, TIME의 flywheel 진입)가 없다는 것, process 종료 단계 뒤에 결정된 발생에 대한 어떤 것, L1을 끈 상태에서 L1 요약에 기댄 결과가 성립한다는 것 (36).
- **의미론과 전이.** maximal 실행의 containment. `Max(σ_U∞) ⊆ Max(σ_U)`는 거짓이고(LP-TW), 실제 실행은 maximal 실행이 아니며, σ_R1 maximal containment는 쓰지 않는다 (27). σ 사이에서 reach, witness(그래서 `allowed-not-ready`, `schedule-dependent`, `ordinal-split`), stall witness(L-DEAD (c), (d)를 빼고), progress 결과를 옮긴다는 것. 전이로 얻은 σ_E progress 진술, σ_E로 옮긴 유한 경계(b ≠ ∞) 진술, σ_R1 모형이 cone을 담지 않는 σ_U 모형에서 옮긴 유한 경계 σ_R1 진술도 주장하지 않는다 (28). `ordinal-split` instance가 스케줄에 의존한다는 것. 스케줄 의존은 같은 ordinal의 두 witness를 가진 `schedule-dependent`에서만 주장한다 (30). 전이로 얻은 σ_E 칸이 실제 실행의 모든 범위 안 발생이 `READY`라고 말한다는 것. 그 칸은 판정된 발생에 대해 `¬Bad_i`, `¬OK_i`, `¬NR_i`를 말하고 `NOT_EVALUATED` 조기 반환을 허용한다 (44).
- **판정의 근거.** 반복된 실패 실행에서 fixed-order나 결정성을 끌어내지 않는다 (4). VC_R 사슬만으로 fixed-order를 내지 않는다 (5). h-cut view에서 b에 h 없이 보편 판정을 내지 않는다 (6). cone이 M_impl-relevant인 M_api 전용 판정의 σ_E 타당성을 주장하지 않는다 (7). A-COMM에 실패한 witness를 trace-sound라 하지 않는다 (8). T-JACM 미검증 동안 `dpor`의 정확성을, 그리고 "동치류마다 실행 하나"를 주장하지 않는다 (9). σ_U 예측·모형 witness에서 σ_E 빈도·class를, hold·gate 실행에서 σ_E class를 끌어내지 않는다 (10, 11).
- **검사기.** `kz proofcheck`가 추출된 사실, skeleton, closure 목록, Φ, `libkz-core`를 믿으면서 그것에 상대적으로 추론 단계를 다시 확인했다는 것보다 강한 뜻의 "독립 검증"을 주장하지 않는다. "via T5"로 표시한 단계는 공유 evaluator나 rule engine도 썼다 (31). lemma의 증명 상태가 `proved`가 아닌데 판정, 보고서, 장부 출력, 논문 문장에서 그 lemma를 "증명됨"이라, 그 lemma에 기댄 진술을 "검증됨"이나 "인증됨"이라 쓰지 않는다. `proved`를 바이너리에 대한 진술로 읽지 않는다 (45). `reach = proved`가 관측만으로 확립된다고 하지 않는다. 그것은 L-CONT (c)와, 그 칸이 필요로 하는 L-MLEVEL, L-MAPI-CONE, L-REACH-CONE에 기대며 그 증명 상태가 칸의 장부에 보인다 (46).
- **R2와 사례.** "R2는 race다"라고 하지 않는다. 근거와 증명 모양으로 보아 fixed-order 의존성 오류이고, certificate는 generic K-ID **와** framework 규칙 R19를 쓴다 (12). Kairos recorder가 재현하기 전에는 R2의 개수를 Kairos 결과로 내지 않는다 (21). 다섯 설계 사례에서 일반성을, 파생·mutant·구성 변경 시험에서 "새 버그"를 주장하지 않는다 (16). 확인 실행 전에는 OSAL F1, F7, App-record 구간을 bug라 하지 않는다 (23). class label 없이 class 정확도를 말하지 않는다. R2에는 reference class 진술이 없다. C1, C2, C3, C5의 "race" 진술을 칸별 class로 옮기는 방법은 어떤 reference도 정하지 않고, PI는 2026-10-15에 이 단어를 도구 class로 자동 변환하지 않기로 했다(P-CLASSGT). class label은 명시한 모형, 구성, 실제 근거에서만 나오고, 원문과의 일치·불일치는 정확도가 아니라 그대로 보고한다 (41). v7.0.1 기본 구성에서 C1, C2, C3, C5를 "재현"했다고 하지 않는다 (42). 한 설계 사례의 결과를 다른 사례의 근거로 쓰거나, 다섯 사례를 독립 표본으로 세지 않는다. 예: C1의 기본 B `inactive(B)` 증명은 v7.0.1 대응물에 대한 generic 층 결과이지 C1에 대한 결과가 아니다 (43).
- **표현과 판정 근거.** R2의 거부된 이벤트 요청을 "이벤트 손실"이나 "패킷 손실"이라 하지 않는다. EVS는 패킷을 만들기 전에 요청을 거절하므로(`cfe_evs.c:185–189`) 정확한 표현은 "이벤트 요청 거부"다 (22, 47). 시스템 호출이 일어났다거나 일어나지 않았다는 주장을 debugger 로그만으로 하지 않는다. 코드 분석(그렇게 밝힌다, [I])이나 정해진 조건의 시스템 호출 관측이 필요하다 (47). `rule` record만으로, 또는 추출한 후보 연속만으로 받아들임, TOLERATED status, harm을 만들지 않는다. 어떤 reference도 정하지 않은 우선순위를 충돌하는 reference 사이에 두지 않는다. K-SPAWN.osal이나 K-LOCKBAL을 준비 상태 위반(호출자 요구 위반)으로 만드는 연구자 작성 요구를 더하지 않는다 (53; QT-4, Q15, QT-5, QT-9).
- **기법의 새로움.** HB/vector clock, DPOR, PCT, lockset, 예측 분석, key 충돌 lock 반전(RVPredict/GPredict), 늦은 등록 손실(InitRacer), 예측된 use-before-init(ExceptioNULL), framework typestate(predictive typestate checking, Joshi & Sen; Lifestate)를 기법으로서 새롭다고 하지 않는다. 새로움은 cFS 의미를 둘러싼 조합에만 주장한다 (18). ad hoc 동기화의 메모리 모형 정확성은 A4로 가정할 뿐이다 (20). 명세 기반·GPredict 꼴 기법이 Allowed 결과, 유한 retry, 유한 horizon을 표현하지 못한다고 하지 않는다. 개정 1.2부터 그런 기법의 재구현은 평가에 들어 있지 않다 (37). detection(경보 유무)만으로 certificate와 예측이 더하는 것을 말하지 않는다 (38).
- **rev 1.3에서 더한 항목** [D] (C-36). R2의 관측 판정 결과를 기여의 증거로 삼지 않는다. 같은 준비 사전조건을 받은 단순 상태 검사도 같은 발생을 찾을 것으로 보며(항목 38), R2는 관측 경로의 기반 검증이다(ADR-057) (58). `PENDING`이나 `OPEN` 발생에서 나온 `INCONCLUSIVE(horizon-open)` 결과를 위반 없음이나 허용된 결과로 세지 않는다(D2) (59). 빌드하거나 실행하지 않은 기존 도구에 대해 "탐지하지 못한다"고 쓰거나 놓친 것으로 세지 않는다. 그 부족분은 문헌·코드 분석의 결과 상태로만 말한다(D4, ADR-058) (60). actor의 애플리케이션 소유가 미확정인 instance의 결과를 앱 간 결과로 내지 않는다(ADR-056) (61). 불완전한 기록 위의 보수적 판정, SMT solver의 사용, 애플리케이션 소유의 표시, certificate의 존재를 그 자체로 새로움이라 주장하지 않는다(spec §1.11 기여 가설) (62).

나머지 항목(평가 쪽 39, 40, 48–52, 54–57 등)은 spec §6.4 원문을 본다.

### 12.6 정의를 시험하는 litmus 프로그램

(spec App. A.8; EXT-20261012 §4.1–4.2)

EXT-20261012가 더한 litmus 프로그램은 실제 OSAL 위에서 gcc로 빌드할 C99 프로그램이고, 기대 결과를 σ와 M마다 적는다. **계획만 있고 하나도 실행하지 않았다.** 기대 결과는 모두 [I]다. 정의에 대한 근거이지 사례에 대한 근거가 아니다(spec App. A.8 완료 조건, §6.4 item 43).

**LP-DL: 준비는 증명되는데 사용 전에 deadlock** (EXT-20261012 §4.1).
- **프로그램.** (a) `loc`은 INIT에서 VALID이고, writer도 disabler도 없다. once actor T0가 mutex `m1`, `m2`를 만들고 τ_A, τ_B를 spawn한다. τ_A는 `m1`, `m2`를 잡고 `loc`을 쓰고 `m2`, `m1`을 놓는다. τ_B는 `m2`, `m1`을 잡고 `m1`, `m2`를 놓는다. `OS_MutSemTake`에는 timeout이 없다. (b) τ_A가 body를 두 번 돌고(상수 횟수 loop, 펼침) 반복마다 semaphore `go`를 준다. τ_B는 먼저 `go`를 받는다.
- **기대.**
  - certificate만 있으면: `protected(σ_U, M)`(b = ∞, `¬NR_i`), reach는 칸마다 `proved`나 `assumed`, progress `unknown(not_checked)`.
  - `exh`가 돌면: b 안에서 protected, progress는 `stall-witness({pre-use})`, kind `deadlock-cycle`, wait-for root는 그 순환, confirmable, harm `hang_or_deadlock`. (a)에서 그렇고, (b)에서는 ordinal (τ_A, 2)에서 그렇다. σ_U∞(다시 계산)와 M_impl에서도 같다.
  - σ_E: 규칙 2로 safety 진술. 실행이 wait-for 순환으로 멈추지 않는 한 progress는 `unknown(not_checked)`.
  - progress 값 없이 "protected"라고 쓰지 않고, 결코 `no-stall-within(b)`가 아니다.
- **확인하는 것.** L-PR의 범위; 네 질문을 모두 쓰는 progress 후보; 닫힌 cone이 τ_A 경로의 lock을 담아 τ_B가 cone에 든다는 것; **L2가 τ_B의 안쪽 `m1` section을 남긴다는 것**(그 lock은 τ_A가 잡는 cone object다. body 조건만 있는 L2는 그 section을 없앨 것이다); L3가 τ_A의 `m1` section을 잘게 남긴다는 것; (b)의 발생별 pre-use stall; 곱 상태; 보이기 규칙.

**LP-TW: 결코 성립하지 않는 predicate를 기다리는 timed wait** (EXT-20261012 §4.2).
- **프로그램.** (a) W가 아무도 주지 않는 semaphore `s`에 `OS_BinSemTimedWait(s, t)`로 기다린 뒤, 두 결과 모두에서 `loc2`(INIT에서 VALID)를 쓴다. (b) basis `B_give`/`B_skip`가 P가 `s`를 주는지를 고른다. W의 성공 갈래는 `s3`를 주고, timeout 갈래는 `abort()`를 부른다. V는 timeout 없이 `s3`를 받고 `loc2`를 쓴다(V의 사용이 instance다). (c) W가 timeout 없이 `s`를 받고 `loc2`를 쓴다. `s`를 주는 것은 litmus teardown actor X뿐이다. X는 B.Input의 stop event로 litmus SHUTDOWN_REQ store가 일어난 뒤에 돈다.
- **기대.**

  | 변형 | σ_U | σ_U∞ | 그 밖 |
  |---|---|---|---|
  | (a) | protected. reach는 timeout을 거쳐 `model(b)`이고 M_impl 실행으로 `proved`. progress `no-stall-within(b)`. flag `timeout-rescued`와 timeout-masked-liveness(W의 instance로 들어오는 사슬이 없으므로 `tml-root`도) | b = ∞에서 protected(규칙 1(a)), b 안에서 `no-final-status`, reach `unreached(b)`, progress `stall-witness({pre-use})`(hang). 도달하지 않으므로 timeout-dependent는 붙이지 않는다 | σ_E: 규칙 2로 safety 진술, reach proved |
  | (b) | 두 basis에서 `stall-witness({pre-use})`, qualifier `terminal-pending`, harm `terminal_pending(abort)`(hang 아님). abort 뒤 상태는 제외된 sink | `B_skip`에서 hang(`hang_or_deadlock`), 그래서 σ_U에 timeout-masked-liveness. `B_give`에서 `no-stall-within(b)` | — |
  | (c) | 모든 σ에서 W는 default scope instance에 대해 죽었다(제외된 상태는 sink). kind `other`의 hang이고 terminal-pending이 아니다(stop 입력은 세지 않는다). X가 준 뒤의 사용은 K-TEARDOWN instance다 | σ_U와 같다 | — |

- **확인하는 것.** prefix containment는 성립하고 maximal containment는 깨진다는 것; 전이 규칙 1–3이 칸·경계마다 일관된 기록 하나를 준다는 것; refinement가 끌어낸 predicate라는 것; 하나로 통일한 process 종료 목록, 제외, sink 규칙, terminal-pending qualifier.

**그 밖의 새 litmus와 다시 쓴 litmus** (spec App. A.8, 요약)

| litmus | 시험하는 것 |
|---|---|
| LP-LEAK | M_impl에서 RESERVED 구간의 by-name 조회가 결정에서 VIOLATING이 되고, 뒤 조회가 in-use stall(`leaked-lock`, confirmable, 첫 조회에서 오는 progress 사슬)이 되는 것; 두 conjunct use의 in-use stall(kind `other`, confirmable 아님); M_api/M_impl 갈림 |
| LP-SCOPE2 | 결정 event에서 정하는 scope, L-SCOPE, L-STATUS (b), 확장된 의존을 쓴 `dpor` 대 `exh` |
| LP-EARLY | 호출이 LP 전에 돌아오는 `NOT_EVALUATED`, 그런 호출을 가진 L-FO, O4(b), O10. retry 변형은 b 안 fixed-order, b = ∞ `unknown(no_decision_route)`. **PROTECTED 변형 (c)**(2026-10-15에 더함): 같은 API인데 registry가 INIT에서 **참**이고 writer도 disabler도 없어 PROTECTED certificate (a)가 있다. `p != NULL`인 발생은 `READY`, `p == NULL`인 발생은 `NOT_EVALUATED`다. σ_U에서 `¬NR_i`가 성립하고(L-PR), 전이 규칙 2로 M_impl의 모든 실제 실행에서도 성립한다. `lk`가 `ERR_ARG`를 돌려주는 실행도 마찬가지이고, 그 실행은 O4(b)의 실패도 σ_E 칸의 반례도 아니다(2026-10-15 전 규칙 2의 문안 "`READY` 밖이 아니다"는 그 실행에 모순되었을 것이다). reach는 `p != NULL`인 호출을 담은 실행으로만 `proved`이고, 관측한 호출이 모두 `p == NULL`이면 `assumed`다 |
| LP-ALLOW | Allowed 종류마다. 변형 (b)는 연속을 `caller` record가 인용한 경우(b-cited)와 인용하지 않은 경우(b-uncited, Q15 엄격한 대안)로 두 번 돈다. b-uncited에서는 모든 (b) 변형이 NOT_READY_UNJUDGED이고 `cont_candidate`, error exit, non-success 반환, 다음 연산의 사실이 consequence에 들어간다. 그 밖에: `rule` record만으로는 받아들이지 않음(b1r, QT-4); supplier-only 항목은 status를 정하지 않음(a2); `appreq` 충돌은 충돌을 밝히고 판정 보류(a3, d5: NOT_READY_UNJUDGED, QT-5); status의 liveness, REJECT 우선, error exit, 문서화된 전파; process 종료 단계 뒤에 판정 없음; 아직 spawn되지 않은 감시 actor와 닫힌 cone; 실제 code 모양 (b6)–(b9), (d3), (d4); 모든 변형에서 `consequence` 기록; `allowed-not-ready`는 `exh`에서만 |
| LP-RESTART | kill timer의 결과 label(`expire-stopped`는 TB가 아니고 `expire-waiting`은 TB), σ_U∞의 K-LIFE와 generation |
| L-PATIENT (다시 씀) | 지연 retry의 세 variant: σ_U∞에서 M마다 결정 전이의 시간 기반 전이, horizon monitor, 결정에 있는 REJECT만의 decision route, 경계별 safety 기록, 죽음, stall 절 집합, 좌초, refinement |

**사례 메커니즘 litmus LP-CASE** (EXT-20261012 §4.1–4.2). 설계 사례마다 그 메커니즘과 코드 모양을 v7.0.1 OSAL 위에서 재현한다. **사례에 대한 근거가 아니다**(spec §0.7 item 5).

| litmus | 프로그램 모양 | 기대 결과 (요약) | 확인하는 것 |
|---|---|---|---|
| **LP-CASE-C1** | provider P가 countsem "S"를 만든다. L은 "S"를 by-name으로 retry 조회한다(`if (ret != OS_ERR_NAME_NOT_FOUND) break; OS_TaskDelay(d);`, litmus bound). 실패하면 (r) 다른 호출 위치의 두 번째 조회로 re-bind하거나, (x) `stopped`를 저장하고 `OS_TaskDelay`에서 돈다. 구경꾼 B는 countsem "T"를 만든 뒤 조회한다. (x)에서는 청소자 Cl이 `stopped`를 기다렸다가 `OS_ForEachObject`를 부른다 | (σ_U, M_impl): key "S"의 ordinal (L, 1)에서 schedule-dependent. L 자신의 instance에는 stall 상태가 없다. `exh`의 딸린 결과는 wait-for root 하나(countsem type lock)를 가진다: P는 `FinalizeNew`에서 죽고, B는 생성에서 죽으며(조회가 앞에 있음), (x)의 Cl은 객체 순회에서, (r)의 L은 re-bind에서(별도 re-bind instance의 in-use stall) 죽는다. 모두 `leaked-lock`, confirmable, 한 번만 세고, L의 VIOLATING 시도에 사슬로 붙는다. (σ_U∞, M_impl)에서 P가 L의 마지막 조회 뒤에야 만들면 L이 좌초하고, L closure의 판정은 `unknown(unmodelled)`다. B와 P의 판정은 정해진다(L을 기다리지 않는다). (M_api)에서 P가 결코 돌지 않으면 L이 죽고 `stall-witness({pre-use, horizon})`, σ_U에 timeout-masked-liveness | M별 시간 기반 전이; 좌초와 그 closure; 다시 들어가는 leaker와 순회하는 청소자가 있는 leaked-lock; 호출 문맥별 instance; wait-for root 집합으로 묶기; `OS_CountSem*`과 순회의 lock mode |
| **LP-CASE-C2** | barrier actor Bt가 soft litmus timeout으로 P의 `declared`나 `stopped` flag를 poll한 뒤 `phase = OP`를 둔다(STOPPED가 RUNNING 위인 `AppState < AppStateId`의 대응물). R은 `phase = OP`를 기다렸다가 `svc(P)`를 부른다(`Req = late = DONE`, `Allowed unspecified_by_reference`). 변형: (buggy) P가 늦은 init 전에 선언한다; (fixed) 늦은 init 뒤에 선언한다; (fixed, fail) 늦은 init이 실패할 수 있고, 그러면 P는 `stopped`를 두고 끝난다; (fixed, cycle) P의 늦은 init이 R이 `svc` 반환 뒤에야 보내는 message를 기다린다 | (buggy) σ_U·σ_U∞ 모두 schedule-dependent, refinement 없음. (fixed) σ_U schedule-dependent(bad 실행에 Bt의 timeout이 필요), σ_U∞ `¬Bad_i`이고 도달 → timeout-dependent. (fixed, fail) `stopped`를 거친 TB 없는 bad 실행 → `Bad_i(σ_U∞)`, refinement 없음. (fixed, cycle) σ_U는 b 안 fixed-order; σ_U∞는 `unreached(b)`, b 안 `no-final-status`. R은 호출 전에 죽는다(pre-use stall, kind `other`, wait-for root는 끝 강연결 성분인 순환 {P, R, Bt}, 사슬 뿌리 없음, 세지 않음). R의 instance에 timeout-masked-liveness와 `tml-root` | 뿌리 없는 progress 결과; predicate 대기 순환의 wait-for root; 사슬 없는 instance의 `tml-root`; timeout-dependent의 reach 조건; 실패한 provider가 barrier를 통과하는 것; soft timeout이 hang을 fixed-order 준비 상태 위반으로 바꾸는 것 |
| **LP-CASE-C3** | registry `reg[]`. cell `X`(처음 0, actor E의 slot)와 cell `Y`(처음 범위 밖). E는 `Y := 0` 후 `reg[0]`을 둔다. S는 `X := 1` 후 `reg[1]`을 둔다. T는 `send(X)`(범위와 `reg[X]` 검사)를 부르고, 미등록이면 `Y`를 읽는 `notreg()`를 부른다(guard 있음, 정의되지 않은 행동 없음). instance는 (1) `send(X)`(`valid(X) ∧ reg[X]`)와 (2) `notreg()`의 `Y` 읽기(`valid(Y)`)이고, 둘 다 `Allowed none` | (1)은 T의 호출이 그것이 지닌 identity의 등록보다 앞설 때만 not ready다. E 등록 뒤 `X = 0`이면 READY인데, 이것은 K-ID가 표현하지 못하는 잘못된 귀속이다(적합성 gap으로 기록). (2)는 (1)의 not-ready 갈래에서만 생기고(사슬 (1) ⇝ (2)), E가 `Y`를 저장하지 않았으면 bad다. 둘 다 유일한 ordinal에서 schedule-dependent이고, σ_U∞에서도 그렇다 | 두 identity cell; value·registry conjunct; 준비 상태 위반 사슬; C3의 K-ID 적합성 gap |
| **LP-CASE-C5** | 등록자 G가 `cnt := 1`을 두고, task A를 만들고, mutex `m` 아래 `tab[A] = USED`를 쓴다(A가 그 전에 돌 수 있다). `cnt`를 둔 뒤 W를 spawn한다. A는 `reg_evs()`(`CFE_EVS_Register`처럼 안에서 status를 test하고 문서화된 전파로 돌려준다. A는 무시한다), `mkpipe()`(status 무시), `runloop()`(실패하면 false, `Allowed unspecified_by_reference`; 성공하면 `cnt`를 한 번 줄임)에서 `tab[self]`를 읽는다. W는 soft litmus timeout으로 `cnt == 0`을 poll한 뒤 `ops()`(`Req = cnt = 0`, `Allowed unspecified_by_reference`)를 부른다. 변형 (early-W): `cnt`를 두기 전에 W를 spawn한다 | A의 세 사용: ordinal 1에서 schedule-dependent(`runloop`는 ordinal이 여럿이지만 구간에 들 수 있는 것은 첫째뿐). W의 instance: σ_U에서 (W, 1)의 schedule-dependent(`runloop`가 실패하면 W가 timeout). σ_U∞에서 그 갈래의 W는 poll에서 죽고, A의 `runloop` 발생에 사슬로 붙은 hang이다 → W의 instance에 timeout-masked-liveness와 timeout-dependent, A의 `runloop` instance에 `tml-root`. (early-W): `cnt`를 두기 전에 W가 통과할 수 있어 TB 없는 bad 실행이 생긴다 → `Bad_i(σ_U∞)`, timeout-dependent 아님. TML은 그대로 | 안에서 test한 뒤 전파되고 무시되는 status; 무시된 status; NOT_READY_UNJUDGED; 구간에 첫 ordinal만 드는 ordinal; not-ready 뿌리를 가진 progress 사슬; timeout-dependent를 주장하기 전에 배제해야 할 counter race |

**완료 조건** (spec App. A.8).
- **EXT-20261012 §4.1.** LP-DL은 사용 전에 deadlock이 있는데 사용의 준비는 증명된다. 어떤 경로도 조건 없는 protected를 내지 않는다. 모든 칸이 reach 값과 progress 값(`unknown(not_checked)`나 stall witness)을 지니고 certificate에 기댄 곳은 lemma 상태도 지니며, 보장 G2와 spec §6.4 item 26이 certificate는 progress를 주장하지 않는다고 말한다. looper 변형과 L2의 lock 조건 덕분에 `exh`는 이 경우 결코 `no-stall-within(b)`를 내지 않는다.
- **EXT-20261012 §4.2.** LP-TW는 timeout이 있으면 진행하고 없으면 멈춘다. σ_U∞ 유한 실행은 σ_U 유한 실행이지만, σ_U∞-maximal 실행은 σ_U-maximal이 아니다. 전이 규칙은 safety 진술만 아래로 옮기고 reach와 progress는 σ마다 다시 계산한다. class는 합친 기록에서 경계마다 계산하므로, σ_U와 σ_U∞ 칸은 어떤 평가 순서에서도 서로 모순되지 않는다.
- **그 밖.** LP-SCOPE2, LP-EARLY, LP-ALLOW(실제 코드 모양 (b6)–(b9), (d3), (d4) 포함), LP-RESTART, L-PATIENT variant 3, LP-CASE-C1/C2/C3/C5는 Allowed 종류마다, 설계 사례마다 메커니즘과 코드 모양을 v7.0.1 OSAL 위에서 확인 가능하게 한다. 정의에 대한 근거이지 사례에 대한 근거가 아니다.

---

## 13. 관련 문서

이 폴더의 모든 문서는 아래 같은 문서 지도를 쓴다. 용어는 [10_decisions_adr.md §3 용어집](10_decisions_adr.md#3-용어집), 이름이 겹치는 기호는 [§0.2 이름 충돌 주의](10_decisions_adr.md#02-이름-충돌-주의), PI 결정 목록은 [§2](10_decisions_adr.md#2-pi가-내려야-할-결정)에 있다.

| 문서 | 내용 | spec |
|---|---|---|
| [README.md](README.md) | 폴더 입구: 읽는 순서, 문서별 한 줄 요약 | — |
| [01_overview.md](01_overview.md) | 문제, 네 가지 예(R2, C1, SB 구독 전 송신, OSAL RESERVED 구간), 기존 기법의 한계, 판정 체계, 기여 가설과 하지 않는 주장(그림 D0) | §0, §1, §6.4 |
| [02_architecture.md](02_architecture.md) | 계층 L0–L5, 구성요소, 두 실행 파일, typed 산출물, 데이터 흐름, obligation O1–O10(그림 D1, D2) | §2 |
| [03_workflow.md](03_workflow.md) | instance 하나의 분석 순서, feedback loop, 판정 상태 기계, 연구 단계 개요(그림 D3, D4 개요, D6, D7) | §4, §9.3 |
| **04_formal_model.md (이 문서)** | 형식 모형: actor·자원·준비 상태 명세, 의미론 σ와 모형 수준 M, MustBefore·PWB·certificate의 형식 유도, 보장과 증명 상태(그림 D10) | §1, §4.2–4.5.1, §6 |
| [05_cfs_mapping.md](05_cfs_mapping.md) | cFS 계층과 Φ_cFS의 대응, 준비 상태 명세 template K-*와 RC1–RC15(준비 범주), R2 certificate, 종료 범위(그림 D5, D8, D9) | §3, §7.3, App. A |
| [06_observation_confirmation.md](06_observation_confirmation.md) | 수정하지 않은 바이너리의 관측(`kz observe`)과 확인 실행(`kz hold`, `kz gate`), 비용 모형(그림 D7) | §4.13, §5 |
| [07_evaluation.md](07_evaluation.md) | 평가 계획: 연구 질문, 기준선, ablation, recall protocol, triage, perturbation 실험군, 통계(그림 D4 상세) | §7.2, §7.5, §8 |
| [08_roadmap.md](08_roadmap.md) | 구현 단계 S0–S10, 출구 조건, 공수 추정, 범위를 줄이는 순서, 위험(그림 D4 개요) | §9, §10.2 |
| [09_prior_work.md](09_prior_work.md) | 선행 연구와의 관계, 빌려 온 것과 새로움, [R04 §5]의 gap G1–G8(그림 D0) | §1.11, §0.3, §0.5, App. B |
| [10_decisions_adr.md](10_decisions_adr.md) | ADR 목록, PI 결정 목록(§2), 이름 충돌 주의(§0.2), 한국어 용어집(§3) | §10, §11, §13 |
| [diagrams/README.md](diagrams/README.md) | 그림 D0–D10 목록, 범례, 다시 렌더링하는 법 | §12 |
| [spec/SPEC_FINAL.md](spec/SPEC_FINAL.md) | 규범 원문(영문). 검토 쟁점 처리는 [spec/ISSUE_RESOLUTION.md](spec/ISSUE_RESOLUTION.md), PI 결정 원문은 [spec/PI_DECISIONS.md](spec/PI_DECISIONS.md) | — |
| [background/README.md](background/README.md) | 배경 보고서 [R01]–[R07]의 목록 | — |
| [history/](history/) | 설계 이력: 설계안 [A]/[B]/[C], 심사 [J1]–[J3], 검토 [X1]–[X3], SPEC v1 | — |

**이 문서와 특히 관련된 곳.**

- spec: 주로 §1, §4.2–4.5.1, §6, App. A.1, A.7, A.8을 풀어 쓴 것이다. 외부 검토에 답한 개정은 spec §0.7과 ADR-039–ADR-044에 있다. 개정 1.2의 형식 정정과 증명 상태는 spec §0.9와 ADR-045에, 2026-10-15 결정의 반영은 §0.10과 ADR-046–ADR-051에, 용어 이름 변경과 CORD 인용 정리는 §0.11, 부록 C, ADR-052에 있다. R2 certificate에 쓰는 cFS 사실과 그림 D8은 [05_cfs_mapping.md §7](05_cfs_mapping.md#7-r2-certificate-예-그림-d8)에 있다.
- 검토 쟁점과 PI 결정: [spec/ISSUE_RESOLUTION.md](spec/ISSUE_RESOLUTION.md), [spec/PI_DECISIONS.md](spec/PI_DECISIONS.md), 2026-10-15 결정문 [spec/PI_DECISIONS_RESOLVED_20261015.md](spec/PI_DECISIONS_RESOLVED_20261015.md).
- 개정 1.2의 형식 정정과 R2 증명 상태: [../plan/formal_corrections.md](../plan/formal_corrections.md), [../plan/proof_status_R2.md](../plan/proof_status_R2.md).
- 개정 1.3: 2026-10-22 결정문 [spec/PI_DECISIONS_RESOLVED_20261022.md](spec/PI_DECISIONS_RESOLVED_20261022.md), 변경표 [../plan/REV13_CHANGE_TABLE.md](../plan/REV13_CHANGE_TABLE.md), spec §0.12와 ADR-053–ADR-058. 설명 집합, 재생 규약, 보류 사유(spec §4.8)는 [03_workflow.md](03_workflow.md) §1.3에 있다.
