from edit_lib import *
import re, glob, os
files=sorted(os.path.basename(f) for f in glob.glob('work/*.md'))
T={f:load(f) for f in files}
# 4.6 components -> K
t=T['04_related.md']
i=t.find('### 4.6'); j=t.find('### 4.7')
blk=re.sub(r'^\| C(\d+) \|', r'| K\1 |', t[i:j], flags=re.M)
blk=rep(blk,'C2·C4·C6–C10을','K2·K4·K6–K10을')
t=t[:i]+blk+t[j:]
t=rep(t,'| F6 (C9) |','| F6 (K9) |')
T['04_related.md']=t
# 6.8 FP and AP
t=T['06_analysis.md']
i=t.find('### 6.8'); j=t.find('### 6.9')
blk=re.sub(r'^\| C(\d+) ', r'| FP\1 ', t[i:j], flags=re.M)
t=t[:i]+blk+t[j:]
t=re.sub(r'(?<![A-Za-z0-9_/\-§])P([1-6])(?![0-9])', r'AP\1', t)
T['06_analysis.md']=t
# internal ids with 검증 기록 prefix
special={'검증 기록 rw-mlir-08·C20':'§4.5.1, 검증 C20','검증 기록 rw-mlir-11':'§4.5.3','검증 기록 probe-11;':'§7.7.3;',
 '검증 기록 rw-mlir-06':'§4.5.1','검증 기록 rw-mlir-04':'§7.5.2','검증 기록 rw-mlir-15,':'§4.5.2,','검증 기록 factcheck-e3·C20':'§4.5.1, 검증 C20',
 '검증 기록 probe-14':'§7.9','(ES·TBL 의미 조사 esTbl-05,':'(§3.3 ES-3,','**[실측]** probe-01:':'**[실측]** `$N/probe/submodules.txt`:',
 'probe-13은':'§7.8의 실측은','(probe-01, §3.0)':'(`$N/probe/submodules.txt`, §3.0)'}
simple={'probe-01':'`$N/probe/submodules.txt`','probe-05':'§7.4.3','probe-08':'§7.5','probe-09':'§7.6','probe-10':'§7.7.1','probe-11':'§7.7.3',
 'probe-12':'§7.7.3','probe-13':'§7.8','probe-14':'§7.9','probe-16':'§7.7.3','rw-mlir-04':'§7.5.2','rw-mlir-06':'§4.5.1','rw-mlir-08':'§4.5.1',
 'rw-mlir-11':'§4.5.3','rw-mlir-14':'§4.5.2','rw-mlir-15':'§4.5.2','rw-pubsub-06':'§4.2.2','factcheck-e3':'§4.5.1','sem-sb-02':'§3.1 SB-4',
 'sem-sb-04':'§3.1 SB-2','sem-sb-06':'§3.2 OS-2','esTbl-05':'§3.3 ES-3','issues-F03':'§2.4.2','issues-F07':'§2.4.2'}
for f in files:
    t=T[f]
    for k,v in special.items(): t=t.replace(k,v)
    for k,v in sorted(simple.items(),key=lambda x:-len(x[0])):
        t=re.sub(r'(?<![A-Za-z0-9_/\-])'+re.escape(k)+r'(?![0-9A-Za-z_/])', v, t)
    t=t.replace('(rw-flight 검증 기록)','(`$N/rw-flight/access_log.txt`, `$N/rw-races/sources_manifest.tsv`)')
    # verification refs
    t=re.sub(r'검증 (?:기록|판정) (C\d\d)', r'검증 \1', t)
    t=re.sub(r'(?<![A-Za-z0-9_/\-–])(C\d\d) 검증', r'검증 \1', t)
    t=re.sub(r'(?<![A-Za-z0-9_/\-–])(C\d\d)(?![0-9A-Za-z_/\-–])', lambda m: m.group(0), t)  # placeholder
    def bare(m):
        s=m.string; a=m.start()
        if s[max(0,a-3):a]=='검증 ': return m.group(0)
        return '검증 '+m.group(1)
    t=re.sub(r'(?<![A-Za-z0-9_/\-–.])(C(?:0[1-9]|1[0-9]|2[0-2]))(?![0-9A-Za-z_/\-–])', bare, t)
    T[f]=t
T['10_claims.md']=T['10_claims.md'].replace('| C | 원래 문장의 요지 |','| ID | 원래 문장의 요지 |')
for f in files: save(f,T[f])
print('ok')
