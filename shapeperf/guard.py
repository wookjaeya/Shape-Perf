"""Runtime isolation of selector decisions (spec §8.3, G2.5).

While `selector_sandbox()` is active in a thread, a process-wide audit hook
rejects file/directory access, process creation, sockets and imports of the
evaluator/census modules. Selectors never need any of these; a violation
raises SelectorIsolationError and aborts the evaluation run.

This complements the static import check (tests/test_isolation.py). It is not
an OS security boundary: a stronger separation (different UID for the selector
process, census/ readable only by the evaluator UID) is recommended on the
measurement VM and is described in docs/STATUS.md.
"""
import sys
import threading
from contextlib import contextmanager

FORBIDDEN_MODULE_PREFIXES = ("shapeperf.evaluate", "shapeperf.census", "shapeperf.broker",
                             "shapeperf.backends", "shapeperf.compile", "shapeperf.measure")
BLOCKED_EVENTS = {"open", "os.listdir", "os.scandir", "os.walk", "shutil.copyfile",
                  "subprocess.Popen", "os.system", "os.posix_spawn", "os.exec", "os.fork",
                  "socket.connect", "socket.bind", "ctypes.dlopen", "pickle.find_class"}


class SelectorIsolationError(RuntimeError):
    pass


_state = threading.local()
_installed = False


def _hook(event, args):
    if not getattr(_state, "active", False):
        return
    if event in BLOCKED_EVENTS:
        _state.active = False  # allow the exception machinery to run
        raise SelectorIsolationError(f"selector attempted '{event}' {args[:1]!r}")
    if event == "import":
        mod = args[0] or ""
        if mod.startswith(FORBIDDEN_MODULE_PREFIXES):
            _state.active = False
            raise SelectorIsolationError(f"selector attempted to import {mod}")


def install():
    global _installed
    if not _installed:
        sys.addaudithook(_hook)
        _installed = True


@contextmanager
def selector_sandbox():
    install()
    _state.active = True
    try:
        yield
    finally:
        _state.active = False
