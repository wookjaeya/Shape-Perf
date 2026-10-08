# gdb Python script: observation points for case R2 (cFE #2663) on the
# unmodified ENV build (cFE c5fb2b4d). Every breakpoint records one JSON line
# and lets the program continue (stop() returns False).
#
# The reporter's method was a gdb breakpoint at cfe_evs.c "main#L185", which
# at the report date was the CFE_EVS_APP_ILLEGAL_APP_ID assignment. At
# c5fb2b4d that statement is L188 (cases/mapping_v701.md). The other points
# are observation points of this study, not conditions from the report.
#
# Output: $R2_TRACE_OUT (default r2_trace.jsonl), one record per hit.
import json
import os
import time

import gdb

OUT = open(os.environ.get("R2_TRACE_OUT", "r2_trace.jsonl"), "w", buffering=1)
# Keep the inferior's environment equal to a run without gdb (ENV E12): drop the
# variable that only this script reads, and the LINES/COLUMNS that gdb adds.
for _v in ("R2_TRACE_OUT", "LINES", "COLUMNS"):
    gdb.execute("unset environment %s" % _v)
SEQ = [0]


def frames(maxn=14):
    out, f = [], gdb.newest_frame()
    while f is not None and len(out) < maxn:
        sal = f.find_sal()
        out.append({"func": f.name(),
                    "file": os.path.basename(sal.symtab.filename) if sal.symtab else None,
                    "line": sal.line})
        f = f.older()
    return out


def val(expr):
    try:
        return str(gdb.parse_and_eval(expr))
    except gdb.error as e:
        return "<error: %s>" % e


def emit(kind, extra):
    SEQ[0] += 1
    t = gdb.selected_thread()
    rec = {"seq": SEQ[0], "t_ns": time.monotonic_ns(), "kind": kind,
           "lwp": t.ptid[1], "thread_name": t.name}
    rec.update(extra)
    OUT.write(json.dumps(rec) + "\n")


class Point(gdb.Breakpoint):
    def __init__(self, spec, kind, fn):
        super().__init__(spec)
        self.kind, self.fn = kind, fn

    def stop(self):
        try:
            emit(self.kind, self.fn())
        except Exception as e:  # never stop the inferior because of the script
            emit(self.kind, {"script_error": repr(e)})
        return False


def pub_pre():
    return {"sb_appid": val("CFE_SB_Global.AppId"), "stack": frames(6)}


def pub():
    return {"sb_appid": val("CFE_SB_Global.AppId"), "stack": frames(6)}


def sb_evs_registered():
    return {"status": val("Status"), "sb_appid": val("CFE_SB_Global.AppId")}


def send_with_appid():
    return {"event_id": val("EventID"), "app_id": val("AppID"),
            "sb_appid_now": val("CFE_SB_Global.AppId"), "stack": frames()}


def outcome():
    return {"app_id": val("AppID"), "event_id": val("EventID")}


gdb.execute("set pagination off")
gdb.execute("set confirm off")
gdb.execute("set print thread-events off")
gdb.execute("set breakpoint pending on")
# OSAL and glibc use real-time and timer signals; pass them through silently.
for sig in ["SIGALRM", "SIGUSR1", "SIGUSR2", "SIGPIPE", "SIGCHLD"] + ["SIG%d" % i for i in range(34, 65)]:
    try:
        gdb.execute("handle %s nostop noprint pass" % sig, to_string=True)
    except gdb.error:
        pass

Point("cfe_sb_task.c:138", "SB_PUB_PRE", pub_pre)           # before CFE_ES_GetAppID(&CFE_SB_Global.AppId)
Point("cfe_sb_task.c:142", "SB_PUB", pub)                   # first statement after it
Point("cfe_sb_task.c:156", "SB_EVS_REGISTERED", sb_evs_registered)  # after CFE_EVS_Register (L155)
Point("CFE_EVS_SendEventWithAppID", "SEND_WITH_APPID", send_with_appid)
Point("cfe_evs.c:188", "OUT_ILLEGAL_APP_ID", outcome)       # the reporter's breakpoint statement
Point("cfe_evs.c:193", "OUT_NOT_REGISTERED", outcome)       # Status = EVS_NotRegistered(AppDataPtr, AppID)
OUT.write(json.dumps({"meta": "breakpoints",
                      "points": [(b.number, b.location, getattr(b, "kind", None)) for b in gdb.breakpoints()]}) + "\n")
