# 연구노트

## MLIR 기반 Flight Software Cross-Application Race 및 Temporal Dependency 분석

- 원노트 작성일: 2026-09-30
- 수정일: 2026-10-01
- 연구 상태: 문제 정의·관련 연구 조사·구현 가능성 검토 단계
- 대상 플랫폼: NASA cFS/cFE
- 원노트의 관심 대상: 앱 간 메시지 순서, startup dependency, 메시지로 갱신되는 상태, Table Services, application/service lifecycle
- 편집 범위: 원노트의 연구 방향을 유지하고, 공개 사건·프레임워크 API·학술 문헌으로 확인되는 근거를 보강한다.

**연구 구상:** cFS/cFE 소스에서 앱·메시지·서비스·상태 갱신 사이의 관계를 복원하고, 실행 순서에 따라 문제가 발생할 수 있는 지점을 MLIR 기반 정적 분석으로 찾을 수 있는지 검토한다.

이 문서에서 ‘Flight Software Cross-Application Race’는 원노트가 정한 연구 범위를 가리키는 표현이다. 독립적으로 표준화된 결함 분류나 선행 논문의 확립된 정의라고 주장하지 않는다. 실제 문헌의 용어와 정의는 각 출처의 범위에서 설명한다.

## 근거를 읽는 방법

| 표기 | 의미 |
| --- | --- |
| **[확인된 사실]** | 직접 확인한 1차 출처의 내용. 버전·플랫폼·확인한 부분의 범위에 한정한다. |
| **[원노트의 연구 구상]** | 원노트에 제시된 연구 대상·접근·실험 아이디어. 아직 구현·검증된 결과가 아니다. |
| **[해석]** | 출처에서 연구 문제로 연결한 편집자의 판단. 출처가 직접 주장한 내용과 구분한다. |
| **[미확인]** | 원문·소스·실행 자료가 부족하거나 검증하지 않은 내용. 결론의 근거로 사용하지 않는다. |

`R01`–`R11`은 학술 자료, `S01`–`S25`는 사건·공개 코드·공식 문서다. §14에 DOI·URL·확인 범위를 기록했다. 한 표 안에서도 사실과 해석이 섞이면 열이나 셀에서 구분한다. 이 문서의 의사 코드와 IR 예시는 구현 완료를 뜻하지 않는다.

**[조사 범위]** 문헌 종합은 정성적으로 수행했다. 논문마다 대상, 분석 속성, 입력, 평가 단위가 달라 정량 효과를 합산하지 않았다. 검색 완전성을 입증한 체계적 문헌고찰을 완료했다고 주장하지 않는다. 실험 결과·검출률·실행시간은 아직 없다.

## 목차

1. 원노트의 연구 문제와 용어
2. 실제 보고 사례에서 확인할 수 있는 범위
3. cFS/cFE가 제공하는 분석 근거
4. Software Bus와 앱 간 메시지 순서
5. Startup dependency
6. 메시지로 갱신되는 상태와 snapshot
7. Table Services와 lifecycle
8. 선행연구의 비교와 연구 위치
9. MLIR의 역할과 frontend 검토
10. 원노트의 정적 분석 구상에 필요한 보정
11. 구현·실험을 구체화하는 순서
12. 평가와 비교 대상
13. 남은 불확실성과 주장–근거 대응
14. 참고문헌·1차 출처·확인 상태

---

## 1. 원노트의 연구 문제와 용어

### 1.1 유지하는 연구 문제

**[원노트의 연구 구상]** 주된 관심은 앱 또는 프레임워크 서비스가 비동기적으로 실행될 때, 메시지 전달·초기화·상태 갱신·상태 사용의 순서 때문에 잘못된 동작이 생기는 경우다. 이를 분석하기 위해 다음 관계를 소스에서 복원하려 한다.

- 어떤 앱이 어떤 MID의 메시지를 발행하는가.
- 어느 pipe가 이를 구독하며 어떤 handler가 처리하는가.
- handler가 어떤 상태 필드를 갱신하는가.
- 다른 메시지·주기적 실행·서비스 호출이 그 상태를 언제 사용하는가.
- 시작 동기화·상태 검사·프레임워크 동작이 그 순서를 어떻게 제한하는가.

**[해석]** 분석의 출발점은 공개 코드에 존재하는 처리 관계다. 결함 판단은 실제로 필요한 선행 처리, 가능한 실행 순서, 관측되는 잘못된 동작을 확인한 뒤에 해야 한다. 소스에서 연결 관계를 찾았다는 사실과 race를 검출했다는 주장은 다르다. [R01, R05, R11]

### 1.2 출처가 사용하는 용어

| 용어·표현 | 출처에서 확인되는 의미 | 이 연구에서의 사용 범위 |
| --- | --- | --- |
| `data race` | [R06]은 공유 메모리의 충돌 접근과 가능한 실행 관계를 정의하고 RTOS에서 분석한다. | 인접 연구와의 비교. 앱 간 메시지 문제 전체를 이 정의로 대체하지 않는다. |
| `message race` | [R05]는 활동에 도착하는 메시지 사이의 HB 관계를 다룬다. [R11]은 전달·수신 trace와 잠재 race를 다룬다. | 메시지 순서 분석의 선행 개념. 서로 다른 논문의 정의·가정을 구별한다. |
| `race conditions / dependencies between CFE core apps` | cFE #73의 제목과 논의에서 확인된다. [S01] | 프레임워크 서비스 시작 순서 문제의 직접 근거. |
| `known race condition` | Swift/BAT Circular에서 임무팀이 사용한다. [S03–S06] | 해당 보고에서의 원인 설명으로 인용한다. 내부 구현까지 추정하지 않는다. |
| `happened-before` | [R01]이 이벤트의 인과적 부분순서를 정의한다. | 순서 추론의 이론적 근거. 물리적 시간 차이와 구분한다. |
| `data age`, `reaction time` | [R07]이 cause-effect chain의 시간 특성을 별도로 정의한다. | 원노트의 freshness 관심과 비교할 때 사용한다. 분석 끝점과 가정을 확인한다. |

**[해석]** ‘message race’가 있다는 이유로 이 연구를 그 분야의 정의 하나에 맞추어 축소할 필요는 없다. 반대로 공개 flight 사례의 모든 문제를 하나의 새로운 결함 분류로 확정할 근거도 아직 없다. 원노트의 넓은 관심 범위를 유지하면서 각 사례를 기존 문헌과 정확히 연결하는 것이 현재 필요한 작업이다.

### 1.3 원노트에서 바로잡아야 할 추론

| 원노트의 표현·추론 | 근거에 따른 보정 |
| --- | --- |
| HB가 증명되지 않으면 race다. | 증명하지 못한 이유가 분석의 불완전성일 수 있다. 실제 가능한 순서인지와 그 결과를 추가 확인해야 한다. [R01, R02, R11] |
| Swift 사례는 서로 다른 epoch의 snapshot 혼합이다. | 공개 보고는 잘못된 attitude 적용과 알려진 race를 말한다. 구체적인 epoch·thread 구조는 제공하지 않는다. [S03–S06] |
| task 생성 순서가 service 준비 순서를 결정한다. | #73은 바로 이 추론이 성립하지 않았던 개발 상황을 보여 준다. [S01] |
| 서로 다른 stream의 최신값을 함께 쓰면 오류다. | 앱이 그런 조합을 허용하는지 먼저 확인해야 한다. 시점 차이 자체만으로 결함을 정할 수 없다. [R07; S22] |
| table update와 주소 사용은 곧바로 경쟁한다. | 공개 API의 주소 보유·갱신 제한을 먼저 반영해야 한다. [S10] |
| MLIR DataFlow를 쓰면 앱 간 동시성 분석이 가능해진다. | solver 기반은 제공되지만 cFS의 메시지·queue·task 의미는 별도로 구현해야 한다. [S14] |
| 호출 그래프나 TSan이 못 찾으면 연구 차별성이 성립한다. | 약하거나 검사 대상이 다른 기준선만으로 차별성을 입증할 수 없다. cFS 모델 기반 테스트 등과도 비교해야 한다. [R03; S20] |

## 2. 실제 보고 사례에서 확인할 수 있는 범위

### 2.1 Swift/BAT

**[확인된 사실]** 아래 네 문서는 임무팀의 공개 Circular다. 각각의 보고 내용과 날짜를 확인했다. [S03–S06]

| 출처 | 보고 날짜 | 직접 확인되는 내용 |
| --- | --- | --- |
| GCN 22706 [S03] | 2018-05-11 | 4U 1416-62 관련 오식별을 설명하면서, 알려진 race 때문에 잘못된 spacecraft attitude 정보가 적용되었다고 보고한다. |
| GCN 32397 [S04] | 2022-07-14 | Cen X-3 관련 잘못된 식별을 알려진 onboard race condition으로 설명한다. |
| GCN 34691 [S05] | 2023-09-14 | SWIFT J1727.8-1613 관련 보고에서 알려진 race와 ‘7 minute problem’을 언급한다. |
| GCN 43470 [S06] | 2026-01-20 | 1A 1118-61 관련 잘못된 식별·알림을 알려진 race condition으로 설명한다. |

**[해석]** 이 자료는 onboard 처리 순서 문제가 임무의 출력에 영향을 줄 수 있다는 연구 동기를 제공한다. 같은 계열의 현상이 여러 차례 공개적으로 보고되었다는 의미는 있지만, 전체 비행 소프트웨어의 race 발생률을 알려 주지는 않는다.

**[미확인]** 이 문서만으로는 문제의 thread 구조, message queue, 공유 메모리 사용, 정확한 시간 차이, 코드상의 root cause를 알 수 없다. 해당 기능의 cFS/cFE 사용 여부도 이번 조사에서는 확인하지 못했다.

**[편집 보정]** 따라서 원노트의 `Observation(t1) + Attitude(t0)` 도식은 실제 Swift 구현의 재구성으로 사용하지 않는다. 공개 소스나 상세 원인 분석이 확보되기 전까지 Swift를 제안 분석기의 검출 대상 benchmark로 세지도 않는다.

### 2.2 cFE core application startup 문제

**[확인된 사실]** cFE #73은 Microblaze 환경에서 core task의 생성 순서와 실행·초기화 순서가 맞지 않는 문제를 기술한다. TIME·SB·EVS의 시작 의존성과 AppID 처리 등이 논의된다. 초기화 순서 변경 및 유효성 확인을 포함한 수정 기록도 있다. [S01]

**[확인된 사실]** 이슈의 GitHub 생성 날짜인 2019-09-30을 최초 발생일로 쓰면 안 된다. 이식된 Trac 댓글에는 2015-05-06 원기록과 2015년의 수정·시험 과정이 있다. 2015-06-16 댓글에는 cFE 6.4.2에서의 시험 기록이 있다. [S01]

**[해석]** 이 사례에서 분석할 핵심은 서비스 사용 경로가 해당 서비스 초기화 완료보다 먼저 실행될 수 있었는지다. task가 생성되었다는 사실, 특정 priority 값, startup 목록의 앞뒤 위치를 곧바로 초기화 완료의 증거로 사용하면 안 된다.

**[미확인]** 공개 기록은 역사적 개발·포팅 상황을 다룬다. 실제 비행에서의 발생은 확인되지 않았다. 최신 cFE에도 같은 결함이 남아 있다고 주장하지 않는다. 또한 여러 원인과 수정이 포함되므로 단일 message race라고 단순화하지 않는다.

### 2.3 관련 기록을 어떻게 사용할 것인가

**[확인된 사실]** #71은 startup synchronization의 semaphore·flag·counter와 관련된 기록이다. #73과 연결된 개발 맥락을 갖는다. [S02] NASA의 2020년 IV&V 보고서는 cFE 6.5 및 관련 앱의 정적 분석·설계 분석 활동을 기술한다. [S07, §2.1–2.2]

**[해석]** #71과 #73을 독립적인 비행 사고 두 건으로 세지 않는다. IV&V 활동의 존재를 본 연구의 검출 성능 근거로 바꾸지도 않는다. 이 자료들은 연구 동기·프레임워크 검증 맥락·역사적 재현 후보를 제공한다.

| 자료의 종류 | 가능한 사용 | 아직 할 수 없는 주장 |
| --- | --- | --- |
| 임무팀 사건 보고 | 현상의 존재와 보고된 설명 | 공개되지 않은 코드 구조·분석기 검출 성공 |
| 개발 이슈·수정 기록 | 원인 논의·버전·수정 전후 분석 | 실제 비행 발생 또는 현재 버전의 동일 결함 |
| 고정 버전의 코드 | API와 구현 경로 확인 | 모든 플랫폼·임무 버전에 대한 일반화 |
| 실제 재현 결과 | 특정 source·환경에서 가능한 오류 실행 확인 | 미실행 조건까지 포함한 발생률·완전 검출 |

## 3. cFS/cFE가 제공하는 분석 근거

### 3.1 공개 코드의 기준점

**[확인된 사실]** API 의미를 확인한 cFE commit은 다음과 같다. commit 기록의 시각은 2026-09-25T18:23:07Z다. [S25]

```text
repository: https://github.com/nasa/cFE
commit: 546a002515be5a1e3b66f9ae2c14f948d9cec76f
headers:
  modules/core_api/fsw/inc/cfe_es.h
  modules/core_api/fsw/inc/cfe_sb.h
  modules/core_api/fsw/inc/cfe_tbl.h
implementation:
  modules/es/fsw/src/cfe_es_api.c
  modules/sb/fsw/src/cfe_sb_priv.c
```

**[해석]** 이 고정점은 분석 내용의 추적을 위한 것이다. 임무에서 인증·운용된 릴리스를 의미하지 않는다. #73의 역사적 코드와도 다르다. 실제 실행 평가에서는 cFE뿐 아니라 OSAL, PSP, 앱, 빌드 옵션, MID 정의와 배치 구성을 함께 고정해야 한다.

### 3.2 공개 프레임워크의 의미를 사용하는 이유

**[확인된 사실]** 공개 cFE API에는 SB의 pipe·구독·발행·수신, ES의 시작 상태 대기, TBL의 주소 획득·해제·갱신 동작이 드러난다. [S08–S12]

**[해석]** 이처럼 호출과 프레임워크 내부 처리를 함께 확인할 수 있다는 점이 분석 연구의 구체적인 장점이다. 앱 함수 호출만 보면 드러나지 않는 message routing이나 자원 수명 관계를 복원할 자료가 존재한다. 다만 공개 문서만으로 모든 앱의 올바른 입력 시점을 알 수 있는 것은 아니다.

### 3.3 application, task, process를 구별

**[해석]** 원노트의 cross-application은 논리적 cFS 앱 사이의 관계를 뜻한다. 이를 OS process 사이의 통신 또는 앱마다 독립된 주소 공간이 있다는 주장으로 바꾸지 않는다. 앱·main task·child task·cFE instance·processor의 실제 대응은 선택한 배치의 ES·OSAL·PSP에서 확인해야 한다. [S08, S11]

**[원노트의 연구 구상]** 우선 로컬 Software Bus와 명시된 앱·서비스의 관계에 집중한다. node 간 SBN 통신, driver·interrupt, 임의 공유 메모리 경쟁, 완전한 RTOS schedule 검증은 별도 확장 문제로 남긴다. 원노트의 SBN 소개 자료를 로컬 SB API의 직접 명세처럼 사용하지 않는다.

## 4. Software Bus와 앱 간 메시지 순서

### 4.1 소스에서 복원하려는 연결

**[원노트의 연구 구상]** App A의 발행과 App B의 수신을 다음 정보로 연결하려 한다.

| 단계 | 복원할 정보 | 직접 근거 |
| --- | --- | --- |
| 발행 | 앱·호출 위치·MID·payload | source, `CFE_SB_TransmitMsg` 의미 [S09, S12] |
| 구독 | MID·pipe·구독 완료·반환 상태 | `CFE_SB_Subscribe` 계열 [S09] |
| 수신 | 성공한 수신·buffer·MID | `CFE_SB_ReceiveBuffer` [S09] |
| 분기 | MID 또는 command code에 따른 handler | 앱의 실제 dispatch source [S21] |
| 갱신 | payload에서 state field로의 복사·변환 | 앱 source와 데이터 흐름 |
| 사용 | 다른 handler·주기 함수가 읽는 state | 앱 source와 제어 흐름 |

**[해석]** 일반적인 직접 호출 그래프에는 A가 B의 handler를 직접 호출하는 간선이 없을 수 있다. MID·pipe·dispatch 관계를 추가하면 앱 간 의존성을 설명할 수 있다. 그러나 기존 분석에 프레임워크 모델을 넣어도 불가능하다는 주장은 아니다. [R03; S09]

### 4.2 전송·수신의 중요한 세부 동작

**[확인된 사실]** 고정 commit의 API와 구현에서 다음을 확인했다. [S09, S12]

| 항목 | 확인한 내용 | 분석상의 의미 **[해석]** |
| --- | --- | --- |
| `TransmitMsg` | 전송 과정에서 메시지 내용을 복사하며, 호출이 반환되기 전에 수신 task가 실행될 수 있다. | 발행 호출의 반환 시점을 모든 수신 처리보다 앞에 놓으면 안 된다. |
| 구독자 없음 | 확인한 전송 구현은 목적 pipe가 없을 때 0개 pipe를 처리하는 경로를 가진다. | 발행 API의 성공을 수신자 준비·처리 완료와 동일시할 수 없다. |
| 목적지별 전달 | 전송 구현은 목적지별 queue 삽입과 그 오류를 처리한다. | 모든 구독자에게 한 번에 원자적으로 전달되는 것으로 가정하면 안 된다. |
| `SubscribeEx` | `MsgLim`은 동일 MID의 메시지가 pipe 안에 존재할 수 있는 수를 제한한다. | 전체 pipe 깊이와 MID별 제한을 구별해야 한다. |
| `ReceiveBuffer` | 성공·timeout·no-message·오류 경로가 있다. | 성공한 수신 경로와 기존 state를 계속 쓰는 경로를 구별해야 한다. |
| 수신 buffer | 동일 pipe의 다음 `ReceiveBuffer` 호출까지만 유효한 읽기 전용 포인터다. | payload 복사와 buffer 주소 보관을 같은 상태 갱신으로 보면 안 된다. |

**[미확인]** 선택할 OSAL/PSP의 queue 순서·선점 동작을 아직 전부 검증하지 않았다. 다른 pipe들 사이의 전역 순서, 전달 지연의 상계, 모든 상황의 손실·복구 특성을 이 header만으로 주장하지 않는다.

### 4.3 MID 연결만으로 race가 결정되지 않는 이유

**[확인된 사실]** [R01]의 send–receive 관계는 동일한 메시지의 전송과 수신을 연결한다. [R11] 역시 전달과 수신을 구별하고 잠재적인 race 후보에 추가 조건이 필요함을 설명한다.

**[해석]** 같은 MID는 메시지 종류를 연결하지만, 반복 발행 중 어느 메시지를 받았는지는 결정하지 않는다. 따라서 `publish(MID_X)`와 `receive(MID_X)`가 있다는 사실만으로 상태의 최신 갱신이나 특정 계산 이전의 수신을 보장할 수 없다.

**[원노트의 연구 구상]** 연결 그래프를 먼저 복원하고, 실제 수신 성공·handler 분기·상태 갱신·사용 순서를 추가 분석한다. graph의 간선은 통신 가능성을 나타내는지, 실제 처리의 선후를 나타내는지 구분해 기록한다.

### 4.4 첫 메시지와 구독 준비

**[해석]** 구독 전에 발행할 수 있다는 사실은 first-message 문제의 후보를 만든다. 하지만 그 첫 메시지를 반드시 받아야 하는지, 후속 주기 메시지로 회복하는지, 초기 상태를 사용하지 않도록 guard가 있는지를 확인해야 한다. [S09, S12]

| 확인할 코드·설명 | 검토할 이유 |
| --- | --- |
| 일회성 발행인지 주기적 발행인지 | 첫 전달을 놓친 경우의 결과가 다름 |
| 해당 수신자가 필수인지 | 구독자가 없는 발행을 정상적으로 허용할 수 있음 |
| 구독의 성공 여부·실패 처리 | 구독 호출의 존재만으로 준비 완료가 되지 않음 |
| 최초 데이터 이전의 계산 경로 | 메시지 유실이 실제 잘못된 state 사용으로 이어지는지 확인 |
| request–reply·재전송·ready 처리 | 복구 및 동기화 경로가 실제로 존재하는지 확인 |
| queue full·MsgLim 처리 | startup 순서 외의 전달 실패를 구별 |

## 5. Startup dependency

### 5.1 분석 대상과 직접 근거

**[원노트의 연구 구상]** startup에서는 서비스 초기화 완료와 그 서비스의 최초 사용 사이의 관계를 확인하려 한다. #73이 이를 검토할 직접적인 개발 사례다. [S01]

**[해석]** 필요한 것은 함수명이 `Init`인지, startup 목록의 어디에 놓였는지에 대한 검사만이 아니다. 초기화가 여러 단계로 나뉘는지, 준비 상태를 언제 기록하는지, 다른 task가 어떤 API를 먼저 호출할 수 있는지까지 보아야 한다.

### 5.2 동기화 API의 실제 동작

**[확인된 사실]** `CFE_ES_WaitForSystemState`는 timeout을 가질 수 있고 상태를 반환한다. `CFE_ES_WaitForStartupSync`는 `OPERATIONAL`을 기다리는 호출을 하는 `void` wrapper이며, 확인한 구현에서는 그 반환 상태를 호출자에게 전달하지 않는다. [S08, S11]

**[해석]** 따라서 ‘startup sync 호출이 있음’이라는 조건만으로 정상 준비를 보장했다고 판정하면 안 된다. timeout 이후에도 실행하는 경로, 실패 때 종료하는 경로, 별도로 상태를 재확인하는 경로를 구별해야 한다.

**[해석]** 시스템 단계의 완료와 앱이 필요로 하는 measurement·명령·table 내용의 준비도 다르다. 프레임워크가 operational이라는 이유로 앱의 모든 입력이 도착했다고 추론할 수 없다.

### 5.3 역사적 결함 재현 시 필요한 자료

**[원노트의 연구 구상]** 원노트는 cFE의 실제 startup 문제를 연구 평가에 활용하려 한다. 이를 실제 재현으로 보고하려면 다음 자료가 필요하다.

- 문제가 있던 source revision과 해당 환경.
- 이슈에서 지적한 초기화·사용 경로.
- 수정 전후의 구체적 변경과 그 목적.
- 실행 가능한 task 순서와 관측한 결과.
- 분석기가 보고한 위치가 원래 문제와 연결되는 근거.

**[미확인]** 이 자료를 모아 원버전 재현을 완료하지 않았다. 원문 내용을 단순화해 만든 예제는 해당 기록에서 파생한 예제로 표시해야 하며, #73 자체의 검출 성공으로 보고하면 안 된다.

### 5.4 재시작과 대기 문제

**[해석]** 앱이 재시작될 수 있다면 최초 startup에서 확보한 준비 상태를 계속 적용할 수 있는지 따로 확인해야 한다. 서로의 준비를 기다리는 코드에서는 진행이 막히는 문제도 생길 수 있으나, 이를 분석하려면 실제 대기·timeout·복구 동작을 확인해야 한다. 원노트의 lifecycle 관심에 해당하지만, 아직 일반적인 progress 증명을 제공하는 단계는 아니다. [S08, S11]

## 6. 메시지로 갱신되는 상태와 snapshot

### 6.1 원노트가 분석하려는 상태의 연결

**[원노트의 연구 구상]** 메시지의 payload가 앱의 상태에 저장되고, 이후 다른 메시지나 주기 함수가 그 상태를 읽는 관계를 복원하려 한다. 이 관계는 단순한 publisher–subscriber 연결보다 넓다.

```text
설명용 의사 코드 — 실제 임무 코드·실행 결과가 아님

OnAttitudeMessage(msg):
    attitude_state = copy(msg.attitude)
    attitude_valid = validate(msg)

OnTrigger():
    if attitude_valid:
        Compute(attitude_state)
```

**[해석]** 위와 같은 구조를 분석하려면 `OnTrigger`가 보는 state가 어느 수신에서 왔는지, validation이 무엇을 보장하는지, reset이나 다른 task가 값을 바꾸는지를 확인해야 한다. 함수·변수 이름만 보고 실제 의미를 정할 수 없다.

### 6.2 메모리 초기화와 유효한 입력 수신

**[해석]** 원노트의 ‘uninitialized state’는 두 의미를 구별해야 한다. 메모리의 값이 정의되어 있다는 것과, 앱이 필요로 하는 정상 입력을 받아 상태를 설정했다는 것은 다르다. 0이나 기본값이 들어 있어도 실제 measurement가 없을 수 있고, 반대로 앱이 그 기본값 사용을 허용할 수도 있다.

**[원노트의 연구 구상]** 다음 내용을 함께 추적한다.

- 최초 값을 설정하는 경로.
- 메시지 수신 이후의 validation과 갱신.
- 상태 사용을 막는 flag·mode·분기 조건.
- 잘못된 입력을 버리는 경로와 저장하는 경로.
- reset·재시작 이후의 상태.

**[미확인]** 실제 앱의 의도와 초기값 허용 여부가 없으면 ‘최초 메시지 이전에 읽음’을 무조건 race로 부를 수 없다. 초기화 순서가 달라져 문제가 생기는지와, 순서와 관계없이 코드가 잘못되었는지도 구분해야 한다.

### 6.3 여러 메시지의 상태를 함께 사용하는 경우

**[원노트의 연구 구상]** attitude·position 등 여러 입력이 따로 갱신되고 한 계산에서 함께 사용되는 경우를 분석 대상으로 검토한다. 각각의 출처와 갱신 시점을 연결하여 어떤 조합이 가능한지 확인하려는 것이다.

**[해석]** 서로 다른 시점의 값이 섞였다는 사실만으로 오류라고 결정할 수는 없다. 알고리즘이 latest-value 조합을 허용하는지, 동일 measurement에 대응하는 입력을 요구하는지, interpolation이나 buffering이 있는지 확인해야 한다. [R07]

**[확인된 사실]** NASA HK의 README는 여러 앱의 telemetry를 조합하는 기능을 설명한다. [S22] **[미확인]** 이 설명만으로 HK의 데이터가 모두 동일 epoch여야 한다고 볼 수는 없다. 상세 source·요구를 확인하지 않고 실제 결함을 부여하면 안 된다.

### 6.4 분석이 가짜 조합을 만들 수 있는 경우

**[해석·분석 예시]** 실제 source의 한 분기에서는 `(A,B)=(k,k)`만 만들고 다른 분기에서는 `(j,j)`만 만든다고 하자. A와 B의 가능한 값만 따로 모으면 둘 다 `{k,j}`가 된다. 이를 독립적으로 조합하면 source에 없던 `(k,j)`를 만들어 낼 수 있다.

**[원노트의 연구 구상에 대한 보정]** state origin을 집합으로 추적할 때 분기·공동 갱신 관계를 얼마나 보존하는지 확인해야 한다. 정보를 잃어 생긴 조합을 실제 snapshot 문제로 확정하지 않는다. 이런 근사와 구체 실행의 관계가 정적 분석의 검증 대상이다. [R02]

### 6.5 Freshness를 다루려면 필요한 정보

**[확인된 사실]** [R07]은 reaction time과 data age를 서로 다른 시간 특성으로 정의하고, task·실행·시계·통신 가정을 사용해 분석한다. 이 논문의 actuation까지의 data age와 앱의 어떤 함수가 입력을 읽을 때의 나이는 끝점이 다를 수 있다.

**[해석]** HB는 선후 관계를 제공하지만, 그것만으로 입력이 몇 ms 이내의 값인지 알 수는 없다. 다음 정보를 확보한 범위에서만 원노트의 freshness 분석을 구체화할 수 있다. [R01, R07]

| 필요한 정보 | 확인할 사항 |
| --- | --- |
| timestamp의 의미 | 측정·계산·발행·수신 중 언제 기록되는 값인가? |
| clock와 단위 | 같은 시간 기준인지, 변환·동기화 오차가 있는가? |
| 데이터 사용 위치 | 실제로 어떤 state의 timestamp를 확인하는가? |
| 허용 가능한 시간 차이 | 앱의 요구나 알고리즘 설명에 근거가 있는가? |
| reset·wrap·time jump | 단순 차감이나 counter 비교가 유효한가? |
| queue·task 대기 | 값의 생성과 사용 사이 지연을 어디까지 설명할 수 있는가? |

**[편집 보정]** 원노트에 등장한 `20 ms`, `100 ms` 등은 임무 근거가 확인된 기준이 아니다. 실제 검출 기준이나 기본 parameter로 채택하지 않는다. 자료가 없으면 freshness 판단을 유보하고, 확인 가능한 갱신·사용 순서를 먼저 분석한다.

## 7. Table Services와 lifecycle

### 7.1 공개 API가 실제로 보장하는 내용

**[확인된 사실]** `CFE_TBL_GetAddress` 문서는 얻은 주소를 table update 또는 blocking 호출 전에 해제하도록 요구한다. 주소가 해제되지 않은 동안 table update가 일어날 수 없다고 명시한다. [S10]

**[확인된 사실]** `CFE_TBL_ERR_NEVER_LOADED`가 반환되는 경우에도 0 내용의 유효한 table 포인터를 얻을 수 있으며, 그 포인터를 해제해야 한다. [S10]

**[해석]** 주소를 보유한 정상적인 읽기와 table update가 무방비하게 경쟁한다고 보는 원노트의 설명은 수정해야 한다. 프레임워크가 제공하는 보호 동작을 먼저 모델에 넣고, 그 사용 규칙을 지키는지 확인해야 한다.

### 7.2 원노트의 검출 후보를 다시 해석

| 원노트의 관심 항목 | 출처에 맞추어 확인할 내용 | 바로 단정하면 안 되는 내용 |
| --- | --- | --- |
| Missing release | 주소 획득 경로별로 해제가 수행되는지, blocking·update 전에 남아 있는지 | 획득 성공만 추적하면 충분하다는 가정 |
| Use after release | 해제 뒤 API에서 얻은 주소를 계속 사용하는 경로가 있는지 | 해당 C 메모리가 즉시 free되어 반드시 crash한다는 주장 |
| Update blocked by reference | 주소 보유와 update 진행 사이의 관계 | update가 동시에 진행되어 항상 값이 찢어진다는 주장 |
| 앱 간 table 공유 | 실제 share·handle·주소 사용·종료·재시작 경로 | 앱 간 공유라는 사실 자체가 race라는 판단 |

**[해석]** 이 항목 중 일부는 동시 실행 순서와 직접 관계가 있을 수 있고, 일부는 순서 경쟁 없이도 발생하는 API 오용일 수 있다. 공개 기록이나 재현을 통해 race와의 관계를 확인하기 전에는 모두 같은 결함으로 묶지 않는다.

### 7.3 연구에서의 위치

**[원노트의 연구 구상]** Table Services는 메시지 흐름 분석 뒤에 확장할 대상으로 둔다. 먼저 register/share, address acquisition, use, release, update의 실제 코드 경로를 복원한다. 오류 반환·blocking·재시작을 잃지 않는지 확인한다.

**[미확인]** 다양한 buffering 설정·앱 공유·종료 상황을 포함한 TBL 전체 의미의 형식화와 실행 검증은 완료하지 않았다. 현재 근거는 고정 버전 API의 확인한 범위다.

## 8. 선행연구의 비교와 연구 위치

### 8.1 조사 방식

**[조사 기록]** cFS concurrent Software Bus testing, cFS/TASTE toolchain, static message race/order types, RTOS static race detection, cause-effect chain timing, cFS runtime monitoring, MLIR dataflow 등을 조사했다. 출판사·저자·기관·공식 프로젝트의 자료를 우선 확인했다. 검색 결과 수·제외 수·중복 제거 수에 대한 전수 로그는 없으므로 그러한 숫자를 제시하지 않는다.

**[해석]** 이 문헌군은 원노트의 문제를 구체화하기 위한 정성적 비교다. 공개 접근 가능한 자료에 편향되어 있고 관련 문헌을 모두 찾았다는 보장은 없다. 전체 원문을 읽지 못한 논문에서는 기능의 부재나 성능상의 한계를 추정하지 않는다.

### 8.2 직접 비교해야 하는 cFS 연구

| 연구 | **[확인된 사실]** | 확인 수준 | **[해석]** 비교해야 할 점 |
| --- | --- | --- | --- |
| Ganesan et al., ISSRE 2016 [R03] | cFS Software Bus의 requirements model과 Spec Explorer 기반 동시성 테스트 생성 | 공식 초록·기관 서지; 전체 원문 미확보 | 소스에서 앱 상태·순서를 복원하는 분석과 어떤 정보·속성을 공유하는가? |
| Valente et al., TECS 2025 [R04] | cFS와 TASTE 통합, 모델 기반 code generation, UPMSat-2 사례 | 출판사 초록·색인된 본문 일부·기관 서지 | 설계 모델과 구현의 관계를 어떻게 유지하며, 원노트의 source 분석과 무엇이 다른가? |
| Perez et al., TACAS 2022 [R09] | FRET–Ogma–Copilot을 통한 runtime monitor 생성과 cFS 연계 | 원문 §3–5 확인 | 실행 중 관측하는 검사와 소스에서 가능한 순서를 찾는 분석의 차이는 무엇인가? |

**[해석]** 따라서 ‘cFS 동시성의 최초 분석’, ‘cFS를 처음 모델링함’과 같은 주장은 현재 근거에 맞지 않는다. 도구체인·모델 기반 테스트·runtime monitoring이 이미 존재한다는 사실을 인정한 상태에서 원노트의 기능을 비교해야 한다.

**[미확인]** R03·R04가 원노트와 같은 앱 간 state origin·순서 분석을 어느 정도 제공하는지 아직 충분히 확인하지 못했다. 초록에서 보이지 않았다는 이유만으로 해당 기능이 없다고 쓰지 않는다.

### 8.3 순서·race·시간 분석 연구의 관계

| 문헌 | 출처에서 확인되는 내용 | 원노트와 연결되는 부분 **[해석]** | 그대로 옮길 수 없는 부분 |
| --- | --- | --- | --- |
| Lamport 1978 [R01] | 이벤트 부분순서, message send–receive, 논리 clock | HB를 이용하는 순서 분석의 기본 근거 | cFS API 내부의 전송·실패·queue 의미 |
| Cousot & Cousot 1977 [R02] | lattice·fixpoint·추상화의 정합성 틀 | 원노트의 데이터 흐름 요약을 검증하는 방법론 | 이 연구에 필요한 구체 도메인·transfer 함수 |
| Order Types 2017 [R05] | 비동기 활동의 communication·flow·HB 정보를 결합한 정적 추론 | message ordering을 source 의미와 함께 분석 | activities·futures 기반 언어와 cFS의 차이 |
| RTOS race detection 2020 [R06] | priority·동기화 의미를 반영한 shared-memory race 분석 | 실제 가능한 interleaving을 판단할 때 플랫폼 의미가 중요함 | 메시지 전달·상태 snapshot의 의미는 별도로 필요 |
| Cause-effect chains 2021 [R07] | data age·reaction time과 실행·통신·시계 가정 | freshness의 정확한 의미와 필요한 입력 정리 | 특정 task model·실행시간 조건을 일반 cFS에 적용 |
| Giotto 2003 [R08] | time-triggered embedded programming의 시간 명세 | 시간 관계를 코드 이름만으로 추정할 수 없다는 비교 맥락 | cFS가 Giotto와 같은 실행 의미를 가진다는 가정 |
| MLIR 2021 [R10] | 확장 가능한 operations·dialects·SSA·regions·interfaces | 도메인 의미를 보존하는 분석 표현 | cFS 전용 분석기가 이미 제공된다는 주장 |
| Message race trace preprint [R11] | send·deliver·receive 구분, 잠재 race와 matching 제한 | 그래프 후보와 실제 가능한 수신을 구별 | Erlang 계열 trace의 가정을 cFS에 적용 |

**[확인된 사실]** R05는 §3.8에서 종료성·건전성의 형식화와 증명을 후속 과제로 남긴다. 따라서 이를 완전한 soundness 증명을 제공한 연구라고 설명하지 않는다. R11은 이 노트에서 preprint로 확인한 자료이며 최종 peer-reviewed 출판 상태를 확인하지 않았다.

### 8.4 문헌을 종합해 얻을 수 있는 판단

**[해석]** 문헌을 비교할 때 적어도 네 가지를 구분해야 한다.

1. 입력이 source, 설계 모델, 실행 trace 중 무엇인가.
2. 검사하는 문제가 공유 메모리 race, message ordering, bus 동작, 시간 지연 중 무엇인가.
3. 가능한 실행 전체를 다루는지, 제한된 모델을 탐색하는지, 관측된 실행만 확인하는지.
4. queue·priority·clock·언어·동기화에 어떤 가정이 있는지.

이 차이를 무시하고 서로 다른 논문의 검출률·runtime을 합산하거나 순위를 매길 수는 없다. 본 연구에서도 source 입력과 검사 대상이 같은 경우에 비교해야 한다.

### 8.5 원노트의 차별화 구상에 대한 현재 평가

**[원노트의 연구 구상]** cFS API의 메시지 연결, 앱 내부 상태 갱신·사용, startup·lifecycle 순서를 하나의 source 분석 흐름에서 연결하려는 것이 핵심이다. MLIR는 이 의미를 유지하고 분석하는 기반으로 검토한다.

**[해석]** 이 연결의 실질적 필요성과 기존 방법 대비 이점은 조사할 이유가 있다. 특히 메시지 topology만으로 설명되지 않는 앱 내부 소비 관계가 비교 지점이 될 수 있다. 그러나 지금은 독창성이 확정된 기여가 아니라 검증해야 하는 원노트의 구상이다.

**[미확인]** 기존 연구보다 더 많은 실제 오류를 찾는지, 오경고를 줄이는지, 구현·모델링 부담을 줄이는지에 대한 결과는 없다.

## 9. MLIR의 역할과 frontend 검토

### 9.1 확인된 기능과 연구자가 구현할 부분

**[확인된 사실]** MLIR는 operations, types, attributes, regions, SSA, dialects와 interfaces를 통해 여러 수준의 IR을 구성하는 기반을 제공한다. DataFlowAnalysis 기반도 제공한다. [R10; S13–S15]

**[원노트의 연구 구상]** cFS API를 단순 외부 함수 호출로만 남기지 않고, 구독·발행·수신·상태 갱신·시작 대기 등의 의미를 가진 연산으로 표현하려 한다.

| 원노트의 표현 구상 | source와 연결할 정보 | 검증할 사항 |
| --- | --- | --- |
| `cfs.sb.create_pipe` | pipe identity·depth·반환 상태 | 생성 실패가 누락되지 않는가? |
| `cfs.sb.subscribe` | MID·pipe·옵션·성공 여부 | 구독 호출과 구독 완료를 구별하는가? |
| `cfs.sb.publish` | MID·payload·발행 위치 | 목적지별 전달·오류·호출 중 선점을 표현하는가? |
| `cfs.sb.receive` | pipe·timeout·수신 buffer·status | 성공한 수신과 buffer 수명을 보존하는가? |
| state update/read | state field·source payload·소비 위치 | alias·분기·다른 task의 영향을 잃지 않는가? |
| startup·table 관련 표현 | wait·status·주소 획득·해제·update | 실제 ES/TBL 의미와 맞는가? |

**[미확인]** 위 `cfs.*` 명칭은 원노트의 dialect 설계 예시다. 구현되어 사용할 수 있는 공식 MLIR 연산이라는 뜻이 아니다. 정확한 type·effect·verifier·전이 의미는 아직 검증되지 않았다.

### 9.2 MLIR가 자동으로 해결하지 않는 부분

**[해석]** SSA use-def 관계만으로 C의 mutable global state, pointer alias, queue, task interleaving이 모두 복원되지는 않는다. 어떤 연산이 메모리나 자원을 변경하는지와 어떤 source 동작을 뜻하는지를 분석 구현이 제공해야 한다. [R10; S13–S15]

**[해석]** IR verifier를 통과하는 것과 source의 동작이 올바르게 보존된 것은 별개다. 반환 오류를 생략한 IR도 구조적으로는 유효할 수 있다. API별 source와 IR의 동작을 대조해야 한다.

**[원노트의 연구 구상에 대한 보정]** 초기에는 한 task의 handler 안에서 state가 어떻게 바뀌는지와 앱 사이의 메시지가 어떤 순서로 전달될 수 있는지를 구별하여 분석한다. 로컬 데이터 흐름을 구한 뒤 전역 실행 관계를 어떻게 연결할지 검증한다. DataFlow solver를 도입했다는 사실을 cross-app race 검출 성과로 보고하지 않는다.

### 9.3 원노트의 LLVM IR 경로를 조건부로 유지

**[확인된 사실]** MLIR의 LLVM IR import 문서는 experimental 상태와 지원 subset의 제한을 설명한다. CIR 문서도 upstream 진행 상태와 기본 build 지원에 대한 경고를 제공한다. [S16, S17]

**[해석]** 따라서 원노트의 `C → LLVM IR → MLIR LLVM dialect → cFS 의미 복원` 경로는 실제 cFS source에서 시험해야 하는 가설이다. field·매크로·source location·API 의미가 필요한 경우, 낮은 IR 단계에서 이를 다시 복원하는 비용도 확인해야 한다. [R10]

| 경로 | 확인된 기반 | 남은 검토 |
| --- | --- | --- |
| LLVM IR import | 공식 import 기능 [S16] | 지원하지 않는 명령·타입, 구조·source 정보 손실 |
| CIR 활용 | 공식 개발 문서 [S17] | 선택 버전의 실제 build·언어 지원·도구 안정성 |
| Clang AST/CFG 활용 | LibTooling·Compilation Database [S18, S19] | 필요한 cFS 호출·field·분기 정보를 복원할 수 있는지 |

**[해석]** AST/CFG 경로는 검토 가능한 대안이다. 이를 새로운 필수 아키텍처로 확정하지 않는다. 실제 앱의 build·추출 가능성과 의미 보존을 확인한 뒤 원노트의 frontend 선택을 정해야 한다.

### 9.4 MLIR 사용의 실질적인 정당성

**[원노트의 연구 구상]** 원노트는 단순한 API 목록 추출을 넘어 상태 출처·제어 흐름·앱 간 관계를 분석할 때 MLIR의 역할이 생긴다고 본다.

**[해석]** 이 주장을 검증하려면 같은 의미를 담은 일반 그래프나 AST 기반 분석과 비교할 필요가 있다. 필요한 정보가 단순 그래프만으로 충분히 유지된다면 MLIR의 기여는 제한적일 수 있다. typed operation, interface, source mapping, analysis 재사용이 실제로 무엇을 개선하는지를 제시해야 한다. [R10]

**[미확인]** MLIR를 사용하면 자동으로 정확도·성능·확장성이 향상된다는 결과는 없다. 도메인 의미를 추가한 효과와 MLIR 구현 기반의 효과를 구분해야 한다.

## 10. 원노트의 정적 분석 구상에 필요한 보정

### 10.1 Happens-before의 출처

**[확인된 사실]** Lamport의 happened-before는 같은 실행 주체의 이벤트 순서, 동일 메시지의 send–receive 관계, 추이성으로 정의된다. 논리 clock의 수치상 선후가 곧 인과 관계를 증명하는 것은 아니다. [R01, pp. 558–560]

**[원노트의 연구 구상]** 앱 내부 순서, 메시지 전달, 동기화, startup·lifecycle에서 얻는 선후 관계를 활용하려 한다.

**[해석]** source에서 얻는 관계와 분석자가 요구된다고 판단한 순서는 구분해야 한다. 필요한 순서를 실제 보장된 순서에 먼저 넣어 놓으면 문제를 검사하기 전에 정상이라고 가정하는 셈이 된다.

| 분석에 사용하는 관계 | 근거로 확인할 내용 |
| --- | --- |
| 동일 task의 순서 | 실제 도달 가능한 CFG 경로·loop·분기 |
| 메시지 전달 | 동일 message의 성공한 전달과 수신 |
| 동기화 | 호출의 실제 효과·성공 조건·timeout·실패 경로 |
| startup | 해당 서비스의 초기화 완료와 최초 사용 경로 |
| table lifecycle | 선택 버전 API의 주소 획득·보유·해제·갱신 동작 |

### 10.2 원노트의 race 판정식을 어떻게 읽을 것인가

**[원노트의 연구 구상]** 원노트에는 필요한 선후 관계가 증명되지 않고 동시 실행 가능성이 있으면 potential race를 찾는다는 식이 있다.

```text
RequiredHB(E1, E2)
and not ProvenHB(E1, E2)
and PotentialConcurrent(E1, E2)
```

**[해석]** 이는 현재 단계에서 후보를 찾는 설명으로 읽을 수 있다. 학술적으로 검증된 cFS race 판정 정리가 아니다. 아래를 확인하지 않으면 실제 결함으로 단정할 수 없다. [R01, R02, R11]

- E1이 먼저 필요하다는 근거가 실제 source·API·요구·기존 결함 설명에 있는가.
- HB가 없다는 결과가 정보 부족이나 지원하지 않는 코드 때문인가.
- 반대 순서가 queue·guard·scheduler 조건에서 실제로 가능한가.
- 그 순서 차이가 어떤 잘못된 state·동작으로 이어지는가.
- 정상적인 비동기 처리 또는 복구 경로는 없는가.

**[편집 보정]** 여기에 별도의 보편적인 race 정의나 새로운 판정식을 덧붙여 연구 전체를 재정의하지 않는다. 실제 cFS 사례와 선택한 선행연구의 정의를 비교한 뒤 필요한 형식화를 진행해야 한다.

### 10.3 State origin 분석의 확인 항목

**[원노트의 연구 구상]** 원문은 수신한 메시지의 field가 어떤 앱 상태로 복사되고 어떤 계산에 사용되는지 추적하려 한다. 분기 합류 때 가능한 origin을 모으고, 초기화 여부 등을 확인하는 초안이 있다.

**[해석]** 구현에서는 아래를 확인해야 한다.

| 항목 | 놓치면 생기는 문제 |
| --- | --- |
| field 수준의 갱신 | 일부 field 갱신을 전체 state 갱신으로 오인 |
| `memcpy`·assignment·변환 | 값은 전달되었는데 출처가 끊기거나 잘못 연결 |
| pointer alias | 다른 경로의 쓰기가 누락 |
| 분기·validation | 실제로 사용할 수 없는 입력을 정상 state로 간주 |
| 공동 갱신 관계 | source에 없는 snapshot 조합 생성 |
| reset·재시작 | 이전 초기화·입력 정보를 새 실행에 재사용 |
| 수신 buffer의 보관 | 복사된 값과 기간이 지난 buffer 주소를 혼동 |
| 다른 task의 쓰기 | 앱 내부 state라는 이유로 interference를 무시 |

### 10.4 추상 상태의 의미를 먼저 정해야 함

**[해석]** 원노트의 `UNINITIALIZED`, `CURRENT`, `POSSIBLY_STALE`, `UNKNOWN`은 설명용 초안이다. 수신 전 상태, validation 실패, 마지막 도착값, 실제로 오래된 측정값이 한 분류로 섞일 수 있다. 각 항목이 source의 어떤 상태를 대표하는지 먼저 정의해야 한다. [R02; S14]

**[해석]** solver가 아직 계산하지 않은 상태와 프로그램의 초기화되지 않은 값도 다르다. 정보를 합치는 과정이 가능한 실행을 누락하지 않는지, 반대로 정보 손실 때문에 가짜 후보를 만드는지 확인해야 한다. 이를 확인하기 전에는 분석의 건전성을 주장할 수 없다.

**[미확인]** 특정 lattice, widening, relational domain, alias 모델을 이 연구의 확정 방법으로 선정하지 않았다. 필요한 정밀도와 구현 비용은 실제 source·사례를 통해 판단해야 한다.

### 10.5 분석 결과에 필요한 설명

**[원노트의 연구 구상]** 경고 위치만 제시하는 것보다 관련 앱·메시지·상태·사용 위치를 함께 보여 주려 한다.

**[해석]** 검토자가 확인할 수 있는 설명에는 최소한 다음이 필요하다.

| 항목 | 보여 줄 내용 |
| --- | --- |
| source 위치 | 발행·구독·수신·갱신·사용·동기화의 관련 위치 |
| 데이터 연결 | 어느 메시지가 어느 state field의 근거인지 |
| 순서 설명 | 왜 해당 실행 순서가 가능한지 |
| 문제의 근거 | API 설명·앱 요구·역사적 결함 등 필요한 선행 처리의 근거 |
| 코드 경로 | 성공·오류 반환, branch·guard가 일관되는지 |
| 남은 불확실성 | unknown call, alias, platform 의미, 분석 경계 |

**[해석]** 후보, 모델에서 가능한 실행, 실제 재현 결과는 설명에서 구별한다. 이를 새로운 결함 분류 체계로 만들 필요는 없지만, 증거의 강도가 다른 결과를 모두 ‘race 검출’로 집계해서는 안 된다.

## 11. 구현·실험을 구체화하는 순서

### 11.1 원노트의 단계 계획

**[원노트의 연구 구상]** API lifting, message graph, state provenance, HB 분석을 먼저 연결하고, 공개 앱·결함 예제를 통해 검토한 뒤 startup·temporal state·table로 확장한다.

**[해석]** 순서는 유지하되, 다음 단계의 주장을 하기 위해 어떤 자료가 필요한지 명시한다.

| 단계 | 수행할 작업 | 확인할 산출물 |
| --- | --- | --- |
| API 확인 | 실제 build와 source에서 SB 호출·반환·resource를 추출 | source 위치와 대응하는 API 의미표 |
| 메시지 연결 | MID·pipe·구독·수신·dispatch를 연결 | 수작업 검토 가능한 publisher–subscriber·handler 관계 |
| State origin | payload→field→계산 경로를 추적 | 복사·분기·reset·unknown 영향의 설명 |
| 순서 분석 | 앱 내부·전달·동기화·시작 관계를 연결 | 가능한 경로와 누락된 정보 |
| 예제 검증 | 원노트의 first-message·초기 상태·snapshot 예제를 구현·검토 | 실제 source와 일치하는 경고·정상 사례 |
| 역사적 검토 | 원버전과 수정 전후 자료 확보 | 원문 이슈와 재현 결과의 대응 |
| 범위 확장 | startup timeout·재시작·table 사용을 추가 | API 보호·오류 처리를 반영한 결과 |

**[미확인]** 위 표는 계획이다. 현재 구현 완료, benchmark 실행, source-level witness 생성 결과는 없다.

### 11.2 최소 prototype의 의미

**[원노트의 연구 구상]** 최초 prototype은 SB API를 중심으로 시작한다. 원문이 제시한 CreatePipe, Subscribe, ReceiveBuffer, TransmitMsg가 출발점이다.

**[해석]** 다만 네 함수 이름만 인식해서는 메시지 처리 관계를 복원할 수 없다. MID 추출, dispatch, status 확인, payload 복사, 실제 state 사용 경로를 함께 보아야 한다. 필요한 의미를 누락한 채 API 인식률만으로 완료를 판단하면 안 된다. [S09, S21]

### 11.3 공개 앱 후보

| 후보 | 확인한 자료 **[확인된 사실]** | 원노트에 활용 가능한 지점 **[해석]** | 추가 확인 |
| --- | --- | --- | --- |
| `sample_app` [S21] | 실제 C source의 app main/init 및 SB·dispatch 흐름 | 첫 API·handler 추출 검증 | 선택 build·실행 결과 |
| `HK` [S22] | 여러 앱의 telemetry를 조합하는 README 설명 | 메시지 유래 데이터의 조합 구조 검토 | 실제 source·table·데이터 시점 요구 |
| `LC` [S23] | telemetry limit 감시와 event·RTS 연계 설명 | 입력 state와 판단 결과의 연결 검토 | 상세 watchpoint·decision source·요구 |
| cFE #73 [S01] | 역사적 startup 문제와 수정 기록 | 원버전 재현과 정적 분석의 대응 검토 | 원버전·환경·재현 완료 |

**[해석]** 후보로 적었다는 이유로 실제 race가 있다고 주장하지 않는다. public app이 실제 임무에서 그대로 운용된 코드라고도 가정하지 않는다. 공개 source, 예제, 역사적 재현, 실제 비행 보고를 구별한다.

### 11.4 합성·변형 예제의 사용 범위

**[원노트의 연구 구상]** 원문에는 최소 예제와 공개 앱 기반 fault injection이 포함된다. 이를 API·분석 의미를 확인하는 데 사용할 수 있다.

**[해석]** 어떤 오류를 넣었다고 설명하는 것만으로 정답이 되지는 않는다. 해당 변경이 실제로 잘못된 순서를 허용하는지, 실제 결과가 달라지는지 확인해야 한다. 최소 예제에서의 성공을 비행 결함 검출 성능으로 일반화하지 않는다.

| 원노트의 예제 | 잘못된 경우와 함께 확인할 정상·경계 경우 |
| --- | --- |
| 구독 전 필수 첫 메시지 발행 | 첫 메시지 유실이 허용되는 앱, 후속 수신까지 사용을 막는 경로 |
| 입력 수신 전 state 사용 | 기본값 사용 허용, 실제 validation·mode guard |
| 다른 시점의 state 조합 | latest-value 허용, 동일 packet의 공동 갱신, buffering·matching |
| startup 준비 전 API 사용 | wait 성공, timeout 후 종료·재확인, restart |
| table 해제 누락 | 정상 주소 보유, blocking 전 해제, `NEVER_LOADED` 뒤 해제 |

## 12. 평가와 비교 대상

### 12.1 무엇을 검출했다고 셀 것인가

**[원노트의 연구 구상]** precision·recall, 분석 시간·메모리, API·분석 coverage를 평가하려 한다.

**[해석]** 이를 계산하기 전에 평가 단위를 정해야 한다. 경고 수, 오류 원인 수, 앱 수, 실행 trace 수는 서로 다르다. 같은 문제에서 여러 경고가 나왔다고 여러 개의 결함을 검출한 것으로 세지 않는다.

**[해석]** 각 평가 사례에는 source revision, 정상 동작의 근거, 수정 또는 주입한 내용, 가능한 잘못된 실행, 검토·재현 여부가 필요하다. 앱의 요구가 불명확하면 true/false label을 억지로 부여하지 않는다.

### 12.2 기본 지표의 해석

**[원노트의 연구 구상]** 확정 label이 있는 사례에서 precision과 recall을 계산할 수 있다.

```text
precision = TP / (TP + FP)
recall    = TP / (TP + FN)
```

**[해석]** 분모가 0인 경우를 처리하는 규칙과 중복 경고 규칙을 명시해야 한다. 분석 실패·timeout·unsupported·정답 불명 사례는 정상 사례로 계산하지 않는다. 제외한 사례와 그 이유를 같이 보고해야 한다.

| 함께 보고할 내용 | 이유 |
| --- | --- |
| 전체 입력 중 분석한 부분 | 쉬운 사례만 남겨 정확도가 높아 보이는지 판단 |
| 후보와 재현 결과의 구분 | 추상 경고가 실제 오류와 얼마나 연결되는지 확인 |
| 오경고 원인 | alias·분기·시점·scheduler·queue 중 어떤 정보가 부족한지 확인 |
| 놓친 문제의 원인 | 누락 의미·잘못된 모델·지원 범위를 구분 |
| source·모델링의 수작업 | 도구 적용에 필요한 부담을 숨기지 않음 |
| 실행 시간·메모리·입력 크기 | 정확도와 비용을 함께 판단 |

**[해석]** 같은 예제의 작은 변형들을 독립적인 실제 결함처럼 세면 안 된다. 합성·공개 앱 변형·역사적 결함·정상 앱 결과를 나누어 해석해야 한다. 유한한 시험에서 미검출이 없었다는 사실만으로 정적 분석의 건전성이 증명되지는 않는다. [R02]

### 12.3 비교 기준선

| 기준선 | 비교 목적 **[해석]** | 주의점 |
| --- | --- | --- |
| 직접 호출 그래프 | 원노트가 복원하는 메시지 관계의 효과 확인 | 이것만 비교하면 기준선이 지나치게 약함 |
| MID·pipe를 아는 그래프 | topology에 state·guard·순서 분석을 더한 효과 확인 | 동일한 API 추출 결과에서 비교할 필요 |
| AST/CFG 기반 분석 | MLIR 표현의 필요성과 구현상 이점 검토 | 같은 의미·알고리즘이면 정확도 차이가 없는 것이 자연스러울 수 있음 |
| cFS model-based testing [R03] | 프레임워크 동시성 검사와의 직접 비교 | 원문·artifact·검사 속성을 확인한 뒤 구체화 |
| 수작업 분석 모델 | 소스에서 복원한 관계가 실제 의도와 맞는지 검토 | 수작업 비용과 사용한 환경 가정을 기록 |
| runtime monitor [R09, S24] | 실제 관측된 동작과 정적 경고의 대응 검토 | 관측되지 않은 모든 경로를 확인했다고 볼 수 없음 |
| ThreadSanitizer [S20] | 공유 메모리 문제와 겹치는 범위의 비교 | 메시지 순서 문제 전반의 기준선으로 쓰지 않음 |

**[미확인]** 원문·artifact를 확보하지 못한 비교는 실행 계획일 뿐이다. 재현하지 않은 도구의 성능이나 검출 가능성을 단정하지 않는다.

### 12.4 분석 정보의 효과를 분리

**[원노트의 연구 구상]** MLIR 사용 정당성을 확인하기 위해 분석에 필요한 정보가 실제로 기여하는지 평가하려 한다.

**[해석]** 다음 정보를 포함하거나 생략했을 때 결과가 어떻게 달라지는지 검토하면 기여를 구체화할 수 있다. 실험 결과는 아직 없다.

- 성공·실패 반환과 timeout.
- handler의 guard와 mode 분기.
- field 수준의 state origin.
- 공동 갱신 및 분기 간 상관관계.
- 동일 MID 안의 구체적인 메시지 대응.
- queue 깊이·MsgLim 및 delivery 실패.
- startup·restart 의미.
- table·수신 buffer의 사용기간.

**[해석]** 이 정보가 만드는 효과는 우선 도메인 분석의 효과다. MLIR라는 구현 기반 자체의 효과와 구별해야 한다. MLIR의 이점은 표현·검증·재사용·확장·진단 구현에서 무엇이 달라졌는지로 설명해야 한다.

### 12.5 제한된 실행 탐색의 해석

**[해석]** queue·반복·메시지 수·시간에 경계를 두어 탐색한다면 그 경계를 결과에 기록해야 한다. 그 안에서 문제가 발견되지 않았다는 결과를 무한한 운용 시간에 대한 안전성으로 바꾸지 않는다. 반례가 나와도 source와 실제 환경에서 가능한지 확인해야 한다.

**[미확인]** 이 연구는 아직 전체 cFS 의미의 형식 증명, 무한 실행의 progress, WCET·deadline 분석을 제공하지 않는다. 원노트의 순서 분석으로 이 속성이 자동 해결된다고 주장하지 않는다. [R06, R07]

## 13. 남은 불확실성과 주장–근거 대응

### 13.1 현재 가장 중요한 미해결 사항

| 질문 | 현재 확보한 근거 | 다음에 필요한 자료 |
| --- | --- | --- |
| 공개 flight 사례와 연결되는가? | Swift 보고, cFE 개발 이슈 | 코드 수준 대응·원버전 재현 |
| 필요한 선행 처리를 어떻게 알 수 있는가? | API 설명·앱 source·기존 결함 기록 | 앱 요구·실제 consumer 동작의 확인 |
| 정상 비동기 처리를 구별하는가? | 문헌과 API에서 확인한 제약 | 정상·오류·모호 사례의 독립 검토 |
| 일반 그래프보다 무엇을 더 복원하는가? | state origin·제어 흐름을 연결하는 원노트 구상 | 실제 앱에서의 추가 정보·검출 결과 |
| 기존 cFS 연구와 다른가? | R03·R04·R09의 존재와 확인한 내용 | 남은 원문·artifact·후속 문헌 대조 |
| MLIR가 실질적으로 기여하는가? | 공식 IR·analysis infrastructure | 대안과의 표현·분석·비용 비교 |
| 분석의 가정이 현실과 맞는가? | 고정 cFE API·구현 | OSAL·PSP·scheduler·앱 구성 확인 |

### 13.2 주장–근거 대응

| 주장 | 지위 | 근거·상태 |
| --- | --- | --- |
| Swift/BAT에서 알려진 race가 보고되었다. | 확인된 사실 | S03–S06 |
| Swift의 내부 원인은 특정 snapshot·epoch 혼합이다. | 미확인 | 공개 보고만으로 확인 불가; 본문에서 단정하지 않음 |
| cFE core startup 순서 문제가 공개 기록에 있다. | 확인된 사실 | S01, S02 |
| 해당 문제가 현재 모든 cFE에 남아 있다. | 미확인 | 역사적 이슈와 현재 source를 구별 |
| 발행 성공은 수신 처리 완료와 같다. | 근거 없는 추론 | S09, S12의 동작과 구분해야 함 |
| startup wait 호출만 있으면 준비가 보장된다. | 근거 없는 추론 | S08, S11의 timeout·반환 경로 확인 필요 |
| table 주소를 보유한 정상 읽기와 update는 무조건 경쟁한다. | 근거 없는 추론 | S10의 보호·해제 규칙을 먼저 반영 |
| cFS 동시성·모델·monitor 선행연구가 존재한다. | 확인된 사실 | R03, R04, R09 |
| source에서 앱 간 관계와 state origin을 연결하는 분석을 만들고자 한다. | 원노트의 연구 구상 | 구현·평가 전 |
| MLIR로 실제 비행 결함을 검출했다. | 미확인 | 그런 실험 결과 없음 |
| MLIR 기반 접근이 기존 방법보다 우수하다. | 미확인 | 비교 결과 없음 |

### 13.3 원노트와 이번 편집의 대응

| 원노트 주제 | 수정본 위치 | 편집 내용 |
| --- | --- | --- |
| 연구 동기·race 구분 | §1–2 | 출처의 실제 용어와 연구 범위 표현을 구분 |
| Swift·cFE 사례 | §2 | 현상과 코드 수준 원인 추정을 분리; 역사적 날짜 보정 |
| cFS·Software Bus | §3–4 | 로컬 API·구현으로 근거 보강; 실패·buffer 수명 반영 |
| Startup dependency | §5 | timeout·준비 단계·재시작의 확인 필요성 추가 |
| State origin·snapshot·freshness | §6, §10 | 실제 입력 요구·분기·상관관계와 시간 의미를 확인하도록 보정 |
| Table lifecycle | §7 | 주소 보유 중 update 제한과 특수 반환 반영 |
| 관련 연구·차별점 | §8 | 직접 cFS 선행연구와 인접 이론을 구분 |
| MLIR·frontend·dialect | §9 | 공식 기능과 원노트의 미구현 설계 구분 |
| HB·분석 초안 | §10 | 증명 실패와 실제 가능한 race를 구별 |
| Prototype·평가·baseline | §11–12 | 정상 사례·실제 재현·비교 범위·coverage의 필요성 보강 |
| 향후 연구 판단 | §13 | 확보한 근거와 미해결 사항을 연결 |

### 13.4 이어서 할 작업

**[원노트의 연구 구상에 대한 구체화]** 다음 작업은 원안의 방향을 실제 연구로 옮기기 위한 것이다. 새 이론이나 별도 검사 체계를 전제로 삼지 않는다.

1. R03의 전체 원문, R04의 남은 내용·artifact를 확인하여 관련 연구 비교를 완성한다.
2. cFE·OSAL·PSP·앱 build를 고정하고, 실제 호출의 의미와 오류 경로를 기록한다.
3. `sample_app`에서 MID·pipe·dispatch·state 사용을 추출해 source와 대조한다.
4. 원노트의 첫 메시지·초기 상태·snapshot 예제를 정상 경로와 함께 구현·검토한다.
5. API·state 관계를 표현하는 데 필요한 MLIR 기능과 대안의 차이를 확인한다.
6. cFE 역사적 문제는 원버전·환경을 확보한 뒤 실제 재현 여부를 판단한다.
7. 실제 결과에 따라 startup·table·시간 분석의 지원 범위와 연구 기여를 결정한다.

**[현재 판단]** 원노트의 핵심은 flight software에서 나타나는 앱·서비스 사이의 순서 문제를 소스에서 찾아내려는 구상이다. 공개 사건과 API는 이 방향을 조사할 구체적인 출발점을 제공한다. 다만 검출 가능성·정확도·새로움·MLIR의 기여는 앞으로 얻을 코드와 실험 근거로 판단해야 한다.

## 14. 참고문헌·1차 출처·확인 상태

**[사실·서지 기록]** 아래 ‘확인 범위’는 이번 편집에서 실제로 사용한 근거의 범위다. 출판사 DOI는 논문의 식별자이고, 별도 저자 PDF는 그 버전의 본문 확인 경로다. 초록·서지 확인을 원문 전체 검토와 같게 표시하지 않는다. 웹 검색에서 다른 자료가 언급되었다는 이유만으로 그 자료를 읽은 것으로 처리하지 않는다.

### 14.1 학술 자료

#### R01

Lamport, L. (1978). **Time, Clocks, and the Ordering of Events in a Distributed System.** *Communications of the ACM*, 21(7), 558–565. DOI: [10.1145/359545.359563](https://doi.org/10.1145/359545.359563).

- 1차 확인 경로: [저자 자료·서지](https://www.microsoft.com/en-us/research/publication/time-clocks-ordering-events-distributed-system/), [원문 PDF](https://www.microsoft.com/en-us/research/wp-content/uploads/2016/12/Time-Clocks-and-the-Ordering-of-Events-in-a-Distributed-System.pdf).
- 확인 범위: 원문의 *The Partial Ordering*, *Logical Clocks*, pp. 558–560 및 저자의 설명.
- 이 노트의 사용: 실제 message identity로 인과 순서를 연결하고 논리 순서와 물리 시간을 구분하는 근거. cFS queue나 특정 임무의 freshness 분석을 직접 다룬 논문은 아니다.

#### R02

Cousot, P., & Cousot, R. (1977). **Abstract Interpretation: A Unified Lattice Model for Static Analysis of Programs by Construction or Approximation of Fixpoints.** *POPL 1977*, 238–252. DOI: [10.1145/512950.512973](https://doi.org/10.1145/512950.512973).

- 1차 확인 경로: [저자 서지·상세 요약](https://cs.nyu.edu/~pcousot/COUSOTpapers/POPL77.shtml), [저자 제공 스캔 PDF](https://cs.nyu.edu/~pcousot/publications.www/CousotCousot-POPL-77-ACM-p238--252-1977.pdf).
- 확인 범위: 저자 요약 및 서지 확인. 스캔 PDF를 열었으나 이 조사에서 원문 텍스트의 상세 검토는 제한됨.
- 이 노트의 사용: 추상화·lattice·단조 전이·fixpoint와 구체 의미 사이의 정합성이라는 방법론의 근거. cFS 분석에 적용할 구체적 도메인·전이 함수는 이 논문에서 제공하는 것이 아니며, 이 연구에서 아직 확정하지 않았다.
- 서지 주의: 저자 페이지의 상단 설명과 BibTeX에 학회 회차 표기가 다르게 나타나므로 여기서는 연도·학회명·쪽수·DOI로 식별한다.

#### R03

Ganesan, D., Lindvall, M., Hafsteinsson, S., Cleaveland, R., Strege, S. L., & Moleski, W. (2016). **Experience Report: Model-Based Test Automation of a Concurrent Flight Software Bus.** *IEEE 27th International Symposium on Software Reliability Engineering (ISSRE)*, 445–454. DOI: [10.1109/ISSRE.2016.47](https://doi.org/10.1109/ISSRE.2016.47).

- 1차 확인 경로: [Fraunhofer 기관 서지·초록](https://publica.fraunhofer.de/entities/publication/88eabd0e-7f42-4a38-888b-1b0b4b1fd750), [ISSRE 공식 논문 초록 목록](https://2016.issre.net/research_papers.html), [연구기관 publication 목록](https://www.cma.fraunhofer.org/en/technical-publications0/technical-publications.html), [출판사 record](https://ieeexplore.ieee.org/document/7774542/).
- 확인 범위: 기관 서지·공식 초록 확인. 출판사 본문 접근 제한으로 논문 전체는 확보하지 못함.
- 이 노트의 사용: cFS Software Bus의 동시성에 대한 model-based testing 선행연구가 있다는 직접 근거. Spec Explorer·requirements model·inter-task testing은 초록에서 확인한다.
- 제한: 모델 세부사항, 정량 효과, 앱 상태 출처·시간 관계 분석 기능의 부재는 단정하지 않는다. 공식 행사 목록의 저자 표기보다 전체 기관 서지를 따른다.

#### R04

Valente, H., de Miguel, M., Pérez-Muñoz, Á., Alonso, A., Zamorano, J., & de la Puente, J. (2025). **Model-based Toolchain for Core Flight System (cFS) Embedded Systems.** *ACM Transactions on Embedded Computing Systems*, 24(3), Article 51, 1–28. DOI: [10.1145/3706587](https://doi.org/10.1145/3706587).

- 1차 확인 경로: [출판사 논문](https://doi.org/10.1145/3706587), [저자기관 UPM 서지](https://portalcientifico.upm.es/en/en/ipublic/item/10381027).
- 확인 범위: 출판사 검색 색인에서 제공된 초록·본문 일부, 기관 서지. 초록, 도구체인 설명 및 사례·비교 일부를 확인했으며 전체 PDF·artifact의 완전 검토는 미완료.
- 이 노트의 사용: cFS–TASTE 통합, model 기반 code generation과 UPMSat-2 사례의 직접 근거. architecture 모델과 implementation consistency를 다루는 선행연구로 비교한다.
- 제한: 일부 본문에 접근했다는 사실만으로 앱 간 race 정적 분석 기능의 부재를 주장하지 않는다. 2023년의 유사 제목 선행 자료와 2025년 저널 논문을 같은 출판물로 혼동하지 않는다.

#### R05

Bagherzadeh, M., & Rajan, H. (2017). **Order Types: Static Reasoning about Message Races in Asynchronous Message Passing Concurrency.** *AGERE 2017*. DOI: [10.1145/3141834.3141837](https://doi.org/10.1145/3141834.3141837).

- 1차 확인 경로: [저자 제공 원문 PDF](https://mbagherz.bitbucket.io/lab-correct-software/papers/order-types.pdf).
- 확인 범위: 원문의 order types, flow/HB 추론 및 §3 말미의 후속 형식화·증명 과제.
- 이 노트의 사용: 순서 의미를 정적 타입·flow에 결합하는 선행 접근. 소스에서 semantic relation을 추출하는 연구의 인접 비교 대상.
- 제한: 해당 언어의 activities·futures 의미를 cFS에 전용하지 않는다. 종료성·건전성 증명을 논문이 완료했다고 표현하지 않는다.

#### R06

Tulsyan, R., Pai, R., & D’Souza, D. (2020). **Static Race Detection for RTOS Applications.** *FSTTCS 2020*, LIPIcs 182, 57:1–57:20. DOI: [10.4230/LIPIcs.FSTTCS.2020.57](https://doi.org/10.4230/LIPIcs.FSTTCS.2020.57).

- 1차 확인 경로: [출판사 record](https://drops.dagstuhl.de/entities/document/10.4230/LIPIcs.FSTTCS.2020.57), [출판사 원문 PDF](https://drops.dagstuhl.de/storage/00lipics/lipics-vol182-fsttcs2020/LIPIcs.FSTTCS.2020.57/LIPIcs.FSTTCS.2020.57.pdf), [저자 preprint](https://arxiv.org/pdf/2010.02642).
- 확인 범위: 출판사 서지·본문의 RTOS model, occurs-in-between·priority·동기화 추론(§3–5) 및 preprint의 관련 설명.
- 이 노트의 사용: 플랫폼 동기화·priority가 feasible interference에 영향을 준다는 근거와 shared-memory race 연구와의 범위 비교.
- 제한: FreeRTOS를 포함한 모델의 결과를 다른 cFS 배치에 그대로 적용하지 않는다. 이 노트는 논문의 정량 결과를 자체 검출 예상치로 쓰지 않는다.

#### R07

Günzel, M., Chen, K.-H., Ueter, N., von der Brüggen, G., Dürr, M., & Chen, J.-J. (2021). **Timing Analysis of Asynchronized Distributed Cause-Effect Chains.** *IEEE Real-Time and Embedded Technology and Applications Symposium (RTAS)*. DOI: [10.1109/RTAS52030.2021.00012](https://doi.org/10.1109/RTAS52030.2021.00012).

- 1차 확인 경로: [저자기관 제공 preprint PDF](https://daes.cs.tu-dortmund.de/storages/daes-cs/r/publications/guenzel21rtas_e2e.pdf), [기관 서지·DOI record](https://ris.uni-paderborn.de/record/66226).
- 확인 범위: preprint의 system model·communication·reaction time·data age 정의(§II–IV) 및 timing 분석의 가정(§V).
- 이 노트의 사용: 시간 끝점과 동기화·task·execution 가정의 중요성. 부분순서만으로 physical freshness bound를 증명하지 않는 근거.
- 제한: periodic activation·fixed execution time 등을 사용하는 분석 조건을 일반 cFS 이벤트 처리로 이전하지 않는다. 이 노트의 소비 시 age는 actuation까지의 data age와 구별한다. 저자 PDF와 출판본의 완전한 동일성은 대조하지 않았다.

#### R08

Henzinger, T. A., Horowitz, B., & Kirsch, C. M. (2003). **Giotto: A Time-Triggered Language for Embedded Programming.** *Proceedings of the IEEE*, 91(1), 84–99. DOI: [10.1109/JPROC.2002.805825](https://doi.org/10.1109/JPROC.2002.805825).

- 1차 확인 경로: [저자기관 ISTA 서지·초록](https://research-explorer.ista.ac.at/record/4469), [Berkeley Giotto 자료 목록](https://ptolemy.berkeley.edu/projects/embedded/giotto/papers.html).
- 확인 범위: 2003년 저널 서지·초록과 저자 프로젝트 자료 목록. 해당 저널 원문 전체는 확보하지 못함.
- 이 노트의 사용: time-triggered embedded programming과 명시 시간 specification이라는 접근의 존재.
- 제한: Berkeley 목록의 2001년 workshop판을 2003년 저널 본문 검토로 대체하지 않는다. Giotto의 세부 정리 또는 logical execution time 의미가 cFS에 있다는 주장은 하지 않는다.

#### R09

Perez, I., Mavridou, A., Pressburger, T., Goodloe, A., & Giannakopoulou, D. (2022). **Automated Translation of Natural Language Requirements to Runtime Monitors.** *TACAS 2022*, LNCS 13243, 387–395. DOI: [10.1007/978-3-030-99524-9_21](https://doi.org/10.1007/978-3-030-99524-9_21).

- 1차 확인 경로: [출판사 서지·본문](https://link.springer.com/chapter/10.1007/978-3-030-99524-9_21), [원문 PDF](https://link.springer.com/content/pdf/10.1007/978-3-030-99524-9_21.pdf).
- 확인 범위: FRET·Ogma·Copilot과 monitor generation 및 cFS integration을 설명하는 §3–5.
- 이 노트의 사용: 요구를 명시하고 실행 시 검증하는 직접 선행연구; 정적 후보의 replay·monitor 기반 관측과 연결할 근거.
- 제한: monitor가 관측하지 않은 모든 경로를 증명하거나, 이 논문이 본 연구의 정적 race analyzer를 제공한다고 표현하지 않는다.

#### R10

Lattner, C., Amini, M., Bondhugula, U., Cohen, A., Davis, A., Pienaar, J., Riddle, R., Shpeisman, T., Vasilache, N., & Zinenko, O. (2021). **MLIR: Scaling Compiler Infrastructure for Domain Specific Computation.** *IEEE/ACM International Symposium on Code Generation and Optimization (CGO)*, 2–14. DOI: [10.1109/CGO51591.2021.9370308](https://doi.org/10.1109/CGO51591.2021.9370308).

- 1차 확인 경로: [저자기관 논문 페이지](https://research.google/pubs/mlir-scaling-compiler-infrastructure-for-domain-specific-computation/), [저자기관 제공 원문 PDF](https://storage.googleapis.com/gweb-research2023-media/pubtools/5945.pdf), [MLIR 공식 citation 안내](https://mlir.llvm.org/getting_started/Faq/).
- 확인 범위: design principles, SSA/regions, operations/dialects/interfaces 등 IR 설계 설명 및 공식 citation.
- 이 노트의 사용: semantic IR·source traceability·interface 재사용을 탐구할 기반.
- 제한: 2020년 arXiv의 다른 제목을 2021년 CGO 논문 제목과 혼동하지 않는다. 논문의 일반 compiler infrastructure 기여가 cFS 분석 성능의 증거는 아니다.

#### R11 — 보조 preprint

González-Abril, J. J., & Vidal, G. **A Lightweight Approach to Computing Message Races with an Application to Causal-Consistent Reversible Debugging.** arXiv:2112.12869. 최초 제출 2021년; 이번에 읽은 PDF는 v3, 2022-02-22. [arXiv record](https://arxiv.org/abs/2112.12869), [본문 PDF](https://arxiv.org/pdf/2112.12869), [arXiv DOI](https://doi.org/10.48550/arXiv.2112.12869).

- 확인 범위: trace의 send/deliver/receive 구분과 §2–3의 potential message race 및 matching 제한.
- 이 노트의 사용: 후보 race와 실제 receive 조건을 만족하는 대안 실행을 구분하는 논증의 보조 자료.
- 제한: 이 노트는 해당 버전의 peer-reviewed 최종 출판 정보·실험 효과를 확인하지 않았다. R01–R10과 동일한 출판 상태의 증거로 취급하지 않는다.

### 14.2 사건·검증 활동의 1차 기록

#### S01

NASA cFE, [GitHub issue #73](https://github.com/nasa/cFE/issues/73).

- 확인 범위: 본문 및 이식된 Trac 댓글; Microblaze 시작 순서·초기화 문제, 수정 논의와 역사적 시험 기록.
- 직접 anchor: [원 Trac 생성일 정보](https://github.com/nasa/cFE/issues/73#issuecomment-536673864), [초기화 대안 논의](https://github.com/nasa/cFE/issues/73#issuecomment-536673867), [EVS/AppID 관련 수정 논의](https://github.com/nasa/cFE/issues/73#issuecomment-536673881), [2015년 시험 기록](https://github.com/nasa/cFE/issues/73#issuecomment-536673921).
- 제한: 역사적 개발 이슈이며 actual flight occurrence를 입증하지 않음.

#### S02

NASA cFE, [GitHub issue #71](https://github.com/nasa/cFE/issues/71).

- 확인 범위: startup synchronization의 semaphore·flag·counter 관련 본문 및 공개 상태.
- 제한: S01과 관련된 개발 기록으로 취급. 별도의 독립 비행 사고 또는 현재 미수정 결함으로 집계하지 않음.

#### S03

Swift/BAT 임무팀, [GCN Circular 22706](https://gcn.nasa.gov/circulars/22706), 2018-05-11.

- 확인 범위: 4U 1416-62 관련 보고 및 잘못 적용된 spacecraft attitude와 알려진 race의 설명.
- 제한: source·thread·cFS·실행 trace를 제공하는 문서가 아님.

#### S04

Swift/BAT 임무팀, [GCN Circular 32397](https://gcn.nasa.gov/circulars/32397), 2022-07-14.

- 확인 범위: Cen X-3 관련 보고와 알려진 onboard race condition의 언급.
- 제한: 원인의 코드 수준 동일성을 확인할 수 없음.

#### S05

Swift/BAT 임무팀, [GCN Circular 34691](https://gcn.nasa.gov/circulars/34691), 2023-09-14.

- 확인 범위: SWIFT J1727.8-1613 관련 보고, 알려진 race와 ‘7 minute problem’ 표현.
- 제한: 내부 타이밍 요구·동기화 설계는 미확인.

#### S06

Swift/BAT 임무팀, [GCN Circular 43470](https://gcn.nasa.gov/circulars/43470), 2026-01-20.

- 확인 범위: 1A 1118-61 관련 잘못된 식별·알림과 알려진 race의 설명.
- 제한: 사건 반복을 prevalence·독립 fault family의 통계로 변환하지 않음.

#### S07

Bradbury, J. W. (2020). **Open Source Core Flight System (cFS) Flight Software (FSW) Verification & Validation (V&V) Final Summary: IV&V Analysis Technical Report.** NASA/CR–20205010026. [NASA NTRS record](https://ntrs.nasa.gov/citations/20205010026), [공식 PDF](https://ntrs.nasa.gov/api/citations/20205010026/downloads/CR%2020205010026%20Core%20Flight%20System%20%28cFS%29%20Flight%20Software%20%28FSW%29%20Final%20Report-1.pdf).

- 확인 범위: §2.1 정적 분석(보고서 인쇄 p. 19), §2.2 설계 분석(인쇄 p. 23) 및 보고 대상·시기. PDF viewer의 page index와 인쇄 쪽수를 혼동하지 않음.
- 제한: cFE 6.5 등을 다룬 역사적 V&V 보고이며 본 연구 모델의 정량 baseline이나 전체 race 발생률 자료가 아님.

### 14.3 cFE API와 구현의 고정 source

**[사실·코드 근거]** S08–S12는 모두 commit `546a002515be5a1e3b66f9ae2c14f948d9cec76f`의 원문을 확인했다. GitHub link에는 branch가 아닌 commit을 사용한다. blob SHA는 읽은 파일의 bytes를 식별하며 repository commit과 다른 값이다.

#### S08

NASA cFE, [cfe_es.h](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_es.h).

- 확인 함수: `CFE_ES_WaitForSystemState`, `CFE_ES_WaitForStartupSync` 및 관련 lifecycle 설명.
- blob SHA: `5aecd0452c04f770d4c628f437b51fdd41968e9f`.
- 사용 범위: API의 상태 대기·timeout·반환 동작. 임무 데이터의 준비 여부는 앱의 구현과 요구사항에서 추가 확인해야 함.

#### S09

NASA cFE, [cfe_sb.h](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb.h).

- 확인 함수: `CFE_SB_CreatePipe`, `CFE_SB_Subscribe`/`SubscribeEx`, `CFE_SB_TransmitMsg`, `CFE_SB_ReceiveBuffer` 및 관련 pipe·옵션·return 설명.
- blob SHA: `cf1413c11976074f7d393586c3f52f9dc0f22ed1`.
- 사용 범위: routing, MsgLim, 발행·수신 status, 수신 buffer 수명, 전송 호출 중 task 실행 가능성.

#### S10

NASA cFE, [cfe_tbl.h](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_tbl.h).

- 확인 함수: `CFE_TBL_GetAddress`, `CFE_TBL_ReleaseAddress`, `CFE_TBL_Update`, 관련 share/manage/load 설명.
- blob SHA: `f3510424300c6ce3797f7b365b498ecacda90704`.
- 사용 범위: 보유 주소의 해제·blocking·update 프로토콜 및 `CFE_TBL_ERR_NEVER_LOADED` 특수 반환.

#### S11

NASA cFE, [cfe_es_api.c](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_api.c).

- 확인 함수: `CFE_ES_WaitForSystemState`, `CFE_ES_WaitForStartupSync`.
- blob SHA: `435a1b115ca22af66275c31929f0e0cf841f4bc2`.
- 사용 범위: 실제 timeout 경로와 wrapper가 반환 상태를 전달하지 않는 구현.

#### S12

NASA cFE, [cfe_sb_priv.c](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_priv.c).

- 확인 함수: `CFE_SB_TransmitTxn_Execute`, `CFE_SB_TransmitTxn_FindDestinations`, `CFE_SB_MessageTxn_ProcessPipes`, `CFE_SB_TransmitTxn_PipeHandler`와 해당 전송 경로.
- blob SHA: `1de11a199c3d2138f7a46e12c529f03268da69d1`.
- 사용 범위: 미구독 0-pipe 경로, 목적지별 queue 삽입과 삽입 오류 처리. 다른 플랫폼 queue 구현의 모든 성질을 이 source로 일반화하지 않음.

#### S25

NASA cFE, [기준 commit record](https://github.com/nasa/cFE/commit/546a002515be5a1e3b66f9ae2c14f948d9cec76f).

- 확인 범위: commit identity 및 2026-09-25T18:23:07Z의 시각 정보.
- 제한: 임무 배치·인증·운용 버전 정보가 아님. 향후 평가의 OSAL/PSP·앱 pinning은 별도 수행해야 함.

### 14.4 공식 compiler·analysis 문서

**[사실·문서 상태]** S13–S20은 조사일의 공식 웹 문서다. commit으로 고정한 code와 달리 live 문서이므로 구현 단계에서 LLVM/Clang revision 및 문서 snapshot을 추가로 기록해야 한다.

| ID | 1차 출처 | 확인 범위와 사용 |
| --- | --- | --- |
| S13 | [MLIR Language Reference](https://mlir.llvm.org/docs/LangRef/) | operation·type·attribute·region·block·SSA 및 verifier 표현의 기반 |
| S14 | [MLIR DataFlowAnalysis tutorial](https://mlir.llvm.org/docs/Tutorials/DataFlowAnalysis/) | lattice·전파·fixpoint 구현의 설명. cFS 전역 동시성 solver가 제공된다는 근거가 아님 |
| S15 | [MLIR Interfaces](https://mlir.llvm.org/docs/Interfaces/) | operation·control·effect 등 interface의 기반. 도메인 의미 보존은 별도 의무 |
| S16 | [MLIR LLVM IR Target](https://mlir.llvm.org/docs/TargetLLVMIR/) | *Translation from LLVM IR*의 experimental·subset 제한 |
| S17 | [Clang CIR](https://clang.llvm.org/docs/CIR/index.html) | upstream 진행·build 지원 경고; 완성된 기본 frontend로 가정하지 않음 |
| S18 | [Clang LibTooling](https://clang.llvm.org/docs/LibTooling.html) | source tool 구축 기반; 선택 경로의 적합성은 제안 |
| S19 | [JSON Compilation Database](https://clang.llvm.org/docs/JSONCompilationDatabase.html) | 실제 compilation 입력 재사용을 위한 공식 format |
| S20 | [Clang ThreadSanitizer](https://clang.llvm.org/docs/ThreadSanitizer.html) | memory data race 탐지의 도구 범위; 시간 관계 문제까지 다루는 기준선으로 과장하지 않음 |

### 14.5 공개 앱·tool 후보의 확인 기록

**[사실·버전 기록]** S21–S24는 표시된 branch의 파일을 읽고 blob SHA를 기록했다. 향후 실행 평가 전에는 repository commit과 submodule을 추가 고정해야 한다. README를 읽은 후보에 대해 앱 전체 구현을 확인했다고 서술하지 않는다.

| ID | repository·읽은 파일 | 조사 시 branch·blob SHA | 확인 범위 |
| --- | --- | --- | --- |
| S21 | [NASA sample_app, sample_app.c](https://github.com/nasa/sample_app/blob/dev/fsw/src/sample_app.c) | `dev`; `b07a4317d340bba95b6a0022b3c1408840c545dc` | 실제 C 파일; app main/init, SB 수신·MID/handler 흐름 |
| S22 | [NASA HK, README](https://github.com/nasa/HK/blob/dev/README.md) | `dev`; `c0c96ddc4c7942df5b3d1e3897e969fbd4ad47a1` | 여러 앱 telemetry 조합 기능의 소개; 데이터 시점에 관한 상세 요구사항 미확인 |
| S23 | [NASA LC, README](https://github.com/nasa/LC/blob/dev/README.md) | `dev`; `4ea5537d4c0965030a14d6aff637820a4331cc89` | telemetry limit·event·RTS 기능의 소개; 상세 consumer source 미검토 |
| S24 | [NASA Ogma, README](https://github.com/nasa/ogma/blob/develop/README.md) | `develop`; `2c271ad6cc6cabb9fcd7a0d3f5e7c7df84edb114` | cFS message bus를 받는 runtime monitor 및 handler 생성 기능 소개 |

### 14.6 추가 확인 대상으로 남긴 문헌

**[미확인·선택 이유]** 아래는 조사 과정에서 발견했으나 이번 연구의 핵심 기술 결론을 지탱하는 자료로 사용하지 않은 후보다. 서지의 존재와 특정 정리·결과의 확인은 별개다. 불완전한 접근을 숨기기 위해 참고문헌의 개수만 늘리지 않는다.

| 후보 | 1차 확인 경로 | 현재 제한·다음 확인 |
| --- | --- | --- |
| Artho, Havelund & Biere, *High-level Data Races*, STVR 13(4), 207–227, 2003 | [저자 publication 목록](https://havelund.com/publications/) | 전체 논문 접근·주장 검토 미완료. multi-variable consistency와 원노트의 상태 일관성 분석 구상의 관계를 원문으로 확인 |
| Havelund, Lowry & Penix, *Formal Analysis of a Space-Craft Controller Using SPIN*, TSE 27(8), 749–765, 2001 | [저자 publication 목록](https://havelund.com/publications/), [저자기관 record](https://research.google/pubs/formal-analysis-of-a-space-craft-controller-using-spin/) | 상세 모델·속성·사건 맥락을 원문에서 확인. 역사적 검증 연구를 실제 비행 사고로 바꾸어 서술하지 않음 |
| 2023년 *Model-based toolchain for cFS embedded systems for microsatellites* | [UPM repository record](https://oa.upm.es/74294/) | 원문 접근 미완료. R04와의 중복·확장 관계를 확인하고 독립 증거로 중복 집계하지 않음 |
| DDS message race case study 및 추가 actor/Erlang race 연구 | 출판사·저자 원문 확보 필요 | 2차 검색 요약만으로 cFS 차별화·기술 결론의 근거로 사용하지 않음 |

**[문헌 관리]** 다음 판에서는 각 논문의 버전, 확인한 section, 출판 상태, artifact URL, 요구·모델·속성·보장 범위, 이 노트의 각 주장을 뒷받침하는 위치를 함께 관리한다. 특히 R03·R04의 남은 검토와 high-level data race/atomicity 연구의 확인이 새로움 판단에 필요하다.


---

**문서 상태:** 공개 자료로 확인한 내용과 원노트의 연구 구상·편집자의 해석을 구분했다. 분석기 구현, 실제 결함 재현, 정량 성능 평가는 아직 완료하지 않았다.
