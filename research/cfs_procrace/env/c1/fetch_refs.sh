#!/usr/bin/env bash
# fetch_refs.sh -- source copies of the C1 P-REC builds (plan/C1_build_manifest.md section 1, DV-P4), reconstructed on
# 2026-10-29 after the session container was replaced and the original c1_build/ scripts were lost (env/REBUILD_20261029.md).
#
#   fetch_refs.sh [REFS_DIR] [C1_DIR]     defaults /home/user/work/procrace/refs, /home/user/work/procrace/c1_build
#
# What the references fix, and where:
#   - blobless clones in refs/ and git worktrees at the pinned SHAs:  plan/C1_basis_plan.md section 13.2
#     ("git clone --filter=blob:none --no-checkout ..." + "git -C refs/<repo> worktree add --detach <path> <sha>");
#     the nine extra clones (ci_lab sch_lab to_lab sample_app sample_lib cFS-GroundSystem elf2cfetbl tblCRCTool tinycbor):
#     plan/C1_build_manifest.md section 4.4; PSP remote jph with techdev-iodriver and jph-unsock-pspmod: C1_basis_plan.md
#     section 13.2 (manifest comment line)
#   - SHAs: plan/C1_build_manifest.md section 1 (BP b051eb0, CF 281a941 / 833fdbb, bplib d4c6db9, tinycbor v0.6.0 =
#     d393c16f, bundles 1df262a / 1b0b338, their gitlinks, PSP bd4a2ea instead of the CI merge = DV-P4)
#   - layout: c1_build/bundle_<cand> (= CI ./cfs-bundle), c1_build/<pre|post>/src/{bp,bplib,cf} (= CI ./src, CI L45-L46,
#     L54-L71), c1_build/tinycbor-source (bplib d4c6db9 .github/actions/setup-tinycbor/action.yml L18-L24); manifest
#     section 3 and derive_c1.sh prepare (tar of c1_build/bundle_<cand> and c1_build/<cfg>/src)
# Reconstruction choices (the manifest does not fix them; marked RC-n in env/REBUILD_20261029.md):
#   RC-1  every bundle submodule path (all eleven gitlinks of the bundle commit, as actions/checkout submodules: true
#         checks out) is a worktree of the matching refs/ clone at the gitlink SHA, psp at bd4a2ea (DV-P4).  The manifest
#         says only "git worktree" of the refs/ clones; this keeps each module a git work tree so that cFE's
#         generate_git_module_version.cmake (git describe --tags --always --dirty) runs in every module directory.
#   RC-2  clone order and the remote names (origin for the nasa/intel repository, jph for jphickey/PSP).
set -euo pipefail
REFS=${1:-/home/user/work/procrace/refs}
C1=${2:-/home/user/work/procrace/c1_build}
mkdir -p "$REFS" "$C1"

clone() {  # <github owner/repo> <dir name>
  [ -d "$REFS/$2" ] || git clone -q --filter=blob:none --no-checkout "https://github.com/$1" "$REFS/$2"
}
for r in nasa/bp:bp nasa/CF:CF nasa/cFS:cFS nasa/cFE:cFE nasa/osal:osal nasa/PSP:PSP nasa/bplib:bplib \
         nasa/ci_lab:ci_lab nasa/sch_lab:sch_lab nasa/to_lab:to_lab nasa/sample_app:sample_app nasa/sample_lib:sample_lib \
         nasa/cFS-GroundSystem:cFS-GroundSystem nasa/elf2cfetbl:elf2cfetbl nasa/tblCRCTool:tblCRCTool intel/tinycbor:tinycbor; do
  clone "${r%%:*}" "${r##*:}"
done
if ! git -C "$REFS/PSP" remote get-url jph > /dev/null 2>&1; then
  git -C "$REFS/PSP" remote add jph https://github.com/jphickey/PSP.git
  git -C "$REFS/PSP" fetch -q jph techdev-iodriver:techdev-iodriver jph-unsock-pspmod:jph-unsock-pspmod
fi

wt() {  # <repo dir> <path> <sha>
  if [ -e "$2/.git" ]; then
    [ "$(git -C "$2" rev-parse HEAD)" = "$3" ] || { echo "ERROR: $2 exists at another commit" >&2; exit 3; }
    return 0
  fi
  git -C "$REFS/$1" worktree add -q --detach "$2" "$3"
}

PSP_SHA=bd4a2ea239cff013b24bf93a5e8c451dbcb7cf97     # DV-P4: the merge result of a260e9cc + techdev-iodriver (fast-forward)
for cand in 1df262a64a09f128357482d8f2e06b0fdfe84d37 1b0b338fd14030038fe1aa0e873c485bf5c851e2; do
  b="$C1/bundle_${cand:0:7}"
  wt cFS "$b" "$cand"
  # gitlinks of the bundle commit: "<sha> <path>"; .gitmodules maps the path to the repository name
  git -C "$REFS/cFS" ls-tree -r "$cand" | awk '$2 == "commit" {print $3, $4}' | while read -r sha path; do
    url=$(git -C "$REFS/cFS" config --blob "$cand:.gitmodules" --get-regexp '^submodule\..*\.path$' | awk -v p="$path" '$2 == p {print $1}' \
          | sed 's/\.path$/.url/' | xargs -I{} git -C "$REFS/cFS" config --blob "$cand:.gitmodules" --get {})
    repo=$(basename "$url" .git)
    case "$repo" in cFE) d=cFE ;; osal) d=osal ;; PSP) d=PSP; sha=$PSP_SHA ;; *) d=$repo ;; esac
    rmdir "$b/$path" 2> /dev/null || true
    wt "$d" "$b/$path" "$sha"
  done
done

for cfg in pre:281a94188cd7d885a5aed01ee041f3ece2b0486f post:833fdbb27a26f15f2429e79392e7eb73d61abdea; do
  s="$C1/${cfg%%:*}/src"; mkdir -p "$s"
  wt bp    "$s/bp"    b051eb0d12bfe76c0230215c2ff851cfaa553ce4
  wt bplib "$s/bplib" d4c6db9f0d197f78967455fbc83bf1c5e5c31c9c
  wt CF    "$s/cf"    "${cfg##*:}"
done
wt tinycbor "$C1/tinycbor-source" d393c16f3eb30d0c47e6f9d92db62272f0ec4dc7   # tag v0.6.0 (CI L30-L37)

# record: every worktree and its HEAD, and the state git describe will see
{
  echo "# fetch_refs.sh $(date -u +%FT%TZ)"
  for d in "$C1"/bundle_* "$C1"/bundle_*/{cfe,osal,psp,apps/*,libs/*,tools/*} "$C1"/{pre,post}/src/* "$C1/tinycbor-source"; do
    [ -e "$d/.git" ] || continue
    echo "${d#$C1/} $(git -C "$d" rev-parse HEAD) describe=$(git -C "$d" describe --tags --always --dirty 2>/dev/null)"
  done
} | tee "$C1/fetch_refs.record"
