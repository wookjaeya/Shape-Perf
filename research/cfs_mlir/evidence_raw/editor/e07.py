from edit_lib import *
import re
n='07_feasibility.md'; t=load(n)
t=rep(t,'경로 표기는 §3.0과 같다. `$N`은 `/tmp/claude-0/-home-user-Shape-Perf/e983f27d-f015-5a5d-b912-399245630e78/scratchpad/cfs_note`다. 주 probe의','주 probe의')
t=rep_between(t,'| 시험 | 명령 | 결과 |\n| --- | --- | --- |\n| alias | `-test-alias-analysis` |','**[확인된 사실]** `LocalAliasAnalysis`는 MLIR core',
'''시험별 명령, 결과 파일, 함의는 §4.5.3의 표에 있다. 요지는 다섯 가지다. 서로 다른 전역 두 개는 MayAlias, 한 struct의 다른 field 두 개는 MustAlias다. 모든 `llvm.call`이 모든 위치에 ModRef이고, `llvm.call`은 `MemoryEffectOpInterface`를 구현하지 않는다. import된 `static` 함수에는 `sym_visibility`가 없어 호출자를 모두 안다고 보지 않는다. `static const` 함수 포인터 table을 통한 호출은 `<Unknown-Callee-Node>` 간선 하나뿐이다(`-O0`, `-O2` 모두). `-test-last-modified`는 원노트 §44 예제에서 Guidance의 읽기를 `<unknown>`으로 돌려준다. 같은 함수 안에서도 같은 전역을 두 번째 `addressof`로 읽으면 `<unknown>`이다.

''')
# P0 (prototype) -> MVP in 7.13
i=t.find('### 7.13 첫 prototype에 대한 의미')
head,tail=t[:i],t[i:]
tail=re.sub(r'P0(?=[은과의을이 ])', 'MVP', tail)
tail=tail.replace('P0 ','MVP ')
t=head+tail
save(n,t); print('ok')
