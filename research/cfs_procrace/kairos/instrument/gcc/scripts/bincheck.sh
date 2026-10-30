#!/usr/bin/env bash
# bincheck.sh <binary>... -- the hook calls in the linked files, independent of the plugin's own check: for
# every call of kz_rt_obs / kz_rt_obs_str / kz_rt_ctl, the containing function and the binding id (the second
# argument: x86-64, the immediate last moved into %esi before the call in that function; i386 (elf32-i386, cdecl:
# the arguments are pushed last to first), the second-to-last push before the call, or the immediate stored to
# 0x4(%esp); "?" if none).
# Output: <file> <function> <hook> <bind> <call address>, then per file the number of calls per binding.
set -uo pipefail
for f in "$@"; do
  fmt=$(objdump -f "$f" | sed -n 's/.*file format //p')
  objdump -d --no-show-raw-insn "$f" | awk -v file="$(basename "$f")" -v fmt="$fmt" '
    function hex(s,   i, c, v) { v = 0; s = tolower(s); for (i = 1; i <= length(s); i++) { c = index("0123456789abcdef", substr(s, i, 1)) - 1; v = v * 16 + c }; return v }
    function imm(line) { match(line, /\$0x[0-9a-f]+/); return hex(substr(line, RSTART + 3, RLENGTH - 3)) }
    /^[0-9a-f]+ <.*>:$/ { fn = $2; gsub(/[<>:]/, "", fn); esi = "?"; p1 = "?"; p2 = "?"; next }
    fmt != "elf32-i386" && /mov +\$0x[0-9a-f]+,%esi/ { esi = imm($0); next }
    fmt == "elf32-i386" && /push +\$0x[0-9a-f]+/ { p2 = p1; p1 = imm($0); next }
    fmt == "elf32-i386" && /push / { p2 = p1; p1 = "?"; next }
    fmt == "elf32-i386" && /movl? +\$0x[0-9a-f]+,0x4\(%esp\)/ { esi = imm($0); next }
    /call.*<kz_rt_(obs|obs_str|ctl)(@plt)?>/ {
      match($0, /<kz_rt_[a-z_]+/); hook = substr($0, RSTART + 1, RLENGTH - 1)
      addr = $1; sub(/:$/, "", addr)
      bind = esi; if (fmt == "elf32-i386" && p2 != "?") bind = p2
      print file, fn, hook, bind, addr; esi = "?"; p1 = "?"; p2 = "?" }'
done
