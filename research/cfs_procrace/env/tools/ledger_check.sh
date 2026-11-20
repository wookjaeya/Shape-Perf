#!/usr/bin/env bash
# ledger_check.sh [LEDGER] -- re-check every row of the evidence ledger (default records/LEDGER.tsv): the cumulative
# byte-identity list that every build re-checks.
#   method sha256c: (cd ROOT && sha256sum -c --quiet LIST); LIST relative to the research directory unless absolute
#   method parts:   (cd ROOT && sha256sum -c --quiet PARTS.sha256)
# A row fails when its list or root is missing, a listed file is missing, or a byte differs.  The four state columns
# (local, verified_copy, remote, published) are printed, not judged.  One line per row and a summary; rc 0 when no
# row fails, 1 otherwise, 64 on usage.  Reads only.
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RD="$(cd "$HERE/../.." && pwd)"
LED=${1:-$RD/records/LEDGER.tsv}
[ -f "$LED" ] || { echo "usage: ledger_check.sh [LEDGER]" >&2; exit 64; }
n=0; nf=0
while IFS=$'\t' read -r id kind method list root local vcopy remote pub note; do
  case "$id" in ''|'#'*|id) continue ;; esac
  n=$((n + 1)); res=; ok=1
  case "$method" in
    sha256c)
      case "$list" in /*) L=$list ;; *) L=$RD/$list ;; esac
      if [ ! -f "$L" ]; then ok=0; res="list missing ($L)"
      elif [ ! -d "$root" ]; then ok=0; res="root missing ($root)"
      else
        out=$(cd "$root" && sha256sum -c --quiet "$L" 2>&1); rc=$?
        cnt=$(grep -c . "$L")
        if [ $rc -eq 0 ]; then res="$cnt files same"; else ok=0; res="$(printf '%s' "$out" | grep -E 'FAILED|No such' | head -3 | tr '\n' ';')"; fi
      fi ;;
    parts)
      if [ ! -f "$root/PARTS.sha256" ]; then ok=0; res="no PARTS.sha256 in $root"
      else
        out=$(cd "$root" && sha256sum -c --quiet PARTS.sha256 2>&1); rc=$?
        if [ $rc -eq 0 ]; then res="$(grep -c . "$root/PARTS.sha256") parts same"; else ok=0; res="$(printf '%s' "$out" | grep -E 'FAILED|No such' | head -3 | tr '\n' ';')"; fi
      fi ;;
    *) ok=0; res="unknown method $method" ;;
  esac
  if [ $ok -eq 1 ]; then echo "OK $id: $res [local=$local copy=$vcopy remote=$remote published=$pub]"
  else echo "FAIL $id: $res"; nf=$((nf + 1)); fi
done < "$LED"
echo "ledger_check: $n rows, $nf failing"
[ $nf -eq 0 ]
