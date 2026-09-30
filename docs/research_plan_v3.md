# Shape-Perf 수정 연구·실험안

## MLIR lowering 결정의 shape별 성능 손해와 원인 분석

**작성일:** 2026-09-29 (Asia/Seoul)  
**상태:** 연구 재설계안. 신규 실험 미실행, 본실험 사전등록 미완료.  
**근거:** 사용자 제공 `Shape-Perf_작업_종합보고서.md`, 고정 버전 ONNX-MLIR 소스, 아래 1차 문헌.  
**범위:** 클라우드 CPU에서 ONNX-MLIR로 컴파일한 FP32 Transformer 추론. 초기 실험은 batch 1·단일 스레드.  
**제외:** 이 문서는 대화용 쉬운 설명을 포함하지 않는다.

---

## 1. 연구 추진 판단

### 1.1 권고

**기존의 SIMD signature 변화점 기반 탐색기 연구는 중단 상태를 유지하고, 제한된 타당성 실험으로 새로운 연구 질문을 검증한다.** 대규모 탐색기 비교와 자동화 확대는 보류한다.

새로운 주 연구 질문은 다음과 같다.

> 동일한 모델·길이·실제 입력에서, shape에 따라 선택되는 특정 MLIR lowering 결정이 다른 적법한 결정에 비해 성능 손해를 만드는 조건은 무엇인가? 이를 해당 결정에 대한 국소 개입과 후속 코드 생성 추적으로 검증하고, 실제 모델 영향과 적용 범위를 설명할 수 있는가?

주 대조는 동일 길이의 원래 static 실행물과 **의심되는 lowering 정책 하나만 변경한 static 실행물**이다. 첫 후보는 소스로 확인한 Transpose unroll이다. batch, dtype, weights, 입력 값, target, 나머지 컴파일 설정은 통제한다.

static/dynamic 비교는 보조 분석이다. 여기서 dynamic은 런타임 JIT가 아니라 **길이 L만 동적으로 유지한 AOT 실행물**이다. 모델 전체의 dynamic 실행은 여러 최적화 경로를 동시에 바꾸므로, 그 차이만으로 특정 lowering의 원인을 판정하지 않는다.

**현재 판단:** 실험 가능성은 있으나, 논문 수준의 독창성과 실용적 필요성은 아직 입증되지 않았다. 제안은 성립 여부를 검증할 연구 계획이며, 성능 역전이 이미 존재한다는 결론이 아니다.

### 1.2 연구로 부족한 결과와 필요한 결과

| 얻은 결과 | 엄격한 해석 | 후속 판단 |
|---|---|---|
| 길이별 latency 곡선만 작성 | 통상적인 벤치마킹 | 독립 연구 공헌으로 부족 |
| static과 dynamic의 평균 속도만 비교 | 기존 성능 비교 방법의 적용 | 단독 공헌으로 부족 |
| IR hash 또는 instruction 수가 다름 | 코드 생성 차이 관측 | 성능 손해나 원인 증거가 아님 |
| SIMD 보고서에 unroll 정보가 없음 | 보고서가 제공하는 정보 범위의 한계 | 문서화된 SIMD 보고서의 결함이라고 단정할 수 없음 |
| 특정 길이에서 재현되는 성능 역전 발견 | 유용한 사례 또는 결함 후보 | 원인·정확성·영향 확인 필요 |
| 동일 의미의 통제 실행물에서 원인 결정의 효과 확인 | 원인에 관한 실험적 증거 | 본실험으로 확장할 근거 |
| 실제 모델 영향, 독립 재현, 조건을 설명하는 규칙 확보 | 실증 연구 공헌 후보 | 문헌 중복과 일반화 범위를 재심사 |

한 모델의 한 결함을 찾았다는 사실만으로 충분한 논문 기여를 주장하지 않는다. 반대로 새 컴파일러 패스가 없다는 이유만으로 연구가 성립하지 않는 것도 아니다. **재현 가능한 현상, 원인 검증, 적용 범위, 재사용 가능한 실험 증거**가 핵심이다.

## 2. 기존 결과의 재해석

현재 상태에 관한 수치는 제공 보고서 [P0]의 기록이다. 이번 재설계에서 저장소 코드나 원시 측정 파일을 독립적으로 재실행한 결과가 아니다.

| 기존 증거 | 허용되는 결론 | 허용되지 않는 결론 |
|---|---|---|
| A_reexport, L=41…256, 216개 길이에서 primary SIMD signature 1종 | 해당 signature가 길이별 후보 우선순위를 제공하지 못함 | 모든 컴파일 정보가 쓸모없음 |
| 인접 215개 경계에서 structural IR signature 모두 변경 | 이진 변화 여부만으로는 경계를 선별하기 어려움 | structural class나 특징값에 정보가 전혀 없음 |
| structural IR class는 14종 | 반복되는 구조적 유형이 있음 | 같은 class의 실행시간이나 의미가 같음 |
| Transpose의 길이 약수 기반 unroll 패턴 | 조사할 구체적인 lowering 메커니즘이 있음 | 그 unroll이 성능 악화의 원인임 |
| L=151/152 최종 정적 instruction 수 차이 | 생성 코드가 다름 | 실행된 instruction 수 또는 실행시간 차이가 그만큼 발생 |
| 개발 컨테이너에서 약 641/628 ms | 기능 확인용 관측 | 유효한 성능 차이, 효과 크기, 사건 존재 |
| ORT에서 재수출·padding 정확성 확인 | 재수출 모델 활용의 근거 | 컴파일된 모든 static/dynamic 실행물의 정확성 보증 |
| ONNX-MLIR 비교는 L=128의 9 features, tolerance 미고정 | 제한된 수치 비교 기록 | 전 길이 정확도 게이트 통과 |

기존 H1의 “정렬 경계 밖 signature 변화가 없으면 중단”은 특정 탐색 정책의 전제였다. 새로운 원인 분석 연구의 성립 조건으로 재사용하지 않는다. 정렬이나 약수로 설명되는 현상도 실제 비용과 재현 가능한 원인 효과가 있으면 조사 가치가 있다. 다만 **단순한 길이 산술로 충분한 설명을 컴파일러 정보의 새로운 예측력으로 포장하지 않는다.**

기존 census는 이후 설계에 사용했으므로 개발 자료이다. 같은 데이터에서 특징을 만들고 같은 데이터의 성능을 이용해 유효성을 주장하는 것을 피한다. 새 가설·선택 규칙·확인 자료를 구분한다.

## 3. 연구의 필요성: 어떤 의사결정을 개선하는가

### 3.1 실제 의사결정

대상 사용자는 ONNX-MLIR 기반 모델 배포자와 컴파일러 개발자이다.

1. **개발자:** shape 정보가 추가되면서 선택되는 lowering이 특정 조건에서 손해를 만드는지 검증한다. 재현 가능한 입력, IR, 국소 개입이 있어야 수정 여부를 판단할 수 있다.
2. **배포자:** compiler revision 또는 검증된 lowering 정책의 선택이 실제 모델에 미치는 영향을 판단한다. static/dynamic 보조 분석까지 수행한 경우에만 길이별 실행물 생성의 실익을 논한다. 실행시간뿐 아니라 추가 컴파일 시간과 저장 용량도 관련된다.
3. **성능 테스트 담당자:** 어떤 shape·lowering 조합을 회귀 테스트에 포함해야 하는지 결정한다. 발견 후 사례를 보관하는 것과 일반적인 선택 알고리즘을 제안하는 것은 별도 공헌이다.

ONNX-MLIR 공식 문서는 compile-time shape 고정이 shape 계산의 상수화와 더 정적인 코드 생성을 가능하게 한다고 설명한다 [R1]. 이것이 모든 입력에서 더 빠름을 보장하지는 않는다. 컴파일러에 추가 정보를 제공해도 최적화 결과가 나빠질 수 있다는 문제 자체는 이미 연구되었다 [R6]. **본 연구의 필요성은 그 일반 명제를 반복하는 데 있지 않고, 실제 tensor shape 특화의 실패 조건과 수정 가능한 원인을 확인하는 데 있다.**

### 3.2 필요성을 반증할 조건

- 재현 가능한 성능 역전이 없고, 측정 정밀도로 유의미한 손해 가능성도 충분히 제한된다.
- 역전이 단일 불안정 VM·계측 코드·잘못된 입력·잘못된 target 설정에만 나타난다.
- 추출 kernel에서만 차이가 있고 모델 실행시간에는 적용 가능한 영향이 없다. 이 경우 kernel 연구로 범위를 축소해야 하며 데이터센터 모델 효과를 주장할 수 없다.
- 현상이 기존 문헌이나 upstream issue에서 동일한 조건과 원인으로 이미 충분히 설명되어 있다.
- MLIR 관찰 없이도 같은 원인과 수정 조건을 얻을 수 있고, MLIR 분석의 추가 진단 가치가 없다.

실험 비용 절감, 운영 latency, 수정 가능한 결함 중 어느 것에도 연결되지 않으면 현상 수집만을 위해 프로젝트를 확대하지 않는다.

### 3.3 최초 문제의식과의 범위 차이

본 안은 AI 컴파일러의 CPU 추론 연구이다. 단일 스레드 결과는 대규모 병렬·GPU 데이터센터 성능의 증거가 아니다. 원인 확인 후 병렬 실행이나 다른 target으로 확장할 수 있지만, 그런 확장을 초기 공헌으로 선반영하지 않는다.

## 4. 선행연구와 차별성 심사

확인일은 2026-09-29이다. 아래는 가까운 관련 연구를 대상으로 한 집중 검토이며, 전체 분야를 빠짐없이 조사했다는 주장이 아니다. 제목·초록·공개 방법 설명으로 확인한 범위와 구현 재현 여부를 구분한다. 이 문서 작성 중 해당 연구 도구를 실행하지 않았다.

| 선행연구·기능 | 이미 알려진 내용 | 본 연구와의 중복 | 남을 수 있는 차이 / 요구 증거 |
|---|---|---|---|
| ONNX-MLIR Performance Testing [R3] | op·shape별 profiling, SIMD/parallel 보고와 runtime 연결 | 단순 shape profiling과 보고서 연계 | 국소 lowering 개입으로 검증한 성능 역전 원인과 전이 조건이 필요 |
| Refined Input, Degraded Output, PLDI 2024 [R6] | 추가 프로그램 정보가 최적화를 악화시키는 현상을 검사 | static 정보가 많아도 느려질 수 있다는 핵심 동기와 매우 가까움 | tensor shape 특화, 실제 모델, 다단계 IR의 구체적 메커니즘을 새로 밝혀야 함. 분야 이름 변경만으로 부족 |
| Jittery, OOPSLA 2026 [R7] | 설정·버전 간 성능 차등 테스트, 단계별 측정, 후보 우선순위 | 두 설정 비교 및 저비용 탐색→엄격 확인 | 방법 자체의 최초성을 주장하지 않음. AOT tensor lowering의 원인 증거가 필요 |
| DietCode, MLSys 2022 [R8] | dynamic shape를 위한 공통 탐색 공간·비용 모델·자동 스케줄링 | 여러 shape를 함께 다루고 컴파일·탐색 비용을 줄이는 동기 | 본 안은 새 auto-scheduler가 아님. 특화 실패의 진단과 검증이 목표 |
| DISC [R9] | MLIR 기반 dynamic-shape 컴파일과 최적화 | dynamic shape의 MLIR 표현·효율적 실행 | dynamic 지원 자체를 공헌으로 주장하지 않음 |
| MLIRTracer, FSE 2025 [R10] | MLIR 계층을 활용한 directed testing | 계층 구조를 활용한 테스트라는 큰 방향 | 정확성·크래시 중심 테스트와 구별되는 latency 효과 및 인과 개입이 필요 |
| DuoReduce, FSE 2025 [R11] | MLIR 프로그램과 pass 경로를 함께 축소하는 결함 격리 | 다단계 compilation의 원인 격리 | IR 축소 자체는 기존 기술. 성능 현상을 유지하는 축소의 타당성을 별도로 입증 |
| CLower, OOPSLA 2026 [R12] | redundant memory access를 통한 compiler pessimization 검사 | 생성 코드의 정적 특징으로 문제를 찾는 접근 | 정적 instruction 수 차이를 독자적인 oracle로 주장하지 않음. 실제 runtime 확인 필요 |
| From Roofline to Ruggedness, arXiv 2026 v2 [R13] | GEMM shape 성능 표면과 tile 관련 불연속의 분석·완화 | 길이에 따른 성능 요철·원인 분석 | 본 안의 동일 입력 static/dynamic 대조와 MLIR lowering 개입을 분명히 해야 함. 해당 자료는 preprint로 구분 |
| NNSmith, ASPLOS 2023 [R14] | 유효한 ML graph 생성과 differential correctness testing | 컴파일러 테스트·모델 생성 | 임의 graph 생성을 추가한다고 자동으로 성능 연구 기여가 생기지 않음 |

### 4.1 제안하는 공헌 — 아직 달성되지 않음

**C1. 실제 workload에서의 shape 의존 lowering 손해에 관한 실증 증거**

- 동일 입력·동일 의미의 원래 lowering/국소 변경 대조 자료와, 지원되는 경우 static/dynamic 보조 자료.
- 전 길이 결과와 실패 길이를 포함한 분모, 독립 확인, 효과 크기와 불확실성.
- 원래 모델에서 추출한 작은 사례와 전체 모델 효과의 연결.

**C2. MLIR lowering 결정의 원인 효과와 관찰 가능 범위**

- high-level op → lowering 결정 → 후속 IR → 실행물 → runtime 효과의 증거 사슬.
- 좁은 소스/IR 개입과 원복, 정확성 검증, 음성 대조군.
- SIMD 보고·구조적 IR·최종 코드 중 어느 수준에서 해당 결정을 확인할 수 있는지 명시.
- 보고서의 목적상 제공하지 않는 정보와 실제 보고 오류를 구분.

**C3. 원인 조건의 재현 및 회귀 테스트 artifact**

- 개발 사례에서 얻은 조건을 새로운 graph 또는 compiler revision에서 사전에 고정해 검증.
- 재현 실패 사례까지 공개하고 적용 범위를 제한.
- 원인별 최소 재현물과 수치·성능 검증 절차 제공.

**주장하지 않을 것:** 최초의 성능 차등 테스트, 최초의 shape profiling, 최초의 MLIR 원인 격리, 범용 새 cost model, 모든 컴파일러·CPU에 적용되는 최적화 규칙.

### 4.2 논문 공헌으로 인정할 자체 심사 기준

다음 질문에 모두 근거 있는 답이 필요하다.

1. 기존 문헌이나 issue의 재현을 넘어 새로 확인한 사실은 무엇인가?
2. 성능 차이를 만든 개입은 무엇이며, 다른 원인을 어떻게 통제했는가?
3. 전체 모델이나 개발자의 수정 판단에 어떤 영향을 주는가?
4. 동일 BERT layer 12개를 독립 사례 12개로 세지 않고도 일반화 근거가 있는가?
5. MLIR 분석을 제거하면 잃는 설명 또는 검증 능력은 무엇인가?

기준을 만족하지 않으면 결과를 사례 보고·artifact·upstream 재현 자료로 정리한다. 소규모 부정 결과를 논문으로 만들기 위해 주장 범위를 키우지 않는다.

### 4.3 이 안에 대한 가장 강한 반론

| 반론 | 검토 결과 |
|---|---|
| loop unrolling의 비용·이득은 이미 알려져 있다 | 맞다. “unroll이 때로 느리다”는 사실이나 cap 8→1 실험 자체는 새 기여가 아니다. 실제 tensor lowering에서 기존 설명으로 충분하지 않은 상호작용·실패 조건·수정 판단의 증거가 필요하다. |
| ONNX-MLIR의 한 heuristic에만 해당하는 엔지니어링이다 | 그럴 가능성이 있다. 한 구현의 한 사례라면 그 범위를 인정한다. 일반화가 없는데 MLIR 전체 문제라고 주장하지 않는다. |
| cap 1이 빠른 길이만 골랐다 | 전 길이 결과, 불리한 길이, 실패를 포함하고 새로운 자료에서 확인한다. cap 1을 새 최적 정책으로 제안하지 않는다. |
| SIMD 보고서에 unroll이 안 나오는 것은 당연하다 | 맞다. 보고서의 누락 자체는 새로운 결함이 아니다. 특정 진단에 어떤 수준의 정보가 필요한지를 검증해야 한다. |
| static/dynamic 비교는 원인을 너무 많이 바꾼다 | 맞다. 그래서 S8/S1을 주 대조로 삼고 static/dynamic을 보조 분석으로 내렸다. |

**이 반론들을 현재 증거만으로 모두 해소할 수는 없다.** 따라서 본 안에 대한 권고는 “논문으로 충분하니 진행”이 아니라 “작은 원인 검증 실험에 투자할 근거가 있다”이다. 새 상호작용이나 유용한 적용 조건이 남지 않으면 논문화를 위한 추가 개발을 중단한다.

## 5. MLIR의 핵심 역할

### 5.1 연구 단위

MLIR을 통한 관찰 단위는 파일 hash가 아니라 **특정 op에서 선택된 구조적 결정**이다. MLIR pass instrumentation과 IR 출력 기능은 이미 제공된다 [R4]. 새로운 최적화 패스를 구현할 필요는 없다.

| 관찰 수준 | 수집할 내용 | 답할 수 있는 질문 |
|---|---|---|
| ONNX dialect | static/dynamic dimension, op와 permutation, shape 계산 | 두 실행물이 같은 연산을 표현하며 어느 shape 정보만 다른가? |
| ONNX→Krnl lowering | loop bound, blocking/unroll, memcpy/view/elementwise 경로 | shape 정보가 어떤 구현 선택으로 이어지는가? |
| 실제 사용되는 Affine/SCF·Vector·MemRef IR | loop step·tail, vector type, access map, allocation | 선택한 결정이 어떤 루프·접근 구조를 만들었는가? |
| LLVM dialect / LLVM IR | loop·vector·call 구조, 후속 최적화 결과 | 앞선 차이가 유지·제거·확대되었는가? |
| 최종 native code | 관련 함수의 assembly, text size, 호출 대상 | 실제 실행물에 어떤 차이가 남았는가? |
| runtime | 동일 입력 latency, 선택적 op profiling·counter | 남은 차이가 성능 효과와 일치하는가? |

실제 pipeline에 없는 dialect나 pass를 문서상의 이상적인 순서에 맞춰 삽입하지 않는다. 고정 버전에서 출력한 pipeline과 pass 이름을 기록한다.

### 5.2 분석 방법

1. 원래 lowering과 국소 변경 실행물 양쪽에서 대응하는 op·출력·location 정보를 기록한다. static/dynamic 보조 분석에도 같은 방법을 적용한다. fusion으로 대응이 다대일이면 그 사실을 보존한다.
2. 관련 lowering 전후 및 후속 주요 단계의 IR을 수집한다.
3. SSA 이름·출력 순서와 같은 표면 차이와 unroll·loop·access 결정 차이를 구분한다.
4. 최초 텍스트 차이를 원인이라고 부르지 않는다. 특히 static/dynamic 비교는 shape type 자체가 처음부터 다르므로 당연한 차이가 존재한다.
5. 성능 손해 후보에서 특정 결정만 바꾸고 나머지 compilation pipeline을 유지한다.
6. 변환 후 correctness와 최종 실행시간을 다시 검증한다.

**MLIR의 추가 가치는 구조화된 중간 표현에서 원인 가설을 검사할 수 있다는 점이다.** 단순히 기존 profiler를 실행하는 것만으로 이를 달성했다고 보지 않는다.

### 5.3 원인 격리와 IR replay의 제한

- `onnx-mlir-opt` 등으로 snapshot을 다시 처리하는 기능은 해당 snapshot의 dialect·entrypoint·constant resource 및 pipeline이 보존될 때만 사용한다.
- 상수를 elide한 출력은 관찰용이다. 누락된 상수를 복구하지 않은 채 실행 가능한 reproducer로 사용하지 않는다.
- replay 경로가 원래 경로와 다른 code generation을 만들면 그 결과를 주 인과 증거로 사용하지 않는다.
- 임의로 pass를 제거하면 합법성·최적화 경로·분석 의존성이 함께 바뀔 수 있다. 통째로 `-O0` 또는 SIMD 전체를 끄는 실험은 좁은 원인 개입을 대체하지 못한다.

## 6. 첫 원인 후보: Transpose의 무나머지 unroll

### 6.1 소스로 확인한 사실

ONNX-MLIR commit `1e017c9fcd7ae7218731ae799f13a957e0cbc80b`에서 다음을 직접 확인했다 [R2a, R2b].

- `scalarTransposeOverOutputs`는 innermost store dimension에 대해 unroll 상한 8을 사용한다.
- `getNoLeftoverUnrollFactor`는 trip count가 literal이 아니면 1을 반환한다.
- literal이면 8부터 2까지 내려가며 trip count를 나누어떨어지게 하는 첫 factor를 반환하고, 없으면 1을 반환한다.
- 실제 적용에는 transpose 경로와 parallel loop 조건도 관련된다.

따라서 해당 루프의 trip count가 L인 경로에서는 다음 후보 규칙을 얻는다.

\[
u(L)=\max\left(\{1\}\cup\{k\in\{2,\ldots,8\}:L\bmod k=0\}\right).
\]

이는 기존 보고서에서 관측한 13개 transpose loop의 패턴과 부합한다. 그러나 다른 버전·모델·경로에서도 동일하게 적용된다는 결론은 아니다. dynamic L을 받아도 특정 내부 루프의 bound가 상수로 추론될 수 있으므로 실제 IR을 확인한다.

### 6.2 검증할 가설

> 특정 shape 특화에서 선택되는 unroll이 후속 vectorization, register allocation, memory access 또는 code layout과 상호작용하여 성능 손해에 기여한다.

각 메커니즘은 대안 가설이다. 초기부터 register spill이나 cache miss를 원인으로 확정하지 않는다. 클라우드 VM에서 hardware counter를 읽지 못하면 해당 원인에 관한 주장을 제한한다.

### 6.3 최소 개입

G1에서 해당 함수의 unroll 상한을 8에서 1로 바꾼 실험용 compiler build를 만든다. 이미 확인한 소스 규칙에 대한 최소 대조군이며, 새로운 최적화 pass 제안이 아니다. 정확한 patch, 원복 commit, build hash를 보관한다. helper 함수를 전역 수정하여 다른 op의 unroll까지 바꾸지 않는다.

| 실행물 | L 처리 | Transpose unroll 정책 |
|---|---|---|
| S8 | static L | 원래 정책 |
| D8 | dynamic L | 원래 정책 |
| S1 | static L | 해당 scalar transpose 경로의 unroll 상한 1 |
| D1 | dynamic L | 동일한 국소 수정 |

**S8/S1이 필수 주 대조이다.** D8/D1은 dynamic 지원과 정확성이 확인된 경우에만 추가한다. 전체 모델에서 S8이 D8보다 빠르더라도 S1이 S8보다 빠를 수 있으므로, S8/D8 결과만으로 국소 lowering 손해를 기각하지 않는다.

전체 모델에서 이 수정은 해당 함수가 처리하는 여러 Transpose에 영향을 줄 수 있다. 따라서 전체 모델 S8/S1 차이는 **이 정책 변경의 총효과**이다. 특정 한 노드의 독립 효과를 주장하려면 실제 모델에서 추출한 해당 subgraph 또는 노드별 개입이 추가로 필요하다.

필수 확인:

- 원래도 factor 1인 추출 op를 음성 대조군으로 사용한다.
- D8/D1의 해당 루프가 동일해야 한다는 예측을 IR로 확인한다. 다른 static 내부 루프가 함께 바뀌면 완전한 음성 대조가 아니라고 표시한다.
- LLVM이 다시 unroll하거나 vectorize할 수 있다. 최종 코드까지 확인한다.
- 개입이 효과를 줄여도 “이 unroll 정책이 항상 잘못됐다”라고 결론 내리지 않는다.
- 개입 후 모든 관련 정확성 검사를 통과하고, 원복 시 효과가 돌아오는지 독립 실행에서 확인한다.

## 7. 연구 질문과 평가 지표

### RQ1. 동일 입력에서 특정 lowering 정책의 손해가 존재하는가?

길이 L, 입력 x, VM 할당 a, paired block b에 대해:

\[
a_{a,b}(L,x)=\log T_{S8,a,b}(L,x)-\log T_{S1,a,b}(L,x).
\]

- 양수이면 원래 unroll 정책의 실행물이 국소 변경 실행물보다 느리다.
- `exp(a)-1`로 상대 slowdown을, `T_S8-T_S1`로 절대 시간 차이를 함께 보고한다.
- 서로 다른 길이의 단순 비율을 주 oracle로 쓰지 않는다. 동일 L·동일 x가 비교 단위이다.
- 인접 길이 변화량 `q(L)=a(L+1)-a(L)`는 해당 정책 효과의 불연속을 설명하는 보조 지표이다. 단독으로 원인을 입증하지 않는다.

### RQ2. 관측한 손해를 어떤 MLIR 및 후속 코드 생성 차이로 설명할 수 있는가?

\[
r(L)=\log T_{S8}(L)-\log T_{D8}(L),
\]

\[
i(L)=\left[\log T_{S8}(L)-\log T_{D8}(L)\right]
-\left[\log T_{S1}(L)-\log T_{D1}(L)\right].
\]

주 분석은 RQ1의 `a(L)`와 IR·assembly·국소 재현물·음성 대조의 일치 여부이다. 위의 `r(L)`와 `i(L)`는 dynamic 조건이 지원될 때 추가하는 보조 분석이다. `i(L)`는 static/dynamic 차이에 대한 정책 개입의 상호작용이며, 관측하지 않은 중간 원인의 완전한 매개효과라고 해석하지 않는다. 각 항은 같은 입력의 paired 측정에서 계산한다.

### RQ3. 원인 조건과 MLIR 관찰 결과가 재현되는가?

- 다른 VM 할당: 환경 잡음에 대한 재현.
- 다른 model graph: workload 전이. 같은 BERT family 안의 전이와 다른 architecture 전이는 구분한다.
- 다른 ONNX-MLIR revision: compiler 변화에 대한 재현 또는 해소 확인.
- source-derived 조건과 실제 IR 결정의 일치·불일치 사례를 모두 보고한다.

### 7.1 컴파일 정보의 추가 예측력을 주장하려면 필요한 비교

본 실험의 필수 공헌은 예측기 개발이 아니다. 예측력을 추가 주장할 때만 다음 baseline을 고정한다.

1. L과 모델 차원만 사용하는 크기 baseline.
2. L의 vector remainder와 `L mod 2…8`, 소스에서 확인한 `u(L)`를 포함하는 산술 baseline.
3. 기존 SIMD report 특징.
4. 2번에 op별 lowering 결정을 추가한 MLIR 특징.

같은 용량의 설명 모델과 동일한 개발/검증 분할을 사용한다. 14개 class를 14개 독립 모델처럼 세지 않는다. 인접 길이의 무작위 train/test 분할은 주 일반화 증거로 사용하지 않는다. held-out model/revision에서 추가 정확도와 수집 비용을 비교한다.

**산술 baseline이 동등하면:** compile-guided 예측의 추가 기여는 기각한다. MLIR을 이용한 원인 확인의 가치는 별도로 평가한다.

## 8. 벤치마크 구성과 선정 근거

### 8.1 필수 대상과 단계적 확장

| 대상 | 용도 | 선정 근거 | 제한 |
|---|---|---|---|
| A: 기존 BERT-base SQuAD 재수출 artifact | 타당성 실험·개발·전 길이 탐색 | ONNX Model Zoo 모델 계보, BERT·SQuAD 문헌, 기존 재수출 검증 [P0, R15–R17] | 재수출 graph이므로 원본 파일 자체의 성능이라고 부르지 않음 |
| A에서 추출한 실제 Transpose 및 producer/consumer subgraph | 원인 분리 | 이미 관찰한 lowering에서 출발 | 임의 크기의 synthetic GEMM을 대체 benchmark로 쓰지 않음 |
| B: MLPerf BERT-Large 공식 artifact | 사전 고정한 원인 규칙의 workload 전이 | MLCommons benchmark와 공식 공개 artifact [R18a, R18b] | 아직 미확보·미검증. A와 같은 BERT 계열이라는 한계 |
| A 또는 B + 다른 ONNX-MLIR revision | 버전 전이·해소 확인 | version differential 연구 설계 [R7] | 버전은 결과를 보고 고르지 않음 |

B를 확보하지 못하면 B 실험을 수행했다고 기재하지 않는다. 출처 불명 mirror나 임의 소형 모델로 교체하지 않는다. 검증 가능한 공식 대체 artifact를 사전등록 변경으로 도입하거나, 모델 일반화 주장을 삭제한다.

### 8.2 A의 길이 집합

기존 공식 전처리의 결과로 관측된 자연 길이 집합을 사용한다 [P0].

\[
\mathcal L_A=\{41,42,\ldots,256\},\quad |\mathcal L_A|=216.
\]

전처리 설정은 기존 기록인 `max_seq_length=256`, `doc_stride=128`, `max_query_length=64`를 유지하고, tokenizer·vocab·dataset·전처리 코드의 정확한 commit/hash를 lock에서 복원한다. 이 설정을 MLPerf의 공식 성능 제출 설정이라고 부르지 않는다.

작업 보고서의 개발 자료는 10,570 questions, 12,006 features이며, `feature`는 문서 sliding window 때문에 question과 일대일 대응하지 않는다. 길이 빈도 가중치는 feature-level과 question-level을 구분한다. 이 분포는 SQuAD 전처리 분포이며 실제 서비스 트래픽 분포가 아니다.

### 8.3 두 종류의 입력

**통제 입력:** 기존의 사전 선택된 L=41 anchor feature를 우측 padding하여 각 L에서 동일한 유효 토큰·mask를 유지한다. L 사이 분석에서 내용 차이를 줄인다. S/D 대조에서는 길이와 전체 입력 byte까지 동일해야 한다.

**자연 입력:** 각 L에 실제로 존재하는 SQuAD feature를 사용한다. 초기 대표 feature는 `(question_id, window_index)` 정렬의 첫 항목처럼 timing을 보지 않는 규칙으로 선택한다. 확인 단계에서는 같은 L의 자연 feature를 추가해 anchor 특이성을 점검한다. 선택 목록과 제외 사유를 고정한다.

추출 subgraph 입력은 원래 모델의 실제 중간 tensor를 저장해 사용한다. transpose-only에는 값 이동의 exact permutation 검증을 적용한다. 수학 연산을 포함한 subgraph에는 원본 중간 출력과 고정한 수치 검증을 적용한다.

### 8.4 원본 및 재수출 통제

- 원본 `bertsquad-12`에는 고정 256 관련 상수가 있어 임의 L을 입력하는 것으로 dynamic 모델이 되지 않았다 [P0]. 원본에 input shape만 덮어써 성능을 재지 않는다.
- S8/S1과 보조 D 실행물은 **동일한 검증된 A_reexport 파일**에서 생성한다. 별도 exporter로 만든 실행물을 주 대조로 비교하지 않는다.
- 재수출 전후 node 수가 다르므로 성능 결과는 재수출 graph의 결과로 명시한다.
- 파일이 byte 단위로 재수출 재현되지 않아도 semantic validation과 정확한 artifact hash는 보존해야 한다. 미보관 검증 기록을 있었던 것처럼 채우지 않는다.

### 8.5 B의 shape 집합

B는 공식 tokenizer·vocab·전처리 설정·체크포인트를 확보한 뒤 자연 길이 catalog를 만든다. A의 41…256을 B에 그대로 강제하지 않는다. 공식 artifact가 고정 shape만 지원하면, 원래 framework와 체크포인트를 이용한 검증된 재수출이 먼저 필요하다. graph의 상수를 편집하여 길이만 늘리는 방법은 금지한다.

본 연구는 MLPerf에서 모델 출처와 정확도 평가 방식을 활용한다. LoadGen·공식 시나리오·규칙을 준수하지 않는 실험에 MLPerf 성능 점수라는 명칭을 붙이지 않는다.

## 9. 환경과 compilation 구성

### 9.1 초기 재현 기준

| 항목 | 초기 기준 |
|---|---|
| ONNX-MLIR | `v0.5.1.1`, SHA `1e017c9fcd7ae7218731ae799f13a957e0cbc80b` |
| LLVM/MLIR | SHA `1053047a4be7d1fece3adaf5e7597f838058c947` |
| 주 runtime | 기존 Python 3.11.15, NumPy 2.2.6, ORT 1.23.2 환경을 우선 복원 |
| dtype / batch | FP32 / 1 |
| inference threads | 1, affinity 고정, 실제 활성 thread 수 확인 |
| optimization | 양쪽 모두 같은 `-O3`와 검증된 target 설정. 주 대조에서는 명시한 국소 lowering 수정만 다름 |
| weights / graph / inputs | 같은 hash와 입력 bytes |
| timing | 비계측 실행물의 `session.run(inputs)` 경계 |
| compilation | timing과 분리. LLVM 빌드·census·다른 성능 작업과 동시 실행하지 않음 |

기존 개발 환경은 기능 확인용이었다. 정식 측정에는 CPU 모델·ISA·vCPU·SMT·NUMA·메모리·quota·provider·allocation ID를 확인 가능한 클라우드 VM을 사용한다. GPU나 별도 구매 HW는 초기 단계에 필요하지 않다. Codex/Claude Code의 개발 컨테이너가 이 조건을 만족하지 않으면 해당 컨테이너의 timing을 본 결과로 사용하지 않는다.

VM 규격은 논문에서 임의로 가져오지 않는다. 기존 최대 메모리 기록과 재빌드 pilot의 측정치를 이용해 부족하지 않은 메모리·디스크를 확보한다. CPU 성능 효과는 확인한 target 범위로 제한한다.

### 9.2 주 대조 및 static/dynamic 보조 빌드 정의

A의 입력은 보고서상 4개이다. 실제 graph signature를 다시 검증한 뒤 compiler argv를 구성한다.

```text
S(L): --shapeInformation=0:1,1:1xL,2:1xL,3:1xL
D:    batch/unique-id shape만 고정하고 sequence dimension L은 dynamic 유지
```

현재 문서가 설명하는 `-1` override는 원래 shape 정보를 유지한다 [R1]. 따라서 `1x-1`을 넣는 것만으로 내부 graph까지 dynamic이라고 판정하지 않는다. 고정 버전의 옵션 파서·입력 IR·관련 loop bound·서로 다른 L의 실제 실행으로 확인한다.

필수 대조 조건:

1. D는 여러 L에서 **같은 shared library**를 재사용한다.
2. D 실행 중 길이별 재컴파일이나 몰래 다른 실행물로 전환이 없어야 한다.
3. S(L)과 D의 shape-dependent 계산 이외 옵션이 같아야 한다.
4. 두 실행물 모두 실제 입력 shape·dtype·stride를 검증한다.
5. D가 컴파일되지 않거나 정확하지 않으면 dynamic 보조 분석을 제외한다. 정확한 S8/S1 주 대조는 진행할 수 있다. 즉석에서 ORT를 D 대신 넣지 않는다.

S8/S1에는 위 조건 중 동일 graph·입력·target·나머지 옵션 통제가 그대로 적용된다. 두 compiler build에서 동일 L을 고정하고, 소스 차이가 지정한 lowering 정책뿐임을 확인한다.

### 9.3 target·문서 버전 주의사항

기존 `--march=x86-64 --mcpu=emeraldrapids`에는 ONNX-MLIR 수준에서 `mcpu`를 무시한다는 경고와 LLVM 수준의 target 적용이 함께 있었다 [P0]. 경고를 숨기거나 전 단계에 동일한 CPU 정보가 적용됐다고 주장하지 않는다. 초기 재현은 지원 ISA가 맞는 VM에서 같은 옵션을 사용하고, actual command·warnings·target attributes·최종 코드의 ISA를 기록한다.

[R1, R3, R4]의 웹 문서는 현재 문서이다. 모든 옵션이 고정 compiler revision에 존재한다는 뜻이 아니다. `--help`, `--help-hidden` 및 해당 버전 소스로 사용 가능성을 확인한다. 특히 shape별 profiling이나 최신 report script가 없으면 그 기능을 구현 완료로 간주하지 않는다.

### 9.4 측정 실행물과 진단 실행물

- 주 latency: profiling·IR debug 출력이 없는 실행물.
- 원인 진단: 지원되는 `--profile-ir` / `--profile-ir-with-sig` 또는 version-compatible 계측 실행물 [R3].
- 두 실행물의 결과와 오버헤드를 분리한다. 계측된 op 시간의 합을 비계측 전체 latency와 동일시하지 않는다.
- hardware counter는 접근 가능할 때만 추가한다. unavailable을 0으로 기록하지 않는다.
- `.so` 전체 크기는 weights 영향을 크게 받으므로 text section·코드 크기와 별도로 기록한다.

## 10. 단계별 실험 프로세스와 게이트

### G0 — 기존 연구 동결 및 환경 복원

**작업**

1. 기존 보고서·census·preregistration·결과를 별도 버전으로 보존한다.
2. 새 연구 질문과 기존 중단 결정을 기록한 design amendment를 작성한다.
3. 실행 시작 시 harness commit, worktree 상태, 실제 source tree 및 dependency hash를 고정한다. 실행 중 바뀐 HEAD를 이미 적재한 코드 버전으로 기록하지 않는다.
4. stale kernel manifest, Python ONNX 버전과 compiler 내부 ONNX 버전 차이, 누락한 correctness 기록을 정리한다.
5. 기존 명세가 요구했지만 실행하지 않은 `check-mlir`를 수행하거나 미수행 범위를 명시한다. Dockerfile은 실제 build 후 image digest를 기록한다. native 실행을 택하면 native 재현 절차를 고정한다.
6. 새 S8/S1 paired measurement가 실제 실행물로 동작하는지 확인한다. 기존 `LiveBackend.confirm` 미구현 상태를 구현 완료로 표시하지 않는다.

**게이트:** immutable provenance와 실제 실행 경로가 확보되어야 한다. 합성 테스트 125개 통과는 본 게이트의 성능 타당성을 대신하지 않는다.

### G1 — 저비용 타당성 pilot

**대상 선택 규칙 — 신규 설계 선택, timing 독립적**

- 기존 14개 structural class 각각에서 길이의 lower median 1개를 선택한다.
- 범위의 최소·최대 길이 41, 256을 추가하고 중복 제거한다.
- 따라서 초기 static 길이는 **최대 16개**이다. 정확한 목록은 원래 census table에서 산출·고정한다.
- 이 표본은 class coverage용이며 전 범위 성능 사건의 대표 표본이나 부재 증거가 아니다.

**실행 순서**

1. 원래 compiler와 Transpose unroll 상한만 1로 바꾼 compiler를 build한다.
2. 각 선택 L에서 S8/S1을 생성하고, 같은 입력으로 correctness와 의도한 IR 차이를 확인한다.
3. 새 측정 VM에서 무작위 paired block으로 S8/S1을 실행한다.
4. A/A 대조로 동일 실행물을 서로 다른 label로 비교해 측정 경로의 가짜 차이를 점검한다.
5. warmup·반복 계층별 분산·측정 비용을 추정한다.
6. `pilot_report`에 효과 추정치·CI·정밀도·실패·예상 전체 비용을 기록한다.

**진행:** 적어도 하나의 재현 가능한 역전 후보와 조사할 구조적 결정이 있고, 독립 확인에 필요한 정밀도를 예산 안에 얻을 수 있을 때 G2로 간다.

**중단/보류:** 주 대조의 correctness 실패, 통제 불가능한 잡음, 또는 조사 가치가 있는 후보가 없는 경우이다. “이 pilot에서는 후보를 못 찾았다”와 “모든 길이에 손해가 없다”를 구분한다. 추가 전 범위 탐색은 별도의 비용 근거가 있을 때만 정한다. D 지원 실패는 이 주 게이트의 중단 사유가 아니다.

### G2 — 원인 분리 pilot

1. G1 후보의 대응 ONNX op와 주요 lowering 결정을 찾는다.
2. 실제 입력 activation을 저장하고 op 또는 producer/consumer subgraph를 추출한다.
3. 원래 모델과 추출 graph에서 의심한 구조가 유지되는지 확인한다. 추출로 fusion·layout·allocation이 달라지면 기록한다.
4. Transpose 가설이 해당 후보에 맞는지 subgraph의 S8/S1로 검증한다. dynamic 지원과 추가 설명 가치가 있으면 D8/D1을 생성한다.
5. 다른 VM 할당의 새 paired block에서 개입·음성 대조·원복을 측정한다.
6. 출력 정확성과 전체 모델 latency 영향을 함께 확인한다.

**진행:** 손해가 확인되고, 좁은 결정 개입의 방향이 가설과 맞으며, 설명이 runtime과 연결될 때 G3로 간다.

**중단/축소:** 단지 IR만 달라졌거나, 구조 차이가 LLVM에서 사라졌거나, 효과가 계측·추출 artifact에만 존재하거나, 전체 모델과 연결하지 못하면 현재 주장을 축소한다. 무한정 다른 signature와 flag를 탐색하지 않는다.

### G3 — 본실험 사전등록

G1/G2는 설계·탐색 자료로 남긴다. 본실험 데이터와 합쳐 독립 확인 결과처럼 사용하지 않는다.

고정할 항목:

- 모델·데이터·툴체인·CPU class·target·patch hash.
- 전체 대상 길이와 입력 선정 규칙.
- 주 지표, correctness 정책, warmup·반복 수 및 산출 근거.
- VM 할당과 block 구조, exclusion·retry·failure 규칙.
- 확인 후보를 선택하는 규칙, 독립 확인 자료의 분리 방식.
- 검정 family·alpha·CI 계산·실용적 크기 해석 규칙.
- 최대 실행 비용과 종료 규칙.
- source-derived 원인 예측과 held-out graph/revision의 선택 규칙.

실제 pilot과 사용 가능한 예산이 없는 현재 문서에서 반복 수나 VM 수를 확정할 수는 없다. 이는 임의 숫자로 채우지 않고 G3 이전에 근거와 함께 채워야 하는 항목이다. 누락 상태의 실행은 pilot으로만 허용한다.

### G4 — A 전 길이 탐색 및 독립 확인

- L=41…256의 S8/S1 결과를 전부 수집한다. timeout·compile failure·incorrect도 포함한다. D 결과를 추가할 경우 별도 지원 집합과 실패 분모를 기록한다.
- 이 집합에서 효과 지형과 원인 후보를 분석한다.
- 사전에 정한 규칙으로 확인 후보 집합 K를 확정한다.
- 새로운 VM 할당·process·order seed에서 K를 독립 확인한다.
- full SQuAD correctness 검증과 실제 feature별 영향 분석을 완료한다.
- 발견에 사용한 추정치와 독립 확인 추정치를 따로 보고한다.

중요한 사건의 인접 길이를 모두 추가 측정하는 것은 메커니즘 확인용이다. 그 구간만을 대상으로 전체 shape 분포에 대한 발생률을 추정하지 않는다.

### G5 — 사전 고정한 조건의 전이 검증

1. BERT-Large artifact의 정확성·shape 변경 적합성·해당 lowering 경로를 먼저 확인한다. dynamic 지원은 보조 분석에만 요구한다.
2. A에서 얻은 메커니즘 규칙과 분석 절차를 수정 없이 적용한다.
3. 다른 compiler revision은 결과를 보기 전에 선택 규칙과 SHA를 고정한다. 예: 연구 동결일의 최신 공식 안정 release; 원본과 같으면 버전 전이 축을 성립한 것으로 세지 않는다.
4. 새로운 조건에 맞춰 특징을 수정하면 해당 자료는 다시 개발 자료가 된다. 원래 예측의 실패를 먼저 기록한다.

BERT-base→BERT-Large 성공은 BERT family 내부의 전이이다. 이를 모든 ML architecture에 대한 증거로 확대하지 않는다. 다른 architecture 추가는 실제 원인 메커니즘이 해당 graph에 존재하며 검증 가능한 benchmark가 있을 때 별도 설계로 진행한다.

### G6 — 공헌 최종 심사와 산출물

§4.2의 질문에 답하는 evidence table을 작성한다. 결론은 다음 중 하나로 구분한다.

- 원인과 적용 조건이 뒷받침되는 실증 연구.
- 특정 구현·모델에 한정된 성능 결함 사례.
- 관측 범위에서 특화 손해가 확인되지 않은 부정 결과.
- 정확성·환경·검정력 문제로 판단 불가.

통계적 비유의성을 부정 결과의 충분한 증거로 쓰지 않는다. 충분히 좁은 효과 범위가 확보되었을 때만 “어느 크기 이상의 손해는 지지되지 않았다”고 기술한다.

## 11. 측정·통계 프로토콜

### 11.1 실험 단위와 무작위화

- 같은 VM, 같은 입력, 인접한 시간 block 안에서 S8/S1 또는 4개 개입 조건의 순서를 무작위화한다.
- 반복 실행은 새 process를 사용하고, load·tokenization·입력 준비·correctness 검사는 timing 밖에서 수행한다.
- VM allocation → process/block → iteration의 계층을 보존한다. 반복 iteration을 독립 VM 표본처럼 세지 않는다.
- 주 runtime 요약은 고정한 warmup 이후의 process별 산술 평균 latency로 정한다. 그 paired log ratio를 추론에 사용한다. median·분위수는 보조로 보고한다.
- cloud 제공자가 다른 CPU를 할당하면 미리 정한 stratum으로 분리하거나 전체 해당 block을 제외한다. 성능이 느리다는 이유로 관측값을 삭제하지 않는다.

동일 VM에서의 무작위 대조와 cloud variation 관리는 [R20], 계층별 반복·정밀도 계획은 [R19]를 따른다. 두 연구의 benchmark는 본 연구와 다르므로 그 논문의 반복 수나 검출 가능한 slowdown을 그대로 가져오지 않는다.

### 11.2 반복 수와 warmup 결정

G1에서 다음을 측정한다.

1. startup·page fault·warm cache에 따른 시간 추이.
2. iteration 내부 변동, process 간 변동, VM 할당 간 변동.
3. 각 반복 계층을 하나 늘릴 때 드는 비용.
4. 미리 정한 전체 예산에서 달성 가능한 효과 CI 폭.

이 결과로 반복 배분과 warmup을 고정한다 [R19]. “일반적으로 30회”, “VM 2개면 충분” 같은 근거 없는 규칙을 사용하지 않는다. 2개 VM에서 할당 평균에 대한 t 검정을 수행하면 자유도가 1이라는 점도 반영한다.

본실험 중 유의해질 때까지 반복하지 않는다. 예산 제약으로 목표 정밀도에 도달하지 못하면 판단 불가를 허용한다. 새로운 precision 목표를 택할 경우 데이터 열람 전 변경 또는 별도 탐색 연구로 기록한다.

### 11.3 추론과 다중 비교

- 주 추론은 독립 확인 자료의 VM allocation별 paired log ratio 평균에 대한 효과 추정과 CI이다.
- 정규성·표본 수·계층 구조에 대한 pilot 진단으로 분석법을 선택하고 G3에서 고정한다. 기존 t 검정 코드를 검토 없이 재사용하지 않는다.
- 선택된 사건에 대한 방향은 탐색에서 고정하고, 확인 자료에서 단측 가설을 검정한다.
- **신규 설계 선택:** 확인 family의 family-wise error 수준을 0.05로 설정하고 Holm 보정을 적용한다 [R21]. 0.05는 자연 법칙이나 특정 compiler 논문의 요구값이 아니라 본 연구의 오류 통제 선택이다.
- family는 최종 연구에서 함께 확정적으로 주장할 model·shape·개입 contrast 목록으로 정의한다. 여러 작은 family로 쪼개 전체 오류를 숨기지 않는다.
- 실패하거나 충분히 측정하지 못한 항목을 성공 사례 분모에서 제거하지 않는다. 사전 고정 family에서 판단 불가로 남긴다.
- 전체 탐색 곡선의 개별 CI와 확인 검정의 다중 비교 보정 여부를 구분한다.
- 여러 효과 threshold를 보고 가장 잘 나오는 값을 주 결과로 선택하지 않는다.

### 11.4 통계적 차이와 실용적 차이

현재 사용자 서비스의 latency 요구나 호출량이 없으므로 **5%·10%를 학술적으로 정당화된 실용 threshold라고 발명하지 않는다.**

1. 주 효과는 상대·절대 latency와 CI로 연속적으로 보고한다.
2. `a>0`의 확인은 원래 lowering 정책의 상대적 손해에 관한 통계적 판단이다. 실용적 중요성을 자동으로 의미하지 않는다. static/dynamic 보조 분석의 `r>0`도 같은 원칙을 적용한다.
3. 서비스 요구가 제공되면 요구 latency 또는 배포 비용에서 최소 중요 효과를 정하고 확인 자료 수집 전에 고정한다.
4. 요구가 없으면 compiled artifact 비용과 아래 손익분기 곡선으로 해석하고, 임의 threshold로 성공/실패를 포장하지 않는다.

개선 실행물 또는 static 특화가 더 빠른 경우, 단순한 wall-time 계산에서 추가 compile cost ΔC와 호출당 절감 ΔT에 대해 `N*=max(0,ΔC)/ΔT`를 계산할 수 있다. ΔT≤0이면 이 기준의 유한한 amortization 이득이 없다. 이 식은 storage·동시 실행·VM 요금 차이를 포함하지 않으므로 전체 경제성으로 부르지 않는다.

### 11.5 정확성 게이트

- 입력 shape/dtype/stride, mask·padding, finite output을 검사한다.
- transpose-only 개입은 실제 tensor 값의 exact permutation을 검증한다.
- floating-point 연산을 포함한 실행물은 pinned upstream verifier의 수치 비교 규칙을 확인해 출처와 값을 G3에 기록한다. 사용 가능한 규칙이 없거나 해당 모델에 부적합하면 별도의 정확성 calibration이 선행되어야 한다. 관측한 최대 오차 바로 위로 tolerance를 사후 설정하지 않는다.
- pilot에서는 선택한 모든 L·입력·실행물의 수치 검증이 필요하다. L=128 통과로 다른 L을 면제하지 않는다.
- 모델 수준 공헌을 주장하기 전 full SQuAD에서 ORT와 compiled 조건의 EM/F1 및 **question별 최종 예측 차이 수**를 보고한다 [R17]. EM/F1이 같아도 개별 예측은 다를 수 있다.
- 보수적인 초기 semantic gate는 ORT 대비 최종 answer 동일성을 요구한다. 이는 공식 benchmark가 요구하는 보편 규칙이 아니라 비교 의미를 단순화하기 위한 본 연구 선택이다. 불일치 시 원인을 분석하고, 허용 정책 변경은 별도 사전등록으로 남긴다.
- 정확성에 실패한 빠른 실행물은 성능 우위로 계산하지 않는다.

### 11.6 subgraph 측정

작은 Transpose의 경우 Python 호출 비용이 kernel 시간을 가릴 수 있다. 이때 고정한 C/C++ runtime entrypoint와 batch-of-calls 측정을 사용하되 호출 수·warmup·출력 소비·buffer 재사용·cache 상태를 기록한다. timer resolution과 wrapper overhead가 효과 판단을 지배하지 않는지 pilot으로 확인한다.

kernel microbenchmark와 전체 모델 측정의 절대 시간을 직접 등치하지 않는다. cache residency, allocation, 주변 op와의 fusion이 달라질 수 있기 때문이다. 전체 모델에서 개입 효과를 재확인해야 한다.

## 12. 저장·컴파일 비용과 현실성

기존 개발 기록에서 S(L=128)의 전체 compile은 약 80초, `.so`는 약 435 MB였다 [P0]. 이는 새 VM의 성능 예측치가 아니다. 단순한 규모 계산은 다음과 같다.

| 구성 | static build 수 | 기존 1점 수치를 곱한 규모 참고 |
|---|---:|---|
| G1, 한 정책 | 최대 16 | 약 21분, 약 7 GB |
| G1, 필수 S8/S1 두 정책 | 최대 32 | 약 43분, 약 14 GB |
| A 전 길이, 한 정책 | 216 | 약 4.8시간, 약 94 GB |
| A 전 길이, S8/S1 두 정책 | 432 | 약 9.6시간, 약 188 GB |

선택적 D compile, 두 compiler build, 정확성 확인, 반복 측정, 추가 개입 조건, 실패·재시도 비용은 위에 포함되지 않는다. BERT-Large에도 같은 비용을 적용하지 않는다. 실제 예산은 새 pilot으로 다시 산정한다.

저장 공간을 줄이려면 shape block별 build→paired 측정→검증된 archive를 사용할 수 있다. 단, class별로 순서대로 처리하여 시간대·VM 상태와 class가 혼동되지 않도록 block 순서를 무작위화하고 각 block 안에서 양쪽 정책을 모두 측정한다. compiler 상태가 바뀌지 않도록 hash를 고정한다.

weights 분리나 실행물 공유로 비용을 줄일 수 있는지는 실제 지원 기능을 검증한 뒤 적용한다. 아직 구현하지 않은 저장 최적화를 무료라고 가정하지 않는다.

## 13. 기존 코드의 재사용과 수정 범위

| 기존 자산 | 처리 |
|---|---|
| toolchain lock·artifact lock | 재사용하되 누락·불일치 수정 |
| SQuAD 전처리·재수출 검증 | 유지. graph·input 계보 보존 |
| randomized block·원시 timing 기록 | S8/S1 및 선택적 4조건 paired 구조로 확장 |
| SIMD/structural census | 개발 증거 및 특징 추출 검증에 활용 |
| IR hash | 무결성·중복 확인용으로 유지. runtime oracle로 사용하지 않음 |
| 7개 selector와 22개 변형 | 주 실험에서 제외·보관 |
| selector 격리·OS jail | 새 연구의 핵심이 아니므로 확대하지 않음 |
| 기존 H1 중단 규칙 | 새 연구 게이트로 대체. 기존 판정 기록 보존 |
| LiveBackend 확인 경로 | 필요하면 실제 paired confirmation runner로 완성 |
| 기존 통계 함수 | 실험 단위·family·추론법 검토 후 제한적으로 재사용 |

### 구현 산출물 제안

아래는 새로 구현할 구성의 **계획상 명칭**이며 현재 저장소에 존재하거나 실행 가능하다는 뜻이 아니다.

```text
design_amendment.md
protocol_v3.json
scripts/build_shape_pair.py
scripts/run_paired_benchmark.py
scripts/collect_lowering_trace.py
scripts/extract_observed_subgraph.py
scripts/run_lowering_intervention.py
analysis/specialization_effects.py
analysis/mechanism_evidence.py
artifacts/cases/<case_id>/
```

구현 우선순위는 원래 정책/국소 변경 build → S8/S1 correctness와 실제 측정 경로 → provenance·필수 IR 수집 → 확인된 후보의 세부 원인 분리이다. 범용 대시보드·새 탐색 알고리즘·모든 pass 자동 분석기를 먼저 만들지 않는다.

## 14. 필수 결과 표와 원인 사례 패키지

### 14.1 결과 표

1. **범위 표:** 모델, revision, CPU, 길이 수, 성공·실패·incorrect·미측정 수.
2. **정책 효과 표:** S8/S1 latency, 상대·절대 효과, CI, 독립 확인 상태. S/D 결과는 보조 표로 분리.
3. **원인 표:** op, source 위치, lowering 결정, 후속 코드 차이, 개입 결과, 대안 설명.
4. **전이 표:** 개발 단계의 예측, 새 graph/revision 관측, 일치·실패·판단 불가.
5. **비용 표:** compile, IR 수집, verification, warmup, measure, 실패, 저장 용량.

### 14.2 사례별 필수 파일

- 원래 모델 hash, 입력 ID·입력 hash, shape·dtype·layout.
- compiler/harness/build/image 또는 native environment provenance.
- S/D 및 개입 조건의 정확한 argv·소스 patch.
- 관련 단계의 IR, normalized feature record, 최종 assembly.
- 원시 paired timing과 correctness 출력.
- 추출 reproducer와 원래 graph의 대응 관계.
- 후보 발견 자료와 독립 확인 자료의 구분.
- 결과의 지지 범위·대안 설명·알려진 제한.

node ID, signature version, pipeline stage를 명시하지 않은 IR 통계만으로 원인 사례를 완성했다고 보지 않는다.

## 15. 의사결정 규칙 요약

| 관측 | 결정 |
|---|---|
| dynamic AOT 실행물이 유효하게 동작하지 않음 | dynamic 보조 분석 제외. 정확한 S8/S1 주 대조는 진행 가능 |
| 최대 16개 길이의 pilot에서 후보가 없음 | 전 범위 부재를 주장하지 않고 중단 또는 별도 예산 근거로 제한 확장 |
| 후보가 새 VM에서 사라짐 | 환경·측정 문제 조사, 본실험 확대 보류 |
| 같은 입력의 역전과 좁은 개입 효과가 재현됨 | 원인 연구의 본실험 진행 |
| MLIR 구조 차이가 runtime 차이를 설명하지 못함 | 해당 가설 기각; IR 차이 자체를 공헌으로 계산하지 않음 |
| 약수·remainder만으로 예측이 충분함 | compile-guided 예측 기여 기각; 원인 실증 연구는 별도 평가 |
| 전체 모델 효과와 원인·독립 전이가 확보됨 | 논문 공헌 후보로 엄격한 문헌·증거 심사 |
| 단일 알려진 현상의 재현에 그침 | 재현 보고·upstream artifact로 정리 |
| 예산 내 정밀도가 부족함 | 판단 불가. 유의해질 때까지 측정하지 않음 |

**우선 수행할 연구는 G0→G1→G2이다.** 이를 통과하기 전 대규모 selector 연구나 새로운 compiler pass 개발에 투자하지 않는다. MLIR의 역할은 원인에 관한 관찰과 통제 실험으로 평가하며, MLIR을 사용했다는 사실 자체를 독창성으로 취급하지 않는다.

---

## 16. 근거와 설계 선택의 구분

| 항목 | 근거의 종류 |
|---|---|
| 216개 길이, 14개 class, primary signature 불변 | 사용자 제공 실험 보고서 [P0] |
| Transpose factor의 literal/divisor 규칙 | 고정 revision 소스 직접 확인 [R2a, R2b] |
| shape별 profiler·pass instrumentation 존재 | 공식 도구 문서 [R3, R4] |
| 추가 정보에 의한 최적화 악화, 차등 성능 테스트 | 선행연구 [R6, R7] |
| BERT·SQuAD·MLPerf model 선정 | 원 논문 및 공식 benchmark artifact [R15–R17, R18a, R18b] |
| 반복 계층·정밀도·cloud paired 측정 원칙 | 측정 방법론 연구 [R19, R20] |
| class별 median + endpoints로 최대 16개 pilot | 본 연구가 제안하는 비용 제한·구조 coverage 설계. 문헌의 표준 표본 수가 아님 |
| S8/D8/S1/D1 factorial contrast | 소스에서 도출한 본 연구의 개입 설계. 실제 효과는 미측정 |
| alpha 0.05, strict answer agreement | 명시적 연구자 선택. 문헌이 강제하는 값이 아님 |
| warmup, 반복 수, VM 수, numerical tolerance | pilot·upstream verifier·정밀도·예산 확인 후 G3에서 고정해야 함 |

학술적 타당성은 모든 설정값에 논문 번호를 붙이는 것으로 확보되지 않는다. **벤치마크 계보, 검증 가능한 가설, 적절한 대조군, 오류 통제, 독립 확인, 사전 고정한 설계 선택**이 함께 필요하다. 문헌에서 보장하지 않는 값은 보장한다고 쓰지 않는다.

## 17. 참고문헌 및 공식 자료

본문의 [P0], [R1] 등은 아래 항목을 가리킨다. 웹 자료 확인일: 2026-09-29. source 파일 외의 main/current 문서는 실행 전에 고정 버전과 대조해야 한다.

- **[P0]** 사용자 제공. `Shape-Perf_작업_종합보고서.md`, 2026-09-29. 특히 §4.1–4.6, §5.4–5.13, §6.2, §8–9. 현재 상태에 관한 1차 프로젝트 보고서이며 독립 재실행 자료는 아니다.
- **[R1]** ONNX-MLIR. *Debugging Numerical Errors*. runtime shape와 compile-time shapeInformation의 구분. https://onnx.ai/onnx-mlir/DebuggingNumericalError.html
- **[R2a]** ONNX-MLIR 고정 소스. `Transpose.cpp`, commit `1e017c9fcd7ae7218731ae799f13a957e0cbc80b`, 특히 `scalarTransposeOverOutputs`와 lines 205–219. https://github.com/onnx/onnx-mlir/blob/1e017c9fcd7ae7218731ae799f13a957e0cbc80b/src/Conversion/ONNXToKrnl/Tensor/Transpose.cpp
- **[R2b]** ONNX-MLIR 고정 소스. `ONNXToKrnlCommon.cpp`, 같은 commit, `getNoLeftoverUnrollFactor`, lines 988–1004. https://github.com/onnx/onnx-mlir/blob/1e017c9fcd7ae7218731ae799f13a957e0cbc80b/src/Conversion/ONNXToKrnl/ONNXToKrnlCommon.cpp
- **[R3]** ONNX-MLIR. *Performance Testing*. https://onnx.ai/onnx-mlir/PerformanceTesting.html
- **[R4]** LLVM MLIR. *Pass Infrastructure*, Pass Instrumentation 및 Standard Instrumentations. https://mlir.llvm.org/docs/PassManagement/
- **[R5]** Jin et al. *Compiling ONNX Neural Network Models Using MLIR*. 2020. https://arxiv.org/abs/2008.08272
- **[R6]** Theodoros Theodoridis and Zhendong Su. *Refined Input, Degraded Output: The Counterintuitive World of Compiler Behavior*. PLDI 2024. https://doi.org/10.1145/3656404 ; 공식 프로그램: https://pldi24.sigplan.org/details/pldi-2024-papers/28/Refined-Input-Degraded-Output-The-Counterintuitive-World-of-Compiler-Behavior ; 저자 소속기관 기록: https://www.research-collection.ethz.ch/items/15e4822c-2599-422a-a224-83f3766c7649
- **[R7]** Zijian Yi, Cheng Ding, August Shi, Milos Gligoric. *Understanding and Finding JIT Compiler Performance Bugs*. PACMPL 10, OOPSLA1, 2026. https://doi.org/10.1145/3798245 ; 공개 본문: https://arxiv.org/html/2603.06551v1 ; artifact: https://github.com/EngineeringSoftware/jittery
- **[R8]** Bojian Zheng et al. *DietCode: Automatic Optimization for Dynamic Tensor Programs*. MLSys 2022. https://proceedings.mlsys.org/paper_files/paper/2022/hash/f89b79c9a28d4cae22ef9e557d9fa191-Abstract.html
- **[R9]** Kai Zhu et al. *DISC: A Dynamic Shape Compiler for Machine Learning Workloads*. 공개 논문, 2021. https://arxiv.org/abs/2103.05288
- **[R10]** Weiyuan Tong et al. *Directed Testing in MLIR: Unleashing Its Potential by Overcoming the Limitations of Random Fuzzing*. FSE 2025. https://conf.researchr.org/details/fse-2025/fse-2025-research-papers/56/Directed-Testing-in-MLIR-Unleashing-Its-Potential-by-Overcoming-the-Limitations-of-R
- **[R11]** Jiyuan Wang et al. *DuoReduce: Bug Isolation for Multi-Layer Extensible Compilation*. FSE 2025. https://conf.researchr.org/details/fse-2025/fse-2025-research-papers/71/DuoReduce-Bug-Isolation-for-Multi-Layer-Extensible-Compilation ; artifact: https://github.com/UCLA-SEAL/DuoReduce
- **[R12]** *CLower: Detecting Compiler Pessimization Bugs through Redundant Memory Accesses*. OOPSLA 2026. https://doi.org/10.1145/3798250 ; 공식 프로그램: https://2026.splashcon.org/details/oopsla-2026/50/CLower-Detecting-Compiler-Pessimization-Bugs-through-Redundant-Memory-Accesses
- **[R13]** Aditya Chatterjee. *From Roofline to Ruggedness: Decomposing and Smoothing the GEMM Performance Landscape*. arXiv:2605.29752v2, 2026-07-17. **Preprint**. https://arxiv.org/abs/2605.29752v2
- **[R14]** Jiawei Liu et al. *NNSmith: Generating Diverse and Valid Test Cases for Deep Learning Compilers*. ASPLOS 2023. https://arxiv.org/abs/2207.13066 ; https://doi.org/10.1145/3575693.3575707
- **[R15]** ONNX Model Zoo. *bertsquad-12*. https://huggingface.co/onnxmodelzoo/bertsquad-12 ; 원 모델 자료: https://github.com/onnx/models/tree/main/validated/text/machine_comprehension/bert-squad
- **[R16]** Jacob Devlin et al. *BERT: Pre-training of Deep Bidirectional Transformers for Language Understanding*. NAACL 2019. https://aclanthology.org/N19-1423/
- **[R17]** Pranav Rajpurkar et al. *SQuAD: 100,000+ Questions for Machine Comprehension of Text*. EMNLP 2016. https://aclanthology.org/D16-1264/
- **[R18a]** MLCommons. *BERT-Large, MLPerf Inference*. https://docs.mlcommons.org/inference/benchmarks/language/bert/ ; reference implementation: https://github.com/mlcommons/inference/tree/master/language/bert
- **[R18b]** Po-Han Huang and Christopher Forster. *MLPerf Inference BERT ONNX Model on SQuAD v1.1 dataset*. 2020. https://doi.org/10.5281/zenodo.3733910
- **[R19]** Tomas Kalibera and Richard Jones. *Rigorous Benchmarking in Reasonable Time*. ISMM 2013. https://doi.org/10.1145/2464157.2464160 ; 저자 원고: https://kar.kent.ac.uk/33611/45/p63-kaliber.pdf
- **[R20]** Christoph Laaber, Joel Scheuner, Philipp Leitner. *Software microbenchmarking in the cloud. How bad is it really?* Empirical Software Engineering, 2019. https://doi.org/10.1007/s10664-019-09681-1 ; 저자 자료: https://joelscheuner.com/publication/laaber-19-emse/
- **[R21]** Sture Holm. *A Simple Sequentially Rejective Multiple Test Procedure*. Scandinavian Journal of Statistics 6(2):65–70, 1979. https://www.jstor.org/stable/4615733
