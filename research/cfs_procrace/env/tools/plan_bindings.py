# plan_bindings.py (python3 -I) -- the binding report of a derived build made from a generated plan: what the plugin
# did with every site of the plan, and whether each instrumented unit got the module id the plan's module table gives.
#
#   python3 -I plan_bindings.py --manifest PLAN.kzs --modules PLAN.mod --bindings MERGED.kzs --module-flags "FLAGS" -o OUT
#
#   --manifest      the generated manifest (kzs-sites 3): its "site N ..." and "module N name=X" lines
#   --modules       the plan's module table (kz-static plan --modules-out): "<id>\t<name>\t<prefix>" lines and "default\t<id>"
#   --bindings      the merged binding file of the build (merge_binds.sh): per unit a header "# kzgcc ... unit=<path>"
#                   and "# manifest=<path> sha256=<hex> module=<id>", then "bind <id> site=<n> ... status=<s> [reason=...]"
#   --module-flags  the plugin's module options as the derive script passed them (derive_module_flags: the table's
#                   prefixes through the path map): -fplugin-arg-kzgcc-module=<id>:<prefix> ... -fplugin-arg-kzgcc-default-module=<id>
# Checks (each a line in OUT; rc 1 if one fails, rc 2 on a usage or input error):
#   sites      every site of the manifest has at least one binding line: bound / inlined / duplicated, or removed /
#              unsupported with the plugin's reason; a site no unit reported is "not-seen" (its unit was not compiled
#              with the plugin or its tu= matched no unit) and fails
#   manifest   every unit's binding header names the manifest's sha256
#   modules    the manifest's module statements and the table name the same ids and names; the flags carry the table's
#              ids in its order with its default; every unit's module id is the one the plugin's rule gives for its path
#              (the first prefix that starts the path, else the default; kzgcc.cpp start_unit)
# The plan states what to record, never what to expect: this report states what the plugin bound, not whether the
# planned sites will fire.
import argparse, collections, hashlib, re, sys

ap = argparse.ArgumentParser()
ap.add_argument('--manifest', required=True)
ap.add_argument('--modules', required=True)
ap.add_argument('--bindings', required=True)
ap.add_argument('--module-flags', required=True)
ap.add_argument('-o', required=True)
a = ap.parse_args()


def sha(p):
    with open(p, 'rb') as f:
        return hashlib.sha256(f.read()).hexdigest()


try:
    man = open(a.manifest, encoding='utf-8').read().split('\n')
    tab = open(a.modules, encoding='utf-8').read().split('\n')
    bnd = open(a.bindings, encoding='utf-8').read().split('\n')
except OSError as e:
    sys.stderr.write('plan_bindings.py: %s\n' % e)
    sys.exit(2)
msha = sha(a.manifest)
sites = collections.OrderedDict()
mmods = {}
for line in man:
    m = re.match(r'site (\d+) (.*)', line)
    if m:
        kv = dict(re.findall(r'(\w+)=("[^"]*"|\S+)', m.group(2)))
        sites[int(m.group(1))] = kv
        continue
    m = re.match(r'module (\d+) name=(\S+)', line)
    if m:
        mmods[int(m.group(1))] = m.group(2)
tmods, tdef, torder = {}, None, []
for line in tab:
    if not line or line.startswith('#'):
        continue
    f = line.split('\t')
    if f[0] == 'default':
        tdef = int(f[1])
    else:
        tmods[int(f[0])] = f[1]
        torder.append(int(f[0]))
flags = a.module_flags.split()
fmods = []
fdef = None
for x in flags:
    m = re.match(r'-fplugin-arg-kzgcc-module=(\d+):(.*)$', x)
    if m:
        fmods.append((int(m.group(1)), m.group(2)))
        continue
    m = re.match(r'-fplugin-arg-kzgcc-default-module=(\d+)$', x)
    if m:
        fdef = int(m.group(1))

# bindings per unit
units = []  # (unit path, manifest sha, module)
binds = collections.defaultdict(list)  # site -> [(status, reason, unit, bind id)]
cur = None
for line in bnd:
    m = re.match(r'# kzgcc .* unit=(\S+) ', line)
    if m:
        cur = [m.group(1), None, None]
        units.append(cur)
        continue
    m = re.match(r'# manifest=\S+ sha256=([0-9a-f]{64}) module=(\d+)', line)
    if m and cur is not None:
        cur[1], cur[2] = m.group(1), int(m.group(2))
        continue
    m = re.match(r'bind (\d+) site=(\d+) .* status=([a-z]+)(.*)$', line)
    if m:
        r = re.search(r'reason="([^"]*)"', m.group(4))
        binds[int(m.group(2))].append((m.group(3), r.group(1) if r else '', cur[0] if cur else '?', int(m.group(1))))

out, bad = [], 0
# sites
cnt = collections.Counter()
for sid, kv in sites.items():
    bl = binds.get(sid, [])
    sts = sorted(set(b[0] for b in bl))
    if not bl:
        st, ok = 'not-seen', False
    elif any(s in ('bound', 'inlined', 'duplicated') for s in sts):
        st, ok = '+'.join(sts), True
    else:
        st, ok = '+'.join(sts), all(b[1] for b in bl)  # removed / unsupported: a reason is required
    cnt[st] += 1
    if not ok:
        bad += 1
    reasons = '; '.join(sorted(set(b[1] for b in bl if b[1])))
    out.append('site\t%d\t%s\t%s\t%s\t%s\t%s\t%s' % (sid, kv.get('label', '?'), kv.get('tu', '?'), kv.get('func', '?'), st,
                                                 'binds=' + ','.join(str(b[3]) for b in bl) if bl else 'binds=-',
                                                 ('reason=' + reasons) if reasons else ''))
extra = sorted(s for s in binds if s not in sites)
for s in extra:
    out.append('site\t%d\tnot in the manifest\t\t\tFAIL\t\t' % s)
    bad += 1
# manifest sha per unit
mbad = [u for u in units if u[1] != msha]
for u in units:
    out.append('unit\t%s\tmanifest_sha256=%s\tmodule=%s' % (u[0], 'plan' if u[1] == msha else (u[1] or 'none'), u[2]))
bad += len(mbad)
# modules
modok = []
modok.append(('manifest modules = table modules plus the default', set(mmods) == set(tmods) | ({tdef} if tdef else set())
              and all(mmods.get(i) == n for i, n in tmods.items())))
modok.append(('flags carry the table ids in its order', [i for i, _ in fmods] == torder))
modok.append(('flags default = table default', fdef is not None and fdef == tdef))
ubad = []
for u in units:
    want = fdef
    for i, p in fmods:
        if u[0].startswith(p):
            want = i
            break
    if u[2] != want:
        ubad.append((u[0], u[2], want))
modok.append(('every unit has the module of the plugin rule', not ubad))
for n, ok in modok:
    out.append('modules\t%s\t%s' % (n, 'yes' if ok else 'NO'))
    if not ok:
        bad += 1
for u, got, want in ubad:
    out.append('modules\tunit %s module %s, rule gives %s' % (u, got, want))
perunit = collections.Counter(u[2] for u in units)
summary = ('summary\tsites %d: %s; units with sites %d (modules %s); manifest sha256 of the plan in %d of %d unit headers; failures %d'
           % (len(sites), ' '.join('%s=%d' % kv for kv in sorted(cnt.items())), len(units),
              ' '.join('%s:%s=%d' % (i, mmods.get(i, '?'), n) for i, n in sorted(perunit.items())), len(units) - len(mbad), len(units), bad))
with open(a.o, 'w', encoding='utf-8') as fh:
    fh.write('# plan_bindings.py: manifest %s sha256 %s; module table %s sha256 %s; bindings %s sha256 %s\n'
             % (a.manifest, msha, a.modules, sha(a.modules), a.bindings, sha(a.bindings)))
    fh.write('# module flags: %s\n' % a.module_flags)
    fh.write('\n'.join(out) + '\n' + summary + '\n')
print(summary)
sys.exit(1 if bad else 0)
