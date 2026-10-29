#!/usr/bin/env bash
# regress_static.sh <old kz build> <new kz build> <out> [<cfg>_<cand>...] -- kz-static basis and extract of the C1 closures
# (env/c1/kz_run.sh closure: its recipe closure.kzb, profile, operations and targets) with the kz-static of two builds,
# into <out>; the containers are compared as kz-dump text.  For the kz-static changes of 2026-10-29 (recipe relocatable,
# the constant evaluation in the command's directory, ELF32, front_end_diagnostic), which must leave a C1 basis and its
# facts as they were.  A tool's own identity differs between the two builds by construction (its source identity and
# schema fingerprint go into the tool hash), and so do the artifact hashes that cover it and the basis hash the facts
# name.  Those lines alone are masked (MASK: each pattern anchored to the one line format that carries such a value);
# every other line must be equal, and the masked comparison says so by a count of 0.  The containers are also compared
# as bytes.  Tool check only.  Default closures: pre_1df262a post_1df262a.
set -uo pipefail
OLD=${1:?usage: regress_static.sh <old kz build> <new kz build> <out> [<cfg>_<cand>...]}; NEW=${2:?new}; O=${3:?out}; shift 3
R="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
P="$R/kairos/profiles/cfs-v7.0.1"
C1=/home/user/work/procrace/c1_build
mkdir -p "$O"
REC="$O/regress_static.record"
: > "$REC"
log() { echo "$*" | tee -a "$REC"; }
log "# regress_static.sh $(date -u +%FT%TZ): old=$OLD new=$NEW profile=$P"
for b in "$OLD" "$NEW"; do log "kz-static $(sha256sum "$b/tools/kz-static/kz-static" | cut -c1-64) ($b)"; done
TG=(--target OS_common_record_t.name_entry --target OS_common_record_t.active_id --target OS_count_sem_internal_record_t.obj_name
    --target CF_Channel_t.sem_id --target CF_ChannelConfig_t.sem_name --target BP_Throttle_t.Name --target BP_Throttle_t.Sem)
OPS=(--delay-op OS_TaskDelay --lock-op CFE_ES_LockSharedData --unlock-op CFE_ES_UnlockSharedData --spawn-op CFE_ES_CreateChildTask)
# identity: the tool hash, the build identity string (its schema fingerprint and source identity; and so the STRINGS
# section's crc, whose strings are printed in the records that use them), the artifact hashes, and the basis hash the
# facts name (basis_sha256, parent, extract.units_from, the first line of each log)
MASK=(-e 's/^(tool_sha256|artifact_sha256|basis_sha256)=[0-9a-f]{64}$/\1=H/'
      -e 's/^(  parent\[[0-9]+\] kind=[a-z]+ role=[0-9]+ sha256=)[0-9a-f]{64}$/\1H/'
      -e 's/^(  BASIS\[[0-9]+\] name="(front_end|extract)\.build_identity" value="kz-core abi=[0-9]+ schema=)[0-9a-f]{64} source=[0-9a-f]{64} /\1H source=H /'
      -e 's/^(section STRINGS type=[0-9]+ version=[0-9]+ record=kz_byte record_size=1 count=[0-9]+ crc32=0x)[0-9a-f]{1,8}$/\1H/'
      -e 's/^(  BASIS\[[0-9]+\] name="extract\.units_from" value="basis )[0-9a-f]{16}" /\1H" /'
      -e 's/^(basis|facts) [0-9a-f]{64}$/\1 H/')
LIST=("$@"); [ ${#LIST[@]} -gt 0 ] || LIST=(pre_1df262a post_1df262a)
for cc in "${LIST[@]}"; do
  for x in $cc; do
    cfg=${x%_*}; cand=${x#*_}
    B=$C1/$cfg/$cand/build
    for w in old new; do
      b=$OLD; [ "$w" = new ] && b=$NEW
      K="$b/tools/kz-static/kz-static"
      "$K" basis --profile "$P" --compile-db "$B/native/default/compile_commands.json" --build-dir "$B" --bundle-root "$C1/bundle_$cand" \
          --recipe "$C1/kz/$x/closure.kzb" -o "$O/$x.$w.basis.kz" > "$O/$x.$w.basis.log" 2>&1; a=$?
      "$K" extract --profile "$P" --basis "$O/$x.$w.basis.kz" "${OPS[@]}" "${TG[@]}" -o "$O/$x.$w.facts.kz" > "$O/$x.$w.extract.log" 2>&1; e=$?
      log "$x $w: basis rc=$a extract rc=$e"
      for k in basis facts; do "$b/tools/kz-dump/kz-dump" "$O/$x.$w.$k.kz" > "$O/$x.$w.$k.dump" 2>&1; done
    done
    for k in basis.log extract.log basis.dump facts.dump; do
      a="$O/$x.old.$k"; c="$O/$x.new.$k"
      sed -E "s#\.(old|new)\.#.W.#g" "$a" > "$O/.a"; sed -E "s#\.(old|new)\.#.W.#g" "$c" > "$O/.b"
      if cmp -s "$O/.a" "$O/.b"; then log "  $k: identical ($(wc -l < "$O/.a") lines)"; continue; fi
      diff "$O/.a" "$O/.b" > "$O/$x.$k.diff"
      sed -E "${MASK[@]}" "$O/.a" > "$O/.am"; sed -E "${MASK[@]}" "$O/.b" > "$O/.bm"
      d=$(diff "$O/.am" "$O/.bm" | grep -cE '^[<>]')
      log "  $k: $(wc -l < "$O/.a") lines, $(grep -cE '^[<>]' "$O/$x.$k.diff") differ; with the identity lines masked: $d differ"
      [ "$d" = 0 ] || diff "$O/.am" "$O/.bm" > "$O/$x.$k.masked.diff"
    done
    for k in basis facts; do
      r=$(cmp -l "$O/$x.old.$k.kz" "$O/$x.new.$k.kz" 2>/dev/null | wc -l)
      log "  $k.kz bytes: sizes $(stat -c%s "$O/$x.old.$k.kz") / $(stat -c%s "$O/$x.new.$k.kz"); $r bytes differ"
    done
  done
done
rm -f "$O/.a" "$O/.b" "$O/.am" "$O/.bm"
