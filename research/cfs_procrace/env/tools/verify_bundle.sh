#!/usr/bin/env bash
# verify_bundle.sh -- read a preserved bundle back from the REMOTE branch and check it (CONFIG_MANAGEMENT.md 5;
# PI_DECISIONS_RESOLVED_20261029.md A2: preserved means "downloaded again from outside the container, hashes checked,
# analysis inputs open").
#
#   verify_bundle.sh <bundle id> <work dir> [<check command> ...]
#
# 1. a fresh shallow clone of origin/<branch> into <work dir>/clone, with only records/<bundle id> checked out
# 2. PARTS.sha256 checked, the parts joined, xz/tar integrity checked, extracted to <work dir>/extract
# 3. MANIFEST.sha256 checked inside the extract
# 4. if a check command is given, it runs with the extract dir as its last argument (e.g. a kz replay of one record)
# Prints VERIFIED <bundle id> <commit> only if every step passed.
set -euo pipefail
ID=${1:?bundle id}; WD=${2:?work dir}; shift 2
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
R="$(cd "$HERE/../.." && pwd)"
TOP="$(git -C "$R" rev-parse --show-toplevel)"
REL="$(realpath --relative-to="$TOP" "$R")/records/$ID"
URL="$(git -C "$TOP" remote get-url origin)"
BR="$(git -C "$TOP" rev-parse --abbrev-ref HEAD)"
[ ! -e "$WD" ] || { echo "refusing: $WD exists" >&2; exit 3; }
mkdir -p "$WD"
git clone -q --depth 1 --filter=blob:none --sparse --branch "$BR" "$URL" "$WD/clone"
git -C "$WD/clone" sparse-checkout set "$REL"
C="$(git -C "$WD/clone" rev-parse HEAD)"
B="$WD/clone/$REL"
[ -f "$B/PARTS.sha256" ] || { echo "no bundle $ID on origin/$BR" >&2; exit 4; }
( cd "$B" && sha256sum -c --quiet PARTS.sha256 )
cat "$B"/bundle.tar.xz.?? > "$WD/bundle.tar.xz"
xz -t "$WD/bundle.tar.xz"
mkdir "$WD/extract"
xz -dc "$WD/bundle.tar.xz" | tar -xf - -C "$WD/extract"
( cd "$WD/extract" && sha256sum -c --quiet "$B/MANIFEST.sha256" )
if [ $# -gt 0 ]; then "$@" "$WD/extract"; fi
echo "VERIFIED $ID $C"
