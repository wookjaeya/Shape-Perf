#!/usr/bin/env bash
# run_cycle.sh STEP BUILD N -- the boots of one automatic C1 cycle, as declared in results 25.12 (written and committed
# by the cycle preparation step; NOT RUN by it).  Revised before any boot (results 26.0 (e), (h); spec 11 items 14-15):
# the verify step first, the eval step after explain, the named-calls input of explain, the uses list of summary.  BUILD c1_pre_tx | c1_post_tx; N is the boot's number: 1, then 2-5 only
# after boot 1 (or its one retry 1r) passed the gate, or 1r only after boot 1 failed it and the cause is committed in
# <build>/addendum_1r.txt.  Every output directory is new; nothing is replaced.  Logs and small records go into
# <build>/boot<N>/ beside this script; large outputs into /home/user/work/redesign/obs1/c1cyc_<s>_<what><N>.
#   verify    re-hash every sha256 row and re-read every build-id row of boot/declaration.tsv (readelf -n, as when the
#             declaration was written; a plan-id row against its plan.side) into <build>/boot<N>/verify.txt (kz-verify 1);
#             rc 0 when no row differs and none is missing, 1 otherwise, 64 on usage
#   boot      the natural boot (run_gi_c1.sh, tx records, rx off) of the derived build with kzgcc 0.5; only after a clean
#             verify.txt of the same N whose head names the sha256 of the declaration as it is now (else rc 1, no boot)
#   control   control record check of the dump text (kz-rt-dump check line), record kinds, losses
#   convert   MC kz convert of the dump with the plan and the binding file, the B' basis
#   kzt-text  kz-dump text of run.kzt and the facts header
#   chain     chain_check.py (manifest, executable, facts_build; same_build_object per object)
#   stop      stop_eval.py (the stop events of the plan in the startup script and the dump)
#   gate      the gate of evaluation protocol 3 from the three files above: PASS or FAIL with the failing item
#   explain   the frozen explanation script revision 4 on the boot with the 16 branches, the candidate list and the
#             named-calls file
#   eval      explain_eval.py (F1-F10, s1-s4, expect_discriminate, precision, in-plan recall, the undetermined split) into
#             <build>/boot<N>/eval.txt; it reads the expectation file for F1 only and writes no record
#   expect    explain_expect.py: the filled expectations against the boot's records (never an input of the steps above)
#   summary   explain_summary.py over every boot of the build that passed the gate, with the list of pre-defined uses
#             (N is ignored)
# Test seams (the verify and boot tests of spec 11 item 14 only; a run that uses one says so in its log):
# RUN_CYCLE_OBS replaces the work directory /home/user/work/redesign/obs1; RUN_CYCLE_BOOT_CMD replaces the boot command.
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
D="$(cd "$HERE/.." && pwd)"
R="$(cd "$D/../.." && pwd)"
GS=$R/kairos/instrument/gcc/scripts
GUARD=$R/env/tools/space_guard.sh
EX=$R/env/obs1
SEM=$R/kairos/semantics
O=${RUN_CYCLE_OBS:-/home/user/work/redesign/obs1}
DECL=$HERE/declaration.tsv
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
resolve() { # a declaration path: absolute; research/... from the repository root; records/... from the research root; else the work directory
  case "$1" in /*) echo "$1" ;; research/*) echo "$R/../../$1" ;; records/*) echo "$R/$1" ;; *) echo "$O/$1" ;; esac
}
verify_clean() { # verify.txt of this N is clean and names the declaration as it is now
  local v=$L/verify.txt
  [ -f "$v" ] || return 1
  grep -qE '^rows [0-9]+ ok [0-9]+ differs 0 missing 0$' "$v" || return 1
  [ "$(sed -n 's/^declaration sha256=\([0-9a-f]*\) .*/\1/p' "$v")" = "$(sha256sum < "$DECL" | cut -c1-64)" ]
}
case "$STEP" in
  verify)
    [ -f "$DECL" ] || { echo "run_cycle.sh: no declaration $DECL" >&2; exit 64; }
    mkdir -p "$L" || exit 2
    [ ! -e "$L/verify.txt" ] || mv "$L/verify.txt" "$L/verify.$(date -u +%Y%m%dT%H%M%SZ).txt"
    { echo "kz-verify 1"
      echo "declaration sha256=$(sha256sum < "$DECL" | cut -c1-64) path=$DECL"
      echo "measured $(date -u +%FT%TZ)"
      [ -z "${RUN_CYCLE_OBS:-}" ] || echo "# RUN_CYCLE_OBS=$RUN_CYCLE_OBS (test seam)"
      n=0; a=0; b=0; c=0; ln=0
      while IFS=$'\t' read -r kind bld name val path; do
        ln=$((ln + 1))
        case "$kind" in ''|'#'*) continue ;; esac
        n=$((n + 1))
        if [ "$kind" = plan-id ]; then
          sp=$(awk -F'\t' -v b="$bld" '$1=="plan" && $2==b && $3=="plan.side" {print $5}' "$DECL")
          f=$(resolve "$sp"); got=$( [ -f "$f" ] && sed -n 's/^plan-id //p' "$f" | head -1 )
          [ -f "$f" ] || got=missing
        else
          f=$(resolve "$path")
          if [ ! -f "$f" ]; then got=missing
          else
            got=$(sha256sum < "$f" | cut -c1-64)
            case "$val" in *" build-id "*) got="$got build-id $(readelf -n "$f" 2> /dev/null | sed -n 's/.*Build ID: //p' | head -1)" ;; esac
          fi
        fi
        if [ "$got" = missing ]; then st=missing; c=$((c + 1)); elif [ "$got" = "$val" ]; then st=ok; a=$((a + 1)); else st=differs; b=$((b + 1)); fi
        printf '%d\t%s\t%s\t%s\t%s\t%s\n' "$ln" "$kind" "${path}" "$val" "$got" "$st"
      done < "$DECL"
      echo "rows $n ok $a differs $b missing $c"
    } > "$L/verify.txt"
    tail -1 "$L/verify.txt"
    grep -qE '^rows [0-9]+ ok [0-9]+ differs 0 missing 0$' "$L/verify.txt" ;;
  boot)
    allowed || { echo "run_cycle.sh: boot $N of $B is not allowed by the boot-number rule (results 25.12)" >&2; exit 2; }
    verify_clean || { echo "run_cycle.sh: no clean verify.txt of boot $N naming the declaration as it is now: not booting (results 26.0 (h))" >&2; exit 1; }
    [ ! -e "$RUND" ] || { echo "run_cycle.sh: $RUND exists; one boot per number" >&2; exit 2; }
    mkdir -p "$L" && mkdir "$RUND" || exit 2
    if [ -n "${RUN_CYCLE_BOOT_CMD:-}" ]; then
      echo "# RUN_CYCLE_BOOT_CMD (test seam): $RUN_CYCLE_BOOT_CMD" > "$L/boot.txt"
      bash -c "$RUN_CYCLE_BOOT_CMD" >> "$L/boot.txt" 2>&1; rc=$?
    else
      g boot 60000000 bash "$GS/run_gi_c1.sh" "$W" "$CFG" 1df262a tx "$RUN" "$RT/kz-rt-dump" "$P/plan.kzs" > "$L/boot.txt" 2>&1; rc=$?
    fi
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
      --branches "$D/candidates/branches_16.txt" --candidates "$D/candidates/candidates.tsv" --named-calls "$D/candidates/named_calls.tsv" \
      --label-record "C1 cycle boot $N of $B (results 25.12)" \
      --vocabulary "$R/kairos/tests/static/data/reason_vocabulary.txt" --out "$XO" > "$L/explain.txt" 2>&1; rc=$?
    echo "rc=$rc" >> "$L/explain.txt"; tail -20 "$L/explain.txt"; exit $rc ;;
  eval)
    [ ! -e "$L/eval.txt" ] || { echo "run_cycle.sh: $L/eval.txt exists" >&2; exit 2; }
    SB=none; [ -f "$L/sheet_boot.tsv" ] && SB=$L/sheet_boot.tsv
    g eval 20000000 python3 -I "$EX/explain_eval.py" --explain "$XO" --build "$B" --boot "boot$N" --gate "$L/gate.txt" --pairs "$ORD/pairs.tsv" \
      --sidecar "$P/plan.side" --relations "$ORD/relations.txt" --uses "$D/uses/uses_$B.tsv" --sheets-static "$D/sheets/sheets_static.tsv" \
      --sheets-boot "$SB" --expect "$D/expect/expect_c1.tsv" --out "$L/eval.txt" > "$L/eval_run.txt" 2>&1; rc=$?
    echo "rc=$rc" >> "$L/eval_run.txt"; tail -6 "$L/eval_run.txt"; exit $rc ;;
  expect)
    python3 -I "$EX/explain_expect.py" --expect "$D/expect/expect_c1.tsv" --records "$XO" --out "$L/expect_compare.txt"; rc=$?
    echo "rc=$rc"; tail -1 "$L/expect_compare.txt"; exit $rc ;;
  summary)
    A=(); for d in "$HERE/$B"/boot*; do n=${d##*/boot}; [ "$(gate_of "$n")" = PASS ] && A+=("boot$n=$O/c1cyc_${S}_explain$n"); done
    [ ${#A[@]} -gt 0 ] || { echo "run_cycle.sh: no boot of $B passed the gate" >&2; exit 2; }
    python3 -I "$EX/explain_summary.py" --out "$HERE/$B/summary.txt" --uses "$D/uses/uses_$B.tsv" "${A[@]}" ;;
  *) echo "usage: run_cycle.sh verify|boot|control|convert|kzt-text|chain|stop|gate|explain|eval|expect|summary BUILD N" >&2; exit 64 ;;
esac
