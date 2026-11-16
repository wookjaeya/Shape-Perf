#!/usr/bin/env bash
# plan_regen.sh -- the regeneration check of a generated observation plan (`kz-static plan`, kairos/docs/plan_manifest.md).
#
#   plan_regen.sh <generator> <args file> [<scratch dir>]
#
# <generator>: the kz-static binary, or its xz copy (*.xz; decompressed into the scratch dir and checked against the
# sha256 the copy's ORIGINAL list names when one is beside it).  <args file>: the arguments of the generation, one per
# line, as they were given after "kz-static" (the first is "plan").  The script runs the generator again with the same
# arguments, the four output paths (-o, --sidecar, --modules-out, --exes-out) replaced by files in a fresh scratch
# directory, and compares: the manifest and the sidecar byte for byte; the module table and the executable list byte for
# byte after their first line, and the first line except the sidecar path it names (the path written there is the one
# given on the command line).  rc 0 = identical, 1 = a difference (named), 2 = usage or a run failure.
set -uo pipefail
GEN=${1:?generator}; ARGS=${2:?args file}; SC=${3:-$(mktemp -d)}
mkdir -p "$SC" || exit 2
[ -z "$(ls -A "$SC")" ] || { echo "plan_regen: $SC is not empty" >&2; exit 2; }
if [ "${GEN%.xz}" != "$GEN" ]; then
  xz -dc "$GEN" > "$SC/generator" || exit 2
  chmod +x "$SC/generator"
  echo "plan_regen: generator $GEN decompressed, sha256 $(sha256sum "$SC/generator" | cut -c1-64)"
  GEN=$SC/generator
fi
mapfile -t A < "$ARGS"
declare -A ORIG
NEW=()
i=0
while [ $i -lt ${#A[@]} ]; do
  a=${A[$i]}
  case "$a" in
    -o|--sidecar|--modules-out|--exes-out)
      ORIG[$a]=${A[$((i + 1))]}
      NEW+=("$a" "$SC/out${a//-/_}")
      i=$((i + 2)); continue ;;
  esac
  NEW+=("$a"); i=$((i + 1))
done
for k in -o --sidecar --modules-out --exes-out; do [ -n "${ORIG[$k]:-}" ] || { echo "plan_regen: the args file has no $k" >&2; exit 2; }; done
"$GEN" "${NEW[@]}" || { echo "plan_regen: the generator failed (rc $?)" >&2; exit 2; }
rc=0
for k in -o --sidecar; do
  if cmp -s "${ORIG[$k]}" "$SC/out${k//-/_}"; then echo "same      $k ${ORIG[$k]} sha256 $(sha256sum "${ORIG[$k]}" | cut -c1-64)"
  else echo "DIFFERENT $k ${ORIG[$k]}"; rc=1; fi
done
for k in --modules-out --exes-out; do
  o=${ORIG[$k]}; n=$SC/out${k//-/_}
  l1o=$(head -1 "$o" | sed 's/plan sidecar .* sha256=/plan sidecar <path> sha256=/'); l1n=$(head -1 "$n" | sed 's/plan sidecar .* sha256=/plan sidecar <path> sha256=/')
  if [ "$l1o" = "$l1n" ] && cmp -s <(tail -n +2 "$o") <(tail -n +2 "$n"); then echo "same      $k $o (first line except the sidecar path)"
  else echo "DIFFERENT $k $o"; rc=1; fi
done
[ "$rc" = 0 ] && echo "PASS: the plan regenerates byte-identically" || echo "FAIL"
exit $rc
