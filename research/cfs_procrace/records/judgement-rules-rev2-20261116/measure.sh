#!/usr/bin/env bash
# measure.sh OUTDIR SCRATCH -- read-only measurements cited by judgement rules revision 2 (name binding of a path A
# absence outcome): the reason vocabulary test as it stands and with one added line (the added line goes to a scratch
# copy, never to the data file), the path A rows of the five builds and which of them the plans hold, the key-text
# events of the C1 boot record at the lookup and create sites, the roles of the framework-layer data, and whether the
# C1 plan holds a site of the object-delete path.  Builds nothing, writes only OUTDIR and SCRATCH.
set -uo pipefail
OUT=${1:?outdir}; SCR=${2:?scratch}
R=/home/user/Shape-Perf/research/cfs_procrace; K=$R/kairos; W=/home/user/work; O=$W/redesign/obs1
mkdir -p "$OUT" "$SCR"
T=$W/merge-check/kz-llvm/tests/static/test_static_reasons
V=$K/tests/static/data/reason_vocabulary.txt
{
  echo "test binary sha256 $(sha256sum "$T" | cut -d' ' -f1)"
  echo "vocabulary sha256 $(sha256sum "$V" | cut -d' ' -f1)"
  echo "== as is"; "$T" "$V" "$K/tests/static/data" 2>&1 | tail -3; echo "rc=${PIPESTATUS[0]}"
  cp "$V" "$SCR/vocab_plus1.txt"; echo name_binding_open >> "$SCR/vocab_plus1.txt"
  echo "== scratch copy with one added reason line"; "$T" "$SCR/vocab_plus1.txt" "$K/tests/static/data" 2>&1 | tail -3; echo "rc=${PIPESTATUS[0]}"
  rm -f "$SCR/vocab_plus1.txt"
  echo "vocabulary sha256 after $(sha256sum "$V" | cut -d' ' -f1)"
} > "$OUT/vocab_test.txt"
{
  printf 'build\trelations_sha256\tA_rows\tA_rows_in_plan\tA_rows_planned\tplanned_A\n'
  for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
    rel=$O/plangen_relcheck/$b/relations.txt; side=$O/plangen/$b/plan.side
    na=$(grep -cE '^  A[0-9]+ ' "$rel")
    ns=$(grep '^row ' "$side" | grep -cE ' A[0-9]+ ')
    np=$(grep '^row ' "$side" | grep -E ' A[0-9]+ ' | grep -c 'state=planned')
    pl=$(grep '^row ' "$side" | grep -E ' A[0-9]+ ' | grep 'state=planned' | awk '{print $3}' | paste -sd, -)
    printf '%s\t%s\t%s\t%s\t%s\t%s\n' "$b" "$(sha256sum "$rel" | cut -c1-16)" "$na" "$ns" "$np" "${pl:--}"
  done
} > "$OUT/path_a_rows.tsv"
{
  side=$O/plangen/c1_pre_tx/plan.side; dump=$O/c1pre_boot/tx/tx.dump.txt
  echo "sidecar $(sha256sum "$side" | cut -c1-16); dump text $(sha256sum "$dump" | cut -c1-16)"
  echo "== row A20 in the C1 sidecar"; grep -n '^row .* A20 ' "$side"
  echo "== key-text sites of the row of relation 1"; grep -n 'osapi-countsem.c' "$side"
  echo "== events at those sites (actor, aseq, values, text)"
  grep -E 'site=27[23]\[' "$dump" | awk '{o=""; for(i=1;i<=NF;i++) if($i ~ /^(actor|aseq|site|v|str)=/) o=o" "$i; print o}'
  echo "== sites of the object-delete path in the C1 plan (OS_ObjectIdFinalizeDelete, OS_CountSemDelete): $(grep -cE 'OS_ObjectIdFinalizeDelete|OS_CountSemDelete' "$side")"
} > "$OUT/c1_record_sites.txt"
{
  f=$K/semantics/framework_api_roles.txt
  echo "framework-layer data sha256 $(sha256sum "$f" | cut -c1-16)"
  grep '^role ' "$f" | awk '{print $2, $3}' | sort | uniq -c
} > "$OUT/framework_roles.txt"
echo done
