"""gdb script (follow-up E1-B): record which DSO's code runs at each entry / wrapper / compute call.

Loaded with `gdb -batch -nx -x scripts/gdb_identity_trace.py --args python -m shapeperf.identity ...`.
Environment:
  IDENTITY_TRACE_OUT   JSON-lines output (one record per breakpoint hit, in execution order)
  IDENTITY_SYMBOLS     comma-separated list of kind=symbol, e.g.
                       entry=run_main_graph_model,wrapper=_mlir_ciface_main_graph_model,compute=main_graph_model
  IDENTITY_REPO        repository root (to import shapeperf.elfinfo)

For every hit it records the program counter, the DSO that contains it (gdb.solib_name), the symbol
and offset (`info symbol`), the module base from /proc/<pid>/maps, the module-relative address and
the SHA-256 of the function bytes read from process memory. The run keeps the default address-space
randomization (set disable-randomization off). Breakpoints make the run slow: never time it.
"""
import hashlib
import json
import os
import re
import sys

import gdb

sys.path.insert(0, os.environ["IDENTITY_REPO"])
from shapeperf import elfinfo  # noqa: E402

OUT = os.environ["IDENTITY_TRACE_OUT"]
SYMBOLS = [kv.split("=", 1) for kv in os.environ["IDENTITY_SYMBOLS"].split(",") if kv]
_elf_cache = {}
_seq = [0]
SYM_RE = re.compile(r"^(\S+)(?: \+ (\d+))? in section (\S+)(?: of (.+))?$")


def _elf(path):
    if path not in _elf_cache:
        _elf_cache[path] = elfinfo.read_elf(path)
    return _elf_cache[path]


def _module_base(pid, path):
    real = os.path.realpath(path)
    with open(f"/proc/{pid}/maps") as f:
        for line in f:
            parts = line.split()
            if len(parts) >= 6 and os.path.realpath(parts[5]) == real and int(parts[2], 16) == 0:
                return int(parts[0].split("-")[0], 16)
    return None


class IdentityBreakpoint(gdb.Breakpoint):
    def __init__(self, kind, symbol):
        super().__init__(symbol, internal=False)
        self.kind, self.symbol = kind, symbol

    def stop(self):
        rec = {"seq": _seq[0], "kind": self.kind, "breakpoint_symbol": self.symbol}
        _seq[0] += 1
        try:
            pid = gdb.selected_inferior().pid
            pc = int(gdb.newest_frame().pc())
            solib = gdb.solib_name(pc)
            text = gdb.execute(f"info symbol {pc:#x}", to_string=True).strip()
            rec.update({"pid": pid, "pc": hex(pc), "solib": solib, "info_symbol": text})
            m = SYM_RE.match(text)
            if m and solib:
                name, off = m.group(1), int(m.group(2) or 0)
                sym = elfinfo.find_symbol(_elf(solib), name, "any")
                start = pc - off
                base = _module_base(pid, solib)
                rec.update({"symbol": name, "offset_in_symbol": off, "function_start": hex(start),
                            "module_base_maps": hex(base) if base is not None else None,
                            "module_relative_start": hex(start - base) if base is not None else None,
                            "elf_symbol_value": hex(sym["value"]) if sym else None,
                            "elf_symbol_size": sym["size"] if sym else None})
                if sym and sym["size"]:
                    mem = bytes(gdb.selected_inferior().read_memory(start, sym["size"]))
                    rec["executed_function_sha256"] = hashlib.sha256(mem).hexdigest()
        except Exception as e:           # record, never stop the inferior because of the tracer
            rec["tracer_error"] = repr(e)
        with open(OUT, "a") as f:
            f.write(json.dumps(rec) + "\n")
        return False


gdb.execute("set pagination off")
gdb.execute("set confirm off")
gdb.execute("set breakpoint pending on")
gdb.execute("set disable-randomization off")
gdb.execute("set startup-with-shell off")
for kind, symbol in SYMBOLS:
    IdentityBreakpoint(kind, symbol)
gdb.execute("run")
