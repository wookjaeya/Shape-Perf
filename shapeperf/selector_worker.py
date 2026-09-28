"""Out-of-process selector host (spec §8.3 isolation).

Protocol over stdin/stdout, one JSON object per line:
  -> {"cmd": "init", "name": ..., "seed": ..., "params": {...}}
  <- {"ok": true, "describe": {...}}
  -> {"cmd": "next", "view": <view_to_json>}
  <- {"ok": true, "action": ["measure"|"probe", length] | null, "selection_ns": int}
  <- {"ok": false, "error": "...", "isolation": bool}   on failure
This process imports only shapeperf.selectors (+ numpy) and never receives
anything but its own View, so evaluator data is not in its address space.
Every decision runs inside selector_sandbox().
"""
import json
import sys
import time
import warnings


def main():
    from shapeperf.guard import SelectorIsolationError, selector_sandbox
    from shapeperf.selectors import REGISTRY
    from shapeperf.selectors.base import view_from_json

    out = sys.stdout
    sel = None
    for line in sys.stdin:
        msg = json.loads(line)
        try:
            if msg["cmd"] == "init":
                if msg.get("class_path"):          # test-only: selectors outside the registry
                    import importlib
                    mod, cls = msg["class_path"].split(":")
                    klass = getattr(importlib.import_module(mod), cls)
                else:
                    klass = REGISTRY[msg["name"]]
                sel = klass(seed=msg.get("seed"), **msg.get("params", {}))
                resp = {"ok": True, "describe": sel.describe()}
            elif msg["cmd"] == "next":
                view = view_from_json(msg["view"])
                # warnings are recorded, not printed: printing would read source
                # files (linecache), which the sandbox rejects
                with warnings.catch_warnings(record=True) as w:
                    warnings.simplefilter("always")
                    t0 = time.perf_counter_ns()
                    with selector_sandbox():
                        act = sel.next_action(view)
                    ns = time.perf_counter_ns() - t0
                resp = {"ok": True, "action": None if act is None else [act.kind, int(act.length)],
                        "selection_ns": ns, "warnings": [str(x.message) for x in w]}
            else:
                resp = {"ok": False, "error": f"unknown cmd {msg['cmd']}", "isolation": False}
        except SelectorIsolationError as e:
            resp = {"ok": False, "error": str(e), "isolation": True}
        except Exception as e:  # selector bug: report, broker aborts the run
            resp = {"ok": False, "error": f"{type(e).__name__}: {e}", "isolation": False}
        out.write(json.dumps(resp) + "\n")
        out.flush()


if __name__ == "__main__":
    main()
