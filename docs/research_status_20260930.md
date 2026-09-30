# Shape-Perf 연구·실험 현황 (2026-09-30)

- 저장소: `wookjaeya/Shape-Perf`, 작업 브랜치 `claude/busy-shannon-y903qc`
  - [PR #1](https://github.com/wookjaeya/Shape-Perf/pull/1): 병합됨, v2
  - [PR #2](https://github.com/wookjaeya/Shape-Perf/pull/2): draft, v3·후속·R1
- 환경: 클라우드 개발 컨테이너 1대
  - 4 vCPU Xeon(Emerald Rapids, AVX-512), RAM 15 GB, GPU 없음
  - 블록 단위 시간 잡음 sd 2–3%
- **이 문서의 모든 시간 수치는 개발 컨테이너 값이며 본 결과가 아니다.** 컴파일 산출물·정확성·정적 분석처럼 시간과 무관한 결과만 사실로 읽는다.
- 이 문서는 deep-research(연구 아이템 조사)와 별개다. 여기서는 **직접 수행한 실험**만 다룬다.

---

## 0. 한눈에 보기

| 단계 | 연구 질문 | 상태 | 핵심 결과 | 지위 |
|---|---|---|---|---|
| **v2** (명세 v2) | 컴파일 정보(opt-report signature)로 shape별 성능 급락 후보를 싸게 찾을 수 있는가 | G0–G2 완료, **G2.5에서 중단** | 216개 길이 전부 opt-report signature가 1종류. 탐색기에 쓸 정보가 없음 → 명세에 따라 중단 | 확정(정적 결과) |
| **v3** (재설계) | shape에 따라 선택되는 특정 lowering 결정(Transpose unroll)이 성능 손해를 만드는가 | G0–G2 수행, G3 이후 미실행 | IR·기계어 차이는 확인. 모델 수준 후보 0. 모델 시간 비중은 0.1–0.3%에 불과 | 개발 컨테이너, 판단 불가 |
| **후속 E0–E1** | v3 측정이 정말 서로 다른 두 구현을 비교했는가 | **완료** | 공동 로딩 비교는 **무효**(나중 모델이 먼저 모델의 코드를 실행). 단독 실행은 유효 | 확정(추적 증거) |
| **방향 탐색** | 시간은 어디에 쓰이는가 | 완료 | Gemm 56–66%, **Gelu 26–28%**. Gelu는 원소마다 scalar libm 호출. ORT보다 2.2–2.6배 느림 | 진단값 |
| **R1** | 컴파일러의 의도·보고와 최종 코드가 어긋나는가, 고치면 얼마나 회수되는가 | **수행**(측정 VM 확인 제외) | 13개 수학 op 중 9개가 원소마다 libm 호출. 좁은 수정 두 개로 호출 0. BERT −27%, Gelu 커널 −98.7%. opt-report는 이 차이를 전혀 반영하지 않음 | 정적 결과 확정, 시간은 개발 컨테이너 |

---

## 1. 흐름 요약

1. **v2**(09-28)는 "컴파일러 보고서의 변화점 = 성능 급락 후보"라는 가정으로 탐색기 비교 실험 환경을 만들었다.
   census에서 opt-report가 길이에 따라 전혀 변하지 않아, 명세의 중단 규칙대로 멈췄다.
2. **v3**(09-29)는 census에서 보인 약수 기반 Transpose unroll을 구체적인 원인 후보로 삼았다.
   컴파일러 상수 하나만 바꾼 대조(S8 대 S1)로 인과를 검증하려 했다. 모델 수준에서는 후보가 없었다.
   커널 수준의 큰 차이는 처음에 "측정 artifact"로 철회했다.
3. **후속 E0–E1**(09-30)에서 그 철회가 틀렸음을 밝혔다.
   무효였던 것은 두 모델을 한 프로세스에 올린 진단 쪽이다. 같은 tag의 모델 라이브러리가 먼저 로드된 쪽 계산 코드를 실행했다.
4. **방향 탐색**에서 BERT 시간의 약 27%가 Gelu의 scalar libm 호출임을 발견했다. shape보다 훨씬 큰 손실이다.
5. **R1**(09-30)에서 그 원인을 패턴 우선순위와 pow 생성 방식으로 좁혔다.
   좁은 수정 두 개로 회수 효과를 측정했고, 같은 문제가 수학 op 전반에 있음을 보였다.

---

## 2. 공통 환경과 원칙

| 항목 | 내용 |
|---|---|
| 툴체인 | ONNX-MLIR v0.5.1.1 `1e017c9f`, LLVM `1053047a`, protobuf v33.5, 모두 소스 빌드(LLVM 2시간 25분, ONNX-MLIR 43분). ONNX Runtime 1.23.2 |
| 모델·데이터 | BERT-SQuAD(`bertsquad-12`)와 길이 축을 가변으로 재수출한 `A_reexport`. SQuAD v1.1 dev(질문 10,570개, feature 12,006개, 유효 길이 41–256) |
| 컴파일러 변형 식별 | 경로가 아니라 **바이너리 SHA-256**으로 식별한다. 패치 빌드 뒤 원복 재빌드가 원본 해시(`e34bcd6a…`)와 같은지 매번 확인했다(v3 cap1, R1 모두 일치) |
| 측정 원칙 | 한 측정 프로세스에 모델 하나(E1 이후 필수), 블록 안 무작위 순서, 추론 단위는 블록, 반복 iteration을 독립 표본으로 세지 않음 |
| provenance | 실행마다 HEAD·diff 해시·미추적 파일 해시·의존성·컴파일러를 고정 기록한다. v3의 dirty 실행은 E0에서 해시 일치로 모두 복원했다 |
| 테스트 | 전체 148개 통과. 이 중 loader 회귀 테스트가 11개 |

---

## 3. v2: 컴파일 정보 기반 급락 탐색 (중단)

| 게이트 | 결과 | 증거 |
|---|---|---|
| G0 환경 고정 | 툴체인 소스 빌드 성공. `check-onnx-lit` 450개 중 304 통과·146 미지원·실패 0. 정적 shape 특화, matmul 경로(외부 BLAS 없음), signature 결정성 확인. 비결정적 노드 이름 접미사 문제를 찾아 `sig-v2/v3` 정규화로 해결 | `results/g0/` |
| G1 원본 재현 | 공식 전처리 재현. ORT로 원본@256 전체 dev **EM 80.6717 / F1 88.0716**. 모델 카드 EM 80.67171과 일치 | `results/g1/` |
| G2 shape 적합성 | 원본은 길이를 바꿀 수 없다(길이 256 상수 70개). 재수출본은 원본 대비 logits 차이 1.1e-5이고, feature별 길이로 실행해도 EM/F1 동일·답 변경 0 | `results/g2/` |
| G2.5 census | 216개 길이 전부 **opt-report signature 1종류** → 명세 §13-15에 따라 **G3 이후 중단**(연구자 결정). IR 구조는 215개 인접 경계 모두에서 바뀌었다. 대부분 4·8·32 주기(정렬)이고, 일부는 약수 기반 Transpose unroll | `results/g2_5/` |

- 독립 검토 4회를 거쳤다(다중 에이전트 반박 검증 포함). 1차 결함 9건·해석 문제 6건, 2차 26건, 3차 20건(+경미 27건), 4차 20건을 확인해 모두 고쳤다.
- 남은 것: G3–G7은 코드만 있고 실행하지 않았다. `LiveBackend.confirm`은 미구현이다.

---

## 4. v3: 특정 lowering 결정의 인과 검증

**대조**: S8은 고정 컴파일러, S1은 `scalarTransposeOverOutputs`의 unroll 상한만 8에서 1로 바꾼 컴파일러다. A/A는 S8의 바이트 복사본이다.
예측 규칙은 `u(L) = max({1} ∪ {k ∈ 2..8 : L mod k = 0})`이다.

| 게이트 | 결과 |
|---|---|
| G0 | 원복 재빌드가 원본 해시와 같다. `check-mlir` 통과 3308·미지원 582·실패 0. L=128 산출물 해시가 v2와 같다. Docker image digest는 못 만들어 native 재현 절차로 대체 |
| G1 pilot (15개 길이, 기록 450개) | IR 예측 **15/15** 일치(`u=1`인 41·149는 S8=S1 바이트 동일). S8과 S1 출력 **30/30 쌍 비트 동일**. 모델 전체 S8/S1 차이 평균 −0.12%(길이 간 sd 1.0%), **후보 0**. 반복 sd 4.3%, 프로세스 sd 2.5%, 블록 sd 2.3–3.0%. A/A도 15개 중 3개가 "유의" |
| G2 (단일 op) | 실제 activation으로 K형 transpose를 단독 측정했다. **L=63·64·65·96에서 S8이 12–45% 느렸다**(단독 실행). 다른 길이는 ≈0. 기계어는 S1이 명령 1.9–5.0배, gather 증가 |
| 모델 연결 | K형 transpose 12개가 모델 시간의 **0.09–0.30%**뿐이다. 커널 차이가 그대로라도 모델 효과는 ≲0.05%(산술 추정) |
| G3–G6 | 미실행(후보 없음, 측정 VM 없음) |

**v3의 결론(현재 판단)**: 이 결정은 IR과 기계어를 크게 바꾼다. 그러나 모델 시간 비중이 너무 작아 모델 수준 손해로 이어질 수 없다.
커널 수준 차이(단독 실행)는 **미확정 관측**이며, 원인(정책·배치·할당기)은 규명하지 않았다.

---

## 5. 후속 E0–E1: 실행 식별 (완료)

근거 문서: `docs/research_plan_v3_followup.md`. 결과: `docs/followup_v3_review.md`.

| 조건 | 판정 (L=64, 호출 단위) |
|---|---|
| 모델 하나만 올린 단독 실행 (S8, S1, AA) | **자기 코드 실행** 6/6 + AA 2/2 |
| 무태그 두 모델 공동 로딩 (두 순서 × 두 첫 호출 + 순차 로드 + v3 scan 순서) | 나중 로드 모델이 **먼저 로드된 모델의 wrapper와 계산 코드 실행**, 15/15. 호출 순서·lazy/BIND_NOW 무관 |
| 고유 tag(alpha/bravo, 교환 포함) 공동 로딩 | 자기 코드 56/56 |
| 같은 tag(진단) | 오결합 재현 2/2 |

- **원인**: 런타임의 `dlopen(RTLD_LAZY | RTLD_GLOBAL)`과, 파일명에서 온 같은 tag(`model`) 때문이다.
  entry는 자기 것이 실행되지만, entry가 PLT로 부르는 `_mlir_ciface_main_graph_model`은 먼저 로드된 정의에 결합된다.
- **선행 보고(09-30 확인)**: 이 부류의 문제는 2023년에 upstream에 보고됐다([onnx/onnx-mlir#2342](https://github.com/onnx/onnx-mlir/issues/2342)).
  [onnx/onnx-mlir#2381](https://github.com/onnx/onnx-mlir/pull/2381)이 `--tag`로 고쳤다. tag를 주지 않으면 확장자를 뺀 파일명이 tag가 된다.
  v3가 겪은 것은 그 **잔여 경우**다. 서로 다른 디렉터리의 `model.so` 둘은 같은 기본 tag를 갖는다.
  upstream main의 `ExecutionSession.cpp`도 `RTLD_GLOBAL`을 쓰고 중복 tag를 검사하지 않는다. 새 현상이 아니라 **알려진 문제의 잔여 경우**로 기록한다.
- **증거**: 네 가지 방법이 모든 호출에서 일치했다.
  - gdb: 실행 PC가 속한 DSO, 메모리의 함수 바이트 해시(ASLR 켬)
  - `LD_DEBUG=bindings`: lazy와 BIND_NOW 모두
  - 독립 에이전트의 추적기 없는 GOT 슬롯 읽기
  - `ud2` 트랩 행렬 48개 프로세스
- **v3 정정**:
  - 공동 로딩 진단(scan, `context_*`, `loadorder_*`)은 S8/S1 비교로서 무효다.
  - "커널 효과는 artifact"라던 철회를 철회한다.
  - G1은 기록 450개가 pid마다 모델 하나였으므로 이 문제와 무관하다.
- **E0**:
  - 기록되지 않았던 compile argv를 복원했고, 그 argv로 다시 컴파일하니 기록된 해시와 비트 단위로 같았다.
  - dirty 상태였던 v3 provenance는 모두 해시 일치로 코드 상태를 복원했다.
  - v3 측정 도구 전부의 프로세스 구성을 독립 감사로 확인했다.
- **재발 방지**:
  - 모델 심볼이 겹치면 로드 전에 거부하는 guard를 넣었다(`--tag=NONE`도 거부, 심볼을 읽을 수 없으면 거부).
  - 모든 worker가 tag를 런타임에 전달한다.
  - 회귀 테스트 11개를 추가했다.
- **E2 진행 가능**(단독 실행 경로). 단 protocol 동결(예산·목표 정밀도는 연구자 결정), 할당기 환경 고정, AA 해시 실측이 먼저다.
  할당기 설정만 바꿔도 단독 커널 시간이 70–90% 바뀌었기 때문이다.

---

## 6. 방향 탐색: 시간은 어디에 쓰이는가

| 항목 | 값 | 비고 |
|---|---|---|
| op별 시간 비중(계측 빌드, L=64/256) | Gemm 56–66%, **Gelu 26–28%**, MatMul 5–9%, Softmax 1–5%, Transpose 전체 0.3–0.7% | 두 프로세스(CPU 고정이 다름)의 계측을 섞은 요약(E0 감사에서 확인) |
| Gelu 최종 코드 | opt-report는 12개 모두 SIMD(VL16). 그러나 `tanhf` 192·`powf` 192 호출 지점(= 12 × 16 lane) | 계측 없는 빌드에서도 같음 |
| ORT 대비 | ORT 단일 스레드가 2.2배(L=64), 2.6배(L=256) 빠름 | 단순 시간 측정 |

이 결과로 연구 방향을 shape 특화에서 **lowering 충실도(R1)**로 옮기는 것을 제안했다.

---

## 7. R1: 의도–바이너리 충실도 (수행)

protocol은 측정 전에 고정했다(`results/r1/protocol_r1.json`). 결과는 `results/r1/`에 있다.

### 7.1 가설과 개입

- **원인 가설**: `ConvertKrnlToLLVM.cpp`가 다항 근사 패턴(legacy API, 우선순위 1)과 MathToLLVM의 `math.tanh → llvm.intr.tanh` 패턴(우선순위 1)을
  한 변환 단계에 함께 넣는다. 그 결과 소스 주석이 밝힌 근사 의도가 실현되지 않는다.
  또 Gelu의 x³이 `math.pow`로 생성되어 libm `powf`가 된다.
- **개입**:

| 변형 | 내용 | 컴파일러 SHA-256 |
|---|---|---|
| orig | 고정 컴파일러 | `e34bcd6a…` |
| r1a | **I1**: 같은 op 집합의 근사 패턴을 우선순위 2로(`patches/r1_i1_math_approx_benefit.patch`) | `a78f5b94…` |
| r1b | **I1 + I2**: 추가로 Gelu x³을 `x*(x*x)`로(`patches/r1_i2_gelu_pow_to_mul.patch`) | `366f4035…` |
| r1c | **I2**만 | `bc26be31…` |

세 변형 모두 소스 원복 후 재빌드한 컴파일러가 원본 해시와 같음을 확인했다.

### 7.2 정적 결과 (결정적): 13개 수학 op 단일 연산

입력 [1, 64, 3072] float32, `-O3 --march=x86-64 --mcpu=emeraldrapids`. 표의 숫자는 최종 `.so`의 libm 호출 지점 수다.

| op | opt-report 주장 (orig) | orig | r1a | r1b | r1c |
|---|---|---|---|---|---|
| Gelu(tanh) | **SIMD, VL16** | tanhf 32, powf 32 | powf 32 | **0** | tanhf 32 |
| Sigmoid | **SIMD, VL32** | expf 64 | 0 | 0 | expf 64 |
| Tanh | SIMD 안 함 | tanhf 48 | 0 | 0 | tanhf 48 |
| Exp | SIMD 안 함 | expf 48 | 0 | 0 | expf 48 |
| Log | SIMD 안 함 | logf 48 | 0 | 0 | logf 48 |
| Sin / Cos | SIMD 안 함 | sinf / cosf 48 | 0 | 0 | 48 |
| Softmax | unsupported | expf 16 | 0 | 0 | expf 16 |
| Atan | SIMD 안 함 | atanf 16 | atanf 16 | atanf 16 | atanf 16 |
| Erf, Gelu(none), Pow³, Sqrt | Gelu(none)·Sqrt·Pow³(Mul)은 SIMD, Erf는 SIMD 안 함 | 0 | 0 | 0 | 0 |

- **13개 중 9개 op에서 원소마다 scalar libm 호출**이 최종 코드에 남는다. 불일치는 두 종류다.
  - **(a) 보고–바이너리 불일치**: Gelu(tanh), Sigmoid. opt-report가 SIMD라고 하는데 최종 코드는 원소마다 libm을 부른다.
  - **(b) 의도–바이너리 불일치**: Tanh, Exp, Log, Sin, Cos, Softmax. 보고서는 SIMD를 주장하지 않지만, 소스 주석의 근사 의도("tanh, sin, cos, exp")가 실현되지 않는다.
- I1은 9개 중 8개를 없앤다. I2는 Gelu의 `powf`만 없앤다. 단독 Pow³은 원래 libm 호출이 없다.
- **기전은 세 가지다**(고정 소스로 확인):
  1. **패턴 선점**(tanh, exp, log, sin, cos와 이를 쓰는 sigmoid·softmax·Gelu): MathToLLVM에 intrinsic 변환이 있는 op는 우선순위가 같은 근사 패턴을 이긴다.
     **자연 대조군**이 있다. Erf는 MathToLLVM에 변환 패턴이 **없어** 경쟁 없이 원래부터 근사되고, libm 호출이 0이다.
     tanh의 intrinsic 패턴은 LLVM PR #125753(2025-02)이 추가했다(deep-research 3-0 확인). 그 전에는 tanh도 erf처럼 근사됐을 가능성이 크다(추론, 이전 LLVM 빌드로 확인 가능).
     upstream MLIR은 이 우선순위를 정하는 benefit 인자를 PR #130782로 제공했지만, ONNX-MLIR은 main에서도 benefit 없는 옛 API를 쓴다(코드 비교, main 빌드는 안 함).
  2. **생성 방식**(Gelu의 x³): `math.pow`로 생성되어 libm `powf`가 된다. SIMD 비용 모델에는 pow 항목이 없다.
  3. **방언 우회**(Atan): ONNX-MLIR이 Atan을 `math.atan`이 아니라 `KrnlAtanOp`로 내리고, 이것이 곧바로 libm `atanf` 호출이 된다(`KrnlUnaryMath.cpp`).
     그래서 근사 패턴이 볼 기회가 없고, I1으로도 바뀌지 않는다.

**BERT 전체(L=256)**:

| | orig | r1a | r1b | r1c |
|---|---|---|---|---|
| libm 호출 지점 | tanhf 192, powf 192, expf 96 | powf 192 | **0** | tanhf 192, expf 96 |
| opt-report(노드 이름 제외) | 582줄 | **4개 빌드 모두 동일** | | |

opt-report는 네 빌드의 차이를 **전혀 반영하지 않는다**. 보고서만으로는 이 결함도, 수정도 볼 수 없다.

### 7.3 수치 정확도

float64 참조와 비교한 최대 절대 오차다. 비교를 위해 ORT 값을 괄호로 붙였다.
- tanh: 2.4e-7, 4 ULP (ORT 2.6e-7)
- exp·log·sigmoid: 1 ULP 안팎
- Gelu(tanh): 7.8e-7 (ORT 7.8e-7)
- **sin·cos**: 2.7e-6. libm 3e-8보다 약 80배 크다(입력 ±50).
  근사 우선순위를 **전역으로** 올리면 정확도 비용이 생기는 op가 있다. 수정은 op별로 선택해야 한다.

### 7.4 효과 (개발 컨테이너, 결과 아님; 5블록, 블록 안 무작위 순서, 프로세스당 모델 하나)

| 대상 | orig | r1a | r1b | r1c |
|---|---|---|---|---|
| Gelu(tanh) 커널 | 6,257 µs | −65.6% [−67.6, −63.5] | **−98.7%** [−98.8, −98.6] (81 µs) | −34.0% [−36.5, −31.5] |
| Tanh 커널 | 3,312 µs | −97.8% | −97.7% | +1.9% (0 포함) |
| Exp 커널 | 623 µs | −83.3% | −82.6% | +0.2% (0 포함) |
| Sigmoid 커널 | 739 µs | −89.3% | −89.1% | +2.0% (0 포함) |
| **BERT 전체 L=256** | 986 ms | −17.4% [−21.0, −13.5] | **−27.4%** [−30.8, −23.9] (716 ms) | −9.9% [−12.2, −7.5] |

- 효과가 잡음(2–3%)보다 수십 배 커서 방향은 이 환경에서도 분명하다. 정식 크기는 측정 VM에서 확인해야 한다.
- r1c의 BERT −9.9%는 protocol의 해석 기준(|효과| ≥ 10%)에 조금 못 미친다. 해석하지 않는다.
- r1c는 pow가 없는 op(Tanh, Exp, Sigmoid)에서 효과가 없다. 이것이 음성 대조 역할을 한다.
- ORT(397 ms) 대비 격차는 2.5배에서 **1.8배**로 줄었다. 남은 격차의 대부분은 Gemm으로 보인다.

### 7.5 정확도 게이트 (전체 SQuAD dev, L=256)

<!-- R1_SQUAD -->
**미완료(07:12Z 중단, 연구 방향 전환)**. 판정 기준은 protocol에 고정된 대로 답 변경 ≤ 10개, |ΔEM|·|ΔF1| ≤ 0.1이다.
- orig: feature 12,006개 중 10,000개의 logits를 저장했다(`results/r1/squad/orig/shard*_part*.npz`, git 추적 제외).
- r1b: 시작하지 않았다.
- 같은 명령으로 다시 실행하면 남은 feature만 이어서 계산한다(`scripts/r1_squad_eval.py`, 드라이버 `/home/user/work/r1_squad_driver.sh`).
- 따라서 **H3의 "정확도를 해치지 않는다" 부분은 아직 판정되지 않았다.** 단일 연산 수치(7.3절)만 있다.

### 7.6 R1의 현재 판단

| 질문 | 판정 | 근거 |
|---|---|---|
| H1 원인(패턴 선점, pow 생성, 방언 우회) | **지지** | 우선순위만 바꾸면 tanh·exp·log·sin·cos의 libm 호출이 사라지고, pow만 바꾸면 powf가 사라진다(요인별로 분리됨). 경쟁 패턴이 없는 Erf는 원래부터 근사된다. Atan은 우회 경로라 I1에 반응하지 않는다 |
| H2 결정적 탐지 | **지지**(ONNX-MLIR 안에서) | 호출 지점 수와 opt-report 대조만으로 9개 op 식별. 시간 측정 불필요 |
| H3 효과 | 지지(개발 컨테이너), 정확도 게이트 미완료 | BERT −27%, Gelu 커널 77배. 측정 VM 확인과 SQuAD 게이트가 남음 |
| 일반성 | 부분 | 한 컴파일러 안의 여러 op로는 일반적이다. 다른 컴파일러(IREE 등)와 LLVM 버전 간 drift는 미확인 |

---

## 8. 정정 이력 (무엇을 말했다가 고쳤는가)

| 시점 | 처음 주장 | 정정 | 근거 |
|---|---|---|---|
| v2 G0 | "`--march`가 x86-64가 아니면 SIMD가 꺼진다" | 요소별 연산에만 맞다. Gemm/MatMul은 SIMD | opt-report 실측 |
| v3 G2 | "낮은 L 커널 효과는 측정 artifact" | **틀림**. 무효는 공동 로딩 진단 쪽이다 | E1 추적 |
| v3 G2 | "로드 순서 고정 시 S8/S1 +1~+7%" | 무효(두 arm이 같은 코드 실행) | E1 |
| v3 G2 | "G1도 같은 교락을 배제 못한다" | 철회(pid마다 모델 하나) | E0 감사 |
| v3 STATUS | "zmm은 S8에만" | L=96의 S8은 zmm 0개 | objdump |
| 방향 탐색 | op 비중 요약 | 두 프로세스 계측을 섞은 요약이었음 | E0 감사 |
| R1 노트 | "MLIR의 성능 결함 탐지는 빈 영역" | MLIR-Smith(2026)가 missed optimization을 다룸 → 신규성을 좁힘 | deep-research 반박 |
| R1 노트 | "remark 대 바이너리 검사기는 신규 공헌" | 근거 불충분(0-3 반박). 신규성 근거에서 뺌 | deep-research 교차 검증 |
| R1 노트 | "x86 AVX-512 tanh의 해법은 MLIR 근사뿐" | AMD libm 표에는 VF16 tanh 행이 있다. glibc·SVML 환경에 한정한 말이다 | deep-research 교차 검증 |
| R2 노트 | 공동 로딩 교차 실행을 연구 2순위로 둠 | upstream에 2023년 보고·수정(#2342, #2381)된 부류의 잔여 경우 → 3순위 | 이슈·PR·main 소스 직접 확인 |
| R1 실험 전 | "13개 수학 op가 모두 SIMD로 보고" | SIMD 주장은 Gelu(tanh)·Sigmoid 등 일부. 나머지는 "SIMD 안 함"인데도 근사 의도가 미실현 | R1 스캔 |
| R1 protocol | 고정 시각 "05:50Z" | 추정값이었음. 실제 파일 작성 시각은 05:37:19Z(첫 모델 컴파일 05:39:10Z보다 앞섬) | 파일 mtime |

---

## 9. 열린 결정과 다음 단계

| 항목 | 필요한 것 | 결정 주체 |
|---|---|---|
| R1 확장 | 다른 컴파일러(IREE), LLVM 버전 간 drift(LLVM 재빌드 2.5시간), Krnl 우회 op 전수 조사, op별 선택적 근사 정책(sin·cos 정확도) | 연구자 |
| R1 upstream | ONNX-MLIR에 패치 제안(op별 우선순위, pow 전개). 정확도 게이트 결과가 전제 | 연구자 |
| E2(v3 후속) | protocol 동결: 예산, 목표 정밀도, 할당기 환경 | 연구자 |
| 측정 VM | 모든 시간 결과의 정식 확인 | 연구자(확보 여부) |
| 연구 방향 | 사용자 결정(09-30): **MLIR 패스 개발로 전환**. 이유: 실제 AI 추론은 주로 GPU에서 돌고, R1의 효과 크기는 CPU 경로에만 해당한다 | 연구자 |
| deep-research 검증 | **완료**: 주장 25건 중 확인 17, 반박 8. R1 원인 사슬은 모두 확인됐다. 결과는 연구 아이템 노트(저장소 밖, 별도 전달) 9절에 있다 | — |

---

## 10. 산출물 위치

| 단계 | 문서 | 결과·코드 |
|---|---|---|
| v2 | `docs/STATUS.md`, `report.md` | `results/g0`, `g1`, `g2`, `g2_5` |
| v3 | `docs/STATUS_v3.md`, `design_amendment.md`, `docs/research_plan_v3.md` | `results/v3/` |
| 후속 E0–E1 | `docs/followup_v3_review.md`, `docs/research_plan_v3_followup.md` | `results/v3_followup/`, `scripts/verify_execution_identity.py`, `shapeperf/identity.py` |
| 방향 탐색 | — | `results/v3/direction_probe/` |
| R1 | 이 문서 7절 | `results/r1/`, `patches/r1_*.patch`, `scripts/r1_*.py`, `scripts/build_r1_variants.sh` |
