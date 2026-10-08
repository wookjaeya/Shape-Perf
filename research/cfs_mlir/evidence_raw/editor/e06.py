from edit_lib import *
n='06_analysis.md'; t=load(n)
t=rep(t,'- op 이름은 §5의 `cfs` dialect 초안을 점 표기로 쓴다(예: `cfs.sb.subscribe`). §5에 없는 op는 처음 나올 때 "추가 필요"로 표시한다.\n',
        '- op 이름은 §5의 `cfs` dialect 초안을 점 표기로 쓴다(예: `cfs.sb.subscribe`). 이 절이 쓰는 op는 모두 §5.5에 정의되어 있다.\n')
t=rep(t,'- `$N = /tmp/claude-0/-home-user-Shape-Perf/e983f27d-f015-5a5d-b912-399245630e78/scratchpad/cfs_note`.\n','')
for s in [' (추가 필요)','(추가 필요)']:
    t=t.replace(s,'')
t=rep(t,'       // §5에 추가 필요','')
# S1 row evidence compress
t=rep_between(t,'| LLVM dialect 모듈 | **[실측]** 이 경로에서 코드에서 정의한 MID가','| S2 보조 입력 |',
'| LLVM dialect 모듈 | **[실측]** 이 경로에서 코드에서 정의한 MID가 call site의 상수 operand가 되었다. 앱별 회수 수와 to_lab 정정은 §7.7.3에 있다. gcc compile DB를 그대로 쓰면 gcc 전용 `-Wno-stringop-*`와 `-Werror` 때문에 실패하고, `-Wno-unknown-warning-option`만 더해도 2개 파일이 clang 경고로 실패한다 (§7.5.2). 그래서 `-Wno-error`가 필요하다. |\n')
# 6.3 memory model bullets
t=rep_between(t,'**메모리 모델을 직접 만드는 이유.** upstream MLIR의 기본 alias 분석은 이 목적에 맞지 않는다.\n','**[설계 제안]** 그래서 field를',
'''**메모리 모델을 직접 만드는 이유.** upstream MLIR의 기본 alias 분석은 이 목적에 맞지 않는다. **[실측]** `LocalAliasAnalysis`는 서로 다른 전역을 MayAlias로, 한 struct의 서로 다른 field를 MustAlias로 판정했다. `llvm.call`은 readnone `memory_effects` 속성이 있어도 모든 위치에 대해 ModRef였다 (§4.5.3; `$N/verify-C20/counter/modref_memattr.out`). **[확인된 사실]** field가 구분되지 않는 것은 GEP가 `ViewLikeOpInterface`라서 base로 풀리기 때문이다 ([LocalAliasAnalysis.cpp L100–L106 @ccac700c](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/lib/Analysis/AliasAnalysis/LocalAliasAnalysis.cpp#L100-L106)). **[해석]** `llvm.call`이 이 경로에서 구체적인 메모리 효과를 내놓지 않기 때문으로 본다.

''')
# 6.4.1 ODS block -> reference
t=rep_between(t,'```tablegen\n// [설계 제안] 효과 선언 스케치.','**[확인된 사실]** IRDL만으로는 효과를 선언할 수 없다.',
'''**[설계 제안]** resource 목록(`CFS_SBRoutes`, `CFS_SBQueues`, `CFS_ESState`, `CFS_TBLRegistry`, `CFS_TBLBuffers` 등)과 op별 effect·operand·결과는 §5.4–§5.5의 정의를 그대로 쓴다. 이 절은 별도의 ODS 초안을 두지 않는다. **[확인된 사실]** 이 interface는 ODS에서는 `MemoryEffectsOpInterface`, C++에서는 `MemoryEffectOpInterface`라는 이름이다 (로컬 `SideEffectInterfaces.td` L26-L28).

''')
save(n,t); print('ok')
