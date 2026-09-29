# Native 재현 절차 (v3)

계획서 §G0-5: image digest를 만들 수 없으면 native 재현 절차를 고정한다. 개발 컨테이너에는 Docker가 없어
`env/Dockerfile`은 이번에도 **빌드하지 않았다**(v2도 마찬가지). 아래는 이번 실행에서 실제로 밟은 절차와 그 검증 증거다.

## 1. 툴체인 (v2에서 빌드, 이번에 재확인)

```bash
SHAPEPERF_WORK=/home/user/work env/build_toolchain.sh python protobuf llvm onnx-mlir   # 고정 SHA는 env/toolchain.env
```

| 항목 | 값 |
|---|---|
| ONNX-MLIR | `v0.5.1.1`, `1e017c9fcd7ae7218731ae799f13a957e0cbc80b` |
| LLVM | `1053047a4be7d1fece3adaf5e7597f838058c947` |
| 원본 컴파일러 `onnx-mlir` SHA-256 | `e34bcd6a3b455e2b620c15eb8f1f89f17fda310790b352a037cc38ab75c47af0` |

## 2. 패치 컴파일러 변형 만들기 (S1)

```bash
cd $SHAPEPERF_WORK/onnx-mlir
mkdir -p ../variants/orig/bin && cp -p build/Release/bin/onnx-mlir ../variants/orig/bin/        # 패치 전 바이너리 보존
git apply /path/to/Shape-Perf/patches/transpose_unroll_cap1.patch                                # 1줄: unroll 상한 8 -> 1
(cd build && ninja -j4 onnx-mlir)
mkdir -p ../variants/cap1/bin && cp -p build/Release/bin/onnx-mlir ../variants/cap1/bin/
git checkout -- src/Conversion/ONNXToKrnl/Tensor/Transpose.cpp                                   # 원복
(cd build && ninja -j4 onnx-mlir)                                                                 # 원복 빌드
ln -sfn $PWD/build/Release/lib ../variants/orig/lib; ln -sfn $PWD/build/Release/lib ../variants/cap1/lib
```

- 패치 파일: `patches/transpose_unroll_cap1.patch`(SHA-256 `3e1b6f5a28b13389653445b67877e9f7cc4cc3ebcdb593df6aeb117a47a0ba8c`).
  고정 트리에 `git apply --check`로 깨끗하게 적용됨을 확인했다.
- 변형 디렉터리는 `bin/onnx-mlir`(복사본)과 `lib`(빌드 트리의 `lib` 링크)만 가진다. 컴파일러는 실행 파일 옆의 `../lib`에서
  런타임을 찾으므로 복사본으로 `--EmitLib`까지 동작한다(확인함).

| 변형 | 바이너리 SHA-256 |
|---|---|
| `orig` (S8) | `e34bcd6a3b455e2b620c15eb8f1f89f17fda310790b352a037cc38ab75c47af0` |
| `cap1` (S1) | `fcea1caa6a858832dbbcb2f7951337c495b29be06b178676cd78c8c70ce49396` |

## 3. 검증 증거

1. **원복 재현**: 소스를 되돌려 다시 빌드한 `build/Release/bin/onnx-mlir`의 SHA-256이 패치 전 원본과 **같다**
   (`e34bcd6a…`). 빌드가 결정적이고 패치가 정확히 되돌려졌다는 뜻이다.
2. **어제 산출물 재현**: 원복 뒤 새 빌드로 L=128, `default` flag set의 `.so`를 컴파일하면 SHA-256 `b8529c683e6ac7a0…`,
   435,165,416 B로, v2 G0(`results/g0/g0_verification.json`)의 산출물과 **바이트 단위로 같다**.
3. **두 arm의 차이는 의도한 것뿐**: `scripts/build_shape_pair.py`가 길이마다 IR 예측을 검사한다(결과는 `results/v3/g1/`).

## 4. 실행 환경 (변경 없음)

Python 3.11.15(`/home/user/work/venv`: numpy 2.2.6, onnx 1.23.0, onnxruntime 1.23.2, scipy 1.15.3), `ubuntu 24.04`,
clang 18.1.3. 컴파일러 안의 onnx는 1.21.0으로 파이썬 패키지와 다르지만, 파이썬 onnx는 모델 읽기(노드 이름 수집, subgraph 생성)에만
쓰이고 컴파일에는 관여하지 않는다.

## 5. 실행할 때마다 고정되는 provenance

`shapeperf/provenance.py`가 실행 시작 시점에 HEAD, 작업 트리 diff와 미추적 파일 해시, 의존성 목록 해시, 컴파일러 바이너리 해시,
호스트를 한 번만 기록하고 `provenance_id`를 만든다. 모든 원자료 기록이 그 id를 가진다. `git_head()`는 프로세스당 한 번만 계산한다.

## 6. 하지 못한 것

- `env/Dockerfile` 이미지 빌드와 digest 기록(컨테이너에 Docker 없음)
- 측정 VM에서의 위 절차 반복(측정 VM 없음). 컴파일 결정은 결정적이지만 VM의 CPU가 다르면 `--mcpu`가 LLVM 단계에 미치는 영향을
  다시 확인해야 한다(계획서 §9.3).
