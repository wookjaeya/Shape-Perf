from edit_lib import *
n='04_related.md'; t=load(n)
t=rep(t,'**표기.** 근거 표기와 `$N`은 1절과 같다. 결함 범주','**표기.** 결함 범주')
# 4.5.3 import row
t=rep_between(t,'| import | `clang -O0 -g` → `mlir-translate --import-llvm` | 154개 파일을','| SB 전달 경로 |',
'| import | `clang -O0 -g` → `mlir-translate --import-llvm` | 154개 파일(cFE module 5개, 앱 디렉터리 17개)을 진단 0개로 import했다. clang으로 설정한 별도 build tree에서 `-Werror`를 빼고 컴파일한 결과다. 연결 module은 정의 함수 2389개, 외부 선언 184개, 간접 호출 73곳이다. 조건과 실패 파일은 §7.5.2 | import 경로는 막히지 않는다. 비용은 import가 아니라 의미 복원에 있다 |\n')
t=rep_between(t,'| MID 상수 | `-O1 -Xclang -disable-llvm-passes`','#### 4.5.4',
'| MID 상수 | `-O1 -Xclang -disable-llvm-passes` → `mlir-opt --inline --sroa --mem2reg --canonicalize --cse` | 코드에서 정의한 MID가 call site 상수가 된다. 나머지는 table·명령 payload에서 온다. macro 이름은 남지 않는다. 앱별 회수 수는 §7.7.3 | MID 이름 복원에는 AST·preprocessor 보조 경로나 값→이름 표가 필요하다 |\n\n')
# 4.9 trim
t=rep_between(t,'### 4.9 원노트·수정본·조사 결과에서 바로잡은 항목\n','ZZZ' if False else '| 조사 단계의 주장: "Swift/BAT와 cFS를 연결하는 출처는 없다" |',
'''### 4.9 원노트·수정본·조사 결과에서 바로잡은 항목

이 절과 관련된 보정은 한곳에 모았다. 원노트 §41·§53·§54·§62는 §10.4.1의 O24, O27, O29, O28이다. 수정본 §8.2(R03)와 S14(DataFlow tutorial)는 §10.4.2의 RV7, RV8이다. 조사 단계 주장의 검증 결과(Goblint ARINC, fprime-topo-analysis 검사 수, ROSInfer state 변수, Ogma monitor, IKOS 가정, `LocalAliasAnalysis`, import 범위, #73 수정 전 코드, Swift/BAT)는 §10.3의 검증 C08, C01, C02, C06, C07, C20, C17, C21, C22 행이다.
''', include_end=False)
import re
t=re.sub(r'\| 조사 단계의 주장: "Swift/BAT와 cFS를 연결하는 출처는 없다" \|[^\n]*\n?','',t)
save(n,t); print('ok')
