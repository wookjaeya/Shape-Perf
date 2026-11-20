#!/usr/bin/env bash
# preserve_bundle.sh -- put one run bundle into the repository's records/ directory so that it survives the session
# container (PI decision 2026-10-29, Architecture/spec/PI_DECISIONS_RESOLVED_20261029.md A2; CONFIG_MANAGEMENT.md 5).
#
#   preserve_bundle.sh <bundle id> <source dir>... [-- <note>]
#
# Writes research/cfs_procrace/records/<bundle id>/:
#   MANIFEST.sha256   sha256 of every file inside the bundle, path relative to the bundle root (<source basename>/...)
#   bundle.tar.xz.NN  the tar of the source dirs, xz -9, split into 45 MB parts (GitHub refuses files over 100 MB)
#   PARTS.sha256      sha256 of each part
#   INFO.txt          when, from where, sizes, repository commit, kernel, the note
# Refuses: an existing bundle id; a source path that does not exist; a file whose text matches a credential pattern
# (the run records hold environment values only for the variables listed in env/lib_record.sh, CONDITIONS.md H-6, but
# a bundle is checked again before it is committed).  It never deletes or changes the source dirs.
# The bundle counts as preserved only after verify_bundle.sh has read it back from the remote branch.
set -euo pipefail
ID=${1:?bundle id}; shift
SRCS=(); NOTE=""
while [ $# -gt 0 ]; do
  if [ "$1" = "--" ]; then shift; NOTE="$*"; break; fi
  SRCS+=("$1"); shift
done
[ ${#SRCS[@]} -gt 0 ] || { echo "no source dir" >&2; exit 2; }
case "$ID" in *[!A-Za-z0-9._-]*|'') echo "bundle id: letters, digits, . _ - only" >&2; exit 2 ;; esac
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
R="$(cd "$HERE/../.." && pwd)"
DEST="$R/records/$ID"
[ ! -e "$DEST" ] || { echo "refusing: $DEST exists" >&2; exit 3; }
for s in "${SRCS[@]}"; do [ -d "$s" ] || { echo "not a directory: $s" >&2; exit 2; }; done
# credential patterns (names of the form used by the harness records are fine; values are not)
PAT='(ghp_|gho_|github_pat_|xox[bap]-|sk-[A-Za-z0-9]{20}|AKIA[0-9A-Z]{16}|-----BEGIN [A-Z ]*PRIVATE KEY|[Aa]uthorization: *(Bearer|Basic) [A-Za-z0-9])'
HIT="$(for s in "${SRCS[@]}"; do grep -rIlE "$PAT" "$s" 2>/dev/null || true; done)"
[ -z "$HIT" ] || { echo "refusing: credential-like text in:"; echo "$HIT"; exit 4; } >&2
mkdir -p "$DEST"
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
LIST="$TMP/list"; : > "$LIST"
for s in "${SRCS[@]}"; do
  p="$(cd "$(dirname "$s")" && pwd)"; b="$(basename "$s")"
  ( cd "$p" && find "$b" -type f -print0 | sort -z | xargs -0 sha256sum ) >> "$DEST/MANIFEST.sha256"
  echo "$p|$b" >> "$LIST"
done
# one tar over all sources, each stored under its basename
TARARGS=(); while IFS='|' read -r p b; do TARARGS+=(-C "$p" "$b"); done < "$LIST"
tar --sort=name --owner=0 --group=0 --numeric-owner -cf - "${TARARGS[@]}" | xz -9 -T2 -c > "$TMP/bundle.tar.xz"
( cd "$TMP" && split -b 45M -d -a 2 bundle.tar.xz "$DEST/bundle.tar.xz." )
( cd "$DEST" && sha256sum bundle.tar.xz.* > PARTS.sha256 )
{
  echo "bundle=$ID"
  echo "created_utc=$(date -u +%FT%T.%3NZ)"
  echo "repo_commit=$(git -C "$R" rev-parse HEAD)"
  echo "kernel=$(uname -r)"
  while IFS='|' read -r p b; do echo "source=$p/$b files=$(find "$p/$b" -type f | wc -l) bytes=$(du -sb "$p/$b" | cut -f1)"; done < "$LIST"
  echo "tar_xz_bytes=$(stat -c %s "$TMP/bundle.tar.xz") parts=$(ls "$DEST" | grep -c '^bundle.tar.xz\.')"
  echo "note=$NOTE"
} > "$DEST/INFO.txt"
cat "$DEST/INFO.txt"
