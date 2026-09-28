"""Access to the machine-readable preregistration (configs/preregistration.json).

Main-phase code calls `require(...)`: it refuses to run while the file is not
frozen or a needed value is still null (spec §10.2 steps 4-5). Development and
pilot runs may pass allow_unfrozen=True; their outputs are then labelled
'unfrozen-preregistration' so they cannot be mistaken for main results.
"""
import hashlib
import json
import os
from pathlib import Path

from .util import REPO_ROOT


def path():
    """SHAPEPERF_PREREG overrides the file (tests, labelled sensitivity analyses)."""
    return Path(os.environ.get("SHAPEPERF_PREREG", REPO_ROOT / "configs/preregistration.json"))


def load():
    with open(path()) as f:
        return json.load(f)


def content_hash(p=None):
    p = p or load()
    body = {k: v for k, v in p.items() if k not in ("frozen_sha256",)}
    return hashlib.sha256(json.dumps(body, sort_keys=True).encode()).hexdigest()


def require(fields, allow_unfrozen=False):
    """fields: dotted paths like 'events.delta_min_effect'. Returns (prereg, label)."""
    p = load()
    missing = []
    for f in fields:
        cur = p
        for part in f.split("."):
            cur = cur.get(part) if isinstance(cur, dict) else None
        if cur is None:
            missing.append(f)
    frozen_ok = p.get("frozen") and p.get("frozen_sha256") == content_hash(p)
    if missing or not frozen_ok:
        if not allow_unfrozen:
            raise SystemExit(f"preregistration not usable for main runs: frozen={bool(frozen_ok)}, "
                             f"missing={missing}. Fix configs/preregistration.json after G3.")
        return p, f"unfrozen-preregistration(missing={missing})"
    return p, f"frozen:{p['frozen_sha256'][:12]}"
