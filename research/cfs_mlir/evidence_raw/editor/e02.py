from edit_lib import *
n='02_motivation.md'; t=load(n)
t=rep(t,'로컬 근거 경로는 `$N = /tmp/claude-0/-home-user-Shape-Perf/e983f27d-f015-5a5d-b912-399245630e78/scratchpad/cfs_note`로 줄여 쓴다.\n\n','')
t=rep_between(t,'**공개 CI의 분석기**\n','---\n\n### 2.6',
'''**공개 CI의 분석기**

- **[확인된 사실]** cFE·cFS 공개 CI는 cppcheck와 CodeQL(security-and-quality, security-extended, JPL/MISRA subset)을 쓴다. JPL subset은 rule 14 "checking-return-values"와 rule 15 "checking-parameter-values"를 뺀다. 소스에는 CodeSonar 억제 주석이 있다. 설정 파일 permalink와 commit message 집계(CodeSonar 4, CodeQL 13, cppcheck 11, "static analysis" 17, Coverity·Polyspace·Klocwork·IKOS·Frama-C 0)는 §4.1.1에 있다.
- **[해석]** cFS에 실제로 적용되어 온 분석기는 lint와 CWE/JPL/MISRA 패턴을 보는 범용 C 검사기다. 공개 CI에서는 반환값 검사 규칙이 꺼져 있다. #2663처럼 버려진 `CFE_EVS_SendEventWithAppID` 반환값이 공개 CI에서 지적되지 않은 것과 맞아떨어진다. 다만 그것이 원인이라는 증거는 아니다.

''')
# 2.6 table compress rows 1 and 4,5 (details elsewhere)
t=rep(t,'''| 4줄은 `CFE_EVS_FIRST_4_STOP` filter의 상한이다. 실제 발행 건수는 모른다. 앞의 세 건은 TO_LAB의 지연 구독과 무관하다. 정상 대조군이다. |''',
'''| 4줄은 `CFE_EVS_FIRST_4_STOP` filter의 상한이다. 실제 발행 건수는 모른다. 앞의 세 건은 TO_LAB의 지연 구독과 무관하다. 정상 대조군이다. 상세 분석은 §7.4.3 |''')
t=rep(t,'''| `$N/sem-es-tbl/runs/R1_rr_allcpu/console.log:105-121` | #1466이 지적한 API 위험이 실제 실행에서 그대로 나타난다. |''',
'''| `$N/sem-es-tbl/runs/R1_rr_allcpu/console.log:105-121` | #1466이 지적한 API 위험이 실제 실행에서 그대로 나타난다. 의미 규칙은 §3.3 ES-4 |''')
t=rep(t,'''공식 FAQ(`cfe_tbl.dox` L333-339)는 이전 동작을 설명한다. |''','''공식 FAQ(`cfe_tbl.dox` L333-339)는 이전 동작을 설명한다. 의미 규칙은 §3.4 TBL-7 |''')
t=rep(t,'''API 모델은 버전마다 구현으로 확인해야 한다. |''','''API 모델은 버전마다 구현으로 확인해야 한다. 의미 규칙은 §3.4 TBL-1 |''')
save(n,t); print('ok')
