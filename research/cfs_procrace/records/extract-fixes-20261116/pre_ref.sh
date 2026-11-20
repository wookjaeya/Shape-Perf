#!/usr/bin/env bash
# pre_ref.sh -- BEFORE the build, with the kz-static in place (5ae220d2): the references the new tool's outputs on the
# re-extracted facts are compared with, on the facts of schema12_extract (the same tool's re-extraction): relations with
# the use-site key listing (for the table old key -> new key) and the plans (the arguments of plans.sh).  New directory.
set -uo pipefail
KS=/home/user/work/merge-check/kz-llvm/tools/kz-static/kz-static
X=/home/user/work/redesign/obs1/schema12_extract
O=${O:-/home/user/work/redesign/obs1/extfix_pre_onextract}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
[ -e "$O" ] && { echo "refusing: $O exists" >&2; exit 2; }
mkdir -p "$O"
echo "kz-static $(sha256sum < "$KS" | cut -c1-64)" > "$O/TOOL"
rc=0
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  D=$X/$b/$b/NP; mkdir -p "$O/$b"
  "$KS" relations "$D/facts.kz" -o "$O/$b/relations.txt" --keys "$O/$b/keys.txt" > "$O/$b/relations.log" 2>&1 || rc=1
  PLANS_FACTS=$D/facts.kz PLANS_BASIS=$D/basis.kz bash "$HERE/plan_one.sh" "$KS" "$O/$b/plan" || rc=1
done
exit $rc
