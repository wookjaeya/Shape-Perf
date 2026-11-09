# RV4 뒤 설계 초안 셋: 요구 순서 규칙 서식, E3 집계 표, P0 mutex 대조 서식 (2026-11-09)

- 성격: **설계 초안이다. 결정이 아니다.** RV4A §7.2 단계 5다. 셋 모두 아래에 적은 PI 결정 뒤에만 확정한다. 각 절 머리에 기다리는 결정을 적는다.
- 지시: PI 2026-11-09 08:40:43Z "타당한 만큼 수용하고 진행해"(RV4A 머리말 [F]). 무조건 받지 않고 파일·출력으로 대조했다.
- 하지 않은 것: 코드를 고치지 않았다. 빌드하지 않았다(RV3A Q12 대기). 추출·relations를 다시 돌리지 않았다. cFS를 부팅하지 않았다. 결정 문서·RV3A·RV4A·STATUS를 바꾸지 않았다. 표준 본문을 새로 열지 않았다.
- 표기: [M] 이번(11-09)에 도구·출력으로 확인, [F] 파일에서 읽음(약칭:줄), [I] 추론, [U] 미확인. [D]는 이 문서에 없다.
- 약칭: RV4(외부 검토서 `Kairos_RV3_Review_Assessment_20261105.md`, "RV4 L n"은 그 줄), RV4A(`plan/RV4_REVIEW_ANALYSIS_20261109.md` 개정 1), RV3A(`plan/RV3_REVIEW_ANALYSIS_20261105.md` 개정 4), PD(`plan/PI_DECISIONS_PENDING_20261030.md` 개정 6), PID(`Architecture/spec/PI_DECISIONS.md`), AR(`plan/step2a_results_20261030.md`), S4(`plan/step2a_s4_20261030.md`), CP(`CONDITIONS_POLICY.md`), T/(`/home/user/work/redesign/step2a/tests/`, 2a 최종 출력), C5S(`/home/user/work/c5tools/build/641/pristine/cFE-6.4.1-OSS-release/`), H01(RV3·RV4가 인용한 이전 검토서; 우리에게 원문이 없다, RV4A §3).
- 이름 주의: 집계 표는 늘 "RV3A E3"이라 쓴다. PD의 E3(E-15 ② 동결 표)과 다른 항목이다.
- 이번에 잰 것과 명령은 §4에 적는다. 모두 읽기다.

## 0. 요약

| 초안 | 정하는 것 | 출발점 | 기다리는 결정 | 결정 전에 쓰는 법 |
|---|---|---|---|---|
| 1. 요구 순서 규칙 서식 (§1) | 규칙 하나마다 적을 다섯 요소, 근거·허용 동작의 종류, relations 행의 새 칸의 꼴 | RV4 L89–105, RV4A §2 #20, RV3A §2.8 | C4(RV3A Q4), RV3A Q6(E2), B5, A4, B4, A2, B6·B1·B2(RV3A Q5). A7(H01)은 근거를 더할 뿐 서식을 바꾸지 않는다 [I] | 규칙을 설계할 때의 점검표로만 |
| 2. E3 집계 표 (§2) | 다섯 집계의 단위·출처와 지금 값, N 밖 행의 공개, 따로 적는 수 | RV4 L235–247, RV4A §2 #21–#23, RV3A E3·Q4(개정 4) | C4(N·묶기·판정자), B8, B7, A2 (b), B6·B1·B2, RV3A Q6, C7 | AR §6 옆 표의 꼴로만. 정밀도를 내지 않는다 |
| 3. P0 mutex 대조 서식 (§3) | 원시 반환값마다 효과와 래퍼 값의 대응, 구멍과 덮는 가정, 위 층 | RV4 L133–139, RV4A §1 17·19·20행, §2 #7–#10, RV3A §2.6 | A4, A7, B4, B8, B5, RV3A Q12 | C5 #9의 기댐 목록을 설명하는 데만. P0 표와 facts를 바꾸지 않는다 |

- 셋을 코드로 옮기려면 코드 변경과 빌드가 든다. 빌드 자리는 RV3A Q12의 답 뒤다. 이번에는 하지 않는다.
- 셋에는 숫자 상한·시간 값이 없다. 필요해지면 CP의 상태(`from-reference`·`deviation`·`unspecified_by_reference`)로 적고 PI에게 묻는다(CP:22, D15, A3·D1).

## 1. 요구 순서 규칙 서식 (초안)

**기다리는 결정**: C4와 RV3A Q4(T의 기준; 이 서식의 "근거"와 "실행 증거"가 T의 요건이 되는지), RV3A Q6(정의 초안 E2; 다섯 요소의 이름을 정의와 맞춘다), B5(2b 범위; 근거를 유도하는 규칙이 2b에 드는지), A4(레퍼런스 의미를 근거로 쓰는지), B4(writer 값 근거가 기대는 완전성 규칙), A2(②의 허용 조치; 사례 무관한 변경만), B6·B1·B2(실행 증거를 기록에 잇는 길; RV3A Q5). A7(H01)은 RV4 L91의 근거를 더할 뿐이다.

### 1.1 출발점

- RV4 L89: 출력 칸을 더하는 것만으로 판정 알고리즘이 서지 않는다 [F: RV4].
- RV4 L93–101: 규칙마다 다섯을 적는다. 적용 대상, 근거, 허용 동작, 실행 증거, 한계 [F: RV4].
- RV4 L103: 사람이 사례마다 정답을 쓰라는 요구가 아니다. 일반 규칙이 근거를 자동으로 내게 하고 적용 한계를 적는다. NASA 이슈와 패치는 사후 사례 검증에만 쓴다 [F: RV4].
- RV4 L91: 실패할 수 있다, 반환값을 버린다, 다른 순서에서 성공한다는 사실만으로 그 시점이 금지됐다고 할 수 없다. 최종 crash 없이도 위반 실행은 확인될 수 있다. RV4는 이것을 H01 §2.4에 둔다. 우리에게는 [문서 보고]다(RV4A §3).
- 같은 뜻의 우리 근거 [F]: RV2 L351 "필요한 순서를 유도하지 못한 단순 read/write 충돌은 확정 결과가 아니다"(RV3A §2.8, E1). RA:51의 한계 문장(RV3A Q4). 그래서 이 서식은 H01 없이도 선다 [I].
- RV3A §2.8 개정 4가 다섯 요소를 받았다. 유도 출처는 callee-check, handling, writer 값이다 [F].
- RV4 L105: 관측된 잘못된 순서와 경합 입증을 같은 수로 합치지 않는다. 이 서식은 둘을 실행 증거의 두 층으로 나눈다(§1.5).
- 지금 relations 행에는 요구 순서 칸이 없다 [M]. 행의 칸은 `dependency`, `run-order`, `feasibility`, `violation`, `flip`, `layers`이고 A행에는 `handling` 줄이 있다(T/ 다섯 빌드의 `relations.txt`). `violation` 칸의 공식은 RP §3.6에서 왔는데 RV2 이름으로 실렸다(RV3A E1). 그 문구는 Q4 답 뒤에 고친다.

### 1.2 규칙 카드 (규칙 하나에 카드 하나)

사례 행마다 쓰는 것이 아니다. 규칙 하나가 모든 빌드의 모든 행에 같게 적용된다.

| 칸 | 적을 것 | 출처의 제한 |
|---|---|---|
| 규칙 이름·판 | 규칙 이름과 판. relations 규칙 판과 함께 출력 머리에 적는다 | — |
| 적용 대상 | 관계 종류(A 조회–생성, B 공유 필드 인자, C 생성·등록), 연산 의미(P0·L0 항목, 유도 연산), 객체 관계(같은 구획·키, 같은 필드, spawn이 만든 자식) | 사례·필드·callee 이름을 쓰지 않는다. relations도 그런 입력을 받지 않는다(`relations.hpp` 머리 주석) [F] |
| 근거 | §1.3의 종류 가운데 무엇인가, facts의 어느 섹션에서 오는가. 행마다 출처 자리(파일:줄)를 단다 | 빌드의 코드(facts)와 CP의 레퍼런스만. 레퍼런스 의미는 A4 뒤 |
| 허용 동작 | §1.4의 종류. 조회 실패·재시도·지연 초기화가 허용되는 근거 | 코드나 레퍼런스에서만. "그럴 수도 있다"는 근거가 아니다 |
| 실행 증거 | §1.5. 어떤 사건과 값이 위반을 보이는가, 어떤 정체성이 필요한가 | 관측 계획(단계 3)이 이 칸에서 관측할 것을 뽑는다 [I]. 사례별 손 지점을 쓰지 않는다 |
| 한계 | §1.6. 근거가 모자라면 무엇으로 남기는가, 기대는 계층·가정 | 기존 `layers`·`rests-on`을 그대로 전파한다 |
| 시험 | 사례 이름 없는 fixture의 기대(RV3A E14) | fixture는 좁은 기능 검증이다. 일반화 근거가 아니다(RV4 L261). 단위 시험 값은 연구 조건이 아니다(CP:23) |
| 변경 기록 | 계기(사례·행·날짜), 이전 판, "출력 뒤" 여부 | A5, A2 ① |

### 1.3 "근거"의 종류

| 종류 | 뜻 | 지금 facts의 자리 [M] | 빈 곳 |
|---|---|---|---|
| (ㄱ) callee 검사 | callee가 그 값(키·id·필드)에 따라 다른 상수를 돌려주는 조건 | `[returns]`·`[retconsts]`의 `when=` 식, B행의 `callee-check` | 식이 풀리지 않은 행 |
| (ㄴ) 결과 처리 | 호출자가 실패 상수를 받았을 때 가는 곳: 종료 호출, 오류 상태 쓰기, 반환 전파 | A행의 `handling` 줄, `[terminals]`, `[loopexits]` | 전파가 끊기는 곳, 간접 호출 |
| (ㄷ) writer 값 | 읽는 쪽이 writer 앞이면 초기값을, 뒤면 쓴 값을 본다. 그 차이가 (ㄱ)·(ㄴ)으로 이어지는가 | `[writers]`, `[init]`, `[fwrites]`, C행 | writer 완전성은 B4의 규칙에 달렸다. 흐름 민감한 도달 정의는 2b 항목이다(B5) |
| (ㄹ) 레퍼런스 의미 | P0·L0 항목이나 cFS 공식 문서가 정한 순서 요구 | `[p0]` 72항목. 원문 대조는 1이다 [F: PD A4] | A4 전에는 "미확인(P0-unverified)" |

- 근거는 사슬로 선다 [I]. (ㄱ)이나 (ㄷ)만으로는 "값이 달라진다"일 뿐이다. 그 값이 (ㄴ)의 실패 처리나 (ㄹ)의 요구에 닿아야 요구 순서의 근거가 된다. RV2 L351과 RV4 L91의 경계가 이것이다.
- 사슬의 고리마다 facts 자리를 단다. 한 고리라도 풀리지 않으면 §1.6의 "미확인(이유)"다.
- 최종 피해(crash 등)는 근거의 요건이 아니다(RV2 L219 [F: RV3A E1 메모]). 최종 피해는 이미 별도 칸이다(RV3A §2.10 셋째 행 메모).

### 1.4 "허용 동작"의 종류

| 종류 | 코드의 꼴 | 지금 있는 것 [M] |
|---|---|---|
| (a) 없으면 만든다 (probe-create, 지연 초기화) | 조회의 실패 결과에서 같은 함수가 같은 키를 만든다 | A행 짝 배제의 `probe-create`(C1 `relations.txt`에 표시가 있다) |
| (b) 성공까지 다시 부른다 | 실패하면 다시 부르는 루프이고, 나가는 조건에 성공이 든다 | `[loopexits]`와 handling의 루프 줄(나가는 식). 나가는 식이 성공을 요구하는지 가르는 규칙은 없다 |
| (c) 레퍼런스가 정상이라 한 실패 | CP의 레퍼런스(공식 문서 등)가 그 실패를 정상 경로로 적는다 | 없음. 인용이 있을 때만 쓴다 |

- 오류 처리 없이 계속한다는 것만으로는 허용 근거가 아니다 [I]. 반환값을 버리는 것도 그렇다(RV4 L91). 허용 근거가 없다는 것이 요구 순서의 근거도 아니다. 근거는 §1.3에서만 온다.
- 순서를 강제하는 동기화(잠금, 조건 대기)는 허용 동작이 아니다. 다른 순서의 실현 가능성(`feasibility`)이 다룬다.
- 허용 동작은 행의 T·F를 정하지 않는다. PD C4 권고는 probe-create·재시도 같은 후속 처리를 따로 된 칸으로만 보고하고 T·F를 정하지 않는다고 적는다. (다) 넓은 F도 쓰지 않는다고 권한다 [F: PD C4 권고; 결정 대기]. 이 서식에서 허용 동작이 하는 일은 그 행을 근거 성립 수(§2 집계 3)에서 빼는 것뿐이다. 그 행을 N 안의 U로 셀지 N 밖으로 둘지는 C4다.
- (c)는 C4 F (iv)(레퍼런스가 정한 허용 의미; H01 확인 뒤의 선택지, RV4A §3)와 닿는다. 이 서식에서는 (c)를 F로 쓰지 않는다.

### 1.5 "실행 증거"

- 꼴 [I]: 읽는 쪽 사건(호출·읽기, 행위자 r, 자원 인스턴스 o, 돌려받은 값 v)이 쓰는 쪽 사건(같은 o의 생성·쓰기 완료, 행위자 w) 앞에 있고, v가 §1.3 사슬이 쓴 그 값(실패 상수·초기값)이다.
- 필요한 정체성:
  - 행위자: 실행의 task와 앱 소속. 생성 호출 인스턴스와 자식 시작을 직접 잇는 증거가 든다(RV4 L125–129, B6 단계 1). root 이름·생성 호출·앱 소속은 다른 정보다.
  - 자원 인스턴스: 키 값, 표 원소, 객체 주소와 그 세대. F0 경로는 tid의 세대를 FORK 시각으로 가른다. 프레임워크 id와 객체의 세대 구분은 두 경로 모두 없다 [F: RV4A §2 #11].
  - 기록과 facts의 결합: 같은 빌드에서 뽑은 facts여야 한다(RV4 L113, RV3A §6.4 L1). 결합 길은 B1과 RV3A Q5 (나)다.
- 두 층을 따로 적는다.
  - E-관측: 잘못된 순서가 근거의 값과 함께 관측됨.
  - E-경합: 순서를 바꾸면 결과가 바뀜이 입증됨.
  - RV3A Q4 개정 4: T 가운데 경합 입증은 부분 수로 따로 보고한다. 반대 순서 실행을 늘 요구하지는 않는다(RV4 L105).
- 다른 행의 실행에서 본 사건은 이 행의 실행 증거가 아니다 [I]. RV4 L255가 RacePro에 대해 든 구분과 같다.
- 기록이 없으면 "실행 증거: 기록 없음"이다. 지금 다섯 빌드의 모든 행이 이렇다 [M: `run-order: no record given` 431 / 431 / 338 / 178 / 175행 = 행 수].

### 1.6 "한계"와 출력 값

| 출력 | 언제 |
|---|---|
| `established`(근거 종류) | §1.3의 사슬이 끝까지 이어지고, §1.4의 허용 근거가 없다고 facts로 가를 수 있다 |
| `allowed-behavior`(종류, 자리) | §1.4의 허용 근거가 있다 |
| `candidate` | §1.3의 근거가 없다. 단순 충돌이다(RV2 L351) |
| `unknown`(이유) | 사슬의 한 고리가 풀리지 않는다: 키 미해결, 조건식 미해결, writer 미완전, 허용 여부를 가르지 못함(루프 나가는 식 등), `P0-unverified`, 선언된 가정에 기댐 |

- 기대는 계층(B·P0·L0·R)과 가정(`assume.*`)·전제는 지금의 `layers`·`rests-on`처럼 행에 그대로 단다.
- `unknown`을 `candidate`로 바꾸지 않는다. 둘의 수를 따로 낸다.

### 1.7 relations 행의 새 칸 (꼴만; 구현하지 않음)

```
      required-order: rule <이름> rev <n>; basis <(ㄱ)–(ㄹ)의 사슬>: <facts 자리들>; allowed: <none | (a)–(c): 자리 | unknown(<이유>)>; evidence: <필요한 사건·값·정체성>; outcome: <established | allowed-behavior | candidate | unknown(<이유>)>
```

- 기존 칸과의 관계 [I]:
  - `dependency`는 관계가 있는 이유다. `required-order`는 순서가 요구되는 이유다. 다르다.
  - `feasibility`는 다른 순서가 실현될 수 있는가다. 그대로 둔다.
  - `violation`은 Q4 답 뒤에 이 칸과 실행 증거로 다시 정한다(RV3A E1).
  - `flip`은 정적 반사실 신호로 그대로 둔다. 신호이지 판정이 아니다(RA:44, RA:51 [F: PD C4]).
- relations와 같이 결정적이어야 하고, 사례 이름을 입력으로 받지 않는다.
- 구현은 코드 변경과 빌드가 든다. B5 범위 안에서, RV3A Q12 뒤에만 한다.

### 1.8 규칙을 만들고 바꾸는 순서 (초안)

1. 규칙 카드(§1.2)를 먼저 쓴다. 계기가 사례 행이면 A5 기록에 계기 사례·행·날짜를 적는다.
2. fixture 기대를 사례 이름 없이 쓴다(RV3A E14).
3. 다섯 빌드 모두에 적용하고 판을 올린다(A2 ② 권고).
4. 사례 행의 변화는 회귀 결과로만 적는다.
5. NASA 이슈·패치는 사후 대조에만 쓴다. 규칙 입력에 넣지 않는다(RV4 L103).
6. 일반화 근거는 동결 뒤 대상에서만 온다(RV4 L263, RV3A E14).

## 2. E3 집계 표 (초안)

**기다리는 결정**: C4(N의 단위와 묶기 규칙, T·F·U 정의, 판정자; PID:423 두 평가자 가운데 한 사람 이상 비저자), RV3A Q4(T의 기준), B8("불가"를 주 평가에서 무엇으로 셀지), B7(좁히기 규칙과 전·후 수), A2 (b)(잴 빌드 목록; `r2_all`·C1 1b0b338을 민감도 줄로 낼지), B6·B1·B2(집계 2·4의 연결; RV3A Q5), RV3A Q6(집계 3의 "근거"의 정의), C7(C3·R2의 원인 공유; ledger X4).

### 2.1 출발점

- RV4 L231–245: 1,553행은 1,553개 race가 아니다. 다섯 집계를 따로 둔다. T/N–(T+U)/N은 같은 단위의 N에서 미확인의 영향 범위이지 통계 신뢰구간이 아니다. 연결되지 않은 행을 분모에서 빼더라도 그 수를 공개한다 [F: RV4].
- RV3A E3 개정 4와 Q4 개정 4가 이것을 받았다 [F]. RV4A §2 #21–#23이 그 정정이다.
- 경로마다 행의 단위가 다르다(RV3A E3) [F]. A행은 조회 지점 × 이름 공간, B행은 (필드, callee#인자)이고 지점을 묶는다, C행은 (spawn, 필드, 읽기 지점)이다.

### 2.2 다섯 집계의 정의

| # | 집계(RV4 L239–243) | 목적(RV4) | 세는 단위 (초안) | 출처 | 지금 |
|---|---|---|---|---|---|
| 1 | 원 관계 행·지점 | 추출량과 분석 부담 | 경로별 행. 경로별 지점(A 조회 지점, B 필드와 지점, C spawn과 필드) | `relations.txt` | 있다 [M] (§2.3) |
| 2 | actor·자원 인스턴스가 연결된 관계 | 연결 범위 | 두 쪽 행위자가 실행 단위(task·앱 소속)로 정해지고 자원이 인스턴스(키 값·원소·객체)로 정해진 행 | 소속 규칙(B6 단계 1), 키·잠금 정체성 | 정의 대기. 정적 대리값만 있다 (§2.4) |
| 3 | 필요한 순서의 근거를 산출한 관계 | 의미 분석 범위 | §1.6 출력이 `established`인 행. `allowed-behavior`·`candidate`·`unknown`은 따로 센다 | §1.7의 새 칸 | 0 (칸이 없다) |
| 4 | 실제 사건에 대응한 경고·미확인 | 동적 연결 성과 | 실행 기록의 사건이 행의 두 쪽에 결합된 행. 그 가운데 E-관측·E-경합(§1.5) | 기록 결합(B1, RV3A Q5) | 0 (기록 입력이 없다) |
| 5 | 독립 검토한 T·F·U와 원인 묶음 | 판정 타당성과 중복 영향 | 판정자가 정한 T·F·U와 독립 원인 묶음 | C4 | 0 (판정자 미지정) |

- 다섯은 서로 다른 단위다. 서로 나눈 비율을 내지 않는다 [I].
- 빌드마다 따로 적는다. 빌드 사이를 더하지 않는다. C1 pre/post는 행 머리 431 가운데 424가 같다 [F: RV3A E3]. 다섯 빌드의 행을 더한 1,553은 행 수의 합일 뿐이다.

### 2.3 집계 1의 지금 값 [M] (T/, relations 규칙 개정 5)

| 빌드 | A 행 | A 지점 | B 행 | B 필드 | B 지점(행의 `sites=` 합) | C 행 | C spawn | C 필드 | 행 합 | 짝 없음 |
|---|---|---|---|---|---|---|---|---|---|---|
| c1_pre_tx | 127 | 113 | 170 | 106 | 367 | 134 | 4 | 13 | 431 | 91 |
| c1_post_tx | 127 | 113 | 170 | 106 | 367 | 134 | 4 | 13 | 431 | 91 |
| r2_core | 68 | 60 | 155 | 102 | 293 | 115 | 4 | 14 | 338 | 48 |
| c5_641 | 66 | 63 | 73 | 42 | 163 | 39 | 2 | 4 | 178 | 64 |
| c5_642 | 66 | 63 | 71 | 41 | 159 | 38 | 2 | 3 | 175 | 64 |

- C1 값은 RV3A E3의 값(지점 113, 필드 106, 지점 367, spawn 4, 필드 13)과 같다. 다른 네 빌드는 이번에 같은 방법으로 셌다. 명령은 §4.
- "짝 없음"은 `feasibility: undecidable: no pair` 행이다. 합 358은 RV3A E3과 같다. 순서 쌍이 아닌 행이다. N 밖 후보다(§2.5).
- B 지점은 행마다 `sites=`를 더한 값이다. 한 지점이 두 B행에 들면 두 번 센다. 서로 다른 지점 수는 아니다.
- 행 합과 등급 분포는 AR §6 표와 같다 [F].

### 2.4 집계 2–5의 지금 상태

- 집계 2: 정의 대기다. 정적 대리값만 있다 [M: `summary` 줄].
  - A행 가운데 다른 행위자에 같은 구획의 생성자가 있는 조회: 36 / 36 / 20 / 2 / 2.
  - 그 가운데 키까지 맞는 것: 2 / 2 / 1 / 0 / 0.
  - 이 행위자는 정적 root 집합이다. 실행의 task 인스턴스도 앱 소속도 아니다(RV4 L129). 그래서 대리값은 집계 2가 아니다. B6 단계 1과 2b의 자원 이름·인덱스 흐름(B5) 뒤에 다시 정한다.
- 집계 3: 0이다. 칸이 없다. 재료만 있다: B행 가운데 callee 검사가 있는 행 40 / 40 / 29 / 26 / 24 [M]. 이것은 §1.3 (ㄱ)의 재료다. 근거 성립 수가 아니다.
- 집계 4: 0이다. 모든 행이 `run-order: no record given`이다 [M]. `violation: unconfirmed`도 모든 행이다 [M].
- 집계 5: 0이다. 판정자를 PI가 아직 지정하지 않았다(C4). 사람을 확보하기 전에는 독립 검토를 했다고 주장하지 않는다(PID:423 [F: PD C4]).

### 2.5 N 밖 행과 범위의 읽기

- N은 같은 단위의 경고 집합이다. 단위와 묶기 규칙은 C4에서 PI가 고른다. 이 초안은 고르지 않는다. 후보 단위는 RV2 L462–465의 셋(호출 수·후보 관계 수·독립 원인 수)이다 [F: RV3A Q4].
- N 밖 행은 이유마다 수를 공개한다. 이유의 초안:
  - 짝 없음(§2.3).
  - `not graded`: 1 / 1 / 5 / 14 / 14 [F: AR §6].
  - 연결 안 됨(집계 2 밖).
  - 기록 없음(집계 4 밖).
  - 허용 동작으로 설명됨, 후보, 미확인(§1.6). 셋은 따로 센다.
- 범위: T/N에서 (T+U)/N까지. 같은 N에서 U가 모두 F일 때와 모두 T일 때의 경계일 뿐이다. 통계 신뢰구간이 아니다(RV4 L245). T/(T+F)는 판정을 마친 후보의 비율일 뿐이다(RV2 L462–471 [F: PD C4]). 층화 표본(PD C4 (나))을 쓰면 그 추정과 시드·층 크기를 따로 적는다.

### 2.6 함께 따로 적는 수

- T 가운데 경합 입증(E-경합)의 부분 수(RV3A Q4 개정 4, RV4 L105).
- 자동 산출과 사람의 사후 검토를 따로(RV3A Q4 개정 4).
- fixture 검증, 개발 성과(다섯 사례), 일반화(동결 뒤 대상)를 따로(RV4 L263, RV3A E14).
- 시험 수: 237 = 옛 경로 51 + 새 186. 186을 컨테이너·직렬화·공통 기능 시험과 추출 규칙 시험으로 나눈다. 186 전부를 탐지 정확도 시험이라 하지 않는다(RV3A O8 개정 4, RV4 L247).
- 드라이버 단위 점검은 cFS 탐지 실험 수에 넣지 않는다(RV4 L178, RV4A §2 #29).
- 사례 관계 행의 비율(AR §6: 2 / 2 / 1 / 8 / 8행, 등급 있는 것 2 / 2 / 1 / 5 / 5행)은 후보 부담이다. 정밀도가 아니다(B7).

## 3. P0 mutex 대조 서식 (초안)

**기다리는 결정**: A4(표준 본문을 레퍼런스로 넣는지; CP:8의 읽기; upstream glibc 2.39와 링크된 배포판 판 가운데 무엇과 대조하는지), A7(RV4 L139의 "공식 표준 사용은 H01에서 정리"; 우리에게는 [U]), B4(선언된 가정 `assume.lock_acquisition_succeeds`의 지위), B8("불가" 행이 이 가정에 기댄다), B5(2b의 P0 대조와 속성 읽기 ①–④, RV3A §2.6), RV3A Q12(서식을 P0 표나 facts에 넣는 코드 변경의 빌드 자리). P0 판을 올리면 다섯 빌드를 다시 돌린다(PD A4 개정 2 ②).

### 3.1 출발점

- RV4 L133: `pthread_mutex_lock`은 EOWNERDEAD를 돌려주면서도 잠금을 얻을 수 있다. 소유자 아닌 해제의 의미는 종류마다 다르다. DEFAULT는 각주와 구현의 대응을 함께 본다 [F: RV4].
- RV4 L135: `setrobust` 0건은 측정이고, 경로 배제는 추론이다 [F: RV4].
- RV4 L137: C5 OSAL 래퍼는 일부 0 아닌 값을 OS_SUCCESS로 바꾼다. 그래서 원시 호출의 반환값만이 아니라 실제 잠금 효과와 래퍼가 위로 전하는 값의 대응을 본다. 이 경로의 오류가 실제로 일어났다고 주장하지 않는다 [F: RV4].
- RV4 L139: 모든 API를 먼저 완성하지 않는다. 강한 판정과 첫 실행 연결이 쓰는 mutex·cond·spawn·lookup 계열부터 판·반환 조건·효과를 대조한다. 검증하지 않은 의미에 기댄 결과에는 한계를 전파한다 [F: RV4].
- RV3 [제안](RV3A §2.6 마지막): 항목마다 구현·판, 반환 조건, 효과, 미지원 경우를 적는다. 지금 판정이 쓰는 항목부터다 [F].
- RV4A §1 17·19·20행, §2 #7–#10, RV3A §2.6 개정 4 [F]. 요약은 §3.8.

### 3.2 우리가 가진 표준 문장과 그 지위

- 읽은 문장 [F: RV4A §1 메모 17, 11-09 접근]. 출처는 The Open Group Base Specifications Issue 7, 2018 edition(IEEE Std 1003.1-2017)의 `pthread_mutex_lock` 페이지다. 그 페이지는 더 새 판이 있다고 적는다. 그래서 판을 고정해 인용한다(RV4A 메모 18).
  - (s1) 각주: "If the mutex type is PTHREAD_MUTEX_DEFAULT, the behavior of pthread_mutex_lock() may correspond to one of the three other standard mutex types"
  - (s2) EOWNERDEAD: "The mutex lock shall be acquired by the calling thread"
  - (s3) may-fail: "[EDEADLK] A deadlock condition was detected."
- 소유자 아닌 해제의 종류별 표 [F: man7 표, RV3A §2.6]: NORMAL non-robust는 UB, DEFAULT non-robust는 UB†, ERRORCHECK·RECURSIVE·robust는 오류(EPERM) 반환. 공식 페이지와 man7 재수록은 같다(RV4A §2 #10).
- 지위: 우리가 읽은 문장이다. 레퍼런스로 인정할지는 A4다. CP의 레퍼런스 넷에 표준 본문은 없다 [F: CP:5–10, PD A4]. 그래서 아래 표는 (s1)–(s3)과 man7 표를 [F]로 적되, 판정 근거로는 A4 뒤에만 쓴다.
- **우리가 문장을 갖고 있지 않은 표준 의미는 [U]다.** 이 초안에서 그런 것:
  - 0을 돌려주면 잠금을 얻었다는 것. S4와 facts의 "zero on success"는 우리 요약이다.
  - RECURSIVE의 재귀 획득은 횟수만 늘린다는 것.
  - EAGAIN이 재귀 횟수 상한의 shall-fail이라는 것.
  - EINVAL의 조건(PRIO_PROTECT 상한, 초기화되지 않은 객체).
  - ENOTRECOVERABLE의 조건.
  - EOWNERDEAD·ENOTRECOVERABLE이 robust mutex에만 묶인다는 것.
  - 오류를 돌려준 호출이 잠금 상태를 바꾸지 않는다는 것.
  - RV3A·RV4A가 이것들을 적었더라도 원문 문장은 우리에게 없다.
- 구현: P0 표는 `pthread_mutex_lock`을 "POSIX.1-2017 / glibc 2.39; source unverified"로 적는다 [M: c5_641 facts `[p0]`]. C1·R2 빌드가 링크한 판은 Ubuntu 2.39-0ubuntu8.9다 [F: PD A4]. 이 판에서 RECURSIVE·PRIO_INHERIT mutex가 어떤 값을 어느 경로로 돌려주는지 대조하지 않았다 [U].

### 3.3 서식의 칸 (행 하나 = P0 항목 × 객체 속성 × 원시 반환값)

| 칸 | 적을 것 |
|---|---|
| P0 항목·판 | 이름, P0 표 개정, 표준 판, 라이브러리 판(upstream·배포판), 원문 대조 상태 |
| 객체와 속성 | 잠금 정체성(LOCK_IDS 번호), 종류·protocol·robust를 정한 소스 줄, 도구가 그 속성을 읽는지 |
| 원시 반환값 | 0, 오류 이름마다 하나, "그 밖" |
| 조건 | 표준 문장(인용과 출처, shall·may) 또는 [U] |
| 구현 대조 | 라이브러리 소스 줄 또는 [U] |
| 효과 | 잡음 / 못 잡음 / 횟수만 바뀜 / 바뀌지 않음 / 미정. 태그를 단다 |
| 래퍼 값 | 래퍼가 돌려주는 상수와 그 줄 |
| 대응 | §3.4의 분류 |
| 도구의 읽기 | lockops 요약의 모드, 실패 경로로 빼는 갈래 |
| 덮는 가정 | `assume.*` 이름 |
| 위 층 | 래퍼를 부르는 함수가 결과를 시험하는가, 실패에서 무엇을 하는가 |
| 기대는 행 | 이 행에 기대는 relations 행(등급별 수) |

### 3.4 대응 분류

| 분류 | 효과 × 래퍼 값 | 뜻 [I] |
|---|---|---|
| 맞음-획득 | 잡음 × 성공 값 | 도구의 읽기와 같다 |
| 맞음-실패 | 못 잡음 × 실패 값 | 호출자가 결과를 시험하면 맞다. 시험하지 않으면 위 층에서 다시 본다 |
| 구멍 | 못 잡음 × 성공 값 | 도구가 성공 값을 "잡음"으로 읽으면 없는 잠금을 셈한다. 거짓 "불가" 쪽이다. 덮는 가정을 행에 단다 |
| 보수 | 잡음 × 실패 값 | 실패 경로로 가서 거짓 `possible` 쪽이다(RV3A §2.6) |
| 미정 | 효과 [U] | A4 뒤 대조로 정한다 |

- 효과가 [U]인데 래퍼 값이 성공이면 "구멍(효과 [U])"으로 적는다. 구멍일 수 있는 행을 미정에 묻지 않으려는 것이다.

### 3.5 C5 #9에 채운 예: `OS_MutSemTake` (C5S `osal/src/os/posix/osapi.c` L3329–3362)

배경:

- 두 판의 `osapi.c`는 같은 파일이다: 641 pristine, 642 pristine, 642 `b/` 모두 sha256 `32649c6f1cf3b586…` [M].
- #9 = `&OS_mut_sem_table[CFE_ES_Global.SharedDataMutex].id`, static-instance [M: c5_641 facts `[lockids]`].
- 속성 [F]: L3143 `pthread_mutexattr_init`, L3161 `setprotocol(PTHREAD_PRIO_INHERIT)`, L3177 `settype(PTHREAD_MUTEX_RECURSIVE)`, L3195 `pthread_mutex_init`. robust 설정은 C5 소스에 0건이다(`setrobust`·`PTHREAD_MUTEX_ROBUST`·`EOWNERDEAD`) [F: RV4A §8의 11-09 측정].
- 도구는 속성 호출을 읽지 않는다. P0 범위가 `pthread_mutex_init`·`pthread_mutexattr_*`을 뺀다 [M: facts `extract.p0_table_scope`].
- 도구의 요약 [M]: `OS_MutSemTake acquire result-conditional success=0 lock &OS_mut_sem_table[$0].id … {vocabulary,assumes-success,parameter-identity}`(c5_641 facts:51702, c5_642 :51855).
- 반환 상수 facts [M](c5_641 facts:37634–37636): -16 [OS_ERR_INVALID_ID](L3338), -6 [OS_SEM_FAILURE](L3348, 조건 `== 22`), 0 [OS_SUCCESS] rows=2(L3355·L3359, 조건에 `22`·`35`). 22·35는 이 빌드 머리 파일의 EINVAL·EDEADLK 값이다 [M].
- `osapi.h`: OS_SUCCESS 0, OS_SEM_FAILURE -6, OS_ERR_INVALID_ID -16 [F: C5S `osal/src/os/inc/osapi.h:87`, `:93`, `:103`].

| 원시 값 | 조건(표준) | 효과 | 래퍼 값(줄) | 대응 | 도구의 읽기 |
|---|---|---|---|---|---|
| (호출 없음) sem_id 범위 밖 또는 `free == TRUE` | — (소스 L3336) | 못 잡음 [F: 잠금 호출 전에 돌아감] | OS_ERR_INVALID_ID (L3336–3339) | 맞음-실패 | -16은 0이 아니라 잡지 않은 돌아옴이다 [M: result-conditional] |
| 0 | [U] | 잡음. 이미 잡았으면 횟수만 늚 [U] | OS_SUCCESS (else, L3357–3359) | 맞음-획득 | 잡음 [M: success=0] |
| EINVAL | [U]. PRIO_PROTECT 조건이라는 읽기는 #9 밖이다(L3161) [I: RV3A §2.6] | 못 잡음 [U] | OS_SEM_FAILURE (L3346–3349) | 맞음-실패 | 가정 아래 실패 경로 [I: S4 §1.3] |
| EDEADLK | may-fail (s3) [F] | 못 잡음 [U] | OS_SUCCESS (L3350–3356, 명시) | 구멍(효과 [U]) | 이 갈래를 가정 아래 실패 경로로 뺀다 [I: S4 §1.3, M: result-conditional]. 래퍼 값 0과 "못 잡음"의 어긋남은 가정이 덮는다 |
| EAGAIN | [U] | 못 잡음 [U] | OS_SUCCESS (else, L3357–3359) | 구멍(효과 [U]) | 가정 아래 이 값은 오지 않는다. else 갈래를 잡음으로 읽는다. 가정이 덮는다 |
| EOWNERDEAD | (s2) [F] | 잡음 [F: s2] | OS_SUCCESS (else) | 맞음-획득 | 잡음. #9에서 닿는지: robust 0건 [F: RV4A §8], 닿지 않음 [I] |
| ENOTRECOVERABLE | [U] | 못 잡음 [U] | OS_SUCCESS (else) | 구멍(닿는다면) | robust 전제로 #9에서 닿지 않음 [I]. 그 전제의 문장이 없다 [U] |
| 그 밖의 값 | [U] | [U] | OS_SUCCESS (else) | 미정 | 가정 아래 오지 않는다 |

- 덮는 가정: 모든 구멍은 `assume.lock_acquisition_succeeds`가 덮는다 [M: c5_641 facts:372 "an acquisition that is not a try succeeds … a derived acquire operation returns the value it returns when it holds the lock; the branch only a failure takes is a failure path"].
- 기대는 행: C5 "불가" 행 6(641)·14(642) 모두가 이 가정을 단다 [M: `feasibility: proven-impossible` 행 가운데 `lock_acquisition_succeeds`를 단 행 6 / 6, 14 / 14]. 그 행들의 지위는 B8이다.
- 이 표는 이 경로의 오류가 실제로 일어났다는 주장이 아니다(RV4 L137, RV4A §2 #9).

### 3.6 위 층: `CFE_ES_LockSharedData`와 호출 지점

- `CFE_ES_LockSharedData`(6.4.1 `cfe_es_api.c` L1910–1928, 6.4.2 L1849–1867) [F]: `OS_MutSemTake`가 OS_SUCCESS가 아니면 syslog를 쓰고 그냥 돌아온다. 돌려주는 값이 없다. 잠그지 못해도 호출자는 계속한다. S4 §1.3도 이 함수를 "오류 분기에서도 그냥 돌아옴"으로 적었다 [F].
- 도구의 요약 [M]: `CFE_ES_LockSharedData acquire every-return … {vocabulary,assumes-success}`(c5_641 facts:51428, c5_642 :51581). 실패 갈래는 가정 아래 실패 경로다 [I].
- 그래서 #9의 구멍은 두 층에 있다 [I].
  - 래퍼 층: EAGAIN·EDEADLK → OS_SUCCESS(§3.5).
  - 위 층: 래퍼의 모든 실패 값(OS_SEM_FAILURE, OS_ERR_INVALID_ID) → 잠금 없이 돌아옴.
  - 둘 다 같은 가정이 덮는다. 위 층의 것은 RV3A §2.6의 "호출자가 status를 무시하는 경우"에 든다. 여기서는 status를 시험하지만 실패에서 멈추지 않는다.
- `OS_MutSemTake`의 직접 호출 지점은 두 판 모두 20곳이다. 그 가운데 결과를 쓰고 시험하는 곳은 6곳이다 [M: facts `[calls]`의 `result-used`·`result-tested`]. 나머지 14곳은 결과를 쓰지 않는다. 그 14곳에서 도구가 획득을 어떻게 읽는지는 이번에 대조하지 않았다 [U]. 가정 문장대로라면 성공으로 읽는다 [I]. 그러면 그 14곳에서는 래퍼의 실패 값도 위 층의 구멍이 된다 [I].
- 서식의 "위 층" 칸은 이 셋(결과를 시험하는가, 실패에서 멈추는가, 도구의 요약 모드)을 적는다.

### 3.7 `OS_MutSemGive` (C5S `osapi.c` L3290–3312)

| 원시 값 | 조건 | 효과 | 래퍼 값(줄) | 대응 |
|---|---|---|---|---|
| (호출 없음) sem_id 범위 밖 또는 `free == TRUE` | — (L3295) | 바뀌지 않음 [F: 해제 호출 전에 돌아감] | OS_ERR_INVALID_ID (L3295–3298) | 맞음-실패 |
| 0 | [U] | 풀림. 재귀면 횟수만 줆 [U] | OS_SUCCESS (L3307–3310) | 맞음 |
| EPERM(소유자 아님, RECURSIVE) | man7 표 [F] | 바뀌지 않음 [U](RV3A §2.6) | OS_SEM_FAILURE (L3303–3306) | 맞음-실패(효과가 서면) |
| 그 밖의 값 | [U] | [U] | OS_SEM_FAILURE (L3303–3306) | 미정 |

- 도구의 요약 [M]: `OS_MutSemGive release result-conditional success=0 … {vocabulary,assumes-success,parameter-identity}`(c5_641 facts:51700, c5_642 :51853).
- C5 전제 (라)의 첫 절반(잡은 스레드만 푼다)은 종류의 귀결로 읽힌다 [I]. 표준 본문을 그 근거로 쓰는 것은 A4다(RV3A §2.6).

### 3.8 EOWNERDEAD 대 EAGAIN·ENOTRECOVERABLE: RV4A가 말하는 것

- RV4A §1 17행과 메모 17: EOWNERDEAD면 잠금을 얻는다. (s2)를 공식 페이지에서 읽었다. RV4 L133과 같다 [F].
- RV4A §1 19행, §2 #7: `setrobust` 0건은 [M]이다. EOWNERDEAD 배제는 [I]다. 그 추론은 두 전제에 선다. POSIX가 EOWNERDEAD를 robust mutex에만 묶는다는 것([U]: 문장이 없다), 그 mutex가 검색한 소스 밖에서 만들어지지 않는다는 것([I]).
- RV4A §1 20행, §2 #9: 래퍼는 EINVAL만 실패로 돌려준다. EDEADLK도 명시적으로 OS_SUCCESS로 바꾼다(L3350–3356). #9에서 잡지 못하고 OS_SUCCESS가 되는 구멍은 둘이다: EAGAIN(shall-fail, 재귀 횟수 상한)과 EDEADLK(may-fail). EOWNERDEAD → OS_SUCCESS는 획득과 맞는다. 실제로 일어났다는 주장은 하지 않는다 [F].
- ENOTRECOVERABLE: RV3A §2.6의 구멍 목록 "(EAGAIN·EDEADLK·ENOTRECOVERABLE·EINVAL)"에는 있다. RV4A §2 #9는 #9의 구멍으로 세지 않는다. 이유는 적지 않았다 [F]. 우리 읽기: EOWNERDEAD와 같은 robust 전제로 #9에서는 닿지 않는다 [I]. 그 전제의 문장은 없다 [U].
- 서식에서의 차이 [I]:
  - EOWNERDEAD는 닿는다면 "맞음-획득"이다. 구멍이 아니다. 그 뒤 mutex 상태의 의미는 문장이 없다 [U]. 이 서식은 다루지 않는다.
  - EAGAIN은 robust와 상관없이 RECURSIVE에서 나올 수 있는 값으로 RV3A·RV4A가 읽었다. 문장은 없다 [U]. 닿기 쉬운지 어려운지는 주장하지 않는다. RV3A 개정 3까지의 "닿기 어렵다 [I]"는 개정 4에서 거뒀다(RV4A §2 #9).
  - EDEADLK는 (s3) 문장이 있다. 그러나 RECURSIVE·PRIO_INHERIT mutex에서 glibc 2.39가 언제 돌려주는지는 대조하지 않았다 [U].
  - EINVAL은 래퍼에서 맞음-실패다. 위 층에서는 구멍이 될 수 있다(§3.6).

### 3.9 채우지 않은 행 (같은 서식으로 다음에)

- C5의 OSAL 표 잠금(`OS_task_table_mut` 등, LOCK_IDS #15·#17 등): `OS_API_Init`이 RECURSIVE·PRIO_INHERIT로 만든다(C5S `osapi.c` L366, L379, L394–418) [F]. 래퍼 `OS_InterruptSafeLock`의 대응은 보지 않았다 [U].
- C1·R2 OSAL: 한 프로그램에 NORMAL과 RECURSIVE가 함께 있다 [F: RV3A §2.6 ④]. 래퍼 대응은 아직 없다.
- `pthread_cond_wait`·`timedwait`(대기에서 mutex를 다시 잡음), `pthread_create`(P0에서 원문 대조가 된 유일한 항목), lookup 계열(P0 비교 표 13항목). RV4 L139의 순서다.
- `trylock`·`timedlock`: C5 두 판에서는 프로그램에 없다 [M: facts `[p0]` "not in the program"].

## 4. 부록: 이번에 잰 것 (2026-11-09, 읽기만)

```
# 디스크 (df -B1 /, Available)
작성 전 1,030,705,152 B

# T/ = /home/user/work/redesign/step2a/tests, <b> = c1_pre_tx c1_post_tx r2_core c5_641 c5_642, f = <b>/NP/relations.txt
grep -cE '^  A[0-9]+ ' f; grep -cE '^  B[0-9]+ ' f; grep -cE '^  C[0-9]+ ' f
  → A 127/127/68/66/66, B 170/170/155/73/71, C 134/134/115/39/38
grep -E '^  A[0-9]+ ' f | awk '{print $3}' | sort -u | wc -l                          → A 지점 113/113/60/63/63
grep -E '^  B[0-9]+ ' f | awk '{print $2}' | sort -u | wc -l                          → B 필드 106/106/102/42/41
grep -E '^  B[0-9]+ ' f | sed -E 's/.* sites=([0-9]+).*/\1/' | awk '{s+=$1} END{print s}'  → 367/367/293/163/159
grep -E '^  C[0-9]+ ' f | sed -E 's/^  C[0-9]+ spawn ([^ ]+) .*/\1/' | sort -u | wc -l   → C spawn 4/4/4/2/2
grep -E '^  C[0-9]+ ' f | sed -E 's/.*; field ([^;]+);.*/\1/' | sort -u | wc -l        → C 필드 13/13/14/4/3
grep -c 'feasibility: undecidable: no pair' f                                          → 91/91/48/64/64 (합 358)
grep -c 'run-order: no record given' f; grep -c 'violation: unconfirmed' f            → 둘 다 431/431/338/178/175
grep '^summary' f  → another actor 36/36/20/2/2, key-matched 2/2/1/0/0, callee check 40/40/29/26/24
grep -c 'feasibility: proven-impossible' (c5_641, c5_642) → 6, 14; 그 가운데 lock_acquisition_succeeds를 다는 행 6, 14
cat T/count/burden.tsv → AR §6 표와 같음

# C5 facts (T/c5_641/NP/facts.txt, T/c5_642/NP/facts.txt)
grep -n '^  OS_MutSemTake acquire\|^  OS_MutSemGive release' → 641 :51700·:51702, 642 :51853·:51855, result-conditional success=0 {vocabulary,assumes-success,parameter-identity}
grep -n '^  CFE_ES_LockSharedData acquire'                    → 641 :51428, 642 :51581, every-return {vocabulary,assumes-success}
sed -n 37634,37636p (641)                                      → OS_MutSemTake returns -16 / -6 (… 22) / 0 rows=2 (… 22 … 35)
grep ' -> OS_MutSemTake direct' | wc -l; | grep -c result-used; | grep -c result-tested → 두 판 모두 20 / 6 / 6
grep -n 'lock_acquisition_succeeds\|p0.pthread_mutex_lock\|p0_table_scope' (BASIS) → 가정 문장, "source unverified", 속성·초기화 항목 제외

# 소스 (C5S = /home/user/work/c5tools/build/641/pristine/cFE-6.4.1-OSS-release)
sha256sum <641 pristine, 642 pristine, 642 b>/…/osal/src/os/posix/osapi.c → 셋 모두 32649c6f1cf3b586cf1bcf31ef1228fd42193de9385e6fabb6a1365c983acac2
osapi.c L3329–3362 OS_MutSemTake, L3290–3312 OS_MutSemGive, L3143–3195 속성·init, L353–418 OS_API_Init 속성·init
osapi.h :87 OS_SUCCESS (0), :93 OS_SEM_FAILURE (-6), :103 OS_ERR_INVALID_ID (-16)
cfe_es_api.c 6.4.1 L1910–1928, 6.4.2 L1849–1867 CFE_ES_LockSharedData
grep EINVAL EDEADLK EAGAIN EOWNERDEAD ENOTRECOVERABLE EPERM /usr/include/asm-generic/errno-base.h, errno.h → 22, 35, 11, 130, 131, 1

# 원문 (읽기만)
RV4 L85–139, L229–263 (업로드 파일, sha256 f21208f38c44a013…)
```
