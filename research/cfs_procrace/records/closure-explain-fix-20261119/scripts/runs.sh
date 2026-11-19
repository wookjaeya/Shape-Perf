#!/usr/bin/env bash
# runs.sh STEP [ARG] -- the runs of results 26.3 after the code commit (no boot, no build).  Every output goes into a new
# directory; each step runs under env/tools/space_guard.sh with its log in ../guard/.  Inputs are the committed code and
# the kept outputs named below; no row id, key or expected verdict is an argument.
#   fixtures            env/obs1/tests/test_explain4.py into <scratch>/fx (counts in ../out/fixtures.txt)
#   evalfx              env/obs1/tests/test_explain_eval.py into <scratch>/evfx (../out/eval_fixtures.txt)
#   runcycle            boot/test_run_cycle.sh into <scratch>/rct (../out/run_cycle_tests.txt)
#   unit                env/tools/tests/run_unit.sh and env/redesign/step2a_tests_unit.sh (../out/unit_*.txt)
#   regress             revision 4 as revised on the 2026-11-16 inputs, the eight D1 branches -> obs1/xr5_regress_rev4fix
#   static BUILD        the real static outputs of BUILD with the 16 branches, the candidate list and the named-calls file,
#                       no run -> obs1/xr5_dry_static_BUILD
#   rtread NAME         the runtime step's test dump NAME (kz-rt-dump of the MC) with the c1_pre_tx plan -> obs1/xr5_dry_rt_NAME
#   synth BUILD ORDER   a SYNTHETIC record (scripts/synth_record.py; order script or reverse) on the real plan of BUILD and
#                       the explanation with the 16 branches -> obs1/xr5_synth_BUILD_ORDER (a test input, never evidence)
#   eval DIR BUILD      explain_eval.py on an explanation output DIR of BUILD with the test expectation file (no gate)
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
R="$(cd "$HERE/../.." && pwd)"
O=/home/user/work/redesign/obs1
MC=/home/user/work/merge-check/kz-llvm
EX=$R/env/obs1
VOC=$R/kairos/tests/static/data/reason_vocabulary.txt
PREP=$R/records/closure-c1prep-20261119
SCR=${RUNS_SCRATCH:?RUNS_SCRATCH (a scratch directory for fixture outputs) is required}
G=$R/env/tools/space_guard.sh
step=$1; a1=${2:-}; a2=${3:-}
g() { local k=$1 n=$2; shift 2; "$G" --need "$n" --kind "$k" --log "$HERE/guard/$k.log" -- "$@"; }
NSARGS() { for f in $(grep '^input namespace-data ' "$1" | sed 's/.* path=//'); do printf -- '--namespaces\n%s\n' "$f"; done; }
case "$step" in
  fixtures) g fixtures 150000000 python3 -I "$EX/tests/test_explain4.py" "$EX" "$VOC" "$SCR/fx" > "$HERE/out/fixtures.txt" 2>&1; rc=$?; tail -1 "$HERE/out/fixtures.txt"; exit $rc ;;
  evalfx) g evalfx 20000000 python3 -I "$EX/tests/test_explain_eval.py" "$EX" "$SCR/evfx" > "$HERE/out/eval_fixtures.txt" 2>&1; rc=$?; tail -1 "$HERE/out/eval_fixtures.txt"; exit $rc ;;
  runcycle) g runcycle 10000000 bash "$PREP/boot/test_run_cycle.sh" "$SCR/rct" > "$HERE/out/run_cycle_tests.txt" 2>&1; rc=$?; tail -1 "$HERE/out/run_cycle_tests.txt"; exit $rc ;;
  unit)
    g unit-tools 200000000 bash "$R/env/tools/tests/run_unit.sh" > "$HERE/out/unit_tools.txt" 2>&1; r1=$?
    g unit-step2a 50000000 bash "$R/env/redesign/step2a_tests_unit.sh" > "$HERE/out/unit_step2a.txt" 2>&1; r2=$?
    tail -1 "$HERE/out/unit_tools.txt"; tail -1 "$HERE/out/unit_step2a.txt"; exit $((r1 | r2)) ;;
  regress)
    OUT=$O/xr5_regress_rev4fix
    g regress 40000000 bash "$R/records/closure-explain-rev4-20261119/scripts/regress.sh" 4 "$OUT" "" "$R/records/closure-explain-rev4-20261119/out/branches_d1.txt" \
      > "$HERE/out/regress.log" 2>&1; rc=$?; tail -10 "$HERE/out/regress.log"; exit $rc ;;
  static)
    b=$a1; OUT=$O/xr5_dry_static_$b; D=$O/xr4_dry_inputs/$b; P=$O/cz_plans_v4_ns2/$b; Z=$O/cz_final_order/$b
    mapfile -t NS < <(NSARGS "$P/plan.side")
    g "static-$b" 60000000 python3 -I "$EX/explain.py" --sidecar "$P/plan.side" --manifest "$P/plan.kzs" --bindings "$D/binds_bindcheck.kzs" \
      --relations "$Z/relations.txt" --keys "$Z/keys.txt" --pairs "$Z/pairs.tsv" "${NS[@]}" --facts-view "$D/facts_view.txt" \
      --bprime-basis "$O/cz_final_extract/$b/$b/NP/basis.kz" --build-label "$b" --branches "$PREP/candidates/branches_16.txt" \
      --candidates "$PREP/candidates/candidates.tsv" --named-calls "$PREP/candidates/named_calls.tsv" \
      --label-record "dry run on the static outputs with no run (results 26.3)" --vocabulary "$VOC" --out "$OUT" > "$HERE/out/dry_static_$b.log" 2>&1; rc=$?
    tail -20 "$HERE/out/dry_static_$b.log"; exit $rc ;;
  rtread)
    n=$a1; OUT=$O/xr5_dry_rt_$n; [ ! -e "$OUT" ] || { echo "$OUT exists" >&2; exit 2; }
    g "rt-$n" 30000000 env DRYRUN_OUT="$OUT" bash -c '
      set -uo pipefail; OUT=$DRYRUN_OUT; R='"$R"'; O='"$O"'; MC='"$MC"'; EX='"$EX"'; VOC='"$VOC"'; PREP='"$PREP"'; b='"$n"'
      mkdir -p "$OUT" || exit 2
      P=$O/cz_plans_v4_ns2/c1_pre_tx; Z=$O/cz_final_order/c1_pre_tx; D=$O/xr4_dry_inputs/c1_pre_tx
      "$MC/runtime/kz-rt-dump" "$MC/runtime/tests/$b.kzrt" > "$OUT/$b.dump.txt"; echo "kz-rt-dump rc=$?"
      printf "# dry-run stand-in for run.meta (no boot): no manifest, file or stop line\n" > "$OUT/run.meta"
      printf "# dry-run stand-in for a startup script (no boot)\n!\n" > "$OUT/startup.scr"
      { echo "# dry-run stand-in build record: the runtime step test executable and test modules"
        echo "manifest sha256 $(sha256sum < "$P/plan.kzs" | cut -c1-64)"
        for f in test_rt_objects librt_objects_mod_id.so librt_objects_mod_noid.so; do echo "$f sha256 $(sha256sum < "$MC/runtime/tests/$f" | cut -c1-64)"; done
      } > "$OUT/build_record.txt"
      python3 -I "$R/env/tools/chain_check.py" --plan "$P/plan.kzs" --build-record "$OUT/build_record.txt" --run-meta "$OUT/run.meta" \
        --dump-text "$OUT/$b.dump.txt" --build-root "$MC/runtime/tests" --main test_rt_objects -o "$OUT/chain.txt"; echo "chain_check rc=$?"
      python3 -I "$EX/explain_read.py" --sidecar "$P/plan.side" --manifest "$P/plan.kzs" --bindings "$D/binds_bindcheck.kzs" \
        --relations "$Z/relations.txt" --keys "$Z/keys.txt" --pairs "$Z/pairs.tsv" --facts-view "$D/facts_view.txt" \
        --bprime-basis "$O/cz_final_extract/c1_pre_tx/c1_pre_tx/NP/basis.kz" --build-label c1_pre_tx --branches "$PREP/candidates/branches_16.txt" \
        --candidates "$PREP/candidates/candidates.tsv" --named-calls "$PREP/candidates/named_calls.tsv" \
        --label-record "dry run on a runtime test dump (no plan of that dump; results 26.3)" --dump-text "$OUT/$b.dump.txt" --run-meta "$OUT/run.meta" \
        --chain "$OUT/chain.txt" --startup-script "$OUT/startup.scr" --normal-out "$OUT/normal.json"; echo "reader rc=$?"
      python3 -I "$EX/explain_judge.py" --normal "$OUT/normal.json" --vocabulary "$VOC" --out "$OUT/judge"; echo "judge rc=$?"' \
      > "$HERE/out/dry_rt_$n.log" 2>&1; rc=$?; cat "$HERE/out/dry_rt_$n.log" | tail -25; exit $rc ;;
  synth)
    b=$a1; ord=$a2; SY=$O/xr5_synth_${b}_${ord}; P=$O/cz_plans_v4_ns2/$b; Z=$O/cz_final_order/$b; D=$O/xr4_dry_inputs/$b
    case "$b" in c1_pre_tx) s=pre ;; c1_post_tx) s=post ;; *) exit 64 ;; esac
    W=$O/c1p2_${s}_derived
    [ ! -e "$SY" ] || { echo "$SY exists" >&2; exit 2; }
    mkdir -p "$SY" || exit 2
    g "synth-$b-$ord" 60000000 python3 -I "$HERE/scripts/synth_record.py" --manifest "$P/plan.kzs" --sidecar "$P/plan.side" --relations "$Z/relations.txt" \
      --pairs "$Z/pairs.tsv" --bindings "$W/binds_instr.kzs" --startup-script "$W/ws/exe/tx/cf/cfe_es_startup.scr" --order "$ord" --out "$SY/record" \
      > "$HERE/out/synth_${b}_${ord}.log" 2>&1 || { cat "$HERE/out/synth_${b}_${ord}.log"; exit 1; }
    mapfile -t NS < <(NSARGS "$P/plan.side")
    g "synth-explain-$b-$ord" 60000000 python3 -I "$EX/explain.py" --sidecar "$P/plan.side" --manifest "$P/plan.kzs" --bindings "$W/binds_instr.kzs" \
      --relations "$Z/relations.txt" --keys "$Z/keys.txt" --pairs "$Z/pairs.tsv" "${NS[@]}" --facts-view "$D/facts_view.txt" \
      --bprime-basis "$O/c1p2_${s}_bprime/$b/NP/basis.kz" --build-label "$b" --branches "$PREP/candidates/branches_16.txt" \
      --candidates "$PREP/candidates/candidates.tsv" --named-calls "$PREP/candidates/named_calls.tsv" \
      --dump-text "$SY/record/tx.dump.txt" --run-meta "$SY/record/run.meta" --chain "$SY/record/chain.txt" --startup-script "$SY/record/startup.scr" \
      --label-record "SYNTHETIC use events (test input, never evidence; results 26.3)" --vocabulary "$VOC" --out "$SY/explain" >> "$HERE/out/synth_${b}_${ord}.log" 2>&1; rc=$?
    echo "explain rc=$rc" >> "$HERE/out/synth_${b}_${ord}.log"; tail -22 "$HERE/out/synth_${b}_${ord}.log"; exit $rc ;;
  eval)
    XD=$a1; b=$a2; P=$O/cz_plans_v4_ns2/$b; Z=$O/cz_final_order/$b; nm=$(basename "$XD"); [ "$nm" = explain ] && nm=$(basename "$(dirname "$XD")")
    g "eval-$nm" 20000000 python3 -I "$EX/explain_eval.py" --explain "$XD" --build "$b" --boot "$nm" --gate none --pairs "$Z/pairs.tsv" \
      --sidecar "$P/plan.side" --relations "$Z/relations.txt" --uses "$PREP/uses/uses_$b.tsv" --sheets-static "$PREP/sheets/sheets_static.tsv" \
      --sheets-boot none --expect "$HERE/out/test_expect_c1_with_class.tsv" --out "$HERE/out/eval_$nm.txt" > "$HERE/out/eval_$nm.log" 2>&1; rc=$?
    cat "$HERE/out/eval_$nm.log"; exit $rc ;;
  *) echo "runs.sh fixtures|evalfx|runcycle|unit|regress|static B|rtread N|synth B ORDER|eval DIR B" >&2; exit 64 ;;
esac
