#!/usr/bin/env python3
"""cfs_snapshot.py -- one measurement snapshot of a running core-cpu1, taken from outside the process.

What is read, and why it does not change the run (BASELINE_MEASURE.md §2):
  * /proc/<pid>/task/<tid>/{stat,status,comm,syscall,wchan}: scheduling policy, rt_priority, nice, CPU, CPU time and
    context switches of every thread (procfs, read only).
  * /proc/<pid>/autogroup, /proc/<pid>/maps (read only).
  * mq_probe <pid>: kernel attributes of every OSAL queue (mq_getattr on a reopened descriptor; no message is
    received or sent; see mq_probe.c).
  * /proc/<pid>/mem: plain reads (pread) of cFE/OSAL/HS global tables at addresses taken from the DWARF debug info
    of the same binaries (cfs_layout.py).  No ptrace attach, so no thread is stopped.  The values are a non-atomic
    copy (no cFE lock is taken), so counters that change during the copy can be off by the changes made during
    the few microseconds of the read.

Usage: cfs_snapshot.py <pid> <layout.json> <mq_probe> <out_prefix> <label>
Writes <out_prefix>.json (everything) and <out_prefix>.txt (readable tables).
"""
import json
import os
import struct
import subprocess
import sys
import time

POLICY = {0: "SCHED_OTHER", 1: "SCHED_FIFO", 2: "SCHED_RR", 3: "SCHED_BATCH", 5: "SCHED_IDLE", 6: "SCHED_DEADLINE"}
APPSTATE = {0: "UNDEFINED", 1: "EARLY_INIT", 2: "LATE_INIT", 3: "RUNNING", 4: "WAITING", 5: "STOPPED"}
SYSSTATE = {0: "UNDEFINED", 1: "EARLY_INIT", 2: "CORE_STARTUP", 3: "CORE_READY", 4: "APPS_INIT",
            5: "OPERATIONAL", 6: "SHUTDOWN"}


def rd(path, default=None):
    try:
        with open(path) as f:
            return f.read()
    except OSError as e:
        return default if default is not None else "ERR:%s" % e.strerror


def threads(pid):
    out = []
    for tid in sorted(os.listdir("/proc/%d/task" % pid), key=int):
        base = "/proc/%d/task/%s" % (pid, tid)
        st = rd(base + "/stat", "")
        if not st:
            continue
        rest = st[st.rfind(")") + 2:].split()
        f = lambda n: rest[n - 3]
        status = rd(base + "/status", "")
        ctx = {}
        for line in status.splitlines():
            if line.startswith(("voluntary_ctxt_switches", "nonvoluntary_ctxt_switches")):
                k, v = line.split(":")
                ctx[k] = int(v)
        sysc = rd(base + "/syscall", "").split()
        out.append({
            "tid": int(tid), "comm": rd(base + "/comm", "").strip(), "state": f(3),
            "utime": int(f(14)), "stime": int(f(15)), "priority": int(f(18)), "nice": int(f(19)),
            "processor": int(f(39)), "rt_priority": int(f(40)), "policy": int(f(41)),
            "policy_name": POLICY.get(int(f(41)), str(f(41))),
            "vol_ctxt": ctx.get("voluntary_ctxt_switches"), "nonvol_ctxt": ctx.get("nonvoluntary_ctxt_switches"),
            "syscall_nr": sysc[0] if sysc else None, "wchan": rd(base + "/wchan", "").strip(),
        })
    return out


def module_bases(pid, layout):
    bases = {}
    for line in rd("/proc/%d/maps" % pid, "").splitlines():
        parts = line.split()
        if len(parts) < 6 or int(parts[2], 16) != 0:
            continue
        for obj, info in layout["objects"].items():
            if obj not in bases and parts[5].endswith("/" + info["file"]):
                bases[obj] = int(parts[0].split("-")[0], 16)
    return bases


def read_tables(pid, layout, bases):
    res = {}
    with open("/proc/%d/mem" % pid, "rb", buffering=0) as mem:
        for key, t in layout["tables"].items():
            if t["object"] not in bases:
                res[key] = {"error": "module %s not mapped" % t["object"]}
                continue
            addr = bases[t["object"]] + t["addr"]
            n = t["stride"] * t["count"]
            if key == "es_state":   # only one field of a 57 kB struct is needed
                fld = t["fields"]["SystemState"]
                blob = os.pread(mem.fileno(), fld["size"], addr + fld["off"])
                res[key] = [{"SystemState": struct.unpack("<i", blob)[0]}]
                continue
            blob = os.pread(mem.fileno(), n, addr)
            rows = []
            for i in range(t["count"]):
                rec = blob[i * t["stride"]:(i + 1) * t["stride"]]
                row = {"_index": i}
                for fname, fd in t["fields"].items():
                    raw = rec[fd["off"]:fd["off"] + fd["size"]]
                    if fd["kind"] == "str":
                        row[fname] = raw.split(b"\0", 1)[0].decode("latin-1")
                    elif fd["kind"] == "raw":
                        row[fname] = raw.hex()
                    else:
                        row[fname] = int.from_bytes(raw, "little", signed=(fd["kind"] == "sint"))
                rows.append(row)
            res[key] = rows
    return res


def mq_attrs(pid, probe):
    p = subprocess.run([probe, str(pid)], capture_output=True, text=True)
    rows = []
    for line in p.stdout.splitlines()[1:]:
        c = line.split("\t")
        name = c[1].replace(" (deleted)", "")
        row = {"fd": int(c[0]), "kernel_name": c[1], "queue_name": name.split(".", 1)[1] if "." in name else name}
        try:
            row.update({"mq_maxmsg": int(c[2]), "mq_msgsize": int(c[3]), "mq_curmsgs": int(c[4])})
        except ValueError:
            row["error"] = c[2]
        rows.append(row)
    return {"rc": p.returncode, "stderr": p.stderr.strip(), "queues": rows}


def binfilters(hexstr):
    b = bytes.fromhex(hexstr)
    out = []
    for i in range(0, len(b), 8):
        eid, mask, cnt, _ = struct.unpack("<HHHH", b[i:i + 8])
        if eid or mask or cnt:
            out.append({"EventID": eid, "Mask": "0x%04x" % mask, "Count": cnt})
    return out


def main():
    pid, layout_path, probe, prefix, label = int(sys.argv[1]), sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5]
    layout = json.load(open(layout_path))
    t0 = time.clock_gettime(time.CLOCK_MONOTONIC)
    snap = {"label": label, "pid": pid, "utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "monotonic_start": t0}
    snap["threads"] = threads(pid)
    snap["autogroup"] = rd("/proc/%d/autogroup" % pid).strip()
    snap["mq"] = mq_attrs(pid, probe)
    bases = module_bases(pid, layout)
    snap["module_bases"] = {k: hex(v) for k, v in bases.items()}
    try:
        snap["mem"] = read_tables(pid, layout, bases)
    except Exception as e:  # keep the /proc and mq parts even if the memory read fails
        snap["mem"] = {"error": "%s: %s" % (type(e).__name__, e)}
    snap["monotonic_end"] = time.clock_gettime(time.CLOCK_MONOTONIC)

    # ------------------------------------------------------------------ joins
    m = snap["mem"]
    joined = {}
    try:
        joined = join(snap, m)
    except Exception as e:
        joined = {}
        snap["join_error"] = "%s: %s" % (type(e).__name__, e)
    pipes = joined.get("pipes", [])
    other_q = joined.get("other_queues", [])
    tasks = joined.get("tasks", [])
    evs = joined.get("evs_apps", [])
    finish(snap, joined, pipes, other_q, tasks, evs, prefix, label, pid, t0)


def join(snap, m):
    joined = {}
    if "error" not in m and all(isinstance(m.get(k), list) for k in ("sb_pipe", "es_app", "os_queue_common")):
        apps = {r["AppId"]: r for r in m["es_app"] if r["AppId"] and r["AppName"]}
        qcommon = {r["active_id"]: r["_index"] for r in m["os_queue_common"] if r["active_id"]}
        mq_by_fd = {q["fd"]: q for q in snap["mq"]["queues"]}
        pipes = []
        for r in m["sb_pipe"]:
            if not r["PipeId"]:
                continue
            qi = qcommon.get(r["SysQueueId"])
            osq = m["os_queue"][qi] if qi is not None else {}
            fd = m["os_queue_impl"][qi]["id"] if qi is not None else None
            k = mq_by_fd.get(fd, {})
            pipes.append({
                "pipe": osq.get("queue_name"), "app": apps.get(r["AppId"], {}).get("AppName"),
                "sb_MaxQueueDepth": r["MaxQueueDepth"], "osal_max_depth": osq.get("max_depth"),
                "osal_max_size": osq.get("max_size"), "fd": fd,
                "kernel_mq_maxmsg": k.get("mq_maxmsg"), "kernel_mq_msgsize": k.get("mq_msgsize"),
                "kernel_mq_curmsgs": k.get("mq_curmsgs"), "kernel_name_check": k.get("queue_name"),
                "sb_CurrentQueueDepth": r["CurrentQueueDepth"], "sb_PeakQueueDepth": r["PeakQueueDepth"],
                "sb_SendErrors": r["SendErrors"], "Opts": r["Opts"]})
        # OSAL queues that are not SB pipes
        sb_q = {p["fd"] for p in pipes}
        other_q = []
        for qi, r in enumerate(m["os_queue_common"]):
            if r["active_id"] and m["os_queue_impl"][qi]["id"] not in sb_q:
                fd = m["os_queue_impl"][qi]["id"]
                k = mq_by_fd.get(fd, {})
                other_q.append({"queue": m["os_queue"][qi]["queue_name"], "osal_max_depth": m["os_queue"][qi]["max_depth"],
                                "osal_max_size": m["os_queue"][qi]["max_size"], "fd": fd,
                                "kernel_mq_maxmsg": k.get("mq_maxmsg"), "kernel_mq_msgsize": k.get("mq_msgsize"),
                                "kernel_mq_curmsgs": k.get("mq_curmsgs")})
        tasks = []
        osal_tasks = {m["os_task"][i]["task_name"]: m["os_task"][i] for i, r in enumerate(m["os_task_common"])
                      if r["active_id"]}
        thr_by_comm = {}
        for t in snap["threads"]:
            thr_by_comm.setdefault(t["comm"], []).append(t)
        for r in m["es_task"]:
            if not r["TaskId"] or not r["TaskName"]:
                continue
            ot = osal_tasks.get(r["TaskName"], {})
            th = thr_by_comm.get(r["TaskName"][:15], [])
            tasks.append({"task": r["TaskName"], "app": apps.get(r["AppId"], {}).get("AppName"),
                          "es_requested_priority": r["StartParams.Priority"],
                          "osal_record_priority": ot.get("priority"),
                          "es_ExecutionCounter": r["ExecutionCounter"],
                          "threads": [{k: t[k] for k in ("tid", "policy_name", "rt_priority", "nice", "priority",
                                                          "state", "utime", "stime", "vol_ctxt", "nonvol_ctxt")}
                                      for t in th]})
        evs = []
        for r in m["evs_app"]:
            if not r["AppID"]:
                continue
            evs.append({"app": apps.get(r["AppID"], {}).get("AppName"), "ActiveFlag": r["ActiveFlag"],
                        "EventCount": r["EventCount"], "SquelchTokens": r["SquelchTokens"],
                        "SquelchedCount": r["SquelchedCount"], "BinFilters": binfilters(r["BinFilters"])})
        joined = {
            "system_state": SYSSTATE.get(m["es_state"][0]["SystemState"], m["es_state"][0]["SystemState"]),
            "apps": [{"app": r["AppName"], "AppState": APPSTATE.get(r["AppState"], r["AppState"]), "Type": r["Type"]}
                     for r in m["es_app"] if r["AppId"] and r["AppName"]],
            "pipes": pipes, "other_queues": other_q, "tasks": tasks, "evs_apps": evs,
            "sb_hk": m["sb_hk"][0], "evs_hk": m["evs_hk"][0], "hs": m["hs"][0] if isinstance(m["hs"], list) else m["hs"],
        }
    return joined


def finish(snap, joined, pipes, other_q, tasks, evs, prefix, label, pid, t0):
    snap["joined"] = joined
    with open(prefix + ".json", "w") as f:
        json.dump(snap, f, indent=1)

    # ------------------------------------------------------------------ text
    L = []
    L.append("# snapshot %s  pid=%d  utc=%s  duration_ms=%.1f" % (
        label, pid, snap["utc"], (snap["monotonic_end"] - t0) * 1000))
    L.append("autogroup: %s   module_bases: %s" % (snap["autogroup"], snap["module_bases"]))
    L.append("## threads (/proc/<pid>/task/<tid>/stat: policy field 41, rt_priority 40, nice 19, priority 18, cpu 39)")
    L.append("%-7s %-16s %-5s %-11s %6s %4s %4s %3s %6s %6s %8s %8s %s" % (
        "tid", "comm", "state", "policy", "rtprio", "nice", "prio", "cpu", "utime", "stime", "volctx", "nvolctx",
        "syscall/wchan"))
    for t in snap["threads"]:
        L.append("%-7d %-16s %-5s %-11s %6d %4d %4d %3d %6d %6d %8s %8s %s/%s" % (
            t["tid"], t["comm"], t["state"], t["policy_name"], t["rt_priority"], t["nice"], t["priority"],
            t["processor"], t["utime"], t["stime"], t["vol_ctxt"], t["nonvol_ctxt"], t["syscall_nr"], t["wchan"]))
    L.append("## mq_probe rc=%s %s: %d queues (fd name maxmsg msgsize curmsgs)" % (
        snap["mq"]["rc"], snap["mq"]["stderr"], len(snap["mq"]["queues"])))
    for q in snap["mq"]["queues"]:
        L.append("  %s %s %s %s %s" % (q["fd"], q["kernel_name"], q.get("mq_maxmsg", q.get("error")),
                                        q.get("mq_msgsize"), q.get("mq_curmsgs")))
    if joined:
        L.append("## SB pipes: SB requested depth / OSAL record depth / kernel mq_maxmsg; SB counters (mem read)")
        L.append("%-20s %-12s %5s %5s %6s %6s %6s %6s %6s %6s %4s" % (
            "pipe", "app", "SBreq", "OSAL", "kmax", "kmsgsz", "kcur", "SBcur", "SBpeak", "SBsErr", "fd"))
        for p in pipes:
            L.append("%-20s %-12s %5s %5s %6s %6s %6s %6s %6s %6s %4s" % (
                p["pipe"], p["app"], p["sb_MaxQueueDepth"], p["osal_max_depth"], p["kernel_mq_maxmsg"],
                p["kernel_mq_msgsize"], p["kernel_mq_curmsgs"], p["sb_CurrentQueueDepth"], p["sb_PeakQueueDepth"],
                p["sb_SendErrors"], p["fd"]))
        for q in other_q:
            L.append("non-SB OSAL queue: %s" % q)
        L.append("## SB HK counters: %s" % joined["sb_hk"])
        L.append("## EVS HK counters: %s" % joined["evs_hk"])
        L.append("## ES system state: %s" % joined["system_state"])
        L.append("## ES apps: " + ", ".join("%s=%s" % (a["app"], a["AppState"]) for a in joined["apps"]))
        L.append("## tasks: ES requested priority / OSAL record priority / kernel policy,rtprio,nice / ExecutionCounter")
        for t in tasks:
            th = t["threads"][0] if t["threads"] else {}
            L.append("%-16s %-10s req=%-4s osal=%-4s kernel=%s,%s,%s tid=%s exec=%s ctx=%s/%s cpu=%s+%s%s" % (
                t["task"], t["app"], t["es_requested_priority"], t["osal_record_priority"], th.get("policy_name"),
                th.get("rt_priority"), th.get("nice"), th.get("tid"), t["es_ExecutionCounter"], th.get("vol_ctxt"),
                th.get("nonvol_ctxt"), th.get("utime"), th.get("stime"),
                "" if len(t["threads"]) == 1 else "  (threads matched: %d)" % len(t["threads"])))
        L.append("## EVS per app: SquelchedCount / SquelchTokens / binary filter counts")
        for e in evs:
            L.append("%-12s squelched=%-3d tokens=%-6d events=%-5d filters=%s" % (
                e["app"], e["SquelchedCount"], e["SquelchTokens"], e["EventCount"],
                ";".join("%d:%s:%d" % (b["EventID"], b["Mask"], b["Count"]) for b in e["BinFilters"])))
        L.append("## HS_AppData: %s" % joined["hs"])
    else:
        L.append("## memory read or join failed: %s %s" % (snap["mem"].get("error") if isinstance(snap["mem"], dict) else "", snap.get("join_error", "")))
    with open(prefix + ".txt", "w") as f:
        f.write("\n".join(L) + "\n")
    print("snapshot %s written (%.1f ms)" % (prefix, (snap["monotonic_end"] - t0) * 1000))


if __name__ == "__main__":
    main()
