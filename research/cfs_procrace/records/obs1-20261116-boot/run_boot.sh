#!/usr/bin/env bash
# run_boot.sh STEP -- the boot of the derived C1 build B' made from the generated plan of c1_pre_tx, its conversion
# and its chain (plan/observation_path_results_20261116.md 6.0).  Logs and small records into this directory.
#   boot | control | rtdump-mc | convert | kzt-text | chain | stop | conditions
# One natural boot only: "boot" refuses to run when its run directory exists (run_gi_c1.sh:35 refuses as well), and a
# second boot is never a replacement of the first record.  Every output directory is new.
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
R="$(cd "$HERE/../.." && pwd)"
GS=$R/kairos/instrument/gcc/scripts
GUARD=$R/env/tools/space_guard.sh
NEED=50000000
O=/home/user/work/redesign/obs1
P=$O/plangen/c1_pre_tx
W=$O/c1pre_derived
RT=$O/c1pre_rt/rt
BP=$O/c1pre_bprime/c1_pre_tx/NP
RUN=$O/c1pre_boot/tx
CV=$O/c1pre_convert
MC=/home/user/work/merge-check/kz-llvm
T8=/home/user/work/redesign/step2a/tests_8c_fix/c1_pre_tx/NP
W6=/home/user/work/w6/run/c1nat/gi/pre_1df262a_tx
g() { local k=$1; shift; "$GUARD" --need "$NEED" --kind "$k" --log "$HERE/df_$k.log" -- "$@"; }
case "${1:-}" in
  boot)
    [ ! -e "$O/c1pre_boot" ] || { echo "run_boot.sh: $O/c1pre_boot exists; one boot only" >&2; exit 2; }
    mkdir "$O/c1pre_boot" || exit 2
    # the two-instance reference procedure (tx recorded, rx KZ_RT_MODE=off); the runtime tree's own kz-rt-dump, as the
    # earlier G-I natural boots passed theirs (env/campaigns/w6_20261029.sh:51)
    g boot bash "$GS/run_gi_c1.sh" "$W" pre 1df262a tx "$RUN" "$RT/kz-rt-dump" "$P/plan.kzs" > "$HERE/boot.txt" 2>&1 ;;
  control)
    # observation only: no control record (ctl, plan, point lines of kz-rt-dump), the window mode and the mode source
    D=$RUN/tx.dump.txt
    { echo "dump text $D sha256 $(sha256sum < "$D" | cut -c1-64) lines $(wc -l < "$D")"
      echo "record kinds (first word, count):"; awk '{print $1}' "$D" | sort | uniq -c | sed 's/^/  /'
      echo "control records (ctl, plan, point): $(grep -cE '^(ctl|plan|point) ' "$D")"
      grep -E '^run abi|^runx window_mode|^runx pool_bytes|^runx end|^caps' "$D" | sed 's/^/  /'
      echo "check lines that do not end in ok/pass (for the record): $(grep -E '^check ' "$D" | grep -cvE ' (ok|pass)$')"
    } > "$HERE/control_check.txt"; cat "$HERE/control_check.txt" ;;
  rtdump-mc)
    # the registry's kz-rt-dump (MC 4390d4f1) on the same dump and manifests, its text against the harness's
    [ ! -e "$CV" ] || { echo "run_boot.sh: $CV exists" >&2; exit 2; }
    mkdir "$CV" || exit 2
    "$MC/runtime/kz-rt-dump" --manifest "$P/plan.kzs" --manifest "$W/binds_instr.kzs" "$RUN/dump/tx.kzrt" > "$CV/tx.dump.mc.txt" 2>&1; rc=$?
    { echo "MC kz-rt-dump $(sha256sum < "$MC/runtime/kz-rt-dump" | cut -c1-64) rc $rc"
      echo "runtime-tree kz-rt-dump $(sha256sum < "$RT/kz-rt-dump" | cut -c1-64) (the harness's; rc in run.meta)"
      cmp -s "$CV/tx.dump.mc.txt" "$RUN/tx.dump.txt" && echo "texts: byte-identical" || { echo "texts: DIFFER"; diff "$RUN/tx.dump.txt" "$CV/tx.dump.mc.txt" | head -40; }
    } > "$HERE/rtdump_compare.txt"; cat "$HERE/rtdump_compare.txt" ;;
  convert)
    g convert "$MC/tools/kz/kz" convert --kzrt "$RUN/dump/tx.kzrt" --manifest "$P/plan.kzs" --manifest "$W/binds_instr.kzs" \
      --basis "$BP/basis.kz" --out "$CV/run.kzt" > "$CV/convert.log" 2>&1; rc=$?
    echo "convert rc=$rc" >> "$CV/convert.log"; cp "$CV/convert.log" "$HERE/convert.log"; cat "$HERE/convert.log"; exit $rc ;;
  kzt-text)
    "$MC/tools/kz-dump/kz-dump" "$CV/run.kzt" > "$CV/run.kzt.txt" 2>&1; echo "kz-dump run.kzt rc $?"
    "$MC/tools/kz-dump/kz-dump" "$BP/facts.kz" 2>&1 | head -8 > "$CV/facts_header.txt"
    (cd "$CV" && sha256sum run.kzt run.kzt.txt facts_header.txt tx.dump.mc.txt convert.log) > "$HERE/convert.sha256"; cat "$HERE/convert.sha256" ;;
  chain)
    L=$(sed -n 's/^instance_files .* list=\([^ ]*\) .*/\1/p' "$RUN/run.meta")
    python3 -I "$R/env/tools/chain_check.py" --plan "$P/plan.kzs" --build-record "$W/build_instr.record" --run-meta "$RUN/run.meta" \
      --build-root "$W/ws" --dump-text "$RUN/tx.dump.txt" --basis-text "$BP/basis.txt" --main exe/tx/core-tx \
      --sidecar "$P/plan.side" --plan-facts "$T8/facts.kz" --plan-basis "$T8/basis.kz" --inst-list "$L" --inst-tree "$W/ws/exe" \
      --dump "$RUN/dump/tx.kzrt" --bindings "$W/binds_instr.kzs" --bprime-basis "$BP/basis.kz" --facts-header "$CV/facts_header.txt" \
      --kzt-text "$CV/run.kzt.txt" -o "$HERE/chain.txt" ;;
  stop)
    python3 -I "$R/env/obs1/stop_eval.py" --manifest "$P/plan.kzs" --sidecar "$P/plan.side" \
      --startup-script "$RUN/inst/tx/cf/cfe_es_startup.scr" --dump-text "$RUN/tx.dump.txt" -o "$HERE/stop_eval.tsv" ;;
  conditions)
    # the harness's conditions against the earlier G-I natural boot of the same configuration (declared: build path lines)
    { echo "conditions.txt against $W6/conditions.txt:"; diff "$W6/conditions.txt" "$RUN/conditions.txt" | sed 's/^/  /'
      echo "run.meta kernel/nproc/run_user/msg_max line equal to the earlier boot's: $( [ "$(sed -n '2p' "$W6/run.meta")" = "$(sed -n '2p' "$RUN/run.meta")" ] && echo yes || echo NO)"
      echo "run.meta second line fields that differ (names only):"
      diff <(sed -n '2p' "$W6/run.meta" | tr ' ' '\n' | sed 's/=.*//') <(sed -n '2p' "$RUN/run.meta" | tr ' ' '\n' | sed 's/=.*//') > /dev/null && \
        for k in kernel nproc run_user mqueue.msg_max; do a=$(sed -n '2p' "$W6/run.meta" | grep -o "$k=[^ ]*"); b=$(sed -n '2p' "$RUN/run.meta" | grep -o "$k=[^ ]*"); [ "$a" = "$b" ] && echo "  $k same" || echo "  $k differs"; done
    } > "$HERE/conditions_compare.txt"; cat "$HERE/conditions_compare.txt" ;;
  *) echo "usage: run_boot.sh boot|control|rtdump-mc|convert|kzt-text|chain|stop|conditions" >&2; exit 64 ;;
esac
