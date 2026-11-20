#!/usr/bin/env bash
# run.sh STEP -- the runs of the five reference builds after the kz-static build of the second group of review fixes
# (plan/full_stage_results_20261116.md 12.0 (b), section 12.11), each into a new directory under OROOT, one step at a time:
#   relcheck      relations --keys on the tests_8c_fix NP facts                         -> rf2_relcheck/<b>
#   order         order --edges on the same facts, run twice (again/)                   -> rf2_order/<b>
#   extract_off   env/redesign/one_build.sh without an extraction option               -> rf2_off_extract/<b>
#   extract_final one_build.sh with every extraction option of the stage               -> rf2_final_extract/<b>
#                 (--object-access, --semantics with the seven framework semantics data files and the P0
#                 declarations file listed below, --result-flow, --loop-defs, --lock-handles: the arguments of the
#                 kept final round, ll_final_extract/<b>/<b>/NP/args.txt)
#   off_order     order --edges twice and relations --keys on rf2_off_extract's facts   -> rf2_off_order/<b>
#   final_order   order --edges --caller-dependent-report twice and relations --keys on rf2_final_extract's facts
#                                                                                       -> rf2_final_order/<b>
#   plans_t8cf    the plan without --manifest (plan_one.sh) on the tests_8c_fix facts   -> rf2_plans_t8cf/<b>
#   plans_final   the same on rf2_final_extract's facts                                 -> rf2_plans_final/<b>
#   plans_v4      the plan with --manifest 4 and the control budget all (plan_one.sh v4) on rf2_final_extract's
#                 facts                                                                 -> rf2_plans_v4/<b>
#   plans_v4_all  as plans_v4 with the tier budget all (a measurement of the rules)     -> rf2_plans_v4_all/<b>
#   plans_v4_kept the plan with --manifest 4 on the KEPT final round ll_final_extract (the facts of the earlier tool):
#                 the generator's change apart from the extraction's                    -> rf2_plans_v4_keptfacts/<b>
# The data files are named one by one (no file-name pattern: other files may appear in kairos/semantics while this
# step runs) and checked against data_inputs.sha256 before every step.  Environment: KZB the tool directory (default
# the MC tree's tools), OROOT the output root (default /home/user/work/redesign/obs1).  rc 0 when every build's run
# returned 0; a failing build stops nothing else and gives rc 1; rc 2 when a step could not start.
set -uo pipefail
REPO=/home/user/Shape-Perf/research/cfs_procrace
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KZB=${KZB:-/home/user/work/merge-check/kz-llvm/tools}
KS=$KZB/kz-static/kz-static
O=${OROOT:-/home/user/work/redesign/obs1}
T=/home/user/work/redesign/step2a/tests_8c_fix
SEM=$REPO/kairos/semantics
STEP=${1:?usage: run.sh STEP}
( cd "$SEM" && sha256sum -c --quiet "$HERE/data_inputs.sha256" ) || { echo "refusing: a data file of kairos/semantics differs from data_inputs.sha256" >&2; exit 2; }
[ -x "$KS" ] || { echo "no kz-static at $KS" >&2; exit 2; }
new_dir() { [ -e "$1" ] && { echo "refusing: $1 exists" >&2; exit 2; }; mkdir -p "$1"; }
BUILDS="c1_pre_tx c1_post_tx r2_core c5_641 c5_642"
rc=0
case "$STEP" in
  relcheck)
    new_dir "$O/rf2_relcheck"
    for b in $BUILDS; do
      mkdir -p "$O/rf2_relcheck/$b"
      "$KS" relations "$T/$b/NP/facts.kz" -o "$O/rf2_relcheck/$b/relations.txt" --keys "$O/rf2_relcheck/$b/keys.txt" > "$O/rf2_relcheck/$b/relations.log" 2>&1; r=$?
      echo "$b rc=$r"; [ $r = 0 ] || rc=1
    done ;;
  order|off_order|final_order)
    case "$STEP" in
      order) D=$O/rf2_order; FR= ;;
      off_order) D=$O/rf2_off_order; FR=$O/rf2_off_extract ;;
      final_order) D=$O/rf2_final_order; FR=$O/rf2_final_extract ;;
    esac
    new_dir "$D"
    for b in $BUILDS; do
      if [ -n "$FR" ]; then F=$FR/$b/$b/NP/facts.kz; else F=$T/$b/NP/facts.kz; fi
      mkdir -p "$D/$b/again"
      X1=(); X2=()
      if [ "$STEP" = final_order ]; then X1=(--caller-dependent-report "$D/$b/cdr.txt"); X2=(--caller-dependent-report "$D/$b/again/cdr.txt"); fi
      r=0
      "$KS" order "$F" -o "$D/$b/order.txt" --edges "$D/$b/edges.tsv" "${X1[@]}" > "$D/$b/order.log" 2>&1 || r=1
      "$KS" order "$F" -o "$D/$b/again/order.txt" --edges "$D/$b/again/edges.tsv" "${X2[@]}" > "$D/$b/again/order.log" 2>&1 || r=1
      if [ -n "$FR" ]; then "$KS" relations "$F" -o "$D/$b/relations.txt" --keys "$D/$b/keys.txt" > "$D/$b/relations.log" 2>&1 || r=1; fi
      echo "$b rc=$r"; [ $r = 0 ] || rc=1
    done ;;
  extract_off|extract_final)
    if [ "$STEP" = extract_off ]; then D=$O/rf2_off_extract; X=
    else
      D=$O/rf2_final_extract; X=--object-access
      for f in framework_semantics_cfe-6.4.1.txt framework_semantics_cfe-6.4.2.txt framework_semantics_cfe-6cccb3f.txt framework_semantics_cfe-7.0.1.txt \
               framework_semantics_osal-4.1.1.txt framework_semantics_osal-7.0.1.txt framework_semantics_osal-f34759d.txt p0_declarations_glibc-2.39-headers.txt; do X="$X --semantics $SEM/$f"; done
      X="$X --result-flow --loop-defs --lock-handles"
    fi
    new_dir "$D"
    for b in $BUILDS; do
      KZB="$KZB" ONE_BUILD_EXTRACT_EXTRA="$X" bash "$REPO/env/redesign/one_build.sh" "$b" "$D/$b"; r=$?
      echo "$b rc=$r"; [ $r = 0 ] || rc=1
    done ;;
  plans_t8cf|plans_final|plans_v4|plans_v4_all|plans_v4_kept)
    case "$STEP" in
      plans_t8cf) D=$O/rf2_plans_t8cf; FR=t8cf; MODE= ;;
      plans_final) D=$O/rf2_plans_final; FR=$O/rf2_final_extract; MODE= ;;
      plans_v4) D=$O/rf2_plans_v4; FR=$O/rf2_final_extract; MODE=v4 ;;
      plans_v4_all) D=$O/rf2_plans_v4_all; FR=$O/rf2_final_extract; MODE=v4all ;;
      plans_v4_kept) D=$O/rf2_plans_v4_keptfacts; FR=/home/user/work/redesign/obs1/ll_final_extract; MODE=v4 ;;
    esac
    new_dir "$D"
    for b in $BUILDS; do
      if [ "$FR" = t8cf ]; then N=$T/$b/NP; else N=$FR/$b/$b/NP; fi
      PLANS_FACTS=$N/facts.kz PLANS_BASIS=$N/basis.kz bash "$HERE/plan_one.sh" "$KS" "$D/$b" $MODE; r=$?
      echo "$b rc=$r"; [ $r = 0 ] || rc=1
    done ;;
  *) echo "usage: run.sh relcheck|order|extract_off|extract_final|off_order|final_order|plans_t8cf|plans_final|plans_v4|plans_v4_all|plans_v4_kept" >&2; exit 64 ;;
esac
exit $rc
