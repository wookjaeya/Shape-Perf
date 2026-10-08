#!/usr/bin/env python3
"""Driver: every case in a FRESH plain python process, lazy (default) and LD_BIND_NOW=1."""
import json
import os
import subprocess
import sys

D = os.path.dirname(os.path.abspath(__file__))
PY = "/home/user/work/venv/bin/python"
C = "/home/user/work/v3/g2/K/L0064"
T = "/home/user/work/v3_followup/e1/K_L0064_tagged"
ARTS = {"S8": {"path": f"{C}/S8_full/model.so"}, "S1": {"path": f"{C}/S1_full/model.so"}}
IO = {"input": f"{C}/input.npy", "expected": f"{C}/expected.npy"}

CASES = {
    "I8": [["load", "S8"], ["call", "S8"], ["entry"]],
    "I1": [["load", "S1"], ["call", "S1"], ["entry"]],
    "C81_callLaterFirst": [["load", "S8"], ["load", "S1"], ["entry"], ["call", "S1"], ["call", "S8"], ["entry"]],
    "C81_callEarlierFirst": [["load", "S8"], ["load", "S1"], ["call", "S8"], ["call", "S1"], ["entry"]],
    "C18_callLaterFirst": [["load", "S1"], ["load", "S8"], ["entry"], ["call", "S8"], ["call", "S1"], ["entry"]],
    "C18_callEarlierFirst": [["load", "S1"], ["load", "S8"], ["call", "S1"], ["call", "S8"], ["entry"]],
    "C81_callLaterOnly": [["load", "S8"], ["load", "S1"], ["call", "S1"], ["entry"]],
    "C18_callLaterOnly": [["load", "S1"], ["load", "S8"], ["call", "S8"], ["entry"]],
}


def clean_env(extra):
    env = {k: v for k, v in os.environ.items() if not k.startswith("LD_")}
    env.update(SHAPEPERF_WORK="/home/user/work", OMP_NUM_THREADS="1")
    env.update(extra)
    return env


def main():
    out_dir = os.path.join(D, "got_runs")
    os.makedirs(out_dir, exist_ok=True)
    summary = {}
    for case, steps in CASES.items():
        for mode, extra in (("lazy", {}), ("bindnow", {"LD_BIND_NOW": "1"})):
            spec = dict(IO, artifacts=ARTS, steps=steps)
            sp = os.path.join(out_dir, f"{case}_{mode}.spec.json")
            json.dump(spec, open(sp, "w"))
            r = subprocess.run([PY, os.path.join(D, "got_probe.py"), sp], capture_output=True, text=True,
                               env=clean_env(extra), cwd=D)
            op = os.path.join(out_dir, f"{case}_{mode}.json")
            open(op, "w").write(r.stdout)
            if r.returncode != 0:
                print(case, mode, "RC", r.returncode, r.stderr[-2000:])
                continue
            res = json.loads(r.stdout)
            summary[f"{case}_{mode}"] = res
            print(f"=== {case} [{mode}] pid={res['pid']} TracerPid={res['tracer_pid']} LD_env={res['env_LD']}")
            for s in res["log"]:
                if s["step"] == "entry-check":
                    for n, e in s["entries"].items():
                        for sym, v in e.items():
                            print(f"   entry {n}.{sym}: dlsym(handle)={v['dlsym_handle']} -> {v['handle_classified']}"
                                  f" [{v['handle_dso']}]  | RTLD_DEFAULT -> {v['default_classified']}")
                    continue
                print(f"  {s['step']}" + (f"  output_exact={s['output_exact']}" if "output_exact" in s else "")
                      + f"  bases={s['bases']}")
                for n, g in s["got"].items():
                    for sym, v in g.items():
                        print(f"     GOT[{n}] {sym:32s} slot={v['slot']} val={v['value']} -> {v['classified']}"
                              f"  (maps:{v['maps_path']}, dladdr:{v['dladdr_fname']}:{v['dladdr_sname']})")
    json.dump({"files": sorted(summary)}, open(os.path.join(out_dir, "index.json"), "w"))


if __name__ == "__main__":
    main()
