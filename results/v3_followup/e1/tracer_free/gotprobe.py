"""Tracer-free in-process GOT probe (independent of shapeperf tooling).

Usage: python gotprobe.py <case_json_path>
case json: {"case_id":..., "artifacts": {name: {"path":..., "tag": str|None}},
            "steps": [["load", name] | ["call", name], ...]}

For every loaded artifact, after every step, reads the process-memory value of the
.got.plt / .got slots (R_X86_64_JUMP_SLOT / R_X86_64_GLOB_DAT) of that artifact and maps
every value to (DSO path, section, symbol+off) using /proc/self/maps + the on-disk ELF.
No ptrace, no gdb, no LD_DEBUG, no LD_AUDIT.
"""
import ctypes
import hashlib
import json
import os
import re
import subprocess
import sys

import numpy as np


def sh(*a):
    return subprocess.run(a, check=True, capture_output=True, text=True).stdout


def elf_info(path):
    syms = []
    for ln in sh("readelf", "-W", "--dyn-syms", path).splitlines():
        m = re.match(r"\s*\d+:\s+([0-9a-f]+)\s+(\d+)\s+(\w+)\s+(\w+)\s+(\w+)\s+(\w+)\s+(\S+)", ln)
        if m:
            val, size, typ, bind, vis, ndx, name = m.groups()
            syms.append(dict(value=int(val, 16), size=int(size), type=typ, bind=bind, ndx=ndx,
                             name=name.split("@")[0]))
    relocs = []
    for ln in sh("readelf", "-rW", path).splitlines():
        m = re.match(r"([0-9a-f]{16})\s+[0-9a-f]+\s+(R_X86_64_(?:JUMP_SLOT|GLOB_DAT|64))\s+([0-9a-f]+)\s+(\S+)", ln)
        if m:
            off, typ, _v, name = m.groups()
            relocs.append(dict(offset=int(off, 16), type=typ, name=name.split("@")[0]))
    secs = []
    for ln in sh("readelf", "-SW", path).splitlines():
        m = re.match(r"\s*\[\s*\d+\]\s+(\S+)\s+\S+\s+([0-9a-f]{16})\s+([0-9a-f]+)\s+([0-9a-f]+)", ln)
        if m:
            name, addr, off, size = m.groups()
            secs.append(dict(name=name, addr=int(addr, 16), offset=int(off, 16), size=int(size, 16)))
    return dict(syms=syms, relocs=relocs, secs=secs)


def file_bytes(path, info, vaddr, size):
    for s in info["secs"]:
        if s["addr"] and s["addr"] <= vaddr < s["addr"] + s["size"]:
            with open(path, "rb") as f:
                f.seek(s["offset"] + (vaddr - s["addr"]))
                return f.read(size)
    raise ValueError("no section")


def read_maps():
    out = []
    with open("/proc/self/maps") as f:
        for ln in f:
            parts = ln.split()
            lo, hi = (int(x, 16) for x in parts[0].split("-"))
            off = int(parts[2], 16)
            path = parts[5] if len(parts) >= 6 else ""
            out.append((lo, hi, off, path))
    return out


def module_base(maps, path):
    rp = os.path.realpath(path)
    bases = [lo - off for lo, hi, off, p in maps if p and os.path.realpath(p) == rp and off == 0]
    return min(bases) if bases else None


def main():
    case = json.load(open(sys.argv[1]))
    arts = case["artifacts"]
    infos = {n: elf_info(a["path"]) for n, a in arts.items()}
    path2name = {os.path.realpath(a["path"]): n for n, a in arts.items()}
    # function bytes of each artifact's compute fn from disk (for byte identity)
    disk_compute = {}
    for n, a in arts.items():
        tag = a["tag"] or "model"
        s = [x for x in infos[n]["syms"] if x["name"] == f"main_graph_{tag}" and x["ndx"] != "UND"][0]
        disk_compute[n] = hashlib.sha256(file_bytes(a["path"], infos[n], s["value"], s["size"])).hexdigest()

    sys.path.insert(0, "/home/user/work/onnx-mlir/build/Release/lib")
    from PyRuntime import OMExecutionSession

    sessions = {}
    x_in = {}
    expected = {}
    loaded = []
    snapshots = []
    calls = []

    def describe(addr, maps):
        for lo, hi, off, p in maps:
            if lo <= addr < hi:
                rp = os.path.realpath(p) if p else p
                d = dict(addr=hex(addr), dso=rp or "[anon]")
                if rp in path2name:
                    n = path2name[rp]
                    d["artifact"] = n
                    base = module_base(maps, rp)
                    rel = addr - base
                    d["rel"] = hex(rel)
                    for s in infos[n]["secs"]:
                        if s["addr"] and s["addr"] <= rel < s["addr"] + s["size"]:
                            d["section"] = s["name"]
                    best = None
                    for s in infos[n]["syms"]:
                        if s["ndx"] != "UND" and s["value"] <= rel < s["value"] + max(s["size"], 1):
                            best = f'{s["name"]}+{rel - s["value"]:#x}'
                    d["sym"] = best
                return d
        return dict(addr=hex(addr), dso="<unmapped>")

    WATCH_JS = None  # all JUMP_SLOT relocs of the artifact
    def snapshot(label):
        maps = read_maps()
        snap = {"after": label, "modules": {}}
        for n in loaded:
            a = arts[n]
            base = module_base(maps, a["path"])
            tag = a["tag"] or "model"
            slots = {}
            for r in infos[n]["relocs"]:
                if r["type"] not in ("R_X86_64_JUMP_SLOT", "R_X86_64_GLOB_DAT"):
                    continue
                val = ctypes.c_uint64.from_address(base + r["offset"]).value
                d = describe(val, maps)
                d["reloc"] = r["type"]
                slots[r["name"]] = d
            snap["modules"][n] = {"base": hex(base), "path": os.path.realpath(a["path"]), "tag": tag, "slots": slots}
        snapshots.append(snap)
        return snap

    def chain(n, snap):
        """entry(n) -> n's _mlir_ciface slot -> W; W's main_graph slot -> C."""
        tag = arts[n]["tag"] or "model"
        m = snap["modules"][n]["slots"]
        cif = m[f"_mlir_ciface_main_graph_{tag}"]
        W = cif.get("artifact")
        res = {"entry_dso": n, "ciface_slot_of_entry": cif}
        if W is None or cif.get("section") in (".plt", ".plt.sec"):
            res["error"] = "ciface slot not resolved to a function"
            return res
        mg = snap["modules"][W]["slots"][f"main_graph_{tag}"]
        res["wrapper_dso"] = W
        res["main_graph_slot_of_wrapper_dso"] = mg
        C = mg.get("artifact")
        res["compute_dso"] = C
        # hash bytes actually present at the resolved compute address in process memory
        if C is not None and mg.get("sym", "").startswith(f"main_graph_{tag}+0x0"):
            size = [s for s in infos[C]["syms"] if s["name"] == f"main_graph_{tag}" and s["ndx"] != "UND"][0]["size"]
            buf = ctypes.string_at(int(mg["addr"], 16), size)
            h = hashlib.sha256(buf).hexdigest()
            res["compute_mem_sha256"] = h
            res["compute_bytes_match"] = sorted(k for k, v in disk_compute.items() if v == h)
        return res

    # own dlsym check (same call the runtime makes: dlsym(handle, "run_main_graph_<tag>"))
    libdl = ctypes.CDLL(None)
    libdl.dlopen.restype = ctypes.c_void_p
    libdl.dlopen.argtypes = [ctypes.c_char_p, ctypes.c_int]
    libdl.dlsym.restype = ctypes.c_void_p
    libdl.dlsym.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
    RTLD_LAZY, RTLD_NOLOAD = 0x1, 0x4

    for step in case["steps"]:
        op, n = step
        a = arts[n]
        if op == "load":
            kw = {"shared_lib_path": a["path"]}
            if a["tag"]:
                kw["tag"] = a["tag"]
            sessions[n] = OMExecutionSession(**kw)
            d = os.path.dirname(os.path.dirname(a["path"]))
            x_in[n] = np.load(os.path.join(d, "input.npy"))
            expected[n] = np.load(os.path.join(d, "expected.npy"))
            loaded.append(n)
            snapshot(f"load {n}")
        elif op == "call":
            outs = sessions[n].run([x_in[n]])
            ok = bool(np.array_equal(outs[0], expected[n]) and outs[0].dtype == expected[n].dtype
                      and outs[0].shape == expected[n].shape)
            snap = snapshot(f"call {n} #{len(calls)}")
            c = chain(n, snap)
            c.update({"call_index": len(calls), "requested": n, "output_exact": ok})
            # dlsym of the entry through the handle the runtime got
            h = libdl.dlopen(a["path"].encode(), RTLD_LAZY | RTLD_NOLOAD)
            tag = a["tag"] or "model"
            ep = libdl.dlsym(h, f"run_main_graph_{tag}".encode())
            c["dlsym_entry"] = describe(ep, read_maps())
            calls.append(c)

    out = {"case_id": case["case_id"], "pid": os.getpid(), "LD_BIND_NOW": os.environ.get("LD_BIND_NOW"),
           "load_order": [s[1] for s in case["steps"] if s[0] == "load"],
           "call_order": [s[1] for s in case["steps"] if s[0] == "call"],
           "disk_compute_sha256": disk_compute, "calls": calls, "snapshots": snapshots}
    json.dump(out, sys.stdout, indent=1)


if __name__ == "__main__":
    main()
