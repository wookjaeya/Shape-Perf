# Shape-Perf: 컴파일 정보를 활용한 shape별 성능 급락 탐지 — 실험 환경

`docs/spec_v2.md`(실험 명세 v2, 2026-09-28)를 구현하는 연구 저장소입니다.
ONNX-MLIR CPU 백엔드 × FP32 BERT-SQuAD × batch=1 × sequence length 한 축.

> **현재 상태는 [`docs/STATUS.md`](docs/STATUS.md)에 게이트별로 기록합니다.**
> "코드 작성 완료"와 "모델 실행·정확도·측정 타당성 확인 완료"를 구분하며,
> 실행하지 않은 단계는 미실행으로 표시합니다. 이 저장소의 어떤 숫자도
> 통제된 측정 VM에서 얻은 성능 결과가 아닙니다.

> **v3 (`docs/research_plan_v3.md`, 재설계안)의 G0–G2 실행 결과는 [`docs/STATUS_v3.md`](docs/STATUS_v3.md)와
> [`design_amendment.md`](design_amendment.md)에 있습니다.** 주 대조는 Transpose unroll 상한 8 대 1(`patches/transpose_unroll_cap1.patch`)이며,
> v2 탐색기 연구는 G2.5 중단 상태 그대로입니다.

## 구성 (명세 §12 제안 인터페이스와의 대응)

| 명세 §12 | 이 저장소 | 역할 |
|---|---|---|
| `environment.lock.json` | `environment.lock.json`, `env/toolchain.env`, `env/requirements*.in` | 고정 버전, 환경 manifest (`scripts/collect_env.py`) |
| `artifacts.lock.json` | `artifacts.lock.json`, `configs/artifacts.json` | 모델·데이터·전처리 코드 출처/SHA-256/라이선스 (`scripts/fetch_artifacts.py`) |
| `prepare_features.py` | `prepare_features.py` | 공식 전처리 호출, 질문/window 추적, `L(x)`, `S_observed`, anchor |
| `validate_shapes.py` | `validate_shapes.py`, `scripts/g2_*.py` | 길이 변경 적합성, 참조 출력(ONNX Runtime) 대비 정확도 |
| `compile_shape.py` | `compile_shape.py`, `shapeperf/compile.py` | `--shapeInformation` 정적 특수화, opt-report/IR, 컴파일·probe 비용 |
| `measure.py` | `measure.py`, `shapeperf/measure.py` | 무계측 warm latency, 프로세스 분리, 블록 내 무작위 순서, 원자료 append-only |
| `selectors/` | `shapeperf/selectors/` | Uniform·Random·Shape-only·Timing-only·Compile-guided·Compile-probe·Timing-adaptive |
| (비용 회계) | `shapeperf/broker.py`, `shapeperf/backends.py` | §8.4 비용 분리·예산 경계 규칙, 후보 확인 절차 |
| `evaluate.py` | `evaluate.py`, `shapeperf/evaluate.py` | 정답표 A, 연속 지표, census 지표(§9.4), 정책 비교(§11.1) |
| `census/` | `census/`, `scripts/run_census.py` | 평가자 전용 G2.5 census (탐색기 접근 차단: `tests/test_isolation.py`) |
| `preregistration.md` | `preregistration.md`, `configs/preregistration.json` | 파일럿 이후 고정할 값 (현재 **미고정**) |
| `report.md` | `report.md` | 결과·실패·한계·재현 방법 |

> 명세의 `selectors/`는 Python 표준 라이브러리 `selectors` 모듈과 이름이 겹쳐
> 저장소 루트에 두면 `subprocess` 등이 깨집니다. 그래서 `shapeperf/selectors/`에 둡니다.

## 빠른 시작

```bash
# 0) 툴체인 (G0) — 4 vCPU에서 수 시간. 측정 VM에서는 env/Dockerfile 사용
SHAPEPERF_WORK=$HOME/shapeperf-work env/build_toolchain.sh python protobuf llvm onnx-mlir
SHAPEPERF_WORK=$HOME/shapeperf-work env/build_toolchain.sh check-onnx-lit   # 빌드 검증

# 1) 데이터·모델 (G1/G2, 컴파일러 불필요)
scripts/setup_data.sh          # fetch → features → 원본 shape 검사 → 재수출 → 재수출 검증
scripts/run_g1_evals.sh        # 전체 dev set EM/F1 (원본@256, 재수출@256, 재수출@L(x))

# 2) 컴파일·정확도·측정 smoke test (개발 환경에서는 기능 확인만)
export SHAPEPERF_WORK=$HOME/shapeperf-work SHAPEPERF_TARGET_CPU=<llc가 아는 CPU 이름>
python compile_shape.py --length 128 --mode full  --out results/compile/smoke/s128
python compile_shape.py --length 128 --mode probe --out results/compile/smoke/s128p
python validate_shapes.py --artifact results/compile/smoke/s128/model.so --length 128
python measure.py --artifact results/compile/smoke/s128/model.so --length 128 \
    --warmup <W> --iterations <N> --seed 1 --out results/raw/smoke.jsonl   # W,N은 파일럿 전 임의값 금지

# 3) 게이트 G2.5 / G3 / G4 / G5 (측정 VM)
python scripts/run_census.py --lengths 41-256            # 평가자 전용
python scripts/pilot_g3.py --seed ... --n-lengths ... --adjacent-pairs ... --processes ... --iterations ... --blocks ...
python scripts/groundtruth_dense.py --role discovery    --seed ... --vm-allocation-id ...
python scripts/groundtruth_dense.py --role confirmation --seed ... --vm-allocation-id ...
python evaluate.py answer-table --discovery <disc>/measurements.jsonl --confirmation <conf>/measurements.jsonl \
    --manifest <disc>/manifest.json            # or --valid 41-256: the valid set must be explicit
python evaluate.py alt-indicators --measurements <disc>/measurements.jsonl --manifest <disc>/manifest.json \
    --alternative-flagset O3_nosimd           # R(s), Q(s) (spec §9.1)
python evaluate.py census --census-table census/.../census_table.json --answer-table results/eval/answer_table_A.json
python evaluate.py compare --compile <disc>/compile.jsonl --measurements <disc>/measurements.jsonl \
    --confirmation-measurements <conf>/measurements.jsonl --census census/.../census.jsonl \
    --answer-table results/eval/answer_table_A.json

# 테스트 (합성 데이터; 결과 숫자 아님)
python -m pytest -q tests
```

본실험용 스크립트(`groundtruth_dense.py`, `evaluate.py`)는 `configs/preregistration.json`이
고정(frozen)되지 않았거나 필요한 값이 `null`이면 실행을 거부합니다.
`--allow-unfrozen`은 개발용이며 출력에 그 사실이 표시됩니다.
