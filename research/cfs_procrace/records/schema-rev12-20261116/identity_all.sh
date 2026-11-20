#!/usr/bin/env bash
# identity_all.sh TREE -- `--identity` of every tool of a build tree that has one (kz-static, kz, kz-dump, kz-phi) and the
# schema revision of `kz-dump --layout`; rc 0 only when every tool prints schema_revision= equal to the layout's revision
# and schema_fingerprint= equal to the layout's fingerprint (no tool of the tree left on another revision).  Reads only.
set -uo pipefail
T=${1:?usage: identity_all.sh TREE}
L=$("$T/tools/kz-dump/kz-dump" --layout) || { echo "kz-dump --layout failed"; exit 1; }
rev=$(printf '%s\n' "$L" | sed -n '1s/^kz-layout abi=[0-9]* revision=\([0-9]*\)$/\1/p')
fp=$(printf '%s\n' "$L" | sed -n 's/^fingerprint=\([0-9a-f]*\)$/\1/p')
echo "layout revision=$rev fingerprint=$fp"
rc=0
for t in tools/kz-static/kz-static tools/kz/kz tools/kz-dump/kz-dump tools/kz-phi/kz-phi; do
  out=$("$T/$t" --identity 2>&1); r=$?
  echo "# $t (rc $r)"; printf '%s\n' "$out"
  printf '%s\n' "$out" | grep -qx "schema_revision=$rev" && printf '%s\n' "$out" | grep -qx "schema_fingerprint=$fp" || { echo "MISMATCH $t"; rc=1; }
done
echo "identity_all: $([ $rc = 0 ] && echo "every tool reports revision $rev" || echo 'a tool reports another revision or none')"
exit $rc
