# 보고서

**상태: G2.5 census에서 중단 (2026-09-28, 연구자 결정).** 명세 §13 항목 15와 §14에 따라 G4 이후는 진행하지 않는다.
게이트별 진행과 증거는 `docs/STATUS.md`에 있다. 이 문서는 결과·실패·한계·재현 방법을 기록한다.

## G2.5 signature census 결과와 중단 결정

**범위와 조건.** 모델 A(`bertsquad-12`와 같은 가중치의 가변 길이 재수출본), batch 1, FP32, ONNX-MLIR v0.5.1.1(`1e017c9f`),
flag set `default` = `-O3 --march=x86-64 --mcpu=emeraldrapids` + `--opt-report=Simd`, probe 단계(`--EmitMLIR`).
유효 길이 41–256 전부(216개; 41은 anchor feature의 자연 길이). 컴파일 실패 0, 손상 보고 0, IR 누락 0.
정규화 규칙 `sig-v3`(4차 검토 뒤 버전을 올려 전 길이를 다시 컴파일했고, `sig-v2` 표기로 먼저 얻은 결과와 같다).
원문 probe IR은 변화점 길이(IR 구조 기준으로는 전 길이)마다 평가자 전용 `census/` 아래에 gzip으로 보관했다(583 MB, git 제외).
개발 컨테이너에서 실행했고 사전등록은 고정되지 않았다. 컴파일 결정은 결정적이며, ONNX-MLIR는 `--march`가 주어지면
자체 결정에 `--mcpu`를 쓰지 않는다(G0 경고 확인). 따라서 이 표는 측정 VM과 같을 가능성이 높지만, VM에서 확인하지는 않았다.

| signature | 변화 경계 수 (215개 중) | 정렬 밖 변화 `C_nonalign` (u=8 / 16 / 32) |
|---|---|---|
| **opt-report (주 signature, sig-v2)** | **0** | **0 / 0 / 0** |
| IR 구조 (보조) | 215 | 162 / 188 / 202 |
| 원문 IR 해시 (ablation 11.2-3) | 215 | — |

증거: `results/g2_5/census_dev_table.json`, `results/g2_5/census_dev_analysis.json`(`scripts/analyze_census.py`로 재생성).

**관찰.**

1. **SIMD 결정은 길이와 무관하다.** 길이마다 582개 보고가 나오며, 노드 이름과 trip count를 빼고 비교해도 216개 길이의 결정
   (연산, 적용 여부, 사유, VL)이 모두 같다. trip count가 L인 보고 77개(LayerNorm류 요소별 연산)는 모든 L에서 VL 32로
   SIMD가 적용되고, 나머지는 스칼라로 처리된다. 미적용 사유 "small j trip count"는 출력이 2개인 마지막 Gemm 1건뿐이다.
   BERT에서 sequence length가 대부분 연산의 최내측 차원이 아니라는 명세 §1.2의 예상과 맞다.
2. **IR 구조 변화의 대부분은 정렬 배수 효과다.** 바뀌는 IR 특징 25개 중 21개는 L의 주기 4·8·32 함수다(나머지 루프 처리).
3. **정렬 밖 결정 하나가 IR에 있다.** 12개 층마다 있는 4차원 transpose 루프(trip count L)는 L이 작은 수로 나누어지면
   그 수로 unroll된다. 예를 들어 소수 41은 unroll되지 않고, 49는 7씩 unroll된다. `affine.load/store/apply` 개수가 L의
   약수 여부로 결정된다. 이 결정은 opt-report에 나타나지 않는다. 성능 영향은 측정하지 않았다(G4 미실행).
4. **최종 실행물은 opt-report가 보지 못하는 정렬 경계에서 바뀐다.** G3 파일럿 한 쌍(151→152, 152=8·19)에서 명령어 구성이
   크게 달라졌다(범용 레지스터 명령 51,797 → 32,924).

**판정과 결정.** 주 signature 기준으로 `C_sig = ∅`이므로 `C_nonalign = ∅`이며, H1(정렬 밖 lowering 결정 변화의 존재)은
opt-report 수준에서 기각된다. 이 조건에서 Compile-guided와 Compile-probe는 opt-report에서 얻는 정보가 없어 Uniform의
순서와 같아진다. IR 구조에는 정렬 밖 변화(3번)가 있지만, 그 signature로 바꾸는 것은 census를 본 뒤의 선택이다.
연구자는 명세대로 **중단·보고**를 선택했다(2026-09-28). 남는 결과는 명세 §14의 표현대로
"이 컴파일러·모델·설정에서 SIMD 수준의 shape 결정은 길이에 따라 바뀌지 않으며, IR 수준 변화는 정렬 배수와 L의 약수로
예측된다"는 관찰이다.

**이 결과가 말하지 않는 것.** 성능 급락의 존재 여부(G4 미실행), 다른 flag set·다중 스레드·batch > 1·모델 B·다른 컴파일러
버전에서의 결과, 길이 41 미만.

**재개하려면.** 새 signature나 설정은 census를 본 뒤의 선택이므로 이탈로 기록하고 사전등록을 다시 고정해야 한다. 하네스
(census·파일럿·G4·정책 비교·격리)는 그대로 쓸 수 있다.

## 실패·이탈 기록

| 날짜 | 게이트 | 내용 | 처리 |
|---|---|---|---|
| 2026-09-28 | G2 | ONNX Model Zoo `bertsquad-12` 원본은 내부 상수에 길이 256이 고정(70개, Reshape/Unsqueeze/Mul/Slice 소비)되어 입력 메타데이터 변경만으로는 255/128/41에서 ORT 실행 실패 | 명세 §4.4 2단계: 동일 가중치를 원 파이프라인(Google BERT `modeling.py` + `create_model` → TF → tf2onnx)으로 가변 길이 재수출. 논문에는 "Model Zoo 원본이 아닌 동일 가중치의 재수출본"으로 표기 |
| 2026-09-28 | G1 | `run_onnx_squad.py`의 `main()`은 bertsquad-12의 출력 형식([batch, L])·4번째 입력과 맞지 않음 | 공식 notebook(BERT-Squad.ipynb)의 feed 방식을 feature 단위로 적용, 전처리·후처리 함수는 공식 코드 그대로 호출 |
| 2026-09-28 | G1 | `tokenization.py`가 TensorFlow를 `tf.gfile.GFile`에만 사용 | TF 미설치 시 `open(encoding="utf-8")` 대체 모듈 등록(토크나이저 로직 불변) |
| 2026-09-28 | G1 | 공식 vocab 배포처(storage.googleapis.com)가 개발 환경 egress에서 403 | 고정 commit의 독립 미러 2곳에서 바이트 일치 교차검증; 기능 검증은 EM/F1 재현 |
| 2026-09-28 | G2 | tf2onnx 재수출은 바이트 결정적이지 않음(노드 순서/자동 이름, `PYTHONHASHSEED=0`에서도) | 반복 수출 4회의 순서·이름 무관 의미 지문 동일 확인. 해시 고정 파일 자체를 artifact로 VM에 복사(재수출 금지) |
| 2026-09-28 | G0 | signature `sig-v1`이 같은 길이의 반복 컴파일에서도 달라짐(컴파일러 생성 노드 이름의 카운터 접미사), opt-report 줄이 출력 버퍼 경합으로 잘림 | `sig-v2`(ONNX 노드 이름으로 정규화) + 컴파일러 줄 단위 버퍼링. 성능 데이터를 보기 전의 재현성 점검에 따른 변경 |
| 2026-09-28 | G0 | 초기 소스 판독의 "`--march`가 x86-64가 아니면 SIMD가 꺼진다"는 주장이 부분적으로 틀림(요소별 연산만 해당, Gemm/MatMul은 SIMD 유지) | 실측으로 정정해 `docs/STATUS.md`, `preregistration.md`, `configs/compile_flags.json`에 반영 |
| 2026-09-28 | 설계 | 정답표 A의 검정을 백분위 부트스트랩에서 t 기반 최소효과 검정으로 변경 | 다중 에이전트 검증에서 부트스트랩 p값이 적은 블록 수에서 Holm 수준 반보수적임을 시뮬레이션으로 확인. 본실험 데이터를 보기 전의 변경 |
| 2026-09-28 | 설계 | 탐색기 격리를 같은 프로세스 audit hook → 별도 프로세스 → OS 감옥(chroot·setuid·netns)으로 강화 | 검증에서 앞의 두 방식의 우회 경로가 재현됨 |
| 2026-09-28 | G2.5 | 주 signature(opt-report) 변화점 0 → H1 기각(opt-report 수준) | 명세 §13-15에 따라 G4 이후 중단, census 표와 분석 보고(위 절). 연구자 결정 |
| 2026-09-28 | 설계 | 3차 검증 반영: 추출 비용 범위(주 signature와 ablation 분리), 확인 절차 비용, census 실패 구간 처리, 독립성 검사(G4 run id·순서 seed), 자연 길이 가중 영향 정의, 확인 단계 할당 최소·재현 표본 수를 사전등록 항목으로 추가 | 본실험 데이터를 보기 전의 변경. 목록은 `docs/STATUS.md` "독립 검토 반영" 3차 |

## 한계 (현재까지)

- 개발 환경(4 vCPU KVM 컨테이너, SMT 없음, cgroup quota 없음, 빌드와 평가가 동시 실행)은
  명세 §5 측정 통제 조건을 만족하지 않는다. 여기서 얻은 시간 수치는 기능 확인용이다.
- 모델 A는 재수출본이다. 원본 artifact로는 shape 연구 진입이 불가능하다(G2 증거 참조).
- 모델 B(MLPerf BERT-Large)는 아직 착수하지 않았다(명세 §3.1 순서상 G6).

## 재현 방법

`README.md`의 빠른 시작 참조. 모든 입력은 `artifacts.lock.json`(SHA-256)과
`env/toolchain.env`(ONNX-MLIR/LLVM/protobuf commit)로 고정되며, 재수출 모델은
`results/g2/*.export_meta.json`의 SHA-256으로 식별한다.
