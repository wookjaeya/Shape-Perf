#!/usr/bin/env python3
"""need3.py -- the per-boot space need of the C1 cycle restated from kept measurements (results 26.9; review findings
fix-judge-02, fix-decl-01, fix-decl-03 in records/closure-review-bootfix-20261119/findings.tsv; evaluation protocol
14.14 item 4).  Run with python3 -I.  Every part is read from a kept file named on its line; nothing is typed by hand
but the floor and the margin, which are those of boot declaration 3 (314,572,800 B and 100,000,000 B).

    need3.py --research DIR --free BYTES --out need.txt

Parts of one boot (block bytes, as du -sB1 counts them; a part read from several kept files takes the largest):
  boot      the dip of the one natural boot of 2026-11-16 (records/closure-space-20261119/need.txt, per_boot.boot)
  convert   the blocks of that boot's convert output (the same need.txt, per_boot.convert)
  explain16 the blocks of the 16-branch x 46-record explanation outputs of the boot shape (the four synthetic records of
            results 26.3, records/closure-prebootfix-20261119/out/xr5_sizes.txt, du -sB1 lines xr5_synth_c1_*_script and
            _reverse; the superseded first trial, 52 records per branch, is another shape and is not used)
  eval      the largest dip of the frozen eval script on the kept outputs (records/closure-prebootfix-20261119/guard/eval-*.log)
  expect    the blocks of the largest kept expectation comparison (records/closure-prebootfix-20261119/compare/*_expect.txt)
  records   the blocks of the 2026-11-16 boot records (the same need.txt, per_boot.records)
  verify    three verify outputs per boot (the verify command, the boot step's, the explain step's): 3 x the blocks of the
            larger kept boot0/verify.txt of the two builds (records/closure-c1prep-20261119/boot/<build>/boot0/verify.txt)
  eval_tail the blocks of the largest kept eval head and tail (records/closure-prebootfix-20261119/out/eval_*_tail.txt)
  lists     the keep step's sha256 lists: the blocks of the largest member list of the synthetic records
            (/home/user/work/preserve/archives-20261119/xr5_synth_c1_*/MEMBERS.sha256, the explanation directory's shape)
            plus one block for each of the three small lists (boot, convert, eval)
Needs: before the cycle's first boot (c1_pre_tx 1) floor + margin + 12 x per boot; before c1_post_tx 1 floor + margin +
6 x per boot (its boots 1, 1r, 2-5 at most; protocol 14.14 item 4).
"""
import argparse
import glob
import os
import re
import sys

FLOOR = 314572800
MARGIN = 100000000


def blocks(p):
    return os.stat(p).st_blocks * 512


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--research", required=True)
    ap.add_argument("--free", required=True, type=int)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    R = a.research
    if os.path.exists(a.out):
        print("need3.py: %s exists" % a.out, file=sys.stderr)
        return 64
    lines = []
    old = open(os.path.join(R, "records/closure-space-20261119/need.txt"), encoding="utf-8").read()

    def old_part(name):
        m = re.search(r"^per_boot\.%s (\d+) " % re.escape(name), old, re.M)
        if not m:
            raise SystemExit("need3.py: no per_boot.%s in the old need.txt" % name)
        return int(m.group(1))

    parts = []
    parts.append(("boot", old_part("boot"), "records/closure-space-20261119/need.txt per_boot.boot"))
    parts.append(("convert", old_part("convert"), "records/closure-space-20261119/need.txt per_boot.convert"))
    sz = open(os.path.join(R, "records/closure-prebootfix-20261119/out/xr5_sizes.txt"), encoding="utf-8").read()
    blk = sz.split("## du -sB1", 1)[1]
    syn = {m.group(2): int(m.group(1)) for m in re.finditer(r"^(\d+)\t(xr5_synth_c1_\w+_(?:script|reverse))$", blk, re.M)}
    if len(syn) != 4:
        raise SystemExit("need3.py: expected four synthetic records in xr5_sizes.txt, found %d" % len(syn))
    k = max(syn, key=syn.get)
    parts.append(("explain16", syn[k], "records/closure-prebootfix-20261119/out/xr5_sizes.txt du -sB1 %s (largest of %s)"
                  % (k, ", ".join("%s %d" % (x, syn[x]) for x in sorted(syn)))))
    dips = {}
    for p in sorted(glob.glob(os.path.join(R, "records/closure-prebootfix-20261119/guard/eval-*.log"))):
        m = re.findall(r" dip=(\d+) ", open(p, encoding="utf-8").read())
        if m:
            dips[os.path.basename(p)] = int(m[-1])
    k = max(dips, key=dips.get)
    parts.append(("eval", dips[k], "records/closure-prebootfix-20261119/guard/%s dip (largest of %d eval logs)" % (k, len(dips))))
    ex = {os.path.basename(p): blocks(p) for p in glob.glob(os.path.join(R, "records/closure-prebootfix-20261119/compare/*_expect.txt"))}
    k = max(ex, key=ex.get)
    parts.append(("expect", ex[k], "records/closure-prebootfix-20261119/compare/%s blocks (largest of %d)" % (k, len(ex))))
    parts.append(("records", old_part("records"), "records/closure-space-20261119/need.txt per_boot.records"))
    vf = {b: blocks(os.path.join(R, "records/closure-c1prep-20261119/boot/%s/boot0/verify.txt" % b)) for b in ("c1_pre_tx", "c1_post_tx")}
    k = max(vf, key=vf.get)
    parts.append(("verify", 3 * vf[k], "3 x blocks of records/closure-c1prep-20261119/boot/%s/boot0/verify.txt (%d)" % (k, vf[k])))
    tl = {os.path.basename(p): blocks(p) for p in glob.glob(os.path.join(R, "records/closure-prebootfix-20261119/out/eval_*_tail.txt"))}
    k = max(tl, key=tl.get)
    parts.append(("eval_tail", tl[k], "records/closure-prebootfix-20261119/out/%s blocks (largest of %d)" % (k, len(tl))))
    ml = {p.split("/")[-2]: blocks(p) for p in glob.glob("/home/user/work/preserve/archives-20261119/xr5_synth_c1_*/MEMBERS.sha256")
          if not p.split("/")[-2].endswith("_trial1")}
    k = max(ml, key=ml.get)
    parts.append(("lists", ml[k] + 3 * 4096, "blocks of /home/user/work/preserve/archives-20261119/%s/MEMBERS.sha256 (%d, largest of %d) + 3 x 4096"
                  % (k, ml[k], len(ml))))
    per = sum(v for _, v, _ in parts)
    need12 = FLOOR + MARGIN + 12 * per
    need6 = FLOOR + MARGIN + 6 * per
    lines.append("# kz-space3-need 1 (records/closure-space3-20261119/need3.py); the parts are [M] in the kept files named, "
                 "their sum per boot is [I] until boots have run (protocol 14.14 item 4)")
    lines.append("floor %d (boot declaration 3)" % FLOOR)
    lines.append("margin %d (boot declaration 3)" % MARGIN)
    for n, v, src in parts:
        lines.append("per_boot.%s %d (%s)" % (n, v, src))
    lines.append("per_boot %d" % per)
    lines.append("per_boot_declared3 51810304 (boot declaration 3; difference %d)" % (per - 51810304))
    lines.append("need_free %d (before c1_pre_tx boot 1: floor + margin + 12 x per_boot)" % need12)
    lines.append("need_post1 %d (before c1_post_tx boot 1: floor + margin + 6 x per_boot)" % need6)
    lines.append("free_now %d" % a.free)
    lines.append("short_now %d" % max(0, need12 - a.free))
    open(a.out, "w", encoding="utf-8").write("\n".join(lines) + "\n")
    print("per_boot %d need_free %d need_post1 %d free_now %d short_now %d" % (per, need12, need6, a.free, max(0, need12 - a.free)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
