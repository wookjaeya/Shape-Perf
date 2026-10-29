#!/usr/bin/env bash
# derive_c5.sh -- the cFE 6.4.1 / 6.4.2 builds of case C5 (research/cfs_procrace/plan/c5_execution_basis_20261028.md,
# "CB") as DERIVED configurations of the reference build of CB section 2.1, with the draft site manifests
# kairos/profiles/c5-cfe-6.4.x/instr/sites-64<V>.draft.kzs (CB section 8).  Pilot/dev only; no boot.
#
#   derive_c5.sh prepare <V> <W>                       the release tree of CB 2.1 (tarballs, DV-1) at <W>/pristine (once)
#   derive_c5.sh build <V> <W> ref                     the reference build, unchanged, at <W>/b (the same path as the
#                                                      derived build, so objects compare byte for byte); its tree is
#                                                      removed after the hashes are written
#   derive_c5.sh build <V> <W> keep                    the reference build at <W>/keep, kept (the input of the static
#                                                      basis, CB section 10.5)
#   derive_c5.sh build <V> <W> instr <kzgcc.so> <rt>   the derived build at <W>/b
#   derive_c5.sh check <V> <W>                         object comparison, binding merge, link-time hook check
# V 1|2.  <rt>: build_runtime.sh -m32 <rt> <the V's manifest> <kzgcc.so>.  DL: the tarball directory (default
# /home/user/work/c5/dl, hashes CB section 1.2).
#
# The commands are those of CB 2.1 (readme 6.4.1 L141-L156 / 6.4.2 L143-L158): `. ./setvars.sh`, `cd build/cpu1`,
# `make config`, `make` (no -j, no variables), run as the user of the call.  Differences of the derived configuration,
# all written into <W>/build_<variant>.record:
#   D1  work-space path <W>/b instead of the reference's own path (path strings in the objects' debug information)
#   D3  (instr) `make LOCAL_COPTS="<plugin options>"`: the variable every compile rule puts first
#       (cfe/fsw/cfe-core/src/make/make-rules.mak L17, app-rules.mak L21) and every unit's Makefile sets empty (CB 8.2):
#       build/cpu1/cfe/{psp,osal,sb,evs,time,tbl}/Makefile and build/cpu1/<app>/Makefile L36 set it empty, cfe/fs does not
#       set it, and cfe/es/Makefile L26-L30 sets -DEXCLUDE_CFE_TBL only when EXCLUDE_CFE_TBL is TRUE, which no file of the
#       build sets (the reference log has no compile line with -DEXCLUDE_CFE_TBL); `make config` is unchanged
#   D3' (instr) `make LIBS="<runtime> <the value of psp/fsw/pc-linux/make/link-rules.mak L26>"`: the core link
#       (link-rules.mak L36 `$(COMPILER) $(DEBUG_FLAGS) -o $(EXE_TARGET) $(CORE_OBJS) $(LDFLAGS) $(LIBS)`) takes the
#       runtime (whole archives libkzrt-posix.a libkzrt.a, the start object kz_rt_posix_auto.c.o and the build identity
#       object kz_rt_build_info.c.o, all -m32); no other rule reads LIBS; the apps' link (L42) is unchanged
#   D5  (instr) every object differs from the reference in its debug information: -g (debug-opts.mak) records the
#       compile options in DW_AT_producer, the plugin options among them; objects are compared also after
#       objcopy --strip-debug, where only the instrumented units (and the subsystem objects that hold them) differ
# Module 1 is core-linux.bin, module 2 sample_app.so (the manifest's module lines): units under apps/sample_app/.
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KZ="$(cd "$HERE/../../.." && pwd)"
DL=${DL:-/home/user/work/c5/dl}
CMD=${1:?usage}; V=${2:?1|2}; W=${3:?work dir}
case "$V" in 1|2) ;; *) echo "V is 1|2" >&2; exit 64 ;; esac
REL=cFE-6.4.$V-OSS-release
SITES="$KZ/profiles/c5-cfe-6.4.x/instr/sites-64$V.draft.kzs"
REF_LIBS="-lm -lpthread -ldl -lrt"  # psp/fsw/pc-linux/make/link-rules.mak L26 (checked against the tree in build)
tree_hash() { ( cd "$1" && find . -type f -print0 | LC_ALL=C sort -z | xargs -0 sha256sum ) | sha256sum | cut -c1-64; }

case "$CMD" in
prepare)
  [ ! -e "$W/pristine" ] || { echo "refusing: $W/pristine exists" >&2; exit 2; }
  mkdir -p "$W/pristine" && cd "$W/pristine" || exit 2
  tar -xzf "$DL/$REL.tar.gz" && cd "$REL" || exit 2
  rm -r osal                                   # CB 2.3 DV-1: the empty place osal/ (readme.txt only)
  tar -zxf "$DL/osal-4.1.1-release.tar.gz"      # readme L141 (6.4.2 L143), the tarball by its absolute path
  mv osal-4.1.1-release osal                   # readme L142 (6.4.2 L144)
  {
    echo "# derive_c5.sh prepare $V $(date -u +%FT%TZ): CB 2.1 (DV-1) at $W/pristine/$REL"
    ( cd "$DL" && sha256sum "$REL.tar.gz" osal-4.1.1-release.tar.gz )
    echo "tree $REL: $(find . -type f | wc -l) files, sha256 of the sorted sha256 list $(tree_hash .)"
    echo "link-rules.mak L26: $(sed -n 26p psp/fsw/pc-linux/make/link-rules.mak)"
  } > "$W/prepare.record"
  cat "$W/prepare.record" ;;
build)
  VAR=${4:?ref|keep|instr}
  case "$VAR" in ref|instr) B="$W/b" ;; keep) B="$W/keep" ;; *) echo "variant is ref|keep|instr" >&2; exit 64 ;; esac
  [ ! -e "$B" ] || { echo "refusing: $B exists" >&2; exit 2; }
  [ "$(sed -n 26p "$W/pristine/$REL/psp/fsw/pc-linux/make/link-rules.mak")" = "LIBS =  $REF_LIBS" ] || { echo "link-rules.mak L26 is not LIBS =  $REF_LIBS" >&2; exit 2; }
  cp -a "$W/pristine" "$B"
  T="$B/$REL"
  R="$W/build_$VAR.record"; L="$W/build_$VAR.log"
  MAKEVARS=()
  { echo "# derive_c5.sh build $V $VAR $(date -u +%FT%TZ) user=$(id -un) at $T; gcc: $(gcc --version | head -1); make: $(make --version | head -1)"
    echo "copy of $W/pristine: tree sha256 $(tree_hash "$T") (prepare: $(sed -n 's/.*sorted sha256 list //p' "$W/prepare.record"))"; } > "$R"
  if [ "$VAR" = instr ]; then
    PLUGIN=${5:?kzgcc.so}; RT=${6:?runtime dir}
    OUT="$W/binds_instr"; rm -rf "$OUT"; mkdir -p "$OUT"
    PF="-fplugin=$PLUGIN -fplugin-arg-kzgcc-manifest=$SITES -fplugin-arg-kzgcc-out=$OUT -fplugin-arg-kzgcc-module=2:$T/apps/sample_app/ -fplugin-arg-kzgcc-default-module=1"
    RTL="-Wl,--whole-archive $RT/libkzrt-posix.a $RT/libkzrt.a -Wl,--no-whole-archive $RT/CMakeFiles/kzrt-posix-auto.dir/src/kz_rt_posix_auto.c.o $RT/CMakeFiles/kzrt-build-info.dir/src/kz_rt_build_info.c.o"
    MAKEVARS=("LOCAL_COPTS=$PF" "LIBS=$RTL $REF_LIBS")
    { echo "D3 make LOCAL_COPTS=$PF"; echo "D3' make LIBS=$RTL $REF_LIBS"
      echo "plugin sha256 $(sha256sum "$PLUGIN" | cut -c1-64) ($(strings "$PLUGIN" | grep -m1 '^kzgcc [0-9]')) manifest sha256 $(sha256sum "$SITES" | cut -c1-64)"
      sed 's/^/runtime /' "$RT/runtime.record"; } >> "$R"
  fi
  ( cd "$T" && . ./setvars.sh && cd build/cpu1 &&
    { echo "## make config"; date -u +%FT%TZ; make config; echo "make config rc=$?"
      echo "## make"; date -u +%FT%TZ; make "${MAKEVARS[@]}"; echo "make rc=$?"; date -u +%FT%TZ; } ) > "$L" 2>&1
  grep -E '^make (config )?rc=' "$L" | tee -a "$R"
  {
    echo "compiler warnings in the build log: make config $(sed -n '/^## make config/,/^## make$/p' "$L" | grep -c 'warning:'), make $(sed -n '/^## make$/,$p' "$L" | grep -c 'warning:') (CB 2.4 counts the make part)"
    echo "compile lines (gcc ... -c): $(grep -cE '^gcc .* -c ' "$L"); with -m32: $(grep -E '^gcc .* -c ' "$L" | grep -c -- ' -m32 ')"
    echo "compile lines with -DEXCLUDE_CFE_TBL: $(grep -E '^gcc .* -c ' "$L" | grep -c -- '-DEXCLUDE_CFE_TBL')"
    echo "compile lines with -fplugin: $(grep -E '^gcc .* -c ' "$L" | grep -c -- '-fplugin=')"
    echo "core link line: $(grep -E '^gcc .* -o core-linux.bin ' "$L" | sed "s#$T#<T>#g")"
    for f in build/cpu1/exe/core-linux.bin build/cpu1/exe/cf/apps/sample_app.so build/cpu1/exe/cf/apps/sample_lib.so build/cpu1/exe/cf/apps/ci_lab.so build/cpu1/exe/cf/apps/to_lab.so build/cpu1/exe/cf/apps/sch_lab.so; do
      echo "$f sha256 $(sha256sum "$T/$f" | cut -c1-64) $(stat -c %s "$T/$f") B $(file -b "$T/$f" | cut -d, -f1-2)"; done
  } >> "$R"
  ( cd "$T/build/cpu1" && find . -name '*.o' -type f | LC_ALL=C sort | xargs sha256sum ) > "$W/objects_$VAR.sha256"
  # the same objects without debug information (D5: -g records the compile options, the plugin's among them)
  ( cd "$T/build/cpu1" && find . -name '*.o' -type f | LC_ALL=C sort | while read -r o; do
      objcopy --strip-debug "$o" "$W/.nodebug.o" && echo "$(sha256sum < "$W/.nodebug.o" | cut -c1-64)  $o"; done ) > "$W/objects_$VAR.nodebug.sha256"
  rm -f "$W/.nodebug.o"
  echo "objects: $(wc -l < "$W/objects_$VAR.sha256") (also hashed after objcopy --strip-debug: $(wc -l < "$W/objects_$VAR.nodebug.sha256"))" >> "$R"
  if [ "$VAR" = ref ]; then
    rm -rf "$B"; echo "reference build tree removed after the hashes (it was built only for the byte comparison at the same path)" >> "$R"
  fi
  cat "$R" ;;
check)
  cd "$W" || exit 2
  T="$W/b/$REL"
  join -1 2 -2 2 <(sed 's/  / /' objects_ref.sha256 | sort -k2) <(sed 's/  / /' objects_instr.sha256 | sort -k2) | awk '$2 != $3 {print $1}' > objects_differ.txt
  join -1 2 -2 2 <(sed 's/  / /' objects_ref.nodebug.sha256 | sort -k2) <(sed 's/  / /' objects_instr.nodebug.sha256 | sort -k2) | awk '$2 != $3 {print $1}' > objects_differ_nodebug.txt
  "$HERE/merge_binds.sh" binds_instr binds_instr.kzs > merge.txt
  units=$(sed -n 's/^# kzgcc.* unit=\([^ ]*\) .*/\1/p' binds_instr/*.kzs | sed "s|^$T/||" | sort)
  {
    echo "# derive_c5.sh check $V $(date -u +%FT%TZ)"
    echo "objects: ref $(wc -l < objects_ref.sha256), instr $(wc -l < objects_instr.sha256), differing: $(wc -l < objects_differ.txt) (D5: the debug information records the compile options); after objcopy --strip-debug: $(wc -l < objects_differ_nodebug.txt)"
    echo "units with sites (binding files): $(echo "$units" | grep -c .)"
    while read -r o; do
      src=$(basename "$o" .o); dir=$(basename "$(dirname "$o")")
      if echo "$units" | grep -q "/$src\.c\$"; then echo "differs without debug information (instrumented unit): $o"
      elif [ "$src" = "$dir" ] && grep -q "^\./cfe/$dir/" <(grep -v "/$dir\.o\$" objects_differ_nodebug.txt); then echo "differs without debug information (the subsystem's ld -r object, which holds an instrumented unit): $o"
      else echo "differs without debug information (UNEXPECTED): $o"; fi
    done < objects_differ_nodebug.txt
    for u in $units; do grep -q "/$(basename "$u" .c).o\$" objects_differ_nodebug.txt || echo "instrumented unit with an unchanged object: $u"; done
    cat merge.txt
    echo "manifest sites $(grep -c '^site ' "$SITES"), bindings $(grep -c '^bind ' binds_instr.kzs), sites without a binding: $(comm -23 <(sed -n 's/^site \([0-9]*\) .*/\1/p' "$SITES" | sort) <(awk '$1 == "bind" {for (i = 3; i <= NF; i++) if ($i ~ /^site=/) {sub(/site=/, "", $i); print $i}}' binds_instr.kzs | sort -u) | paste -sd' ')"
    # the value slots each binding reads (vals=<mask>) against the slots the manifest declares for its site
    echo "bindings whose vals mask is not the declared slots: $(awk 'function hex(s,   i, v) { v = 0; s = tolower(s); sub(/^0x/, "", s); for (i = 1; i <= length(s); i++) v = v * 16 + index("0123456789abcdef", substr(s, i, 1)) - 1; return v }
      FNR == NR { if ($1 == "value") m[$2] += 2 ^ $3; next }
      $1 == "bind" { site = ""; vals = 0; for (i = 3; i <= NF; i++) { if ($i ~ /^site=/) site = substr($i, 6); if ($i ~ /^vals=/) vals = hex(substr($i, 6)) }
                     if (vals != m[site] + 0) printf "%s(site %s: vals %d, declared %d) ", $2, site, vals, m[site] }' "$SITES" binds_instr.kzs)"
    "$HERE/bincheck.sh" "$T/build/cpu1/exe/core-linux.bin" "$T/build/cpu1/exe/cf/apps/sample_app.so" > bincheck.txt
    echo "hook calls: $(wc -l < bincheck.txt) ($(awk '{print $1, $3}' bincheck.txt | sort | uniq -c | paste -sd';'))"
    "$HERE/hookcheck.sh" binds_instr.kzs bincheck.txt > hookcheck.txt; echo "hookcheck rc=$? $(tail -1 hookcheck.txt)"
    grep -vE '^(ok|gap|summary)' hookcheck.txt | sed 's/^/  /'
    echo "runtime symbols in core-linux.bin (dynamic, defined): $(nm -D --defined-only "$T/build/cpu1/exe/core-linux.bin" | grep -c ' kz_rt_')"
    echo "hook symbols sample_app.so needs: $(nm -D --undefined-only "$T/build/cpu1/exe/cf/apps/sample_app.so" | awk '/kz_rt_/ {print $2}' | sort -u | paste -sd' ')"
  } > check.record
  cat check.record ;;
*) echo "usage: derive_c5.sh prepare|build|check <V> <W> [ref | keep | instr <kzgcc.so> <rt>]" >&2; exit 64 ;;
esac
