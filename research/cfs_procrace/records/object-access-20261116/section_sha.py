#!/usr/bin/env python3
"""section_sha.py KZDUMP FACTS... -- per section of each facts container (kz-dump text): the section line (name, count,
crc32) and the sha256 of the section's dump text (its `section` line and the record lines up to the next one), so that
a later round can say which sections of an intermediate re-extraction it re-creates (plan 4.3 practice: an intermediate
round is not kept compressed; its per-section sha256 is).  Prints `<file> <section> count=<n> crc32=<c> sha256=<h>`."""
import hashlib
import re
import subprocess
import sys


def main():
    if len(sys.argv) < 3:
        print(__doc__, file=sys.stderr)
        return 2
    kd = sys.argv[1]
    for path in sys.argv[2:]:
        text = subprocess.run([kd, path], capture_output=True, check=True).stdout.decode("utf-8", "surrogateescape").split("\n")
        cur, buf, head = None, [], None

        def flush():
            if cur is not None:
                h = hashlib.sha256("\n".join(buf).encode("utf-8", "surrogateescape")).hexdigest()
                m = re.match(r"^section (\S+) .*count=(\d+) crc32=(\S+)$", head)
                print("%s %s count=%s crc32=%s sha256=%s" % (path, m.group(1), m.group(2), m.group(3), h))
        for l in text:
            if l.startswith("section "):
                flush()
                cur, buf, head = l.split()[1], [l], l
            elif cur is not None:
                buf.append(l)
        flush()
    return 0


if __name__ == "__main__":
    sys.exit(main())
