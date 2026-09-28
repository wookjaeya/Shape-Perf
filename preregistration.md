# 사전등록 (Preregistration) — 초안, **미고정**

명세 §10.2: 파일럿(G3)에서 결정하고 본실험 전에 잠그는 값. 기계 판독용 사본은
`configs/preregistration.json`이며, `scripts/freeze_prereg.py`로 고정(content hash)한다.
고정 전에는 `scripts/groundtruth_dense.py`, `evaluate.py`가 본실험 실행을 거부한다.

**현재 상태: 아래의 모든 "미정" 값은 비어 있다. 임의의 기본값을 넣지 않았다(명세 §2).**

## A. 이미 코드로 고정된 규칙 (결과를 보기 전에 정함)

| 항목 | 규칙 | 위치 |
|---|---|---|
| 공통 첫 질의 | 유효 집합의 최소·최대 길이를 모든 정책이 먼저 질의 | `selectors/base.py: endpoint_action` |
| 구간·가운데 값 | 측정 완료 shape 사이 미측정 구간, 짝수 개면 낮은 쪽 가운데 | `base.py: intervals, lower_middle` |
| 동률 처리 | (b) 미측정 수 많은 구간 → (c) 왼쪽 끝점 작은 구간 | `policies.py: _tie_key` |
| 실패 처리 | 실패 shape도 질의된 것으로 셈. signature 비교에서 `FAILED:<type>`는 별도 값, timing 비교에서 실패 끝점 구간 점수는 0 | `policies.py` 머리말 |
| Timing-only | 1순위 `abs(log(T_right/T_left))`, `T`=해당 질의 프로세스의 지연 통계 | `policies.py` |
| Compile-guided | 1순위: 두 끝점 signature 상이 | `policies.py` |
| Compile-probe | 끝점은 측정(전체 컴파일이 signature 제공), 확정 변화점 양쪽 측정 큐 우선, probe 지출이 `budget_fraction × 예산`에 도달하면 probe 중단. hybrid(Uniform 채우기)는 ablation 전용 | `policies.py` |
| Timing-adaptive [선택] | log latency에 2차 다항식 추세 적합, 구간 관측 log 비와 추세 log 비의 차 | `policies.py` |
| Compile-probe 단계 분리 | probe 단계 signature끼리만 비교하고 전체 컴파일 signature와 섞지 않음. 두 단계 불일치는 `probe_vs_full_signatures`로 기록(G3) | `policies.py`, `broker.py` |
| Align-only ablation | 구간 [a,b] 안에 B_align 경계가 있을 때만 signature 차이를 인정(B_align과 동일 정의) | `policies.py: has_aligned_boundary` |
| 예산 경계 | 완료 시각 ≤ 예산인 질의만 인정, 초과 질의는 기록만. 후보·확인도 예산 내 질의의 것만 셈 | `broker.py`, `evaluate.py: discoveries, recall_cost` |
| 비용 분리 | 공통(전체 컴파일·검증·워밍업·측정·확인) vs 정책 추가(정보 추출, probe). 비용 제외 변형(§11.2-2)에서도 실제 probe 지출로 probe 상한 적용 | `broker.py` |
| 후보 확인 | 유효 집합에서 인접한 두 shape가 모두 측정되는 순간 한 번 검사, 관측 \|log 비\| ≥ log(1+δ)이면 확인 절차 실행 | `broker.py` |
| 탐색기 격리 | 탐색기는 별도 프로세스에서 자기 View만 받음 + 그 안에서 파일·프레임 조회·신규 import 차단(위반은 except로 숨길 수 없음) | `broker.py`, `selector_worker.py`, `guard.py` |
| signature 정규화 | `sig-v1` (trip count·정수 literal·SSA·위치 제외, 결정 필드만) | `shapeperf/signature.py` |
| 검정 | 기본 양측: p = 2·min(두 꼬리). 방향(`events.direction`)은 사전등록 | `evaluate.py: effect_test` |
| 검정 족 | 유효 집합의 모든 인접 쌍(데이터가 없는 쌍은 p=1로 포함) — 데이터와 무관하게 고정 | `evaluate.py: answer_table_a` |
| 최소 블록 | 짝지은 블록이 `min_blocks_per_pair` 미만인 쌍은 '불충분'(부트스트랩 퇴화 방지) | `evaluate.py: effect_test` |
| 정답표 A | 발견 데이터에서 Holm 보정 후보 → 독립 확인 데이터에서 발견 방향 단측·δ 초과(Holm) | `shapeperf/evaluate.py` |
| 부트스트랩 해상도 | 최소 p(양측 2/reps) × 족 크기 ≥ α이면 실행 거부 (예: 215개 경계·α=0.05·양측 → 8,601 이상) | `evaluate.py: answer_table_a` |
| B_align 정의 | 경계 (s, s+1)에서 s 또는 s+1이 정렬 단위의 배수 | `evaluate.py: aligned_boundaries` |
| CLI 값 우선순위 | 사전등록 값이 우선, `[]`도 유효한 값. 고정된 실행에서 CLI 대체값 거부 | `evaluate.py: pick` |

## B. 파일럿(G3) 이후 고정할 값 — 모두 미정

| 키 (`configs/preregistration.json`) | 결정 근거 | 값 |
|---|---|---|
| `correctness.logit_abs_tolerance` (+근거) | upstream 테스트 허용치 검토 또는 FP32 독립 구현 간 차이·과제 정확도 (§7) | 미정 |
| `measurement.warmup_iterations` | G3 warmup 추이 (`pilot_report.json: warmup_profile_ns`) | 미정 |
| `measurement.timed_iterations`, `processes_per_shape`, `blocks`, `vm_allocations` | G3 계층별 분산·비용, Kalibera–Jones (`kalibera_jones`) | 미정 |
| `measurement.cpus` | 측정 VM의 NUMA/코어 구성 | 미정 |
| `measurement.process_statistic` | mean 또는 median (프로세스 내 요약) | 미정 |
| `events.delta_min_effect` (+`delta_source`), `sensitivity_deltas` | 배포 요구가 없으면 연구 목적·측정 정밀도 근거 (§9.2) | 미정 |
| `events.alpha`, `confidence_level` | 관례에 따른 분석 선택으로 명시 (§10.3) | 미정 |
| `events.direction` | 양측(two-sided) 또는 증가만(increase) — 연구 질문의 "급락" 정의 | 미정 |
| `events.min_blocks_per_pair`, `min_allocations_per_pair` | G3 블록 수·재할당 계획 | 미정 |
| `events.bootstrap_reps`, `bootstrap_seed` | 해상도 가드 충족 | 미정 |
| `events.match_tolerance_lengths` | ±0 또는 ±1 (§9.4) | 미정 |
| `selectors.random_seeds` | seed 수는 파일럿 분산으로 (§10.3) | 미정 |
| `selectors.shape_only_alignment_units` (+근거) | **G0에서 실제 opt-report로 확인한 대상 사실**만 사용 (§8.1). 관측된 급락에 맞춰 사후 선택 금지 | 미정 |
| `selectors.compile_probe_budget_fraction` | G3 probe : 측정 비용비 (§8.2b) | 미정 |
| `selectors.budgets_ns` | G3 비용 추정 | 미정 |
| `signature.probe_stage` | G3: probe 단계가 §6.2 feature를 모두 결정하는지, 최종 실행물과의 불일치율 | 미정 |

### 정렬 단위 후보에 관한 사실 (결정 아님)

고정한 ONNX-MLIR 소스(v0.5.1.1) 정적 판독 결과, x86에서 krnl 수준 SIMD 기계 모델은
`--march=x86-64`일 때만 SSE4.2 모델(128-bit, FP32 VL=4)로 설정되고, 그 밖의 `--march`
값(미지정·`native` 포함)은 SIMD 미지원 모델이 된다(`src/Compiler/CompilerPasses.cpp:69`,
`src/Dialect/Mlir/VectorMachineSupport.cpp`). LLVM 단계의 자동 벡터화는 `--mcpu`에 따라
더 넓은 레지스터를 쓸 수 있다. **이 두 사실은 G0에서 실제 opt-report·역어셈블로 확인한 뒤에만**
Shape-only 정렬 단위의 근거로 쓸 수 있다.

## C. 고정 절차

1. G3 파일럿 보고서를 근거로 B의 값을 JSON과 이 문서에 동시에 기입(근거 문장 포함).
2. `python scripts/freeze_prereg.py` → `frozen_sha256` 기록.
3. 이후 변경은 이탈(deviation)로 `report.md`에 기록하고 재고정.
