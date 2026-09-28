#!/usr/bin/env bash
# Build the pinned ONNX-MLIR toolchain (spec §5.4, gate G0).
#
# Follows docs/BuildOnLinuxOSX.md and docker/Dockerfile.llvm-project *at the
# pinned ONNX-MLIR commit* (env/toolchain.env). Every deviation from those
# documents is listed in DEVIATIONS below and written to the build manifest.
#
# Usage:
#   env/build_toolchain.sh [stage ...]
#   stages: python protobuf llvm onnx-mlir check-mlir check-onnx-lit all
#   (default: python protobuf llvm onnx-mlir)
#
# Environment:
#   SHAPEPERF_WORK   work dir for sources/builds   (default: $HOME/shapeperf-work)
#   NPROC            build parallelism [pilot]     (default: nproc)
#   LINK_JOBS        parallel link jobs for LLVM   (default: 2)
#   PROTOBUF_PREFIX  protobuf install prefix       (default: /usr/local, as in
#                                                   the official Dockerfile)
#
# Do not run this on the timing VM while measurements are running (spec §5.3).
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=toolchain.env
source "${REPO_ROOT}/env/toolchain.env"

WORK="${SHAPEPERF_WORK:-$HOME/shapeperf-work}"
NPROC="${NPROC:-$(nproc)}"
LINK_JOBS="${LINK_JOBS:-2}"
PROTOBUF_PREFIX="${PROTOBUF_PREFIX:-/usr/local}"
VENV="${WORK}/venv"
LOGDIR="${WORK}/logs"
TIMED="python3 ${REPO_ROOT}/env/timed_run.py"
export CC="${CC:-clang}" CXX="${CXX:-clang++}"

# Deviations from the pinned official build instructions. Build-speed only;
# none of them changes what the compiler generates.
DEVIATIONS=(
  "LLVM: -DLLVM_USE_LINKER=lld and -DLLVM_PARALLEL_LINK_JOBS=${LINK_JOBS} (link speed/memory on a small VM)"
  "LLVM/protobuf/onnx-mlir: shallow git fetch of the pinned SHA instead of a full clone"
  "protobuf: python wheel from PyPI (protobuf==6.33.5) instead of a bazel build of the python package"
  "python deps installed into a venv (${VENV}) instead of --prefix=/usr"
)

mkdir -p "${WORK}" "${LOGDIR}"
log() { echo "[build_toolchain $(date -u +%H:%M:%S)] $*"; }

fetch_sha() {  # fetch_sha <repo-url> <sha-or-tag> <dir> [--recursive]
  local url=$1 ref=$2 dir=$3 rec=${4:-}
  if [[ ! -d "${dir}/.git" ]]; then
    git init -q "${dir}"
    git -C "${dir}" remote add origin "${url}"
  fi
  git -C "${dir}" fetch -q --depth 1 origin "${ref}"
  git -C "${dir}" checkout -q FETCH_HEAD
  if [[ "${rec}" == "--recursive" ]]; then
    git -C "${dir}" submodule update -q --init --recursive --depth 1
  fi
}

stage_python() {
  log "python venv at ${VENV}"
  python3 -m venv "${VENV}"
  "${VENV}/bin/pip" install -q --upgrade pip setuptools packaging
  "${VENV}/bin/pip" install -q -r "${REPO_ROOT}/env/requirements.in"
  "${VENV}/bin/pip" freeze --all > "${WORK}/requirements.freeze.txt"
}

stage_protobuf() {
  log "protobuf ${PROTOBUF_TAG} -> ${PROTOBUF_PREFIX}"
  fetch_sha "${PROTOBUF_REPO}" "${PROTOBUF_TAG}" "${WORK}/protobuf" --recursive
  mkdir -p "${WORK}/protobuf/build"
  cd "${WORK}/protobuf/build"
  cmake -G Ninja -DCMAKE_INSTALL_PREFIX="${PROTOBUF_PREFIX}" \
        -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=ON -Dprotobuf_BUILD_TESTS=OFF .. \
        > "${LOGDIR}/protobuf.configure.log" 2>&1
  ${TIMED} --out "${LOGDIR}/protobuf.build.json" --label protobuf-build -- \
    cmake --build . --parallel "${NPROC}" > "${LOGDIR}/protobuf.build.log" 2>&1
  cmake --install . > "${LOGDIR}/protobuf.install.log" 2>&1
  ldconfig || true
}

stage_llvm() {
  log "llvm ${LLVM_SHA}"
  fetch_sha "${LLVM_REPO}" "${LLVM_SHA}" "${WORK}/llvm-project"
  mkdir -p "${WORK}/llvm-project/build"
  cd "${WORK}/llvm-project/build"
  # Flags from docs/BuildOnLinuxOSX.md (utils/build-mlir.sh) at ONNX_MLIR_SHA,
  # which also builds clang + the OpenMP runtime (needed for OMP support in
  # onnx-mlir: src/CMakeLists.txt looks for omp.h under lib/clang/*/include).
  cmake -G Ninja ../llvm \
     -DLLVM_ENABLE_PROJECTS="mlir;clang" \
     -DLLVM_ENABLE_RUNTIMES="openmp" \
     -DLLVM_TARGETS_TO_BUILD="host" \
     -DCMAKE_BUILD_TYPE=Release \
     -DLLVM_ENABLE_ASSERTIONS=ON \
     -DLLVM_ENABLE_RTTI=ON \
     -DLLVM_ENABLE_LIBEDIT=OFF \
     -DLLVM_USE_LINKER=lld \
     -DLLVM_PARALLEL_LINK_JOBS="${LINK_JOBS}" \
     > "${LOGDIR}/llvm.configure.log" 2>&1
  ${TIMED} --out "${LOGDIR}/llvm.build.json" --label llvm-build -- \
    cmake --build . --parallel "${NPROC}" > "${LOGDIR}/llvm.build.log" 2>&1
}

stage_check_mlir() {
  cd "${WORK}/llvm-project/build"
  ${TIMED} --out "${LOGDIR}/check-mlir.json" --label check-mlir -- \
    cmake --build . --parallel "${NPROC}" --target check-mlir \
    > "${LOGDIR}/check-mlir.log" 2>&1
}

stage_onnx_mlir() {
  log "onnx-mlir ${ONNX_MLIR_TAG} (${ONNX_MLIR_SHA})"
  fetch_sha "${ONNX_MLIR_REPO}" "${ONNX_MLIR_SHA}" "${WORK}/onnx-mlir" --recursive
  test "$(git -C "${WORK}/onnx-mlir" rev-parse HEAD)" = "${ONNX_MLIR_SHA}"
  # The LLVM pin must be the one this onnx-mlir commit asks for.
  grep -q "${LLVM_SHA}" "${WORK}/onnx-mlir/utils/clone-mlir.sh"
  mkdir -p "${WORK}/onnx-mlir/build"
  cd "${WORK}/onnx-mlir/build"
  # Flags from utils/install-onnx-mlir.sh at ONNX_MLIR_SHA (pythonLocation branch).
  cmake -G Ninja \
        -DCMAKE_C_COMPILER="$(command -v "${CC}")" \
        -DCMAKE_CXX_COMPILER="$(command -v "${CXX}")" \
        -DCMAKE_BUILD_TYPE=Release \
        -DLLVM_ENABLE_ASSERTIONS=ON \
        -DPython3_ROOT_DIR="${VENV}" \
        -DPython3_EXECUTABLE="${VENV}/bin/python" \
        -DMLIR_DIR="${WORK}/llvm-project/build/lib/cmake/mlir" \
        .. > "${LOGDIR}/onnx-mlir.configure.log" 2>&1
  ${TIMED} --out "${LOGDIR}/onnx-mlir.build.json" --label onnx-mlir-build -- \
    cmake --build . --parallel "${NPROC}" > "${LOGDIR}/onnx-mlir.build.log" 2>&1
}

stage_check_onnx_lit() {
  cd "${WORK}/onnx-mlir/build"
  LIT_OPTS=-v ${TIMED} --out "${LOGDIR}/check-onnx-lit.json" --label check-onnx-lit -- \
    cmake --build . --target check-onnx-lit > "${LOGDIR}/check-onnx-lit.log" 2>&1
}

write_manifest() {
  local out="${WORK}/build_manifest.json"
  python3 - "$out" <<PY
import json, os, subprocess, sys
def sh(c):
    try: return subprocess.run(c, shell=True, capture_output=True, text=True).stdout.strip()
    except Exception: return "unavailable"
work = "${WORK}"
m = {
  "onnx_mlir": {"tag": "${ONNX_MLIR_TAG}", "sha": "${ONNX_MLIR_SHA}",
                "checked_out": sh(f"git -C {work}/onnx-mlir rev-parse HEAD 2>/dev/null") or "not built"},
  "llvm": {"sha": "${LLVM_SHA}",
           "checked_out": sh(f"git -C {work}/llvm-project rev-parse HEAD 2>/dev/null") or "not built"},
  "protobuf": {"tag": "${PROTOBUF_TAG}", "prefix": "${PROTOBUF_PREFIX}"},
  "cc": sh("${CC} --version | head -1"), "cxx": sh("${CXX} --version | head -1"),
  "cmake": sh("cmake --version | head -1"), "ninja": sh("ninja --version"),
  "nproc_used": ${NPROC}, "link_jobs": ${LINK_JOBS},
  "deviations": [l for l in """$(printf '%s\n' "${DEVIATIONS[@]}")""".splitlines() if l],
  "stage_logs": {f: json.load(open(os.path.join("${LOGDIR}", f)))
                 for f in sorted(os.listdir("${LOGDIR}")) if f.endswith(".json")},
  "disk_usage": sh(f"du -sh {work}/llvm-project/build {work}/onnx-mlir/build 2>/dev/null"),
}
json.dump(m, open(sys.argv[1], "w"), indent=2)
print("manifest:", sys.argv[1])
PY
}

stages=("$@")
[[ ${#stages[@]} -eq 0 ]] && stages=(python protobuf llvm onnx-mlir)
[[ "${stages[0]}" == "all" ]] && stages=(python protobuf llvm onnx-mlir check-mlir check-onnx-lit)
for s in "${stages[@]}"; do
  case "$s" in
    python) stage_python ;;
    protobuf) stage_protobuf ;;
    llvm) stage_llvm ;;
    check-mlir) stage_check_mlir ;;
    onnx-mlir) stage_onnx_mlir ;;
    check-onnx-lit) stage_check_onnx_lit ;;
    *) echo "unknown stage: $s" >&2; exit 2 ;;
  esac
  write_manifest
done
log "done: ${stages[*]}"
