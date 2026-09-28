#!/usr/bin/env python3
"""Run a command and record wall time, CPU time and memory use as JSON.

Used for build stages (spec §5.3: build parallelism, peak RSS and disk use are
pilot values, so they have to be measured, not assumed).

    timed_run.py --out stage.json --label llvm-build -- cmake --build .

Recorded:
  wall_ns                 monotonic wall time of the whole command
  cpu_user_s / cpu_sys_s  rusage of the command and all waited-for descendants
  max_rss_single_proc_kb  ru_maxrss: peak RSS of the largest single descendant
  peak_system_mem_used_kb sampled MemTotal-MemAvailable peak (aggregate, includes
                          everything else running on the machine)
"""
import argparse
import json
import os
import subprocess
import sys
import threading
import time


def mem_used_kb():
    info = {}
    with open("/proc/meminfo") as f:
        for line in f:
            k, v = line.split(":", 1)
            info[k] = int(v.split()[0])
    return info["MemTotal"] - info["MemAvailable"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("--label", required=True)
    ap.add_argument("--sample-interval", type=float, default=1.0)
    ap.add_argument("cmd", nargs=argparse.REMAINDER)
    a = ap.parse_args()
    cmd = a.cmd[1:] if a.cmd and a.cmd[0] == "--" else a.cmd
    if not cmd:
        ap.error("missing command")

    peak = {"v": mem_used_kb()}
    stop = threading.Event()

    def sampler():
        while not stop.wait(a.sample_interval):
            peak["v"] = max(peak["v"], mem_used_kb())

    t = threading.Thread(target=sampler, daemon=True)
    t.start()
    start_wall = time.time()
    t0 = time.monotonic_ns()
    proc = subprocess.Popen(cmd)
    _, status, ru = os.wait4(proc.pid, 0)
    t1 = time.monotonic_ns()
    stop.set()
    t.join()
    rc = os.waitstatus_to_exitcode(status)
    rec = {
        "label": a.label,
        "cmd": cmd,
        "cwd": os.getcwd(),
        "start_unix": start_wall,
        "wall_ns": t1 - t0,
        "cpu_user_s": ru.ru_utime,
        "cpu_sys_s": ru.ru_stime,
        "max_rss_single_proc_kb": ru.ru_maxrss,
        "peak_system_mem_used_kb": peak["v"],
        "returncode": rc,
    }
    os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
    with open(a.out, "w") as f:
        json.dump(rec, f, indent=2)
    sys.exit(rc)


if __name__ == "__main__":
    main()
