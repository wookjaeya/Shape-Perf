## 2. 동기 사례와 실제 근거

이 절에서는 원노트가 연구 동기로 든 사례를 다시 확인한다. 대상은 네 가지다.

- Swift/BAT GCN Circular
- cFE #73과 #71
- NASA 공개 저장소에서 찾은 다른 이슈
- 2020년 IV&V 보고서

각 사례마다 확인된 사실, 아직 모르는 것, cFS와의 관계, 재현 가능성을 나누어 적는다. 마지막 2.7절에서는 이 사례들이 연구 주장을 어디까지 지지하는지 정리한다.

**[확인된 사실]** 이번 조사에서는 egress proxy가 gcn.nasa.gov, ntrs.nasa.gov, arxiv.org와 GitHub REST API를 막았다. GitHub issue는 HTML 페이지(WebFetch 요약)와 검색 API의 metadata로만 읽었다. WebFetch 요약은 원문을 그대로 옮기지 않고 바꿔 말한다. Trac에서 이식된 issue 댓글은 열지 못했다. 따라서 각 항목에 확인 수준을 따로 적는다.

### 2.0 사례 요약

| 사례 | **[확인된 사실]** 성격 | **[확인된 사실]** cFS와의 관계 | 비행 중 발생 | 공개 코드 | **[해석]** 이 노트에서의 용도 |
| --- | --- | --- | --- | --- | --- |
| Swift/BAT GCN Circular | 임무팀의 운용 보고 | cFS 위에서 동작한다는 근거가 없다. GSFC의 cFE heritage 목록에는 올라 있다(2.1.5절). | 보고된 현상이다. 원인 코드는 공개되지 않았다. | 없음 | 동기로만 쓴다. benchmark로 쓰지 않는다. |
| cFE #73 (Trac #42) | core app 사이의 startup 결함 | cFE 자체 | 확인되지 않았다. Microblaze 포팅 중에 발견되었다. | 수정 전 6.4.1과 수정 후 6.4.2가 SourceForge에 있다. | 파생 benchmark 후보 |
| cFE #71, #72, #105, #1034 | startup sync primitive의 결함과 후속 조치 | cFE 자체 | 확인되지 않았다. | 6.4.1과 6.4.2, 이후 git 이력 | #73과 묶어 한 사례로 센다. |
| 기타 후보 38건 | 개발 이슈와 수정 기록 | cFE와 공개 앱 | 확인되지 않았다. | 대부분 공개 | 재현 후보와 정상 대조군 |
| NASA/CR-20205010026 | IV&V/SARP V&V 보고 | cFS가 대상 | 해당 없음 | 해당 없음 | 정적 분석이 적용된 맥락 |

---

### 2.1 Swift/BAT GCN Circular

#### 2.1.1 확인 수준

- **[확인된 사실]** 수정본은 GCN 22706, 32397, 34691, 43470의 날짜와 요지를 확인했다고 기록한다 [S03–S06].
- **[미확인]** 이번 조사에서는 Circular 원문을 다시 열지 못했다. gcn.nasa.gov가 curl과 WebFetch 모두에서 차단되었다. 아래 영문 인용은 검색 색인의 snippet이다. 원문과 문자 단위로 대조하지 않았다.
- **[미확인]** 따라서 "Circular가 X를 말하지 않는다"는 서술은 snippet에 그 내용이 없다는 뜻이다. 원문 전체에 없다는 뜻이 아니다.

#### 2.1.2 Circular별 보고 문구

| Circular | 날짜 | trigger와 천체 | snippet 문구 | 확인 수준 |
| --- | --- | --- | --- | --- |
| GCN 22706 [S03] | 2018-05-11 | trigger 831888, 4U 1416-62 | "The misidentification was caused by a known race condition in the Swift/BAT software, which leads to incorrect spacecraft attitude information to be applied to a known source." | **[확인된 사실]** 요지는 S03에 있다. 문구는 snippet이다. |
| GCN 32397 [S04] | 2022-07-14 | trigger 1116172 ("=GRB 220714A"), Cen X-3 | "A known race condition in the onboard software caused the currently-active source Cen X-3 to be misidentified" | **[확인된 사실]** 요지는 S04에 있다. snippet의 제목은 "... is not a real astrophysical event"다. |
| GCN 34691 [S05] | 2023-09-14 | trigger 1191994, SWIFT J1727.8-1613 | "BAT misidentified the location of a strong image peak due to a known race condition (the '7 minute problem') in the on-board software." | **[확인된 사실]** 요지는 S05에 있다. snippet에는 attitude가 나오지 않는다. |
| GCN 43470 [S06] | 2026-01-20 | trigger 1442421, 1A 1118-61 | "The source was misidentified on board due to a known race condition in the software, leading to some spurious GCN Notices which did not identify the source name." | **[확인된 사실]** 요지는 S06에 있다. |
| GCN 21077 [S32] | 2017-05-08 | trigger 751928, Cen X-3 | "due to a known race condition in the Swift/BAT software, BAT misidentified Cen X-3 as a different source" | **[미확인]** snippet만 있다. |

**[해석]** 다섯 보고에서 공통으로 나오는 것은 "known race condition"이라는 임무팀의 원인 표현뿐이다. 결과는 보고마다 다르다. 알려진 천체의 오식별, 이미지 peak 위치 오류, 실제 천체가 아닌 trigger, 천체 이름이 빠진 잘못된 Notice가 있다. 22706만이 race의 결과로 "incorrect spacecraft attitude information"을 언급한다.

#### 2.1.3 검색으로 추가 발견한 Circular

**[미확인]** 아래 Circular도 "known race condition"을 원인으로 든다. 모두 검색 snippet으로만 확인했다. 체계적으로 열거한 목록이 아니다.

| Circular | 날짜 | snippet 요지 |
| --- | --- | --- |
| GCN 14910 [S26] | 2013-06-16 | trigger 557910. "incorrect spacecraft attitude information applied to a known source"라는 문구가 있다. snippet에는 race가 나오지 않는다. |
| GCN 14920 [S27] | 2013-06-23 | trigger 558828. "known race condition in the Swift software". 실제 천체는 4U 1700-377이었다. |
| GCN 15695 [S28] | 2014-01-07 | KS 1947+300. GCN Notice가 known race condition의 결과였다고 한다. |
| GCN 17919 [S29] | 2015-06-10 | trigger 643078, H1743-322. 다른 천체 위치로 오식별되어 Transient Source response가 발생했다. slew 후 해당 천체는 XRT 시야에 없었다. |
| GCN 19982 [S30] | 2016-10-04 | trigger 715209, Sco X-1 오식별 |
| GCN 20555 [S31] | 2017-01-28 | trigger 735413. "known race condition in the BAT onboard software" |
| GCN 21190 [S33] | 2017-06-04 | trigger 755848, Cen X-3 |
| GCN 18262, 21305/21306 | 날짜 미확인 | GRS 1915+105(trigger 654954), GRS 1716-249(trigger 760835) |
| GCN 22940 [S34] | 2018-07-11 | trigger 847216, IGR J16385-2057 |
| GCN 23260 [S35] | 2018-09-24 | trigger 863053. "misidentified as IGR 17511-3057 due to incorrect attitude information ... known race condition in the on-board software" |

**[해석]** snippet 기준으로 이 표현은 2013년부터 2026년까지 반복해서 나온다. 반복 보고는 같은 표현이 계속 쓰였다는 사실만 보여 준다. 같은 결함인지, 몇 개의 결함인지, 얼마나 자주 발생하는지는 알 수 없다. 이 목록으로 건수나 발생률을 계산하지 않는다.

#### 2.1.4 알려진 것과 알려지지 않은 것

| 질문 | 상태 |
| --- | --- |
| 임무팀이 원인을 race로 부르는가 | **[확인된 사실]** 그렇다 [S03–S06]. |
| 결과가 운용에 영향을 주었는가 | **[확인된 사실]** 잘못된 trigger, 실제 천체가 아니라고 정정된 GRB 지정, 천체 이름이 빠진 잘못된 GCN Notice가 보고되었다 [S04, S06]. **[미확인]** slew 등 다른 운용 영향은 snippet에만 있다(17919). |
| attitude가 오래된 값이었는가, 다른 epoch의 값이었는가 | **[미확인]** 22706은 "incorrect"라고만 한다. stale이나 epoch라는 표현은 snippet에 없다. |
| race에 관여한 task, thread, ISR와 공유 자료 | **[미확인]** 공개 자료가 없다. |
| "7 minute problem"이 무엇인가 | **[미확인]** 기술적 설명을 찾지 못했다. 검색 대상인 swift.gsfc, ADS, arXiv는 차단되었다. |
| 여러 Circular의 race가 같은 결함인가 | **[미확인]** snippet 기준으로 34691은 22706을 언급하지 않는다. |
| 결함이 수정되었거나 수정 계획이 있는가 | **[미확인]** |
| 결함이 RAD6000 쪽 C++ 코드에 있는가, DSP 쪽에 있는가 | **[미확인]** |

#### 2.1.5 cFS와의 관계

이 판정은 검증에서 의견이 갈렸다. 검증 3건 중 2건은 수정된 문구로 주장을 유지했고 1건은 주장을 반박했다. 세 건 모두 "Swift/BAT와 cFS를 잇는 출처가 없다"는 문장은 틀렸다고 보았다. 그래서 불확실성을 그대로 적는다.

- **[미확인]** arXiv:astro-ph/0408494(2004, 발사 전) snippet은 BAT 소프트웨어를 다음과 같이 설명한다 (abstract [R59], 원문 차단).
  - Image Processor의 RAD6000에서 VxWorks 위의 C++ 코드가 실행된다.
  - 별도의 ADSP21020 DSP가 이미지 생성과 처리를 맡는다.
  - "Command and Data Handling Software Bus"를 포함한 engineering code는 원래 Triana용으로 개발되었다.
- **[미확인]** snippet에 따르면 Swift는 2004-11-20에 발사되었고, cFE는 LRO(2009)에서 처음 사용되었다.
- **[확인된 사실]** GSFC 발표자료 NTRS 20090005965(Charlie Wildermann/FSW GSFC, "cFE/CFS", 2008-11-13)의 "cFE Heritage" 슬라이드에 "Swift BAT"와 "(12/04)"가 나온다. 같은 슬라이드에 Triana, SMEX-Lite, TRACE, WIRE, ST-5 (5/06), JWST ISIM (2011)과 "Core FSW Executive"도 나온다. 이 자료는 NTRS 원본이 아니라 제3자 GitHub mirror의 OCR 본문으로 읽었다 (mirror @2338b2e [S111]; `$N/verify-C22/ntrs_20090005965_djvu.txt:1,5-6,58,62,91,101-103`).
- **[해석]** BAT 비행 소프트웨어는 cFE 이전의 GSFC heritage 시스템으로 보는 것이 맞다. BAT가 cFE나 cFS 위에서 동작한다는 근거는 없다. heritage의 방향이 BAT에서 cFE로 향한다는 판단은 슬라이드 제목에서 추론한 것이다. OCR이 슬라이드의 화살표 배치를 잃었기 때문이다.
- **[해석]** 앞선 fact-check는 "Swift/BAT와 cFS를 잇는 출처가 없다"고 판정했다. 위 heritage 슬라이드가 이 판정을 반박한다. 반대로 "BAT는 cFS 사례다"라는 서술도 근거가 없다.
- **[미확인]** BAT의 Triana 계열 Software Bus 코드가 cFE SB의 직접 조상인지는 확인되지 않았다. 이름이 같다고 해서 cFE SB의 queue, MsgLim, 구독 의미가 BAT에도 있다고 가정하지 않는다.

#### 2.1.6 원노트 서술의 보정

| 원노트 서술 | 보정 |
| --- | --- |
| **[원노트 구상]** "2023년에는 '7 minute problem'이라고 명시하면서 같은 계열의 known race condition으로 잘못된 위치가 계산됐다" (원노트 §4.1) | **[해석]** 34691은 이미지 peak 위치 오류를 "known race condition (the '7 minute problem')"으로 설명한다. snippet 기준으로 22706의 attitude race와 같은 계열이라는 말은 없다. "7 minute problem"은 설명되지 않은 팀 내부 명칭으로 둔다. |
| **[원노트 구상]** `Observation(t1) + Attitude(t0)` 도식, `TemporalCoherent(Observation, Attitude)` 위반 (원노트 §4.1, §22) | **[해석]** 원노트가 세운 추론이다. Circular는 "incorrect attitude"만 말한다. 이 도식은 실제 Swift 구조를 재구성한 것으로 쓰지 않는다. 수정본 §2.1의 편집 보정과 같은 결론이다. |
| **[원노트 구상]** "Swift 같은 실제 비행 시스템에서 state/timing 관련 onboard race가 반복적으로 보고됐다"를 cFS 분석기의 근거로 사용 (원노트 §62) | **[해석]** BAT는 cFS가 아닌 시스템(C++/VxWorks/RAD6000)이다. cFE heritage 목록에 올라 있지만 cFE를 쓰지 않는다. "cFS 밖에서 보고된 비행 동기 사례"로 표시해 인용한다. "state/timing 관련"이라는 수식은 22706의 attitude 언급 이상으로 확장하지 않는다. |
| **[원노트 구상]** "이 문제는 단발적인 historical anomaly가 아니다" | **[해석]** 같은 원인 표현이 2013–2026년 동안 반복된 것은 snippet으로 지지된다. 한 결함의 반복인지 여러 결함인지는 모른다. |

#### 2.1.7 그 밖의 비-cFS 비행 사례

- **[미확인]** Havelund, Lowry, Penix(TSE 27(8):749–765, 2001)와 후속 보고의 snippet을 보면 다음과 같다. DS1 Remote Agent의 plan execution 모듈을 SPIN으로 검사해 동시성 오류 다섯 건을 찾았다. 같은 패턴의 다른 결함은 시험을 통과했고, 1999-05-18 비행 중 deadlock을 일으켰다. 원인은 두 EXEC thread 사이의 critical section 누락이었다 (Semantic Scholar record [R21], NTRS 20000055731; 원문 차단).
- **[해석]** 이 사례는 thread와 lock 수준의 결함이다. 앱 사이 pub/sub 순서 결함이 아니다. "순서 결함이 비행에서 문제가 된다"는 일반 주장만 지지한다. 이 연구가 다루는 결함 범주의 근거로는 쓰지 않는다.

---

### 2.2 cFE #73: core app startup 순서 결함

#### 2.2.1 기록의 정체

| 항목 | 내용 | 근거 |
| --- | --- | --- |
| GitHub issue | nasa/cFE #73 [S01], "Race conditions / dependencies between CFE core apps", label bug, milestone 6.4.2, state_reason completed, 작성 계정 skliper | **[확인된 사실]** 검색 API metadata와 본문 [S01] |
| GitHub 날짜 | 생성 2019-09-30T17:50:27Z, 종료 17:50:37Z, 댓글 14개 | **[확인된 사실]** 검색 API metadata |
| 이식 여부 | 같은 계정이 같은 날 #71(17:50:02), #72(17:50:20), #105(17:54:02)를 몇 초에서 몇 분 간격으로 만들었다. | **[해석]** Trac에서 일괄 이식된 기록으로 본다. |
| Trac ticket | cFE 6.4.2 Version Description Document(2015-06-29) Attachment 1에 "#42 Race conditions / dependencies between CFE core apps, defect, 6.4.2, other"가 있다. | **[확인된 사실]** `$N/verify_C21/vdd642.txt:13,626-627` (6.4.2 OSS tarball 안의 VDD) |
| 대응 관계 | 제목과 milestone이 같다. | **[해석]** GitHub #73과 Trac #42를 같은 ticket으로 본다. |
| 발견 환경 | "the Microblaze processor used by the EVA team at GRC" | **[확인된 사실]** issue 본문 |
| 원기록 날짜 | 수정본은 Trac 댓글에 2015-05-06 원기록과 2015-06-16 시험 기록이 있다고 적는다 [S01]. | **[미확인]** 이번 조사에서는 댓글이 표시되지 않았다(API 403, HTML에 댓글 없음). |
| 공개 release 날짜 | SourceForge 목록 기준으로 cFE-6.4.1은 2014-12-12, cFE-6.4.2-OSS-release.tar.gz는 2015-07-13이다. | **[확인된 사실]** SourceForge coreflightexec files [S110] (검증 단계 확인). **[해석]** 수정이 2015년에 이루어졌다는 수정본 기록과 맞는다. 그러나 댓글 날짜를 직접 확인한 것은 아니다. |

#### 2.2.2 실패 경로

**[확인된 사실]** 아래 단계는 issue 본문에 있다. 오른쪽 열은 수정 전 공개 release인 cFE 6.4.1에서 해당 코드를 찾은 위치다. 6.4.1 tarball의 sha256은 `ec26e33b4ab40b9616ccdbc984c5a00a790a7d05b260f2628c3335a29c23e98f`이다 (`$N/verify_C21/sha256.txt`). 아래 경로는 `$N/verify_C21/v641/cFE-6.4.1-OSS-release/cfe/fsw/` 기준이다.

| 단계 | issue 본문 | cFE 6.4.1 코드 위치 |
| --- | --- | --- |
| 1 | core task를 EVS, SB, ES, TIME, TBL 순서로 만든다(본문에는 "BL"로 오타). | `cfe-core/src/es/cfe_es_objtab.c:120-129` |
| 2 | default/example 설정의 priority는 EVS 61, SB 64, ES 68, TIME 60, TBL 70이다. | `platform_inc/cpu1/cfe_platform_cfg.h:1115,1138,1161,1188,1217` |
| 3 | 네 번째로 생성된 TIME의 TaskMain이 먼저 실행된다. TIME은 `CFE_SB_CreatePipe`를 호출한다. | `cfe-core/src/time/cfe_time_task.c:308` |
| 4 | `CFE_SB_CreatePipe`가 `CFE_EVS_SendEventWithAppID(..., CFE_SB.AppId, ...)`를 호출한다. 이때 SB가 아직 실행되지 않아 `CFE_SB.AppId`는 0이다. | `cfe-core/src/sb/cfe_sb_api.c:195,264` |
| 5 | AppID 0은 범위 검사를 통과한다. 하지만 EVS에 등록되지 않았으므로 `EVS_NotRegistered`를 거쳐 `EVS_SendEvent`로 간다. | `cfe-core/src/evs/cfe_evs.c:285-292`, `cfe_evs_utils.c:194-211` |
| 6 | `EVS_SendEvent`가 `CFE_EVS_GlobalData.EVS_AppID`로 `EVS_IsFiltered`를 호출한다. EVS가 초기화되지 않았으므로 이 값은 sentinel `0xFFFFFFFF`이다. | `cfe_evs_utils.c:616`, `cfe_evs_task.c:190`, `cfe_evs_task.h:105` |
| 7 | `EVS_IsFiltered`가 범위 검사 없이 배열을 index하여 segfault가 나고 cFE core가 멈춘다. | `cfe_evs_utils.c:243-244` ("Caller has verified that AppID is good") |

**[해석]** 범위를 벗어난 index는 SB의 AppId가 아니다. EVS 자신의 sentinel `0xFFFFFFFF`이다. SB의 AppId 0은 다른 앱의 유효한 ID로 읽힐 수 있는 값이다. 이 값 때문에 호출이 등록되지 않은 앱의 처리 경로로 들어갔다. 원노트의 "결국 uninitialized AppID가 사용되며 segfault가 발생했다"는 이 두 값을 구분하지 않는다.

**[해석]** 원노트 §16의 `A.ServiceReady → B.UseService` 구조에 그대로 맞는 사례다. 여기서 A는 EVS와 SB, B는 TIME이다. 다만 사용자 앱 사이의 결함이 아니라 framework core service 사이의 결함이다.

#### 2.2.3 수정: 6.4.2

**[확인된 사실]** 6.4.2 VDD는 이 build의 목적을 "to fix a series of startup race conditions that were found in cFE build 6.4.1"이라고 적는다 (`$N/verify_C21/vdd642.txt:40`). 아래 표는 issue가 제안한 세 가지 수정과 6.4.2의 실제 반영을 비교한다. 6.4.2 tarball의 sha256은 `c64ed2aac910e79a641f49279dfdff3b7b49374e54ed56b595d47e16a3f53a4f`이다. 경로는 `$N/verify_C21/v642/cFE-6.4.2-OSS-release/cfe/fsw/cfe-core/src/` 기준이다.

| issue의 제안 | 6.4.2에 반영된 내용 | 근거 |
| --- | --- | --- |
| core app을 하나씩 RunLoop까지 동기화한다. | **[확인된 사실]** 반영되었다. ES는 core task를 하나 만들 때마다 `CFE_ES_ApplicationSyncDelay`로 기다리고, 시간 초과면 `CFE_PSP_Panic(CFE_PSP_PANIC_CORE_APP)`를 호출한다. 각 core app은 `CFE_ES_WaitForStartupSync`를 호출한다. | VDD Trac #40 (`vdd642.txt:74`); `es/cfe_es_start.c:936-949`(함수 정의는 1007행); `sb/cfe_sb_task.c:143`; `time/cfe_time_task.c:209`. 6.4.1에는 `ApplicationSyncDelay`가 없다(grep 0건). |
| `EVS_IsFiltered`에 범위 검사를 넣는다. | **[확인된 사실]** 부분 반영이다. 검사는 호출자인 `EVS_SendEvent`에 들어갔다(`EVS_AppID < CFE_ES_MAX_APPLICATIONS &&`). `EVS_IsFiltered` 안에는 여전히 "Caller has verified" 주석만 있다. | VDD Trac #42 (`vdd642.txt:85`); `evs/cfe_evs_utils.c:615-621` |
| SB와 EVS의 AppID 초기값을 일관되게 만든다. | **[확인된 사실]** 반영되지 않았다. `EVS_AppID = CFE_EVS_UNDEF_APPID`는 수정 전 6.4.1에도 이미 있다(`cfe_evs_task.c:190`). 6.4.2는 `EVS_AppID`를 TaskInit의 마지막에 설정하도록 바꾸었을 뿐이다(`cfe_evs_task.c:436`). SB AppId는 여전히 SB TaskInit에서만 설정된다. | VDD Trac #42 (`vdd642.txt:86`) |

**[확인된 사실]** 같은 코드가 v6.5.0a(git commit [b2765d9f](https://github.com/nasa/cFE/blob/b2765d9f905aa11f0c3b419b62233597366a7db9/cfe/fsw/cfe-core/src/es/cfe_es_start.c#L934-L952), import commit 792f5e35)에도 남아 있다. 가드는 [cfe_evs_utils.c#L612-L617](https://github.com/nasa/cFE/blob/b2765d9f905aa11f0c3b419b62233597366a7db9/cfe/fsw/cfe-core/src/evs/cfe_evs_utils.c#L612-L617)에 있다. SB AppId는 [cfe_sb_task.c#L206](https://github.com/nasa/cFE/blob/b2765d9f905aa11f0c3b419b62233597366a7db9/cfe/fsw/cfe-core/src/sb/cfe_sb_task.c#L206)에서만 설정된다.

**[확인된 사실]** 앞선 fact-check와 issue 조사는 "v6.5.0a에 수정이 있다"고 적었고, `EVS_AppID = CFE_EVS_UNDEF_APPID`를 일관된 초기화 수정으로 들었다. 이 판정은 검증 단계에서 반박되었다. 수정이 처음 공개된 판은 6.4.2다. `CFE_EVS_UNDEF_APPID`(0xFFFFFFFF)는 수정이 아니라 issue가 문제로 지목한 바로 그 sentinel이다.

#### 2.2.4 현재 버전(546a002)에서의 상태

- **[확인된 사실]** 고정 commit [546a002](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_start.c#L767-L876)에서 `CFE_ES_CreateObjects`는 두 단계로 동작한다 [S25].
  - 먼저 모든 core module의 EarlyInit을 실행한다(L777-799).
  - 그다음 TaskMain이 있는 module(ES, EVS, SB, TBL, TIME)의 task를 하나씩 만든다.
  - task를 하나 만들 때마다 `CFE_ES_MainTaskSyncDelay(CFE_ES_AppState_RUNNING, CFE_PLATFORM_CORE_MAX_STARTUP_MSEC)`로 기다린다(L861). 기본 대기 시간은 30000 ms다.
  - 실패하면 `CFE_PSP_Panic`를 호출한다(L864-871).
- **[확인된 사실]** 순서는 기본 `MISSION_CORE_MODULES` 목록이 정한다([mission_defaults.cmake#L21-L40](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/cmake/mission_defaults.cmake#L21-L40)). 임무는 이 목록을 바꿀 수 있다.
- **[확인된 사실]** `EVS_SendEvent`는 `EVS_GetAppDataByID`로 `EVS_AppID`를 검사한다([cfe_evs_utils.c#L607-L635](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/evs/fsw/src/cfe_evs_utils.c#L607-L635)). `CFE_EVS_SendEventWithAppID`는 알 수 없는 AppID에 대해 `CFE_EVS_APP_ILLEGAL_APP_ID`를 반환한다([cfe_evs.c#L185-L188](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/evs/fsw/src/cfe_evs.c#L185-L188)).
- **[실측]** native build(cFE 546a002, OSAL 5befd8e9, PSP 36c24cb9)를 4개 scheduling 구성에서 실행했다. 구성마다 본 실행 1회와 반복 5회를 합쳐 모두 24회다. 24회 모두 core 초기화 event 순서가 ES, EVS, SB, TBL, TIME이었다. SCHED_RR 구성에서는 CFE_TIME main task의 RT priority가 75로 core main task 중 가장 높았다(EVS 74, SB 73, ES 72, TBL 71).
  - 명령: `grep -ao 'cFE [A-Z]* Initialized' runs/{R*,rep_*}/console.log`
  - 결과: `$N/sem-es-tbl/runs/probe_results_summary.txt:7-40`
- **[해석]** #73의 crash 경로는 현재 코드에서 닫혀 있다. 이 판단은 코드 분석과 24회의 정상 실행에 근거한 추론이다. 결함을 일부러 일으키는 표적 재현 시도는 아니다. 따라서 현재 코드는 #73의 양성 사례가 될 수 없다.

**같은 패턴의 잔여 형태: cFE #2663**

- **[확인된 사실]** `CFE_SB_CreatePipe`는 성공하면 `CFE_EVS_SendEventWithAppID(CFE_SB_PIPE_ADDED_EID, CFE_EVS_EventType_DEBUG, CFE_SB_Global.AppId, ...)`를 보내고, 반환값을 버린다([cfe_sb_api.c#L272-L281](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_api.c#L272-L281)).
- **[확인된 사실]** `CFE_SB_Global`은 EarlyInit에서 0으로 지워진다(`cfe_sb_init.c:50`). AppId는 SB TaskInit에서만 설정된다(`cfe_sb_task.c:128`). 그런데 ES TaskInit(`cfe_es_task.c:367`)과 EVS TaskInit(`cfe_evs_task.c:289`)은 SB task가 생기기 전에 pipe를 만든다.
- **[확인된 사실]** open issue cFE #2663 [S41](2025-08-06)은 이 호출이 ES/EVS 초기화 중에 `CFE_EVS_APP_ILLEGAL_APP_ID`를 반환하지만 아무도 이를 확인하지 않는다고 보고한다.
- **[확인된 사실]** 이 문제를 고치려던 PR #2820 [S42]은 2026-09-08에 merge 없이 닫혔다. 작성자는 "the fix is vacuous"라고 남겼다. 이는 WebFetch 요약으로만 확인했다.
- **[실측]** 고정 native build에 gdb를 붙여 `cfe_evs.c:188`(ILLEGAL_APP_ID 경로)에 breakpoint를 걸었다. 두 번 실행했고, 매번 AppID=0으로 24번 멈추었다. 내역은 EventID 5(PIPE_ADDED) 2회, 10(SUBSCRIPTION_RCVD) 4회, 14(SEND_NO_SUBS) 18회다. 모두 ES와 EVS의 TaskInit 중이었다.
  - 로그: `$N/verify-C12/counter_lens/gdb_attach.log`, `gdb_attach2.log`
  - 명령: `grep '^BP188' gdb_attach.log | sort | uniq -c`
- **[해석]** #73과 같은 "제공 서비스가 초기화되기 전에 그 서비스의 정체(AppId)를 사용하는" 구조가 지금도 남아 있다. 결과는 crash가 아니라 event가 조용히 버려지는 것이다. 현재 버전에서 재현 가능한 후보로 쓸 수 있다. 다만 기능상 결함인지는 요구에 달려 있다.

#### 2.2.5 재현 가능성

| 자료 | 상태 |
| --- | --- |
| 수정 전 source (6.4.1) | **[확인된 사실]** SourceForge에 공개되어 있다. 내려받아 해시를 기록했다. |
| 수정 후 source (6.4.2) | **[확인된 사실]** 공개되어 있다. VDD가 함께 있다. |
| GitHub 이력 | **[확인된 사실]** root commit 26ffd61 다음이 "Add 6.5.0a from Sourceforge release" 792f5e35(2017-06-29)다. 이미 수정된 코드부터 시작한다. |
| Microblaze, GRC EVA build 설정 | **[미확인]** 공개되어 있지 않다. issue의 priority 값은 default/example 설정의 값이다. |
| 원래 환경에서의 재현 | **[미확인]** 시도하지 않았다. |
| Trac 댓글 | **[미확인]** 열람하지 못했다. |

**[확인된 사실]** issue 조사 단계는 "수정 전 공개 코드가 없으므로 #73/#71은 공개 소스로 재현할 수 없다"고 판단했다. 검증 단계에서 SourceForge의 6.4.1과 6.4.2를 확보하면서 이 판단이 정정되었다. source 수준의 수정 전후 비교는 가능하다. 원래 플랫폼에서의 실행 재현은 여전히 불가능하다.

**[설계 제안]** #73은 다음과 같은 "파생 사례" 기록으로 평가 집합에 넣는다. 원노트 §39의 ground truth 항목(Fault ID, Affected applications, Required ordering, Possible violating ordering, Expected analyzer result, Runtime manifestation)을 따른다.

```text
case_id:            H-73-derived            # 원 환경 재현이 아님을 이름에 표시
provenance:         nasa/cFE#73 = Trac #42 (제목·milestone 일치에 의한 대응)
source_pre:         cFE-6.4.1-OSS-release.tar.gz  sha256 ec26e33b...e98f
source_post:        cFE-6.4.2-OSS-release.tar.gz  sha256 c64ed2aa...a4f
affected_apps:      CFE_TIME (사용자), CFE_SB, CFE_EVS (제공자)
required_ordering:  EVS.TaskInit sets EVS_AppID  ->hb  any EVS_SendEvent reached from another core task
                    SB.TaskInit sets CFE_SB.AppId ->hb  SB API use that reports events with it
violating_order:    TIME.TaskMain -> CFE_SB_CreatePipe -> SendEventWithAppID(AppId=0)
                    -> EVS_NotRegistered -> EVS_SendEvent -> EVS_IsFiltered(0xFFFFFFFF)
expected_pre:       경고 1건(사용 위치 time_task.c:308, 근거 위치 evs_utils.c:616/243)
expected_post:      core task 생성 직후 ApplicationSyncDelay가 HB 간선을 만들고
                    caller guard가 있으므로 crash 경로는 경고하지 않음.
                    SB AppId 선사용은 별도 진단(#2663 계열)으로 남을 수 있음
manifestation:      6.4.1: 범위 밖 index (issue 보고상 Microblaze segfault)
label_basis:        issue 본문 + 6.4.1/6.4.2 diff + VDD Trac #40/#42
not_claimed:        Microblaze 실행 재현, 비행 발생, 현재 cFE의 동일 결함
```

```text
case_id:            C-2663
source:             cFE 546a002515be5a1e3b66f9ae2c14f948d9cec76f
use:                CFE_SB_CreatePipe/SubscribeFull/TransmitMsg -> SendEventWithAppID(CFE_SB_Global.AppId)
callers:            CFE_ES_TaskInit (cfe_es_task.c:367), CFE_EVS_TaskInit (cfe_evs_task.c:289)
provider_ready:     CFE_SB_Global.AppId 설정 (cfe_sb_task.c:128)
observed:           ILLEGAL_APP_ID 24회/실행(EID 5:2, 10:4, 14:18), 반환값 미사용
expected:           "provider identity used before provider init" + "status discarded"
severity_basis:     crash 없음. 결함 여부는 event 손실의 허용 여부에 따름
```

---

### 2.3 cFE #71과 같은 시기의 기록

| 기록 | **[확인된 사실]** 내용 | 수정 | **[해석]** 사용 |
| --- | --- | --- | --- |
| cFE #71 [S02] (Trac #40) | "CFE ES 'StartupSyncSemaphore' subject to multiple race conditions". startup sync가 "a binary semaphore, a boolean flag, and a counter"를 따로따로 다루어서 race가 생긴다. GRC EVA 팀이 Xilinx Microblaze에 CFS를 배치하다 겪었다. milestone 6.4.2, 생성 17:50:02Z, 종료 17:50:19Z, 댓글 25개. | VDD Trac #40: `CFE_ES_WaitForStartupSync`를 semaphore 대신 지연을 넣은 polling loop로 바꾸었다. 대기는 보통 50 ms 미만이다 (`vdd642.txt:71-74`). | sync primitive 자체의 결함이다. 앱 사이 순서 결함이 아니다. |
| cFE #72 [S36] (Trac #41로 추정) | "Race condition within CFE_ES_AppCreate". ES가 `OS_TaskCreate` 앞에서 lock을 풀었다가 다시 잡는다. 그 사이 child가 덜 채워진 표를 읽으면 `CFE_ES_GetAppID`가 실패할 수 있다. 본문은 core app 생성과 child task 생성은 lock을 유지하므로 안전하다고 적는다. | VDD Trac #41: 해제-재획득 코드를 없앴다 (`vdd642.txt:76-80`). | 공유 표에 대한 초기화 순서 결함이다. |
| cFE #105 [S37] | "CFE ES unit test failures caused by startup sync fix". #71 수정으로 unit test가 실패했다. "As an interim fix this check will be removed from the main code." label bug, wontfix, milestone 6.4.2 | child 시작 검증을 제거했다. | 수정이 시험 편의 때문에 일부 되돌려진 기록이다. |
| cFE #1034 [S82] | RV 도구가 찾은 문제다. `SystemState`가 공유 변수인데 atomic이 아니었다. | b9fed681 [S83](2020-12-03): `uint32` → `volatile sig_atomic_t` | 공유 메모리 성격의 후속 조치다. |

- **[확인된 사실]** 6.5.0a에는 semaphore가 없다. startup sync는 `SystemState`, `AppReadyCount`, `AppStartedCount`를 polling한다([cfe_es_api.c#L651-L694 @792f5e35](https://github.com/nasa/cFE/blob/792f5e3594c10e21a3560479bb72ea0584eb8ead/cfe/fsw/cfe-core/src/es/cfe_es_api.c#L651-L694)).
- **[해석]** #71, #72, #73, #105는 GRC EVA 팀의 Microblaze 포팅이라는 한 사건에서 나온 기록 묶음이다. 독립된 사례 네 건으로 세지 않는다. 비행 사고로 세지도 않는다. 수정본 §2.3의 판단과 같다.
- **[미확인]** 한 검색 요약은 #71의 race를 IV&V Klocwork 결과로 돌렸다. #71 본문은 발견 경위를 GRC EVA 팀의 포팅으로 적는다. 이 연결은 근거가 없으므로 쓰지 않는다.

---

### 2.4 NASA 저장소에서 수집한 기타 이력

#### 2.4.1 수집 방법과 한계

- **[확인된 사실]** 조사한 저장소는 20개다: cFE, cFS, osal, PSP, sample_app, ci_lab, to_lab, sch_lab, HK, LC, SC, SCH, DS, FM, MD, MM, CS, HS, CF, SBN.
- **[확인된 사실]** 조사 규모는 다음과 같다.
  - issue 검색 페이지 37개(중복 포함 약 379행)
  - 개별 issue/PR 페이지 42개
  - blobless clone으로 얻은 commit 제목 8,198개 → keyword로 478개 선별 → 25개를 전부 읽음
  - 이렇게 얻은 후보는 38건이다(`$N/issues/candidates.json`, `$N/issues/search_log.tsv`).
- **[확인된 사실]** 범주별 후보 수는 다음과 같다.
  - startup service dependency 7
  - first-message/구독 3
  - table lifecycle과 use-before-init 8
  - 공유 table race 1
  - 메시지 유래 상태 2
  - snapshot 2
  - framework SB race 1
  - lifecycle·기타 11
  - 범위 밖 shared-memory 3
- **[미확인]** 검색 페이지 하나에 약 12행만 보였다. 조직 전체 검색은 85건 중 10건만 보였다. 표현이 다른 issue는 놓쳤을 수 있다. 이 후보 집합으로 발생 비율을 계산하지 않는다.
- **[미확인]** issue 본문은 WebFetch 요약으로 읽었다. 정확한 인용은 git에서 직접 읽은 commit message와 코드 주석에만 쓴다.

#### 2.4.2 후보 표

"앱 간"은 서로 다른 앱이나 core service 사이의 관계가 결함에 관여하는지를 뜻한다. 재현 가능성 열은 코드와 issue 본문을 근거로 한 판단이며, 실행해 본 결과가 아니다(#2663만 예외).

**startup과 서비스 의존 (원노트 F5)**

| 기록 | **[확인된 사실]** 내용 | 앱 간 | **[확인된 사실]** 수정 | **[해석]** 재현 가능성 |
| --- | --- | --- | --- | --- |
| cFE #198 [S38] (milestone 6.6.0) | EVA CWS 프로젝트의 앱들이 `CFE_ES_WaitForStartupSync`를 썼는데도, 늦은 단계의 앱 간 초기화를 끝내기 전에 다른 앱의 요청을 받았다. | 예 (사용자 앱 사이) | 6.6.0a(2661d19f)에 APPS_INIT system state, LATE_INIT app state, `CFE_ES_WaitForSystemState`가 추가되었다([cfe_es.h#L645](https://github.com/nasa/cFE/blob/2661d19f45dd325daf068e81a2967cbcc08ccab8/fsw/cfe-core/src/inc/cfe_es.h#L645)). 6.5.0a에는 없다. | 부분 가능. 수정 전(6.5.0a)과 수정 후(6.6.0a) cFE는 공개되어 있다. EVA CWS 앱은 비공개라 합성 앱 2개가 필요하다. |
| CF #184 [S43] (2022-01-13 개설) | CF가 `CF_CFDP_InitEngine`에서 다른 앱(CI/TO 등)이 만드는 throttle counting semaphore에 붙는다. 생성 앱이 먼저 실행된다는 보장이 없어 CF가 중단된다. 소유 앱이 재시작하면 semaphore ID가 바뀔 수 있다는 지적도 있다. | 예 (SB가 아닌 자원) | 833fdbb [S44] / PR #342 [S45](2022-12-01 merge): `OS_ERR_NAME_NOT_FOUND`인 동안 100 ms 간격으로 최대 25회 재시도한다. 코드 주석은 "There is a start up race condition because CFE starts all apps at the same time"이다. | 가능. parent commit이 공개되어 있고, semaphore를 늦게 만드는 합성 앱을 붙이면 된다. 실행해 보지는 않았다. 수정은 대기 시간을 2.5 s로 제한할 뿐이고 소유 앱의 재시작은 다루지 않는다. |
| cFE #1466 [S39] + PR #2273 [S40] | `CFE_ES_WaitForStartupSync`가 상태를 반환하지 않는다. #1466은 2021-04-30부터 open이다. | 모든 앱에 해당하는 API 위험 | PR #2273은 승인(2023-09-29)과 CCB:Ready label(2024-01-25)을 받았지만 2026-07-07에 merge 없이 닫혔다. 546a002에서도 wrapper는 `void`다([cfe_es_api.c#L616-L619](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/es/fsw/src/cfe_es_api.c#L616-L619)). | 코드 속성으로 확인된다. **[실측]** 2.6절 참조. |
| cFE #2663 [S41] | SB AppId가 설정되기 전에 ES/EVS 초기화 중의 SB 호출이 그 AppId로 event를 보낸다. | 예 (core service 사이) | open. PR #2820은 닫혔다. | **[실측]** gdb로 재현했다(2.2.4절). |

**first-message와 구독 (원노트 F1)**

| 기록 | **[확인된 사실]** 내용 | 앱 간 | **[확인된 사실]** 수정 | **[해석]** 재현 가능성과 용도 |
| --- | --- | --- | --- | --- |
| to_lab d3d52da [S46] (2026-04-21, "Fix cFS/cFS#676, startup msg limit errors") | table 기반 telemetry 구독을 `CFE_ES_WaitForStartupSync`(기본 10000 ms) 뒤로 옮겼다. 주석은 "Defer subscribing until the system is operational will avoid possibly seeing MsgLimit errors due to the apps sending many events at start up"이다([to_lab_app.c#L69-L73](https://github.com/nasa/to_lab/blob/d27c6014cb5d2979140500f635876ee87b81995b/fsw/src/to_lab_app.c#L69-L73)). command MID는 그 전에 `TO_LAB_init`에서 구독한다. | 예 (모든 event 발행 앱 → TO) | 의도된 설계다. 처음 포함된 release는 v7.0.1이다. | 두 commit 모두 공개. 정상 대조군으로 쓴다. 발행이 구독보다 먼저라고 경고하면 오경고다. cFS/cFS#676은 관련 없는 PR을 가리킨다. |
| cFE #926 [S47] (+ #918 [S48]) | queue가 가득 차거나 MsgLim에 걸려 전달이 실패해도 SB send는 성공을 반환한다. 'critical' 구독을 제안한다. | framework 의미 | #926은 open이고 #918은 wontfix로 닫혔다. | 코드 속성. 546a002에서 구독자가 없으면 `NoSubscribersCounter`를 늘리고 `CFE_SUCCESS`를 반환한다([cfe_sb_priv.c#L1091-L1097](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_priv.c#L1091-L1097)). |
| cFE #2131 [S49] | startup 때 event가 몰려 event squelch가 걸린다. | framework | 닫힘 | 부분 가능. |

**메시지·table 유래 상태와 snapshot (원노트 F2, F3, F4)**

| 기록 | **[확인된 사실]** 내용 | 앱 간 | **[확인된 사실]** 수정 | **[해석]** 재현 가능성 |
| --- | --- | --- | --- | --- |
| HS #148 [S50] | table이 등록되었지만 로드되지 않은 상태에서 `HS_EnableAppMonCmd` → `HS_AppMonStatusRefresh`가 NULL인 `AMTablePtr`를 역참조한다. issue의 재현 방법은 `hs_amt.tbl`을 지우고 명령을 보내는 것이다. 결과는 segfault와 processor reset이다. | 아니오 (명령 도착과 table 가용성) | b7530d9 [S51](2026-07-28) | 가능. parent commit과 issue의 재현 방법이 있다. |
| MD #79 [S52] | table을 CDS가 아닌 파일에서 로드하면 `MD_AppData.MD_DwellTables[]`로 복사하지 않는다. 그래서 dwell 처리가 지연 0으로 동작한다. | 아니오 | open. HEAD 65eb7b3의 [md_app.c#L423-L449](https://github.com/nasa/MD/blob/65eb7b3b0aa8acd05076128a623cd696582b6d7c/fsw/src/md_app.c#L423-L449)에서 코드로 확인했다(CDS 경로 347-366행과 갱신 경로 499-505행은 복사한다). | HEAD에서 바로 쓸 수 있다. timing과 무관하게 항상 발생하는 결함이다. |
| LC #8 [S53] | STALE에서 FALSE로 바뀔 때 `TtoFValue`가 설정되는 문제. 내부 tracker GSFCCFS-1075에서 이식되었다. | 아니오 (메시지 유래 watchpoint 상태) | open | **[미확인]** 코드를 보지 않았다. |
| cFE #2739 [S54] | 인증되지 않은 TIME tone/data 메시지가 `CFE_TIME_ToneDataCmd` 등을 거쳐 MET, STCF, leap 상태를 바꾼다. | 예 (발행 앱 → TIME 상태 → 모든 소비자) | — | **[미확인]** 코드를 보지 않았다. 순서 결함이 아니라 출처(provenance) 문제다. |
| cFE #46 [S55] / #1544 [S56] | "SMP: CFE_TIME_GetReference() has insufficient protection against update while reading". 6.5.0a는 7개 필드를 non-volatile `VersionCount`를 두 번 읽는 사이에 읽는다([cfe_time_utils.c#L712-L736 @792f5e35](https://github.com/nasa/cFE/blob/792f5e3594c10e21a3560479bb72ea0584eb8ead/cfe/fsw/cfe-core/src/time/cfe_time_utils.c#L712-L736)). | 예 (TIME이 쓰고 모든 앱이 읽음) | 6.6.0a에서 volatile counter 도입. 69b0940 [S84](2021-06-01)에서 `ReferenceState[]` ring과 RetryCount=4 도입 | 수정 전, 수정 후, 현재 코드를 정적으로 비교할 수 있다. 실행 중 발현은 SMP가 필요하고 시연하지 않았다. 공유 메모리 성격이다. |
| HK #4 [S57] | 누락된 packet을 세도록 요청한다. GSFCCFS-991에서 이식되었다. | 예 (여러 앱의 packet을 조합) | — | 앱 간 snapshot 불일치를 보고한 사례는 아니다. |

**Table lifecycle (원노트 F6)**

| 기록 | **[확인된 사실]** 내용 | 앱 간 | **[확인된 사실]** 수정 | **[해석]** 재현 가능성 |
| --- | --- | --- | --- | --- |
| sample_app #101 [S58] (#28 수정에서 생긴 회귀) | load 뒤 첫 `GetAddress`는 `CFE_TBL_INFO_UPDATED`를 반환한다. 코드가 이를 실패로 보고 주소를 해제하지 않았다. 이 결함은 #28 수정 693d75f [S59](2019-12-06)에서 생겼다. | 아니오 | 61f657d [S60](2021-01-06): `!= CFE_SUCCESS`를 `< CFE_SUCCESS`로 바꿈 | 가능. 두 commit이 공개되어 있다. 해제 누락의 영향을 보려면 v6.7 시기의 cFE와 짝지어야 한다. |
| sch_lab #24 [S61] | init에서 `INFO_UPDATED`만 받아들였다. | 아니오 | c6342bd [S62] | 가능 |
| DS #146 [S63] | `GetAddress`의 반환 상태를 무시했다. issue 본문은 비어 있다. | 아니오 | c609e35 [S64] | commit은 공개. 영향은 모른다. |
| cFE #1750 [S65] | 잠긴 table에 Load하면 `INFO_TABLE_LOCKED`를 반환하지만 `LoadInProgress`가 남는다. | owner와 주소 보유자 (공유되면 앱 간) | open | 기존 functional test `TestReleaseAddress`가 이 동작을 보여 준다. 다시 실행하지는 않았다. |
| cFE #1509 [S66] | 공유 table의 GetInfo/Modified와 share/unshare/unregister 사이의 race를 코드 검사로만 지적했다. | 예 (공유 table) | open | **[미확인]** TBL이 refactor된 뒤인 546a002에 여전히 있는지 확인하지 않았다. |
| sample_app #255 [S67] | `Manage` 전에 `ReleaseAddress`가 빠졌다고 주장한다. | 아니오 | open | **[확인된 사실]** dev 199476a에서는 전제가 확인되지 않는다. `ProcessCmd`는 성공 경로에서 해제하고, `SendHkCmd`는 주소를 들고 있지 않다. |

**lifecycle (원노트 범주 밖, 원노트의 lifecycle 관심에 해당)**

| 기록 | **[확인된 사실]** 내용 | 앱 간 | **[확인된 사실]** 수정 | **[해석]** 재현 가능성 |
| --- | --- | --- | --- | --- |
| cFE #950 [S68] | 앱이 스스로 종료하는 것과 ES background cleanup이 경쟁해 segfault가 난다. 수정한 sample_app으로 안정적으로 재현되었다. | framework와 앱 lifecycle | 6932f1f [S69]: 두 단계 cleanup | 가능. parent commit과 issue의 재현 방법이 있다. |
| cFE #591 [S70] | FreeRTOS 10 / cFE 6.7.0(Capella)에서 `CFE_ES_CreateObjects`가 deadlock에 빠진다. | framework startup (lock) | 1b03958 [S71] | 부분 가능 |
| cFE #701 [S72], osal 7ba42a6 [S73](osal #642) | 종료 시 SCH timer callback과 timebase lock 사이에 deadlock이 생긴다(osal#472로 수정). POSIX `OS_TaskDelete`가 지연 처리되다가 동기 처리로 바뀌었다. | 구성요소 사이 | 수정됨 | 부분 가능 |
| cFE #2433 [S74], #2107 [S75], #1383 [S76], #2140 [S77], SBN #79 [S78] | 각각 다음 문제다. SB mutex를 쥔 채 취소된 task 때문에 종료가 멈춘다. mutex를 쥔 앱을 멈추면 cleanup이 RC=-6으로 실패한다. init에 실패하는 앱이 끝없이 재시작한다. 정지한 앱이 kill timeout을 끝까지 기다린다. 부분 init 뒤 cleanup이 pipe ID 0을 지운다. | 대부분 framework lifecycle | 모두 open | **[미확인]** 실행하지 않았다. |
| cFE #1073 [S79] | native에서 몇 시간 실행한 뒤 EVS, SB, TIME이 구독하지 않은 MID를 받았다. EVS가 0x1810을 `cfe_evs_task.c:378`에서 받은 예가 있다. | 증상은 앱 간이고 원인은 SB 내부 공유 메모리다. | b17cd1e [S80](2021-01-12, PR #1092): pipe 표와 routing을 SB global lock 아래로 옮김 | 부분 가능. 발현까지 몇 시간이 걸린다. 대조용 사례다. |

**범위 밖 (shared-memory data race)**

| 기록 | **[확인된 사실]** 내용 | **[해석]** 용도 |
| --- | --- | --- |
| cFE #2523 (dd596c96 [S81], 2025) | EVS counter 증가에 mutex를 추가했다. | TSan 같은 기준선과의 대조용 |
| cFE #2427 (25ccc0ce, 2023) | TestCreateChild의 race. test 코드다. | 제외 |
| cFE #456 (merge 04f1fddb) | perflog race | 제외 |

#### 2.4.3 원노트 결함 범주별 공개 근거

| 원노트 범주 | 공개 양성 사례 (결함) | 정상 또는 의도된 대조 사례 | **[해석]** 평가에 주는 의미 |
| --- | --- | --- | --- |
| F1 구독 전 발행 | **[확인된 사실]** 검색 범위 안에서는 발행이 구독보다 먼저여서 기능 실패가 났다는 공개 보고를 찾지 못했다. | to_lab d3d52da의 의도된 지연 구독. #926의 조용한 drop 의미. 2.6절의 **[실측]** | 양성 사례는 합성하거나 주입해야 한다. d3d52da는 음성 대조군이다. |
| F2 초기화 전 사용 | #73(서비스 초기화 전 사용, 역사적). HS #148, MD #79(table 유래, timing과 무관) | — | 메시지 유래 상태에 대한 공개 사례가 없다. table 로드를 상태의 출처로 함께 다루어야 한다. |
| F3 오래된 상태 | LC #8(코드 미검토) | HK의 DataPresent flag와 `HK_DISCARD_INCOMPLETE_COMBO`(기본값 0) 정책([hk_internal_cfg.h#L57-L69](https://github.com/nasa/HK/blob/0dc16b7a7bab71747b9c063cf405ad59a65a40a5/fsw/inc/hk_internal_cfg.h#L57-L69)) | 결함과 의도된 latest-value 정책을 구분해야 한다. |
| F4 여러 stream의 snapshot 불일치 | **[확인된 사실]** 앱 사이 사례는 찾지 못했다. 앱 내부·framework 사례로 #46/#1544(TIME reference 다중 필드, 공유 메모리 성격)가 있다. | TIME의 versioned read(현재 코드) | 앱 간 양성 사례는 합성해야 한다. TIME 소비자에게 snapshot 경고를 내면 오경고가 되지 않도록 모델이 필요하다. |
| F5 startup 서비스 의존 | #73(core, 역사적), #198, CF #184, #2663(현재, 조용함), #1466(API 위험) | 현재 core 생성 직렬화 | 가장 풍부한 범주다. 원 소스 쌍이 있는 역사적 사례가 있다. |
| F6 table lifecycle 오용 | sample_app #101, sch_lab #24, DS #146, #1750, #1509(검사만) | sample_app #255(전제 미확인) | 대부분 한 앱의 API protocol 결함이다. 앱 간 race로 집계하지 않는다. |

---

### 2.5 NASA IV&V 보고서와 정적 분석의 적용 맥락

**보고서 자체**

- **[확인된 사실]** 수정본은 다음 보고서를 S07로 인용한다. Bradbury, J. W.(2020), NASA/CR-20205010026, "Open Source Core Flight System (cFS) Flight Software (FSW) Verification & Validation (V&V) Final Summary"(NTRS record [S07]). 수정본은 §2.1 정적 분석(인쇄 p. 19)과 §2.2 설계 분석(인쇄 p. 23)을 확인했다고 적는다. 그 내용은 cFE 6.5와 관련 앱에 대한 활동이라고 기록한다 [S07].
- **[미확인]** 이번 조사에서는 NTRS가 차단되어 PDF를 다시 열지 못했다. 아래는 검색 snippet의 내용이다.
  - 2020년 12월, J. W. Bradbury(Engility/TASC), GSFC 대상, NASA IV&V SARP 승인
  - 목표 중 하나는 "completion of Klocwork static code analysis (SCA) as a gauge of likely code maturity and dependability including delivery of higher impact SCA issues to the cFS development team"이다.
  - "With the exception of string processing, the cFS software is generally free of the kinds of potential bugs that SCA readily identifies."
  - "Static code analysis results were observed to be among the most readily transferred verification and validation evidence."
  - cFE 6.5.0을 참조한다.
- **[미확인]** 보고서가 startup이나 순서 관련 결함을 보고했는지는 모른다. snippet에는 그런 내용이 없다. 이것을 "그런 결함이 없다"는 근거로 쓰지 않는다.
- **[해석]** 이 보고서는 IV&V가 범용 상용 C 분석기(Klocwork)를 성숙도 지표로 썼다는 맥락을 준다. 이 연구의 정량 기준선이나 결함 발생률 자료는 아니다. 수정본 §2.3의 판단과 같다.

**공개 CI의 분석기**

- **[확인된 사실]** cFE·cFS 공개 CI는 cppcheck와 CodeQL(security-and-quality, security-extended, JPL/MISRA subset)을 쓴다. JPL subset은 rule 14 "checking-return-values"와 rule 15 "checking-parameter-values"를 뺀다. 소스에는 CodeSonar 억제 주석이 있다. 설정 파일 permalink와 commit message 집계(CodeSonar 4, CodeQL 13, cppcheck 11, "static analysis" 17, Coverity·Polyspace·Klocwork·IKOS·Frama-C 0)는 §4.1.1에 있다.
- **[해석]** cFS에 실제로 적용되어 온 분석기는 lint와 CWE/JPL/MISRA 패턴을 보는 범용 C 검사기다. 공개 CI에서는 반환값 검사 규칙이 꺼져 있다. #2663처럼 버려진 `CFE_EVS_SendEventWithAppID` 반환값이 공개 CI에서 지적되지 않은 것과 맞아떨어진다. 다만 그것이 원인이라는 증거는 아니다.

---

### 2.6 현재 bundle에서 관측한 관련 현상

**[실측]** 아래는 이번 probe에서 고정 build를 실행해 얻은 관측이다. 모두 결함으로 판정한 것이 아니다. 해당 framework 의미는 SB, ES, TBL을 다루는 절에서 자세히 다룬다.

| 현상 | 측정 | 근거 | **[해석]** 의미 |
| --- | --- | --- | --- |
| startup 중 구독자 없는 발행 | cFS bundle 5a9b075(cFE 546a002) native 실행 8회 모두에서, OPERATIONAL 전에 `CFE_SB 14: No subscribers for MsgId 0x808`(`CFE_EVS_LONG_EVENT_MSG_MID`)가 4줄씩 나왔다. 메시지를 낸 것은 CFE_SB이고, sender는 CFE_SB, CFE_TBL, CFE_TIME(이 셋은 CORE_READY 전), TO_LAB이다. | `$N/probe/run_nobody.log:63,65,67,93`(CORE_READY 69행, OPERATIONAL 137행). `grep -c 'No subscribers for MsgId 0x808'` 결과 8개 로그 모두 4 | 4줄은 `CFE_EVS_FIRST_4_STOP` filter의 상한이다. 실제 발행 건수는 모른다. 앞의 세 건은 TO_LAB의 지연 구독과 무관하다. 정상 대조군이다. 상세 분석은 §7.4.3 |
| 구독 보고 drop | 매 실행에서 TO_LAB이 보낸 0x80E(`CFE_SB_ONESUB_TLM_MID`) 16건이 SBNSubPipe에서 버려졌다. non-root 구성에서는 Pipe Overflow(depth 10으로 잘림), root+IPC namespace 구성에서는 Msg Limit Err(MsgLim 16)로 나타났다. | `$N/probe/run_nobody.log:208-223` | 16도 filter 상한이다. SBN 기능에 영향이 있는지는 확인하지 않았다. 결함으로 보고하지 않는다. |
| OPERATIONAL과 앱 준비 상태의 불일치 | 2.5 s 늦게 시작하는 probe 앱이 RUNNING이 아닌 상태에서 ES가 "Startup Sync failed"를 두 번 기록하고 OPERATIONAL에 들어갔다. 다른 앱의 `WaitForSystemState(OPERATIONAL,5000)`은 1.95–2.01 s 뒤 성공을 반환했다. 24회 중 24회 같았다. | `$N/sem-es-tbl/runs/R1_rr_allcpu/console.log:105-121` | #1466이 지적한 API 위험이 실제 실행에서 그대로 나타난다. 의미 규칙은 §3.3 ES-4 |
| 소유 앱 재시작과 table 공유 | 다른 앱이 table을 share한 상태에서 소유 앱을 재시작했다. 새 인스턴스의 `CFE_TBL_Register`는 sharer가 Unregister할 때까지 `CFE_TBL_ERR_DUPLICATE_NOT_OWNED`(0xcc00000d)를 반환했다. cleanup 뒤 sharer의 `GetAddress`는 `CFE_TBL_ERR_UNREGISTERED`(0xcc000009)와 NULL을 반환했다. 4개 구성에서 각 1회 실행했다. | `$N/sem-es-tbl/runs/R1_rr_allcpu/console.log:144-157` | 버전에 따라 다른 후보다. 이 동작은 v7.0.0의 c1ab1b7(2026-01-22) 이후 코드에서 나온다. 이전 tag는 table 이름을 지웠다(실행 없이 코드에서 추론). 공식 FAQ(`cfe_tbl.dox` L333-339)는 이전 동작을 설명한다. 의미 규칙은 §3.4 TBL-7 |
| table 문서와 구현의 불일치 | 고정 commit의 header(cfe_tbl.h L540-544)는 `CFE_TBL_ERR_NEVER_LOADED`일 때도 0으로 채운 유효 포인터를 준다고 적는다. 구현은 `*TblPtr = NULL`을 주고 lock을 걸지 않는다([cfe_tbl_registry.c#L192-L196](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/tbl/fsw/src/cfe_tbl_registry.c#L192-L196)). v6.7.0의 구현은 내부 buffer를 쓰는 table에 한해 header와 일치했다. | probe 출력 `GetAddress(T) BEFORE load st=0xcc000005 ptr=(nil)` (`$N/sem-es-tbl/runs/probe_results_summary.txt:50-51`) | 수정본 §7.1이 header 문구를 확인된 사실로 적은 부분을 고쳐야 한다. API 모델은 버전마다 구현으로 확인해야 한다. 의미 규칙은 §3.4 TBL-1 |

---

### 2.7 이 사례들이 지지하는 것과 지지하지 못하는 것

#### 2.7.1 주장별 판정

| 주장 | 판정 | 근거 |
| --- | --- | --- |
| cFE core service 사이의 startup 순서 결함이 실제 포팅 중에 발생했고 수정되었다. | **[확인된 사실]** 지지된다. | #73, VDD 6.4.2 Trac #42, 6.4.1/6.4.2 diff |
| 그 결함은 비행 중에 발생했다. | **[미확인]** 근거가 없다. | 기록은 Microblaze 포팅 중의 발견이다. |
| 같은 결함이 현재 cFE에 있다. | **[해석]** 근거가 없다. crash 경로는 닫혔다. | 546a002 코드, 24회 실행 |
| #73과 같은 구조의 잔여 형태가 현재 cFE에 있다. | **[실측]** 지지된다. 다만 조용한 event 손실이며 crash가 아니다. | #2663, gdb 24회/실행 |
| #73은 원래 환경에서 재현할 수 있다. | **[미확인]** 근거가 없다. 수정 전후 source 비교만 가능하다. | 2.2.5절 |
| 사용자 앱 사이의 startup 의존이 공개 코드에 있다. | **[확인된 사실]** 지지된다. | #198, CF #184 |
| startup sync 호출이 다른 앱의 준비를 보장한다. | **[실측]** 반박된다. | #1466, 24/24 실행 |
| 구독 전 발행이 공개 cFS에서 기능 실패를 일으켰다. | **[미확인]** 공개 사례를 찾지 못했다. | 2.4.3절. 의도된 지연 구독만 있다. |
| 앱 사이 여러 stream의 snapshot 불일치 결함이 공개 cFS에 보고되었다. | **[미확인]** 찾지 못했다. | TIME 내부 사례만 있다. |
| 비행 소프트웨어 임무팀이 onboard race를 반복 보고한다. | **[확인된 사실]** 지지된다. 문구는 snippet 수준이다. | S03–S06, 추가 Circular는 미확인 |
| Swift race의 원인은 epoch이 다른 snapshot의 혼합이다. | **[미확인]** 근거가 없다. | 22706은 "incorrect attitude"만 말한다. |
| Swift/BAT는 cFS 사례다. | **[해석]** 근거가 없다. cFE 이전의 heritage 시스템이다. | 2.1.5절(판정에 이견이 있었음) |
| 제안한 분석기가 Swift나 #73을 검출한다. | **[미확인]** 구현이나 실행 결과가 없다. | — |
| 이 결함 종류의 발생률은 얼마다. | **[해석]** 산출할 수 없다. | 검색이 불완전하고 표본이 체계적이지 않다. |
| IV&V 정적 분석이 순서 결함을 찾지 못했다. | **[미확인]** 판단할 수 없다. | 보고서 본문을 다시 열지 못했다. |
| 이 연구 주제는 새롭다. | **[해석]** 이 절의 사례로는 판단할 수 없다. | 새로움은 관련 연구 절에서 판단한다. |

#### 2.7.2 평가 데이터 구성

**[원노트 구상]** 원노트 §37은 평가 자료를 세 층으로 구상한다. 최소 합성 benchmark, 공개 cFS 앱 변형, 실제 cFE historical bug 재현이다.

**[설계 제안]** 이 절의 사례를 근거에 따라 다음과 같이 나누어 집계한다. 층이 다른 결과는 합산하지 않는다.

| 층 | 사례 | label 근거 | 주의 |
| --- | --- | --- | --- |
| H: 역사적 양성(수정 전후 source 쌍) | H-73-derived(6.4.1 ↔ 6.4.2), H-198-derived(6.5.0a ↔ 6.6.0a와 합성 앱 2개), H-CF184(833fdbb의 parent와 합성 semaphore 생성 앱) | issue 본문과 수정 diff | 원 환경 재현이 아니다. "derived"로 표기한다. |
| C: 현재 버전 후보 | C-2663, #1466 API 위험, 소유 앱 재시작 시 DUPLICATE_NOT_OWNED | 고정 코드와 **[실측]** | crash가 아니다. 결함인지는 요구에 달려 있다. 요구가 없으면 label을 보류한다. |
| A: 한 앱의 API protocol | sample_app #101(693d75f ↔ 61f657d), HS #148, MD #79, DS #146, sch_lab #24 | 수정 commit | 앱 간 순서 결함으로 집계하지 않는다. |
| N: 정상·의도 대조군 | to_lab d3d52da, HK DataPresent 정책, 현재 TIME의 versioned read, 2.6절의 0x808 구독자 없는 발행 | source 주석과 설계 | 경고하면 오경고(FP)로 센다. |
| S: 합성 양성 | F1(필수 일회성 메시지를 구독 전에 발행), F4(앱 간 epoch 불일치) | 주입과 실행 확인 | 공개 양성 사례가 없는 범주다. 비행 결함 검출 성능으로 일반화하지 않는다. |
| X: 범위 밖 대조 | #1073(SB 내부 공유 메모리), #2523, #46/#1544(공유 메모리 성격) | 수정 commit | ThreadSanitizer 같은 기준선과 겹치는 범위를 보여 주는 데만 쓴다. |

#### 2.7.3 요약

- **[해석]** 공개 근거가 가장 강한 것은 F5(startup 서비스 의존)다. #73은 수정 전후의 공개 source와 VDD를 갖춘 역사적 사례다. 현재 버전에도 그 잔여 형태(#2663)와 API 수준의 위험(#1466)이 실측으로 확인된다.
- **[해석]** F1과 F4는 이 연구의 핵심 관심사지만 공개 cFS 양성 사례가 없다. 이 두 범주의 평가는 합성 사례와 정상 대조군에 기대야 한다.
- **[해석]** Swift/BAT는 "비행 소프트웨어 임무팀이 onboard race를 원인으로 반복 보고한다"는 동기만 제공한다. cFS가 아니고, 내부 구조와 원인이 공개되지 않았다. 따라서 탐지 대상, benchmark, 결함 모형의 근거로 쓰지 않는다.
