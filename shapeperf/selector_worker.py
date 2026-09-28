"""Out-of-process selector host (spec §8.3 isolation).

    python -m shapeperf.selector_worker <request_fd> <reply_fd> <jail_dir>

Threat model: selectors are research code that must not be able to see
anything but their own query history - by accident or by construction. The
guarantee comes from the operating system, not from Python-level checks:

1. Everything the selector needs is imported first (numpy submodules etc.).
2. The init request is read. A test-only class_path selector (allowed only
   with SHAPEPERF_ALLOW_CLASS_PATH=1) has its module SOURCE read here; the
   module's code itself runs only after step 3, inside the jail.
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


# modules a test-only class_path module may have imported for it before the
# jail: the selectors' own allow-list (tests/test_isolation.py) plus what the
# test selectors need. Nothing is derived from the module text alone, so no
# other module's import-time code runs unjailed.
PREIMPORT_ALLOWED = {"__future__", "math", "bisect", "dataclasses", "types", "typing", "collections",
                     "functools", "itertools", "os", "pathlib", "numpy",
                     "shapeperf.selectors", "shapeperf.selectors.base", "shapeperf.selectors.policies"}


def _preimport_dependencies(text):
    """Import, before the jail, the allow-listed modules a class_path module
    imports (the jail has no file system to import from)."""
    import ast
    import importlib
    names = set()
    for node in ast.walk(ast.parse(text)):
        if isinstance(node, ast.Import):
            names |= {a.name for a in node.names}
        elif isinstance(node, ast.ImportFrom) and node.module and node.level == 0:
            names.add(node.module)
            names |= {f"{node.module}.{a.name}" for a in node.names}
    for n in sorted(names):
        if n in PREIMPORT_ALLOWED or n.startswith("numpy."):
            importlib.import_module(n)


def _locate(mod):
    """Module spec of a dotted class_path WITHOUT importing its parents (a
    parent's __init__ would run unjailed). Parents must be namespace packages
    or already imported."""
    import importlib.machinery
    parts = mod.split(".")
    path = None
    for i, part in enumerate(parts):
        spec = importlib.machinery.PathFinder.find_spec(part, path)
        if spec is None:
            raise ModuleNotFoundError(f"class_path module {mod!r} not found")
        name = ".".join(parts[:i + 1])
        if i < len(parts) - 1:
            if spec.origin is not None and name not in sys.modules:
                raise RuntimeError(f"class_path parent {name!r} is a regular package: its __init__ would "
                                   "run outside the jail (use a namespace package or a top-level module)")
            path = spec.submodule_search_locations
    if spec.origin is None or not spec.origin.endswith(".py"):
        raise RuntimeError(f"class_path module {mod!r} must be a single .py file")
    return spec


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
        src = None
        if first.get("class_path"):          # test-only: selectors outside the registry
            if os.environ.get("SHAPEPERF_ALLOW_CLASS_PATH") != "1":
                raise RuntimeError("class_path selectors are test-only (set SHAPEPERF_ALLOW_CLASS_PATH=1)")
            mod, qual = first["class_path"].split(":")
            spec = _locate(mod)
            with open(spec.origin) as f:
                src = (mod, qual, spec, f.read())
            _preimport_dependencies(src[3])
        elif klass is None:
            raise KeyError(f"unknown selector {first['name']!r}")
        level = _jail(jail_dir, {req_fd, rep_fd})
        if src is not None:
            # the module's top-level code runs jailed (no file system, no network)
            import importlib.util
            mod, qual, spec, text = src
            module = importlib.util.module_from_spec(spec)      # __spec__, __package__, __file__ set
            sys.modules[mod] = module                           # e.g. dataclasses looks it up
            exec(compile(text, spec.origin, "exec"), module.__dict__)
            klass = getattr(module, qual.split(".")[0])
            for part in qual.split(".")[1:]:
                klass = getattr(klass, part)
            level += "+test-class"
        with warnings.catch_warnings(record=True):
            warnings.simplefilter("always")
            with selector_sandbox():
                sel = klass(seed=first.get("seed"), **first.get("params", {}))
                desc = sel.describe()
        reply({"seq": seq, "ok": True, "describe": desc, "isolation_level": level})
    except SelectorIsolationError as e:
        reply({"seq": seq, "ok": False, "error": str(e), "isolation": True})
        return
    except BaseException as e:            # SystemExit from selector code is reported, not fatal
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
