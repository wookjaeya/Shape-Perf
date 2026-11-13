#!/usr/bin/env bash
# q2_check.sh -- checks before the Q2 deletion (RV3A 6.6 Q2 deletion option (ga), 6.7 step 1), 2026-11-10.
# Reads only. Writes into this directory: list_<name>.tsv (sha256, bytes, path relative to the target),
# rv3_vs_ck.tsv, vb_check.txt. Deletes nothing.
#   bash q2_check.sh <scratchpad> <repo research dir> <CK>
set -u
S=${1:?scratchpad}; R=${2:?research/cfs_procrace}; CK=${3:?rv3_check}
OUT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LC_ALL=C; export LC_ALL

# 1. file lists of the five targets
for t in gaps/vb_boots gaps/vb_rebuild gaps/vb_replays gaps/vb_w6replays rv3; do
  n=$(basename "$t")
  ( cd "$S/$t" && find . -type f -printf '%P\n' | sort | while IFS= read -r p; do
      printf '%s\t%s\t%s\n' "$(sha256sum < "$p" | cut -d' ' -f1)" "$(stat -c %s "$p")" "$p"; done ) > "$OUT/list_$n.tsv"
  ( cd "$S/$t" && find . -type d -printf '%P\n' | sort ) > "$OUT/dirs_$n.txt"
  printf '%s\tfiles=%s\tdirs=%s\tother=%s\tbytes=%s\tdu_sb=%s\n' "$t" "$(wc -l < "$OUT/list_$n.tsv")" \
    "$(find "$S/$t" -type d | wc -l)" "$(find "$S/$t" ! -type f ! -type d | wc -l)" \
    "$(awk -F'\t' '{s+=$2} END{print s+0}' "$OUT/list_$n.tsv")" "$(du -sb "$S/$t" | cut -f1)"
done > "$OUT/targets.tsv"

# 2. scratchpad rv3/ against CK rv3/ (same path, or the same path + .gz with the same uncompressed bytes)
{
  printf 'path\tscratch_sha256\tck_file\tck_sha256\tresult\n'
  while IFS=$'\t' read -r h sz p; do
    if [ -f "$CK/rv3/$p" ]; then
      c=$(sha256sum < "$CK/rv3/$p" | cut -d' ' -f1); f="rv3/$p"
    elif [ -f "$CK/rv3/$p.gz" ]; then
      c=$(gzip -dc "$CK/rv3/$p.gz" | sha256sum | cut -d' ' -f1); f="rv3/$p.gz (gzip -dc)"
    else
      c=-; f=-
    fi
    d=$(awk -v k="rv3/$p" '$2==k{print $1}' "$CK/DUMP_SHA256_UNCOMPRESSED.txt")
    if [ "$c" = - ]; then r=NOT_IN_CK; elif [ "$c" = "$h" ]; then r=SAME; else r=DIFFERENT; fi
    if [ -n "$d" ]; then [ "$d" = "$h" ] && r="$r;DUMP_SHA256_UNCOMPRESSED_SAME" || r="$r;DUMP_SHA256_UNCOMPRESSED_DIFFERENT"; fi
    printf '%s\t%s\t%s\t%s\t%s\n' "$p" "$h" "$f" "$c" "$r"
  done < "$OUT/list_rv3.tsv"
  ( cd "$CK/rv3" && find . -type f -printf '%P\n' | sort ) | while IFS= read -r q; do
    b=${q%.gz}
    grep -q -P "\t\Q$q\E\$|\t\Q$b\E\$" "$OUT/list_rv3.tsv" || printf '%s\t-\trv3/%s\t%s\tONLY_IN_CK\n' "$q" "$q" "$(sha256sum < "$CK/rv3/$q" | cut -d' ' -f1)"
  done
} > "$OUT/rv3_vs_ck.tsv"
# the CK copies are checked by CK's own list too
( cd "$CK" && sha256sum -c --quiet SHA256SUMS >/dev/null 2>&1 && echo "CK SHA256SUMS: OK" || echo "CK SHA256SUMS: FAIL" ) >> "$OUT/rv3_vs_ck.tsv.note"
( cd "$(dirname "$CK")" && sha256sum -c --quiet "$R/records/ck-20261109-rv3check/MANIFEST.sha256" >/dev/null 2>&1 \
  && echo "CK against records/ck-20261109-rv3check/MANIFEST.sha256 ($(wc -l < "$R/records/ck-20261109-rv3check/MANIFEST.sha256") lines): OK" \
  || echo "CK against records/ck-20261109-rv3check/MANIFEST.sha256: FAIL" ) >> "$OUT/rv3_vs_ck.tsv.note"
# are the CK rv3/ files members of the local CK bundle with the same hashes
( cd "$CK/rv3" && find . -type f -printf '%P\n' | sort ) | while IFS= read -r q; do
  h=$(sha256sum < "$CK/rv3/$q" | cut -d' ' -f1)
  m=$(awk -v k="rv3_check/rv3/$q" '$2==k{print $1}' "$R/records/ck-20261109-rv3check/MANIFEST.sha256")
  [ "$m" = "$h" ] && echo "in bundle ck-20261109-rv3check: rv3/$q SAME" || echo "in bundle ck-20261109-rv3check: rv3/$q MISSING_OR_DIFFERENT"
done >> "$OUT/rv3_vs_ck.tsv.note"

# 3. vb_* against the committed bundle records
{
  echo "repo HEAD $(git -C "$R" rev-parse HEAD)"
  for pair in vb_boots:c5-20261029-boots vb_replays:c5-20261029-replays vb_w6replays:w6-20261029-replays vb_rebuild:rebuild-20261029-records; do
    v=${pair%%:*}; id=${pair#*:}; W="$S/gaps/$v"; B="$R/records/$id"
    echo "== $v -> $id"
    rel="research/cfs_procrace/records/$id"
    echo "clone HEAD $(git -C "$W/clone" rev-parse HEAD 2>&1)"
    echo "clone sparse-checkout: $(git -C "$W/clone" sparse-checkout list 2>&1 | tr '\n' ' ')"
    echo "clone remote branch: $(git -C "$W/clone" rev-parse --abbrev-ref HEAD 2>&1)"
    for f in INFO.txt MANIFEST.sha256 PARTS.sha256 bundle.tar.xz.00; do
      a=$(sha256sum < "$W/clone/$rel/$f" | cut -d' ' -f1); b=$(sha256sum < "$B/$f" | cut -d' ' -f1)
      [ "$a" = "$b" ] && echo "clone $f = repo records/$id/$f ($a)" || echo "clone $f DIFFERENT from repo ($a vs $b)"
    done
    j=$(sha256sum < "$W/bundle.tar.xz" | cut -d' ' -f1); k=$(cat "$B"/bundle.tar.xz.?? | sha256sum | cut -d' ' -f1)
    [ "$j" = "$k" ] && echo "joined bundle.tar.xz = cat of repo parts ($j)" || echo "joined bundle.tar.xz DIFFERENT ($j vs $k)"
    ( cd "$B" && sha256sum -c --quiet PARTS.sha256 >/dev/null 2>&1 && echo "repo PARTS.sha256: OK" || echo "repo PARTS.sha256: FAIL" )
    ( cd "$W/extract" && sha256sum -c --quiet "$B/MANIFEST.sha256" >/dev/null 2>&1 && echo "extract against repo MANIFEST.sha256: OK" || echo "extract against repo MANIFEST.sha256: FAIL" )
    nm=$(wc -l < "$B/MANIFEST.sha256"); ne=$(cd "$W/extract" && find . -type f | wc -l)
    echo "MANIFEST lines $nm, files in extract $ne"
    comm -13 <(sed 's/^[0-9a-f]\{64\}  //' "$B/MANIFEST.sha256" | sort) <(cd "$W/extract" && find . -type f -printf '%P\n' | sort) | sed 's/^/extract file not in MANIFEST: /'
    ( cd "$W" && find . -mindepth 1 -maxdepth 1 -printf 'top entry: %P\n' | sort )
    echo "VERIFIED output file of this run under $v: $(cd "$W" && find . -path ./clone -prune -o -path ./extract -prune -o -type f ! -name bundle.tar.xz -print | wc -l) (files other than clone/, extract/, bundle.tar.xz)"
  done
} > "$OUT/vb_check.txt" 2>&1
