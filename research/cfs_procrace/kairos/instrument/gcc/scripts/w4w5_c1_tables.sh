#!/usr/bin/env bash
# w4w5_c1_tables.sh <out root> -- W4/W5 (2026-10-28) C1: summary TSVs from the outputs of w4w5_c1_all.sh (replay/) and
# w4w5_c1_occ.sh (occ/).  Reads only those outputs and the records' own text files (run.meta, smoke.meta, hold.meta,
# driver.txt, the instance console); nothing is replayed.  Pilot/dev and controlled runs only.  Writes <out root>/tables/:
#   records.tsv     one row per record: path, kernel, build, configuration, instance, plan, loss, events, occurrences,
#                   realized class (CF's lookup results per key and CF's end), console events of CF and BP, realized control
#   occ_rows.tsv    one row per occurrence and reading (kz occ show): key name, ordinal, owner, status, summary, basis kinds,
#                   assumptions, outcome check, edge rules (positions left out)
#   classes.tsv     per set, plan or natural configuration, and reading: the per-instance judgement signature (key,
#                   ordinal range, status, summary, basis kinds) and the instances that have it
#   identity.tsv    per record and successful lookup: the out id, the executing task's owner, the provider's owner, relation
#   control.tsv     per controlled plan instance: the realized control lines (control_cmp.sh) of the four records and the
#                   control edges of the replay
#   withheld.tsv    per controlled record: the occurrences withheld as INCONCLUSIVE(horizon-open) by the reading without
#                   control order and without P1 (ctloff_p1off) and how many of them p1off (control order on) judges
set -uo pipefail
O=${1:?out root}; O="$(cd "$O" && pwd)"; mkdir -p "$O/tables"
python3 -I - "$O" <<'PY'
import os, re, sys, glob, collections
O = sys.argv[1]; RP = O + "/replay"; T = O + "/tables"
SRC = {"p_f0tx": "/home/user/work/procrace/c1_runs/smoke3", "p_f0rx": "/home/user/work/procrace/c1_runs/confirm1",
       "p_gi": "/home/user/work/instr/gcc/d3/c1", "p_f0ctl": "/home/user/work/procrace/c1_runs/confirm2",
       "p_gih": "/home/user/work/instr/gcc/f1/gih", "w_f0tx": "/home/user/work/w5/run/c1nat/f0tx",
       "w_f0rx": "/home/user/work/w5/run/c1nat/f0rx", "w_gi": "/home/user/work/w5/run/c1nat/gi",
       "w_f0ctl": "/home/user/work/w5/run/c1ctl/f0", "w_gih": "/home/user/work/w5/run/c1ctl/gih"}
def rd(p):
    try: return open(p, errors="replace").read()
    except OSError: return ""
def parse_name(s, b):
    x = b[2:] if b.startswith("N_") else b
    plan = "N"
    if x.startswith("W"): plan, x = x.split("_", 1)
    f = x.split("_"); inst = f[2] if len(f) > 2 else "tx"
    return plan, f[0], f[1], inst
def path_of(s): return "F0" if "f0" in s else ("G-I-H" if s.endswith("gih") else "G-I")
def build_of(s): return "ref" if "f0" in s else ("d2" if s == "p_gi" else "f1")
def occ_rows(f):
    rows = []
    for line in rd(f).splitlines():
        if not line.startswith("#"): continue
        p = line.split(" | ")
        if len(p) < 10: continue
        st, sm = p[6].split(" ", 1)
        m = re.match(r"basis \{([^}]*)\} assume \{([^}]*)\}.* outcome (\S+)", p[7])
        name = re.sub(r"^args sem_name=", "", p[8]).split()[0] if "sem_name=" in p[8] else p[8]
        edges = " ".join(sorted(set(re.sub(r"#\d+->#\d+", "", e) for e in p[9].split()[2:])))
        rows.append(dict(rspec=p[0].split(" ", 1)[1], owner=p[2], res=p[3], ord=int(p[4]), exec=p[5], status=st, summary=sm,
                         basis=m.group(1) if m else "?", assume=m.group(2) if m else "?", outcome=m.group(3) if m else "?",
                         name=name, edges=edges))
    return rows
def strip_class(c): return re.sub(r" \[[^]]*\]", "", c).strip()
recs = []
for s in sorted(SRC):
    for d in sorted(glob.glob(RP + "/" + s + "/*/")):
        b = os.path.basename(d.rstrip("/")); R = SRC[s] + "/" + b
        plan, cfg, cand, inst = parse_name(s, b)
        meta = rd(R + "/run.meta") + rd(R + "/smoke.meta") + rd(R + "/hold.meta")
        k = re.search(r"kernel=(\S+)", meta); kernel = k.group(1) if k else "?"
        cur = rd(d + "cur.txt"); conv = rd(d + "convert.txt")
        loss = re.search(r"^loss .*$", cur, re.M); loss = loss.group(0) if loss else "?"
        ev = re.search(r"raw events (\d+), trace events (\d+)", cur)
        nl = re.search(r"neutral layer: .*events (\d+).*losses (\d+), control records (\d+)", conv)
        events = ("raw %s trace %s" % ev.groups()) if ev else ""
        if nl: events += (" " if events else "") + "neutral events %s losses %s ctl %s" % nl.groups()
        rc = re.search(r"realized control: records (\d+), releases that resumed a held thread (\d+), control edges (\d+)", cur)
        cls = strip_class(rd(d + "class.txt").splitlines()[0]) if rd(d + "class.txt") else "?"
        con = rd(R + "/inst/" + inst + "/console_output.log")
        cev = []
        for l in con.splitlines():
            m = re.search(r"/(CF|BP) (\d+): (.*)$", l)
            if m and (m.group(1) == "CF" or m.group(2) == "15"):
                t = m.group(3)
                t = "fail " + re.search(r"name (\S+),", t).group(1) if "failed to get sem id" in t else ("created " + t.split()[-1] if "Created semaphore" in t else t[:30])
                cev.append(m.group(1) + m.group(2) + ":" + t)
        ctl = rd(d + "control.txt").split("\n"); ctl = [x for x in ctl if x]
        holds = sum(1 for x in ctl if x.startswith("hold ")); res = sum(1 for x in ctl if x.endswith(" resumed"))
        sat = sum(1 for x in ctl if x.endswith(" satisfied")); unrel = sum(1 for x in ctl if x.startswith("unreleased"))
        exits = rd(d + "exit.txt").split(); bad = [exits[i] for i in range(0, len(exits) - 1, 2) if exits[i + 1] != "0"]
        rows = {r: occ_rows(d + "occ_" + r + ".txt") for r in ("cur", "p1off", "nocode", "ctloff_p1off", "ctloff") if os.path.exists(d + "occ_" + r + ".txt")}
        recs.append(dict(set=s, run=b, d=d, R=R, plan=plan, cfg=cfg, cand=cand, inst=inst, kernel=kernel, loss=loss, events=events,
                         nocc=len(rows["cur"]), cls=cls, console=" ; ".join(cev), ctl=ctl, holds=holds, res=res, sat=sat, unrel=unrel,
                         rc=rc.groups() if rc else ("", "", ""), bad=",".join(bad) or "ok", rows=rows, cur=cur))
with open(T + "/records.tsv", "w") as f:
    f.write("set\trun\tpath\tkernel\tbuild\tcfg\tcand\tinst\tplan\tloss\tevents\toccurrences\tclass\tconsole_CF_BP\tholds\tresumed\tsatisfied\tunreleased\tnctl_records\tnctl_resumed\tcontrol_edges\tsteps\n")
    for r in recs:
        f.write("\t".join(map(str, [r["set"], r["run"], path_of(r["set"]), r["kernel"], build_of(r["set"]), r["cfg"], r["cand"], r["inst"], r["plan"],
                r["loss"], r["events"], r["nocc"], r["cls"], r["console"], r["holds"], r["res"], r["sat"], r["unrel"]] + list(r["rc"]) + [r["bad"]])) + "\n")
with open(T + "/occ_rows.tsv", "w") as f:
    f.write("set\trun\treading\tname\tordinal\towner\tstatus\tsummary\tbasis\tassume\toutcome\tedges\n")
    for r in recs:
        for rdg, rows in r["rows"].items():
            for o in rows:
                f.write("\t".join(map(str, [r["set"], r["run"], rdg, o["name"], o["ord"], o["owner"], o["status"], o["summary"], o["basis"], o["assume"], o["outcome"], o["edges"]])) + "\n")
def signature(rows):
    out, prev = [], None
    for o in sorted(rows, key=lambda o: (o["name"], o["ord"])):
        k = (o["name"], o["status"], o["summary"], o["basis"])
        if prev and prev[0] == k and prev[2] == o["ord"] - 1: prev[2] = o["ord"]; continue
        prev = [k, o["ord"], o["ord"]]; out.append(prev)
    return " ; ".join("%s %s %s %s {%s}" % (k[0], str(a) if a == b else "%d-%d" % (a, b), k[1], k[2], k[3]) for k, a, b in out)
with open(T + "/classes.tsv", "w") as f:
    f.write("set\tgroup\treading\tinstances\tn\tsignature\n")
    g = collections.OrderedDict()
    for r in recs:
        for rdg, rows in r["rows"].items():
            g.setdefault((r["set"], r["plan"] + "_" + r["cfg"], rdg, signature(rows)), []).append(r["cand"][:2] + "-" + r["inst"])
    for (s, grp, rdg, sig), inst in g.items(): f.write("%s\t%s\t%s\t%s\t%d\t%s\n" % (s, grp, rdg, ",".join(inst), len(inst), sig))
with open(T + "/identity.tsv", "w") as f:
    f.write("set\trun\tname\tordinal\tstatus\tout_id\texec_owner\tprovider_owner\trelation\n")
    for r in recs:
        txt = r["cur"]; idn = dict(re.findall(r"^identity (\S+?):? creator .*? owner confirmed id \S+ record (\d+)", txt, re.M))
        for blk in re.split(r"^occurrence ", txt, flags=re.M)[1:]:
            h = re.match(r"\S+ actor (\S+) ordinal (\d+)", blk); st = re.search(r"^  status (\S+)", blk, re.M)
            out = re.search(r"^  out sem_id=(\S+)", blk, re.M); nm = re.search(r'sem_name text "([^"]+)"', blk)
            ex = re.search(r"^  identity exec \S+ creator \S+ owner (\S+) \[(\d+)\]", blk, re.M)
            rel = re.search(r"^  relation (\S+) owners confirmed: provider\(s\) (.*)$", blk, re.M)
            prov = ",".join(idn.get(p.strip(), "?") for p in rel.group(2).split(",")) if rel else "-"
            f.write("\t".join([r["set"], r["run"], nm.group(1) if nm else "?", h.group(2) if h else "?", st.group(1) if st else "?",
                               out.group(1) if out else "-", ("%s[%s]" % ex.groups()) if ex else "?", prov, rel.group(1) if rel else "?"]) + "\n")
by = {(r["set"], r["run"]): r for r in recs}
with open(T + "/control.tsv", "w") as f:
    f.write("run\tp_f0ctl\tp_gih\tw_f0ctl\tw_gih\tall_equal\tcontrol_edges(p_f0,p_gih,w_f0,w_gih)\n")
    for r in recs:
        if r["set"] != "p_f0ctl": continue
        sides = [by.get((s, r["run"])) for s in ("p_f0ctl", "p_gih", "w_f0ctl", "w_gih")]
        L = [" | ".join(x["ctl"]) if x else "missing" for x in sides]
        f.write("\t".join([r["run"]] + L + ["yes" if len(set(L)) == 1 else "NO", ",".join(x["rc"][2] if x else "-" for x in sides)]) + "\n")
with open(T + "/withheld.tsv", "w") as f:
    f.write("set\trun\tplan\twithheld_ctloff_p1off\tjudged_p1off\tjudged_with_control_order\tremaining\tremaining_keys\tjudged_nocode\n")
    for r in recs:
        if "ctloff_p1off" not in r["rows"]: continue
        key = lambda o: (o["name"], o["ord"])
        w = [o for o in r["rows"]["ctloff_p1off"] if o["summary"].startswith("INCONCLUSIVE") and "horizon-open" in o["summary"]]
        p1 = {key(o): o for o in r["rows"]["p1off"]}; nc = {key(o): o for o in r["rows"]["nocode"]}
        j = [o for o in w if not p1[key(o)]["summary"].startswith("INCONCLUSIVE")]
        jc = [o for o in j if "control-order" in p1[key(o)]["basis"]]
        rem = [o for o in w if p1[key(o)]["summary"].startswith("INCONCLUSIVE")]
        jn = [o for o in w if not nc[key(o)]["summary"].startswith("INCONCLUSIVE")]
        f.write("\t".join(map(str, [r["set"], r["run"], r["plan"], len(w), len(j), len(jc), len(rem), ",".join("%s#%d" % key(o) for o in rem) or "-", len(jn)])) + "\n")
# ad62.tsv: one row per configuration group (natural pre/post, the nine plans), the units of AD section 6.2, each value
# with the sets that have it ("=" joins sets whose value is the same)
def grouped(vals):  # {set: value} -> "value [sets] || value [sets]"
    inv = collections.OrderedDict()
    for s, v in vals.items(): inv.setdefault(v, []).append(s)
    return " || ".join("%s [%s]" % (v, " ".join(ss)) for v, ss in inv.items())
groups = collections.OrderedDict()
for r in recs: groups.setdefault(r["plan"] + "_" + r["cfg"], []).append(r)
wh = {}
for line in open(T + "/withheld.tsv").read().splitlines()[1:]:
    f = line.split("\t"); wh.setdefault((f[0], f[2]), []).append((int(f[3]), int(f[4]), int(f[6]), int(f[8])))
with open(T + "/ad62.tsv", "w") as f:
    f.write("group\trecords\texecution_class\tconsole_CF_BP\tloss\tevents\tidentity\tcur\tnocode\tcontrol\twithheld_judged_remaining_nocode_per_instance\n")
    for gname, rs in groups.items():
        sets = collections.OrderedDict()
        for r in rs: sets.setdefault(r["set"], []).append(r)
        def per_set(fn): return grouped(collections.OrderedDict((s, "; ".join(sorted(set(fn(x) for x in v)))) for s, v in sets.items()))
        idt = collections.OrderedDict()
        for s, v in sets.items():
            ids = set()
            for x in v:
                for blk in re.split(r"^occurrence ", x["cur"], flags=re.M)[1:]:
                    nm = re.search(r'sem_name text "([^"]+)"', blk); out = re.search(r"^  out sem_id=(\S+)", blk, re.M)
                    if nm and out: ids.add(nm.group(1) + "=" + out.group(1))
            owners = set(o["owner"] for x in v for o in x["rows"]["cur"])
            idt[s] = ",".join(sorted(ids)) + " exec " + ",".join(sorted(owners))
        wj = grouped(collections.OrderedDict((s, ",".join("%d/%d/%d/%d" % t for t in sorted(set(wh[(s, gname.split("_")[0])])))) for s in sets if (s, gname.split("_")[0]) in wh))
        f.write("\t".join([gname, " ".join("%s:%d" % (s, len(v)) for s, v in sets.items()), per_set(lambda x: x["cls"].replace("class: ", "")),
                per_set(lambda x: x["console"]), per_set(lambda x: x["loss"]), per_set(lambda x: x["events"]), grouped(idt),
                per_set(lambda x: signature(x["rows"]["cur"])), per_set(lambda x: signature(x["rows"]["nocode"])),
                per_set(lambda x: " | ".join(x["ctl"]) + (" edges %s" % x["rc"][2] if x["ctl"] else "")), wj or "-"]) + "\n")
# occ_summary.tsv: kz occ diff results of w4w5_c1_occ.sh per group and reading
if os.path.exists(O + "/occ/occ.tsv"):
    agg = collections.OrderedDict()
    for line in open(O + "/occ/occ.tsv").read().splitlines()[1:]:
        f = line.split("\t"); k = (f[0], f[2]); a = agg.setdefault(k, [0] * 10 + [f[15]])
        a[0] += 1; a[1] += f[3] != "0"; a[2] += int(f[6] or 0); a[3] += int(f[12] or 0); a[4] += int(f[13] or 0)
        a[5] += int(f[7] or 0); a[6] += int(f[8] or 0); a[7] += int(f[9] or 0); a[8] += int(f[10] or 0); a[9] += int(f[11] or 0)
    with open(T + "/occ_summary.tsv", "w") as f:
        f.write("group\treading\tpairs\texit3\tmatched_occ\tverdict_diff\tbasis_diff\tonly_a\tonly_b\tambiguous\twithout_owner\tby_text\tmatching\n")
        for k in sorted(agg): f.write("\t".join(list(k) + [str(x) for x in agg[k]]) + "\n")
print("records", len(recs))
PY
sha256sum "$O"/tables/*.tsv "$O/replay/runs.log" "$0" > "$O/tables/sha256.txt"
