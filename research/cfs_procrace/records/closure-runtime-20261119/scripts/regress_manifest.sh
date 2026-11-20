#!/usr/bin/env bash
# regress_manifest.sh OUT PLUGIN -- versions 1-3 manifests bound by PLUGIN (the old kzgcc 0.3 27a49a6a, then the new
# kzgcc 0.4), into the new directory OUT (results 23.0).  Groups, each compiled at -O2 into OUT/<group>/{obj,binds}:
#   v1-target  kzgcc_target.kzs (kzs-sites 1) on instrument/gcc/tests/kzgcc_target.c
#   v2-shapes  kzgcc_shapes.kzs (kzs-sites 2) on kzgcc_shapes.c
#   v3-owner   kzgcc_owner.kzs (kzs-sites 3) on kzgcc_owner.c (compiled in the tests directory, as the plugin test does)
#   v3-c1plan  the first observation path's C1 plan (obs1/plangen/c1_pre_tx/plan.kzs, kzs-sites 3, the derived build's
#              manifest) on the twelve units of the bind check, with their kept compile commands (c1_build/pre)
# Compared by compare_manifest.py: every object the same bytes, every binding file the same apart from its line 2 (the
# plugin's version text).
set -uo pipefail
O=${1:?usage: regress_manifest.sh OUT PLUGIN}; PL=${2:?}
K=/home/user/Shape-Perf/research/cfs_procrace/kairos; T=$K/instrument/gcc/tests; W=/home/user/work/redesign/obs1
[ -e "$O" ] && { echo "refusing: $O exists" >&2; exit 2; }
mkdir -p "$O"
one() { # group manifest source [cwd]
  mkdir -p "$O/$1/obj" "$O/$1/binds"
  ( cd "${4:-$T}" && gcc -O2 -c "$3" -o "$O/$1/obj/u.o" -fplugin="$PL" -fplugin-arg-kzgcc-manifest="$2" -fplugin-arg-kzgcc-out="$O/$1/binds" ) > "$O/$1/compile.log" 2>&1
  echo "$1 rc=$?"
}
{
  echo "plugin $(sha256sum < "$PL" | cut -c1-64)"
  one v1-target "$T/kzgcc_target.kzs" "$T/kzgcc_target.c"
  one v2-shapes "$T/kzgcc_shapes.kzs" "$T/kzgcc_shapes.c"
  one v3-owner "$T/kzgcc_owner.kzs" kzgcc_owner.c "$T"
  mkdir -p "$O/v3-c1plan"
  python3 -I - "$PL" "$O/v3-c1plan" <<'PY'
import json, os, shlex, subprocess, sys
pl, out = sys.argv[1], sys.argv[2]
W = '/home/user/work/redesign/obs1'
units = ['apps/sch_lab/fsw/src/sch_lab_app.c', 'cfe/modules/time/fsw/src/cfe_time_task.c', 'osal/src/os/shared/src/osapi-countsem.c',
         'osal/src/os/shared/src/osapi-file.c', 'osal/src/os/shared/src/osapi-timebase.c', 'psp/fsw/modules/soft_timebase/cfe_psp_soft_timebase.c',
         'src/bp/fsw/src/bp_flow.c', 'src/cf/fsw/src/cf_app.c', 'src/cf/fsw/src/cf_cfdp.c', 'src/cf/fsw/src/cf_cfdp_r.c',
         'src/cf/fsw/src/cf_cfdp_s.c', 'src/cf/fsw/src/cf_cmd.c']
db = json.load(open('/home/user/work/procrace/c1_build/pre/1df262a/build/native/default/compile_commands.json'))
mods = [l.rstrip('\n').split('\t') for l in open(W + '/plangen/c1_pre_tx/plan.mod') if not l.startswith('#') and l.strip()]
args = ['-fplugin=' + pl, '-fplugin-arg-kzgcc-manifest=' + W + '/plangen/c1_pre_tx/plan.kzs', '-fplugin-arg-kzgcc-out=' + out + '/binds']
args += ['-fplugin-arg-kzgcc-module=%s:%s' % (m[0], m[2]) for m in mods if m[0] != 'default']
args += ['-fplugin-arg-kzgcc-default-module=' + m[1] for m in mods if m[0] == 'default']
os.makedirs(out + '/obj'); os.makedirs(out + '/binds')
rc = 0
with open(out + '/compile.log', 'w') as log:
    for n, tu in enumerate(units):
        e = [x for x in db if x['file'].endswith('/' + tu)][0]
        argv = shlex.split(e['command']); argv[argv.index('-o') + 1] = '%s/obj/%02d.o' % (out, n)
        r = subprocess.run(argv + args, cwd=e['directory'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        log.write('%s rc %d\n%s' % (tu, r.returncode, r.stdout.decode('utf-8', 'replace'))); rc |= r.returncode
print('v3-c1plan rc=%d' % rc)
PY
} > "$O/rc.txt"
cat "$O/rc.txt"
