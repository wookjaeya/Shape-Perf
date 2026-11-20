#!/usr/bin/env bash
# runs.sh STEP [ARG...] -- the runs of results 26.5 (the review fixes after the re-freeze; no boot, no build).  Every step
# that writes runs under env/tools/space_guard.sh with its log in ../guard/.  Large outputs (explanation records, about
# 25-50 MB each; evaluation outputs, about 1 MB each) are written into the scratch directory RUNS_SCRATCH, compared with
# the kept outputs of results 26.3, and removed after their comparison, sha256 list and tail are copied into ../out or
# ../compare (the free space is short of the boots' need: results 26.4 open point 1).  No row id, key or expected verdict
# is an argument of any tool.
#   fixtures | evalfx | runcycle      the three fixture sets into RUNS_SCRATCH (../out/<step>.txt)
#   unit                              env/tools/tests/run_unit.sh and env/redesign/step2a_tests_unit.sh (../out/unit_*.txt)
#   regress                           the 2026-11-16 inputs, the eight D1 branches, with the code of this step; compared
#                                     with obs1/xr5_regress_rev4fix (../compare/regress.txt)
#   static BUILD                      the static outputs with the 16 branches, no run; compared with obs1/xr5_dry_static_BUILD
#   rtread NAME                       the runtime step's test dump NAME; compared with obs1/xr5_dry_rt_NAME
#   synth BUILD ORDER                 the kept SYNTHETIC record of obs1/xr5_synth_BUILD_ORDER (a test input, never evidence)
#                                     explained again; compared with its kept explanation; explain_expect.py with
#                                     expect_c1_2.tsv on the new output (../compare/synth_BUILD_ORDER_expect.txt)
#   loss BUILD ORDER TAG LOSSLINE     the same SYNTHETIC record with one loss line added before its format line (spec 11.5
#                                     item 1): explained, compared with the kept explanation and with expect_c1_2.tsv
#   eval DIR BUILD NAME               the falsifier and evaluation script of this step on a kept explanation output DIR
#                                     with expect_c1_2.tsv, no gate (../out/eval_NAME_tail.txt: head, tail, ED lines, sha256)
#   summary BUILD                     the frozen summary over the two kept synthetic outputs of BUILD as two stand-in boots,
#                                     with the uses list (../out/summary_synth_BUILD.txt and its --check-summary)
#   namedcalls                        the committed named_calls.py with the arguments of results 26.3's g01 run into
#                                     ../out/named_calls_rerun_after_commit.tsv, compared with candidates/named_calls.tsv
#   expectregen                       expect_write2.py with predictions3.tsv into RUNS_SCRATCH, compared with expect_c1_2.tsv
#                                     and elements_by_rule_2.tsv (../out/expect_regen_cmp.txt)
#   classify2 BUILD ORDER             scripts/classify_compare2.py on results 26.4's comparison of that synthetic output
#   sizes                             du -sb and du -sB1 of obs1/xr5_* (../out/xr5_sizes.txt)
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
R="$(cd "$HERE/../.." && pwd)"
O=/home/user/work/redesign/obs1
MC=/home/user/work/merge-check/kz-llvm
EX=$R/env/obs1
VOC=$R/kairos/tests/static/data/reason_vocabulary.txt
PREP=$R/records/closure-c1prep-20261119
FIXR=$R/records/closure-explain-fix-20261119
PRE=$R/records/closure-preboot-20261119
CMPR=$FIXR/scripts/compare_records.py
SCR=${RUNS_SCRATCH:?RUNS_SCRATCH (a scratch directory) is required}
G=$R/env/tools/space_guard.sh
step=${1:?usage: runs.sh STEP}; a1=${2:-}; a2=${3:-}; a3=${4:-}; a4=${5:-}
g() { local k=$1 n=$2; shift 2; "$G" --need "$n" --kind "$k" --log "$HERE/guard/$k.log" -- "$@"; }
NSARGS() { for f in $(grep '^input namespace-data ' "$1" | sed 's/.* path=//'); do printf -- '--namespaces\n%s\n' "$f"; done; }
fresh() { [ ! -e "$1" ] || { echo "runs.sh: $1 exists" >&2; exit 2; }; }
keep_sha() { (cd "$1" && find . -type f | LC_ALL=C sort | xargs -d '\n' sha256sum) > "$2"; }
synth_explain() { # BUILD RECORD_DIR OUT LABEL: the explanation of a synthetic record as results 26.3 ran it
  local b=$1 rd=$2 out=$3 lab=$4 s P Z D
  case "$b" in c1_pre_tx) s=pre ;; c1_post_tx) s=post ;; *) return 64 ;; esac
  P=$O/cz_plans_v4_ns2/$b; Z=$O/cz_final_order/$b; D=$O/xr4_dry_inputs/$b
  mapfile -t NS < <(NSARGS "$P/plan.side")
  python3 -I "$EX/explain.py" --sidecar "$P/plan.side" --manifest "$P/plan.kzs" --bindings "$O/c1p2_${s}_derived/binds_instr.kzs" \
    --relations "$Z/relations.txt" --keys "$Z/keys.txt" --pairs "$Z/pairs.tsv" "${NS[@]}" --facts-view "$D/facts_view.txt" \
    --bprime-basis "$O/c1p2_${s}_bprime/$b/NP/basis.kz" --build-label "$b" --branches "$PREP/candidates/branches_16.txt" \
    --candidates "$PREP/candidates/candidates.tsv" --named-calls "$PREP/candidates/named_calls.tsv" \
    --dump-text "$rd/tx.dump.txt" --run-meta "$rd/run.meta" --chain "$rd/chain.txt" --startup-script "$rd/startup.scr" \
    --label-record "$lab" --vocabulary "$VOC" --out "$out"
}
case "$step" in
  fixtures) fresh "$HERE/out/fixtures.txt"
    g fixtures 150000000 python3 -I "$EX/tests/test_explain4.py" "$EX" "$VOC" "$SCR/fx" > "$HERE/out/fixtures.txt" 2>&1; rc=$?
    tail -1 "$HERE/out/fixtures.txt"; rm -rf "$SCR/fx"; exit $rc ;;
  evalfx) fresh "$HERE/out/eval_fixtures.txt"
    g evalfx 20000000 python3 -I "$EX/tests/test_explain_eval.py" "$EX" "$SCR/evfx" > "$HERE/out/eval_fixtures.txt" 2>&1; rc=$?
    tail -1 "$HERE/out/eval_fixtures.txt"; rm -rf "$SCR/evfx"; exit $rc ;;
  runcycle) fresh "$HERE/out/run_cycle_tests.txt"
    g runcycle 10000000 bash "$PREP/boot/test_run_cycle.sh" "$SCR/rct" > "$HERE/out/run_cycle_tests.txt" 2>&1; rc=$?
    tail -1 "$HERE/out/run_cycle_tests.txt"; rm -rf "$SCR/rct"; exit $rc ;;
  unit) fresh "$HERE/out/unit_tools.txt"
    g unit-tools 200000000 bash "$R/env/tools/tests/run_unit.sh" > "$HERE/out/unit_tools.txt" 2>&1; r1=$?
    g unit-step2a 50000000 bash "$R/env/redesign/step2a_tests_unit.sh" > "$HERE/out/unit_step2a.txt" 2>&1; r2=$?
    tail -1 "$HERE/out/unit_tools.txt"; tail -1 "$HERE/out/unit_step2a.txt"; exit $((r1 | r2)) ;;
  regress) OUT=$SCR/xr6_regress; fresh "$OUT"
    g regress 40000000 bash "$R/records/closure-explain-rev4-20261119/scripts/regress.sh" 4 "$OUT" "" "$R/records/closure-explain-rev4-20261119/out/branches_d1.txt" \
      > "$HERE/out/regress.log" 2>&1; rc=$?; tail -4 "$HERE/out/regress.log"; [ $rc = 0 ] || exit $rc
    python3 -I "$CMPR" "$O/xr5_regress_rev4fix" "$OUT" > "$HERE/compare/regress.txt" 2>&1; rc=$?
    keep_sha "$OUT" "$HERE/out/regress_sha256.txt"; rm -rf "$OUT"; grep -c 'equal:' "$HERE/compare/regress.txt"; exit $rc ;;
  static) b=$a1; OUT=$SCR/xr6_dry_static_$b; fresh "$OUT"; D=$O/xr4_dry_inputs/$b; P=$O/cz_plans_v4_ns2/$b; Z=$O/cz_final_order/$b
    mapfile -t NS < <(NSARGS "$P/plan.side")
    g "static-$b" 60000000 python3 -I "$EX/explain.py" --sidecar "$P/plan.side" --manifest "$P/plan.kzs" --bindings "$D/binds_bindcheck.kzs" \
      --relations "$Z/relations.txt" --keys "$Z/keys.txt" --pairs "$Z/pairs.tsv" "${NS[@]}" --facts-view "$D/facts_view.txt" \
      --bprime-basis "$O/cz_final_extract/$b/$b/NP/basis.kz" --build-label "$b" --branches "$PREP/candidates/branches_16.txt" \
      --candidates "$PREP/candidates/candidates.tsv" --named-calls "$PREP/candidates/named_calls.tsv" \
      --label-record "dry run on the static outputs with no run (results 26.3)" --vocabulary "$VOC" --out "$OUT" > "$HERE/out/dry_static_$b.log" 2>&1; rc=$?
    tail -3 "$HERE/out/dry_static_$b.log"; [ $rc = 0 ] || exit $rc
    python3 -I "$CMPR" "$O/xr5_dry_static_$b" "$OUT" > "$HERE/compare/static_$b.txt" 2>&1; rc=$?
    keep_sha "$OUT" "$HERE/out/dry_static_${b}_sha256.txt"; rm -rf "$OUT"; exit $rc ;;
  rtread) n=$a1; OUT=$SCR/xr6_dry_rt_$n; fresh "$OUT"; K=$O/xr5_dry_rt_$n
    g "rt-$n" 30000000 env DRYRUN_OUT="$OUT" bash -c '
      set -uo pipefail; OUT=$DRYRUN_OUT; R='"$R"'; O='"$O"'; EX='"$EX"'; VOC='"$VOC"'; PREP='"$PREP"'; K='"$K"'; b='"$n"'
      mkdir -p "$OUT" || exit 2
      P=$O/cz_plans_v4_ns2/c1_pre_tx; Z=$O/cz_final_order/c1_pre_tx; D=$O/xr4_dry_inputs/c1_pre_tx
      python3 -I "$EX/explain_read.py" --sidecar "$P/plan.side" --manifest "$P/plan.kzs" --bindings "$D/binds_bindcheck.kzs" \
        --relations "$Z/relations.txt" --keys "$Z/keys.txt" --pairs "$Z/pairs.tsv" --facts-view "$D/facts_view.txt" \
        --bprime-basis "$O/cz_final_extract/c1_pre_tx/c1_pre_tx/NP/basis.kz" --build-label c1_pre_tx --branches "$PREP/candidates/branches_16.txt" \
        --candidates "$PREP/candidates/candidates.tsv" --named-calls "$PREP/candidates/named_calls.tsv" \
        --label-record "dry run on a runtime test dump (no plan of that dump; results 26.3)" --dump-text "$K/$b.dump.txt" --run-meta "$K/run.meta" \
        --chain "$K/chain.txt" --startup-script "$K/startup.scr" --normal-out "$OUT/normal.json"; echo "reader rc=$?"
      cmp "$OUT/normal.json" "$K/normal.json" && echo "normal.json byte-identical to the kept one"
      python3 -I "$EX/explain_judge.py" --normal "$OUT/normal.json" --vocabulary "$VOC" --out "$OUT/judge"; echo "judge rc=$?"' \
      > "$HERE/out/dry_rt_$n.log" 2>&1; rc=$?; tail -4 "$HERE/out/dry_rt_$n.log"
    python3 -I "$CMPR" "$K/judge" "$OUT/judge" > "$HERE/compare/rt_$n.txt" 2>&1
    keep_sha "$OUT" "$HERE/out/dry_rt_${n}_sha256.txt"; rm -rf "$OUT"; exit $rc ;;
  synth) b=$a1; ord=$a2; K=$O/xr5_synth_${b}_${ord}; OUT=$SCR/xr6_synth_${b}_${ord}; fresh "$OUT"
    g "synth-$b-$ord" 60000000 bash -c "$(declare -f synth_explain NSARGS); O='$O'; EX='$EX'; VOC='$VOC'; PREP='$PREP'; synth_explain '$b' '$K/record' '$OUT' 'SYNTHETIC use events (test input, never evidence; results 26.3), explained again (results 26.5)'" \
      > "$HERE/out/synth_${b}_${ord}.log" 2>&1; rc=$?; tail -3 "$HERE/out/synth_${b}_${ord}.log"; [ $rc = 0 ] || exit $rc
    python3 -I "$CMPR" "$K/explain" "$OUT" > "$HERE/compare/synth_${b}_${ord}_records.txt" 2>&1; r1=$?
    python3 -I "$EX/explain_expect.py" --expect "$PREP/expect/expect_c1_2.tsv" --records "$OUT" --out "$HERE/compare/synth_${b}_${ord}_expect.txt" > /dev/null 2>&1; r2=$?
    tail -2 "$HERE/compare/synth_${b}_${ord}_expect.txt"
    keep_sha "$OUT" "$HERE/out/synth_${b}_${ord}_sha256.txt"; rm -rf "$OUT"; exit $((r1 | r2)) ;;
  loss) b=$a1; ord=$a2; tag=$a3; line=$a4; K=$O/xr5_synth_${b}_${ord}; W=$SCR/xr6_loss_$tag; fresh "$W"
    mkdir -p "$W/record" && cp "$K/record/run.meta" "$K/record/chain.txt" "$K/record/startup.scr" "$W/record/" || exit 2
    awk -v L="$line" '/^format /{print L} {print}' "$K/record/tx.dump.txt" > "$W/record/tx.dump.txt" || exit 2
    { echo "the kept synthetic record of $K with one line added before its format line: $line"
      diff "$K/record/tx.dump.txt" "$W/record/tx.dump.txt"; } > "$HERE/out/loss_${tag}_record_diff.txt"
    g "loss-$tag" 60000000 bash -c "$(declare -f synth_explain NSARGS); O='$O'; EX='$EX'; VOC='$VOC'; PREP='$PREP'; synth_explain '$b' '$W/record' '$W/explain' 'SYNTHETIC use events with a loss line added (test input, never evidence; results 26.5)'" \
      > "$HERE/out/loss_$tag.log" 2>&1; rc=$?; tail -3 "$HERE/out/loss_$tag.log"; [ $rc = 0 ] || exit $rc
    python3 -I "$CMPR" "$K/explain" "$W/explain" > "$HERE/compare/loss_${tag}_records.txt" 2>&1; r1=$?
    python3 -I "$EX/explain_expect.py" --expect "$PREP/expect/expect_c1_2.tsv" --records "$W/explain" --out "$HERE/compare/loss_${tag}_expect.txt" > /dev/null 2>&1; r2=$?
    tail -2 "$HERE/compare/loss_${tag}_expect.txt"
    keep_sha "$W" "$HERE/out/loss_${tag}_sha256.txt"; rm -rf "$W"; exit $((r1 | r2)) ;;
  eval) XD=$a1; b=$a2; nm=$a3; P=$O/cz_plans_v4_ns2/$b; Z=$O/cz_final_order/$b; F=$SCR/eval_$nm.txt; fresh "$F"; fresh "$HERE/out/eval_${nm}_tail.txt"
    g "eval-$nm" 20000000 python3 -I "$EX/explain_eval.py" --explain "$XD" --build "$b" --boot "$nm" --gate none --pairs "$Z/pairs.tsv" \
      --sidecar "$P/plan.side" --relations "$Z/relations.txt" --uses "$PREP/uses/uses_$b.tsv" --sheets-static "$PREP/sheets/sheets_static.tsv" \
      --sheets-boot none --expect "$PREP/expect/expect_c1_2.tsv" --out "$F" > "$HERE/out/eval_$nm.log" 2>&1; rc=$?
    cat "$HERE/out/eval_$nm.log"
    if [ -f "$F" ]; then
      { echo "== eval_$nm.txt (lines $(wc -l < "$F"), sha256 $(sha256sum < "$F" | cut -c1-64)); the full output was in the scratch directory and is removed"
        grep -vP '^(F1|F2a|F2b|F3|F4|F5|F6|F7|F8a|F8b|F9|F10|s1|s2|s3|s4|records|expect_discriminate|precision|in_plan_recall|undetermined_split|count)\t' "$F"
        echo "== the expect_discriminate lines"; grep -P '^expect_discriminate\t' "$F"
        echo "== the F3, F4 and F9 lines of the main branch"; grep -P '^(F3|F4|F9)\tboot\tD1-R0\t' "$F"; } > "$HERE/out/eval_${nm}_tail.txt"
      rm -f "$F"
    fi
    exit $rc ;;
  summary) b=$a1; fresh "$HERE/out/summary_synth_$b.txt"
    sha256sum "$EX/explain_summary.py" > "$HERE/out/summary_synth_${b}_script_sha_before.txt"
    g "summary-$b" 20000000 python3 -I "$EX/explain_summary.py" --out "$HERE/out/summary_synth_$b.txt" --uses "$PREP/uses/uses_$b.tsv" \
      "boot1=$O/xr5_synth_${b}_script/explain" "boot2=$O/xr5_synth_${b}_reverse/explain" > "$HERE/out/summary_synth_${b}_run.txt" 2>&1; rc=$?
    echo "rc=$rc" >> "$HERE/out/summary_synth_${b}_run.txt"; cat "$HERE/out/summary_synth_${b}_run.txt"; [ $rc = 0 ] || exit $rc
    python3 -I "$EX/explain_summary.py" --check-summary "$HERE/out/summary_synth_$b.txt" > "$HERE/out/summary_synth_${b}_check.txt" 2>&1; rc=$?
    echo "rc=$rc" >> "$HERE/out/summary_synth_${b}_check.txt"; cat "$HERE/out/summary_synth_${b}_check.txt"
    sha256sum "$EX/explain_summary.py" > "$HERE/out/summary_synth_${b}_script_sha_after.txt"; exit $rc ;;
  namedcalls) T=$HERE/out/named_calls_rerun_after_commit.tsv; fresh "$T"
    (cd "$PREP" && git log -1 --format='%h %cI' -- scripts/named_calls.py && sha256sum scripts/named_calls.py) > "$HERE/out/named_calls_rerun_cmp.txt"
    g named-calls-rerun 20000000 bash -c "cd '$PREP' && python3 -I scripts/named_calls.py --out '$T' --kzdump '$MC/tools/kz-dump/kz-dump' \
      '$O/cz_plans_v4_ns2/c1_pre_tx:$O/cz_final_extract/c1_pre_tx/c1_pre_tx/NP/facts.kz' '$O/cz_plans_v4_ns2/c1_post_tx:$O/cz_final_extract/c1_post_tx/c1_post_tx/NP/facts.kz'" \
      >> "$HERE/out/named_calls_rerun_cmp.txt" 2>&1; rc=$?
    { echo "rc=$rc"; sha256sum "$T" "$PREP/candidates/named_calls.tsv"; cmp "$T" "$PREP/candidates/named_calls.tsv" && echo "byte-identical"; } >> "$HERE/out/named_calls_rerun_cmp.txt" 2>&1
    cat "$HERE/out/named_calls_rerun_cmp.txt"; exit $rc ;;
  expectregen) E=$PREP/expect; X=$SCR/expect_regen; fresh "$X"; mkdir -p "$X" || exit 2
    U=(); for b in c1_pre_tx c1_post_tx; do U+=(--uses "$b:$PREP/uses/uses_$b.tsv:$O/cz_plans_v4_ns2/$b/plan.side:$O/cz_final_order/$b/relations.txt"); done
    g expect-regen 5000000 python3 -I "$PREP/scripts/expect_write2.py" --branches "$PREP/candidates/branches_16.txt" \
      --static "$PREP/scratch/branches16/static_elements_16.tsv" --candidates "$PREP/candidates/candidates.tsv" \
      --predictions "$E/predictions3.tsv" "${U[@]}" --elements-out "$X/elements_by_rule_2.tsv" -o "$X/expect_c1_2.tsv" > "$HERE/out/expect_regen.txt" 2>&1; rc=$?
    { cat "$HERE/out/expect_regen.txt"; echo "rc=$rc"
      sha256sum "$E/predictions2.tsv" "$E/predictions3.tsv" "$X/expect_c1_2.tsv" "$E/expect_c1_2.tsv" "$X/elements_by_rule_2.tsv" "$E/elements_by_rule_2.tsv"
      cmp "$X/expect_c1_2.tsv" "$E/expect_c1_2.tsv" && echo "expect_c1_2.tsv byte-identical"
      cmp "$X/elements_by_rule_2.tsv" "$E/elements_by_rule_2.tsv" && echo "elements_by_rule_2.tsv byte-identical"; } > "$HERE/out/expect_regen_cmp.txt" 2>&1
    cat "$HERE/out/expect_regen_cmp.txt"; rm -rf "$X"; exit $rc ;;
  classify2) b=$a1; ord=$a2; T=$HERE/compare/classify2_synth_${b}_${ord}.tsv; fresh "$T"
    g "classify2-${b}_${ord}" 2000000 python3 -I "$HERE/scripts/classify_compare2.py" --compare "$PRE/compare/synth_${b}_${ord}.txt" \
      --records "$O/xr5_synth_${b}_${ord}/explain" --order "$ord" --uses "$PREP/uses/uses_$b.tsv" --out "$T" ;;
  sizes) T=$HERE/out/xr5_sizes.txt; fresh "$T"
    { echo "# du -sb (apparent bytes) and du -sB1 (block bytes) of the kept outputs of results 26.3 (obs1/xr5_*), $(date -u +%FT%TZ)"
      echo "## du -sb"; (cd "$O" && du -sb xr5_*); echo "## du -sb, total of the twelve"; (cd "$O" && du -sbc xr5_* | tail -1)
      echo "## du -sb, total of the five synthetic outputs (xr5_synth_*)"; (cd "$O" && du -sbc xr5_synth_* | tail -1)
      echo "## du -sB1"; (cd "$O" && du -sB1 xr5_*); echo "## du -sB1, total"; (cd "$O" && du -sB1 -c xr5_* | tail -1); } > "$T"
    cat "$T" ;;
  *) echo "usage: runs.sh fixtures|evalfx|runcycle|unit|regress|static B|rtread N|synth B O|loss B O TAG LINE|eval DIR B NAME|summary B|namedcalls|expectregen|classify2 B O|sizes" >&2; exit 64 ;;
esac
