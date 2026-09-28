# 게이트 현황 (명세 §11, §13)

범례: **완료** = 실행하고 증거 파일로 확인함 · **부분** = 일부만 실행/확인 ·
**코드만** = 구현했으나 실제 실행으로 확인하지 않음 · **미실행**

개발 환경: 클라우드 컨테이너(4 vCPU Xeon @2.1GHz KVM, AVX-512, 15 GB RAM, SMT 비활성).
명세 §5.1에 따라 여기서는 **기능 확인만** 하며 성능 결과를 산출하지 않는다.

| 게이트 | 상태 | 요약 | 증거 |
|---|---|---|---|
| G0 환경 고정 | 부분 (빌드 진행 중) | ONNX-MLIR v0.5.1.1 (`1e017c9f`), LLVM `1053047a`(그 commit의 `utils/clone-mlir.sh`), protobuf v33.5. 공식 문서 플래그 + 기록된 빌드 속도용 이탈 | `env/toolchain.env`, `env/build_toolchain.sh`, 빌드 manifest |
| G1 원본 재현 | 완료 (원본 artifact) | 공식 전처리 재현: 질문 10,570 / feature 12,006 / 고유 유효 길이 216 (41–256). ORT가 zoo test_data_set 출력을 max abs 6.2e-6로 재현. **원본@256 전체 dev set: EM 80.6717 / F1 88.0716 — 모델 카드 EM 80.67171과 일치.** 재수출본@256·재수출본@L(x) 평가 진행 중 | `data/features/A/catalog.json`, `results/g1/bertsquad-12_official/reference_eval.json` |
| G2 shape 적합성 | 부분 | 원본 artifact: **길이 변경 불가**(길이 256 상수 70개, 255/128/41에서 ORT 실패). 동일 가중치 재수출본: 256에서 원본 대비 logits max abs 1.1e-5, 256 상수 0개, s∈{L,L+1,64,128,129,255,256} 139쌍에서 유효 위치 logits max abs 1.1e-5·argmax 100% 일치. ONNX-MLIR 정적 특수화·정확도는 빌드 후 확인 | `results/g2/*.json` |
| G2.5 census | 코드만 | 평가자 전용 실행기, 격리 테스트 통과 | `scripts/run_census.py`, `census/README.md`, `tests/test_isolation.py` |
| G3 파일럿 | 코드만 | warmup 추이·계층 분산·Kalibera–Jones 제안·probe:측정 비용비·probe/최종 불일치율 수집 | `scripts/pilot_g3.py` |
| G4 유한 공간 평가 | 코드만 | 발견/확인 역할 분리 조밀 측정, 정답표 A(Holm+독립 확인), 연속 지표 A(s)와 연산량 비 | `scripts/groundtruth_dense.py`, `evaluate.py answer-table` |
| G5 탐색 비교 | 코드만 (합성 데이터 테스트) | 7개 정책 + ablation(align-only, hybrid, 비용 제외), 실측 비용 재생 backend | `evaluate.py compare`, `tests/` |
| G6 외적 확인 | 미실행 | 모델 B 미착수 | — |
| G7 원인·효용 | 미실행 | — | — |

## G0 상세

- 고정: `env/toolchain.env`. LLVM commit은 ONNX-MLIR 해당 commit의 `utils/clone-mlir.sh`에서 읽고 빌드 스크립트가 일치 여부를 검사한다.
- 공식 문서(`docs/BuildOnLinuxOSX.md`)의 LLVM 구성: `mlir;clang` + `openmp` runtime, Release, assertions ON, RTTI ON.
  (공식 CI Dockerfile은 `mlir`만 빌드하지만 그 경우 ONNX-MLIR의 OpenMP 지원이 꺼진다: `src/CMakeLists.txt`가 `lib/clang/*/include/omp.h`를 찾음.)
- 기록된 이탈(빌드 속도만): `lld` 링커, 병렬 링크 2, 얕은 fetch, protobuf python은 PyPI wheel, venv.
- R1 문서(`docs/PerformanceTesting.md`)는 태그 이후 main에만 있으나, 문서가 설명하는 `--profile-ir-with-sig` 옵션은 태그 소스에도 있음을 확인.
- **matmul/attention lowering 경로**: 미확정 — 실제 컴파일의 opt-report와 `.so` 동적 import로 판정해야 함(`shapeperf/signature.py: matmul_path`). 추정하지 않음.
- **SIMD 기계 모델(소스 정적 판독, G0에서 실측 확인 필요)**: x86 krnl SIMD는 `--march=x86-64`에서만 SSE4.2 모델(128-bit, FP32 VL=4), 그 외 `--march`(미지정·native 포함)는 SIMD 미지원 모델. 따라서 기본 flag set을 `-O3 --march=x86-64 --mcpu=<VM CPU>`로 둔 것은 **후보**이며 G0에서 opt-report의 VL 값으로 확인한다. (참고: `AVX2x86VectorMachineSupport`는 비트폭 258을 반환하는 오타가 있으나 속성 인자가 항상 빈 문자열이라 선택되지 않는다.)

## G1 상세

- 입력 signature(실제 graph): `unique_ids_raw_output___9:0 [N]`, `segment_ids:0 / input_mask:0 / input_ids:0 [N,256]` int64; 출력 `unstack:0`(start), `unstack:1`(end) `[N,256]`, `unique_ids:0`. producer tf2onnx 1.5.2, opset 12.
- 공식 `run_onnx_squad.py main()`은 이 출력 형식과 맞지 않아(주석 참조) 공식 notebook의 feed 방식을 feature 단위로 사용. 전처리/후처리/평가는 공식 함수·스크립트 그대로.
- 오른쪽 padding 확인: 12,006개 feature 모두 `mask=1…1,0…0`, padding 위치 id=0·segment=0 (위반 0).
- anchor(§4.3): 유효 길이 최소 41, 동률은 (질문 ID, window ID) 사전순 → `56e10179cd28a01900c67415`, window 0.
- EM/F1 (ONNX Runtime 1.23.2, 1 thread, 공식 `write_predictions` + `evaluate_v1.1.py`):

  | 모델 | 길이 정책 | EM | F1 | 모델 카드 EM |
  |---|---|---|---|---|
  | 원본 `bertsquad-12` | 256 고정 | 80.6717 | 88.0716 | 80.67171 |
  | 재수출본 | feature별 L(x) | 진행 중 | | |
  | 재수출본 | 256 고정 | 진행 중 | | |

## 격리 증명 (G2.5·G5 진입 조건)

- 정적: `shapeperf/selectors/*.py`는 허용 목록(math, bisect, dataclasses, types, typing, numpy, collections)만 import, 파일·프로세스·프레임 API와 `census` 문자열 사용 금지.
- 프로세스 경계(기본): 탐색기는 `shapeperf/selector_worker.py` 별도 프로세스에서 실행되며 자기 View(JSON)만 받는다. 정답표·census·backend 객체는 broker 프로세스에만 있으므로 frame 탐색이나 이미 로드된 평가자 모듈 import로 닿을 수 없다.
- 프로세스 안 sandbox: 파일 열기·디렉터리 조회·프로세스 생성·소켓·동적 로딩·신규 import·frame 조회(`sys._getframe`, `tb_frame` 등) 시 `SelectorIsolationError`. 위반은 sticky라 탐색기가 예외를 잡아도 결정 종료 시 다시 발생한다.
- 테스트: 악의적 탐색기(파일 읽기, 예외 삼키기, 디렉터리 조회, frame 탐색, 평가자 import)가 두 격리 모드에서 모두 차단됨 (`tests/test_isolation.py`, `tests/malicious_selectors.py`).
- 권한: census 디렉터리 0700/파일 0600. OS 수준 분리(탐색기를 다른 UID로 실행)는 측정 VM에서 권장.

## 독립 코드 검토 반영 (2026-09-28)

별도 검토 에이전트가 정책·비용 회계·평가 코드를 명세와 대조해 결함 9건과 해석 문제 6건을 보고했고, 모두 재현 테스트와 함께 수정했다:
Compile-probe의 probe/전체 컴파일 signature 혼합, align-only 경계 off-by-one, sandbox 예외 삼키기 우회, 이미 로드된 모듈·frame 탐색을 통한 격리 우회(→ 프로세스 분리), `--census` 없는 compare 충돌, 사전등록 `[]` 처리, 블록 1개일 때 부트스트랩 퇴화(→ 최소 블록), 부호로 방향을 고른 비보정 단측 검정(→ 양측 2·min), 예산 초과 질의의 후보 집계,
그리고 비용 제외 변형의 probe 상한, 유효 집합 이웃 기준 인접성, 데이터와 무관한 검정 족, 미사용 예산 비율 보고, opt-report 출력 비용(G3에서 `default_noreport`로 측정).
