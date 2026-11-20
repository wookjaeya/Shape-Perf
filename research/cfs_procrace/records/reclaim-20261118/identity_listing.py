#!/usr/bin/env python3
# identity_listing.py ROOT PAIRS OUTDIR TAG
#
# For every pair (candidate, reference) of PAIRS (tab-separated: candidate, reference, candidate ledger row,
# reference ledger row; '#' lines skipped), both directories below ROOT: compute the sha256 of every regular file
# of the candidate and of the reference and write OUTDIR/listing_TAG_<candidate>.txt, one line per relative path:
#
#   status  relpath  size_candidate  sha256_candidate  size_reference  sha256_reference  mtime_candidate_utc  mode_candidate
#
# status: same (in both, same size and sha256), differs (in both, another size or sha256), only-candidate,
# only-reference.  Entries that are neither a regular file nor a directory (symbolic links, devices, sockets) are
# listed with status other-candidate / other-reference and make the candidate not eligible, as does an empty
# directory of the candidate (it has no file to compare).  The modification time and mode of the candidate's files
# are written because a removal loses them; they are not compared.
#
# OUTDIR/identity_summary_TAG.txt has one line per pair with the counts and the verdict:
#   identical-no-extra   every file of the candidate is `same`, the candidate has no other entry, the two
#                        directories are different directories and share no inode
#   not-eligible(<why>)  anything else
# Files of the reference that the candidate does not have are counted (only_reference) and do not change the verdict.
# Reads only below ROOT; refuses to overwrite an output file.  rc 0 when every pair was read (whatever the
# verdicts), 1 on a missing directory or an existing output, 64 on usage.
import hashlib
import os
import stat
import sys
import time


def sha256_of(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def scan(top):
    """relative path -> (kind, lstat) for every entry below top; kind is file, dir or other."""
    out = {}
    for root, dirs, files in os.walk(top, followlinks=False):
        for name in sorted(dirs) + sorted(files):
            p = os.path.join(root, name)
            st = os.lstat(p)
            if stat.S_ISREG(st.st_mode):
                kind = "file"
            elif stat.S_ISDIR(st.st_mode):
                kind = "dir"
            else:
                kind = "other"
            out[os.path.relpath(p, top)] = (kind, st)
    return out


def main(argv):
    if len(argv) != 5:
        sys.stderr.write("usage: identity_listing.py ROOT PAIRS OUTDIR TAG\n")
        return 64
    root, pairs_path, outdir, tag = argv[1:]
    pairs = []
    with open(pairs_path) as f:
        for line in f:
            line = line.rstrip("\n")
            if not line or line.startswith("#"):
                continue
            c = line.split("\t")
            if len(c) != 4:
                sys.stderr.write("identity_listing.py: bad pairs line: %r\n" % line)
                return 64
            pairs.append(c)
    summary_path = os.path.join(outdir, "identity_summary_%s.txt" % tag)
    targets = [summary_path] + [os.path.join(outdir, "listing_%s_%s.txt" % (tag, p[0])) for p in pairs]
    for t in targets:
        if os.path.exists(t):
            sys.stderr.write("identity_listing.py: output exists, not overwritten: %s\n" % t)
            return 1
    rows = []
    for cand, ref, cand_row, ref_row in pairs:
        cdir = os.path.join(root, cand)
        rdir = os.path.join(root, ref)
        for d in (cdir, rdir):
            if os.path.islink(d) or not os.path.isdir(d):
                sys.stderr.write("identity_listing.py: not a directory (or a symbolic link): %s\n" % d)
                return 1
        cs, rs = scan(cdir), scan(rdir)
        lines = []
        n = dict(same=0, differs=0, only_c=0, only_r=0, other_c=0, other_r=0, shared_inode=0, empty_dir_c=0)
        bytes_c = bytes_r = bytes_same = blocks_c = 0
        files_c = sum(1 for k in cs.values() if k[0] == "file")
        files_r = sum(1 for k in rs.values() if k[0] == "file")
        for rel in sorted(set(cs) | set(rs)):
            ck, cst = cs.get(rel, (None, None))
            rk, rst = rs.get(rel, (None, None))
            if ck == "dir" or (ck is None and rk == "dir"):
                if ck == "dir" and not any(k.startswith(rel + os.sep) for k in cs):
                    n["empty_dir_c"] += 1
                    lines.append("empty-dir-candidate\t%s\t-\t-\t-\t-\t-\t-" % rel)
                if ck == "dir" and rk not in ("dir", None):
                    n["only_c"] += 1
                continue
            csz = csha = rsz = rsha = mt = mode = "-"
            if ck == "file":
                csz = cst.st_size
                csha = sha256_of(os.path.join(cdir, rel))
                mt = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(cst.st_mtime))
                mode = "%04o" % stat.S_IMODE(cst.st_mode)
                bytes_c += cst.st_size
                blocks_c += cst.st_blocks * 512
            if rk == "file":
                rsz = rst.st_size
                rsha = sha256_of(os.path.join(rdir, rel))
                bytes_r += rst.st_size
            if ck == "other":
                status = "other-candidate"
                n["other_c"] += 1
            elif rk == "other":
                status = "other-reference"
                n["other_r"] += 1
                if ck == "file":
                    n["only_c"] += 1
            elif ck == "file" and rk == "file":
                if (cst.st_dev, cst.st_ino) == (rst.st_dev, rst.st_ino):
                    n["shared_inode"] += 1
                if csz == rsz and csha == rsha:
                    status = "same"
                    n["same"] += 1
                    bytes_same += cst.st_size
                else:
                    status = "differs"
                    n["differs"] += 1
            elif ck == "file":
                status = "only-candidate"
                n["only_c"] += 1
            else:
                status = "only-reference"
                n["only_r"] += 1
            lines.append("%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s" % (status, rel, csz, csha, rsz, rsha, mt, mode))
        why = []
        if os.path.realpath(cdir) == os.path.realpath(rdir):
            why.append("the same directory")
        if files_c == 0:
            why.append("no file")
        if n["differs"]:
            why.append("%d files differ" % n["differs"])
        if n["only_c"]:
            why.append("%d entries only in the candidate" % n["only_c"])
        if n["other_c"]:
            why.append("%d entries of the candidate are not regular files" % n["other_c"])
        if n["empty_dir_c"]:
            why.append("%d empty directories in the candidate" % n["empty_dir_c"])
        if n["shared_inode"]:
            why.append("%d files share an inode with the reference" % n["shared_inode"])
        if n["same"] != files_c and not why:
            why.append("same count %d is not the file count %d" % (n["same"], files_c))
        verdict = "identical-no-extra" if not why else "not-eligible(%s)" % "; ".join(why)
        with open(os.path.join(outdir, "listing_%s_%s.txt" % (tag, cand)), "x") as f:
            f.write("# candidate %s (ledger row %s)\n# reference %s (ledger row %s)\n" % (cdir, cand_row, rdir, ref_row))
            f.write("# status\trelpath\tsize_candidate\tsha256_candidate\tsize_reference\tsha256_reference\t"
                    "mtime_candidate_utc\tmode_candidate\n")
            for ln in lines:
                f.write(ln + "\n")
            f.write("# files_candidate %d bytes_candidate %d files_reference %d bytes_reference %d same %d "
                    "bytes_same %d verdict %s\n" % (files_c, bytes_c, files_r, bytes_r, n["same"], bytes_same, verdict))
        rows.append((cand, ref, cand_row, ref_row, files_c, bytes_c, blocks_c, files_r, bytes_r, n["same"], bytes_same,
                     n["differs"], n["only_c"], n["only_r"], n["other_c"], n["other_r"], n["empty_dir_c"],
                     n["shared_inode"], verdict))
    with open(summary_path, "x") as f:
        f.write("# root %s; run %s\n" % (root, time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())))
        f.write("# candidate\treference\tcandidate_row\treference_row\tfiles_candidate\tbytes_candidate\t"
                "allocated_bytes_candidate\tfiles_reference\tbytes_reference\tsame\tbytes_same\tdiffers\t"
                "only_candidate\tonly_reference\tother_candidate\tother_reference\tempty_dir_candidate\t"
                "shared_inode\tverdict\n")
        for r in rows:
            f.write("\t".join(str(x) for x in r) + "\n")
        ok = [r for r in rows if r[-1] == "identical-no-extra"]
        f.write("# pairs %d; identical-no-extra %d (files %d, bytes %d, allocated bytes %d); not eligible %d\n" % (
            len(rows), len(ok), sum(r[4] for r in ok), sum(r[5] for r in ok), sum(r[6] for r in ok),
            len(rows) - len(ok)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
