#!/usr/bin/env python3
"""hand_compare_record.py -- the hand-manifest record of 2026-10-29 (W6 G-I natural boot, pre_1df262a_tx) against the
record of the generated plan, for the actors of relation 1 (comparison material only; the hand manifest is no input of
the generated path).

  python3 -I hand_compare_record.py --hand-dump tx.dump.txt --gen-dump tx.dump.txt --gen-record RECORD.json [--map FILE]

The site correspondence is the one measured before the boot (records/obs1-20261116-plangen/hand_compare_c1_pre.txt,
lines "MATCH hand <n> ... gen <m>[; gen <m2>]"; same function, place, callee and line).  The actors: in the generated
record the consumer and the publisher of the record; in the hand record the actor whose events hold the hand site of
the matched consumer use (at-call of the lookup) and the actor of the matched publisher create.  For every matched pair
both records' events are printed with their values; then the events of those actors at sites only one plan has.
"""
import argparse
import json
import os
import re

EV = re.compile(r"^ev actor=(\S+) aseq=(\d+) site=(\d+)\[([^\]]*)\].* t=(\d+) .*v=\[([^\]]*)\](?: str=(\"(?:[^\"\\]|\\.)*\"))? flags=")


def events(path):
    out = []
    for i, ln in enumerate(open(path).read().splitlines(), 1):
        m = EV.match(ln)
        if m:
            out.append({"line": i, "actor": m.group(1), "aseq": int(m.group(2)), "site": int(m.group(3)), "label": m.group(4),
                        "t": int(m.group(5)), "v": m.group(6), "str": m.group(7)})
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--hand-dump", required=True)
    ap.add_argument("--gen-dump", required=True)
    ap.add_argument("--gen-record", required=True)
    ap.add_argument("--map", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "obs1-20261116-plangen", "hand_compare_c1_pre.txt"))
    o = ap.parse_args()
    pairs = {}
    for ln in open(o.map):
        m = re.match(r"MATCH\s+hand\s+(\d+)\s+(\S+).*?gen (\d+)(.*)$", ln)
        if m:
            gens = [int(m.group(3))] + [int(x) for x in re.findall(r"gen (\d+)", m.group(4))]
            pairs[int(m.group(1))] = (m.group(2), gens)
    rec = json.load(open(o.gen_record))
    gc = rec["consumer_actor"]["run_actor"]
    gp = [p["run_actor"] for p in rec["publisher_actors"]]
    H, G = events(o.hand_dump), events(o.gen_dump)
    g_use = {u["at_call"]["site"] for u in rec["order"]["use_events"] if "at_call" in u}
    gm = {x["publisher_actor"] for x in rec["link"]["instance_evidence"] if "differs_from_use_key" not in x}
    g_pub = {p["at_call"]["site"] for p in rec["order"]["publish_events"] if "at_call" in p and p["actor"] in gm}
    use_at = [s for s, (lab, g) in pairs.items() if g_use & set(g)]
    create_at = [s for s, (lab, g) in pairs.items() if g_pub & set(g)]
    hc = sorted({e["actor"] for e in H if e["site"] in use_at})
    hp = sorted({e["actor"] for e in H if e["site"] in create_at})
    print("hand record %s; generated record %s" % (o.hand_dump, o.gen_dump))
    print("actors: consumer hand %s / generated %s; publisher hand %s / generated %s" % (",".join(hc), gc, ",".join(hp), ",".join(gp)))
    print("\n== matched sites (both plans): hand events | generated events")
    hands = set(hc + hp)
    gens = set([gc] + gp)
    seen_g = set()
    for s, (lab, gs) in sorted(pairs.items()):
        he = [e for e in H if e["site"] == s and e["actor"] in hands]
        ge = [e for e in G if e["site"] in gs and e["actor"] in gens]
        for e in ge:
            seen_g.add((e["actor"], e["aseq"]))
        if not he and not ge:
            continue
        print("hand %d %s / gen %s" % (s, lab, ",".join(map(str, gs))))
        for e in he:
            print("  hand  L%-4d actor %s aseq %d v=[%s]%s" % (e["line"], e["actor"], e["aseq"], e["v"], " str=" + e["str"] if e["str"] else ""))
        for e in ge:
            print("  gen   L%-4d actor %s aseq %d site %d v=[%s]%s" % (e["line"], e["actor"], e["aseq"], e["site"], e["v"], " str=" + e["str"] if e["str"] else ""))
    print("\n== hand-only events of those actors (sites the generated plan does not have)")
    for e in H:
        if e["actor"] in hands and e["site"] not in pairs:
            print("  hand  L%-4d actor %s aseq %d site %d[%s] v=[%s]%s" % (e["line"], e["actor"], e["aseq"], e["site"], e["label"], e["v"], " str=" + e["str"] if e["str"] else ""))
    print("\n== generated-only events of those actors (sites the hand manifest does not have)")
    for e in G:
        if e["actor"] in gens and (e["actor"], e["aseq"]) not in seen_g:
            print("  gen   L%-4d actor %s aseq %d site %d[%s] v=[%s]%s" % (e["line"], e["actor"], e["aseq"], e["site"], e["label"], e["v"], " str=" + e["str"] if e["str"] else ""))


if __name__ == "__main__":
    main()
