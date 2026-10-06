from edit_lib import *
n='05_dialect.md'; t=load(n)
t=rep_between(t,'**[확인된 사실]** 근거는 import 경로가 이미 작동한다는 것이다. C17의 정정 문구는','**[해석]** C 의미를 새 dialect로 다시 정의하면',
'**[확인된 사실]** 근거는 import 경로가 이미 작동한다는 것이다. cFE module 5개와 앱 디렉터리 17개, 모두 154개 파일이 진단 0개로 import되었다. 단 이 결과는 clang으로 설정한 build tree에서 `-Werror`를 뺀 조건의 것이다 (조건은 §7.5.2, 검증 C17 정정 문구). 공식 문서(S16)는 LLVM IR에서의 번역을 experimental subset으로 기술한다. 이 범위의 cFS 코드에서는 그 제한이 import를 막지 않았다 (§7.5). ')
t=rep_between(t,'**[확인된 사실]** C18 정정 문구의 앱별 상수 회수는 이렇다.','to_lab의 3개 중 1개는 helper',
'**[확인된 사실]** 앱별 상수 회수 수와 나머지 site의 출처(table, 명령 payload)는 §7.7.3의 표에 있다 (검증 C18 정정 문구).\n\n')
save(n,t); print('ok')
