import os, shutil, sys, hashlib
D = sys.argv[1]
src = {"S8": ("/home/user/work/v3/g2/K/L0064/S8_full/model.so", {"E": 0x2ff0, "C": 0x2f90, "M": 0x23b0}),
       "S1": ("/home/user/work/v3/g2/K/L0064/S1_full/model.so", {"E": 0x3a70, "C": 0x3a10, "M": 0x23b0})}
for arm, (path, offs) in src.items():
    for lvl, off in offs.items():
        d = os.path.join(D, "poison", f"{arm}x{lvl}")
        os.makedirs(d, exist_ok=True)
        dst = os.path.join(d, "model.so")
        shutil.copyfile(path, dst)
        with open(dst, "r+b") as f:
            f.seek(off)             # text segment: file offset == vaddr (LOAD 0x2000 -> 0x2000)
            f.write(b"\x0f\x0b")    # ud2 at function start
        print(arm, lvl, hex(off), dst, hashlib.sha256(open(dst, "rb").read()).hexdigest()[:16])
