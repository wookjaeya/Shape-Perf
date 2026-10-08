import json, subprocess, sys, os
G2 = "/home/user/work/v3/g2/K/L0064"
AA = os.path.abspath("AA_full/model.so")
ARMS = {"S8": f"{G2}/S8_full/model.so", "S1": f"{G2}/S1_full/model.so", "AA": AA}
L = lambda *a: [["load", x] for x in a]; C = lambda *a: [["call", x] for x in a]
CASES = {"SCAN": L("S8","S1","AA")+C("S8","S1","AA","S1","AA","S8"), "CAA": L("S8","AA")+C("AA","S8"),
         "CAA-rev": L("AA","S8")+C("S8","AA"), "IAA": L("AA")+C("AA")}
env = dict(os.environ, OMP_NUM_THREADS="1")
for cid, steps in CASES.items():
    used = sorted({a for _o, a in steps})
    spec = {"arms": {a: {"path": ARMS[a], "tag": None} for a in used}, "steps": steps,
            "input": f"{G2}/input.npy", "expected": f"{G2}/expected.npy"}
    p = subprocess.run([sys.executable, "got_probe.py", json.dumps(spec)], capture_output=True, text=True, env=env)
    r = json.loads(p.stdout.strip().splitlines()[-1]) if p.returncode == 0 else {"calls": [], "err": p.stderr[-400:]}
    print(f"{cid:10s}", " ".join(f"{c['req']}->w:{','.join(c['wrapper_in'])}/c:{','.join(c['compute_in'])}" for c in r["calls"]), r.get("err", ""))
