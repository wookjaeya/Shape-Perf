#!/usr/bin/env bash
# bincheck.sh <binary>... -- the hook calls in the linked files, independent of the plugin's own check: for
# every call of kz_rt_obs / kz_rt_obs_str / kz_rt_ctl, the containing function and the binding id (the
# immediate last moved into %esi, the second argument, before the call in that function; "?" if none).
# Output: <file> <function> <hook> <bind> <call address>, then per file the number of calls per binding.
set -uo pipefail
for f in "$@"; do
  objdump -d --no-show-raw-insn "$f" | awk -v file="$(basename "$f")" '
    function hex(s,   i, c, v) { v = 0; s = tolower(s); for (i = 1; i <= length(s); i++) { c = index("0123456789abcdef", substr(s, i, 1)) - 1; v = v * 16 + c }; return v }
    /^[0-9a-f]+ <.*>:$/ { fn = $2; gsub(/[<>:]/, "", fn); esi = "?"; next }
    /mov +\$0x[0-9a-f]+,%esi/ { match($0, /\$0x[0-9a-f]+/); esi = hex(substr($0, RSTART + 3, RLENGTH - 3)); next }
    /call.*<kz_rt_(obs|obs_str|ctl)(@plt)?>/ {
      match($0, /<kz_rt_[a-z_]+/); hook = substr($0, RSTART + 1, RLENGTH - 1)
      addr = $1; sub(/:$/, "", addr)
      print file, fn, hook, esi, addr; esi = "?" }'
done
