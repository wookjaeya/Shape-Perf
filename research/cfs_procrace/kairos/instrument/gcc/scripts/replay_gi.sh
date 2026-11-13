#!/usr/bin/env bash
# replay_gi.sh -- kz convert of a G-I dump (run_gi_c1.sh / run_gi_r2.sh) with its manifests and the build's B' (basis_gi.sh),
# then kz replay with the profiles, constants and readings of the F0 replays of the same configuration
# (/home/user/work/procrace/integ_20261025/replays.sh, the readings of results_pilot_20261025.md 0; C1 confirm1 natural rx
# used the same, confirm1/scripts/replays.sh).  Pilot/dev only.
#
#   replay_gi.sh c1 <W> <CFG> <CAND> <INST> <run dir> <B' dir>
#   replay_gi.sh r2 <W> <run dir> <B' dir>
# Writes <run dir>/replay/{run.kzt,convert.log,<reading>.txt,cur.kz,p1off.kz,trace.txt,exit.txt,sha256.txt}.
# Optional environment (2026-10-26, stage D leftovers): KZ_SITES = the logical site manifest to convert with (default
# the profile's file; give the version the build used when the file changed since), KZ_PROCESS = the process id the
# harness recorded for the dump's process (run.meta tx_pid/rx_pid; for a dump without RUNX process, kzrt-dump 1.1),
# KZ_OUT = the output directory (default <run dir>/replay).  The reading nocode is p1off with --reads-from-outcome off:
# no returned code is used, neither to fix a condition nor to show a reads_from use successful.
# The run's basis (the replay's --basis, the converted record's parent) is B' itself: the kz-static basis names the
# derived binary (binary.sha256) and is the parent of the facts.  It carries no SYMBOLS section, so an owner's calling
# function comes only from the binding of its request site (schema.md 12.3.1); none is read from an address.
set -uo pipefail
KZ_BIN=${KZ_BIN:-/home/user/work/kairos-build/tools/kz/kz}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KZ="$(cd "$HERE/../../.." && pwd)"
readings() {   # <out> <args...>: the F0 readings
  local O=$1; shift
  : > $O/exit.txt
  $KZ_BIN replay "$@" --compare-narrowing --out $O/cur.kz > $O/cur.txt 2>&1; echo "cur $?" >> $O/exit.txt
  $KZ_BIN replay "$@" --outcome-narrowing off --out $O/p1off.kz > $O/p1off.txt 2>&1; echo "p1off $?" >> $O/exit.txt
  $KZ_BIN replay "$@" --owner-identity-targets off > $O/idoff.txt 2>&1; echo "idoff $?" >> $O/exit.txt
  $KZ_BIN replay "$@" --clearing-writes counted > $O/cw_counted.txt 2>&1; echo "cw_counted $?" >> $O/exit.txt
  $KZ_BIN replay "$@" --clearing-writes excluded > $O/cw_excluded.txt 2>&1; echo "cw_excluded $?" >> $O/exit.txt
  $KZ_BIN replay "$@" --emit-trace > $O/trace.txt 2>&1; echo "trace $?" >> $O/exit.txt
  $KZ_BIN replay "$@" --outcome-narrowing off --reads-from-outcome off > $O/nocode.txt 2>&1; echo "nocode $?" >> $O/exit.txt
}
PROC=(); [ -n "${KZ_PROCESS:-}" ] && PROC=(--process "$KZ_PROCESS")
case "${1:-}" in
c1)
  W=${2:?W}; CFG=${3:?cfg}; CAND=${4:?cand}; INST=${5:?inst}; R=${6:?run}; BP=${7:?bprime}
  case "$CFG" in pre) REV=281a941 ;; post) REV=833fdbb ;; *) echo "cfg pre|post" >&2; exit 64 ;; esac
  P=$KZ/profiles/c1-bp-b051eb0; O=${KZ_OUT:-$R/replay}; mkdir -p $O
  $KZ_BIN convert --kzrt $R/dump/$INST.kzrt --manifest ${KZ_SITES:-$P/instr/sites-cf$REV.kzs} --manifest $W/binds_instr.kzs --basis $BP/basis.kz \
      "${PROC[@]}" --out $O/run.kzt > $O/convert.log 2>&1; echo "convert rc=$?" >> $O/convert.log
  A=($O/run.kzt --profile $P --profile $P/cf-$REV --profile $P/bundle-$CAND --profile $P/rspec-$REV
     --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 --const OS_MAX_API_NAME=20
     --basis $BP/basis.kz --facts $BP/facts.kz --static-basis $BP/basis.kz)
  readings $O "${A[@]}"
  $KZ_BIN replay "${A[@]}" --reads-from-edges off > $O/rfoff.txt 2>&1; echo "rfoff $?" >> $O/exit.txt
  $KZ_BIN replay "${A[@]}" --reads-from-edges off --outcome-narrowing off > $O/rfoff_p1off.txt 2>&1; echo "rfoff_p1off $?" >> $O/exit.txt ;;
r2)
  W=${2:?W}; R=${3:?run}; BP=${4:?bprime}
  P=$KZ/profiles/cfs-v7.0.1; O=${KZ_OUT:-$R/replay}; mkdir -p $O
  $KZ_BIN convert --kzrt $R/dump/cpu1.kzrt --manifest ${KZ_SITES:-$P/instr/sites.kzs} --manifest $W/binds_instr.kzs --basis $BP/basis.kz \
      "${PROC[@]}" --out $O/run.kzt > $O/convert.log 2>&1; echo "convert rc=$?" >> $O/convert.log
  A=($O/run.kzt --profile $P --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32
     --basis $BP/basis.kz --core-request-caller CFE_ES_CreateObjects --facts $BP/facts.kz --static-basis $BP/basis.kz)
  readings $O "${A[@]}"
  $KZ_BIN replay "${A[@]}" --owner-identity-targets off --outcome-narrowing off > $O/idoff_p1off.txt 2>&1; echo "idoff_p1off $?" >> $O/exit.txt ;;
*) echo "usage: replay_gi.sh c1 <W> <CFG> <CAND> <INST> <run> <bprime> | r2 <W> <run> <bprime>" >&2; exit 64 ;;
esac
sha256sum $O/*.txt $O/*.kz $O/run.kzt $BP/basis.kz $BP/facts.kz $KZ_BIN > $O/sha256.txt
