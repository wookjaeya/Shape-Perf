# 보고서 (진행 중)

본실험(G4 이후) 결과는 아직 없다. 게이트별 진행과 증거는 `docs/STATUS.md`에 있다.
이 문서는 결과·실패·한계·재현 방법을 누적 기록한다.

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
