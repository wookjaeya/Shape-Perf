#!/usr/bin/env bash
# no_expect_check4.sh OUT -- spec 11 item 15 again for the run script of results 26.9: the expectation files and the hand
# predictions are no input of the generator, the runner, the boot, the conversion, the chain check, the stop evaluation
# or the explanation; the two named exceptions are the expect step (explain_expect.py) and the eval step, whose
# explain_eval.py opens the expectation file for F1 only (its open-watch fixture in env/obs1/tests/test_explain_eval.py).
# A copy of records/closure-prebootfix-20261119/scripts/no_expect_check3.sh with its record directory and this line
# changed (the keep step names no expectation file either; it is listed with the other steps).  Reads only.
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; R="$(cd "$HERE/../.." && pwd)"
B=$R/records/closure-c1prep-20261119/boot; O=/home/user/work/redesign/obs1
OUT=${1:?OUT}; [ ! -e "$OUT" ] || exit 2
{ echo "check (results 26.9; spec 11 item 15): the expectation files (expect_c1.tsv, expect_c1_2.tsv) and the hand predictions (predictions.tsv, predictions2.tsv, predictions3.tsv) are no input of any step but the two named exceptions"
  echo "run_cycle.sh sha256 $(sha256sum < "$B/run_cycle.sh" | cut -c1-64)"
  echo "lines of run_cycle.sh that name an expectation file or the predictions:"
  grep -nE 'expect_c1|predictions' "$B/run_cycle.sh" | sed 's/^/  /'
  echo "steps whose command names one (by case label):"
  awk '/^  [a-z-]+\)/{lab=$1} /expect_c1|predictions/ && !/^#/ && lab!="" {print "  " lab}' "$B/run_cycle.sh" | sort -u
  echo "the explain step's explain.py arguments naming an expect or prediction file: $(sed -n '/^  explain)/,/^  eval)/p' "$B/run_cycle.sh" | grep -cE 'expect_c1|predictions')"
  echo "plan generation (args.txt of the two plans) naming one: $(for b in c1_pre_tx c1_post_tx; do printf '%s:%s ' "$O/cz_plans_v4_ns2/$b/args.txt" "$(grep -cE 'expect|prediction' "$O/cz_plans_v4_ns2/$b/args.txt")"; done)"
  echo "explain_eval.py lines that open the expectation file (the F1 branch):"
  grep -nE 'open\(.*expect|a\.expect|o\.expect' "$R/env/obs1/explain_eval.py" | sed 's/^/  /'
  echo "the eval fixtures' open and write watches: $(grep -cE 'F1 only|outside F1|opens the expectation|writes outside' "$R/env/obs1/tests/test_explain_eval.py") lines naming them in test_explain_eval.py (116/116 pass on the frozen bytes: records/closure-prebootfix-20261119/out/freeze_eval_fixtures.txt)"
} > "$OUT"
cat "$OUT"
