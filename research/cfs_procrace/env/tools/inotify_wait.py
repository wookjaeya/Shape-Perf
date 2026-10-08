#!/usr/bin/env python3
"""inotify_wait.py -- event-based wait for IN_CLOSE_WRITE of one file name in a directory (inotify(7)).

Prints "READY" once the watch is in place (the caller must not trigger the write before that), then
"CLOSE_WRITE <name> <CLOCK_MONOTONIC seconds>" and exits 0 when the event arrives, or "TIMEOUT" and exits 1 when
the bound passes.  The bound is a failure bound, not a timing condition (BASELINE_MEASURE.md §2).

Usage: inotify_wait.py <dir> <name> <bound_ms>
"""
import ctypes
import os
import select
import struct
import sys
import time

IN_CLOSE_WRITE = 0x00000008
d, name, bound_ms = sys.argv[1], sys.argv[2], int(sys.argv[3])
libc = ctypes.CDLL("libc.so.6", use_errno=True)
fd = libc.inotify_init1(os.O_CLOEXEC)
if fd < 0 or libc.inotify_add_watch(fd, d.encode(), IN_CLOSE_WRITE) < 0:
    print("ERROR errno=%d" % ctypes.get_errno(), flush=True)
    sys.exit(2)
print("READY", flush=True)
deadline = time.monotonic() + bound_ms / 1000.0
while True:
    rem = deadline - time.monotonic()
    if rem <= 0:
        print("TIMEOUT", flush=True)
        sys.exit(1)
    r, _, _ = select.select([fd], [], [], rem)
    if not r:
        continue
    buf = os.read(fd, 65536)
    i = 0
    while i < len(buf):
        wd, mask, cookie, ln = struct.unpack_from("iIII", buf, i)
        n = buf[i + 16:i + 16 + ln].split(b"\0", 1)[0].decode()
        i += 16 + ln
        if n == name and mask & IN_CLOSE_WRITE:
            print("CLOSE_WRITE %s %.6f" % (n, time.clock_gettime(time.CLOCK_MONOTONIC)), flush=True)
            sys.exit(0)
