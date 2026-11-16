#!/usr/bin/env python3
# vocab_check.py JR VOCAB -- the reason vocabulary data file of kairos (tests/static/data/reason_vocabulary.txt) against
# the table of reasons in section 3.5 of the judgement rules (plan/ovm_rev1_20261116/JUDGEMENT_RULES_REV1.md): the reason
# names of the table's first column (in order; a cell may name several) must be the lines of the data file (in order).
# Prints SAME or DIFF with both lists; exits 0 / 1.
import re
import sys


def main() -> int:
    jr, vocab = sys.argv[1], sys.argv[2]
    text = open(jr, encoding="utf-8").read()
    sec = text[text.index("### 3.5"):text.index("### 3.6")]
    names = []
    for line in sec.splitlines():
        if line.startswith("| `"):
            names += re.findall(r"`([a-z_]+)`", line.split("|")[1])
    lines = [l for l in open(vocab, encoding="utf-8").read().splitlines() if l and not l.startswith("#")]
    same = names == lines
    print(("SAME" if same else "DIFF") + " table %d reasons, data file %d lines" % (len(names), len(lines)))
    if not same:
        print("table:", names)
        print("file: ", lines)
    return 0 if same else 1


if __name__ == "__main__":
    sys.exit(main())
