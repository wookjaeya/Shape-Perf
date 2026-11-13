# Kairos / cFS Process-Level Race 연구 검토 결과

- 작성일: 2026-10-12
- 검토 대상: `claude/busy-shannon-y903qc` 브랜치, 커밋 `b4ac55fbab244d24286154f317003aa3fa2518ae`
- 목적: 현재 연구 근거, 형식 모델, 비교 설계, 용어 선택을 검토하고 다음 구현·실험의 우선순위를 제안한다.
- 성격: 검토 의견과 수정 권고다. 사용자 결정이나 기존 로드맵을 대체하는 확정 계획이 아니다.
- 검증 범위: 첨부 문서, 해당 커밋의 설계·사례·결과 문서, 저장된 gdb 원시 trace 3개, 관련 연구의 1차 자료.
- 실행 범위: 기존 trace를 다시 집계했다. 이번 검토에서 cFS를 새로 빌드하거나 실행하지 않았으며, Kairos 구현이나 저장소 원문도 변경하지 않았다.

## 1. 종합 판단

**연구는 계속할 가치가 있다. 그러나 현재 자료만으로 ‘아키텍처가 확정되었고 학술적으로 탄탄하다’고 판단하기는 이르다.**

cFS 애플리케이션·서비스 태스크 사이의 자원 의존성을 구체적인 사례와 연결한 점, 관측과 보편적 주장을 구분한 점, 허용된 미준비 결과와 실제 위반을 분리하려는 점은 유효하다. 반면 형식 정의와 증명 사이에 남은 틈이 있고, 검증된 구현 성과에 비해 계획 범위가 크다.

현재의 우선순위는 다음과 같다.

1. 안전성과 진행성을 구분하고, 실행 의미 사이의 포함 관계를 수정한다.
2. R2 하나에서 추출·관측·판정·인증서 생성까지 연결한다.
3. C1에서 즉시 성공, 재시도 후 성공, 최종 실패를 구분한다.
4. 같은 판정 정보를 제공한 대조군과 비교한다.
5. 설계에 사용하지 않은 사례에서 추가 규칙과 수작업 비용까지 측정한다.

범위는 애플리케이션·서비스 태스크 사이의 순서·의존성 위반으로 유지한다. 현재 POSIX 구성에서 이들은 한 OS 프로세스 안의 스레드로 실행되므로, ‘process-level’이 논리적 애플리케이션 수준을 뜻한다는 설명은 필요하다. 이 구현 사실이 연구 대상을 메모리 data race로 바꾸는 것은 아니다. [S1, §1.1](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/Architecture/spec/SPEC_FINAL.md)

## 2. 실제 근거가 확보된 부분

### 2.1 다섯 사례의 상태

| 사례 | 현재 근거 | 한계 | 다음 검증 |
|---|---|---|---|
| R2 | gdb 원시 trace 3개에서 각각 발행 전 사용 24회와 대응하는 오류 결과를 재확인 | 기존 관측 기록이며 Kairos의 탐지 결과가 아니다. 반복 관측만으로 모든 실행의 고정 순서를 증명하지 못한다 | Kairos로 재관측하고, 고정 순서 인증서를 생성한다 |
| C1 | 기본 구성의 semaphore 이름이 비어 있어 해당 조회가 실행되지 않는다는 코드 대응 | 현재 기본 구성에서 사례를 재현한 것이 아니다 | 공개 근거에 기반한 provider와 이름 설정으로 경로를 활성화한다 |
| C2 | 원래 애플리케이션은 비공개이며, 현재 초기화 구조는 변경되어 있다 | 원래 사례를 그대로 재현할 수 없다 | 역할을 재구성한 실험임을 명시하고 원 사례와의 대응·차이를 기록한다 |
| C3 | 원 결함 코드는 6.4.x에 있으며 v7.0.1에는 그대로 존재하지 않는다 | 현재 버전에서의 재현으로 주장할 수 없다 | 과거 버전의 수정 전후를 비교한다 |
| C5 | 원 lock-scope 결함은 과거 버전에 있고 현재 코드는 변경되어 있다 | 현재 남은 후보 구간은 원 결함과 구분해야 한다 | 과거 사례와 현재 후보의 조건·원인을 각각 검증한다 |

근거: [현재 코드 대응표 S2], [사례 근거 기록 S3], [R2 gdb 결과 S4].

### 2.2 R2 원시 trace 재집계

세 trace에서 SB 호출자가 수행한 `SEND_WITH_APPID`를 추출하고, `SB_PUB` 이전의 사용을 분리했다. 각 사용 뒤 같은 LWP에서 다음 사용 전까지 나타난 오류 결과를 대응시켰다. AppId는 원시 기록에 문자열로 저장되어 있어 숫자값으로 변환해 확인했다.

| 기록 | SB의 전체 사용 횟수 | 발행 전 사용 | 발행 전 AppId = 0 | 대응한 ILLEGAL_APP_ID 결과 |
|---|---:|---:|---:|---:|
| r2_g1 | 223 | 24 | 24 | 24 |
| r2_g2 | 229 | 24 | 24 | 24 |
| r2_g3 | 229 | 24 | 24 | 24 |

발행 전 사용의 호출 함수별 집계도 세 기록에서 같았다.

| 호출 함수 | 실행당 횟수 |
|---|---:|
| CFE_SB_CreatePipe | 2 |
| CFE_SB_SubscribeFull | 4 |
| CFE_SB_MessageTxn_ReportSingleEvent | 18 |
| 합계 | 24 |

이 결과는 **저장된 관측 기록에서 잘못된 AppId 사용과 오류 결과가 대응한다**는 근거다. 교란 없는 실행의 발생 확률이나 보편적 고정 순서의 증명은 아니다. 원시 기록: [T1](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/env/logs/r2_g1_trace.jsonl), [T2](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/env/logs/r2_g2_trace.jsonl), [T3](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/env/logs/r2_g3_trace.jsonl).

**24회는 독립적인 결함 24개가 아니다.** 논문에서는 결함 원인 수, 정적 사용 인스턴스 수, 실행 중 발생 횟수, 반복 실행 수를 분리해야 한다.

### 2.3 결과 문서의 갱신 누락

`R2_cord_v701.md`의 §7은 버전 테이블을 복구한 r2_c4–r2_c6에서 각각 24회를 관측했다고 기록한다. 그러나 §8은 여전히 11회만 관측했으며 24회는 미실행이라고 적는다. [S5](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/results/R2_cord_v701.md)

이는 관측 결과 자체의 반박이 아니라 **결과 요약의 갱신 누락**이다. 다음처럼 정리해야 한다.

- r2_c1–r2_c3: 버전 테이블이 다른 초기 구성, 11회.
- r2_c4–r2_c6: 기준 버전 테이블을 복구한 구성, 24회.
- gdb 실행과 수정 바이너리 실행은 관측 방식과 바이너리가 다르므로 별도 실험군으로 유지한다.
- 최신 주장 범위와 제한사항을 §8에 반영한다.

## 3. 논문에서 중심에 둘 공헌

R2의 강점은 **동기화가 존재하면서도 의존성을 잘못된 방향으로 강제할 수 있다**는 점이다.

1. ES·EVS가 SB의 AppId를 사용하는 경로에 도달한다.
2. SB의 AppId 발행은 뒤의 초기화 단계에서 수행된다.
3. 초기화 제어가 이 순서를 강제한다.
4. 이 관계를 입증하면 스케줄 변경만으로 해결할 수 없는 오류임을 설명할 수 있다.

Kairos의 유력한 공헌 후보는 다음 연결이다.

**cFS 자원 의미와 구성의 추출 → 관측된 위반 → 고정 순서·스케줄 의존의 구분 → 가정과 근거를 공개한 인증서 또는 실행 증거**

단, 이것은 구현·평가로 입증해야 할 공헌 후보다. 현재의 설계 설명만으로 신규성이나 일반성이 확정되지는 않는다. R2 인증서에도 일반 K-ID 조건 외에 사례를 알고 작성한 R19 프레임워크 규칙과 신뢰 가정이 필요하다는 기존 문서의 설명을 유지해야 한다. [S1, §7.2 및 Appendix A.1](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/Architecture/spec/SPEC_FINAL.md)

‘명세를 사용한다’는 사실만으로는 충분한 차별점이 되지 않는다.

| 선행 연구 | 확인한 관련 기능 | Kairos에 필요한 차별화 |
|---|---|---|
| PRETEX, ASE 2008 | 멀티스레드 프로그램의 typestate 오류 검사와 다른 실행에서의 오류 예측 | cFS의 실제 자원·초기화 의미를 어떻게 얻고 적용하는지 |
| GPredict, ICSE 2015 | 일반적인 동시성 속성 명세와 인과 모델에 기반한 위반 예측 | 같은 속성 정보를 제공했을 때 추가되는 판정·증거 |
| Lifestate, ECOOP 2019 | 프레임워크 lifecycle·API 사용 규칙·callback 흐름의 통합 모델링 | cFS의 순서 분류와 증명 범위, 모델링 비용 |

근거: [P1](https://people.eecs.berkeley.edu/~ksen/papers/ASE.pdf), [P2](https://fsl.cs.illinois.edu/publications/huang-luo-rosu-2015-icse.pdf), [P3](https://drops.dagstuhl.de/entities/document/10.4230/LIPIcs.ECOOP.2019.1). 이 표는 관련 개념의 비교이며 원 도구의 cFS 실행 결과가 아니다.

## 4. 구현 전에 수정할 형식적 쟁점

### 4.1 P0 — protected의 정의가 현재 인증서보다 강하다

**위치:** [S1, §1.10, §4.5, §6.2 L-PR, §6.3 G2](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/Architecture/spec/SPEC_FINAL.md)

현재 `protected`는 다음을 요구한다.

- 사용에 도달하는 실행이 존재한다.
- 도달한 실행에는 나쁜 결과가 없다.
- 해당 사용을 막는 영구 대기 실행도 없다: `S_i = ∅`.

그러나 PROTECTED 인증서와 L-PR은 **사용 지점에 도달했을 때 요구 상태가 참**이라는 성질만 보인다. 도달 전에 교착되거나 영구 대기하는 실행이 없다는 증명 의무가 없다.

반례 구조는 간단하다. 처음부터 준비된 자원을 사용하는 태스크 앞에 두 mutex 획득이 있고, 다른 태스크와의 획득 순서에 따라 교착 또는 정상 진행이 가능하다고 하자. 도달한 모든 사용은 준비 상태일 수 있지만, 교착 실행은 여전히 존재한다. 준비 상태의 증명은 그 교착을 배제하지 못한다.

**권고 수정:**

- 첫 구현의 인증서는 ‘사용에 도달한 경우의 안전성’을 보장하도록 한정한다.
- 영구 대기·교착·진행 가능성은 별도의 판정으로 둔다.
- 기존 `protected` 정의를 유지하려면 `S_i = ∅`를 위한 별도 증명 의무와 검사 절차를 추가한다.
- 정의 수정은 판정 표, 인증서, L-PR, G2, 결과 표시 방식에 함께 반영한다.

**완료 조건:** 사용의 준비 상태는 보장되지만 도달 전 교착이 가능한 작은 예에서, 무조건적인 protected 판정이 나오지 않아야 한다.

### 4.2 P0 — maximal 실행에 대한 포함 관계를 다시 정의해야 한다

**위치:** [S1, §1.8 및 §6.2 L-CONT](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/Architecture/spec/SPEC_FINAL.md)

문서는 실행을 maximal execution으로 정의하면서 다음 포함을 주장한다.

`Exec(B, σ_U∞, M) ⊆ Exec(B, σ_U, M)`

하지만 전이를 제거하면 새로운 maximal 실행이 생길 수 있다.

- 어떤 준비 상태가 끝내 성립하지 않는 timed wait를 생각한다.
- σ_U에서는 timeout 전이를 통해 실행을 연장할 수 있다.
- σ_U∞에서는 timeout 전이가 제거되어 그 자리에서 막힌 실행이 maximal이 될 수 있다.
- 그 막힌 실행은 σ_U에서는 연장 가능하므로 maximal 실행이 아니다.

따라서 현재 정의에서 ‘전이를 줄였으므로 maximal 실행 집합도 부분집합’이라는 논증은 충분하지 않다. 이는 모델이 전부 잘못되었다는 뜻이 아니라, 포함 관계와 보장 이전을 주장하는 대상이 정교하지 않다는 지적이다.

**권고 수정:**

- 유한 실행 prefix 집합의 포함과 안전성 보존을 먼저 정의한다.
- 최대 실행, 공정성, timeout 제거에 따른 교착·진행성 변화는 별도로 증명한다.
- 무한 대기나 stuttering을 도입하려면 실행 의미에 명시하고 기존 보장을 다시 점검한다.
- 안전성 이전과 진행성 이전을 하나의 transfer rule로 처리하지 않는다.

**완료 조건:** timeout이 있는 모델에서는 진행하지만 timeout 제거 모델에서는 정지하는 최소 사례에 대해, 실행 포함 관계와 결과 이전 규칙이 모순 없이 설명되어야 한다.

### 4.3 인증서의 독립 검증 범위를 정확히 표현해야 한다

`kz proofcheck`는 분석 엔진과 코드를 공유하지 않지만 `facts.kz`의 추출 결과를 신뢰한다. 따라서 추론 단계의 재검증과 실제 소스에서 사실을 정확히 추출했다는 보장은 구분된다. [S1, §4.5 및 §6.1](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/Architecture/spec/SPEC_FINAL.md)

이는 독립 checker의 가치를 부정하는 지적이 아니다. 다음을 판정마다 명시해야 한다.

- 독립적으로 재검증한 추론 단계
- 추출기에 의존하는 사실
- 프레임워크 프로파일에서 신뢰한 가정
- 관측으로 반박할 수 있지만 완전히 증명하지 않은 항목

## 5. 평가 설계의 수정

### 5.1 P1 — 기존 접근 비교와 요소 제거 실험을 구분한다

현재 GPredict 방식 비교군에서는 `Allowed`, 결과 의미, capture point, certificate 등을 제거한다. [S6, §4.4](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/Architecture/07_evaluation.md)

이 비교는 제거한 요소의 효과를 측정하는 ablation으로 유효하다. 그러나 같은 결과만으로 기존 명세 기반 기법이 해당 속성을 표현할 수 없다고 결론 내리면 안 된다. GPredict는 정규식으로 속성을 기술하고 유한상태 모니터로 다룰 수 있는 형식도 논의한다. 따라서 유한한 재시도·허용 결과를 비교군에서 표현할 수 있는지 먼저 검토해야 한다. [P2, §II.B](https://fsl.cs.illinois.edu/publications/huang-luo-rosu-2015-icse.pdf)

| 질문 | 비교 조건 | 보고할 결과 |
|---|---|---|
| 위반을 발견하는가? | 가능한 한 같은 이벤트와 정상·오류 판정 조건 | 탐지·미탐·오탐 |
| 스케줄 의존성을 판정하는가? | 같은 분석 대상과 범위 | 좋은·나쁜 실행 증거, 미확정 사유 |
| 고정 순서를 증명하는가? | 같은 빌드·구성과 가정 범위 | 인증서, 적용 범위, 신뢰 가정 |
| 모델링 비용은 얼마인가? | 작성 범위를 명시 | 작성 시간, 규칙 수, 추가 수정량 |
| 특정 요소의 효과는 무엇인가? | Kairos에서 해당 요소만 제거 | ablation 결과 |

원 도구, 의미를 옮긴 재구현, Kairos의 요소 제거 실험은 명칭과 결과 해석을 분리한다. 실행하지 않은 도구의 `NOT_RUN`을 미탐으로 계산하지 않는 기존 규칙은 유지한다.

### 5.2 P1 — 경보 유무만으로 공헌을 평가하지 않는다

단순 상태 검사기도 같은 AppId 유효성 조건을 받으면 관측된 R2를 발견할 수 있다. 따라서 instance별 경보 yes/no만으로는 고정 순서 인증서의 추가 가치를 보여주기 어렵다.

다음 세 결과를 각각 측정한다.

1. 위반 발견 여부
2. 고정 순서·스케줄 의존 분류 여부
3. 분류를 뒷받침하는 증거의 범위와 검증 가능성

단순 모니터와 탐지 결과가 같아도, Kairos가 허용된 분석 범위 전체에서 고정 순서를 증명한다면 그 증거가 별도의 평가 대상이다.

### 5.3 P1 — 정답의 외부 출처와 맹검을 구분한다

수정 문서에서 기대 판정을 얻었다는 사실만으로 맹검이 성립하지 않는다. 현재 문서의 ‘기대 판정이 수정 문서에서 오므로 recall mutant에 대해 맹검을 주장할 수 있다’는 문구는 수정해야 한다. [S6, §3.5 및 §6.1](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/Architecture/07_evaluation.md)

맹검 여부에는 다음 기록이 필요하다.

- 설계자가 해당 사례·수정·기대 결과를 이미 보았는가?
- 평가 집합을 언제 분리했는가?
- 엔진·프로파일·판정 기준을 언제 동결했는가?
- 동결 이후 새 규칙이나 예외 처리를 추가했는가?

과거 수정에서 만든 mutant의 검출률은 그 평가 집합에 대한 검출률이다. 실제 순서 결함 전체에 대한 recall로 일반화하지 않는 기존 제한은 유지한다.

### 5.4 P1 — 관측 단위와 독립성을 구분한다

24회 사용, 3회 부팅, 한 결함 원인은 서로 다른 단위다. 동일 원인에서 파생된 인스턴스나 mutant를 모두 독립 표본으로 취급해서는 안 된다.

결함 원인·수정 이력·프로그램 변형·정적 인스턴스·실행 횟수를 구분해 보고하고, 통계 절차의 독립성 가정을 그 단위에 맞춰 점검한다.

## 6. 용어 검토

### 6.1 readiness specification 권고는 유지한다

‘준비 상태 명세’는 요구 상태, 허용 결과, 적용 범위, 근거를 담은 작성물 전체를 지칭하기에 적절하다. 근거의 종류에 따라 문서화된 의무, 코드가 인정한 가정, 연구자가 제안한 조건을 구분하는 방향도 유효하다. [S8](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/Architecture/background/08_terminology.md)

다만 명칭 자체가 신규성이나 정확성을 보장하지는 않는다.

### 6.2 P1 — invariant와 학습을 동일시하지 않는다

‘invariant는 실행에서 학습한 조건’이라는 서술은 과도하다. 불변식은 직접 작성하거나 증명할 수도 있다. 학습 여부는 불변식의 정의와 별개다.

κ 전체를 invariant라고 부르지 않겠다는 선택은 가능하다. 그 근거는 κ가 단일 술어만이 아니라 허용 결과·근거 등을 포함하는 명세 객체라는 데 두는 편이 낫다. 특정 학습 기반 도구와의 혼동 가능성은 보조 설명으로 둔다.

### 6.3 P1 — contract 사용 0/52의 범위를 재검토한다

GPredict 원문은 Java Iterator 사용 제약을 “a more generic contract”라고 설명한다. [P2, 인쇄 첫 페이지의 Introduction](https://fsl.cs.illinois.edu/publications/huang-luo-rosu-2015-icse.pdf)

따라서 ‘관련 문헌은 contract를 쓰지 않는다’는 일반화는 유지할 수 없다. 0/52가 공식 명칭만 센 것인지, 본문에서 요구 제약을 부른 표현까지 포함한 것인지 조사 기준을 다시 명시해야 한다. 용어 빈도만으로 contract를 배제하지 않는다.

### 6.4 실패 처리와 애플리케이션 의무를 분리한다

문서화된 오류 반환값이 존재한다는 사실만으로 호출자의 의무가 사라지는 것은 아니다. 다음 세 층을 구분해야 한다.

| 층 | 질문 |
|---|---|
| API 동작 | 준비되지 않았을 때 API가 어떤 결과를 반환하는가? |
| 애플리케이션·임무 요구 | 해당 호출자는 어떤 상태나 전달 결과를 보장해야 하는가? |
| 관측 결과 | 실제로 어떤 실패·손실·종료가 발생했는가? |

SB의 무구독 송신이 프레임워크 수준에서 허용될 수 있다는 점은 유지한다. 그러나 특정 임무가 반드시 전달하도록 요구한다면 애플리케이션 수준 판단은 별도로 해야 한다. 프레임워크의 정상 동작과 임무 요구 충족을 같은 판정으로 합치지 않는다.

## 7. 구현·실험 범위 조정 권고

현재 thin MVC는 S0 + S1 + S2 + S3a다. 문서의 단계별 추정을 합산하면 다음과 같다. [S7, §8.2](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/Architecture/08_roadmap.md)

| 단계 | 문서의 추정 |
|---|---:|
| S0 | 3–4 person-weeks |
| S1 | 5–7 person-weeks |
| S2 | 3–5 person-weeks |
| S3a | 8–12 person-weeks |
| 합계 | 19–28 person-weeks |

이는 실제 소요시간 측정값이 아니다. 다만 계획상으로도 첫 가설 검증 범위가 크다는 신호다.

### 권고하는 다음 실행 순서

| 순서 | 수행할 일 | 완료 기준 |
|---|---|---|
| 1 | protected와 진행성 분리, 실행 포함 관계 수정 | 정의·증명·결과 표시가 일치한다 |
| 2 | 하나의 고정 구성에서 R2의 K-ID를 끝까지 연결 | 추출·관측·판정·인증서 생성과 독립 검사가 동작한다 |
| 3 | 올바른 발행 순서의 대조 구성 평가 | 원래 fixed-order 판정이 적절히 철회된다 |
| 4 | C1 경로를 활성화하고 세 결과를 평가 | 즉시 성공·재시도 성공·최종 실패를 구분한다 |
| 5 | 설계에 사용하지 않은 사례·수정 전후 쌍 평가 | 추가 규칙·수작업 비용·미확정 사유를 함께 보고한다 |

R2의 대조 구성은 새로운 분석 기준 B′다. 두 구성의 결과 차이는 구성 민감도 근거이며, 같은 구성의 스케줄 의존성 근거가 아니다.

C1의 스케줄 의존성을 주장하려면 **같은 빌드·구성·분석 의미에서 좋은 실행과 나쁜 실행**이 필요하다. hold로 timeout을 발생시켰다면 조작된 실행이라는 표시를 유지하고 자연 발생 빈도로 해석하지 않는다.

MLIR·SMT·DPOR와 15개 종류 전체의 구현은 이 작은 검증이 확보된 후 필요성이 드러나는 순서로 확장하는 것을 권고한다.

## 8. 수정 우선순위와 산출물

| 우선순위 | 항목 | 수정 대상 | 필요한 산출물 |
|---|---|---|---|
| P0 | protected의 안전성·진행성 불일치 | SPEC_FINAL §1.10, §4.5, L-PR, G2 | 일관된 정의와 반례 검사 |
| P0 | maximal 실행 포함 관계 | SPEC_FINAL §1.8, L-CONT, transfer rules | prefix·최대 실행·진행성의 구분 |
| P1 | 기존 접근 비교와 ablation 혼재 | 평가 계획 §4 | 같은 속성 정보를 제공하는 비교와 요소 제거 실험 |
| P1 | 탐지 여부만으로 공헌 평가 | 평가 계획의 지표 | 탐지·분류·증거의 별도 결과 |
| P1 | 정답 출처와 맹검 혼동 | 평가 계획 §3.5, §6.1 | 사례 노출·동결·변경 기록 |
| P1 | 용어 논거의 과도한 일반화 | STATUS 및 08_terminology | invariant·contract 서술 수정 |
| P1 | 첫 구현 범위 과대 | 로드맵 | R2→C1 중심의 작은 검증 단계 |
| P2 | CORD 결과 요약의 갱신 누락 | R2_cord_v701 §8 | 초기 구성과 복구 구성의 결과 분리 |

P0는 현재 형식 보장 문구를 사용하기 전에 해결해야 할 항목이다. P1은 구현과 평가 방향을 결정하는 항목이며, P2는 기존 증거를 정확히 인용하기 위한 문서 정리다.

## 9. 현재 허용되는 주장 범위

| 주장 | 검토 판단 |
|---|---|
| 저장된 gdb 3회 기록에 각각 R2의 발행 전 사용 24회와 대응한 오류 결과가 있다 | 원시 trace 재집계로 확인 |
| Kairos가 R2를 탐지·증명했다 | 아직 주장 불가: 구현 전 |
| 반복 관측으로 R2의 고정 순서가 증명됐다 | 주장 불가: 정적 증명과 가정 검토가 필요 |
| 기존 process-level 도구가 다섯 사례를 놓쳤다 | 주장 불가: NOT_RUN은 미탐이 아님 |
| 명세 기반 판정 자체가 새롭다 | 주장 불가: 가까운 선행 연구가 존재 |
| cFS 의미를 반영한 순서 분류와 인증서가 공헌이 될 수 있다 | 검증할 가치가 있는 공헌 후보 |
| 다섯 사례를 현재 기본 구성에서 재현했다 | 주장 불가: 사례별 버전·구성·공개성 차이가 있음 |
| 준비 상태 명세라는 용어가 적절하다 | 권고 가능: 수정된 논거와 명확한 정의가 필요 |
| 현재 thin MVC만 완성하면 학회 논문이 보장된다 | 주장 불가: 차별성·평가 결과·일반화 수준에 달림 |

## 10. 근거 링크

저장소 링크는 모두 검토한 커밋에 고정했다. 문서 안의 소스 코드 설명은 해당 연구 기록을 근거로 검토했으며, 이번 검토에서 NASA cFS 전체 소스를 새로 빌드·실행하여 모두 재검증한 것은 아니다.

### 연구 문서

- [S1 — SPEC_FINAL.md](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/Architecture/spec/SPEC_FINAL.md): 정의, 알고리즘, 인증서, 보장, 평가 계획.
- [S2 — mapping_v701.md](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/cases/mapping_v701.md): 다섯 사례와 현재 기준 코드의 대응.
- [S3 — cases/ledger.md](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/cases/ledger.md): 사례별 원인, 버전, 플랫폼, 재현 종류.
- [S4 — R2_gdb_v701.md](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/results/R2_gdb_v701.md): 기존 gdb 관측 결과와 제한.
- [S5 — R2_cord_v701.md](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/results/R2_cord_v701.md): 초기 11회 결과, 구성 복구 후 24회 결과, 요약 갱신 누락.
- [S6 — 07_evaluation.md](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/Architecture/07_evaluation.md): 비교군, 지표, recall, 맹검, 통계 계획.
- [S7 — 08_roadmap.md](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/Architecture/08_roadmap.md): 단계별 추정과 thin MVC.
- [S8 — 08_terminology.md](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/Architecture/background/08_terminology.md): 용어 검토 및 수정 제안.
- [S9 — STATUS_20261012.md](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/STATUS_20261012.md): 현재 현황과 결정 요청.
- [S10 — 09_prior_work.md](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/Architecture/09_prior_work.md): 기존 기법과의 관계 및 신규성 가설.

### 재집계한 원시 기록

- [T1 — r2_g1_trace.jsonl](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/env/logs/r2_g1_trace.jsonl)
- [T2 — r2_g2_trace.jsonl](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/env/logs/r2_g2_trace.jsonl)
- [T3 — r2_g3_trace.jsonl](https://github.com/wookjaeya/Shape-Perf/blob/b4ac55fbab244d24286154f317003aa3fa2518ae/research/cfs_procrace/env/logs/r2_g3_trace.jsonl)

### 확인한 선행 연구

- **P1.** Pallavi Joshi and Koushik Sen. *Predictive Typestate Checking of Multithreaded Java Programs.* ASE 2008, pp. 288–296. [저자 제공 원문](https://people.eecs.berkeley.edu/~ksen/papers/ASE.pdf) · [DOI](https://doi.org/10.1109/ASE.2008.39). 이번 검토에서 초록·서론의 typestate 검사 및 예측 설명을 확인했다.
- **P2.** Jeff Huang, Qingzhou Luo, and Grigore Roșu. *GPredict: Generic Predictive Concurrency Analysis.* ICSE 2015, pp. 847–857. [저자 제공 원문](https://fsl.cs.illinois.edu/publications/huang-luo-rosu-2015-icse.pdf). 이번 검토에서 초록·서론과 §II.B의 명세 형식, contract 표현을 확인했다.
- **P3.** Shawn Meier, Sergio Mover, and Bor-Yuh Evan Chang. *Lifestate: Event-Driven Protocols and Callback Control Flow.* ECOOP 2019, LIPIcs 134, pp. 1:1–1:29. [공식 출판 페이지](https://drops.dagstuhl.de/entities/document/10.4230/LIPIcs.ECOOP.2019.1) · [DOI](https://doi.org/10.4230/LIPIcs.ECOOP.2019.1). 이번 검토에서 공식 초록의 lifecycle·typestate 통합 및 동적 검증 설명을 확인했다.

