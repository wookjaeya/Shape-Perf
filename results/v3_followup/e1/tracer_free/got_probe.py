"""Independent check (no gdb, no shapeperf code): after the calls, read each loaded model's
.got.plt slots for the wrapper and compute symbols and resolve which DSO they point into."""
import ctypes, json, os, re, subprocess, sys
import numpy as np
sys.path.insert(0, '/home/user/work/onnx-mlir/build/Release/lib')

def elf_meta(path, tag):
    t = tag or "model"
    names = {"entry": f"run_main_graph_{t}", "wrapper": f"_mlir_ciface_main_graph_{t}", "compute": f"main_graph_{t}"}
    syms = {}
    for ln in subprocess.run(["readelf", "-W", "--dyn-syms", path], capture_output=True, text=True).stdout.splitlines():
        f = ln.split()
        if len(f) >= 8 and f[3] == "FUNC" and f[6] != "UND":
            syms[f[7]] = (int(f[1], 16), int(f[2]))
    slots = {}
    for ln in subprocess.run(["readelf", "-W", "-r", path], capture_output=True, text=True).stdout.splitlines():
        f = ln.split()
        if len(f) >= 5 and f[2].endswith("JUMP_SLOT"):
            slots[f[4]] = int(f[0], 16)
    return names, syms, slots

def main(spec):
    from PyRuntime import OMExecutionSession
    arms = spec["arms"]
    meta = {a: elf_meta(v["path"], v["tag"]) for a, v in arms.items()}
    x = np.load(spec["input"]); exp = np.load(spec["expected"])
    sess, log, outs = {}, [], []
    for op, a in spec["steps"]:
        if op == "load":
            v = arms[a]
            sess[a] = OMExecutionSession(shared_lib_path=v["path"], tag=v["tag"]) if v["tag"] else OMExecutionSession(shared_lib_path=v["path"])
        else:
            o = sess[a].run([x]); outs.append(o)
            log.append((a, bool(np.array_equal(o[0], exp))))
    # base addresses via RTLD_NOLOAD handles + own-scope dlsym of the entry
    base, ranges = {}, {}
    for a in sess:
        names, syms, slots = meta[a]
        h = ctypes.CDLL(arms[a]["path"], mode=4 | os.RTLD_LAZY)   # RTLD_NOLOAD
        addr = ctypes.cast(getattr(h, names["entry"]), ctypes.c_void_p).value
        base[a] = addr - syms[names["entry"]][0]
    def owner_of(addr, kind):
        hits = [b for b in sess if base[b] + meta[b][1].get(meta[b][0][kind], (-1, 0))[0] == addr]
        if hits: return hits
        own_plt = [b for b in sess if base[b] <= addr < base[b] + 0x10000]
        return [f"unbound(plt of {own_plt})"]
    got = {}
    for a in sess:
        names, syms, slots = meta[a]
        got[a] = {k: owner_of(ctypes.c_uint64.from_address(base[a] + slots[names[k]]).value, k) for k in ("wrapper", "compute")}
    chains = []
    for a, ok in log:
        w = got[a]["wrapper"]
        c = got[w[0]]["compute"] if len(w) == 1 and w[0] in got else ["?"]
        chains.append({"req": a, "wrapper_in": w, "compute_in": c, "exact": ok})
    print(json.dumps({"got": got, "calls": chains}))

if __name__ == "__main__":
    main(json.loads(sys.argv[1]))
