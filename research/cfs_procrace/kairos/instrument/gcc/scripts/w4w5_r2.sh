#!/usr/bin/env bash
# w4w5_r2.sh <out root> [closures|replay|occ|all] -- W4/W5 R2 package (Architecture/spec/PI_DECISIONS_RESOLVED_20261028.md
# section 5 items 2 and 4): the four R2 records read with one fixed analysis configuration, pilot/dev only, no run is made.
#   records  p3    preserved F0  /home/user/work/procrace/r2_runs/v153_pilot3 (kernel v153, start state: zero EEPROM file
#                  and key files present, no -R option)
#            d4gi  preserved G-I /home/user/work/instr/gcc/d4/r2/run1 (its build tree is gone; converted with its own
#                  binding list d4/r2/binds_instr.kzs and the B' of the re-derived tree, see below)
#            f0b   fresh F0      /home/user/work/w5/run/r2/f0b (w5 campaign, R2 common Power-On start, --reset PO)
#            gi    fresh G-I     /home/user/work/w5/run/r2/gi  (w5 campaign, re-derived build /home/user/work/w5/r2_derived)
#   closures the F0 static closure of the reference build (docs/static_facts.md section 2 command, the same as
#            /home/user/work/procrace/integ_20261025/closures.sh r2) and B' of the re-derived build (basis_gi.sh r2), both
#            with the current profiles/cfs-v7.0.1.  The re-derived tree has the same binding lines as d4 (the binding lists
#            differ only in comment lines: paths and merge time) and the same manifest, plugin and runtime inputs
#            (campaign R/env/campaigns/w5_20261028.sh); its binary hash differs (derive_r2.sh D4 strings).
#   readings cur (defaults, --compare-narrowing as replay_gi.sh), p1off (--outcome-narrowing off), nocode
#            (--outcome-narrowing off --reads-from-outcome off), each with --out; constants and extra inputs as
#            plan/results_pilot_20261025.md section 0 (CFE_PLATFORM_ES_MAX_APPLICATIONS=32, --core-request-caller
#            CFE_ES_CreateObjects; F0 --basis = the run's own observe basis).
#   occ      kz occ show per record and reading; kz occ diff p3-f0b, d4gi-gi, f0b-gi, p3-d4gi per reading (exit 3 =
#            differences or ambiguous keys, kept in exit.txt); W4: kz occ diff of the earlier stored containers
#            (replay_i1025/cur.kz, d4 run1 replay/cur.kz, schema before revision 4: positional) against the new cur.kz.
# Every output directory gets a sha256.txt of its inputs and outputs.
set -uo pipefail
O=${1:?usage: w4w5_r2.sh <out root> [closures|replay|occ|all]}; STEP=${2:-all}
KZ_BIN=${KZ_BIN:-/home/user/work/kairos-build/tools/kz/kz}
K=${KZ_STATIC:-/home/user/work/kairos-build/k1r3-llvm/tools/kz-static/kz-static}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KZ="$(cd "$HERE/../../.." && pwd)"
P=$KZ/profiles/cfs-v7.0.1
REF=/home/user/work/procrace/cfs_ref/cFS
P3=/home/user/work/procrace/r2_runs/v153_pilot3
D4=/home/user/work/instr/gcc/d4/r2
W5=/home/user/work/w5/run/r2
WR2=/home/user/work/w5/r2_derived
CF=$O/closure_f0; BP=$O/bprime
mkdir -p "$O"

closures() {
  mkdir -p "$CF"
  ( cd "$CF" && $K basis --profile $P --compile-db $REF/build-native_std/native/default_cpu1/compile_commands.json \
      --build-dir $REF/build-native_std --bundle-root $REF --recipe $P/basis/core-cpu1.kzb -o static_basis.kz > static_basis.log 2>&1
    echo "f0 basis rc=$?" > exit.txt
    $K extract --profile $P --basis static_basis.kz --delay-op OS_TaskDelay --lock-op CFE_ES_LockSharedData \
      --unlock-op CFE_ES_UnlockSharedData --spawn-op CFE_ES_CreateChildTask -o facts.kz > facts.log 2>&1
    echo "f0 extract rc=$?" >> exit.txt
    sha256sum static_basis.kz facts.kz $REF/build-native_std/native/default_cpu1/compile_commands.json \
      $REF/build-native_std/exe/cpu1/core-cpu1 $P/basis/core-cpu1.kzb $K > sha256.txt ) &
  "$HERE/basis_gi.sh" r2 "$WR2" "$BP" > "$O/bprime.out" 2>&1 &
  wait
  sha256sum $WR2/cFS/build-native_std/native/default_cpu1/compile_commands.json $WR2/cFS/build-native_std/exe/cpu1/core-cpu1 \
    $P/basis/core-cpu1.kzb >> "$BP/sha256.txt"
  cat "$CF/exit.txt" "$O/bprime.out"
}

readings() {   # <out> <args...>
  local D=$1; shift
  : > $D/exit.txt
  $KZ_BIN replay "$@" --compare-narrowing --out $D/cur.kz > $D/cur.txt 2>&1; echo "cur $?" >> $D/exit.txt
  $KZ_BIN replay "$@" --outcome-narrowing off --out $D/p1off.kz > $D/p1off.txt 2>&1; echo "p1off $?" >> $D/exit.txt
  $KZ_BIN replay "$@" --outcome-narrowing off --reads-from-outcome off --out $D/nocode.kz > $D/nocode.txt 2>&1; echo "nocode $?" >> $D/exit.txt
}
COMMON=(--profile $P --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 --core-request-caller CFE_ES_CreateObjects)

f0rec() {   # <name> <kzt> <observe basis>
  local D=$O/$1; mkdir -p $D
  readings $D "$2" "${COMMON[@]}" --basis "$3" --facts $CF/facts.kz --static-basis $CF/static_basis.kz
  sha256sum "$2" "$3" $CF/facts.kz $CF/static_basis.kz $D/*.kz $D/*.txt $KZ_BIN > $D/sha256.tmp; mv $D/sha256.tmp $D/sha256.txt
}
girec() {   # <name> <dump> <binding list>
  local D=$O/$1; mkdir -p $D
  $KZ_BIN convert --kzrt "$2" --manifest $P/instr/sites.kzs --manifest "$3" --basis $BP/basis.kz --out $D/run.kzt > $D/convert.log 2>&1
  echo "convert rc=$?" >> $D/convert.log
  readings $D $D/run.kzt "${COMMON[@]}" --basis $BP/basis.kz --facts $BP/facts.kz --static-basis $BP/basis.kz
  sha256sum "$2" "$3" $P/instr/sites.kzs $BP/basis.kz $BP/facts.kz $D/run.kzt $D/*.kz $D/*.txt $D/convert.log $KZ_BIN > $D/sha256.tmp; mv $D/sha256.tmp $D/sha256.txt
}
replay() {
  f0rec p3 $P3/v153_pilot3.kzt $P3/v153_pilot3_basis.kz &
  f0rec f0b $W5/f0b/w5_r2_f0b.kzt $W5/f0b/w5_r2_f0b_basis.kz &
  wait
  girec d4gi $D4/run1/dump/cpu1.kzrt $D4/binds_instr.kzs &
  girec gi $W5/gi/dump/cpu1.kzrt $WR2/binds_instr.kzs &
  wait
  for r in p3 f0b d4gi gi; do echo "$r $(tr '\n' ' ' < $O/$r/exit.txt) $(tail -1 $O/$r/convert.log 2>/dev/null)"; done
}
occ() {
  local D=$O/occ; mkdir -p $D; : > $D/exit.txt
  for r in p3 f0b d4gi gi; do for g in cur p1off nocode; do
    $KZ_BIN occ show $O/$r/$g.kz > $D/show_${r}_$g.txt 2>&1; echo "show_${r}_$g $?" >> $D/exit.txt
  done; done
  for pr in p3:f0b d4gi:gi f0b:gi p3:d4gi; do a=${pr%:*}; b=${pr#*:}; for g in cur p1off nocode; do
    $KZ_BIN occ diff $O/$a/$g.kz $O/$b/$g.kz > $D/diff_${a}_${b}_$g.txt 2>&1; echo "diff_${a}_${b}_$g $?" >> $D/exit.txt
  done; done
  # W4: the earlier stored containers against the unified-configuration replay of the same record
  $KZ_BIN occ diff $P3/replay_i1025/cur.kz $O/p3/cur.kz > $D/w4_p3_i1025_cur.txt 2>&1; echo "w4_p3_i1025_cur $?" >> $D/exit.txt
  $KZ_BIN occ diff $P3/replay_i1025/p1off.kz $O/p3/p1off.kz > $D/w4_p3_i1025_p1off.txt 2>&1; echo "w4_p3_i1025_p1off $?" >> $D/exit.txt
  $KZ_BIN occ diff $D4/run1/replay/cur.kz $O/d4gi/cur.kz > $D/w4_d4gi_d4_cur.txt 2>&1; echo "w4_d4gi_d4_cur $?" >> $D/exit.txt
  $KZ_BIN occ diff $D4/run1/replay/p1off.kz $O/d4gi/p1off.kz > $D/w4_d4gi_d4_p1off.txt 2>&1; echo "w4_d4gi_d4_p1off $?" >> $D/exit.txt
  sha256sum $O/*/cur.kz $O/*/p1off.kz $O/*/nocode.kz $P3/replay_i1025/cur.kz $P3/replay_i1025/p1off.kz \
    $D4/run1/replay/cur.kz $D4/run1/replay/p1off.kz $D/*.txt $KZ_BIN > $D/sha256.tmp; mv $D/sha256.tmp $D/sha256.txt
  cat $D/exit.txt
}
# sites: the calls at the three SB event points (CFE_EVS_SendEventWithAppID.id_valid@sb_api_275 pipe created,
# @sb_api_1183 subscription received, @sb_priv_786 transmit event; cfe_sb_api.c L275 L1183, cfe_sb_priv.c L786) counted
# from the neutral layer of each record (F0: kz convert of the .kzt, kept as <r>/neutral.kzt; G-I: <r>/run.kzt) per owner
# application record (the AppID's record key, the occurrence key's app[<key>]), site, EventID (slot 0) and AppID passed
# (slot 2).  Writes <r>/sbsites.tsv.
sites() {
  local DUMP=${KZ_DUMP:-/home/user/work/kairos-build/tools/kz-dump/kz-dump} r n
  for r in p3 f0b d4gi gi; do
    case $r in p3) $KZ_BIN convert $P3/v153_pilot3.kzt --out $O/p3/neutral.kzt > $O/p3/neutral.log 2>&1; n=$O/p3/neutral.kzt ;;
               f0b) $KZ_BIN convert $W5/f0b/w5_r2_f0b.kzt --out $O/f0b/neutral.kzt > $O/f0b/neutral.log 2>&1; n=$O/f0b/neutral.kzt ;;
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
    { grep -v -e '/neutral.kzt$' -e '/sbsites.tsv$' -e '/kz-dump$' -e "/${P3##*/}.kzt$" -e '/w5_r2_f0b.kzt$' $O/$r/sha256.txt
      sha256sum $n $O/$r/sbsites.tsv $DUMP; } > $O/$r/sha256.tmp; mv $O/$r/sha256.tmp $O/$r/sha256.txt
    case $r in p3) sha256sum $P3/v153_pilot3.kzt >> $O/$r/sha256.txt ;; f0b) sha256sum $W5/f0b/w5_r2_f0b.kzt >> $O/$r/sha256.txt ;; esac
    echo "$r $(awk -F'\t' '{n[$2 == "evs_send_entry" ? "entry" : "sb"] += $5} END {print "evs.send.entry", n["entry"], "SB-site", n["sb"]}' $O/$r/sbsites.tsv)"
  done
}
case "$STEP" in
  closures) closures ;; replay) replay ;; occ) occ ;; sites) sites ;;
  all) closures; replay; occ; sites ;;
  *) echo "step closures|replay|occ|sites|all" >&2; exit 64 ;;
esac
