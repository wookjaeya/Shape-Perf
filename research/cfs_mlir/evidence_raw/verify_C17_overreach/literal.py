# Literal reproduction of the claim's stated procedure on the GCC compile DB (probe build):
# keep all original flags (incl. -Werror, -Wno-stringop-*), add -S -emit-llvm -O0 -g -Wno-unknown-warning-option.
import json, os, re, shlex, subprocess, concurrent.futures as cf
W=os.path.dirname(os.path.abspath(__file__))
ROOT=os.path.abspath(os.path.join(W,'..','probe','cFS'))
DB=os.path.join(ROOT,'build-native_std/native/default_cpu1/compile_commands.json')
B='/home/user/work/llvm-project/build/bin'
OUT=os.path.join(W,'lit'); os.makedirs(OUT,exist_ok=True)
sel=re.compile(r'/(apps/[^/]+/fsw/src/|cfe/modules/(sb|es|tbl|evs|time)/fsw/src/)')
db=[e for e in json.load(open(DB)) if sel.search(e['file']) and '/unit-test' not in e['file'] and '/ut-' not in e['file']]
def run(ie):
    i,e=ie
    args=shlex.split(e['command']) if 'command' in e else e['arguments']
    a=[];skip=False
    for x in args[1:]:
        if skip: skip=False; continue
        if x=='-o': skip=True; continue
        if x=='-c': continue
        a.append(x)
    tag=str(i)+'_'+os.path.relpath(e['file'],ROOT).replace('/','__')
    ll=os.path.join(OUT,tag+'.ll'); ml=os.path.join(OUT,tag+'.mlir')
    r=subprocess.run([B+'/clang',*a,'-S','-emit-llvm','-O0','-g','-Wno-unknown-warning-option','-o',ll],cwd=e['directory'],capture_output=True,text=True)
    row=dict(file=os.path.relpath(e['file'],ROOT),clang_rc=r.returncode,clang_err=[l for l in r.stderr.splitlines() if 'error:' in l][:3])
    if r.returncode==0:
        r2=subprocess.run([B+'/mlir-translate','--import-llvm','--mlir-print-debuginfo',ll,'-o',ml],capture_output=True,text=True)
        row['import_rc']=r2.returncode; row['import_diag_lines']=len([l for l in r2.stderr.splitlines() if 'warning' in l or 'error' in l])
        os.remove(ll); os.remove(ml)
    return row
with cf.ThreadPoolExecutor(4) as ex: rows=list(ex.map(run,enumerate(db)))
json.dump(rows,open(os.path.join(W,'literal_results.json'),'w'),indent=1)
print('selected',len(rows),'clang ok',sum(r['clang_rc']==0 for r in rows),'import ok',sum(r.get('import_rc')==0 for r in rows),'import diag lines',sum(r.get('import_diag_lines',0) for r in rows))
for r in rows:
    if r['clang_rc']!=0: print('FAIL',r['file'],r['clang_err'])
