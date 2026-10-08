from edit_lib import *
import re
n='09_roadmap.md'; t=load(n)
t=re.sub(r'- \*\*\[편집 메모\]\*\*[^\n]*\n','',t)
t=rep(t,'ablation A1–A10·I1–I7.','ablation AB1–AB10·I1–I7.')
t=rep(t,'- 결함 범주 번호는 §1.2를 따른다. F7은 메시지 간 순서, F8은 재시작 창이다.',
        '- 결함 범주 번호는 §1.2를 따른다. F7은 메시지 간 순서, F8은 재시작 창이다. 첫 prototype은 "단계 1(MVP)"로 부르며 §5.8의 pass P0(build·import)과 구별한다.\n- GO-3의 측정 항목은 G3-a–G3-d로 부른다.')
t=rep(t,'B0·B1·B2·B4, A1–A10, I1–I5','B0·B1·B2·B4, AB1–AB10, I1–I5')
t=rep(t,'도메인 A1–A10을 flag로 끈다.','도메인 AB1–AB10을 flag로 끈다.')
t=rep(t,'ablation A1–A5, I1–I3만','ablation AB1–AB5, I1–I3만')
t=t.replace('SYN-F7-*(=F8)','SYN-F8-*').replace('SYN-F7-01·02(=F8)','SYN-F8-01·02').replace('SYN-F7-01/02','SYN-F8-01/02').replace('SYN-F7-02','SYN-F8-02')
assert 'SYN-F7' not in t
t=rep(t,'(§6 P4)','(§6 AP4)')
t=rep(t,'# §6 P3','# §6 AP3')
t=re.sub(r'C3-([a-d])', r'G3-\1', t)
# 9.3.6 SB table
t=rep_between(t,'**[설계 제안] SB 모델 적합성.** W8에 §8.3.4의 SB 다섯 줄도 확인한다.','| 판정 | 조건 | 다음 행동 |',
'''**[설계 제안] SB 모델 적합성.** W8에 §8.3.4의 SB 다섯 줄(MsgLim 초과, pipe 가득 참, 구독 해제 뒤 송신, 수신 순서, 발행 중 선점)도 확인한다. 실측 근거는 §8.3.4의 표에 있다. 분석기 모델의 예측은 차례로 다음과 같아야 한다. 해당 pipe에서만 drop하고 반환은 `CFE_SUCCESS`다. depth 초과분은 drop하고 반환은 성공이다. 구독 해제 뒤 송신은 counter·event 없이 성공하고 sequence가 증가한다. HB-FIFO는 같은 pipe·같은 sender에만 적용한다. `tx.e → hdl` 간선은 없다. 예측이 하나라도 틀리면 그 의미를 쓰는 판정은 L0으로만 보고한다.

''')
# 9.6.2 TBL table
t=rep_between(t,'| 적합성 시험 | 모델이 낼 결론 | 실측 근거 |\n| --- | --- | --- |\n| `NEVER_LOADED` | 포인터 NULL, lock 없음 |','| 역사 사례 | 공개 자료 | 계획한 사용 **[설계 제안]** |',
'''**[설계 제안] TBL 적합성 시험.** 다섯 시험은 §8.3.4 표의 `NEVER_LOADED`, handle 단위 lock, 단일 buffer, 이중 buffer, owner 재시작이다. 모델이 낼 결론과 실측 근거(로그 줄, 상태 코드, 재시작 시간 1.57–1.63 s)는 그 표에 있다 (검증 C13–C15).

''')
save(n,t); print('ok')
