#!/usr/bin/env bash
# selftest.sh SCRATCH -- archive.sh on a made-up directory: one that matches its list (PASS, removed), one whose file
# differs from its list (FAIL at step 1, kept).
set -uo pipefail
S=${1:?scratch}; H="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
rm -rf "$S/st"; mkdir -p "$S/st/src/t1/a" "$S/st/src/t2" "$S/st/dst"
echo x > "$S/st/src/t1/a/f1"; printf 'y\n' > "$S/st/src/t1/f2"; echo z > "$S/st/src/t2/f3"
(cd "$S/st/src/t1" && find . -type f | LC_ALL=C sort | xargs sha256sum) > "$S/st/l1.sha256"
(cd "$S/st/src/t2" && find . -type f | LC_ALL=C sort | xargs sha256sum) > "$S/st/l2.sha256"; echo changed > "$S/st/src/t2/f3"
export ARCHIVE_SRC_ROOT=$S/st/src ARCHIVE_DST_ROOT=$S/st/dst
bash "$H/archive.sh" t1 "$S/st/l1.sha256" > /dev/null; r1=$?
bash "$H/archive.sh" t2 "$S/st/l2.sha256" > /dev/null; r2=$?
ok=0
[ $r1 = 0 ] && [ ! -e "$S/st/src/t1" ] && [ -s "$S/st/dst/t1/t1.tar.xz" ] && (cd "$S/st/dst/t1" && sha256sum -c --quiet PARTS.sha256) && ok=$((ok + 1))
[ $r2 = 1 ] && [ -e "$S/st/src/t2/f3" ] && ok=$((ok + 1))
echo "selftest: case 1 (matches, archived and removed) rc=$r1; case 2 (differs, kept) rc=$r2; $ok of 2 pass"
rm -f "$H/roundtrip_t1.txt" "$H/roundtrip_t2.txt"; rm -rf "$S/st"
[ $ok = 2 ]
