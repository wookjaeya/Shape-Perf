#!/usr/bin/env bash
# selftest.sh SCRATCH -- archive.sh on made-up directories (results 26.8; the self test of records/closure-space-20261119
# with the tiers of this step): one that matches its list and is a tier A line (PASS, removed), one whose file differs
# from its list (FAIL at step 1, kept), one that is a line of another tier (refused, rc 2, kept), one with a symbolic
# link (FAIL at step 1, kept), one whose list writes the paths without the leading ./ (PASS, removed).
set -uo pipefail
S=${1:?scratch}; H="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
rm -rf "$S/st"; mkdir -p "$S/st/src/t1/a" "$S/st/src/t2" "$S/st/src/t3" "$S/st/src/t4" "$S/st/src/t5/b" "$S/st/dst"
echo x > "$S/st/src/t1/a/f1"; printf 'y\n' > "$S/st/src/t1/f2"; echo z > "$S/st/src/t2/f3"; echo w > "$S/st/src/t3/f4"
echo v > "$S/st/src/t4/f5"; echo u > "$S/st/src/t5/b/f6"; echo s > "$S/st/src/t5/f7"
for t in t1 t2 t3 t4; do (cd "$S/st/src/$t" && find . -type f | LC_ALL=C sort | xargs sha256sum) > "$S/st/l_$t.sha256"; done
(cd "$S/st/src/t5" && find . -type f | LC_ALL=C sort | sed 's#^\./##' | xargs sha256sum) > "$S/st/l_t5.sha256"
echo changed > "$S/st/src/t2/f3"; ln -s f5 "$S/st/src/t4/link"
printf 'seq\ttier\tdir\n1\tA\tredesign/obs1/t1\n2\tA\tredesign/obs1/t2\n3\t1\tredesign/obs1/t3\n4\tA\tredesign/obs1/t4\n5\tA\tredesign/obs1/t5\n' > "$S/st/chosen.tsv"
export ARCHIVE_SRC_ROOT=$S/st/src ARCHIVE_DST_ROOT=$S/st/dst ARCHIVE_CHOSEN=$S/st/chosen.tsv
r=()
for t in t1 t2 t3 t4 t5; do bash "$H/archive.sh" $t "$S/st/l_$t.sha256" > /dev/null 2>&1; r+=($?); done
ok=0
[ "${r[0]}" = 0 ] && [ ! -e "$S/st/src/t1" ] && [ -s "$S/st/dst/t1/t1.tar.xz" ] && (cd "$S/st/dst/t1" && sha256sum -c --quiet PARTS.sha256) && ok=$((ok + 1))
[ "${r[1]}" = 1 ] && [ -e "$S/st/src/t2/f3" ] && ok=$((ok + 1))
[ "${r[2]}" = 2 ] && [ -e "$S/st/src/t3/f4" ] && [ ! -e "$S/st/dst/t3" ] && ok=$((ok + 1))
[ "${r[3]}" = 1 ] && [ -e "$S/st/src/t4/f5" ] && ok=$((ok + 1))
[ "${r[4]}" = 0 ] && [ ! -e "$S/st/src/t5" ] && (cd "$S/st/dst/t5" && sha256sum -c --quiet PARTS.sha256 && cmp -s MEMBERS.sha256 "$S/st/l_t5.sha256") && ok=$((ok + 1))
echo "selftest: case 1 (matches, archived and removed) rc=${r[0]}; case 2 (differs, kept) rc=${r[1]}; case 3 (a line of another tier, refused) rc=${r[2]}; case 4 (symbolic link, kept) rc=${r[3]}; case 5 (list without ./, archived and removed) rc=${r[4]}; $ok of 5 pass"
rm -f "$H/archive/roundtrip_t1.txt" "$H/archive/roundtrip_t2.txt" "$H/archive/roundtrip_t3.txt" "$H/archive/roundtrip_t4.txt" "$H/archive/roundtrip_t5.txt"; rm -rf "$S/st"
[ $ok = 5 ]
