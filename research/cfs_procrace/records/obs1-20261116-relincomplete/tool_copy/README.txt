Tool copy of 2026-11-16 (merge-check tree /home/user/work/merge-check/kz-llvm), made before the in-place rebuild of
relations rules revision 6.
- kz-static a763e255 (--identity tool_sha256 90046f5d): relation rows as records and use-site keys; it made the five
  use-site key listings under /home/user/work/redesign/obs1/relrows_check/.  The next rebuild overwrites it in place.
Lists: ORIGINAL_SHA256 (the original, path under /home/user/work), XZ_SHA256 (the .xz file), NAMES.txt (copy name, path
in the tree).  Made with xz -9 -T4 -c under env/tools/space_guard.sh (space_guard.log).  Round trip: ROUNDTRIP.txt
(xz -t, xz -dc | sha256sum = ORIGINAL_SHA256, xz -dc | cmp with the original).  Nothing in the tree was changed.
Record in the repository: research/cfs_procrace/records/obs1-20261116-relincomplete/tool_copy/.
