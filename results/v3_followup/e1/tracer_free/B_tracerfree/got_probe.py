"""Tracer-free GOT probe (independent of shapeperf.*, gdb, LD_DEBUG).

usage: python got_probe.py <out.json> <steps>   steps e.g. "load:S8,load:S1,call:S1,call:S8"
Arms S8/S1 map to the untagged L=64 artifacts (or override with ARM_<name>=path env vars).
After every step it snapshots, for every loaded arm:
  - load base (dl_iterate_phdr dlpi_addr) and the /proc/self/maps ranges,
  - the 8-byte GOT slot for _mlir_ciface_main_graph_model and main_graph_model (JUMP_SLOT offsets from readelf -rW),
  - the DSO each slot value points to (dladdr AND an independent /proc/self/maps lookup),
  - whether the slot still holds the unresolved lazy value (base + PLT stub + 6),
  - the entry address dlsym(handle, "run_main_graph_model") with handle = dlopen(path, RTLD_NOLOAD|RTLD_LAZY),
    and dlsym(RTLD_DEFAULT, ...) for comparison.
"""
import ctypes
import ctypes.util
import json
import os
import re
import subprocess
import sys

sys.path.insert(0, "/home/user/work/onnx-mlir/build/Release/lib")
import numpy as np  # noqa: E402

ARMS = {
    "S8": "/home/user/work/v3/g2/K/L0064/S8_full/model.so",
    "S1": "/home/user/work/v3/g2/K/L0064/S1_full/model.so",
}
for k, v in os.environ.items():
    if k.startswith("ARM_"):
        ARMS[k[4:]] = v
TAGS = {k[4:]: v for k, v in os.environ.items() if k.startswith("TAG_")}
INPUT = os.environ.get("PROBE_INPUT", "/home/user/work/v3/g2/K/L0064/input.npy")
EXPECTED = os.environ.get("PROBE_EXPECTED", "/home/user/work/v3/g2/K/L0064/expected.npy")

libc = ctypes.CDLL(None)
libc.dlopen.restype = ctypes.c_void_p
libc.dlopen.argtypes = [ctypes.c_char_p, ctypes.c_int]
libc.dlsym.restype = ctypes.c_void_p
libc.dlsym.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
libc.dlerror.restype = ctypes.c_char_p


class Dl_info(ctypes.Structure):
    _fields_ = [("dli_fname", ctypes.c_char_p), ("dli_fbase", ctypes.c_void_p),
                ("dli_sname", ctypes.c_char_p), ("dli_saddr", ctypes.c_void_p)]


libc.dladdr.argtypes = [ctypes.c_void_p, ctypes.POINTER(Dl_info)]
libc.dladdr.restype = ctypes.c_int


class dl_phdr_info(ctypes.Structure):
    _fields_ = [("dlpi_addr", ctypes.c_uint64), ("dlpi_name", ctypes.c_char_p),
                ("dlpi_phdr", ctypes.c_void_p), ("dlpi_phnum", ctypes.c_uint16)]


CB = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.POINTER(dl_phdr_info), ctypes.c_size_t, ctypes.c_void_p)
RTLD_LAZY, RTLD_NOLOAD = 0x1, 0x4
RTLD_DEFAULT = ctypes.c_void_p(0)


def tag_of(arm):
    return TAGS.get(arm) or "model"


def static_info(path, tag):
    """JUMP_SLOT offsets + PLT stub addresses + symbol values from binutils (separate processes)."""
    want = [f"_mlir_ciface_main_graph_{tag}", f"main_graph_{tag}", f"run_main_graph_{tag}"]
    rel = subprocess.run(["readelf", "-rW", path], capture_output=True, text=True, check=True).stdout
    slots = {}
    for line in rel.splitlines():
        p = line.split()
        if len(p) >= 5 and p[2] == "R_X86_64_JUMP_SLOT" and p[4] in want:
            slots[p[4]] = int(p[0], 16)
    dis = subprocess.run(["objdump", "-d", "-j", ".plt", path], capture_output=True, text=True, check=True).stdout
    plt = {}
    for m in re.finditer(r"^([0-9a-f]+) <(\S+)@plt>:", dis, re.M):
        plt[m.group(2)] = int(m.group(1), 16)
    sy = subprocess.run(["readelf", "-sW", "--dyn-syms", path], capture_output=True, text=True, check=True).stdout
    syms = {}
    for line in sy.splitlines():
        p = line.split()
        if len(p) >= 8 and p[7] in want and p[6] != "UND":
            syms[p[7]] = int(p[1], 16)
    return {"slots": slots, "plt": plt, "syms": syms}


def loaded_objects():
    out = []

    def cb(info, size, data):
        i = info.contents
        out.append((i.dlpi_name.decode() if i.dlpi_name else "", int(i.dlpi_addr)))
        return 0
    libc.dl_iterate_phdr(CB(cb), None)
    return out


def maps():
    rows = []
    with open("/proc/self/maps") as f:
        for line in f:
            p = line.split()
            lo, hi = (int(x, 16) for x in p[0].split("-"))
            rows.append((lo, hi, p[1], p[5] if len(p) >= 6 else ""))
    return rows


def maps_owner(addr, mp):
    for lo, hi, perm, path in mp:
        if lo <= addr < hi:
            return {"path": path, "perm": perm, "range": f"{lo:#x}-{hi:#x}"}
    return None


def dladdr(addr):
    info = Dl_info()
    if not libc.dladdr(ctypes.c_void_p(addr), ctypes.byref(info)):
        return None
    return {"fname": info.dli_fname.decode() if info.dli_fname else None,
            "fbase": hex(info.dli_fbase or 0),
            "sname": info.dli_sname.decode() if info.dli_sname else None,
            "saddr": hex(info.dli_saddr or 0)}


def arm_of_path(path, bases):
    real = os.path.realpath(path) if path else path
    for arm, p in ARMS.items():
        if real == os.path.realpath(p):
            return arm
    return path


def snapshot(label, loaded_arms, static):
    objs = loaded_objects()
    mp = maps()
    bases = {}
    for arm in loaded_arms:
        real = os.path.realpath(ARMS[arm])
        hits = [b for n, b in objs if n and os.path.realpath(n) == real]
        bases[arm] = hits[0] if len(hits) == 1 else hits
    snap = {"label": label, "bases": {a: hex(b) if isinstance(b, int) else b for a, b in bases.items()}, "arms": {}}
    for arm in loaded_arms:
        tag = tag_of(arm)
        st = static[arm]
        base = bases[arm]
        rec = {"slots": {}}
        for sym, off in st["slots"].items():
            addr = base + off
            val = ctypes.c_uint64.from_address(addr).value
            unresolved = st["plt"].get(sym)
            dl = dladdr(val)
            mo = maps_owner(val, mp)
            rec["slots"][sym] = {
                "got_addr": hex(addr), "got_off": hex(off), "value": hex(val),
                "lazy_unresolved": unresolved is not None and val == base + unresolved + 6,
                "dladdr": dl,
                "dladdr_arm": arm_of_path(dl["fname"], bases) if dl else None,
                "maps_arm": arm_of_path(mo["path"], bases) if mo else None,
                "module_relative_in_target": None,
            }
            tgt_arm = rec["slots"][sym]["maps_arm"]
            if tgt_arm in bases and isinstance(bases[tgt_arm], int):
                rec["slots"][sym]["module_relative_in_target"] = hex(val - bases[tgt_arm])
        # entry address the session would use: dlsym on this object's own handle
        h = libc.dlopen(ARMS[arm].encode(), RTLD_LAZY | RTLD_NOLOAD)
        entry_name = f"run_main_graph_{tag}".encode()
        e = libc.dlsym(ctypes.c_void_p(h), entry_name) if h else None
        g = libc.dlsym(RTLD_DEFAULT, entry_name)
        rec["handle"] = hex(h) if h else None
        rec["entry_dlsym_handle"] = {"value": hex(e) if e else None,
                                     "maps_arm": arm_of_path((maps_owner(e, mp) or {}).get("path"), bases) if e else None,
                                     "module_relative": hex(e - base) if e else None,
                                     "elf_st_value": hex(st["syms"].get(f"run_main_graph_{tag}", -1))}
        rec["entry_dlsym_RTLD_DEFAULT"] = {"value": hex(g) if g else None,
                                           "maps_arm": arm_of_path((maps_owner(g, mp) or {}).get("path"), bases) if g else None}
        snap["arms"][arm] = rec
    return snap


def main():
    out_path, steps = sys.argv[1], [s.split(":") for s in sys.argv[2].split(",")]
    from PyRuntime import OMExecutionSession
    x = np.load(INPUT)
    expected = np.load(EXPECTED)
    static = {a: static_info(p, tag_of(a)) for a, p in ARMS.items()}
    env = {k: v for k, v in os.environ.items() if k.startswith("LD_")}
    result = {"pid": os.getpid(), "ld_env": env, "tracer_pid": None, "steps": steps, "static": {
        a: {"slots": {k: hex(v) for k, v in s["slots"].items()}, "plt": {k: hex(v) for k, v in s["plt"].items()
            if k in s["slots"]}, "syms": {k: hex(v) for k, v in s["syms"].items()}} for a, s in static.items()},
        "snapshots": [], "calls": []}
    with open("/proc/self/status") as f:
        for line in f:
            if line.startswith("TracerPid:"):
                result["tracer_pid"] = int(line.split()[1])
    sessions, loaded, keep = {}, [], []
    for i, (op, arm) in enumerate(steps):
        if op == "load":
            tag = TAGS.get(arm)
            sessions[arm] = OMExecutionSession(shared_lib_path=ARMS[arm], tag=tag) if tag else \
                OMExecutionSession(shared_lib_path=ARMS[arm])
            loaded.append(arm)
        elif op == "call":
            out = sessions[arm].run([x])
            keep.append(out)
            exact = bool(len(out) == 1 and out[0].dtype == expected.dtype and np.array_equal(out[0], expected))
            result["calls"].append({"step": i, "arm": arm, "exact": exact})
        result["snapshots"].append(snapshot(f"after step {i}: {op} {arm}", loaded, static))
    with open(out_path, "w") as f:
        json.dump(result, f, indent=1)
    print(json.dumps({"pid": result["pid"], "tracer_pid": result["tracer_pid"], "ld_env": env,
                      "calls": result["calls"]}))


if __name__ == "__main__":
    main()
