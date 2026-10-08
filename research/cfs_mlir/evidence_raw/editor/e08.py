from edit_lib import *
import re
n='08_evaluation.md'; t=load(n)
t=rep(t,' backtick으로 쓴 local 경로는 `/tmp/claude-0/-home-user-Shape-Perf/e983f27d-f015-5a5d-b912-399245630e78/scratchpad/cfs_note/` 기준의 상대 경로다.','')
t=re.sub(r'`(probe|rw-mlir|sem-es-tbl|sem-sb|issues|rw-flight|rw-races|rw-pubsub)/', r'`$N/\1/', t)
# 8.2 intro and rows
t=rep(t,'**[원노트 구상]** 원노트 §38은 F1–F6을 정의했다. **[설계 제안]** 이 절은 재시작 창(restart window)과 앱 간 table handle 문제를 F7로 분리한다. 원노트 §24-D("cross-application stale handle")를 평가 범주로 옮긴 것이다.',
'**[원노트 구상]** 원노트 §38은 F1–F6을 정의했다. **[설계 제안]** 평가 범주는 §1.2의 F1–F8을 따른다. F7은 원노트 §14의 메시지 간 도착 순서다. F8은 재시작 창(restart window)과 앱 간 table handle 문제를 분리한 범주로, 원노트 §24-D("cross-application stale handle")를 평가 범주로 옮긴 것이다.')
t=rep(t,'| F7 재시작·lifecycle | 재시작한 앱의 재구독·재등록 → 다른 앱의 의존 사용 |',
'| F7 메시지 간 도착 순서 | 소비자에서 `rcv(m1)` → `rcv(m2)`, 또는 인과 순서의 보존 | **[확인된 사실]** 목적지별 전달 순서, 송신 반환 전 수신, cFE 버전에 따른 lock 위치(550e7f7d)는 §1.2 F7과 §3.1·§3.6에 있다. |\n| F8 재시작·lifecycle | 재시작한 앱의 재구독·재등록 → 다른 앱의 의존 사용 |')
t=t.replace('SYN-F7-0','SYN-F8-0')
t=rep(t,'| F6/F7 ','| F6/F8 ')
t=rep(t,'| F7 대조 사례.','| F8 대조 사례.')
t=rep(t,'| F7 현재 사례.','| F8 현재 사례.')
t=rep(t,'# F1..F7','# F1..F8')
t=rep(t,'| F3, F7 |','| F3, F8 |')
t=rep(t,'| F4(교차 pipe 순서 역전) |','| F7, F4(교차 pipe 순서 역전) |')
t=rep(t,'| 재시작 의미(pipe 삭제·route 유지·새 AppId) | F7 |','| 재시작 의미(pipe 삭제·route 유지·새 AppId) | F8 |')
t=rep(t,'F1–F7 검사','F1–F8 검사')
# ablation ids
i=t.find('#### 8.10.1'); j=t.find('#### 8.10.2')
blk=t[i:j]
blk=re.sub(r'^\| A(\d+) \|', r'| AB\1 |', blk, flags=re.M)
t=t[:i]+blk+t[j:]
# 8.4.3 condense
t=rep_between(t,'**[실측]** 이미 관측된 두 시작 시 현상은 이 분류의 첫 시험 대상이다.\n','### 8.5 층 (c)',
'''**[실측]** 이미 관측된 두 시작 시 현상은 이 분류의 첫 시험 대상이다. 하나는 8회 실행 모두에서 OPERATIONAL 이전에 나온 `MsgId 0x808`(`CFE_EVS_LONG_EVENT_MSG_MID`)의 no-subscriber event다. 다른 하나는 TO_LAB의 37개 table 구독이 낸 구독 보고(`0x80E`)가 SBN pipe에서 16건 이상 손실된 것이다. 로그 위치, event를 낸 주체, CORE_READY 전의 세 줄, 추가 구독자(DS, HS)는 §7.4.3에 있다. **[미확인]** SBN 손실의 기능적 영향은 확인하지 않았다. 둘 다 결함으로 label하지 않는다.

''')
# 8.8.2 scope paragraph
t=rep_between(t,'**[확인된 사실]** 154개 파일의 범위를 정확히 적어야 한다','#### 8.8.3',
'**[확인된 사실]** 154개 파일의 범위(cFE module 5개, 앱 디렉터리 17개)와 성공 조건(clang으로 구성한 별도 build tree, `-Werror` 제거)은 §7.5.2에 있다 (검증 C17 정정 문구).\n\n')
# 8.8.3 rows
t=rep_between(t,'| API 호출 인식 | 모델링한 cFE API 호출 위치 / 전체 호출 위치(SB, ES, TBL, EVS, MSG별) |','| dispatch 복원 |',
'| API 호출 인식 | 모델링한 cFE API 호출 위치 / 전체 호출 위치(SB, ES, TBL, EVS, MSG별) | **[실측]** 앱별 `-O0` 호출 위치 수는 §7.6의 표 |\n| MID 해석 | MID 인자 위치를 세 종류로 나눈 비율: code 상수로 해석, table·명령 데이터에서 해석, 미해석 | **[실측]** 앱별 상수 회수 수는 §7.7.3의 표(to_lab은 검증 C18에서 3/7로 정정) |\n')
save(n,t); print('ok')
