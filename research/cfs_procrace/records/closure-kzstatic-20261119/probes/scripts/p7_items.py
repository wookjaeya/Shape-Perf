#!/usr/bin/env python3
# P7: for each control row of a revision-4 plan, which name-binding items have their static inputs today, and
# which data state each needs.  Inputs: plan sidecar and manifest, relations text, name-space data, and the kept
# outputs of the probes P2 (success path) and P3 (buffer writers).  A statement about a run is never made here:
# "inputs present" means the item can be decided from a record, not that it will hold.
import re, sys, collections
build, nsd_path, outdir = sys.argv[1], sys.argv[2], sys.argv[3]
W = "/home/user/work/redesign/obs1"
side = open("%s/rf2_plans_v4/%s/plan.side" % (W, build), errors="replace").read().split("\n")
plan = open("%s/rf2_plans_v4/%s/plan.kzs" % (W, build), errors="replace").read().split("\n")
rel = open("%s/rf2_final_order/%s/relations.txt" % (W, build), errors="replace").read().split("\n")
nsd = open(nsd_path).read().split("\n")
p2 = open(outdir + "/p2_success_path_%s.txt" % build).read().split("\n")
p3 = open(outdir + "/p3_buffer_writers_%s.txt" % build).read().split("\n")
kinds = []; cur = None
for i, l in enumerate(nsd, 1):
    m = re.match(r"kind (\S+) value=(\d+)", l)
    if m: cur = dict(kind=m.group(1), value=m.group(2), line=i, ops=collections.defaultdict(list)); kinds.append(cur); continue
    m = re.match(r"\s+(lookup|create|delete|retire|clear|stored-name|other-store) (\S+)", l)
    if m and cur: cur["ops"][m.group(1)].append((m.group(2), i))
rows = [(m.group(1), m.group(2), l) for l in side for m in [re.match(r"row ([0-9a-f]{16}) (A\d+) ", l)] if m and "control-state=planned" in l]
print("# build %s; control rows planned %d; data: framework_namespaces_osal-f34759d.txt" % (build, len(rows)))
for key, rid, sl in rows:
    h = [i for i, l in enumerate(rel) if re.match(r"  %s lookup " % rid, l)][0]
    body = []
    for l in rel[h:]:
        if body and re.match(r"  [ABC]\d+ ", l): break
        body.append(l)
    ucal = re.match(r"  \S+ lookup \S+ in \S+ -> (\S+?)#", body[0]).group(1)
    crs = sorted(set(m.group(1) for l in body for m in [re.match(r"      (?:creator: .*?|not a pair: .*?): \S+ in \S+ -> (\S+) key", l)] if m))
    cact = sorted(set(a for l in body if re.match(r"      (creator:|not a pair:)", l) for a in re.findall(r"actors \{([^}]*)\}", l)))
    uact = next((re.search(r"actors: \{([^}]*)\}", l).group(1) for l in body if l.strip().startswith("actors:")), "")
    ab = next((l.strip() for l in body if l.strip().startswith("absence:")), "")
    part = next((l.strip() for l in body if l.strip().startswith("partition:")), "")
    lay = next((l.strip() for l in body if l.strip().startswith("layers:")), "")
    ent = [k for k in kinds if any(f == ucal for f, _ in k["ops"]["lookup"])]
    entc = [k for k in kinds if any(f in crs for f, _ in k["ops"]["create"])]
    e = ent[0] if ent else None
    sites = collections.Counter(re.search(r"label=s\d+\.(\S+)", l).group(1) for l in plan if re.match(r"site \d+ ", l) and re.search(r"row=\S*%s" % key, l))
    needs = {m.group(1): m.group(2) for l in side for m in [re.match(r"need row %s (\S+) (\S+)" % key, l)] if m}
    roots = set(m.group(1) for l in plan for m in [re.match(r"site \d+ label=s\d+\.actor-root op=entry\.(\S+)", l)] if m)
    def root_ok(a): return all((x.split(" (")[0] in roots) or x.startswith("main (process)") for x in a.split(", ") if x)
    b4 = {c: next((("HELD" if "=> basis 4 static line: HELD" in l else "MISSING") for l in p2 if l.startswith(c + " ") or l.startswith(c + ":")), "no line") for c in crs}
    buf = next((l for l in p3 if e and (" %s " % e["kind"]) in l and "condition-input" in l), None)
    print("row %s %s use callee %s; consumer actors {%s}; creator callees %s; creator actors %s" % (rid, key, ucal, uact, crs, cact))
    T = []
    T.append(("basis 1 same build", "both", "needs the build-id records (runtime step); today the chain record says same_build unconfirmed"))
    T.append(("basis 2 ties", "both", "root sites in the plan for the consumer %s and the creators %s" % ("yes" if uact and root_ok(uact) else "NO (%s)" % (uact or "no actor root reaches the use"), "yes" if cact and all(root_ok(a) for a in cact) else "NO")))
    T.append(("basis 3 absence constant", "both", "row absence line: %s" % ("value with a no-match chain, reached" if re.match(r"absence: -?\d+ \(no match", ab) and ": reached" in ab else "NOT evaluated (%s)" % ab[:60])))
    T.append(("basis 4 creator success", "both", "; ".join("%s: static line %s (probe P2)" % kv for kv in b4.items()) or "no creator callee in the row"))
    T.append(("basis 5 key texts", "both", "plan sites of this row: key-text %d, creator-call %d, use-call %d" % (sites.get("key-text", 0), sites.get("creator-call", 0), sum(v for k, v in sites.items() if "use-call" in k))))
    T.append(("basis 6 create after use", "both", "decided by the record (times of the two calls)"))
    T.append(("(ga) name-space entry", "both", ("entry %s (data line %d) has the lookup %s and the create %s" % (e["kind"], e["line"], ucal, [c for c in crs if any(f == c for f, _ in e["ops"]["create"])]) if e and all(any(f == c for f, _ in e["ops"]["create"]) for c in crs) and crs else "NO entry names the lookup %s%s" % (ucal, " (an entry names the create only: %s)" % entc[0]["kind"] if entc else ""))))
    T.append(("(na) comparison semantics", "state 2 only", "the comparison's C library call is in the row's layers line: %s" % (re.search(r"P0-unverified\([^)]*\)", lay).group(0) if "P0-unverified" in lay else "no unverified entry")))
    T.append(("(na) bound", "both, after the plan carries the extent value", "stored-name entry of the kind: %s" % ("yes" if e and e["ops"]["stored-name"] else "NO")))
    T.append(("(da) no other name mapping", "state 2 only, and with the buffer-escape rule", ("buffer field row of probe P3: " + re.search(r"per-field=\S+", buf).group(0) + ", " + re.search(r"copy-at-entry=\d+ fill0-at-entry=\d+ fill0-elsewhere=\d+ other=\d+", buf).group(0)) if buf else "no stored-name entry for the row's kind"))
    T.append(("(ra) same kind", "both", "static half: %s; run half: lookup-side value at the key-text site of the lookup API; create-side value: no site today" % ("partition line says it selects the table" if "selects the table" in part else "NO partition line that selects the table")))
    T.append(("(ma) no delete between", "both", "object-delete need: %s" % needs.get("object-delete", "none")))
    for name, st, txt in T: print("   %-28s | data state: %-46s | %s" % (name, st, txt))
