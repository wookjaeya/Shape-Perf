#!/usr/bin/env python3
"""m1_analyze.py -- build the M1 result tables (BASELINE_MEASURE.md) from env/logs: console logs of run01-run07
and m1_*, the M1 snapshots (.S1/.S2/.S2b.json), the pipe-info files and the persisted-data records.
Read only.  Usage: m1_analyze.py <logs_dir>  (prints to stdout)
"""
import glob
import json
import os
import re
import sys

LOGS = sys.argv[1]
M1 = ["A1", "A2", "A3", "B1", "B2", "B3"]
OLD = sorted(os.path.basename(p) for p in glob.glob(os.path.join(LOGS, "run0*.log")))

# app -> (kind of evidence, regex of its own init-complete report)
INIT = {
    "CFE_ES": ("EVS CFE_ES 1", r"/CFE_ES 1: cFE ES Initialized"),
    "CFE_EVS": ("EVS CFE_EVS 1", r"/CFE_EVS 1: cFE EVS Initialized"),
    "CFE_SB": ("EVS CFE_SB 1", r"/CFE_SB 1: cFE SB Initialized"),
    "CFE_TBL": ("EVS CFE_TBL 1", r"/CFE_TBL 1: cFE TBL Initialized"),
    "CFE_TIME": ("EVS CFE_TIME 1", r"/CFE_TIME 1: cFE TIME Initialized"),
    "SCH_LAB": ("OS_printf (no EVS event, no timestamp)", r"^SCH Lab Initialized\."),
    "CI_LAB": ("EVS CI_LAB 3", r"/CI_LAB 3: CI Lab Initialized"),
    "TO_LAB": ("EVS TO_LAB 1", r"/TO_LAB 1: TO Lab Initialized"),
    "SAMPLE_APP": ("EVS SAMPLE_APP 1", r"/SAMPLE_APP 1: Sample App Initialized"),
    "LC": ("EVS LC 2", r"/LC 2: LC Initialized"),
    "CF": ("EVS CF 20", r"/CF 20: CF Initialized"),
    "DS": ("EVS DS 1", r"/DS 1: Application initialized"),
    "FM": ("EVS FM 1", r"/FM 1: Initialization complete"),
    "HK": ("EVS HK 1", r"/HK 1: HK Initialized"),
    "HS": ("EVS HS 1 (HS_INIT_INF_EID)", r"/HS 1: HS Initialized"),
    "MM": ("EVS MM 1", r"/MM 1: MM Initialized"),
    "SC": ("EVS SC 9", r"/SC 9: SC Initialized"),
    "MD": ("EVS MD 1", r"/MD 1: MD Initialized"),
    "CS": ("EVS CS 1", r"/CS 1: CS Initialized"),
    "SBN": ("EVS SBN 4 'initialized (ProcessorID=' (SBN_INIT_EID)", r"/SBN 4: initialized \(ProcessorID="),
}
CHILD_INIT = {"FM_CHILD_TASK": r"/FM 72: Child Task initialization complete"}
CORE = ["CFE_ES", "CFE_EVS", "CFE_SB", "CFE_TBL", "CFE_TIME"]


def rd(p):
    with open(p, errors="replace") as f:
        return f.read()


def m1log(r):
    return os.path.join(LOGS, "m1_%s_po_fixed30s_esrestart.log" % r)


def snap(r, s):
    p = os.path.join(LOGS, "m1_%s_po_fixed30s_esrestart.%s.json" % (r, s))
    return json.load(open(p)) if os.path.exists(p) else None


def meta_val(text, key):
    m = re.search(r"^%s=(.*)$" % re.escape(key), text, re.M)
    return m.group(1) if m else None


out = []
P = out.append

# ---------------------------------------------------------------- 1. console loss indicators
P("## 1. Console: SB 'Pipe Overflow' events (EID 25, filter CFE_EVS_FIRST_16_STOP), 'Events squelched' (EVS 44), 'Msg Limit Err' (SB 17), 'No subscribers' (SB 14)")
P("%-36s %4s %5s %-46s %5s %5s %5s %5s %-10s %-8s" % ("log", "ovf", "=16?", "per pipe (sender)", "sqlch", "msglm", "nosub", "dots", "reset", "exit"))
for name in OLD + [os.path.basename(m1log(r)) for r in M1]:
    t = rd(os.path.join(LOGS, name))
    ov = re.findall(r"Pipe Overflow,MsgId (0x[0-9a-f]+),pipe (\S+),sender (\S+)", t)
    per = {}
    for mid, pipe, snd in ov:
        per[(pipe, snd)] = per.get((pipe, snd), 0) + 1
    perstr = " ".join("%s(%s)=%d" % (p, s, n) for (p, s), n in sorted(per.items()))
    dots = sum(len(m) for m in re.findall(r"^\.+", t, re.M))
    rt = re.search(r"Starting the cFE with a (POWER ON|PROCESSOR) reset", t)
    ex = re.search(r"Exiting cFE with (POWERON|PROCESSOR) Reset status", t)
    P("%-36s %4d %5s %-46s %5d %5d %5d %5d %-10s %-8s" % (
        name, len(ov), "yes" if len(ov) == 16 else "no", perstr, t.count("Events squelched"),
        t.count("Msg Limit Err"), len(re.findall(r"No subscribers for MsgId", t)), dots,
        rt.group(1) if rt else "?", ex.group(1) if ex else "none"))
P("dots = HS aliveness string '.' (HS_CPU_ALIVE_STRING, printed every HS_CPU_ALIVE_PERIOD=5 HS cycles; OS_printf without newline)")

# ---------------------------------------------------------------- 2. SB/EVS counters from memory
P("\n## 2. M1 runs: cFE counters read from memory at S1 (OPERATIONAL line read) and S2 (end of 30 s window)")
P("%-3s %-3s %6s %7s %6s %6s %8s %7s %8s %9s %8s %7s" % ("run", "pt", "POEC", "MsgSErr", "MsgLim", "NoSub", "f25cnt", "printed", "SBNsqlch", "EVSsent", "LogOvf*", "LogFull*"))
for r in M1:
    printed = len(re.findall(r"Pipe Overflow", rd(m1log(r))))
    for s in ("S1", "S2", "S2b"):
        d = snap(r, s)
        if not d:
            continue
        j = d["joined"]
        h, e = j["sb_hk"], j["evs_hk"]
        sb = [x for x in j["evs_apps"] if x["app"] == "CFE_SB"][0]
        f25 = [b["Count"] for b in sb["BinFilters"] if b["EventID"] == 25][0]
        sbn = [x for x in j["evs_apps"] if x["app"] == "SBN"][0]
        P("%-3s %-3s %6d %7d %6d %6d %8d %7s %8d %9d %8d %7d" % (
            r, s, h["PipeOverflowErrorCounter"], h["MsgSendErrorCounter"], h["MsgLimitErrorCounter"],
            h["NoSubscribersCounter"], f25, printed if s == "S2" else "", sbn["SquelchedCount"],
            e["MessageSendCounter"], e["LogOverflowCounter"], e["LogFullFlag"]))
P("POEC = CFE_SB HK PipeOverflowErrorCounter (incremented on every OS_QUEUE_FULL, cfe_sb_priv.c L1190-1193);")
P("f25cnt = EVS binary-filter counter of CFE_SB event 25 (attempts that reached EVS); printed = console lines.")
P("* LogOvf/LogFull are copied into the EVS HK packet only when EVS housekeeping runs (every 4.2 s, sch_lab_table.c L60).")

# ---------------------------------------------------------------- 3. queue capacities
P("\n## 3. Queue capacity: requested (SB CreatePipe depth) / OSAL record / kernel mq_getattr, S1 of every M1 run")
ref = None
same = True
for r in M1:
    for s in ("S1", "S2"):
        d = snap(r, s)
        rows = [(p["pipe"], p["app"], p["sb_MaxQueueDepth"], p["osal_max_depth"], p["kernel_mq_maxmsg"],
                 p["kernel_mq_msgsize"], p["osal_max_size"]) for p in d["joined"]["pipes"]]
        if ref is None:
            ref = rows
        elif sorted(rows) != sorted(ref):
            same = False
            P("DIFFERS in %s %s" % (r, s))
P("identical in all 12 snapshots (A1-B3, S1+S2): %s; non-SB OSAL queues: %s; mq_probe queues per snapshot: %s" % (
    same, sorted({len(snap(r, s)["joined"]["other_queues"]) for r in M1 for s in ("S1", "S2")}),
    sorted({len(snap(r, s)["mq"]["queues"]) for r in M1 for s in ("S1", "S2")})))
P("%-20s %-11s %6s %6s %7s %8s %9s" % ("pipe", "app", "SBreq", "OSAL", "kernel", "msgsize", "truncated"))
for row in ref:
    P("%-20s %-11s %6d %6d %7d %8d %9s" % (row[0], row[1], row[2], row[3], row[4], row[5],
                                          "yes" if row[4] < row[2] else "no"))
P("sum requested=%d, sum kernel=%d, pipes=%d" % (sum(r[2] for r in ref), sum(r[4] for r in ref), len(ref)))

# ---------------------------------------------------------------- 4. per-pipe Peak / SendErrors
P("\n## 4. Per-pipe SB statistics at S2 (memory read): PeakQueueDepth (SendErrors); kernel depth for comparison")
pipes = [x[0] for x in ref]
P("%-20s %4s " % ("pipe", "kmax") + " ".join("%-7s" % r for r in M1))
for i, pn in enumerate(pipes):
    cells = []
    for r in M1:
        p = [x for x in snap(r, "S2")["joined"]["pipes"] if x["pipe"] == pn][0]
        flag = "*" if p["sb_PeakQueueDepth"] > p["kernel_mq_maxmsg"] else " "
        cells.append("%2d(%d)%s " % (p["sb_PeakQueueDepth"], p["sb_SendErrors"], flag))
    P("%-20s %4d " % (pn, ref[i][4]) + " ".join("%-7s" % c for c in cells))
P("* PeakQueueDepth above the kernel depth: SB counted more buffers in flight than the queue can hold.")
P("SendErrors is incremented only for the per-MsgId limit (cfe_sb_priv.c L1058-1063), not for a full queue.")

# ---------------------------------------------------------------- 5. pipe info file vs memory
P("\n## 5. Variant B: SB Write-Pipe-Info file vs memory read (S2 before the command, S2b after the file)")
for r in ("B1", "B2", "B3"):
    f = json.load(open(os.path.join(LOGS, "m1_%s_po_fixed30s_esrestart.pipeinfo.json" % r)))
    diffs2, diffs2b = [], []
    for e in f["entries"]:
        for tag, d, dl in (("S2", snap(r, "S2"), diffs2), ("S2b", snap(r, "S2b"), diffs2b)):
            p = [x for x in d["joined"]["pipes"] if x["pipe"] == e["PipeName"]]
            if not p:
                dl.append("%s missing" % e["PipeName"])
                continue
            p = p[0]
            for k, m in (("MaxQueueDepth", "sb_MaxQueueDepth"), ("PeakQueueDepth", "sb_PeakQueueDepth"),
                         ("SendErrors", "sb_SendErrors"), ("CurrentQueueDepth", "sb_CurrentQueueDepth")):
                if e[k] != p[m]:
                    dl.append("%s.%s file=%d mem=%d" % (e["PipeName"], k, e[k], p[m]))
            if e["AppName"] != p["app"]:
                dl.append("%s app file=%s mem=%s" % (e["PipeName"], e["AppName"], p["app"]))
    hook = rd(m1log(r) + ".hook")
    c2c = re.search(r"command-to-close_ms=(\S+)", hook)
    pre = re.search(r"pipe file before command: (.*)", hook)
    P("%s: %d entries, %d bytes (64 header + 60 x n), header=%s; file before command: %s; command->close %s ms" % (
        r, f["entry_count"], f["file_size"], {k: f["header"][k] for k in ("ContentType", "SubType", "Description")},
        pre.group(1) if pre else "?", c2c.group(1) if c2c else "?"))
    P("    differences file vs S2: %s" % (diffs2 or "none"))
    P("    differences file vs S2b: %s" % (diffs2b or "none"))

# ---------------------------------------------------------------- 6. scheduling
P("\n## 6. Scheduling: ES requested priority / OSAL record / kernel (policy, rt_priority, nice), every M1 snapshot")
sched = {}
for r in M1:
    for s in ("S1", "S2"):
        d = snap(r, s)
        for t in d["joined"]["tasks"]:
            th = t["threads"][0] if t["threads"] else {}
            sched.setdefault(t["task"], set()).add((t["es_requested_priority"], t["osal_record_priority"],
                                                    th.get("policy_name"), th.get("rt_priority"), th.get("nice")))
        named = {t["task"][:15] for t in d["joined"]["tasks"]}
        for th in d["threads"]:
            if th["comm"] not in named:
                sched.setdefault("(unnamed) " + th["comm"] + " syscall " + str(th["syscall_nr"]) + " " + th["wchan"], set()).add(
                    ("-", "-", th["policy_name"], th["rt_priority"], th["nice"]))
for k, v in sched.items():
    P("%-58s %s" % (k, sorted(v, key=str)))
ags = sorted({snap(r, s)["autogroup"].split()[0] for r in M1 for s in ("S1",)})
P("autogroup per run (S1): %s" % ags)

# ---------------------------------------------------------------- 7. app status split
P("\n## 7. App status split per run: (a) created  (b) own init-complete report  (c) at S2")
P("(a) = ES 'Loading file ... APP: X' line (core apps: CFE_ES_CreateObjects) and, for M1, a thread named X in /proc at S1")
P("(b) = the app's own init-complete report on the console (list in BASELINE_MEASURE.md §7)")
P("(c) = M1 only: thread present at S2, ES AppState at S2, main-loop ExecutionCounter S1->S2 (CFE_ES_RunLoop L441)")
apps = list(INIT.keys())
for name in OLD + [os.path.basename(m1log(r)) for r in M1]:
    t = rd(os.path.join(LOGS, name))
    r = name[3:5] if name.startswith("m1_") else None
    s1, s2 = (snap(r, "S1"), snap(r, "S2")) if r else (None, None)
    cells = []
    for a in apps:
        created = (a in CORE and ("Calling EarlyInit for %s" % a) in t) or ("APP: %s\n" % a) in t
        if s1:
            created = created and any(th["comm"] == a[:15] for th in s1["threads"])
        init = bool(re.search(INIT[a][1], t, re.M))
        c = ""
        if s2:
            tk1 = {x["task"]: x for x in s1["joined"]["tasks"]}
            tk2 = {x["task"]: x for x in s2["joined"]["tasks"]}
            st = {x["app"]: x["AppState"] for x in s2["joined"]["apps"]}
            alive = any(th["comm"] == a[:15] and th["state"] not in ("Z", "X") for th in s2["threads"])
            dx = tk2[a]["es_ExecutionCounter"] - tk1[a]["es_ExecutionCounter"] if a in tk1 and a in tk2 else None
            c = "/%s,%s,+%s" % ("T" if alive else "-", st.get(a, "?")[:3], dx)
        cells.append("%s%s%s%s" % (a, ":" , ("a" if created else "-") + ("b" if init else "-"), c))
    P("%-36s %s" % (name, "  ".join(cells)))
P("child tasks (M1, S1->S2 voluntary context switches; no RunLoop counter): ")
for r in M1:
    s1, s2 = snap(r, "S1"), snap(r, "S2")
    th1 = {t["comm"]: t for t in s1["threads"]}
    th2 = {t["comm"]: t for t in s2["threads"]}
    P("  %s: %s" % (r, ", ".join("%s %s->%s (state %s)" % (c, th1[c]["vol_ctxt"], th2[c]["vol_ctxt"], th2[c]["state"])
                                for c in ("ES_BG_TASK", "TIME_TONE_TASK", "TIME_ONEHZ_TASK", "FM_CHILD_TASK")
                                if c in th1 and c in th2)))

# ---------------------------------------------------------------- 8. HS
P("\n## 8. HS: console after HS event 74, and HS_AppData from memory")
for name in OLD + [os.path.basename(m1log(r)) for r in M1]:
    t = rd(os.path.join(LOGS, name))
    i = t.find("/HS 74:")
    after = re.findall(r"^.*/HS \d+:.*$", t[i + 1:], re.M) if i >= 0 else []
    other = re.findall(r"^.*(?:HS App:|Application Terminating|CFE_ES_ExitApp|RestartApp|ERROR.*HS).*$", t, re.M)
    P("%-36s HS events after 74: %d %s; HS syslog/exit lines: %d" % (name, len(after), after[:3], len(other)))
for r in M1:
    for s in ("S1", "S2"):
        h = snap(r, s)["joined"]["hs"]
        tk = {x["task"]: x for x in snap(r, s)["joined"]["tasks"]}["HS"]
        P("%s %s HS: RunStatus=%d exec=%d Aliveness=%d CmdCount=%d UtilCpuAvg=0x%08x UtilCpuPeak=0x%08x SysMonPspModuleId=0x%08x Subsys=%d Subch=%d AppMon=%d EventMon=%d CPUHog=%d WatchdogFlag=%d CDSState=%d UtilCycleCtr=%d" % (
            r, s, h["RunStatus"], tk["es_ExecutionCounter"], h["AlivenessCounter"], h["CmdCount"], h["UtilCpuAvg"],
            h["UtilCpuPeak"], h["SysMonPspModuleId"], h["SysMonSubsystemId"], h["SysMonSubchannelId"],
            h["CurrentAppMonState"], h["CurrentEventMonState"], h["CurrentCPUHogState"], h["ServiceWatchdogFlag"],
            h["CDSState"], h["UtilizationCycleCounter"]))

# ---------------------------------------------------------------- 9. persisted data
P("\n## 9. Persisted data before/after each M1 run")
for r in M1:
    for when in ("before", "after"):
        t = rd(os.path.join(LOGS, "m1_%s_persist_%s.txt" % (r, when)))
        ee = re.search(r"EEPROM.DAT: size=(\d+) mtime=(\S+) .*sha256=(\w+) nonzero_bytes=(\d+)", t)
        segs = re.findall(r"^\s+(0x[0-9a-f]+)\s+\d+\s+\w+\s+\d+\s+(\d+)\s+(\d+)", t, re.M)
        ram = re.findall(r"^  file (\S+): size=(\d+) mtime=(\S+) sha256=(\w{12})", t, re.M)
        osal = re.findall(r"^(/dev/shm/osal:\S+): dir mtime=(\S+)", t, re.M)
        P("%s %-6s EEPROM size=%s mtime=%s sha256=%s.. nonzero=%s | shm segs=%s | osal dirs=%s | files=%s" % (
            r, when, ee.group(1), ee.group(2)[11:23], ee.group(3)[:12], ee.group(4), segs or "none",
            [o[0] for o in osal] or "none", ram or "none"))

# ---------------------------------------------------------------- 10. timing
P("\n## 10. Timing (ms since launch, run_cfs.sh .meta)")
for r in M1:
    mt = rd(m1log(r) + ".meta")
    P("%s: operational_line_read=%s hook_calls=%s stop_initiated=%s process_end=%s exit=%s" % (
        r, meta_val(mt, "operational_line_read_ms"), meta_val(mt, "hook_calls"), meta_val(mt, "stop_initiated_ms"),
        meta_val(mt, "process_end_ms"), meta_val(mt, "exit_status")))
for name in OLD:
    mt = rd(os.path.join(LOGS, name + ".meta"))
    P("%s: operational_line_read=%s" % (name, meta_val(mt, "operational_line_read_ms") or meta_val(mt, "operational_line_ms")))

print("\n".join(out))
