#!/usr/bin/env bash
# replay_gih.sh -- the stage F replays: one kz and one profile directory for the G-I-H runs and their F0 confirm2
# counterparts, so that the two families are read by the same rules (plan/results_instr_gate_20261026.md).  Pilot/dev only.
#
#   replay_gih.sh gih <W> <CFG> <CAND> <INST> <run dir> <B' dir> <profiles>   a G-I-H run of run_gih_c1.sh
#   replay_gih.sh f0 <confirm2 run dir> <profiles> <out dir>                    an F0 confirm2 run (kz hold), read again
# <profiles>: the profiles directory (a snapshot of a commit: the working tree's profile may be changing).
# G-I-H: kz convert --kzrt (the build's manifest and binding list, B' as the parent), then the readings of the F0 replays
# (confirm2/scripts/replays.sh: cur with --compare-narrowing, p1off, rfoff, rfoff_p1off, idoff, cw_counted, cw_excluded) with
# the P11 reading decided on 2026-10-26 (control order on, the default) and, as sensitivity, the same with --control-order
# off (ctloff, ctloff_p1off); the trace and its class (classify_gih.awk); the realized control (control_cmp.sh).
# F0: the same readings on the confirm2 run container (no NCTL: the control order has nothing to read; P11 reading A of
# confirm2 stays classify.awk's), with the closures confirm2 used (c1_build/kz/<cfg>_<cand>/c1prof_i1025[_rx]).
set -uo pipefail
KZ_BIN=${KZ_BIN:-/home/user/work/instr/gcc/f1/kz-build/tools/kz/kz}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KZ="$(cd "$HERE/../../.." && pwd)"
readings() {   # <out> <args...>
  local O=$1; shift
  $KZ_BIN replay "$@" --compare-narrowing --out $O/cur.kz > $O/cur.txt 2>&1; echo "cur $?" >> $O/exit.txt
  $KZ_BIN replay "$@" --outcome-narrowing off --out $O/p1off.kz > $O/p1off.txt 2>&1; echo "p1off $?" >> $O/exit.txt
  $KZ_BIN replay "$@" --reads-from-edges off > $O/rfoff.txt 2>&1; echo "rfoff $?" >> $O/exit.txt
  $KZ_BIN replay "$@" --reads-from-edges off --outcome-narrowing off > $O/rfoff_p1off.txt 2>&1; echo "rfoff_p1off $?" >> $O/exit.txt
  $KZ_BIN replay "$@" --owner-identity-targets off > $O/idoff.txt 2>&1; echo "idoff $?" >> $O/exit.txt
  $KZ_BIN replay "$@" --clearing-writes counted > $O/cw_counted.txt 2>&1; echo "cw_counted $?" >> $O/exit.txt
  $KZ_BIN replay "$@" --clearing-writes excluded > $O/cw_excluded.txt 2>&1; echo "cw_excluded $?" >> $O/exit.txt
  $KZ_BIN replay "$@" --emit-trace > $O/trace.txt 2>&1; echo "trace $?" >> $O/exit.txt
}
case "${1:-}" in
gih)
  W=${2:?W}; CFG=${3:?cfg}; CAND=${4:?cand}; INST=${5:?inst}; R=${6:?run}; BP=${7:?bprime}; PR=${8:?profiles}
  case "$CFG" in pre) REV=281a941 ;; post) REV=833fdbb ;; *) echo "cfg pre|post" >&2; exit 64 ;; esac
  P=$PR/c1-bp-b051eb0; O=$R/replay; mkdir -p $O; : > $O/exit.txt
  $KZ_BIN convert --kzrt $R/dump/$INST.kzrt --manifest $KZ/profiles/c1-bp-b051eb0/instr/sites-cf$REV.kzs --manifest $W/binds_instr.kzs --basis $BP/basis.kz \
      --out $O/run.kzt > $O/convert.log 2>&1; echo "convert $?" >> $O/exit.txt
  A=($O/run.kzt --profile $P --profile $P/cf-$REV --profile $P/bundle-$CAND --profile $P/rspec-$REV
     --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 --const OS_MAX_API_NAME=20
     --basis $BP/basis.kz --facts $BP/facts.kz --static-basis $BP/basis.kz)
  readings $O "${A[@]}"
  $KZ_BIN replay "${A[@]}" --control-order off --out $O/ctloff.kz > $O/ctloff.txt 2>&1; echo "ctloff $?" >> $O/exit.txt
  $KZ_BIN replay "${A[@]}" --control-order off --outcome-narrowing off > $O/ctloff_p1off.txt 2>&1; echo "ctloff_p1off $?" >> $O/exit.txt
  awk -f $HERE/classify_gih.awk $O/trace.txt > $O/class.txt 2>&1; echo "classify $?" >> $O/exit.txt
  $HERE/control_cmp.sh gih $R > $O/control.txt 2>&1; echo "control $?" >> $O/exit.txt
  sha256sum $O/*.txt $O/*.kz $O/run.kzt $BP/basis.kz $BP/facts.kz $KZ_BIN > $O/sha256.txt ;;
f0)
  R=${2:?run}; PR=${3:?profiles}; O=${4:?out}
  b=$(basename $R); x=${b#*_}; CFG=${x%%_*}; CAND=$(echo $x | cut -d_ -f2); INST=${b##*_}
  SUF=; [ "$INST" = rx ] && SUF=_rx
  case "$CFG" in pre) REV=281a941 ;; post) REV=833fdbb ;; *) echo "bad run name $b" >&2; exit 64 ;; esac
  CL=/home/user/work/procrace/c1_build/kz/${CFG}_${CAND}/c1prof_i1025$SUF
  P=$PR/c1-bp-b051eb0; mkdir -p $O; : > $O/exit.txt
  A=("$R/run.kzt" --profile $P --profile $P/cf-$REV --profile $P/bundle-$CAND --profile $P/rspec-$REV
     --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 --const OS_MAX_API_NAME=20
     --basis "$R/basis_core.kz" --facts $CL/closure.facts.kz --static-basis $CL/closure.basis.kz)
  readings $O "${A[@]}"
  $HERE/control_cmp.sh f0 $R > $O/control.txt 2>&1; echo "control $?" >> $O/exit.txt
  sha256sum $O/*.txt $O/*.kz $CL/closure.facts.kz $CL/closure.basis.kz $KZ_BIN > $O/sha256.txt ;;
*) echo "usage: replay_gih.sh gih <W> <CFG> <CAND> <INST> <run> <bprime> <profiles> | f0 <run> <profiles> <out>" >&2; exit 64 ;;
esac
