import itertools, json, os, subprocess, sys
D = sys.argv[1]
PY = "/home/user/work/venv/bin/python"
ORIG = {"S8": "/home/user/work/v3/g2/K/L0064/S8_full/model.so", "S1": "/home/user/work/v3/g2/K/L0064/S1_full/model.so"}
rows = []
for bindnow in (False, True):
    env = {k: v for k, v in os.environ.items() if not k.startswith("LD_")}
    if bindnow:
        env["LD_BIND_NOW"] = "1"
    for arm in ("S8", "S1"):
        other = "S1" if arm == "S8" else "S8"
        for lvl in ("E", "C", "M"):
            P = os.path.join(D, "poison", f"{arm}x{lvl}", "model.so")
            Q = ORIG[other]
            for p_first in (True, False):
                order = (P, Q) if p_first else (Q, P)
                for call_p in (True, False):
                    which = order.index(P if call_p else Q)
                    r = subprocess.run([PY, os.path.join(D, "trap_run.py"), order[0], order[1], str(which)],
                                       env=env, capture_output=True, text=True)
                    trapped = r.returncode == -4
                    # predictions
                    pred_B = (call_p) if lvl == "E" else p_first          # entry own; ciface+compute from FIRST loaded
                    pred_own = call_p                                      # every call runs its own code
                    rows.append({"bind_now": bindnow, "poisoned": f"{arm}x{lvl}", "load_order": "P,Q" if p_first else "Q,P",
                                 "called": "P" if call_p else "Q", "rc": r.returncode, "stdout": r.stdout.strip(),
                                 "trapped": trapped, "pred_claimB": pred_B, "pred_own": pred_own,
                                 "stderr_tail": r.stderr.strip()[-200:]})
                    print(f"bindnow={int(bindnow)} poisoned={arm}x{lvl} order={'P,Q' if p_first else 'Q,P'} call={'P' if call_p else 'Q'}"
                          f" rc={r.returncode:3d} out={r.stdout.strip() or '-':8s} trapped={int(trapped)} predB={int(pred_B)} predOwn={int(pred_own)}"
                          f" {'OK' if trapped == pred_B else 'MISMATCH_vs_B'}")
json.dump(rows, open(os.path.join(D, "trap_matrix.json"), "w"), indent=1)
n = len(rows); agreeB = sum(r["trapped"] == r["pred_claimB"] for r in rows); agreeOwn = sum(r["trapped"] == r["pred_own"] for r in rows)
print(f"runs={n} agree_with_claimB={agreeB} agree_with_each_runs_own={agreeOwn}")
