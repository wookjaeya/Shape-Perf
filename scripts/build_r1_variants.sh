#!/usr/bin/env bash
# R1 compiler variants: r1a = I1, r1b = I1+I2, r1c = I2; then revert and check the original hash.
set -euo pipefail
W=/home/user/work; O=$W/onnx-mlir; P=/home/user/Shape-Perf/patches
cd $O
build() { echo "== build $1 $(date -u +%T)"; (cd build && ninja -j4 onnx-mlir) | tail -3;
          mkdir -p $W/variants/$1/bin; cp -p build/Release/bin/onnx-mlir $W/variants/$1/bin/;
          ln -sfn $O/build/Release/lib $W/variants/$1/lib; sha256sum $W/variants/$1/bin/onnx-mlir; }
git diff --quiet -- src/Conversion/KrnlToLLVM/ConvertKrnlToLLVM.cpp && { echo "I1 not applied"; exit 1; }
build r1a
git apply $P/r1_i2_gelu_pow_to_mul.patch
build r1b
git checkout -- src/Conversion/KrnlToLLVM/ConvertKrnlToLLVM.cpp
build r1c
git checkout -- src/Conversion/ONNXToKrnl/Math/Elementwise.cpp
echo "== revert $(date -u +%T)"; git status --short
(cd build && ninja -j4 onnx-mlir) | tail -2
sha256sum build/Release/bin/onnx-mlir
echo "== done $(date -u +%T)"
