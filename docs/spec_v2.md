# 컴파일 정보를 활용한 shape별 성능 급락 탐지: 실험 명세

작성일: 2026-09-28  
개정: 2026-09-28 v2 (검토 의견 반영판)  
상태: 문헌·공식 구현에 근거한 구축 설계. 이 문서 작성 과정에서 컴파일러 빌드, 모델 호환성 검증, 클라우드 성능 실험은 수행하지 않았다. v2에서 추가된 내용도 동일하게 미실행 설계다.

## 0. v2 개정 요약

| 항목 | 변경 | 근거 |
|---|---|---|
| 가설 메커니즘 명시 (§1.2) | 컴파일 정보의 추가 가치가 성립하는 조건을 세 가설 H1–H3으로 분리 | 정적 특수화에서 정렬 배수 경계는 컴파일 없이 예측 가능하므로, 이득은 비분석적 결정 변화에서만 발생 [본인 분석·판단] |
| Signature census 관문 신설 (§11 G2.5) | 본실험 전에 평가자 전용으로 전 길이 IR을 수집해 변화점 수·위치를 계수 | 신호 부재를 G5에서야 확인하는 비용 회피 |
| Compile-probe 정책 추가 (§8.2b) | 실행 없이 IR만 뽑는 probe로 변화점을 이분 탐색한 뒤 양쪽만 실측하는 2단계 정책 | 정적 특수화에서 컴파일은 모든 정책의 공통 비용이므로 컴파일 정보의 실질적 차별점은 probe에 있음 |
| IR 신호 정밀도 지표 (§9.4, §11.1) | signature 변화점 중 정답표 A 사건과 일치한 비율 | signature 변화 ≠ 성능 변화 |
| matmul lowering 경로 확인 (§6.1, §6.2, G0) | 자체 코드 생성인지 외부 라이브러리 호출인지 기록 | 외부 라이브러리 호출이면 급락 원인이 컴파일러 결정 밖에 있음 |
| 재수출 경로 병행 준비 (§4.4) | `bertsquad-12`의 고정 길이 상수 위험에 대비해 가변 축 export를 G1과 병행 | G2 진입 실패 확률 완화 |
| 적응 기준선 보강 (§8.1) | Timing-adaptive(변화점/GP) 기준선을 [선택]으로 추가, 미채택 시 주장 범위 한정 | Timing-only 이분 탐색만으로는 "실행시간만 쓰는 방법"을 대표하지 못함 |
| 규모 참고치 (§8.5) | 컴파일·측정 규모 추정 근거 추가 | 정답표 A 구축 비용 판단 |
| 참고문헌 R23–R25 추가 | ONNX-MLIR 논문, FTuner, TileBench | 정렬 경계 기준선의 강도와 컴파일 결정의 예측 변수 타당성 |

## 1. 무엇을 연구할 것인가

**연구 질문:** 실제 모델에서 유래한 입력 shape 공간을 탐색할 때, 컴파일 과정에서 얻는 정보를 이용하면 **컴파일·정보 추출·실행 비용의 합이 같은 조건에서** 성능 이상 후보 및 독립적으로 확인된 성능 저하를 더 효율적으로 발견하는가?

첫 구현은 **ONNX-MLIR의 CPU 백엔드, FP32 BERT 질의응답 모델, batch=1, sequence length 한 축**으로 제한한다. 새 컴파일러 패스 개발은 필수가 아니다. 기존 최적화 보고서, IR 출력, 실행 API를 수집·분석하는 외부 실험 도구로 시작한다.[R1–R5]

여기서 “성능 급락”은 실행 시간이 갑자기 증가한다는 뜻이다. 다음 세 가지는 구분한다.

| 관찰 | 이 연구에서의 취급 | 추가로 필요한 증거 |
|---|---|---|
| 길이가 증가하면서 실행 시간이 증가함 | 일반적인 비용 곡선 | 계산량 증가만으로 설명되는지 확인 |
| 인접한 길이에서 재현 가능한 큰 변화가 생김 | shape 경계의 성능 이상 후보 | 독립 재측정, 동일 입력 내용 통제, 실행 환경 확인 |
| 같은 shape·입력·정확도에서 다른 정당한 컴파일 설정이 더 빠름 | 피할 수 있는 성능 저하의 증거 | 원인 개입, 모델 전체 효과, 반복 검증 |
| 같은 shape에서 이전 컴파일러보다 새 버전이 느려짐 | 버전 간 성능 회귀 후보 | 동일 대상 ISA·설정·입력의 버전 비교 |

**길이 128과 129의 시간이 다르다는 사실만으로 컴파일러 버그라고 하지 않는다.** IR 변화와 시간 변화의 동시 발생도 인과관계의 증명은 아니다.

### 1.1 연구 필요성과 신규성의 한계

ONNX-MLIR에는 이미 연산별·shape별 프로파일링과 최적화 보고 기능이 있다. 따라서 “MLIR에서 shape별 시간을 시각화했다”만으로는 기여가 약하다.[R1] MLIRTracer는 MLIR의 지향적 테스트, Perphecy는 성능 변화 탐지를 위한 테스트 선택, CITADEL은 성능 버그 탐지와 관련된 선행 연구다.[R18–R20]

본 연구의 **검증할 차별점**은 다음 결합에 있다.

1. 실제 벤치마크에서 추적 가능한 shape와 입력을 사용한다.
2. 컴파일 정보를 얻는 비용까지 포함한 탐색 효율을 비교한다.
3. 잡음·정상적인 연산량 증가·피할 수 있는 저하를 구별한다.
4. 탐색 알고리즘이 보지 못한 독립 측정으로 발견 결과를 확인한다.

이 결합의 우수성이나 최초성은 아직 입증되지 않았다. 선행 도구의 탐색 정책을 직접 재현할 수 있는지는 별도 검토 대상이며, 단순 무작위 탐색을 이들 논문 전체의 대체물로 취급하지 않는다.

### 1.2 컴파일 정보의 추가 가치가 성립하는 조건 [제안]

정적 shape 특수화에서 컴파일 결정은 길이 `s`의 결정 함수다. 따라서 vector lane 수·타일 크기처럼 **사전에 알 수 있는 정렬 단위의 배수 경계**에서 일어나는 결정 변화는 컴파일하지 않고도 예측할 수 있고, 이는 Shape-only 정책(§8.1)이 이미 활용한다. 컴파일 정보가 추가 가치를 가지려면 다음 세 가설이 성립해야 한다.

| 가설 | 내용 | 검증 단계 |
|---|---|---|
| **H1 (비분석적 변화의 존재)** | 정렬 배수 경계 밖에서도 lowering 결정(fusion 성립 여부, SIMD 폴백, 임시 버퍼 크기 클래스, 병렬화 임계값, matmul 경로 선택)이 바뀌는 길이가 존재한다 | G2.5 census |
| **H2 (성능과의 일치)** | H1의 변화점이 정답표 A의 재현 가능한 성능 변화와 유의하게 일치한다 | G4–G5, §9.4 |
| **H3 (probe의 비용 우위)** | IR만 얻는 컴파일 전용 probe 비용이 컴파일+정확도 검사+측정 비용보다 충분히 작아, 변화점 주변만 실측하는 편이 같은 예산에서 더 많은 사건을 찾는다 | G3 비용 파일럿, G5 |

H1이 기각되면 컴파일 정보는 Shape-only의 재서술이므로 본실험을 중단한다. H1이 성립하고 H2가 기각되면 "IR 변화는 성능 변화의 대리 지표가 아니다"라는 부정 결과를 보고한다. H3이 기각되면 Compile-guided(§8.2)와 Compile-probe(§8.2b)의 차이가 사라지며 컴파일 정보의 이득은 실측 끝점의 signature 비교에만 남는다.

BERT에서 sequence length는 대부분 연산의 최내측 차원이 아니다(hidden size가 최내측). 최내측 차원 길이로 SIMD 적용이 결정된다면 QKᵀ·softmax(seq×seq)를 제외한 연산의 signature는 길이와 무관할 수 있다. 이 가능성은 census로 확인하며, 사전에 "변화가 있을 것"이라고 가정하지 않는다. [본인 분석·판단]

## 2. 근거를 적용하는 원칙

각 설정을 아래 네 범주로 구분한다. **새로운 연구의 모든 설계 선택이 기존 논문에 그대로 존재할 수는 없다.** 문헌에서 가져온 사실과 연구자가 검증할 선택을 섞지 않는 것이 핵심이다.

| 표시 | 의미 | 예 |
|---|---|---|
| **[공식]** | 공식 벤치마크·모델·코드가 제공하는 값이나 절차 | 모델 체크포인트, tokenizer, 최대 길이 |
| **[방법론]** | 선행 연구에 근거한 실험 원칙 | 계층별 변동 측정, 같은 VM 안의 비교 순서 무작위화 |
| **[제안]** | 이번 연구에서 새로 정의하고 비교·반증할 설계 | IR 차이 기반 다음 shape 선택 |
| **[파일럿]** | 실제 환경에서 측정한 뒤 본실험 전에 고정할 값 | 반복 수, 워밍업 길이, VM 메모리 용량 |

임의로 넣지 않을 값: 특정 GPU/CPU 상품, 최소 RAM, 반복 30회, 워밍업 10회, 급락 기준 10%, 일정 길이의 배수 주변만 뽑은 shape 목록, 모델을 임의로 축소한 hidden size·layer 수.

## 3. 벤치마크와 모델 선정

### 3.1 핵심 실험과 독립 확인

| 역할 | 정확한 출처와 대상 | 원래 설정 | 이 연구의 사용 범위 | 진입 조건 |
|---|---|---|---|---|
| **A: 개발·탐색 실험** | ONNX Model Zoo `bertsquad-12`, FP32, SQuAD v1.1 | 예제 전처리 최대 길이 256, doc stride 128, 최대 query 길이 64, batch 1 | 원본 모델 재현 후 실제 feature의 padding 길이 변화 | 실제 ONNX 그래프의 길이 변경 가능성과 ONNX-MLIR 실행·정확도 통과 |
| **B: 독립 확인** | MLCommons Inference의 BERT-Large FP32 ONNX, SQuAD v1.1 | 최대 길이 384, doc stride 128, 최대 query 길이 64 | A에서 선택 정책을 고정한 뒤 규모가 다른 모델에서 확인 | 공식 artifact·전처리 재현, 길이 변경·정확도 통과 |
| **C: 선택적 측정 대조군** | MLPerf ResNet50-v1.5, ImageNet validation | 공식 입력 해상도 224×224 | 같은 고정 shape를 반복하여 측정 장치의 거짓 변화 검출 확인 | 데이터 접근 권한, 공식 전처리·정확도 재현 |

출처: A는 [R6,R7], B는 [R8,R9], C는 [R10,R11]. C는 핵심 shape 탐색 벤치마크가 아니다. 핵심 모델이 지원되지 않을 때 C로 바꾸어 같은 연구 질문을 충족했다고 주장하지 않는다.

**중요한 제한:** A와 B는 모두 BERT 계열이고 데이터셋도 같다. 둘의 성공은 모델 규모에 대한 제한적인 재현 근거이며, CNN·LLM decode·GPU·분산 학습 전체로의 일반화 근거가 아니다. B를 끝까지 검증하지 못하면 단일 모델 사례 연구라고 명시한다.

### 3.2 이 구성이 타당한 이유

- BERT와 SQuAD는 모델·과제의 학술적 출처가 있고, 사용할 체크포인트와 전처리 구현을 특정할 수 있다.[R12,R13]
- sequence length는 사용자 정의의 가상 축이 아니라 실제 토큰 입력의 차원이다. 단, padding을 줄여 실행하는 정책은 원래 고정 길이 벤치마크와 다른 **연구용 변형**이다.
- batch와 precision을 고정하면 길이, 배치, 정밀도, 스레드 수가 한꺼번에 변하는 혼동을 줄인다. batch=1은 A의 공식 예제에도 나타나며, 본 실험에서는 단일 요청의 길이 효과를 분리하는 범위 제한이다.[R6]
- ONNX Model Zoo 모델은 ONNX-MLIR의 공식 테스트 흐름과 연결된다. 다만 공식 테스트 문서의 BERT 예시는 특정 모델 버전에 관한 것이므로, `bertsquad-12`와 모든 변경 shape의 지원을 보장하지 않는다.[R4]

### 3.3 MLPerf라는 이름을 사용할 때의 제한

MLPerf에서 모델·데이터를 가져왔더라도 padding, 입력 shape, 실행 방식, LoadGen 사용, 측정 조건을 바꾸면 **공식 MLPerf 결과로 제시할 수 없다**. 문서·논문에는 “MLPerf BERT artifact와 SQuAD를 재사용한 shape 실험”이라고 표기한다. 공식 MLPerf 성능 점수와 본 연구의 반복 실행 평균을 같은 지표처럼 비교하지 않는다.[R9,R11]

### 3.4 처음부터 제외하는 구성

- 출처 없는 임의 MatMul 크기 목록을 전체 벤치마크로 사용하기.
- BERT의 hidden size나 head 수를 바꾸어 모델 수를 늘리기.
- ResNet의 해상도를 임의 변경한 뒤 원래 벤치마크 정확도를 인용하기.
- 대규모 GPU가 필요한 모델부터 시작하여 컴파일 정보의 효과와 자원 부족을 혼동하기.
- 서로 다른 checkpoint, tokenizer 또는 opset 변환본을 같은 모델로 취급하기.

## 4. shape와 입력을 만드는 정확한 절차

### 4.1 먼저 원본 과제를 재현한다

1. 공식 모델 파일, tokenizer/vocabulary, SQuAD v1.1 개발 데이터, 전처리·후처리 코드를 확보한다.
2. 각 파일의 SHA-256, 저장소 commit, 다운로드 출처, 라이선스를 기록한다.
3. **원래 최대 길이**로 전체 개발 데이터를 feature로 만든다. A는 256, B는 384이다. 서로의 설정을 혼용하지 않는다.[R6–R9]
4. 원래 padding 상태에서 ONNX Runtime 등 독립 참조 실행기로 예측하고, 공식 후처리·평가 코드를 사용하여 EM/F1을 재현한다.
5. 질문 ID와 생성된 window ID를 보존한다. 한 질문이 여러 window를 만들 수 있으므로 질문 수, feature 수, 고유 길이 수를 따로 보고한다.

모델 카드의 설명만 보고 입력 개수·이름·dtype을 고정하지 않는다. 실제 ONNX graph와 공식 실행 코드가 일치하는지 검사한다. A의 실행 코드는 token ID, mask, segment ID를 사용한다.[R7]

### 4.2 자연 입력 길이 집합

각 feature `x`의 유효 길이를 다음과 같이 정의한다.

```text
L(x) = input_mask에서 유효 토큰으로 표시된 위치의 수
S_observed = 정렬된 모든 고유 L(x)
```

이 정의는 공식 코드가 유효 토큰 뒤에 mask=0인 padding을 붙인다는 사실을 검사한 뒤 적용한다.[R7,R9] 특수 토큰은 그대로 포함하며, 원래 위치·segment 구분·정답 span 매핑을 보존한다.

**길이를 바꿀 때 tokenizer나 sliding window를 다시 실행하지 않는다.** 이미 생성한 feature의 뒤쪽 padding만 제거하거나 다시 붙인다. 최대 길이를 매번 바꾸어 window를 새로 만들면 입력 내용도 달라져 길이 효과를 분리할 수 없다.

`S_observed`와 그 빈도는 **SQuAD 전처리 결과의 분포**다. 실제 데이터센터 요청 분포라고 부르지 않는다.

### 4.3 통제된 padding 실험 공간 [제안]

동일한 내용으로 인접 shape를 비교하기 위해 다음 별도 실험을 구성한다.

```text
선택한 실제 feature x에 대해:
S_padding(x) = {L(x), L(x)+1, ..., 공식 최대 길이}
x_s = 원래 유효 토큰은 그대로 두고 길이 s까지 공식 padding을 붙인 입력
```

이것은 **공식 벤치마크가 정한 shape 목록이 아니라**, 실제 입력·공식 길이 한계에 근거한 통제 실험이다. 가능한 정수 길이를 모두 포함하므로 64/128/256처럼 보기 좋은 값이나 결과가 좋은 값만 선택하지 않는다.

최초 메커니즘 점검에서는 전체 범위를 넓게 덮기 위해 가장 짧은 유효 feature를 사용하고, 동률은 `(질문 ID, window ID)` 사전순으로 해소한다. 이 선택은 성능을 보기 전에 고정하는 **연구자 설계**이며 대표성의 근거가 아니다. 이후 다음 검증을 추가한다.

- 발견한 경계 `s → s+1`에서 `L(x) ≤ s`인 다른 실제 feature들로 재현한다.
- 비용 때문에 전부 검사하지 못하면 질문 단위 무작위 표본을 사용하고, 표본 크기는 입력 간 변동의 파일럿 결과로 고정한다.
- 짧은 anchor 하나에서만 확인되면 그 입력에 대한 사례로 제한한다.
- 자연 입력 정책에서 해당 길이가 실제 발생하는지 `S_observed`와 대조한다.

한 feature의 여러 padding 길이, 같은 질문의 여러 window를 독립된 모델 표본으로 세지 않는다.

### 4.4 정적 shape 특수화와 동적 실행을 분리한다

**주 실험은 shape별 정적 특수화**다. 길이 `s`마다 입력 차원을 컴파일 시점에 지정하여 실행물을 만든다. 서로 연관된 모든 입력에 동일한 batch·length를 적용한다.

ONNX-MLIR의 `RunONNXModel.py --shape-info`는 실행 입력 생성용 설정이며, 그 자체로 컴파일 시점의 정적 특수화를 보장하지 않는다. 정적 특수화는 컴파일러의 `shapeInformation` 설정과 실제 생성 IR로 확인한다.[R3]

실제 그래프가 길이 256/384에 고정된 내부 reshape 상수를 갖고 있다면 입력 메타데이터만 고쳐서는 안 된다. 다음 순서로 처리한다.

1. 공식 artifact가 길이 변경을 지원하는지 실제로 검사한다.
2. 지원하지 않으면 동일 checkpoint를 공식 export 경로에서 가변 길이로 내보낼 수 있는지 조사한다.
3. 재수출본은 exporter·opset·commit을 기록하고 원본과 의미·정확도를 검증한다.
4. 정당한 경로를 확보하지 못하면 해당 모델은 **shape 연구 진입 실패**로 기록한다. 임의 그래프 수선으로 성공 처리하지 않는다.

**일정 위험 완화 [제안]:** `bertsquad-12`는 TensorFlow 체크포인트에서 export된 오래된 artifact이므로 내부 reshape에 길이 상수가 고정되어 있을 가능성이 있다(미검증 — G1에서 실제 graph로 확인). 2번 경로(동일 체크포인트를 tf2onnx 등 공식 exporter로 가변 sequence 축 export)를 G1과 **병행** 준비하되, 원본 artifact 검사 결과가 나오기 전에는 재수출본을 주 모델로 채택하지 않는다. 재수출본을 쓰게 되면 논문에는 "ONNX Model Zoo 원본이 아닌 동일 체크포인트의 재수출본"임을 명시하고, 원본과의 출력 일치를 §7의 절차로 검증한다.

동적 shape 바이너리 하나를 여러 길이에서 실행하는 연구는 별도 실험이다. 정적 특수화의 결과와 합산하지 않는다.

## 5. 클라우드 실행 환경

### 5.1 개발 환경과 측정 환경의 역할

| 환경 | 수행할 작업 | 성능 결과로 인정하는 조건 |
|---|---|---|
| Claude Code·Codex 등이 실행되는 개발 환경 | 저장소 구성, 의존성 설치, 데이터 검증, 파서·실행 도구 개발, 기능 smoke test | 아래 측정 통제 조건을 충족하지 못하면 기능 확인 결과만 사용 |
| 식별 가능한 Linux CPU VM | 컴파일 비용 측정, 반복 추론, 환경 기록, 독립 확인 | CPU·ISA·vCPU·메모리·스레드·백그라운드 부하·할당 식별 가능 |
| 새로 할당한 동일 사양 VM | 결과가 특정 할당에만 의존하는지 확인 | 동일 이미지·artifact를 사용하고 CPU 세대 차이는 별도 층으로 분리 |

에이전트가 클라우드에서 실행된다는 이유만으로 전용 GPU나 안정적인 CPU 성능이 보장되는 것으로 가정하지 않는다. 본 단계는 **별도 HW 구매 없이 CPU VM 임대**로 가능하도록 설계한다. 비용이 0이라는 의미는 아니다.

클라우드 변동을 다룬 선행 연구는 동일 VM에서의 비교 순서 무작위화와 환경별 변동 확인의 중요성을 뒷받침한다. 그 연구의 Java/Go 결과나 검출 가능한 변화율을 BERT에 그대로 적용하지 않는다.[R15]

### 5.2 고정·기록할 항목

| 층 | 필수 기록 |
|---|---|
| VM | 공급자·리전·상품명·할당 식별자·생성 시각·vCPU·메모리·CPU 모델·NUMA·가상화 정보 |
| OS | 이미지 digest/ID·kernel·glibc·컨테이너 런타임·cgroup CPU quota·cpuset·메모리 제한 |
| 컴파일러 | ONNX-MLIR SHA·그 SHA가 요구하는 LLVM/MLIR SHA·빌드 옵션·C/C++ 컴파일러·OpenMP runtime |
| 모델 | checkpoint SHA-256·ONNX opset·입력 signature·precision·exporter·전처리 SHA |
| 실행 | CPU affinity·실제 사용 스레드·OpenMP/BLAS 환경 변수·실행 순서·seed·시계 종류 |
| 상태 | 측정 시간대·동시 작업·swap/OOM·가능하면 steal time·주파수·throttling·page fault |

권한 때문에 확인할 수 없는 항목은 `unavailable`로 남긴다. VM에서 보이는 정보만으로 물리 호스트 독점을 주장하지 않는다. 버스트 크레딧·CPU quota에 따른 제한은 반드시 확인하며, 제어할 수 없으면 연구 한계에 반영한다.

### 5.3 자원과 스레드 설정

- **RAM·디스크·빌드 병렬도 [파일럿]:** 공식 빌드를 작은 병렬도로 시작하여 peak RSS와 디스크 사용량을 측정한다. 모델 컴파일 중 OOM이 나면 자원 요구 실패로 기록한 뒤 용량을 조정한다. 특정 GB를 문헌 근거 없이 필수 사양으로 못 박지 않는다.
- **주 측정 [제안]:** 단일 추론 요청, CPU affinity 고정, 단일 실행 스레드 조건으로 길이 효과를 먼저 분리한다. 환경 변수만 설정했다고 단일 스레드가 보장된다고 하지 않고 실제 스레드 동작을 확인한다.
- **확장 [제안]:** VM에서 사용 가능한 동일 NUMA 영역의 코어 집합을 이용한 다중 스레드 조건. 단일 스레드 결과와 따로 제시한다. 물리 코어와 vCPU/SMT를 구별할 수 없으면 그 한계를 기록한다.
- compiler build, shape compile, inference timing을 동일 VM에서 동시에 수행하지 않는다. 실험 중 에이전트의 빌드·인덱싱 작업도 측정 CPU를 경쟁 사용하지 않게 한다.

컨테이너는 의존성 재현 수단이며 호스트 성능 잡음 제거 수단은 아니다.[R15,R16]

### 5.4 버전 고정과 빌드

ONNX-MLIR 공식 Linux 빌드 지침의 LLVM/MLIR 연동을 따른다.[R2] 설치 순간의 `latest` 조합을 사용하는 대신 다음 순서를 따른다.

1. ONNX-MLIR commit을 선택하고 고정한다.
2. **그 commit의** 빌드 문서·CI가 지정하는 LLVM commit 및 의존성을 확인한다.
3. 공식 Release 구성, RTTI·assertion·OpenMP 관련 요구 조건을 그대로 기록한다.
4. 문서에 있는 MLIR·ONNX-MLIR 테스트를 실행하여 빌드 자체를 검증한다.
5. Python 환경·ONNX·ONNX Runtime·NumPy 버전을 lockfile로 고정한다.
6. CPU 코드 생성 target을 명시하고 저장한다. VM CPU가 바뀌면 `native`로 만든 실행물을 무조건 재사용하지 않는다.

공식 모델 카드에 적힌 과거 ONNX Runtime 버전은 artifact 이력이지, 현재 컴파일러와의 호환성을 확인한 권장 버전 세트가 아니다.

## 6. 측정할 정보와 수집 방법

### 6.1 세 종류의 빌드/측정을 분리한다

| 종류 | 목적 | 사용할 정보 | 주 성능 숫자로 사용? |
|---|---|---|---|
| 무계측 실행물 | 모델 전체 추론 시간 | 실행 시간 원자료, 자원 상태 | **예** |
| 컴파일 정보 수집 | 탐색 정책의 feature | 최적화 보고·선택한 단계의 IR·컴파일 비용 | 추론 시간에는 사용하지 않음 |
| 진단용 계측 실행물 | 후보 원인 설명 | 연산별 시간·shape·선택적 sampling | **아니오**, 별도 진단 결과 |
| 컴파일 전용 probe [제안] | signature 변화점 탐색(§8.2b) | signature 추출 단계까지의 IR·probe 비용 | **아니오**, 실행물을 만들지 않음 |

ONNX-MLIR의 shape 프로파일과 최적화 보고를 재사용한다.[R1] MLIR pass timing은 컴파일 단계의 시간이지 추론 시간이 아니다.[R17] IR dump·파싱·추가 진단 컴파일의 시간과 저장 비용을 숨기지 않는다.

### 6.2 첫 feature 집합 [제안]

첫 구현에서는 출처를 확인할 수 있는 작은 집합만 사용한다.

| feature | 수집 근거 | 사용할 때의 주의 |
|---|---|---|
| 연산별 SIMD 적용 여부와 미적용 사유 | ONNX-MLIR 최적화 보고 | 선택한 commit에서 실제 출력되는 필드만 사용 |
| 병렬화 결정 | 지원되는 최적화 보고/IR | 단일 스레드와 다중 스레드 조건을 구분 |
| lowering 뒤 연산·loop·vector 구조의 변화 | 고정한 IR 단계 | 원래 ONNX node와 대응 관계를 보존 |
| 생성 코드 크기, 컴파일 wall time·CPU time·peak RSS | 실행물·프로세스 계측 | 성능 원인의 확정 증거로 취급하지 않음 |
| matmul/attention lowering 경로: 자체 krnl 코드 생성 vs 외부 라이브러리 호출 | 선택한 commit의 lowering 옵션, 생성 IR의 외부 call 존재 여부 | 외부 라이브러리를 호출하면 그 연산의 급락 원인은 컴파일러 결정 밖에 있으므로 별도 층으로 분리한다 (미검증 — G0에서 확인) |

`tile size`, kernel dispatch ID, occupancy 등을 모든 백엔드가 제공한다고 가정하지 않는다. 없는 정보는 결측으로 표시한다.

shape 숫자만 달라져도 IR 텍스트 hash는 달라질 수 있다. 따라서 탐색용 구조 signature는 입력 길이 literal, SSA 이름, 주소·경로·시각 등 비구조 요소를 제외하고, **실제로 의미 있는 lowering 결정**을 비교한다. 정규화 규칙과 원문을 함께 저장한다. 정규화로 중요한 차이를 지우는 위험은 원문 대조로 검토한다.

### 6.3 공식 API를 사용하는 최소 실행 형태

다음은 ONNX-MLIR Python Runtime API에 근거한 **timing 경계 예시**다. 완성된 통계 실험 코드는 아니다.[R5]

```python
from time import perf_counter_ns
from PyRuntime import OMExecutionSession

# 모델 로딩, 입력 파일 읽기, tokenizer 실행은 측정 밖에서 수행한다.
session = OMExecutionSession(shared_lib_path="model.so")
# inputs: 실제 signature와 dtype이 검증된 numpy 배열 목록

for _ in range(warmup_count):       # 파일럿 후 고정한 값
    outputs = session.run(inputs)

samples_ns = []
for _ in range(iteration_count):    # 파일럿 후 고정한 값
    start = perf_counter_ns()
    outputs = session.run(inputs)
    stop = perf_counter_ns()
    samples_ns.append(stop - start)
```

이 경계는 Python 호출과 runtime의 호출 비용을 포함한다. 출력 객체의 해제·재할당 등도 정책을 통일한다. 작은 subgraph에서 호출 비용이 지배적이면 공식 C/C++ runtime 경로를 이용한 harness를 별도로 구성하고 경계 차이를 명시한다. 서로 다른 경계의 시간을 한 그래프에서 동일 지표로 비교하지 않는다.

기본 측정은 warm steady-state의 단일 호출 지연이다. cold start, tokenizer, 모델 로딩, 서비스 큐 대기 시간은 별도 지표이며 이 결과를 서비스 p99로 표현하지 않는다.

## 7. 정확도·의미 보존 검증

성능 측정보다 먼저 다음을 통과시킨다.

1. **원본 과제 재현:** 원래 padding과 공식 후처리로 전체 SQuAD 개발 데이터의 EM/F1을 계산한다.
2. **padding 의미 보존:** 원본 길이와 변경 길이에서 참조 runtime의 유효 위치 logits, 최종 정답과 정답 span 처리 결과를 비교한다. padding 위치 출력은 길이에 따라 달라지므로 비교 범위를 명시한다.
3. **컴파일러 일치:** 같은 shape·같은 입력에서 참조 runtime과 ONNX-MLIR의 출력을 비교한다. 각 성능 확인 사례의 대안 컴파일 설정도 동일 검증을 거친다.
4. **과제 수준 확인:** 선택한 가변 padding 정책을 전체 개발 데이터에 적용했을 때 EM/F1을 다시 계산한다. 최종 발표에 쓰는 정책은 이 확인을 생략하지 않는다.

수치 오차 허용치는 선택한 upstream 테스트가 제공하는 값과 의미를 먼저 검토한다. 적절한 기준이 없으면 원래 FP32 모델·독립 참조 구현 간 차이와 과제 정확도 요구에 근거해 허용 기준을 별도 정당화하고 본실험 전에 고정한다. **느린/빠른 구현이 통과하도록 결과를 보고 tolerance를 완화하지 않는다.** 유효 토큰 logits 오차와 EM/F1은 서로 보완적인 검사이며 한쪽만으로 완전한 의미 동등성을 주장하지 않는다.[R3,R12]

컴파일 실패, 정확도 실패, timeout, OOM은 제외한 뒤 잊는 것이 아니라 전체 시도 분모와 실패 원인에 포함한다.

## 8. 탐색 방법과 공정한 비교

### 8.1 비교할 정책

아래 정책은 같은 유효 shape 집합, 초기 관측, 정확도 검사, 실행 harness를 공유한다. 균등·무작위는 최소 기준선이며, 무작위 탐색의 사용은 탐색 알고리즘 비교의 기본 원칙에 근거한다.[R21] 이 shape 문제에서 우수하다는 선행 결론을 전제하지 않는다.

| 정책 | 다음 shape를 고르는 정보 | 목적 |
|---|---|---|
| Uniform | 아직 탐색하지 않은 가장 넓은 구간의 중간 shape | 비용이 낮은 체계적 커버리지 |
| Random | 미측정 shape에서 비복원 무작위 추출 | 규칙 없는 탐색 대비 효과 |
| Shape-only | 입력 길이·대상 ISA에서 사전에 정의한 정렬 경계, 나머지는 Uniform | IR 없이도 설명되는 효과 제거 |
| Timing-only | 이미 관측한 인접 끝점의 log latency 차이, 이후 구간 폭 | 실행 결과만으로 가능한 적응 탐색 |
| **Compile-guided** | 이미 확보한 끝점의 구조 signature 차이, 이후 구간 폭 | 컴파일 정보의 추가 가치 검증 |
| **Compile-probe** [제안] | 컴파일 전용 probe로 얻은 signature만으로 변화점을 이분 탐색하고, 확정된 변화점 양쪽 shape만 실측 | H3 검증: 실측 없는 probe가 비용 우위를 주는지 |
| Timing-adaptive [선택] | 관측한 latency만으로 변화점 가능성이 큰 구간(구간별 기울기 변화 또는 GP 기반 획득 함수) | "실행시간만 쓰는 방법"의 강한 대표. 구현하지 않으면 논문의 주장 범위를 "단순 적응 기준선 대비"로 명시적으로 한정한다 |

**Shape-only의 구체화 [제안]:** 대상의 FP32 vector lane 수처럼 사전에 확인할 수 있는 정렬 단위의 배수와 바로 양옆 유효 shape를 우선한다. 우선 집합 내부는 균등 커버리지 순서로 방문한다. 정렬 단위를 확인할 수 없거나 길이 축과 연관이 없다면 해당 가정을 무효로 기록하고 Uniform을 사용한다. 관측한 성능 급락에 맞춰 경계 단위를 사후 선택하지 않는다.

### 8.2 가장 단순한 Compile-guided 정책 [제안]

학습 모델이나 임의 가중치 없이 검증 가능한 첫 버전을 정의한다.

```text
1. 유효 shape 집합의 최소·최대 끝점을 모든 정책에 공통으로 질의한다.
2. 측정된 shape를 길이순으로 정렬한다.
3. 아직 미측정 shape가 남은 인접 끝점 구간들을 만든다.
4. 다음 우선순위로 구간을 고른다.
   a. 두 끝점의 컴파일 구조 signature가 서로 다른 구간
   b. 포함된 미측정 shape 수가 많은 구간
   c. 왼쪽 끝점이 작은 구간
5. 그 구간 안 미측정 shape 목록의 가운데 값을 질의한다.
   짝수 개면 낮은 쪽 가운데 값을 사용한다.
6. 실제 누적 비용이 예산에 도달할 때까지 반복한다.
```

모든 끝점 signature가 같으면 이 정책은 구간 분할 기준선에 가까워진다. 양 끝의 signature가 같지만 중간에 다른 lowering이 존재하는 경우 놓칠 수 있다. 이 한계 자체가 실험 대상이다. 단순 정책이 충분하지 않으면 후속 연구로 발전시키되, 실패한 정책 결과도 남긴다.

비교 정책도 동일한 구간·가운데 값·동률 처리 규칙을 사용한다. Timing-only의 첫 우선순위는 `abs(log(T_right / T_left))`가 큰 구간이며, 나머지 동률 처리는 Compile-guided와 같다. 이는 단순한 실행 기반 기준선이지 최적의 black-box 탐색기라는 가정은 아니다. Random의 seed 목록과 Shape-only의 경계 순서는 본실험 전에 고정한다.

### 8.2b Compile-probe 정책 [제안]

Compile-guided는 실측한 끝점의 signature만 쓰므로 컴파일 정보를 실측과 같은 빈도로만 얻는다. 정적 특수화에서 컴파일은 모든 정책의 공통 비용이므로(§8.4), 컴파일 정보의 실질적 차별점은 **실측 없이 컴파일만 하는 probe**가 실측보다 싸다는 데서 나온다.

```text
1. 유효 shape 집합의 최소·최대 끝점을 probe하고 실측한다(모든 정책 공통).
2. 아직 probe하지 않은 shape가 남은 인접 probe 구간을 만든다.
3. 두 끝점의 구조 signature가 다른 구간을 우선하고, 동률은 §8.2의 b·c 규칙으로 해소한다.
4. 그 구간의 가운데 shape를 probe한다(컴파일 + IR 추출만, 실행·정확도 검사 없음).
5. 인접 probe 쌍의 signature가 다르고 그 사이에 미probe shape가 없으면 변화점으로 확정한다.
6. 확정된 변화점의 양쪽 shape를 실측 큐에 넣는다. 실측은 §8.3의 전체 질의 절차를 따른다.
7. probe 비용과 실측 비용을 합한 실제 누적 비용이 예산에 도달할 때까지 반복한다.
   probe 예산 비율은 파일럿의 비용비(probe : 실측)에 근거해 사전등록한다.
```

probe는 링크·실행물 생성을 생략하고 signature 추출에 필요한 단계까지만 컴파일한다. 어느 단계까지 컴파일해야 §6.2의 feature가 모두 결정되는지는 G3에서 확인하고 고정한다. probe 단계 IR의 결정과 최종 실행물의 결정이 다를 수 있으면(후속 패스가 결정을 뒤집는 경우) 그 불일치율을 측정해 보고한다.

이 정책은 signature 변화점을 찾는 데는 효율적이지만, signature가 바뀌지 않는 성능 변화는 구조적으로 놓친다. 남은 예산을 Uniform으로 채우는 hybrid 변형은 ablation(§11.2)으로만 다룬다.

### 8.3 금지할 정보 누출

- 전체 shape를 미리 컴파일하고, 그 IR을 무료로 알고 있는 탐색기로 평가하지 않는다.
- 질의하지 않은 shape의 시간·IR·최적화 결정·실패 여부를 탐색기에 제공하지 않는다.
- 평가용 exhaustive 결과를 보고 signature 정규화·threshold·초기 shape를 수정하지 않는다.
- 확인 모델 B의 결과로 정책을 튜닝한 뒤 B를 held-out이라고 하지 않는다.

질의는 기본적으로 `shape 선택 → 컴파일 및 정보 수집 → 정확도 검사 → 실행 측정`이다. 컴파일만 하는 probe를 추가한다면 그것도 비용과 질의 이력에 넣는다.

### 8.4 비용 회계

주 비교는 **새 모델에 cold-start로 적용하는 상황**이다.

```text
C_method = 선택 로직 + 질의별 컴파일 + 필요한 IR/보고서 생성·파싱
           + 정확도 검사 + 워밍업 + 측정 + 실패한 시도
```

wall time을 주 예산으로 사용하고 CPU time, peak RSS, artifact 용량을 함께 보고한다. 같은 수의 shape만 비교하는 그래프는 보조 분석이다. 방법마다 필요 없는 IR 출력까지 강제로 생성하여 기준선을 불리하게 만들지 않는다.

**컴파일 비용의 귀속 [제안]:** 정적 특수화에서는 어느 정책이든 실측할 shape를 컴파일해야 하므로, 실측 질의의 컴파일 비용은 모든 정책에 공통이다. Compile-guided의 한계 비용은 IR 덤프·파싱·signature 계산이고, Compile-probe의 한계 비용은 실측하지 않는 shape에 대한 probe 컴파일이다. 비용 곡선에는 (i) 공통 컴파일 비용과 (ii) 정책별 추가 비용을 분리해 표시한다. "컴파일 비용 때문에 진다"는 결론은 (ii)에 근거해야 하며, (i)를 컴파일 정보 기반 정책에만 귀속하지 않는다.

공통 compiler toolchain 구축·모델 다운로드 비용은 별도 초기화 비용으로 공개한다. 특정 정책만 요구하는 추가 구축·전처리는 해당 정책 비용에 포함한다. 평가자가 정답표를 만들기 위해 수행한 exhaustive 측정 비용도 전체 연구 비용에 별도 공개한다.

이미 컴파일된 artifact를 재사용하는 CI 상황은 별도의 warm-cache 시나리오로만 평가한다. 기존 artifact가 항상 존재한다고 가정하여 주 결과에서 컴파일 비용을 삭제하지 않는다.

예산 지점에서는 **그 시각까지 끝난 질의·확인만** 발견 수에 반영한다. 마지막 질의가 예산을 넘기면 초과 비용도 기록하고, 그 질의의 결과를 더 작은 예산의 성과에 소급하지 않는다. 경계 사건은 두 끝점의 관측과 사전 정의한 확인 절차를 완료했을 때 발견으로 센다. 후보 생성 시점과 확인 완료 시점을 모두 저장한다.

### 8.5 규모 참고치 [파일럿에서 대체]

정답표 A는 길이 축 전체(A: 1–256, B: 1–384)의 조밀 컴파일·측정을 요구한다. ONNX-MLIR 논문의 POWER9 참고치는 ResNet50 컴파일 약 7.7초, 추론 약 7.5초다[R23]. BERT-base의 x86 컴파일 시간은 미검증이며 G3에서 실측한다. 컴파일이 수십 초 규모라면 256회 컴파일은 수 시간, 길이당 수십 회 반복 측정은 수십 분 규모로 CPU VM 하나에서 감당 가능하다고 추정한다[본인 분석·판단]. 이 추정은 예산 편성용이며 결과 해석이나 반복 수 결정에 쓰지 않는다.

## 9. 성능 이상 판정과 독립 정답표

### 9.1 먼저 연속 지표를 보고한다

동일한 실제 feature를 padding한 입력에 대해 `T_c(s)`를 설정 `c`, 길이 `s`의 기대 warm latency라고 하자. 완전한 정수 길이 공간에서는 다음을 계산한다.

```text
인접 길이 변화:       A_c(s) = T_c(s+1) / T_c(s)
같은 shape 대안 효과: R(s)   = T_default(s) / T_alternative(s)
경계에서 효과 변화:   Q(s)   = R(s+1) / R(s)
```

`A`는 계산량 증가도 포함한다. `R>1`은 같은 shape에서 대안이 빠르다는 뜻이지만 모든 길이에 공통된 최적화일 수도 있다. `Q`와 전체 곡선은 그 효과가 특정 경계에 집중되는지 확인하는 보조 지표다. 이 수식은 이번 연구의 분석 정의이며 기존 벤치마크의 표준 지표가 아니다.

### 9.2 threshold를 만드는 원칙

“10% 느리면 버그” 같은 보편 기준을 넣지 않는다.

- 실제 배포 환경의 latency/cost 요구가 있으면 그 요구로 최소 관심 효과 `δ`를 정하고 출처를 기록한다.
- 그런 요구가 없으면 다양한 효과 크기에 대한 **검출·비용 곡선과 신뢰구간**을 주 결과로 제시한다.
- 특정 이진 사건 수가 필요하면 파일럿의 측정 정밀도와 연구 목적을 근거로 `δ`를 사전등록하고 여러 `δ`에 대한 민감도 분석을 병기한다. 이를 산업적 중요도의 증거로 쓰지 않는다.
- 통계적으로 0과 구별되는 작은 차이와 실용적으로 중요한 차이를 구별한다.

따라서 정확한 recall/precision을 먼저 발표하려면, 무엇을 정답 사건으로 셀지 본실험 전에 확정해야 한다.

### 9.3 두 단계 정답 정의

**정답표 A — 재현 가능한 shape 성능 변화:** 평가자만 접근하는 조밀한 길이 측정에서 후보를 만들고, 별도 실행·비교 순서·VM 할당에서 효과와 불확실성을 확인한다. IR signature를 정답의 필수 조건으로 삼지 않는다. 그렇지 않으면 IR 기반 정책에 유리한 순환 평가가 된다.

**정답표 B — 피할 수 있는 저하/회귀:** A의 후보에 대해 다음 증거 중 해당하는 것을 추가한다.

- 동일 shape에서 문서화된 대안 최적화 설정과 비교하고 정확도를 확인한다.
- 원인으로 의심한 최적화를 켜고 끄는 개입을 하되, 실제 commit에서 지원하는 flag만 사용한다.
- 회귀 주장이라면 고정한 이전 compiler revision과 동일 조건으로 비교한다.
- 연산 수준 진단과 무계측 모델 전체 실행을 모두 확인한다.

`-O3`와 `-O0`처럼 공식적으로 존재하는 설정 비교는 정확도·진단 경로의 출발점일 수 있지만, `-O0`가 경쟁력 있는 최적 대안이라는 가정은 하지 않는다.[R3] 진단 중 발견한 대안은 독립 확인에서 다시 평가하고 탐색 예산에 필요한 진단 비용을 공개한다.

정답표 B가 완전하지 않다면 “모든 성능 버그에 대한 recall”을 주장할 수 없다. **유한한 사전 정의 공간의 확인된 사건에 대한 recall** 또는 확인된 발견 수로 제한한다. exhaustive 평가를 감당하지 못하면 부분 정답표임을 명시한다.

### 9.4 IR 신호의 정밀도와 정렬 경계 분해 [제안]

census(§11 G2.5)로 얻은 signature 변화점 집합을 `C_sig`, 사전에 정의한 정렬 단위 배수 경계 집합을 `B_align`(§8.1 Shape-only와 동일 정의), 정답표 A 사건 집합을 `E_A`라 하자.

```text
비분석적 변화점:      C_nonalign = C_sig \ B_align
IR 신호 정밀도:       Prec_sig   = |C_sig ∩ E_A| / |C_sig|
IR 신호 재현율:       Rec_sig    = |C_sig ∩ E_A| / |E_A|
정렬 밖 기여:         |C_nonalign ∩ E_A| / |E_A|
```

변화점과 사건의 일치 판정에 쓰는 길이 허용 범위(예: ±0 또는 ±1)는 사전등록한다. `Prec_sig`가 낮으면 Compile-guided/Compile-probe는 성능과 무관한 IR 변화에 예산을 쓴다. `C_nonalign ∩ E_A`가 비어 있으면 H1·H2가 동시에 기각되며 Shape-only가 컴파일 정보 없이 같은 사건을 찾는다. 이 지표들은 탐색 정책과 무관하게 IR 정보 자체의 가치를 보고하며, 정책 비교(§11.1)와 별도로 제시한다.

## 10. 반복·무작위화·통계 분석

### 10.1 독립성의 단위

| 수준 | 예 | 처리 |
|---|---|---|
| VM 할당 | 같은 상품의 서로 다른 할당 | 할당 간 변동과 일반화 범위 확인 |
| 프로세스/시간 블록 | 새 프로세스, 다른 실행 시간대 | 비교군을 같은 블록에서 무작위 순서로 실행 |
| 반복 호출 | 한 프로세스 안의 추론 반복 | 프로세스 안 변동 추정 |
| 입력 질문 | 다른 SQuAD 질문 | 입력 의존성 확인, window는 질문에 묶음 |

한 VM·한 프로세스에서 1,000번 실행해도 독립 VM 표본 1,000개가 되지 않는다. 반복 수는 변동이 발생하는 수준에 배분한다.[R14] 실행 순서와 환경 배치가 결과를 편향시킬 수 있으므로 비교 순서·seed·블록을 기록한다.[R16]

### 10.2 파일럿에서 결정하고 본실험 전에 잠그는 값

1. 원본 길이와 성능을 보지 않고 고른 길이들에서 워밍업 추이를 수집한다.
2. iteration·process·allocation 수준의 분산과 각 반복의 비용을 추정한다.
3. 요구하는 효과 추정 정밀도에 맞춰 반복 수를 배분한다. Kalibera–Jones의 비용·변동 기반 접근을 참조한다.[R14]
4. 선택한 반복 수·워밍업·블록 구성·효과 기준·분석 코드를 사전등록한다.
5. 본실험은 고정된 계획으로 실행한다. 유의해질 때까지 통상적인 신뢰구간을 반복 계산하며 중단하는 방식은 사용하지 않는다.

파일럿과 확인 실험을 구분하고, 파일럿에서 발생한 실패·설정 변경을 기록한다. 안정 구간을 확보하지 못하면 숫자를 평균내어 안정적인 것으로 포장하지 않는다.

### 10.3 보고할 통계

- shape별 원자료, 평균 latency, 분산 요약, 효과 비율의 신뢰구간.
- 같은 VM/시간 블록의 짝지어진 비교를 유지한 효과 추정.
- 계층적 bootstrap 등을 사용할 때 실제 표본 구조에 맞는 재표집 단위 명시.
- 무작위 탐색은 여러 독립 seed의 분포 및 불확실성 보고. 반복 seed 수 역시 파일럿으로 결정.
- 다수 경계에서 개별 유의성 검정을 한다면 하나의 검정 family를 사전 정의하고 Holm 등의 다중검정 보정을 적용.[R22]
- 95% 같은 신뢰수준은 관례에 따른 **분석 선택**으로 명시한다. 개별 신뢰구간과 다중검정 보정 결과를 혼동하지 않는다.

최솟값만 골라 보고하거나 큰 latency 표본을 임의 삭제하지 않는다. OOM·quota 초과 등 제외 규칙이 필요하면 성능 결과를 보기 전에 고정하고 제외 전후 결과를 공개한다. 공식 도구의 예제 반복 수나 기본 quartile-trimmed 통계는 논문용 반복 설계의 근거로 사용하지 않는다.

## 11. 실험 실행 순서와 통과 기준

| 단계 | 작업 | 산출물 | 다음 단계 진입 조건 |
|---|---|---|---|
| **G0: 환경 고정** | CPU VM 식별, compiler/LLVM 연동 빌드, 공식 테스트 | 환경 manifest, lockfiles, build logs | 요구 정보 기록, 빌드·기능 테스트 통과 |
| **G1: 원본 재현** | A artifact·전처리·SQuAD 평가 재현 | artifact hash, reference EM/F1, signatures | 원본 실행과 출처 확인 |
| **G2: shape 적합성** | padding 변형, 정적 특수화, 정확도 확인 | shape catalog, correctness results | 유효 길이 변경 지원, 의미 보존 |
| **G2.5: signature census** [신설] | 평가자 전용으로 전 유효 길이를 signature 추출 단계까지 컴파일, 변화점 수·위치·정렬 배수 여부 계수, matmul 경로 확인 | census 표(`C_sig`, `B_align`, `C_nonalign`), 정규화 규칙과 원문 IR | `C_nonalign`이 비어 있지 않음(H1). 비어 있으면 §14에 따라 중단 또는 축소. census 결과는 탐색기와 격리 보관 |
| **G3: 측정 파일럿** | warmup·계층별 변동·메모리·컴파일 비용, probe : 실측 비용비, probe 단계 결정과 최종 실행물 결정의 불일치율 측정 | pilot report, 고정된 본실험 계획 | 신뢰할 측정 가능, 자원·예산 추정 가능, probe 단계 확정 |
| **G4: 유한 공간 평가** | 평가자용 조밀 측정과 독립 확인 | 정답표 A, 가능한 정답표 B | 원자료·판정 기준·불확실성 공개 |
| **G5: 탐색 비교** | 정책별 동일 자원·실제 비용 비교, ablation | 비용별 발견 곡선, 실패·비용 회계 | IR 정보 누출 없음, census 접근 차단 증명, 기준선 공정성 확인 |
| **G6: 외적 확인** | 정책 고정 후 B 또는 독립 compiler revision에서 반복 | 독립 결과와 실패 사례 | 확인 데이터로 재튜닝하지 않음 |
| **G7: 원인·효용 분석** | 개입·연산 진단·모델 전체 효과·중복 원인 통합 | 사례별 evidence bundle | 설명과 인과 주장 수준 구별 |

G2.5의 census는 §8.3 누출 금지의 예외가 아니라 평가자 측 자산이다. 탐색기(`selectors/`)는 census 파일에 접근할 수 없어야 하며, 접근 차단을 코드 수준(별도 디렉터리·권한)과 테스트로 증명한다. G2에서 막히면 대규모 벤치마크를 예약하지 않는다. G3에서 잡음 또는 컴파일 비용이 예산을 지배하면 실험 범위를 조정하고 변경 이유를 기록한다. G5에서 기준선과 차이가 없으면 개선 효과가 없는 결과로 보고한다.

### 11.1 평가 지표

| 주/보조 | 지표 | 해석 |
|---|---|---|
| 주 | 누적 실제 비용에 따른 독립 확인 사건 수 | 비용 대비 발견 효율 |
| 주 | 특정 확인 사건을 찾기까지의 비용 분포 | 탐색 속도·안정성 |
| 주 | 전체 유한 정답표가 있을 때의 recall–cost | 놓친 사건까지 포함한 평가 |
| 보조 | 후보 중 독립 확인 비율 | 오탐·재확인 부담 |
| 보조 | 원인별로 중복 제거한 발견 수 | 같은 lowering 현상의 과다 집계 방지 |
| 보조 | 정확도 통과율·컴파일 실패율·peak RSS | 적용 가능성과 부대 비용 |
| 보조 | 자연 길이 분포로 가중한 latency 효과 | SQuAD 유래 정책에서의 영향, 실제 서비스 효과는 아님 |
| 주 | `Prec_sig`, `Rec_sig`, 정렬 밖 기여 (§9.4) | IR 정보 자체의 가치. 정책 비교와 독립 |
| 보조 | probe : 실측 비용비, probe 단계 결정과 최종 실행물 결정의 불일치율 | Compile-probe의 전제 조건(H3) |

전체 곡선을 제공하고 유리한 단일 예산 지점만 선택하지 않는다. 효과가 큰 한 사례와 정책의 평균 우수성은 별개의 주장이다.

### 11.2 최소 ablation

1. Compile-guided에서 컴파일 구조 정보를 제거한 Uniform/Timing-only 비교.
2. 컴파일 정보 비용을 포함한 주 결과와, 비용을 가상으로 제외한 보조 결과의 차이.
3. raw IR hash와 구조 signature의 차이: 단순 shape literal 변화가 정보처럼 작동하는지 확인.
4. 단일 feature의 controlled sweep과 다른 실제 입력에서의 재현 비교.
5. Compile-probe와 Compile-guided의 비교: probe의 이득이 실측 빈도 제한 때문인지 확인.
6. `C_nonalign`을 제거한 Compile-guided(정렬 배수 변화점만 사용)와 원래 정책의 비교: Shape-only 대비 이득이 비분석적 변화점에서 나오는지 확인.

학술적 핵심은 1·2·6이다. 비용을 포함하면 우위가 사라지는 결과도 중요한 반증이며 숨기지 않는다.

## 12. 구현 산출물과 데이터 스키마

아래 파일·명령 이름은 **앞으로 구현할 연구 저장소의 제안 인터페이스**다. ONNX-MLIR에 이미 존재하는 공식 명령이 아니다.

| 파일/모듈 | 책임 |
|---|---|
| `environment.lock.json` | image·compiler·LLVM·Python·target·VM 정보 |
| `artifacts.lock.json` | 모델·데이터·tokenizer·전처리 URL·SHA·라이선스 |
| `prepare_features.py` | 공식 전처리 호출, 질문/window 추적, shape catalog |
| `validate_shapes.py` | 변경 길이의 그래프 적합성·참조 출력·과제 정확도 |
| `compile_shape.py` | 검증된 CLI 구성, 실행물·IR·최적화 보고·비용 수집 |
| `measure.py` | 계측 없는 고정 경계 추론, 환경·순서·원자료 저장 |
| `selectors/` | Uniform·Random·Shape-only·Timing-only·Compile-guided |
| `evaluate.py` | 탐색과 격리된 정답표 접근, 효과 추정·비용 곡선 |
| `preregistration.md` | 파일럿 이후 고정한 모든 숫자·제외 규칙·정답 정의 |
| `census/` | 평가자 전용 signature census 결과와 `B_align` 정의. `selectors/`에서 import·읽기 금지, 차단 테스트 포함 |
| `report.md` | 결과·실패·한계·재현 방법 |

모든 기록은 최소 다음 key로 결합 가능해야 한다.

```text
run_id, experiment_phase, model_hash, preprocessing_commit,
question_id, window_id, valid_length, padded_length, batch, dtype,
compiler_commit, llvm_commit, target, compile_flags,
vm_allocation_id, process_id, block_id, thread_config, seed,
selector, query_index, correctness_status,
compile_wall_ns, probe_wall_ns, feature_extract_wall_ns, verify_wall_ns,
warmup_wall_ns, measurement_wall_ns, cumulative_cost_ns,
iteration_index, latency_ns, peak_rss_bytes,
artifact_hash, ir_signature, signature_stage, matmul_path, failure_type
```

raw timing과 환경 logs는 append-only로 유지한다. 파생 통계는 언제든 원자료에서 재생성할 수 있어야 한다. 질문/window와 timing 식별자를 분리해도 결합 키는 보존한다.

## 13. 클라우드 코딩 에이전트에 전달할 작업 지시문

아래 내용을 이 명세와 함께 Claude Code·Codex 등에 전달할 수 있다.

```text
이 명세의 G0~G3을 먼저 구현하라. 아직 성능 개선을 달성했다고 가정하지 말라.

1. ONNX-MLIR과 그 commit에 대응하는 LLVM을 공식 빌드 지침에 따라 고정한다.
   latest 의존성을 무조건 혼합하지 말고 lockfile과 환경 manifest를 만든다.
2. ONNX Model Zoo bertsquad-12 FP32와 공식 SQuAD v1.1 전처리를 확보한다.
   출처·commit·SHA-256을 기록하고 실제 graph의 signature를 검사한다.
3. 공식 최대 길이 256에서 원본 정확도를 먼저 재현한다.
4. 원래 생성된 feature의 trailing padding만 바꾸는 shape catalog를 만든다.
   고정 reshape 상수를 임의 수정하지 않는다. 지원 실패는 명확히 보고한다.
5. --shape-info와 compiler shapeInformation을 구별한다.
   실제 설치된 버전의 --help와 생성 IR로 정적 특수화를 확인한다.
6. 무계측 timing 실행물과 진단용 profile 실행물을 분리한다.
   PyRuntime session 로딩과 tokenizer를 timing 밖에 둔다.
7. 공식 optimization report와 IR에서 실제 제공되는 field만 추출한다.
   존재하지 않는 tile/dispatch 정보를 추정해 채우지 않는다.
8. 임의 반복 횟수·급락 비율·VM 최소 용량을 본실험 설정으로 넣지 않는다.
   파일럿으로 변동·비용·자원 사용을 측정하고 미확정값을 보고한다.
9. 개발 환경이 측정 통제 조건을 충족하지 않으면 smoke test만 수행하고,
   통제 가능한 CPU VM에서 실행할 동일 이미지·스크립트를 준비한다.
10. G3 통과 후에 정책·예산·통계 계획을 고정하고 G4~G7을 진행한다.
    탐색기는 미질의 shape의 IR/시간이나 평가 정답표를 읽을 수 없어야 한다.
11. correctness 실패, OOM, timeout을 삭제하거나 성공으로 대체하지 않는다.
    테스트를 통과시키기 위해 reference output·tolerance를 사후 변경하지 않는다.
12. 실행하지 않은 단계는 미실행으로 표시한다. 그래프에 예시 숫자를 넣어
    실측 결과처럼 보고하지 않는다. 비용을 발생시키는 VM 작업은 별도 실행 설정으로 둔다.
13. G0에서 선택 commit의 matmul/attention lowering 경로(자체 코드 생성 vs 외부 라이브러리 호출)를
    확인해 environment.lock.json에 기록한다. 추정하지 않는다.
14. G2 통과 후 G3 전에 signature census를 수행한다: 전 유효 길이를 signature 추출 단계까지만
    컴파일하고, 변화점 목록·정렬 배수 여부·정규화 규칙·원문 IR을 census/ 아래에 저장한다.
    이 디렉터리는 evaluate.py만 읽을 수 있게 하고 selectors/에서의 접근 차단을 테스트로 증명한다.
15. census에서 정렬 배수 밖 변화점이 0이면 G4 이후를 진행하지 말고 그 사실과 census 표를 보고한다.
```

실제 구축 시에는 `G0 완료`, `G1 완료`처럼 통과한 단계와 증거 파일을 보고하게 한다. “코드 작성 완료”와 “모델 실행·정확도·측정 타당성 확인 완료”를 구별한다.

## 14. 연구를 계속할지 판단하는 기준

### 계속할 근거

- 독립 확인되는 성능 현상이 실제로 존재한다.
- IR이 단순 길이·정렬 경계·과거 timing보다 유용한 추가 정보를 제공한다.
- 컴파일·파싱·확인 비용을 포함해도 발견 효율이 개선된다.
- census에서 정렬 배수 밖 변화점(`C_nonalign`)이 존재하고, 그 일부가 정답표 A와 일치한다(H1·H2).
- probe 비용이 실측 비용보다 충분히 작아 Compile-probe가 Compile-guided를 앞선다(H3).
- 모델 전체에서 영향이 확인되고 입력·VM 재할당에 대해 재현된다.

### 축소하거나 중단할 근거

- 실제 graph의 shape 변경을 정당하게 지원할 수 없다.
- 구조 signature가 거의 변하지 않거나 모든 길이에서 기계적으로 달라져 탐색 신호가 없다.
- census의 `C_nonalign`이 비어 있다(H1 기각). 이때 남는 결과는 "이 컴파일러·모델에서 shape 결정은 정렬 배수로 완전히 예측된다"는 관찰이다.
- probe 비용이 실측 비용과 같은 자릿수라 Compile-probe가 Compile-guided와 구별되지 않는다(H3 기각).
- matmul/attention이 외부 라이브러리 호출로 lowering되어 주요 급락이 컴파일러 결정 밖에서 발생한다.
- 단순 Shape-only 또는 Uniform이 같은 예산에서 동등하거나 더 낫다.
- 컴파일 비용이 절약한 측정 비용보다 크다.
- 후보가 정상 연산량 증가·클라우드 잡음으로 설명되고 피할 수 있는 저하가 없다.
- 한 입력·한 VM에서만 나타나며 독립 확인에 실패한다.

이 경우에도 재현 가능한 shape 성능 데이터와 측정 방법 분석은 남을 수 있다. 다만 그것만으로 강한 탐색 기법 논문이 된다고 보장하지 않는다.

## 15. 레퍼런스와 적용 범위

확인 기준일: 2026-09-28. `main/master` 문서는 변경될 수 있으므로 실제 실험에서는 해당 문서·코드도 commit으로 고정해야 한다. 아래 출처는 벤치마크·방법론의 근거이며, 본 연구가 제안한 탐색 정책의 유효성을 대신 증명하지 않는다.

| ID | 1차 출처 | 뒷받침하는 내용 / 적용 한계 |
|---|---|---|
| **R1** | ONNX-MLIR, [Performance Testing](https://onnx.ai/onnx-mlir/PerformanceTesting.html) | 기존 profiling·shape signature·optimization report. 자체 기능 구현을 신규 기여라고 할 수 없음 |
| **R2** | ONNX-MLIR, [Build on Linux/OSX](https://onnx.ai/onnx-mlir/BuildOnLinuxOSX.html) | LLVM/MLIR 연동과 빌드 요구. 실제 선택 commit의 지침이 우선 |
| **R3** | ONNX-MLIR, [Debugging Numerical Error](https://onnx.ai/onnx-mlir/DebuggingNumericalError.html) | 참조 출력 비교, compile option 비교, runtime shape와 compile-time shape 구별 |
| **R4** | ONNX-MLIR, [Testing](https://onnx.ai/onnx-mlir/Testing.html) | Model Zoo 기반 테스트 흐름. 모든 opset·shape 지원 보장은 아님 |
| **R5** | ONNX-MLIR, [Using Python Runtime](https://onnx.ai/onnx-mlir/UsingPyRuntime.html) | `OMExecutionSession`과 모델 실행 API |
| **R6** | ONNX Model Zoo, [bertsquad-12 모델 카드](https://huggingface.co/onnxmodelzoo/bertsquad-12), [원본 README](https://github.com/onnx/models/blob/main/validated/text/machine_comprehension/bert-squad/README.md) | FP32 artifact, 256/128/64/batch 1의 공식 예제. 가변 padding 평가 자체는 본 연구의 변형 |
| **R7** | ONNX Model Zoo, [run_onnx_squad.py](https://github.com/onnx/models/blob/main/validated/text/machine_comprehension/bert-squad/dependencies/run_onnx_squad.py) | feature/window 생성, right padding, mask, 실제 입력 feed와 후처리 |
| **R8** | MLCommons, [BERT benchmark 문서](https://docs.mlcommons.org/inference/benchmarks/language/bert/), [reference README](https://github.com/mlcommons/inference/blob/master/language/bert/README.md) | BERT-Large FP32·SQuAD·공식 artifact 경로·추론 경계 |
| **R9** | MLCommons, [squad_QSL.py](https://github.com/mlcommons/inference/blob/master/language/bert/squad_QSL.py), [create_squad_data.py](https://github.com/mlcommons/inference/blob/master/language/bert/create_squad_data.py) | 최대 길이 384와 전처리·windowing·padding 구현 |
| **R10** | MLCommons, [ResNet50 benchmark](https://docs.mlcommons.org/inference/benchmarks/image_classification/resnet50/), [ImageNet 데이터 준비](https://docs.mlcommons.org/inference/benchmarks/image_classification/get-resnet50-data/) | 선택적 고정 shape 대조군의 공식 모델·데이터 |
| **R11** | MLCommons, [Inference Rules](https://github.com/mlcommons/inference_policies/blob/master/inference_rules.adoc) | 정확도·실행·벤치마크 준수 조건. 본 연구의 변형은 공식 MLPerf 점수가 아님 |
| **R12** | Rajpurkar et al., 2016, [SQuAD: 100,000+ Questions for Machine Comprehension of Text](https://aclanthology.org/D16-1264/), EMNLP | 데이터셋·과제의 학술적 근거 |
| **R13** | Devlin et al., 2019, [BERT: Pre-training of Deep Bidirectional Transformers for Language Understanding](https://aclanthology.org/N19-1423/), NAACL | 모델 계열의 학술적 근거. 개별 ONNX artifact는 R6/R8로 특정 |
| **R14** | Kalibera & Jones, 2013, [Rigorous Benchmarking in Reasonable Time](https://kar.kent.ac.uk/33611/), ISMM, DOI [10.1145/2464157.2464160](https://doi.org/10.1145/2464157.2464160) | 반복 수준별 변동·비용·정밀도와 효과 신뢰구간. 고정 반복 횟수를 제공하는 근거가 아님 |
| **R15** | Laaber, Scheuner & Leitner, 2019, [Software Microbenchmarking in the Cloud. How Bad Is It Really?](https://joelscheuner.com/publication/laaber-19-emse/), Empirical Software Engineering, DOI [10.1007/s10664-019-09681-1](https://doi.org/10.1007/s10664-019-09681-1) | 클라우드 잡음과 동일 인스턴스 비교 방법론. Java/Go 실험의 수치를 ML 추론에 전용하지 않음 |
| **R16** | Mytkowicz et al., 2009, [Producing Wrong Data Without Doing Anything Obviously Wrong!](https://sape.inf.usi.ch/publications/asplos09.html), ASPLOS, DOI [10.1145/1508244.1508275](https://doi.org/10.1145/1508244.1508275) | 환경과 실험 배치에 의한 측정 편향 |
| **R17** | LLVM/MLIR, [Pass Infrastructure](https://mlir.llvm.org/docs/PassManagement/) | pass instrumentation·IR 출력·컴파일 시간 관측. 추론 시간과 구별 |
| **R18** | FSE 2025, [Directed Testing in MLIR: Unleashing Its Potential by Overcoming the Limitations of Random Fuzzing](https://conf.researchr.org/details/fse-2025/fse-2025-research-papers/56/Directed-Testing-in-MLIR-Unleashing-Its-Potential-by-Overcoming-the-Limitations-of-R) | MLIRTracer 관련 선행 연구. 방향성 있는 MLIR 테스트 자체의 신규성 제한 |
| **R19** | Zhang et al., [CITADEL: Context Similarity Based Deep Learning Framework Bug Finding](https://arxiv.org/abs/2406.12196), arXiv:2406.12196, v5 (2025) | 기존 버그와 context가 유사한 API에 oracle을 이전하는 접근. 성능 버그도 다루며 본 연구와 직접 동일한 shape 탐색 문제는 아님 |
| **R20** | Oliveira et al., [Perphecy: Performance Regression Test Selection Made Simple but Effective](https://sape.inf.usi.ch/publications/icst17.html), ICST 2017, DOI [10.1109/ICST.2017.17](https://doi.org/10.1109/ICST.2017.17) | 성능 변화 탐지를 위한 테스트 선택이라는 기존 연구 방향 |
| **R21** | Bergstra & Bengio, 2012, [Random Search for Hyper-Parameter Optimization](https://jmlr.org/papers/v13/bergstra12a.html), JMLR 13:281–305 | 무작위 탐색 비교의 방법론적 참고. shape 탐색용 최적 알고리즘이라는 근거는 아님 |
| **R22** | Holm, 1979, [A Simple Sequentially Rejective Multiple Test Procedure](https://www.ime.usp.br/~abe/lista/pdf4R8xPVzCnX.pdf), Scandinavian Journal of Statistics 6(2):65–70 | 다수 가설 검정의 family-wise error 통제 |
| **R23** | Jin et al., 2020, [Compiling ONNX Neural Network Models Using MLIR](https://arxiv.org/abs/2008.08272), arXiv:2008.08272 (저자 표기·수치는 2차 요약 경유, 원문 대조 필요 — 미검증) | ONNX-MLIR 설계와 POWER9 컴파일·추론 시간 참고치(§8.5). 예산 편성용 |
| **R24** | [FTuner: A Fast Dynamic Shape Tensors Program Auto-Tuner for Deep Learning Compilers](https://arxiv.org/abs/2407.21418), arXiv:2407.21418 (2024) | 패딩·경계 검사에 의한 성능 저하와 정렬 단위 배수 근처 타일 규칙. Shape-only 기준선이 강력한 이유의 문헌 근거. GPU 튜너 대상이므로 수치를 CPU에 전용하지 않음 |
| **R25** | [TileBench: A Controlled Benchmark for Performance Evaluation and Bottleneck Diagnosis of Tile-Based Programming Models](https://arxiv.org/abs/2609.29067), arXiv:2609.29067 (2026) | 생성 명령어가 dtype·타일 shape·레이아웃·컴파일러 결정에 의존한다는 관찰. 컴파일 결정을 예측 변수로 삼는 근거이나 GPU 타일 언어 대상이므로 ONNX-MLIR CPU에 직접 전용하지 않음 |

## 16. 착수 시점에 아직 확정하면 안 되는 사항

| 미확정 사항 | 확정 근거 |
|---|---|
| compiler/LLVM/Python 패키지의 정확한 버전 조합 | G0 빌드·공식 테스트 |
| A/B artifact의 가변 길이 적합성 | G1–G2 실제 graph·실행·정확도 검사 |
| VM 상품과 RAM·디스크·빌드 병렬도 | G3 peak RSS·시간·예산 |
| warmup·반복 수·VM 재할당 수 | G3 계층별 분산·목표 정밀도 |
| 의미 있는 성능 차이 기준 | 실제 요구 조건 또는 사전등록한 연구 목적·민감도 분석 |
| 탐색에 유용한 IR feature가 존재하는지 | 실제 report/IR 관찰과 ablation |
| signature 변화점이 정렬 배수 밖에 존재하는지(H1) | G2.5 census |
| probe 컴파일 단계와 probe : 실측 비용비(H3) | G3 파일럿 |
| matmul/attention lowering이 자체 생성인지 외부 호출인지 | G0 commit 옵션·IR 확인 |
| 성능 개선 폭·발견 가능한 bug 수·논문 게재 가능성 | 본실험 전에는 미확정 |

**권장 착수 범위:** A 모델의 원본 재현과 shape 적합성, 계측 없는 CPU timing, 컴파일 비용을 포함한 파일럿까지 먼저 구축한다. 이 단계의 증거로 전체 탐색 실험을 수행할 가치와 비용을 판단한다.
