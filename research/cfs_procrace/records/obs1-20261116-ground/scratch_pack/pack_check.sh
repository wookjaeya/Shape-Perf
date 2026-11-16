#!/bin/bash
# pack_check.sh SCRATCH PACKDIR NAME PATH...: sha256 list of the regular files under PATH..., tar.xz of them,
# stream re-hash of the archive compared with the list. Removes nothing. Prints one summary line; rc 0 only if same.
set -u -o pipefail
S=$1; P=$2; N=$3; shift 3
cd "$S" || exit 2
export LC_ALL=C
find "$@" -type f -print0 | sort -z | xargs -0 -r sha256sum | sort -k2 > "$P/$N.sha256" || { echo "$N: hash failed"; exit 1; }
find "$@" ! -type f ! -type d -printf '%y\t%p\t%l\n' | sort > "$P/$N.other"
b=$(find "$@" -type f -printf '%s\n' | awk '{s+=$1}END{print s+0}')
tar --sort=name --owner=0 --group=0 --numeric-owner -cf - "$@" | xz -6 -T2 > "$P/$N.tar.xz" || { echo "$N: pack failed"; exit 1; }
xz -dc "$P/$N.tar.xz" | tar -xf - --to-command='sha256sum | sed "s|-\$|$TAR_FILENAME|"' | sort -k2 > "$P/$N.check" || { echo "$N: stream failed"; exit 1; }
xz -dc "$P/$N.tar.xz" | tar -tvf - | awk '$1 !~ /^[-d]/{print $1, $6, $7, $8}' | sort > "$P/$N.other_stream"
if cmp -s "$P/$N.sha256" "$P/$N.check"; then
  printf '%s\tfiles=%s\tbytes=%s\txz=%s\txz_sha256=%s\tlist_sha256=%s\tother=%s\tstream=same\n' "$N" "$(wc -l < "$P/$N.sha256")" "$b" \
    "$(stat -c %s "$P/$N.tar.xz")" "$(sha256sum < "$P/$N.tar.xz" | cut -c1-64)" "$(sha256sum < "$P/$N.sha256" | cut -c1-64)" "$(wc -l < "$P/$N.other")"
else echo "$N: STREAM DIFFERS"; exit 1; fi
