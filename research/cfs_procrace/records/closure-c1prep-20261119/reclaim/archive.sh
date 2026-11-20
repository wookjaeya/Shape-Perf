#!/usr/bin/env bash
# archive.sh NAME LIST -- replace the kept output directory /home/user/work/redesign/obs1/NAME (one of the option-off
# extract rounds that results section 12.10 item 2 allows to be replaced by a compressed archive) by an xz tar archive
# in /home/user/work/preserve/archives-20261119/NAME/, after a file-by-file round trip from the archive.
#   LIST  the directory's member list (the sha256 list of its ledger row, relative to the research directory)
# Steps, each checked before the next (rc 1 at the first failure; nothing is removed then):
#   1. the directory against LIST (sha256sum -c), and the set of files on disk = the set LIST names
#   2. the archive: tar of the directory's contents (members ./<path>, mode, owner and times kept) through xz -6
#   3. PARTS.sha256 (the archive's sha256, the ledger's "parts" method) and MEMBERS.sha256 (a copy of LIST)
#   4. round trip: every regular member of the archive streamed through sha256sum (tar --to-command), the sorted
#      result equal to the sorted LIST line by line; the member names equal to the files on disk
#   5. only then the directory is removed
# Writes roundtrip_NAME.txt beside this script.  ARCHIVE_SRC_ROOT and ARCHIVE_DST_ROOT replace the two roots (the
# self test, selftest.sh, only).
set -uo pipefail
NAME=${1:?name}; LIST=${2:?list}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RD=/home/user/Shape-Perf/research/cfs_procrace
SRC=${ARCHIVE_SRC_ROOT:-/home/user/work/redesign/obs1}/$NAME
DST=${ARCHIVE_DST_ROOT:-/home/user/work/preserve/archives-20261119}/$NAME
OUT=$HERE/roundtrip_$NAME.txt
case "$LIST" in /*) L=$LIST ;; *) L=$RD/$LIST ;; esac
fail() { echo "FAIL: $*" | tee -a "$OUT"; exit 1; }
[ -d "$SRC" ] || { echo "no directory $SRC" >&2; exit 2; }
[ -f "$L" ] || { echo "no list $L" >&2; exit 2; }
[ ! -e "$DST" ] || { echo "refusing: $DST exists" >&2; exit 2; }
: > "$OUT"
echo "# archive.sh $NAME $LIST $(date -u +%FT%TZ)" >> "$OUT"
# 1
( cd "$SRC" && sha256sum -c --quiet "$L" ) >> "$OUT" 2>&1 || fail "the directory does not match its list"
cmp -s <(cd "$SRC" && find . -type f | LC_ALL=C sort) <(sed 's/^[0-9a-f]\{64\}  //' "$L" | LC_ALL=C sort) || fail "the files on disk are not the files the list names"
echo "1. directory = list: $(grep -c . "$L") files, $(du -sb "$SRC" | cut -f1) bytes (apparent), $(du -s --block-size=1 "$SRC" | cut -f1) bytes (blocks)" >> "$OUT"
# 2
mkdir -p "$DST" || fail "mkdir"
( cd "$SRC" && tar -cf - . ) | xz -6 -T2 > "$DST/$NAME.tar.xz" || fail "tar | xz"
# 3
( cd "$DST" && sha256sum "$NAME.tar.xz" > PARTS.sha256 ) || fail "PARTS.sha256"
cp "$L" "$DST/MEMBERS.sha256" || fail "MEMBERS.sha256"
echo "2-3. archive $DST/$NAME.tar.xz $(stat -c %s "$DST/$NAME.tar.xz") bytes sha256 $(cut -c1-64 "$DST/PARTS.sha256")" >> "$OUT"
# 4
xz -t "$DST/$NAME.tar.xz" || fail "xz -t"
tar -xJf "$DST/$NAME.tar.xz" --to-command='printf "%s  %s\n" "$(sha256sum | cut -c1-64)" "$TAR_FILENAME"' > "$DST/.rt" 2>> "$OUT" || fail "streaming the members"
cmp -s <(LC_ALL=C sort "$DST/.rt") <(LC_ALL=C sort "$L") || { diff <(LC_ALL=C sort "$DST/.rt") <(LC_ALL=C sort "$L") | head -20 >> "$OUT"; rm -f "$DST/.rt"; fail "round trip differs from the list"; }
n=$(grep -c . "$DST/.rt"); rm -f "$DST/.rt"
cmp -s <(tar -tJf "$DST/$NAME.tar.xz" | grep -v '/$' | LC_ALL=C sort) <(cd "$SRC" && find . -type f | LC_ALL=C sort) || fail "member names differ from the files on disk"
echo "4. round trip: $n members streamed from the archive, every sha256 equal to the list line of the same path; member names = files on disk" >> "$OUT"
# 5
rm -rf "$SRC" || fail "rm"
[ ! -e "$SRC" ] || fail "the directory is still there"
echo "5. directory $SRC removed $(date -u +%FT%TZ)" >> "$OUT"
echo "PASS" >> "$OUT"
cat "$OUT"
