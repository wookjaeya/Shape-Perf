from edit_lib import *
n='03_semantics.md'; t=load(n)
t=rep(t,'`$N`은 `/tmp/claude-0/-home-user-Shape-Perf/e983f27d-f015-5a5d-b912-399245630e78/scratchpad/cfs_note`를 뜻한다.\n\n','')
t=rep_between(t,'**[확인된 사실]** 다음 세 문장은 고쳐야 한다.','**[미확인]** 다음 항목은 이 절의 결론에 쓰지 않았다.',
'**[확인된 사실]** 수정본에서 고쳐야 할 세 문장(§4.2 L185의 0-pipe 경로, §7.1 L315의 update 차단, §7.1 L317의 `NEVER_LOADED` 포인터)은 근거 항목(SB-5·SB-6, TBL-4·TBL-5, TBL-1)과 함께 §10.4.2의 RV3–RV5에 모았다.\n\n')
t=rep(t,'### 3.10 revision note에 대한 정정과 남은 미확인 항목','### 3.10 수정본에 대한 정정과 남은 미확인 항목')
save(n,t); print('ok')
