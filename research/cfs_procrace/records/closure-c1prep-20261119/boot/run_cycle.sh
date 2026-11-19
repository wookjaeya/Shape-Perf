#!/usr/bin/env bash
# run_cycle.sh STEP BUILD N -- the boots of one automatic C1 cycle, as declared in results 25.12 (written and committed
# by the cycle preparation step; NOT RUN by it).  Revised before any boot (results 26.0 (e), (h); spec 11 items 14-15):
# the verify step first, the eval step after explain, the named-calls input of explain, the uses list of summary.  Revised again
# before any boot (results 26.4): the declaration it verifies is boot/declaration2.tsv (it supersedes declaration.tsv of
# 860c2cd, kept unchanged), and the eval and expect steps read expect/expect_c1_2.tsv (the expectations rewritten from
# the rules; expect_c1.tsv is kept unchanged and read by no step).  Revised again before any boot (results 26.5; evaluation
# protocol 14.13 items 8-9; spec 11.5 item 4): boot/declaration3.tsv with count rows; the boot step checks that the
# declaration and this script are committed and runs the verify step again right before the boot command; control and
# gate name every loss record of the dump by kind and count.  Revised again before any boot (results 26.9; evaluation
# protocol 14.14; spec 11.6; review records/closure-review-bootfix-20261119): the declaration it verifies is
# boot/declaration4.tsv (it supersedes declaration3.tsv, kept unchanged), whose count rows give the number of entries
# that are not directories and whose need rows give the floor, the margin and the per-boot value of the space need; the
# eval output goes under the work directory and only its head and per-branch tail into the record directory; a keep step
# writes the sha256 lists and ledger rows of each boot's kept outputs; control, kzt-text, chain, stop and gate write once;
# a boot step that never created the instance is not booted; the boot step measures the space need of the boots
# remaining; the declaration is pinned at the cycle's first boot; explain and summary verify again.
# BUILD c1_pre_tx | c1_post_tx; N is the boot's number: 1, then 2-5 only after boot 1 (or its one retry 1r) passed the
# gate, or 1r only after boot 1 failed it and the cause is committed in <build>/addendum_1r.txt.  Every output directory
# is new; nothing is replaced: a step whose output exists refuses (rc 2), and a record a step writes again on a lawful
# re-run is first kept under a new name (UTC nanoseconds).  Logs and small records go into <build>/boot<N>/ beside this
# script; large outputs into /home/user/work/redesign/obs1/c1cyc_<s>_<what><N>.
#   verify    re-hash every sha256 row and re-read every build-id row of boot/declaration4.tsv (readelf -n, as when the
#             declaration was written; a plan-id row against its plan.side; a count row: the entries under the directory
#             that are not directories, find ! -type d; need rows are values, not measured: the declaration's own sha256
#             holds them) into <build>/boot<N>/verify.txt (kz-verify 1); rc 0 when no row differs and none is missing, 1
#             otherwise, 64 on usage
#   boot      the natural boot (run_gi_c1.sh, tx records, rx off) of the derived build with kzgcc 0.5; only after a clean
#             verify.txt of the same N whose head names the sha256 of the declaration as it is now, with the declaration
#             and this script in git and not changed, with the declaration's sha256 equal to boot/declaration_pin.txt
#             (written by the cycle's first boot, c1_pre_tx 1; no other boot runs without it), after a verify of the same N
#             run again by this step is clean, and with the free space at or above the need of the boots remaining
#             (boot<N>/space_at_boot.txt; else rc 1, no boot).  A boot command that ends non-zero before the instance
#             directory <work>/c1cyc_<s>_boot<N>/tx exists is "not booted" (protocol 14.14 item 3): the empty directory is
#             removed, boot.txt is kept as boot.notbooted.<UTC ns>.txt, and the same N may run again
#   control   control record check of the dump text (kz-rt-dump check line), record kinds, loss fields, loss records by
#             kind and count (once per N)
#   convert   MC kz convert of the dump with the plan and the binding file, the B' basis (a refusal at start by the space
#             guard leaves no output directory)
#   kzt-text  kz-dump text of run.kzt and the facts header (once per N)
#   chain     chain_check.py (manifest, executable, facts_build; same_build_object per object) (once per N)
#   stop      stop_eval.py (the stop events of the plan in the startup script and the dump) (once per N)
#   gate      the gate of evaluation protocol 3 from the three files above: PASS or FAIL with the failing item (once per
#             N: gate.txt is the input of the boot-number rule)
#   explain   the frozen explanation script revision 4 on the boot with the 16 branches, the candidate list and the
#             named-calls file; after a verify of the same N run again by this step is clean (verify_at_explain.txt)
#   eval      explain_eval.py (F1-F10, s1-s4, expect_discriminate, precision, in-plan recall, the undetermined split) into
#             <work>/c1cyc_<s>_eval<N>/eval.txt, and its head and per-branch tail into <build>/boot<N>/eval_tail.txt (lead
#             ruling 26.7 (다)); --sheets-boot none (no reader in this cycle; a boot<N>/sheet_boot.tsv makes it refuse); it
#             reads the expectation file for F1 only and writes no record
#   expect    explain_expect.py: the filled expectations against the boot's records (never an input of the steps above)
#   keep      the sha256 list of each kept output directory of the boot (boot, convert, explain, eval) into
#             <build>/boot<N>/c1cyc_<s>_<what><N>.sha256 and one ledger row each, added to records/LEDGER.tsv only after
#             ledger_check.sh passed on the proposed ledger (once per N)
#   summary   explain_summary.py over every boot of the build that passed the gate, with the list of pre-defined uses
#             (N is ignored); only after boot5/gate.txt exists and a verify with N = 5 run again by this step is clean
# Test seams (the tests of spec 11 item 14, 11.5 item 4 and 11.6 only; a run that uses one says so in its log):
# RUN_CYCLE_OBS replaces the work directory /home/user/work/redesign/obs1; RUN_CYCLE_BOOT_CMD replaces the boot command;
# RUN_CYCLE_FREE_CMD replaces df -B1 / in the boot step's space check.
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
D="$(cd "$HERE/.." && pwd)"
R="$(cd "$D/../.." && pwd)"
GS=$R/kairos/instrument/gcc/scripts
GUARD=$R/env/tools/space_guard.sh
EX=$R/env/obs1
SEM=$R/kairos/semantics
O=${RUN_CYCLE_OBS:-/home/user/work/redesign/obs1}
DECL=$HERE/declaration4.tsv
PIN=$HERE/declaration_pin.txt
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
EV=$O/c1cyc_${S}_eval$N
g() { local k=$1 n=$2; shift 2; "$GUARD" --need "$n" --kind "$k" --log "$L/guard_$k.log" -- "$@"; }
ts() { date -u +%Y%m%dT%H%M%S.%NZ; }
keep_old() { # FILE [INFIX]: an existing FILE moves to <base>.[INFIX.]<UTC ns>.<ext> (results 26.5, 26.9); rc 0 unless the move fails
  [ -e "$1" ] || return 0
  local base=${1%.*} ext=${1##*.} k
  k=$base.${2:+$2.}$(ts).$ext; [ ! -e "$k" ] || k=$k.$$
  mv "$1" "$k"
}
refuse_exists() { echo "run_cycle.sh: $1 exists; this step writes once per boot number (results 26.9, protocol 14.14 item 2)" >&2; exit 2; }
decl_sha() { sha256sum < "$DECL" | cut -c1-64; }
pin_ok() { [ -f "$PIN" ] && [ "$(sed -n 's/^declaration sha256=\([0-9a-f]*\) .*/\1/p' "$PIN")" = "$(decl_sha)" ]; }
need_pin() { pin_ok || { echo "run_cycle.sh: the declaration's sha256 is not the one boot/declaration_pin.txt names (or no pin): not running $STEP (results 26.9, protocol 14.14 item 5)" >&2; exit 1; }; }
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
verify_clean() { # DIR: DIR/verify.txt is clean and names the declaration as it is now
  local v=$1/verify.txt
  [ -f "$v" ] || return 1
  grep -qE '^rows [0-9]+ ok [0-9]+ differs 0 missing 0$' "$v" || return 1
  [ "$(sed -n 's/^declaration sha256=\([0-9a-f]*\) .*/\1/p' "$v")" = "$(decl_sha)" ]
}
reverify() { # OUT NN: run the verify step of NN again, its stdout and stderr into OUT (an earlier OUT kept); rc 0 when clean
  keep_old "$1" || return 1
  bash "$HERE/run_cycle.sh" verify "$B" "$2" > "$1" 2>&1 && verify_clean "$HERE/$B/boot$2"
}
need_val() { awk -F'\t' -v n="$1" '$1=="need" && $3==n {print $4}' "$DECL" | head -1; }
free_now() { if [ -n "${RUN_CYCLE_FREE_CMD:-}" ]; then bash -c "$RUN_CYCLE_FREE_CMD"; else df -B1 --output=avail / | tail -1 | tr -d ' '; fi; }
measured_max() { # the largest bytes one boot already run left (its four work directories and its record directory): "BYTES WHERE"
  local mx=0 w=none d s n t x
  for d in "$HERE"/c1_pre_tx/boot* "$HERE"/c1_post_tx/boot*; do
    [ -f "$d/boot.txt" ] || continue
    case "$d" in */c1_pre_tx/*) s=pre ;; *) s=post ;; esac
    n=${d##*/boot}; t=$(du -sB1 "$d" | cut -f1)
    for x in boot convert explain eval; do [ -d "$O/c1cyc_${s}_$x$n" ] && t=$((t + $(du -sB1 "$O/c1cyc_${s}_$x$n" | cut -f1))); done
    if [ "$t" -gt "$mx" ]; then mx=$t; w=${d#$HERE/}; fi
  done
  echo "$mx $w"
}
space_check() { # protocol 14.14 item 4: the free space against the need of the boots remaining; writes $L/space_at_boot.txt
  local fl mg pb own rem mx mw used need f out=$L/space_at_boot.txt
  fl=$(need_val floor); mg=$(need_val margin); pb=$(need_val "per boot")
  case "$fl$mg$pb" in *[!0-9]*|'') echo "run_cycle.sh: the need rows of the declaration are not three byte counts" >&2; return 1 ;; esac
  case "$N" in 1) own=6 ;; 1r) own=5 ;; 2|3|4|5) own=$((6 - N)) ;; *) return 1 ;; esac
  rem=$own; [ "$B" = c1_pre_tx ] && rem=$((own + 6))
  read -r mx mw < <(measured_max)
  used=$pb; [ "$mx" -gt "$pb" ] && used=$mx
  need=$((fl + mg + rem * used))
  f=$(free_now); case "$f" in *[!0-9]*|'') f=0 ;; esac
  keep_old "$out" || return 1
  { echo "kz-space-at-boot 1 (run_cycle.sh boot; results 26.9, evaluation protocol 14.14 item 4)"
    echo "declaration sha256=$(decl_sha) path=$DECL"
    echo "measured $(date -u +%FT%TZ) free $f (df -B1 /)"
    [ -z "${RUN_CYCLE_FREE_CMD:-}" ] || echo "# RUN_CYCLE_FREE_CMD (test seam): $RUN_CYCLE_FREE_CMD"
    echo "declared floor $fl margin $mg per_boot $pb"
    echo "measured per boot (the largest over the boots already run): $mx ($mw)"
    echo "per boot used $used; boots remaining in the cycle, this one included: $rem"
    echo "need $need"
    if [ "$f" -ge "$need" ]; then echo "verdict met (free - need = $((f - need)))"; else echo "verdict short by $((need - f))"; fi
  } > "$out"
  [ "$f" -ge "$need" ]
}
case "$STEP" in
  verify)
    [ -f "$DECL" ] || { echo "run_cycle.sh: no declaration $DECL" >&2; exit 64; }
    mkdir -p "$L" || exit 2
    keep_old "$L/verify.txt" || exit 2   # the earlier output under a new name (nanoseconds: two verifies in one second, results 26.5)
    { echo "kz-verify 1"
      echo "declaration sha256=$(decl_sha) path=$DECL"
      echo "measured $(date -u +%FT%TZ)"
      [ -z "${RUN_CYCLE_OBS:-}" ] || echo "# RUN_CYCLE_OBS=$RUN_CYCLE_OBS (test seam)"
      n=0; a=0; b=0; c=0; ln=0; nv=
      while IFS=$'\t' read -r kind bld name val path; do
        ln=$((ln + 1))
        case "$kind" in ''|'#'*) continue ;; need) nv="$nv $name=$val;"; continue ;; esac
        n=$((n + 1))
        if [ "$kind" = plan-id ]; then
          sp=$(awk -F'\t' -v b="$bld" '$1=="plan" && $2==b && $3=="plan.side" {print $5}' "$DECL")
          f=$(resolve "$sp"); got=$( [ -f "$f" ] && sed -n 's/^plan-id //p' "$f" | head -1 )
          [ -f "$f" ] || got=missing
        elif [ "$kind" = count ]; then   # the entries under a directory that are not directories (results 26.9; 26.5 counted regular files)
          f=$(resolve "$path"); if [ -d "$f" ]; then got=$(find "$f" ! -type d | wc -l); else got=missing; fi
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
      echo "need rows (values, not measured; held by the declaration's sha256 above):${nv:- none}"
      echo "rows $n ok $a differs $b missing $c"
    } > "$L/verify.txt"
    tail -1 "$L/verify.txt"
    grep -qE '^rows [0-9]+ ok [0-9]+ differs 0 missing 0$' "$L/verify.txt" ;;
  boot)
    allowed || { echo "run_cycle.sh: boot $N of $B is not allowed by the boot-number rule (results 25.12)" >&2; exit 2; }
    verify_clean "$L" || { echo "run_cycle.sh: no clean verify.txt of boot $N naming the declaration as it is now: not booting (results 26.0 (아))" >&2; exit 1; }
    # results 26.5 (protocol 14.13 item 9): the declaration and this script committed and unchanged
    git -C "$R" ls-files --error-unmatch "$DECL" "$HERE/run_cycle.sh" > /dev/null 2>&1 && [ -z "$(git -C "$R" status --porcelain -- "$DECL" "$HERE/run_cycle.sh")" ] \
      || { echo "run_cycle.sh: the declaration or run_cycle.sh is not committed or has uncommitted changes: not booting (results 26.5)" >&2; exit 1; }
    # results 26.9 (protocol 14.14 item 5): the declaration pinned at the cycle's first boot
    if [ -f "$PIN" ]; then need_pin
    elif [ "$B" != c1_pre_tx ] || [ "$N" != 1 ]; then
      echo "run_cycle.sh: no boot/declaration_pin.txt: only the cycle's first boot (c1_pre_tx 1) runs without it: not booting (results 26.9)" >&2; exit 1
    fi
    # results 26.5: verify again here (results 26.9: an earlier verify_at_boot.txt is kept under a new name)
    reverify "$L/verify_at_boot.txt" "$N" \
      || { echo "run_cycle.sh: the verify step run again before the boot is not clean: not booting (results 26.5)" >&2; exit 1; }
    # results 26.9 (protocol 14.14 item 4): the space need of the boots remaining
    space_check || { echo "run_cycle.sh: the free space is below the need of the boots remaining ($L/space_at_boot.txt): not booting (results 26.9; results 26.0 (카))" >&2; exit 1; }
    [ ! -e "$RUND" ] || { echo "run_cycle.sh: $RUND exists; one boot per number" >&2; exit 2; }
    mkdir -p "$L" && mkdir "$RUND" || exit 2
    keep_old "$L/boot.txt" || exit 2
    if [ -n "${RUN_CYCLE_BOOT_CMD:-}" ]; then
      echo "# RUN_CYCLE_BOOT_CMD (test seam): $RUN_CYCLE_BOOT_CMD" > "$L/boot.txt"
      bash -c "$RUN_CYCLE_BOOT_CMD" >> "$L/boot.txt" 2>&1; rc=$?
    else
      g boot 60000000 bash "$GS/run_gi_c1.sh" "$W" "$CFG" 1df262a tx "$RUN" "$RT/kz-rt-dump" "$P/plan.kzs" > "$L/boot.txt" 2>&1; rc=$?
    fi
    echo "rc=$rc" >> "$L/boot.txt"
    if [ $rc -ne 0 ] && [ ! -e "$RUN" ]; then   # results 26.9 (protocol 14.14 item 3): not booted
      nb=$L/boot.notbooted.$(ts).txt; mv "$L/boot.txt" "$nb"
      if rmdir "$RUND" 2> /dev/null; then echo "# $RUND (empty) removed" >> "$nb"; else echo "# $RUND not empty: kept" >> "$nb"; fi
      tail -5 "$nb"
      echo "run_cycle.sh: not booted (rc $rc before $RUN existed; protocol 14.14 item 3): no boot of protocol 2; the same N may run again once the cause is removed" >&2
      exit $rc
    fi
    if [ ! -f "$PIN" ]; then
      { echo "kz-declaration-pin 1 (run_cycle.sh boot; results 26.9, evaluation protocol 14.14 item 5)"
        echo "declaration sha256=$(decl_sha) path=$DECL"
        echo "written $(date -u +%FT%TZ) by the boot step of $B $N (boot command rc $rc)"; } > "$PIN"
    fi
    tail -5 "$L/boot.txt"; exit $rc ;;
  control)
    [ ! -e "$L/control_check.txt" ] || refuse_exists "$L/control_check.txt"
    T=$RUN/tx.dump.txt
    { echo "dump text sha256 $(sha256sum < "$T" | cut -c1-64) lines $(wc -l < "$T")"
      echo "record kinds (first word, count):"; awk '{print $1}' "$T" | sort | uniq -c | sed 's/^/  /'
      grep -E '^run abi|^runx |^caps|^buildid ' "$T" | sed 's/^/  /'
      echo "loss fields not 0 (kind=value, count): $(grep -oE '\b(overflow|partial|dropped_bytes|pool_dropped|boot_bytes_dropped)=[1-9][0-9]*' "$T" | sort | uniq -c | tr -s ' ' | tr '\n' ';')"
      echo "loss records by kind (lines, sum of count=; no-build-id and object-table lose no event, event_semantics 13.5): $(awk '$1=="loss"{n=0; for(i=3;i<=NF;i++) if ($i ~ /^count=/) n=substr($i,7); c[$2]+=n; l[$2]++} END{for (k in c) printf "%s lines=%d count=%d\n", k, l[k], c[k]}' "$T" | sort | tr '\n' ';')"
      echo "pool overflow fields not 0: $(grep -oE '\b(overflow|pool_dropped)=[1-9][0-9]*' "$T" | wc -l)"
      echo "kz-rt-dump check summary: $(grep -E '^check ' "$T" | tail -1)"
    } > "$L/control_check.txt"; cat "$L/control_check.txt" ;;
  convert)
    [ ! -e "$CV" ] || { echo "run_cycle.sh: $CV exists" >&2; exit 2; }
    mkdir "$CV" || exit 2
    g convert 20000000 "$MC/tools/kz/kz" convert --kzrt "$RUN/dump/tx.kzrt" --manifest "$P/plan.kzs" --manifest "$W/binds_instr.kzs" \
      --basis "$BP/basis.kz" --out "$CV/run.kzt" > "$CV/convert.log" 2>&1; rc=$?
    echo "convert rc=$rc" >> "$CV/convert.log"
    if [ $rc -eq 75 ] && [ ! -e "$CV/run.kzt" ]; then   # results 26.9: refused at start by the space guard, kz convert did not run
      mv "$CV/convert.log" "$L/convert.refused.$(ts).log" && rmdir "$CV"
      echo "run_cycle.sh: convert refused at start by the space guard (not run; $CV removed; protocol 14.14 item 3)" >&2; exit 75
    fi
    cp "$CV/convert.log" "$L/convert.log"; cat "$L/convert.log"; exit $rc ;;
  kzt-text)
    for x in "$CV/run.kzt.txt" "$CV/facts_header.txt" "$L/convert.sha256"; do [ ! -e "$x" ] || refuse_exists "$x"; done
    "$MC/tools/kz-dump/kz-dump" "$CV/run.kzt" > "$CV/run.kzt.txt" 2>&1; echo "kz-dump run.kzt rc $?"
    "$MC/tools/kz-dump/kz-dump" "$BP/facts.kz" 2>&1 | head -8 > "$CV/facts_header.txt"
    (cd "$CV" && sha256sum run.kzt run.kzt.txt facts_header.txt convert.log) > "$L/convert.sha256"; cat "$L/convert.sha256" ;;
  chain)
    [ ! -e "$L/chain.txt" ] || refuse_exists "$L/chain.txt"
    IL=$(sed -n 's/^instance_files .* list=\([^ ]*\) .*/\1/p' "$RUN/run.meta")
    python3 -I "$R/env/tools/chain_check.py" --plan "$P/plan.kzs" --build-record "$W/build_instr.record" --run-meta "$RUN/run.meta" \
      --build-root "$W/ws" --dump-text "$RUN/tx.dump.txt" --basis-text "$BP/basis.txt" --main exe/tx/core-tx \
      --sidecar "$P/plan.side" --plan-facts "$PF/facts.kz" --plan-basis "$PF/basis.kz" --inst-list "$IL" --inst-tree "$W/ws/exe" \
      --dump "$RUN/dump/tx.kzrt" --bindings "$W/binds_instr.kzs" --bprime-basis "$BP/basis.kz" --facts-header "$CV/facts_header.txt" \
      --kzt-text "$CV/run.kzt.txt" -o "$L/chain.txt"; rc=$?; echo "chain_check rc=$rc"; tail -12 "$L/chain.txt"; exit 0 ;;
  stop)
    [ ! -e "$L/stop_eval.tsv" ] || refuse_exists "$L/stop_eval.tsv"
    python3 -I "$EX/stop_eval.py" --manifest "$P/plan.kzs" --sidecar "$P/plan.side" \
      --startup-script "$RUN/inst/tx/cf/cfe_es_startup.scr" --dump-text "$RUN/tx.dump.txt" -o "$L/stop_eval.tsv" ;;
  gate)
    # evaluation protocol 3: (1) the control record check is clean, (2) losses 0 or named, a pool overflow fails,
    # (3) the chain is confirmed.  same_build is not in the gate (it is basis 1 of every pair).
    # a loss other than a pool overflow passes only named: its kinds and counts are this file's second line.  What the
    # explanation records do with it is the frozen judgement part's rule, not this comment: a loss line of a kind in its
    # NONEVENT_LOSS (no-build-id, object-table) is no loss marker and marks no need unknown-loss; every other loss
    # marks the needs not reached unknown-loss (NEED_REASON; results 26.5, 26.9 fix-judge-08)
    [ ! -e "$L/gate.txt" ] || refuse_exists "$L/gate.txt"
    c1=$(grep -c '^kz-rt-dump check summary: check ok' "$L/control_check.txt")
    c2=$(grep -c '^pool overflow fields not 0: 0$' "$L/control_check.txt")
    c3=$(grep -cP '^chain\tconfirmed\t' "$L/chain.txt")
    lo=$(sed -n 's/^loss fields not 0 (kind=value, count): //p' "$L/control_check.txt")
    lr=$(sed -n 's/^loss records by kind ([^)]*): //p' "$L/control_check.txt")
    { if [ "$c1" = 1 ] && [ "$c2" = 1 ] && [ "$c3" = 1 ]; then echo "gate PASS"; else
        echo "gate FAIL control_ok=$c1 no_pool_overflow=$c2 chain_confirmed=$c3"; fi
      echo "losses: fields ${lo:-none}; records ${lr:-none}"
      echo "inputs: control_check.txt $(sha256sum < "$L/control_check.txt" | cut -c1-16) chain.txt $(sha256sum < "$L/chain.txt" | cut -c1-16)"
    } > "$L/gate.txt"; cat "$L/gate.txt" ;;
  explain)
    need_pin
    [ ! -e "$XO" ] || { echo "run_cycle.sh: $XO exists" >&2; exit 2; }
    reverify "$L/verify_at_explain.txt" "$N" \
      || { echo "run_cycle.sh: the verify step run again before explain is not clean: not explaining (results 26.9, protocol 14.14 item 6)" >&2; exit 1; }
    IL=$(sed -n 's/^instance_files .* list=\([^ ]*\) .*/\1/p' "$RUN/run.meta")
    NS=(); for f in framework_namespaces2_cfe-6.4.1.txt framework_namespaces2_cfe-6.4.2.txt framework_namespaces2_osal-4.1.1.txt \
                    framework_namespaces2_osal-7.0.1.txt framework_namespaces2_osal-f34759d.txt; do NS+=(--namespaces "$SEM/$f"); done
    keep_old "$L/explain.txt" || exit 2
    g explain 60000000 python3 -I "$EX/explain.py" --sidecar "$P/plan.side" --manifest "$P/plan.kzs" --bindings "$W/binds_instr.kzs" \
      --relations "$ORD/relations.txt" --keys "$ORD/keys.txt" --pairs "$ORD/pairs.tsv" "${NS[@]}" \
      --facts-view "$O/xr4_dry_inputs/$B/facts_view.txt" --bprime-basis "$BP/basis.kz" --build-label "$B" \
      --dump-text "$RUN/tx.dump.txt" --run-meta "$RUN/run.meta" --chain "$L/chain.txt" --stop-eval "$L/stop_eval.tsv" \
      --startup-script "$RUN/inst/tx/cf/cfe_es_startup.scr" --instance-list "$IL" \
      --branches "$D/candidates/branches_16.txt" --candidates "$D/candidates/candidates.tsv" --named-calls "$D/candidates/named_calls.tsv" \
      --label-record "C1 cycle boot $N of $B (results 26.9, boot/declaration4.tsv)" \
      --vocabulary "$R/kairos/tests/static/data/reason_vocabulary.txt" --out "$XO" > "$L/explain.txt" 2>&1; rc=$?
    echo "rc=$rc" >> "$L/explain.txt"
    if [ $rc -eq 75 ] && [ ! -e "$XO" ]; then   # results 26.9: refused at start by the space guard, explain.py did not run
      mv "$L/explain.txt" "$L/explain.refused.$(ts).txt"
      echo "run_cycle.sh: explain refused at start by the space guard (not run; protocol 14.14 item 3)" >&2; exit 75
    fi
    tail -20 "$L/explain.txt"; exit $rc ;;
  eval)
    need_pin
    [ ! -e "$L/sheet_boot.tsv" ] || { echo "run_cycle.sh: $L/sheet_boot.tsv exists: this cycle has no reader (protocol 14.5, 14.14 item 6): not evaluating" >&2; exit 2; }
    for x in "$EV" "$L/eval_tail.txt" "$L/eval.txt"; do [ ! -e "$x" ] || refuse_exists "$x"; done
    keep_old "$L/eval_run.txt" || exit 2
    mkdir "$EV" || exit 2
    g eval 20000000 python3 -I "$EX/explain_eval.py" --explain "$XO" --build "$B" --boot "boot$N" --gate "$L/gate.txt" --pairs "$ORD/pairs.tsv" \
      --sidecar "$P/plan.side" --relations "$ORD/relations.txt" --uses "$D/uses/uses_$B.tsv" --sheets-static "$D/sheets/sheets_static.tsv" \
      --sheets-boot none --expect "$D/expect/expect_c1_2.tsv" --out "$EV/eval.txt" > "$L/eval_run.txt" 2>&1; rc=$?
    echo "rc=$rc" >> "$L/eval_run.txt"
    if [ $rc -eq 75 ] && [ ! -e "$EV/eval.txt" ]; then   # results 26.9: refused at start by the space guard, explain_eval.py did not run
      rmdir "$EV"; mv "$L/eval_run.txt" "$L/eval_run.refused.$(ts).txt"
      echo "run_cycle.sh: eval refused at start by the space guard (not run; $EV removed; protocol 14.14 item 3)" >&2; exit 75
    fi
    if [ -f "$EV/eval.txt" ]; then   # lead ruling 26.7 (다): the head and the per-branch tail only (the extraction of results 26.5)
      F=$EV/eval.txt
      { echo "== $F (lines $(wc -l < "$F"), sha256 $(sha256sum < "$F" | cut -c1-64)); the full output is kept under the work directory with a ledger row (the keep step; results 26.9)"
        grep -vP '^(F1|F2a|F2b|F3|F4|F5|F6|F7|F8a|F8b|F9|F10|s1|s2|s3|s4|records|expect_discriminate|precision|in_plan_recall|undetermined_split|count)\t' "$F"
        echo "== the expect_discriminate lines"; grep -P '^expect_discriminate\t' "$F"
        echo "== the F3, F4 and F9 lines of the main branch"; grep -P '^(F3|F4|F9)\tboot\tD1-R0\t' "$F"; } > "$L/eval_tail.txt"
    fi
    tail -6 "$L/eval_run.txt"; exit $rc ;;
  expect)
    python3 -I "$EX/explain_expect.py" --expect "$D/expect/expect_c1_2.tsv" --records "$XO" --out "$L/expect_compare.txt"; rc=$?
    echo "rc=$rc"; tail -1 "$L/expect_compare.txt"; exit $rc ;;
  keep)
    LED=$R/records/LEDGER.tsv
    [ -f "$LED" ] || { echo "run_cycle.sh: no ledger $LED" >&2; exit 64; }
    for x in boot convert explain eval; do
      awk -F'\t' -v i="OBS_c1cyc_${S}_$x$N" '$1==i {f=1} END {exit !f}' "$LED" && refuse_exists "ledger row OBS_c1cyc_${S}_$x$N"
      [ ! -e "$L/c1cyc_${S}_$x$N.sha256" ] || refuse_exists "$L/c1cyc_${S}_$x$N.sha256"
    done
    [ ! -e "$L/keep.txt" ] || refuse_exists "$L/keep.txt"
    PR=$O/c1cyc_ledger_proposed.$$.tsv; [ ! -e "$PR" ] || { echo "run_cycle.sh: $PR exists" >&2; exit 2; }
    cp "$LED" "$PR" || exit 2
    k=0
    echo "kz-keep 1 (run_cycle.sh keep; results 26.9, evaluation protocol 14.14 item 1) $(date -u +%FT%TZ)" > "$L/keep.txt"
    for x in boot convert explain eval; do
      d=$O/c1cyc_${S}_$x$N
      [ -d "$d" ] || { echo "$x $d: absent, no row" >> "$L/keep.txt"; continue; }
      [ -n "$(find "$d" -type f -print -quit)" ] || { echo "$x $d: holds no file, no row" >> "$L/keep.txt"; continue; }
      lst=$L/c1cyc_${S}_$x$N.sha256
      (cd "$d" && find . -type f -print0 | LC_ALL=C sort -z | xargs -0 -r sha256sum) > "$lst" \
        || { rm -f "$PR"; for y in boot convert explain eval; do keep_old "$L/c1cyc_${S}_$y$N.sha256" failed; done; keep_old "$L/keep.txt" failed
             echo "run_cycle.sh: hashing $d failed (the attempt kept as *.failed.*)" >&2; exit 1; }
      case $x in boot) w="the boot's work directory (run.meta, conditions, instance tree, dump and its text)" ;;
                 convert) w="kz convert of the boot's dump, its kz-dump text, the facts header" ;;
                 explain) w="the explanation records of the boot (16 branches)" ;; eval) w="the full eval output (lead ruling 26.7 (다))" ;; esac
      printf '%s\toutput\tsha256c\t%s\t%s\tyes\tnone\tnone\tno\t%s\n' "OBS_c1cyc_${S}_$x$N" "${lst#$R/}" "$d" \
        "C1 cycle boot $N of $B: $w (results 27; boot/declaration4.tsv, run_cycle.sh keep)" >> "$PR"
      echo "$x $d: $(grep -c . "$lst") files, row OBS_c1cyc_${S}_$x$N, list ${lst#$R/}" >> "$L/keep.txt"
      k=$((k + 1))
    done
    keep_old "$L/ledger_check_keep.txt" failed || exit 2
    bash "$R/env/tools/ledger_check.sh" "$PR" > "$L/ledger_check_keep.txt"; lc=$?
    if [ $lc -ne 0 ]; then   # the ledger is not changed; this attempt's files are kept as *.failed.<UTC ns>.* so the step can run again
      rm -f "$PR"; for x in boot convert explain eval; do keep_old "$L/c1cyc_${S}_$x$N.sha256" failed; done
      keep_old "$L/keep.txt" failed; keep_old "$L/ledger_check_keep.txt" failed
      echo "run_cycle.sh: ledger_check of the proposed ledger rc $lc: the ledger is not changed (the attempt kept as *.failed.*)" >&2; exit 1
    fi
    cp "$PR" "$LED" && cmp -s "$PR" "$LED" || { echo "run_cycle.sh: copying the proposed ledger failed" >&2; exit 1; }
    rm -f "$PR"; echo "keep: $k ledger rows added; $(tail -1 "$L/ledger_check_keep.txt")" | tee -a "$L/keep.txt" ;;
  summary)
    need_pin
    [ -f "$HERE/$B/boot5/gate.txt" ] || { echo "run_cycle.sh: no $B/boot5/gate.txt: summary runs after boot 5 (results 26.9)" >&2; exit 2; }
    reverify "$HERE/$B/verify_at_summary.txt" 5 \
      || { echo "run_cycle.sh: the verify step run again before summary is not clean: no summary (results 26.9, protocol 14.14 item 6)" >&2; exit 1; }
    A=(); for d in "$HERE/$B"/boot*; do n=${d##*/boot}; [ "$(gate_of "$n")" = PASS ] && A+=("boot$n=$O/c1cyc_${S}_explain$n"); done
    [ ${#A[@]} -gt 0 ] || { echo "run_cycle.sh: no boot of $B passed the gate" >&2; exit 2; }
    python3 -I "$EX/explain_summary.py" --out "$HERE/$B/summary.txt" --uses "$D/uses/uses_$B.tsv" "${A[@]}" ;;
  *) echo "usage: run_cycle.sh verify|boot|control|convert|kzt-text|chain|stop|gate|explain|eval|expect|keep|summary BUILD N" >&2; exit 64 ;;
esac
