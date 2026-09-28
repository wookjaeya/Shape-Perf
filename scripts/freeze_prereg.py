#!/usr/bin/env python3
"""Freeze configs/preregistration.json (spec §10.2 step 4). Refuses while any
value in the listed sections is null. After freezing, any edit changes the
content hash and main-phase scripts refuse to run until it is re-frozen - which
must then be reported as a deviation in report.md."""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf import prereg  # noqa: E402

SECTIONS = ["correctness", "measurement", "events", "selectors", "signature"]


def nulls(d, prefix=""):
    out = []
    for k, v in d.items():
        if k.startswith("_"):
            continue
        if isinstance(v, dict):
            out += nulls(v, f"{prefix}{k}.")
        elif v is None:
            out.append(prefix + k)
    return out


def main():
    p = prereg.load()
    missing = [n for s in SECTIONS for n in nulls(p[s], s + ".")]
    if missing:
        sys.exit("cannot freeze, unfixed values:\n  " + "\n  ".join(missing))
    p["frozen"] = True
    p["status"] = "frozen"
    p["frozen_sha256"] = None
    p["frozen_sha256"] = prereg.content_hash(p)
    with open(prereg.path(), "w") as f:
        json.dump(p, f, indent=2)
        f.write("\n")
    print("frozen:", p["frozen_sha256"])


if __name__ == "__main__":
    main()
