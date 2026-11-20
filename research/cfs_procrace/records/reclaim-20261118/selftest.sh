#!/usr/bin/env bash
# selftest.sh TMPDIR -- identity_listing.py on a made-up tree: one identical candidate and five that must not be
# eligible (a differing byte, an extra file, a symbolic link, an empty directory, a hard link to the reference).
# Prints the summary with TMPDIR written as <scratch>; rc 0 when the six verdicts are the expected ones.
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
S=${1:?usage: selftest.sh TMPDIR}
[ ! -e "$S" ] || { echo "selftest.sh: $S exists" >&2; exit 64; }
mkdir -p "$S"/root/{ref,same,diff,extra,link,empty,hardlink}/b1 "$S/out" || exit 1
for d in ref same diff extra link empty hardlink; do echo aaa > "$S/root/$d/b1/x.txt"; : > "$S/root/$d/b1/x.log"; done
for d in ref same diff extra link empty; do echo more > "$S/root/$d/b1/y.txt"; done
echo aab > "$S/root/diff/b1/x.txt"; echo z > "$S/root/extra/b1/z.txt"; ln -s x.txt "$S/root/link/b1/l"
mkdir "$S/root/empty/e"; ln "$S/root/ref/b1/y.txt" "$S/root/hardlink/b1/y.txt"
printf '%s\tref\tROW_C\tROW_R\n' same diff extra link empty hardlink > "$S/pairs.tsv"
python3 "$HERE/identity_listing.py" "$S/root" "$S/pairs.tsv" "$S/out" t; rc=$?
echo "identity_listing rc $rc"
sed "s|$S|<scratch>|g" "$S/out/identity_summary_t.txt" | grep -v '^# root'
got=$(grep -v '^#' "$S/out/identity_summary_t.txt" | awk -F'\t' '{print $1 ":" ($NF == "identical-no-extra" ? "eligible" : "not")}' | tr '\n' ' ')
want="same:eligible diff:not extra:not link:not empty:not hardlink:not "
python3 "$HERE/identity_listing.py" "$S/root" "$S/pairs.tsv" "$S/out" t 2> /dev/null; rc2=$?
echo "second run into the same output names rc $rc2 (1 expected: no overwrite)"
if [ $rc -eq 0 ] && [ $rc2 -eq 1 ] && [ "$got" = "$want" ]; then echo "selftest: OK"; else echo "selftest: FAIL ($got)"; exit 1; fi
