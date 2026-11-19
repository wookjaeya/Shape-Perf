#!/usr/bin/env bash
# regress.sh REV OUTDIR -- the explanation script of revision REV (3: env/obs1/explain_rev3.py or the committed
# revision-3 explain.py given as $3; 4: env/obs1/explain.py) on the inputs of the 2026-11-16 boot (the inputs of
# records/obs1-20261116-reviewfix/inputs.txt), into a new directory.  A record seen before the rules.
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
R="$(cd "$HERE/../.." && pwd)"
O=/home/user/work/redesign/obs1
P=$O/plangen/c1_pre_tx; W=$O/c1pre_derived; BP=$O/c1pre_bprime/c1_pre_tx/NP; RUN=$O/c1pre_boot/tx
REL=$O/plangen_relcheck/c1_pre_tx; EX0=$O/c1pre_explain; BOOTREC=$R/records/obs1-20261116-boot
FIX=$R/records/obs1-20261116-reviewfix
REV=$1; OUT=$2; SCRIPT=${3:-}
[ ! -e "$OUT" ] || { echo "regress.sh: $OUT exists" >&2; exit 2; }
COMMON=(--sidecar "$P/plan.side" --manifest "$P/plan.kzs" --bindings "$W/binds_instr.kzs"
  --relations "$REL/relations.txt" --keys "$REL/keys.txt" --vocabulary "$R/kairos/tests/static/data/reason_vocabulary.txt"
  --facts-view "$EX0/facts_view.txt" --bprime-basis "$BP/basis.kz" --build-label c1_pre_tx
  --dump-text "$RUN/tx.dump.txt" --run-meta "$RUN/run.meta" --chain "$BOOTREC/chain.txt" --stop-eval "$BOOTREC/stop_eval.tsv"
  --startup-script "$RUN/inst/tx/cf/cfe_es_startup.scr" --instance-list "$RUN/inst_files.sha256" --boot-budget "$FIX/boot_budget.tsv")
case "$REV" in
  3) python3 -I "$SCRIPT" "${COMMON[@]}" --out "$OUT" ;;
  4) python3 -I "$R/env/obs1/explain.py" "${COMMON[@]}" --branches "$4" --label-record "seen before the rules" --out "$OUT" ;;
  *) echo "regress.sh: REV 3 or 4" >&2; exit 2 ;;
esac
