#!/usr/bin/env python3
"""section_text_sha_2.py KZDUMP FACTS... -- (results 8.0b, 8.0c) per section of each facts container (kz-dump text): the section
name, its record count and the sha256 of its record lines only (the lines after the `section` line up to the next one),
in which kz-dump writes every string reference as its text.  The `section` line is left out because its crc32 is over the
record bytes, and a string reference in the bytes is a position in the canonical STRINGS pool (sorted, every referenced
string once: lib/core/container_writer.cpp, lib/core/container_reader.cpp), so that a string added by a new section
moves the positions of the strings after it in every section that references one.  section_sha.py is unchanged.
(results 8.0c) A copy of section_text_sha.py that takes the record lines only (the lines that begin with two spaces):
the first copy also took the trailer line `artifact_sha256=` of the dump into the last section.
Prints `<file> <section> count=<n> text_sha256=<h> lines=<k>`."""
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
        cur, buf, count = None, [], None

        def flush():
            if cur is not None:
                h = hashlib.sha256("\n".join(buf).encode("utf-8", "surrogateescape")).hexdigest()
                print("%s %s count=%s text_sha256=%s lines=%d" % (path, cur, count, h, len(buf)))
        for l in text:
            if l.startswith("section "):
                flush()
                cur, buf, count = l.split()[1], [], re.search(r" count=(\d+) ", l).group(1)
            elif cur is not None and l.startswith("  "):
                buf.append(l)
        flush()
    return 0


if __name__ == "__main__":
    sys.exit(main())
