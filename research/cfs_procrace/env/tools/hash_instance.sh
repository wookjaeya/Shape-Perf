#!/usr/bin/env bash
# hash_instance.sh DIR LIST -- sha256 of every regular file under an instance directory (executables, modules, tables,
# startup scripts, any data file), before a boot, so that the run record names each file the instance can read.
# Writes LIST (sha256sum format, paths relative to DIR, sorted, LC_ALL=C) and LIST.other (entries that are neither
# regular files nor directories: type, path, link target).  Prints one line for run.meta:
#   instance_files dir=<DIR> files=<n> other=<k> list=<LIST> list_sha256=<hex>
# Reads DIR only.  Refuses an existing LIST.  rc 0, or 1 when hashing failed, 64 on usage.
set -uo pipefail
DIR=${1:-}; LIST=${2:-}
[ -d "$DIR" ] && [ -n "$LIST" ] || { echo "usage: hash_instance.sh DIR LIST" >&2; exit 64; }
[ ! -e "$LIST" ] || { echo "refusing: $LIST exists" >&2; exit 64; }
export LC_ALL=C
( set -o pipefail; cd "$DIR" && find . -type f -print0 | sort -z | xargs -0 -r sha256sum ) > "$LIST"
rc=$?
( cd "$DIR" && find . ! -type f ! -type d -printf '%y\t%p\t%l\n' | sort ) > "$LIST.other"
[ $rc -eq 0 ] || { echo "instance_files dir=$DIR hashing failed (rc $rc)"; exit 1; }
printf 'instance_files dir=%s files=%s other=%s list=%s list_sha256=%s\n' "$DIR" "$(wc -l < "$LIST")" \
  "$(wc -l < "$LIST.other")" "$LIST" "$(sha256sum < "$LIST" | cut -c1-64)"
