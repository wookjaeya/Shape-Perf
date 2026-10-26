#!/usr/bin/env bash
# basis_gi.sh -- the basis B' and static closure of a stage D derived (kzgcc) build, extracted from ITS compile database
# with the same profiles, recipe and extract arguments as the F0 closures of the same configuration
# (/home/user/work/procrace/integ_20261025/closures.sh, c1prof_i1025 / replay_i1025), as decided in docs/static_facts.md
# section 7a (one B' per derived build; the reference facts are not reused).  Pilot/dev only.
#
#   basis_gi.sh c1 <W> <CFG pre|post> <CAND 1df262a|1b0b338> <INST tx|rx> <out dir>
#   basis_gi.sh r2 <W> <out dir>
# <W>: the work space of derive_c1.sh (<W>/ws/build, <W>/bundle_<CAND>) or derive_r2.sh (<W>/cFS/build-native_std).
# The recipe is the reference build's recipe file read in place (paths in it are relative to the build directory): C1
# closure.kzb (tx) or closure_rx.kzb (rx; the tx recipe with the three tx paths replaced, results_pilot_20261025.md 0),
# R2 profiles/cfs-v7.0.1/basis/core-cpu1.kzb.  Outputs: <out>/basis.kz, <out>/facts.kz, their logs, <out>/sha256.txt.
set -uo pipefail
K=${KZ_STATIC:-/home/user/work/kairos-build/k1r3-llvm/tools/kz-static/kz-static}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KZ="$(cd "$HERE/../../.." && pwd)"
case "${1:-}" in
c1)
  W=${2:?W}; CFG=${3:?cfg}; CAND=${4:?cand}; INST=${5:?inst}; O=${6:?out}
  case "$CFG" in pre) REV=281a941 ;; post) REV=833fdbb ;; *) echo "cfg pre|post" >&2; exit 64 ;; esac
  case "$INST" in tx) RC=closure.kzb ;; rx) RC=closure_rx.kzb ;; *) echo "inst tx|rx" >&2; exit 64 ;; esac
  RC=/home/user/work/procrace/c1_build/kz/${CFG}_${CAND}/$RC
  P=$KZ/profiles/c1-bp-b051eb0
  PROF=(--profile $P --profile $P/cf-$REV --profile $P/bundle-$CAND --profile $P/rspec-$REV)
  TG=(--target OS_common_record_t.name_entry --target OS_common_record_t.active_id --target OS_count_sem_internal_record_t.obj_name
      --target CF_Channel_t.sem_id --target CF_ChannelConfig_t.sem_name --target BP_Throttle_t.Name --target BP_Throttle_t.Sem)
  B=$W/ws/build
  mkdir -p "$O"
  $K basis "${PROF[@]}" --compile-db $B/native/default/compile_commands.json --build-dir $B --bundle-root $W/bundle_$CAND --recipe $RC \
      -o "$O/basis.kz" > "$O/basis.log" 2>&1; echo "c1 $CFG $CAND $INST basis rc=$?"
  $K extract "${PROF[@]}" --basis "$O/basis.kz" --delay-op OS_TaskDelay --lock-op CFE_ES_LockSharedData --unlock-op CFE_ES_UnlockSharedData \
      --spawn-op CFE_ES_CreateChildTask "${TG[@]}" -o "$O/facts.kz" > "$O/facts.log" 2>&1; echo "c1 $CFG $CAND $INST extract rc=$?"
  sha256sum "$O/basis.kz" "$O/facts.kz" "$RC" "$K" > "$O/sha256.txt" ;;
r2)
  W=${2:?W}; O=${3:?out}
  B=$W/cFS/build-native_std
  mkdir -p "$O"
  $K basis --profile $KZ/profiles/cfs-v7.0.1 --compile-db $B/native/default_cpu1/compile_commands.json --build-dir $B \
      --bundle-root $W/cFS --recipe $KZ/profiles/cfs-v7.0.1/basis/core-cpu1.kzb -o "$O/basis.kz" > "$O/basis.log" 2>&1; echo "r2 basis rc=$?"
  $K extract --profile $KZ/profiles/cfs-v7.0.1 --basis "$O/basis.kz" --delay-op OS_TaskDelay --lock-op CFE_ES_LockSharedData \
      --unlock-op CFE_ES_UnlockSharedData --spawn-op CFE_ES_CreateChildTask -o "$O/facts.kz" > "$O/facts.log" 2>&1; echo "r2 extract rc=$?"
  sha256sum "$O/basis.kz" "$O/facts.kz" "$K" > "$O/sha256.txt" ;;
*) echo "usage: basis_gi.sh c1 <W> <CFG> <CAND> <INST> <out> | r2 <W> <out>" >&2; exit 64 ;;
esac
