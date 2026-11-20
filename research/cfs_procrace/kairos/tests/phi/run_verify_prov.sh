#!/bin/sh
# run_verify_prov.sh <kz-phi> <bundle-root> <profile-dir>...
# Re-reads every citation of the profile under the bundle.  Exit status 77 = skipped (no bundle).
kzphi="$1"; root="$2"; shift 2
if [ ! -d "$root" ]; then
  echo "skip: the pinned bundle is not at $root"
  exit 77
fi
exec "$kzphi" verify-prov --root "$root" "$@"
