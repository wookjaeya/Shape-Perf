## 4. 선행연구 지형과 연구 위치

이 절은 세 가지를 한다.

1. 선행연구를 다섯 무리로 나누어 비교한다. 4.1 flight software 검증, 4.2 publish/subscribe 구조 복원, 4.3 순서·race 이론과 도구, 4.4 cause-effect chain과 data age, 4.5 MLIR·ClangIR·대안 분석 기반이다.
2. 이 연구의 구성 요소마다 선행 범위를 정리하고(4.6), 신규성 경계를 정한다(4.7).
3. 예상되는 심사 반론과 답변을 정리한다(4.8). 마지막 4.9절은 원노트·수정본·조사 결과에서 바로잡은 서술을 모은다.

**표기.** 결함 범주 F1–F8과 실행 가정 Σ는 1.2–1.3절, cFE 의미 항목은 3절을 가리킨다. 비교표의 열은 다음 뜻이다.

| 열 | 뜻 |
| --- | --- |
| 확인 수준 | '전문'은 본문 전체나 해당 절을 읽었다는 뜻이다. '코드·문서'는 고정 commit의 파일을 읽었다는 뜻이다. '초록·snippet'은 검색 색인의 요약만 보았다는 뜻이다. 초록·snippet 항목은 **[미확인]**으로 표시하고 결론의 근거로 쓰지 않는다 |
| 입력 | 방법·도구가 받는 것. 소스, 설계 모델, 실행 trace, timing 파라미터 등 |
| 속성 | 검사하거나 계산하는 성질 |
| 정적/동적 | 실행 없이 판정하는가, 관측한 실행으로 판정하는가 |
| 보장 | 저자가 밝힌 보장의 범위. 밝히지 않았으면 '명시 없음' |
| 겹침 | 이 연구와 같은 입력·속성 |
| 간극 | **[해석]** 이 연구가 다루려는 것 가운데 그 작업이 다루지 않는 것 |

---

### 4.0 조사 범위와 한계

- **[확인된 사실]** 조사는 네 갈래로 나누어 했다. flight software 검증(`$N/rw-flight`), publish/subscribe 구조 복원(`$N/rw-pubsub`), 순서·race 이론과 도구(`$N/rw-races`), MLIR·대안 도구(`$N/rw-mlir`)다. 핵심 주장 22개는 검증자 3명이 따로 다시 확인했다. 검증에서 기각되거나 이견이 나온 주장은 이 절에서 정정된 문구로만 쓴다 (`$N/verify*`).
- **[확인된 사실]** egress proxy가 학술 host 대부분을 막았다. ieeexplore.ieee.org, dl.acm.org, link.springer.com, arxiv.org, ntrs.nasa.gov, drops.dagstuhl.de, Fraunhofer CESE, havelund.com, people.cs.uchicago.edu, people.kth.se 등이다. Scholar Gateway는 "Could not resolve user identity from CONNECT"로 실패했다. GitHub, raw.githubusercontent.com, microsoft.com은 열렸다. 기록은 `$N/rw-flight/access_log.txt`, `$N/rw-races/sources_manifest.tsv`, `$N/rw-pubsub/sources_log.md`에 있다.
- **[확인된 사실]** 전문을 읽은 논문은 다음과 같다. ROSDiscover(평가 저장소 사본), ROSInfer(저자 PDF), HAROS IRC'19 [R45]·IROS'20 [R46]·RoSE'21 [R47], TaxDC, Burrows–Leino 원고, PCT, P 언어 TR. R01·R05·R06·R07·R09·R10의 확인 범위는 수정본 §14의 기록을 따른다.
- **[미확인]** 초록·snippet만 본 자료는 다음과 같다. Ganesan 등 ISSRE 2016 [R03]·SPLC 2009·WCRE 2010, Valente 등 TECS 2025 [R04], TASTE model checking 논문들, IV&V 2020(S07, 이번 조사 기준), IKOS–BioSentinel slide, Havelund 등 TSE 2001, DCatch, DroidRacer·CAFA·SIERRA·nAdroid, InitRacer·WebRacer, Artho 등 HLDR, Flanagan–Qadeer, AVIO, Netzer–Miller, Casini·Becker·Teper의 본문, Garcia 등 ICSE 2020, ROSpec·ROSCallBaX 본문, Fehr 등 PLDI 2025, Peng 등 POPL 2026, RacerF, CodeQL ISSTA 2025 보고.
- **[해석]** 이 조사는 체계적 문헌고찰이 아니다. 검색 건수나 제외 건수를 제시하지 않는다. 아래의 '찾지 못했다'는 부재의 증거가 아니라 이 검색의 결과다.

---

### 4.1 Flight software 검증

#### 4.1.1 cFS를 대상으로 한 연구와 도구

| 연구·도구 | 확인 수준 | 입력 | 속성 | 정적/동적 | 보장 | 겹침 | 간극 **[해석]** |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Ganesan, Lindvall, Hafsteinsson, Cleaveland, Strege, Moleski, ISSRE 2016, pp. 445–454 (DOI 10.1109/ISSRE.2016.47 [R03]) | **[미확인]** 이번 조사는 초록 snippet. 수정본 §14는 기관 서지·공식 초록을 확인했다고 기록한다. 본문 미확보 | 요구사항에서 손으로 만든 Microsoft Spec Explorer 모델. 이 모델로 SB module API에 대한 시험을 자동 생성한다 | 동시 publish/subscribe 사용에서 SB API가 요구사항과 맞는가 | 동적 (모델 기반 시험) | 생성된 시험과 모델의 범위 | cFS SB의 동시성 | 대상은 SB 서비스 자체의 요구사항 적합성이다. 앱 state나 앱 간 순서를 다루는지는 본문을 읽기 전 판단하지 않는다. 별도 slide의 snippet은 메시지를 정수, pipe를 정수 집합으로 모델링했다고 쓴다. 그 slide는 저자 목록이 달라(Valdimarsdottir 포함) 논문 모델과 동일시하는 것은 추론이다 |
| Ganesan, Lindvall, Ackermann, McComas, Bartholomew, SPLC 2009, pp. 161–170 (DOI 10.1145/1753235.1753258 [R16]; NTRS 20090016208) | **[미확인]** 초록 snippet | CFS 구현 소스와 developer's guide에서 뽑은 아키텍처 규칙. 도구는 Fraunhofer SAVE다. SAVE와 CFS의 연결은 별도 FSW-08 발표(NTRS 20090004613 [S113], Ganesan·Lindvall·McComas)의 snippet에서 온다 | 규칙 적합성. snippet의 SB 규칙: application layer module은 compile time에 서로 직접 의존하지 않고, 상호작용은 SB 서비스로 한다. 초록은 code review가 놓친 아키텍처상 중요한 일탈을 찾았다고 쓴다 | 정적 | 명시 없음 | 소스에서 cFS 구조를 복원해 규칙과 대조 | snippet의 규칙은 compile-time 의존 규칙이다. MID·pipe 간선 복원이나 순서 검사인지는 미확인이다. "cFS 구조를 소스에서 처음 복원했다"는 주장은 성립하지 않는다 |
| Valente 등, TECS 24(3) Art. 51, 2025 (DOI 10.1145/3706587 [R04]). UPM record oa.upm.es/74294 [R04] (2023-03-13) | **[미확인]** 이번 조사는 초록 snippet. 수정본은 출판사 색인의 본문 일부를 확인했다고 기록 | TASTE 설계 모델 | 모델과 자동 code generation으로 설계–구현 일치 유지. UPMSat-2 사례 | 정적 (생성) | 생성된 부분에 한해 구성상 일치 (**[해석]**) | cFS 앱 구조의 모델 표현 | 손으로 쓴 앱 C 코드를 분석하지 않는다. 한 검색 요약의 "model checking 가능" 문구는 아래 TASTE 논문의 것이다. Valente에 귀속하지 않는다. 2023 record는 같은 작업의 초기 기탁일 가능성이 높아 독립 근거로 세지 않는다 |
| TASTE model checking: arXiv 2111.10132 [R18], MODELS-C 2022 (DOI 10.1145/3550356.3561541 [R19]), ISSE 2025 (DOI 10.1007/s11334-025-00607-3 [R20]) | **[미확인]** snippet | TASTE AADL/SDL 설계 모델 | Boolean stop condition, Message Sequence Chart(함수 간 I/O 사건의 원하는·원하지 않는 순서), SDL observer | 정적 (IF model checker) | 유한 모델에서 망라 | 함수 간 메시지 순서 속성 | 설계 모델이 있어야 한다. cFS C 소스만 있는 경우는 다루지 않는다 |
| Perez 등, TACAS 2022 (DOI 10.1007/978-3-030-99524-9_21 [R09])와 Ogma cFS template (copilot_cfs.c @08b384b4 [S93]; S24) | 템플릿·생성기 코드 전문 **[확인된 사실]**. 논문은 수정본 §14의 §3–5 기록. 아래 상세는 검증에서 이견(contested)이 있어 정정 문구를 쓴다 | FRET 요구, MID·변수 DB | Copilot이 생성한 시간 논리 monitor | 동적 | 관측된 실행만 | 수신 메시지 값을 전역에 복사한 뒤 monitor를 평가하는 구조 | 관측되지 않은 경로를 판정하지 않는다. 템플릿에는 startup sync 호출이 없다. 상세는 표 아래 |
| Bradbury, NASA/CR-20205010026, 2020 (NTRS [S07]) | **[미확인]** 이번 조사는 snippet(NTRS 차단). 수정본은 §2.1(인쇄 p.19)·§2.2(p.23)를 읽었다고 기록 | cFE 6.5.0 시기의 cFS 코드 | Klocwork SCA로 코드 성숙도를 판단하고, 영향이 큰 SCA 결과를 cFS 팀에 전달 | 정적 (일반 C 결함) | 명시 없음 | cFS 정적 분석 | 순서·startup 결과를 보고했는지 미확인이다. 기준선으로 쓰지 않는다. 한 검색 요약은 cFE #71을 Klocwork 결과로 돌렸지만 근거가 없다. #71 본문은 GRC EVA 팀의 Microblaze 이식 중 발견이라고 쓴다 |
| IKOS ([analyzer/README.md @ac7f7c17](https://github.com/NASA-SW-VnV/ikos/blob/ac7f7c1738976cabc58c6a53413df6e458995c38/analyzer/README.md#L237-L257)). BioSentinel 적용 slide (NTRS 20190032510 [S112]) | 도구 문서 **[확인된 사실]**. BioSentinel 수치는 **[미확인]** snippet | LLVM bitcode, entry point, 사용자가 쓴 stub | 내장 checker 17개. 대부분 runtime error다: buffer overflow, division by zero, null dereference, 미초기화 읽기, signed·unsigned overflow, shift count, pointer overflow·비교, unaligned pointer, 함수 포인터 type, double free·use-after-free. 그 밖에 dead code, soundness 검사, `__ikos_assert` prover가 있다. 순서·동시성 checker는 없다 | 정적 (abstract interpretation) | 가정 아래 sound. 가정: 단일 thread([L535](https://github.com/NASA-SW-VnV/ikos/blob/ac7f7c1738976cabc58c6a53413df6e458995c38/analyzer/README.md#L528-L546)), signal·interrupt 없음, 구현이 없는 extern 함수는 전역을 바꾸지 않음(L538). 여러 entry point는 "as if they were running in different processes" 독립 분석(L341) | cFS 앱에 framework model을 붙여 정적 분석한 선례. snippet: 앱마다 약 1200 LOC CFE model, 평균 경고율 약 1.31%, 실제 결함 약 17개 | stub(`__ikos_forget_mem`, `__ikos_nondet_*`; README L586-607)으로 SB 수신 내용은 havoc할 수 있다. task 간 순서는 모델 밖이다. [TROUBLESHOOTING.md L94-L96](https://github.com/NASA-SW-VnV/ikos/blob/ac7f7c1738976cabc58c6a53413df6e458995c38/TROUBLESHOOTING.md#L94-L96)은 "IKOS does not handle analyzing multi-threaded code"라고 쓴다. C 전역은 0으로 초기화되므로 미초기화 읽기 검사는 '유효 입력 수신 전 사용'(F2)과 다르다. 1200 LOC 수치는 미확인이므로 모델링 비용 추정에 쓰지 않는다 |
| cFS·cFE 공개 CI ([static-analysis.yml L45-L52 @546a002](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/.github/workflows/static-analysis.yml#L45-L52), [jpl-misra.qls L1-L25 @5a9b075](https://github.com/nasa/cFS/blob/5a9b075cd4c818ee8555f349a9f78ba5632d0384/.github/codeql/jpl-misra.qls#L1-L25)) | **[확인된 사실]** 설정 파일 | cFE·cFS 소스, `compile_commands.json` | cppcheck(core module은 strict), CodeQL security-and-quality·security-extended·JPL/MISRA subset. JPL subset은 rule 14 `checking-return-values`와 rule 15 `checking-parameter-values`를 뺀다. CodeSonar는 소스의 suppression 주석으로 확인된다 ([cfe_sb_api.c L436](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/fsw/src/cfe_sb_api.c#L436)) | 정적 pattern | 명시 없음 | cFS에 실제로 쓰는 정적 분석 | 일반 C 결함 pattern이다. 반환값 검사 규칙이 빠져 있어 `CFE_SB_Subscribe`·`CFE_SB_ReceiveBuffer`·`CFE_ES_WaitForSystemState`의 상태 무시는 공개 CI에서 강제되지 않는다 |
| cFS 보안 분석 2022–2026: Schalk 등 MILCOM 2022 [R60], Falco–Thummala "WannaFly" SMC-IT 2023 [R61], Jero 등 SpaceSec 2024 [R62], Furgala 등 SpaceSec 2026 [R63], Vanlyssel 등 arXiv 2608.14532 [R64], Idan 등 arXiv 2609.15425 [R65], Khor–Lutz RE 2023 [R66] | **[미확인]** snippet | cFS 소스·배치 | 인증 없는 SB 명령, 공격면, 신뢰 경계, 자동 module·app 의존 지도(WannaFly), 위성 저장소 126개의 SAST(2,827건) | 정적·동적 혼합 | 명시 없음 | module·앱 의존 추출 | 대상은 보안과 신뢰 경계다. 앱 간 순서·시간 state의 정확성은 snippet 범위에서 보이지 않는다 |
| cfs-msgid-guard README @4dda7e0 [S126] | **[확인된 사실]** README | topicid header | MID 충돌 | 정적 | 명시 없음 | MID 추출 | MID의 정적 추출은 이미 작은 도구로 존재한다 |
| cFE의 CCSDS EDS ([cfe_sb.xml L970-L1033, L1068-L1135](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/sb/eds/cfe_sb.xml#L970-L1033); [EdsLib README](https://github.com/nasa/EdsLib/blob/8ec0656888e79f34c28fc857b3f53879abc3e058/README.md#L1-L30)) | **[확인된 사실]** 파일 | 앱별 EDS XML | 앱이 받는 command TopicId, 내는 telemetry TopicId와 payload type. indication은 `mode="async"`. 주석은 TC와 TM stream이 "no direct relationship"이라고 쓴다 | 선언 | – | MID·payload type 해석의 입력 | 누가 어떤 telemetry를 소비하는지, 순서·freshness·행동을 선언하지 않는다. **[실측]** `grep -rl StateMachine repos/cFE --include=*.xml \| wc -l` → 0. 필요 순서의 출처가 될 수 없다 |

**Ogma 템플릿 상세** (검증에서 이견이 있었던 항목. 아래는 정정된 문구다).

- **[확인된 사실]** 템플릿은 pipe 하나를 만든다([L116](https://github.com/nasa/ogma/blob/08b384b4335cce324fac74577adc7f08e61b7cce/ogma-core/templates/cfs/copilot/fsw/src/copilot_cfs.c#L116-L153)). 감시 대상 입력 변수의 MID와 `COPILOT_CFS_REEVAL_CMD_MID`(기본 0x1883)를 구독한다(L127-153). 각 subscribe는 앞 단계가 성공했을 때만 실행된다. 감시 변수는 `--variable-file`이 정하고, 없으면 spec의 외부 변수에서 온다. DB가 해석하지 못한 변수는 조용히 빠진다 ([CFSApp.hs L136-L266](https://github.com/nasa/ogma/blob/08b384b4335cce324fac74577adc7f08e61b7cce/ogma-core/src/Command/CFSApp.hs#L136-L266)).
- **[확인된 사실]** main loop는 `CFE_SB_ReceiveBuffer(&SBBufPtr, COPILOT_CommandPipe, 500)`로 받는다([L68-L80](https://github.com/nasa/ogma/blob/08b384b4335cce324fac74577adc7f08e61b7cce/ogma-core/templates/cfs/copilot/fsw/src/copilot_cfs.c#L68-L80)). data MID handler는 payload 전체나 한 field를 전역에 복사한다. 그 입력이 DB에서 `"active": true`일 때만 `copilot_step()`을 부른다([L218-L239](https://github.com/nasa/ogma/blob/08b384b4335cce324fac74577adc7f08e61b7cce/ogma-core/templates/cfs/copilot/fsw/src/copilot_cfs.c#L174-L239)). REEVAL 명령은 새 data 없이 `copilot_step()`을 부른다.
- **[실측]** `grep -rn 'WaitForStartupSync\|WaitForSystemState' ogma-core/templates/cfs` → 일치 없음 (`$N/rw-flight/evidence_extracts.txt` E14, `$N/verify-C06`).
- **[확인된 사실]** README의 예제 호출은 `examples/cfs-variables`를 쓰고, 그 파일은 `position` 한 줄이다. 그래서 그 앱은 data stream 하나만 구독한다. 두 stream 예제는 `cfs-002-state-machines`다. `state`는 active(STATE_MID), `input`은 passive(SAMPLE_MID)다 (db.json [S93]).
- **[해석]**(이견 있음) 두 개 이상의 입력을 고르면 monitor는 따로 갱신되는 전역의 마지막 값을 함께 읽는다. 검증자 사이에 이 일반화에 대한 이견이 있었다. Ogma 생성 앱을 평가 대상으로 쓸 때는 입력 선택과 active/passive 설정을 고정해 보고한다.
- **[해석]** runtime monitor는 관측된 실행만 판정한다. 정적 분석이 더할 수 있는 것은 실행되지 않은 경로와, startup sync가 없는 시작 구간이다.

**CI 이력.** **[실측]** cFE 전체 이력(2153 commit)의 commit message를 `git -C repos/cFE log --all -i --oneline --grep=<도구> | wc -l`로 셌다. CodeSonar 4, CodeQL 13, cppcheck 11, 'static analysis' 17, Coverity·Polyspace·Klocwork·IKOS·Frama-C 0이다 (`$N/rw-flight/evidence_extracts.txt`). issue·PR 본문과 앱 저장소는 검색하지 않았다. 따라서 0건은 사용하지 않았다는 증거가 아니다.

#### 4.1.2 다른 flight framework와 avionics 분석

| 연구·도구 | 확인 수준 | 입력 | 속성 | 정적/동적 | 보장 | 겹침 | 간극 **[해석]** |
| --- | --- | --- | --- | --- | --- | --- | --- |
| F′ FPP 생성 코드 ([TopSetupTeardownFns.scala L40-L68 @91b47409](https://github.com/nasa/fpp/blob/91b474097ea1a5503139440bed661aee5e16a8ad/compiler/lib/src/main/scala/codegen/CppWriter/TopologyCppWriter/TopSetupTeardownFns.scala#L40-L68)) | **[확인된 사실]** 생성기 소스, 참조 생성 코드, 사양 | FPP topology 모델 | 시작 순서를 생성 코드가 고정한다: `initComponents → configComponents → setBaseIds → connectComponents → regCommands → readParameters → loadParameters → startTasks`. 능동 component의 queue는 init에서 만든다. 출력 port 호출은 `FW_ASSERT(port.isConnected())`를 거친다 ([PassiveSerialComponentAc.ref.cpp L3146-L3176](https://github.com/nasa/fpp/blob/91b474097ea1a5503139440bed661aee5e16a8ad/compiler/tools/fpp-to-cpp/test/component/base/PassiveSerialComponentAc.ref.cpp#L3146-L3176)). async queue가 가득 차면 기본은 assert다 ([Port-Instance-Specifiers.adoc L145-L166](https://github.com/nasa/fpp/blob/91b474097ea1a5503139440bed661aee5e16a8ad/docs/spec/Specifiers/Port-Instance-Specifiers.adoc#L145-L166)). `fpp-check -u`는 연결되지 않은 port를 파일로 낸다 | 정적 (모델) + 실행 시 assert | 생성된 wiring은 task 시작 전에 완성된다 (**[해석]**) | framework가 시작 순서와 연결을 강제하는 방식 | 한 F′ topology 안에서는 '구독자가 붙기 전 발행'이 wiring 때문에 생기지 않는다. cFS 구독은 앱이 시작된 뒤의 runtime 호출이다(3.1절). 이 대조가 cFS에서 F1·F8을 분석해야 하는 이유다. F′에도 configComponents의 사용자 코드, startTasks 안의 task 시작 순서, data 준비 같은 순서 문제는 남는다 |
| F′ CodeQL query ([FprimeCommandResponse.ql @4b58f366](https://github.com/nasa/fprime/blob/4b58f3666fe4ca8a3cacd389aed5faf1fb32b063/.github/codeql/fprime-queries/FprimeCommandResponse.ql#L1-L15) 외. 약 30개 중 4개 확인) | **[확인된 사실]** query 소스와 CI | F′ C++ 소스 | framework 의미를 담은 pattern. 모든 `*_cmdHandler` 경로가 `cmdResponse_out`을 부르거나 opCode·cmdSeq를 저장한다. internal interface handler를 직접 부르지 않는다. rate group(`schedIn`) handler에서 blocking하지 않는다. lock을 쥐고 동기 file I/O를 하지 않는다 | 정적, component 내부 | 명시 없음 | framework 규칙을 query로 표현 | component 사이의 분석이 아니다. 'CFE_SB_Subscribe 상태 무시'나 '첫 수신 전 state 읽기' 같은 cFS query가 이 연구가 넘어서야 할 단순 기준선이다 |
| [FireflySpace/fprime-topo-analysis @f0db61c](https://github.com/FireflySpace/fprime-topo-analysis/blob/f0db61c55e04c2e60a076656eddaf9490316be69/README.md#L1-L53) | **[확인된 사실]** README 전문, 소스 grep (검증 정정 반영). 도구는 실행하지 않음 | `fpp-to-json` topology(input port마다 sync·guarded·async 표시)와 libclang이 `compile_commands.json`에서 복원한 handler → output port 흐름 | 약 20개 분석. `checks.py`에 17개(README 표는 15개)가 있고, 별도 analyzer 3개가 있다. lock-order deadlock(self, ABBA, lock-order ABBA, atomic 재잠금), lock을 고려한 member data race, async queue의 producer·우선순위·채움률과 낮은 우선순위로의 self re-queue, 미연결 port, sync cycle, dead telemetry, buffer 소유, 직접 parameter taint에 한한 assert 도달성 ([README L294-L436](https://github.com/FireflySpace/fprime-topo-analysis/blob/f0db61c55e04c2e60a076656eddaf9490316be69/README.md#L294-L436)) | 정적 | 명시 없음. 해석하지 못한 handler는 `--permissive`가 없으면 오류다 | 프레임워크 연결 모델과 소스의 handler 흐름을 결합해 동시성을 분석한다. 구조상 가장 가까운 선례다 | README의 "deliberately not modeled"는 사용자 thread·callback("Only F′ port calls are followed"), runtime guard(항상 실행된다고 가정), runtime port routing이다 ([L545-L560](https://github.com/FireflySpace/fprime-topo-analysis/blob/f0db61c55e04c2e60a076656eddaf9490316be69/README.md#L545-L560)). command 등록은 시작 시 단일 thread라고 가정한다 ([checks.py L50-L53](https://github.com/FireflySpace/fprime-topo-analysis/blob/f0db61c55e04c2e60a076656eddaf9490316be69/src/fprime_topology_analysis/checks.py#L50-L53)). 메시지 → state 출처, freshness, 시작·lifecycle 순서 분석은 소스 grep에서 보이지 않았다. queue 우선순위와 지연은 분석한다 |
| Goblint v1.1.0의 `arinc.ml`·`osek.ml` ([arinc.ml @5e1a53e2](https://github.com/goblint/analyzer/blob/5e1a53e2c87660f8be8a3bbd43b64075fc15233d/src/analyses/arinc.ml#L545-L560), [osek.ml L81-L95](https://github.com/goblint/analyzer/blob/5e1a53e2c87660f8be8a3bbd43b64075fc15233d/src/analyses/osek.ml#L81-L95)) | **[확인된 사실]** 소스 (검증에서 원래 주장이 기각되어 정정 반영) | C 소스. OSEK은 OIL 파일도 받는다 | ARINC: 파일 머리는 "Tracking of arinc processes and their actions. Output to console, graphviz and promela."다. process 생성·시작·정지·일시정지, preemption, partition mode, blackboard, semaphore, event, TimedWait·PeriodicWait를 action으로 만든다. sampling·queuing port와 buffer 호출은 인식만 하고 `todo()` → `Nop`이다(L386, L545-560). OSEK: OIL을 읽어 ceiling priority를 계산하고 `DisableAllInterrupts` 등을 pseudo-resource로 다루는 data race 분석이다 | 정적 | 명시 없음 | RTOS API 호출 인식 → process·동기화 모델 추출(graphviz·Promela) | 메시지 port의 데이터 흐름은 모델링하지 않았다. `arincUtil.ml`의 주석은 queuing message·buffer를 "communication with outside"라서 시스템 상태에 영향이 없다며 뺐다. 이 분석은 goblint-1.0.0(2017)부터 있었고 osek은 v0.9.5(2011)부터 있었다. v2.0.0에서 제거되었다 ([CHANGELOG L124-L127](https://github.com/goblint/analyzer/blob/d0ee7d5b0e2b6c12f0d33a285973397ab29eb9fa/CHANGELOG.md#L124-L127)) |
| Havelund, Lowry, Penix, TSE 27(8):749–765, 2001 (서지 [R21]) | **[미확인]** snippet | DS1 Remote Agent EXEC 서비스를 손으로 옮긴 PROMELA 모델 | deadlock, assertion | 정적 (SPIN) | 모델에 대해 망라 | 비행 SW의 순서 결함 | snippet에 따르면 미발견 동시성 오류 5개를 찾았고, 같은 pattern의 결함(빠진 critical section)이 1999-05-18 비행 중 deadlock을 일으켰다. thread·lock 문제이고 모델이 수작업이다 |
| Space ROS 문서 ([IKOS.rst L33-L39 @5fa4e62](https://github.com/space-ros/docs/blob/5fa4e62f0141a78c6372bbba76f1dec4e167684b/source/Related-Projects/IKOS.rst#L33-L39)) | **[확인된 사실]** 문서 | ROS 기반 우주 SW | IKOS를 Docker image와 ament(`ament_ikos`)에 넣고 결과를 SARIF로 모은다. Cobra 등 다른 분석기도 나열한다 | 정적 | – | 인증 증거용 정적 분석 | topic 순서나 메시지 유래 state 분석은 문서에 보이지 않는다 |

**fprime-topo-analysis의 이력.** **[확인된 사실]** main branch는 commit 17개(2026-08-29–09-14, 저자 1명)이고 pyproject 버전은 0.1.0, tag는 없다. 첫 commit의 제목은 "Exfil topo analysis from fprime-full-stack repo"이고 약 6,000줄을 더한다. 따라서 commit 날짜는 이 저장소의 이력일 뿐 도구가 처음 만들어진 시점이 아니다. **[미확인]** README는 논문을 인용하지 않는다. 동료심사 논문은 찾지 못했다 (철저한 검색은 아님).

#### 4.1.3 이 무리에서 얻는 판단

- **[해석]** cFS에는 이미 여섯 종류의 검증이 있다. 아키텍처 규칙 검사(2009), SB 동시성 모델 기반 시험(2016), 설계 모델 생성·검사(TASTE), runtime monitor(Ogma), framework stub을 쓴 sound abstract interpretation(IKOS), 일반 정적 분석 CI다. 따라서 "cFS 동시성의 첫 분석"이나 "cFS의 첫 모델링"은 주장할 수 없다.
- **[해석]** cFS 앱의 C 소스에서 앱 간 순서와 메시지 유래 state를 정적으로 판정한 작업은 이 조사 범위에서 찾지 못했다. 가장 큰 공백은 R03과 SPLC 2009의 본문을 읽지 못한 것이다 (4.7.3).
- **[해석]** 구조상 가장 가까운 선례는 cFS가 아니라 F′의 fprime-topo-analysis다. F′는 wiring을 task 시작 전에 고정한다. 같은 접근을 cFS로 옮기면 runtime 구독, table·명령으로 바뀌는 구독, 재시작을 추가로 모델링해야 한다. 이 추가분이 이 연구의 범위다.

---

### 4.2 Publish/subscribe 구조 복원

#### 4.2.1 비교표

| 연구·도구 | 확인 수준 | 입력 | 속성 | 정적/동적 | 보장·결과 | 겹침 | 간극 **[해석]** |
| --- | --- | --- | --- | --- | --- | --- | --- |
| ROSDiscover, ICSA 2022, pp. 112–123 (DOI 10.1109/ICSA53651.2022.00019 [R14]; paper.pdf @4648ad9 [R14]) | **[확인된 사실]** 전문 (평가 저장소 사본). 학회명은 ROSInfer 참고문헌과 저자 bib 파일로 확인. 평가 README의 "ICSA, 2021"은 연도 오기로 보인다 | ROS 1 C++ 소스(Clang AST, symbolic function summary), launch file(구성·remap·parameter), 손으로 쓴 모델 15개 | topic·service·action port와 parameter 복원. 논문은 Acme 규칙 4개를 쓴다: message type 불일치 1개, dangling publisher/subscriber 변형 3개. 배포된 `ROSFam.acme`에는 service self call, parameter reader/writer, 중복 node 같은 구조 규칙이 더 있다 | 정적 | 'quasi-static' 구조 가정. 모르는 값은 top. API 호출의 87.37%를 완전히 복원했다. 3개 시스템에서 과소 9.09%, 과대 6.82%. misconfiguration 결함 19개 중 8개를 찾았다. 오경고는 AutoRally 8, Husky 5, TurtleBot 2 | 소스에서 pub/sub topology 복원 | 순서·시작·시간 속성을 검사하지 않는다. 저자들은 NASA F′를 일반화 대상으로 꼽는다 (txt L1384-1393). cFS는 언급하지 않는다 |
| ROSInfer, ICSE 2024 (DOI 10.1145/3597503.3639206 [R15]; 저자 PDF [R15], sha256 59bcb083…) | **[확인된 사실]** 전문, 그리고 [states_analyzer.py @78301b86](https://github.com/cmu-rss-lab/rosdiscover/blob/78301b86fca119cda9ad55500dcf800e56bb9504/src/rosdiscover/recover/states_analyzer.py#L30-L61) | ROS 1 C++ 소스 (ROSDiscover 확장) | component 상태기계 `C=(S, s0, I, O, δ)`. 입력 trigger: 메시지(`subscribe`, `registerCallback`, `advertiseService`의 callback), 주기(`Rate::sleep`, `createTimer`), component 시작 event. state 변수: 구조적 행동(메시지 발행, state 변경과 그 호출자)의 제어 조건에 쓰이고(usage) 전역·component 범위인(scope) 변수. 코드에서는 발행 조건에 나오고 발행 함수가 아닌 다른 함수에서 대입되는 변수다. PlusCal/TLA+를 만들어 LTL(예: 기대 출력이 결국 나온다)을 검사한다 | 정적 추론 + 모델 검사 | component 534개. recall은 주기 93%, 반응 82%, state 변수 71%, 전이 69%. precision은 100%, 91%, 76%, 88%. ROSDiscover 데이터셋에서 이미 알려진 결함 3개(autoware-02/03/10)를 component 목록, 기대 출력, parameter를 주고 다시 찾았다 | 메시지 → state 변수 → 조건부 발행의 추론과 '필요 입력이 오지 않음' 검사. 원노트 §44, 수정본 §6.1의 pattern과 구조가 같다 | "To keep the model simple, we do not model the content of messages." future work 절은 정적 분석이 실행 시간을 알 수 없어 "most kinds of performance analysis, bottleneck analysis, or analysis of race condition"에 쓸 수 없다고 쓴다. 다시 찾은 결함 3개는 발행자가 아예 없는 경우다. 구독 완료 전 발행, staleness, 수신 status·timeout 경로는 다루지 않는다. NASA FPrime을 일반화 대상으로 꼽는다 (L1434-1438) |
| HAROS IRC'19 (PDF @f7e268d [R45]; DOI 10.1109/IRC.2019.00018), HPL ([lang.md L21-L52](https://github.com/git-afsantos/hpl-specs/blob/9c2bcd5e69923137688aef2eb57b60a1fedef103/docs/lang.md#L21-L52)), IROS'20 Electrum [R46] ([template L80-L96](https://github.com/git-afsantos/haros-plugin-electrum/blob/b3b3c111026e3ea8b20ab851efaa88a9774e7683/src/haros_plugin_electrum/templates/model.ele.jinja#L80-L96)) | **[확인된 사실]** IRC'19·IROS'20·RoSE'21 전문, 추출기 코드. ENASE'22 HPL 논문은 **[미확인]** | launch file, CMake, C++ AST, 사용자 추출 hint | 호출마다 이름·type·queue size·latched·path condition·loop 여부를 기록한다 ([extractor.py L1355-L1378](https://github.com/git-afsantos/haros/blob/efe832c38bfb9682ca7575337a7decd46e59beb6/haros/extractor.py#L1355-L1378)). 예시 query는 구조적이다: type 불일치, 무한 queue, queue size 1("can lead to message loss"), 이름이 비슷한 끊긴 topic. HPL은 absence·existence·precedence·response·prevention과 "within 100 ms" 같은 시간 한계를 쓴다 | 추출은 정적. HPL은 runtime monitor·property-based test. Electrum은 정적 모델 검사 | Electrum에서는 "the real-time features of HPL are not supported". template은 모든 outbox 메시지가 결국 모든 구독자에 도달하도록 강제하고, 구독 집합은 고정이다 | path condition·queue size·latched 정보의 추출 | latched flag를 기록만 하고 검사에 쓰지 않는다. 고정 구독·신뢰 전달 가정 때문에 '구독 전 발행'과 staleness를 표현할 수 없다. 논문은 message_filters·tf2를 추출 한계로 든다 |
| ROSpec ([utils.py L131-L145 @e47d6eb](https://github.com/pcanelas/rospec/blob/e47d6eb0e72e0fb5e00c90b7d019dcb472c40fff/src/rospec/verification/utils.py#L131-L145)) | 코드 **[확인된 사실]**. 논문 [R58] **[미확인]** | 개발자가 쓰는 ROSpec DSL의 구성 명세 | 선언된 QoS의 호환성. Volatile 제공자–TransientLocal 소비자, BestEffort–Reliable, liveliness, deadline 순서를 검사한다. history·lifespan·depth는 항상 통과시킨다 | 정적 | – | 'latching'에 대한 양쪽 합의를 정적으로 검사 | 선언된 QoS만 본다. 구독과 발행의 시점 순서는 보지 않는다 |
| ROS 1 latch ([node_handle.h L236-L238](https://github.com/ros/ros_comm/blob/30483a9f218f1545eec16d3934bf3cb042e2cb5b/clients/roscpp/include/ros/node_handle.h#L236-L238)), ROS 2 QoS ([문서 L48-L61, L164-L191](https://github.com/ros2/ros2_documentation/blob/e2388aa72c17598a54fa228df6e9e9a44c5e7aec/source/ROS-Framework/interfaces/topics/About-Quality-of-Service-Settings.rst#L48-L61)), Fast DDS ([standardQosPolicies.rst L82-L125](https://github.com/eProsima/Fast-DDS-docs/blob/b2af9caf0411a407a40d80622b1e6e43c9a41577/docs/fastdds/dds_layer/core/policy/standardQosPolicies.rst#L82-L125)) | **[확인된 사실]** 문서·header. OMG DDS 사양은 열지 못함 | 실행 시 설정 | latch는 마지막 메시지를 새 구독자에게 보낸다. TRANSIENT_LOCAL은 늦게 붙는 구독자를 위해 sample을 보존한다. Volatile 발행자와 Transient-local 구독자는 "No communication"이다. LIFESPAN은 만료 메시지를 버리고, DEADLINE은 missed-deadline event를 낸다. Fast DDS는 DestinationOrder·Presentation을 "will be implemented in future releases"로 표시한다 | 동적 (framework 기능) | 설정한 범위 | F1·F3을 framework 설정으로 다룬다 | cFS SB에는 대응하는 설정이 없다. `CFE_SB_Qos_t`는 "currently unused"다 (1.2절 F1, 3.1절). cFS에서는 순서와 앱의 복구 관용구로 판정해야 한다 |
| Ganesan, Lindvall, Ruley, Wiegand, Ly, Tsui, WCRE 2010, pp. 173–182 (DOI 10.1109/WCRE.2010.27 [R17]) | 서지는 HAROS IRC'19 참고문헌 [12]로 **[확인된 사실]**. 내용은 **[미확인]** snippet | NASA GMSEC(지상 시스템) 구현 | pub/sub 양식에서 나온 재사용 질문. 구조 질문은 정적으로, 행동 질문은 probe를 넣은 runtime trace로 답한다 | 정적 + 동적 | 명시 없음 | pub/sub topology 복원과 상호작용 검사 | cFS가 아니라 GMSEC이다. 같은 그룹의 pub/sub 분석 선례로 인용한다 |

**ROS 결함 연구.**

- **[실측]** ROBUST 데이터셋(@521fe75 [S133])의 결함 221개를 fault code로 집계했다. CONCURRENCY 코드가 붙은 결함은 19개다. 코드별로 NO-SYNC 12, BAD-SYNC 4, SIGNALS 3, TIMING 1이고 대부분 thread 수준이다. 제목·설명의 keyword 검사에서 '구독자 연결 전 발행으로 메시지 유실' 결함은 없었다 (`$N/rw-pubsub/robust_concurrency_subset.csv`). issue 본문과 논문 codebook은 읽지 않았다.
- **[미확인]** Garcia 등 ICSE 2020 [R55]은 Apollo·Autoware 결함 499개 중 동시성을 6개로 집계했다(snippet). Chen 등(arXiv 2507.10235) [R56]은 interaction 결함 121개를 보고하고 ROSDiscover는 dangling 부분만 찾는다고 쓴다(snippet). ROSCallBaX(FSE 2025) [R57]는 callback 구성과 executor 설정을 정적으로 검사한다(snippet).
- **[해석]** 공개 ROS 결함 코퍼스는 F1의 기저율이나 정답 사례를 주지 않는다. 이 연구의 평가는 cFS의 역사적 사례와 label을 단 합성·변형 사례에 기대야 한다 (1.6절 RQ3).

#### 4.2.2 ROS 도구를 cFS로 옮길 때 달라지는 점

| 측면 | ROS (ROSDiscover·ROSInfer의 가정) | cFS (고정 버전) | 분석 함의 **[해석]** |
| --- | --- | --- | --- |
| topic 결합 | **[확인된 사실]** `advertise()`가 publisher 객체에 topic을 묶는다 | **[확인된 사실]** MID는 `CFE_MSG_Init`으로 buffer에 묶인다. `CFE_SB_TransmitMsg`에는 MID 인자가 없다 ([sample_app.c L134](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app.c#L134), [sample_app_cmds.c L61](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app_cmds.c#L61)) | buffer 객체를 따라가는 dataflow가 필요하다 |
| 반응 trigger | **[확인된 사실]** topic별 callback 등록 | **[확인된 사실]** pipe 하나의 `CFE_SB_ReceiveBuffer` loop, `CFE_SB_MsgId_Equal` 분기, FcnCode switch ([sample_app_dispatch.c L132-L160](https://github.com/nasa/sample_app/blob/199476a34827ae84d50d66f97619227854cd971a/fsw/src/sample_app_dispatch.c#L132-L160)). 비교 대상은 첫 호출 때 채워지는 function-static MID cache다 | dispatch 분기와 out-parameter(`CFE_MSG_GetMsgId`) 의미를 복원해야 한다 |
| 구성 | **[확인된 사실]** launch file. ROSDiscover는 quasi-static 구조를 가정한다 | **[확인된 사실]** HK copy table([hk_utils.c L322](https://github.com/nasa/HK/blob/0dc16b7a7bab71747b9c063cf405ad59a65a40a5/fsw/src/hk_utils.c#L316-L324)), TO_LAB subscription table([to_lab_app.c L504](https://github.com/nasa/to_lab/blob/d27c6014cb5d2979140500f635876ee87b81995b/fsw/src/to_lab_app.c#L500-L506)), 지상 명령([to_lab_cmds.c L218](https://github.com/nasa/to_lab/blob/d27c6014cb5d2979140500f635876ee87b81995b/fsw/src/to_lab_cmds.c#L216-L219)), table 갱신 시 재구독. **[실측]** `apps/*/fsw/src`의 `CFE_SB_Subscribe*` 호출 47곳 중 8곳이 table·명령·설정 구조체에서 MID를 받는다 (`$N/verify_C16_overreach/subscribe_sites_bundle.txt`) | table 이미지가 입력이어야 한다. 명령으로 생기는 구독은 '동적'으로 따로 표시한다. quasi-static 가정은 일부 깨진다 |
| 주기 trigger | **[확인된 사실]** `Rate::sleep`, `createTimer` | **[확인된 사실]** SCH_LAB이 table의 MessageID를 PacketRate tick마다 보낸다 ([sch_lab_app.c L246-L248](https://github.com/nasa/sch_lab/blob/607e2f90c5f828f3f3ae655a4debacf8b2cc4bbb/fsw/src/sch_lab_app.c#L246-L248)) | ROSInfer의 주기 검출 방식이 적용되지 않는다. 주기는 SCH table에서 온다 |
| 늦은 구독자 | **[확인된 사실]** latch, TRANSIENT_LOCAL | **[확인된 사실]** 대응 설정이 없다. 경로 없음과 목적지 0개는 모두 발신자에게 `CFE_SUCCESS`다 (1.2절 F1) | 설정 검사 대신 순서 판정을 해야 한다 |
| 구독자 사이 전달 순서 | **[해석]** 위 도구들은 모델링하지 않는다 | **[확인된 사실]** head 삽입이라 마지막 구독자부터 전달한다. v7.0.0 이후는 SB mutex 밖에서 목적지별 put을 한다. 송신 호출이 반환되기 전에 수신 task가 실행될 수 있다 (1.2절 F7, 3.1절) | cFE 버전과 Σ를 매개변수로 둔다 |
| topic 식별자 | **[확인된 사실]** 문자열 이름과 remap | **[확인된 사실]** 설정 header의 정수 MID macro. EDS build에서는 runtime 함수 결과다 ([eds_cfe_core_api_msgid_mapping.h L37-L66](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/core_api/config/eds_cfe_core_api_msgid_mapping.h#L37-L66)) | build 설정 header가 입력이어야 한다. IR에서는 macro 이름이 사라진다 (4.5.3) |
| 소비자 없음 | **[확인된 사실]** ROSDiscover는 dangling publisher로 경고한다 | **[확인된 사실]** 의도된 경우가 있다. TO_LAB은 table 기반 구독을 startup sync 뒤로 미룬다 ([to_lab_app.c L69-L73](https://github.com/nasa/to_lab/blob/d27c6014cb5d2979140500f635876ee87b81995b/fsw/src/to_lab_app.c#L69-L73)) | 'required consumer' 계약이 없으면 경고하지 않는다 |

#### 4.2.3 이 무리에서 얻는 판단

- **[해석]** C/C++ 소스에서 Clang으로 pub/sub topology를 복원하는 일(ROSDiscover)과, 메시지가 바꾸는 state 변수와 그 변수로 조건화된 발행을 추론하는 일(ROSInfer)은 이미 확립되었다. 이 연구의 신규성은 그 일반형에 둘 수 없다.
- **[해석]** ROSInfer가 스스로 밝힌 공백은 다음과 같다. 메시지 내용(payload field)을 모델링하지 않는다. 실행 시간과 race를 다루지 않는다. '필요 입력이 오지 않음' 검사는 발행자가 없는 경우이지, 구독 완료 전 발행으로 생기는 유실이 아니다. 수신 실패·timeout 경로도 없다. 이 네 가지가 cFS 판에서 더 보일 수 있는 부분이다.
- **[미확인]** ROSInfer의 PlusCal/TLA+ 생성기는 `cmu-rss-lab/rosdiscover`와 `rosdiscover-evaluation`의 기본 branch에서 찾지 못했다. 그 모델이 '구독 전 발행된 메시지'를 표현할 수 있는지 확인하지 못했다. 검증 중 읽은 저자의 2024 학위 제안서는 timing data를 모으는 동적 분석 확장을 계획한다. 2025 학위논문은 열지 못했다.

---

### 4.3 순서·race 이론과 도구

#### 4.3.1 이론과 분산 시스템의 메시지 타이밍

| 연구·도구 | 확인 수준 | 입력 | 속성 | 정적/동적 | 보장 | 겹침 | 간극 **[해석]** |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Lamport 1978 [R01] | **[확인된 사실]** 수정본 §14 (pp. 558–560) | 분산 사건 | happened-before 부분순서: 같은 실행 주체의 순서, 같은 메시지의 send–receive, 추이성 | 이론 | – | 이 연구의 보장 관계 `G`의 기초 | cFS의 전달 실패·queue 의미는 없다. 1.5절에서 정한 대로 send–receive 간선은 성공한 수신에만 생긴다 |
| Cousot & Cousot 1977 [R02] | **[확인된 사실]** 수정본 §14 (저자 요약) | – | lattice·fixpoint·추상화의 정합성 틀 | 이론 | 구체 의미에 대한 건전성 틀 | 분석 설계를 검증하는 방법 | 구체 domain과 transfer 함수는 이 연구가 정해야 한다 |
| Netzer & Miller, LOPLAS 1(1):74–88, 1992 (DOI 10.1145/130616.130623) [R27] | **[미확인]** snippet | 실행 trace | feasible race와 apparent race의 구분. feasible race 찾기는 NP-hard. message-passing 추적·replay는 receive 중심의 동적 분석 | 동적 | – | 'HB 미증명은 후보일 뿐'이라는 구분 | 이 연구의 정적 후보는 apparent race 수준이다 (1.5절). 원문 확인 전에는 정의를 인용하지 않는다 |
| Leesatapornwongsa, Lukman, Lu, Gunawi, TaxDC, ASPLOS 2016 (PDF @87a6a05 [R12]; DOI 10.1145/2872362.2872374) | **[확인된 사실]** 전문 | Cassandra 19개, HBase 30개, Hadoop MapReduce 36개, ZooKeeper 19개의 distributed concurrency 결함 (2011–2014 보고) | 분류. order violation(메시지가 다른 사건보다 먼저(늦게) 오면 결함이 나고, 반대 순서면 나지 않음)만으로 생긴 결함 46개(44%), atomicity violation만 21개(20%), 둘 이상 4개(4%). untimely message가 원인인 결함이 64%, fault·reboot timing이 32% | 경험 연구. §8.4는 제안이다 | – | §8.4의 "generic detection framework": (1) 메시지와 계산 사이의 order·atomicity timing specification을 얻는다. (2) 그 위반을 동적 또는 정적 분석으로 찾는다. invariant template 예: "message bc should arrive at C before message ac (ca) arrives (leaves)". 명시적 오류에서 거꾸로 spec을 추론하는 error-guided 방향도 제안한다. 사례 m3274(먼저 도착한 kill 메시지를 무시), h5780(보안 key 메시지 도착 전 join 요청 → 초기화 중단) | 원노트 §47의 `RequiredHB ∧ ¬ProvenHB ∧ PotentialConcurrent`의 골격(필요 순서 spec + 위반 검출)은 TaxDC §8.4가 연구 방향으로 이미 제안했다. 구현과 평가는 없다. 대상은 shared-nothing node이고, node 내부 thread interleaving(LC)은 따로 분류한다. 저자들은 통계를 일반화하지 말라고 쓴다. 'Global missing messages'(9%)는 트리거 node가 응답을 보내지 않는 경우이고 전송 중 유실이 아니다 |
| DCatch, ASPLOS 2017 [R28] | **[미확인]** snippet | 올바른 실행의 trace와 정적 pruning | message·task·event·program(MTEP) HB 규칙, 분산 while-loop(polling) 동기화 규칙, ZooKeeper식 notification 규칙. program order는 한 event handler·RPC 함수 안에서만 성립한다. 32개 DC 결함 보고, 그중 20개 유해 | 동적 + 정적 pruning | – | HB 원천을 message·lifecycle·sync로 나누는 구조 (원노트 §46) | 검출 목표는 여전히 충돌하는 메모리 접근이다. polling 규칙은 `CFE_ES_WaitForSystemState`를 loop 탈출에 조건부인 HB로 모델링할 선례다 (규칙 이름 미확인) |
| FCatch, ASPLOS 2018 [R29] (model.md @ab7ed6b5 [S135]) | 프로젝트 사이트 **[확인된 사실]**. 논문 **[미확인]** | 올바른 실행 | time-of-fault(TOF) 결함: fault(메시지 유실, node crash)의 시점 때문에 남는 예상 밖 state. crash-regular와 crash-recovery 범주 | 동적 예측 | – | 재시작 시점 결함 (F8) | 메시지 유실을 주입된 fault로 본다. F1은 순서 때문에 생기는 유실이다. 둘을 섞지 않는다 |

#### 4.3.2 공유 메모리의 순서·원자성·일관성

| 연구·도구 | 확인 수준 | 입력 | 속성 | 정적/동적 | 보장 | 겹침 | 간극 **[해석]** |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Lu 등, 2008 (MSR 서지 [R24]; DOI 10.1145/1346281.1346323) | **[미확인]** 초록만 | MySQL·Apache·Mozilla·OpenOffice 결함 105개 | 조사한 non-deadlock 결함의 약 1/3이 프로그래머의 순서 의도 위반. 약 34%가 여러 변수에 걸친다. 약 92%는 접근 4개 이하의 순서 강제로 재현된다. 약 73%는 lock 추가·변경으로 고쳐지지 않았다 | 경험 연구 | – | F2·F5가 order violation임 | 정의 원문과 범주별 수는 읽지 못했다 |
| PCT, ASPLOS 2010 (PDF [R25]) | **[확인된 사실]** §2.3, Fig. 1 | 다중 thread 프로그램 | 결함 depth. Lu 등의 ordering bug를 depth 1로 본다. Fig. 1(a)는 Thread 2가 Thread 1의 초기화 전에 `t`에 접근하면 나타나는 ordering bug다 | 동적 (scheduling 탐색) | 이 노트는 확인하지 않음 | '초기화 전 사용' = order violation | thread 수준. 메시지는 없다 |
| Artho, Havelund, Biere, high-level data race, STVR 13(4), 2003. 후속 ATVA 2004 (block-local atomicity) [R22] [R23] | **[미확인]** snippet | 다중 thread 프로그램 (2004판은 JNuke 위 구현) | 원자적으로 접근해야 하는 관련 field 집합의 일관성(view, maximal view, view consistency). ATVA 2004는 JNuke에서 data-flow 기반 정적 기법으로 stale-value 오류를 찾는다고 쓴다 | 2003년 판의 정적/동적 여부는 미확인 | – | F4의 참조 개념 | view는 lock 보호 block으로 정의된다. 메시지 유래 state에 적용되는지는 원문을 읽기 전에 판단하지 않는다 |
| Flanagan & Qadeer, PLDI 2003 (type system for atomicity) [R30]; Lu 등, AVIO, ASPLOS 2006 [R31] | **[미확인]** snippet | Java(RCC/Java 위) / 실행 trace | 원자성: 어떤 실행이 끼어듦 없는 실행과 동등함(Lipton reduction). AVIO는 올바른 실행에서 interleaving 불변식을 배운다 | 정적 type system / 동적 | – | F4의 참조 개념 | 한 code block 안의 끼어듦을 다룬다. run-to-completion handler에서는 끼어듦이 없다 (1.2절 F4) |
| Burrows & Leino, stale-value errors (krml107 [R13]; CCPE 16, 2004, pp. 1161–1172) | **[확인된 사실]** 원고 전문. 출판본과는 대조하지 않음. DOI 10.1002/cpe.866은 해석하지 못함 | Java, ESC/Java 확장 | 한 critical section에서 읽어 지역 변수에 담은 값을, 같은 thread가 뒤의 critical section에 들어간 뒤(`wait()` 재획득 포함) 사용하는 오류. 나간 직후의 사용은 'justifiable'로 보고 경고하지 않는다. 지역 변수마다 ghost boolean `stale_t`·`from_critical_t`를 둔다. 어느 lock인지는 대체로 무시한다 | 정적, 주석 불필요 | Java 8개 프로그램 617 kloc에서 경고 48개: 오경고 43, 무해 race 1, 결함 4. 저자들은 ambit·jigsaw의 분류가 확실하지 않다고 쓴다 | F3의 정적 선례 | staleness의 기준이 critical section 재진입이다. 물리 시간이나 생산자의 새 발행 여부가 아니다. 대상은 지역 변수다. cFS state는 앱 전역에 있다 |
| Tulsyan, Pai, D'Souza, FSTTCS 2020 [R06] | **[확인된 사실]** 수정본 §14 (§3–5) | FreeRTOS 앱 C 소스 | RTOS 모델, occurs-in-between 관계, 우선순위·동기화 추론을 쓴 data race | 정적 | 논문 기준 (이 노트는 정량 결과를 쓰지 않음) | 플랫폼 우선순위가 가능한 interference를 바꾼다는 점 | 메시지 전달과 state snapshot 의미는 없다 |
| Schwarz 등, POPL 2011 (priority ceiling interrupt program) [R32], VMCAI 2014 (정수 변수로 만든 값 의존 동기화) [R33]; Chopra 등 (RTOS kernel high-level race) [R34] | **[미확인]** snippet | OSEK·AUTOSAR 계열 C | 우선순위 ceiling, 손으로 만든 flag 동기화를 고려한 race 분석 | 정적 | – | validity flag 같은 guard를 동기화로 보는 선례 | data race가 목표다. 메시지 유래 state는 다루지 않는다 |
| Goblint HEAD ([mHP.ml @d0ee7d5b](https://github.com/goblint/analyzer/blob/d0ee7d5b0e2b6c12f0d33a285973397ab29eb9fa/src/cdomains/mHP.ml#L10-L63)) | **[확인된 사실]** 소스 | C (pthread) | thread-modular abstract interpretation과 memory location별 race. MHP domain은 현재 thread id, 만든 thread, 반드시 join된 thread를 추적한다. `definitely_not_started`는 현재 thread가 조상이면서 아직 만들지 않은 자손과의 병행을 배제한다. pthread 의미는 `libraryFunctions.ml` L487-523에 하드코딩되어 있다. `ana.thread.wrappers` 옵션이 있다 (`$N/rw-mlir/web/goblint_libraryFunctions.ml`) | 정적 | 가정 아래 sound | thread 생성 순서를 HB 원천으로 쓰는 선례. `OS_TaskCreate`를 wrapper로 등록할 수 있다 (**[해석]**) | 메시지 순서 HB는 없다. 공유 메모리 부분의 정적 기준선으로 쓴다 (1.6절 RQ5) |

#### 4.3.3 이벤트·메시지·actor

| 연구·도구 | 확인 수준 | 입력 | 속성 | 정적/동적 | 보장 | 겹침 | 간극 **[해석]** |
| --- | --- | --- | --- | --- | --- | --- | --- |
| EventRacer (README @07ced1cc [S134]), WebRacer PLDI 2012 [R35], InitRacer OOPSLA 2017 [R36] | EventRacer README **[확인된 사실]**. 논문들은 **[미확인]** snippet | 계측한 WebKit 브라우저의 trace (`ER_actionlog`) | web event race. snippet: ad hoc 동기화를 다루는 'race coverage'. InitRacer의 세 범주: form-input-overwritten, late-event-handler-registration, access-before-definition | 동적 | – | late handler 등록 ≈ 늦은 구독(F1), 정의 전 접근 ≈ F2, guard flag를 동기화로 보는 관점 | 모두 동적이다. 정적 cFS 분석과는 기법이 다르고 결함 정의는 겹친다 |
| DroidRacer [R37], CAFA (PLDI 2014) [R38], SIERRA (ASPLOS 2018) [R39], nAdroid (CGO 2018) [R40] (artifact @db22a7d [S136]) | **[미확인]** snippet. nAdroid artifact는 파일 목록만 | Android 앱 | lifecycle·event queue를 반영한 HB. SIERRA는 lifecycle·GUI event 순서의 자동 생성 모델, action 민감 분석, backward symbolic execution으로 오경고 반증. nAdroid는 callback을 thread로 바꾸어 같은 event loop 위 callback 사이의 순서 위반을 찾는다 (snippet: 앱 27개에서 유해 위반 88개) | DroidRacer·CAFA 동적, SIERRA·nAdroid 정적 | – | 프레임워크 lifecycle 순서를 모델로 복원해 순서 위반을 정적으로 찾는 일반형 | 원노트 §53의 일반형 신규성 문장을 무너뜨린다. 비교는 cFS의 SB·ES·TBL 의미로 좁힌 뒤에만 의미가 있다 |
| Bagherzadeh & Rajan, Order Types, AGERE 2017 [R05] | **[확인된 사실]** 수정본 §14 | activity·future 기반 언어의 프로그램 | 비동기 메시지의 HB와 flow를 type으로 추론하는 message race 분석 | 정적 | 종료성·건전성 증명은 후속 과제 (§3.8) | 메시지 순서의 정적 추론 | 언어 의미가 cFS와 다르다 |
| González-Abril & Vidal, arXiv 2112.12869 v3 [R11] | **[확인된 사실]** 수정본 §14 (preprint) | Erlang 계열 trace | send·deliver·receive 구분, 잠재 message race와 matching 조건 | 동적 | – | 후보와 실제 가능한 수신의 구분 | trace 기반. 출판 상태 미확인 |
| Erlang Dialyzer race 분석 (dialyzer_races.erl @OTP-24.3.4 [S137]; [notes.xml @OTP-25.0](https://github.com/erlang/otp/blob/OTP-25.0/lib/dialyzer/doc/src/notes.xml#L70-L73)) | 코드·release note **[확인된 사실]**. PADL 2010·2011 논문 [R41]은 **[미확인]** | Erlang 코드 | name registry·ETS·mnesia에 대한 check-then-act race (`whereis/register`, `ets lookup/insert`, mnesia dirty read/write) | 정적, 운영 도구에 통합 | – | 메시지 전달 언어의 정적 race 검사가 실제 도구에 들어간 선례 | pub/sub timing이나 메시지 유래 state가 아니다. `-Wrace_conditions`는 Dialyzer 2.1.0에서 들어와 5.0(OTP 25)에서 제거되었다. 제거 이유는 확인하지 않았다 |
| P 언어, MSR-TR-2012-116 (PDF [R26]) | **[확인된 사실]** 초록과 §2 | P로 쓴 상태기계 | 상태마다 deferred·ignored event 집합을 둔다. 둘 다 아닌 event가 오면 'unhandled event' 위반이다. event를 무한히 미루지 못하게 하는 liveness 검사도 있다 | 정적 (model checking) + C 생성 | 모델에 대해 | F1의 필요 순서를 '처리해야 함·미뤄도 됨·버려도 됨'으로 명시하는 형식 | P 모델을 검사한다. 기존 C 코드가 아니다. cFS에는 이런 선언이 없다. 앱별 계약 형식으로 빌려 쓴다 (**[설계 제안]**, 1.5절) |
| ZeroMQ guide ([chapter1.txt L215-L224 @6752d24](https://github.com/booksbyus/zguide/blob/6752d24b215997aa161e6896941bfd6010d4d3f5/chapter1.txt#L215-L224)) | **[확인된 사실]** 문서 | – | PUB–SUB에서 "the subscriber will always miss the first messages that the publisher sends"('slow joiner') | – | – | F1이 잘 알려진 위험이라는 근거 | 검출 도구가 아니다 |

#### 4.3.4 결함 범주별 기존 정의 대응

| 범주 | 가장 가까운 기존 정의·도구 | 확인 수준 | 판단 **[해석]** | cFS에서 남는 고유 부분 |
| --- | --- | --- | --- | --- |
| F1 첫 메시지 유실 | TaxDC m3274(message–computation order violation), InitRacer late-event-handler-registration, P unhandled event, ZeroMQ slow joiner, ROS 2 TRANSIENT_LOCAL | 전문·문서, InitRacer는 snippet | 위험 자체는 잘 알려져 있다 | 경로 없음·목적지 0의 조용한 성공, 사용되지 않는 QoS, startup sync 뒤로 미룬 구독(TO_LAB), 재시작 뒤에도 남는 경로. 소스에서 '반드시 받아야 하는' 첫 메시지가 유실될 수 있는지 판정하는 일 |
| F2 첫 갱신 전 사용 | Lu 등(order violation), PCT Fig. 1(a), TaxDC h5780, InitRacer access-before-definition, guard flag를 동기화로 보는 EventRacer·Schwarz VMCAI 2014 | Lu·InitRacer·Schwarz는 snippet | 교과서적 order violation이다 | payload field → 앱 전역 field의 출처, HK `DataPresent` 같은 정책 guard, 메모리 초기값과 유효 입력의 구별 |
| F3 경계를 넘은 사용 | Burrows–Leino(동기화 범위 기준), Artho ATVA 2004, R07 data age | 전문 / snippet / preprint | 부분적으로만 덮인다 | 경계 β를 이벤트(트리거 메시지)로 정의. header timestamp는 전송 시각이다 (1.2절 F3) |
| F4 다중 stream snapshot | HLDR·view consistency, atomicity type·AVIO, TaxDC atomicity violation, ROS message_filters·rclc LET | HLDR·atomicity는 snippet | 확인한 정의 중 가장 덜 덮인다. run-to-completion handler에서는 끼어듦이 없다 | HLDR 원문과 time-disparity 문헌을 확인하기 전에는 열린 질문으로만 둔다. child task가 같은 state를 쓰면 atomicity 범주로 간다 |
| F5 시작 의존 | Lu 등, TaxDC h5780과 reboot timing, DroidRacer·SIERRA lifecycle, Goblint 생성 MHP, DCatch polling, R06 | 혼합 | order violation 범주로 덮인다 | ES soft sync, `WaitForSystemState`가 호출자 AppState를 먼저 올리는 자기 준비 표시, core 앱과 script 앱의 차이, 다른 앱이 만든 자원(CF #184) (1.2절 F5, 3.3절) |
| F6 Table 수명 | typestate·API protocol 검사기 | 검색하지 않음 | 판단 보류 | handle 단위 잠금, buffering에 따른 갱신 동작, `NEVER_LOADED`에서 header와 구현의 불일치 (1.2절 F6, 3.4절) |
| F7 메시지 간 순서 | R05, R11, Netzer–Miller, DCatch, Dialyzer | R05·R11은 수정본 기록, 나머지 snippet·코드 | message race 일반형은 선행이 있다 | head 삽입에 따른 역순 전달, 송신 반환 전 수신 실행, 버전에 따른 lock 위치(550e7f7d), pipe별 FIFO만 있고 전역 순서 없음 |
| F8 재시작 창 | TaxDC fault·reboot timing, FCatch crash-recovery | 전문 / 사이트 | 재시작 timing 범주는 선행이 있다 | 경로·sequence counter 유지, pipe와 queue 메시지 폐기, `CFE_TBL_ERR_DUPLICATE_NOT_OWNED`(v7.0.0 이후), 옛 AppId 무효 (1.2절 F8) |

---

### 4.4 Cause-effect chain과 data age

| 연구·도구 | 확인 수준 | 입력 | 속성 | 정적/동적 | 보장 | 겹침 | 간극 **[해석]** |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Becker 등, RTCSA 2016, JSA 2017 (DOI 10.1016/j.sysarc.2017.09.004) [R42] | **[미확인]** 원문 미확인. Dortmund E2E 프레임워크의 docstring과 제3자 학위논문 요약(Edmaier, 2024 [R43])만 봄 | 주기 task 집합, 읽기 구간 [Rmin, Rmax]와 data 구간 [Dmin, Dmax], 지식 수준(정보 없음, WCRT, 정확한 schedule, LET) | data propagation tree로 data age 계열 지표(MRDA 등)를 계산한다. 프레임워크 문서는 MRT·MRRT·MDA·MRDA를 정의한다 | 정적 분석 | 주기 task, implicit 통신, local chain 가정 아래 | 원노트의 freshness 관심 | chain 구조와 timing 파라미터가 입력이다. 소스에서 chain을 복원하지 않는다 |
| Günzel 등, RTAS 2021 [R07] | **[확인된 사실]** 수정본 §14 (preprint §II–V) | 비동기 분산 chain, task·실행·시계·통신 가정 | reaction time, data age의 정의와 상한 | 정적 분석 | 가정 아래 상한 | freshness의 정확한 뜻 | actuation까지의 data age와 앱 함수가 입력을 읽는 시점의 나이는 끝점이 다르다 (수정본 §6.5) |
| Dortmund E2E 평가 프레임워크 (README @18347b7 [S138]) | README·코드 **[확인된 사실]**. 동봉 학위논문은 2차 자료 | 분석 구현 모음 | Becker 2016·2017, Hamann 2017(LET, MDA·MRT), Dürr 2019(sporadic), Günzel 2021·2023 | – | – | 분석들의 통신 모델 비교 | 학위논문(2차 자료)은 explicit 통신(job 중 아무 때나 읽고 쓰기)을 가정한 분석은 거의 없다고 쓴다. cFS SB는 queue에서 `ReceiveBuffer` 시점에 읽는다. implicit·LET 모델과 맞지 않는다 (**[해석]**) |
| Casini 등, ECRTS 2019 [R44] (구현 crate ecrts19.rs @6530456 [S139]); ROS 2 executor 문서 ([About-Executors L189-L203](https://github.com/ros2/ros2_documentation/blob/e2388aa72c17598a54fa228df6e9e9a44c5e7aec/source/ROS-Framework/client-libraries/About-Executors/About-Executors.rst#L189-L203)) | crate·문서 **[확인된 사실]**. 논문 **[미확인]** | supply-bound function, request-bound function(arrival curve, WCET), interference 집합, 주어진 chain 구조 | ROS 2 processing chain의 응답시간 상한 (Lemma 1, 3, 4/5, 8; Blass RTSS 2021 포함) | 정적 분석 | 모델 가정 아래 상한 | chain 응답시간 | chain은 입력이고 복원 대상이 아니다. 문서는 기본 executor가 ready 메시지를 "round-robin fashion - but not in FIFO order"로 처리한다고 쓴다. timer 우선순위 제거를 말하는 문서와 rolling `rclcpp` 코드는 대조하지 못했다. executor 의미도 버전에 따라 다르다 |
| Teper 등, RTSS 2022 [R49] (artifact README @59c8cbd [S140]) | artifact README **[확인된 사실]**. 논문 **[미확인]** | ROS 2 chain | 실측 지연, simulation 하한, 분석 상한 | 혼합 | – | data age 분석과 실측의 대조 방식 | cFS에서 같은 일을 하려면 timing 모델이 먼저 있어야 한다 |
| rclc LET ([executor.h L43-L59 @3064baa](https://github.com/ros2/rclc/blob/3064baadeabdaca6dabfae3f8351510bdbe53071/rclc/include/rclc/executor.h#L43-L59)), message_filters ([index.rst L241-L266 @815ce9f](https://github.com/ros2/message_filters/blob/815ce9f9f388762ed79b0571e0157fc0234bd76b/doc/index.rst#L241-L266)) | **[확인된 사실]** 코드·문서 | – | LET: 한 sampling 시점에 모든 ready 구독의 새 data를 가져오고, callback은 그 시점 data를 쓴다. TimeSynchronizer 등: header timestamp가 맞는 N개 메시지를 한 callback으로 준다 | 동적 (라이브러리) | – | F4의 snapshot 일관성을 라이브러리로 제공한다 | cFS에서는 같은 역할을 앱이 손으로 쓴 guard가 맡는다 (HK `DataPresent`). HAROS도 message_filters를 추출 한계로 든다 |
| AUTOSAR TIMEX의 age·reaction LatencyTimingConstraint [S114] | **[미확인]** snippet (autosar.org 차단) | – | – | – | – | – | 원문을 읽지 못했다 |
| Henzinger 등, Giotto, 2003 [R08] | **[확인된 사실]** 수정본 §14 (서지·초록) | time-triggered program | 명시적 시간 명세 | – | – | 시간 관계를 코드 이름으로 추정할 수 없다는 비교 | cFS가 Giotto의 실행 의미를 갖는다는 가정은 하지 않는다 |

**판단.**

- **[해석]** data age·reaction 분석은 chain 구조와 period·offset·WCET·통신 의미를 입력으로 받는다. 소스에서 chain을 복원하지 않는다. 이 연구가 복원하는 MID chain과 SCH 주기는 그런 분석의 입력이 될 수 있다. 그 연결은 후속 작업이고 이 연구의 결과가 아니다.
- **[확인된 사실]** 기본 `CFE_MSG_OriginationAction`은 IsOrigination=true인 telemetry의 header 시각을 전송 시점으로 덮어쓴다 ([cfe_msg_integrity.c L30-L53](https://github.com/nasa/cFE/blob/546a002515be5a1e3b66f9ae2c14f948d9cec76f/modules/msg/fsw/src/cfe_msg_integrity.c#L30-L53)). **[해석]** header 시각으로 data age를 재면 측정 시각이 아니라 발행 시각을 재게 된다.
- **[해석]** 그래서 이 연구의 F3·F4는 이벤트 경계로 정의한 정성 속성으로 둔다. 물리 시간 상한은 범위 밖이다 (1.4절).
- **[미확인]** 다중 입력 사이의 시간 차(time-disparity) 문헌은 검색하지 않았다. F4의 신규성 판단은 이 검색 전까지 보류한다.

---

### 4.5 MLIR·ClangIR·대안 분석 기반

#### 4.5.1 MLIR 계열

| 기반·연구 | 확인 수준 | 입력 | 제공하는 것 | 겹침 | 간극 **[해석]** |
| --- | --- | --- | --- | --- | --- |
| MLIR [R10]과 DataFlow framework ([DataFlowFramework.h L300 @ccac700c](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/include/mlir/Analysis/DataFlowFramework.h#L300), [Utils.h L23-L32](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/include/mlir/Analysis/DataFlow/Utils.h#L23-L32), [DeadCodeAnalysis.cpp L192, L421-L447](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/lib/Analysis/DataFlow/DeadCodeAnalysis.cpp#L421-L447), [DenseAnalysis.h L163-L173](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/include/mlir/Analysis/DataFlow/DenseAnalysis.h#L163-L173)) | **[확인된 사실]** 코드 (local 1053047a와 main ccac700c) | 임의 dialect IR | sparse·dense, forward·backward 분석과 fixpoint solver. `interprocedural`의 기본값은 true다 | 원노트 §8의 solver 기반 | DeadCodeAnalysis와 SparseConstantPropagation을 먼저 load해야 한다. 코드는 이를 "interim fix"라 부른다. 호출 지점을 join하는 context-insensitive 분석이다. public symbol은 predecessor를 모르는 것으로 본다. 몸체 없는 callee는 entry state로 간다. 간접 호출(non-symbol callable)은 TODO다. header에 thread·interleaving 개념이 없다. 공식 tutorial [S14]은 현재 header에 없는 `ForwardDataFlowAnalysis`·`LatticeElement`를 설명한다 |
| alias·effect 기반 ([LocalAliasAnalysis.cpp L100-L106, L343-L351, L497-L501](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/lib/Analysis/AliasAnalysis/LocalAliasAnalysis.cpp#L100-L106), [LLVMOps.td L294-L298, L819-L824](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/include/mlir/Dialect/LLVMIR/LLVMOps.td#L819-L824), [SideEffectsAndSpeculation.md L78-L121](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/mlir/docs/Rationale/SideEffectsAndSpeculation.md#L78-L121)) | **[확인된 사실]** 코드·문서 | LLVM dialect 등 | `LocalAliasAnalysis`는 `mlir/` 안의 유일한 구현이고 기본으로 등록된다. llvm-project 전체에는 Flang의 FIR 전용 `fir::AliasAnalysis`와 ClangIR의 `CIRBasicAliasAnalysis`도 있다. effect는 resource 위계 위에 정의되고, non-addressable resource는 pointer 메모리와 alias하지 않는다. main의 `getModRef`는 non-addressable resource effect를 NoAlias로 본다 | 메모리 모델의 기반 | GEP는 `ViewLikeOpInterface`라 base로 풀린다(field 구분 없음). `llvm.mlir.addressof`는 ConstantLike라 두 상수는 MayAlias다. 코드의 TODO가 이것이 지나치게 보수적이라고 인정한다. `llvm.call`은 `MemoryEffectOpInterface`가 없어 ModRef다. 문서는 effect 기구가 "deliberately *not* intended for fine-grained regions ... or offset-based disambiguation"이라고 쓴다 |
| ClangIR (S17; [index.md L3-L7 @ccac700c](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/clang/docs/CIR/index.md#L3-L7), [CIRBasicAliasAnalysis.cpp L236-L245](https://github.com/llvm/llvm-project/blob/ccac700c68e92a7eabcb2f745789978213f16f2d/clang/lib/CIR/Dialect/Analysis/CIRBasicAliasAnalysis.cpp#L236-L245)) | 문서·코드 **[확인된 사실]**. 로컬 **[실측]** | C/C++ AST | 구조적 고수준 IR(`cir.if/switch/scope/while/for`). `cir.get_member`가 field 이름을 유지한다. main에는 2026년 8–9월에 들어온 `CIRBasicAliasAnalysis`가 있다: 같은 base 안의 byte offset으로 field를 구분하고, 서로 다른 `cir.alloca`만 NoAlias로 증명한다 | 원노트 §10의 장기 frontend | main에서도 opt-in이다: "ClangIR is not included in a default clang build". `CLANG_ENABLE_CIR`는 `option()` 선언이 없어 기본 off다. 위 alias 분석은 "TODO: Extend to cover global addresses"라 전역은 아직 구분하지 않는다. `cir.call`도 `MemoryEffectOpInterface`가 없다. incubator `llvm/clangir`는 HEAD 63412d47("[CIR] Incubator is now closed", 2026-02-20)로 닫혔고, 그 LifetimeCheck pass(`cir-lifetime-check`)는 ccac700c의 upstream에 없다. **[실측]** 로컬 clang은 `-fclangir -emit-cir`를 "rebuild clang with -DCLANG_ENABLE_CIR=ON"으로 거부한다. `-fclangir -S -emit-llvm`은 성공하지만 출력이 일반 codegen과 byte 단위로 같다. 플래그가 조용히 무시된다 (`$N/probe/cir/emit_cir.log`, `$N/rw-mlir/cir_test/clangir_local_test.log`) |
| VAST·Macroni (trailofbits/vast [S143], HEAD 60d605d0; `$N/rw-mlir/web/trailofbits_vast_README.md`, `vast_vast-detect-parsers.md`) | **[확인된 사실]** README·`.td`. 빌드하지 않음 | C/C++ (LLVM 19 필요) | MLIR 'tower of IRs'. `hl` dialect가 field 이름(`hl.member`), typedef, 간접 호출을 유지한다. `vast-detect-parsers`는 YAML 함수 모델로 C를 domain 'parser dialect'로 올린다. Macroni는 macro 확장을 더하지만 PASTA 전용 clang이 필요하다(마지막 commit 2024-07-02) | 함수 모델 기반 domain dialect lifting의 선례 | 'dialect를 쓴다'는 사실은 신규성이 아니다. LLVM 버전을 따로 고정해야 한다 |
| PoTATo (Jezurko/potato [S144], 2026-03-25; `$N/rw-mlir/web/Jezurko_potato_README.md`) | README **[확인된 사실]**. 빌드 **[실측]** | MLIR (`--llvm-ir-to-potato`) | MLIR DataFlow 위의 Andersen·Steensgaard points-to. README는 LLVM 19에서 시험했다고 쓴다 | 간접 호출 해석의 후보 | **[실측]** commit 80d157c·20e7d8f를 로컬 LLVM 1053047a에 빌드하면 `RegionSuccessor`·DataFlow API 오류로 실패한다 (`$N/rw-mlir/potato_build.log`, `potato_build2.log`). MLIR 분석 API의 변동은 실제 유지 비용이다 |
| SYCL-MLIR (Tiotto 등, CGO 2024 [R50]; AliasAnalysis.h @baecca1 [S145]) | 소스 **[확인된 사실]**. 논문 **[미확인]** | SYCL | runtime 의미를 담은 domain dialect, `LocalAliasAnalysis`를 상속한 domain alias 규칙 | domain 의미 + MLIR 분석 기반의 선례 | 대상이 SYCL host/device다 |
| Polygeist (README @77c04bb2 [S146]) | **[확인된 사실]** README·commit | C | affine·polyhedral MLIR로 올리기 | – | 마지막 commit이 2024-07-31이고 자체 llvm-project를 고정한다. 제어·state 코드 분석에는 맞지 않는다 (**[해석]**) |
| Fehr 등, PLDI 2025 (DOI 10.1145/3729309) [R51]; Peng 등, POPL 2026 (DOI 10.1145/3776722) [R52] | **[미확인]** snippet | MLIR dialect 의미 | SMT 의미 dialect, translation validation, dataflow transfer 함수의 건전성. 합성·검증된 KnownBits·ConstantRange transformer | cfs dialect의 transfer 함수 건전성 논증에 쓸 수 있다 | C 응용 코드 분석이 아니다. MLIR로 embedded·flight C의 메시지 순서나 state 출처를 분석한 동료심사 논문은 찾지 못했다 (검색은 비체계적) |

#### 4.5.2 대안 분석 도구

축은 네 가지다. (a) pub/sub 그래프 복원, (b) payload → state 출처, (c) 앱 간 순서·HB, (d) 소스 위치 진단. **[해석]** 아래 판정은 문서·소스를 읽은 결과이고, Clang Static Analyzer 말고는 cFS에서 실행하지 않았다.

| 도구 | 확인 수준 | (a) | (b) | (c) | (d) | 비고 |
| --- | --- | --- | --- | --- | --- | --- |
| CodeQL ([DataFlowPrivate.qll L93-L120 @f1d3f1de](https://github.com/github/codeql/blob/f1d3f1defc5ca73bcf1ed48031fb31c62fd91952/cpp/ql/lib/semmle/code/cpp/ir/dataflow/internal/DataFlowPrivate.qll#L93-L120)) | **[확인된 사실]** 소스·문서. 실행 안 함 | `isAdditionalFlowStep`으로 같은 상수 MID의 `TransmitMsg`→`ReceiveBuffer` 간선을 더할 수 있다 (**[설계 제안]**) | 가능. global data flow, data-extension model(문서상 'Beta ... subject to change') | HB·interleaving 모델이 없다. `jumpStep`은 전역을 통해 함수 사이를 순서와 무관하게 잇는다 | 가능 | cFS CI가 이미 CodeQL을 돌린다. ISSTA 2025 보고(DOI 10.1145/3728923) [R53]는 embedded 프로젝트 258개에서 진짜 결함 709개, 오경고 34%라고 한다(snippet). 'why not CodeQL'에 대한 가장 강한 기준선이다 |
| Clang Static Analyzer | **[실측]** probe 프로그램. 문서 **[확인된 사실]** | 모델 없이는 불가 | 한 경로 안에서, custom 모델이 있을 때 | 없음 | 가능 | **[실측]** YAML taint 설정(`CFE_SB_ReceiveBuffer`를 propagation, `Control`을 sink)으로 시험했다. int payload 직접 사용은 보고했다 (`rx_direct_int.c:6`, `:7`). double payload는 보고하지 않았다. 다음 `ReceiveBuffer` 같은 불투명 호출을 지나면 `static` 전역에서도 taint가 사라졌다. 불투명 호출이 없을 때만 보고했다 (`inval.c:7`) (`$N/rw-mlir/csa_exp/csa_all.log`). double 미보고의 원인은 `SValBuilder.cpp:73`의 "FIXME: Handle floats"로 추정되나 증명하지 않았다 |
| PhASAR (README @41d97dab [S147]) | README **[확인된 사실]** | 모델 필요 | IFDS/IDE taint와 CFE_* summary | README에 동시성 언급 없음 | LLVM debug location | LLVM 16–22.1. 사용자가 entry point 목록을 준다 |
| SVF (6a4bb08f [S148]; `$N/rw-mlir/web/svf_ThreadAPI.cpp` L108-117) | README·소스 **[확인된 사실]** | 모델 필요 | field·flow sensitive points-to, memory SSA | MTA는 pthread fork/join MHP와 lock 분석이다. thread API는 `pthread_create`→`TD_FORK`, `pthread_mutex_lock`·`sem_wait`→`TD_ACQUIRE`로 하드코딩되어 있다 | LLVM debug location | 함수 포인터 해석이 강하다. 그 결과를 MLIR 파이프라인에 넣는 것도 가능하다 (**[해석]**) |
| Joern (98a815cb [S149]; `$N/rw-mlir/web/joern_joern-cli_frontends_c2cpg_README.md`) | README **[확인된 사실]** | MID 상수에 대한 graph query로 가능 | 근사 dataflow | 없음 | 가능 | Eclipse CDT 기반 `c2cpg`, `compile_commands.json` 지원. 해석하지 못한 코드는 problem node가 된다 |
| Frama-C 33.0 'Arsenic' [S150] (`$N/rw-mlir/web/framac_33_opam`) | opam 설명 **[확인된 사실]**. 공식 site 차단 | stub·ACSL 필요 | Eva | 설명에 동시성 plugin이 없다. Mthread 존재 여부는 **[미확인]** | 가능 | Aoraï로 앱 내부 호출 순서(예: Subscribe 전 Receive 금지)를 검사할 수 있다 (**[해석]**). RacerF(ECOOP 2025, pthread data race) [R54]는 snippet만 봄 |
| IKOS | 4.1.1 | stub 필요 | 값 분석 | entry point를 독립 프로세스처럼 분석 | 가능 | 4.1.1 참조 |
| Goblint HEAD | 4.3.2 | 없음 | 값 분석 | pthread lock 기반 공유 메모리만 | 가능 | 공유 메모리 부분의 정적 기준선 |

#### 4.5.3 upstream이 주는 것과 주지 않는 것 (실측)

| 실험 | 명령·입력 | 결과 **[실측]** | 함의 **[해석]** |
| --- | --- | --- | --- |
| alias | `mlir-opt alias_llvm.mlir -pass-pipeline='builtin.module(llvm.func(test-alias-analysis))'` (local 1053047a) | `g_state#0 <-> g_other#0: MayAlias`, `g_state.att#0 <-> g_state.valid#0: MustAlias`, 전역 대 alloca는 NoAlias (`$N/rw-mlir/mlir_probe/alias_llvm.out`) | 서로 다른 전역도, 한 struct의 서로 다른 field도 구분하지 못한다. 원노트 §44의 `global.att` 추적에는 연구자가 만든 메모리 모델이 필요하다 |
| modref | `-test-alias-analysis-modref` | 외부 `llvm.call @CFE_ES_PerfLogAdd`가 모든 위치에 `ModRef`. `.att`로의 store가 `.valid`에 `Mod`. `memory_effects`가 모두 none인 call도 `ModRef` (`$N/rw-mlir/mlir_probe/modref_llvm.out`, `$N/verify-C20/modref_memattr.myout`) | 모델링하지 않은 CFE 호출 하나가 모든 C 메모리를 덮어쓴 것으로 처리된다 |
| visibility | `-test-dead-code-analysis` | import된 `llvm.func internal @SetState`는 `op_preds: predecessors:`(미지), `func.func private`는 `op_preds: (all) predecessors:` (`$N/rw-mlir/mlir_probe/dca_vis.out`). importer는 `sym_visibility`를 설정하지 않는다. 손으로 `private`를 붙이면 `(all)`로 바뀐다 (`$N/verify_C20_overreach/st_dca_priv.out`) | closed-world 전처리 pass가 필요하다. 작은 pass로 해결된다 |
| 간접 호출 | const 함수 포인터 table `Table[i](m)`, `-test-print-callgraph`, -O0·-O2 | `Dispatch`의 간선은 `<Unknown-Callee-Node>` 하나뿐 (`$N/rw-mlir/cir_test/t2.callgraph.out`) | points-to(PoTATo, SVF, 자체 구현)가 필요하다. 연결된 mission module에 간접 `llvm.call`이 73곳 있다 (SBN·CF·ES pool 중심) |
| 마지막 쓰기 | `-test-last-modified`, 원노트 §44 예제 (손으로 쓴 판과 clang import 판) | 기본, `interprocedural=false`, `assume-func-writes=true` 모두에서 `Guidance`의 읽기가 `<unknown>`. 같은 SSA 주소의 store–load만 찾았다 (`$N/rw-mlir/mlir_probe/lastmod_llvm.out`, `lastmod_llvm_iso.out`, `$N/verify-C20/s44_lastmod.myout`) | upstream 시험 분석은 SSA 값으로 메모리를 식별한다. §44 수준의 추적은 이 연구가 만들어야 할 부분이다 |
| IRDL | `mlir-opt --irdl-file`로 장난감 `cfs` dialect 선언 | 구조 검증은 된다 ("op expects exactly 2 operands, but got 3"). IRDL op에는 `MemoryEffectOpInterface`가 없다 (`$N/rw-mlir/mlir_probe/cfs_irdl.out`) | effect를 쓰려면 ODS·C++ 정의가 필요하다 |
| import | `clang -O0 -g` → `mlir-translate --import-llvm` | 154개 파일(cFE module 5개, 앱 디렉터리 17개)을 진단 0개로 import했다. clang으로 설정한 별도 build tree에서 `-Werror`를 빼고 컴파일한 결과다. 연결 module은 정의 함수 2389개, 외부 선언 184개, 간접 호출 73곳이다. 조건과 실패 파일은 §7.5.2 | import 경로는 막히지 않는다. 비용은 import가 아니라 의미 복원에 있다 |
| SB 전달 경로 | 연결 module 검사 | SB 전달은 `OS_QueuePut`·`OS_QueueGet`에서 끝나며 둘 다 몸체 없는 선언이다 (`$N/rw-mlir/import_exp/mission.mlir` L57018, L61947) | 발행자에서 수신 handler까지 직접 call·def-use 경로가 없다. SB 모델은 어떤 도구를 쓰든 필요하므로 MLIR의 차별점이 아니다 |
| MID 상수 | `-O1 -Xclang -disable-llvm-passes` → `mlir-opt --inline --sroa --mem2reg --canonicalize --cse` | 코드에서 정의한 MID가 call site 상수가 된다. 나머지는 table·명령 payload에서 온다. macro 이름은 남지 않는다. 앱별 회수 수는 §7.7.3 | MID 이름 복원에는 AST·preprocessor 보조 경로나 값→이름 표가 필요하다 |

#### 4.5.4 이 무리에서 얻는 판단

- **[해석]** upstream MLIR이 주는 것은 IR 기반, solver, 위치 보존이다. 다음은 모두 연구자가 만들어야 한다. field·전역을 구분하는 메모리 모델, closed-world visibility, 간접 호출 해석, MID·field 이름 복원, SB·ES·TBL 모델, task 사이의 HB 층.
- **[해석]** cfs dialect가 더할 수 있는 것은 다섯 가지다. (1) SB·ES·TBL 호출을 typed op로 만들어 MID·pipe·status·timeout을 operand로 드러낸다. 반환값 누락과 미지의 MID가 verifier에서 보인다. (2) pipe·MID queue·table buffer·ES state를 non-addressable resource에 대한 effect로 표현해, 모델링한 호출이 C 메모리 전체를 덮어쓰지 않게 한다. (3) app·task·run loop를 region으로 표현해 task 안의 dataflow와 task 사이의 순서를 분리한다. (4) solver를 재사용한다. (5) 진단의 소스 위치를 유지한다.
- **[해석]** 이 이점은 (c) 앱 간 순서 판정과 확장 비용에서 보여야 한다. (a)·(b)만으로는 같은 SB 모델을 넣은 CodeQL·PhASAR·SVF와 구별되지 않는다. 함수 모델로 domain dialect를 만드는 일은 VAST와 SYCL-MLIR이 이미 했다.

---

### 4.6 구성 요소별 선행 범위

| ID | 이 연구의 구성 요소 | 가장 가까운 선행 | 선행이 다루는 범위 | 판정 **[해석]** | 근거 수준 |
| --- | --- | --- | --- | --- | --- |
| K1 | **[원노트 구상]** 코드 literal MID의 pub/sub 간선 복원 | ROSDiscover, cfs-msgid-guard, SPLC 2009, WannaFly | 소스 → topology, MID 추출 | 선행 있음 | 전문·코드, 일부 snippet |
| K2 | **[설계 제안]** table 이미지·지상 명령·table 갱신 재구독을 포함한 구독 집합과 그 시간 변화 | ROSDiscover(launch file, quasi-static), HAROS(고정 구독) | 정적 구성 파일 | 부분. cFS의 table·명령 기반 동적 구조를 다룬 도구는 찾지 못함 | 전문 |
| K3 | **[설계 제안]** build 설정·EDS에 따른 MID 해석 | EDS 선언, ROS remap | 이름 해석 | 부분. cFS 고유의 공학 문제 | 코드 |
| K4 | **[원노트 구상]** payload field → 앱 전역 field → 소비 위치의 출처 | ROSInfer(state 변수, 내용 미모델링), CodeQL global data flow, CSA taint, IKOS 값 분석 | 변수 수준, flow-insensitive, 한 경로 안 | 부분. field 단위 출처와 메시지 식별의 결합은 찾지 못함 | 전문·코드·**[실측]** |
| K5 | **[설계 제안]** guard·정책 인식 (validity flag, `DataPresent`, active/passive, 지연 구독) | EventRacer race coverage, Schwarz VMCAI 2014, ROSInfer의 조건 추론 | ad hoc 동기화, 값 의존 동기화 | 일반형은 선행. cFS 정상 대조 집합(1.6절 RQ4)은 새 공학 | snippet·전문 |
| K6 | **[원노트 구상]** 구독 완료와 첫 발행의 순서 (F1) | TaxDC 범주, P unhandled event, InitRacer, ZeroMQ, ROS 2 QoS, ROSpec | 위험의 정의, 동적 검출, 설정 검사 | 위험은 선행. cFS 소스에서 정적으로 판정한 작업은 찾지 못함 | 전문·문서·snippet |
| K7 | **[설계 제안]** ES 시작 의미(soft sync, 자기 준비 표시, core 직렬화)에 기반한 F5 판정 | DroidRacer·SIERRA lifecycle, Goblint 생성 MHP, DCatch polling, F′ 고정 setup | lifecycle HB | 일반형은 선행. cFE ES 의미 모델은 찾지 못함 | snippet·코드 |
| K8 | **[설계 제안]** 목적지별 전달 순서, 송신 반환 전 수신, 버전별 lock 위치 (F7) | R05, R11, DCatch, Netzer–Miller | message HB | 일반형은 선행. cFE 전달 의미는 고유 | 수정본 기록·snippet |
| K9 | **[원노트 구상]** TBL handle 단위 잠금 typestate, buffering 의존 (F6) | typestate·API protocol 문헌 | – | 검색하지 않음. 판정 보류 | **[미확인]** |
| K10 | **[원노트 구상]** 재시작 창: 경로 유지, pipe 폐기, `DUPLICATE_NOT_OWNED` (F8) | TaxDC reboot timing, FCatch | 분산 시스템 재시작 | 범주는 선행. cFE 재시작 의미는 고유 | 전문·사이트 |
| K11 | **[원노트 구상]** run-to-completion handler에서 epoch가 다른 입력 조합 (F4) | HLDR, atomicity, message_filters·LET | 공유 메모리 view, 라이브러리 동기화 | 확인한 정의 중 가장 덜 덮임. HLDR 원문과 time-disparity 문헌은 미확인 | snippet |
| K12 | 물리 시간 freshness | R07, Becker, Casini | 시간 상한 | 선행 있음. 이 연구 범위 밖 | preprint·2차 |
| K13 | **[원노트 구상]** typed cfs dialect, non-addressable resource effect, solver 재사용 | VAST parser dialect, SYCL-MLIR | 함수 모델을 쓴 domain lifting | 'dialect 사용'은 선행. cFS 순서 판정에 쓴 사례는 찾지 못함 | 코드·snippet |
| K14 | **[설계 제안]** K2·K4·K6–K10을 cFS C 소스 위 하나의 정적 분석으로 결합 | fprime-topo-analysis(F′, lock·race·queue), ROSInfer(ROS, 상태기계) | 다른 framework에서의 결합 | 찾지 못함. 가장 유력한 기여 후보 | 검색 한정 |

---

### 4.7 신규성 경계

#### 4.7.1 분명한 선행: 기여로 주장하지 않는 것

1. 소스에서 pub/sub topology를 복원하는 일. ROSDiscover가 Clang으로 했고, cFS 구조와 규칙의 대조는 SPLC 2009(snippet)가 했다. MID 추출은 작은 도구가 있다.
2. 메시지 callback이 state 변수를 바꾸고 그 변수가 발행을 조건화하는 상태기계를 추론하는 일, 그리고 '필요 입력 미도착' 검사. ROSInfer가 했다.
3. 프레임워크 연결 모델과 소스의 handler 흐름을 결합한 동시성 분석. fprime-topo-analysis가 F′에서 했다.
4. '필요 순서 spec을 얻고 그 위반을 정적·동적 분석으로 찾는다'는 골격. TaxDC §8.4가 제안했다. 원노트 §47의 식은 이 골격의 한 형식화다.
5. F1·F2·F5·F7·F8이 속하는 결함 범주. order violation, message race, event race, reboot timing은 TaxDC, Lu 등(초록), PCT, P, ZeroMQ, R05, R11에서 이미 정의된다.
6. stale-value 오류의 정적 검출. Burrows–Leino가 동기화 범위 기준으로 했다.
7. framework stub과 sound abstract interpretation을 cFS 앱에 적용하는 일. IKOS(수치는 snippet).
8. RTOS API 호출 인식으로 process·동기화 모델을 추출하는 일. Goblint의 ARINC 분석(메시지 port는 제외).
9. 함수 모델로 C를 domain dialect로 올리는 일과 MLIR의 generic solver. VAST, SYCL-MLIR, MLIR 자체.
10. cFS 동시성 시험 [R03], 설계 모델 검사(TASTE), runtime monitor(R09·Ogma), 일반 정적 분석 CI.
11. late joiner·첫 메시지 유실의 위험과 framework 해결책. ZeroMQ, ROS latch, DDS durability.
12. chain 구조가 주어졌을 때의 data age·reaction 상한. R07, Becker, Casini.

#### 4.7.2 그럴듯하게 새로운 것: 검증할 가설

| ID | 가설 **[해석]** | 그럴듯한 이유 | 반박 조건 |
| --- | --- | --- | --- |
| N1 | cFE의 전달·lifecycle 의미를 필요 순서 `Req`와 보장 관계 `G`의 출처로 쓰는 정적 판정 | **[확인된 사실]** cFE에서 경로 없음, 목적지 0개, MsgLim, pipe full이 모두 발신자에게 `CFE_SUCCESS`다 (1.2절 F1·F7). ES 동기화는 soft limit이고 호출자 자신을 준비 상태로 표시한다 (F5). TBL 잠금은 handle 단위이고 buffering에 따라 갱신 동작이 다르다 (F6). 재시작은 경로를 남기고 pipe를 버린다 (F8). 이 의미를 모델로 쓴 분석은 확인한 범위에서 찾지 못했다 | R03·SPLC 2009·R04 본문이 같은 의미를 모델링했음이 드러나면 약해진다 |
| N2 | table 이미지·지상 명령·table 갱신을 포함해 시간에 따라 바뀌는 구독 집합을 분석 입력으로 쓰는 것 | **[실측]** 구독 site 47곳 중 8곳이 data·명령 구동이고 TO_LAB table은 37개 항목이다. ROSDiscover는 quasi-static을 가정한다 | 단독으로는 공학이다. N1·N3과 결합할 때만 기여로 센다 |
| N3 | payload field 단위 출처와 순서 판정의 결합 | ROSInfer는 내용을 모델링하지 않는다. CodeQL의 전역 흐름은 순서와 무관하다. CSA는 불투명 호출에서 taint를 잃는다 (**[실측]**) | 같은 SB 모델을 넣은 CodeQL·PhASAR 구현이 같은 후보 집합을 내면 기각한다 (1.6절 RQ5·RQ6) |
| N4 | 정상 대조(TO_LAB 지연 구독, HK `DataPresent` 정책, Ogma passive 입력, 직렬화된 core 시작)를 경고하지 않는 guard·정책 인식 | **[확인된 사실]** 정상 대조 네 가지가 모두 공개 코드에 있다 (1.2절) | 앱별 계약 없이는 구별되지 않으면, 기여는 '계약 검사'로 줄어든다 |
| N5 | F4: run-to-completion에서 epoch가 다른 입력 조합의 정적 판정 | 확인한 정의 중 직접 적용되는 것이 없다 (4.3.4) | HLDR 원문이나 time-disparity 문헌이 이를 이미 정의하면 기각한다. 확인 전에는 신규로 주장하지 않는다 |
| N6 | cFE 버전과 Σ를 매개변수로 둔 판정 | **[확인된 사실]** 버전에 따라 의미가 바뀐다: 550e7f7d(lock 위치, v7.0.0), c1ab1b7(TBL 재등록, v7.0.0), d3d52da(TO_LAB 지연 구독, v7.0.1) (1.3절). **[실측]** 같은 코드에서 relay 역전이 1 CPU 500/500, 4 CPU 0/500이었다 | 이를 반영한 선행이 발견되면 기각한다. 단독으로는 실험 설계 원칙이다 |

#### 4.7.3 미확인: 주장 전에 닫아야 할 항목

| 항목 | 중요한 이유 | 막힌 이유 | 닫는 방법 **[설계 제안]** |
| --- | --- | --- | --- |
| R03 ISSRE 2016 본문 | SB 모델이 앱 순서나 state를 다루면 N1이 약해진다 | IEEE·Fraunhofer·slide host 차단 | 기관 사본이나 저자 사본 확보, 다른 네트워크에서 접근 |
| SPLC 2009 본문, FSW-08 발표(NTRS 20090004613) | MID·pipe 간선을 복원했는지 | NTRS·ucsc host 403 | NTRS 접근 |
| WCRE 2010 본문 | pub/sub 행동 검사의 범위 | NTRS·IEEE 차단 | 같음 |
| R04 Valente 전체와 artifact | TASTE 모델 검사와 연계했는지 | ACM·UPM 차단 | 출판사 PDF |
| IV&V 2020 §2.1–2.2 [S07] | 순서·startup 결과를 보고했는지 | NTRS 차단 (수정본은 읽었다고 기록) | 수정본 기록을 원문과 다시 대조 |
| IKOS–BioSentinel slide | CFE model 비용(약 1200 LOC) | NTRS 차단 | NTRS 접근 |
| ROSInfer TLA+ 생성기 코드와 2025 학위논문 | 구독 전 발행·timing 확장이 이미 있는지 | 기본 branch에 없음, 학위논문 접근 불가 | 저자 저장소의 다른 branch·tag, 학위논문 |
| HAROS ENASE 2022 (HPL) [R48] | 시간 속성을 정적으로 검사하는지 | 열람하지 않음 | 저자 사이트 사본 |
| Artho 등 HLDR(STVR 2003), ATVA 2004 | F4 (N5) | kth.se·springer 차단 | 저자 PDF |
| time-disparity 문헌, AUTOSAR TIMEX | F4·F3 | 검색하지 않음, autosar.org 차단 | 실시간 학회 문헌 검색 |
| typestate·API protocol 문헌 | F6 (K9) | 검색하지 않음 | 검색 |
| SIERRA·nAdroid·DCatch·InitRacer 본문 | lifecycle HB·polling 규칙의 정확한 범위 | ACM·uchicago·cs.au.dk 차단 | 저자 PDF |
| GitHub 전체 code search | 놓친 cFS 분석 도구 | MCP code search timeout | 재시도 |
| Frama-C 현재 문서 | 동시성 plugin(Mthread) 여부 | frama-c.com 차단 | 공식 문서 |

#### 4.7.4 신규성 문장 **[설계 제안]**

원노트 §53의 문장을 다음으로 바꾼다.

> 본 연구의 기여 후보는 race 검출 일반이나 프레임워크 의미 복원 일반이 아니다. cFS C 소스에 대해 다음 네 가지를 하나의 정적 분석으로 결합하는 것이다. (1) table 이미지, 지상 명령, build 설정까지 포함해 시간에 따라 바뀌는 Software Bus 구독 집합을 복원한다. (2) 수신 payload field에서 앱 state field와 그 소비 위치까지의 출처를 field 단위로 추적한다. (3) cFE의 전달 의미(발신자에게 보이지 않는 목적지별 실패, 구독 순서에 따른 전달 순서, cFE 버전에 따른 잠금 구조)와 ES·TBL lifecycle 의미(soft startup 동기화와 자기 준비 표시, handle 단위 table 잠금, 재시작 창)를 필요 순서와 보장 관계의 출처로 써서 앱 간 순서·시간 의존 결함 후보를 판정한다. (4) 설계로 허용된 지연 구독과 결합 정책을 결함과 구별한다. 확인한 범위의 선행에서는 이 결합을 찾지 못했다. 이 판단은 4.7.3의 본문을 읽기 전까지 잠정적이다.

> The candidate contribution is neither race detection nor framework-semantics recovery in general. It is a static analysis of cFS C source that combines four things: (1) it recovers the time-varying Software Bus subscription set, including table images, ground commands and build configuration; (2) it tracks field-level provenance from received payload fields to application state fields and their uses; (3) it decides cross-application ordering and temporal-dependency fault candidates, using cFE delivery semantics (per-destination failures invisible to the sender, subscription-order-dependent delivery, version-dependent locking) and ES/TBL lifecycle semantics (soft startup synchronization with self-declared readiness, per-handle table locking, restart windows) as the sources of required and guaranteed orderings; and (4) it separates designed deferral and combination policies from faults. We did not find this combination in the prior work we could check. This judgement is provisional until the full texts listed in §4.7.3 are read.

---

### 4.8 예상 심사 반론과 답변

| # | 반론 | 답변 **[해석]** | 근거 | 필요한 실험·자료 **[설계 제안]** |
| --- | --- | --- | --- | --- |
| 1 | "cFS 동시성 분석은 이미 있다 (ISSRE 2016, SPLC 2009)." | 인정한다. R03은 SB 서비스의 요구사항 적합성 시험이고(초록), SPLC 2009는 compile-time 아키텍처 규칙 검사다(snippet). 이 연구의 입력은 앱 C 소스이고 속성은 앱 간 순서와 state 출처다. 두 본문을 읽기 전에는 "최초"라는 말을 쓰지 않는다 | 4.1.1 | 두 본문을 확보해 4.1.1 표를 갱신한다. R03 모델이 같은 SB 의미를 쓰는지 대조한다 |
| 2 | "ROSDiscover·ROSInfer를 cFS로 옮긴 것이다." | topology 복원과 state 변수 추론은 선행으로 인정한다. 차이는 4.2.2의 여덟 가지 cFS 차이와, ROSInfer가 스스로 밝힌 공백(내용 미모델링, race·timing 불가, 수신 status 없음, 구독 전 발행 없음)이다 | 4.2 | ROSInfer식 모델(변수 수준, 내용 없음, 순서 없음)을 cFS에 구현한 기준선과 비교한다. field 출처를 끄는 ablation을 한다 (1.6절 H3′) |
| 3 | "fprime-topo-analysis가 이미 같은 일을 한다." | 그 도구는 F′를 대상으로 lock·data race·queue 우선순위를 본다. runtime guard와 runtime routing은 비목표다. F′는 wiring을 task 시작 전에 고정하지만 cFS 구독은 runtime 호출이다. 이 도구를 구조적 선례로 인용하고, 'topology + handler 흐름 결합' 자체를 기여로 주장하지 않는다. 동료심사 출판은 찾지 못했다 | 4.1.2 | 같은 결합 방식(정적 topology + handler 흐름, 순서 판정 없음)을 cFS에 구현한 기준선을 둔다 |
| 4 | "TaxDC §8.4가 RequiredHB와 위반 검출을 이미 제안했다." | 맞다. 인용한다. TaxDC의 제안은 구현·평가가 없고 대상이 shared-nothing node다. 기여는 cFS에서 `Req`와 `G`를 얻는 출처(API 의미, 앱 guard, P식 계약)와 C 소스 위의 정적 판정에 둔다 | 4.3.1 | 후보마다 `Req`의 출처(API, 데이터 의존, 계약, 공개 결함)를 보고한다 |
| 5 | "같은 SB 모델을 CodeQL에 넣으면 된다. MLIR은 왜 필요한가." | (a)·(b)는 CodeQL로도 가능하다. (c) 순서 판정에는 HB 모델이 필요하다. MLIR의 이점(typed effect, region, solver 재사용)은 주장이 아니라 측정 대상이다. 비용은 4.5.3의 실측 공백이다 | 4.5 | 1.6절 RQ6: 같은 SB 모델의 CodeQL 구현과 같은 사례 집합에서 후보 집합, 모델·분석 코드 크기, 새 API 추가 비용을 비교한다. CodeQL이 같은 결과를 더 적은 코드로 내면 MLIR 주장을 철회한다 (원노트 §43) |
| 6 | "IKOS나 Frama-C에 CFE stub을 붙이면 된다." | stub으로 SB 수신 내용은 havoc할 수 있다. 하지만 IKOS는 entry point를 독립 프로세스처럼 분석하고 multi-thread를 다루지 않으므로 앱 간 순서는 모델 밖이다. C 전역은 0으로 초기화되어 미초기화 검사는 F2와 다르다. Frama-C의 동시성 plugin은 확인하지 못했다 | 4.1.1, 4.5.2 | IKOS + CFE stub 기준선에서 단일 앱 안의 F2 사례를 찾는지 측정한다. 이 범주에서는 기준선이 강할 수 있다 |
| 7 | "첫 메시지 유실은 잘 알려진 문제다." | 맞다 (ZeroMQ slow joiner, ROS latch, DDS durability, TaxDC m3274). cFS에는 durability 설정이 없어 설정 검사로 풀 수 없다. 의도된 유실(TO_LAB)도 있어 단순 경고는 오경고가 된다. 질문을 '소스에서 반드시 받아야 하는 첫 메시지가 유실될 수 있는가'로 좁힌다 | 4.2.1, 4.3.3 | TO_LAB 정상 대조에서 경고 0, 주입 변형에서 경고 |
| 8 | "F4는 high-level data race나 atomicity와 같다." | 확인한 정의는 run-to-completion handler에 바로 적용되지 않는다. 하지만 HLDR 원문을 읽지 못했고 time-disparity 문헌을 검색하지 않았다. 그래서 F4를 신규로 주장하지 않고 열린 질문으로 둔다. child task가 같은 state를 쓰는 앱은 atomicity 범주로 간다 | 4.3.2, 4.3.4 | HLDR 원문, time-disparity 문헌, child task를 쓰는 공개 앱 조사 |
| 9 | "실제 결함은 없고 합성 예제뿐이다." | 공개 사례가 있다: cFE #198(6.5.0a/6.6.0a), CF #184(수정 833fdbb), sample_app #101, owner 재시작 시 `DUPLICATE_NOT_OWNED`(**[실측]**), 조용한 잔여 사례 #2663, #73 파생 예제(6.4.1/6.4.2 소스). MD #79·HS #148은 순서와 무관한 결정적 결함이라 따로 센다. F1의 공개 양성 사례는 찾지 못해 합성으로 만든다 | 1.2절, 2절 | 수정 전후 쌍에서 판정이 기대한 방향으로 바뀌는지 (1.6절 RQ3) |
| 10 | "결과가 플랫폼에 따라 다르면 soundness가 없다." | 맞다. 판정은 Σ에 상대적이다. **[실측]** relay 역전은 1 CPU 500/500, 4 CPU 0/500이었다. lock 위치는 cFE 버전마다 다르다. 이 연구는 'Σ를 명시한 모델에 대한' 판정만 주장할 수 있고, 형식 증명 전에는 건전성을 주장하지 않는다 | 1.3절, 3.6절 | 모든 결과에 Σ를 기록하고 최소 두 Σ에서 재현을 시도한다 |
| 11 | "동기 사례 Swift/BAT는 cFS가 아니다." | 현재 근거로는 그렇게 본다. BAT flight software는 RAD6000·VxWorks 위의 C++다(snippet). GSFC의 2008 slide 하나가 'Swift BAT (12/04)'를 cFE heritage로 나열하지만 BAT가 cFE를 실행했다는 근거는 없다. 이 판단은 검증에서 이견이 있었다(검증 C22, contested). 동기 사례로만 쓰고 benchmark로 쓰지 않는다 | 1.7절, 2절 | – |
| 12 | "cFE #73은 이미 고쳐졌다." | 맞다. 6.4.2에서 고쳐졌고 현재 코드에서는 경로가 닫혀 있다. 6.4.1/6.4.2 공개 소스로 파생 예제만 만들 수 있고, 결과는 '파생 예제'로 보고한다. 현재의 잔여 사례는 #2663이다 | 1.2절 F5 | 6.4.1 vs 6.4.2 소스 대조 예제 |
| 13 | "data age 분석(R07 등)을 쓰면 freshness도 해결된다." | 그 분석들은 chain 구조와 timing 파라미터를 입력으로 받고, 소스에서 chain을 복원하지 않는다. cFS 통신은 implicit·LET 모델과 맞지 않는다(2차 자료). header 시각은 전송 시각이다. 이 연구는 이벤트 경계의 F3까지만 다룬다 | 4.4 | 복원한 chain을 그런 분석에 넘기는 일은 후속 연구로 둔다 |
| 14 | "ROSInfer 저자들이 이미 NASA framework로의 일반화를 예고했다." | 두 논문 모두 F′를 일반화 대상으로 꼽는다. cFS는 언급하지 않는다. 저자의 2025 학위논문은 확인하지 못했다. 이 위험을 4.7.3에 기록하고 신규성 문장을 그에 맞게 잠정적으로 둔다 | 4.2.3 | 학위논문과 후속 저장소 확인 |

---

### 4.9 원노트·수정본·조사 결과에서 바로잡은 항목

이 절과 관련된 보정은 한곳에 모았다. 원노트 §41·§53·§54·§62는 §10.4.1의 O24, O27, O29, O28이다. 수정본 §8.2 [R03]와 S14(DataFlow tutorial)는 §10.4.2의 RV7, RV8이다. 조사 단계 주장의 검증 결과(Goblint ARINC, fprime-topo-analysis 검사 수, ROSInfer state 변수, Ogma monitor, IKOS 가정, `LocalAliasAnalysis`, import 범위, #73 수정 전 코드, Swift/BAT)는 §10.3의 검증 C08, 검증 C01, 검증 C02, 검증 C06, 검증 C07, 검증 C20, 검증 C17, 검증 C21, 검증 C22 행이다.
