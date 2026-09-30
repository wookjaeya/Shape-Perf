import json, os, subprocess, sys

D = "/tmp/claude-0/-home-user-Shape-Perf/e983f27d-f015-5a5d-b912-399245630e78/scratchpad/e1_verify"
PY = "/home/user/work/venv/bin/python"
T = "/home/user/work/v3_followup/e1/K_L0064_tagged"
A = {
    "S8": {"path": "/home/user/work/v3/g2/K/L0064/S8_full/model.so", "tag": None},
    "S1": {"path": "/home/user/work/v3/g2/K/L0064/S1_full/model.so", "tag": None},
    "S8a": {"path": f"{T}/S8_alpha_full/model.so", "tag": "alpha"},
    "S1b": {"path": f"{T}/S1_bravo_full/model.so", "tag": "bravo"},
    "S8b": {"path": f"{T}/S8_bravo_full/model.so", "tag": "bravo"},
    "S1a": {"path": f"{T}/S1_alpha_full/model.so", "tag": "alpha"},
}

def pair(x, y, first):
    other = y if first == x else x
    return [["load", x], ["load", y], ["call", first], ["call", other], ["call", first], ["call", other]]

CASES = {
    # (A) untagged, only one artifact in process
    "A-ISO-S1": [["load", "S1"], ["call", "S1"], ["call", "S1"]],
    "A-ISO-S8": [["load", "S8"], ["call", "S8"], ["call", "S8"]],
    # positive control for the method (claim B shape)
    "CTRL-U-81-1": pair("S8", "S1", "S1"),
    "CTRL-U-18-8": pair("S1", "S8", "S8"),
    # (C) distinct tags, both load orders x both first-call orders
    "C-8a1b-8a": pair("S8a", "S1b", "S8a"),
    "C-8a1b-1b": pair("S8a", "S1b", "S1b"),
    "C-1b8a-8a": pair("S1b", "S8a", "S8a"),
    "C-1b8a-1b": pair("S1b", "S8a", "S1b"),
    # (C) interleaved load/call
    "C-SEQ-8a-call-1b": [["load", "S8a"], ["call", "S8a"], ["load", "S1b"], ["call", "S1b"], ["call", "S8a"]],
    "C-SEQ-1b-call-8a": [["load", "S1b"], ["call", "S1b"], ["load", "S8a"], ["call", "S8a"], ["call", "S1b"]],
    # (C) distinct tags, identical policy (identical compute bytes) - address decides
    "C-8a8b-8b": pair("S8a", "S8b", "S8b"),
    # (C) same tag again
    "SAME-8a1a-8a": pair("S8a", "S1a", "S8a"),
    "SAME-8a1a-1a": pair("S8a", "S1a", "S1a"),
    "SAME-1a8a-8a": pair("S1a", "S8a", "S8a"),
    "SAME-1a8a-1a": pair("S1a", "S8a", "S1a"),
}

only = sys.argv[1:]
env = dict(os.environ, SHAPEPERF_WORK="/home/user/work")
env.pop("LD_BIND_NOW", None)
for cid, steps in CASES.items():
    if only and cid not in only:
        continue
    names = sorted({s[1] for s in steps})
    spec = {"case_id": cid, "artifacts": {n: A[n] for n in names}, "steps": steps}
    sp = f"{D}/cases/{cid}.spec.json"
    os.makedirs(f"{D}/cases", exist_ok=True)
    json.dump(spec, open(sp, "w"), indent=1)
    r = subprocess.run([PY, f"{D}/gotprobe.py", sp], capture_output=True, text=True, env=env)
    if r.returncode != 0:
        print(cid, "FAILED", r.stderr[-2000:])
        continue
    open(f"{D}/cases/{cid}.out.json", "w").write(r.stdout)
    out = json.loads(r.stdout)
    print(f"== {cid} pid={out['pid']} load={out['load_order']} calls={out['call_order']}")
    for c in out["calls"]:
        print(f"  call#{c['call_index']} req={c['requested']:4s} entry(dlsym)={c['dlsym_entry'].get('artifact')}:{c['dlsym_entry'].get('sym')}"
              f" | ciface_slot[{c['requested']}]->{c['ciface_slot_of_entry'].get('artifact')}:{c['ciface_slot_of_entry'].get('sym')}"
              f" | main_graph_slot[{c.get('wrapper_dso')}]->{c.get('compute_dso')}:{c.get('main_graph_slot_of_wrapper_dso',{}).get('sym')}"
              f" @{c.get('main_graph_slot_of_wrapper_dso',{}).get('addr')} bytes~{c.get('compute_bytes_match')} out_exact={c['output_exact']}")
