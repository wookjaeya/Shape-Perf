import json, subprocess, sys, os
G2 = "/home/user/work/v3/g2/K/L0064"; T = "/home/user/work/v3_followup/e1/K_L0064_tagged"
ARMS = {"S8": (f"{G2}/S8_full/model.so", None), "S1": (f"{G2}/S1_full/model.so", None),
        "S8a": (f"{T}/S8_alpha_full/model.so", "alpha"), "S1b": (f"{T}/S1_bravo_full/model.so", "bravo"),
        "S8b": (f"{T}/S8_bravo_full/model.so", "bravo"), "S1a": (f"{T}/S1_alpha_full/model.so", "alpha")}
L = lambda *a: [["load", x] for x in a]; C = lambda *a: [["call", x] for x in a]
CASES = {
 "I8": L("S8")+C("S8","S8"), "I1": L("S1")+C("S1","S1"),
 "C81-1": L("S8","S1")+C("S1","S8","S1","S8"), "C81-8": L("S8","S1")+C("S8","S1"),
 "C18-8": L("S1","S8")+C("S8","S1"), "C18-1": L("S1","S8")+C("S1","S8"),
 "SEQ81": L("S8")+C("S8")+L("S1")+C("S1","S8"), "SEQ18": L("S1")+C("S1")+L("S8")+C("S8","S1"),
 "LOAD1-CALL1-LOAD8-CALL8only": L("S1")+C("S1")+L("S8")+C("S8"),
 "C8a1b-1": L("S8a","S1b")+C("S1b","S8a"), "C1b8a-8": L("S1b","S8a")+C("S8a","S1b"),
 "C8b1a-1": L("S8b","S1a")+C("S1a","S8b"), "C1a8b-8": L("S1a","S8b")+C("S8b","S1a"),
 "SEQ8a1b": L("S8a")+C("S8a")+L("S1b")+C("S1b","S8a"), "SEQ1b8a": L("S1b")+C("S1b")+L("S8a")+C("S8a","S1b"),
 "CAA-8a8b": L("S8a","S8b")+C("S8b","S8a"), "SAMETAG-8a1a": L("S8a","S1a")+C("S1a","S8a"),
 "SAMETAG-1a8a": L("S1a","S8a")+C("S8a","S1a"),
}
env = dict(os.environ, OMP_NUM_THREADS="1", SHAPEPERF_WORK="/home/user/work")
for cid, steps in CASES.items():
    used = sorted({a for _o, a in steps})
    spec = {"arms": {a: {"path": ARMS[a][0], "tag": ARMS[a][1]} for a in used}, "steps": steps,
            "input": f"{G2}/input.npy", "expected": f"{G2}/expected.npy"}
    p = subprocess.run([sys.executable, "got_probe.py", json.dumps(spec)], capture_output=True, text=True, env=env)
    if p.returncode:
        print(cid, "ERROR", p.stderr[-600:]); continue
    r = json.loads(p.stdout.strip().splitlines()[-1])
    print(f"{cid:28s}", " ".join(f"{c['req']}->w:{','.join(c['wrapper_in'])}/c:{','.join(c['compute_in'])}{'' if c['exact'] else '!OUT'}" for c in r["calls"]))
