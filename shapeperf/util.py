"""Small shared helpers: paths, hashing, JSON/JSONL I/O."""
import hashlib
import json
import os
import subprocess
import time
import uuid
from functools import lru_cache
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
DATA_DIR = REPO_ROOT / "data"
RESULTS_DIR = REPO_ROOT / "results"
CENSUS_DIR = REPO_ROOT / "census"
THIRD_PARTY_DIR = REPO_ROOT / "third_party"


def sha256_file(path, chunk=1 << 20):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while True:
            b = f.read(chunk)
            if not b:
                break
            h.update(b)
    return h.hexdigest()


def sha256_bytes(b):
    return hashlib.sha256(b).hexdigest()


@lru_cache(maxsize=None)
def git_head(path=REPO_ROOT):
    """HEAD (+'-dirty') of the checkout, frozen at the FIRST call of this process. A commit made
    while a long run is in progress must not relabel records written by code that was already
    loaded (v2 census records changed their harness_commit mid-run)."""
    try:
        out = subprocess.run(["git", "-C", str(path), "rev-parse", "HEAD"],
                             capture_output=True, text=True, check=True).stdout.strip()
        dirty = subprocess.run(["git", "-C", str(path), "status", "--porcelain"],
                               capture_output=True, text=True).stdout.strip()
        return out + ("-dirty" if dirty else "")
    except Exception:
        return "unavailable"


def new_run_id(prefix="run"):
    return f"{prefix}-{time.strftime('%Y%m%dT%H%M%SZ', time.gmtime())}-{uuid.uuid4().hex[:8]}"


def write_json(path, obj):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(path.suffix + ".tmp")
    with open(tmp, "w") as f:
        json.dump(obj, f, indent=2, sort_keys=False, default=str)
        f.write("\n")
    os.replace(tmp, path)


def read_json(path):
    with open(path) as f:
        return json.load(f)


def append_jsonl(path, rec):
    """Append one record. Raw logs are append-only (spec §12)."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    line = json.dumps(rec, sort_keys=True, default=str)
    with open(path, "a") as f:
        f.write(line + "\n")
        f.flush()
        os.fsync(f.fileno())


def read_jsonl(path):
    with open(path) as f:
        return [json.loads(line) for line in f if line.strip()]
