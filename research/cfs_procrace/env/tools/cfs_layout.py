#!/usr/bin/env python3
"""cfs_layout.py -- extract, from the DWARF debug info of the staged binaries, the addresses and field offsets
of the cFE/OSAL/HS data that cfs_snapshot.py reads from a running core-cpu1 through /proc/<pid>/mem.

Static only: gdb is run in batch mode on the *files* (core-cpu1 and cf/hs.so); no process is attached.
The build is `native_std` = CMAKE_BUILD_TYPE debug (`-g`, CONDITIONS.md C5), so the debug info describes exactly
the code that runs.  Addresses are link-time addresses of the position-independent objects; cfs_snapshot.py adds
the load base read from /proc/<pid>/maps.

Usage: cfs_layout.py <exe_dir> <out.json>
"""
import hashlib
import json
import os
import re
import subprocess
import sys

# (key, object, array expression, count expression or None, [(field, size-kind)])
# size-kind: 'int' (unsigned, width from sizeof), 'sint' (signed), 'str' (char array), 'raw' (bytes)
SPECS = [
    ("sb_pipe", "core", "CFE_SB_Global.PipeTbl", None,
     [("PipeId", "int"), ("AppId", "int"), ("SysQueueId", "int"), ("Opts", "int"), ("SendErrors", "int"),
      ("MaxQueueDepth", "int"), ("CurrentQueueDepth", "int"), ("PeakQueueDepth", "int")]),
    ("sb_hk", "core", "(&CFE_SB_Global.HKTlmMsg.Payload)", "1",
     [("CommandCounter", "int"), ("CommandErrorCounter", "int"), ("NoSubscribersCounter", "int"),
      ("MsgSendErrorCounter", "int"), ("MsgReceiveErrorCounter", "int"), ("InternalErrorCounter", "int"),
      ("CreatePipeErrorCounter", "int"), ("SubscribeErrorCounter", "int"),
      ("DuplicateSubscriptionsCounter", "int"), ("PipeOverflowErrorCounter", "int"),
      ("MsgLimitErrorCounter", "int"), ("MemInUse", "int")]),
    ("os_queue_common", "core", "(&OS_common_table[OS_QUEUE_BASE])",
     "sizeof(OS_queue_table)/sizeof(OS_queue_table[0])", [("active_id", "int"), ("creator", "int")]),
    ("os_queue", "core", "OS_queue_table", None, [("queue_name", "str"), ("max_size", "int"), ("max_depth", "int")]),
    ("os_queue_impl", "core", "OS_impl_queue_table", None, [("id", "sint")]),
    ("os_task_common", "core", "(&OS_common_table[OS_TASK_BASE])",
     "sizeof(OS_task_table)/sizeof(OS_task_table[0])", [("active_id", "int"), ("creator", "int")]),
    ("os_task", "core", "OS_task_table", None, [("task_name", "str"), ("stack_size", "int"), ("priority", "int")]),
    ("es_app", "core", "CFE_ES_Global.AppTable", None,
     [("AppId", "int"), ("AppName", "str"), ("AppState", "int"), ("Type", "int"), ("MainTaskId", "int")]),
    ("es_task", "core", "CFE_ES_Global.TaskTable", None,
     [("TaskId", "int"), ("TaskName", "str"), ("AppId", "int"), ("ExecutionCounter", "int"),
      ("StartParams.Priority", "int"), ("StartParams.StackSize", "int")]),
    ("es_state", "core", "(&CFE_ES_Global)", "1", [("SystemState", "sint")]),
    ("evs_app", "core", "CFE_EVS_Global.AppData", None,
     [("AppID", "int"), ("ActiveFlag", "int"), ("EventCount", "int"), ("SquelchTokens", "sint"),
      ("SquelchedCount", "int"), ("BinFilters", "raw")]),
    ("evs_hk", "core", "(&CFE_EVS_Global.EVS_TlmPkt.Payload)", "1",
     [("MessageSendCounter", "int"), ("MessageTruncCounter", "int"), ("UnregisteredAppCounter", "int"),
      ("LogOverflowCounter", "int"), ("LogFullFlag", "int")]),
    ("hs", "hs", "(&HS_AppData)", "1",
     [("RunStatus", "int"), ("CmdCount", "int"), ("CmdErrCount", "int"), ("AlivenessCounter", "int"),
      ("ServiceWatchdogFlag", "int"), ("CurrentAppMonState", "int"), ("CurrentEventMonState", "int"),
      ("CurrentAlivenessState", "int"), ("CurrentCPUHogState", "int"), ("AppMonLoaded", "int"),
      ("EventMonLoaded", "int"), ("CDSState", "int"), ("SysMonPspModuleId", "int"), ("SysMonSubsystemId", "int"),
      ("SysMonSubchannelId", "int"), ("UtilizationCycleCounter", "int"), ("CurrentCPUUtilIndex", "int"),
      ("UtilCpuAvg", "int"), ("UtilCpuPeak", "int"), ("EventsMonitoredCount", "int"), ("MsgActExec", "int")]),
]
OBJECTS = {"core": "core-cpu1", "hs": "cf/hs.so"}


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()


def gdb_eval(path, cmds):
    args = ["gdb", "-batch", "-nx"]
    for c in cmds:
        args += ["-ex", c]
    args.append(path)
    out = subprocess.run(args, capture_output=True, text=True, timeout=300)
    return out.stdout + out.stderr


def main():
    exe_dir, out_path = sys.argv[1], sys.argv[2]
    layout = {"objects": {}, "tables": {}}
    for obj, rel in OBJECTS.items():
        path = os.path.join(exe_dir, rel)
        layout["objects"][obj] = {"file": rel, "sha256": sha256(path)}
        cmds = []
        for key, o, arr, cnt, fields in SPECS:
            if o != obj:
                continue
            count = cnt or "sizeof(%s)/sizeof(%s[0])" % (arr, arr)
            cmds.append('printf "BASE %s %%lu %%lu %%lu\\n", (unsigned long)&%s[0], (unsigned long)sizeof(%s[0]), '
                        '(unsigned long)(%s)' % (key, arr, arr, count))
            for fname, kind in fields:
                cmds.append('printf "FIELD %s %s %%lu %%lu\\n", (unsigned long)((char*)&%s[0].%s - (char*)&%s[0]), '
                            '(unsigned long)sizeof(%s[0].%s)' % (key, fname, arr, fname, arr, arr, fname))
        text = gdb_eval(path, cmds)
        for line in text.splitlines():
            m = re.match(r"BASE (\S+) (\d+) (\d+) (\d+)$", line)
            if m:
                layout["tables"][m.group(1)] = {"object": obj, "addr": int(m.group(2)), "stride": int(m.group(3)),
                                                "count": int(m.group(4)), "fields": {}}
                continue
            m = re.match(r"FIELD (\S+) (\S+) (\d+) (\d+)$", line)
            if m:
                kind = dict(dict((k, f) for k, _, _, _, f in SPECS)[m.group(1)])[m.group(2)]
                layout["tables"][m.group(1)]["fields"][m.group(2)] = {"off": int(m.group(3)), "size": int(m.group(4)),
                                                                     "kind": kind}
        missing = [k for k, o, _, _, f in SPECS if o == obj and
                   (k not in layout["tables"] or len(layout["tables"][k]["fields"]) != len(f))]
        if missing:
            sys.stderr.write("gdb output for %s:\n%s\n" % (rel, text))
            sys.exit("layout extraction incomplete for: %s" % ", ".join(missing))
    with open(out_path, "w") as f:
        json.dump(layout, f, indent=1, sort_keys=True)
    print("layout written: %s (%d tables)" % (out_path, len(layout["tables"])))


if __name__ == "__main__":
    main()
