#!/usr/bin/env bash
# plugin05.sh STEP -- the kzgcc 0.5 build and its checks (results 25.5 (na)), each under the space guard, logs into ../guard,
# records into ../plugin05, outputs into new directories.
#   build     cmake -S kairos/instrument/gcc -B /home/user/work/instr/kzgcc-0.5/plugin-build (KZGCC_KZ = the MC kz), build
#   ctest     the plugin's ctest, first run with -V
#   regress   versions 1-3 manifests (records/closure-runtime-20261119/scripts/regress_manifest.sh) -> obs1/c1p_regress_manifest_k05,
#             compared with kzgcc 0.4's obs1/rtb_regress_manifest_new (compare_manifest_2.py 0.4 0.5)
#   units     every planned unit of both C1 plans with kzgcc 0.4 and with kzgcc 0.5 (unit_check.py) -> obs1/c1p_unitcheck_k04_<b>,
#             obs1/c1p_unitcheck_k05_<b>, compared unit by unit (compare_units.py)
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; D="$(cd "$HERE/.." && pwd)"; R="$(cd "$D/../.." && pwd)"
GUARD=$R/env/tools/space_guard.sh; O=/home/user/work/redesign/obs1; PB=/home/user/work/instr/kzgcc-0.5/plugin-build
K04=/home/user/work/instr/kzgcc-0.4/plugin-build/kzgcc.so; KZ=/home/user/work/merge-check/kz-llvm/tools/kz/kz
g() { local k=$1 n=$2; shift 2; "$GUARD" --need "$n" --kind "$k" --log "$D/guard/p_$k.log" -- "$@"; }
mkdir -p "$D/plugin05"
case "${1:-}" in
  build)
    [ ! -e "$PB" ] || { echo "refusing: $PB exists" >&2; exit 2; }
    mkdir -p "$PB" && g plugin-build 40000000 bash -c "cmake -S '$R/kairos/instrument/gcc' -B '$PB' -DKZGCC_KZ='$KZ' > '$D/plugin05/configure.log' 2>&1 && cmake --build '$PB' -j4 > '$D/plugin05/build.log' 2>&1"; rc=$?
    { echo "build rc=$rc"; echo "warnings: $(grep -c 'warning:' "$D/plugin05/build.log")"; sha256sum "$PB/kzgcc.so"; strings "$PB/kzgcc.so" | grep -E '^kzgcc [0-9]+\.[0-9]+$' | sort -u; } > "$D/plugin05/build_result.txt"; cat "$D/plugin05/build_result.txt"; exit $rc ;;
  ctest) ( cd "$PB" && ctest -V ) > "$D/plugin05/ctest_first.log" 2>&1; rc=$?; tail -3 "$D/plugin05/ctest_first.log"; exit $rc ;;
  regress)
    g regress 20000000 bash "$R/records/closure-runtime-20261119/scripts/regress_manifest.sh" "$O/c1p_regress_manifest_k05" "$PB/kzgcc.so" > "$D/plugin05/regress_manifest.txt" 2>&1 || exit 1
    python3 -I "$HERE/compare_manifest_2.py" "$O/rtb_regress_manifest_new" "$O/c1p_regress_manifest_k05" 0.4 0.5 > "$D/plugin05/compare_manifest_k04_k05.txt" 2>&1; rc=$?; cat "$D/plugin05/compare_manifest_k04_k05.txt"; exit $rc ;;
  units)
    rc=0
    for b in pre post; do
      P=$O/cz_plans_v4_ns2/c1_${b}_tx; CDB=/home/user/work/procrace/c1_build/$b/1df262a/build/native/default/compile_commands.json
      for v in 04 05; do
        pl=$K04; [ $v = 05 ] && pl=$PB/kzgcc.so
        g unitcheck-k${v}-$b 30000000 python3 -I "$HERE/unit_check.py" --plan "$P/plan.kzs" --modules "$P/plan.mod" --compile-db "$CDB" --plugin "$pl" --out "$O/c1p_unitcheck_k${v}_$b" > "$D/plugin05/unitcheck_k${v}_$b.txt" 2>&1
        echo "unit_check k$v $b rc=$?" >> "$D/plugin05/unitcheck_k${v}_$b.txt"
      done
      python3 -I "$HERE/compare_units.py" "$O/c1p_unitcheck_k04_$b" "$O/c1p_unitcheck_k05_$b" > "$D/plugin05/compare_units_k04_k05_$b.txt" 2>&1 || rc=1
      tail -2 "$D/plugin05/compare_units_k04_k05_$b.txt"
    done; exit $rc ;;
  *) echo "usage: plugin05.sh build|ctest|regress|units" >&2; exit 64 ;;
esac
