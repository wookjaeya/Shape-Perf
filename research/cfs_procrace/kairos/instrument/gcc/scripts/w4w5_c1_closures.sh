#!/usr/bin/env bash
# w4w5_c1_closures.sh <out root> -- W4/W5 (2026-10-28) C1: the static closures of the fixed analysis configuration, all
# extracted again with the current profiles (kairos/profiles/c1-bp-b051eb0 at the commit of the analyzer), because the
# closures of i1025 and of the 97a24d8 snapshot do not carry the current profile hash.  Pilot/dev tooling; nothing is run.
#
#   F0  the four reference builds (c1_build/<cfg>/<cand>/build), tx and rx: the recipe and commands of
#       procrace/integ_20261025/closures.sh (= c1_build/kz_run_c1.sh, current rspec only; rx = closure_rx.kzb,
#       results_pilot_20261025.md section 0)                                  -> <out>/f0/<cfg>_<cand>[_rx]/closure.{basis,facts}.kz
#   B'  the derived builds the instrumented records used, per instance (basis_gi.sh, docs/static_facts.md section 7a):
#       stage D d2 (the natural G-I records of d3, run.meta "bindings .../d2/c1/...")   -> <out>/bprime_d2/<cfg>_<cand>_<inst>/
#       stage F f1 (the G-I-H records of f1/gih and the W5 G-I and G-I-H records)        -> <out>/bprime_f1/<cfg>_<cand>_<inst>/
# Two jobs at a time (KZ_JOBS, default 2).  Each output directory gets a sha256.txt.
set -uo pipefail
O=${1:?out root}; mkdir -p "$O"; O="$(cd "$O" && pwd)"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KZ="$(cd "$HERE/../../.." && pwd)"
K=${KZ_STATIC:-/home/user/work/kairos-build/k1r3-llvm/tools/kz-static/kz-static}
CB=/home/user/work/procrace/c1_build
P=$KZ/profiles/c1-bp-b051eb0
f0() {  # CFG CAND INST
  local CFG=$1 CAND=$2 INST=$3 REV RC=closure.kzb SUF=
  [ "$INST" = rx ] && { RC=closure_rx.kzb; SUF=_rx; }
  case $CFG in pre) REV=281a941 ;; post) REV=833fdbb ;; esac
  local PROF=(--profile $P --profile $P/cf-$REV --profile $P/bundle-$CAND --profile $P/rspec-$REV)
  local B=$CB/$CFG/$CAND/build D=$O/f0/${CFG}_${CAND}$SUF
  local TG=(--target OS_common_record_t.name_entry --target OS_common_record_t.active_id --target OS_count_sem_internal_record_t.obj_name
    --target CF_Channel_t.sem_id --target CF_ChannelConfig_t.sem_name --target BP_Throttle_t.Name --target BP_Throttle_t.Sem)
  mkdir -p "$D"; cd $CB/kz/${CFG}_${CAND} || return 1
  $K basis "${PROF[@]}" --compile-db $B/native/default/compile_commands.json --build-dir $B --bundle-root $CB/bundle_$CAND --recipe $RC \
      -o $D/closure.basis.kz > $D/closure.basis.log 2>&1; local a=$?
  $K extract "${PROF[@]}" --basis $D/closure.basis.kz --delay-op OS_TaskDelay --lock-op CFE_ES_LockSharedData --unlock-op CFE_ES_UnlockSharedData \
      --spawn-op CFE_ES_CreateChildTask "${TG[@]}" -o $D/closure.facts.kz > $D/closure.extract.log 2>&1; local b=$?
  sha256sum $D/closure.basis.kz $D/closure.facts.kz $CB/kz/${CFG}_${CAND}/$RC $B/native/default/compile_commands.json $K > $D/sha256.txt
  echo "f0 $CFG $CAND $INST basis=$a extract=$b"
}
bp() {  # STAGE CFG CAND INST
  local S=$1 CFG=$2 CAND=$3 INST=$4
  local W=/home/user/work/instr/gcc/$S/c1/${CFG}_${CAND} D=$O/bprime_$S/${CFG}_${CAND}_$INST
  KZ_STATIC=$K "$HERE/basis_gi.sh" c1 $W $CFG $CAND $INST $D | tr '\n' ' '
  sha256sum $W/ws/build/native/default/compile_commands.json $W/binds_instr.kzs >> $D/sha256.txt; echo " ($S)"
}
export -f f0 bp; export O HERE K CB P
{ for CFG in pre post; do for CAND in 1df262a 1b0b338; do for INST in tx rx; do
    echo "f0 $CFG $CAND $INST"; echo "bp d2 $CFG $CAND $INST"; echo "bp f1 $CFG $CAND $INST"
  done; done; done; } | xargs -P ${KZ_JOBS:-2} -L 1 bash -c '"$@"' _ | sort > "$O/closures.log"
cat "$O/closures.log"
