#!/usr/bin/env python3
"""row_sites.py -- the sites of one planned row, taken from the sidecar's map line, with each site's binding and its
events in the dump (python3 -I row_sites.py --sidecar plan.side --manifest plan.kzs --bindings binds.kzs --dump-text
tx.dump.txt --key USE-SITE-KEY).  Replaces a hand count: the row's site set is the sidecar map line, not a match on
the manifest's ref text (a store site's ref carries the row key followed by its source position)."""
import argparse, collections, hashlib, re, sys

ap = argparse.ArgumentParser()
for a in ("sidecar", "manifest", "bindings", "dump-text", "key"):
    ap.add_argument("--" + a, required=True)
o = ap.parse_args()


def rd(p):
    b = open(p, "rb").read()
    return b.decode("utf-8", "replace"), hashlib.sha256(b).hexdigest()


side, hs = rd(o.sidecar)
man, hm = rd(o.manifest)
bnd, hb = rd(o.bindings)
dmp, hd = rd(o.dump_text)
print("# row_sites.py: sidecar %s; manifest %s; bindings %s; dump text %s; key %s" % (hs[:16], hm[:16], hb[:16], hd[:16], o.key))
mp = [l for l in side.splitlines() if l.startswith("map %s " % o.key)]
if len(mp) != 1:
    print("map lines for the key: %d" % len(mp))
    sys.exit(2)
ids = [int(x) for x in mp[0].split()[2:]]
print("# sidecar map line: %s (%d sites)" % (mp[0], len(ids)))
sites = {}
for l in man.splitlines():
    m = re.match(r"site (\d+) (.*)$", l)
    if m:
        sites[int(m.group(1))] = dict((k, v.strip('"')) for k, v in re.findall(r'(\w+)=("[^"]*"|\S+)', m.group(2)))
binds = collections.defaultdict(list)
for l in bnd.splitlines():
    m = re.match(r"bind (\d+) site=(\d+) .*status=(\w+)", l)
    if m:
        binds[int(m.group(2))].append((m.group(3), m.group(1)))
evs = collections.defaultdict(collections.Counter)
for l in dmp.splitlines():
    m = re.match(r"ev actor=(\S+) aseq=\d+ site=(\d+)\[", l)
    if m:
        evs[int(m.group(2))][m.group(1)] += 1
print("site\tlabel\tfunc\tplace\tcallee\tline\tref_is_exactly_row_key\tbinding\tbind_ids\tevents\tevents_per_actor")
nb = ne = 0
for n in ids:
    s = sites.get(n, {})
    ref = s.get("ref", "")
    b = binds.get(n, [])
    st = "+".join(sorted({x[0] for x in b})) or "none"
    e = sum(evs[n].values())
    nb += st != "none"
    ne += e > 0
    print("\t".join([str(n), s.get("label", "?"), s.get("func", "?"), s.get("place") or s.get("op", "?"), s.get("callee", "-"),
                     s.get("line", "-"), "yes" if ref == "row %s" % o.key else "no (%s)" % ref, st, ",".join(x[1] for x in b) or "-",
                     str(e), ",".join("%s:%d" % kv for kv in sorted(evs[n].items(), key=lambda x: int(x[0]))) or "-"]))
print("summary\tsites %d; with a binding line %d; with events in the dump %d; without events %d: %s"
      % (len(ids), nb, ne, len(ids) - ne, ",".join(str(n) for n in ids if not evs[n]) or "-"))
