#!/usr/bin/env python3
"""check_extract.py KZDUMP NEW REF -- one re-extracted build (NEW, schema12_extract/<b>/<b>/NP) against its reference
(REF, tests_8c_fix/<b>/NP), record by record, with exactly the differences the declaration allows
(plan/full_stage_results_20261116.md 4.0).  The containers are compared as kz-dump text streamed line by line (both read
by the same KZDUMP; nothing is written to disk); the text outputs line by line.

Allowed, each only as a whole line of the stated form on both sides:
  basis.kz, facts.kz  header tool_sha256= / artifact_sha256= / basis_sha256= (64 hex), facts' parent[0] (the basis);
                      the STRINGS section line with the same count and another crc32; the BASIS records
                      extract.build_identity / front_end.build_identity where only the schema= and source= values differ
                      (the schema fingerprint and the core source identity of the new build); facts' BASIS
                      extract.units_from where only the basis hash differs;
  basis.txt           the line "basis <hash>" (line 1, or after the compiler's warnings) and the front_end.build_identity
                      line (schema= / source= only);
  extract.txt         line 1 (facts <hash>);
  sha256.txt          the lines of basis.kz, facts.kz and the kz-static binary (hash only).
relations.txt is not compared here: the reference's relations.txt is that of the relations rules of its own tool; compare.sh
extract compares the re-extraction's relations.txt with the relations re-run of this build (schema12_relcheck) apart from
line 2.
Every other line must be equal, and line counts must be equal.  args.txt and relations.log must be equal.  Prints one line
per file and a summary; rc 0 when nothing else differs, 1 otherwise, 2 on a tool error."""
import os
import re
import subprocess
import sys

HEX = r"[0-9a-f]{64}"
CONTAINER_RULES = [
    re.compile(r"^(tool_sha256|artifact_sha256|basis_sha256)=" + HEX + r"$"),
    re.compile(r"^  parent\[0\] kind=basis role=1 sha256=" + HEX + r"$"),
]
STRINGS = re.compile(r"^(section STRINGS type=1 version=1 record=kz_byte record_size=1 count=\d+ crc32=)0x[0-9a-f]+$")
IDENTITY = re.compile(r'^(  BASIS\[\d+\] name="(?:extract|front_end)\.build_identity" value="kz-core abi=1 schema=)' + HEX +
                      r"( source=)" + HEX + r'( compiler=.*)$')
UNITS = re.compile(r'^(  BASIS\[\d+\] name="extract\.units_from" value="basis )[0-9a-f]{16}(".*)$')
TEXT_IDENTITY = re.compile(r"^(front_end\.build_identity = kz-core abi=1 schema=)" + HEX + r"( source=)" + HEX + r"( compiler=.*)$")


def container_line_allowed(a, b):
    for r in CONTAINER_RULES:
        if r.match(a) and r.match(b) and a.split("=")[0] == b.split("=")[0]:
            return "header"
    ma, mb = STRINGS.match(a), STRINGS.match(b)
    if ma and mb and ma.group(1) == mb.group(1):
        return "STRINGS crc32"
    ma, mb = IDENTITY.match(a), IDENTITY.match(b)
    if ma and mb and ma.groups() == mb.groups():
        return "build_identity"
    ma, mb = UNITS.match(a), UNITS.match(b)
    if ma and mb and ma.groups() == mb.groups():
        return "units_from"
    return None


def stream(kzdump, path):
    return subprocess.Popen([kzdump, path], stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def compare_container(kzdump, new, ref):
    pa, pb = stream(kzdump, ref), stream(kzdump, new)
    n, kinds, bad = 0, {}, []
    while True:
        a, b = pa.stdout.readline(), pb.stdout.readline()
        if not a and not b:
            break
        n += 1
        if a == b:
            continue
        if not a or not b:
            bad.append("line %d: one dump ends" % n)
            break
        x, y = a.decode("utf-8", "surrogateescape").rstrip("\n"), b.decode("utf-8", "surrogateescape").rstrip("\n")
        k = container_line_allowed(x, y)
        if k is None:
            bad.append("line %d: %s | %s" % (n, x[:160], y[:160]))
        else:
            kinds[k] = kinds.get(k, 0) + 1
    ra, rb = pa.wait(), pb.wait()
    if ra != 0 or rb != 0:
        sys.stderr.write("kz-dump rc %d / %d\n" % (ra, rb))
        sys.exit(2)
    return n, kinds, bad


def read_lines(p):
    with open(p, encoding="utf-8", errors="surrogateescape") as f:
        return f.read().split("\n")


def compare_text(name, new, ref):
    a, b = read_lines(ref), read_lines(new)
    if len(a) != len(b):
        return len(b), {}, ["line counts %d vs %d" % (len(a), len(b))]
    kinds, bad = {}, []
    for i, (x, y) in enumerate(zip(a, b), 1):
        if x == y:
            continue
        k = None
        if name == "basis.txt" and re.match(r"^basis " + HEX + "$", x) and re.match(r"^basis " + HEX + "$", y):
            k = "basis line"
        elif name == "basis.txt":
            ma, mb = TEXT_IDENTITY.match(x), TEXT_IDENTITY.match(y)
            if ma and mb and ma.groups() == mb.groups():
                k = "build_identity"
        elif name == "extract.txt" and i == 1 and re.match(r"^facts " + HEX + "$", x) and re.match(r"^facts " + HEX + "$", y):
            k = "line 1"
        elif name == "sha256.txt":
            mx, my = re.match(r"^" + HEX + r"  (.*)$", x), re.match(r"^" + HEX + r"  (.*)$", y)
            if mx and my and mx.group(1) == my.group(1) and (mx.group(1) in ("basis.kz", "facts.kz") or mx.group(1).endswith("/kz-static")):
                k = mx.group(1).rsplit("/", 1)[-1]
        if k is None:
            bad.append("line %d: %s | %s" % (i, x[:160], y[:160]))
        else:
            kinds[k] = kinds.get(k, 0) + 1
    return len(b), kinds, bad


def main():
    if len(sys.argv) != 4:
        print(__doc__, file=sys.stderr)
        return 64
    kzdump, new, ref = sys.argv[1:]
    total = 0
    for name in ("basis.kz", "facts.kz"):
        n, kinds, bad = compare_container(kzdump, os.path.join(new, name), os.path.join(ref, name))
        print("%s %s: %d dump lines, allowed %s, other %d" % ("OK" if not bad else "BAD", name, n, dict(sorted(kinds.items())), len(bad)))
        for x in bad[:20]:
            print("  " + x)
        total += len(bad)
    for name in ("basis.txt", "extract.txt", "sha256.txt", "args.txt", "relations.log"):
        n, kinds, bad = compare_text(name, os.path.join(new, name), os.path.join(ref, name))
        print("%s %s: %d lines, allowed %s, other %d" % ("OK" if not bad else "BAD", name, n, dict(sorted(kinds.items())), len(bad)))
        for x in bad[:20]:
            print("  " + x)
        total += len(bad)
    print("check_extract: %s" % ("only declared differences" if total == 0 else "%d undeclared differences" % total))
    return 0 if total == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
