from edit_lib import *
n='01_problem.md'; t=load(n)
# header notation/fixed point
t=rep_between(t,'**표기.** 근거 표기는','### 1.1 연구 문제',
 '표기, `$N`, 고정점은 문서 머리의 메타데이터와 「근거 표기」를 따른다.\n\n---\n\n')
# F1 TO_LAB + 0x808
t=rep_between(t,'- **정상 대조.** **[확인된 사실]** TO_LAB은 table 기반','- **공개 기록.** **[확인된 사실]** 조사한 issue 범위에서',
'- **정상 대조.** **[확인된 사실]** TO_LAB은 table 기반 telemetry 구독을 `CFE_ES_WaitForStartupSync` 뒤로 미룬다. 명령 MID 구독은 그 전에 `TO_LAB_init`에서 한다. 이 지연은 commit d3d52da(2026-04-21)에서 들어왔고 tag 중에서는 v7.0.1에만 있다. **[실측]** 기본 bundle 8회 실행 모두에서 OPERATIONAL 이전에 `No subscribers for MsgId 0x808`(`CFE_EVS_LONG_EVENT_MSG_MID`)이 정확히 4번 기록되었다. 4는 filter 상한이므로 실제 유실 수는 알 수 없다. 4건 중 앞의 3건은 CORE_READY 이전이라 TO_LAB의 지연과 무관하다. 소스 주석, 발신자, filter 상한의 상세는 §7.4.3에 있다.\n- **[해석]** 이 사례는 의도된 첫 메시지 유실이다. F1 검사가 이를 결함으로 경고하면 안 된다.\n')
# F4 normal controls
t=rep_between(t,'- **정상 대조 1.** **[확인된 사실]** HK는','- **[확인된 사실]** ROS 2는 이 문제를 라이브러리로',
'- **정상 대조 1.** **[확인된 사실]** HK는 입력마다 `DataPresent` flag를 두고, 결합 packet에 누락이 있으면 `MissingDataCtr`를 올린다. 기본값 `HK_DISCARD_INCOMPLETE_COMBO`=0이면 불완전한 결합 packet도 그대로 보낸다. 누락 event는 DEBUG type이라 기본 EVS type mask 0xE에서 꺼져 있다 (상세 §3.7). **[해석]** 이는 설정된 정책이다. F4 경고 대상이 아니다.\n- **정상 대조 2.** **[확인된 사실]** Ogma 예제 `cfs-002-state-machines`에서 `state`는 active, `input`은 passive 입력이다. 생성 템플릿은 메시지 값을 전역에 복사하고, active 입력일 때만 `copilot_step()`을 부른다 (R09·S24 관련; 템플릿 상세와 검증 이견은 §4.1.1). **[해석, 이견 있음]** passive 입력은 다음 active 입력이 올 때까지 마지막 값으로 남는다. latest-value 조합을 설계로 허용한 사례다. "Ogma 생성 앱은 일반적으로 여러 stream의 latest value를 결합한다"로 넓히지 않는다. README의 예제 호출은 `position` 하나만 감시한다 (검증 C06, contested).\n')
# F5 #73 bullets -> condensed
t=rep_between(t,'- **[확인된 사실] cFE #73(보정).**','- **[확인된 사실] 앱 사이의 사례.**',
'- **[확인된 사실] cFE #73 [S01].** Microblaze에서 우선순위 60인 TIME이 먼저 실행되어 생긴 역사적 core startup 결함이다. 범위를 벗어난 index는 SB의 AppId가 아니라 EVS 자신의 sentinel `EVS_AppID = 0xFFFFFFFF`였다. 수정 전 소스(cFE 6.4.1, 2014-12-12)와 수정 후 소스(6.4.2, 2015-07-13)가 SourceForge에 공개되어 있다. 실패 경로, 6.4.2의 수정 내용, SB AppId 초기값 불일치가 고쳐지지 않은 점은 §2.2에 정리했다. **[미확인]** 수정본 §2.2의 Trac 댓글 날짜(2015-05-06, 2015-06-16)는 댓글을 읽지 못해 다시 확인하지 못했다.\n- **[확인된 사실]·[실측] 현재의 잔여 사례.** open issue #2663(2025-08-06)은 ES·EVS 초기화 중 SB API가 아직 0인 SB AppId로 EVS를 불러 `CFE_EVS_APP_ILLEGAL_APP_ID`를 받고, 그 반환을 버린다고 보고한다. 검증 중 gdb로 이 반환을 실행당 24번 관측했다 (§2.2.4). **[해석]** #73과 같은 구조의 사례가 현재 코드에서 crash 없이 조용히 실패하는 형태로 남아 있다.\n')
t=rep(t,'- **[확인된 사실] core 앱은 직렬화되어 있다.** `CFE_ES_CreateObjects`는 core task를 만들기 전에','- **[확인된 사실] core 앱은 직렬화되어 있다.** `CFE_ES_CreateObjects`는 core-app task를 만들기 전에')
t=rep(t,'**[해석]** cFE #73의 crash 경로는 이 commit에서 닫혀 있다. #73은 현재 코드의 양성 사례가 될 수 없다.','상세는 §3.3 ES-2. **[해석]** cFE #73의 crash 경로는 이 commit에서 닫혀 있다. #73은 현재 코드의 양성 사례가 될 수 없다.')
# F6
t=rep_between(t,'#### F6 — Table 수명 프로토콜 위반\n\n','- **[해석]** 공개 기록의 다수는 단일 앱 API 오용이다.',
'#### F6 — Table 수명 프로토콜 위반\n\n- **[확인된 사실]** 잠금은 access descriptor(handle) 단위다. release 없이 `CFE_TBL_GetAddress`를 다시 부르면 보호 대상이 새 활성 buffer로 옮겨 간다 (§3.4 TBL-2). **[실측]** double-buffered table에서 sharer가 해제하지 않은 첫 포인터는 owner의 다음 Load 뒤 값 1 대신 3을 읽었다. 네 가지 설정 모두 같았다 (`$N/sem-es-tbl/runs/probe_results_summary.txt` L122-137).\n- **[확인된 사실]** 한 번도 load되지 않은 table에서 `CFE_TBL_GetAddress`는 `CFE_TBL_ERR_NEVER_LOADED`(0xcc000005)와 `*TblPtr = NULL`을 반환하고 lock을 걸지 않는다. header(S10)는 "0 내용의 유효한 포인터를 반환하며 해제해야 한다"고 쓴다. 이 불일치는 v7.0.0에도 있다. v6.7.0의 구현은 내부 buffer를 쓰는 table에 한해 header와 일치했다 (§3.4 TBL-1, §3.6).\n- **[확인된 사실]** buffering 방식에 따라 갱신 동작이 다르다. single-buffered table은 load가 `CFE_TBL_INFO_TABLE_LOCKED`로 보류되었다가 release 후 Manage나 Update에서 적용된다. double-buffered table은 load가 진행되고 보유자는 옛 내용을 계속 본다. 그다음 load는 `CFE_TBL_ERR_NO_BUFFER_AVAIL`로 거절될 수 있다 (§3.4 TBL-4, TBL-5).\n')
# F7
t=rep_between(t,'#### F7 — 메시지 간 도착 순서\n\n','#### F8 — 재시작 창',
'#### F7 — 메시지 간 도착 순서\n\n- **[확인된 사실]** `CFE_SB_TransmitMsg`는 SB mutex 아래에서 목적지 목록을 head(마지막 구독자)부터 고르고, mutex를 푼 뒤 목적지마다 `OS_QueuePut`을 부른다. 우선순위가 높은 구독 task는 전송 호출이 반환되기 전에 실행될 수 있다 (§3.1 SB-3·SB-4; S09, S12).\n- **[확인된 사실]** mutex 밖에서 put하는 구조는 cFE commit 550e7f7d(2024-02-26, tag 중 v7.0.0에 처음 포함)에서 생겼다. 그 이전 버전은 SB mutex를 쥔 채 `OS_QueuePut`을 불렀다 (§3.6). **[해석]** F7 판정은 cFE 버전을 매개변수로 가져야 한다.\n- **[확인된 사실]** Linux POSIX OSAL에서는 pipe마다 FIFO이고, queue가 가득 차면 메시지를 버린다. pipe 사이의 전역 순서는 없다 (§3.2 OS-1). **[미확인]** VxWorks queue의 순서 의미는 확인하지 않았다.\n- **[실측]** 1 CPU·SCHED_RR에서는 송신자보다 우선순위가 높은 수신자 3개가 전송 호출 반환 전에 모두 실행되었다(200/200, 2회). 관찰자 pipe가 먼저, relay pipe가 나중에 구독한 설정에서 relay가 송신자보다 높으면 관찰자가 relay의 출력을 원 메시지보다 먼저 꺼냈다(500/500, 2회). relay가 낮으면 0/500, 4 CPU에서는 두 설정 모두 0/500, 우선순위가 무시된 실행에서는 15/500·30/500이었다 (§3.1 SB-3·SB-4; `$N/sem-sb/probe_runs/SBPROBE_lines_all_runs.txt`).\n- **[해석]** 이 숫자는 하나의 Linux 환경에서 관측한 것이다. 발생률이 아니다. 4 CPU의 0/500은 불가능을 뜻하지 않는다.\n\n')
# F8
t=rep_between(t,'#### F8 — 재시작 창\n\n','- **[해석]** 원노트 §24-D의',
'#### F8 — 재시작 창\n\n- **[확인된 사실]** `CFE_ES_RestartApp`은 요청만 기록한다. ES background scan이 나중에 `CFE_ES_CleanUpApp`과 `CFE_ES_AppCreate`를 실행하고 새 AppId를 준다. cleanup은 SB pipe를 모든 경로에서 빼고 queue의 메시지를 버리며, TBL descriptor를 해제하고 앱이 소유한 table을 무소유로 표시한다. SB 경로와 그 sequence counter는 남는다 (§3.3 ES-9·ES-10, §3.1 SB-12, §3.4 TBL-7).\n- **[해석]** 재시작 창 동안 다른 앱의 발행은 목적지 0개 경로로 조용히 사라진다. F1과 같은 기전이다. 다른 앱이 보관한 옛 AppId는 무효가 된다.\n- **[실측]·[확인된 사실]** 재시작 요청에서 새 instance 진입까지 1.57–1.63 s 걸렸다. owner의 table을 다른 앱이 share하고 있으면 새 instance의 `CFE_TBL_Register`는 sharer의 descriptor가 해제될 때까지 `CFE_TBL_ERR_DUPLICATE_NOT_OWNED`(0xcc00000d)를 받았다. cleanup 이전 약 1.5 s 동안 sharer는 옛 포인터를 그대로 받았다 (`$N/sem-es-tbl/runs/probe_results_summary.txt` L144-157, 네 가지 설정 모두). 이 동작은 v7.0.0 \'cFS Draco\' commit c1ab1b7 이후의 것이고, 같은 commit의 TBL FAQ(`docs/src/cfe_tbl.dox` L333-339)는 여전히 옛 동작을 설명한다 (§3.4 TBL-7, §3.6; `$N/verify_C15_counter/tbl_name_clearing_by_version.txt`).\n')

# RQ1 evidence
t=rep_between(t,'- **현재 근거.**\n  - **[실측]** `-O1 -Xclang -disable-llvm-passes`로 IR을 만들고','#### RQ2',
'- **현재 근거.** **[실측]** 코드에서 정의한 MID는 `-O1 -Xclang -disable-llvm-passes`와 MLIR SSA 정리 뒤 call site의 상수 operand가 된다. 상수 회수/전체 site는 sample_app 3/3, hk 4/7, sch_lab 1/2, ci_lab 4/4, to_lab 3/7이다 (§7.7.3; to_lab은 검증에서 3/7로 정정). `apps/*/fsw/src`의 `CFE_SB_Subscribe*` 호출 47곳 중 8곳은 table·명령·설정 구조체에서 MID를 받는다 (§3.7). **[확인된 사실]** IR에는 MID macro 이름이 남지 않는다. Clang AST에는 literal의 spelling 위치가 남는다 (§7.7.3).\n- **신규성 주의.** **[확인된 사실]** 소스에서 pub/sub topology를 복원하는 작업은 선행 사례가 있다. ROSDiscover(ICSA 2022)는 ROS C++에서 port를 복원하고 구조 규칙만 검사한다 (§4.2.1). SPLC 2009(Ganesan 등)는 CFS 구현을 개발자 가이드의 아키텍처 규칙에 대조했다 (초록·snippet 수준, §4.1.1). **[해석]** RQ1의 결과는 기반 작업이며 그 자체로 기여가 아니다.\n\n')
# RQ2 evidence
t=rep_between(t,'- **현재 근거.**\n  - **[실측]** debug info를 켠 import에서 전역 field store 83곳을','#### RQ3',
'- **현재 근거.** **[실측]** debug info를 켠 import에서 전역 field store 83곳을 찾았다. 모두 file:line을 가졌다. 세 패턴(offset 0 field, API out-parameter 쓰기, 지역 포인터를 통한 쓰기)은 놓쳤고 sch_lab에서는 0곳이었다 (§7.8). upstream MLIR의 `LocalAliasAnalysis`는 서로 다른 전역을 MayAlias로, 한 struct의 서로 다른 field를 MustAlias로 판정한다. 외부 `llvm.call`은 모든 위치에 대해 ModRef다. 테스트 분석 `-test-last-modified`는 §44 예제에서 `<unknown>`을 반환한다 (§4.5.3). **[해석]** field·전역을 구분하는 메모리 모델은 연구자가 만들어야 한다.\n\n')
# RQ5 evidence
t=rep_between(t,'- **현재 근거.**\n  - **[실측]** Clang Static Analyzer의 taint는','#### RQ6',
'- **현재 근거.** **[실측]** Clang Static Analyzer의 taint는 int payload에서만 전파되었다. double payload와, 다음 `ReceiveBuffer` 같은 불투명 호출 뒤에서는 `static` 전역에서도 사라졌다 (§4.5.2). **[확인된 사실]** CodeQL의 C/C++ global data flow는 전역 변수를 통해 함수 사이를 순서와 무관하게 잇는다 (§4.5.2). **[해석]** 순서 판정은 별도로 붙여야 한다.\n\n')
# RQ6 evidence
t=rep_between(t,'- **현재 근거.**\n  - **[실측]** `C → LLVM IR → mlir-translate --import-llvm` 경로로','#### 가설의 재진술',
'- **현재 근거.** **[실측]** `C → LLVM IR → mlir-translate --import-llvm` 경로로 154개 C 파일을 진단 0개로 import했다. 범위는 cFE의 5개 module과 17개 앱 디렉터리이고, 성공한 compile flag에는 조건이 있다 (§7.5.2). 파일별 import에서 `llvm.call`과 `llvm.load`는 모두 file:line:col을 가졌다 (§7.9). 로컬 clang에서는 ClangIR을 쓸 수 없고 `-fclangir -S -emit-llvm`은 조용히 무시된다 (§7.10). **[해석]** import 경로의 비용은 import가 아니라 의미 복원에 있다.\n\n')
# 1.7 rows
t=rep_between(t,'| "프레임워크 의미를 복원해 순서 결함을 찾는 방법이 새롭다" |','| "cFS topology 복원이 기여다" |',
'| "프레임워크 의미를 복원해 순서 결함을 찾는 방법이 새롭다" | **[확인된 사실]** ROSInfer(ICSE 2024)는 ROS C++에서 입력 trigger(구독 callback, 주기, component 시작)가 state 변수를 바꾸고 그 변수가 발행 조건을 정하는 상태기계를 추론한다. 생성한 PlusCal/TLA+ 모델로 이미 알려진 결함 3개를 다시 찾았다. 메시지 내용은 모델링하지 않는다 (§4.2.1). fprime-topo-analysis는 F′ topology와 C++ handler 흐름을 결합해 lock·data race·queue 우선순위를 분석한다 (§4.1.2; peer review 미확인). Goblint v1.1.0의 `arinc.ml`은 ARINC 653 process·동기화 동작을 graphviz·Promela 모델로 내보냈지만 sampling·queuing port 호출은 `Nop`으로 처리했다 (§4.1.2; 검증 C08 정정 문구). **[미확인]** Android의 SIERRA·nAdroid는 검색 요약만 보았다 |\n')
t=rep_between(t,'| "Swift/BAT는 cFS 사례다" 또는 "Swift/BAT는 cFS와 무관하다" |','| "cFE #73을 검출했다" |',
'| "Swift/BAT는 cFS 사례다" 또는 "Swift/BAT는 cFS와 무관하다" | **[확인된 사실]** 검색 snippet에 따르면 BAT flight software는 RAD6000·VxWorks 위의 C++이고 Triana에서 온 \'Command and Data Handling Software Bus\'를 쓴다. 반면 GSFC의 2008년 발표 \'cFE/CFS\'(NTRS 20090005965)의 \'cFE Heritage\' slide는 \'Swift BAT (12/04)\'를 나열한다 (§2.1.5). **[해석]** BAT는 cFE를 실행한 것으로 확인되지 않은 cFE 계보의 선행 시스템이다. 검증 과정에서도 이 부분은 이견이 있었다 (검증 C22, contested). benchmark로 쓰지 않고, 동기 사례로만 쓴다 |\n')
# 1.8 -> pointer
t=rep_between(t,'### 1.8 이 절에서 바로잡은 원노트·수정본의 항목','ZZZ_END' if False else '| 원노트 Property 4: timestamp 비교가 있으면 consistency guard |',
'### 1.8 이 절에서 바로잡은 원노트·수정본의 항목\n\n이 절과 관련된 원노트·수정본의 보정(원노트 §4.1·§4.2·§21·§27·§41·§53, 수정본 §2.1·§2.2·§7.1)은 §10.4의 O1, O4, O17, O22, O24, O27, RV1, RV2, RV5에 모았다.\n', include_end=False)
# remove the last remaining row line
import re
t=re.sub(r'\| 원노트 Property 4: timestamp 비교가 있으면 consistency guard \|[^\n]*\n?','',t)
save(n,t); print('ok')
