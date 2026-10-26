#!/usr/bin/env bash
# hook_census.sh -- where the hook calls of a kz-instr-clang build ended up in the machine code (x86-64, objdump).
# For every call of kz_rt_obs / kz_rt_obs_str / kz_rt_obsv / kz_rt_ctl it prints the containing function and the
# site and bind constants loaded into edi and esi before the call in the same function (the hook ABI's first two
# arguments).  A bind that appears in more than one place is duplicated by the compiler (unrolling, cloning, or
# inlining into several callers); a bind that does not appear was removed; a bind found in a function other than
# the one the source names was inlined there.  Calls whose constants are not found are printed as unresolved.
#   hook_census.sh <binary>...
set -euo pipefail
for b in "$@"; do
  objdump -d --no-show-raw-insn -w "$b" | awk -v bin="$(basename "$b")" '
    function hex(s,   i, n, c) { n = 0; s = tolower(s); sub(/^0x/, "", s); for (i = 1; i <= length(s); i++) { c = index("0123456789abcdef", substr(s, i, 1)) - 1; n = n * 16 + c }; return n }
    /^[0-9a-f]+ <.*>:$/ { fn = $2; gsub(/[<>:]/, "", fn); site = ""; bind = ""; next }
    /mov +\$0x[0-9a-f]+,%edi$/ { v = $NF; sub(/^\$/, "", v); sub(/,%edi$/, "", v); site = v; next }
    /mov +\$0x[0-9a-f]+,%esi$/ { v = $NF; sub(/^\$/, "", v); sub(/,%esi$/, "", v); bind = v; next }
    /call.*<kz_rt_(obs|obs_str|obsv|ctl)(@plt)?>/ {
      h = $0; sub(/.*<kz_rt_/, "kz_rt_", h); sub(/(@plt)?>.*/, "", h)
      if (site == "" || bind == "") printf "%s %s %s unresolved\n", bin, fn, h
      else printf "%s %s %s site=%d bind=%d\n", bin, fn, h, hex(site), hex(bind)
      site = ""; bind = ""
    }'
done
