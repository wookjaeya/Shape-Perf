#!/usr/bin/env bash
# replay_w6.sh <out root> [closures|replay|occ|sites|intervals|all] -- W6 R2 replays (PD29 section 4 step 3): the two W6
# R2 records (bundle records/w6-20261029-runs2) read with the commands of kairos/instrument/gcc/scripts/w4w5_r2.sh.
# Pilot/dev only; no run is made.  w4w5_r2.sh is not reused as is because its record and build paths are fixed in it
# (L29-L34: P3, D4 run1, W5, WR2 = /home/user/work/w5/r2_derived, all gone with the 2026-10-29 container change).
# Here only those paths differ:
#   records  f0  /home/user/work/w6/run/r2/f0  F0 (observe_cfs_smoke.sh --start poweron, env/campaigns/w6_20261029.sh L80)
#            gi  /home/user/work/w6/run/r2/gi  G-I (run_gi_r2.sh ... poweron, w6_20261029.sh L84) of the derived build
#                /home/user/work/instr/gcc/d4/r2 (44 bindings, rebuilt 2026-10-29 05:57Z, plan/r2_intervals_20261029.md
#                section 5.1); converted with that build's own binding list and the manifest of the profile at HEAD
#   closures as w4w5_r2.sh closures() L38-L53: the F0 static closure of the reference build (docs/static_facts.md section 2
#            command) and B' of the derived build (basis_gi.sh r2), both with the current profiles/cfs-v7.0.1
#   readings as w4w5_r2.sh readings() L55-L61 (cur --compare-narrowing, p1off, nocode) and COMMON L62 (constant
#            CFE_PLATFORM_ES_MAX_APPLICATIONS=32, --core-request-caller CFE_ES_CreateObjects; F0 --basis = the run's own
#            observe basis, f0rec L64-L68; G-I convert and replay as girec L69-L75).  Added: ctloff (--control-order off),
#            the C1 package's reading (w4w5_c1_replay.sh L74, f0ctl_confirm2.sh L36); R2 records carry no control input,
#            so it is the current reading without --compare-narrowing (PD26 section 3: control order on = current reading)
#   occ      as w4w5_r2.sh occ() L85-L91: kz occ show per record and reading, kz occ diff f0-gi per reading
#            (exit 0 / 3 differences / 4 unpaired; kairos/docs/replay_output.md section 4)
#   sites    as w4w5_r2.sh sites() L107-L132: the R2 calls per owner record, site, EventID and AppID from the neutral
#            layer of each record (<r>/sbsites.tsv)
#   intervals kairos/instrument/gcc/scripts/r2_intervals_tag.sh on both records with the cur reading text
#            (plan/r2_intervals_20261029.md section 4.1)
# Every output directory gets a sha256.txt of its inputs and outputs.
set -uo pipefail
O=${1:?usage: replay_w6.sh <out root> [closures|replay|occ|sites|intervals|all]}; STEP=${2:-all}
KZ_BIN=${KZ_BIN:-/home/user/work/kairos-build/tools/kz/kz}
K=${KZ_STATIC:-/home/user/work/kairos-build/k1r3-llvm/tools/kz-static/kz-static}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KZ="$(cd "$HERE/../../kairos" && pwd)"
S=$KZ/instrument/gcc/scripts
P=$KZ/profiles/cfs-v7.0.1
REF=/home/user/work/procrace/cfs_ref/cFS
D4=/home/user/work/instr/gcc/d4/r2
W6=/home/user/work/w6/run/r2
CF=$O/closure_f0; BP=$O/bprime
mkdir -p "$O"
wait_campaign() { while [ -e /home/user/work/.campaign_running ]; do sleep 60; done; }

closures() {   # w4w5_r2.sh L38-L53, WR2 -> D4
  wait_campaign
  mkdir -p "$CF"
  ( cd "$CF" && $K basis --profile $P --compile-db $REF/build-native_std/native/default_cpu1/compile_commands.json \
      --build-dir $REF/build-native_std --bundle-root $REF --recipe $P/basis/core-cpu1.kzb -o static_basis.kz > static_basis.log 2>&1
    echo "f0 basis rc=$?" > exit.txt
    $K extract --profile $P --basis static_basis.kz --delay-op OS_TaskDelay --lock-op CFE_ES_LockSharedData \
      --unlock-op CFE_ES_UnlockSharedData --spawn-op CFE_ES_CreateChildTask -o facts.kz > facts.log 2>&1
    echo "f0 extract rc=$?" >> exit.txt
    sha256sum static_basis.kz facts.kz $REF/build-native_std/native/default_cpu1/compile_commands.json \
      $REF/build-native_std/exe/cpu1/core-cpu1 $P/basis/core-cpu1.kzb $K > sha256.txt ) &
  "$S/basis_gi.sh" r2 "$D4" "$BP" > "$O/bprime.out" 2>&1 &
  wait
  sha256sum $D4/cFS/build-native_std/native/default_cpu1/compile_commands.json $D4/cFS/build-native_std/exe/cpu1/core-cpu1 \
    $P/basis/core-cpu1.kzb >> "$BP/sha256.txt"
  cat "$CF/exit.txt" "$O/bprime.out"
}

readings() {   # <out> <args...>   w4w5_r2.sh L55-L61, and ctloff
  local D=$1; shift
  : > $D/exit.txt
  $KZ_BIN replay "$@" --compare-narrowing --out $D/cur.kz > $D/cur.txt 2>&1; echo "cur $?" >> $D/exit.txt
  $KZ_BIN replay "$@" --outcome-narrowing off --out $D/p1off.kz > $D/p1off.txt 2>&1; echo "p1off $?" >> $D/exit.txt
  $KZ_BIN replay "$@" --outcome-narrowing off --reads-from-outcome off --out $D/nocode.kz > $D/nocode.txt 2>&1; echo "nocode $?" >> $D/exit.txt
  $KZ_BIN replay "$@" --control-order off --out $D/ctloff.kz > $D/ctloff.txt 2>&1; echo "ctloff $?" >> $D/exit.txt
}
COMMON=(--profile $P --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 --core-request-caller CFE_ES_CreateObjects)
READS=(cur p1off nocode ctloff)

f0rec() {   # <name> <kzt> <observe basis>   w4w5_r2.sh L64-L68
  local D=$O/$1; mkdir -p $D
  readings $D "$2" "${COMMON[@]}" --basis "$3" --facts $CF/facts.kz --static-basis $CF/static_basis.kz
  sha256sum "$2" "$3" $CF/facts.kz $CF/static_basis.kz $D/*.kz $D/*.txt $KZ_BIN > $D/sha256.tmp; mv $D/sha256.tmp $D/sha256.txt
}
girec() {   # <name> <dump> <binding list>   w4w5_r2.sh L69-L75
  local D=$O/$1; mkdir -p $D
  $KZ_BIN convert --kzrt "$2" --manifest $P/instr/sites.kzs --manifest "$3" --basis $BP/basis.kz --out $D/run.kzt > $D/convert.log 2>&1
  echo "convert rc=$?" >> $D/convert.log
  readings $D $D/run.kzt "${COMMON[@]}" --basis $BP/basis.kz --facts $BP/facts.kz --static-basis $BP/basis.kz
  sha256sum "$2" "$3" $P/instr/sites.kzs $BP/basis.kz $BP/facts.kz $D/run.kzt $D/*.kz $D/*.txt $D/convert.log $KZ_BIN > $D/sha256.tmp; mv $D/sha256.tmp $D/sha256.txt
}
replay() {   # w4w5_r2.sh L76-L84 for the two W6 records
  wait_campaign
  f0rec f0 $W6/f0/w6_r2_f0.kzt $W6/f0/w6_r2_f0_basis.kz &
  girec gi $W6/gi/dump/cpu1.kzrt $D4/binds_instr.kzs &
  wait
  for r in f0 gi; do echo "$r $(tr '\n' ' ' < $O/$r/exit.txt) $(tail -1 $O/$r/convert.log 2>/dev/null)"; done
}
occ() {   # w4w5_r2.sh L85-L91 for the pair f0-gi
  local D=$O/occ r g; mkdir -p $D; : > $D/exit.txt
  for r in f0 gi; do for g in "${READS[@]}"; do
    $KZ_BIN occ show $O/$r/$g.kz > $D/show_${r}_$g.txt 2>&1; echo "show_${r}_$g $?" >> $D/exit.txt
  done; done
  for g in "${READS[@]}"; do
    $KZ_BIN occ diff $O/f0/$g.kz $O/gi/$g.kz > $D/diff_f0_gi_$g.txt 2>&1; echo "diff_f0_gi_$g $?" >> $D/exit.txt
  done
  # the readings of one record against each other (same record: the reading line differs, the exit status does not count it)
  for r in f0 gi; do for g in nocode ctloff; do
    $KZ_BIN occ diff $O/$r/cur.kz $O/$r/$g.kz > $D/diff_${r}_cur_$g.txt 2>&1; echo "diff_${r}_cur_$g $?" >> $D/exit.txt
  done; done
  sha256sum $O/f0/*.kz $O/gi/*.kz $D/*.txt $KZ_BIN > $D/sha256.tmp; mv $D/sha256.tmp $D/sha256.txt
  cat $D/exit.txt
}
sites() {   # w4w5_r2.sh L107-L132 (same awk), F0 neutral layer from kz convert, G-I run.kzt
  local DUMP=${KZ_DUMP:-/home/user/work/kairos-build/tools/kz-dump/kz-dump} r n
  for r in f0 gi; do
    case $r in f0) $KZ_BIN convert $W6/f0/w6_r2_f0.kzt --out $O/f0/neutral.kzt > $O/f0/neutral.log 2>&1; n=$O/f0/neutral.kzt ;;
               *) n=$O/$r/run.kzt ;; esac
    $DUMP $n | awk '
      function hex2(x,   i, d, v) { v = 0; x = tolower(x); sub(/^0x/, "", x); for (i = 1; i <= length(x); i++) { d = index("0123456789abcdef", substr(x, i, 1)) - 1; v = v * 16 + d }; return v }
      /NSITES\[/ { i = $1; sub(/.*\[/, "", i); sub(/\].*/, "", i); l = $0; sub(/.*label="/, "", l); sub(/".*/, "", l); lab[i] = l }
      /NACTORS\[/ { i = $1; sub(/.*\[/, "", i); sub(/\].*/, "", i); o = $0; sub(/.* owner_id=/, "", o); sub(/ .*/, "", o); own[i] = o }
      /NEVENTS\[/ { s = $0; sub(/.* site=/, "", s); sub(/ .*/, "", s); if (lab[s] !~ /id_valid@sb_/ && lab[s] != "evs.send.entry") next
                    a = $0; sub(/.* actor=/, "", a); sub(/ .*/, "", a); f = $0; sub(/.* first_value=/, "", f); sub(/ .*/, "", f)
                    ev[++ne] = a SUBSEP s; fv[ne] = f }
      /NVALUES\[/ { i = $1; sub(/.*\[/, "", i); sub(/\].*/, "", i); v = $2; sub(/v=/, "", v); val[i] = v }
      END { for (k = 1; k <= ne; k++) { split(ev[k], e, SUBSEP); o = own[e[1]]; rec = (o == "none") ? "none" : sprintf("%d", o - 1114112)
              l = lab[e[2]]; sub(/.*@/, "", l); sub(/^evs.send.entry$/, "evs_send_entry", l); c[rec "\t" l "\t" hex2(val[fv[k]]) "\t" val[fv[k] + 2]]++ }
            for (x in c) print x "\t" c[x] }' | sort -t$'\t' -k1,1n -k2,2 -k3,3n > $O/$r/sbsites.tsv
    sha256sum $n $O/$r/sbsites.tsv $DUMP > $O/$r/sbsites.sha256.txt
    [ $r = f0 ] && sha256sum $W6/f0/w6_r2_f0.kzt >> $O/$r/sbsites.sha256.txt
    echo "$r $(awk -F'\t' '{n[$2 == "evs_send_entry" ? "entry" : "sb"] += $5} END {print "evs.send.entry", n["entry"], "SB-site", n["sb"]}' $O/$r/sbsites.tsv)"
  done
}
intervals() {   # plan/r2_intervals_20261029.md section 4.1
  "$S/r2_intervals_tag.sh" $W6/f0/w6_r2_f0.kzt $O/f0/cur.txt $O/intervals/f0 > $O/intervals/f0.out 2>&1; echo "f0 rc=$?"
  "$S/r2_intervals_tag.sh" $O/gi/run.kzt $O/gi/cur.txt $O/intervals/gi > $O/intervals/gi.out 2>&1; echo "gi rc=$?"
}
mkdir -p $O/intervals
case "$STEP" in
  closures) closures ;; replay) replay ;; occ) occ ;; sites) sites ;; intervals) intervals ;;
  all) closures; replay; occ; sites; intervals ;;
  *) echo "step closures|replay|occ|sites|intervals|all" >&2; exit 64 ;;
esac
