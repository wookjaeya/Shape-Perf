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
| G2.5 census | **개발 환경 전 범위 실행(41–256) → 주 signature 변화점 0, H1 기각 → 중단(연구자 결정, 명세 §13-15)** | 기본 flag set에서 opt-report signature 변화점 **0개**(216개 길이 모두 같은 결정). IR 구조는 모든 인접 길이에서 바뀌며, 대부분 4·8·32 주기(정렬), 일부는 L의 약수(3·5·7)에 따른 transpose 루프 unroll. 결과·결정은 `report.md` "G2.5 signature census 결과와 중단 결정" | `results/g2_5/census_dev_table.json`, `results/g2_5/census_dev_analysis.json`, `scripts/analyze_census.py` |
| G3 파일럿 | 코드만(개발 smoke 실행만) · G2.5 중단으로 본실행 안 함 | warmup 추이·계층 분산·Kalibera–Jones 제안·probe:측정 비용비(검증 비용 포함)·probe/최종 불일치율·opt-report 출력 비용 | `scripts/pilot_g3.py` |
| G4 유한 공간 평가 | 중단(G2.5) · 코드만 | 발견/확인 역할 분리 조밀 측정(여러 flag set을 같은 블록에서), 정답표 A, R(s)/Q(s), 다른 입력으로 경계 재현 | `scripts/groundtruth_dense.py`, `scripts/replicate_boundaries.py`, `evaluate.py` |
| G5 탐색 비교 | 중단(G2.5) · 코드만 (합성 데이터 테스트) | 7개 정책 + ablation(align-only, hybrid, 비용 제외, raw-IR/IR-구조 signature), 실측 비용 재생, δ별·seed 불확실성 | `evaluate.py compare`, `tests/` |
| G6 외적 확인 | 중단(G2.5) · 미실행 | 모델 B 미착수 (MLPerf artifact의 zenodo가 개발 환경에서 차단) | — |
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
- **SIMD 적용 양상 (opt-report 실측, 길이 128)**: `--march=x86-64`에서 요소별·축약 연산 SIMD(보고 VL 32), Gemm VL 16, MatMul VL 8. `--march` 미지정·`native`에서는 요소별·축약 연산 408건이 SIMD 미적용이고 Gemm/MatMul만 SIMD. **초기 소스 판독에서 "그 외 `--march`면 SIMD가 꺼진다"고 적었던 것은 요소별 연산에만 맞았다**. `--march`와 `--mcpu`를 함께 주면 ONNX-MLIR 자체 결정은 `--mcpu`를 무시한다고 경고하지만, LLVM에는 전달되어 최종 `.so`가 zmm(AVX-512) 명령을 쓴다. 기본 flag set `-O3 --march=x86-64 --mcpu=<VM CPU>`는 여전히 후보이며 측정 VM에서 같은 확인을 반복한다.
- **컴파일 비용(길이 128, 개발 환경)**: 전체 컴파일 78초·최대 RSS 2.8 GB·`.so` 435 MB(상수 내장). probe(`--EmitMLIR`)는 상수를 그대로 출력하면 IR 834 MB·7.7–8.2초였고, 큰 상수 생략 출력(`--mlir-elide-*`, 컴파일 결과·signature 불변 확인)으로 IR 2.3 MB·2.9초. probe : 전체 컴파일 ≈ 0.04 (H3의 첫 근거; 측정 VM의 G3에서 다시 측정).
- **디스크 요구(측정 VM 준비용)**: 모델 A의 `.so`는 길이마다 약 0.42 GB라 G4 조밀 측정(216개 길이, 블록 내 무작위 순서 때문에 동시 보관 필요)에는 artifact만 약 90 GB가 든다. 추가 flag set 하나당 같은 양이 더 필요하다.
- **signature 결정성**: 같은 길이를 두 번 probe해도 opt-report·IR 구조·원문 IR 해시 모두 동일, probe와 전체 컴파일의 opt-report signature 동일, 보고 무결성 정상 (`sig-v2`, 아래 참고).

### G0에서 발견해 고친 signature 문제 (성능 결과를 보기 전)

1. **노드 이름의 비결정적 접미사**: ONNX-MLIR는 재작성 중 만든 연산에 `<원래 이름>_<카운터>`(융합 연산은 `<a>-<b>_<카운터>`) 이름을 붙이는데, 같은 길이를 두 번 컴파일해도 카운터가 달랐다(예: `mul_3_12` vs `mul_3_13`). 그대로 두면 모든 인접 길이가 가짜 변화점이 된다. → `sig-v2`는 모델의 ONNX 노드 이름으로 정규화한다.
2. **보고 줄 뒤섞임**: opt-report(C `printf`)와 LLVM 자체 출력 스트림이 같은 파일에 따로 flush되어 보고 한 줄이 잘렸다. → 컴파일러를 `stdbuf -oL`로 실행하고, 파싱 불가 줄이 있으면 signature를 `CORRUPT:`로 표시한다.

## G2.5 개발 census (2026-09-28, 개발 환경, 사전등록 미고정 — 결과 아님)

설정: 모델 A 재수출본, flag set `default`(`-O3 --march=x86-64 --mcpu=emeraldrapids`, `--opt-report=Simd`), probe 단계(`--EmitMLIR`), 길이 41–256 전부(216개), 실패 0·손상 보고 0·IR 누락 0, 정규화 `sig-v3`(버전을 올린 뒤 다시 실행, 결과 동일). probe 합계 588초(길이당 약 2.7초). 원문 IR은 평가자 전용 `census/`에 보관.

- **opt-report signature(주 signature)**: 216개 길이 모두 같다. 노드 이름을 빼고 (연산, 적용 여부, 사유, VL) 다중집합으로 비교해도 1종류 — 정규화가 차이를 지운 것이 아니다. 길이마다 582개 보고, 바뀌는 것은 trip count뿐(설계상 제외). trip count가 L인 77개 보고(LayerNorm류 요소별 연산)는 모든 L에서 VL 32로 SIMD 적용. "small j trip count" 미적용 1건은 출력 2개짜리 마지막 Gemm(길이 무관). 즉 이 컴파일러에서 SIMD 결정은 L ≥ 41에서 길이와 무관하다(명세 §1.2 마지막 문단이 예상한 경우). ONNX-MLIR는 `--march`가 있으면 자체 결정에 `--mcpu`를 쓰지 않으므로 측정 VM에서도 같을 가능성이 높다(VM에서 재확인 필요).
- **IR 구조 signature(보조, 사전 정의됨)**: 215개 인접 경계 모두 변화. 바뀌는 특징 25개 중 21개는 L의 주기 4·8·32 함수(나머지 루프 처리 = 정렬). `affine.load/store/apply` 3개는 L의 약수 여부로 결정된다: 12개 층마다 있는 4차원 transpose 루프(trip count L)가 L이 작은 수(예: 7)로 나누어지면 그 수로 unroll된다(L=41 소수는 unroll 없음, L=49는 step 7). `arith.constant`는 단순 규칙 없음(L 값 자체가 상수로 들어감).
- **최종 실행물**: 파일럿의 151→152(8의 배수)에서 명령어 구성이 크게 바뀜(범용 레지스터 명령 51,797→32,924) — opt-report는 이를 보지 못한다(probe/최종 불일치율 1.0, 한 쌍).
- 정렬 단위 후보별(분석용): u=8 B_align 53, u=16 27, u=32 13. opt-report 기준 C_nonalign은 모든 단위에서 0. IR 구조 기준 C_nonalign은 162/188/202(대부분 약수 기반 unroll과 4 주기 변화).

**명세상 의미**: 주 signature(opt-report)로는 `C_sig = ∅`이므로 `C_nonalign = ∅` — 명세 §13-15는 "G4 이후를 진행하지 말고 그 사실과 census 표를 보고"하라고 한다(§14 축소·중단 근거). 이 결과에서 Compile-guided/Compile-probe는 opt-report로는 정보가 없어 Uniform과 같아진다. IR 구조에는 정렬 밖 변화(약수 기반 unroll)가 있지만, 그 signature로 바꾸는 것은 census를 본 뒤의 선택이므로 이탈로 기록해야 한다. 어느 쪽으로 갈지는 연구자 결정이다.

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

**3차 (다중 에이전트 검증 워크플로 2회차: 2차 수정 재검증)**: 20건 확인 + 경미 27건. 주요 수정:
- 비용: probe 추출 비용에 ablation 전용 IR 해싱(대용량 IR 읽기)과 ONNX 모델 로딩이 섞여 Compile-probe가 약 5배 과다 청구 → 주 signature 추출(opt-report 파싱·해시)만 `feature_extract_wall_ns`, IR 구조·원문 해시는 `ablation_extract_wall_ns`로 분리해 ablation 변형에만 청구. G3 probe:측정 비용비도 같은 정의(probe 컴파일 + 추출)로 계산.
- 비용: 빠르게 실패한 컴파일에서 `compile_ns - report_ns`가 음수가 되어 예산을 돌려주던 문제 → `report_ns`는 성공한 컴파일에만, 컴파일 시간을 넘지 않게. LiveBackend도 사전등록 `report_ns`를 같은 규칙으로 사용.
- 비용: 확인 절차를 프로세스 1개 비용으로 청구하던 문제 → 정답표 A가 실제로 쓴 확인 run의 두 끝점 프로세스 시도 전체의 wall time. 실패한 측정 시도도 재생 시 비용과 실패로 반영, 프로세스 기동 비용 포함.
- census: 실패 길이 너머의 결정 변화가 사라지고, 전부 실패해도 "H1 기각 → 중단"이 나오던 문제 → 실패 구간 양쪽의 가장 가까운 성공 길이를 비교해 구간(span)으로 기록, 성공 길이가 2개 미만·손상 보고·IR 누락이면 INCOMPLETE. `--resume`은 손상·IR 누락 길이를 다시 컴파일, 요청 범위 밖 기록은 무시.
- CORRUPT signature가 실제 값처럼 쓰이던 문제 → 탐색기에는 `FAILED:corrupt_report`(사전등록 실패 규칙), census에서는 실패처럼 건너뜀, `evaluate census`는 불완전한 표를 고정 실행에서 거부하고 개발 실행에서는 경고.
- 독립성 검사가 실제로는 작동하지 않던 문제(run_id가 프로세스마다 새로 생김) → G4 run id·블록 순서 seed·역할을 기록하고 비교, 할당 id가 없으면 공유로 간주. 고정 실행은 run id가 검증돼야 함.
- 정답표·per-shape 요약·compare가 flag set을 섞던 문제, compile 기록 없는 측정 길이를 무료 실패로 재생하던 문제(`--reuse-compile` 기록도 새 run에 복사), 고정 G4가 정확도 실패 artifact를 측정할 수 있던 문제, `validate_shapes.py`가 `SHAPEPERF_PREREG`를 무시하던 문제.
- 통계: R(s)/Q(s) CI를 할당 단위로, 확인 단계 할당 최소를 별도 사전등록 값으로, seed 1개일 때 CI 없음, 사건이 없으면 recall 정의 안 됨(NaN 대신 null).
- 자연 길이 가중 영향: 사건 비율을 끝점 빈도로 곱하던 정의 → "피할 수 있었던 padding 초과" `T(L)/min_{s≥L}T(s) − 1`을 자연 길이 빈도로 가중(확인 데이터의 per-shape 평균), 카탈로그가 없으면 "unavailable".
- 파싱: 보고 줄 끝에 이물 텍스트가 붙으면 예외로 중단 → 파싱 불가 줄로 기록(`CORRUPT:`). 위치 정보 제거 정규식이 `memref.alloc(` 안의 `loc(`까지 지우던 문제. 생성 이름이 다른 원래 노드 이름과 겹칠 때 op 종류로 구분, 카운터가 여러 개인 이름 처리.
- 격리: 테스트 전용 `class_path` 모듈의 최상위 코드가 감옥 밖에서 실행되던 문제 → 소스만 먼저 읽고 감옥 안에서 실행, 환경변수로 명시적으로 허용할 때만 사용, 격리 수준에 `+test-class` 표시. 작업자가 보고한 선택 시간·행동 형식 검증.
- 기타: timeline에 §12 조인 키 `query_index` 복원, 후보는 생성 시점에 기록(예산 소진으로 확인을 못 해도), 재현 스크립트의 정답표 라벨·표본 수·신뢰수준을 사전등록에서.

**4차 (3차 수정분 재검증 워크플로, 검토 5명 + 반박 검증 5명)**: 20건 확인(5건 기각). 주요 수정:
- 고정 실행의 정책 비교가 실패한 길이 하나만 있어도 거부되던 문제(확인 데이터가 있을 수 없는 길이까지 요구) → 성공 측정이 있는 길이만 요구. 대신 compile·측정 기록이 아예 없는 부분 입력은 거부.
- `--reuse-compile`이 실패한 컴파일 기록 때문에 전체 실행을 중단하던 문제 → 측정할 artifact만 존재·해시 확인, 모든 검증을 출력 전에. 재사용한 정확도 판정은 판정한 사전등록·허용치와 함께 기록하고, 고정 실행에서 다르면 다시 검증.
- 비용: 정확도 실패(컴파일 성공) 길이의 `report_ns` 이동이 재생에서 빠지던 문제, ablation이 쓰지 않는 추출(opt-report 파싱·다른 ablation의 IR 계산)까지 청구되던 문제, 노드 이름 적재(약 0.7초)가 어디에도 청구되지 않던 문제 → 각각 수정(노드 이름 적재는 signature 정책에 실행당 한 번).
- census: 실패 구간을 가로지르는 변화를 "정렬됨"으로 보고 H1 기각을 내리던 문제 → "판정 불가". `--resume` 전에 원문 IR을 지워 변화점 IR을 잃던 문제 → 불완전한 동안 삭제하지 않고, 변화점 IR 누락을 표에 기록. 이전 형식 표(구간 없음)와 signature 버전 혼합을 무결성 검사에서 거부.
- 정규화 규칙 변경을 `sig-v3`로 올리고 규칙 설명을 갱신(이전 기록과 섞이지 않게).
- 한쪽만 할당 id가 없을 때 새 할당 요구를 통과하던 문제, 자연 길이 가중 영향이 유효 집합 밖 길이로 중단·음수가 되던 문제와 최소값 선택 편향(교차 적합으로 수정).
- 격리: class_path 부모 패키지 `__init__`가 감옥 밖에서 실행되던 경로, 모듈 텍스트에 적힌 표준 라이브러리(`venv.__main__` 등)를 감옥 밖에서 import하던 경로 → 부모를 import하지 않고 찾기, 고정 허용 목록만 선import, 정상 모듈로 적재.
- `freeze_prereg.py`가 새 `replication` 절을 검사하지 않던 문제 → 모든 절 검사.

### 남은 한계 (알고 있는 것)

- **생성 노드 이름의 모호성**: ONNX-MLIR의 이름 부여 순서가 포인터 주소 순이라, 생성 연산이 **원래 모델에 있던 이름**(예: 제거된 `X_1`)을 재사용하면 sig-v3도 구분하지 못할 수 있다. 길이 128에서는 반복 컴파일 signature가 같았다. 측정 VM의 G0에서 `scripts/g0_verify.py --determinism-lengths ...`로 여러 길이에서 반복 컴파일 결정성을 확인한다.
- **시간 부채널**: 탐색기는 자기 결정 시간을 `time.perf_counter`로 잴 수 있고(정적 검사는 `time` import를 막지만 런타임은 막지 않음), 이는 측정 결과가 아니라 자기 계산 시간이므로 정보 누출로 보지 않는다.
- **LiveBackend 확인 절차**: G5 실시간 실행의 확인 절차는 G3/G4 이후 사전등록으로 고정한다(현재 `NotImplementedError`).

