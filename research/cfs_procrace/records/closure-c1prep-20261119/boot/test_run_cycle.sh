#!/usr/bin/env bash
# test_run_cycle.sh WORK_DIR -- the tests of the steps of run_cycle.sh (spec 11 item 14; results 26.0 (h); spec 11.5 item 4,
# results 26.5; spec 11.6 item 2, results 26.9).
# Each case copies run_cycle.sh into its own stub tree under WORK_DIR (a research root with records/x/boot/, a copy of
# env/tools/space_guard.sh and env/tools/ledger_check.sh, a stand-in env/obs1/explain_eval.py, a ledger with its header
# line only; a work directory given by the test seam RUN_CYCLE_OBS; a repository path and an absolute path), writes a stub
# declaration (declaration4.tsv) of seven measured rows (a plan-id, a plan file, an ELF with its GNU build-id read by
# readelf -n from a copy of a system executable, a count of the entries under a directory, a research-root file, a
# repository file, an absolute file) and three need rows (floor, margin and per boot, 1,000,000 B each), makes the stub
# root a git repository and commits it, and runs the steps; the boot command is the test seam RUN_CYCLE_BOOT_CMD (by
# default it writes a marker file), the boot step's free space is the test seam RUN_CYCLE_FREE_CMD and the guard's is
# SPACE_GUARD_FREE_CMD, so nothing is booted and the free space of the disk is not read.  Cases (results 26.5): a clean
# declaration; a changed declared value; a missing file; a declaration changed after a clean verify; no verify.txt; a
# second verify keeps the first output under a new name; a declared file changed after a clean verify; a declaration with
# an uncommitted change; a file added under a counted directory; control and gate name the loss records.  Cases (results
# 26.9): run_cycle.sh with an uncommitted change (no boot); a counted directory missing (missing); a symbolic link added
# under a counted directory (differs); control, chain, stop, gate and kzt-text refuse an existing output (rc 2, the output
# unchanged); a boot refused by its own verify and run again keeps the first verify_at_boot.txt; a boot command that ends
# before the instance exists is not booted (the work directory removed, boot.notbooted.* kept, no pin, the same N runs
# again); the space need of the boots remaining (12 before c1_pre_tx 1, 6 before c1_post_tx 1; short: no boot; at the
# need: boot) and a measured per-boot value that is larger than the declared one; the declaration pin (written by the
# first boot; a committed change of the declaration refuses boot 2; c1_post_tx 1 without a pin refuses); the eval output
# under the work directory and only the tail in the record directory, a second eval and a sheet_boot.tsv refused; explain
# verifies again; keep (sha256 lists, ledger rows checked, a directory with no file gets no row, a second keep refused);
# a convert refused at start by the guard leaves no directory and can run again; summary verifies again; a keep whose
# proposed ledger fails its check leaves the ledger unchanged and can run again.
# One line per check and a count; rc 0 when every check passes.  Writes only under WORK_DIR.  (2026-11-19, results 26.4:
# the stub declaration was named declaration2.tsv; results 26.5: declaration3.tsv, the git stub, the count row and the
# four cases after the sixth; results 26.9: declaration4.tsv, the need rows, the stub tools and the cases after the tenth.)
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RS="$(cd "$HERE/../../.." && pwd)"
WK=${1:?usage: test_run_cycle.sh WORK_DIR}
[ ! -e "$WK" ] || { echo "test_run_cycle.sh: $WK exists" >&2; exit 64; }
mkdir -p "$WK" || exit 2
NC=0; NF=0
res() { NC=$((NC + 1)); if [ "$2" = 0 ]; then echo "PASS $1 ($3)"; else echo "FAIL $1 ($3)"; NF=$((NF + 1)); fi; }
ELF=$(readlink -f /usr/bin/true)
G=(env GIT_AUTHOR_NAME=t GIT_AUTHOR_EMAIL=t GIT_COMMITTER_NAME=t GIT_COMMITTER_EMAIL=t GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_NOSYSTEM=1 git)
commit() { # TREE: commit everything of the stub root
  local r=$WK/$1/root
  "${G[@]}" -C "$r" add -A > /dev/null 2>&1 && "${G[@]}" -C "$r" commit -q -m stub > /dev/null 2>&1
}
tree() { # NAME: a stub tree, committed; prints its boot directory
  local t=$WK/$1 b rr
  rr=$t/root/research/cfs_procrace; b=$rr/records/x/boot
  mkdir -p "$b" "$rr/env/tools" "$rr/env/obs1" "$t/obs/plans/c1_pre_tx" "$t/obs/elf" "$t/abs"
  cp "$HERE/run_cycle.sh" "$b/run_cycle.sh"
  cp "$RS/env/tools/space_guard.sh" "$RS/env/tools/ledger_check.sh" "$rr/env/tools/"
  cat > "$rr/env/obs1/explain_eval.py" <<'PYEOF'
import sys
a = sys.argv[1:]
out = a[a.index("--out") + 1]
sb = a[a.index("--sheets-boot") + 1]
open(out, "x").write("kz-eval 1\nscript stand-in (test_run_cycle.sh)\nsheets-boot %s\n"
                     "F1\tboot\tD1-R0\tc1_pre_tx\tA\tk\t0\tpass\tg\tn\n"
                     "F3\tboot\tD1-R0\tc1_pre_tx\tA\tk\t0\tnot_evaluable\tg\tn\n"
                     "F3\tboot\tD2-R0\tc1_pre_tx\tA\tk\t0\tpass\tg\tn\n"
                     "expect_discriminate\tstatic\tD1-R0\tc1_pre_tx\tA\tk\t-\tmet\tg\tn\n"
                     "count\tF1\tboot\tpass\tD1-R0\t1\n"
                     "branch D1-R0 main (declared before the boot: D1/R0): stand-in\n" % sb)
print("eval stand-in wrote", out)
PYEOF
  printf '# Evidence ledger (stub)\nid\tkind\tmethod\tlist\troot\tlocal\tverified_copy\tremote\tpublished\tnote\n' > "$rr/records/LEDGER.tsv"
  echo "plan-id abc123" > "$t/obs/plans/c1_pre_tx/plan.side"
  cp "$ELF" "$t/obs/elf/prog"
  echo r > "$rr/records/x/f_r.txt"
  echo repo > "$rr/env/f_repo.txt"
  echo tool > "$t/abs/tool.bin"
  local bid; bid=$(readelf -n "$t/obs/elf/prog" 2> /dev/null | sed -n 's/.*Build ID: //p' | head -1)
  { echo "# kind	build	name	sha256 or value	path"
    printf 'plan-id\tc1_pre_tx\tplan-id\tabc123\t-\n'
    printf 'plan\tc1_pre_tx\tplan.side\t%s\tplans/c1_pre_tx/plan.side\n' "$(sha256sum < "$t/obs/plans/c1_pre_tx/plan.side" | cut -c1-64)"
    printf 'elf\tc1_pre_tx\tprog\t%s build-id %s\telf/prog\n' "$(sha256sum < "$t/obs/elf/prog" | cut -c1-64)" "$bid"
    printf 'count\tc1_pre_tx\telf files\t1\telf\n'
    printf 'cycle\t*\tf_r.txt\t%s\trecords/x/f_r.txt\n' "$(sha256sum < "$rr/records/x/f_r.txt" | cut -c1-64)"
    printf 'frozen\t*\tf_repo\t%s\tresearch/cfs_procrace/env/f_repo.txt\n' "$(sha256sum < "$rr/env/f_repo.txt" | cut -c1-64)"
    printf 'tool\t*\ttool.bin\t%s\t%s\n' "$(sha256sum < "$t/abs/tool.bin" | cut -c1-64)" "$t/abs/tool.bin"
    printf 'need\t*\tfloor\t1000000\t-\nneed\t*\tmargin\t1000000\t-\nneed\t*\tper boot\t1000000\t-\n'
  } > "$b/declaration4.tsv"
  "${G[@]}" -C "$t/root" init -q > /dev/null 2>&1
  commit "$1"
  echo "$b"
}
rc_step() { # TREE STEP [BUILD [N]]: run a step of the tree's run_cycle.sh with the seams (RS_BOOT, RS_FREE, RS_GFREE override); prints rc
  local t=$WK/$1
  RUN_CYCLE_OBS=$t/obs RUN_CYCLE_BOOT_CMD="${RS_BOOT:-echo booted >> $t/booted}" RUN_CYCLE_FREE_CMD="${RS_FREE:-echo 100000000000}" \
    SPACE_GUARD_FREE_CMD="${RS_GFREE:-echo 100000000000}" \
    bash "$t/root/research/cfs_procrace/records/x/boot/run_cycle.sh" "$2" "${3:-c1_pre_tx}" "${4:-1}" > "$t/$2.out" 2>&1; echo $?
}
shaf() { sha256sum < "$1" 2> /dev/null | cut -c1-64; }
[ -n "$(readelf -n "$ELF" 2> /dev/null | sed -n 's/.*Build ID: //p')" ]; res "the stand-in ELF has a GNU build-id" $? "$ELF"
# 1 clean
b=$(tree clean); r1=$(rc_step clean verify); r2=$(rc_step clean boot)
ok=0; [ "$r1" = 0 ] && grep -qx 'rows 7 ok 7 differs 0 missing 0' "$b/c1_pre_tx/boot1/verify.txt" && [ "$r2" = 0 ] && [ -f "$WK/clean/booted" ] \
  && grep -qx 'rows 7 ok 7 differs 0 missing 0' "$b/c1_pre_tx/boot1/verify_at_boot.txt" && grep -q '^need rows .* floor=1000000;' "$b/c1_pre_tx/boot1/verify.txt" || ok=1
res "clean declaration: verify rc 0 (need rows not measured), the boot step verifies again and runs" $ok "verify rc $r1, boot rc $r2, $(tail -1 "$b/c1_pre_tx/boot1/verify.txt")"
# 2 a changed declared value
b=$(tree differs); Z=$(printf '0%.0s' $(seq 64)); sed -i "s/^\(elf\tc1_pre_tx\tprog\t\)[0-9a-f]\{64\}/\1$Z/" "$b/declaration4.tsv"; commit differs
r1=$(rc_step differs verify); r2=$(rc_step differs boot)
ok=0; [ "$r1" = 1 ] && grep -qP '\telf\t.*\tdiffers$' "$b/c1_pre_tx/boot1/verify.txt" && grep -qx 'rows 7 ok 6 differs 1 missing 0' "$b/c1_pre_tx/boot1/verify.txt" \
  && [ "$r2" = 1 ] && [ ! -e "$WK/differs/booted" ] || ok=1
res "a changed declared value: differs, rc 1; the boot step stops before the boot command" $ok "verify rc $r1, boot rc $r2, $(tail -1 "$b/c1_pre_tx/boot1/verify.txt")"
# 3 a missing file
b=$(tree missing); rm "$WK/missing/abs/tool.bin"
r1=$(rc_step missing verify); r2=$(rc_step missing boot)
ok=0; [ "$r1" = 1 ] && grep -qP '\ttool\t.*\tmissing$' "$b/c1_pre_tx/boot1/verify.txt" && grep -qx 'rows 7 ok 6 differs 0 missing 1' "$b/c1_pre_tx/boot1/verify.txt" \
  && [ "$r2" = 1 ] && [ ! -e "$WK/missing/booted" ] || ok=1
res "a missing file: missing, rc 1; no boot" $ok "verify rc $r1, boot rc $r2, $(tail -1 "$b/c1_pre_tx/boot1/verify.txt")"
# 4 the declaration changed after a clean verify
b=$(tree changed); r1=$(rc_step changed verify); echo "# a later line" >> "$b/declaration4.tsv"; r2=$(rc_step changed boot)
ok=0; [ "$r1" = 0 ] && [ "$r2" = 1 ] && [ ! -e "$WK/changed/booted" ] && grep -q 'not booting' "$WK/changed/boot.out" || ok=1
res "the declaration changed after a clean verify: no boot" $ok "verify rc $r1, boot rc $r2"
# 5 no verify.txt
b=$(tree noverify); r2=$(rc_step noverify boot)
ok=0; [ "$r2" = 1 ] && [ ! -e "$WK/noverify/booted" ] || ok=1
res "no verify.txt: no boot" $ok "boot rc $r2"
# 6 a second verify keeps the first under a new name
b=$(tree second); r1=$(rc_step second verify); r3=$(rc_step second verify)
ok=0; [ "$r1" = 0 ] && [ "$r3" = 0 ] && [ "$(ls "$b/c1_pre_tx/boot1/" | grep -c '^verify\.')" = 2 ] || ok=1
res "a second verify keeps the first output under a new name" $ok "rc $r1 $r3"
# 7 a declared file changed after a clean verify (results 26.5): the boot step's own verify finds it
b=$(tree drift); r1=$(rc_step drift verify); echo later >> "$WK/drift/obs/plans/c1_pre_tx/plan.side"; r2=$(rc_step drift boot)
ok=0; [ "$r1" = 0 ] && [ "$r2" = 1 ] && [ ! -e "$WK/drift/booted" ] && grep -q 'verify step run again before the boot is not clean' "$WK/drift/boot.out" \
  && grep -qx 'rows 7 ok 6 differs 1 missing 0' "$b/c1_pre_tx/boot1/verify.txt" || ok=1
res "a declared file changed after a clean verify: the boot step verifies again and does not boot" $ok "verify rc $r1, boot rc $r2, $(tail -1 "$b/c1_pre_tx/boot1/verify.txt")"
# 8 an uncommitted change of the declaration, verified clean (results 26.5): no boot
b=$(tree uncommitted); echo "# an uncommitted line" >> "$b/declaration4.tsv"; r1=$(rc_step uncommitted verify); r2=$(rc_step uncommitted boot)
ok=0; [ "$r1" = 0 ] && [ "$r2" = 1 ] && [ ! -e "$WK/uncommitted/booted" ] && grep -q 'not committed or has uncommitted changes' "$WK/uncommitted/boot.out" || ok=1
res "a declaration with an uncommitted change: no boot" $ok "verify rc $r1, boot rc $r2"
# 9 a file added under a counted directory (results 26.5)
b=$(tree counted); echo x > "$WK/counted/obs/elf/added"; r1=$(rc_step counted verify)
ok=0; [ "$r1" = 1 ] && grep -qP '\tcount\telf\t1\t2\tdiffers$' "$b/c1_pre_tx/boot1/verify.txt" || ok=1
res "a file added under a counted directory: differs" $ok "verify rc $r1, $(grep -P '\tcount\t' "$b/c1_pre_tx/boot1/verify.txt")"
# 10 control and gate name the loss records by kind and count (results 26.5; evaluation protocol 3 item 2)
b=$(tree losses); T=$WK/losses/obs/c1cyc_pre_boot1/tx; mkdir -p "$T" "$b/c1_pre_tx/boot1"
printf 'run abi=1\nloss no-build-id actor=- key=9 count=1\nloss no-build-id actor=- key=3 count=2\nloss after-stop actor=2 key=0 count=4\ncheck ok (0 failures)\n' > "$T/tx.dump.txt"
printf 'chain\tconfirmed\t3 links\n' > "$b/c1_pre_tx/boot1/chain.txt"
r1=$(rc_step losses control); r2=$(rc_step losses gate)
ok=0; [ "$r1" = 0 ] && [ "$r2" = 0 ] && grep -qx 'gate PASS' "$b/c1_pre_tx/boot1/gate.txt" \
  && grep -qx 'losses: fields none; records after-stop lines=1 count=4;no-build-id lines=2 count=3;' "$b/c1_pre_tx/boot1/gate.txt" || ok=1
res "control and gate name every loss record by kind and count" $ok "control rc $r1, gate rc $r2, $(sed -n 2p "$b/c1_pre_tx/boot1/gate.txt")"
# 11 run_cycle.sh with an uncommitted change (results 26.9, fix-judge-05): no boot
b=$(tree scriptdirty); r1=$(rc_step scriptdirty verify); echo "# an uncommitted line" >> "$b/run_cycle.sh"; r2=$(rc_step scriptdirty boot)
ok=0; [ "$r1" = 0 ] && [ "$r2" = 1 ] && [ ! -e "$WK/scriptdirty/booted" ] && grep -q 'not committed or has uncommitted changes' "$WK/scriptdirty/boot.out" || ok=1
res "run_cycle.sh with an uncommitted change: no boot" $ok "verify rc $r1, boot rc $r2"
# 12 a counted directory missing (results 26.9, fix-judge-05): missing
b=$(tree countgone); rm -r "$WK/countgone/obs/elf"; r1=$(rc_step countgone verify)
ok=0; [ "$r1" = 1 ] && grep -qP '\tcount\telf\t1\tmissing\tmissing$' "$b/c1_pre_tx/boot1/verify.txt" || ok=1
res "a counted directory missing: missing, rc 1" $ok "verify rc $r1, $(grep -P '\tcount\t' "$b/c1_pre_tx/boot1/verify.txt")"
# 13 a symbolic link added under a counted directory (results 26.9, fix-decl-11): differs
b=$(tree countlink); ln -s /etc/hostname "$WK/countlink/obs/elf/link"; r1=$(rc_step countlink verify)
ok=0; [ "$r1" = 1 ] && grep -qP '\tcount\telf\t1\t2\tdiffers$' "$b/c1_pre_tx/boot1/verify.txt" || ok=1
res "a symbolic link added under a counted directory: differs" $ok "verify rc $r1, $(grep -P '\tcount\t' "$b/c1_pre_tx/boot1/verify.txt")"
# 14 control, gate, chain, stop and kzt-text write once (results 26.9, fix-decl-05)
b=$(tree once); T=$WK/once/obs/c1cyc_pre_boot1/tx; L1=$b/c1_pre_tx/boot1; mkdir -p "$T" "$L1" "$WK/once/obs/c1cyc_pre_convert1"
printf 'run abi=1\ncheck ok (0 failures)\n' > "$T/tx.dump.txt"; printf 'chain\trefused\t0 links\n' > "$L1/chain.txt"
r1=$(rc_step once control); s1=$(shaf "$L1/control_check.txt"); r2=$(rc_step once gate); s2=$(shaf "$L1/gate.txt")
printf 'run abi=1\ncheck FAILED (1 failures)\n' > "$T/tx.dump.txt"; printf 'chain\tconfirmed\t3 links\n' > "$L1/chain.txt"
r3=$(rc_step once control); r4=$(rc_step once gate); r5=$(rc_step once chain); echo stop > "$L1/stop_eval.tsv"; r6=$(rc_step once stop)
echo t > "$WK/once/obs/c1cyc_pre_convert1/run.kzt.txt"; r7=$(rc_step once kzt-text)
ok=0; [ "$r1" = 0 ] && [ "$r2" = 0 ] && [ "$r3" = 2 ] && [ "$r4" = 2 ] && [ "$r5" = 2 ] && [ "$r6" = 2 ] && [ "$r7" = 2 ] \
  && [ "$(shaf "$L1/control_check.txt")" = "$s1" ] && [ "$(shaf "$L1/gate.txt")" = "$s2" ] && grep -q '^gate FAIL' "$L1/gate.txt" \
  && [ "$(ls "$L1" | grep -c '^gate')" = 1 ] && [ "$(cat "$L1/stop_eval.tsv")" = stop ] && [ ! -e "$L1/convert.sha256" ] || ok=1
res "a second control, gate, chain, stop, kzt-text: rc 2, the first output unchanged (a FAIL gate stays FAIL)" $ok "rc $r1 $r2 then $r3 $r4 $r5 $r6 $r7, $(head -1 "$L1/gate.txt")"
# 15 a boot refused by its own verify, then run again, keeps the first verify_at_boot.txt (results 26.9, fix-judge-06)
b=$(tree reboot); r1=$(rc_step reboot verify); echo later >> "$WK/reboot/obs/plans/c1_pre_tx/plan.side"; r2=$(rc_step reboot boot)
printf 'plan-id abc123\n' > "$WK/reboot/obs/plans/c1_pre_tx/plan.side"; r3=$(rc_step reboot verify); r4=$(rc_step reboot boot)
nv=$(ls "$b/c1_pre_tx/boot1/" | grep -c '^verify_at_boot\.')
ok=0; [ "$r1" = 0 ] && [ "$r2" = 1 ] && [ "$r3" = 0 ] && [ "$r4" = 0 ] && [ "$nv" = 2 ] && [ -f "$WK/reboot/booted" ] \
  && grep -q 'differs 1' "$(ls "$b"/c1_pre_tx/boot1/verify_at_boot.2*.txt | head -1)" || ok=1
res "a boot refused by its own verify and run again: two verify_at_boot records, the first kept" $ok "rc $r1 $r2 $r3 $r4, files $nv"
# 16 a boot command that ends before the instance exists: not booted (results 26.9, fix-decl-12)
b=$(tree notbooted); r1=$(rc_step notbooted verify); r2=$(RS_BOOT='echo refused; exit 3' rc_step notbooted boot)
nb=$(ls "$b/c1_pre_tx/boot1/" | grep -c '^boot\.notbooted\.'); nm=$(grep -c 'not booted' "$WK/notbooted/boot.out"); g1=0; [ ! -e "$WK/notbooted/obs/c1cyc_pre_boot1" ] && [ ! -e "$b/c1_pre_tx/boot1/boot.txt" ] && [ ! -e "$b/declaration_pin.txt" ] || g1=1
r3=$(rc_step notbooted boot)
ok=0; [ "$r1" = 0 ] && [ "$r2" = 3 ] && [ "$nb" = 1 ] && [ "$g1" = 0 ] && [ "$nm" = 1 ] && [ "$r3" = 0 ] && [ -f "$WK/notbooted/booted" ] \
  && [ -f "$b/declaration_pin.txt" ] && [ -f "$b/c1_pre_tx/boot1/boot.txt" ] || ok=1
res "not booted: work directory removed, boot.notbooted kept, no pin; the same N runs again" $ok "rc $r1 $r2 then $r3, notbooted files $nb"
# 17 the space need of the boots remaining (results 26.9, fix-decl-01, -02, -03)
b=$(tree space); L1=$b/c1_pre_tx/boot1; r1=$(rc_step space verify); r2=$(RS_FREE='echo 13999999' rc_step space boot); g1=$([ ! -e "$WK/space/obs/c1cyc_pre_boot1" ] && [ ! -e "$WK/space/booted" ] && echo 0 || echo 1)
q1=$(grep -c -e '^need 14000000$' -e '^verdict short by 1$' -e 'this one included: 12$' "$L1/space_at_boot.txt")
r3=$(RS_FREE='echo 14000000' rc_step space boot)
L2=$b/c1_post_tx/boot1; r4=$(rc_step space verify c1_post_tx 1); r5=$(RS_FREE='echo 7999999' rc_step space boot c1_post_tx 1)
q2=$(grep -c -e '^need 8000000$' -e 'this one included: 6$' "$L2/space_at_boot.txt"); r6=$(RS_FREE='echo 8000000' rc_step space boot c1_post_tx 1)
ok=0; [ "$r1" = 0 ] && [ "$r2" = 1 ] && [ "$g1" = 0 ] && [ "$q1" = 3 ] && [ "$r3" = 0 ] && [ "$(ls "$L1" | grep -c '^space_at_boot')" = 2 ] \
  && [ "$r4" = 0 ] && [ "$r5" = 1 ] && [ "$q2" = 2 ] && [ "$r6" = 0 ] && [ "$(grep -c . "$WK/space/booted")" = 2 ] || ok=1
res "space: c1_pre_tx 1 needs floor + margin + 12 x per boot, c1_post_tx 1 6 x; below: no boot; at the need: boot" $ok "rc $r1 $r2 $r3, post $r4 $r5 $r6, lines $q1 $q2"
# 18 a measured per-boot value larger than the declared one is used (results 26.9, protocol 14.14 item 4)
b=$(tree measured); r1=$(rc_step measured verify); r2=$(rc_step measured boot); head -c 3000000 /dev/zero > "$WK/measured/obs/c1cyc_pre_boot1/big"
echo "gate PASS" > "$b/c1_pre_tx/boot1/gate.txt"; r3=$(rc_step measured verify c1_pre_tx 2); r4=$(rc_step measured boot c1_pre_tx 2)
m=$(sed -n 's/^measured per boot (the largest over the boots already run): \([0-9]*\) (c1_pre_tx\/boot1)$/\1/p' "$b/c1_pre_tx/boot2/space_at_boot.txt")
u=$(sed -n 's/^per boot used \([0-9]*\);.*/\1/p' "$b/c1_pre_tx/boot2/space_at_boot.txt")
ok=0; [ "$r1" = 0 ] && [ "$r2" = 0 ] && [ "$r3" = 0 ] && [ "$r4" = 0 ] && [ -n "$m" ] && [ "$m" -gt 3000000 ] && [ "$u" = "$m" ] \
  && grep -qx "need $((2000000 + 10 * m))" "$b/c1_pre_tx/boot2/space_at_boot.txt" || ok=1
res "a measured per-boot value larger than the declared one: used for the boots after it" $ok "rc $r1 $r2 $r3 $r4, measured ${m:-none} used ${u:-none}"
# 19 the declaration pin (results 26.9, fix-decl-09)
b=$(tree pin); r1=$(rc_step pin verify); r2=$(rc_step pin boot); p1=$(sed -n 's/^declaration sha256=\([0-9a-f]*\) .*/\1/p' "$b/declaration_pin.txt")
echo "gate PASS" > "$b/c1_pre_tx/boot1/gate.txt"; echo "# a committed later line" >> "$b/declaration4.tsv"; commit pin
r3=$(rc_step pin verify c1_pre_tx 2); r4=$(rc_step pin boot c1_pre_tx 2)
b2=$(tree nopin); r5=$(rc_step nopin verify c1_post_tx 1); r6=$(rc_step nopin boot c1_post_tx 1)
ok=0; [ "$r1" = 0 ] && [ "$r2" = 0 ] && [ -n "$p1" ] && [ "$r3" = 0 ] && [ "$r4" = 1 ] && grep -q 'is not the one boot/declaration_pin.txt names' "$WK/pin/boot.out" && [ ! -e "$WK/pin/obs/c1cyc_pre_boot2" ] \
  && [ "$r5" = 0 ] && [ "$r6" = 1 ] && grep -q 'no boot/declaration_pin.txt' "$WK/nopin/boot.out" && [ ! -e "$WK/nopin/booted" ] || ok=1
res "the pin: written by the first boot; a committed declaration change refuses boot 2; c1_post_tx 1 with no pin refuses" $ok "rc $r1 $r2 $r3 $r4, nopin $r5 $r6"
# 20 the eval output under the work directory, the tail in the record directory (results 26.9, fix-judge-01, fix-decl-08, fix-decl-10)
b=$(tree eval); L1=$b/c1_pre_tx/boot1; r1=$(rc_step eval verify); r2=$(rc_step eval boot); r3=$(rc_step eval eval); E=$WK/eval/obs/c1cyc_pre_eval1/eval.txt; s1=$(shaf "$E")
r4=$(rc_step eval eval); mkdir -p "$b/c1_pre_tx/boot2"; echo sheet > "$b/c1_pre_tx/boot2/sheet_boot.tsv"; r5=$(rc_step eval eval c1_pre_tx 2)
ok=0; [ "$r1" = 0 ] && [ "$r2" = 0 ] && [ "$r3" = 0 ] && [ -f "$E" ] && [ ! -e "$L1/eval.txt" ] && grep -q '^== .*eval1/eval.txt (lines 9, sha256 ' "$L1/eval_tail.txt" \
  && grep -qx 'sheets-boot none' "$L1/eval_tail.txt" && grep -qP '^expect_discriminate\t' "$L1/eval_tail.txt" && ! grep -qP '^F1\t' "$L1/eval_tail.txt" \
  && [ "$(grep -cP '^F3\tboot\tD1-R0\t' "$L1/eval_tail.txt")" = 1 ] && ! grep -qP '^F3\tboot\tD2-R0\t' "$L1/eval_tail.txt" \
  && [ "$r4" = 2 ] && [ "$(shaf "$E")" = "$s1" ] && [ "$r5" = 2 ] && [ ! -e "$WK/eval/obs/c1cyc_pre_eval2" ] || ok=1
res "eval: full output under the work directory, head and main tail in the record directory; a second eval and a sheet_boot.tsv refused" $ok "rc $r1 $r2 $r3, again $r4, sheet $r5"
# 21 explain verifies again (results 26.9, fix-decl-10)
b=$(tree expl); r1=$(rc_step expl verify); r2=$(rc_step expl boot); echo later >> "$WK/expl/obs/plans/c1_pre_tx/plan.side"; r3=$(rc_step expl explain)
ok=0; [ "$r1" = 0 ] && [ "$r2" = 0 ] && [ "$r3" = 1 ] && [ ! -e "$WK/expl/obs/c1cyc_pre_explain1" ] && grep -q 'before explain is not clean' "$WK/expl/explain.out" \
  && grep -q 'differs 1' "$b/c1_pre_tx/boot1/verify_at_explain.txt" || ok=1
res "explain: a declared file changed after the boot: the verify again is not clean, no explanation output" $ok "rc $r1 $r2 $r3"
# 22 keep: sha256 lists and ledger rows checked before the ledger changes (results 26.9, fix-judge-01, fix-decl-08)
b=$(tree keep); rr=$WK/keep/root/research/cfs_procrace; r1=$(rc_step keep verify); r2=$(rc_step keep boot)
mkdir -p "$WK/keep/obs/c1cyc_pre_boot1/tx" "$WK/keep/obs/c1cyc_pre_eval1" "$WK/keep/obs/c1cyc_pre_convert1"
echo d > "$WK/keep/obs/c1cyc_pre_boot1/tx/tx.dump.txt"; echo e > "$WK/keep/obs/c1cyc_pre_eval1/eval.txt"
r3=$(rc_step keep keep); s1=$(shaf "$rr/records/LEDGER.tsv"); lc=0; bash "$rr/env/tools/ledger_check.sh" > "$WK/keep/lc.out" 2>&1 || lc=1
r4=$(rc_step keep keep)
ok=0; [ "$r1" = 0 ] && [ "$r2" = 0 ] && [ "$r3" = 0 ] && [ "$lc" = 0 ] && grep -qx 'ledger_check: 2 rows, 0 failing' "$WK/keep/lc.out" \
  && [ "$(grep -cP '^OBS_c1cyc_pre_(boot|eval)1\toutput\tsha256c\trecords/x/boot/c1_pre_tx/boot1/c1cyc_pre_(boot|eval)1\.sha256\t' "$rr/records/LEDGER.tsv")" = 2 ] \
  && grep -q '^convert .*holds no file, no row$' "$b/c1_pre_tx/boot1/keep.txt" && grep -q '^explain .*absent, no row$' "$b/c1_pre_tx/boot1/keep.txt" \
  && [ "$r4" = 2 ] && [ "$(shaf "$rr/records/LEDGER.tsv")" = "$s1" ] && [ -z "$(ls "$WK/keep/obs" | grep proposed)" ] || ok=1
res "keep: lists and ledger rows (checked before the ledger changes), no row for an empty or absent directory, a second keep refused" $ok "rc $r1 $r2 $r3, ledger check $(tail -1 "$WK/keep/lc.out"), again $r4"
# 23 a convert refused at start by the guard leaves no directory and can run again (results 26.9, protocol 14.14 item 3)
b=$(tree conv); r1=$(rc_step conv verify); r2=$(RS_GFREE='echo 1' rc_step conv convert); r3=$(RS_GFREE='echo 1' rc_step conv convert)
ok=0; [ "$r1" = 0 ] && [ "$r2" = 75 ] && [ "$r3" = 75 ] && [ ! -e "$WK/conv/obs/c1cyc_pre_convert1" ] && [ "$(ls "$b/c1_pre_tx/boot1" | grep -c '^convert\.refused\.')" = 2 ] || ok=1
res "a convert refused at start by the space guard: no directory left, the refused log kept, the step can run again" $ok "rc $r1 $r2 $r3"
# 24 summary verifies again (results 26.9, fix-decl-10)
b=$(tree summ); r1=$(rc_step summ verify); r2=$(rc_step summ boot); r3=$(rc_step summ summary)
mkdir -p "$b/c1_pre_tx/boot5"; echo "gate PASS" > "$b/c1_pre_tx/boot5/gate.txt"; echo later >> "$WK/summ/obs/plans/c1_pre_tx/plan.side"; r4=$(rc_step summ summary)
ok=0; [ "$r1" = 0 ] && [ "$r2" = 0 ] && [ "$r3" = 2 ] && [ "$r4" = 1 ] && grep -q 'before summary is not clean' "$WK/summ/summary.out" \
  && grep -q 'differs 1' "$b/c1_pre_tx/verify_at_summary.txt" && [ ! -e "$b/c1_pre_tx/summary.txt" ] || ok=1
res "summary: refused before boot5/gate.txt; a declared file changed: the verify again is not clean, no summary" $ok "rc $r1 $r2 $r3 $r4"
# 25 keep with a proposed ledger that fails its check: the ledger is not changed, the attempt is kept, keep runs again (results 26.9)
b=$(tree keepbad); rr=$WK/keepbad/root/research/cfs_procrace; r1=$(rc_step keepbad verify); r2=$(rc_step keepbad boot)
mkdir -p "$WK/keepbad/obs/c1cyc_pre_boot1/tx"; echo d > "$WK/keepbad/obs/c1cyc_pre_boot1/tx/tx.dump.txt"
cp "$rr/records/LEDGER.tsv" "$WK/keepbad/ledger.orig"; printf 'BAD\toutput\tsha256c\trecords/none.sha256\t/nonexistent\tyes\tnone\tnone\tno\tbad\n' >> "$rr/records/LEDGER.tsv"
s0=$(shaf "$rr/records/LEDGER.tsv"); r3=$(rc_step keepbad keep); s1=$(shaf "$rr/records/LEDGER.tsv"); L1=$b/c1_pre_tx/boot1
nf=$(ls "$L1" | grep -c '\.failed\.'); g1=0; [ ! -e "$L1/c1cyc_pre_boot1.sha256" ] && [ ! -e "$L1/keep.txt" ] || g1=1
cp "$WK/keepbad/ledger.orig" "$rr/records/LEDGER.tsv"; r4=$(rc_step keepbad keep)
ok=0; [ "$r1" = 0 ] && [ "$r2" = 0 ] && [ "$r3" = 1 ] && [ "$s1" = "$s0" ] && [ "$nf" = 3 ] && [ "$g1" = 0 ] && [ "$r4" = 0 ] \
  && [ "$(grep -c '^OBS_c1cyc_pre_boot1' "$rr/records/LEDGER.tsv")" = 1 ] || ok=1
res "keep with a failing proposed ledger: rc 1, the ledger unchanged, the attempt kept as *.failed.*; keep runs again" $ok "rc $r1 $r2 $r3, failed files $nf, again $r4"
echo "run_cycle tests: $((NC - NF)) of $NC pass"
exit $((NF > 0))
