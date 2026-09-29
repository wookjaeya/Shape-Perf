# 설계 변경서 v3 (design amendment)

작성일: 2026-09-29 · 근거: `docs/research_plan_v3.md`(연구자가 제공한 "Shape-Perf 수정 연구·실험안", 원본 그대로 보존)

이 문서는 v2 연구(탐색기 비교, G2.5에서 중단)와 v3 연구(shape 특화 lowering 결정의 성능 손해와 원인)의 관계,
v3의 실행 규칙, 그리고 **이번 실행이 실제로 한 일과 하지 못한 일**을 기록한다. 계획서를 대체하지 않는다.
계획서와 다른 점은 8절에 모두 적는다.

## 1. 지위

| 항목 | 상태 |
|---|---|
| v2 탐색기 연구 | **중단 유지**. 결과·판정·데이터는 `main`의 `28382a9`에 그대로 있다(`report.md`, `docs/STATUS.md`, `results/g2_5/`, `preregistration.md`). 이 문서와 v3 작업은 그것을 고치지 않는다 |
| v2의 H1 중단 규칙("정렬 밖 signature 변화점 0이면 중단") | v2 탐색기 정책의 전제였다. v3 성립 조건으로 재사용하지 않는다(계획서 §2) |
| v3 | 재설계안. 이번 실행은 계획서의 **G0 → G1 → G2 중 개발 컨테이너에서 가능한 부분**이다. 본실험(G3 이후)은 사전등록과 측정 VM이 없어 실행하지 않았다 |
| 사전등록 | `protocol_v3.json`은 **초안**이다. 파일럿으로 정해야 하는 값은 모두 `null`이며 어떤 값도 임의로 채우지 않았다 |

## 2. 새 연구 질문과 대조

계획서 §1.1의 질문을 그대로 쓴다: 동일한 모델·길이·실제 입력에서, shape에 따라 선택되는 특정 MLIR lowering 결정이 다른 적법한
결정에 비해 성능 손해를 만드는 조건은 무엇인가.

- **주 대조(S8 대 S1)**: 같은 길이 L의 static 실행물 둘. 컴파일러 소스의 한 상수만 다르다.
  - S8: 고정 컴파일러 그대로. `scalarTransposeOverOutputs`의 unroll 상한 8
  - S1: 같은 컴파일러에서 그 함수의 상수만 1로 바꾼 빌드(`patches/transpose_unroll_cap1.patch`). 도우미 함수
    `getNoLeftoverUnrollFactor`는 건드리지 않아 다른 op의 unroll은 바뀌지 않는다
- **보조 대조(D8/D1, static 대 dynamic)**: 계획서 §9.2. 이번 실행에서는 만들지 않았다(8절)
- 효과 정의: `a(L,x) = log T_S8 − log T_S1`(계획서 §7 RQ1). 양수면 원래 정책이 더 느리다

가설(계획서 §6.2): 특정 shape 특화에서 선택되는 unroll이 후속 vectorization, register allocation, memory access 또는 code layout과
상호작용하여 성능 손해에 기여한다. 이 가설은 **검증 대상이며 이미 성립한 결론이 아니다**.

## 3. 소스로 확인한 사실 (고정 커밋 `1e017c9f`)

- `Transpose.cpp` `scalarTransposeOverOutputs`: 출력 마지막 차원 루프에 `const int64_t unroll = 8;`을 쓰고
  `getNoLeftoverUnrollFactor(lb, ub, unroll, tripCount)`를 부른다(patch 대상 줄은 1줄)
- `ONNXToKrnlCommon.cpp` `getNoLeftoverUnrollFactor`: trip count가 literal이 아니면 1, literal이면 `u = unrollFactor … 2`를 내려가며
  나누어떨어지는 첫 값, 없으면 1
- 따라서 `u(L) = max({1} ∪ {k ∈ 2..8 : L mod k = 0})`. 이 함수는 `shapeperf/lowering_diff.py:no_leftover_unroll`에 옮겼다
- 이 경로를 타는 것은 마지막 차원이 바뀌는 전치다. 모델 A에서는 K 전치(perm `[0,2,3,1]`) 12개와 마지막 출력 전치 1개, 모두 13개다.
  Q·V 전치(마지막 차원 유지)는 `blockTranspose`를 타므로 이 패치의 영향이 없다
- 위 규칙은 v2 census에서 관찰한 13개 루프와 부합한다. 다른 버전·모델·경로에도 적용된다는 뜻은 아니다

## 4. 실행 규칙 (이번 실행에 적용)

1. **컴파일러 변형은 바이너리 해시로 식별**한다. 모든 컴파일 기록에 `compiler_variant`, `compiler_bin_sha256`을 남긴다.
2. **원복 재현**: 패치 빌드 뒤 소스를 되돌려 다시 빌드한 바이너리의 SHA-256이 원본과 같아야 한다.
3. **실행 시작 시점의 provenance를 한 번 고정**한다(`shapeperf/provenance.py`): HEAD, 작업 트리 diff 해시, 미추적 파일 해시,
   의존성 목록 해시, 컴파일러 해시, 호스트. 모든 원자료 기록이 그 `provenance_id`를 가진다.
   `git_head()`도 프로세스당 한 번만 계산한다(v2 census는 실행 도중 라벨이 바뀌었다).
4. **정확성 게이트**: S8과 S1의 출력은 **비트 단위로 같아야** 한다(개입이 데이터 이동만 바꾸므로 허용치가 필요 없다).
   ORT와의 차이는 서술만 하고 판정하지 않는다(허용치 미등록). 사후에 허용치를 정하지 않는다(계획서 §11.5).
5. **음성 대조군**: 원래도 unroll이 없는 길이(`u(L)=1`)에서는 S8과 S1의 IR이 같아야 한다. 컴파일 산출물도 같으면 그 길이의
   시간 차이는 측정 경로의 잡음만이다.
6. **A/A 대조**: 같은 `.so`의 바이트 동일 사본을 별도 arm(AA)으로 측정해 가짜 차이를 점검한다.
7. **무작위화**: 길이 순서와, 블록 안의 arm·process 순서를 seed로 섞는다. 반복은 새 process, 추론 단위는 블록이다
   (반복 iteration을 독립 표본으로 세지 않는다).
8. **개발 컨테이너의 시간 값은 결과가 아니다**(계획서 §9.1). 4 vCPU KVM, SMT 없음, 할당 ID·provider 미확인 컨테이너다.
   시간 기록에는 `DEV-CONTAINER PILOT (not a result)` 라벨을 붙인다. 이 값은 측정 경로 점검, 잡음 크기와 필요 표본 수의 추정에만 쓴다.
9. 정렬·약수로 설명되는 현상도 실제 비용과 재현 가능한 원인 효과가 있으면 조사한다. 단순한 길이 산술로 충분한 설명을
   컴파일러 정보의 새로운 예측력으로 포장하지 않는다(계획서 §2).

## 5. 게이트와 이번 실행의 범위

| 게이트 | 계획서의 내용 | 이번 실행 |
|---|---|---|
| G0 동결·환경 복원 | 기존 결과 보존, 설계 변경서, provenance, 환경 정리, `check-mlir`, image digest | 6절 |
| G1 저비용 타당성 pilot | class별 lower median + 41, 256(≤16개 길이)에서 S8/S1: 정확성, 의도한 IR 차이, paired 측정, A/A, 분산·비용 | 패치 빌드, 정확성, IR 검사, A/A는 실행. **시간 측정은 개발 컨테이너 파일럿**(결과 아님) |
| G2 원인 분리 | 후보의 op 대응, 실제 activation subgraph 추출, S8/S1(D8/D1), 다른 VM 할당에서 재측정, 전체 모델과 연결 | G1 결과에 따라 결정. 다른 VM 할당·전체 모델 정밀 확인은 측정 VM이 필요해 **불가** |
| G3–G6 | 사전등록, 전 길이, 독립 확인, 전이, 최종 심사 | 실행하지 않음 |

## 6. G0 상세

- **보존**: v2의 상태는 `main`의 `28382a9`(squash)와 그 이전 개발 이력이다. 이 저장소의 어떤 v2 결과 파일도 이번에 수정하지 않았다.
- **환경 복원**: 결과는 `results/v3/g0/g0_restoration.json`에 있다(생성: `scripts/record_g0_v3.py`). 소스 트리 원복, 세 바이너리의 SHA-256, 패치 적용성, `check-mlir` 결과를 기록한다.
- **native 재현 절차**: 컨테이너 안에서 Docker를 쓸 수 없어 image digest를 만들지 못했다. 계획서 §G0-5에 따라 native 재현 절차를
  `docs/native_reproduction.md`에 고정한다.
- **`check-mlir`**: 8절에 수행 여부와 범위를 적는다.

## 7. `protocol_v3.json`의 구성

- **이미 정해진 값**: 툴체인 SHA, 두 컴파일러 바이너리의 SHA-256, 패치 SHA-256, 컴파일 옵션, 길이 선택 규칙과 그 산출 길이,
  입력 선택 규칙, process 요약(warmup 이후 산술 평균), 추론 단위(블록), 다중 비교(Holm, family-wise 0.05 — 연구자 선택),
  의미 게이트(ORT와 최종 답 동일 — 연구자 선택)
- **null(파일럿·예산으로 정해야 함)**: 허용치, warmup, 반복 수, process 수, 블록 수, VM 할당 수, CPU class, 최소 중요 효과,
  최대 실행 비용과 종료 규칙, held-out graph/revision
- `null`인 채로 실행한 것은 파일럿뿐이다.

## 8. 계획서와 다른 점, 미실행 항목 (이번 실행)

실행 결과(G0–G2)는 [`docs/STATUS_v3.md`](docs/STATUS_v3.md)에 있다. 여기에는 계획서와 **다르게 한 것, 못 한 것, 나중에 알게 된 약점**만 적는다.

### 8.1 못 한 것·하지 않은 것

| 항목 | 상태 | 이유·영향 |
|---|---|---|
| G3–G6 (사전등록, 전 길이 탐색, 독립 확인, 전이, 최종 심사) | 미실행 | 조사할 후보가 없고 측정 VM·사전등록이 없다. `protocol_v3.json`은 초안이며 `null`을 채우지 않았다 |
| 측정 VM에서의 timing | **불가** | 개발 컨테이너뿐이다. 이 컨테이너의 시간은 결과가 아니다(계획서 §9.1). 모든 시간 산출물에 `not_a_result`를 붙였다 |
| 다른 VM 할당에서의 재측정 (G2-5) | **불가** | 할당이 하나뿐이다. 후보가 사라지는지의 검사는 못 했다 |
| D8/D1 (정적 대 동적 L) 보조 대조 | 미실행 | 만들지 않았다. 주 게이트의 중단 사유는 아니다(계획서 G1) |
| Dockerfile image digest | 미실행 | 컨테이너 안에서 Docker를 쓸 수 없다. native 재현 절차를 `docs/native_reproduction.md`에 고정했다(계획서 G0-5) |
| `LiveBackend.confirm` (v2 기존 미구현) | 여전히 미구현 | 이번 실행은 `LiveBackend`를 쓰지 않았다. 구현 완료로 표시하지 않는다(계획서 G0-6). S8/S1 짝 측정은 `scripts/run_paired_benchmark.py`가 실제 실행물로 수행했다 |
| 전 범위 커널 스캔 | 중단 | 그 장치(프로세스 안 arm 짝짓기)가 로드 순서에 교락돼 있음을 알게 되어 결과를 쓰지 않았다. 다시 하려면 로드 순서를 무작위화한다 |
| 다른 모델·held-out 그래프(계획서 §8.5) | 미실행 | G5 이후 |

### 8.2 계획서와 다르게 한 것

- **스크립트 이름**: 계획서의 `extract_observed_subgraph.py`와 `run_lowering_intervention.py`는 `scripts/run_subgraph_benchmark.py`
  (빌드·측정·스캔)와 `shapeperf/paired.py`로 합쳤다. `analysis/mechanism_evidence.py`는 만들지 않았고, 그 역할(IR 차이, 최종 코드 확인)은
  `shapeperf/lowering_diff.py`, `scripts/collect_lowering_trace.py`, `analysis/subgraph_effects.py`가 맡는다. `artifacts/cases/<case_id>/`는
  확정한 사례가 없어 만들지 않았다.
- **G1 길이**: 계획서는 최대 16개. class별 lower median + {41, 256} 규칙을 적용하고 중복을 제거해 15개가 되었다(`g1_lengths.json`).
- **G2 순서**: 계획서의 G2는 G1의 후보에서 출발한다. 후보가 없어, 원인 진단을 위한 단일 op 실험(K형, 음성 대조 Q형)을 낮은 비용으로 추가했다.
  이는 G1 후보 없음을 뒤집지 않는다.
- **선별 규칙**(G1 후보 판정: 평균·median의 95% 구간이 같은 부호로 0을 제외하고 |평균| > A/A 블록 sd의 2배)은 파일럿의 길이별 표를 **본 뒤에** 적었다.
  가설 검정이 아니라 이 pilot의 선별용이며, 이후 자료에 적용하기 전에 고정했다.
- **컴파일러 변형 식별**은 경로가 아닌 바이너리 SHA-256이다. 원복 재빌드 바이너리가 저장해 둔 S8과 같음을 기록했다
  (`results/v3/g0/g0_restoration.json`).

### 8.3 실행 중·후에 알게 된 약점

- **로드 순서 교락(측정 장치)**: 프로세스에서 먼저 로드한 `.so`가 그 프로세스의 커널 타이밍 수준을 정한다(S8을 먼저 로드한 프로세스가 S1을 먼저 로드한 프로세스보다 L=63에서 +60%, L=64에서 +45%, L=96에서 +10%, 절대 +6~11 µs).
  arm과 교락되어 처음에는 낮은 L의 큰 효과로 보였고, 진단 후 **철회**했다. 파일 복사본 A/A는 이를 검출하지 못했다. 원인은 규명하지 못했다.
  로드 순서를 고정한 비교에서 S8/S1은 +1~+7%(여섯 조건, 모든 95% 구간이 0 포함)다. G1의 모델 전체 측정도 프로세스당 arm 하나를 쓰므로
  같은 종류의 교락을 배제하지 못한다. 모델 전체 효과가 관측되지 않았으므로 결론은 바뀌지 않지만, 이 설계를 측정 VM에서 그대로 쓰면 안 된다.
- **A/A 구간의 반보수성**: 같은 실행물끼리의 비교에서 15개 중 3개의 95% 구간이 0을 제외했다. 구간을 문자 그대로 읽지 않는다.
- **provenance**: G1의 배치 1·2는 작업 트리가 커밋되지 않은 상태(`dirty`)에서 실행했다. 코드 상태는 diff·미추적 파일 해시로 식별되지만 당시 diff 자체는
  저장하지 않았고, 이후 커밋된 코드와 다를 수 있다. 배치 3과 G2 측정은 커밋된 코드(`harness_commit`)에서 실행했다.
- **부하 통제**: 측정 구간에 컴파일·`check-mlir`·다른 빌드는 겹치지 않았다(로그 시각 기준, `check-mlir`은 마지막 측정 뒤 08:11에 시작). 그 밖의 부하(같은 호스트의
  이웃 포함)는 기록하지 않았고 할 수도 없다.
- **G0-4의 정리 항목**: 컴파일러 안의 ONNX 1.21.0과 Python 패키지 버전의 차이는 `docs/native_reproduction.md`에 기록했다(Python은 모델 읽기에만 사용).
  v2 `environment.lock.json`의 커널 항목(`6.18.44-fc-v42`)은 현재 컨테이너(`fc-v49`)와 달라 낡았다. v2 파일은 고치지 않았고, v3의 모든 측정은
  실행 시점의 호스트 정보를 provenance에 기록한다.
- **`check-mlir`**: 수행했다(`results/v3/g0/check-mlir.log`). 고정 LLVM의 MLIR 프로젝트 테스트이며 통과 3308, 미지원 582, 예상 실패 1, 실패 0,
  약 3.6분(빌드 포함, 테스트 62초). ONNX-MLIR 자체 lit 테스트는 v2의 `check-onnx-lit`이다.
- **모델 수준 상한의 성격**: '≲ 0.05%'는 단일 op 측정 × 12개 transpose ÷ 모델 지연의 산술 추정이지 모델 전체의 직접 관측이 아니다.
