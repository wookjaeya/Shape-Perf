#!/usr/bin/env bash
# test_run_cycle.sh WORK_DIR -- the tests of the verify step of run_cycle.sh (spec 11 item 14; results 26.0 (h)).
# Each case copies run_cycle.sh into its own stub tree under WORK_DIR (a research root with records/x/boot/, a work
# directory given by the test seam RUN_CYCLE_OBS, a repository path and an absolute path), writes a stub declaration of
# six rows (a plan-id, a plan file, an ELF with its GNU build-id read by readelf -n from a copy of a system executable, a
# research-root file, a repository file, an absolute file) and runs the verify and boot steps; the boot command is the
# test seam RUN_CYCLE_BOOT_CMD (it writes a marker file), so nothing is booted.  Cases: a clean declaration (verify rc 0,
# the boot runs the mock); a changed declared value (differs, rc 1, the boot refuses and the mock is not called); a
# missing file (missing, rc 1, no boot); a declaration changed after a clean verify (no boot); no verify.txt (no boot).
# One line per check and a count; rc 0 when every check passes.  Writes only under WORK_DIR.
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WK=${1:?usage: test_run_cycle.sh WORK_DIR}
[ ! -e "$WK" ] || { echo "test_run_cycle.sh: $WK exists" >&2; exit 64; }
mkdir -p "$WK" || exit 2
NC=0; NF=0
res() { NC=$((NC + 1)); if [ "$2" = 0 ]; then echo "PASS $1 ($3)"; else echo "FAIL $1 ($3)"; NF=$((NF + 1)); fi; }
ELF=$(readlink -f /usr/bin/true)
tree() { # NAME: a stub tree; prints its boot directory
  local t=$WK/$1 b
  b=$t/root/research/cfs_procrace/records/x/boot
  mkdir -p "$b" "$t/root/research/cfs_procrace/env" "$t/obs/plans/c1_pre_tx" "$t/obs/elf" "$t/abs"
  cp "$HERE/run_cycle.sh" "$b/run_cycle.sh"
  echo "plan-id abc123" > "$t/obs/plans/c1_pre_tx/plan.side"
  cp "$ELF" "$t/obs/elf/prog"
  echo r > "$t/root/research/cfs_procrace/records/x/f_r.txt"
  echo repo > "$t/root/research/cfs_procrace/env/f_repo.txt"
  echo tool > "$t/abs/tool.bin"
  local bid; bid=$(readelf -n "$t/obs/elf/prog" 2> /dev/null | sed -n 's/.*Build ID: //p' | head -1)
  { echo "# kind	build	name	sha256 or value	path"
    printf 'plan-id\tc1_pre_tx\tplan-id\tabc123\t-\n'
    printf 'plan\tc1_pre_tx\tplan.side\t%s\tplans/c1_pre_tx/plan.side\n' "$(sha256sum < "$t/obs/plans/c1_pre_tx/plan.side" | cut -c1-64)"
    printf 'elf\tc1_pre_tx\tprog\t%s build-id %s\telf/prog\n' "$(sha256sum < "$t/obs/elf/prog" | cut -c1-64)" "$bid"
    printf 'cycle\t*\tf_r.txt\t%s\trecords/x/f_r.txt\n' "$(sha256sum < "$t/root/research/cfs_procrace/records/x/f_r.txt" | cut -c1-64)"
    printf 'frozen\t*\tf_repo\t%s\tresearch/cfs_procrace/env/f_repo.txt\n' "$(sha256sum < "$t/root/research/cfs_procrace/env/f_repo.txt" | cut -c1-64)"
    printf 'tool\t*\ttool.bin\t%s\t%s\n' "$(sha256sum < "$t/abs/tool.bin" | cut -c1-64)" "$t/abs/tool.bin"
  } > "$b/declaration.tsv"
  echo "$b"
}
rc_step() { # TREE STEP: run a step of the tree's run_cycle.sh with the two seams; prints rc
  local t=$WK/$1
  RUN_CYCLE_OBS=$t/obs RUN_CYCLE_BOOT_CMD="echo booted > $t/booted" bash "$t/root/research/cfs_procrace/records/x/boot/run_cycle.sh" "$2" c1_pre_tx 1 \
    > "$t/$2.out" 2>&1; echo $?
}
[ -n "$(readelf -n "$ELF" 2> /dev/null | sed -n 's/.*Build ID: //p')" ]; res "the stand-in ELF has a GNU build-id" $? "$ELF"
# 1 clean
b=$(tree clean); r1=$(rc_step clean verify); r2=$(rc_step clean boot)
ok=0; [ "$r1" = 0 ] && grep -qx 'rows 6 ok 6 differs 0 missing 0' "$b/c1_pre_tx/boot1/verify.txt" && [ "$r2" = 0 ] && [ -f "$WK/clean/booted" ] || ok=1
res "clean declaration: verify rc 0, the boot step runs" $ok "verify rc $r1, boot rc $r2, $(tail -1 "$b/c1_pre_tx/boot1/verify.txt")"
# 2 a changed declared value
b=$(tree differs); Z=$(printf '0%.0s' $(seq 64)); sed -i "s/^\(elf\tc1_pre_tx\tprog\t\)[0-9a-f]\{64\}/\1$Z/" "$b/declaration.tsv"
r1=$(rc_step differs verify); r2=$(rc_step differs boot)
ok=0; [ "$r1" = 1 ] && grep -qP '\telf\t.*\tdiffers$' "$b/c1_pre_tx/boot1/verify.txt" && grep -qx 'rows 6 ok 5 differs 1 missing 0' "$b/c1_pre_tx/boot1/verify.txt" \
  && [ "$r2" = 1 ] && [ ! -e "$WK/differs/booted" ] || ok=1
res "a changed declared value: differs, rc 1; the boot step stops before the boot command" $ok "verify rc $r1, boot rc $r2, $(tail -1 "$b/c1_pre_tx/boot1/verify.txt")"
# 3 a missing file
b=$(tree missing); rm "$WK/missing/abs/tool.bin"
r1=$(rc_step missing verify); r2=$(rc_step missing boot)
ok=0; [ "$r1" = 1 ] && grep -qP '\ttool\t.*\tmissing$' "$b/c1_pre_tx/boot1/verify.txt" && grep -qx 'rows 6 ok 5 differs 0 missing 1' "$b/c1_pre_tx/boot1/verify.txt" \
  && [ "$r2" = 1 ] && [ ! -e "$WK/missing/booted" ] || ok=1
res "a missing file: missing, rc 1; no boot" $ok "verify rc $r1, boot rc $r2, $(tail -1 "$b/c1_pre_tx/boot1/verify.txt")"
# 4 the declaration changed after a clean verify
b=$(tree changed); r1=$(rc_step changed verify); echo "# a later line" >> "$b/declaration.tsv"; r2=$(rc_step changed boot)
ok=0; [ "$r1" = 0 ] && [ "$r2" = 1 ] && [ ! -e "$WK/changed/booted" ] && grep -q 'not booting' "$WK/changed/boot.out" || ok=1
res "the declaration changed after a clean verify: no boot" $ok "verify rc $r1, boot rc $r2"
# 5 no verify.txt
b=$(tree noverify); r2=$(rc_step noverify boot)
ok=0; [ "$r2" = 1 ] && [ ! -e "$WK/noverify/booted" ] || ok=1
res "no verify.txt: no boot" $ok "boot rc $r2"
# 6 a second verify keeps the first under a new name
r3=$(rc_step clean verify)
ok=0; [ "$r3" = 0 ] && [ "$(ls "$WK/clean/root/research/cfs_procrace/records/x/boot/c1_pre_tx/boot1/" | grep -c '^verify\.')" = 2 ] || ok=1
res "a second verify keeps the first output under a new name" $ok "rc $r3"
echo "run_cycle tests: $((NC - NF)) of $NC pass"
exit $((NF > 0))
