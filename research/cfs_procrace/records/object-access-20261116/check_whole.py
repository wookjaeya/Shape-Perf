#!/usr/bin/env python3
"""check_whole.py KZDUMP OLD NEW -- (copied from records/review-fixes-20261116 for the types and object access step) two facts
containers of one build (the re-extraction of the review fixes and that of this step without --object-access) read by the
same kz-dump, compared line by line over the whole dump: equal apart from the lines of
records/extract-fixes-20261116/check_head.py (tool_sha256=, artifact_sha256=, basis_sha256=, a facts' parent[0]) and, as in
the declarations of plan/full_stage_results_20261116.md 4.0 and 5.0, the STRINGS section line with another crc32 only and
the BASIS record `extract.units_from` with another basis hash only (addendum 6.0 (나-1): the basis's artifact hash holds
the tool hash, and the facts quote it).  rc 0 when nothing else differs, 1 otherwise."""
import re
import subprocess
import sys

H = r"[0-9a-f]{64}"
ALLOW = [re.compile(r"^(tool_sha256|artifact_sha256|basis_sha256)=" + H + "$"), re.compile(r"^  parent\[0\] kind=basis role=1 sha256=" + H + "$")]
STR = re.compile(r"^(section STRINGS .* )crc32=0x[0-9a-f]{1,8}$")  # kz-dump writes the crc32 without leading zeros
UNITS = re.compile(r'^(  BASIS\[\d+\] name="extract\.units_from" value="basis )[0-9a-f]{16}(" .*)$')


def main():
    if len(sys.argv) != 4:
        print(__doc__, file=sys.stderr)
        return 2
    kd, a, b = sys.argv[1:]
    x = subprocess.run([kd, a], capture_output=True, check=True).stdout.decode("utf-8", "surrogateescape").split("\n")
    y = subprocess.run([kd, b], capture_output=True, check=True).stdout.decode("utf-8", "surrogateescape").split("\n")
    allowed, other = 0, []
    if len(x) != len(y):
        other.append("line count %d / %d" % (len(x), len(y)))
    for i, (p, q) in enumerate(zip(x, y), 1):
        if p == q:
            continue
        if any(r.match(p) and r.match(q) for r in ALLOW):
            allowed += 1
            continue
        for r in (STR, UNITS):
            mp, mq = r.match(p), r.match(q)
            if mp and mq and mp.groups() == mq.groups():
                allowed += 1
                break
        else:
            other.append("line %d: %s | %s" % (i, p[:150], q[:150]))
    print("check_whole: lines %d, allowed %d, other %d" % (len(y), allowed, len(other)))
    for o in other[:10]:
        print("  " + o)
    return 0 if not other else 1


if __name__ == "__main__":
    sys.exit(main())
