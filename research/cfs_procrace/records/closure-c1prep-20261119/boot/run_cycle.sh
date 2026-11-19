#!/usr/bin/env bash
# run_cycle.sh STEP BUILD N -- the boots of one automatic C1 cycle, as declared in results 25.12 (written and committed
# by the cycle preparation step; NOT RUN by it).  BUILD c1_pre_tx | c1_post_tx; N is the boot's number: 1, then 2-5 only
# after boot 1 (or its one retry 1r) passed the gate, or 1r only after boot 1 failed it and the cause is committed in
# <build>/addendum_1r.txt.  Every output directory is new; nothing is replaced.  Logs and small records go into
# <build>/boot<N>/ beside this script; large outputs into /home/user/work/redesign/obs1/c1cyc_<s>_<what><N>.
#   boot      the natural boot (run_gi_c1.sh, tx records, rx off) of the derived build with kzgcc 0.5
#   control   control record check of the dump text (kz-rt-dump check line), record kinds, losses
#   convert   MC kz convert of the dump with the plan and the binding file, the B' basis
#   kzt-text  kz-dump text of run.kzt and the facts header
#   chain     chain_check.py (manifest, executable, facts_build; same_build_object per object)
#   stop      stop_eval.py (the stop events of the plan in the startup script and the dump)
#   gate      the gate of evaluation protocol 3 from the three files above: PASS or FAIL with the failing item
#   explain   the frozen explanation script revision 4 on the boot with the 16 branches and the candidate list
#   expect    explain_expect.py: the filled expectations against the boot's records (never an input of the steps above)
#   summary   explain_summary.py over every boot of the build that passed the gate (N is ignored)
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
D="$(cd "$HERE/.." && pwd)"
R="$(cd "$D/../.." && pwd)"
GS=$R/kairos/instrument/gcc/scripts
GUARD=$R/env/tools/space_guard.sh
EX=$R/env/obs1
SEM=$R/kairos/semantics
O=/home/user/work/redesign/obs1
MC=/home/user/work/merge-check/kz-llvm
STEP=${1:?usage: run_cycle.sh STEP BUILD N}; B=${2:?build}; N=${3:-}
case "$B" in c1_pre_tx) S=pre; CFG=pre ;; c1_post_tx) S=post; CFG=post ;; *) echo "run_cycle.sh: BUILD is c1_pre_tx or c1_post_tx" >&2; exit 64 ;; esac
P=$O/cz_plans_v4_ns2/$B
W=$O/c1p2_${S}_derived
RT=$O/c1p2_${S}_rt/rt
BP=$O/c1p2_${S}_bprime/$B/NP
PF=$O/cz_final_extract/$B/$B/NP
ORD=$O/cz_final_order/$B
L=$HERE/$B/boot$N
RUND=$O/c1cyc_${S}_boot$N
RUN=$RUND/tx
CV=$O/c1cyc_${S}_convert$N
XO=$O/c1cyc_${S}_explain$N
g() { local k=$1 n=$2; shift 2; "$GUARD" --need "$n" --kind "$k" --log "$L/guard_$k.log" -- "$@"; }
gate_of() { sed -n 's/^gate \([A-Z]*\).*/\1/p' "$HERE/$B/boot$1/gate.txt" 2> /dev/null; }
allowed() {
  case "$N" in
    1) return 0 ;;
    1r) [ "$(gate_of 1)" = FAIL ] && git -C "$R" ls-files --error-unmatch "$HERE/$B/addendum_1r.txt" > /dev/null 2>&1 \
          && [ -z "$(git -C "$R" status --porcelain -- "$HERE/$B/addendum_1r.txt")" ] ;;
    2|3|4|5) [ "$(gate_of 1)" = PASS ] || [ "$(gate_of 1r)" = PASS ] ;;
    *) return 1 ;;
  esac
}
[ "$STEP" = summary ] || [ -n "$N" ] || { echo "run_cycle.sh: N is required" >&2; exit 64; }
case "$STEP" in
  boot)
    allowed || { echo "run_cycle.sh: boot $N of $B is not allowed by the boot-number rule (results 25.12)" >&2; exit 2; }
    [ ! -e "$RUND" ] || { echo "run_cycle.sh: $RUND exists; one boot per number" >&2; exit 2; }
    mkdir -p "$L" && mkdir "$RUND" || exit 2
    g boot 60000000 bash "$GS/run_gi_c1.sh" "$W" "$CFG" 1df262a tx "$RUN" "$RT/kz-rt-dump" "$P/plan.kzs" > "$L/boot.txt" 2>&1; rc=$?
    echo "rc=$rc" >> "$L/boot.txt"; tail -5 "$L/boot.txt"; exit $rc ;;
  control)
    T=$RUN/tx.dump.txt
    { echo "dump text sha256 $(sha256sum < "$T" | cut -c1-64) lines $(wc -l < "$T")"
      echo "record kinds (first word, count):"; awk '{print $1}' "$T" | sort | uniq -c | sed 's/^/  /'
      grep -E '^run abi|^runx |^caps|^buildid ' "$T" | sed 's/^/  /'
      echo "loss fields not 0 (kind=value, count): $(grep -oE '\b(overflow|partial|dropped_bytes|pool_dropped|boot_bytes_dropped)=[1-9][0-9]*' "$T" | sort | uniq -c | tr -s ' ' | tr '\n' ';')"
      echo "pool overflow fields not 0: $(grep -oE '\b(overflow|pool_dropped)=[1-9][0-9]*' "$T" | wc -l)"
      echo "kz-rt-dump check summary: $(grep -E '^check ' "$T" | tail -1)"
    } > "$L/control_check.txt"; cat "$L/control_check.txt" ;;
  convert)
    [ ! -e "$CV" ] || { echo "run_cycle.sh: $CV exists" >&2; exit 2; }
    mkdir "$CV" || exit 2
    g convert 20000000 "$MC/tools/kz/kz" convert --kzrt "$RUN/dump/tx.kzrt" --manifest "$P/plan.kzs" --manifest "$W/binds_instr.kzs" \
      --basis "$BP/basis.kz" --out "$CV/run.kzt" > "$CV/convert.log" 2>&1; rc=$?
    echo "convert rc=$rc" >> "$CV/convert.log"; cp "$CV/convert.log" "$L/convert.log"; cat "$L/convert.log"; exit $rc ;;
  kzt-text)
    "$MC/tools/kz-dump/kz-dump" "$CV/run.kzt" > "$CV/run.kzt.txt" 2>&1; echo "kz-dump run.kzt rc $?"
    "$MC/tools/kz-dump/kz-dump" "$BP/facts.kz" 2>&1 | head -8 > "$CV/facts_header.txt"
    (cd "$CV" && sha256sum run.kzt run.kzt.txt facts_header.txt convert.log) > "$L/convert.sha256"; cat "$L/convert.sha256" ;;
  chain)
    IL=$(sed -n 's/^instance_files .* list=\([^ ]*\) .*/\1/p' "$RUN/run.meta")
    python3 -I "$R/env/tools/chain_check.py" --plan "$P/plan.kzs" --build-record "$W/build_instr.record" --run-meta "$RUN/run.meta" \
      --build-root "$W/ws" --dump-text "$RUN/tx.dump.txt" --basis-text "$BP/basis.txt" --main exe/tx/core-tx \
      --sidecar "$P/plan.side" --plan-facts "$PF/facts.kz" --plan-basis "$PF/basis.kz" --inst-list "$IL" --inst-tree "$W/ws/exe" \
      --dump "$RUN/dump/tx.kzrt" --bindings "$W/binds_instr.kzs" --bprime-basis "$BP/basis.kz" --facts-header "$CV/facts_header.txt" \
      --kzt-text "$CV/run.kzt.txt" -o "$L/chain.txt"; rc=$?; echo "chain_check rc=$rc"; tail -12 "$L/chain.txt"; exit 0 ;;
  stop)
    python3 -I "$EX/stop_eval.py" --manifest "$P/plan.kzs" --sidecar "$P/plan.side" \
      --startup-script "$RUN/inst/tx/cf/cfe_es_startup.scr" --dump-text "$RUN/tx.dump.txt" -o "$L/stop_eval.tsv" ;;
  gate)
    # evaluation protocol 3: (1) the control record check is clean, (2) losses 0 or named, a pool overflow fails,
    # (3) the chain is confirmed.  same_build is not in the gate (it is basis 1 of every pair).
    # a loss other than a pool overflow passes only named: its kinds and counts are this file's second line, and the
    # explanation records mark the needs it touches unknown-loss (the judgement part's NEED_REASON)
    c1=$(grep -c '^kz-rt-dump check summary: check ok' "$L/control_check.txt")
    c2=$(grep -c '^pool overflow fields not 0: 0$' "$L/control_check.txt")
    c3=$(grep -cP '^chain\tconfirmed\t' "$L/chain.txt")
    lo=$(sed -n 's/^loss fields not 0 (kind=value, count): //p' "$L/control_check.txt")
    { if [ "$c1" = 1 ] && [ "$c2" = 1 ] && [ "$c3" = 1 ]; then echo "gate PASS"; else
        echo "gate FAIL control_ok=$c1 no_pool_overflow=$c2 chain_confirmed=$c3"; fi
      echo "losses: ${lo:-none}"
      echo "inputs: control_check.txt $(sha256sum < "$L/control_check.txt" | cut -c1-16) chain.txt $(sha256sum < "$L/chain.txt" | cut -c1-16)"
    } > "$L/gate.txt"; cat "$L/gate.txt" ;;
  explain)
    [ ! -e "$XO" ] || { echo "run_cycle.sh: $XO exists" >&2; exit 2; }
    IL=$(sed -n 's/^instance_files .* list=\([^ ]*\) .*/\1/p' "$RUN/run.meta")
    NS=(); for f in framework_namespaces2_cfe-6.4.1.txt framework_namespaces2_cfe-6.4.2.txt framework_namespaces2_osal-4.1.1.txt \
                    framework_namespaces2_osal-7.0.1.txt framework_namespaces2_osal-f34759d.txt; do NS+=(--namespaces "$SEM/$f"); done
    g explain 60000000 python3 -I "$EX/explain.py" --sidecar "$P/plan.side" --manifest "$P/plan.kzs" --bindings "$W/binds_instr.kzs" \
      --relations "$ORD/relations.txt" --keys "$ORD/keys.txt" --pairs "$ORD/pairs.tsv" "${NS[@]}" \
      --facts-view "$O/xr4_dry_inputs/$B/facts_view.txt" --bprime-basis "$BP/basis.kz" --build-label "$B" \
      --dump-text "$RUN/tx.dump.txt" --run-meta "$RUN/run.meta" --chain "$L/chain.txt" --stop-eval "$L/stop_eval.tsv" \
      --startup-script "$RUN/inst/tx/cf/cfe_es_startup.scr" --instance-list "$IL" \
      --branches "$D/candidates/branches_16.txt" --candidates "$D/candidates/candidates.tsv" \
      --label-record "C1 cycle boot $N of $B (results 25.12)" \
      --vocabulary "$R/kairos/tests/static/data/reason_vocabulary.txt" --out "$XO" > "$L/explain.txt" 2>&1; rc=$?
    echo "rc=$rc" >> "$L/explain.txt"; tail -20 "$L/explain.txt"; exit $rc ;;
  expect)
    python3 -I "$EX/explain_expect.py" --expect "$D/expect/expect_c1.tsv" --records "$XO" --out "$L/expect_compare.txt"; rc=$?
    echo "rc=$rc"; tail -1 "$L/expect_compare.txt"; exit $rc ;;
  summary)
    A=(); for d in "$HERE/$B"/boot*; do n=${d##*/boot}; [ "$(gate_of "$n")" = PASS ] && A+=("boot$n=$O/c1cyc_${S}_explain$n"); done
    [ ${#A[@]} -gt 0 ] || { echo "run_cycle.sh: no boot of $B passed the gate" >&2; exit 2; }
    python3 -I "$EX/explain_summary.py" --out "$HERE/$B/summary.txt" "${A[@]}" ;;
  *) echo "usage: run_cycle.sh boot|control|convert|kzt-text|chain|stop|gate|explain|expect|summary BUILD N" >&2; exit 64 ;;
esac
