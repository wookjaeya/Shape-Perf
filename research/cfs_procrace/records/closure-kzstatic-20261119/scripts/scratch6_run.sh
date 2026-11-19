#!/usr/bin/env bash
# scratch6_run.sh STEP -- the runs of the five reference builds by the scratch check build of the complete source of the
# kz-static closure build (results section 20), each step into a new directory under OROOT (a scratch directory; there is
# no default), one step at a time.  A scratch runner: the outputs are [I] for the declaration, not the outputs of a
# declared tool.  The steps follow records/review-fixes-2-mc-20261119/run.sh with two changes: the order steps also write
# the complete pair file (--pairs, order rules revision 8), and the plan steps add the plan on the name-space data files of
# coverage rule revision 2 (plan rules revision 5).
#   relcheck      relations --keys on the tests_8c_fix NP facts                         -> s6_relcheck/<b>
#   order         order --edges --pairs on the same facts, run twice (again/)            -> s6_order/<b>
#   extract_off   env/redesign/one_build.sh without an extraction option               -> s6_off_extract/<b>
#   extract_final one_build.sh with every extraction option (the arguments of rf2_final_extract)
#                                                                                       -> s6_final_extract/<b>
#   off_order     order --edges --pairs twice and relations --keys on s6_off_extract's facts -> s6_off_order/<b>
#   final_order   order --edges --pairs --caller-dependent-report twice and relations --keys on s6_final_extract's
#                 facts                                                                 -> s6_final_order/<b>
#   plans_t8cf    the plan without --manifest on the tests_8c_fix facts                 -> s6_plans_t8cf/<b>
#   plans_final   the same on s6_final_extract's facts                                  -> s6_plans_final/<b>
#   plans_v4      --manifest 4 with the revision-1 name-space files (the arguments of rf2_plans_v4) on
#                 s6_final_extract's facts                                              -> s6_plans_v4/<b>
#   plans_v4_ns2  --manifest 4 with the revision-2 name-space files (plan_one_ns.sh 2) on s6_final_extract's facts
#                                                                                       -> s6_plans_v4_ns2/<b>
# The data files are checked against ../scratch6/data_inputs.sha256 before every step.  Environment: KZB the tool
# directory (required), OROOT the output root (required).  rc 0 when every build's run returned 0, 1 when one failed,
# 2 when a step could not start.
set -uo pipefail
REPO=/home/user/Shape-Perf/research/cfs_procrace
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RF=$REPO/records/review-fixes-2-mc-20261119
KZB=${KZB:?KZB: the tool directory of the scratch check build}
KS=$KZB/kz-static/kz-static
O=${OROOT:?OROOT: a scratch output root}
T=/home/user/work/redesign/step2a/tests_8c_fix
SEM=$REPO/kairos/semantics
STEP=${1:?usage: scratch6_run.sh STEP}
( cd "$SEM" && sha256sum -c --quiet "$HERE/../scratch6/data_inputs.sha256" ) || { echo "refusing: a data file of kairos/semantics differs from data_inputs.sha256" >&2; exit 2; }
[ -x "$KS" ] || { echo "no kz-static at $KS" >&2; exit 2; }
new_dir() { [ -e "$1" ] && { echo "refusing: $1 exists" >&2; exit 2; }; mkdir -p "$1"; }
BUILDS="c1_pre_tx c1_post_tx r2_core c5_641 c5_642"
rc=0
case "$STEP" in
  relcheck)
    new_dir "$O/s6_relcheck"
    for b in $BUILDS; do
      mkdir -p "$O/s6_relcheck/$b"
      "$KS" relations "$T/$b/NP/facts.kz" -o "$O/s6_relcheck/$b/relations.txt" --keys "$O/s6_relcheck/$b/keys.txt" > "$O/s6_relcheck/$b/relations.log" 2>&1; r=$?
      echo "$b rc=$r"; [ $r = 0 ] || rc=1
    done ;;
  order|off_order|final_order)
    case "$STEP" in
      order) D=$O/s6_order; FR= ;;
      off_order) D=$O/s6_off_order; FR=$O/s6_off_extract ;;
      final_order) D=$O/s6_final_order; FR=$O/s6_final_extract ;;
    esac
    new_dir "$D"
    for b in $BUILDS; do
      if [ -n "$FR" ]; then F=$FR/$b/$b/NP/facts.kz; else F=$T/$b/NP/facts.kz; fi
      mkdir -p "$D/$b/again"
      X1=(); X2=()
      if [ "$STEP" = final_order ]; then X1=(--caller-dependent-report "$D/$b/cdr.txt"); X2=(--caller-dependent-report "$D/$b/again/cdr.txt"); fi
      r=0
      "$KS" order "$F" -o "$D/$b/order.txt" --edges "$D/$b/edges.tsv" --pairs "$D/$b/pairs.tsv" "${X1[@]}" > "$D/$b/order.log" 2>&1 || r=1
      "$KS" order "$F" -o "$D/$b/again/order.txt" --edges "$D/$b/again/edges.tsv" --pairs "$D/$b/again/pairs.tsv" "${X2[@]}" > "$D/$b/again/order.log" 2>&1 || r=1
      if [ -n "$FR" ]; then "$KS" relations "$F" -o "$D/$b/relations.txt" --keys "$D/$b/keys.txt" > "$D/$b/relations.log" 2>&1 || r=1; fi
      echo "$b rc=$r"; [ $r = 0 ] || rc=1
    done ;;
  extract_off|extract_final)
    if [ "$STEP" = extract_off ]; then D=$O/s6_off_extract; X=
    else
      D=$O/s6_final_extract; X=--object-access
      for f in framework_semantics_cfe-6.4.1.txt framework_semantics_cfe-6.4.2.txt framework_semantics_cfe-6cccb3f.txt framework_semantics_cfe-7.0.1.txt \
               framework_semantics_osal-4.1.1.txt framework_semantics_osal-7.0.1.txt framework_semantics_osal-f34759d.txt p0_declarations_glibc-2.39-headers.txt; do X="$X --semantics $SEM/$f"; done
      X="$X --result-flow --loop-defs --lock-handles"
    fi
    new_dir "$D"
    for b in $BUILDS; do
      KZB="$KZB" ONE_BUILD_EXTRACT_EXTRA="$X" bash "$REPO/env/redesign/one_build.sh" "$b" "$D/$b"; r=$?
      echo "$b rc=$r"; [ $r = 0 ] || rc=1
    done ;;
  plans_t8cf|plans_final|plans_v4|plans_v4_ns2)
    case "$STEP" in
      plans_t8cf) D=$O/s6_plans_t8cf; FR=t8cf ;;
      plans_final) D=$O/s6_plans_final; FR=$O/s6_final_extract ;;
      plans_v4) D=$O/s6_plans_v4; FR=$O/s6_final_extract ;;
      plans_v4_ns2) D=$O/s6_plans_v4_ns2; FR=$O/s6_final_extract ;;
    esac
    new_dir "$D"
    for b in $BUILDS; do
      if [ "$FR" = t8cf ]; then N=$T/$b/NP; else N=$FR/$b/$b/NP; fi
      case "$STEP" in
        plans_t8cf|plans_final) PLANS_FACTS=$N/facts.kz PLANS_BASIS=$N/basis.kz bash "$RF/plan_one.sh" "$KS" "$D/$b" ;;
        plans_v4) PLANS_FACTS=$N/facts.kz PLANS_BASIS=$N/basis.kz bash "$RF/plan_one.sh" "$KS" "$D/$b" v4 ;;
        plans_v4_ns2) PLANS_FACTS=$N/facts.kz PLANS_BASIS=$N/basis.kz bash "$HERE/plan_one_ns.sh" "$KS" "$D/$b" 2 ;;
      esac
      r=$?
      echo "$b rc=$r"; [ $r = 0 ] || rc=1
    done ;;
  *) echo "usage: scratch6_run.sh relcheck|order|extract_off|extract_final|off_order|final_order|plans_t8cf|plans_final|plans_v4|plans_v4_ns2" >&2; exit 64 ;;
esac
exit $rc
