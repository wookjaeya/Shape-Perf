Tool copies of 2026-11-16 (merge-check tree /home/user/work/merge-check/kz-llvm), made before any rebuild of that tree.
- kz-static 0024f192: the binary that made every step-2a output up to tests_8c_fix; later rebuilds overwrite it in place.
- kz_lint b0f0d6a5: rebuilt by the next lint change.
- CMakeCache.txt and the flags.make of each tool (kz, kz-static, kz-dump, kz-phi, kz-rt-dump, kz_lint, kz-instr-clang).
Lists: ORIGINAL_SHA256 (sha256 of the originals, paths under /home/user/work), XZ_SHA256 (the .xz files), NAMES.txt (copy name, path in the tree).
Made with xz -9 -T4 -c under env/tools/space_guard.sh (space_guard.log). Round trip: ROUNDTRIP.txt (xz -t, xz -dc | sha256sum = ORIGINAL_SHA256, xz -dc | cmp with the original). Nothing in the tree was changed.
Not copied here: kz 931dd39c, kz-dump 05623054 and kz-rt-dump 4390d4f1 (the last two have copies in tools-20261109), kz-phi 7d34e6e2, kz-instr-clang 28db4017 and the two plugins: none of them is rebuilt by the next steps; each is copied before the first step that overwrites it.
Record in the repository: research/cfs_procrace/records/obs1-20261116-ground/tool_copies/.
