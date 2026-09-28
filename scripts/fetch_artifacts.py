#!/usr/bin/env python3
"""Download pinned artifacts and write artifacts.lock.json (spec §4.1 steps 1-2).

For every entry in configs/artifacts.json: try its urls in order, record which
URL succeeded (and why earlier ones failed), SHA-256, size and license. If an
artifacts.lock.json already exists, downloads must reproduce the locked hash.
Entries with cross_check_urls are compared byte-for-byte with those mirrors.
"""
import argparse
import sys
import time
import urllib.request
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf.util import REPO_ROOT, read_json, sha256_file, write_json  # noqa: E402

LOCK = REPO_ROOT / "artifacts.lock.json"


def download(url, dest, timeout=600):
    dest.parent.mkdir(parents=True, exist_ok=True)
    tmp = dest.with_suffix(dest.suffix + ".part")
    req = urllib.request.Request(url, headers={"User-Agent": "shapeperf-fetch/1"})
    with urllib.request.urlopen(req, timeout=timeout) as r, open(tmp, "wb") as f:
        while True:
            b = r.read(1 << 20)
            if not b:
                break
            f.write(b)
    tmp.replace(dest)


def fmt(url, a):
    return url.format(commit=a.get("commit", ""), path=a.get("path", ""))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--only", nargs="*", help="artifact names to fetch")
    ap.add_argument("--skip-optional", action="store_true")
    args = ap.parse_args()

    cfg = read_json(REPO_ROOT / "configs/artifacts.json")
    lock = read_json(LOCK) if LOCK.exists() else {"artifacts": {}}
    ok = True
    for a in cfg["artifacts"]:
        name = a["name"]
        if args.only and name not in args.only:
            continue
        if args.skip_optional and a.get("optional"):
            continue
        dest = REPO_ROOT / a["dest"]
        locked = lock["artifacts"].get(name)
        expected = a.get("expected_sha256") or (locked or {}).get("sha256")
        attempts = []
        if dest.exists() and expected and sha256_file(dest) == expected:
            print(f"[ok] {name}: present, sha256 matches lock")
            continue
        used = None
        for u in a["urls"]:
            url = fmt(u, a)
            t0 = time.time()
            try:
                download(url, dest)
                used = url
                attempts.append({"url": url, "status": "ok", "seconds": round(time.time() - t0, 1)})
                break
            except Exception as e:  # record every failure (spec §7: failures are data)
                attempts.append({"url": url, "status": f"failed: {e!r}"})
        if used is None:
            print(f"[FAIL] {name}: all urls failed: {attempts}")
            ok = ok and bool(a.get("optional"))
            continue
        digest = sha256_file(dest)
        if expected and digest != expected:
            print(f"[FAIL] {name}: sha256 {digest} != locked {expected}")
            ok = False
            continue
        cross = []
        for u in a.get("cross_check_urls", []):
            url = fmt(u, a)
            tmp = dest.with_suffix(dest.suffix + ".crosscheck")
            try:
                download(url, tmp)
                d2 = sha256_file(tmp)
                cross.append({"url": url, "sha256": d2, "match": d2 == digest})
            except Exception as e:
                cross.append({"url": url, "status": f"failed: {e!r}"})
            finally:
                tmp.unlink(missing_ok=True)
        if any(c.get("match") is False for c in cross):
            print(f"[FAIL] {name}: cross-check mismatch {cross}")
            ok = False
        lock["artifacts"][name] = {
            "dest": a["dest"],
            "role": a.get("role"),
            "repo": a.get("repo"),
            "commit": a.get("commit"),
            "path": a.get("path"),
            "url_used": used,
            "attempts": attempts,
            "official_source": a.get("official_source"),
            "provenance_note": a.get("provenance_note"),
            "cross_check": cross,
            "sha256": digest,
            "size_bytes": dest.stat().st_size,
            "license": a.get("license"),
            "fetched_at_unix": int(time.time()),
        }
        print(f"[ok] {name}: {digest} via {used}")
    write_json(LOCK, lock)
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
