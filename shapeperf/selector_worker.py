"""Out-of-process selector host (spec §8.3 isolation).

    python -m shapeperf.selector_worker <request_fd> <reply_fd> <jail_dir>

Threat model: selectors are research code that must not be able to see
anything but their own query history - by accident or by construction. The
guarantee comes from the operating system, not from Python-level checks:

1. Everything the selector needs is imported first (numpy submodules etc.).
2. The init request is read; a test-only class_path selector is imported.
3. The process jails itself (when started as root):
     - unshare(CLONE_NEWNET): no network
     - chroot(<empty directory>): no file system at all
     - setgroups([]), setgid/setuid(nobody); RLIMIT_NPROC=0, RLIMIT_FSIZE=0
   All inherited file descriptors except the two protocol pipes are closed;
   fds 0/1/2 point to /dev/null and sys.stdout/stderr to an in-memory sink,
   so print() cannot corrupt the protocol.
4. The selector is constructed and every decision runs inside
   selector_sandbox() (audit hook: defence in depth, clear error messages).
The isolation level actually achieved is reported to the broker
('os:netns+chroot+setuid', ... or 'audit-only' when not root); main-phase
policy comparisons require an 'os:' level.

Protocol (one JSON object per line on the dedicated pipes; seq echoes the request):
  -> {"seq": n, "cmd": "init", "name", "seed", "params", ["class_path"]}
  <- {"seq": n, "ok": true, "describe": {...}, "isolation_level": "..."}
  -> {"seq": n, "cmd": "next", "view": <view_to_json>}
  <- {"seq": n, "ok": true, "action": [kind, length] | null, "selection_ns": int, "warnings": [...]}
  <- {"seq": n, "ok": false, "error": "...", "isolation": bool}
"""
import ctypes
import io
import json
import operator
import os
import resource
import sys
import time
import warnings

import numpy  # noqa: F401  (preloaded before the jail)
import numpy.linalg  # noqa: F401
import numpy.random  # noqa: F401

from shapeperf.guard import SelectorIsolationError, selector_sandbox
from shapeperf.selectors import REGISTRY
from shapeperf.selectors.base import view_from_json

NOBODY = 65534
CLONE_NEWNET = 0x40000000


def _jail(jail_dir, keep_fds):
    """Return the isolation level achieved. Never raises: a failed step shows
    up in the level string and the broker decides whether it suffices."""
    devnull = os.open(os.devnull, os.O_RDWR)
    for fd in (0, 1, 2):
        os.dup2(devnull, fd)
    for fd in range(3, 1024):
        if fd not in keep_fds:
            try:
                os.close(fd)
            except OSError:
                pass
    sys.stdout = io.StringIO()
    sys.stderr = io.StringIO()
    if os.getuid() != 0:
        return "audit-only (not root: no chroot/setuid)"
    parts = []
    try:
        libc = ctypes.CDLL(None, use_errno=True)
        if libc.unshare(CLONE_NEWNET) == 0:
            parts.append("netns")
    except Exception:
        pass
    try:
        os.chroot(jail_dir)
        os.chdir("/")
        parts.append("chroot")
    except OSError:
        pass
    try:
        os.setgroups([])
        os.setgid(NOBODY)
        os.setuid(NOBODY)
        parts.append("setuid")
    except OSError:
        pass
    for lim in (resource.RLIMIT_NPROC, resource.RLIMIT_FSIZE):
        try:
            resource.setrlimit(lim, (0, 0))
        except (OSError, ValueError):
            pass
    if "chroot" in parts and "setuid" in parts:
        return "os:" + "+".join(parts)
    return ("partial:" + "+".join(parts)) if parts else "audit-only (jail failed)"


def _plain_action(a):
    if a is None:
        return None
    if a.kind not in ("measure", "probe"):
        raise ValueError(f"invalid action kind {a.kind!r}")
    if isinstance(a.length, bool):
        raise TypeError("action length must be an integer, got bool")
    return [a.kind, operator.index(a.length)]      # rejects floats instead of truncating


def main():
    req_fd, rep_fd, jail_dir = int(sys.argv[1]), int(sys.argv[2]), sys.argv[3]
    rin = os.fdopen(req_fd, "r", buffering=1)
    rout = os.fdopen(rep_fd, "w", buffering=1)

    def reply(obj):
        rout.write(json.dumps(obj) + "\n")
        rout.flush()

    first = json.loads(rin.readline())
    seq = first.get("seq")
    try:
        if first.get("cmd") != "init":
            raise RuntimeError("first request must be init")
        klass = REGISTRY.get(first["name"])
        if first.get("class_path"):          # test-only: selectors outside the registry
            import importlib
            mod, cls = first["class_path"].split(":")
            klass = getattr(importlib.import_module(mod), cls)
        if klass is None:
            raise KeyError(f"unknown selector {first['name']!r}")
        level = _jail(jail_dir, {req_fd, rep_fd})
        with warnings.catch_warnings(record=True):
            warnings.simplefilter("always")
            with selector_sandbox():
                sel = klass(seed=first.get("seed"), **first.get("params", {}))
                desc = sel.describe()
        reply({"seq": seq, "ok": True, "describe": desc, "isolation_level": level})
    except SelectorIsolationError as e:
        reply({"seq": seq, "ok": False, "error": str(e), "isolation": True})
        return
    except Exception as e:
        reply({"seq": seq, "ok": False, "error": f"{type(e).__name__}: {e}", "isolation": False})
        return

    for line in rin:
        msg = json.loads(line)
        seq = msg.get("seq")
        try:
            if msg.get("cmd") != "next":
                raise RuntimeError(f"unknown cmd {msg.get('cmd')!r}")
            view = view_from_json(msg["view"])
            # warnings are recorded, not printed (printing reads source files)
            with warnings.catch_warnings(record=True) as w:
                warnings.simplefilter("always")
                t0 = time.perf_counter_ns()
                with selector_sandbox():
                    act = sel.next_action(view)
                ns = time.perf_counter_ns() - t0
            reply({"seq": seq, "ok": True, "action": _plain_action(act), "selection_ns": ns,
                   "warnings": [str(x.message) for x in w]})
        except SelectorIsolationError as e:
            reply({"seq": seq, "ok": False, "error": str(e), "isolation": True})
        except Exception as e:  # selector bug: report, broker aborts the run
            reply({"seq": seq, "ok": False, "error": f"{type(e).__name__}: {e}", "isolation": False})


if __name__ == "__main__":
    main()
