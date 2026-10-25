# kz predict — 한정 예측(명시적 상태 탐색)과 C1 후보 (2026-10-25)

- 지위: **prediction candidate**(SPEC rev 1.3 §4.9 `MODEL_CANDIDATE`). 결과가 아니다. 후보는 제어 실행으로 확인되기 전까지 모델의 후보이며(SPEC §4.13 CF-01–CF-04), 후보 수는 성과가 아니다(RV §11 단계 D) [D]. 입력 기록은 build-smoke(natural) smoke3 tx 넷이고, 이 문서의 어떤 수도 natural·controlled 결과와 더하지 않는다. tx·rx는 별개 인스턴스이며(P7) rx 자연 기록은 없다. 번들 후보 A(`1df262a`)·B(`1b0b338`)는 따로 돌렸다(P4).
- 결정 근거: 단계 D(`Architecture/spec/PI_DECISIONS_RESOLVED_20261025.md` §2 D), P-Z3(명시적 탐색 먼저, SMT는 같은 작은 모델에서 필요가 보일 때; 2026-10-15/10-22 결정 유지) [D].
- 표기: [M] 실행해 확인, [D] 결정, [F] 고정 소스에서 읽음, [I] 해석, [U] 미확인.

## 1. 설계 [D][M]

**모델 파일**(`kzp-predict 1`, `include/kz/predict/model.hpp`). 콘(cone)의 행위자마다 프로그램을 적는다: Φ 프로필의 연산 호출(`use`·`create`·`delete-each`·`spawn`·`clear`·`set`·`exit`), 반환 코드에 따른 분기(`if-result`/`unless-result`), 상수로 묶인 반복(`counter`·`if-below`), 대기(`await`), 지역 사건(`event`·`mark`). 모든 단계는 인용(`prov`: 프로필의 출처 기록, 또는 모델 파일의 `cite` = 파일·줄·인용 줄의 SHA-256)을 단다. 사례 지식은 `profiles/c1-bp-b051eb0/predict/*.kzm`에만 있고 `lib/predict`·`include/kz/predict`에는 없다(`lint_case-ids` 통과).

**상태 의미는 프로필에서 온다.** `use`의 결과는 그 단계 시점의 자원 상태에서 계산한다: conjunct(`state(K(arg.P)) = S`)가 서면 ready 결과 코드, 아니면 not_ready 결과 코드. 이 계산은 P1 조건부 매핑을 쓰므로 그 `narrow` 적용 조건(키를 텍스트로 가짐, 길이 < `OS_MAX_API_NAME`, 호출 문맥)이 모델 입력에서 서야 하며, 서지 않으면 모델을 거부한다. `create`는 프로필의 lifecycle 효과(새 세대·id, 소유 = 생성 행위자), `clear`/`set`은 같은 지점·같은 목표 상태의 프로필 효과가 있어야 받아들인다. `exit`할 때 그 행위자가 만든 객체가 살아 있으면(ES 정리가 지울 대상) 모델이 계속할 수 없다고 표시한다(C1에서는 없음: CF는 콘의 객체를 만들지 않는다).

**두 종류의 순서를 분리한다**(RV §5.3, §11 D).
- 제약(모든 탐색 순서에서 유지): 행위자 안의 프로그램 순서, 생성(생성된 task는 그 생성 단계 뒤에만 실행; R2-P-TASK), 프로그램의 대기(종료 요청이 있어야 teardown), 기록의 stratum S 간선(행위자 사이).
- 변수(관측 실행에 종속, 순서마다 다시 계산): 기록의 stratum O 간선(RF: 성공 조회와 생성의 reads-from), 결과만이 정하는 순서(실패 조회 → 그 뒤의 생성). 예측 탐색에 제약으로 넣지 않는다. 탐색한 순서마다 결과를 상태에서 계산하므로 RF는 그 순서에서 다시 유도된다.

**앵커.** 기록의 콘 단계(행위자별 호출·키·결과)를, 기록의 행위자 간 간선 전부(S와 O)를 지키는 모델 실행 하나로 재현해야 예측을 한다. 재현하지 못하거나, 모델 밖 행위자가 콘의 키를 쓰면 예측을 하지 않는다. 앵커 실행의 모델 재생 판정은 기록의 관측 판정과 같아야 한다(아래 §4에서 넷 모두 같음 [M]).

**탐색.** 가시 단계(선형화 시점의 원자 단계) 하나가 전이 하나다. 방문 상태 집합을 쓰는 깊이 우선 탐색이며 부분순서 축소가 없다(SPEC rev 1.3 "Engine": 기준 비교용 명시적 탐색). 종단 실행은 인스턴스 행위자(역할 `dependent_use`의 사용을 가진 행위자)의 결과 이력으로 묶는다(키마다 결과의 연속 구간, 횟수는 1/여럿으로 추상, "생성 전 사용 수", 끝 상태). 각 묶음의 대표(횟수가 가장 작은 실행)와 가장 큰 실행을 **같은 프로필의 재생**(`eval::replay`, model record)으로 다시 판정한다: 관측 판정과 같은 준비 명세·결과 매핑·감시(monitor)·범위. 재생이 conjunct 지점의 상태와 어긋나는 결과, guard 실패, 입력 오류를 찾으면 그 후보를 **기각**한다.

**보류 계획(.kzh) 합성과 점검.** 후보의 대표 실행에서 키마다 필요한 순서를 정한다: 생성이 첫 사용보다 앞(사용자 보류, 생성 뒤 해제), 모든 사용 뒤(생성자 보류, 마지막 사용 뒤 해제), 그 사이(생성자 보류, k번째 사용 뒤 해제; 다음 사용은 스케줄러에 남음). 보류·해제 지점은 모델이 단계마다 선언한 `hit`(관측 프로필의 사이트, `caller-in`·`name` 조건)에서, 그 실행에서 처음 닿는 지점만 고른다(kzp-hold 1은 첫 해당 hit에만 작용한다). 한 주소에 제어점 하나, 주소 넷 이하(디버그 레지스터)를 지킨다. 그런 다음 **같은 모델을 계획의 supervisor 의미로 다시 탐색**해 무엇을 강제하는지 보고한다: 게이트는 첫 해당 hit를 붙잡고, 해제는 게이트가 아직 붙잡지 않았어도 첫 hit에서 소비되며, 풀린 스레드는 디버그 레지스터가 지워져(`probe/hold/supervisor.cpp` L384–L385, 다음 ptrace 정지에서만 다시 무장) 이후 hit가 아무것도 하지 않는다고 본다. 전체 계획이 그 후보를 강제하지 못하면 키 하나씩의 계획도 만들어 점검한다. 같은 제어점의 계획은 앞의 것을 가리킨다.

## 2. 사용법 [M]

```sh
P=kairos/profiles/c1-bp-b051eb0
kz predict <run.kzt> --profile $P --profile $P/cf-<REV> --profile $P/bundle-<CAND> --profile $P/rspec-<REV> \
   --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 --const OS_MAX_API_NAME=20 \
   --basis <basis_core-tx.kz> --facts <closure.facts.kz> --static-basis <closure.basis.kz> \
   --model $P/predict/00_common.kzm --model $P/predict/cf-<REV>.kzm \
   --prov-root <prov_roots/<cfg>_<cand>> --observe-profile $P/observe/sites-cf<REV>.kzp --out-dir <dir>
#  ablation: --edges fixed (관측 종속 순서를 제약으로) / --outcomes keep (기록의 결과·분기 유지, 그 순서 제거)
#  --max-states N: 방문 상태 상한(연구자 선택, 기본 2,000,000, 출력에 적힘). --bind M=R: 모델 행위자 M = 기록 행위자 R
```

기록 재생은 `kz replay`와 같은 입력·옵션으로 한다(결과 표 `plan/results_pilot_20261025.md` §0과 같은 상수·닫힘). 출력 디렉터리: `predict.txt`(보고), `anchor.trace`, `W<n>.trace`(후보의 모델 trace: `kz-phi replay --trace`로 독립 재판정 가능 [M]), `W<n>[-k].kzh`(계획; `kz hold`의 파서로 다시 읽고 관측 프로필의 사이트·종류를 확인함). `--prov-root`는 모델 파일의 `cite` 13개를 고정 번들에서 다시 읽는다(네 구성 모두 13/13 일치 [M]). 시험: `tests/predict/test_predict_model.cpp`(작은 프로필·모델; 62 검사).

## 3. 경계 — 레퍼런스가 정하지 않는 연구자 선택과 탐색하지 않은 것 [D]

| 경계 | 선택 | 탐색하지 않은 것 |
|---|---|---|
| 콘 | 행위자 core-tx(시작 상태·task 생성·teardown), CFE_ES(종료 요청), CF 주 task, BP 주 task; 자원 `BP0_tsem`·`BP1_tsem`·종료 상태 | 다른 task·객체(기록의 다른 task 생성 7, 다른 객체 삭제 1을 목록으로 냄; SCH_LAB의 세마포어는 다른 키) |
| 단위 | OSAL 호출 하나 = 선형화 시점의 원자 단계(조회는 type 전역 잠금 아래 검색, 생성·삭제는 EXCLUSIVE 트랜잭션: `osal_get_by_name`, `osal_convert_token_global`, `osal_allocate_new`, `osal_transaction_finish`) | 호출 안의 창: 생성 중 RESERVED 구간(A5, P-C1HOLD 후속 항목), 다른 생성 실패(빈 id 없음 등) |
| 시간 | 시간 없는 교차(지연은 길이 없는 단계, SPEC §4.9 "C_σ is never added") | 실제 지연(100 ms×25, 시작 지연 50 ms)과 실행 길이가 배제하는 순서; 빈도 |
| 범위 | 인스턴스 행위자가 끝나기 전에 종료 요청이 오면 그 요청까지 탐색하고 scope-cut으로 센다(기본 범위 witness 없음); TEARDOWN 뒤에는 teardown 자신의 삭제만 | 요청 뒤의 K-TEARDOWN 계열, teardown 중 task 삭제 |
| BP 경로 | BP_FlowInit부터; BP_FlowInit 성공 가정(계획의 해제 지점 `es.child_task.entry`가 BP_CLA_Init에 의존, 모든 기록에서 성공) | BP의 앞선 초기화 실패(P-E, E-10), 다른 생성자(E-12) |
| 상한 | 방문 상태 2,000,000(연구자 선택) | 넷 모두 상한 미도달: pre 89 상태, post 8,393 상태로 완료 [M] |

반복 상한 25와 이름은 프로필 상수(`cf_platform_retries`, `bp_semcfg_names`)에서만 온다(연구자 선택 아님).

## 4. C1 결과 (smoke3 tx, 후보 A·B 각각; A와 B의 묶음·계획 동일 [M])

산출물: `/home/user/work/procrace/c1_runs/predict1/<pre|post>_<cand>/{cur,edges_fixed,outcomes_keep,p1off}/` + `*.log`, 드라이버 `run_predict.sh`, 입력 해시 `sha256_inputs.txt`(kz `ac91e349…`). 모든 실행 종료 0.

**앵커** [M]: pre(A·B) 55 상태, post(A·B) 41 상태에서 기록을 재현. 모델 재생 판정 = 기록 판정(pre: (CF,`BP0_tsem`,1) NOT_READY_UNJUDGED → `INCONCLUSIVE(caller_requirement_absent)`; post: (CF,`BP0_tsem`,1) TOLERATED, (…,2) READY, (CF,`BP1_tsem`,1) READY). 간선: 제약 = R2-P-TASK 3(core-tx → CFE_ES·CF·BP); 변수 = post의 RF 2(BP 생성 → CF 2·3번째 조회), 결과 결정 순서(pre·post 각 1: CF 첫 실패 조회 → BP의 `BP0_tsem` 생성).

**탐색** [M]: pre 묶음 8(scope-cut 5, 후보 3: 관측 1 + 새 2), post 묶음 26(scope-cut 13, 후보 13: 관측 1 + 새 12). 기각 0. 각 묶음의 최대 횟수 실행도 같은 상태로 판정됨.

| 기록 | 후보 | 묶음(CF의 키별 결과) | 재생 판정(모델) | 계획(경로는 `…/predict1/<기록>/cur/`) · 모델 점검 | 확인 실행의 라벨(결정 기록 §2) |
|---|---|---|---|---|---|
| pre | W1 | `BP0` 실패 1, 종료 | 관측과 같음 | 없음(자연) | — |
| pre | **W2** | `BP0` 첫 조회 성공, **`BP1` 실패 1**, 종료 | (CF,`BP0`,1) READY; **(CF,`BP1`,1) NOT_READY_UNJUDGED** → `INCONCLUSIVE(caller_requirement_absent)` | `W2.kzh`: **실현 불가**(CF를 붙잡았다 푼 뒤 CF의 종료로 BP를 풀어야 함 → supervisor 한계). `W2-2.kzh`(CF를 `BP0` 생성 뒤까지 보류만; 묶음 2개 중 하나, `BP1`은 스케줄러), `W2-1.kzh`(G(`BP1_tsem`)+CF 종료; `BP0`은 스케줄러) | W2-2: researcher intervention. W2-1: P-C1HOLD 형태(G(name)+R-d) |
| pre | **W3** | 두 키 모두 첫 조회 성공, "CF Initialized" | 둘 다 READY → `NO_VIOLATION_IN_CHECKED_PREFIX` | `W3.kzh`: CF를 첫 조회 진입에서 보류, BP가 `CFE_ES_CreateChildTask`(BP_CLA_OUT, bp.so 호출)에 닿을 때 해제 → 모델에서 강제(해제가 먼저 오면 정체 가능) | researcher intervention(단계 C와 같은 순서, 더 늦은 해제) |
| post | **W1** | `BP0` 25회 실패, 종료(E-06) | (CF,`BP0`,1–25) VIOLATING → `OBSERVED_VIOLATION`; 시간 기반 전이 | `W1.kzh` = G(`BP0_tsem`) + CF `ExitApp`(단계 B 계획과 같은 제어점) → 강제 | controlled (P-C1HOLD), timeout-exercised(CF-04, T-16) |
| post | **W13** | 두 키 모두 첫 조회 성공 | 둘 다 READY | `W13.kzh`(pre W3와 같은 제어점) → 강제 | researcher intervention |
| post | **W10** | `BP0` 첫 조회 성공, **`BP1` 25회 실패**, 종료 | (CF,`BP1`,1–25) VIOLATING → `OBSERVED_VIOLATION`; 시간 기반 | `W10.kzh` 실현 불가(위와 같은 supervisor 한계). `W10-1.kzh` = `W2-2.kzh`(G(`BP1_tsem`)+CF 종료: `BP1` 소진 강제, `BP0`은 스케줄러) · `W10-2.kzh`(`BP0` 첫 성공만) | W10-1: P-C1HOLD 형태; W10-2: researcher intervention |
| post | W2, W6 | `BP0` 재시도 성공(m=2, m≥3) + `BP1` 25회 실패 | `BP0` TOLERATED…READY, `BP1` 1–25 VIOLATING | `W2-2.kzh`/`W6-2.kzh`(같은 계획: G(`BP1_tsem`)+CF 종료) | P-C1HOLD 형태 |
| post | W9 | `BP0` m=2, `BP1` 첫 성공 | 관측과 같음 | 없음(자연) | — |
| post | W3–W5, W7, W8, W11, W12 | `BP0`·`BP1`의 재시도 성공 조합(m=2…25) | TOLERATED…READY | 부분 계획만: `W6-1.kzh`(=`W7`·`W8`: BP를 `BP0` 생성에서 보류, CF의 첫 재시도 지연 뒤 해제 = §9.2.1 R-c 형태), `W12-1.kzh`(`BP1`에 같은 형태), `W5-2.kzh`, `W10-2.kzh`(=`W11`·`W12-2`). 횟수 m≥3을 정확히 고르는 지점은 kzp-hold 1에 없다(첫 hit만) | researcher intervention |

**ablation** [M]: `--edges fixed`(RF·결과 결정 순서를 제약으로) → pre·post 모두 **관측 묶음 하나만** 남는다: E-06(post W1), `BP1` 소진, 수정 전 READY가 모두 사라진다(RV §5.3이 경고한 제거). `--outcomes keep`(기록의 결과·분기를 유지하고 그 순서를 버림) → pre 후보 2 중 1, post 6 중 5가 재생에서 **기각**(성공 결과가 생성 앞에 놓이거나 실패가 생성 뒤에 놓임: "result disagrees with the state"). `--outcome-narrowing off` → 기록의 관측 판정만 바뀌고(pre (CF,`BP0`,1) UNDETERMINED → `INCONCLUSIVE(caller_requirement_absent, explanation_ambiguous)`, post 1번째 UNDETERMINED → `INCONCLUSIVE(horizon-open, explanation_ambiguous)`) 예측 묶음은 같다: 모델은 결과를 상태에서 계산하며, 그 계산 자체가 P1 매핑(적용 조건 포함)에 기댄다.

**기대 결과표(동결) 대조** [M]: E-01(pre A1 READY, "예측 W5")↔pre W3; E-04(post `BP1` A1)↔post W13 등; E-05(A2 m)↔post W9(m=2)와 m=3…25 묶음; E-06↔post W1; E-03↔pre W1(관측); E-09(채널 0 실패 시 `BP1` 조회 없음)↔모델 규칙(`break`, 아래). E-07·E-08·E-10·E-12는 모델 밖(§3). 표는 고치지 않았다.

## 5. 모델이 배제한 순서와 이유 [F][M]

- 생성 앞의 성공 조회: 성공은 그 키의 LIVE 상태에서만 나오고, LIVE는 BP의 생성만 만든다(정적 닫힘: writer는 `OS_ObjectIdInit`·`OS_CountSemAPI_Init`·`OS_CountSemCreate`·`OS_CountSemDelete` 안에서만, `r_countsem_type_slices`). `--outcomes keep`에서 실제로 생성되어 기각됨.
- 채널 0 최종 실패 뒤의 `BP1` 조회: CF 코드가 채널 루프를 `break`한다(`cf_cfdp_fail_event` L1014–L1019@833fdbb, `cf_cfdp_lookup` L991–L1001@281a941).
- 수정 전의 TOLERATED·재시도: 조회 1회, Allowed `unspecified_by_reference`(rspec-281a941).
- `BP1`을 `BP0`보다 먼저 만드는 BP: 같은 스레드의 루프 순서(`bp_flow_throttle_init` L121–L144, `m_bp_num_throttles`).
- CF 종료가 BP의 세마포어를 지움: ES 정리는 종료하는 앱의 task가 만든 객체만 지운다(`m_es_cleanup_task`, `m_osal_creator`, `m_osal_foreach_creator`).
- 기본 범위 안의 삭제·재생성: 삭제는 TEARDOWN(종료 요청 뒤)에만; 요청이 CF의 결정 앞이면 scope-cut(pre 5, post 13 묶음)이며 witness가 아니다.
- CF와 BP 사이의 순서를 정하는 동기화: 없음(시작 동기화 호출 없음, ES 전역 대기는 ES만 막음; `plan/C1_basis_plan.md` §6.2). 그래서 두 방향 모두 탐색한다.

## 6. 단순 관측 판정과 비교해 보이는 것과 보이지 않는 것 (H3) [I]

**보이는 것.** (1) 자연 기록 하나(구성마다 tx 1회)와 인용한 코드 응답만으로, 관측 판정이 같은 순번에서 주지 못하는 **반대쪽 판정**을 모델 후보로 낸다: pre 기록(나쁜 쪽만 관측)에서 (CF,`BP0`,1) READY 후보(W3), post 기록(좋은 쪽만 관측)에서 (CF,`BP0`,1–25) VIOLATING 후보(W1)와 첫 조회 성공(W13). 단계 B·C가 연구자 설계로 만든 순서와 같은 순서를 기록·모델에서 기계적으로 다시 얻었다. (2) **관측도 제어 실행도 보인 적 없는 후보**: `BP1_tsem` 키의 나쁜 쪽 — pre W2((CF,`BP1`,1) NOT_READY_UNJUDGED), post W10/W2/W6((CF,`BP1`,1–25) VIOLATING). (3) 후보마다 실현 계획과 그 계획이 모델에서 무엇을 강제하는지, 그리고 현재 hold 기구로 실현할 수 없는 후보(pre W2 전체, post W10 전체)를 이유와 함께 낸다. (4) 관측 종속 간선을 제약으로 고정하면 실패 순서가 사라지고, 결과를 유지한 채 간선만 버리면 실행 불가능한 후보가 생겨 재생이 기각함을 같은 기록에서 보였다.

**보이지 않는 것.** 후보의 실제 실현 가능성(확인 실행 전), 시간이 허락하는지(모델은 시간이 없다), 빈도·재현성(P8), rx, 다른 구성. W1·W3/W13은 단계 B·C에서 이미 같은 순서가 실현됐으나 그 실행은 예측 전에 연구자 설계로 한 것이며 이 후보의 확인으로 세지 않는다(§7 P12). 모델 판정은 P1 매핑으로 결과를 계산하므로 반환값 독립 검증이 아니다. 후보 수(pre 2, post 12)는 결과가 아니다.

## 7. 남은 것과 PI 결정

- 확인 실행(Confirm): 위 표의 계획. 우선순위 [I]: post `W2-2.kzh`(`BP1` 소진, 새 후보), pre `W3.kzh`·post `W13.kzh`(READY 순서, 더 늦은 해제), post `W1.kzh`(E-06 재확인), pre `W2-2.kzh`(`BP1` NOT_READY_UNJUDGED를 노리나 `BP1` 순서는 스케줄러에 남음). 인스턴스마다 따로(tx·rx, P7), 결과는 controlled 라벨로만.
  - **(2026-10-25 실행됨)** confirm1: 결과는 `plan/results_pilot_20261025.md` §5. 실현: pre W2·W3, post W6·W12(rx 만)·W13 + 단계 B 재사용 post W1; refuted 0; 실현 안 됨은 모두 제어 원인(부분 계획, 해제 선소비, L384–L385, 첫 hit 해제). 새 PI 결정 요청 P13(해제되지 않은 보류 실행의 앞부분) [M].
- 보류 기구의 한계(공학 항목, 결정 아님): 풀린 스레드의 디버그 레지스터를 즉시 다시 무장하지 않아(`supervisor.cpp` L384–L385) pre W2·post W10 전체 계획은 실현 불가로 점검된다. 정확한 순번(m≥3)의 해제에는 `occurrence <n>` 같은 어휘가 필요하다.
- **P11(대기 중)**: 확인 실행 재생에서 보류기의 정지·해제 순서를 HB로 쓰는가. 읽기 A(쓴다): W1·W10/W2-2의 미준비 25개와 W2의 미준비가 개입 순서로 서고 P1과 무관해진다. 읽기 B(안 쓴다): 단계 B처럼 미준비는 ON(반환 코드 좁힘, P1 조건부), READY 후보(W3·W13)는 RF(RD)에 기댄다. 이 문서는 정하지 않는다.
- **새 PI 결정 요청 P12: 예측 전에 한 제어 실행(단계 B·C)을 같은 순서의 후보(post W1, pre W3)의 확인으로 셀 수 있는가.** SPEC §4.13은 후보의 지향 확인(directed confirmation)과 CF-01–CF-04만 정하고, 확인 실행이 예측 뒤에 그 후보의 계획으로 돌아야 하는지는 말하지 않는다. RV §11 D는 "후보를 실제 실행으로 확인"이라고만 적는다. 단계 C의 해제(`BP1_tsem` 생성 진입)는 W3의 해제(BP_CLA_OUT 생성 진입)와 다르다. 권고 [I]: 세지 않고, 예측 계획으로 새로 돌린다.

## 8. 보류 계획 어휘 개정과 다시 만든 계획 (predict2, confirm2, 2026-10-25) [M]

- 보류 기구 개정(`docs/c1_hold_runs.md` §8): 풀린 스레드 무장 유지(§1 의 "L384–L385, 다음 정지에서만 다시 무장" 은 이제 해당 없음), `occurrence <n>`, 한 주소의 여러 점(주소 넷 이하), 보류된 hit 의 해제는 재개 때, `unless-released`. 계획 점검은 같은 의미로 탐색한다(`lib/predict/explore.cpp`; 게이트는 가시 단계의 before-hit 에만). 범위 절단(종료 요청이 먼저 옴) 실행의 보류는 정체로 세지 않는다.
- 합성(`lib/predict/predict.cpp`): 첫 hit 가 아닌 지점은 그 표지의 몇 번째 hit 인지(occurrence)로 쓴다. 키의 생성이 k 번째와 k+1 번째 사용 사이(0 < k < 모든 사용)이면, 공급자를 생성에서 잡고 k 번째 사용 뒤(k+1 번째 사용 전)에 푸는 것에 더해 **소비자를 k+1 번째 사용에서 잡고 생성 뒤에 푸는 쪽**을 더한다(이전 노트 "the use after it is left to the scheduler" 는 없어진다; 더한 쪽은 필요 목록 끝에 붙여 이전 번호를 지킨다). 해제는 그 창에서 첫 hit 표지를 먼저 고르고(이전 계획을 그대로 지킴), 계획이 주소 넷을 넘으면 모든 해제를 가장 가까운 hit 로 다시 고른다(계획 노트에 적힘). 모든 게이트에 `unless-released`.
- 결과(`/home/user/work/procrace/c1_runs/predict2/`, `run_predict.sh`; 같은 smoke3 tx 기록·입력, 현행 모드만): 후보는 predict1 과 같다(pre 3, post 13; 기각 0). **모든 새 후보의 전체 계획이 모델에서 "강제, 정체 없음"** 이다(pre W2·W3, post W1–W8·W10–W13). 그래서 키 하나씩의 계획(`W<n>-<k>`)은 만들지 않았다. pre W2·post W10·post W1 의 제어점은 predict1 과 같고 `unless-released` 만 더해졌다. A·B 두 후보의 계획은 주석만 다르다.
- 이 점검은 시간 없는 모델의 것이다. 실제 실행에서 해제가 게이트보다 먼저 오는지는 시간에 달려 있고(그래서 `unless-released`), 확인은 실행으로만 한다(`plan/results_pilot_20261025.md` §5.6).
