#!/usr/bin/env bash
# derive_plan.sh c1|c5 <arguments of derive_c1.sh or derive_c5.sh> --manifest FILE --modules TABLE --exes LIST
# The entry for derived builds made from a generated plan.  It refuses to start
#   - without all three plan inputs (no fixed manifest, module list or executable list is used on this path), and
#   - when the module table's header does not name the plan sidecar it came from: its first line must be
#       # kz module table; plan sidecar <path> sha256=<64 hex>
#     and <path> must be a file whose sha256 is that value.
# Then it runs derive_c1.sh or derive_c5.sh with the same arguments (derive_args.sh reads the plan inputs).
# rc 64 on a refusal; otherwise the rc of the derive script.
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
T=${1:-}; shift || true
case "$T" in c1) S=$HERE/derive_c1.sh ;; c5) S=$HERE/derive_c5.sh ;; *) echo "usage: derive_plan.sh c1|c5 ARGS... --manifest FILE --modules TABLE --exes LIST" >&2; exit 64 ;; esac
# DERIVE_PLAN_SCRIPT replaces the derive script (the unit checks, env/tools/tests/run_unit.sh, use a stand-in)
[ -z "${DERIVE_PLAN_SCRIPT:-}" ] || S=$DERIVE_PLAN_SCRIPT
M=; MT=; EX=; prev=
for a in "$@"; do
  case "$prev" in --manifest) M=$a ;; --modules) MT=$a ;; --exes) EX=$a ;; esac
  prev=$a
done
[ -n "$M" ] && [ -n "$MT" ] && [ -n "$EX" ] || { echo "refusing: a generated plan needs --manifest, --modules and --exes" >&2; exit 64; }
for f in "$M" "$MT" "$EX"; do [ -s "$f" ] || { echo "refusing: $f is missing or empty" >&2; exit 64; }; done
H=$(head -1 "$MT")
SC=$(printf '%s\n' "$H" | sed -n 's/^# kz module table; plan sidecar \(.*\) sha256=\([0-9a-f]\{64\}\)$/\1/p')
SH=$(printf '%s\n' "$H" | sed -n 's/^# kz module table; plan sidecar \(.*\) sha256=\([0-9a-f]\{64\}\)$/\2/p')
[ -n "$SC" ] && [ -n "$SH" ] || { echo "refusing: the module table's first line does not name the plan sidecar (# kz module table; plan sidecar <path> sha256=<hex>)" >&2; exit 64; }
[ -f "$SC" ] || { echo "refusing: the plan sidecar $SC named by the module table does not exist" >&2; exit 64; }
[ "$(sha256sum "$SC" | cut -c1-64)" = "$SH" ] || { echo "refusing: the plan sidecar $SC does not have the sha256 the module table names" >&2; exit 64; }
echo "derive_plan.sh: module table $MT from plan sidecar $SC (sha256 $SH)"
exec "$S" "$@"
