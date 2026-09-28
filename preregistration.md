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
| 실패 처리 | 실패 shape도 질의된 것으로 셈. signature 비교에서 `FAILED:<type>`는 별도 값, timing 비교에서 실패 끝점 구간 점수는 0. 손상된 opt-report(`CORRUPT:`)는 탐색기에게 `FAILED:corrupt_report`. 실패한 측정 프로세스 시도는 그 질의의 실패이며 비용을 낸다 | `policies.py` 머리말, `backends.py` |
| census 실패 길이 | 실패 길이에 닿는 경계는 결정 변화가 아니라 실패 경계로 따로 기록. 결정은 실패 구간 양쪽의 가장 가까운 성공 길이끼리 비교해 구간 [a, b]로 기록(구간 안에 사건이 있으면 일치, 구간 안에 단위 배수가 있으면 정렬). 손상 보고·IR 누락·성공 길이 2개 미만이면 INCOMPLETE(판정 없음) | `evaluate.py: split_failure_boundaries, census_metrics`, `run_census.py` |
| Timing-only | 1순위 `abs(log(T_right/T_left))`, `T`=해당 질의 프로세스의 지연 통계 | `policies.py` |
| Compile-guided | 1순위: 두 끝점 signature 상이 | `policies.py` |
| Compile-probe | 끝점은 측정하고 **별도로 probe도 함**(§8.2b 1단계). probe 단계 signature로만 이분 탐색, 확정 변화점 양쪽 측정 큐 우선. 끝점 probe 이후 실제 probe 지출이 `budget_fraction × 예산`에 도달하면 probe 중단. 전체 컴파일 signature는 쓰지 않으므로 그 추출 비용을 내지 않음. hybrid(Uniform 채우기)는 ablation 전용 | `policies.py` |
| Timing-adaptive [선택] | log latency에 2차 다항식 추세 적합, 구간 관측 log 비와 추세 log 비의 차 | `policies.py` |
| Compile-probe 단계 분리 | probe 단계 signature끼리만 비교하고 전체 컴파일 signature와 섞지 않음. 두 단계 불일치는 `probe_vs_full_signatures`로 기록(G3) | `policies.py`, `broker.py` |
| Align-only ablation | 구간 [a,b] 안에 B_align 경계가 있을 때만 signature 차이를 인정(B_align과 동일 정의) | `policies.py: has_aligned_boundary` |
| 예산 경계 | 완료 시각 ≤ 예산인 질의만 인정, 초과 질의는 기록만. 후보는 예산 내 질의가 만든 것만, 발견은 확인이 예산 내에 끝난 것만 셈. 후보는 생성 시점에 기록(예산 소진으로 확인을 못 해도) | `broker.py`, `evaluate.py: discoveries, recall_cost` |
| 비용 분리 | 공통(전체 컴파일·검증·프로세스 기동·워밍업·측정·확인) vs 정책 추가(정보 추출, probe). 추출 비용은 그 정책이 쓰는 signature의 것만: 주 signature = opt-report 파싱·해시, IR 구조·원문 해시는 해당 ablation에만. 비용 제외 변형(§11.2-2)에서도 실제 probe 지출로 probe 상한 적용 | `broker.py`, `compile.py` |
| 확인 절차 비용 | 정답표 A의 확인에 실제로 쓰인 확인 run의 두 끝점 프로세스 시도 전체 wall time | `evaluate.py: confirmation_costs` |
| 후보 확인 | 유효 집합에서 인접한 두 shape가 모두 측정되는 순간 한 번 검사, 관측 \|log 비\| ≥ log(1+δ)이면 확인 절차 실행 | `broker.py` |
| 탐색기 격리 | 탐색기는 별도 프로세스(OS 감옥: chroot·setuid·netns)에서 자기 View만 받음 + 그 안에서 파일·프레임 조회·신규 import 차단(위반은 except로 숨길 수 없음). 등록부 밖의 탐색기(`class_path`)는 테스트 전용(명시적 환경변수)이고 모듈 코드도 감옥 안에서 실행 | `broker.py`, `selector_worker.py`, `guard.py` |
| signature 정규화 | `sig-v2` = sig-v1(trip count·정수 literal·SSA·위치 제외, 결정 필드만) + 컴파일러가 만든 노드 이름의 `_<카운터>` 접미사(여러 개일 수 있음)를 원래 ONNX 노드 이름으로 정규화, 이름이 원래 노드와 겹치면 op 종류로 구분. 이유: G0에서 **같은 길이를 두 번 컴파일해도** 카운터가 달라짐(성능 결과를 보기 전의 재현성 점검). 컴파일러는 줄 단위 버퍼링(`stdbuf -oL`)으로 실행, 파싱 불가 줄이 있으면 signature를 `CORRUPT:`로 표시 | `shapeperf/signature.py`, `shapeperf/compile.py` |
| 검정 | t 기반 최소효과 검정(H0: \|효과\| ≤ log(1+δ)), 양측 p = 2·min(두 단측 p). 검정 단위: 쌍에 VM 할당이 2개 이상이면 할당 평균, 아니면 블록(주장은 그 할당으로 한정). 부트스트랩은 기술용 CI만. 이유: 백분위 부트스트랩 p값이 블록 수가 적을 때 Holm 수준에서 크게 반보수적(FWER 0.2–0.99)으로 확인됨 | `evaluate.py: effect_test` |
| 검정 족 | 유효 집합의 모든 인접 쌍(데이터가 없는 쌍은 p=1로 포함) — 데이터와 무관하게 고정 | `evaluate.py: answer_table_a` |
| 최소 블록 | 짝지은 블록이 `min_blocks_per_pair` 미만, 할당이 `min_allocations_per_pair` 미만, 검정 단위 2개 미만, 또는 분산 0인 쌍은 '불충분'(검정 족에는 p=1로 남음) | `evaluate.py: effect_test` |
| 정답표 A | 발견 데이터에서 Holm 보정 후보 → 독립 확인 데이터에서 발견 방향 단측·δ 초과(Holm) | `shapeperf/evaluate.py` |
| 독립 확인 | 확인 데이터가 발견 데이터와 블록·G4 run(`g4_run_id`)·블록 순서 seed를 공유하거나 발견 역할로 표시돼 있으면 거부. `confirmation_requires_new_allocation`이 참이면 VM 할당도 달라야 함(할당 id가 없으면 공유로 간주). 고정 실행은 모든 기록에 G4 run id 필요. 확인 단계 할당 최소는 `min_allocations_confirmation` | `evaluate.py: check_independent` |
| 유효 집합 | 정답표·census·정책 비교 모두 G4 manifest(또는 `--valid`)의 **명시적** 길이 집합을 씀. 실패한 길이도 집합에 남아 인접 쌍이 실패 길이를 건너뛰지 않음 | `evaluate.py` |
| 확인 절차 기록 | 확인은 측정과 별개의 timeline 항목. 예산 초과 여부를 항목별로 판정 | `broker.py` |
| opt-report 출력 비용 | `report_ns`(G3에서 측정해 사전등록)를 성공한 전체 컴파일의 공통 비용에서 빼고(컴파일 시간 상한), 전체 컴파일 signature를 쓰는 정책(Compile-guided)에만 정책 추가 비용으로 부과. 재생·실시간 모두 같은 규칙 | `broker.py`, `backends.py` |
| 자연 길이 가중 영향(보조, 기술용) | `Σ_L freq(L)/N · (T(L)/min_{s≥L} T(s) − 1)`, T는 확인 데이터 per-shape 평균. 확인된 사건 경계를 넘는 몫을 따로 보고 | `evaluate.py: natural_weighted_impact` |
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
| `events.min_blocks_per_pair`, `min_allocations_per_pair`, `min_allocations_confirmation` | G3 블록 수·재할당 계획(확인 단계는 새 할당 1개가 보통) | 미정 |
| `events.bootstrap_reps_descriptive_ci` | 기술용 부트스트랩 CI 반복 수(검정에는 쓰지 않음) | 미정 |
| `events.confirmation_requires_new_allocation` | 확인 데이터를 다른 VM 할당에서 얻을지 | 미정 |
| `selectors.report_overhead_ns` | G3 `report_emission_overhead_ns` | 미정 |
| `events.match_tolerance_lengths` | ±0 또는 ±1 (§9.4) | 미정 |
| `selectors.random_seeds` | seed 수는 파일럿 분산으로 (§10.3) | 미정 |
| `selectors.shape_only_alignment_units` (+근거) | **G0에서 실제 opt-report로 확인한 대상 사실**만 사용 (§8.1). 관측된 급락에 맞춰 사후 선택 금지 | 미정 |
| `selectors.compile_probe_budget_fraction` | G3 probe : 측정 비용비 (§8.2b) | 미정 |
| `selectors.budgets_ns` | G3 비용 추정 | 미정 |
| `signature.probe_stage` | G3: probe 단계가 §6.2 feature를 모두 결정하는지, 최종 실행물과의 불일치율 | 미정 |
| `replication.features_per_event` (+근거) | 경계 재현(§4.3) 표본 수, 파일럿 분산으로 | 미정 |

### 정렬 단위 후보에 관한 사실 (결정 아님)

G0 실측(`results/g0/g0_verification.json`, 길이 128, 개발 환경)에서 확인한 사실:

- `--march=x86-64`: opt-report 기준 요소별·축약 연산 SIMD(보고 VL 32), Gemm(VL 16), MatMul(VL 8).
- `--march` 미지정 또는 `native`: 요소별·축약 연산은 SIMD 미적용(408건), Gemm/MatMul은 여전히 SIMD.
  (초기 소스 판독에서 "그 밖의 `--march`면 SIMD가 꺼진다"고 적었던 것은 부분적으로 틀렸음 — 요소별 연산에만 해당.)
- `--march`와 `--mcpu`를 함께 주면 ONNX-MLIR 자체 결정에서는 `--mcpu`를 무시한다고 경고하지만, LLVM 단계에는
  `--mcpu`가 전달되어 최종 `.so`는 AVX-512(zmm) 명령을 사용한다.
- 보고되는 VL은 컴파일러가 연산별로 출력한 SIMD 폭이며 대상 레지스터 폭과 같지 않다.

정렬 단위를 사전등록하려면 이 사실과 대상 VM에서의 같은 확인을 근거로 삼는다. 관측된 급락에 맞춰 고르지 않는다.

#### census 전 사전 기록 (2026-09-28, 개발 환경 census 실행 전에 커밋)

- **후보 정렬 단위: {8, 16, 32}** — 근거는 G0 opt-report에서 관측된 SIMD 보고 VL 값(MatMul 8, Gemm 16,
  요소별·축약 연산 32)뿐이다. 성능 데이터와 census 결과를 보기 전에 정했다.
- 최종 사전등록 값(`selectors.shape_only_alignment_units`)은 이 후보 안에서만 고르거나, 고르지 않을 경우
  "정렬 가정 무효(`[]`)"로 둔다. census 결과를 보고 이 후보 밖의 단위를 추가하지 않는다.
- 개발 환경 census는 `--units 8 16 32`를 분석용 단위로 보고하며, 어떤 단위도 사전등록되지 않은 상태에서는
  H1 판정을 내리지 않는다(`run_census.py`).

## C. 고정 절차

1. G3 파일럿 보고서를 근거로 B의 값을 JSON과 이 문서에 동시에 기입(근거 문장 포함).
2. `python scripts/freeze_prereg.py` → `frozen_sha256` 기록.
3. 이후 변경은 이탈(deviation)로 `report.md`에 기록하고 재고정.
