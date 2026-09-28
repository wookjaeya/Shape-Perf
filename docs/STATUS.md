# 게이트 현황 (명세 §11, §13)

범례: **완료** = 실행하고 증거 파일로 확인함 · **부분** = 일부만 실행/확인 ·
**코드만** = 구현했으나 실제 실행으로 확인하지 않음 · **미실행**

개발 환경: 클라우드 컨테이너(4 vCPU Xeon @2.1GHz KVM, LLVM host CPU `emeraldrapids`, AVX-512, 15 GB RAM, SMT 비활성).
명세 §5.1에 따라 여기서는 **기능 확인만** 하며 성능 결과를 산출하지 않는다. 아래 시간 수치는 빌드·평가가
동시에 돌던 개발 환경의 기능 확인용 값이다.

| 게이트 | 상태 | 요약 | 증거 |
|---|---|---|---|
| G0 환경 고정 | 개발 환경 기능 검증 완료 | ONNX-MLIR v0.5.1.1 (`1e017c9f`)·LLVM `1053047a`·protobuf v33.5 소스 빌드 성공. 공식 빌드 테스트 `check-onnx-lit`: 450개 중 304 통과·146 미지원(NNPA 등)·실패 0 (`check-mlir`은 미실행). 실제 컴파일로 확인: 정적 특수화, matmul은 컴파일러 생성 코드(외부 BLAS 없음), `--march`별 SIMD 적용 양상, signature 결정성, ORT 대비 정확도 | `results/g0/g0_verification.json`, `results/g0/matmul_path.json` |
| G1 원본 재현 | 완료 | 공식 전처리 재현: 질문 10,570 / feature 12,006 / 고유 유효 길이 216 (41–256). ORT가 zoo test_data_set 출력을 max abs 6.2e-6로 재현. **원본@256 전체 dev set: EM 80.6717 / F1 88.0716 — 모델 카드 EM 80.67171과 일치** | `data/features/A/catalog.json`, `results/g1/*/reference_eval.json` |
| G2 shape 적합성 | 완료 (재수출본, 개발 환경) | 원본 artifact는 **길이 변경 불가**(길이 256 상수 70개). 동일 가중치 재수출본: 원본 대비 logits 1.1e-5, 139쌍에서 padding 의미 보존. §7.4: 재수출본을 feature별 L(x)로 실행해도 EM/F1 동일·답 변경 0건. ONNX-MLIR@128: 정적 특수화 확인, ORT 대비 유효 위치 logits max abs 1.5e-5·argmax 일치(허용치는 미등록) | `results/g2/*.json`, `results/g0/validation.jsonl` |
| G2.5 census | 코드만 | 평가자 전용 실행기(실패 경계 분리, 원문 IR 해시 변화 위치 저장) | `scripts/run_census.py`, `census/README.md` |
| G3 파일럿 | 코드만 | warmup 추이·계층 분산·Kalibera–Jones 제안·probe:측정 비용비(검증 비용 포함)·probe/최종 불일치율·opt-report 출력 비용 | `scripts/pilot_g3.py` |
| G4 유한 공간 평가 | 코드만 | 발견/확인 역할 분리 조밀 측정(여러 flag set을 같은 블록에서), 정답표 A, R(s)/Q(s), 다른 입력으로 경계 재현 | `scripts/groundtruth_dense.py`, `scripts/replicate_boundaries.py`, `evaluate.py` |
| G5 탐색 비교 | 코드만 (합성 데이터 테스트) | 7개 정책 + ablation(align-only, hybrid, 비용 제외, raw-IR/IR-구조 signature), 실측 비용 재생, δ별·seed 불확실성 | `evaluate.py compare`, `tests/` |
| G6 외적 확인 | 미실행 | 모델 B 미착수 (MLPerf artifact의 zenodo가 개발 환경에서 차단) | — |
| G7 원인·효용 | 미실행 | 진단용 계측 경로만 준비 | `scripts/diag_profile.py` |

## G0 상세

- 고정: `env/toolchain.env`. LLVM commit은 ONNX-MLIR 해당 commit의 `utils/clone-mlir.sh`에서 읽고 빌드 스크립트가 일치 여부를 검사한다.
- 공식 문서(`docs/BuildOnLinuxOSX.md`)의 LLVM 구성: `mlir;clang` + `openmp` runtime, Release, assertions ON, RTTI ON.
  (공식 CI Dockerfile은 `mlir`만 빌드하지만 그 경우 ONNX-MLIR의 OpenMP 지원이 꺼진다: `src/CMakeLists.txt`가 `lib/clang/*/include/omp.h`를 찾음. 빌드 후 `omp.h` 생성 확인.)
- 기록된 이탈(빌드 속도만): `lld` 링커, 병렬 링크 2, 얕은 fetch, protobuf python은 PyPI wheel, venv.
- 빌드 비용(4 vCPU, 병렬 4): LLVM 2시간 25분(단일 프로세스 최대 RSS 2.8 GB), ONNX-MLIR 43분.
- 버전 문자열: `onnx-mlir version 0.5.1, onnx version 1.21.0 (... 1e017c9f...)`, `LLVM version 23.0.0git (... 1053047a...)`.
- R1 문서(`docs/PerformanceTesting.md`)는 태그 이후 main에만 있으나, 문서가 설명하는 `--profile-ir-with-sig` 옵션은 태그 소스에도 있음을 확인.
- **정적 특수화**: `--shapeInformation=0:1,1:1x128,2:1x128,3:1x128`으로 만든 `.so`의 PyRuntime 입력 signature가 `[1]`, `[1,128]`×3, probe IR의 `main_graph` 인자가 `memref<1x128xi64>` 등 정적 memref임을 확인. `RunONNXModel.py --shape-info`는 실행 입력 생성 옵션임을 `--help`로 기록.
- **matmul/attention lowering 경로**: 컴파일러 생성 코드. `.so`의 동적 import에 BLAS류 심볼이 없고(`expf`, `tanhf`, `powf`, `malloc` 등 libc/libm만), opt-report에 Gemm 81건·MatMul 28건의 SIMD 기록이 있다.
- **SIMD 적용 양상 (opt-report 실측, 길이 128)**: `--march=x86-64`에서 요소별·축약 연산 SIMD(보고 VL 32), Gemm VL 16, MatMul VL 8. `--march` 미지정·`native`에서는 요소별·축약 연산 407건이 SIMD 미적용이고 Gemm/MatMul만 SIMD. **초기 소스 판독에서 "그 외 `--march`면 SIMD가 꺼진다"고 적었던 것은 요소별 연산에만 맞았다**. `--march`와 `--mcpu`를 함께 주면 ONNX-MLIR 자체 결정은 `--mcpu`를 무시한다고 경고하지만, LLVM에는 전달되어 최종 `.so`가 zmm(AVX-512) 명령을 쓴다. 기본 flag set `-O3 --march=x86-64 --mcpu=<VM CPU>`는 여전히 후보이며 측정 VM에서 같은 확인을 반복한다.
- **컴파일 비용(길이 128, 개발 환경)**: 전체 컴파일 78초·최대 RSS 2.8 GB·`.so` 435 MB(상수 내장), probe(`--EmitMLIR`) 7.7–8.2초·0.94 GB. probe : 전체 컴파일 ≈ 0.1 (H3의 첫 근거; 측정 VM의 G3에서 다시 측정).
- **signature 결정성**: 같은 길이를 두 번 probe해도 opt-report·IR 구조·원문 IR 해시 모두 동일, probe와 전체 컴파일의 opt-report signature 동일, 보고 무결성 정상 (`sig-v2`, 아래 참고).

### G0에서 발견해 고친 signature 문제 (성능 결과를 보기 전)

1. **노드 이름의 비결정적 접미사**: ONNX-MLIR는 재작성 중 만든 연산에 `<원래 이름>_<카운터>`(융합 연산은 `<a>-<b>_<카운터>`) 이름을 붙이는데, 같은 길이를 두 번 컴파일해도 카운터가 달랐다(예: `mul_3_12` vs `mul_3_13`). 그대로 두면 모든 인접 길이가 가짜 변화점이 된다. → `sig-v2`는 모델의 ONNX 노드 이름으로 정규화한다.
2. **보고 줄 뒤섞임**: opt-report(C `printf`)와 LLVM 자체 출력 스트림이 같은 파일에 따로 flush되어 보고 한 줄이 잘렸다. → 컴파일러를 `stdbuf -oL`로 실행하고, 파싱 불가 줄이 있으면 signature를 `CORRUPT:`로 표시한다.

## G1 상세

- 입력 signature(실제 graph): `unique_ids_raw_output___9:0 [N]`, `segment_ids:0 / input_mask:0 / input_ids:0 [N,256]` int64; 출력 `unstack:0`(start), `unstack:1`(end) `[N,256]`, `unique_ids:0`. producer tf2onnx 1.5.2, opset 12.
- 공식 `run_onnx_squad.py main()`은 이 출력 형식과 맞지 않아 공식 notebook의 feed 방식을 feature 단위로 사용. 전처리/후처리/평가는 공식 함수·스크립트 그대로.
- 오른쪽 padding 확인: 12,006개 feature 모두 `mask=1…1,0…0`, padding 위치 id=0·segment=0 (위반 0).
- anchor(§4.3): 유효 길이 최소 41, 동률은 (질문 ID, window ID) 사전순 → `56e10179cd28a01900c67415`, window 0.
- EM/F1 (ONNX Runtime 1.23.2, 1 thread, 공식 `write_predictions` + `evaluate_v1.1.py`):

  | 모델 | 길이 정책 | EM | F1 | 모델 카드 EM |
  |---|---|---|---|---|
  | 원본 `bertsquad-12` | 256 고정 | 80.6717 | 88.0716 | 80.67171 |
  | 재수출본 | feature별 L(x) | 80.6717 | 88.0716 | — (원본@256과 답 텍스트 10,570/10,570 동일) |
  | 재수출본 | 256 고정 | 80.6717 | 88.0716 | — (원본@256과 답 텍스트 10,570/10,570 동일) |

## 격리 증명 (G2.5·G5 진입 조건)

- **OS 경계(기본 'process' 모드)**: 탐색기는 `shapeperf/selector_worker.py` 별도 프로세스에서 실행된다. 필요한 모듈을 미리 로드한 뒤 빈 디렉터리로 `chroot`, 네트워크 네임스페이스 분리, `nobody`로 권한 강등, 프로세스 생성·파일 쓰기 rlimit 0, 프로토콜 파이프 외 모든 fd를 닫는다. 생성자도 이 감옥 안에서만 실행된다(`SelectorSpec`: 평가자 프로세스는 탐색기를 인스턴스화하지 않음). 달성한 격리 수준이 실행 기록에 남고, 고정된 정책 비교는 `os:` 수준을 요구한다.
- **audit-hook 계층(심층 방어)**: 파일·디렉터리·프로세스·소켓·동적 로딩·신규 import·frame 조회 시 `SelectorIsolationError`, 위반은 sticky. 탐색기가 이 계층을 스스로 꺼도 OS 감옥 때문에 파일을 읽을 수 없음을 테스트로 확인했다.
- **프로토콜**: 전용 파이프 + 순번(seq), stdout은 버림, 응답 시간 제한. 위조 응답은 순번 불일치로 감지.
- **View**: 누적 비용은 View에서 제외(측정 시간이 timing 정책이 아닌 정책에 새는 경로 차단).
- `inprocess` 모드는 디버깅 전용이며 보안 경계가 아님을 테스트로 명시.
- 정적 검사: `shapeperf/selectors/*.py`는 허용 목록만 import, 파일·프로세스·프레임 API와 `census` 문자열 사용 금지.

## 독립 검토 반영

**1차 (에이전트 1명)**: 결함 9건·해석 문제 6건 — Compile-probe signature 혼합, align-only off-by-one, sandbox 우회, compare 충돌, `[]` 처리, 부트스트랩 퇴화, 비보정 단측 검정, 예산 초과 후보 집계 등을 수정.

**2차 (다중 에이전트 검증 워크플로, 95개 에이전트, 문제마다 반박자 3명)**: 1차 수정을 다시 검증해 26건 확인(3건 기각). 주요 수정:
- 격리: 같은 인터프리터 안의 sandbox 상태를 탐색기가 끌 수 있었음, 생성자가 sandbox 밖에서 실행, stdout 채널 위조 가능 → OS 감옥·`SelectorSpec`·전용 파이프.
- 통계: 백분위 부트스트랩 p값이 블록 수가 적을 때 Holm 수준에서 크게 반보수적(FWER 0.2–0.99) → t 기반 최소효과 검정, 할당 단위 검정, 시뮬레이션 검정 크기 테스트.
- 정답표와 정책 비교가 서로 다른 유효 집합을 써서 실패 길이 옆 사건을 찾을 수 없었음 → 명시적 유효 집합 공유.
- 예산: 같은 질의 안의 두 번째 확인이 예산을 넘으면 첫 번째 확인까지 무효화 → 확인을 별도 timeline 항목으로.
- Compile-probe가 쓰지 않는 전체 컴파일 signature 추출 비용을 내던 문제, `[]` 정렬 단위의 잔여 처리, census 실패 경계가 H1 판정을 뒤집던 문제, 부분 census로 인한 충돌, 미고정 정답표의 혼입, 확인 데이터 독립성 미강제, View의 누적 비용 누출.
- 명세 누락 구현: R(s)/Q(s), ablation 3(원문 IR 해시)·4(다른 입력 재현), δ별 결과와 seed 불확실성, per-shape 요약, 자연 길이 가중 효과, 공통/정책 추가 비용 분리 출력, 실행별 timeline 보존.
