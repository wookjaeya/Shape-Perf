"""Runtime isolation of selector decisions (spec §8.3, G2.5).

Primary boundary: selectors run in a SEPARATE PROCESS (shapeperf/selector_worker.py)
that imports only shapeperf.selectors and receives nothing but its own View
over a pipe. Ground truth, census data and backend objects live only in the
broker's process, so they cannot be reached by frame walking or by importing
already-loaded evaluator modules.

Secondary boundary, inside that worker (and for in-process debugging runs):
while `selector_sandbox()` is active, an audit hook rejects file/directory
access, process creation, sockets, dynamic loading, new imports and frame
introspection. A violation is *sticky*: even if the selector catches the
exception, leaving the sandbox raises SelectorIsolationError, so a
try/except around a forbidden call cannot hide it.

Not an OS security boundary: on the measurement VM, running the worker under a
different UID with census/ readable only by the evaluator UID is recommended.
"""
import sys
import threading
from contextlib import contextmanager

FORBIDDEN_MODULE_PREFIXES = ("shapeperf.evaluate", "shapeperf.census", "shapeperf.broker",
                             "shapeperf.backends", "shapeperf.compile", "shapeperf.measure")
BLOCKED_EVENTS = {"open", "os.listdir", "os.scandir", "os.walk", "shutil.copyfile",
                  "subprocess.Popen", "os.system", "os.posix_spawn", "os.exec", "os.fork",
                  "socket.connect", "socket.bind", "ctypes.dlopen", "pickle.find_class",
                  "sys._getframe", "sys._current_frames", "object.__getattr__",
                  "import", "exec", "compile"}


class SelectorIsolationError(RuntimeError):
    pass


_state = threading.local()
_installed = False


def _hook(event, args):
    if not getattr(_state, "active", False):
        return
    if event == "object.__getattr__":
        # attribute access through object.__getattr__ is audited only for
        # frame/code internals (f_back, f_locals, tb_frame, ...)
        name = args[1] if len(args) > 1 else ""
        if not str(name).startswith(("f_", "tb_", "gi_", "cr_", "co_")):
            return
    if event in BLOCKED_EVENTS:
        msg = f"selector attempted '{event}' {args[:2]!r}"
        if event == "import":
            mod = args[0] or ""
            msg = f"selector attempted to import {mod}" + (
                " (evaluator module)" if mod.startswith(FORBIDDEN_MODULE_PREFIXES) else "")
        if getattr(_state, "violation", None) is None:
            _state.violation = msg
        raise SelectorIsolationError(msg)


def install():
    global _installed
    if not _installed:
        sys.addaudithook(_hook)
        _installed = True


@contextmanager
def selector_sandbox():
    install()
    _state.violation = None
    _state.active = True
    try:
        yield
    finally:
        _state.active = False
        v, _state.violation = _state.violation, None
        if v is not None:
            raise SelectorIsolationError(v)
