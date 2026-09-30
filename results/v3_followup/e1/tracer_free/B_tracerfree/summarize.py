import json, sys, glob, os
for p in sys.argv[1:]:
    r = json.load(open(p))
    print(f"=== {os.path.basename(p)} pid={r['pid']} TracerPid={r['tracer_pid']} LD_env={r['ld_env']} calls={[(c['arm'],c['exact']) for c in r['calls']]}")
    for s in r["snapshots"]:
        print(f"  -- {s['label']}  bases={s['bases']}")
        for arm, a in s["arms"].items():
            e = a["entry_dlsym_handle"]; g = a["entry_dlsym_RTLD_DEFAULT"]
            print(f"     {arm}: entry dlsym(handle={a['handle']})={e['value']} -> {e['maps_arm']} rel {e['module_relative']} (st_value {e['elf_st_value']}); dlsym(RTLD_DEFAULT)->{g['maps_arm']}")
            for sym, sl in a["slots"].items():
                st = "UNRESOLVED(lazy stub)" if sl["lazy_unresolved"] else f"-> {sl['maps_arm']} (dladdr {sl['dladdr_arm']}:{sl['dladdr']['sname'] if sl['dladdr'] else None}) rel {sl['module_relative_in_target']}"
                print(f"        GOT[{sl['got_off']}] @{sl['got_addr']} {sym:32s} = {sl['value']}  {st}")
