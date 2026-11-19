#!/usr/bin/env bash
# Re-run of the probe scripts of records/closure-texts-20261118 over the outputs of the fixed kz-static bf2ec2ee (rf2_*); only the input directory names differ from the kept copies.  run_probes.sh WORKDIR OUTDIR
#   WORKDIR holds views/c1_pre_tx.txt and views/c1_post_tx.txt (`kz-static show` of the final option-on facts,
#   /home/user/work/redesign/obs1/rf2_final_extract/<build>/<build>/NP/facts.kz); OUTDIR is new.
# Every command's rc is checked before the next one.
set -uo pipefail
WD=$1; OUT=$2; HERE=$(cd "$(dirname "$0")" && pwd); R=$(cd "$HERE/../../../.." && pwd)
W=/home/user/work/redesign/obs1; NSD=$R/kairos/semantics/framework_namespaces_osal-f34759d.txt
[ -e "$OUT" ] && { echo "OUT exists" >&2; exit 2; }; mkdir -p "$OUT" || exit 2
cd "$WD" || exit 2
run() { local o=$1; shift; "$@" > "$OUT/$o"; local rc=$?; echo "rc=$rc $o" | tee -a "$OUT/commands_rc.txt"; [ $rc -eq 0 ] || exit 1; }
for b in pre post; do
  for row in A20 A21 A22 A26 A63; do
    [ $b = post ] && [ $row != A20 ] && [ $row != A21 ] && continue
    run rel_${b}_$row.txt python3 -I "$HERE/row_extract.py" $W/rf2_final_order/c1_${b}_tx/relations.txt $row
  done
done
for row in A20 A21 A22 A26; do run p1_alternatives_c1_pre_tx_$row.txt python3 -I "$HERE/p1_alternatives.py" views/c1_pre_tx.txt $W/rf2_final_order/c1_pre_tx/relations.txt $row; done
for row in A20 A21; do run p1_alternatives_c1_post_tx_$row.txt python3 -I "$HERE/p1_alternatives.py" views/c1_post_tx.txt $W/rf2_final_order/c1_post_tx/relations.txt $row; done
for b in c1_pre_tx c1_post_tx; do
  run p2_success_path_$b.txt python3 -I "$HERE/p2_success_path.py" views/$b.txt $W/rf2_final_order/$b/relations.txt
  run p3_buffer_writers_$b.txt python3 -I "$HERE/p3_buffer_writers.py" $NSD views/$b.txt
done
run p3b_name_entry_uses_c1_pre_tx.txt python3 -I "$HERE/p3b_pointer_uses.py" views/c1_pre_tx.txt OS_common_record_t.name_entry
for b in c1_pre_tx c1_post_tx; do for row in A21 A20 A22 A26; do
  [ $b = c1_post_tx ] && [ $row != A20 ] && [ $row != A21 ] && continue
  run p4_branch_i_${b}_$row.txt python3 -I "$HERE/p4_branch_i.py" views/$b.txt $W/rf2_final_order/$b/relations.txt $row
done; done
run p5_pairs.txt python3 -I "$HERE/p5_pairs.py"
run p6_keys.txt python3 -I "$HERE/p6_keys.py"
for b in c1_pre_tx c1_post_tx; do run p7_items_$b.txt python3 -I "$HERE/p7_items.py" $b $NSD "$OUT"; done
run open_reason_population.txt python3 -I "$HERE/open_reason_population.py" $W/rf2_final_order c1_pre_tx c1_post_tx r2_core c5_641 c5_642
echo done
