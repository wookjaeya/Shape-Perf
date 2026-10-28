# H4 비교 계획 — 기존 process-level 방법 × 다섯 사례: 결과 범주, 도구×사례 표, 능력 분해, 다음 비교 절차 (2026-10-28)

- 위치와 역할: `plan/h4_comparison_20261028.md`. 가설 H4("Kairos가 기존 process-level race·order-violation 방법에 없는 능력을 더하는가")를 시험할 비교 표와 절차다. PI 결정 기록 `Architecture/spec/PI_DECISIONS_RESOLVED_20261028.md`(이하 PD28) §5의 병행 작업("기존 process-level 연구·도구 조사(H4)")이다. 로드맵 §12 작업 14의 산출물 `process_race_comparison.md`(아직 없음)의 입력이 된다.
- 근거 결정 [D]: PD28 Q4, §2 항목 5·6, §3(기존 방법 비교, 평가 자료, 연구 범위, 수치 보고), §4. 2026-10-22 D4(`PI_DECISIONS_RESOLVED_20261022.md` §1). SPEC §8.3 "Unrun tools"(ADR-058), ADR-046·ADR-047.
- 성격: **합성·설계 문서**다. 도구 실행, 설치, 빌드, 네트워크 접근, 저자 연락을 하지 않았다. 새 논문을 읽지 않았다. 도구에 대한 사실은 [process_race_method_matrix.md](process_race_method_matrix.md)(이하 MX, 2026-10-16판)와 `plan/methods/*.md`(이하 도구별 파일)에서 옮겼다. 칸의 강도는 원 칸보다 올리지 않았다. MX와 도구별 파일은 고치지 않았다. Kairos 쪽 사실은 기록된 실행 문서에서 인용했다.
- 규칙: [CONDITIONS_POLICY.md](../CONDITIONS_POLICY.md). 이 문서는 반복 수, 임계값, 시간 한도, 예산, 노력 수치를 정하지 않는다. 그런 값은 비교를 설계할 때 측정 질문마다 정하며, 그때까지 `unspecified_by_reference`다(PD28 Q4).
- 표기: MX §0.1을 따른다. [F] 인용 사실, [M] 측정, [I] 추론·분석, [D] 결정, [U] 미확인, [제안] 앞으로 할 일. 근거 등급(`전문`, `초록`, `2차`, `기록`, `코드 직접 읽음`)은 도구별 파일의 용어다. "분석"은 읽은 설계 기술에서 도출했고 실행하지 않은 것이다. "분석"을 "못 한다"로 읽지 않는다(MX §0.1).
- 이 문서의 [M]은 하나다. 입력 문서의 sha256을 읽었다(§7).

## 0. 요약

1. **기존 도구의 실제 적용 결과는 오늘 0건이다.** 여덟 도구 × 다섯 사례 40칸에 실행 기록이 없다(MX §0.2 항목 1, §3.2; `baselines/availability.md` "Status at a glance"). MX 이후 저장소에 도구 실행 기록은 더해지지 않았다(`STATUS_20261027.md` §1 H4 행 "비교 미착수"). [M] 도구별 파일 여덟 개의 sha256은 MX §8.1이 읽은 판과 같다.
2. **결과 범주 다섯을 정의했다(§1).** 실행하지 않음(NOT_ATTEMPTED), 실행 불가(COULD_NOT_RUN), 지원 범위 밖(OUTSIDE_SUPPORTED_SCOPE), 실행 후 미탐지(RAN_NOT_DETECTED), 실행 후 탐지(RAN_DETECTED)다. 실행 가능성, 범위, 실행 결과를 한 칸에 합치지 않는다. MX §3의 상태, SPEC §8.3의 D4 상태, `availability.md` §5.1의 실행 라벨에 대응시켰다.
3. **오늘 범주.** RacePro는 이 컨테이너에서 COULD_NOT_RUN(환경)이고, 별도 머신 경로는 NOT_ATTEMPTED(PI 결정 대기)다. 나머지 일곱은 COULD_NOT_RUN(자료: 접근 가능한 출처에서 구현 미도달)이고, 저자 요청 경로는 NOT_ATTEMPTED다. C2는 모든 방법에 사례 쪽 입력 사유가 더해진다. **OUTSIDE_SUPPORTED_SCOPE는 0칸이다.** 원문에서 사례를 명시적으로 배제하는 문장은 찾지 못했다. 후보 문장(DESCRY p.1 등)의 판단은 PI 몫이다(MX §3.3, §5.4 항목 1·14).
4. **Kairos도 같은 범주로 적는다(§1.4, §3.9).** R2·C1은 실행 기록이 있다(pilot·build-smoke·제어). C5는 사례 경로가 이 환경에서 아직 실행되지 않았다(COULD_NOT_RUN, 사례 실행 조건). F0 경로는 32비트 6.4.x 실행 파일을 받지 않는다(현재 구현의 범위 밖, `kairos/probe/anchor/elf_image.cpp` L57–58). C2·C3은 NOT_ATTEMPTED다.
5. **능력 분해(§4).** 예측·탐색과 실행 확인은 여러 기존 도구에 이미 있다(MX §4.3, §4.4 행 5). Kairos와의 차이 후보는 cFS 내부 상태·값 전달의 관측, 이름·ID·AppId의 정체성 결합, 허용 오류·회복과 최종 결과의 분리, 순서 근거 종류의 보고다. 모두 가설이다. 확인된 부족은 0건이다(MX §4.1).
6. **다음 실행 후보는 RacePro다(§5.1).** 공개 코드가 있는 유일한 도구다. 별도 머신 결정(MX §5.4 항목 5)이 먼저다. cFS 쪽 레퍼런스와 RacePro의 i386 커널 요구가 함께 맞을 여지가 가장 큰 사례는 6.4.x(C5)다. 6.4.x readme가 i386 빌드 host를 적는다 [F]. 그 userland가 2.6.35-scribe 커널에서 도는지는 [U]다. SysPro는 ReproSys 접근이 생겨야 한다. 나머지는 구현과 실행 환경(Simics, KLEE 등)이 함께 없다.
7. **공정 비교의 조건(§5.2)과 쓰지 않을 주장(§5.3)을 적었다.** 판정 단위는 순서위반이다. 최종 실패·피해는 따로 적는다(PD28 §2 항목 6, §3).

## 1. 결과 범주

### 1.1 정의

칸 하나에 범주 하나를 쓴다. 실행 축의 사실(차단 요인)과 범위 축의 사실(배제 문장 후보)은 §2·§3 표의 별도 열에 모두 적는다.

| 범주 | 정의 | 칸에 반드시 적을 근거 | 집계 |
|---|---|---|---|
| `NOT_ATTEMPTED` | 실행을 시도하지 않았다. 시도를 막는 구체적 차단 요인을 기록으로 보이지 않았거나, 시도 여부가 결정 대기다 | 대기 중인 결정의 번호, 또는 시도하지 않은 이유 | 미탐지 수에 넣지 않는다 |
| `COULD_NOT_RUN` | 실행에 필요한 것이 없음을 기록으로 보였다. 사유 종류는 넷이다. **E** 환경, **M** 자료(구현·전문), **I** 입력(방법이 요구하는 사례 입력이 없음), **C** 사례 실행 조건(사례 경로가 그 환경에서 실행되지 않음) | 구체적 차단 요인, 측정·검색 기록과 그 날짜, 근거 등급. "찾지 못함"은 "없음"의 확인이 아니다(MX §1.2) | 미탐지 수에 넣지 않는다 |
| `OUTSIDE_SUPPORTED_SCOPE` | 방법 자신의 명시된 모델·대상 범위가 사례의 자원·사건·실행 단위를 배제한다 | 원출처의 배제 문장과 쪽·절. "서술이 없다", "less effective", "초점 밖" 같은 문장은 배제 문장이 아니다. 그런 칸은 후보로만 적고 PI 판단을 기다린다(MX §3.3, §5.4 항목 1·14) | 미탐지 수에 넣지 않는다. 이유와 원문 위치를 보고한다 |
| `RAN_NOT_DETECTED` | 방법을 실행했다(실행 라벨 필수). 사례의 순서위반에 대응하는 출력이 없다 | 원인 하나: (a) 목표 경로 미실행, (b) 관측 부족, (c) 분석 부족, (d) 방법 자체의 유보 출력(예: RacePro의 Diverged, Kairos의 `INCONCLUSIVE`), (e) 출력은 있으나 §1.3 대응 확인에 실패. 실행 라벨, 판, 입력 구성, deviation 목록 | 원인별로 센다. (a)는 탐지 능력 판정을 보류한다(MX §3.1) |
| `RAN_DETECTED` | 방법을 실행했고, 출력이 §1.3의 대응 확인을 통과했다 | 대응 확인 네 항목, 실행 라벨(자연·제어), 방법 자신의 실패 기준에 따른 결과 | 확인하지 않은 출력은 넣지 않는다 |

### 1.2 기존 상태·라벨과의 대응

| 이 문서 | MX §3.1 상태(로드맵 §10.4) | SPEC §8.3 "Unrun tools"(D4) | `availability.md` §5.1 실행 라벨 |
|---|---|---|---|
| `NOT_ATTEMPTED` | `NOT_RUN`(사유: 결정) | `not run` | 없음 |
| `COULD_NOT_RUN` | `NOT_RUN`(사유: 환경·자료·입력·사례 조건) | `not run`, 환경이면 `environment not buildable` | 없음 |
| `OUTSIDE_SUPPORTED_SCOPE` | `INAPPLICABLE` | 원문 배제 문장이 있는 `out of observation scope`, `insufficient expressiveness` | `ANALYSIS`(문헌) |
| `RAN_NOT_DETECTED` (a) | `PATH_NOT_EXECUTED` | `run but not reported` | `ORIGINAL-RUN`, `DOCUMENTED-SETUP-RUN`, `DEVIATION-RUN`, `EXTENSION`, `REIMPLEMENTATION` 중 하나 |
| `RAN_NOT_DETECTED` (b) | `OBSERVATION_GAP` | 같음 | 같음 |
| `RAN_NOT_DETECTED` (c) | `ANALYSIS_GAP` | 같음 | 같음 |
| `RAN_NOT_DETECTED` (d)(e) | 대응 상태 없음. 원인 문장을 그대로 두고 MX 상태로 바꾸지 않는다 | `run but not reported` | 같음 |
| `RAN_DETECTED` | `REPORTED`(대응 확인 뒤) | `detected` | 같음 |

- **실행 전 분석은 결과 범주가 아니다.** 문헌·코드 분석에서 나온 D4 상태(관측 범위 밖, 표현 불충분)는 §3 표의 "분석" 열에 "관측 범위 밖(분석)"처럼 적는다. GAP 이름을 쓰지 않는다. SPEC §8.3은 이 두 상태를 질문에 따라 `INAPPLICABLE` 또는 `OBSERVATION_GAP`·`ANALYSIS_GAP`에 대응시킨다. MX §3.1·§4.1은 GAP 두 상태와 "확인된 부족"을 실행 뒤로 한정한다. 이 문서는 강도가 낮은 쪽(MX)을 따랐다. 두 문서는 고치지 않았다(§6 N3).
- **실행 라벨의 승격 금지.** `DEVIATION-RUN`, `EXTENSION`, `REIMPLEMENTATION`에서 나온 (b)(c)는 "deviation 조건 한정", "확장 한정", "재구현 한정"으로 따로 적는다. 원 도구의 부족으로 올리지 않는다(MX §5.3 S2·S4·S6). 재구현 결과가 생겨도 원 도구 칸의 범주를 바꾸지 않는다.

### 1.3 `RAN_DETECTED`의 대응 확인

PD28 Q4가 정한 네 항목을 그대로 쓴다. 출력이 넷 모두에서 사례의 순서위반과 맞아야 한다.

| 항목 | 내용 | 사례 쪽 기준 |
|---|---|---|
| 위반 대상 | 사용 지점(호출)과 그 발생 | `cases/ledger.md` §1a "User task / site". C5는 `plan/c5_execution_basis_20261028.md` §4 |
| 관련 task | 사용 task와 provider·생성자 task | ledger §1a "Publisher task / site" |
| 공유자원 | 자원 키와 세대 | ledger §1a "Resource" |
| 판정 근거 | 순서를 세운 근거(무엇이 무엇보다 먼저였음을 무엇이 보였는가) | ledger §1a "Required order + provenance" |

- 출력이 다른 자원이나 다른 순서를 가리키면 (e)다. 예: RacePro가 go-live 뒤 무관한 후보 이름으로 실패를 보고하는 경우(RacePro p.363, MX §2.4.1 C1 Q5). R2 ③의 fd table 쌍(MX §6.2)도 AppId 의존과 다른 자원이다.
- 방법 자신의 실패 기준(checker, oracle, failure point, 재현 판정)이 순서위반 기준과 다르면 두 결과를 함께 적는다(SPEC §8.3 "Unrun tools").
- **최종 실패·피해는 별도 열이다.** CF 종료, `ILLEGAL_APP_ID` 거부, `CFE_ES_ERR_APPID`, crash가 그 예다. 최종 실패만 재현한 출력은 순서위반 탐지로 세지 않는다. 최종 실패 없이 순서위반만 있는 경우(R2: 프로세스 실패 없음)도 순서위반으로 판정할 수 있다(PD28 §2 항목 6, §3 연구 범위).

### 1.4 Kairos의 대칭 보고

- Kairos 칸에도 같은 범주와 같은 근거 요구를 쓴다.
- 사례 쪽 차단(§3.0)은 모든 방법에 같은 사유로 적는다. 방법의 결함으로 쓰지 않는다(MX §3.2 "사례별 추가 사유").
- Kairos 구현이 받지 않는 입력은 Kairos의 `OUTSIDE_SUPPORTED_SCOPE`로 적는다. 근거는 코드·문서 위치다. 예: F0 관측기는 little-endian ELF64 x86-64만 받는다(`kairos/probe/anchor/elf_image.cpp` L57–58) [F]. 이것은 현재 구현의 범위이며 계획된 엔진 작업이 있다(c5_execution_basis §0 항목 8, §10 E-F0).
- Kairos의 `INCONCLUSIVE`는 `RAN_NOT_DETECTED` (d)다. 안전의 근거가 아니다(`Architecture/07_evaluation.md` §0 "경보 없음은 안전이 아니다", SPEC §6.4 항목 2).
- Kairos의 관측 부족도 같은 이름으로 적는다. 예: F0 R2 pilot3에서 `CFE_SB_Global.AppId` 위치는 `open{effect_unsupported}`였다(`CFE_ES_GetAppID` 저장 미관측). 판정은 그 위치를 읽지 않고 인자 직접 포획으로 섰다(`plan/results_pilot_20261025.md` §1 R2-1). 계측 경로는 그 값을 직접 포획한다(`plan/results_instr_pilot_20261026.md` §8).
- 내부 대조(F0 대 G-I, F0 대 G-I-H)와 Kairos ablation은 외부 방법과의 비교가 아니다(PD28 §3; MX §5.3 S5). G-I-H 36회는 실행 경로 대조이며 H3에 넣지 않는다(PD28 §2 항목 5).
- Kairos 결과도 실행 라벨(natural pilot, build-smoke, controlled …)과 함께 적고, 라벨이 다른 수를 더하지 않는다(`plan/results_pilot_20261025.md` 머리말). 지금까지의 Kairos 실행은 모두 pilot·build-smoke·제어 실행이며 본 실험 결과가 아니다(`STATUS_20261027.md` 머리말).
- **실행 단위의 정의 차이를 먼저 밝힌다.** 이 연구의 Process-Level은 cFS 애플리케이션·task 단위의 운용 정의다(로드맵 §1.2). native cFS는 한 Linux 프로세스이고 OSAL task는 pthread다. 기존 도구 대부분의 단위는 OS 프로세스다(MX §0.3). 이 차이에서 생기는 결과를 도구의 결함처럼 쓰지 않는다.

## 2. 도구 수준 표 (사례와 무관)

### 2.1 모델: 지원 자원, 순서·동기화, 입력 사건, 출력

칸은 MX §2.2와 도구별 파일 §2.1의 압축이다. 쪽 번호의 기준 판은 MX §1.1에 있다. STVR(SimRacer·SIMEXPLORER·SimEvo가 기대는 서술)과 RRF의 쪽 번호는 단일 취득 경로이며 독립 재확인이 없다(MX §1.1 "취득 경로").

| 도구 (본문 등급) | 지원 공유자원 | 순서·동기화 모델 | 관측·모델하는 사건 | 사용자 입력 | 출력·판정 기준 |
|---|---|---|---|---|---|
| RacePro (전문 + 코드 직접 읽음) | 커널 객체. 논문 Table 2(p.357): inode, file, file-table, mmap, cred, global, pid, ppid. 코드: INODE, FILE, FILES_STRUCT, PID, FUTEX, IPC(System V), MMAP, PPID, SUNADDR. FUTEX는 RESOURCE 후보에서 뺀다(`racecore.py:194, 282`). 공유 메모리는 page ownership으로 기록하나 HB·후보에 쓰지 않는다(p.358, K5). POSIX mqueue에는 Scribe 잠금이 없다(K1) | 프로그램 순서와 HB 4종: fork(`clone` 포함)→자식, exit→wait, kill→신호 전달, stream write→겹치는 read(pp.357–358). 공유 메모리 의존과 lock은 HB가 아니다 | syscall, signal, 커널 객체 load/store micro-op(§5.2) | 배포 실행의 기록, 프로그램별 checker(`<test>.test`) | 후보마다 Diverged/Benign/Harmful. checker가 "녹화 때 없던 실패"를 볼 때만 보고(p.363) |
| SimRacer (초록) | SimRacer 자체는 불명. STVR 설계 기준은 아래 SIMEXPLORER 행 | SimRacer 자체는 불명(SimEvo p.3 `2차`의 partial order 서술뿐) | SimRacer 자체는 불명. 같은 해 Sim-O/C는 process 사이 사건을 system call로 서술(ITJ p.188 `2차`) | PuT와 짝 프로그램(STVR 기준) | race와 fault의 구분(초록) |
| SIMEXPLORER (전문, 저자 수락본) | SR: shared memory, fd, inode, file data, `/proc` process data, hardware(STVR p.8, p.14). 메모리는 Simics 중단점으로 잡으며 SR 지정이 전제다. 지정 형식은 서술이 없다 | syscall 의미에서 추론한 HB. semaphore와 lock은 HB 규칙 밖(p.9) | SR 접근, 동기화 사건, syscall 23종의 read/write 효과. 반환값은 사건이 아님 | PuT와 짝 프로그램·handler, test input, 출력 기반 oracle(p.16–18), SR·syscall 추가 지정 | PRaceSet / RaceSet / oracle fault. 출력에 영향이 없는 real race는 fault로 세지 않음(p.19) |
| RRF (전문, 단일 취득 경로) | Scribe의 resource lock 기록(inode, files_struct, file 등). 엔지니어가 bug report에서 자원 이름을 지정(p.4–5) | 같은 자원에 접근하는 두 프로세스의 syscall 쌍. 두 순서·네 패턴으로 열거(p.5–7) | syscall, signal. 메모리·futex·mutex·semaphore 사건은 서술 없음 | bug report, PuD 소스(p.5), oracle 저장소(p.6), sleep 길이 `ST`와 반복 `r` | race report(재현 여부, targeted/incidental/new) |
| DESCRY (전문) | syscall로 접근하는 시스템 전역 자원(p.3–4). 사건·자원 모델 상세는 [21, 38]에 위임 | 관계 5종: program order, fork, wait, pipe, signal(p.3, p.7) | 실패 실행의 default log message, PuD의 syscall 사건 | 실패 로그, PuD 소스(p.1, p.3) | ⟨입력, schedule⟩와 재현 성공 여부. 성공은 failure point 도달(p.3–4) |
| SimEvo (전문) | syscall R/W 효과. 식별 모델은 RacePro·SimRacer에 위임(p.3, p.5) | partial order 5종: program, fork-return, wait-exit, pipe-read, signal(p.3, p.6) | syscall R/W 효과와 fork, wait, exit, pipe, signal. Def-Use = syscall write/read(p.6) | 두 판 S·S′, 기존 테스트 T, 커버리지 기준(p.4) | 선택된 테스트, Def-Use 쌍 커버리지, 오라클 불일치 결함(p.4, p.9) |
| ReDPro (초록·`2차`) | 초록의 "a shared data item"뿐 | 불명 | PIN run-time monitoring. 계측 사건 종류는 불명 | 불명 | race 탐지와 failure 재생성(초록). 형식은 불명 |
| SysPro (전문, arXiv v1) | 자원을 식별하지 않음(p.3, p.14) | 보고서에 함께 나온 syscall 쌍의 순서. 쌍은 파일 시스템 syscall(p.6, p.11, p.22, p.25) | 실행 사건을 수집하지 않음. 입력은 보고서 텍스트와 소스의 정적 구조(p.16, p.20–21) | bug report, 대상 소스, system call manual, 프로세스 집합, test case(p.4–5, p.19–20) | 후보 syscall 위치 순위, 재현 시 schedule. 재현 판정 방법은 서술 없음(p.24) |

### 2.2 실행 요구와 오늘의 차단 요인, 범위 밖 후보

차단 요인의 측정은 도구별 파일의 2026-10-15·16 기록이다. 이 문서는 다시 재지 않았다.

| 도구 | 구현 | 실행 요구(도구별 파일 §4.1) | 오늘 차단(기록) | 막힘을 풀 경로 | 범위 밖 후보 문장(PI 판단 대기, MX §3.3) |
|---|---|---|---|---|---|
| RacePro | **공개**(`columbia/racepro` @`681f94e3` 외, RacePro.md §7) | i386 Linux 2.6.35 + Scribe 내장 커널(번들 `kernel_config` L3, L6–7, L1609), 설치와 부팅(`deploy.rb` L24–25), Python 2·Cython ≥ 0.13·networkx·pygraphviz(커널 README 39–44행, `setup.py` L13), replay용 파일 시스템 체크포인트(p.362). userland 배포판은 `unspecified_by_reference` | **E**: kexec·module 적재·`/dev/kvm`·vmx/svm 없음, `/boot` 비어 있음, python2·cython 없음(RacePro.md §4.1 [M] 2026-10-15T08:17Z, 2026-10-16T00:07Z) | 별도 머신(KVM 가능 또는 bare metal), MX §5.4 항목 5(RacePro.md P-3) | p.362 "less effective in finding races on OS resources between threads of the same process". 배제 문장이 아니다. 논문은 thread를 기록 대상에 넣는다(p.354, p.356) |
| SimRacer | 찾지 못함(SimRacer.md §1.1) | Simics(판·머신·스크립트 `unspecified_by_reference`), guest, 제어용 dummy device·driver | **M**: 구현, ISSTA'13 본문. **E**: Simics 없음 | 저자 요청(MX §5.4 항목 3), 본문 확보(항목 8·17) | 원문 위치를 보고할 수 없다(본문 미독) |
| SIMEXPLORER | 찾지 못함. 분석 대상 24개는 first author에게 요청(STVR p.15) | Simics, guest(단일 코어 x86 Linux 2.6.15·3.5.0, p.18), Controller, cFS용 SR·oracle 입력 | **M**: 구현. **E**: Simics 없음. 디스크 여유(2026-10-16 2.3 GB)가 벤더 문서의 설치 크기(약 10 GB)보다 작음 | 저자 요청(항목 3), Simics 설치(항목 6) | 단일 process 안의 multithreaded 응용을 기존 thread-level 기법의 영역으로 둔다(p.1, p.4–5). 배제 문장은 없다(SIMEXPLORER.md §1.3 항목 11) |
| RRF | 찾지 못함 | Scribe 패치 커널(판 서술 없음), 이름 없는 정적 분석 도구, 내부 oracle 저장소, PuD 소스 | **M**: 구현. **E**: Scribe 환경(RacePro와 같은 차단, RRF.md R2) | 저자 요청, 학위논문·[36] 본문 | p.1–2 "두 프로세스가 OS 자원에 접근하는 race", thread 수준 탐지기와 대비(p.1). 배제 문장은 없다(RRF.md §1.3 항목 8) |
| DESCRY | 찾지 못함. 프로젝트 페이지 403 | LLVM 로그 분석기, KLEE(LLVM 3.4 기반, p.8), Simics(p.7), 실패 로그(p.1, p.3) | **M**: 구현. **E**: `klee`·`llvm-3.4` 패키지 없음(LLVM 18.1.3뿐), Simics 없음. **I**: 다섯 사례 본문에 실패 로그 없음, 댓글 미독 | 저자 요청(논문에 이메일), 댓글 확보(항목 9) | p.1: 초점은 inter-process bug이고 intra-process(thread-level) bug는 구분한다. 후보 가운데 가장 직접적이다. 반대 근거는 p.3의 단일 thread 가정이 "for ease of presentation"이라는 서술(DESCRY.md §2.3 항목 1·2) |
| SimEvo | 찾지 못함. 기반 SimRacer 구현도 없음 | Simics, CodeSurfer(상용, 현재 가용성 불명), C++ GA, S·S′, T, 오라클 | **M**: 구현. **E**: Simics·CodeSurfer 없음. **I**: cFS의 T·오라클 정의가 레퍼런스에 없음 | 저자 요청, 정의는 PI 결정(항목 11) | 명시 대상은 multi-process와 handler(p.1, p.7). 배제 문장인지 SimEvo.md가 정하지 못함(§4.3-1) |
| ReDPro | 찾지 못함 | 전문을 못 읽어 요구 자체가 불명. PIN 판 `unspecified_by_reference` | **M**: 구현과 전문. **E**: PIN 없음 | 전문 확보(항목 8·15·17) | 붙일 수 없다(전문 미독) |
| SysPro | ReproSys(`github.com/tarannumzaman/ReproSys`)는 익명 접근에 인증 요구. 2026-10-16 `add_repo`(읽기)가 "not found 또는 접근 권한 없음"(SysPro.md §1.1) | PIN, srcML, NLTK·Gensim, SPMF·Java, syscall 이름 목록·man 페이지. sleep 길이와 재현 판정은 서술 없음(Y12, Y13) | **M**: 구현. **E**: PIN·srcML·NLTK·Gensim·SPMF 없음(2026-10-16 [M]) | 저자 요청 또는 접근 권한이 있는 계정(항목 3) | inter-process 범위(p.2–3), 프로세스 집합 입력(p.4–5). 배제 문장인지는 PI 판단 |

## 3. 도구 × 사례 표

### 3.0 사례 쪽 공통 사유 (모든 방법에 같이 적용)

| 사례 | 사례 쪽 사실(저장소 기록) | 범주에 주는 영향 |
|---|---|---|
| C1 CF#184 | 자원은 OSAL count semaphore의 이름 표(사용자 공간 표, 이름 → ID → 슬롯). 생성과 이름 조회를 순서 짓는 장치가 없다. OSAL 표 lock은 호출을 원자로 만들 뿐 순서를 짓지 않는다(ledger §1a). 실패 출력과 빈도는 `unspecified_by_reference`. 원 보고자의 cFE·OSAL·PSP는 미상이고 공개 BP 구성(D22)을 쓴다(MX §0.1, §3.2) | 공개 BP 구성의 실행은 원 보고 환경의 재현이 아니다. 구성 차이는 deviation으로 적는다 |
| C2 cFE#198 | 앱 P·R은 비공개(EVA CWS). 공개된 것은 단일 단계 시작 동기화와 6.6.0a의 두 단계 방식뿐이다(ledger §1a, MX §6.6) | **모든 방법에 I.** 원 시스템을 실행할 수 없다. 역할 재구성(P-C2) 결과는 "C2 메커니즘의 역할 기반 검증"이며 원 시스템의 재현이 아니다. 실제 앱·함수 자료를 어디까지 확보할 수 있는지가 먼저다(PD28 §4) |
| C3 cFE#73 | 원 플랫폼 Microblaze의 OS·PSP·OSAL은 미상. 파생 구성(32-bit x86 pc-linux)은 deviation(ledger §1b). 같은 6.4.1 tarball의 readme 빌드는 이 컨테이너에서 `msg_max` 때문에 운용 상태 전에 끝났다(c5_execution_basis §0 항목 3) | 원 조건을 충족하지 않은 실행은 원 사례의 재현으로 세지 않는다(PD28 §4). 이 컨테이너에서는 **C**도 붙는다고 본다 [I]. C3 파생 구성 자체로 기동한 기록은 없다 |
| C5 cFE#72 | 수정 전 6.4.1, 수정 후 6.4.2, OSAL 4.1.1, PSP pc-linux 1.2.0.0(c5_execution_basis §0 항목 1). build-smoke가 운용 상태 전에 끝나 `CFE_ES_AppCreate` 경로가 실행되지 않았다. readme 해법 가운데 "root로 실행"은 이 컨테이너에서 성립하지 않는다(`CAP_SYS_RESOURCE` 없음). 남는 해법은 PI 결정 대기다(같은 문서 §3.5). 실패 창(A8)은 기술 확인 과제다(PD28 §2 항목 6) | 이 컨테이너에서는 **모든 방법에 C**. 사례 경로가 실행되기 전의 미탐지는 (a)일 수밖에 없어 정보가 없다 |
| R2 cFE#2663 | 병합된 수정이 없다. 자연 쌍 `b9bec567^` → `b9bec567`(P-R2CTRL (c2) [D]). 순서가 고정인지 스케줄 의존인지 사례 기록상 미결(MX §4.4 행 6). 공통 시작 조건(Power-On 명시)으로 다시 실행하기로 했다(PD28 Q3, `plan/r2_start_poweron_20261028.md`) | 비교 실행은 PD28 Q3의 공통 시작 절차를 따른다 |

### 3.1 RacePro (전문 + 코드 직접 읽음; MX §2.4.1, §6)

열의 뜻(§3.1–§3.9 공통): "자원·동기화"와 "필요 사건"은 사례의 의존이 그 방법의 지원 자원·입력 사건에 드는지에 대한 **분석**이다. "실행 가능성"은 그 사례에 대한 차단 요인이다. "실제 결과"는 기록된 실행이다.

| 사례 | 자원·동기화 → 도구 모델 (분석) | 필요 사건 → 입력 사건 (분석) | 실행 가능성 | 필요한 확장·추가 입력 | 실제 결과 | 오늘 범주 |
|---|---|---|---|---|---|---|
| C1 | 관측 범위 밖(분석, 후보 생성 입력 기준). OSAL 이름 표는 커널 객체가 아니다. 테이블 lock은 HB 간선이 아니고, 대기·wake 때의 futex는 후보에서 빠진다 | 생성·조회는 syscall 0건 `[M-probe 기록]`(OSAL `d2d877a`, 경합 없음, C1 OSAL 후보 아님). 반환값과 CF 초기화 결과는 checker 층에서만 닿는다(MX §6.3 C1-d) | E(§2.2). 추가로 BP 공개 구성은 x86-64 빌드이고, i386 2.6.35 userland에서의 빌드 조건은 레퍼런스에 없다 | `EXTENSION`: OSAL 레지스트리를 자원으로 추가(H-RP-1), `mem_owned_*` 사건을 후보 생성에 사용(K5). 연구 쪽 입력: CF abort를 실패로 보는 checker. i386 재빌드는 deviation | 없음 | `COULD_NOT_RUN`(E). 별도 머신은 `NOT_ATTEMPTED` |
| C2 | 불명. ES 시작 동기화의 상태 polling과 sleep은 micro-op을 만들지 않는다(MX §6.6, 분석) | 불명(P·R 비공개) | E + I(§3.0) | 역할 재구성 입력부터 | 없음 | `COULD_NOT_RUN`(E, I) |
| C3 | 관측 범위 밖(분석). AppId·등록 플래그는 메모리다. 파생 6.4.1 구성의 TIME `CreatePipe` → `mq_open`은 FILES_STRUCT 쓰기만 만들고 의존 상태와 이어지지 않는다(K1–K3, MX §6.4 C3-b) | 저장·이벤트 요청·범위 검사 없는 접근은 사용자 공간 `[I 소스]` | E + C(§3.0). 원 플랫폼 미상 | `EXTENSION`: AppId 메모리 자원화. built-in checker에 segfault가 있으나 잘못된 상태 사용과 crash를 구분하는 출력은 없다(MX §6.4 C3-e) | 없음 | `COULD_NOT_RUN`(E, C) |
| C5 | 등록 쓰기·조회는 관측 범위 밖(분석, ES lock 아래 메모리). `clone`은 HB 노드·간선이다(코드가 `clone`을 fork로 취급, `unistd.py:373`) — 대응(부분) | 생성의 `clone`만 입력 사건. `CFE_ES_ERR_APPID`는 built-in checker 밖(MX §6.5) | E + C(§3.0). 6.4.x는 readme대로 32비트 pc-linux이고 readme가 i386 빌드 host를 적는다(c5_execution_basis §1.3) [F] | `EXTENSION`: `TaskTable` 자원화. checker: 반환값 또는 소비 결과(c5_execution_basis §4 O1–O3)를 볼 것 | 없음 | `COULD_NOT_RUN`(E, C) |
| R2 | ①AppId 설정 ②값 전달 ④오류 반환은 관측 범위 밖(분석). S1의 RUNNING polling은 HB가 아니다(p.358). ③ `mq_open`은 FILES_STRUCT 쌍이 될 수 있으나 fd table 접근이고 AppId와 무관하다(K2–K4, 코드 읽기, 미실행) | ⓪ `clone`은 HB의 원천. ①②④는 입력 사건이 없다 `[I 소스]`(MX §6.2) | E. 추가로 v7.0.1 `core-cpu1`은 x86-64 ELF이고 커널 ≥ 3.2.0을 요구한다(RacePro.md §4.1) | `EXTENSION`: AppId 자원화. R2는 출력이 없어 checker가 내부 반환값을 보려면 cFS 쪽 관측 추가가 필요하다 [I]. 보고 조건 "녹화 때 없던 실패"(p.363)와의 관계(H-RP-5). i386 재빌드는 deviation | 없음 | `COULD_NOT_RUN`(E) |

### 3.2 SimRacer (초록; MX §2.4.2)

| 사례 | 자원·동기화 (분석) | 필요 사건 (분석) | 실행 가능성 | 필요한 확장·추가 입력 | 실제 결과 | 오늘 범주 |
|---|---|---|---|---|---|---|
| C1, C3, C5, R2 | SimRacer 자체는 **불명**(Q1·Q2 불명, MX §2.3). STVR 설계 기준의 분석은 §3.3이 대신한다 | 불명 | M + E(§2.2). C3·C5는 + C | 원 도구가 없어 `REIMPLEMENTATION`만 가능하다. STVR에 기댄 재구현은 SIMEXPLORER 기준이지 SimRacer가 아니다(SimRacer.md P-5) | 없음 | `COULD_NOT_RUN`(M, E) |
| C2 | 불명 | 불명 | M + E + I | — | 없음 | `COULD_NOT_RUN`(M, E, I) |

### 3.3 SIMEXPLORER (STVR 전문, 단일 취득 경로; MX §2.4.3)

| 사례 | 자원·동기화 (분석) | 필요 사건 (분석) | 실행 가능성 | 필요한 확장·추가 입력 | 실제 결과 | 오늘 범주 |
|---|---|---|---|---|---|---|
| C1 | SR 지정을 전제로 관측 대상(분석). OSAL 표 주소가 SR이면 메모리 중단점 사건이다. 테이블 lock은 HB 규칙 밖(p.9) | 등록 쓰기·조회 읽기는 사건이 될 수 있다(지정 전제). 반환값은 사건 목록 밖 | M + E | `REIMPLEMENTATION`(MX §5.4 항목 4). SR 지정(형식 서술 없음), oracle(출력 기반) | 없음 | `COULD_NOT_RUN`(M, E) |
| C2 | 불명 | 불명 | M + E + I | — | 없음 | `COULD_NOT_RUN`(M, E, I) |
| C3 | SR 지정 전제(분석). 세 필드는 서로 다른 SR. 순서는 우선순위가 만들며 HB 추론 대상이 아님 | `mq_open`이 23종에 드는지 불명 | M + E + C. 원 플랫폼 Microblaze와 STVR의 x86 guest가 다름 | 위와 같음. crash를 따로 검출한다는 서술 없음 | 없음 | `COULD_NOT_RUN`(M, E, C) |
| C5 | SR 지정 전제(분석). `clone`이 fork 규칙의 간선을 만드는지 서술 없음. 6.4.2 수정(lock 범위)은 HB 규칙 밖 | 반환값은 oracle이 볼 출력이어야 한다 | M + E + C | oracle이 내부 반환값을 보지 않는다. 출력 기반 oracle로는 판정 대상이 정해지지 않음 | 없음 | `COULD_NOT_RUN`(M, E, C) |
| R2 | SR 지정 전제(분석). S1 직렬화는 HB 규칙에 없음 | 반환값은 사건 아님. 출력이 없어 출력 기반 oracle에 차이가 안 나타날 수 있음(p.19) | M + E | 내부 상태를 보는 oracle(확장) | 없음 | `COULD_NOT_RUN`(M, E) |

### 3.4 RRF (전문, 단일 취득 경로; MX §2.4.4)

| 사례 | 자원·동기화 (분석) | 필요 사건 (분석) | 실행 가능성 | 필요한 확장·추가 입력 | 실제 결과 | 오늘 범주 |
|---|---|---|---|---|---|---|
| C1 | 관측 범위 밖(분석). 관련 이벤트는 syscall·resource lock 로그에서 정해진다. 설정 파일의 자원 이름은 Scribe가 보는 파일 이름이고 OSAL 이름이 아니다 | 생성·조회 syscall 0건 `[M-probe 기록]` | M + E(§2.2) | 사람이 소스 위치를 직접 지정하는 입력(서술 없음 → `EXTENSION`). oracle(내부 oracle [36]의 입력 형식 불명). `ST`·`r`의 논문 값은 논문 사례의 값이고 cFS 조건으로는 `unspecified_by_reference`(RRF.md R6) | 없음 | `COULD_NOT_RUN`(M, E) |
| C2 | 불명 | 불명 | M + E + I(PuD 소스 필요, p.5) | — | 없음 | `COULD_NOT_RUN`(M, E, I) |
| C3 | 관측 범위 밖(분석). 메모리 접근. 파생 구성의 `mq_open`은 R2 ③과 같은 분석 | 저장·사용이 syscall 쌍이 아님 | M + E + C | 위와 같음 | 없음 | `COULD_NOT_RUN`(M, E, C) |
| C5 | 관측 범위 밖(분석). `clone` 계열만 syscall. RRF의 쌍은 서로 다른 프로그램의 두 프로세스 사이에서 정의된다(전제 2) | `TaskTable` 쓰기·`GetAppID` 조회는 메모리 `[I 소스]` | M + E + C | 위와 같음 | 없음 | `COULD_NOT_RUN`(M, E, C) |
| R2 | 관측 범위 밖(분석). AppId 설정·값 읽기는 메모리 | `mq_open`과 ⓪만 syscall `[I 소스]`. 반환값은 기록 대상 아님 | M + E | 반환값을 보는 oracle. 출력이 없어 출력 기반 oracle에 차이가 안 나타날 수 있음 | 없음 | `COULD_NOT_RUN`(M, E) |

### 3.5 DESCRY (전문; MX §2.4.5)

| 사례 | 자원·동기화 (분석) | 필요 사건 (분석) | 실행 가능성 | 필요한 확장·추가 입력 | 실제 결과 | 오늘 범주 |
|---|---|---|---|---|---|---|
| C1 | 서술된 사건 집합 밖(분석). 사건·자원 모델이 [21, 38]에 위임돼 구현의 범위는 불명 | 생성·조회 syscall 0건 `[M-probe 기록]`. 수정 전 소스에 실패 문장 후보가 있으나 logging point로 식별되는지는 불명 | M + E + I(실패 로그 없음) | 메모리 사건(`EXTENSION`), 실패 로그 | 없음 | `COULD_NOT_RUN`(M, E, I) |
| C2 | 도구 쪽 분석: 관계 5종에 초기화 완료가 없다. 사례 쪽은 불명 | 불명 | M + E + I(PuD 소스, p.1, p.3) | — | 없음 | `COULD_NOT_RUN`(M, E, I) |
| C3 | 서술된 사건 집합 밖(분석). message queue 모델 서술 없음("mq_" 0건) | 현상이 segfault이고 메시지가 없어 failure point를 세울 수 없음 | M + E + I + C | failure point의 정의 | 없음 | `COULD_NOT_RUN`(M, E, I, C) |
| C5 | 서술된 사건 집합 밖(분석). `clone3`는 사건이 될 수 있으나 thread 생성 서술이 없다 | failure point는 앱 의존 | M + E + I + C | 위와 같음 | 없음 | `COULD_NOT_RUN`(M, E, I, C) |
| R2 | 서술된 사건 집합 밖(분석) | 출력 메시지가 없어 failure point를 세울 수 없음. 목표 완화(p.4)는 처음부터 메시지가 없는 경우를 다루지 않음 | M + E + I | 위와 같음 | 없음 | `COULD_NOT_RUN`(M, E, I) |

### 3.6 SimEvo (전문; MX §2.4.6)

| 사례 | 자원·동기화 (분석) | 필요 사건 (분석) | 실행 가능성 | 판 쌍 S → S′(SimEvo.md §3.2) | 필요한 확장·추가 입력 | 실제 결과 | 오늘 범주 |
|---|---|---|---|---|---|---|---|
| C1 | 서술된 사건 종류 밖(분석). 식별은 불명 | 이름 등록·조회는 사건 종류에 없음 | M + E + I(T·오라클) | `281a941` → `833fdbb`(원출처 사실, 비교 후보는 D22 [D]) | T·오라클 정의(MX §5.4 항목 11) | 없음 | `COULD_NOT_RUN`(M, E, I) |
| C2 | 불명 | 불명 | M + E + I | 6.5.0a → 6.6.0a(수정과 #198의 연결은 추론) | — | 없음 | `COULD_NOT_RUN`(M, E, I) |
| C3 | 서술된 사건 종류 밖(분석). "publish-last"와 시작 barrier는 partial order 5종 밖 | 같음 | M + E + I + C | 6.4.1 → 6.4.2(tarball 차이) | 같음 | 없음 | `COULD_NOT_RUN`(M, E, I, C) |
| C5 | 서술된 사건 종류 밖(분석). 수정이 lock 범위인데 lock은 사건 종류에 없음 | 같음 | M + E + I + C | 6.4.1 → 6.4.2(tarball 차이) | 같음 | 없음 | `COULD_NOT_RUN`(M, E, I, C) |
| R2 | 서술된 사건 종류 밖(분석). 시작 직렬화는 partial order 밖 | 같음 | M + E + I | `b9bec567^` → `b9bec567`(P-R2CTRL (c2) [D]). CMake 목록 변경이 변경 집합에 드는지 서술 없음 | 같음 | 없음 | `COULD_NOT_RUN`(M, E, I) |

### 3.7 ReDPro (초록·`2차`; MX §2.4.7)

로드맵 §10.1에 따라 탐지·재현 표의 확정 행으로 넣지 않는다. 기록만 둔다.

| 사례 | 자원·동기화 | 필요 사건 | 실행 가능성 | 필요한 확장 | 실제 결과 | 오늘 범주 |
|---|---|---|---|---|---|---|
| 다섯 사례 | 불명 | 불명(PIN의 계측 대상이 초록에 없음) | M(구현·전문) + E(PIN 없음, 판 미정). C2 + I | 불명(전문이 먼저) | 없음 | `COULD_NOT_RUN`(M, E) |

### 3.8 SysPro (arXiv v1 전문; MX §2.4.8)

| 사례 | 자원·동기화 (분석) | 필요 사건 (분석) | 실행 가능성 | 필요한 확장·추가 입력 | 실제 결과 | 오늘 범주 |
|---|---|---|---|---|---|---|
| C1 | 제어 단위(syscall 호출 지점)에 생성·조회 지점이 없다(분석) | 사건 수집 없음. 보고서 본문은 "provider의 시작을 지연"이라는 사람용 방법을 적는다 | M + E(§2.2) | cFS 앞단(Linux syscall 이름 대신 cFE/OSAL API)은 `EXTENSION`(MX §5.3). sleep 길이와 재현 판정은 `unspecified_by_reference` | 없음 | `COULD_NOT_RUN`(M, E) |
| C2 | 도구 쪽 분석: 관계가 syscall 쌍의 순서이고 초기화 완료 개념이 없다. 사례 쪽은 불명 | 불명 | M + E + I(대상 응용 소스 비공개, Y8) | — | 없음 | `COULD_NOT_RUN`(M, E, I) |
| C3 | 제어 단위 밖(분석). 의존 접근은 메모리 `[I 소스]`. `mq_open`은 보이나 의존 상태가 아님 | 같음 | M + E + C. 원 플랫폼과 PIN 대상 ISA의 관계 불명 | 위와 같음 | 없음 | `COULD_NOT_RUN`(M, E, C) |
| C5 | 제어 단위 밖(분석). `clone3`만 syscall | 같음 | M + E + C | 위와 같음. 참조 OSAL에서 `clone3` 직후 지연이 child를 막는다는 소스 분석이 있다(기록, 미실행) | 없음 | `COULD_NOT_RUN`(M, E, C) |
| R2 | 제어 단위 밖(분석). `mq_open`만 보임 | 같음 | M + E | 위와 같음. 고정 순서라면 지연이 새 순서를 만들 역할이 없다(분석) | 없음 | `COULD_NOT_RUN`(M, E) |

### 3.9 Kairos (같은 열, §1.4의 대칭 규칙)

| 사례 | 자원·동기화 → 모델 | 필요 사건 → 입력 사건 | 실행 가능성 | 필요한 확장·추가 입력 | 실제 결과(기록) | 오늘 범주 |
|---|---|---|---|---|---|---|
| C1 | Φ 키 `osal.countsem(name)`. 일곱 항목이 모두 섰다(writer 27, 별칭, 시작 상태, 수명, 프로브 등). 테이블 lock은 순서 근거로 쓰지 않는다(results_pilot_20261025 §1 C1-N1) [M] | 생성 공개, CF 조회의 이름·id·반환, 재시도, CF 종료 [M] | 실행됨. F0(uprobe·tracefs)와 계측 경로(G-I, G-I-H). 빌드 변경은 `-Wno-error=restrict` 하나(STATUS_20261027 §3.2) | 사람이 쓴 프로필, 호출 역할(`dependent_use`/`existence_probe`), 요구·허용 기록(caller 기록) — 수동 입력이다 | build-smoke 자연, 수정 전: `INCONCLUSIVE(caller_requirement_absent)`. CF 종료는 별도 결과. 자연, 수정 후: TOLERATED → READY. 제어 E-06: `OBSERVED_VIOLATION` ×25(`timeout-exercised`). 예측 → 확인: confirm2 계획 9개 × 4 인스턴스 실현, 반대 결과 0(results_pilot_20261025 §1, §5.6; STATUS_20261027 §3.2–§3.3) | 자연 수정 전: `RAN_NOT_DETECTED` (d). 제어: `RAN_DETECTED`(controlled, 채택한 요구·허용 모델의 판정. PR #342의 의도된 상한과 함께 읽음) |
| C2 | 프로필 없음 | — | 사례 쪽 I(§3.0) | 역할 재구성(P-C2) 또는 실제 앱 자료 확보(PD28 §4) | 없음 | `NOT_ATTEMPTED` |
| C3 | 프로필 없음 | — | 사례 쪽 C(§3.0). 원 플랫폼 미상 | 원 실행 조건 확인(PD28 §4) | 없음 | `NOT_ATTEMPTED` |
| C5 | 초안 manifest(`kairos/profiles/c5-cfe-6.4.x/instr/`)의 site가 6.4.1 32/32, 6.4.2 30/30 결합(단위 컴파일 확인, c5_execution_basis §8.2) [M]. 판정 구조는 C1의 "생성 전 조회"와 같다고 본다(같은 문서 §0 항목 9) [I] | 등록 쓰기 L649–L652와 해제 L673, 새 task의 잠금 읽기(같은 문서 §4) [F] | 사례 경로 미실행: 6.4.x가 운용 상태 전에 끝남(C, §3.0). F0: ELF64 x86-64만 받음(`elf_image.cpp` L57–58). 계측: 32비트 `kz-rt-dump`가 링크되지 않음(같은 문서 §8.3) | 엔진 작업: ELF32 관측 backend, 생성 뒤에 정해지는 소유 앱 결합, 32비트 회수 도구(같은 문서 §10) | 없음. Kairos 관측기를 붙인 실행이 없다(같은 문서 머리말) | `COULD_NOT_RUN`(C). F0 경로는 `OUTSIDE_SUPPORTED_SCOPE`(현재 구현) |
| R2 | `evs.reg` fixed(writer 4/4, 시작 상태). SB AppId 위치는 F0에서 `open{effect_unsupported}`, 계측 경로에서 직접 포획(§1.4) | AppID 인자 직접 포획(DC), 등록, 생성 요청 caller | 실행됨. F0 pilot3 1회, 계측 G-I(STATUS_20261027 §3.1). 두 기록의 시작 상태가 다르다 | 프로필(`appreq` `cfe_evs.h:169–172`), `--core-request-caller` | 위반 24 `OBSERVED_VIOLATION`(DC). SB 자기 전송 5 READY(DC + SO, A-OWNER). 다른 task 240(F0)·243(G-I) READY(DC + ON). 거부(`CFE_EVS_APP_ILLEGAL_APP_ID`)는 별도 결과이고 앱 종료는 없다(results_pilot_20261025 §1 R2-1–R2-4; results_instr_pilot_20261026 §8.1) | `RAN_DETECTED`(natural pilot). R2는 기반 검증 사례이고 기여의 증거가 아니다(ADR-057). 24는 한 경로의 호출 수이며 버그 24개가 아니다 |

## 4. 능력 분해

### 4.1 네 층: 관측, 모델링, 판정, 제어·재현

기존 도구 칸은 MX §2.2·§4.3의 압축이며 모두 미실행 분석이다. Kairos 칸의 꼬리표: **[기록]** 기록된 실행으로 보임(인용), **[구현]** 구현·설계만 있고 사례 실행 기록 없음, **[단위]** 단위 시험만.

| 방법 | 관측 | 모델링 | 판정 | 제어·재현 |
|---|---|---|---|---|
| RacePro | syscall, signal, 커널 객체 접근 순서, 공유 메모리 page ownership(재생용)(§4 pp.356–357) | 커널 객체 micro-op, HB 4종 + 프로그램 순서, 벡터 시계, 경로별 위치(§5) | 후보(논문: load-store·wait-wakeups·wakeup-waits, 코드: RESOURCE·EXIT-WAIT·SIGNAL). checker가 녹화 때 없던 실패를 볼 때 Harmful로 보고(p.363) | 앵커 syscall 순서를 바꾼 branch → replay → go-live. 결정적 재생(§6) |
| SimRacer | 자체는 불명 | 자체는 불명 | race와 fault 구분(초록) | Phase 1 예측 + Phase 2 실행 확인(초록). 결정적 뒤집기인지 불명 |
| SIMEXPLORER | SR 접근(메모리는 Simics 중단점, 지정 전제), syscall 23종, 동기화 사건(p.8, p.14) | 같은 SR, 하나 이상 쓰기, HB 없는 쌍. lock·semaphore는 HB 밖(p.9–10) | PRaceSet / RaceSet / oracle fault(p.19) | 프로세스 멈춤·재개의 사건 기준 제어, 한 실행에 한 쌍, 뒤집을 수 없으면 race로 세지 않음, 기다림 교착 해소(p.10–14) |
| RRF | Scribe syscall·signal·resource lock(p.1–4) | 엔지니어가 지정한 자원으로 거른 syscall 쌍 × 두 순서 × 네 패턴(p.5–7) | oracle(확장 가능), targeted/incidental/new(p.6, p.8–10) | sleep으로 순서 강제, 실제 실행으로 확인(p.5–6) |
| DESCRY | 실패 로그 + syscall 사건(p.3, p.5) | POG, 관계 5종(p.3, p.7) | failure point 도달, 목표 완화(p.3–4) | 예측 schedule + Simics replay, infeasible 건너뜀, KLEE 입력(p.4–7) |
| SimEvo | syscall R/W + 동기화 5종(p.3, p.6) | 변경 영향(정적 CIA + 동적), Def-Use 쌍(p.4–6) | 오라클 불일치 결함, 커버리지(p.4, p.9) | offline schedule + Simics 재생(p.6–7) |
| ReDPro | PIN run-time monitoring(사건 불명) | potential race pair(생성 방식 불명) | failure 재생성(정의 불명) | PIN으로 순서 제어(초록) |
| SysPro | 실행 사건 수집 없음. 보고서·소스(p.16, p.20–21) | 보고서 syscall 쌍 빈도와 소스 호출 순서로 위치 순위(p.14–19) | 재현 판정 방법 서술 없음(p.24) | 순위대로 PIN sleep 삽입, 재실행(p.19–21) |
| **Kairos F0** | uprobe·tracefs로 재계측하지 않은 기준 빌드의 호출·인자·반환·site 사건과 정체성(task → 소유 앱) **[기록]** R2 pilot3, C1 smoke3·hold1·confirm1·confirm2(results_pilot_20261025 §0–§1, §5). 32비트 실행 파일은 받지 않음(§1.4) | Φ 프로필(자원 키, writer, 별칭, 시작 상태, 수명, 호출 역할, 요구·허용), 정적 닫힘 B **[기록]** C1·R2. C5는 프로필 없음 | `kz replay`: 판정·요약, 근거 종류 DC·SO·RD·ON, 신뢰 가정, 관계(P10), 민감도 읽기 **[기록]** C1·R2 | `kz predict`(C1 한정 명시적 상태 탐색, 수정 전 89 상태·후보 3, 수정 후 8,393 상태·후보 13) **[기록]** C1. `kz hold`(ptrace + debug register, `kzp-hold 1`) **[기록]** C1 제어 실행. R2 제어 실행은 없음 |
| **Kairos 계측** | `kzgcc` 0.2 + `kzrt` 0.3.0. 호출 인자와 성공 경로 out 값 store의 직접 포획 **[기록]** C1 8회(네 빌드 × tx·rx), R2(results_instr_pilot_20261026 §0, §8). 6.4.x 32비트는 site 결합까지 **[구현]**(c5_execution_basis §8.2) | 같은 Φ + B′ **[기록]** C1·R2 | 같은 분석기. `nocode` 읽기에서 반환 코드 없이 서는 판정: C1 수정 후 성공 조회 8건, R2 24·5 **[기록]**(같은 문서 §8.3). 기록된 제어 순서 근거(P11) **[기록]** C1 G-I-H(results_instr_gate_20261026 §3) | 협조 게이트 G-I-H **[기록]** C1 계획 9개 × 4 인스턴스 = 36회, 실제 제어가 F0와 36/36 같음(같은 문서 §1). 실행 경로 대조이며 H3에 넣지 않음(PD28 §2 항목 5). C1 게이트는 POSIX adapter를 썼고, OSAL 객체를 쓰는 adapter는 **[단위]**(같은 문서 §0, §7). F0 보류 기록을 판정 입력에 잇는 일은 진행 중 **[구현]**(STATUS_20261027 §6) |

**Kairos가 보이지 않은 것**(같은 꼬리표 규칙): 자연 실행에서의 순서 빈도·발생 가능성, 관측 영향(hook 비용·군 비교), R2·C5의 제어, C2·C3·C5 적용, 새 실패 기전(STATUS_20261027 §5; results_instr_gate_20261026 §7). 프로필·관측 지점·요구 조건의 상당 부분은 소스를 읽고 사람이 썼다(PD28 머리말의 근거 문서 AD §5.2, "필요한 조건을 자동으로 찾는가" 행).

### 4.2 H4 차이 후보와 확인 조건

2026-10-22 PI 확인 범위: 탐지의 핵심 알고리즘(관측 판정, 제약·탐색 기반 예측, 제어 실행 확인)은 기존 기법을 적용한다. 새로 도출할 대상은 정체성 매핑, 자원, 준비 상태 모델, 부분 관측 판정 기준이다(`PI_DECISIONS_RESOLVED_20261022.md` §1 끝) [D]. 그래서 H4는 아래 행 1·2·3·4에서 시험한다. 행 5는 차이로 쓰지 않는다.

| # | 차이 후보 (로드맵 §10.5 행) | 비슷한 일을 이미 하는 기존 방법 (MX §4.3) | Kairos 현재 근거 | H4를 확인하려면 필요한 결과 |
|---|---|---|---|---|
| 1 | cFS 내부 상태·값 전달의 관측과 소스 연결 (행 1) | SIMEXPLORER의 메모리 SR 관측(지정 전제). RacePro의 page ownership 기록(후보에는 안 씀). ReDPro의 PIN(계측 대상 불명) | R2 AppID 인자 직접 포획 24(DC), 계측 경로의 SB AppId store 포획 **[기록]** | 원 도구의 `ORIGINAL-RUN` 또는 `DOCUMENTED-SETUP-RUN`에서 같은 사례·같은 사건이 (b) 또는 (c)에 이른 칸(MX §4.1의 "확인된 부족"). `EXTENSION`으로 메워지면 주장은 "원 도구의 입력 모델 밖"까지다(H-RP-1) |
| 2 | 이름·ID·등록 정보와 task → AppId의 정체성 결합 (행 2) | RacePro의 경로 이름 → 부모 inode 위치, pid 가상화. RRF의 엔지니어 지정 자원 이름 | C1 이름·id 포획(`0x30002`, `0x30003`), task → 소유 앱 `confirmed`, 앱 간 관계(CF–BP, 코어–앱) **[기록]**. C5의 생성 뒤 정체성 결합은 **[구현]** 전 | 위와 같음. C5에서 등록 전 관측과 사후 귀속이 성립하는지가 먼저다(PD28 §5 항목 7) |
| 3 | 허용 오류·회복과 최종 결과의 분리 (행 4) | 사용자 checker·oracle로 구분할 수 있을 수 있다(RacePro p.363, SIMEXPLORER p.18, RRF oracle 확장). 능력 부족보다 입력 형식 부재에 가깝다(MX §4.4 행 4) | C1 `retry_until_ready(25)` 감시, TOLERATED·VIOLATING, 결과 열 분리 **[기록]** | 같은 정보를 checker로 받은 원 도구 실행과의 대조. 차이가 입력 형식뿐이면 H4 주장이 아니다 |
| 4 | 순서 근거 종류·전제·신뢰 범위의 보고 (행 6) | RacePro의 divergence 거름(부분, H-RP-4). DESCRY의 infeasible 건너뜀 | 근거 종류 DC·SO·RD·ON·제어 순서, 신뢰 가정, 민감도 읽기 **[기록]** | R2 순서가 고정인지 스케줄 의존인지가 먼저 정해져야 한다(MX §4.4 행 6). 재현·회귀가 목적인 도구에서는 목적 차이일 수 있다 |
| 5 | (차이로 쓰지 않음) 관련 작업·자원으로 좁힌 예측·탐색과 실행 확인 (행 5) | RacePro, SimRacer 계열, DESCRY, SimEvo가 이미 한다. RRF는 열거와 sleep 확인, SysPro는 순위 탐색과 재실행, ReDPro는 초록 수준 | `kz predict` + 보류·게이트 **[기록]** C1 | 쓰려면 앵커 종류·탐색 범위로 한정한다(MX §4.2 행 5). 제어 방식의 차이(사건 기반 보류 대 sleep)는 결핍이 아니다(H-RRF-4) |
| 6 | 시작 동기화와 앱 고유 초기화 완료의 구분 (행 3) | — | C2 미착수 | C2 자료 확보 전에는 H4 행으로 쓰지 않는다(MX §4.4 행 3) |

## 5. 다음 비교 절차 [제안]

### 5.1 다음에 실행할 수 있는 것과 필요한 것

순서는 의존 관계만 나타낸다. 일정과 노력은 정하지 않는다.

| 순서 | 방법·경로 | 대상 | 필요한 것 | 막는 결정·자료 | 결과 라벨 |
|---:|---|---|---|---|---|
| 0 | Kairos C5 적용 | C5 | 6.4.x가 사례 경로까지 실행되는 조건(c5_execution_basis §3.5의 선택지), F0 ELF32 backend 또는 계측 경로, 회수 도구 | PD28 §5 항목 7. CS §7 항목 4의 PI 결정 | Kairos의 C5 실행. 기존 도구 비교의 전제다. 사례 경로가 실행되지 않으면 어느 방법의 미탐지도 (a)뿐이다 |
| 1 | RacePro 문서화된 설정 실행 | RacePro 자체 예제 `racepro/tests/tests.list` 11개(RacePro.md §4.1) | 별도 머신(KVM 가능 또는 bare metal), i386 Linux 2.6.35 + Scribe 커널(번들 `kernel_config`), Python 2·Cython·networkx 1.x·pygraphviz, unionfs | MX §5.4 항목 5(P-3). userland 선택은 `unspecified_by_reference`(§6 N1) | `DOCUMENTED-SETUP-RUN`. 사례 결과가 아니다 |
| 2 | RacePro × C5 | C5(6.4.1 수정 전, 6.4.2 수정 후) | 1번 기계. cFS 쪽은 6.4.x readme의 i386 빌드 host userland 후보: 6.4.1 readme L82–91의 CentOS 6.5-x86·Ubuntu 12.04 LTS x86, 6.4.2 readme L129의 CentOS 6.5 i386(c5_execution_basis §1.3) [F]. 그 userland가 2.6.35-scribe 커널에서 도는지 [U]. readme 해법 "root로 실행"(`msg_max`, RT)이 그 기계에서 성립하는지 [U]. checker(c5_execution_basis §4 O1–O3 중 무엇을 볼지)는 첫 출력 전에 동결 | 0번, 1번 | `ORIGINAL-RUN` 후보. host 커널 교체는 cFS 쪽 deviation으로 기록 |
| 3 | RacePro × R2·C1 | R2, C1 | v7.0.1 번들과 BP 공개 구성의 i386 재빌드. 레퍼런스가 없다 | 1번 | `DEVIATION-RUN` |
| 4 | SysPro | C1, C3, C5, R2 | ReproSys 접근, PIN·srcML·NLTK·Gensim·SPMF의 판(`unspecified_by_reference`), sleep 길이와 재현 판정(서술 없음) | MX §5.4 항목 3·10 | `ORIGINAL-RUN`(입력은 보고서 본문). cFS 앞단을 붙이면 `EXTENSION` |
| 5 | RRF | 1번 기계를 함께 쓸 수 있을 여지 [I]. RRF가 쓴 Scribe 판이 RacePro 저장소의 판과 같다는 확인은 없다(RRF.md R2) | 구현, 정적 분석 도구의 이름, oracle 저장소 | 항목 3 | — |
| 6 | SimRacer, SIMEXPLORER, SimEvo, DESCRY | — | 구현. Simics(EULA, 벤더 문서의 설치 약 10 GB). DESCRY는 KLEE(LLVM 3.4)와 실패 로그, SimEvo는 CodeSurfer | 항목 3·4·6 | — |
| 7 | ReDPro | — | 전문부터 | 항목 8·15·17 | — |
| 8 | 재구현(K2) | 사례별 | 논문에 서술된 부분만 만든다. 서술되지 않은 선택은 각각 deviation 또는 `unspecified_by_reference`(MX §5.3 S3). F1에서 동결, 논문 예제로 golden test, 구현자와 그가 본 사례를 기록(SPEC §8.3) | 항목 4 | `REIMPLEMENTATION`. 별도 표 |

### 5.2 공정 비교의 조건

1. **같은 사례 판과 입력.** 사례의 소스 판(ledger §1b, `plan/case_basis_register.md`), 빌드 명령, 시작 상태(R2는 PD28 Q3의 Power-On 공통 절차), 시작 스크립트·앱 집합을 같게 한다. 다른 것은 행마다 deviation으로 적는다(CONDITIONS_POLICY).
2. **같은 환경 또는 기록된 차이.** 기존 도구가 다른 실행 환경을 요구하면(예: RacePro의 i386 2.6.35-scribe) 두 길이 있다. (a) 같은 기계에서 Kairos도 실행한다. (b) 환경 차이를 deviation으로 적고 같은 사례 판의 순서위반 판정만 대조한다. 빈도·분포는 비교하지 않는다. F0 경로는 uprobe·tracefs를 쓴다. 그 커널에서의 가용성은 이 문서가 확인하지 않았다 [U](§6 N4).
3. **같은 판정 단위.** 단위는 순서위반이다. 네 항목(위반 대상, 관련 task, 공유자원, 판정 근거)으로 대응을 확인한다(§1.3, PD28 Q4). 최종 실패·피해는 별도 열이다(PD28 §2 항목 6). 방법 자신의 실패 기준에 따른 결과를 함께 적는다(SPEC §8.3).
4. **받은 정보를 적는다.** Kairos가 받은 것(Φ 프로필, site, 요구·허용 기록)과 기존 도구가 받은 것(checker·oracle, SR 지정, 자원 이름, 보고서, 소스)을 행마다 적는다. 기존 도구에 cFS 고유 사건·자원을 더 주면 `EXTENSION`이다. Kairos의 사례별 프로필은 수동 입력으로 표시한다(로드맵 §11.5 "수동 입력과 자동 추출을 구분").
5. **같은 실행 라벨끼리.** 자연 실행과 제어 실행을 나눠 적는다. 제어 실행의 결과는 빈도가 아니다. 같은 제어 사실을 주지 않은 대조를 수집·제어 경로의 우월로 읽지 않는다(PD28 §3 "83/86"의 원칙).
6. **동결.** 사례마다 첫 출력 전에 기대표를 동결한다. 비교 전에 분석기·cFS 규칙·관측 계획·판정 기준·checker·재구현을 동결한다(로드맵 §11.3, `Architecture/07_evaluation.md` §0). 이후 고친 것은 변경 이유와 함께 따로 표시한다.
7. **형상 연결.** 행마다 도구 판(커밋), 논문 판(쪽 기준), 입력 구성, deviation 목록(MX §5.3 S8), 실행마다의 커널, 해시를 적는다(`CONFIG_MANAGEMENT.md` §1).
8. **수치.** 사례 수, 순서 구조 수, 계획 수, 실행 수, 호출 수를 따로 적는다(PD28 §3). 도구 사이에 점수·순위·짝지은 검정을 두지 않는다(SPEC §8.3). 반복 수·임계값·시간 한도는 비교를 설계할 때 측정 질문마다 정하며 그 전에는 `unspecified_by_reference`다(PD28 Q4). 도구 매개변수(RRF의 `ST`·`r`, DESCRY의 `N`·`L_max`, SysPro의 sleep 길이)의 논문 값은 논문 사례의 값이다. cFS 조건으로 가져오지 않는다(도구별 파일 §4.1). 레퍼런스에 있는 상수나 사건 기반 제어만 쓴다(CONDITIONS_POLICY).
9. **평가 자료.** 실제 cFS 사례와 1차 출처를 쓴다. 작은 합성 입력은 도구 시험으로만 쓰고 사례 수에 넣지 않는다(PD28 §3). RacePro의 자체 예제(5.1의 1번)도 사례 결과가 아니다.

### 5.3 쓰지 않을 주장

- "도구 X가 사례 Y를 탐지(재현)하지 못한다." 실행하지 않은 도구에 쓰지 않는다. `NOT_ATTEMPTED`, `COULD_NOT_RUN`, `OUTSIDE_SUPPORTED_SCOPE`를 미탐지·false negative로 세지 않는다(D4, ADR-058, `availability.md` §5.4).
- 탐지율, false negative 수, 성능 비교, 도구 순위.
- "기존 process-level 기법은 순서위반을 다루지 못한다"(`availability.md` §5.4). RacePro는 순서위반을 겨냥해 설계됐다(p.355). RRF의 race 정의는 order violation을 포함한다(p.2).
- "RacePro는 공유 메모리를 기록하지 못한다." 기록하되 HB와 후보에 쓰지 않는다(p.357–358).
- "관측된 실행만 다루는 기존 방법에 비해 Kairos는 예측·탐색과 실행 확인을 한다." 근거가 없다(MX §0.2 항목 5, §4.4 행 5).
- 확인하지 않은 출력(`REPORTED`)을 사례 탐지로 쓰는 것(§1.3).
- 재구현·확장의 결과를 원 도구의 결과로 쓰는 것. Kairos ablation이나 내부 F0·G-I·G-I-H 대조를 기존 도구 결과로 쓰는 것(MX §5.3 S2·S4·S5, PD28 §3).
- 읽지 않은 도구(SimRacer 본문, ReDPro 전문)의 능력이나 범위를 초록·검색 발췌로 말하는 것(SPEC §6.4 항목 54).
- `availability.md` §5.4 "May use"의 둘째 문장("none of the five case dependencies is an event in that model")을 도구 구분 없이 쓰는 것. SIMEXPLORER에는 쓸 수 없고 SimRacer에는 단정할 수 없다(MX §2.5 항목 8, §5.5).
- C2의 역할 재구성 결과를 원 비공개 시스템의 재현으로 쓰는 것(P-C2). C3의 파생 구성 실행을 원 사례의 재현으로 쓰는 것(PD28 §4).
- 강제 보류로 만든 소진(E-06, `timeout-exercised`)을 새 운영 결함이나 자연 빈도로 쓰는 것(STATUS_20261027 §5).

### 5.4 비교 결과표의 열 (로드맵 §12 작업 14의 형식 후보)

| 열 | 내용 |
|---|---|
| 방법·판 | 도구 커밋 또는 Kairos 분석기 커밋·프로필 해시·읽기 옵션 |
| 논문 판 | 쪽 기준 판, 취득 경로 표시(MX §1.1) |
| 사례·판 | 소스 판, 빌드, 시작 상태, 시작 스크립트 |
| 환경 | 기계, 커널(실행마다), userland |
| 실행 라벨 | `ORIGINAL-RUN` … `REIMPLEMENTATION`, natural / controlled |
| 받은 입력 | checker·oracle·SR 지정·자원 이름·보고서·소스·프로필 |
| 더한 사건·확장 | 없음, 또는 무엇을 더했는가 |
| 범주와 원인 | §1.1 |
| 대응 확인 | §1.3의 네 항목 각각 |
| 방법 자신의 실패 기준 결과 | checker·oracle·failure point·재현 판정 |
| 최종 실패·피해 | 별도 |
| deviation 목록 | CONDITIONS_POLICY |
| 해시 | `CONFIG_MANAGEMENT.md` §1 |

## 6. 남은 것과 PI 판단 후보 (이 문서는 정하지 않았다)

- MX §5.4의 열린 항목은 그대로다. 이 문서에 직접 걸린 것: 항목 1(같은 프로세스 안 task 쌍의 `INAPPLICABLE`), 3(저자 연락), 4(재구현·확장), 5(RacePro 별도 머신), 6(Simics 설치), 14(C2 입력 미충족의 `INAPPLICABLE`).
- **N1. RacePro 기계의 userland.** RacePro 쪽은 `unspecified_by_reference`다. 6.4.x readme의 i386 빌드 host를 쓰면 cFS 쪽 레퍼런스와 함께 맞출 후보가 된다(§5.1 2번) [I]. 선택은 PI 결정이다.
- **N2. RacePro checker의 범위.** checker가 cFS 내부 반환값이나 상태를 보게 하는 것이 논문이 허용한 사용자 checker(p.363)인지, cFS 쪽 관측을 더한 `EXTENSION`인지. R2는 출력이 없어 이 판단이 결과의 라벨을 정한다.
- **N3. 문서 정합(lead 확인).** SPEC §8.3 "Unrun tools"는 D4의 문헌·코드 상태를 질문에 따라 `OBSERVATION_GAP`·`ANALYSIS_GAP`에도 대응시킨다. MX §3.1·§4.1은 두 GAP 상태를 실행 뒤로 한정한다. 이 문서는 MX를 따랐다(§1.2). 두 문서는 고치지 않았다.
- **N4. 환경이 다를 때의 Kairos 실행.** §5.2 항목 2의 (a)·(b) 가운데 무엇을 쓸지. (a)라면 F0의 uprobe·tracefs와 계측 경로의 런타임이 그 기계에서 도는지부터 확인해야 한다 [U].
- 사례 이슈 댓글(MX §5.2 "공통" 행): DESCRY의 실패 로그 입력과 SysPro의 보고서 입력 판단이 이 경로에 걸려 있다.
- MX §5.5가 적은 다른 문서의 정정(예: `availability.md` §1·§3·§5.4의 낡은 서술)은 아직 반영되지 않았다. 이 문서는 그 문장을 근거로 쓰지 않았다.

## 7. 출처와 읽은 판

[M] 2026-10-28, 저장소 HEAD `9d7eccb`에서 읽은 판(sha256 앞 16자):

| 파일 | sha256 앞 16자 | 쓴 곳 |
|---|---|---|
| `plan/process_race_method_matrix.md` | `a21556c09ec9199c` | §0.1 표기, §1.1–§1.3 근거·도구 칸·사건 대응, §3.1–§3.5 상태, §4 부족 표, §5.3–§5.5 |
| `plan/methods/RacePro.md` | `02f921ce132c88c8` | §2.1, §2.5 K1–K7, §4.1–§4.3 |
| `plan/methods/SimRacer.md` | `0a8ab54390c1ccab` | §4.1 |
| `plan/methods/SIMEXPLORER.md` | `d32a569fce01f83b` | §4.1 |
| `plan/methods/RRF.md` | `fb39c1f05d938589` | §4.1 |
| `plan/methods/DESCRY.md` | `f1f523d3d95e1336` | §2.3, §4.1–§4.3 |
| `plan/methods/SimEvo.md` | `6de44b43e56a40e5` | §4.1 |
| `plan/methods/ReDPro.md` | `deae34c4432499a0` | §4.1 |
| `plan/methods/SysPro.md` | `bad2d37b6f566006` | §4.1 |
| `baselines/availability.md` | `46d94f30e02b988f` | 머리말, §1, §5.1–§5.4 |
| `plan/ROADMAP_20261015.md` | `59c495b120423dca` | §10, §11.3–§11.5, §12 |
| `STATUS_20261027.md` | `a27ab647d9bd2f35` | §1–§6 |
| `plan/results_pilot_20261025.md` | `be4138d7660dd581` | §0–§3, §5.4, §5.6 |
| `plan/results_instr_pilot_20261026.md` | `05419140f1ce22d7` | §0, §6, §8 |
| `plan/results_instr_gate_20261026.md` | `75e550fa127b2426` | §0, §1, §7 |
| `plan/c5_execution_basis_20261028.md` | `9e72d74c376cd7e2` | §0, §1.3, §3.5, §4, §8 |
| `Architecture/spec/PI_DECISIONS_RESOLVED_20261028.md` | `2b65dd6d14e50336` | 결정 Q1–Q4, §2–§5 |
| `Architecture/spec/SPEC_FINAL.md` | `ef5e3c9c7a3d8be5` | §8.3 |
| `cases/ledger.md` | `e7520cde245544d6` | §1a, §1b |

- 도구별 파일 여덟 개의 해시는 MX §8.1의 2026-10-16판과 같다. 그래서 MX의 칸을 그대로 인용했다.
- 함께 읽은 것: `CONDITIONS_POLICY.md`, `CONFIG_MANAGEMENT.md`, `Architecture/spec/PI_DECISIONS_RESOLVED_20261022.md` §1, `Architecture/07_evaluation.md` §4, `Architecture/10_decisions_adr.md` ADR-046·047·058, `kairos/docs/predict.md` §1, `kairos/probe/anchor/elf_image.cpp` L50–L62.
- 원출처(논문·도구 코드)는 이 문서에서 다시 열지 않았다. 쪽 번호와 코드 위치는 도구별 파일의 같은 칸을 거친다.
