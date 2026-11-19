#!/usr/bin/env bash
# run.sh STEP -- the runs of the five reference builds by the MC kz-static of the declared kz-static closure build
# (results section 21), each step into a new directory /home/user/work/redesign/obs1/cz_<step>, one step at a time.
# The steps and arguments are those of records/closure-kzstatic-20261119/scripts/scratch6_run.sh (the scratch check
# build's runner, results 20.3), so that every output has a scratch twin; only the output root (cz_ in place of the
# scratch root's s6_), the tool directory (the MC tree) and this header differ.
#   relcheck      relations --keys on the tests_8c_fix NP facts                         -> cz_relcheck/<b>
#   order         order --edges --pairs on the same facts, run twice (again/)            -> cz_order/<b>
#   extract_off   env/redesign/one_build.sh without an extraction option               -> cz_off_extract/<b>
#   extract_final one_build.sh with every extraction option (the arguments of rf2_final_extract)
#                                                                                       -> cz_final_extract/<b>
#   off_order     order --edges --pairs twice and relations --keys on cz_off_extract's facts -> cz_off_order/<b>
#   final_order   order --edges --pairs --caller-dependent-report twice and relations --keys on cz_final_extract's
#                 facts                                                                 -> cz_final_order/<b>
#   plans_t8cf    the plan without --manifest on the tests_8c_fix facts                 -> cz_plans_t8cf/<b>
#   plans_final   the same on cz_final_extract's facts                                  -> cz_plans_final/<b>
#   plans_v4      --manifest 4 with the revision-1 name-space files (the arguments of rf2_plans_v4) on
#                 cz_final_extract's facts                                              -> cz_plans_v4/<b>
#   plans_v4_ns2  --manifest 4 with the revision-2 name-space files (plan_one_ns.sh 2) on cz_final_extract's facts
#                                                                                       -> cz_plans_v4_ns2/<b>
# The control budget reason of the manifest-4 plans says "control runs of design rows" (plan_one.sh, plan_one_ns.sh).
# The data files are checked against data_inputs.sha256 (a copy of the scratch run's list) before every step.
# Environment: KZB the tool directory (default the MC tree).  rc 0 when every build's run returned 0, 1 when one
# failed, 2 when a step could not start.
set -uo pipefail
REPO=/home/user/Shape-Perf/research/cfs_procrace
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RF=$REPO/records/review-fixes-2-mc-20261119
SCR=$REPO/records/closure-kzstatic-20261119/scripts
KZB=${KZB:-/home/user/work/merge-check/kz-llvm/tools}
KS=$KZB/kz-static/kz-static
O=/home/user/work/redesign/obs1
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
    new_dir "$O/cz_relcheck"
    for b in $BUILDS; do
      mkdir -p "$O/cz_relcheck/$b"
      "$KS" relations "$T/$b/NP/facts.kz" -o "$O/cz_relcheck/$b/relations.txt" --keys "$O/cz_relcheck/$b/keys.txt" > "$O/cz_relcheck/$b/relations.log" 2>&1; r=$?
      echo "$b rc=$r"; [ $r = 0 ] || rc=1
    done ;;
  order|off_order|final_order)
    case "$STEP" in
      order) D=$O/cz_order; FR= ;;
      off_order) D=$O/cz_off_order; FR=$O/cz_off_extract ;;
      final_order) D=$O/cz_final_order; FR=$O/cz_final_extract ;;
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
    if [ "$STEP" = extract_off ]; then D=$O/cz_off_extract; X=
    else
      D=$O/cz_final_extract; X=--object-access
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
      plans_t8cf) D=$O/cz_plans_t8cf; FR=t8cf ;;
      plans_final) D=$O/cz_plans_final; FR=$O/cz_final_extract ;;
      plans_v4) D=$O/cz_plans_v4; FR=$O/cz_final_extract ;;
      plans_v4_ns2) D=$O/cz_plans_v4_ns2; FR=$O/cz_final_extract ;;
    esac
    new_dir "$D"
    for b in $BUILDS; do
      if [ "$FR" = t8cf ]; then N=$T/$b/NP; else N=$FR/$b/$b/NP; fi
      case "$STEP" in
        plans_t8cf|plans_final) PLANS_FACTS=$N/facts.kz PLANS_BASIS=$N/basis.kz bash "$RF/plan_one.sh" "$KS" "$D/$b" ;;
        plans_v4) PLANS_FACTS=$N/facts.kz PLANS_BASIS=$N/basis.kz bash "$RF/plan_one.sh" "$KS" "$D/$b" v4 ;;
        plans_v4_ns2) PLANS_FACTS=$N/facts.kz PLANS_BASIS=$N/basis.kz bash "$SCR/plan_one_ns.sh" "$KS" "$D/$b" 2 ;;
      esac
      r=$?
      echo "$b rc=$r"; [ $r = 0 ] || rc=1
    done ;;
  *) echo "usage: run.sh relcheck|order|extract_off|extract_final|off_order|final_order|plans_t8cf|plans_final|plans_v4|plans_v4_ns2" >&2; exit 64 ;;
esac
exit $rc
