#!/usr/bin/env python3
"""sb_pipeinfo_parse.py -- decode the file written by the cFE SB "Write Pipe Info" command (CFE_SB_WRITE_PIPE_INFO_CC).

File = CFE_FS_Header_t (64 bytes: 8 x uint32 big-endian, cfe/modules/fs/fsw/src/cfe_fs_api.c L234-242 swaps the
header to big-endian on a little-endian host, then char Description[32]) followed by one CFE_SB_PipeInfoEntry_t
per pipe in use, written in host byte order (little-endian here).  Entry layout of the running binary, from its
debug info (gdb ptype/o, BASELINE_MEASURE.md §2): PipeId u32 @0, AppId u32 @4, PipeName char[20] @8,
AppName char[20] @28, MaxQueueDepth u16 @48, CurrentQueueDepth u16 @50, PeakQueueDepth u16 @52, SendErrors u16 @54,
Opts u8 @56, Spare[3] @57; 60 bytes (cfe/modules/sb/config/default_cfe_sb_msgdefs.h L143-155,
CFE_MISSION_MAX_API_LEN 20).

Usage: sb_pipeinfo_parse.py <file> <out_prefix>   -> <out_prefix>.json, <out_prefix>.txt
"""
import json
import struct
import sys

HDR = 64
ENT = 60
path, prefix = sys.argv[1], sys.argv[2]
data = open(path, "rb").read()
h = struct.unpack(">8I", data[:32])
hdr = dict(zip(("ContentType", "SubType", "Length", "SpacecraftID", "ProcessorID", "ApplicationID", "TimeSeconds",
                "TimeSubSeconds"), h))
hdr["ContentType"] = "0x%08x" % hdr["ContentType"]
hdr["Description"] = data[32:64].split(b"\0", 1)[0].decode("latin-1")
body = data[HDR:]
entries = []
for i in range(len(body) // ENT):
    e = body[i * ENT:(i + 1) * ENT]
    pid, aid = struct.unpack("<II", e[0:8])
    mx, cur, peak, serr = struct.unpack("<HHHH", e[48:56])
    entries.append({"PipeId": "0x%08x" % pid, "AppId": "0x%08x" % aid,
                    "PipeName": e[8:28].split(b"\0", 1)[0].decode("latin-1"),
                    "AppName": e[28:48].split(b"\0", 1)[0].decode("latin-1"),
                    "MaxQueueDepth": mx, "CurrentQueueDepth": cur, "PeakQueueDepth": peak, "SendErrors": serr,
                    "Opts": e[56]})
res = {"file_size": len(data), "header": hdr, "entry_count": len(entries),
       "trailing_bytes": len(body) - len(entries) * ENT, "entries": entries}
json.dump(res, open(prefix + ".json", "w"), indent=1)
with open(prefix + ".txt", "w") as f:
    f.write("# %s: %d bytes, header %s\n" % (path, len(data), hdr))
    f.write("# entries=%d trailing_bytes=%d\n" % (len(entries), res["trailing_bytes"]))
    f.write("%-20s %-12s %5s %5s %5s %6s %4s\n" % ("PipeName", "AppName", "Max", "Cur", "Peak", "SndErr", "Opts"))
    for e in entries:
        f.write("%-20s %-12s %5d %5d %5d %6d %4d\n" % (e["PipeName"], e["AppName"], e["MaxQueueDepth"],
                                                      e["CurrentQueueDepth"], e["PeakQueueDepth"], e["SendErrors"],
                                                      e["Opts"]))
print("parsed %d entries from %s" % (len(entries), path))
