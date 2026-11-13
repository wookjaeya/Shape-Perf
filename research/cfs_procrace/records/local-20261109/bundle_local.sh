#!/usr/bin/env bash
# bundle_local.sh -- make one LOCAL bundle outside the repository and verify it by streaming (RV4 analysis 7.2 step 2).
#
#   bundle_local.sh <id> dirs <parent> <name>... -- <note>     each <parent>/<name> is stored as <name>/... (as preserve_bundle.sh)
#   bundle_local.sh <id> list <root> <listfile> -- <note>      the files named in <listfile> (relative to <root>) are stored as named
#
# Writes /home/user/work/preserve/bundles-20261109/<id>/:
#   bundle.tar.xz.NN (45 MB parts), PARTS.sha256, MANIFEST.sha256 (sha256sum format, as preserve_bundle.sh),
#   MANIFEST.tsv (sha256, size, path per file), INFO.txt, verify.log, VERIFIED_LOCAL.txt (only when every check passed)
# Same tar and xz settings as env/tools/preserve_bundle.sh:44 (xz -9; -T4 here), split -b 45M as :45, but piped: no temp copy.
# Verification differs from env/tools/verify_bundle.sh on purpose (disk): no clone (nothing is pushed), no joined xz file,
# no extract. The parts are streamed: PARTS.sha256 -c; cat parts | xz -t; cat parts | xz -dc | tar -x --to-command=<hash>
# in an empty directory (nothing is written there; checked), and every member hash and size is compared with MANIFEST.tsv.
# Member types are listed from the same stream (tar -vv); only regular files and directories are allowed.
# The originals are checked again against MANIFEST.sha256 at the end. Nothing is deleted except this script's temp dir.
set -uo pipefail
BASE=${BUNDLE_BASE:-/home/user/work/preserve/bundles-20261109}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO=/home/user/Shape-Perf/research/cfs_procrace
FLOOR=314572800
ID=${1:?id}; MODE=${2:?mode}; shift 2
ROOT=${1:?root or parent}; shift
ITEMS=(); NOTE=""
while [ $# -gt 0 ]; do if [ "$1" = "--" ]; then shift; NOTE="$*"; break; fi; ITEMS+=("$1"); shift; done
case "$ID" in *[!A-Za-z0-9._-]*|'') echo "bad id" >&2; exit 2 ;; esac
D="$BASE/$ID"; [ ! -e "$D" ] || { echo "refusing: $D exists" >&2; exit 3; }
free() { df -B1 / | awk 'NR==2{print $4}'; }
now() { date -u +%FT%T.%3NZ; }
export LC_ALL=C
TMP="$(mktemp -d "$BASE/.tmp.$ID.XXXXXX")"; trap 'rm -rf "$TMP"' EXIT
LIST="$TMP/files"   # NUL-separated, relative to ROOT
case "$MODE" in
  dirs) for n in "${ITEMS[@]}"; do [ -d "$ROOT/$n" ] || { echo "not a dir: $ROOT/$n" >&2; exit 2; }; done
        ( cd "$ROOT" && find "${ITEMS[@]}" -type f -print0 | sort -z ) > "$LIST"
        NONREG=$( cd "$ROOT" && find "${ITEMS[@]}" ! -type f ! -type d | wc -l ) ;;
  list) LF=${ITEMS[0]:?listfile}; ( cd "$ROOT" && tr '\n' '\0' < "$LF" | sort -z ) > "$LIST"
        NONREG=$( cd "$ROOT" && tr '\n' '\0' < "$LF" | xargs -0 -I{} sh -c '[ -f "{}" ] && [ ! -L "{}" ] || echo "{}"' | wc -l ) ;;
  *) echo "mode: dirs|list" >&2; exit 2 ;;
esac
[ "$NONREG" = 0 ] || { echo "refusing: $NONREG entries are not regular files or dirs" >&2; exit 2; }
NF=$(tr -cd '\0' < "$LIST" | wc -c)
U=$( cd "$ROOT" && xargs -0 stat -c %s < "$LIST" | awk '{s+=$1} END{print s+0}' )
F0=$(free)
NEED=$(( U / 4 > 52428800 ? U / 4 : 52428800 ))
echo "start $(now) id=$ID files=$NF bytes=$U free=$F0 need_bound=$NEED floor=$FLOOR"
[ $((F0 - NEED)) -ge $FLOOR ] || { echo "SKIP: free $F0 - bound $NEED < floor $FLOOR"; exit 5; }
# credential patterns of preserve_bundle.sh:31; hits are listed by file name only
PAT='(ghp_|gho_|github_pat_|xox[bap]-|sk-[A-Za-z0-9]{20}|AKIA[0-9A-Z]{16}|-----BEGIN [A-Z ]*PRIVATE KEY|[Aa]uthorization: *(Bearer|Basic) [A-Za-z0-9])'
HITS=$( cd "$ROOT" && xargs -0 grep -IlE "$PAT" < "$LIST" 2>/dev/null | sort )
ALLOWED=$(printf '%s\n' ${ALLOW_HITS:-} | sort)
if [ -n "$HITS" ] && [ "$HITS" != "$ALLOWED" ]; then echo "refusing: credential-pattern hits:"; echo "$HITS"; exit 4; fi
mkdir "$D"
# MANIFEST from the originals
( cd "$ROOT" && xargs -0 sha256sum < "$LIST" ) > "$D/MANIFEST.sha256" || { echo "FAIL manifest"; exit 6; }
( cd "$ROOT" && xargs -0 stat -c '%s	%n' < "$LIST" ) > "$TMP/sizes" || { echo "FAIL sizes"; exit 6; }
awk -F'\t' 'NR==FNR{sz[$2]=$1; next} {h=substr($0,1,64); p=substr($0,67); print h "\t" sz[p] "\t" p}' "$TMP/sizes" "$D/MANIFEST.sha256" > "$D/MANIFEST.tsv"
[ "$(wc -l < "$D/MANIFEST.tsv")" = "$NF" ] || { echo "FAIL manifest count"; exit 6; }
# tar | xz | split, no temp copy
T0=$(now)
if [ "$MODE" = dirs ]; then
  tar --sort=name --owner=0 --group=0 --numeric-owner -cf - -C "$ROOT" "${ITEMS[@]}" | xz -9 -T4 -c | split -b 45M -d -a 2 - "$D/bundle.tar.xz."
else
  tar --sort=name --owner=0 --group=0 --numeric-owner --no-recursion -cf - -C "$ROOT" --null -T "$LIST" | xz -9 -T4 -c | split -b 45M -d -a 2 - "$D/bundle.tar.xz."
fi
PS=("${PIPESTATUS[@]}"); T1=$(now)
[ "${PS[*]}" = "0 0 0" ] || { echo "FAIL create pipeline status ${PS[*]}"; exit 7; }
( cd "$D" && sha256sum bundle.tar.xz.* > PARTS.sha256 )
C=$(cat "$D"/bundle.tar.xz.?? | wc -c); NP=$(ls "$D" | grep -c '^bundle.tar.xz\.')
WHOLE=$(cat "$D"/bundle.tar.xz.?? | sha256sum | cut -d' ' -f1)
F1=$(free)
{
  echo "bundle=$ID"
  echo "created_utc=$T1"
  echo "repo_commit=$(git -C "$REPO" rev-parse HEAD)"
  echo "kernel=$(uname -r)"
  if [ "$MODE" = dirs ]; then for n in "${ITEMS[@]}"; do echo "source=$ROOT/$n files=$(find "$ROOT/$n" -type f | wc -l) bytes=$(du -sb "$ROOT/$n" | cut -f1)"; done
  else echo "source=$ROOT (list of $NF files, MANIFEST.tsv) bytes=$U"; fi
  echo "tar_xz_bytes=$C parts=$NP tar_xz_sha256=$WHOLE"
  echo "location=$D (local, not in the repository, not pushed)"
  echo "made_with=bundle_local.sh (tar --sort=name --owner=0 --group=0 --numeric-owner | xz -9 -T4 | split -b 45M), started $T0"
  echo "df_free_before=$F0 df_free_after_create=$F1"
  echo "credential_pattern_hits=${HITS:-none}"
  echo "note=$NOTE"
} > "$D/INFO.txt"
cat "$D/INFO.txt"
# ---- streaming verification ----
V="$D/verify.log"; : > "$V"; ok=1
vlog() { echo "$*" | tee -a "$V"; }
vlog "verify_start $(now)"
( cd "$D" && sha256sum -c PARTS.sha256 ) >> "$V" 2>&1 && vlog "parts_sha256 OK $NP" || { vlog "parts_sha256 FAIL"; ok=0; }
cat "$D"/bundle.tar.xz.?? | xz -t && vlog "xz_t OK" || { vlog "xz_t FAIL"; ok=0; }
mkdir "$TMP/empty"; export HASHOUT="$TMP/members.tsv"; : > "$HASHOUT"
( cd "$TMP/empty" && cat "$D"/bundle.tar.xz.?? | xz -dc | tar -xvvf - --to-command="sh $HERE/member_sha256.sh"; echo "${PIPESTATUS[*]}" > "$TMP/ps" ) > "$TMP/listing" 2> "$TMP/tar.err"
read -r -a PS < "$TMP/ps"
vlog "stream_pipeline_status ${PS[*]} (tar stderr lines: $(wc -l < "$TMP/tar.err"))"
[ -z "$(find "$TMP/empty" -mindepth 1 | head -1)" ] && vlog "nothing_written_to_disk OK" || { vlog "nothing_written_to_disk FAIL"; ok=0; }
TYPES=$(awk '{print substr($1,1,1)}' "$TMP/listing" | sort | uniq -c | awk '{printf "%s=%s ", $2, $1}')
vlog "member_types $TYPES"
BAD=$(awk '{t=substr($1,1,1); if (t!="-" && t!="d") n++} END{print n+0}' "$TMP/listing"); [ "$BAD" = 0 ] || { vlog "member_types FAIL other types $BAD"; ok=0; }
NM=$(wc -l < "$HASHOUT"); NR_=$(awk 'substr($1,1,1)=="-"' "$TMP/listing" | wc -l)
sort -t$'\t' -k3 "$HASHOUT" > "$TMP/m.sorted"; sort -t$'\t' -k3 "$D/MANIFEST.tsv" > "$TMP/f.sorted"
NDIFF=$(diff "$TMP/m.sorted" "$TMP/f.sorted" | grep -c '^[<>]')
vlog "members_hashed=$NM regular_in_listing=$NR_ manifest_lines=$NF diff_lines=$NDIFF"
[ "$NM" = "$NF" ] && [ "$NR_" = "$NF" ] && [ "$NDIFF" = 0 ] && [ "${PS[*]}" = "0 0 0" ] || { ok=0; diff "$TMP/m.sorted" "$TMP/f.sorted" | head -20 >> "$V"; }
MH=$(sha256sum < "$TMP/m.sorted" | cut -d' ' -f1); FH=$(sha256sum < "$TMP/f.sorted" | cut -d' ' -f1)
vlog "sorted_member_list_sha256=$MH sorted_manifest_sha256=$FH"
( cd "$ROOT" && sha256sum -c --quiet "$D/MANIFEST.sha256" ) >> "$V" 2>&1 && vlog "originals_match_manifest OK" || { vlog "originals_match_manifest FAIL"; ok=0; }
F2=$(free); vlog "verify_end $(now) df_free=$F2"
if [ $ok = 1 ]; then
  {
    echo "VERIFIED_LOCAL $ID $(git -C "$REPO" rev-parse --short=12 HEAD) $(now)"
    echo "방법: 스트림 검증(디스크에 풀지 않음). PARTS.sha256 -c, cat 조각 | xz -t, cat 조각 | xz -dc | tar -x --to-command(멤버마다 sha256)."
    echo "결과: 조각 $NP개 OK, xz -t OK, 멤버 $NM개의 sha256·크기가 MANIFEST.tsv $NF줄과 같다(차이 0줄), 멤버 종류 $TYPES, 원본도 MANIFEST.sha256과 같다."
    echo "tar_xz_bytes=$C tar_xz_sha256=$WHOLE"
    echo "verify_bundle.sh와 다르다: clone 없음(push하지 않았다), 이은 xz 파일 없음, 풀기 없음. 이유는 디스크다(여유 $F0 B, 하한 $FLOOR B)."
    echo "이것은 CM §5의 '보존됨'이 아니다. 원격에서 다시 받아 확인하지 않았다. 컨테이너 손실은 막지 못한다."
  } > "$D/VERIFIED_LOCAL.txt"
  cat "$D/VERIFIED_LOCAL.txt"
else
  echo "NOT VERIFIED $ID (see $V)"; exit 8
fi
