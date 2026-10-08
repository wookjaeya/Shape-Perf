## 10. 주장–근거 대응과 미해결 질문

이 절은 노트 전체의 주장을 한곳에서 점검한다. 하는 일은 세 가지다.

1. 노트의 중요한 주장마다 지위와 근거를 붙인다 (10.2).
2. 원노트(2026-09-30)와 수정본(2026-10-01)의 서술 가운데 고치거나 기각한 것을 모은다 (10.3, 10.4).
3. 남은 질문을 연구 결론에 주는 영향 순으로 정렬한다 (10.5).

이 절은 새 사실을 더하지 않는다. 근거는 앞 절들과 같은 증거 묶음이다. 각 행의 '상세' 열은 그 주장을 자세히 다룬 절을 가리킨다.

---

### 10.1 지위 표기와 교차 검증 규칙

| 지위 | 이 절에서의 뜻 | 결론에 쓸 수 있는가 |
| --- | --- | --- |
| **[확인된 사실]** | 고정 commit의 코드, 공식 문서, 논문 본문을 직접 읽었다. 확인 범위(전문·초록·검색 snippet)를 함께 적는다 | 쓴다. 단 snippet 수준이면 그 범위를 밝힌다 |
| **[실측]** | 이 컨테이너에서 명령을 실행해 얻었다. 명령과 결과 파일을 적는다 | 쓴다. 측정 조건 Σ(권한, CPU 수, `msg_max`, OSAL permissive 설정)를 함께 쓴다 |
| **[원노트 구상]** | 원노트·수정본이 제안한 아이디어다. 아직 검증하지 않았다 | 연구 방향으로만 쓴다 |
| **[설계 제안]** | 이 노트가 제안한 구체 설계다. 구현하지 않았다 | 실험 계획으로만 쓴다 |
| **[해석]** | 사실에서 끌어낸 판단이다 | 판단임을 밝혀 쓴다 |
| **[미확인]** | 근거를 열지 못했거나 실행하지 않았다 | 결론의 근거로 쓰지 않는다 |

**[확인된 사실] 교차 검증.** 핵심 주장 22개(C01–C22)는 검증자 세 명이 각자 1차 출처를 다시 열어 판정했다. 결과는 confirmed 17개, refuted 3개(검증 C08, 검증 C17, 검증 C21), contested 2개(검증 C06, 검증 C22)다. confirmed 판정도 대부분 표현의 정밀도를 고쳤다.

**검증 claim 목록.** 노트 본문의 '검증 Cnn'은 아래 claim을 가리킨다. refuted·contested와 결론에 영향을 준 보정은 10.3에 정리했다.

| ID | 대상 | 판정 |
| --- | --- | --- |
| 검증 C01 | fprime-topo-analysis의 입력·검사 범위 | confirmed |
| 검증 C02 | ROSDiscover(ICSA 2022)와 ROSInfer의 범위 | confirmed |
| 검증 C03 | Ganesan·Lindvall 그룹의 cFS 검증 선례(ISSRE 2016, SPLC 2009) | confirmed |
| 검증 C04 | TaxDC의 정의와 범위 | confirmed |
| 검증 C05 | Burrows–Leino의 stale-value 정의 | confirmed |
| 검증 C06 | Ogma cFS monitor template의 구독·평가 구조 | contested |
| 검증 C07 | IKOS의 검사 범위와 가정 | confirmed |
| 검증 C08 | Goblint v1.1.0의 ARINC 653·OSEK 분석 | refuted |
| 검증 C09 | SB 전달 실패가 발신자에게 보이지 않음 | confirmed |
| 검증 C10 | SB 전달 순서, sequence counter, lock 위치 | confirmed |
| 검증 C11 | ES startup의 timeout과 준비 비보장 | confirmed |
| 검증 C12 | core startup 직렬화와 #73 경로 | confirmed |
| 검증 C13 | `NEVER_LOADED`의 포인터와 lock | confirmed |
| 검증 C14 | TBL handle 단위 잠금과 buffering | confirmed |
| 검증 C15 | 재시작의 비동기 요청과 teardown | confirmed |
| 검증 C16 | data·설정·명령이 정하는 구독 관계 | confirmed |
| 검증 C17 | bundle build와 154개 파일 import 조건 | refuted |
| 검증 C18 | IR에서 MID 이름 소실과 상수성 | confirmed |
| 검증 C19 | 로컬 ClangIR 사용 불가 | confirmed |
| 검증 C20 | upstream MLIR의 alias·modref·visibility 동작 | confirmed |
| 검증 C21 | cFE #73 기록과 수정 이력 | refuted |
| 검증 C22 | Swift/BAT circular와 cFS의 관계 | contested |

**규칙.** refuted 주장은 원래 문장으로 쓰지 않는다. 검증자가 뒷받침한 보정 문장만 쓴다. contested 주장은 이견을 함께 쓴다. 아래 표의 '검증' 열은 해당 C 번호와 판정이다.

---

### 10.2 주장–근거 대응표

#### 10.2.1 동기 사례

| # | 주장 | 지위와 근거 | 검증 | 상세 |
| --- | --- | --- | --- | --- |
| CM1 | Swift 팀 GCN Circular들은 BAT의 on-board 오식별을 'known race condition'으로 설명한다. 대상은 22706(2018-05-11, 4U 1416-62), 32397(2022-07-14, Cen X-3), 34691(2023-09-14, SWIFT J1727.8-1613), 43470(2026-01-20, 1A 1118-61), 21077(2017-05-08, Cen X-3)이다 | **[확인된 사실]** 수정본이 S03–S06을 읽었다. 이번 조사에서는 gcn.nasa.gov가 차단되어 검색 snippet으로만 다시 확인했다 ([22706](https://gcn.nasa.gov/circulars/22706), [32397](https://gcn.nasa.gov/circulars/32397), [34691](https://gcn.nasa.gov/circulars/34691), [43470](https://gcn.nasa.gov/circulars/43470), [21077](https://gcn.nasa.gov/circulars/21077)). 검증자 snippet에는 2013년(GCN 14920)까지 거슬러 가는 사례도 있었다. **[해석]** 반복 보고는 있으나 발생률은 알 수 없다 | 검증 C22 (contested) | §2.1 |
| CM2 | 22706은 race가 "leads to incorrect spacecraft attitude information to be applied to a known source"라고 쓴다. 오래된 attitude나 다른 epoch라는 말은 없다 | **[확인된 사실]** 문구는 snippet 수준이다. '없다'는 판단도 snippet 본문 범위에 한정된다 | 검증 C22 (contested) | §2.1 |
| CM3 | 34691의 'the "7 minute problem"'은 attitude를 언급하지 않고 22706과의 관계도 말하지 않는다 | **[확인된 사실]** snippet 수준. **[미확인]** '7 minute problem'의 기술적 설명은 찾지 못했다 | 검증 C22 (contested) | §2.1 |
| CM4 | BAT의 원인은 `Observation(t1) + Attitude(t0)`, 즉 다른 epoch의 snapshot 혼합이다 | **[미확인]** 공개 근거가 없다. 원노트 §4.1의 도식이다. 결론에 쓰지 않는다 | – | §2.1 |
| CM5 | Swift/BAT과 cFS의 관계 | **[확인된 사실]** snippet에 따르면 BAT flight software는 RAD6000·VxWorks 위의 C++이고, Triana에서 온 'Command and Data Handling Software Bus'를 쓴다 (arXiv:astro-ph/0408494). cFE의 첫 비행은 LRO(2009)다(snippet). GSFC 발표 'cFE/CFS'(2008-11-13, NTRS 20090005965)의 'cFE Heritage' slide는 'Swift BAT (12/04)'를 나열한다 ([OCR mirror](https://raw.githubusercontent.com/cpu0/gutenberg/2338b2e24640a2d6553326ff5f05a6b6bc079348/NASA_NTRS_Archive_20090005965/NASA_NTRS_Archive_20090005965_djvu.txt) L58, L101-103; 로컬 `$N/verify-C22/ntrs_20090005965_djvu.txt`). **[해석]** BAT는 cFE를 실행한 근거가 없는 cFE 계보의 선행 시스템이다. OCR이 slide 배치를 잃어 heritage의 방향은 추론이다. 검증자 셋 중 하나는 이 부분을 기각했다 | 검증 C22 (contested) | §2.1 |
| CM6 | cFE #73의 결함 경로: 우선순위 60인 TIME이 먼저 실행된다. TIME의 `CFE_SB_CreatePipe`가 아직 0인 `CFE_SB.AppId`로 `CFE_EVS_SendEventWithAppID`를 부른다. `EVS_NotRegistered` → `EVS_SendEvent` → `EVS_IsFiltered`가 EVS 자신의 sentinel `EVS_AppId = 0xFFFFFFFF`를 범위 검사 없이 쓰고 segfault한다 | **[확인된 사실]** [#73](https://github.com/nasa/cFE/issues/73)(S01) 본문. 범위를 벗어난 값은 SB의 AppId가 아니라 EVS의 sentinel이다 | 검증 C21 (refuted, 다른 부분) | §2.2 |
| CM7 | #73의 GitHub 기록은 2019-09-30에 10초 안에 생성·종료된 이관 기록이다. milestone 6.4.2, 댓글 14개 | **[확인된 사실]** GitHub search API 메타데이터. **[미확인]** 이관된 Trac 댓글의 2015-05-06·2015-06-16 날짜는 댓글을 열지 못해 다시 확인하지 못했다 | 검증 C21 (refuted) | §2.2 |
| CM8 | #73의 수정은 cFE 6.4.2에서 처음 공개되었다. 수정 전 6.4.1 소스도 공개되어 있다 | **[확인된 사실]** [SourceForge coreflightexec](https://sourceforge.net/projects/coreflightexec/files/)에 cFE-6.4.1(2014-12-12)과 6.4.2(2015-07-13) tarball이 있다. 6.4.1에는 per-core-app 동기화와 `EVS_AppID` 범위 검사가 없다. 6.4.2에는 둘 다 있다. 6.4.2 Version Description Document(2015-06-29)는 Trac #42 'Race conditions / dependencies between CFE core apps'를 결함으로 나열한다 (`$N/verify_C21/vdd642.txt` L626-627). 범위 검사는 `EVS_IsFiltered`가 아니라 호출자 `EVS_SendEvent`에 들어갔다 ([cfe_evs_utils.c L612-L617 @b2765d9f](https://github.com/nasa/cFE/blob/b2765d9f905aa11f0c3b419b62233597366a7db9/cfe/fsw/cfe-core/src/evs/cfe_evs_utils.c#L612-L617)). `EVS_AppID = CFE_EVS_UNDEF_APPID`는 수정 전에도 있던 sentinel이므로 수정이 아니다 | 검증 C21 (refuted) | §2.2 |
| CM9 | 546a002에서 #73의 crash 경로는 닫혀 있다 | **[확인된 사실]** `CFE_ES_CreateObjects`는 모든 core module의 EarlyInit을 먼저 부르고, core task를 하나씩 만든 뒤 각 task가 RUNNING이 되기를 `CFE_PLATFORM_CORE_MAX_STARTUP_MSEC`(기본 30000) 동안 기다린다. 실패하면 `CFE_PSP_Panic`이다 ([cfe_es_start.c L767-L876](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_start.c#L767-L876), L861). **[실측]** 24회 실행 모두에서 core 초기화 event가 ES, EVS, SB, TBL, TIME 순서였다. RR 우선순위로 CFE_TIME main task가 가장 높은 rtprio(75)를 가진 설정도 포함한다 (`$N/sem-es-tbl/runs/{R*,rep_*}/console.log`; rtprio는 `probe_results_summary.txt` L13-18) | 검증 C12 (confirmed) | §2.2, §3.3 |
| CM10 | #73과 같은 모양의 잔여 문제가 546a002에 있다. ES·EVS TaskInit 중의 SB 호출이 아직 0인 SB AppId로 EVS를 부르고, `CFE_EVS_APP_ILLEGAL_APP_ID`가 무시된다 | **[확인된 사실]** 열린 issue [#2663](https://github.com/nasa/cFE/issues/2663)(2025-08-06). 정적 경로: [cfe_sb_api.c L272-L281](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_api.c#L272-L281), [cfe_evs.c L185-L188](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/evs/fsw/src/cfe_evs.c#L185-L188). **[실측]** 검증 중 gdb로 `cfe_evs.c:188` 반환을 24회 관측했다. 모두 AppID=0이었다 (`grep -c "^BP188" $N/verify-C12/counter_lens/gdb_attach.log` → 24). 증상은 crash가 아니라 event 유실이다 | 검증 C12 (confirmed) | §2.2 |
| CM11 | 공개 저장소에 수정 전후를 비교할 수 있는 순서·lifecycle 사례가 있다 | **[확인된 사실]** [cFE #198](https://github.com/nasa/cFE/issues/198)(6.5.0a 대 6.6.0a의 APPS_INIT·LATE_INIT 도입), [CF #184](https://github.com/nasa/CF/issues/184)(다른 앱이 만드는 semaphore에 대한 시작 race, 수정 [833fdbb](https://github.com/nasa/CF/commit/833fdbb27a26f15f2429e79392e7eb73d61abdea)), [sample_app #101](https://github.com/nasa/sample_app/issues/101)(`INFO_UPDATED` 경로의 주소 미해제), [cFE #950](https://github.com/nasa/cFE/issues/950)(exit·cleanup race, 수정 [6932f1f](https://github.com/nasa/cFE/commit/6932f1fe9180f4fd3f6756e4074b2c4b27a703cf)). **[미확인]** 어느 것도 build·재현하지 않았다 | – | §2.4, §8.5 |
| CM12 | table에서 오는 state를 load 전에 쓰는 공개 결함이 있다. 단 순서와 무관한 결정적 결함이다 | **[확인된 사실]** [HS #148](https://github.com/nasa/HS/issues/148)(수정 [b7530d9](https://github.com/nasa/HS/commit/b7530d94e322032d80cfd714b2a29c2b423473cb)), [MD #79](https://github.com/nasa/MD/issues/79)(open; [md_app.c L423-L449 @65eb7b3](https://github.com/nasa/MD/blob/65eb7b3b0aa8acd05076128a623cd696582b6d7c/fsw/src/md_app.c#L423-L449)) | – | §1.2 F2 |
| CM13 | '구독 준비 전 발행' 때문에 기능 오류가 난 공개 보고는 찾지 못했다 | **[확인된 사실]** 검색 범위 한정 결과다. 질의와 페이지는 `$N/issues/search_log.tsv`에 있다. 각 검색 페이지가 약 12행만 보여 recall이 불완전하다 | – | §1.2 F1 |
| CM14 | NASA IV&V의 cFS 보고서(NASA/CR-20205010026)는 Klocwork 정적 분석을 수행했다 | **[확인된 사실]** 검색 snippet 수준. **[미확인]** 순서·startup 관련 결과의 보고 여부. NTRS가 차단되어 본문(수정본 S07의 §2.1–2.2)을 다시 열지 못했다 (`$N/rw-flight/access_log.txt`) | – | §2.5 |
| CM15 | SBN(GSC-16917-1)은 cFE 인스턴스 사이의 peer-to-peer 다리다. 로컬 SB의 명세가 아니다 | **[확인된 사실]** [NASA Software Catalog 2025-26](https://ntts-prod.s3.amazonaws.com/t2p/prod/software/NASA_Software_Catalog_2025-26.pdf) 인쇄 p.207 (`$N/factcheck/catalog_2025-26.txt` L8968-8972) | – | §3.1 |

#### 10.2.2 Software Bus 의미

| # | 주장 | 지위와 근거 | 검증 | 상세 |
| --- | --- | --- | --- | --- |
| CS1 | 경로가 없는 MID로 발행하면 `NoSubscribersCounter`(uint8)가 오르고 `CFE_SB_SEND_NO_SUBS_EID`가 요청되며 `CFE_SUCCESS`가 반환된다 | **[확인된 사실]** [cfe_sb_priv.c L1091-L1097](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_priv.c#L1091-L1097) (S12). event가 실제로 나가려면 두 조건이 필요하다. 하나는 SB 앱 전체가 공유하는 `CFE_EVS_FIRST_4_STOP` filter가 남아 있는 것이다 ([cfe_sb_internal_cfg.h L221-L243](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/inc/cfe_sb_internal_cfg.h#L221-L243)). 다른 하나는 발행 앱이 EVS에 등록되어 있고 INFORMATION type이 켜져 있는 것이다 ([cfe_evs_utils.c L190-L240](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/evs/fsw/src/cfe_evs_utils.c#L190-L240), [cfe_evs_task.c L1463-L1485](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/evs/fsw/src/cfe_evs_task.c#L1463-L1485)) | 검증 C09 (confirmed) | §3.1 |
| CS2 | SBR은 경로를 지우지 않는다. 구독 해지·pipe 삭제로 목적지가 0개가 된 경로로 발행하면 counter도 event도 없이 `CFE_SUCCESS`다. IsOrigination=true면 sequence counter는 계속 오른다 | **[확인된 사실]** [cfe_sbr_route_unsorted.c L72-L114](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sbr/fsw/src/cfe_sbr_route_unsorted.c#L72-L114), [cfe_sb_priv.c L1055-L1064](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_priv.c#L1055-L1064). **[실측]** 한 번도 구독되지 않은 MID는 `NoSubDelta=1`, 구독 후 해지한 MID는 `NoSubDelta=0`이었고 둘 다 rc=0이었다. 목적지 0개로 3회 보낸 뒤 sequence는 1에서 5가 되었다 (5회 실행 동일, `$N/sem-sb/probe_runs/SBPROBE_lines_all_runs.txt`) | 검증 C09 (confirmed) | §3.1 |
| CS3 | MsgLim 초과나 pipe full은 그 pipe에서만 메시지를 버리고, `TransmitMsg`는 여전히 `CFE_SUCCESS`를 반환한다 | **[확인된 사실]** [cfe_sb_priv.c L1004-L1012, L1194-L1227](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_priv.c#L1004-L1012). cFE 기능 시험이 MsgLim 경우의 반환값을 단언한다 ([sb_sendrecv_test.c L103-L107](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/cfe_testcase/src/sb_sendrecv_test.c#L103-L107)). **[실측]** MsgLim=2로 5회 보내자 rc는 모두 0, 수신 2개, `MsgLimitErrorCounter` +3이었다. depth 3에 5회 보내자 수신 3개, `PipeOverflowErrorCounter` +2였다 (같은 파일). **[해석]** 발행 성공은 수신 준비나 처리 완료를 뜻하지 않는다. 인자·크기·buffer 할당 오류는 여전히 오류를 반환한다 | 검증 C09 (confirmed) | §3.1 |
| CS4 | SB에는 늦은 구독자를 위한 durability·latch·마지막 값 보존이 없다 | **[확인된 사실]** `CFE_SB_Qos_t`는 "Currently an unused parameter"다 ([default_cfe_sb_extern_typedefs.h L114-L125](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/config/default_cfe_sb_extern_typedefs.h#L114-L125)). route entry에는 목적지 목록, MsgId, SeqCnt만 있다. 이 판단은 SB·SBR 코드를 읽은 범위에 한정된다. SBN·원격 구독은 확인하지 않았다 | 검증 C09 (confirmed) | §3.1 |
| CS5 | `TransmitMsg`는 SB mutex 안에서 목적지 목록을 head부터 훑고(마지막 구독자가 먼저), mutex를 푼 뒤 목적지마다 `OS_QueuePut`을 부른다. 한 번의 발행은 원자적 broadcast가 아니다 | **[확인된 사실]** [cfe_sb_priv.c L1032-L1131](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_priv.c#L1032-L1131)(lock L1049, unlock L1114), [L1187-L1188](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_priv.c#L1187-L1188); [cfe_sb.h L238-L244, L417-L422](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb.h#L238-L244) (S09). **버전 의존:** lock 밖 put은 [550e7f7d](https://github.com/nasa/cFE/commit/550e7f7dd349c24e234ab0881f5c13e667f05741)(2024-02-26, 첫 release v7.0.0)부터다. 그 이전(v6.7.0, draco-rc5, parent [9e5d452a](https://github.com/nasa/cFE/blob/9e5d452a854753c9314362d8ddfae6f8a7f69a58/modules/sb/fsw/src/cfe_sb_api.c))은 put을 mutex 안에서 했다 | 검증 C10 (confirmed) | §3.1, §3.6 |
| CS6 | Linux OSAL에서 pipe는 FIFO이고 가득 차면 버린다. pipe 사이의 전역 순서는 없다 | **[확인된 사실]** `mq_timedsend`에 우선순위 1과 0 절대 timeout을 쓴다 ([os-impl-queues.c L285-L323](https://github.com/nasa/osal/blob/dad0ee99c121f2ecc11f8c0798c1e836e41d8ab1/src/os/posix/src/os-impl-queues.c#L285-L323)). Linux v6.12 mqueue는 같은 우선순위를 tail에 넣는다. **[미확인]** VxWorks queue의 순서 의미는 OSAL 코드만 읽었다 | – | §3.2 |
| CS7 | 구독자 사이의 전달 순서와 인과 순서는 구독 순서, 우선순위, CPU 수, OSAL 설정에 따라 바뀐다 | **[실측]** 1 CPU·SCHED_RR(root): 송신자보다 높은 우선순위의 수신자 3개가 `TransmitMsg` 반환 전에 모두 실행되었다(run1·run5 각 200/200, 구독 역순). relay pipe가 관찰자 pipe보다 뒤에 구독되어 list head에 있는 설정에서, 송신자보다 높은 relay는 관찰자 pipe의 인과 순서를 뒤집었다(각 500/500). 낮은 relay는 0/500이었다. 4 CPU에서는 두 설정 모두 0/500이었고 수신자 깨어남 순서가 6가지 순열을 모두 보였다. non-root(SCHED_OTHER, `msg_max` 10) 1회 실행에서는 relay 상위 설정 15/500, 하위 설정 30/500이었다 (`$N/sem-sb/probe_runs/run1_root_cpu0.log`, `run5_root_cpu0_repeat.log`, `run2_root_allcpu.log`, `run3_root_allcpu.log`, `run4_nonroot_cpu0_msgmax10.log`). **[해석]** 4 CPU의 0/500은 관측일 뿐 보장이 아니다. 다른 구독 순서는 시험하지 않았다 | 검증 C10 (confirmed) | §3.1, §7.4 |
| CS8 | 수신 buffer는 같은 pipe의 다음 수신 호출까지만 유효하다. 수신은 성공·`NO_MESSAGE`·`TIME_OUT`·오류로 갈린다 | **[확인된 사실]** [cfe_sb.h L440-L479](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_sb.h#L440-L479) (S09), [cfe_sb_api.c L1310-L1347](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_api.c#L1310-L1347) | – | §3.1 |
| CS9 | 기본 origination action은 telemetry header의 시각을 전송 시점으로 덮어쓴다 | **[확인된 사실]** [cfe_msg_integrity.c L30-L53](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/msg/fsw/src/cfe_msg_integrity.c#L30-L53). **[미확인]** mission별 MSG override | – | §1.2 F3 |
| CS10 | 실제 pipe 깊이는 실행 구성에 따라 요청값과 다를 수 있다 | **[실측]** non-root와 permissive 모드에서 OSAL은 깊이를 `msg_max`(이 컨테이너 10)로 줄였다. 깊이 20을 요청하자 10개만 받았다. host IPC의 root는 `mq_open` EINVAL로 시작에 실패했다 (`$N/sem-sb/probe_runs/run4_nonroot_cpu0_msgmax10.log`, `$N/probe/run_root.log`). **[확인된 사실]** [os-impl-queues.c L59-L69](https://github.com/nasa/osal/blob/dad0ee99c121f2ecc11f8c0798c1e836e41d8ab1/src/os/posix/src/os-impl-queues.c#L59-L69), [native_osconfig.cmake L22-L39](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/cmake/sample_defs/native_osconfig.cmake#L22-L39) | – | §7.4 |
| CS11 | 기본 bundle의 시작 구간에서 SB 전달 실패가 관측된다 | **[실측]** 8회 실행 모두 OPERATIONAL 이전에 `No subscribers for MsgId 0x808`(`CFE_EVS_LONG_EVENT_MSG_MID`)이 4줄, SBN pipe에서 `0x80e` 손실이 16줄이었다 (`grep -c "No subscribers for MsgId 0x808" $N/probe/run_nobody.log` → 4). 4와 16은 EVS filter 상한이므로 실제 수는 알 수 없다. 앞의 3줄은 CORE_READY 이전이다 | 검증 C16 (confirmed) | §7.4.3 |

#### 10.2.3 ES·TBL·재시작 의미

| # | 주장 | 지위와 근거 | 검증 | 상세 |
| --- | --- | --- | --- | --- |
| CE1 | startup script 앱에 대한 ES 동기화는 soft limit이다. OPERATIONAL은 모든 앱이 RUNNING임을 뜻하지 않는다 | **[확인된 사실]** ES는 LATE_INIT과 RUNNING을 각각 `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC`(기본 1000)만 기다리고, 실패하면 syslog만 쓴다 ([cfe_es_start.c L204-L229](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_start.c#L204-L229)). **[실측]** probe 앱 하나를 2.5 s 늦추자 'Startup Sync failed'가 두 번 기록되고 OPERATIONAL로 갔다. 다른 앱의 `WaitForSystemState(OPERATIONAL,5000)`은 1.95–2.01 s 뒤 `CFE_SUCCESS`였다. 4개 scheduling 설정과 반복 20회 모두 같았다 (`$N/sem-es-tbl/runs/probe_results_summary.txt` L66-80, L240-254, L415-427, L590-604). core 앱은 다르다(CM9) | 검증 C11 (confirmed) | §3.3 |
| CE2 | `CFE_ES_WaitForSystemState`는 기다리기 전에 호출자 자신의 AppState를 올린다. `CFE_ES_RunLoop`도 RUNNING으로 올린다 | **[확인된 사실]** [cfe_es_api.c L514-L619](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_api.c#L514-L619) (S11), [L469-L472](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_api.c#L469-L472). **[해석]** ES의 RUNNING은 '초기화 완료'가 아니라 '이 함수를 불렀음'이다 | 검증 C11 (confirmed) | §3.3 |
| CE3 | `CFE_ES_WaitForStartupSync`는 상태를 버리는 `void` wrapper다. header의 "Lower values will be rounded up"은 구현되어 있지 않다 | **[확인된 사실]** [cfe_es_api.c L616-L619](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_api.c#L616-L619), [cfe_es.h L440-L444](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_es.h#L440-L444) (S08). [#1466](https://github.com/nasa/cFE/issues/1466)은 열려 있다. [PR #2273](https://github.com/nasa/cFE/pull/2273)은 승인과 CCB:Ready 뒤 2026-07-07에 병합 없이 닫혔다 | 검증 C11 (confirmed) | §3.3 |
| CE4 | 우선순위 순 시작은 RT 권한과 1 CPU에서만 관측된다 | **[실측]** RR·cpu0에서 5/5가 우선순위 순(HI, OWN, SHR, SLOW, LO)이었다. RR·전체 CPU에서는 5/5가 script 순이었다. 권한 없는 설정에서는 대체로 script 순이었다 (`$N/sem-es-tbl/runs/order_repeats.txt`). **[확인된 사실]** 권한이 없으면 OSAL permissive 모드는 모든 task를 SCHED_OTHER로 조용히 돌린다 ([os-impl-tasks.c L227-L457 @5befd8e9](https://github.com/nasa/osal/blob/5befd8e9f6c62b44bb7ff4c15db8629c0e00efa0/src/os/posix/src/os-impl-tasks.c#L227-L457)). **[해석]** 설정당 5회는 확률 주장에 부족하다 | – | §3.3 |
| CE5 | 앱의 초기화 완료 순서는 실행마다 다르다 | **[실측]** 기본 bundle 짧은 실행 6회에서 6가지 순서가 나왔다. CI_LAB은 두 번째로 load되지만 완료는 5–10번째였다 (`$N/probe/runs/*.log`) | – | §7.4.2 |
| CT1 | 546a002에서 한 번도 load되지 않은 table에 `CFE_TBL_GetAddress`를 부르면 `CFE_TBL_ERR_NEVER_LOADED`(0xcc000005)와 NULL 포인터를 반환하고 잠그지 않는다. header의 '0 내용의 유효한 포인터' 설명과 다르다 | **[확인된 사실]** [cfe_tbl_registry.c L192-L196, L207](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/tbl/fsw/src/cfe_tbl_registry.c#L174-L220), [cfe_tbl.h L540-L544](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/fsw/inc/cfe_tbl.h#L536-L544) (S10, blob `f3510424`). v6.7.0은 header와 맞았다 ([cfe_tbl_internal.c L571, L616-L619 @2105b932](https://github.com/nasa/cFE/blob/2105b9324eca7dd58b4d6aea17628d10956dcfdb/fsw/cfe-core/src/tbl/cfe_tbl_internal.c#L568-L619)). v7.0.0과 main도 546a002와 같다. **[실측]** 4개 설정 모두 `GetAddress(T) BEFORE load st=0xcc000005 ptr=(nil)` (`$N/sem-es-tbl/runs/probe_results_summary.txt` L50-51, L220-221, L399-400, L575-576). 내부 buffer를 쓰는 dump-only table과 CDS에서 복원한 critical table은 load 없이도 활성 buffer가 있다 | 검증 C13 (confirmed) | §3.4 |
| CT2 | TBL 잠금은 포인터가 아니라 access descriptor(handle) 단위다. 해제 없이 `GetAddress`를 다시 부르면 보호가 새 buffer로 옮겨 간다 | **[확인된 사실]** [cfe_tbl_accdesc.h L54-L64](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/tbl/fsw/src/cfe_tbl_accdesc.h#L54-L64), [cfe_tbl_registry.c L204-L216](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/tbl/fsw/src/cfe_tbl_registry.c#L204-L216), [cfe_tbl_api.c L478-L509](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/tbl/fsw/src/cfe_tbl_api.c#L478-L509). **[실측]** double-buffered D에서 해제하지 않은 첫 포인터가 owner의 `Load(v3)` 뒤 1이 아니라 3을 읽었다 (4개 설정 R1–R4) | 검증 C14 (confirmed) | §3.4 |
| CT3 | table 갱신 동작은 buffering에 따라 다르다 | **[확인된 사실]** [cfe_tbl_internal.c L416-L576](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/tbl/fsw/src/cfe_tbl_internal.c#L416-L576). single-buffered: 잠긴 buffer가 있으면 `Load`가 `CFE_TBL_INFO_TABLE_LOCKED`(0x4c000018)를 반환하고 load는 대기 상태로 남는다. 해제 뒤 `Manage`나 `Update`가 적용한다. double-buffered: 갱신이 진행되고 holder는 옛 내용을 본다. holder의 descriptor가 아직 비활성 buffer를 가리키면 다음 load는 `CFE_TBL_ERR_NO_BUFFER_AVAIL`(0xcc00000f)로 거절되고 대기로 남지 않는다. **[실측]** 위 값들을 4개 설정에서 관측했다. single-buffered 경우는 owner 자신의 잠금으로만 관측했다. sharer의 잠금이 같은 결과를 내는지는 코드 추론이다 | 검증 C14 (confirmed) | §3.4 |
| CT4 | 앱 재시작은 비동기 요청이다. ES background scan이 나중에 `CleanUpApp`과 `AppCreate`를 실행하고 새 AppId를 준다. cleanup callback은 task 삭제보다 먼저 실행된다 | **[확인된 사실]** [cfe_es_appctrl.c L318-L499](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_appctrl.c#L318-L499), [cfe_es_apps.c L1047-L1119](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_apps.c#L1047-L1119). SB는 앱의 pipe를 모든 경로에서 빼고 queue를 비운 뒤 지운다 ([cfe_sb_priv.c L87-L124](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_priv.c#L87-L124)). 경로와 sequence counter는 남는다. **[실측]** `RestartApp` 호출에서 새 인스턴스 진입까지 1.57–1.63 s, AppId 1114121 → 1114123 (설정당 1회) | 검증 C15 (confirmed) | §3.3 |
| CT5 | owner 앱이 재시작될 때 다른 앱이 그 table을 공유하고 있으면, 새 owner의 `CFE_TBL_Register`는 남은 access descriptor가 모두 해제될 때까지 `CFE_TBL_ERR_DUPLICATE_NOT_OWNED`(0xcc00000d)를 받는다 | **[확인된 사실]** [cfe_tbl_accdesc.c L169-L193](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/tbl/fsw/src/cfe_tbl_accdesc.c#L169-L193), [cfe_tbl_registry.c L302-L365](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/tbl/fsw/src/cfe_tbl_registry.c#L302-L365). **[실측]** 4개 설정 모두 재현했다. sharer의 `GetAddress`는 cleanup 뒤 `CFE_TBL_ERR_UNREGISTERED`(0xcc000009)와 NULL을 받았다. cleanup 전 약 1.5 s 동안은 옛 포인터를 계속 받았다 (`$N/sem-es-tbl/runs/probe_results_summary.txt` L97-178). **버전 의존:** 이 동작은 v7.0.0의 [c1ab1b7](https://github.com/nasa/cFE/commit/c1ab1b77f9e0)부터다. 그 전 버전은 cleanup에서 table 이름을 지웠다(코드 읽기, 실행하지 않음). 공식 TBL FAQ([cfe_tbl.dox L333-L339](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/docs/src/cfe_tbl.dox#L333-L339))는 옛 동작을 설명한다 | 검증 C15 (confirmed) | §3.4, §3.6 |
| CT6 | owner의 `CFE_TBL_Load`는 registry lock을 일찍 풀므로 sharer의 `GetAddress`와 겹칠 수 있다 | **[미확인]** 코드 읽기 후보다 ([cfe_tbl_api.c L287-L348](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/tbl/fsw/src/cfe_tbl_api.c#L287-L348)). stress 시험을 하지 않았다 | – | §3.4 |
| CT7 | child task가 부른 ES·SB·TBL API는 부모 앱의 AppId로 동작한다 | **[확인된 사실]** [cfe_es_resource.c L308-L338](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_resource.c#L308-L338). **[미확인]** child task가 TBL 잠금과 부모 AppState에 주는 실행 영향 | – | §3.3 |

#### 10.2.4 통신 관계를 정하는 입력

| # | 주장 | 지위와 근거 | 검증 | 상세 |
| --- | --- | --- | --- | --- |
| CG1 | HK는 copy table에 있는 MID를 구독한다. table 갱신 때 구독을 해지하고 `CFE_TBL_Update` 뒤 다시 구독한다. 입력마다 `DataPresent` flag가 있고, 기본 설정은 불완전한 결합 packet도 보낸다 | **[확인된 사실]** [hk_utils.c L316-L324, L483-L504, L567-L594](https://github.com/nasa/HK/blob/0dc16b7a7bab71747b9c063cf405ad59a65a40a5/fsw/src/hk_utils.c#L316-L324), [hk_internal_cfg.h L57-L69](https://github.com/nasa/HK/blob/0dc16b7a7bab71747b9c063cf405ad59a65a40a5/fsw/inc/hk_internal_cfg.h#L57-L69). 누락 event는 DEBUG type이고 기본 EVS mask 0xE에서 꺼져 있다 ([cfe_evs_internal_cfg.h L176-L177](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/evs/fsw/inc/cfe_evs_internal_cfg.h#L176-L177)). 항상 보이는 신호는 `MissingDataCtr`다 | 검증 C16 (confirmed) | §3.7 |
| CG2 | TO_LAB은 table 기반 telemetry 구독을 `CFE_ES_WaitForStartupSync` 뒤로 미룬다. 명령 MID는 그 전에 구독한다. 지상 명령 AddPacket도 구독을 더한다 | **[확인된 사실]** [to_lab_app.c L69-L73, L205-L214, L500-L506](https://github.com/nasa/to_lab/blob/d27c6014cb5d2979140500f635876ee87b81995b/fsw/src/to_lab_app.c#L69-L73), [to_lab_cmds.c L216-L219](https://github.com/nasa/to_lab/blob/d27c6014cb5d2979140500f635876ee87b81995b/fsw/src/to_lab_cmds.c#L216-L219). 지연 구독은 [d3d52da](https://github.com/nasa/to_lab/commit/d3d52da34fa195cacfd085e92d1615ea1f036d15)(2026-04-21)에서 들어왔고 tag 중 v7.0.1에만 있다 | 검증 C16 (confirmed) | §3.7 |
| CG3 | SCH_LAB은 table의 MessageID로 주기 메시지를 보낸다 | **[확인된 사실]** [sch_lab_app.c L243-L248](https://github.com/nasa/sch_lab/blob/607e2f90c5f828f3f3ae655a4debacf8b2cc4bbb/fsw/src/sch_lab_app.c#L243-L248). sch_lab 저장소의 기본 table은 비어 있다. 실제 MID는 bundle의 [sample_defs/tables/sch_lab_table.c L55-L79](https://github.com/nasa/cFS/blob/5a9b075cd4c818ee8555f349a9f78ba5632d0384/sample_defs/tables/sch_lab_table.c#L55-L79)에서 온다. table은 init 때 한 번만 읽는다 | 검증 C16 (confirmed) | §3.7 |
| CG4 | 구독 site 중 일부는 MID를 table·명령·설정 구조체에서 받는다 | **[실측]** `apps/*/fsw/src`의 `CFE_SB_Subscribe*` 47곳 중 8곳이다(CF, DS 2곳, HK, LC, SBN, TO_LAB 2곳). bundle의 TO_LAB table은 37개 항목이다 (`$N/verify_C16_overreach/subscribe_sites_bundle.txt`). **[해석]** 실행 간선 수로는 table 구동이 더 많을 수 있으나 site 수로는 소수다 | 검증 C16 (confirmed) | §3.7, §1.6 RQ1 |
| CG5 | non-EDS build의 MID는 컴파일 시간 상수다. EDS build의 MID는 `CFE_PSP_GetProcessorId()`에 의존하는 실행 시간 호출 결과다 | **[확인된 사실]** [eds_cfe_core_api_msgid_mapping.h L37-L66](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/config/eds_cfe_core_api_msgid_mapping.h#L37-L66), [cfe_sb_eds_msg_id_util.c L231-L235](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_eds_msg_id_util.c#L231-L235). **[미확인]** EDS build는 컴파일하지 않았다 | 검증 C18 (confirmed) | §3.7, §7.7 |
| CG6 | cFE의 EDS는 앱별 명령·telemetry topic과 type을 선언한다. 소비자, 순서, 상태기계는 선언하지 않는다 | **[확인된 사실]** [cfe_sb.xml L970-L1033](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/eds/cfe_sb.xml#L970-L1033). cFE EDS XML 중 `StateMachine`을 쓰는 파일은 0개다 (`grep -rl StateMachine --include=*.xml` → 0) | – | §3.7 |
| CG7 | SB 전달은 코드만으로 닫히지 않는다. 경로는 bodiless OSAL 함수에서 끝난다 | **[확인된 사실]** `OS_QueuePut`([cfe_sb_priv.c L1188](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_priv.c#L1187-L1188)), `OS_QueueGet`([cfe_sb_api.c L428](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_api.c#L428)). **[실측]** 연결한 mission module에서 두 함수는 선언만 있다 (`$N/rw-mlir/import_exp/mission.mlir` L57018, L61947). **[해석]** 어떤 도구든 SB API 모델이 필요하다. 그래서 SB 모델 자체는 MLIR의 차별점이 아니다 | – | §4.5 |

#### 10.2.5 frontend 실측

| # | 주장 | 지위와 근거 | 검증 | 상세 |
| --- | --- | --- | --- | --- |
| CX1 | 고정 bundle은 README 절차로 build되고 compile DB를 낸다 | **[실측]** gcc 13.3.0, `make native_std.install` 48.8 s(wall), 컴파일 경고 0개. prep에는 SBN의 CMake deprecation 경고 6개가 있다. cpu1 compile DB는 1435개 항목이고 unit-test·coverage 중복을 포함한다 (`$N/probe/install.log`, `$N/probe/prep.log`, `$N/probe/compile_commands.cpu1.json`) | 검증 C17 (refuted) | §7.3 |
| CX2 | `C → LLVM IR → mlir-translate --import-llvm` 경로는 고정 cFS 코드에서 진단 없이 동작한다 | **[실측]** 154개 파일을 진단 0개로 import했다. 범위는 cFE module 5개(es, evs, sb, tbl, time; 56개)와 앱 디렉터리 17개(98개)다. msg·sbr·fs·resourceid·config와 OSAL·PSP는 빠졌다. 성공한 실행은 clang으로 configure한 별도 build tree에서 `-Werror`를 빼고 `-Wno-everything`, `-Xclang -disable-O0-optnone`을 더했다 (`$N/rw-mlir/import_exp/import_summary.txt`). README의 gcc DB에 `-Wno-unknown-warning-option`만 더하면 `cf_codec.c`와 `cs_table_processing.c`가 `-Werror`로 실패한다. `-Wno-error`를 더하면 154/154가 된다 (`$N/verify_C17_counter/per_file_results.json`). 연결한 module은 정의 함수 2389개, 외부 선언 184개(생략한 cFE module 44개 포함), 간접 호출 73곳이고 import에 약 1.2–1.7 s가 걸렸다 (`$N/rw-mlir/import_exp/link_import2.txt`). **[해석]** 공식 문서의 'experimental, substantially limited subset' 경고([TargetLLVMIR.md L947-L950](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/docs/TargetLLVMIR.md#L947-L950), S16)는 이 범위의 `-O0 -g` 코드에서 막히지 않았다. 다른 최적화 수준과 교차 build는 시험하지 않았다 | 검증 C17 (refuted, 원 문장) | §7.5 |
| CX3 | 코드에서 정한 MID는 SSA 정리 뒤 call site 상수가 된다. macro 이름은 남지 않는다 | **[실측]** `-O0`에서는 `CFE_SB_ValueToMsgId(i32 const)` → alloca store → load를 거친다 (`$N/probe/ir/sample_app/sample_app.ll` L143-149). `-O1 -Xclang -disable-llvm-passes`와 `mlir-opt --inline --sroa --mem2reg --canonicalize --cse` 뒤에는 상수 operand가 된다. site 단위 상수 회수/전체: sample_app 3/3, hk 4/7, sch_lab 1/2, ci_lab 4/4, to_lab 3/7 (`$N/probe/mlir_opt/*.json`; to_lab은 검증에서 2/7에서 정정). 나머지는 table·명령·실행 상태에서 온다. `-fdebug-macro`의 DIMacro도 import에서 사라진다 | 검증 C18 (confirmed) | §7.7 |
| CX4 | 앱 전역 struct field의 직접 store는 인식된다. 세 패턴은 놓친다 | **[실측]** 5개 앱에서 전역 field store 83곳을 찾았다. offset 0 field, API out-parameter 쓰기, 지역 포인터를 통한 쓰기는 놓쳤다. sch_lab은 0곳이었다 (`$N/probe/mlir/O0_*.json`). 탐지는 regex·함수 내부 수준이므로 recall이 아니다 | – | §7.8 |
| CX5 | 호출과 load는 소스 위치를 가진다 | **[실측]** `--mlir-print-debuginfo` import에서 `llvm.call` 8830개와 `llvm.load` 26378개가 모두 file:line:col을 가졌다 (`$N/rw-mlir/import_exp/loc_by_op.txt`). 상수와 `addressof`는 위치가 없다 | 검증 C17 (refuted) | §7.9 |
| CX6 | 로컬 toolchain으로는 ClangIR을 쓸 수 없다. `-fclangir`는 일부 모드에서 조용히 무시된다 | **[실측]** `-fclangir -emit-cir`는 "rebuild clang with -DCLANG_ENABLE_CIR=ON"으로 실패한다. `-fclangir -S -emit-llvm`은 rc=0이지만 출력이 일반 codegen과 byte 단위로 같다 (`$N/probe/cir/emit_cir.log`). **[확인된 사실]** upstream에서도 CIR은 기본 build에 없다 ([clang/docs/CIR/index.md L3-L7 @ccac700c](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/clang/docs/CIR/index.md#L3-L7), S17). incubator는 2026-02-20에 닫혔고 LifetimeCheck pass는 upstream에 없다 | 검증 C19 (confirmed) | §7.10 |

#### 10.2.6 upstream MLIR가 주는 것과 주지 않는 것

| # | 주장 | 지위와 근거 | 검증 | 상세 |
| --- | --- | --- | --- | --- |
| CL1 | MLIR은 fixpoint 반복과 분석 의존을 관리하는 generic dataflow solver를 준다. thread·interleaving 개념은 없다 | **[확인된 사실]** `/home/user/work/llvm-project/mlir/include/mlir/Analysis/DataFlowFramework.h` L9-12, L305-322(로컬 `1053047a`; upstream main에서도 같은 문장). DataFlow header에 thread·interleav·concurren 단어가 없다(`grep -i`). 공식 tutorial(S14)은 현재 header에 없는 `ForwardDataFlowAnalysis`·`LatticeElement`를 설명한다 ([DataFlowAnalysis.md L182-L245 @ccac700c](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/docs/Tutorials/DataFlowAnalysis.md#L182-L245)) | 검증 C20 (confirmed) | §4.5, §6 |
| CL2 | upstream alias 분석은 이 연구의 메모리 모델로 쓸 수 없다 | **[실측]** `LocalAliasAnalysis`는 서로 다른 전역을 MayAlias, 한 struct의 서로 다른 field를 MustAlias로 판정했다 (`$N/rw-mlir/mlir_probe/alias_llvm.out`). `llvm.call`은 readnone `memory_effects`가 있어도 모든 위치에 ModRef였다 (`$N/verify-C20/counter/modref_memattr.out`). **[확인된 사실]** GEP가 `ViewLikeOpInterface`라서 base로 풀린다 ([LocalAliasAnalysis.cpp L100-L106](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/lib/Analysis/AliasAnalysis/LocalAliasAnalysis.cpp#L100-L106)). `llvm.call`은 `MemoryEffectOpInterface`를 구현하지 않는다. `mlir/lib` 안의 alias 구현은 이것 하나다(시험용 subclass 제외). monorepo에는 FIR용 `fir::AliasAnalysis`와 CIR용 `CIRBasicAliasAnalysis`가 따로 있으나 LLVM dialect에는 적용되지 않는다 | 검증 C20 (confirmed) | §6.3 |
| CL3 | import한 C 함수는 `static`이라도 MLIR에서 public이라 호출자 미상으로 처리된다 | **[실측]** `-test-dead-code-analysis`가 `llvm.func internal`에 대해 '(all)' 없는 predecessors를 출력했다. clang으로 import한 `static` 함수에 `sym_visibility = "private"`를 손으로 붙이면 '(all)'이 된다 (`$N/rw-mlir/mlir_probe/dca_vis.out`, `$N/verify-C20/counter/s44_O0_dca.out`, `s44_O0_dca_priv.out`). **[확인된 사실]** [DeadCodeAnalysis.cpp L192](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/lib/Analysis/DataFlow/DeadCodeAnalysis.cpp#L192) | 검증 C20 (confirmed) | §6.5 |
| CL4 | 함수 포인터 table을 통한 호출은 `<Unknown-Callee-Node>`로만 이어진다 | **[실측]** `-O0`, `-O2` 모두 (`$N/rw-mlir/cir_test/t2.callgraph.out`). mission module에는 간접 호출이 73곳 있다 | 검증 C20 (confirmed) | §6.5 |
| CL5 | upstream 시험 분석은 원노트 §44 예제의 `global.att` 출처를 찾지 못한다 | **[실측]** `-test-last-modified`는 세 옵션 모두에서 `<unknown>`을 냈다 (`$N/rw-mlir/mlir_probe/lastmod_llvm.out`). 같은 SSA 주소일 때만 store를 찾았다. **[해석]** §44 예제의 `Guidance`는 `HandleAttitude`와 호출 경로가 없으므로, `<unknown>`의 일부는 handler 사이 모델이 없어서다. 같은 함수 안의 두 번째 `addressof`에서도 `<unknown>`인 것은 메모리 모델 문제다 | 검증 C20 (confirmed) | §6.3 |
| CL6 | IRDL로 toy `cfs` dialect를 정의하면 구조 검사는 되지만 effect는 없다 | **[실측]** 잘못된 operand 수를 verifier가 잡았다. IRDL op은 `MemoryEffectOpInterface`가 없어 `--test-side-effects`가 아무것도 보고하지 않았다 (`$N/rw-mlir/mlir_probe/cfs_irdl.out`) | – | §5 |
| CL7 | MLIR 분석 API는 자주 바뀐다 | **[실측]** PoTATo(`80d157c`, `20e7d8f`)가 로컬 LLVM `1053047a`에 대해 `RegionSuccessor`·DataFlow API 오류로 build되지 않았다 (`$N/rw-mlir/potato_build.log`) | – | §6.10 |
| CL8 | 같은 SB 모델을 넣은 대안 도구가 일부 기능을 이미 한다 | **[확인된 사실]** CodeQL C/C++ global data flow는 전역 변수를 통해 함수 사이를 순서와 무관하게 잇는다 ([DataFlowPrivate.qll L93-L120](https://github.com/github/codeql/blob/f1d3f1defc5ca73bcf1ed48031fb31c62fd91952/cpp/ql/lib/semmle/code/cpp/ir/dataflow/internal/DataFlowPrivate.qll#L93-L120)). **[실측]** Clang Static Analyzer의 taint는 int payload에서만 전파되었고, double payload와 불투명 호출(다음 `ReceiveBuffer` 등) 뒤에서는 `static` 전역이라도 사라졌다 (`$N/rw-mlir/csa_exp/csa_all.log`). **[미확인]** CodeQL·PhASAR·SVF·Joern·Frama-C·IKOS·Goblint는 실행하지 않았다 | – | §4.5 |

#### 10.2.7 선행연구와 신규성

| # | 주장 | 지위와 근거 | 검증 | 상세 |
| --- | --- | --- | --- | --- |
| CP1 | '필요 순서 명세를 얻고 그 위반을 정적·동적으로 찾는다'는 골격은 TaxDC §8.4가 제안했다 | **[확인된 사실]** [TaxDC 저자 PDF](https://raw.githubusercontent.com/ucare-uchicago/ucare-html/87a6a05aa681a3f4d8993e0b84f786c9b994babc/pdf/asplos16-TaxDC.pdf) 전문(sha256 `9d81b28f…`). 104개 결함(Cassandra 19, HBase 30, Hadoop MapReduce 36, ZooKeeper 19). order violation만으로 생긴 결함 46개(44%). 64%가 untimely message로 촉발된다. §8.4는 'Lessons Learned'의 제안이고 구현·평가된 검출기가 아니다. 대상은 shared-nothing node다 | 검증 C04 (confirmed) | §4.3 |
| CP2 | stale-value 오류의 정적 검출 선행이 있다 | **[확인된 사실]** Burrows–Leino(krml107 원고 전문). critical section을 다시 들어간 뒤의 사용을 잡는다. 617 kloc에서 false alarm 43, benign 1, bug 4. **[해석]** 동기화 범위 기준이며 메시지 epoch나 물리 시간 기준이 아니다 | 검증 C05 (confirmed) | §4.3 |
| CP3 | ROS에서 소스로 pub/sub topology를 복원하고(ROSDiscover), 메시지가 바꾸는 state 변수로 상태기계를 추론하는(ROSInfer) 선행이 있다 | **[확인된 사실]** ROSDiscover는 ICSA 2022(DOI 10.1109/ICSA53651.2022.00019)이고 구조 규칙만 검사한다 ([paper.pdf @4648ad9](https://github.com/cmu-rss-lab/rosdiscover-evaluation/blob/4648ad9d3c35b1c167130c3d592da2ab395b1ae9/paper.pdf)). ROSInfer(ICSE 2024, DOI 10.1145/3597503.3639206)는 메시지 내용을 모델링하지 않는다. 이미 알려진 결함 3개를 다시 찾았다. 저자들은 실행 시간을 정적으로 알 수 없어 대부분의 race 분석에 쓸 수 없다고 쓴다 ([ROSInfer PDF](https://raw.githubusercontent.com/clegoues/clegoues.github.io/master/assets/papers/Duerschmid2024ROSInfer.pdf)). 두 논문 모두 F′를 일반화 대상으로 꼽는다. cFS는 언급하지 않는다 | 검증 C02 (confirmed) | §4.2 |
| CP4 | framework topology와 소스 handler 흐름을 결합한 동시성 분석이 F′에 있다 | **[확인된 사실]** fprime-topo-analysis([README @f0db61c](https://github.com/FireflySpace/fprime-topo-analysis/blob/f0db61c55e04c2e60a076656eddaf9490316be69/README.md#L545-L560)): lock 순서 deadlock, lock 인식 data race, queue 우선순위·overflow 등 약 20개 분석. runtime guard와 runtime routing은 비목표다. state로의 순서와 freshness 분석은 없다. 출판·peer review는 찾지 못했다 | 검증 C01 (confirmed) | §4.1 |
| CP5 | Ganesan·Lindvall 그룹이 cFS를 직접 시험·검사했다 | **[확인된 사실]** ISSRE 2016(DOI 10.1109/ISSRE.2016.47, R03)은 Spec Explorer로 SB API를 모델 기반 시험했다(초록). SPLC 2009(DOI 10.1145/1753235.1753258)는 CFS 구현을 아키텍처 규칙에 대조했다(초록·snippet). WCRE 2010(DOI 10.1109/WCRE.2010.27)은 GMSEC의 pub/sub 분석이다(snippet). **[미확인]** 세 논문이 MID·pipe 간선을 복원하거나 순서·startup·앱 state를 검사했는지. 본문을 열지 못했다 | 검증 C03 (confirmed) | §4.1 |
| CP6 | IKOS는 단일 thread를 가정하고 entry point를 독립 프로세스처럼 분석한다 | **[확인된 사실]** [analyzer/README.md L341, L535, L538 @ac7f7c17](https://github.com/NASA-SW-VnV/ikos/blob/ac7f7c1738976cabc58c6a53413df6e458995c38/analyzer/README.md#L528-L546). **[미확인]** BioSentinel 적용 수치(약 1200 LOC CFE 모델, 약 1.31% 경고율, 약 17개 bug)는 NTRS slide의 snippet뿐이다 | 검증 C07 (confirmed) | §4.1 |
| CP7 | Goblint v1.1.0의 ARINC 분석은 process·semaphore·event·blackboard를 graphviz·Promela 모델로 내보냈다. sampling·queuing port와 buffer 호출은 `Nop`이다 | **[확인된 사실]** [arinc.ml L386, L545-L560 @5e1a53e2](https://github.com/goblint/analyzer/blob/5e1a53e2c87660f8be8a3bbd43b64075fc15233d/src/analyses/arinc.ml#L545-L560). 파일은 `goblint-1.0.0`(2017)에도 있고 v2.0.0에서 제거되었다 ([CHANGELOG L124-L127](https://github.com/goblint/analyzer/blob/d0ee7d5b0e2b6c12f0d33a285973397ab29eb9fa/CHANGELOG.md#L124-L127)) | 검증 C08 (refuted, 원 문장) | §4.1 |
| CP8 | Ogma가 생성하는 cFS monitor 앱은 받은 값을 전역에 복사하고 active 입력에서만 `copilot_step()`을 부른다. startup sync를 부르지 않는다 | **[확인된 사실]** [copilot_cfs.c L68-L80, L174-L239 @08b384b4](https://github.com/nasa/ogma/blob/08b384b4335cce324fac74577adc7f08e61b7cce/ogma-core/templates/cfs/copilot/fsw/src/copilot_cfs.c#L174-L239). cfs template 디렉터리에 `WaitForStartupSync`·`WaitForSystemState`가 없다. **이견:** README의 예제 실행은 `position` 하나만 감시한다. 여러 stream의 latest value를 결합하는 구조는 입력을 두 개 이상 고를 때만 생긴다. 저장소 예제 중에는 `cfs-002-state-machines`(active `state`, passive `input`)가 그 경우다 | 검증 C06 (contested) | §4.1 |
| CP9 | cFS CI의 정적 분석은 일반 C 검사다 | **[확인된 사실]** cppcheck와 CodeQL JPL/MISRA subset이다. JPL rule 14(반환값 검사)는 제외된다 ([jpl-misra.qls L1-L25](https://github.com/nasa/cFS/blob/5a9b075cd4c818ee8555f349a9f78ba5632d0384/.github/codeql/jpl-misra.qls#L1-L25)). **[해석]** `CFE_SB_Subscribe`·`ReceiveBuffer` 반환 미검사는 공개 CI가 강제하지 않는다 | – | §4.1 |
| CP10 | F′는 생성 코드에서 시작 순서를 고정한다. 모든 연결과 queue가 task 시작 전에 생긴다 | **[확인된 사실]** [TopSetupTeardownFns.scala L40-L68](https://github.com/nasa/fpp/blob/91b474097ea1a5503139440bed661aee5e16a8ad/compiler/lib/src/main/scala/codegen/CppWriter/TopologyCppWriter/TopSetupTeardownFns.scala#L40-L68). **[해석]** cFS 구독은 각 앱이 시작 뒤 부르는 runtime 호출이라 같은 보장이 없다 | – | §4.1 |
| CP11 | 첫 메시지 유실(late joiner)은 잘 알려진 middleware 위험이고 framework 해결책이 있다 | **[확인된 사실]** ZeroMQ 'slow joiner' ([chapter1.txt L215-L224](https://raw.githubusercontent.com/booksbyus/zguide/6752d24b215997aa161e6896941bfd6010d4d3f5/chapter1.txt)), ROS 2 TRANSIENT_LOCAL durability ([QoS 문서 L48-L61](https://github.com/ros2/ros2_documentation/blob/e2388aa72c17598a54fa228df6e9e9a44c5e7aec/source/ROS-Framework/interfaces/topics/About-Quality-of-Service-Settings.rst#L48-L61)). **[해석]** cFS SB에는 그런 설정이 없으므로(CS4) 설정 검사가 아니라 순서 판정이 필요하다 | – | §4.2 |
| CP12 | 확인한 범위에서 cFS 앱 사이의 순서·시간 state를 정적으로 분석한 작업은 찾지 못했다 | **[미확인]** 부재 판단이다. 핵심 본문(R03, SPLC 2009, WCRE 2010, R04, IV&V 보고서, ROSInfer 후속)을 egress 차단으로 읽지 못했다 (`$N/rw-flight/access_log.txt`). 결론의 근거로 쓰지 않는다 | – | §4.7 |

#### 10.2.8 설계 제안 (구현 전)

| # | 주장 | 지위 | 검증할 방법 | 상세 |
| --- | --- | --- | --- | --- |
| CD1 | 결함을 네 조건(필요 순서 `Req`, 보장 부재, Σ에서 실행 가능, 관측 가능한 결과)으로 정의하고, 후보·모델상 가능·재현을 따로 센다 | **[설계 제안]** 원노트 §47 식은 조건 1–3에 해당한다. 조건 4가 없었다 | 사례마다 증거 수준을 기록 | §1.1 |
| CD2 | 결함 범주 F1–F8: 원노트 F1–F6에 메시지 간 순서(F7)와 재시작 창(F8)을 더한다 | **[설계 제안]** | 범주마다 양성·정상 대조 사례 | §1.2 |
| CD3 | `cfs` dialect: 모든 cFS op이 status 결과를 낸다. MID는 값이 정체이고 이름은 부가 정보다. pipe·MID queue·table buffer·ES 상태를 non-addressable resource의 effect로 표현한다 | **[설계 제안]** **[확인된 사실]** non-addressable effect의 NoAlias 처리는 main에 있고 로컬 `1053047a`에는 없다 | 구현 후 verifier·effect 시험 | §5 |
| CD4 | 추상 상태 σ♯ = (Prov, Mem, Aux, Facts). 출처는 field `(전역 symbol, 상수 byte 범위)` 단위이고 공동 갱신 상관을 보존한다 | **[설계 제안]** 원노트 §45의 field별 합집합이 만드는 가짜 조합(수정본 §6.4)을 피하려는 설계다 | 5개 lab 앱의 수작업 label과 비교 | §6.3 |
| CD5 | HB는 cFE 의미에서 얻고 Σ와 cFE 버전을 매개변수로 둔다 | **[설계 제안]** CS5·CT5·CG2의 버전 의존과 CS7·CE4의 Σ 의존이 근거다 | 최소 두 Σ에서 재현 시도 | §6.2 |
| CD6 | 평가는 합성 앱, 공개 앱 결함 주입, 역사적 결함 재현 후보의 세 층으로 하고, 정상 대조(TO_LAB 지연 구독, HK 결합 정책, Ogma passive 입력, 직렬화된 core 시작)에서 경고 0을 요구한다 | **[설계 제안]** | §8의 정답 스키마 | §8 |
| CD7 | 같은 SB·ES·TBL 모델을 넣은 기준선(CodeQL, fprime-topo-analysis식 결합, ROSInfer식 상태기계, CSA, IKOS+stub, Goblint)과 비교하고, field 출처·status·guard 등을 끄는 ablation을 한다 | **[설계 제안]** | 같은 사례 집합, 같은 모델 | §8.9, §8.10 |
| CD8 | 신규성 문장을 'cFS 고유의 `Req`·`G` 출처 + 시간에 따라 바뀌는 구독 집합 + field 단위 출처 + 설계된 정책의 구별'의 결합으로 좁힌다 | **[설계 제안]**·**[해석]** 4.7.3의 본문을 읽기 전까지 잠정적이다 | 10.5의 Q2 | §4.7.4 |

#### 10.2.9 결과에 관한 주장

| # | 주장 | 지위 | 이유 |
| --- | --- | --- | --- |
| CR1 | 제안 분석기의 검출률, precision, recall, 분석 시간 | **[미확인]** | 분석기를 구현하지 않았다 |
| CR2 | MLIR 기반 구현이 대안보다 정확하거나 싸다 | **[미확인]** | 기준선을 실행하지 않았다 |
| CR3 | 분석의 건전성 | **[미확인]** | σ♯의 구체 의미와 증명이 없다. 판정은 Σ에 상대적이다 |
| CR4 | cFE #73을 검출했다 | 주장할 수 없음 | 546a002에서는 경로가 닫혀 있다(CM9). 6.4.1/6.4.2로 파생 예제를 만들 수 있으나 Microblaze·GRC EVA 환경은 공개되어 있지 않다. 결과는 '#73 파생 예제'로만 보고한다 |
| CR5 | Swift/BAT 결함을 설명하거나 검출한다 | 주장할 수 없음 | 코드와 원인이 공개되어 있지 않다(CM4). cFE 실행 근거도 없다(CM5) |

---

### 10.3 교차 검증에서 기각·이견·보정된 주장

**[확인된 사실]** 아래는 검증 단계에서 원래 문장이 기각되었거나, 이견이 남았거나, 결론에 영향을 주는 보정을 받은 주장이다. 노트는 오른쪽 열의 문장만 쓴다.

| ID | 원래 문장의 요지 | 판정 | 노트가 쓰는 문장 | 근거 |
| --- | --- | --- | --- | --- |
| 검증 C08 | Goblint v1.1.0은 ARINC 653의 queuing·sampling port, buffer 등 IPC를 Promela로 추출했다. v1.0.0에는 파일이 없다 | refuted | port·buffer 호출은 `todo()` → `Nop`이다. 모델에 들어가는 것은 process 수명, semaphore, event, blackboard, timed wait다. `v1.0.0` tag는 없다. 실제 1.0.0 tag `goblint-1.0.0`(2017)에 두 파일이 있다 | CP7 |
| 검증 C17 | gcc compile DB에 `-Wno-unknown-warning-option`만 더하면 154개 파일이 import된다. 범위는 'cFE core + 20 apps'다 | refuted | CX2의 범위·절차 문장. 앱 디렉터리는 17개이고 cFE module은 5개다 | CX2 |
| 검증 C21 | #73의 수정 전 코드는 공개되어 있지 않다. `EVS_AppID = CFE_EVS_UNDEF_APPID`가 일관된 AppId 초기화 수정이다 | refuted | 6.4.1(수정 전)과 6.4.2(수정 후)가 SourceForge에 공개되어 있다. `UNDEF_APPID`는 수정 전부터 있던 sentinel이다. SB AppId 초기화의 불일치는 고쳐지지 않았고 #2663으로 남아 있다 | CM8, CM10 |
| 검증 C06 | Ogma cFS 앱은 DB의 모든 MID를 구독하고, 매 수신 뒤 모든 monitor를 평가한다. 그래서 monitor는 따로 갱신된 메시지들의 latest value를 평가한다 | contested | 구독은 선택된 입력 변수의 MID와 REEVAL MID다. `copilot_step()`은 active 입력에서만 불린다. 여러 stream의 latest-value 결합은 입력을 둘 이상 고를 때만 생긴다. 검증자 셋 중 하나는 기각했다 | CP8 |
| 검증 C22 | 어떤 출처도 Swift/BAT를 cFS와 잇지 않는다 | contested | BAT가 cFE를 실행했다는 근거는 없다. 그러나 GSFC slide 하나가 BAT를 'cFE Heritage'로 나열한다. 동기 사례로만 쓰고 benchmark로 쓰지 않는다 | CM5 |
| 검증 C09 | 구독자 없음 event가 '발생한다' | confirmed, 보정 | event는 SB 전체 공유 filter와 발행 앱의 EVS 등록·type mask에 막힐 수 있다. 원래 probe는 EVS에 등록하지 않아 이 event가 보이지 않았다 | CS1 |
| 검증 C10 | 발행 때 sequence counter가 오른다 | confirmed, 보정 | IsOrigination=true이고 경로가 있을 때만 오른다. relay 역전은 relay pipe가 list head에 있는 구독 순서에서만 시험했다 | CS2, CS7 |
| 검증 C11 | ES startup은 timeout으로 묶여 준비를 보장하지 않는다 | confirmed, 보정 | startup script 앱에만 해당한다. core 앱은 30 s hard limit과 panic이 있다. 1000 ms는 바꿀 수 있는 기본값이다 | CE1, CM9 |
| 검증 C12 | core 시작은 직렬화되어 #73을 재현할 수 없다. #2663은 재현하지 않았다 | confirmed, 보정 | '재현 불가'는 코드와 24회 실행에서의 추론이다. #2663의 `ILLEGAL_APP_ID`는 검증 중 gdb로 24회 관측했다 | CM9, CM10 |
| 검증 C15 | owner 재시작 시 `DUPLICATE_NOT_OWNED` | confirmed, 보정 | v7.0.0(c1ab1b7) 이후 동작이다. 이전 버전에서는 일어나지 않을 것으로 추론된다(미실행). 공식 FAQ는 옛 동작을 설명한다 | CT5 |
| 검증 C16 | TO_LAB은 startup sync 뒤에만 구독한다 | confirmed, 보정 | table 기반 telemetry 구독만 미룬다. 명령 MID 구독은 init에서 한다. 이 동작은 v7.0.1에만 있다 | CG2 |
| 검증 C18 | to_lab 상수 회수 2/7 | confirmed, 보정 | 3/7이다. helper `TO_LAB_CmdSubscribe`가 inline 뒤 상수 6272·6273을 받는다 | CX3 |
| 검증 C20 | `LocalAliasAnalysis`가 유일한 upstream alias 구현이다 | confirmed, 보정 | `mlir/lib` 안에서만 유일하다(시험용 subclass 제외). FIR·CIR용 구현이 monorepo에 따로 있다. 인터페이스 이름은 `MemoryEffectOpInterface`다 | CL2 |
| 검증 C01 | fprime-topo-analysis는 약 15개 검사를 하고 메시지 순서 분석이 없다 | confirmed, 보정 | `checks.py` 17개와 별도 analyzer 3개로 약 20개다. queue 우선순위·지연 분석은 있다. 없는 것은 state로의 순서와 freshness 분석이다. commit 날짜는 도구의 기원을 말하지 않는다 | CP4 |
| 검증 C07 | IKOS는 "external functions do not update global variables"를 가정한다 | confirmed, 보정 | README 원문은 "Extern functions (without implementation)"이다. stub을 붙이면 그 함수는 분석된다. 남는 한계는 task 간 순서다 | CP6 |
| 검증 C02 | ROSInfer의 state 변수는 subscriber callback에서 대입되는 변수다 | confirmed, 보정 | 논문 정의는 발행·상태 변경 코드의 조건에 쓰이는 전역·component 범위 변수다. callback 대입은 하위 분류다. 결함 3개는 새로 찾은 것이 아니라 알려진 것을 다시 찾았다 | CP3 |

---

### 10.4 원노트·수정본의 보정·기각 목록

#### 10.4.1 원노트(2026-09-30)

'판정' 열의 뜻: **기각**은 근거가 반대를 보인다. **보정**은 방향은 맞으나 사실이나 범위를 고친다. **유지·한정**은 맞지만 조건을 붙인다.

| # | 원노트 위치 | 원래 주장 | 판정 | 보정된 서술 | 근거 |
| --- | --- | --- | --- | --- | --- |
| O1 | §4.1 L177 | 2023년 '7 minute problem'은 같은 계열의 known race condition이다 | 보정 | GCN 34691은 '7 minute problem'을 known race condition으로 부르지만 22706과의 관계나 attitude를 말하지 않는다 | CM3 |
| O2 | §4.1 L187-205, §22 | 원인은 `Observation(t1) + Attitude(t0)`의 시간 불일치다 | 기각(사실로서) | 공개 근거는 'incorrect spacecraft attitude information'(22706)뿐이다. 도식은 원노트의 해석이며 BAT 재구성으로 쓰지 않는다 | CM2, CM4 |
| O3 | §4.1 L183, §62-3 | Swift는 반복된 실제 비행 race로서 cFS 분석기의 동기다 | 보정 | Circular 반복 보고는 맞다(CM1). BAT는 cFE를 실행한 근거가 없는 cFE heritage 선행 시스템이다(이견 있음). 비cFS 동기 사례로 명시하고 benchmark로 쓰지 않는다 | CM1, CM5 |
| O4 | §4.2 L256-258 | SB가 다시 EVS를 불렀고 uninitialized AppID가 쓰여 segfault했다 | 보정 | SB의 AppId(0)가 EVS 미등록 경로로 이끌었고, 범위를 벗어난 값은 EVS 자신의 sentinel `0xFFFFFFFF`였다 | CM6 |
| O5 | §4.2 L280, §16 L850, §34, §58-P6 | #73은 가장 중요한 startup dependency 사례이고 분석기가 이를 모델링·재현한다 | 보정 | 546a002에서 경로가 닫혀 있다. 현재 코드의 양성 사례가 될 수 없다. 6.4.1/6.4.2 공개 소스로 파생 예제를 만들 수 있다. 현재 코드의 같은 모양 잔여 사례는 #2663(조용한 event 유실)이다 | CM8–CM10 |
| O6 | §4.2 L262-268 | task 생성 순서 ≠ 실행 순서 ≠ 초기화 완료 순서 | 유지·한정 | core 앱은 지금 직렬화되어 세 순서가 일치한다. startup script 앱은 완료 순서가 실행마다 달랐다(6회 6종) | CM9, CE5 |
| O7 | §5-6 | 실제 race/startup dependency bug가 공개 issue로 남아 있다 | 유지·한정 | #71·#72·#73은 milestone 6.4.2로 닫힌 이관 기록이다. 열린 관련 issue는 #2663, #1466, #1509(검사로만 발견), #1750이다 | CM10, CE3 |
| O8 | §5 L336 | SBN 설명이 MID 구독 database와 배포라는 pub/sub 구조를 보여 준다 | 보정 | SBN은 peer cFS 인스턴스 사이의 다리다. 로컬 SB 의미는 cFE 소스로 인용한다 | CM15, CS1–CS6 |
| O9 | §6 | 일반 call graph로는 앱 간 의존이 보이지 않는다 | 유지·한정 | 맞다. 전달은 bodiless `OS_QueuePut`/`OS_QueueGet`에서 끝난다. 그러나 이것은 모든 도구가 SB 모델을 넣어야 한다는 뜻이며, 그 자체로 MLIR의 차별점은 아니다 | CG7, CL8 |
| O10 | §7, §11.1 | `#cfs.mid<ATTITUDE>`처럼 이름이 MID의 정체이고 depth는 속성이다 | 보정 | IR에는 MID 값만 남는다. 정체는 값이고 이름은 sidecar 정보다. depth는 macro 상수 operand다 | CX3, CD3 |
| O11 | §8 L449, §62-7 | MLIR의 generic dataflow solver가 이 semantic layer 구현에 적합하다 | 유지·한정 | solver는 있다(CL1). 메모리 모델, closed-world visibility, 간접 호출 해석, 앱 간 HB 층은 연구자가 만들어야 한다. 적합성은 측정 대상이다 | CL1–CL5, Q1 |
| O12 | §10 | 초기 경로 C → LLVM IR → MLIR LLVM dialect, 장기 경로 CIR | 유지·한정 | 초기 경로는 조건부로 동작한다(CX2). CIR은 로컬에서 시험할 수 없고 `-fclangir`가 조용히 무시되는 모드가 있다(CX6) | CX2, CX6 |
| O13 | §13, §47 | `E1 \|\| E2`이면 potential race다. `Race = RequiredHB ∧ ¬ProvenHB ∧ PotentialConcurrent` | 보정 | 후보 판정일 뿐이다. 관측 가능한 결과 조건이 빠져 있다. 골격 자체는 TaxDC §8.4가 제안했다 | CD1, CP1 |
| O14 | §15, §31 | 구독 준비 전 발행은 first-message race다 | 보정 | SB에는 durability가 없어 유실은 실제로 가능하다(CS1–CS4). 그러나 TO_LAB은 의도적으로 늦게 구독한다. 반드시 받아야 하는 첫 메시지인지가 `Req`의 조건이다. 공개 양성 사례는 찾지 못했다 | CG2, CM13 |
| O15 | §16 L870-878 | scheduler priority 때문에 소비자가 먼저 실행될 수 있으면 race다 | 보정 | 우선순위 순 실행은 RT 권한과 1 CPU에서만 관측되었다. 권한이 없으면 OSAL이 우선순위를 조용히 버린다. 판정은 Σ에 상대적이다 | CE4, CS7 |
| O16 | §17 | Temporal state consistency가 가장 발전 가능성이 높은 영역이다 | 유지·한정 | 확인한 정의 중 run-to-completion에서의 epoch 불일치에 바로 적용되는 것은 없다. 그러나 HLDR 원문과 time-disparity 문헌을 읽지 않아 신규로 주장하지 않는다. HK·Ogma는 latest-value 결합을 설계로 허용한다 | CG1, CP8, Q4 |
| O17 | §21 Property 4 | timestamp 비교가 있으면 consistency guard다 | 보정 | header 시각은 전송 시각으로 덮어써진다. 비교하는 field가 측정 시각인지 먼저 확인한다. TIME 시작 직후는 clock이 NOT_SET이다 | CS9 |
| O18 | §20, §45 | lattice `UNINITIALIZED/CURRENT/POSSIBLY_STALE/UNKNOWN`, 합류에서 출처 합집합 | 보정 | 합집합은 가짜 조합을 만든다(수정본 §6.4). 상관을 보존하는 곱 도메인으로 바꾼다. 메모리 초기화와 유효 입력 수신을 분리한다 | CD4 |
| O19 | §23, §35 | Table lifecycle은 Acquire → Use → Release 프로토콜이다 | 보정 | 잠금은 handle 단위다. 재`GetAddress`는 이전 포인터의 보호를 없앤다. `NEVER_LOADED`는 NULL을 준다. 모델은 buffering 옵션과 버전에 따라야 한다 | CT1–CT3 |
| O20 | §24-C | 미해제 참조가 update를 막는다 | 보정 | single-buffered는 막힘이 아니라 연기(`INFO_TABLE_LOCKED`, 이후 `Manage`)다. double-buffered는 갱신이 진행되고 다음 load가 거절될 수 있다 | CT3 |
| O21 | §24-D | 앱 간 stale handle은 향후 연구다 | 보정(구체화) | 이미 실측으로 재현되는 구체 의존이 있다. owner 재시작 뒤 sharer가 해제할 때까지 재등록이 `DUPLICATE_NOT_OWNED`다. 버전 의존이다 | CT5 |
| O22 | §27 Q1 | SB 통신 관계를 compiler IR에서 정확하게 복원한다 | 보정 | IR만으로는 안 된다. MID macro 이름이 사라지고, 구독 site 47곳 중 8곳이 table·명령에서 MID를 받는다. table 이미지, build 설정, MID 매핑 header가 입력이어야 한다 | CX3, CG4 |
| O23 | §30, §56 | MVP는 SB API 네 개(CreatePipe, Subscribe, ReceiveBuffer, TransmitMsg)로 시작한다 | 보정 | 발행 MID는 `TransmitMsg` 인자가 아니라 앞선 `CFE_MSG_Init`이 buffer에 묶는다. `CFE_MSG_GetMsgId`, dispatch 비교, status, table 소스가 MVP에 필요하다 | CG1–CG4, §5.12 |
| O24 | §41 | 직접적인 경쟁 도구가 없을 가능성이 높다. 기준선은 call graph와 TSan이다 | 기각 | fprime-topo-analysis, ROSInfer, 같은 SB 모델을 넣은 CodeQL, CSA, IKOS+stub이 비교 대상이다. call graph와 TSan은 너무 약한 기준선이다 | CP3, CP4, CL8, CD7 |
| O25 | §44 | §44 예제의 interprocedural 출처 복원부터 MLIR dataflow가 의미를 가진다 | 유지·한정 | upstream은 이 예제를 풀지 못한다(CL5). 바로 그 부분을 연구자가 만들어야 한다. 같은 일을 CodeQL·PhASAR로 할 때의 비용과 비교해야 한다 | CL2–CL5, Q1 |
| O26 | §48–49 | 계약 예: `max_skew = 20ms`, `max_delta=100ms` | 보정 | 임무 근거가 없다(수정본 §6.5). 물리 시간 β는 범위 밖에 두고 이벤트 경계로 F3을 정의한다 | §1.2 F3 |
| O27 | §53 | 신규성은 framework 수준 semantic dependency 복원이다 | 보정 | 그 일반형에는 선행이 있다(ROSInfer, fprime-topo-analysis, Goblint ARINC, SPLC 2009). cFS 고유 의미와 field 단위 출처의 결합으로 좁히고 잠정 가설로 둔다 | CP3–CP7, CD8 |
| O28 | §62 마지막 목록 | 동일 문제를 해결한 기존 분석기가 없는지 미검증 | 유지 | 이번 검색에서 찾지 못했으나 핵심 본문을 읽지 못해 여전히 미검증이다 | CP12, Q2 |
| O29 | §54 | 관련 연구는 7개 무리다 | 보정 | §4의 다섯 무리(flight software 검증, pub/sub 구조 복원, 순서·race 이론과 도구, cause-effect chain, MLIR·대안 분석 기반)로 다시 묶는다. 'framework topology + handler 흐름 결합 도구'와 'cause-effect chain'을 더한다. message race는 §4.3.3의 한 행이다 | §4.1–§4.5 |

#### 10.4.2 수정본(2026-10-01)

| # | 수정본 위치 | 원래 서술 | 판정 | 보정된 서술 | 근거 |
| --- | --- | --- | --- | --- | --- |
| RV1 | §2.2 L110 | Trac 댓글의 2015-05-06 원기록과 2015-06-16 6.4.2 시험 기록은 확인된 사실이다 | **[미확인]**으로 낮춤 | 댓글을 열지 못했다. 6.4.1(2014-12)과 6.4.2(2015-07) 공개일, VDD 날짜(2015-06-29)와는 모순되지 않는다(**[해석]**) | CM7, CM8 |
| RV2 | §2.1 L102 | 해당 기능의 cFS/cFE 사용 여부를 확인하지 못했다 | 보정(구체화) | BAT는 C++·VxWorks·RAD6000이고 cFE 첫 비행보다 앞선다. GSFC slide 하나가 cFE heritage로 나열한다. cFE 실행 근거는 없다(이견 있음) | CM5 |
| RV3 | §4.2 L185 | 구독자 없음은 0-pipe 경로를 가진다 | 보정 | '경로 없음'과 '목적지 0개 경로'는 다르다. 앞의 것만 counter와 (조건부) event를 남긴다 | CS1, CS2 |
| RV4 | §7.1 L315 | 주소가 해제되지 않은 동안 table update는 일어날 수 없다 | 보정 | single-buffered `Update` 경로에서만 맞고 그때도 연기다. double-buffered는 갱신이 진행된다 | CT3 |
| RV5 | §7.1 L317 | `NEVER_LOADED`에서도 0 내용의 유효한 포인터를 얻고 해제해야 한다(확인된 사실) | 기각(546a002 구현 기준) | header 설명일 뿐이다. 546a002·v7.0.0·main 구현은 NULL을 주고 잠그지 않는다. v6.7.0은 header와 맞았다. benchmark 행 'NEVER_LOADED 뒤 해제'(L570)는 버전을 명시해야 한다 | CT1 |
| RV6 | §6.3 L286 | HK는 README만 확인했다 | 보정(구체화) | HK 소스에 `DataPresent`와 설정된 불완전 packet 정책이 있다. 결합은 설계된 정책이다 | CG1 |
| RV7 | §8.2, R03 | R03은 공식 초록·기관 서지로 확인했다 | 한정 | 이번에는 기관 page도 차단되어 snippet 수준만 재확인했다. slide의 모델 세부를 논문에 귀속하는 것은 추론이다 | CP5 |
| RV8 | S14 | MLIR DataFlow tutorial을 API 근거로 쓴다 | 보정 | tutorial은 현재 header에 없는 클래스를 설명한다. 고정 commit의 header로 바꾼다 | CL1 |
| RV9 | S07 | IV&V 보고서 §2.1·§2.2를 읽었다 | 한정 | 이번 조사에서는 다시 열지 못했다. 순서·startup 결과의 보고 여부는 미확인이다 | CM14 |
| RV10 | §5.3, §11.3 | #73은 원버전·환경을 확보해야 재현 여부를 판단할 수 있다 | 보정(구체화) | 수정 전후 소스(6.4.1/6.4.2)는 공개되어 있다. 없는 것은 Microblaze·GRC EVA 환경이다 | CM8 |

---

### 10.5 미해결 질문: 연구에 주는 영향 순

**[해석] 정렬 기준.** 답에 따라 (a) 핵심 기여 문장, (b) MLIR 사용 정당성, (c) 평가 설계 중 무엇이 바뀌는지로 영향을 매겼다. '상'은 답에 따라 연구의 기여나 도구 선택이 바뀐다. '중'은 범위나 평가 설계가 바뀐다. '하'는 인용 정확도나 부수 모델만 바뀐다.

| 순위 | 질문 | 영향 | 막는 주장 | 현재 근거 | 닫는 방법 **[설계 제안]** | 답에 따른 결정 |
| --- | --- | --- | --- | --- | --- | --- |
| Q1 | 같은 SB·ES·TBL 모델을 넣은 CodeQL(`isAdditionalFlowStep`)이나 AST 구현이 `cfs` dialect + MLIR solver와 같은 후보 집합을 내는가? | 상 | 원노트 §42–43의 MLIR 정당성, RQ6, CD8의 (2)–(3) | **[확인된 사실]** CodeQL은 전역을 통해 순서와 무관하게 잇는다(CL8). **[실측]** CSA taint는 double과 불투명 호출에서 사라진다. upstream MLIR은 메모리 모델·visibility·간접 호출을 해결하지 않는다(CL2–CL4). **[미확인]** 어느 기준선도 실행하지 않았다 | 5개 lab 앱과 합성 앱에서 두 구현을 같은 모델로 만든다. 후보 집합, 모델·분석 코드 줄 수, TBL API 추가 비용, 진단의 소스 위치 완전성을 비교한다 | CodeQL이 같은 결과를 더 적은 코드로 내면 MLIR 주장을 철회한다(원노트 §43). 기여는 도메인 의미 쪽에 남긴다 |
| Q2 | 읽지 못한 핵심 선행 본문이 cFS 앱의 순서·state를 이미 모델링했는가? (R03 ISSRE 2016, SPLC 2009, WCRE 2010, R04 Valente 2025, ROSInfer TLA+ 생성기와 2025 학위논문, IV&V §2.1–2.2) | 상 | 신규성 문장 CD8, CP12 | **[확인된 사실]** 모두 초록·snippet 수준이다(CP3, CP5, CM14). 차단 host 목록: `$N/rw-flight/access_log.txt` | 다른 네트워크나 기관 사본으로 본문을 확보한다. 각 논문의 입력(소스·모델·trace), 속성, 보장을 4.6절 표에 채운다 | 하나라도 cFS C 소스에서 앱 간 순서·state 출처를 다루면 이 연구를 그 확장·비교로 재배치한다 |
| Q3 | 앱별 계약 없이 `Req`를 얼마나 얻을 수 있는가? | 상 | H2′·H3′, RQ4, N4(정상 대조 구별) | **[확인된 사실]** API 의미에서 얻는 `Req`가 있다(CT1–CT3, CS8, CE3). 첫 메시지 필수 여부와 latest-value 허용 여부는 코드에 없다. TO_LAB·HK·Ogma는 설계로 허용한 사례다(CG1, CG2, CP8) | 5개 lab 앱의 후보를 모두 수작업으로 분류한다. 계약 없이 판정되는 비율, P식 deferred/ignored 주석이 필요한 비율을 센다 | 대부분 계약이 필요하면 기여를 '계약 검사'로 좁힌다. 자동 판정 주장을 하지 않는다 |
| Q4 | F4(epoch가 다른 입력 조합)는 기존 정의로 이미 다뤄지는가? 공개 cFS 앱에 같은 epoch를 요구하는 소비자가 있는가? | 상 | 원노트 §17의 '가장 유망한 영역', N5, F4 범주 | **[미확인]** Artho 등 HLDR, atomic-set serializability, time-disparity·AUTOSAR TIMEX를 읽거나 검색하지 않았다. **[확인된 사실]** HK는 latest-value 결합을 설계로 허용한다. F4의 공개 issue는 찾지 못했다(TIME 기준 다중 field 읽기 #46·#1544는 앱 사이 사례가 아니다) | 문헌을 읽는다. LC·HK·SC·MD 등 공개 앱에서 여러 MID의 state를 함께 쓰는 소비자를 전수 조사한다 | 기존 정의가 덮으면 인용하고 신규 주장을 버린다. 그런 소비자가 없으면 F4는 합성 전용 범주로 낮춘다 |
| Q5 | 판정이 Σ(OSAL·RTOS, CPU 수, 권한, cFE 버전)에 얼마나 의존하는가? | 상 | 모든 '모델상 가능' 판정, 건전성 서술 | **[실측]** relay 역전 1 CPU 500/500 대 4 CPU 0/500(CS7). 우선순위 순 시작은 RT+1 CPU에서만(CE4). **[확인된 사실]** put의 lock 위치(CS5), TBL 재등록(CT5), TO_LAB 지연 구독(CG2)이 버전마다 다르다. **[미확인]** VxWorks queue 순서, RTOS 비행 build, 다중 코어 `SystemState` 메모리 순서 | RTOS 대상 build 하나 이상을 고정해 CS7·CE1·CT5 probe를 다시 실행한다. OSAL·PSP 고정값을 하나로 통일한다(현재 두 조합) | Σ마다 결과가 다르면 Σ별로만 보고한다. Σ 없는 일반 주장은 하지 않는다 |
| Q6 | 수작업 주석 없이 field 단위 출처를 복원할 수 있는가? | 중 | RQ2, H3′, CD4 | **[실측]** 직접 store 83곳 인식, offset 0·out-parameter·지역 포인터 누락(CX4). 간접 호출 73곳(CL4). PoTATo build 실패(CL7) | CD4 도메인을 구현해 5개 lab 앱의 수작업 label과 비교한다. out-parameter는 API 모델로, 지역 포인터는 SSA·summary로 처리한다 | 복원이 안 되면 H3′를 기각하고 앱 단위의 거친 출처로 후퇴한다 |
| Q7 | 소스·table·build 설정으로 실행 중 구독 집합을 누락 없이 복원하는가? | 중 | RQ1, H1′ | **[실측]** 코드 정의 MID는 상수로 회수된다(CX3). 47곳 중 8곳이 data·명령 구동이다(CG4). **[미확인]** 실행 시간에 file로 load한 table, EDS build | SB subscription reporting을 MsgLim이 큰 전용 pipe로 받아 정답 집합을 만든다. 기본 SBN pipe에서는 보고가 MsgLim·overflow로 버려졌다(CS11) | 누락 간선을 IR 손실·table 해석·모델 누락으로 나누어 보고한다 |
| Q8 | 역사적 수정 전후 쌍에서 판정이 기대한 방향으로 바뀌는가? | 중 | 평가 층 (c), RQ3 | **[확인된 사실]** 후보 쌍이 공개되어 있다: #73 파생(6.4.1/6.4.2), #198(6.5.0a/6.6.0a), CF #184, sample_app #101, #950(CM8, CM11). **[미확인]** 어느 것도 build·실행하지 않았다 | 수정 전후를 각각 build하고 분석기 판정과 native 재실행 결과를 함께 기록한다 | 방향이 맞지 않으면 해당 범주의 `Req` 출처를 다시 검토한다 |
| Q9 | F1의 실제 양성 사례가 있는가? | 중 | 외적 타당성, H2′의 F1 부분 | **[확인된 사실]** 검색 범위에서 찾지 못했다(CM13). TO_LAB은 의도된 유실이다 | 검색 범위를 넓힌다. 공개 앱 결함 주입으로 양성 사례를 만든다 | 없으면 F1은 합성·주입 사례로만 보고한다 |
| Q10 | 시작 구간의 실제 SB 손실 수와 그 기능 영향은? | 중 | CS11 해석, 정상 대조 label | **[실측]** 보이는 것은 EVS filter 상한(4, 16)뿐이다. SBN의 `0x80e` 손실 영향은 미확인이다 | SB housekeeping의 `NoSubscribersCounter`·`MsgLimitErrorCounter`·`PipeOverflowErrorCounter`를 수집한다. SBN의 회복 경로를 읽는다 | 기능 영향이 있으면 공개 bundle의 F1·F7 후보로 올린다 |
| Q11 | ClangIR로 바꾸면 field·구조 정보 손실이 줄고 비용은 감당할 만한가? | 하 | frontend 선택 | **[실측]** 로컬 불가(CX6). LLVM dialect 경로는 동작한다(CX2) | CIR을 켠 clang을 별도로 build하고 154개 파일의 컴파일 coverage를 잰다 | LLVM 경로가 막히지 않으므로 MVP에는 영향이 없다. ablation으로만 둔다 |
| Q12 | TBL의 부수 의미: owner `Load`의 lock 밖 구간(CT6), child task의 handle 공유(CT7), ground Load→Validate→Activate 경로와 알림 유실 | 하 | F6 모델의 세부 | **[미확인]** 코드 읽기만 했다 | stress 시험과 child task probe | 결과에 따라 F6 모델에 상태를 더한다 |
| Q13 | 모델링 비용은 얼마인가? (IKOS–BioSentinel의 약 1200 LOC CFE 모델) | 하 | 비용 추정 | **[미확인]** snippet뿐이다(CP6) | NTRS slide 원문 | 비용 서술에만 영향 |
| Q14 | Swift/BAT race의 실제 원인과 cFE와의 관계 | 하 | 동기 사례의 서술 | **[미확인]** 원인 문서가 없다(CM4). heritage 관계는 이견이다(CM5) | Swift 팀 문서·BAT 논문 원문 | 설계·평가에는 영향 없음. 동기 서술만 바뀐다 |
| Q15 | #73 Trac 댓글의 원래 날짜 | 하 | 인용 정확도 | **[미확인]**(CM7) | nasa/cFE API 접근이 허용된 환경에서 댓글을 읽는다 | 인용 날짜만 바뀐다 |

---

### 10.6 현재 판단

**[해석]** 확인된 것은 출발점이다. cFE 546a002의 SB, ES, TBL은 발신자에게 보이지 않는 전달 실패, soft startup 동기화와 자기 준비 표시, handle 단위 table 잠금, 버전에 따라 다른 재시작 동작을 가진다(CS1–CS7, CE1–CE3, CT1–CT5). 이 의미는 정적 분석의 `Req`와 `G`의 구체 출처가 될 수 있다. 원노트의 frontend 경로도 실제 cFS 코드에서 막히지 않았다(CX2–CX5).

**[해석]** 확인되지 않은 것은 기여 자체다. 세 가지가 남아 있다. 같은 의미 모델을 넣은 대안이 같은 결과를 내는지(Q1), 읽지 못한 선행 본문이 같은 일을 했는지(Q2), 그리고 앱별 계약 없이 `Req`를 얻을 수 있는지(Q3)다. 이 셋이 닫히기 전에는 신규성, MLIR의 이점, 검출 성능을 주장하지 않는다. 다음 단계는 §7.13의 추출 정답표를 자동으로 재현하는 것이고, race 검출 여부는 그다음 질문이다.
