#!/usr/bin/env bash
# kz_run_c1.sh CFG CAND [altP3] [rx] -- the static closure of one C1 P-REC build read with the C1 profile
# (kairos/profiles/c1-bp-b051eb0 + cf-<rev> + bundle-<cand> + rspec-<rev>[-altP3]), reconstructed 2026-10-29
# (env/REBUILD_20261029.md).  The original c1_build/kz_run_c1.sh (= procrace/integ_20261025/closures.sh) was outside the
# repository and is lost.
#
#   kz_run_c1.sh <pre|post> <1df262a|1b0b338> [altP3] [rx]
#   -> c1_build/kz/<cfg>_<cand>/c1prof[_altP3][_rx]/closure.{basis.kz,facts.kz,basis.log,extract.log}, sha256.txt
#
# Fixed by the references:
#   usage and output directory   kairos/docs/c1_smoke_observe.md section 9.2 ("c1_build/kz_run_c1.sh <pre|post> <cand>
#                                [altP3] -> c1_build/kz/<cfg>_<cand>/c1prof[_altP3]/closure.{basis,facts}.kz"; the
#                                recipe is kz_run.sh's closure.kzb)
#   rx                           closure_rx.kzb (plan/results_pilot_20261025.md section 0)
#   commands                     kairos/instrument/gcc/scripts/w4w5_c1_closures.sh f0 ("the recipe and commands of
#                                procrace/integ_20261025/closures.sh (= c1_build/kz_run_c1.sh)"): profiles, compile DB,
#                                build dir, bundle root, recipe, extract operations and the seven targets
# Reconstruction choices (RC-8): the order of the optional words, the _rx suffix of the output directory (the records
# named it c1prof_i1025_rx), sha256.txt; kz_run.sh recipes is run first when closure.kzb is absent.
set -uo pipefail
CFG=${1:?pre|post}; CAND=${2:?1df262a|1b0b338}; shift 2
ALT=; INST=tx
for a in "$@"; do case "$a" in altP3) ALT=-altP3 ;; rx) INST=rx ;; *) echo "unknown word $a (altP3|rx)" >&2; exit 64 ;; esac; done
case "$CFG" in pre) REV=281a941 ;; post) REV=833fdbb ;; *) echo "CFG is pre|post" >&2; exit 64 ;; esac
case "$CAND" in 1df262a|1b0b338) ;; *) echo "CAND is 1df262a|1b0b338" >&2; exit 64 ;; esac
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
C1=${C1_DIR:-/home/user/work/procrace/c1_build}
KZ=${KZ_SRC:-/home/user/Shape-Perf/research/cfs_procrace/kairos}
K=${KZ_STATIC:-/home/user/work/kairos-build/k1r3-llvm/tools/kz-static/kz-static}
P=$KZ/profiles/c1-bp-b051eb0
PROF=(--profile $P --profile $P/cf-$REV --profile $P/bundle-$CAND --profile $P/rspec-$REV$ALT)
TG=(--target OS_common_record_t.name_entry --target OS_common_record_t.active_id --target OS_count_sem_internal_record_t.obj_name
    --target CF_Channel_t.sem_id --target CF_ChannelConfig_t.sem_name --target BP_Throttle_t.Name --target BP_Throttle_t.Sem)
B=$C1/$CFG/$CAND/build
KD=$C1/kz/${CFG}_${CAND}
RC=closure.kzb; SUF=; [ "$INST" = rx ] && { RC=closure_rx.kzb; SUF=_rx; }
[ -f "$KD/$RC" ] || "$HERE/kz_run.sh" "$CFG" "$CAND" recipes > /dev/null
D=$KD/c1prof$ALT$SUF; mkdir -p "$D"
cd "$KD" || exit 2
"$K" basis "${PROF[@]}" --compile-db $B/native/default/compile_commands.json --build-dir $B --bundle-root $C1/bundle_$CAND --recipe $RC \
    -o $D/closure.basis.kz > $D/closure.basis.log 2>&1; a=$?
"$K" extract "${PROF[@]}" --basis $D/closure.basis.kz --delay-op OS_TaskDelay --lock-op CFE_ES_LockSharedData --unlock-op CFE_ES_UnlockSharedData \
    --spawn-op CFE_ES_CreateChildTask "${TG[@]}" -o $D/closure.facts.kz > $D/closure.extract.log 2>&1; b=$?
sha256sum $D/closure.basis.kz $D/closure.facts.kz $KD/$RC $B/native/default/compile_commands.json $K > $D/sha256.txt
echo "kz_run_c1 $CFG $CAND rspec-$REV$ALT $INST basis rc=$a extract rc=$b"
