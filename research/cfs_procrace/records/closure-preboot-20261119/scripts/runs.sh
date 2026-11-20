#!/usr/bin/env bash
# runs.sh STEP [ARG...] -- the runs of results 26.4 (the re-freeze, the expectations from the rules, the new boot
# declaration; no boot, no build).  Every output goes into a new file or directory; every step that writes runs under
# env/tools/space_guard.sh with its log in ../guard/.  No row id, key or expected verdict is an argument of any tool.
#   fixtures             env/obs1/tests/test_explain4.py into <scratch>/fx (../out/fixtures.txt), on the frozen bytes
#   evalfx               env/obs1/tests/test_explain_eval.py into <scratch>/evfx (../out/eval_fixtures.txt)
#   runcycle             boot/test_run_cycle.sh into <scratch>/rct (../out/run_cycle_tests.txt)
#   expect               scripts/expect_write2.py (the expectations from the rules) -> expect/expect_c1_2.tsv and its
#                        element table; then the frozen explain_expect.py on an empty records directory (form check)
#   compare DIR NAME     the frozen explain_expect.py with expect/expect_c1_2.tsv on the explanation output DIR
#                        (a kept output of results 26.3) -> ../compare/NAME.txt
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
R="$(cd "$HERE/../.." && pwd)"
O=/home/user/work/redesign/obs1
EX=$R/env/obs1
VOC=$R/kairos/tests/static/data/reason_vocabulary.txt
PREP=$R/records/closure-c1prep-20261119
G=$R/env/tools/space_guard.sh
step=${1:?usage: runs.sh STEP}; a1=${2:-}; a2=${3:-}
g() { local k=$1 n=$2; shift 2; "$G" --need "$n" --kind "$k" --log "$HERE/guard/$k.log" -- "$@"; }
scr() { echo "${RUNS_SCRATCH:?RUNS_SCRATCH (a scratch directory for fixture outputs) is required}"; }
case "$step" in
  fixtures) S=$(scr) || exit 64; [ ! -e "$HERE/out/fixtures.txt" ] || exit 2
    g fixtures 150000000 python3 -I "$EX/tests/test_explain4.py" "$EX" "$VOC" "$S/fx" > "$HERE/out/fixtures.txt" 2>&1; rc=$?
    tail -1 "$HERE/out/fixtures.txt"; exit $rc ;;
  evalfx) S=$(scr) || exit 64; [ ! -e "$HERE/out/eval_fixtures.txt" ] || exit 2
    g evalfx 20000000 python3 -I "$EX/tests/test_explain_eval.py" "$EX" "$S/evfx" > "$HERE/out/eval_fixtures.txt" 2>&1; rc=$?
    tail -1 "$HERE/out/eval_fixtures.txt"; exit $rc ;;
  runcycle) S=$(scr) || exit 64; [ ! -e "$HERE/out/run_cycle_tests.txt" ] || exit 2
    g runcycle 10000000 bash "$PREP/boot/test_run_cycle.sh" "$S/rct" > "$HERE/out/run_cycle_tests.txt" 2>&1; rc=$?
    tail -1 "$HERE/out/run_cycle_tests.txt"; exit $rc ;;
  expect)
    E=$PREP/expect; [ ! -e "$E/expect_c1_2.tsv" ] || { echo "expect_c1_2.tsv exists" >&2; exit 2; }
    S=$(scr) || exit 64; mkdir -p "$S/empty" || exit 2
    U=(); for b in c1_pre_tx c1_post_tx; do U+=(--uses "$b:$PREP/uses/uses_$b.tsv:$O/cz_plans_v4_ns2/$b/plan.side:$O/cz_final_order/$b/relations.txt"); done
    g expect 5000000 python3 -I "$PREP/scripts/expect_write2.py" --branches "$PREP/candidates/branches_16.txt" \
      --static "$PREP/scratch/branches16/static_elements_16.tsv" --candidates "$PREP/candidates/candidates.tsv" \
      --predictions "$E/predictions2.tsv" "${U[@]}" --elements-out "$E/elements_by_rule_2.tsv" -o "$E/expect_c1_2.tsv" \
      > "$HERE/out/expect_write2.txt" 2>&1; rc=$?
    cat "$HERE/out/expect_write2.txt"; [ $rc = 0 ] || exit $rc
    g expect-form 5000000 python3 -I "$EX/explain_expect.py" --expect "$E/expect_c1_2.tsv" --records "$S/empty" \
      --out "$E/form_check2_no_records.txt" > "$HERE/out/expect_form.txt" 2>&1; rc=$?
    cat "$HERE/out/expect_form.txt"; exit $rc ;;
  compare)
    D=${a1:?DIR}; N=${a2:?NAME}; mkdir -p "$HERE/compare" || exit 2; [ ! -e "$HERE/compare/$N.txt" ] || exit 2
    g "compare-$N" 5000000 python3 -I "$EX/explain_expect.py" --expect "$PREP/expect/expect_c1_2.tsv" --records "$D" \
      --out "$HERE/compare/$N.txt" > "$HERE/out/compare_$N.log" 2>&1; rc=$?
    cat "$HERE/out/compare_$N.log"; exit $rc ;;
  *) echo "usage: runs.sh fixtures|evalfx|runcycle|expect|compare" >&2; exit 64 ;;
esac
